/////////////////////////////////////////////
//                                         //
//         Authors: Carson Morris          //
//         Class: CAM8302E - 012           //
//       Ciena PWM Fan Controller          //
//           Date: 04-01-2021              //
//                                         //
/////////////////////////////////////////////
#include <avr/io.h>                             //AVR IO for Watchdog
#include <avr/wdt.h>                            //Watchdog Timer for Reset

#define Reset_AVR() wdt_enable(WDTO_30MS);      //Set Watchdog Timeout timer for 30ms                      

#define thermFront A0                          // Front Thermistor Pin
#define thermBack A1                           // Back Thermistor Pin
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07; //Thermistor Constants

#define fanB 5        // Variable for holding Back Fan Pin
#define fanF 3        // Variable for holding Front Fan Pin
String command;       // Temporary serial COM input variable

float tempFront;     // Front temperature variable
int setFront;        // Selected allowable front temperature variable
float tempBack;      // Back temperature variable
int setBack;         // Selected allowable back temperature variable
int outputF, outputB, fanSpeedF, fanSpeedB;  // Fan Control Variables
unsigned long timerF, timerB;  // Timers for Fan Speed Incrementing 

float setPoint = 22;   // Variable for Holding Temperature Wanted by System
float deadBand = 1.5;  // Variable for Allowable temperature Offset

int mode = 1;          // Set Starting Mode as Command Mode

void setup() {
  Serial.begin(9600);
  Serial.println("Ready For Commands:");

  pinMode(fanB, OUTPUT);        //Set Back Fan as Output Pin
  pinMode(fanF, OUTPUT);        //Set Front Fan as Output Pin
}

///////////////////////////////////////////////////// MAIN LOOP ///////////////////////////////////////////////////////
void loop() {
  if (mode == 1) { ///////////////// SERIAL INPUT MODE ///////////////////
    if (Serial.available()) {                  // If New Serial Input Data is Recieved
      command = Serial.readStringUntil('\n');  // Read Input Until New Line is Sent
      command.remove(command.length() - 1, 2); // Remove '\n' New Line From End of String

      if (isNumber(command)) {                 // If Command Sent is a Number
        if (command.charAt(0) == 'F') {        // If Command is Sent to Front Fan Temp
          command.remove(0, 1);                // Remove F From Start of Number
          tempFront = command.toFloat();       // Set tempFront to Command Temperature Input
          setFanFront();                       // Readjust Front Fan
        } else if (command.charAt(0) == 'B') { // If Command is Sent to Back Fan Temp
          command.remove(0, 1);                // Remove B From Start of Number
          tempBack = command.toFloat();        // Set tempBack to Command Temperature Input
          setFanBack();                        // Readjust Back Fan
        } else {
          Serial.println("Please Spesify Front or Back Temperature (F or B)");
        }
      } else {
        commands();             //Check if valid command
      }
    }
  } else { ///////////////// THERMISTOR READ MODE ///////////////////
    tempFront = calcTemp(thermFront);   // Set tempFront to Front Thermistor Reading
    setFanFront();                      // Readjust Front Fan
    tempBack = calcTemp(thermBack);     // Set tempBack to Back Thermistor Reading
    setFanBack();                       // Readjust Back Fan
  }
}

///////////////////////////////////////////////////// FUNCTIONS ///////////////////////////////////////////////////////
boolean isNumber(String input) {

  boolean decPt = false;     // Reset Decimal Point Flag Variable
  int idx = 0;               // Reset Indexing Variable

  if (input.charAt(0) == 'F' || input.charAt(0) == 'B') { // Check for F or B at start of Number
    idx = 1;                       // Set our first Index
    if (input.charAt(1) == '.') {  // Check for decimal as first number
      idx = 2;                     // Set our Second Index
      decPt = true;                // Set Decimal Point Flag True
    };
  }

  if (input.length() <= idx) return false;  // In No Number Detected Return False.

  for (int i = idx; i < input.length(); i++) { // Check for Every Letter In the String
    if (input.charAt(i) == '.') {  // If Decimal Point Is Detected
      if (decPt) return false;     // If Decimal Point Flag Active Return Not a Number
      else decPt = true;           // Otherwise set the Decimal Point Flag True
    } else if (input.charAt(i) < '0' || input.charAt(i) > '9') return false;
  }
  return true;
}

void commands() {                      // Check For Valid Commands
  if (command.equals("init")) {        // Run INITIZALIZE Command
    initialize();
  } else if (command.equals("help")) { // Run HELP LIST Command
    help();
  } else if (command.equals("sleep")) { // Run SLEEP Command
    sleep();
  } else if (command.equals("reboot")) {// Run REBOOT Command
    reboot();
  } else if (command.equals("modeT")) { // Run MODE CHANGE Command
    mode = 2;
  } else {
    Serial.println("Command not reccognized!");
  }
}

