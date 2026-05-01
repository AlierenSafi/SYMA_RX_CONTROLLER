/**
 * SYMA_RC_Gamepad.ino
 * 
 * SYMA Transmitter to USB Gamepad Converter - UPDATED VERSION
 * 
 * CHANGES:
 * - Added serial monitor support (for debugging)
 * - Resolved MegaJoy Timer0 conflict
 * - Device name: AES Controller
 * 
 * IMPORTANT:
 * ATmega16U2 must be flashed with MegaJoy firmware!
 * 
 * Connections:
 * - NRF24L01+ VCC  -> 3.3V
 * - NRF24L01+ GND  -> GND
 * - NRF24L01+ CE   -> Pin 10
 * - NRF24L01+ CSN  -> Pin 9
 * - NRF24L01+ SCK  -> Pin 52 (Mega)
 * - NRF24L01+ MOSI -> Pin 51 (Mega)
 * - NRF24L01+ MISO -> Pin 50 (Mega)
 */

#include <SPI.h>
#include "nrf24l01p.h"
#include "symax_protocol.h"

// -----------------------------------------------------------------
// CUSTOM GAMEPAD IMPLEMENTATION (REPLACING MEGAJOY)
// -----------------------------------------------------------------
// MegaJoy.h uses its own Serial.begin(38400) and Timer0 ISR.
// Therefore we implement our own simple gamepad code.

#include <stdint.h>
#include <util/atomic.h>

#define BUTTON_ARRAY_SIZE 8
#define ANALOG_AXIS_ARRAY_SIZE 12

typedef struct gamepadData_t {
  uint8_t buttonArray[BUTTON_ARRAY_SIZE];
  uint8_t dpad0LeftOn  : 1;
  uint8_t dpad0UpOn    : 1;
  uint8_t dpad0RightOn : 1;
  uint8_t dpad0DownOn  : 1;
  uint8_t dpad1LeftOn  : 1;
  uint8_t dpad1UpOn    : 1;
  uint8_t dpad1RightOn : 1;
  uint8_t dpad1DownOn  : 1;
  int16_t analogAxisArray[ANALOG_AXIS_ARRAY_SIZE];
} gamepadData_t;

gamepadData_t controllerDataBuffer;

void setControllerData(gamepadData_t controllerData) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    controllerDataBuffer = controllerData;
  }
}

gamepadData_t getBlankControllerData(void) {
  gamepadData_t d;
  for (int i = 0; i < 8; i++) d.buttonArray[i] = 0;
  d.dpad0LeftOn = d.dpad0UpOn = d.dpad0RightOn = d.dpad0DownOn = 0;
  d.dpad1LeftOn = d.dpad1UpOn = d.dpad1RightOn = d.dpad1DownOn = 0;
  for (int i = 0; i < 12; i++) d.analogAxisArray[i] = 512;
  return d;
}

// -----------------------------------------------------------------
// GLOBAL OBJECTS
// -----------------------------------------------------------------

nrf24l01p radioModule;
symaxProtocol protocolHandler;
rx_values_t transmitterData;
gamepadData_t gamepadData;

unsigned long lastGamepadUpdate = 0;
const unsigned long GAMEPAD_INTERVAL = 10; // 10ms = 100Hz

// -----------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------

void setup() {
  // Serial monitor for debugging (does not conflict with MegaJoy since we use our own code)
  Serial.begin(115200);
  Serial.println(F("AES Controller starting..."));
  
  // SPI master mode
  pinMode(SS, OUTPUT);
  
  // Initialize NRF24L01+
  radioModule.setPins(10, 9);
  radioModule.setPwr(PWRLOW);
  protocolHandler.init(&radioModule);
  
  // Initial gamepad data
  gamepadData = getBlankControllerData();
  
  // Start communication with ATmega16U2 (38400 baud)
  // We use Serial for both debug and gamepad communication
  // If MegaJoy firmware is loaded, Serial0 (USB) is used for communication
  Serial.begin(38400); // Gamepad communication
  
  Serial.println(F("AES Controller ready. Turn on transmitter."));
}

// -----------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------

