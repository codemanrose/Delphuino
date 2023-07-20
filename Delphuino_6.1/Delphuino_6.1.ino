
#include "LiquidCrystal_I2C.h" // Updated by Dale Follett 7-20-23 to look at locally attached library
#include "PID_v2.h" // PID library - updated by Dale Follett 7-20-23 to look at locally attached library
#include "j1850vpw.h" // This calls the locally tab attached J1850 library. Dale Follett 7-15-23

#define MAP A0 // Pin for MAP sensor (boost sensor)
#define solenoidPin1 9 // MAC Valve 1 to PWM pin 9
#define solenoidPin2 10 // MAC Valve 2 to PWM pin 10
#define SETPOINT_PRESSURE 6 // setpoint pressure in PSI
#define SAMPLE_TIME 1000 // sample time for PID in milliseconds
#define PWM_FREQUENCY 15 // PWM frequency for MAC Valve in Hz
#define wideband A7 // Wideband input signal to Aanolog port 7
#define coolant A3 // coolant pressure sensor 30 psi sensor to A3
#define fuel A2 // fuel pressure sensor 100 psi sensor to A2


LiquidCrystal_I2C lcd(0x27, 24, 4); //Address will differ dipending on your jumper set on the I2C board
const int buttonPin = 11; //The pin the pushbutton is on digital port 11
const int ledPin = 13; //Light up led on nano or led connected to pin 13

//Wideband variables
const float widebandVoltageMin = 0.0;     // Minimum voltage value from the wideband controller
const float widebandVoltageMax = 5.0;     // Maximum voltage value from the wideband controller
const float widebandAfrMin = 10.0;        // Minimum AFR value
const float widebandAfrMax = 20.0;        // Maximum AFR value

//MAP Sensor/Boost pressure variables
const float mapVoltageMin = 0.02;      // Minimum voltage value from the MAP sensor
const float mapVoltageMax = 4.9;       // Maximum voltage value from the MAP sensor
const float boostMin = 0.4;         // Minimum boost pressure value in PSI
const float boostMax = 29.01;       // Maximum boost pressure value in PSI

//Coolant pressure sensor (30psi)
const float coolantVoltageMin = 0.5; // Minimum voltage value from the coolant pressure sensor
const float coolantVoltageMax = 4.5; // Maximum voltage value from the coolant pressure sensor 
const float coolantPressureMin = 0;       // Minimum Coolant pressure value in PSI
const float coolantPressureMax = 30;      // Maximum Coolant pressure value in PSI

//Fuel Pressure sensor (100psi)
const float fuelVoltageMin = 0.5; // Minimum voltage value from the fuel pressure sensor
const float fuelVoltageMax = 4.5; // Maximum voltage value from the fuel pressure sensor 
const float fuelPressureMin = 0;       // Minimum fuel pressure value in PSI
const float fuelPressureMax = 100;      // Maximum fuel pressure value in PSI


float mapRange(float value, float inMin, float inMax, float outMin, float outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;



}

//Variabels for pushbutton Duty Cycle change on Mac Valve (0,1,2,3,4,5,6 - 0%/20%/30%/40%/50%/60%/70%)
// These wont change
int buttonState = 0; //State of button
int lastState = 0; //last state of button
byte count = 0; //Button press counter

//Duty Cycle Settings
int DutyCycle0;
int DutyCycle1;
int DutyCycle2;
int DutyCycle3;
int DutyCycle4;
int DutyCycle5;
int DutyCycle6;
int DutyCycle7;
int DutyCycle8;

float transistorOntime; // Info place holder - this becomes the transistor duty cycle

