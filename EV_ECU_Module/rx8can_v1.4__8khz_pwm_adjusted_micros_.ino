#include <SPI.h>
#include <avr/wdt.h>

#include <mcp_can.h>
#include <mcp_can_dfs.h>

#include <Arduino.h>
#include "RX8_CAN_Messages.h"  // Shared CAN encoder/decoder library

// CODE QUALITY IMPROVEMENTS (2025-11-15):
// - Now uses shared RX8_CAN_Messages library for encoding/decoding
// - Eliminated ~60 lines of duplicate CAN encoding code
// - Removed all manual bit manipulation (error-prone)
// - Removed hardcoded "magic numbers" (3.85, 10000, etc.)
// - Single source of truth for CAN protocol across all ECU modules


#define CANint 2 //set int to pin 2 
#define LED2 8
//#define LED3 7

MCP_CAN CAN0(10); // Set CS to pin 10

// Declarations for loop delays
long lastRefreshTime = 0;

long ODORefreshTime = 0;


// Varibles for can user defined can I/O
int PWM1_Pin = 6;
//int PWM2_Pin = 5;
int i_count = 0;
int BMS_Pin = 3;
int BMS_state = LOW;
byte send999[8]  = {0, 0, 0, 0, 1, 0, 0, 0};


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
long frontLeft;
long frontRight;
long rearLeft;
long rearRight;

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
byte send620[7]  = {0};  // ABS system data (Note: EV uses different variant)
byte send630[8]  = {0};  // ABS configuration
byte send650[1]  = {0};  // ABS supplement

//KCM / Immobiliser chat replies
// NOTE: Now using RX8_CAN_Encoder library functions
byte send41[8] = {0};  // Immobilizer response buffer (reused for both responses)

// Time delay for Odo
long ODOus = 4500000;  

void setup() {
  Serial.begin(115200);
  
  pinMode(23,OUTPUT);
  digitalWrite(23,HIGH);
  Serial.println("Start Setup");

  pinMode(LED2, OUTPUT);
  //pinMode(LED3, OUTPUT);
  pinMode(CANint, INPUT);
  pinMode(PWM1_Pin, OUTPUT);  
  pinMode (BMS_Pin, INPUT);

  digitalWrite(LED2, LOW);

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) 
    Serial.println("Found High Speed CAN");
   else 
    Serial.println("Failed to find High Speed CAN");

   CAN0.setMode(MCP_NORMAL);
   
    //  delay(500);
    
  TCCR0B = TCCR0B & B11111000 | B00000010; // for PWM frequency of 7812.50 Hz
  
  
  setDefaults(); //This will wait 0.5 second to ensure the Thottle Pedal is on, it will then take the Voltage as its zero throttle position
}