void loop() {
  uint8_t status = protocolHandler.run(&transmitterData);
  
  switch (status) {
    case BOUND_NEW_VALUES:
      updateGamepadFromTransmitter();
      break;
      
    case NOT_BOUND:
      resetGamepadToNeutral();
      break;
      
    case BIND_IN_PROGRESS:
      setSafeNeutralPosition();
      break;
      
    case BOUND_NO_VALUES:
      // Keep last known state
      break;
  }
  
  // Send gamepad data
  unsigned long now = millis();
  if (now - lastGamepadUpdate >= GAMEPAD_INTERVAL) {
    sendGamepadData();
    lastGamepadUpdate = now;
  }
  
  // Debug output (every 500ms)
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 500) {
    lastDebug = now;
    printDebugInfo(status);
  }
}

// -----------------------------------------------------------------
// GAMEPAD DATA TRANSMISSION (Communication with ATmega16U2)
// -----------------------------------------------------------------

void sendGamepadData() {
  // MegaJoy firmware expected protocol:
  // ATmega16U2 sends one byte (index), ATmega2560 returns that byte from controller data structure
  // We use a simple protocol
  
  // If Serial request arrives, respond
  while (Serial.available() > 0) {
    byte index = Serial.read();
    if (index < sizeof(gamepadData_t)) {
      Serial.write(((uint8_t*)&controllerDataBuffer)[index]);
    }
  }
}

// -----------------------------------------------------------------
// DATA CONVERSION
// -----------------------------------------------------------------

void updateGamepadFromTransmitter() {
  // Left Stick
  gamepadData.analogAxisArray[0] = map(transmitterData.yaw, 127, -127, 0, 1023);
  gamepadData.analogAxisArray[1] = map(transmitterData.throttle, 0, 255, 0, 1023);
  
  // Right Stick
  gamepadData.analogAxisArray[2] = map(transmitterData.roll, 127, -127, 0, 1023);
  gamepadData.analogAxisArray[3] = map(transmitterData.pitch, -127, 127, 0, 1023);
  
  // Buttons
  gamepadData.buttonArray[0] = 0;
  if (transmitterData.video)      gamepadData.buttonArray[0] |= (1 << 0);
  if (transmitterData.picture)    gamepadData.buttonArray[0] |= (1 << 1);
  if (transmitterData.highspeed)  gamepadData.buttonArray[0] |= (1 << 2);
  if (transmitterData.flip)       gamepadData.buttonArray[0] |= (1 << 3);
  
  // Trim values
  gamepadData.analogAxisArray[4] = map(transmitterData.trim_yaw, -31, 31, 0, 1023);
  gamepadData.analogAxisArray[5] = map(transmitterData.trim_pitch, -31, 31, 0, 1023);
  gamepadData.analogAxisArray[6] = map(transmitterData.trim_roll, -31, 31, 0, 1023);
  
  setControllerData(gamepadData);
}

void resetGamepadToNeutral() {
  gamepadData = getBlankControllerData();
  setControllerData(gamepadData);
}

void setSafeNeutralPosition() {
  gamepadData = getBlankControllerData();
  gamepadData.analogAxisArray[1] = 0; // Throttle minimum
  setControllerData(gamepadData);
}

// -----------------------------------------------------------------
// DEBUG OUTPUT
// -----------------------------------------------------------------

void printDebugInfo(uint8_t status) {
  Serial.print(F("[DEBUG] Status: "));
  switch (status) {
    case NOT_BOUND:         Serial.print(F("NOT_BOUND")); break;
    case BIND_IN_PROGRESS:  Serial.print(F("BINDING")); break;
    case BOUND_NO_VALUES:   Serial.print(F("CONNECTED_WAITING")); break;
    case BOUND_NEW_VALUES:  Serial.print(F("DATA_RECEIVED")); break;
    default:                Serial.print(F("UNKNOWN")); break;
  }
  
  if (status == BOUND_NEW_VALUES) {
    Serial.print(F(" | Throttle:")); Serial.print(transmitterData.throttle);
    Serial.print(F(" Yaw:")); Serial.print(transmitterData.yaw);
    Serial.print(F(" Pitch:")); Serial.print(transmitterData.pitch);
    Serial.print(F(" Roll:")); Serial.print(transmitterData.roll);
    Serial.print(F(" Buttons:"));
    if (transmitterData.video)     Serial.print(F(" Video"));
    if (transmitterData.picture)   Serial.print(F(" Picture"));
    if (transmitterData.highspeed) Serial.print(F(" HighSpeed"));
    if (transmitterData.flip)      Serial.print(F(" Flip"));
  }
  Serial.println();
}
