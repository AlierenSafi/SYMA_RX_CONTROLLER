# SYMA RC Controller to USB Gamepad Adapter

A professional-grade firmware that converts SYMA X5C-1/X5SW 2.4GHz RC transmitter signals into standard USB HID gamepad input. This project enables the use of SYMA drone transmitters as PC game controllers for simulators and games.

## Overview

This firmware runs on an Arduino Mega 2560 with an NRF24L01+ 2.4GHz transceiver module. It receives RF signals from SYMA transmitters and emulates a USB gamepad using the MegaJoy library.

### Features

- Full USB HID gamepad emulation (64 buttons, 12 analog axes, 2 D-pads)
- Support for SYMA X5C-1, X5SW, X11, X11C, X12 transmitters
- Real-time frequency hopping synchronization
- Automatic binding procedure
- Signal loss detection and safe fail-state
- 100Hz USB update rate
- Professional-grade code architecture with modular design

## Hardware Requirements

### Required Components

| Component | Specification | Purpose |
|-----------|--------------|---------|
| Arduino Mega 2560 | ATmega2560 + ATmega16U2 | Main controller and USB interface |
| NRF24L01+ Module | 2.4GHz transceiver | RF communication with transmitter |
| SYMA Transmitter | X5C-1, X5SW, X11, X11C, X12 | Input device |

### Wiring Diagram

```
NRF24L01+ Module    Arduino Mega 2560
----------------    -----------------
VCC                 3.3V  (NOT 5V)
GND                 GND
CE                  Pin 10
CSN                 Pin 9
SCK                 Pin 52
MOSI                Pin 51
MISO                Pin 50
IRQ                 Not connected
```

### Important Hardware Notes

1. **Power Supply**: NRF24L01+ requires 3.3V. Do not connect to 5V.
2. **Logic Levels**: Arduino Mega 2560 operates at 5V logic. Use level shifters if your NRF24L01+ module is not 5V tolerant.
3. **Decoupling**: Add 10uF electrolytic and 100nF ceramic capacitors across VCC-GND near the NRF24L01+ module for stable operation.
4. **Antenna**: Keep antenna away from metal objects and other RF sources.

## Software Architecture

### Project Structure

```
SYMA_RX_CONTROLLER/
├── SYMA_RX_CONTROLLER.ino      # Main application sketch
├── src/
│   ├── nrf24/
│   │   ├── nrf24l01p.h         # NRF24L01+ driver header
│   │   └── nrf24l01p.cpp       # NRF24L01+ driver implementation
│   ├── symax/
│   │   ├── symax_protocol.h    # SYMA protocol handler header
│   │   └── symax_protocol.cpp  # SYMA protocol implementation
│   └── megajoy/
│       └── MegaJoy.h           # USB gamepad library
├── firmware/
│   └── MegaJoy.hex             # ATmega16U2 USB firmware
├── docs/
│   └── hardware_setup.md       # Detailed hardware guide
├── README.md                   # This file
├── LICENSE                     # MIT License
└── .gitignore                  # Git ignore rules
```

### Module Descriptions

#### NRF24L01+ Driver (`src/nrf24/`)

Low-level SPI driver for the NRF24L01+ 2.4GHz transceiver. Handles register access, packet transmission/reception, and RF configuration.

**Key Features:**
- SPI communication at 8MHz
- Configurable RF channel (2400-2525 MHz)
- Adjustable output power (-18dBm to 0dBm)
- Multiple data rates (250kbps, 1Mbps, 2Mbps)
- 5-byte address width
- 16-byte payload support

#### SYMA Protocol Handler (`src/symax/`)

Implements the SYMA 2.4GHz RC protocol for X5C-1, X5SW, X11, X11C, X12 transmitters. Manages binding procedure and continuous data reception.

**Protocol Details:**
- Frequency hopping on 4 channels
- 250kbps data rate
- 16-byte packets with XOR checksum
- 5-byte transmitter address
- Fixed bind address: `0xAB 0xAC 0xAD 0xAE 0xAF`