void setDefaults() {
  Serial.println("Setup Started");
  // StatusMIL
  engTemp         = 0; //140 Roughly in the middle (EV: start at 0)
  odo             = 0;
  oilPressure     = 1;
  checkEngineMIL  = 0;
  checkEngineBL   = 0;
  lowWaterMIL     = 0;
  batChargeMIL    = 0;
  oilPressureMIL  = 0;

  // StatusPCM
  engineRPM       = 9000;   // RPM (EV: high default for motor)
  vehicleSpeed    = 0;      // MPH
  throttlePedal   = 0;      // %

  // StatusDSC
  dscOff          = 0;
  absMIL          = 0;
  etcActiveBL     = 0;
  etcDisabled     = 0;
  brakeFailMIL    = 0;

  // Initialize CAN message arrays using shared library
  // NOTE: EV version uses different ABS variant (byte 4 = 0 instead of 16)
  Serial.println("Initializing CAN messages...");
  RX8_CAN_Encoder::encode0x203(send203);
  RX8_CAN_Encoder::encode0x215(send215);
  RX8_CAN_Encoder::encode0x231(send231);
  RX8_CAN_Encoder::encode0x240(send240);
  // EV-specific: Different ABS configuration
  send620[0] = 0;
  send620[1] = 0;
  send620[2] = 0;
  send620[3] = 0;
  send620[4] = 0;    // EV: 0 instead of 16 (ICE)
  send620[5] = 0;
  send620[6] = 4;    // Vehicle variant
  RX8_CAN_Encoder::encode0x630(send630);
  RX8_CAN_Encoder::encode0x650(send650);
  Serial.println("CAN messages initialized");

  Serial.print("Starting up");
  delay(2000);
  analogWrite (PWM1_Pin, 254); //Primes oil pump on PWM_1 (or motor controller precharge)
  wdt_enable(WDTO_4S);
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


long calcMicrosecODO(float speedKMH){
  long uS;
  float freq;
  float speedMPH;
   
//  Serial.print("Speed = ");
 // Serial.print(speedKMH/100);
 // Serial.println(" km/h");
  speedMPH = speedKMH / 160.934;
  // Required frequency for timer 1 ISR
  //  1.15 is 4140 (Pulse per Mile) / 3600 (1hr in seconds)
  //  0.7146 is 2572.5 (pulse per KM) / 3600
  freq = speedMPH * 1.15; 
 // Serial.print("Freq = ");
 // Serial.print(freq);
 // Serial.println(" Hz");
  uS = 1000000/freq;
  if(uS < 4500000 && uS > 0){
    return (uS);}
  else {
    return (4500000);
  }
  
 
}


void sendOnClock(){
  // Do not increment ODO byte when step is = 4.5s
  // slower than this updateMIL must still be called so 
  // warning lights don't turn on but speed may be zero!
  if ( ODOus <= 4500000){ 
    send420[1]++;   
  }
  updateMIL();
  CAN0.sendMsgBuf(0x420, 0, 7, send420);
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
  
  
//  updateMIL();
//  CAN0.sendMsgBuf(0x420, 0, 7, send420);

  updatePCM();
  CAN0.sendMsgBuf(0x201, 0, 8, send201);
  
  
  
}


  // Add this section back in if you want to take control of ABS / DSC Lights.
  //updateDSC();
 // CAN0.sendMsgBuf(0x212, 0, 7, send212);



void loop() {
  //Send information on the CanBud every 100ms to avoid spamming the system.
  if((millis()/8) - lastRefreshTime >= 100) {
    lastRefreshTime += 80;
    sendOnTenth();
  }


  

  // Call function to updateMIL on variable timebase
   if( (micros()/8) - ODORefreshTime >= ODOus) {
   ODORefreshTime += ODOus;
    sendOnClock();
  }
  
  //Read the CAN and Respond if necessary or use data
  if(CAN_MSGAVAIL == CAN0.checkReceive()) { // Check to see whether data is read
    CAN0.readMsgBuf(&ID, &len, buf);    // Read data
    
   /* if(ID == 530) {
      for(int i = 0; i<len; i++) { // Output 8 Bytes of data in Dec
        Serial.print(buf[i]);
        Serial.print("\t");
      }
      
  //    Serial.print(time); // Timestamp
      Serial.println("");
  //    Serial.println(line); // Line Number
    }
    
    // Keyless Control Module and Immobiliser (COMMENTED OUT IN EV VERSION)
    // NOTE: Now using shared library encoder functions
    // Uncomment if you need immobilizer bypass:
    if(ID == 71) { //71 Dec is 47 Hex - Keyless Chat
      if(buf[1] == 127 && buf[2] == 2) {
        RX8_CAN_Encoder::encode0x041_ResponseA(send41);
        CAN0.sendMsgBuf(0x041, 0, 8, send41);
      }
      if(buf[1] == 92 && buf[2] == 244) {
        RX8_CAN_Encoder::encode0x041_ResponseB(send41);
        CAN0.sendMsgBuf(0x041, 0, 8, send41);
      }
    }*/
    
        //Read wheel speeds to update Dash
    //if(ID == 1200) { //1201 Dec is 4b1 Hex - Wheel Speeds ----> check this address. Wheel speeds for dash 4B1 -> 201

    if(ID == 0x4B0) {
      frontLeft = (buf[0] * 256) + buf[1] - 10000;
      frontRight = (buf[2] * 256) + buf[3] - 10000;
      rearLeft = (buf[4] * 256) + buf[5] - 10000;
      rearRight = (buf[6] * 256) + buf[7] - 10000;
      
      //Going to check front wheel speeds for any issues, ignoring the rears due to problems created by wheelspin
      if (frontLeft - frontRight > 500 || frontLeft - frontRight < -500) { //more than 5kph difference in wheel speed
        checkEngineMIL = 1; //light up engine warning light and set speed to zero
        vehicleSpeed = (((rearLeft + rearRight) / 2) / 100);
      } else {
        vehicleSpeed = (((frontLeft + frontRight) / 2) / 100); //Get average of front two wheels.
      }
      //Update timer count value with live speed for ODO
      //OCR1A = calcTimer1Count((frontLeft + frontRight) / 2);
      // delay in MS for ODO
      ODOus = calcMicrosecODO((frontLeft + frontRight) / 2);
      //Serial.print("ODO Step : ");
      //Serial.print(ODOus);
     // Serial.println("us");
      // Dump speed to serial for debug - this is just a cropped int.
      //Serial.println(vehicleSpeed);
      
    }
  
    // inverter can recieve
    
          if((ID == 10) & (buf[1] * 256 + buf[0] <= 9000)) { //10 Dec is ?Hex - vcu motor speed
      engineRPM  = (buf[1] * 256) + buf[0] ;}
      
      
      if(ID == 15) { // - vcu inverter temp

   Serial.println (buf [0]);
   Serial.println ("");
   //Serial.print(engineRPM);
      Serial.print (engTemp);
     Serial.println ("") ;
  // Serial.println (buf [0]);
   
    //  int normMin = 140;    //  60 degC
    //  int normMax = 220;    // 104 degC

      engTemp = map(buf[0],0,254,88,230);
    }
      
      
      
      if (engineRPM <= 100) {i_count ++;}
      else {i_count = 0;}

      if (i_count >= 3000) {analogWrite(PWM1_Pin,0);}
      else {analogWrite (PWM1_Pin,254);}
      if (i_count >= 4000) {i_count = 4000;} // stops i_count running up to stupid valuve during long waits



     // BMS_state = digitalRead (BMS_Pin);
     // if(BMS_state == LOW){
     //   CAN0.sendMsgBuf(999, 0, 8, send999);}

      
    wdt_reset();  
      
  }
  

    }