void setup() 
{
  pinMode(buttonPin, INPUT); // initialize button pin as an input
  pinMode(ledPin, OUTPUT); //initialize the LED as and output
  pinMode(MAP, INPUT); // initialize the MAP sensor as an input
  pinMode(wideband, INPUT); // initialize the wideband controller as an input
  pinMode(coolant, INPUT); // initialize coolant pressure sensor as an input
  pinMode(fuel, INPUT); // initialize fuel pressure sensor as an input

// set PWM frequency for pins D9 and D10 ONLY to 15 hz 10 bit. 
noInterrupts(); // Turn off all interrupts before setting up timer - Dale Follett 7-20-23
TCCR1A = 0b00000011; // 10bit
TCCR1B = 0b00001101; // x1024 fast pwm
interrupts(); // Turn the interrupts back on - Dale Follett 7-20-23

pinMode(solenoidPin1, OUTPUT);
pinMode(solenoidPin2, OUTPUT);

//Setup LCD's Number of columns and rows
lcd.begin();

lcd.setCursor(0, 1);
lcd.print("LVL:");
lcd.setCursor(7,1);
lcd.print("AFR:");
lcd.setCursor(13,3);
lcd.print("IAT:");
lcd.setCursor(17,3);
lcd.print("68F");


Serial.begin(9600);

 //Duty Cycle integers from 0-255 (0-100% DC). You can add to this for more levels. Be a psycho and make a level for every percentage of duty cycle...
 DutyCycle0 = 0; //wastegate
 DutyCycle1 = 51; //20%
 DutyCycle2 = 77; //30%
 DutyCycle3 = 102; //40%
 DutyCycle4 = 128; //50%
 DutyCycle5 = 153; //60%
 DutyCycle6 = 178; //70%
 DutyCycle7 = 204; //80%
 DutyCycle8 = 229; //90%
}

