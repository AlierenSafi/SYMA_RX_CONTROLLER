/*
  nrf24l01p.cpp - NRF24L01+ 2.4GHz Transceiver Driver Implementation
  
  Copyright (C) 2014 Alexandre Clienti
  Copyright (C) 2016 Suxsem
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
*/

#include "nrf24l01p.h"

static uint8_t rf_setup;

nrf24l01p::nrf24l01p() {
  rf_setup = 0x0F;
}

nrf24l01p::~nrf24l01p() {
  // Destructor
}

void nrf24l01p::setPins(uint8_t cePin, uint8_t csPin) {
  mCePin = cePin;
  mCsPin = csPin;
  pinMode(mCePin, OUTPUT);
  pinMode(mCsPin, OUTPUT);
  setCeLow();
  setCsHigh();
}

void nrf24l01p::setPwr(uint8_t power) {
  mPower = power;
}

void nrf24l01p::init(uint8_t payloadSize) {
  SPI.begin();
  mPayloadSize = payloadSize;
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}

uint8_t nrf24l01p::rxMode() {
  setCeLow();
  
  // Enable CRC (2 bytes)
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO));
  delayMicroseconds(100);
  
  // Disable auto acknowledgment
  writeRegister(EN_AA, 0x00);
  
  // Enable first data pipe
  writeRegister(EN_RXADDR, 0x01);
  
  // 5 bytes address width
  writeRegister(SETUP_AW, 0x03);
  
  // 15 retransmit, 4000us pause
  writeRegister(SETUP_RETR, 0xFF);
  
  // Channel 8
  writeRegister(RF_CH, 0x08);
  
  // Set 250kbps data rate for SYMA protocol
  setBitrate(NRF24L01_BR_250K);
  setPower(mPower);
  
  // Clear status register
  writeRegister(STATUS, 0x70);
  
  // RX payload of 10 bytes
  writeRegister(RX_PW_P0, 0x0A);
  writeRegister(FIFO_STATUS, 0x00);
  
  delay(50);
  flushTx();
  flushRx();
  delayMicroseconds(100);
  
  // Power up
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP));
  delayMicroseconds(100);
  
  // Enter RX mode
  writeRegister(CONFIG, _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP) | _BV(PRIM_RX));
  delayMicroseconds(100);
  setCeHigh();
  delayMicroseconds(100);
}

uint8_t nrf24l01p::readRegister(uint8_t reg) {
  setCsLow();
  delayMicroseconds(10);
  SPI.transfer(R_REGISTER | (REGISTER_MASK & reg));
  uint8_t result = SPI.transfer(0xFF);
  setCsHigh();
  return result;
}

uint8_t nrf24l01p::writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len) {
  setCsLow();
  uint8_t result = SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
  while (len--) {
    SPI.transfer(*buf++);
  }
  setCsHigh();
  return result;
}

uint8_t nrf24l01p::writeRegister(uint8_t reg, uint8_t value) {
  setCsLow();
  uint8_t result = SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
  SPI.transfer(value);
  setCsHigh();
  return result;
}

uint8_t nrf24l01p::setAddress(const uint8_t* buf, uint8_t len) {
  return writeRegister(RX_ADDR_P0, buf, len);
}

uint8_t nrf24l01p::readPayload(void* buf, uint8_t len) {
  uint8_t result;
  uint8_t* current = reinterpret_cast<uint8_t*>(buf);
  
  uint8_t data_len = min(len, mPayloadSize);
  uint8_t blank_len = mPayloadSize - data_len;
  
  setCsLow();
  result = SPI.transfer(R_RX_PAYLOAD);
  while (data_len--) {
    *current++ = SPI.transfer(0xFF);
  }
  while (blank_len--) {
    SPI.transfer(0xFF);
  }
  setCsHigh();
  
  return result;
}

uint8_t nrf24l01p::flushRx(void) {
  setCsLow();
  uint8_t result = SPI.transfer(FLUSH_RX);
  setCsHigh();
  return result;
}

uint8_t nrf24l01p::flushTx(void) {
  setCsLow();
  uint8_t result = SPI.transfer(FLUSH_TX);
  setCsHigh();
  return result;
}

uint8_t nrf24l01p::setBitrate(uint8_t bitrate) {
  // Bit 0 goes to RF_DR_HIGH, bit 1 to RF_DR_LOW
  rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
  return writeRegister(RF_SETUP, rf_setup);
}

uint8_t nrf24l01p::setPower(uint8_t power) {
  uint8_t nrf_power = 0;
  switch (power) {
    case PWRLOW:    nrf_power = 0; break;
    case PWRMEDIUM: nrf_power = 1; break;
    case PWRHIGH:   nrf_power = 2; break;
    case PWRMAX:    nrf_power = 3; break;
    default:        nrf_power = 0; break;
  }
  rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
  return writeRegister(RF_SETUP, rf_setup);
}
