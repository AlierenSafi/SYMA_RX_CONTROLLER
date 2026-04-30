/*
  MegaJoy.h - USB Gamepad Library for Arduino Mega
  
  Copyright (c) 2012 Alan Chatham
  RMIT Exertion Games Lab
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  DESCRIPTION:
  This library enables Arduino Mega 2560 to act as a native USB HID gamepad.
  It communicates with the ATmega16U2 USB interface chip via serial protocol.
  
  IMPORTANT NOTES:
  - Pins 0 and 1 are reserved for serial communication
  - Timer0 is used for USB communication timing
  - Serial is initialized at 38400 baud
  - Cannot use Serial for debugging when using this library
  
  SETUP REQUIREMENTS:
  1. Flash ATmega16U2 with MegaJoy firmware (MegaJoy.hex)
  2. Upload sketch to ATmega2560
  3. Reconnect USB cable
  
  USAGE:
  1. Call setupMegaJoy() in setup()
  2. Create megaJoyControllerData_t variable
  3. Update data structure with button/stick values
  4. Call setControllerData() to send to USB host
*/

#ifndef MEGAJOY_H
#define MEGAJOY_H

#include <stdint.h>
#include <util/atomic.h>
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Data Structure Definitions
// -----------------------------------------------------------------------------

#define BUTTON_ARRAY_SIZE 8
#define ANALOG_AXIS_ARRAY_SIZE 12

/*
  Main controller data structure
  This exact layout is required for communication with ATmega16U2 firmware
  Do not modify field order or sizes
*/
typedef struct megaJoyControllerData_t {
  // 64 buttons total (8 bytes x 8 bits)
  // buttonArray[0] bit 0 = Button 1, bit 1 = Button 2, etc.
  uint8_t buttonArray[BUTTON_ARRAY_SIZE];
  
  // D-Pad 1 (4 directions)
  uint8_t dpad0LeftOn  : 1;
  uint8_t dpad0UpOn    : 1;
  uint8_t dpad0RightOn : 1;
  uint8_t dpad0DownOn  : 1;
  
  // D-Pad 2 (4 directions)
  uint8_t dpad1LeftOn  : 1;
  uint8_t dpad1UpOn    : 1;
  uint8_t dpad1RightOn : 1;
  uint8_t dpad1DownOn  : 1;
  
  // 12 analog axes (0-1023 typical range)
  // analogAxisArray[0] = X, [1] = Y, [2] = Z, etc.
  int16_t analogAxisArray[ANALOG_AXIS_ARRAY_SIZE];
} megaJoyControllerData_t;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

// Initialize MegaJoy with default 1ms polling interval
void setupMegaJoy(void);

// Initialize MegaJoy with custom polling interval (ms)
// Note: Maximum recommended is 20ms due to ATmega16U2 timeout
void setupMegaJoy(int interval);

// Send controller data to USB host
// Call this regularly in loop()
void setControllerData(megaJoyControllerData_t controllerData);

// Get initialized controller data with:
// - All buttons released
// - All analog axes centered (512)
megaJoyControllerData_t getBlankDataForMegaController(void);

// -----------------------------------------------------------------------------
// Internal Implementation
// -----------------------------------------------------------------------------

// Data buffer for USB communication
megaJoyControllerData_t controllerDataBuffer;

// Set controller data with atomic operation
void setControllerData(megaJoyControllerData_t controllerData) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    controllerDataBuffer = controllerData;
  }
}

// Serial polling interval (milliseconds)
volatile int serialCheckInterval = 1;
int serialCheckCounter = 0;

// Initialize MegaJoy library
void setupMegaJoy(void) {
  controllerDataBuffer = getBlankDataForMegaController();
  Serial.begin(38400);
  
  // Configure Timer0 compare match A interrupt
  // Fires approximately every 1ms
  OCR0A = 128;
  TIMSK0 |= (1 << OCIE0A);
}

// Initialize with custom polling interval
void setupMegaJoy(int interval) {
  serialCheckInterval = interval;
  setupMegaJoy();
}

// Timer0 compare match A interrupt handler
// Handles serial communication with ATmega16U2
ISR(TIMER0_COMPA_vect) {
  serialCheckCounter++;
  if (serialCheckCounter >= serialCheckInterval) {
    serialCheckCounter = 0;
    
    while (Serial.available() > 0) {
      byte inByte = Serial.read();
      // Send requested byte from controller data structure
      Serial.write(((uint8_t*)&controllerDataBuffer)[inByte]);
    }
  }
}

// Initialize blank controller data
megaJoyControllerData_t getBlankDataForMegaController(void) {
  megaJoyControllerData_t controllerData;
  
  // Clear all buttons
  for (int i = 0; i < BUTTON_ARRAY_SIZE; i++) {
    controllerData.buttonArray[i] = 0;
  }
  
  // Clear D-Pad 1
  controllerData.dpad0LeftOn = 0;
  controllerData.dpad0UpOn = 0;
  controllerData.dpad0RightOn = 0;
  controllerData.dpad0DownOn = 0;
  
  // Clear D-Pad 2
  controllerData.dpad1LeftOn = 0;
  controllerData.dpad1UpOn = 0;
  controllerData.dpad1RightOn = 0;
  controllerData.dpad1DownOn = 0;
  
  // Center all analog axes
  for (int i = 0; i < ANALOG_AXIS_ARRAY_SIZE; i++) {
    controllerData.analogAxisArray[i] = 512;
  }
  
  return controllerData;
}

#endif /* MEGAJOY_H */
