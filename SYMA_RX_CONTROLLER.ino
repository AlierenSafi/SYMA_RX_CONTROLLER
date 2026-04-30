/*
  SYMA RC Controller to USB Gamepad Adapter
  
  Copyright (c) 2024
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  DESCRIPTION:
  This firmware converts SYMA X5C-1/X5SW 2.4GHz RC transmitter signals into
  standard USB HID gamepad input. It uses an Arduino Mega 2560 with NRF24L01+
  module to receive RF signals and MegaJoy library for USB gamepad emulation.
  
  HARDWARE REQUIREMENTS:
  - Arduino Mega 2560
  - NRF24L01+ 2.4GHz transceiver module
  - SYMA X5C-1 or compatible transmitter
  
  CONNECTIONS:
  NRF24L01+    Arduino Mega
  ---------    -----------
  VCC          3.3V
  GND          GND
  CE           Pin 10
  CSN          Pin 9
  SCK          Pin 52
  MOSI         Pin 51
  MISO         Pin 50
  
  COMPATIBLE TRANSMITTERS:
  - SYMA X5C-1 (blue/green LED)
  - SYMA X5SW
  - SYMA X11/X11C
  - SYMA X12
  
  NOTES:
  - Pins 0 and 1 are reserved for serial communication with ATmega16U2
  - Timer0 is used by MegaJoy library for USB communication
  - Do not use Servo library - it conflicts with Timer0
  
  Based on:
  - symaxrx library by execuc and Suxsem
  - MegaJoy library by Alan Chatham
*/

#include <SPI.h>
#include "src/nrf24/nrf24l01p.h"
#include "src/symax/symax_protocol.h"
#include "src/megajoy/MegaJoy.h"

// -----------------------------------------------------------------------------
// Configuration Constants
// -----------------------------------------------------------------------------

// Update interval for USB gamepad reports (milliseconds)
const unsigned long UPDATE_INTERVAL_MS = 10;

// Analog axis value ranges
const int AXIS_MIN = 0;
const int AXIS_MAX = 1023;
const int AXIS_CENTER = 512;

// -----------------------------------------------------------------------------
// Global Objects
// -----------------------------------------------------------------------------

// NRF24L01+ radio module driver
nrf24l01p radioModule;

// SYMA protocol handler
symaxProtocol protocolHandler;

// Container for received transmitter data
rx_values_t transmitterData;

// USB gamepad data structure
megaJoyControllerData_t gamepadData;

// Last successful USB update timestamp
unsigned long lastUpdateTime = 0;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

void updateGamepadFromTransmitter(void);
void resetGamepadToNeutral(void);
void setSafeNeutralPosition(void);

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup() {
  // Initialize MegaJoy USB gamepad library
  // This configures Serial at 38400 baud and sets up Timer0 interrupt
  setupMegaJoy();
  
  // Configure SPI chip select as output (required for SPI master mode)
  pinMode(SS, OUTPUT);
  
  // Initialize NRF24L01+ module
  // CE pin: 10, CSN pin: 9
  radioModule.setPins(10, 9);
  radioModule.setPwr(PWRLOW);
  
  // Initialize SYMA protocol and bind to radio module
  protocolHandler.init(&radioModule);
  
  // Initialize gamepad data with safe default values
  gamepadData = getBlankDataForMegaController();
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------

void loop() {
  // Process SYMA protocol state machine and receive data
  uint8_t connectionStatus = protocolHandler.run(&transmitterData);
  
  switch (connectionStatus) {
    case BOUND_NEW_VALUES:
      // Valid transmitter data received - update gamepad
      updateGamepadFromTransmitter();
      break;
      
    case NOT_BOUND:
      // No transmitter bound - reset all controls to neutral
      resetGamepadToNeutral();
      break;
      
    case BIND_IN_PROGRESS:
      // Binding in progress - set safe position (throttle at minimum)
      setSafeNeutralPosition();
      break;
      
    case BOUND_NO_VALUES:
      // Connected but no new data - maintain last known state
      break;
  }
  
  // Send updated gamepad data to USB host at fixed interval
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    setControllerData(gamepadData);
    lastUpdateTime = currentTime;
  }
}

// -----------------------------------------------------------------------------
// Gamepad Data Update Functions
// -----------------------------------------------------------------------------

