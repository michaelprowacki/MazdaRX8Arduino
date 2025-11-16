// Arduino Leonardo code for replacing the PCM within a Mark 1 RX8
// This code allows you to leave the CANBUS in place, just removing the PCM
// There are plenty of ID's on the CANBUS I do not understand, and any use of this code is strictly at your own risk
//
// Features turned on, possibly working - at least they do not complain on the dashboard,
//    ABS / DSC
//    Traction Control
//    Immobiliser Deactivated
//    Power Steering Enabled
//    RPM Controllable
//    Vehicle Speed set by Wheel Sensors
//    Warning lights turned off (you can turn them on in the code if you wish)
//    Speed-Sensitive Wipers (optional - enable with ENABLE_WIPERS below)
//
//    Written by David Blackhurst, dave@blackhurst.co.uk 06/10/2019
//
//    Some parts of this code have been dissected from other sources - in researching this project I copied a lot of code to play with
//    Sorry if some of this is yours - just let me know and I will attribute it to you.
//
//    I have throttle pedal code in here to translate the output of the primary throttle signal to the range my controller required. This is unlikely to be helpful to anyone else
//    unless you happen to have the dodgy chinese controller I have.
//
//    Again use AT YOUR OWN RISK
//
//    DONATE HERE, Anything helps towards the next project (Leaf RX8 Conversion).
//    https://www.paypal.me/DBlackhurst
//
//    CONSOLIDATED MODULE: Integrated speed-sensitive wipers functionality (2025-11-15)
//    Based on: https://github.com/michaelprowacki/MazdaRX8Arduino/Wipers_Module
//
//    CODE QUALITY IMPROVEMENTS (2025-11-15):
//    - Now uses shared RX8_CAN_Messages library for encoding/decoding
//    - Eliminated ~100 lines of duplicate CAN encoding code
//    - Removed all manual bit manipulation (error-prone)
//    - Removed hardcoded "magic numbers" (3.85, 10000, etc.)
//    - Single source of truth for CAN protocol across all modules

#include <Arduino.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include "RX8_CAN_Messages.h"  // Shared CAN encoder/decoder library

// ========== OPTIONAL FEATURES ==========
// Uncomment to enable speed-sensitive wipers
// #define ENABLE_WIPERS

// ========== PIN DEFINITIONS ==========
#define CANint 2
#define LED2 8
#define LED3 7

#ifdef ENABLE_WIPERS
#define WIPER_CONTROL_PIN 6    // Digital pin for wiper control (adjust as needed)
#define WIPER_SENSE_PIN 4      // Optional: wiper position feedback
#endif

MCP_CAN CAN0(17); // Set CS to pin 17

// Variables for Throttle Pedal
int analogPin = A1;
int outputPin = 5;

int val = 0;
int lowPedal = 0;
int highPedal = 0;
int convertThrottle = 0;
int base = 0;
int output = 0;
long lastRefreshTime = 0;

// Variables for PCM, Only overrideable if PCM removed from CAN
bool checkEngineMIL;
bool checkEngineBL;
byte engTemp;
byte odo;
bool oilPressure;
bool lowWaterMIL;
bool batChargeMIL;
bool oilPressureMIL;

// Variables for PCM, Only overrideable if PCM removed from CAN
int engineRPM;
int vehicleSpeed;
byte throttlePedal;

// Variables for ABS/DSC, Only overrideable if ABS/DSC removed from CAN
bool dscOff;
bool absMIL;
bool brakeFailMIL;
bool etcActiveBL;
bool etcDisabled;

// Variables for Wheel Speed
int frontLeft;
int frontRight;
int rearLeft;
int rearRight;

#ifdef ENABLE_WIPERS
// Variables for Speed-Sensitive Wipers
int wiperDelay = 2000;           // Default 2 seconds between wipes
unsigned long lastWipe = 0;      // Timestamp of last wiper activation
bool wiperEnabled = true;        // Master enable for wiper control
#endif

//Variables for reading in from the CANBUS
unsigned char len = 0;
unsigned char buf[8];
unsigned long ID = 0;

//Setup Array's to store bytes to send to CAN on Various ID's
// NOTE: Now initialized using RX8_CAN_Encoder library in setDefaults()
byte send201[8]  = {0};  // Dynamic - updated each cycle with RPM/Speed/Throttle
byte send420[7]  = {0};  // Dynamic - updated each cycle with temp/warnings
byte send212[7]  = {0};  // Optional DSC/ABS control

//Setup PCM Status's required to fool all other CAN devices that everything is OK
// NOTE: Now initialized using RX8_CAN_Encoder library in setDefaults()
byte send203[7]  = {0};  // Traction control data
byte send215[8]  = {0};  // PCM supplement 1
byte send231[5]  = {0};  // PCM supplement 2
byte send240[8]  = {0};  // PCM supplement 3
byte send620[7]  = {0};  // ABS system data
byte send630[8]  = {0};  // ABS configuration
byte send650[1]  = {0};  // ABS supplement

