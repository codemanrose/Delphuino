// Multi-stage boost controller for 35A MAC Valve
// Project Delphuino

//Parts used:
// Arduino Nano
// MPX4250AP 2.5 bar Sensor
// 35A-ACA-DDAA-1BA MAC Valve Solenoid (3 port)
// Spartan2 OEM Wideband controller (Non I2C version, Linear output version)
// Dual MOSFET PWM trigger switch
// 10K restor for pulldown on switch
// Push button switch
// 12V to 5V adjustable voltage regulator
// Diode between MAC Valve power leads to prevent any flyback
// I2C 1602 LCD

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x23, 16, 2);

const int ledPin = 13; //Port 13/onboard for LED pin
const int buttonPin = 11; // Button on pin 11
int buttonState = 0; //State of button
int lastState = 0; //last state of button
byte count = 0; //Button press counter


//Duty Cycle Settings
int DutyCycle0;
int DutyCycle1;
int DutyCycle2;
int DutyCycle3;


const int MAP = A2; //Map sensor input on A2
const int solenoidPin = 3; //MAC valve on pin 3 (PWM)
const int wideband = A1; //Set Spartan OEM 2 sensor to Analog pin 1


void setup() {
  // initialize pins

  pinMode (ledPin, OUTPUT);
  pinMode (buttonPin, INPUT);
  pinMode (MAP, INPUT);
  pinMode (wideband, INPUT);

  // set PWM frequency of 30.64 Hz
TCCR2B = TCCR2B & B11111000 | B00000111;
pinMode(solenoidPin, OUTPUT);
 
 //Setup LCD rows and columns
 lcd.begin();
 
 Serial.begin(9600);

 //Duty Cycle integers from 0-255 (0-100% DC). You can add to this for more levels. Be a psycho and make a level for every percentage of duty cycle...
 DutyCycle0 = 0; //wastegate
 DutyCycle1 = 80; //30%
 DutyCycle2 = 115; //45%
 DutyCycle3 = 153; //60%
 
 }

void loop() {

//Reading button press to digital pin
  buttonState = digitalRead(buttonPin);


//Setup Math for MAP sensor read
int MAPValue = analogRead(MAP);//Reads the MAP sensor voltage raw value on Analog port 2
  float MAPOutput = (((MAPValue/(float)1023-0.04)/.00369)*.145)-15.2; // MPX4250 = -16.3 min, 23.0 max, See comment below on setting up your locations elevation.

  //For code directly above I would like to eventually make it calibrate absolute atmospheric pressure before doing math.
//For example: My location is 1000ft above sea level. This is about 14.2 absolute atmospheric pressure
//So to get 0psi at rest I need to take 14.7 - 14.2 = 0.5 psi difference. If I add 0.5 + 14.7 at the end of the MAPOutput math it will set base pressure to 0 for my location.
//Idea is to run a calibration check based on location and then do the math using set calibration. I assume this would be done in Setup before Loop.
//For now make your change based on your location or maybe try a calibration check code you can build and see if it works. 



  
  if (buttonState == HIGH ) {
    digitalWrite(ledPin, HIGH);

  }
  else {
    digitalWrite(ledPin, LOW);
  }

// check if the pushbutton is pressed. If it is, then the buttonState is HIGH:
if (buttonState == HIGH && lastState == 0)
{
  
  if(count < 3)
    count += 1;
  else
    count = 0;
  //setting up counter output to LCD and serial monitor
  Serial.println(count);
  lcd.setCursor(4,1);
  lcd.print(count);
 lastState = 1;
 
} 


else if(buttonState == LOW) {
  lastState = 0;
}
// Setting up button count to duty cycle for MAC Valve
// MAC valve will not start until there is 2 psi positive pressure to MAP sensor. It will function at the set stage over 2 psi and be off at 1 psi or less.
// Doing this will make it so that you do not get a vacuum leak at idle from the valve running. Under pressure is not an issue.

// Stage 0
if (count == 0){
  analogWrite (solenoidPin, DutyCycle0);
}
// Stage 1
if (count == 1 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle1);
}
// Stage 2
if (count == 2 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle2);
}
// Stage 3
if (count == 3 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle3);
}
// Stage 0 code if MAP sensor does not see at least 2 psi charge pressure. 
if (MAPOutput < 2){
  analogWrite (solenoidPin, DutyCycle0);
}



//setup LCD display
lcd.setCursor(0,0);
lcd.print("Boost PSI");



//output raw data for MAP sensor to serial monitor
Serial.println(MAPOutput);

//Setup Math for wideband sensor (Spartan OEM 2 Linear version)
int widebandValue = analogRead(wideband);//Reads raw voltage of wideband pin
  float widebandvoltage = widebandValue * (5.0 / 1023.0);
  float widebandAFR = (widebandvoltage * 2.375) + 7.3125;
  Serial.println(widebandAFR);

lcd.setCursor(11,0);
lcd.print("               "); //sets number back to zero if addtional number gets set on screen
lcd.setCursor(11,0);
lcd.print(MAPOutput,0);
lcd.setCursor(0,1);
lcd.print("LVL:");
lcd.setCursor(7,1);
lcd.print("AFR:");
lcd.setCursor(11,1);
lcd.print(widebandAFR);

}
