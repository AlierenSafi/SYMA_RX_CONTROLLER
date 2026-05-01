/**
 * SYMA_RC_Debug.ino
 * 
 * Transmitter connection and data test tool.
 * Displays connection status and received data via serial monitor.
 * 
 * Connections:
 * - NRF24L01+ VCC -> 3.3V
 * - NRF24L01+ GND -> GND
 * - NRF24L01+ CE  -> Pin 10
 * - NRF24L01+ CSN -> Pin 9
 * - NRF24L01+ SCK -> Pin 52 (Mega)
 * - NRF24L01+ MOSI -> Pin 51 (Mega)
 * - NRF24L01+ MISO -> Pin 50 (Mega)
 * 
 * Usage:
 * 1. Upload to Arduino
 * 2. Open serial monitor (115200 baud)
 * 3. Turn on transmitter
 * 4. Observe connection status and data
 */

#include <SPI.h>
#include "nrf24l01p.h"
#include "symax_protocol.h"

nrf24l01p radioModule;
symaxProtocol protocolHandler;
rx_values_t transmitterData;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 100; // Update every 100ms

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; } // Wait for Leonardo/Micro
  
  Serial.println(F("========================================"));
  Serial.println(F("  SYMA RC Transmitter Test Tool"));
  Serial.println(F("========================================"));
  Serial.println();
  
  pinMode(SS, OUTPUT);
  radioModule.setPins(10, 9);
  radioModule.setPwr(PWRLOW);
  protocolHandler.init(&radioModule);
  
  Serial.println(F("NRF24L01+ initialized. Waiting for transmitter..."));
  Serial.println(F("Turn on transmitter and wait."));
  Serial.println();
}

void loop() {
  uint8_t status = protocolHandler.run(&transmitterData);
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = currentTime;
    
    switch (status) {
      case NOT_BOUND:
        Serial.println(F("[STATUS] Not Bound - Is transmitter on?"));
        break;
        
      case BIND_IN_PROGRESS:
        Serial.println(F("[STATUS] Binding in progress..."));
        break;
        
      case BOUND_NO_VALUES:
        Serial.println(F("[STATUS] Connected - Waiting for data..."));
        break;
        
      case BOUND_NEW_VALUES:
        printTransmitterData();
        break;
    }
  }
}

void printTransmitterData() {
  Serial.println(F("----------------------------------------"));
  Serial.println(F("[DATA RECEIVED]"));
  Serial.println();
  
  // Analog values
  Serial.println(F("--- Analog Controls ---"));
  Serial.print(F("Throttle:          ")); 
  printBar(transmitterData.throttle, 0, 255);
  Serial.print(F("  ")); Serial.print(transmitterData.throttle); Serial.println(F("/255"));
  
  Serial.print(F("Yaw (Rotation):    ")); 
  printSignedBar(transmitterData.yaw, -127, 127);
  Serial.print(F("  ")); Serial.print(transmitterData.yaw); Serial.println();
  
  Serial.print(F("Pitch (Tilt):      ")); 
  printSignedBar(transmitterData.pitch, -127, 127);
  Serial.print(F("  ")); Serial.print(transmitterData.pitch); Serial.println();
  
  Serial.print(F("Roll (Bank):       ")); 
  printSignedBar(transmitterData.roll, -127, 127);
  Serial.print(F("  ")); Serial.print(transmitterData.roll); Serial.println();
  
  Serial.println();
  
  // Trim values
  Serial.println(F("--- Trim Settings ---"));
  Serial.print(F("Yaw Trim:    ")); Serial.println(transmitterData.trim_yaw);
  Serial.print(F("Pitch Trim:  ")); Serial.println(transmitterData.trim_pitch);
  Serial.print(F("Roll Trim:   ")); Serial.println(transmitterData.trim_roll);
  
  Serial.println();
  
  // Buttons
  Serial.println(F("--- Buttons ---"));
  Serial.print(F("Video:      ")); printOnOff(transmitterData.video);
  Serial.print(F("Picture:    ")); printOnOff(transmitterData.picture);
  Serial.print(F("High Speed: ")); printOnOff(transmitterData.highspeed);
  Serial.print(F("Flip:       ")); printOnOff(transmitterData.flip);
  
  Serial.println(F("----------------------------------------"));
  Serial.println();
}

void printBar(uint8_t value, uint8_t minVal, uint8_t maxVal) {
  int width = 20;
  int pos = map(value, minVal, maxVal, 0, width);
  Serial.print(F("["));
  for (int i = 0; i < width; i++) {
    if (i < pos) Serial.print(F("="));
    else Serial.print(F(" "));
  }
  Serial.print(F("]"));
}

void printSignedBar(int8_t value, int8_t minVal, int8_t maxVal) {
  int width = 20;
  int center = width / 2;
  int pos = map(value, minVal, maxVal, 0, width);
  
  Serial.print(F("["));
  for (int i = 0; i < width; i++) {
    if (i == center) Serial.print(F("|"));
    else if (i < pos) Serial.print(F("="));
    else Serial.print(F(" "));
  }
  Serial.print(F("]"));
}

void printOnOff(bool state) {
  if (state) {
    Serial.println(F("[ ON ]"));
  } else {
    Serial.println(F("[OFF ]"));
  }
}