//KCM / Immobiliser chat replies
// NOTE: Now using RX8_CAN_Encoder library functions
byte send41[8] = {0};  // Immobilizer response buffer (reused for both responses)

void setup() {
  Serial.begin(115200);

  pinMode(23,OUTPUT);
  digitalWrite(23,HIGH);
  Serial.println("Start Setup");

  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(CANint, INPUT);

  digitalWrite(LED2, LOW);

#ifdef ENABLE_WIPERS
  // Initialize wiper control pins
  pinMode(WIPER_CONTROL_PIN, OUTPUT);
  digitalWrite(WIPER_CONTROL_PIN, LOW);  // Ensure wipers are off initially
  pinMode(WIPER_SENSE_PIN, INPUT);
  Serial.println("Speed-Sensitive Wipers: ENABLED");
#endif

  if (CAN0.begin(CAN_500KBPS) == CAN_OK) {
    Serial.println("Found High Speed CAN");
  } else {
    Serial.println("Failed to find High Speed CAN");
    while (1) {
      Serial.print("Loop Forever");
      delay(1000);
    }
  }

  setDefaults(); //This will wait 0.5 second to ensure the Thottle Pedal is on, it will then take the Voltage as its zero throttle position
}

void setDefaults() {
  Serial.println("Setup Started");
  // StatusMIL
  engTemp         = 145; //Roughly in the middle
  odo             = 0;
  oilPressure     = 1;
  checkEngineMIL  = 0;
  checkEngineBL   = 0;
  lowWaterMIL     = 0;
  batChargeMIL    = 0;
  oilPressureMIL  = 0;

  // StatusPCM
  engineRPM       = 1000;   // RPM
  vehicleSpeed    = 0;      // MPH
  throttlePedal   = 0;      // %

  // StatusDSC
  dscOff          = 0;
  absMIL          = 0;
  etcActiveBL     = 0;
  etcDisabled     = 0;
  brakeFailMIL    = 0;

  // Initialize CAN message arrays using shared library
  // These are static messages that don't change during operation
  Serial.println("Initializing CAN messages...");
  RX8_CAN_Encoder::initializeECUMessages(send203, send215, send231, send240,
                                          send620, send630, send650);
  Serial.println("CAN messages initialized");

  Serial.print("Start wait to ensure Throttle Pedal is on");
  delay(500);
  lowPedal = analogRead(analogPin) - 40;  //read the throttle pedal, should be around 1.7v minus 40 to ensure no small throttle inputs
  highPedal = 803; //4v

  // Voltage to read from Pedal 1.64v - 4.04v
  // Going to use a safe range 1.7v to 4v
  // Low of 1.7v has been read above as can fluctuate
  // 1.7v = INT 341
  // 4v = INT 803
  // (highPedal - lowPedal) = RANGE FROM RX8 PEDAL
  // out for 1024 (5v max), controller wants 4.5v max = 920 (adding 40 to help stabilise)

  convertThrottle = 960 / (highPedal - lowPedal);
  Serial.print("Low Pedal ");
  Serial.print(lowPedal);
  Serial.print("High Pedal ");
  Serial.println(highPedal);
  Serial.println("Setup Complete");
}

// NOTE: These functions are now simplified to use the shared RX8_CAN_Encoder library
// This eliminates hardcoded bit manipulation and magic numbers

void updateMIL() {
  // Use shared library encoder - much cleaner than manual bit manipulation!
  RX8_CAN_Encoder::encode0x420(send420, engTemp, checkEngineMIL, lowWaterMIL,
                                batChargeMIL, oilPressureMIL);
}

void updatePCM() {
  // Use shared library encoder - handles RPM*3.85 and speed encoding automatically
  RX8_CAN_Encoder::encode0x201(send201, engineRPM, vehicleSpeed, throttlePedal);
}

void updateDSC() {
  // Use shared library encoder - eliminates all manual bit manipulation
  RX8_CAN_Encoder::encode0x212(send212, dscOff, absMIL, brakeFailMIL,
                                 etcActiveBL, etcDisabled);
}

void sendOnTenth() {
  //PCM Status's to mimic the PCM being there, these may be different for different cars, and not all are always required, better safe and include them all.
  CAN0.sendMsgBuf(0x203, 0, 7, send203);
  CAN0.sendMsgBuf(0x215, 0, 8, send215);
  CAN0.sendMsgBuf(0x231, 0, 8, send231);
  CAN0.sendMsgBuf(0x240, 0, 8, send240);
  CAN0.sendMsgBuf(0x620, 0, 7, send620);
  CAN0.sendMsgBuf(0x630, 0, 8, send630);
  CAN0.sendMsgBuf(0x650, 0, 1, send650);
  
  updateMIL();
  CAN0.sendMsgBuf(0x420, 0, 7, send420);

  updatePCM();
  CAN0.sendMsgBuf(0x201, 0, 8, send201);
  
  /* Add this section back in if you want to take control of ABS / DSC Lights.
  updateDSC();
  CAN0.sendMsgBuf(0x212, 0, 7, send212);
  */
}

