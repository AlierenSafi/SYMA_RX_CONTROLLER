/*
  symax_protocol.cpp - SYMA RC Protocol Implementation
  
  Copyright (C) 2014 Alexandre Clienti
  Copyright (C) 2016 Suxsem
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
*/

#include "symax_protocol.h"

// Bind address - fixed for all SYMA transmitters during binding
const uint8_t bind_rx_tx_addr[] = {0xAB, 0xAC, 0xAD, 0xAE, 0xAF};

// Bind channels - transmitter cycles through these during binding
const uint8_t chans_bind[] = {0x4B, 0x30, 0x40, 0x20};

// Frequency hopping tables stored in program memory
static const PROGMEM uint8_t START_CHANS_1[] = {0x0A, 0x1A, 0x2A, 0x3A};
static const PROGMEM uint8_t START_CHANS_2[] = {0x2A, 0x0A, 0x42, 0x22};
static const PROGMEM uint8_t START_CHANS_3[] = {0x1A, 0x3A, 0x12, 0x32};

// Operational frequency channels (calculated from transmitter ID)
int currentFrequency = 58;
int frequencyCounter = 20;
long previousFrequency = 0;

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

symaxProtocol::symaxProtocol() {
  mState = NO_BIND;
  mLastSignalTime = 0;
  mRfChNum = 0;
}

symaxProtocol::~symaxProtocol() {
  // Destructor
}

// -----------------------------------------------------------------------------
// Protocol Initialization
// -----------------------------------------------------------------------------

void symaxProtocol::init(nrf24l01p* wireless) {
  mWireless = wireless;
  mWireless->init(PSIZE);
  delayMicroseconds(100);
  mWireless->rxMode();
  mWireless->setAddress(bind_rx_tx_addr, 5);
  mWireless->switchFreq(chans_bind[0]);
  mLastSignalTime = millis();
}

// -----------------------------------------------------------------------------
// Checksum Calculation
// -----------------------------------------------------------------------------

uint8_t symaxProtocol::checksum(uint8_t* data) {
  uint8_t sum = data[0];
  for (int i = 1; i < PSIZE - 1; i++) {
    sum ^= data[i];
  }
  return sum + 0x55;
}

// -----------------------------------------------------------------------------
// Frequency Channel Calculation
// -----------------------------------------------------------------------------

void symaxProtocol::setRFChannel(uint8_t address) {
  uint8_t laddress = address & 0x1F;
  uint8_t i;
  uint32_t* pchans = (uint32_t*)mRFChanBufs;
  
  if (laddress < 0x10) {
    if (laddress == 6) laddress = 7;
    for (i = 0; i < FSIZE; i++) {
      mRFChanBufs[i] = pgm_read_byte(START_CHANS_1 + i) + laddress;
    }
  } else if (laddress < 0x18) {
    for (i = 0; i < FSIZE; i++) {
      mRFChanBufs[i] = pgm_read_byte(START_CHANS_2 + i) + (laddress & 0x07);
    }
    if (laddress == 0x16) {
      mRFChanBufs[0] += 1;
      mRFChanBufs[1] += 1;
    }
  } else if (laddress < 0x1E) {
    for (i = 0; i < FSIZE; i++) {
      mRFChanBufs[i] = pgm_read_byte(START_CHANS_3 + i) + (laddress & 0x07);
    }
  } else if (laddress == 0x1E) {
    *pchans = 0x38184121;
  } else {
    *pchans = 0x39194121;
  }
}

// -----------------------------------------------------------------------------
// Main State Machine
// -----------------------------------------------------------------------------

uint8_t symaxProtocol::run(rx_values_t* rx_value) {
  uint8_t returnValue = UNKNOWN;
  
  switch (mState) {
    case BOUND:
      returnValue = handleBoundState(rx_value);
      break;
      
    case NO_BIND:
      returnValue = handleNoBindState();
      break;
      
    case WAIT_FIRST_SYNCHRO:
      returnValue = handleWaitSynchroState();
      break;
      
    default:
      break;
  }
  
  return returnValue;
}

// -----------------------------------------------------------------------------
// State Handlers
// -----------------------------------------------------------------------------

uint8_t symaxProtocol::handleBoundState(rx_values_t* rx_value) {
  unsigned long newTime = millis();
  uint8_t returnValue = BOUND_NO_VALUES;
  
  if (!mWireless->rxFlag()) {
    // No data available - cycle through frequencies
    cycleFrequency();
    delayMicroseconds(5000);
    
    // Check for signal loss timeout (600 seconds)
    if ((newTime - mLastSignalTime) > 600000) {
      // Signal lost - return to bind mode
      mWireless->setAddress(bind_rx_tx_addr, 5);
      mWireless->switchFreq(chans_bind[0]);
      mState = NO_BIND;
      mLastSignalTime = newTime;
    }
  } else {
    // Data received
    bool incrementChannel = false;
    mWireless->resetRxFlag();
    
    while (!mWireless->rxEmpty()) {
      mWireless->readPayload(mFrame, PSIZE);
      
      if (checksum(mFrame) == mFrame[PSIZE - 1]) {
        incrementChannel = true;
        
        // Check if this is a data frame (not bind frame)
        if (mFrame[5] != 0xAA && mFrame[6] != 0xAA) {
          returnValue = BOUND_NEW_VALUES;
          extractData(rx_value);
          mLastSignalTime = newTime;
        }
      }
    }
    
    if (incrementChannel) {
      mRfChNum++;
      if (mRfChNum >= FSIZE) mRfChNum = 0;
    }
  }
  
  return returnValue;
}