**State Machine:**
1. `NO_BIND`: Scanning for transmitter on bind channels
2. `WAIT_FIRST_SYNCHRO`: Bind received, synchronizing operational channels
3. `BOUND`: Fully connected and receiving data

#### MegaJoy Library (`src/megajoy/`)

Enables Arduino Mega 2560 to act as a native USB HID gamepad. Communicates with the ATmega16U2 USB interface chip via serial protocol at 38400 baud.

**Limitations:**
- Pins 0 and 1 reserved for serial communication
- Timer0 used for USB communication timing
- Cannot use Serial for debugging

## Installation

### Prerequisites

- Arduino IDE 1.8.x or 2.x
- Arduino Mega 2560 board support
- SYMA transmitter (X5C-1, X5SW, X11, X11C, or X12)
- NRF24L01+ module

### Step 1: Flash ATmega16U2 USB Firmware

The Arduino Mega 2560's USB interface chip (ATmega16U2) must be flashed with the MegaJoy firmware to enable gamepad functionality.

**Windows:**
```batch
# Put ATmega16U2 in DFU mode (short pins or use reset button)
# Flash firmware
TurnIntoAnArduino.bat
```

**Linux/macOS:**
```bash
# Put ATmega16U2 in DFU mode
sudo ./TurnIntoAnArduino.sh
```

**Manual Method:**
1. Short the ICSP header pins to enter DFU mode
2. Use `dfu-programmer` or Atmel Flip to flash `firmware/MegaJoy.hex`
3. Reset the board

### Step 2: Upload Main Sketch

1. Open `SYMA_RX_CONTROLLER.ino` in Arduino IDE
2. Select Board: `Tools > Board > Arduino Mega 2560`
3. Select Port: `Tools > Port > (your COM port)`
4. Click Upload

### Step 3: Bind Transmitter

1. Power on the Arduino Mega 2560
2. Turn on the SYMA transmitter
3. The transmitter will automatically bind within 1-2 seconds
4. LED indicators (if connected) show bind status

## Controller Mapping

### Analog Axes

| Axis Index | SYMA Control | Range | Gamepad Function |
|------------|--------------|-------|------------------|
| 0 | Left Stick X (Yaw) | 0-1023 | Left Stick X |
| 1 | Left Stick Y (Throttle) | 0-1023 | Left Stick Y |
| 2 | Right Stick X (Roll) | 0-1023 | Right Stick X |
| 3 | Right Stick Y (Pitch) | 0-1023 | Right Stick Y |
| 4 | Yaw Trim | 0-1023 | Axis 4 |
| 5 | Pitch Trim | 0-1023 | Axis 5 |
| 6 | Roll Trim | 0-1023 | Axis 6 |

### Buttons

| Button | SYMA Control | Windows ID | PS3 Equivalent |
|--------|--------------|------------|----------------|
| 1 | Video Button | 1 | Square |
| 2 | Picture Button | 2 | Cross |
| 3 | High Speed | 3 | Circle |
| 4 | Flip | 4 | Triangle |

### Windows Button Mapping Reference

| Windows ID | PS3 Button |
|------------|------------|
| 1 | Square |
| 2 | Cross (X) |
| 3 | Circle |
| 4 | Triangle |
| 5 | L1 |
| 6 | R1 |
| 7 | L2 |
| 8 | R2 |
| 9 | Select |
| 10 | Start |
| 11 | L3 |
| 12 | R3 |
| 13 | Home (PS) |

## Configuration

### Update Interval

The default USB update interval is 10ms (100Hz). Modify in `SYMA_RX_CONTROLLER.ino`:

```cpp
const unsigned long UPDATE_INTERVAL_MS = 10;  // 10ms = 100Hz
```

### RF Power Level

Adjust NRF24L01+ output power in `setup()`:

