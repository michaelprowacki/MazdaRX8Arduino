/*
 *  Project     RX-8 Arduino
 *  @author     Christian Groleau
 *  @link       https://gitlab.com/christiangroleau/rx8-arduino
 *  @license    MIT - Copyright (c) 2022 Christian Groleau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

#include "mcp2515_can.h"
#include "forza_data.h"

// set to 1 to view serial output
#define DEBUG 0

EthernetUDP Udp;

// local mac
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE }; // enter real mac

// ip & port listening for udp data
byte ip[] = { 10, 0, 1, 15 };   // enter real ip
unsigned int local_port = 5600;

// prepare the udp packet buffer
const uint32_t UDP_PACKET_SIZE = sizeof(ForzaData_t);
char packet_buffer[UDP_PACKET_SIZE];
ForzaData_t *forza_data = (ForzaData_t *)packet_buffer;

// CAN bus
byte output_data[8]; // output buffer to send to the cluster
mcp2515_can CAN(9);  // set the CS pin

// tachometer and speedometer values to send to cluster
float rpm;
float kmh_speed;

// constant for converting meters per second
const int MPS_TO_KPH = 3.6;

// track time for delay to disable warning lights
const unsigned long dashboardPeriod = 40;
const unsigned long handbrakePeriod = 120;
unsigned long dashboardStartMillis;
unsigned long handbrakeStartMillis;
unsigned long dashboardMillis;
unsigned long handbrakeMillis;

const int handbrakePin = 7; // handbrake connected to pin 7
bool handbrakeHigh = true; // is the handbrake disengaged (true = yes)

/**
 * Initializes UPD server and CAN bus client
 *
 * Optionally, initializes the serial writer for debug
 *
 */
void setup() {
#if DEBUG
  Serial.begin(19200);
  Serial.println("Starting up...");
#endif

  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  Udp.begin(local_port);
  CAN.begin(CAN_500KBPS);

  pinMode(handbrakePin, INPUT_PULLUP); // Set up the handbrake with the pullup resistor

  // track the starting time
  dashboardStartMillis = millis();
  handbrakeStartMillis = millis();
}

/**
 * Continously receive, transform and send data to the cluster
 *
 */
void loop() {
  // without a minimal delay the malfunction indicator lamps (MIL) stay lit
  dashboardMillis = millis();
  if (dashboardMillis - dashboardStartMillis >= dashboardPeriod) {
    disableWarningLights();
    dashboardStartMillis = dashboardMillis;
  }

  
  // Send a serial input to the PC as if the letter K was being pressed on the keyboard
  // this serial input is read by import_serial.py which must be running

  // Use a delay so that the computer isn't being overloaded with serial inputs/prints
  handbrakeMillis = millis();
  if (handbrakeMillis - handbrakeStartMillis >= handbrakePeriod){
    if (digitalRead(handbrakePin) == LOW){
      Serial.println("pressK");
      handbrakeHigh = false;
    }
    if (digitalRead(handbrakePin) == HIGH){
      // No need to constantly be sending a blank serial print line, only do it once 
      // until the handbrake has been toggled again
      if (handbrakeHigh == false){
        Serial.println("");
        handbrakeHigh = true;
      }
      
    }
    handbrakeStartMillis = handbrakeMillis;
  }


  int packet_size = Udp.parsePacket();

  if (packet_size) {
    Udp.read(packet_buffer, UDP_PACKET_SIZE);

    // update cluster only when in a race
    if (forza_data->is_race_on) {
      transform();
      updateDashboard();

#if DEBUG
      Serial.print("Packet size: ");
      Serial.print(packet_size);
      Serial.print(" -- current_engine_rpm: ");
      Serial.print(forza_data->current_engine_rpm);
      Serial.print(" -- speed: ");
      Serial.println(forza_data->speed);
#endif
    }
  }

}

/**
 * Transform game data to cluster-specific data
 * 
 */
void transform() {
  // Reverse engineering the data expected by the RX-8 tach and speedometer is *tricky*
  // and required quite a bit of trial and error.
  // The following calculations yield very accurate results when comparing the
  // in-game displays to the real cluster.
  // Small discrepancies are noticeable during quick acceleration otherwise
  // when, at relatitvely stables RPMs and speeds, the accuracy is practically 100%.
  //
  // These calculations are based on jimkoeh's finding here: 
  // https://github.com/jimkoeh/rx8/blob/66522f15c519f0cb0d17c018c006b84faf86a5e6/rx8.ino#L213
  //
  // His RPM approximations range from 0.98 down to 0.955 but for simplicity
  // we settled on a 0.95 constant. This yields *very* good results.
  rpm = forza_data->current_engine_rpm * 0.95;

  // Similar to RPM approximations the speedometer values are very accurate
  // when increased by a factor over 100. Through trial and error it was found that 118 
  // yields near perfect results.
  //
  // referring to jimkoeh's repo again, he speculated on the resolution of this data as well here:
  // https://github.com/jimkoeh/rx8/blob/66522f15c519f0cb0d17c018c006b84faf86a5e6/rx8.ino#L71
  kmh_speed = forza_data->speed * MPS_TO_KPH * 118;

  // Prevent the RX-8 speed display from rolling over to 0 when travelling beyond 300km/h
  kmh_speed = kmh_speed <= 30000.00 ? kmh_speed : 30000.00;
}

/**
 * Continuously send updated data to the cluster tach dial and speed digital display
 *
 */
void updateDashboard() {

  // tachometer
  output_data[0] = (rpm * 4) / 256;                   // rpm (high byte)
  output_data[1] = ((int)(rpm * 4)) % 256;            // rpm (low byte)

  output_data[2] = 0xFF;                              // unknown
  output_data[3] = 0xFF;                              // unknown

  // speedometer
  output_data[4] = (kmh_speed + 10000) / 256;         // speed (high byte)
  output_data[5] = ((int)(kmh_speed + 10000)) % 256;  // speed (low byte)

  output_data[6] = 0x00;                              // unused (accelerator pedal)
  output_data[7] = 0x00;                              // unknown
  
  CAN.sendMsgBuf(0x201, 0, 8, output_data);
}

/**
 * Continuously send data to the cluster to disable warning lights
 * 
 * The disable signals must be continuously sent as their 
 * default mode is to remain 'on'
 *
 */
void disableWarningLights() {
  // note: empty fuel gauge and air bag MIL will remain lit and are addressed through hardware

  // turn off steering MIL
  output_data[0] = 0x00;

  CAN.sendMsgBuf(0x300, 0, 8, output_data);

  // turn off various dynamic stability control (DSC) lamps
  output_data[0] = 0xFE; // unknown
  output_data[1] = 0xFE; // unknown
  output_data[2] = 0xFE; // unknown
  output_data[3] = 0x34; // DSC off in combination with byte 5
  output_data[4] = 0x00; // ABS warning/brake failure
  output_data[5] = 0x40; // traction control system (TCS) in combination with byte 3
  output_data[6] = 0x00; // unknown
  output_data[7] = 0x00; // unknown

  CAN.sendMsgBuf(0x212, 0, 8, output_data);

  // other gauges / warning lights
  output_data[0] = 0x98; // engine temperature
  output_data[1] = 0x00; // odometer (0x10, 0x11, 0x17 increments by 0.1 miles)
  output_data[2] = 0x00; // unknown
  output_data[3] = 0x00; // unknown
  output_data[4] = 0x01; // oil pressure
  output_data[5] = 0x00; // check engine light
  output_data[6] = 0x00; // coolant, oil and battery
  output_data[7] = 0x00; // unknown

  CAN.sendMsgBuf(0x420, 0, 8, output_data);
}