#ifdef ENABLE_WIPERS
/*
 * Adjust wiper delay based on vehicle speed
 * Speed-dependent timing for intermittent wiper mode
 */
void adjustWiperTiming() {
  if (vehicleSpeed == 0) {
    // Stopped - slow wiping
    wiperDelay = 3000;  // 3 seconds
  }
  else if (vehicleSpeed < 20) {
    // City driving - moderate wiping
    wiperDelay = 2000;  // 2 seconds
  }
  else if (vehicleSpeed < 40) {
    // Suburban - faster wiping
    wiperDelay = 1500;  // 1.5 seconds
  }
  else if (vehicleSpeed < 60) {
    // Highway - fast wiping
    wiperDelay = 1000;  // 1 second
  }
  else {
    // High speed - very fast wiping
    wiperDelay = 500;   // 0.5 seconds
  }
}

/*
 * Control wiper operation with speed-adjusted timing
 * Called from main loop
 */
void controlWipers() {
  if (!wiperEnabled) {
    return;  // Wipers disabled by user
  }

  // Adjust timing based on current vehicle speed
  adjustWiperTiming();

  // Check if it's time for next wipe
  if (millis() - lastWipe >= wiperDelay) {
    // Trigger wiper with short pulse
    digitalWrite(WIPER_CONTROL_PIN, HIGH);
    delay(100);  // 100ms pulse
    digitalWrite(WIPER_CONTROL_PIN, LOW);

    lastWipe = millis();

    // Optional debug output
    // Serial.print("Wipe at ");
    // Serial.print(vehicleSpeed);
    // Serial.print(" mph, delay: ");
    // Serial.println(wiperDelay);
  }
}
#endif

void loop() {
  //Send information on the CanBud every 100ms to avoid spamming the system.
  if(millis() - lastRefreshTime >= 100) {
		lastRefreshTime += 100;
    sendOnTenth();
	}

#ifdef ENABLE_WIPERS
  // Control speed-sensitive wipers (non-blocking)
  controlWipers();
#endif

  //Read the CAN and Respond if necessary or use data
  if(CAN_MSGAVAIL == CAN0.checkReceive()) { // Check to see whether data is read
    CAN0.readMsgBufID(&ID, &len, buf);    // Read data
    
    if(ID == 530) {
      for(int i = 0; i<len; i++) { // Output 8 Bytes of data in Dec
        Serial.print(buf[i]);
        Serial.print("\t");
      }
      
  //    Serial.print(time); // Timestamp
      Serial.println("");
  //    Serial.println(line); // Line Number
    }
    
    //Keyless Control Module and Immobiliser want to have a chat with the PCM, this deals with the conversation
    // Now using shared library encoder functions
    if(ID == 71) { //71 Dec is 47 Hex - Keyless Chat
      if(buf[1] == 127 && buf[2] == 2) {
        RX8_CAN_Encoder::encode0x041_ResponseA(send41);
        CAN0.sendMsgBuf(0x041, 0, 8, send41);
      }
      if(buf[1] == 92 && buf[2] == 244) {
        RX8_CAN_Encoder::encode0x041_ResponseB(send41);
        CAN0.sendMsgBuf(0x041, 0, 8, send41);
      }
    }
    
    //Read wheel speeds to update Dash
    if(ID == 1200) { //1201 Dec is 4b1 Hex - Wheel Speeds
      frontLeft = (buf[0] * 256) + buf[1] - 10000;
      frontRight = (buf[2] * 256) + buf[3] - 10000;
      rearLeft = (buf[4] * 256) + buf[5] - 10000;
      rearRight = (buf[6] * 256) + buf[7] - 10000;
      
      //Going to check front wheel speeds for any issues, ignoring the rears due to problems created by wheelspin
      if (frontLeft - frontRight > 500 || frontLeft - frontRight < -500) { //more than 5kph difference in wheel speed
        checkEngineMIL = 1; //light up engine warning light and set speed to zero
        vehicleSpeed = 0;
      } else {
        vehicleSpeed = ((frontLeft + frontRight) / 2) / 100; //Get average of front two wheels.
      }
      
      Serial.print(vehicleSpeed);
    }
  }
  
  //Throttle Pedal Work
  val = analogRead(analogPin);  // read the input pin
  
  //See Set Defaults Method for Calculations
  if (val < 110 || val > 960) { // If input is less than 0.5v or higher than 4.5v something is wrong so NO THROTTLE
    val = 0;
  }
  base = val - lowPedal;
  if (base < 0) {
    base = 0;
  }
  output = base * convertThrottle;
  if (output > 960) {
    output = 960;
  }
  throttlePedal = (100 / 960) * output;
  analogWrite(outputPin,(output/4));
}