/*
  Maps transmitter stick values to USB gamepad analog axes.
  
  Axis Mapping:
  - Axis 0: Left Stick X  (Yaw)      - Left/Right rotation
  - Axis 1: Left Stick Y  (Throttle) - Up/Down (elevator)
  - Axis 2: Right Stick X (Roll)     - Left/Right (aileron)
  - Axis 3: Right Stick Y (Pitch)    - Forward/Backward
  - Axis 4: Yaw Trim
  - Axis 5: Pitch Trim
  - Axis 6: Roll Trim
*/
void updateGamepadFromTransmitter(void) {
  // Left Stick X - Yaw (rotation left/right)
  // Input: 127 (left) to -127 (right)
  gamepadData.analogAxisArray[0] = map(transmitterData.yaw, 127, -127, AXIS_MIN, AXIS_MAX);
  
  // Left Stick Y - Throttle (elevation)
  // Input: 0 (min) to 255 (max)
  gamepadData.analogAxisArray[1] = map(transmitterData.throttle, 0, 255, AXIS_MIN, AXIS_MAX);
  
  // Right Stick X - Roll (bank left/right)
  // Input: 127 (left) to -127 (right)
  gamepadData.analogAxisArray[2] = map(transmitterData.roll, 127, -127, AXIS_MIN, AXIS_MAX);
  
  // Right Stick Y - Pitch (nose up/down)
  // Input: -127 (back) to 127 (forward)
  gamepadData.analogAxisArray[3] = map(transmitterData.pitch, -127, 127, AXIS_MIN, AXIS_MAX);
  
  // Map transmitter buttons to gamepad buttons
  // Button Array[0] bits: 0=Video, 1=Picture, 2=HighSpeed, 3=Flip
  
  // Button 1 (Square) - Video button
  if (transmitterData.video) {
    gamepadData.buttonArray[0] |= (1 << 0);
  } else {
    gamepadData.buttonArray[0] &= ~(1 << 0);
  }
  
  // Button 2 (Cross) - Picture button
  if (transmitterData.picture) {
    gamepadData.buttonArray[0] |= (1 << 1);
  } else {
    gamepadData.buttonArray[0] &= ~(1 << 1);
  }
  
  // Button 3 (Circle) - High Speed mode
  if (transmitterData.highspeed) {
    gamepadData.buttonArray[0] |= (1 << 2);
  } else {
    gamepadData.buttonArray[0] &= ~(1 << 2);
  }
  
  // Button 4 (Triangle) - Flip button
  if (transmitterData.flip) {
    gamepadData.buttonArray[0] |= (1 << 3);
  } else {
    gamepadData.buttonArray[0] &= ~(1 << 3);
  }
  
  // Map trim values to additional analog axes
  gamepadData.analogAxisArray[4] = map(transmitterData.trim_yaw, -31, 31, AXIS_MIN, AXIS_MAX);
  gamepadData.analogAxisArray[5] = map(transmitterData.trim_pitch, -31, 31, AXIS_MIN, AXIS_MAX);
  gamepadData.analogAxisArray[6] = map(transmitterData.trim_roll, -31, 31, AXIS_MIN, AXIS_MAX);
}

/*
  Resets all gamepad controls to neutral state.
  Called when transmitter connection is lost.
*/
void resetGamepadToNeutral(void) {
  // Clear all buttons
  for (int i = 0; i < 8; i++) {
    gamepadData.buttonArray[i] = 0;
  }
  
  // Clear D-Pad
  gamepadData.dpad0LeftOn = 0;
  gamepadData.dpad0UpOn = 0;
  gamepadData.dpad0RightOn = 0;
  gamepadData.dpad0DownOn = 0;
  
  // Center all analog axes
  for (int i = 0; i < 12; i++) {
    gamepadData.analogAxisArray[i] = AXIS_CENTER;
  }
}

/*
  Sets safe neutral position during binding.
  Throttle is set to minimum for safety.
*/
void setSafeNeutralPosition(void) {
  // Clear all buttons
  for (int i = 0; i < 8; i++) {
    gamepadData.buttonArray[i] = 0;
  }
  
  // Set throttle to minimum (safe position)
  gamepadData.analogAxisArray[1] = AXIS_MIN;
  
  // Center other primary axes
  gamepadData.analogAxisArray[0] = AXIS_CENTER;  // Yaw
  gamepadData.analogAxisArray[2] = AXIS_CENTER;  // Roll
  gamepadData.analogAxisArray[3] = AXIS_CENTER;  // Pitch
}
