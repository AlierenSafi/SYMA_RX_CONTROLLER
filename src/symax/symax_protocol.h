/*
  symax_protocol.h - SYMA RC Protocol Handler
  
  Copyright (C) 2014 Alexandre Clienti
  Copyright (C) 2016 Suxsem
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  DESCRIPTION:
  Implements the SYMA 2.4GHz RC protocol for X5C-1, X5SW, X11, X11C, X12
  transmitters. Handles binding procedure and continuous data reception.
  
  PROTOCOL DETAILS:
  - Frequency hopping on 4 channels
  - 250kbps data rate
  - 16-byte packets with checksum
  - 5-byte address
  - Bind address: 0xAB 0xAC 0xAD 0xAE 0xAF
  
  BIND PROCEDURE:
  1. Receiver scans bind channels for transmitter
  2. On valid bind packet, extracts transmitter ID
  3. Calculates hopping channels from ID
  4. Switches to operational channels
*/

#ifndef SYMAX_PROTOCOL_H_
#define SYMAX_PROTOCOL_H_

#include "Arduino.h"
#include "../nrf24/nrf24l01p.h"

// Packet and frequency definitions
#define PSIZE 16  // Packet size in bytes
#define FSIZE 4   // Number of frequency hop channels

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------

/*
  Received transmitter values structure
  All stick values are mapped to gamepad axes in main sketch
*/
typedef struct __attribute__((__packed__)) {
  uint8_t throttle;    // 0-255 (0=min, 255=max)
  int8_t yaw;          // 127 to -127 (127=left, -127=right)
  int8_t pitch;        // 127 to -127 (127=forward, -127=backward)
  int8_t roll;         // 127 to -127 (127=left, -127=right)
  int8_t trim_yaw;     // -31 to 31
  int8_t trim_pitch;   // -31 to 31
  int8_t trim_roll;    // -31 to 31
  bool video;          // Video button state
  bool picture;        // Picture button state
  bool highspeed;      // High speed mode toggle
  bool flip;           // Flip button state
} rx_values_t;

// -----------------------------------------------------------------------------
// State Enumerations
// -----------------------------------------------------------------------------

enum rxState {
  NO_BIND = 0,           // Initial state, searching for transmitter
  WAIT_FIRST_SYNCHRO,    // Bind received, waiting for first data packet
  BOUND                  // Fully connected and receiving data
};

enum rxReturn {
  BOUND_NEW_VALUES = 0,  // New data received successfully
  BOUND_NO_VALUES,       // Connected but no new data this cycle
  NOT_BOUND,             // Not bound to any transmitter
  BIND_IN_PROGRESS,      // Binding procedure in progress
  UNKNOWN                // Reserved
};

// -----------------------------------------------------------------------------
// SYMA Protocol Class
// -----------------------------------------------------------------------------

class symaxProtocol {
  public:
    symaxProtocol();
    ~symaxProtocol();
    
    // Initialize with NRF24L01+ instance
    void init(nrf24l01p* wireless);
    
    // Main state machine - call repeatedly in loop
    // Returns connection status, fills rx_value on success
    uint8_t run(rx_values_t* rx_value);

  protected:
    uint8_t checksum(uint8_t* data);
    void setRFChannel(uint8_t address);
    
    // State handlers
    uint8_t handleBoundState(rx_values_t* rx_value);
    uint8_t handleNoBindState(void);
    uint8_t handleWaitSynchroState(void);
    
    // Helper functions
    void cycleFrequency(void);
    void extractData(rx_values_t* rx_value);
    
    nrf24l01p* mWireless;
    uint8_t mRfChNum;
    uint8_t mFrame[PSIZE];
    uint8_t mRFChanBufs[FSIZE];
    uint8_t mState;
    unsigned long mLastSignalTime;
};

#endif /* SYMAX_PROTOCOL_H_ */
