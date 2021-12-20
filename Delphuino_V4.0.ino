
// Multi-stage boost controller for 35A MAC Valve and Spartan 2 OEM
// Project Delphuino

//Parts used:
// Arduino Mega 2560 Pro Mini
// MPX4250AP 2.5 bar Sensor/Stock GM 3 bar MAP
// 35A-ACA-DDAA-1BA MAC Valve Solenoid (3 port)W/ flyback diode
// Spartan2 OEM Wideband controller (Non I2C version, Linear output version)
// IRF520N MOSFET (2X)
// 10K restor for pulldown on switch
// Push button switch and/or Pot
// 12V to 5V adjustable voltage regulator
// I2C 1602 LCD using 2 pullup resistors (2.2K for SDA/SCL)

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Start testing PID control for boost set
//#include <PID_v1.h>
//double Setpoint; //PID setpoint
//double MAPOutput; //MAP Value
//double solenoidPin; //MAC Valve control


const int ledPin = 13; //Port 13/onboard for LED pin
const int buttonPin = 11; // Button on pin 11
int buttonState = 0; //State of button
int lastState = 0; //last state of button
byte count = 0; //Button press counter
const int Dip_1 = 8; //Dip switch 1 is set to Digital pin 8
const int Dip_2 = 6; //Dip switch 2 is set to Digital pin 6


//Duty Cycle Settings
int DutyCycle0;
int DutyCycle1;
int DutyCycle2;
int DutyCycle3;
int DutyCycle4;
int DutyCycle5;
int DutyCycle6;


const int MAP = A2; //Map sensor input on A2
const int solenoidPin = 3; //MAC valve on pin 3 (PWM)
const int wideband = A1; //Set Spartan 2 OEM sensor to Analog pin 1
const int wideband2 = A3; //Set Spartan 2 OEM second sensor too Analog pin 3


void setup() {
  // initialize pins

  pinMode (ledPin, OUTPUT);
  pinMode (buttonPin, INPUT);
  pinMode (MAP, INPUT);
  pinMode (wideband, INPUT);
  pinMode (Dip_1, INPUT);
  pinMode (Dip_2, INPUT);

  // set PWM frequency of 30.64 Hz
TCCR2B = TCCR2B & B11111000 | B00000111;
pinMode(solenoidPin, OUTPUT);
 
 //Setup LCD rows and columns
 lcd.begin();
 
 Serial.begin(9600);

 //Duty Cycle integers from 0-255 (0-100% DC). You can add to this for more levels. Be a psycho and make a level for every percentage of duty cycle...
 DutyCycle0 = 0; //wastegate
 DutyCycle1 = 51; //20%
 DutyCycle2 = 77; //30%
 DutyCycle3 = 102; //40%
 DutyCycle4 = 128; //50%
 DutyCycle5 = 153; //60%
 DutyCycle6 = 178; //70%
 
 }

void loop() 
 

{if (Dip_1 == LOW);

{
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
  
  if(count < 6)
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
// Stage 4
if (count == 4 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle4);
}
// Stage 5
if (count == 5 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle5);
}
// Stage 6
if (count == 6 && MAPOutput > 2){
  analogWrite (solenoidPin, DutyCycle6);
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

int widebandValue2 = analogRead(wideband2);//Reads raw voltage of wideband pin
  float widebandvoltage2 = widebandValue2 * (5.0 / 1023.0);
  float widebandAFR2 = (widebandvoltage2 * 2.375) + 7.3125;
  Serial.println(widebandAFR2); 

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

//if (Dip_2 == LOW)
// setup code for Potentiameter control and Duty cycle (take 0-1023 to output integer of 0-255)
//This will also be setup for different gauge layout

//if (Dip_1 && Dip_2 == LOW)
// setup code for PID control based on MAP
//This will also be setup for different gauge layout


}