```cpp
radioModule.setPwr(PWRLOW);     // -18dBm (lowest, safest)
radioModule.setPwr(PWRMEDIUM);  // -12dBm
radioModule.setPwr(PWRHIGH);    // -6dBm
radioModule.setPwr(PWRMAX);     // 0dBm (highest range)
```

### Axis Calibration

Modify axis ranges in `updateGamepadFromTransmitter()`:

```cpp
const int AXIS_MIN = 0;
const int AXIS_MAX = 1023;
const int AXIS_CENTER = 512;
```

## Troubleshooting

### No Binding

**Symptoms:** Transmitter does not connect, `NOT_BOUND` status persists.

**Solutions:**
1. Verify NRF24L01+ wiring (especially 3.3V power)
2. Check that transmitter is powered on
3. Ensure transmitter is within 1 meter during binding
4. Verify transmitter model compatibility
5. Add decoupling capacitors (10uF + 100nF)

### Intermittent Connection

**Symptoms:** Connection drops frequently or signal is weak.

**Solutions:**
1. Increase RF power: `radioModule.setPwr(PWRMAX)`
2. Check antenna placement
3. Reduce distance between transmitter and receiver
4. Avoid interference sources (WiFi, Bluetooth, microwave ovens)
5. Verify stable 3.3V power supply

### USB Not Recognized

**Symptoms:** Windows does not detect gamepad.

**Solutions:**
1. Verify ATmega16U2 firmware is flashed correctly
2. Check USB cable (use data cable, not charge-only)
3. Try different USB port
4. Check Device Manager for driver issues
5. Re-flash MegaJoy firmware

### Erratic Axis Behavior

**Symptoms:** Axes jump or have incorrect values.

**Solutions:**
1. Calibrate transmitter sticks
2. Check for loose connections
3. Verify correct axis mapping in code
4. Add deadzone handling if needed

## Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/yourusername/SYMA_RX_CONTROLLER.git
cd SYMA_RX_CONTROLLER

# Open in Arduino IDE
# File > Open > SYMA_RX_CONTROLLER.ino

# Compile and upload
# Select board: Arduino Mega 2560
# Click Upload
```

### Adding Custom Features

**Example: Add deadzone to throttle:**

```cpp
int16_t applyDeadzone(int16_t value, int16_t deadzone) {
  if (abs(value - AXIS_CENTER) < deadzone) {
    return AXIS_CENTER;
  }
  return value;
}

// In updateGamepadFromTransmitter():
gamepadData.analogAxisArray[1] = applyDeadzone(
  map(transmitterData.throttle, 0, 255, AXIS_MIN, AXIS_MAX),
  50  // 50-unit deadzone
);
```

## Technical Specifications

### Performance

- USB Update Rate: 100 Hz
- RF Data Rate: 250 kbps
- Latency: <20ms end-to-end
- Range: 50-100 meters (line of sight)

### Power Consumption

- Arduino Mega 2560: ~50mA @ 5V
- NRF24L01+: ~12mA @ 3.3V (RX mode, PWRLOW)
- Total: ~62mA typical

### Protocol Timing

- Bind channel dwell time: 128ms
- Operational channel dwell time: variable
- Packet interval: ~4ms per channel

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

- **symaxrx library**: MIT License (Alexandre Clienti, Suxsem)
- **MegaJoy library**: MIT License (Alan Chatham, RMIT Exertion Games Lab)

## Acknowledgments

- Alexandre Clienti and Suxsem for the original symaxrx library
- Alan Chatham and RMIT Exertion Games Lab for the MegaJoy library
- DeviationTX team for SYMA protocol reverse engineering
- hexfet from DeviationTX forum for invaluable support

## Support

For issues, questions, or contributions, please use the GitHub issue tracker.

## Disclaimer

This project is for educational and personal use. SYMA is a trademark of SYMA Industrial Co., Ltd. This project is not affiliated with or endorsed by SYMA.

Use at your own risk. The authors are not responsible for any damage to equipment or injury resulting from the use of this software.
