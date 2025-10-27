# UART Torque Control Setup Guide

## Overview
This guide explains the configuration for controlling the hoverboard motors via UART from an ESP32-S3 using **Torque Control Mode**.

## Hardware Configuration

### Hoverboard Side (Right Sensor Cable - USART3)

**Pinout (4-pin connector):**
- Pin 1: GND (Black) → Connect to ESP32-S3 GND
- Pin 2: 12/15V (Red) → **DO NOT CONNECT!** (Will damage ESP32)
- Pin 3: TX (PA2) → Connect to ESP32-S3 RX
- Pin 4: RX (PA3) → Connect to ESP32-S3 TX

**Note:** USART3 pins are **5V tolerant**, making them safe for direct connection to ESP32-S3 (3.3V logic).

### ESP32-S3 Side

**Recommended connections:**
```
Hoverboard          ESP32-S3
---------           --------
GND (Pin 1)    →    GND
TX (PA2)       →    RX (GPIO 16 or your choice)
RX (PA3)       →    TX (GPIO 17 or your choice)
15V (Pin 2)    →    NOT CONNECTED!
```

## Firmware Configuration

### Changes Made to config.h

#### 1. Variant Selection (Line 14)
```c
#define VARIANT_USART       // Variant for Serial control via USART3 input
```

#### 2. Control Type Selection (Line 151)
```c
#define CTRL_TYP_SEL    FOC_CTRL        // Field Oriented Control
#define CTRL_MOD_REQ    TRQ_MODE        // TORQUE control mode
```

#### 3. USART Configuration (Lines 319-320)
```c
#define CONTROL_SERIAL_USART3  0    // Enable control on USART3 (right cable)
#define FEEDBACK_SERIAL_USART3      // Enable feedback on USART3
```

### Current Motor Parameters

**Current Limits:**
- `I_MOT_MAX`: 15A (Maximum single motor current)
- `I_DC_MAX`: 17A (Maximum DC link current)
- `N_MOT_MAX`: 1000 RPM (Maximum motor speed)

**Field Weakening:**
- `FIELD_WEAK_ENA`: 0 (Disabled by default)
- Can be enabled for higher speeds if needed

**Optional Features (currently disabled):**
- `STANDSTILL_HOLD_ENABLE`: Holds position at standstill
- `ELECTRIC_BRAKE_ENABLE`: Adds braking when torque = 0

## Communication Protocol

### Command Structure (ESP32 → Hoverboard)

```c
typedef struct {
   uint16_t start;      // 0xABCD (Start frame)
   int16_t  steer;      // -1000 to +1000 (steering command)
   int16_t  speed;      // -1000 to +1000 (torque in TRQ_MODE)
   uint16_t checksum;   // XOR of start ^ steer ^ speed
} SerialCommand;
```

**Size:** 8 bytes
**Baud Rate:** 115200
**Update Rate:** Recommended 100ms (10 Hz) or faster

### Feedback Structure (Hoverboard → ESP32)

```c
typedef struct {
   uint16_t start;         // 0xABCD (Start frame)
   int16_t  cmd1;          // Echo of command 1
   int16_t  cmd2;          // Echo of command 2
   int16_t  speedR_meas;   // Right motor speed [RPM]
   int16_t  speedL_meas;   // Left motor speed [RPM]
   int16_t  batVoltage;    // Battery voltage * 100
   int16_t  boardTemp;     // Board temperature * 10
   uint16_t cmdLed;        // LED command
   uint16_t checksum;      // XOR of all fields
} SerialFeedback;
```

**Size:** 18 bytes

### Checksum Calculation

**Command checksum:**
```c
checksum = start ^ steer ^ speed;
```

**Feedback checksum:**
```c
checksum = start ^ cmd1 ^ cmd2 ^ speedR_meas ^ speedL_meas
         ^ batVoltage ^ boardTemp ^ cmdLed;
```

## Torque Control Mode Details

### What is Torque Mode?

In `TRQ_MODE`, the `speed` field in the command structure represents the **torque request** instead of speed:

- **Value Range:** -1000 to +1000
- **Behavior:**
  - `0`: Motors freewheel (no torque applied)
  - `Positive values`: Forward torque
  - `Negative values`: Reverse torque
- **Motor Response:** Torque is applied proportionally to the command value

### Advantages of Torque Mode

1. **Natural feel:** Motor freely spins when no torque is commanded
2. **Smooth control:** Direct torque control feels more responsive
3. **Energy efficient:** Motors don't fight against movement
4. **Ideal for:** Walking assistance devices, where natural movement is important

## ESP32-S3 Example Usage

See `ESP32_UART_Control_Example.ino` for a complete working example.

### Basic Usage

```cpp
// In your loop, send torque commands every 100ms
void loop() {
  int16_t steer = 0;        // -1000 to +1000 (left/right)
  int16_t torque = 200;     // -1000 to +1000 (forward/reverse)

  Send(steer, torque);
  delay(100);
}
```