uint8_t symaxProtocol::handleNoBindState(void) {
  uint8_t returnValue = NOT_BOUND;
  unsigned long newTime = millis();
  
  if (!mWireless->rxFlag()) {
    // Scan bind channels
    if ((newTime - mLastSignalTime) > 128) {
      mRfChNum++;
      if (mRfChNum >= FSIZE) mRfChNum = 0;
      mWireless->switchFreq(chans_bind[mRfChNum]);
      mLastSignalTime = newTime;
    }
  } else {
    // Potential bind packet received
    mWireless->resetRxFlag();
    
    while (!mWireless->rxEmpty()) {
      mWireless->readPayload(mFrame, PSIZE);
      
      // Verify bind frame
      if (checksum(mFrame) == mFrame[PSIZE - 1] && 
          mFrame[5] == 0xAA && mFrame[6] == 0xAA) {
        
        // Extract transmitter address (reversed)
        uint8_t txAddr[5];
        for (int k = 0; k < 5; k++) {
          txAddr[k] = mFrame[4 - k];
        }
        
        // Calculate operational channels
        mWireless->setAddress(txAddr, 5);
        setRFChannel(txAddr[0]);
        mRfChNum = 0;
        mRFChanBufs[mRfChNum] += 28;
        mWireless->switchFreq(mRFChanBufs[mRfChNum]);
        
        mLastSignalTime = newTime;
        mState = WAIT_FIRST_SYNCHRO;
        mWireless->flushRx();
        returnValue = BIND_IN_PROGRESS;
        break;
      }
    }
  }
  
  return returnValue;
}

uint8_t symaxProtocol::handleWaitSynchroState(void) {
  mWireless->switchFreq(mRFChanBufs[mRfChNum]);
  unsigned long newTime = millis();
  uint8_t returnValue = BIND_IN_PROGRESS;
  
  // Frequency search during synchronization
  frequencyCounter--;
  if (frequencyCounter <= 0) {
    frequencyCounter = 5;
    currentFrequency++;
    if (currentFrequency >= 80) currentFrequency = 50;
    mWireless->switchFreq(currentFrequency);
    delay(5);
  }
  
  if (mWireless->rxFlag()) {
    mWireless->resetRxFlag();
    bool incrementChannel = false;
    
    while (!mWireless->rxEmpty()) {
      mWireless->readPayload(mFrame, PSIZE);
      
      if (checksum(mFrame) == mFrame[PSIZE - 1]) {
        incrementChannel = true;
        mState = BOUND;
        mLastSignalTime = newTime;
      }
    }
    
    if (incrementChannel) {
      mRfChNum++;
      if (mRfChNum >= FSIZE) mRfChNum = 0;
      mWireless->switchFreq(mRFChanBufs[mRfChNum]);
    }
  }
  
  return returnValue;
}

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

void symaxProtocol::cycleFrequency(void) {
  switch (currentFrequency) {
    case 61: currentFrequency = 64; break;
    case 64: currentFrequency = 72; break;
    case 72: currentFrequency = 75; break;
    case 75: currentFrequency = 61; break;
    default: currentFrequency = 61; break;
  }
  
  if ((currentFrequency - previousFrequency) > 0) {
    for (int i = previousFrequency; i <= currentFrequency; i++) {
      mWireless->switchFreq(i);
    }
  } else if ((currentFrequency - previousFrequency) < 0) {
    mWireless->switchFreq(currentFrequency);
  }
  previousFrequency = currentFrequency;
}

void symaxProtocol::extractData(rx_values_t* rx_value) {
  // Throttle - direct value
  rx_value->throttle = mFrame[0];
  
  // Yaw - convert signed value
  rx_value->yaw = mFrame[2];
  if (rx_value->yaw < 0) {
    rx_value->yaw = 128 - rx_value->yaw;
  }
  
  // Pitch - convert signed value
  rx_value->pitch = mFrame[1];
  if (rx_value->pitch < 0) {
    rx_value->pitch = 128 - rx_value->pitch;
  }
  
  // Roll - convert signed value
  rx_value->roll = mFrame[3];
  if (rx_value->roll < 0) {
    rx_value->roll = 128 - rx_value->roll;
  }
  
  // Trim values (6-bit signed)
  rx_value->trim_yaw = mFrame[6] & 0x3F;
  if (rx_value->trim_yaw >= 32) {
    rx_value->trim_yaw = 32 - rx_value->trim_yaw;
  }
  
  rx_value->trim_pitch = mFrame[5] & 0x3F;
  if (rx_value->trim_pitch >= 32) {
    rx_value->trim_pitch = 32 - rx_value->trim_pitch;
  }
  
  rx_value->trim_roll = mFrame[7] & 0x3F;
  if (rx_value->trim_roll >= 32) {
    rx_value->trim_roll = 32 - rx_value->trim_roll;
  }
  
  // Button states
  rx_value->video = mFrame[4] & 0x80;
  rx_value->picture = mFrame[4] & 0x40;
  rx_value->highspeed = mFrame[5] & 0x80;
  rx_value->flip = mFrame[6] & 0x40;
}