void loop() 
{
// Read the analog input from the MAP sensor
  int mapAnalogValue = analogRead(MAP);

  // Convert the MAP analog value to voltage
  float mapVoltage = map(mapAnalogValue, 0, 1023, mapVoltageMin * 1000, mapVoltageMax * 1000) / 1000.0;  // Convert to volts

  // Convert the voltage to boost pressure in PSI
  float boostPressure = mapRange(mapVoltage, mapVoltageMin, mapVoltageMax, boostMin, boostMax)- 13;

  //For code directly above I would like to eventually make it calibrate absolute atmospheric pressure before doing math.
//The GM 2 bar map sensor has a 9 KPA offest (adds to the value which is at 0V). So since we need initially subtract 14.7 PSI absolute pressure AT SEA LEVEL to give us what is "gauge pressure" we need to also add that offset of 9 KPA (1.30 psi) to that number.
//in this example it would be -14.7 + 1.30 = -13.32. So you take the mapRange variable/math above for boostPressure and -13.32 which will output the correct gauge pressure.
//With this said if you are higher elevation you will need to roughly findr you atmosperic pressure. I'm just under 1000 ft so with that math its comes out to -13 with the offset. This gets me to 0 psi at key on, engine off. 

/*
Dale adding thoughts. We should be able to do this map sensor calibration once we get the J1850 library going. Set it up to call a function on power on, if the engine is off (no RPM input),
then perform calibration, and if different than what is saved in EEPROM, update the EEPROM. If not different than eeprom, make no change. If powered on during engine
running, read eeprom value and use it.
*/


//Setup Math for wideband sensor (SLC Free 2)
  // Read the analog input from the wideband controller
  int widebandValue = analogRead(wideband);
  // Convert the analog value to voltage
  float widebandVoltage = map(widebandValue, 0, 1023, 0, 5000) / 1000.0;  // Convert to volts
  // Convert the voltage to AFR
  float widebandAfr = mapRange(widebandVoltage, widebandVoltageMin, widebandVoltageMax, widebandAfrMin, widebandAfrMax);

//Setup Math for coolant pressure sensor
 int coolantValue = analogRead(coolant); 
 // Convert the analog value to voltage
 float coolantVoltage = map(coolantValue, 0, 1023, coolantVoltageMin * 1000, coolantVoltageMax * 1000) / 1000.0;  // Convert to volts
// Convert the voltage to coolant pressure in PSI
  float coolantPressure = mapRange(coolantVoltage, coolantVoltageMin, coolantVoltageMax, coolantPressureMin, coolantPressureMax);

//Setup Math for fuel pressure sensor
  int fuelValue = analogRead(fuel); 
  //convert the analog value to voltage
  float fuelVoltage = map(fuelValue, 0, 1023, fuelVoltageMin * 1000, fuelVoltageMax * 1000) / 1000.0;  // Convert to volts
  //Convert the voltage to pressure in PSI
  float fuelPressure = mapRange(fuelVoltage, fuelVoltageMin, fuelVoltageMax, fuelPressureMin, fuelPressureMax);
  
  
  
  //Print Wideband voltage and AFR

  //Dales notes - added (F) bracket to serial message that do not change. This is to program these to the main program storage space compared to dynamic memory.
  //This free's up some dynamic memory being we have so little to work with on a nano.
  Serial.print(F("Wideband Voltage: "));
  Serial.print(widebandVoltage, 2);  // Print with 2 decimal places
  Serial.print(F(" V, Wideband AFR: "));
  Serial.println(widebandAfr, 2);    // Print with 2 decimal places

  //Print MAP voltage and and boost pressure
  Serial.print(F("MAP Voltage: "));
  Serial.print(mapVoltage, 2);  // Print with 2 decimal places
  Serial.print(F(" V, Boost Pressure: "));
  Serial.println(boostPressure, 2);    // Print with 2 decimal places  

  // Print the coolant voltage and pressure values
  Serial.print(F("Coolant Voltage: "));
  Serial.print(coolantVoltage, 2);  // Print with 2 decimal places
  Serial.print(F(" V, Coolant Pressure: "));
  Serial.println(coolantPressure, 2);    // Print with 2 decimal places

  //print the fuel voltage and pressure values
  Serial.print(F("Fuel Voltage: "));
  Serial.print(fuelVoltage, 2); // Print with 2 decimal places
  Serial.print(F(" V, Fuel Pressure: "));
  Serial.print(fuelPressure, 2);  // Print with 2 decimal places

  


//Reading button press to digital pin
  buttonState = digitalRead(buttonPin);

  
  if (buttonState == HIGH ) {
    digitalWrite(ledPin, HIGH);

  }
  else {
    digitalWrite(ledPin, LOW);
  }

// check if the pushbutton is pressed. If it is, then the buttonState is HIGH:
if (buttonState == HIGH && lastState == 0)
{
  
  if(count < 8)
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

// Stage 0: 0% DC
if (count == 0){
  analogWrite (solenoidPin1, DutyCycle0);
  analogWrite (solenoidPin2, DutyCycle0);
}
// Stage 1: 20% DC
if (count == 1 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle1);
  analogWrite (solenoidPin2, DutyCycle1);
}
// Stage 2: 30% DC
if (count == 2 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle2);
  analogWrite (solenoidPin2, DutyCycle2);
}
// Stage 3: 40% DC
if (count == 3 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle3);
  analogWrite (solenoidPin2, DutyCycle3);
}
// Stage 4: 50% DC
if (count == 4 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle4);
  analogWrite (solenoidPin2, DutyCycle4);
}
// Stage 5: 60% DC
if (count == 5 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle5);
  analogWrite (solenoidPin2, DutyCycle5);
}
// Stage 6: 70% DC
if (count == 6 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle6);
  analogWrite (solenoidPin2, DutyCycle6);
}
// Stage 7: 80% DC
if (count == 7 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle7);
  analogWrite (solenoidPin2, DutyCycle7);
}
// Stage 8: 90% DC
if (count == 8 && boostPressure > 2){
  analogWrite (solenoidPin1, DutyCycle8);
  analogWrite (solenoidPin2, DutyCycle8);
}
// Stage 0 code if MAP sensor does not see at least 2 psi charge pressure. 
if (boostPressure < 2){
  analogWrite (solenoidPin1, DutyCycle0);
  analogWrite (solenoidPin2, DutyCycle0);
}


//setup LCD display
lcd.setCursor(0,0);
lcd.print(F("Boost PSI"));


//output raw data for MAP sensor to serial monitor
Serial.println(boostPressure);

lcd.setCursor(11, 0);
lcd.print(F("               ")); //sets number back to zero if addtional number gets set on screen
lcd.setCursor(11, 0);
lcd.print(boostPressure,0);
lcd.setCursor(11,1);
lcd.print(widebandAfr, 1);
lcd.setCursor(0,3);
lcd.print(F("Cool Psi:"));
lcd.setCursor(9,3);
lcd.print(coolantPressure, 1);
lcd.setCursor(0, 2);
lcd.print(F("Fuel Pressure:"));
lcd.setCursor(14, 2);
lcd.print(fuelPressure, 1);
}