### Reading Feedback

```cpp
void loop() {
  Receive();  // Process incoming feedback

  // Access feedback data
  float batteryVolts = Feedback.batVoltage / 100.0;
  float tempCelsius = Feedback.boardTemp / 10.0;
  int16_t speedLeft = Feedback.speedL_meas;  // RPM
  int16_t speedRight = Feedback.speedR_meas; // RPM
}
```

## Building and Flashing

### Prerequisites
- STM32 development toolchain
- ST-Link programmer (or compatible)
- PlatformIO or STM32CubeIDE

### Build Instructions

**Using PlatformIO (Recommended):**

The environment is already configured. Use one of these methods:

**Option 1: Command Line**
```bash
cd AllTerrain-Gehhilfe-HS2504
pio run -e VARIANT_USART
pio run -e VARIANT_USART -t upload
```

**Option 2: VS Code PlatformIO Extension**
1. Open the project in VS Code
2. Click on the PlatformIO icon in the sidebar
3. Under "Project Tasks" → "VARIANT_USART"
4. Click "Build" to compile
5. Click "Upload" to flash to the hoverboard

**Option 3: Default Environment (already set)**
```bash
pio run          # Builds VARIANT_USART (default)
pio run -t upload # Builds and uploads
```

**Using Make:**
```bash
make
# Flash using your preferred method (st-flash, openocd, etc.)
```

### Environment Details

The `VARIANT_USART` environment ([platformio.ini](platformio.ini:48-67)) is configured with:
- **Platform:** STM32
- **Board:** STM32F103RC (64KB RAM, 256KB Flash)
- **Upload Protocol:** ST-Link
- **Build Flags:** Includes `-D VARIANT_USART` to enable the variant

## Testing Procedure

1. **Power Off:** Ensure hoverboard is powered off
2. **Connect:** Wire ESP32-S3 to hoverboard USART3 (right cable)
3. **Flash Firmware:** Upload configured firmware to hoverboard
4. **Upload ESP32 Code:** Upload example code to ESP32-S3
5. **Power On:** Turn on hoverboard
6. **Monitor:** Open serial monitor on ESP32-S3 (115200 baud)
7. **Test:** Start with low torque values (±100) and gradually increase

## Safety Notes

⚠️ **IMPORTANT SAFETY WARNINGS:**

1. **Never connect 15V wire** from sensor cable to ESP32-S3
2. **Start with low torque values** during initial testing
3. **Always have emergency stop** mechanism ready
4. **Test in a safe environment** with the board elevated
5. **Monitor battery voltage** - stop if voltage drops too low
6. **Monitor temperature** - stop if board overheats
7. **Respect current limits** - don't exceed I_MOT_MAX settings

## Troubleshooting

### No Communication
- Check wiring (TX→RX, RX→TX)
- Verify baud rate (115200)
- Confirm USART3 is enabled in config.h
- Check GND connection

### Invalid Checksums
- Ensure START_FRAME is 0xABCD
- Verify checksum calculation
- Check for noise on signal lines

### Motors Not Responding
- Verify CTRL_MOD_REQ is set to TRQ_MODE
- Check torque values are in range (-1000 to +1000)
- Ensure command rate is at least 5 Hz (every 200ms)
- Check TIMEOUT setting in config.h (default 20 commands)

### Unexpected Behavior
- Enable DEBUG_SERIAL_PROTOCOL for detailed diagnostics
- Monitor feedback data for error conditions
- Check battery voltage (low voltage = reduced performance)
- Verify motor current limits are appropriate

## Advanced Configuration

### Adjusting Current Limits

In `Inc/config.h`:
```c
#define I_MOT_MAX       15    // Increase for more torque (max ~25A tested)
#define I_DC_MAX        17    // Keep 2A above I_MOT_MAX
```

### Enabling Electric Brake

To add braking when torque = 0:
```c
#define ELECTRIC_BRAKE_ENABLE
#define ELECTRIC_BRAKE_MAX    100   // Braking strength
#define ELECTRIC_BRAKE_THRES  120   // Engagement threshold
```

### Enabling Standstill Hold

To hold position at zero speed:
```c
#define STANDSTILL_HOLD_ENABLE
```

### Debug Serial Protocol

For advanced parameter control:
```c
#define DEBUG_SERIAL_PROTOCOL    // Enable parameter adjustment via UART
```

## Additional Resources

- **Original Firmware:** https://github.com/EFeru/hoverboard-firmware-hack-FOC
- **Hardware Schematics:** `/docs/20150722_hoverboard_sch.pdf`
- **FOC Controller:** https://github.com/EFeru/bldc-motor-control-FOC
- **Arduino Example:** `/Arduino/hoverserial/hoverserial.ino`

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review the original firmware wiki
3. Test with the Arduino example first to verify hardware

---

**Configuration Date:** 2025-10-27
**Firmware Version:** FOC-based hoverboard firmware
**Target Platform:** STM32F103RCT6 / GD32F103RCT6
