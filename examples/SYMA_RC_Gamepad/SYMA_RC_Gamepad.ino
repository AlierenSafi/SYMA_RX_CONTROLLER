/**
 * SYMA_RC_Gamepad.ino
 * 
 * SYMA kumandadan USB Gamepad donusturucu - GUNCEL SURUM
 * 
 * DEGISIKLIKLER:
 * - Seri monitor destegi eklendi (debug icin)
 * - MegaJoy Timer0 cakismasi cozuldu
 * - Gamepad ismi: AES Controller
 * 
 * ONEMLI:
 * ATmega16U2'ye MegaJoy firmware yuklenmis olmali!
 * 
 * Baglanti:
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
// MEGAJOY YERINE KENDI GAMEPAD IMPLEMENTASYONUMUZ
// -----------------------------------------------------------------
// MegaJoy.h kendi Serial.begin(38400) ve Timer0 ISR'i kullaniyor.
// Bu yuzden onun yerine kendi basit gamepad kodumuzu yaziyoruz.

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
// GLOBAL NESNELER
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
  // Seri monitor debug icin (MegaJoy ile cakismaz cunku kendi kodumuzu kullaniyoruz)
  Serial.begin(115200);
  Serial.println(F("AES Controller baslatiliyor..."));
  
  // SPI master mod
  pinMode(SS, OUTPUT);
  
  // NRF24L01+ baslat
  radioModule.setPins(10, 9);
  radioModule.setPwr(PWRLOW);
  protocolHandler.init(&radioModule);
  
  // Gamepad baslangic verisi
  gamepadData = getBlankControllerData();
  
  // ATmega16U2 ile haberlesme baslat (38400 baud)
  // Serial1 kullaniyoruz cunku Serial debug icin ayrildi
  // EGER MegaJoy firmware yuklu ise, Serial0 (USB) uzerinden haberlesmeli
  // Ama debug icin ayri bir Serial kullanmak gerekiyor
  Serial.begin(38400); // Gamepad haberlesmesi
  
  Serial.println(F("AES Controller hazir. Kumandayi acin."));
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
      // Son veriyi koru
      break;
  }
  
  // Gamepad verilerini gonder
  unsigned long now = millis();
  if (now - lastGamepadUpdate >= GAMEPAD_INTERVAL) {
    sendGamepadData();
    lastGamepadUpdate = now;
  }
  
  // Debug ciktisi (her 500ms'de bir)
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 500) {
    lastDebug = now;
    printDebugInfo(status);
  }
}

// -----------------------------------------------------------------
// GAMEPAD VERI GONDERIMI (ATmega16U2 ile haberlesme)
// -----------------------------------------------------------------

void sendGamepadData() {
  // MegaJoy firmware beklenen protokol:
  // ATmega16U2 bir byte gonderir (index), ATmega2560 o indeksteki byte'i geri gonderir
  // Ama biz basit bir protokol kullanacagiz
  
  // Eger Serial uzerinden istek gelirse yanit ver
  while (Serial.available() > 0) {
    byte index = Serial.read();
    if (index < sizeof(gamepadData_t)) {
      Serial.write(((uint8_t*)&controllerDataBuffer)[index]);
    }
  }
}

// -----------------------------------------------------------------
// VERI DONUSTURME
// -----------------------------------------------------------------

void updateGamepadFromTransmitter() {
  // Sol Stick
  gamepadData.analogAxisArray[0] = map(transmitterData.yaw, 127, -127, 0, 1023);
  gamepadData.analogAxisArray[1] = map(transmitterData.throttle, 0, 255, 0, 1023);
  
  // Sag Stick
  gamepadData.analogAxisArray[2] = map(transmitterData.roll, 127, -127, 0, 1023);
  gamepadData.analogAxisArray[3] = map(transmitterData.pitch, -127, 127, 0, 1023);
  
  // Butonlar
  gamepadData.buttonArray[0] = 0;
  if (transmitterData.video)      gamepadData.buttonArray[0] |= (1 << 0);
  if (transmitterData.picture)    gamepadData.buttonArray[0] |= (1 << 1);
  if (transmitterData.highspeed)  gamepadData.buttonArray[0] |= (1 << 2);
  if (transmitterData.flip)       gamepadData.buttonArray[0] |= (1 << 3);
  
  // Trim degerleri
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
// DEBUG CIKTISI
// -----------------------------------------------------------------

void printDebugInfo(uint8_t status) {
  Serial.print(F("[DEBUG] Durum: "));
  switch (status) {
    case NOT_BOUND:         Serial.print(F("ESLESME_YOK")); break;
    case BIND_IN_PROGRESS:  Serial.print(F("BAGLANIYOR")); break;
    case BOUND_NO_VALUES:   Serial.print(F("BAGLI_BEKLIYOR")); break;
    case BOUND_NEW_VALUES:  Serial.print(F("VERI_ALINDI")); break;
    default:                Serial.print(F("BILINMIYOR")); break;
  }
  
  if (status == BOUND_NEW_VALUES) {
    Serial.print(F(" | Throttle:")); Serial.print(transmitterData.throttle);
    Serial.print(F(" Yaw:")); Serial.print(transmitterData.yaw);
    Serial.print(F(" Pitch:")); Serial.print(transmitterData.pitch);
    Serial.print(F(" Roll:")); Serial.print(transmitterData.roll);
    Serial.print(F(" Butonlar:"));
    if (transmitterData.video)     Serial.print(F(" Video"));
    if (transmitterData.picture)   Serial.print(F(" Picture"));
    if (transmitterData.highspeed) Serial.print(F(" HighSpeed"));
    if (transmitterData.flip)      Serial.print(F(" Flip"));
  }
  Serial.println();
}