void initialize() {       // Turn Fans ON FULL
  analogWrite(fanF, 255); // Set FRONT Fan on Full
  analogWrite(fanB, 255); // Set BACK Fan on Full
}

void help() {            // Get current INFO from Microntroller
  Serial.println("Creators: Carson Morris & Daniel Becking");
  Serial.println("Adaptable Fan Microcontroller");
  Serial.println("Rev 0.8");
  Serial.println(" ");
  Serial.println("Commands:");
  Serial.println("help     ----  Program Information");
  Serial.println("modeT    ----  Sets unit to accept Thermistor Input");
  Serial.println("init     ----  Initialize Program (Run Fans)");
  Serial.println("sleep    ----  Terminate Program (Stop Fans)");
  Serial.println("reboot   ----  Reboot Microcontroller");
}

void sleep() {           // Turn OFF Fans
  analogWrite(fanF, 0);  // Set FRONT Fan OFF
  analogWrite(fanB, 0);  // Set BACK Fan OFF
}

void reboot() {    // Reboot the Microcontroller
  Reset_AVR();     // Run Watchdog Timeout
}

float calcTemp(unsigned char thermPin) { // Change Thermistor Data to Temperature

  float value = analogRead(thermPin);    //Read Current Reading From Selected Pin

  value = (1023 / value) - 1;   // (1023/ADC - 1) 
  value = 100000 * value;       // 10K / (1023/ADC - 1)
  value = log(value);
  value = (1.0 / (c1 + c2 * value + c3 * value * value)); // Integrate Constants
  value = value - 243.15;      //Convert to Celcius
  return (value);
}

void setFanFront() { // Set Front Fan Speed Based on Temperature
  if ((millis() - timerF) > 4000) {                 // If It Has Been at Least 4 Seconds Since Last Change
    if ((tempFront > (setPoint + (deadBand / 2))) && (outputF <= 95)) {
      if (outputF >= 30) {                          // If Output is Currently Greater Than or Equal to 30%
        outputF = outputF + 5;                      // Increase Front Fan Speed by 5%
      } else {
        outputF = 30;                                // Set Front Fan Output to 30%
      }
      timerF = millis();                            // Reset Front Fan Interval Timer
      Serial.print("FRONT Fan Speed Adjusted UP: ");
      Serial.println(outputF);                      // Print New Speed to Serial
    } else if ((tempFront <= (setPoint - (deadBand / 2)))) {
      if (outputF > 30) {                           // If Output is Currently Greater Than 30%
        outputF = outputF - 5;                      // Decrease Front Fan Speed by 5%
        Serial.print("FRONT Fan Speed Adjusted DOWN: ");
        Serial.println(outputF);                    // Print New Speed to Serial
      } else {
        outputF = 0;                                // Set Front Fan Output to OFF
      }
      timerF = millis();                            // Reset Front Fan Interval Timer
    }
    fanSpeedF = map(outputF, 30, 100, 76, 255);    // Map fanSpeedF from 0 to 100 to PWM Value
    analogWrite(fanF, fanSpeedF);                  // Set Front Fan to fanSpeedF
  }
}

void setFanBack() { // Set Front Fan Speed Based on Temperature
  if ((millis() - timerB) > 4000) {                 // If It Has Been at Least 4 Seconds Since Last Change
    if ((tempBack > (setPoint + (deadBand / 2))) && (outputB <= 95)) {
      if (outputB >= 30) {                          // If Output is Currently Greater Than or Equal to 30%
        outputB = outputB + 5;                      // Increase Back Fan Speed by 5%
      } else {
        outputB = 30;                                // Set Back Fan Output to 30%
      }
      timerB = millis();                            // Reset Back Fan Interval Timer
      Serial.print("BACK Fan Speed Adjusted UP: ");
      Serial.println(outputB);                      // Print New Speed to Serial
    } else if ((tempBack <= (setPoint - (deadBand / 2)))) {
      if (outputB > 30) {                           // If Output is Currently Greater Than 30%
        outputB = outputB - 5;                      // Decrease Back Fan Speed by 5%
        Serial.print("BACK Fan Speed Adjusted DOWN: ");
        Serial.println(outputB);                    // Print New Speed to Serial
      } else {
        outputB = 0;                                // Set Back Fan Output to OFF
      }
      timerB = millis();                            // Reset Back Fan Interval Timer
    }
    fanSpeedB = map(outputB, 30, 100, 76, 255);    // Map fanSpeedB from 0 to 100 to PWM Value
    analogWrite(fanB, fanSpeedB);                  // Set Front Back to fanSpeedB
  }
}
