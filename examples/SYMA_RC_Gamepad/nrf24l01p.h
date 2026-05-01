/*
  nrf24l01p.h - NRF24L01+ 2.4GHz Transceiver Driver
  
  Copyright (C) 2014 Alexandre Clienti
  Copyright (C) 2016 Suxsem
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  DESCRIPTION:
  Low-level SPI driver for Nordic Semiconductor NRF24L01+ 2.4GHz transceiver.
  Provides register access, packet handling, and RF configuration.
  
  HARDWARE:
  - NRF24L01+ module
  - 3.3V logic level (5V tolerant on some pins, use level shifter recommended)
  
  FEATURES:
  - SPI communication at 8MHz
  - Configurable RF channel (2400-2525 MHz)
  - Adjustable output power (-18dBm to 0dBm)
  - 250kbps, 1Mbps, 2Mbps data rates
  - 5-byte address width
  - 16-byte payload support
*/

#ifndef NRF24_L01P_H_
#define NRF24_L01P_H_

#include "Arduino.h"
#include <SPI.h>

// -----------------------------------------------------------------------------
// Register Map
// -----------------------------------------------------------------------------

#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define FIFO_STATUS 0x17

// -----------------------------------------------------------------------------
// Bit Mnemonics
// -----------------------------------------------------------------------------

#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define RX_DR       6
#define TX_DS       5
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

// -----------------------------------------------------------------------------
// Instruction Mnemonics
// -----------------------------------------------------------------------------

#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

// -----------------------------------------------------------------------------
// RF Power Levels
// -----------------------------------------------------------------------------

enum rfPower {
  PWRLOW = 0,      // -18dBm (16uW)
  PWRMEDIUM,       // -12dBm (60uW)
  PWRHIGH,         // -6dBm (250uW)
  PWRMAX           // 0dBm (1mW)
};

// -----------------------------------------------------------------------------
// Data Rates
// -----------------------------------------------------------------------------

enum {
  NRF24L01_BR_1M = 0,
  NRF24L01_BR_2M,
  NRF24L01_BR_250K,
  NRF24L01_BR_RSVD
};

// -----------------------------------------------------------------------------
// NRF24L01+ Class
// -----------------------------------------------------------------------------

class nrf24l01p {
  public:
    nrf24l01p();
    ~nrf24l01p();
    
    // Pin configuration
    void setPins(uint8_t cePin, uint8_t csPin);
    
    // Power level setting
    void setPwr(uint8_t power = PWRLOW);
    
    // Module initialization
    void init(uint8_t payloadSize);
    
    // Chip Enable control
    inline void setCeLow() {
      digitalWrite(mCePin, LOW);
    }
    
    inline void setCeHigh() {
      digitalWrite(mCePin, HIGH);
    }
    
    // Chip Select control
    inline void setCsLow() {
      digitalWrite(mCsPin, LOW);
    }
    
    inline void setCsHigh() {
      digitalWrite(mCsPin, HIGH);
    }
    
    // Frequency channel (0-125, actual frequency = 2400 + channel MHz)
    inline void switchFreq(uint8_t freq) {
      writeRegister(RF_CH, freq);
    }
    
    // RX data ready flag
    inline bool rxFlag() {
      return (readRegister(STATUS) & _BV(RX_DR));
    }
    
    // Clear RX flag
    inline void resetRxFlag() {
      writeRegister(STATUS, _BV(RX_DR));
    }
    
    // RX FIFO empty check
    inline bool rxEmpty() {
      return (readRegister(FIFO_STATUS) & _BV(RX_EMPTY));
    }
    
    // Read received payload
    uint8_t readPayload(void* buf, uint8_t len);
    
    // Enter RX mode
    uint8_t rxMode();
    
    // Flush buffers
    uint8_t flushRx();
    uint8_t flushTx();
    
    // Set RX address
    uint8_t setAddress(const uint8_t* buf, uint8_t len);

  protected:
    uint8_t readRegister(uint8_t reg, uint8_t* buf, uint8_t len);
    uint8_t readRegister(uint8_t reg);
    uint8_t writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len);
    uint8_t writeRegister(uint8_t reg, uint8_t value);
    uint8_t setBitrate(uint8_t bitrate);
    uint8_t setPower(uint8_t power);

    uint8_t mPower;
    uint8_t mCePin;
    uint8_t mCsPin;
    uint8_t mPayloadSize;
};

#endif /* NRF24_L01P_H_ */
