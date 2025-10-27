# Quick Start Guide - UART Torque Control

## TL;DR - Get Running Fast

### 1. Select PlatformIO Environment

**Environment to use:** `VARIANT_USART`

**In VS Code:**
- Bottom toolbar → Select environment → `VARIANT_USART`

**In Command Line:**
```bash
pio run -e VARIANT_USART -t upload
```

### 2. Hardware Connections

```
Hoverboard Right Cable        ESP32-S3
-----------------------       --------
GND (Black - Pin 1)      →    GND
TX (PA2 - Pin 3)         →    RX (GPIO 16)
RX (PA3 - Pin 4)         →    TX (GPIO 17)
15V (Red - Pin 2)        →    ⚠️ LEAVE DISCONNECTED!
```

### 3. Build & Flash

**PlatformIO GUI (VS Code):**
1. Project Tasks → VARIANT_USART → Build
2. Project Tasks → VARIANT_USART → Upload

**Command Line:**
```bash
pio run -e VARIANT_USART          # Build only
pio run -e VARIANT_USART -t upload # Build & Upload
```

### 4. Upload ESP32 Code

Flash [ESP32_UART_Control_Example.ino](ESP32_UART_Control_Example.ino) to your ESP32-S3.

### 5. Test

1. Power on hoverboard
2. Open serial monitor on ESP32 (115200 baud)
3. You should see feedback data streaming
4. Motors will ramp slowly (test code)

## Configuration Summary

### Changes Made to config.h

| Setting | Value | Location |
|---------|-------|----------|
| Variant | `VARIANT_USART` | Line 14 |
| Control Type | `FOC_CTRL` | Line 150 |
| Control Mode | `TRQ_MODE` | Line 151 |
| UART Port | `CONTROL_SERIAL_USART3` | Line 319 |
| Feedback | `FEEDBACK_SERIAL_USART3` | Line 320 |

### platformio.ini

| Setting | Value |
|---------|-------|
| default_envs | `VARIANT_USART` |
| Environment | `[env:VARIANT_USART]` |
| Board | `genericSTM32F103RC` |
| Upload | `stlink` |

## Communication Protocol Quick Reference

### Send Command (ESP32 → Hoverboard)

```cpp
struct {
  uint16_t start = 0xABCD;
  int16_t  steer;      // -1000 to +1000
  int16_t  speed;      // -1000 to +1000 (torque!)
  uint16_t checksum;   // start ^ steer ^ speed
} Command;
```

### Receive Feedback (Hoverboard → ESP32)

```cpp
struct {
  uint16_t start = 0xABCD;
  int16_t  cmd1, cmd2;
  int16_t  speedR_meas, speedL_meas;  // RPM
  int16_t  batVoltage;                // *100
  int16_t  boardTemp;                 // *10
  uint16_t cmdLed;
  uint16_t checksum;
} Feedback;
```

**Values:**
- Battery: `batVoltage / 100.0` = Volts
- Temperature: `boardTemp / 10.0` = °C
- Speed: `speedX_meas` = RPM

## Torque Control

### What the Speed Value Means in TRQ_MODE

| Value | Meaning |
|-------|---------|
| 0 | Freewheel (no torque) |
| +500 | Forward torque (medium) |
| +1000 | Forward torque (maximum) |
| -500 | Reverse torque (medium) |
| -1000 | Reverse torque (maximum) |

### Safe Testing Values

Start with these:
- **Low torque:** ±100 to ±200
- **Medium torque:** ±300 to ±500
- **High torque:** ±600 to ±1000

## Troubleshooting Quick Fixes

| Problem | Solution |
|---------|----------|
| Won't compile | Select `VARIANT_USART` environment |
| Upload fails | Check ST-Link connection |
| No motor response | Verify TX/RX swap, check GND |
| Invalid checksum | Verify START_FRAME = 0xABCD |
| Motors timeout | Send commands faster (< 200ms) |
| ESP32 won't power | Check you didn't connect 15V! |

## Command Line Cheat Sheet

```bash
# Build
pio run -e VARIANT_USART

# Build and upload
pio run -e VARIANT_USART -t upload

# Clean build
pio run -e VARIANT_USART -t clean

# Monitor serial (if debugging enabled)
pio device monitor -b 115200

# Build all variants (optional)
pio run
```

## Important Safety Reminders

⚠️ **Before first power-on:**
- [ ] Verify wiring (especially no 15V to ESP32!)
- [ ] Double-check GND connection
- [ ] Elevate hoverboard (wheels off ground)
- [ ] Start with LOW torque values (±100)
- [ ] Keep emergency power-off ready

## Next Steps

After successful testing:

1. **Adjust Parameters** in [config.h](Inc/config.h):
   - `I_MOT_MAX` - Increase for more torque
   - `ELECTRIC_BRAKE_ENABLE` - Add braking at zero torque
   - `STANDSTILL_HOLD_ENABLE` - Hold position when stopped

2. **Customize ESP32 Code**:
   - Add your control logic
   - Integrate sensors (IMU, encoders, etc.)
   - Implement safety features

3. **Monitor Performance**:
   - Watch battery voltage
   - Check temperature
   - Verify current draw

## Files Reference

- **Firmware Config:** [Inc/config.h](Inc/config.h)
- **PlatformIO:** [platformio.ini](platformio.ini)
- **ESP32 Example:** [ESP32_UART_Control_Example.ino](ESP32_UART_Control_Example.ino)
- **Full Documentation:** [UART_TORQUE_CONTROL_SETUP.md](UART_TORQUE_CONTROL_SETUP.md)

---

**Ready to build?** Run: `pio run -e VARIANT_USART -t upload`
