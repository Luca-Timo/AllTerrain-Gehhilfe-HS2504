# ST-Link Setup Guide for Hoverboard Flashing

## Quick Answer

ST-Link is **already configured** in [platformio.ini](platformio.ini:52-53):
```ini
debug_tool      = stlink
upload_protocol = stlink
```

You don't need to select a COM port - ST-Link is automatically detected via USB.

## How to Flash with ST-Link

### Method 1: PlatformIO in VS Code (Easiest)

1. **Connect ST-Link to Hoverboard:**
   - SWDIO → SWDIO
   - SWCLK → SWCLK (or SWDCLK)
   - GND → GND
   - 3.3V → 3.3V (optional, can power from ST-Link)

2. **Connect ST-Link to Computer:**
   - Plug ST-Link USB into your computer

3. **Upload:**
   - Open VS Code
   - PlatformIO sidebar → Project Tasks → VARIANT_USART → Upload
   - **Or** click the → arrow in the bottom toolbar

### Method 2: Command Line

```bash
# Build and upload
pio run -e VARIANT_USART -t upload

# If you get detection errors, try:
pio run -e VARIANT_USART -t upload --upload-port usb
```

## ST-Link Connection to Hoverboard

### Standard ST-Link V2 Pinout

```
ST-Link V2          Hoverboard Mainboard
----------          ---------------------
3.3V (Pin 1)   →    3.3V (optional)
SWDIO (Pin 2)  →    SWDIO
GND (Pin 3)    →    GND
SWCLK (Pin 4)  →    SWCLK
```

### Hoverboard Debug Header Location

The debug header is usually a **4-pin header** on the mainboard:
- Look for labels: "SWDIO", "SWCLK", "GND", "3.3V"
- Often located near the STM32 chip
- See [mainboard pinout diagram](docs/pictures/mainboard_pinout.png)

## Troubleshooting ST-Link Detection

### Issue: "No ST-Link Found"

**On Windows:**
1. Install ST-Link drivers from STMicroelectronics
2. Or let Windows auto-install via Windows Update
3. Verify in Device Manager under "Universal Serial Bus devices"

**On macOS:**
```bash
# Install ST-Link tools via Homebrew
brew install stlink

# Check if ST-Link is detected
st-info --probe
```

**On Linux:**
```bash
# Install ST-Link tools
sudo apt-get install stlink-tools

# Add udev rules for ST-Link
sudo nano /etc/udev/rules.d/99-stlink.rules
# Add this line:
# SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Check if ST-Link is detected
st-info --probe
```

### Issue: "Target Not Found" or "No Target Connected"

**Possible causes:**
1. **Loose wiring** - Check all 4 connections (especially GND!)
2. **Wrong pins** - Verify SWDIO/SWCLK are correct
3. **Hoverboard not powered** - Some boards need main power on
4. **Protection enabled** - Read protection may be enabled

**Solutions:**

**Try with power on:**
```bash
# Power the hoverboard from battery while flashing
# ST-Link should still work via USB
```

**Check connection:**
```bash
# Verify ST-Link can see the chip
st-info --probe

# Expected output:
# Found 1 stlink programmers
# version:    V2J45S7
# serial:     xxxxxxxxxxxx
# flash:      262144 (0x40000)
# sram:       65536 (0x10000)
# chipid:     0x0414
```

**If read protection is enabled:**
```bash
# WARNING: This will erase the entire flash!
st-flash --connect-under-reset erase
```

### Issue: Upload Fails with "Error: init mode failed"

**Solution 1: Connect Under Reset**

Edit [platformio.ini](platformio.ini) and add to `VARIANT_USART` section:
```ini
upload_flags =
    --connect-under-reset
```

**Solution 2: Hold Reset During Upload**
1. Press and hold the RESET button on the hoverboard
2. Start the upload
3. Release RESET when upload begins

**Solution 3: Specify Upload Options**

Add to [platformio.ini](platformio.ini):
```ini
upload_flags =
    --reset
    --halt
```

## Advanced Configuration Options

### If Auto-Detection Fails

You can manually specify ST-Link settings in [platformio.ini](platformio.ini):

```ini
[env:VARIANT_USART]
platform        = ststm32
framework       = stm32cube
board           = genericSTM32F103RC
debug_tool      = stlink
upload_protocol = stlink

; Force specific ST-Link interface
upload_flags =
    --interface=swd
    --target=stm32f1x

; If you have multiple ST-Links, specify serial number
; upload_flags =
;     --serial=48FF6E064989555346281487

; For stubborn connections
; upload_flags =
;     --connect-under-reset
;     --halt
```

## Verify Your Setup

### Step 1: Check ST-Link Hardware

```bash
# List all USB devices
pio device list

# You should see ST-Link listed
```

### Step 2: Test Connection

```bash
# Try to read chip info
st-info --chipid

# Expected: 0x0414 (STM32F103RC) or 0x0430 (GD32F103RC)
```

### Step 3: Test Flash

```bash
# Just compile (don't upload)
pio run -e VARIANT_USART

# If build succeeds, try upload
pio run -e VARIANT_USART -t upload
```

## Alternative: OpenOCD Upload

If ST-Link continues to have issues, you can use OpenOCD:

```ini
[env:VARIANT_USART]
upload_protocol = stlink
upload_flags =
    --openocd-backend
```

## Alternative: Serial Bootloader (No ST-Link Needed!)

STM32F103 has a built-in bootloader accessible via UART:

### Requirements:
- USB-to-Serial adapter (3.3V!)
- BOOT0 jumper or button on hoverboard

### Steps:

1. **Set BOOT0 = HIGH** (connect BOOT0 pin to 3.3V)
2. **Power on** hoverboard
3. **Connect USB-Serial:**
   - TX → PA9 (USART1 TX)
   - RX → PA10 (USART1 RX)
   - GND → GND

4. **Change platformio.ini:**
```ini
[env:VARIANT_USART]
upload_protocol = serial
upload_port = /dev/ttyUSB0  ; or COM3 on Windows
```

5. **Upload:**
```bash
pio run -e VARIANT_USART -t upload
```

6. **Set BOOT0 = LOW** and reset to run firmware

## Expected Upload Output

**Successful upload looks like:**
```
Configuring upload protocol...
AVAILABLE: blackmagic, cmsis-dap, jlink, serial, stlink
CURRENT: upload_protocol = stlink
Uploading .pio/build/VARIANT_USART/firmware.elf
xPack Open On-Chip Debugger 0.12.0+dev (2023-01-30-2028)
Licensed under GNU GPL v2
Info : auto-selecting first available session transport "hla_swd".
Info : The selected transport took over low-level target control.
Info : clock speed 1000 kHz
Info : STLINK V2J45S7 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.261514
Info : [stm32f1x.cpu] Cortex-M3 r1p1 processor detected
Info : [stm32f1x.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for stm32f1x.cpu on 3333
Info : [stm32f1x.cpu] external reset detected
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
========================= [SUCCESS] Took 12.34 seconds =========================
```

## Quick Reference Commands

```bash
# Upload firmware
pio run -e VARIANT_USART -t upload

# Erase chip
st-flash erase

# Read memory
st-flash read dump.bin 0x8000000 0x40000

# Write firmware manually
st-flash write .pio/build/VARIANT_USART/firmware.bin 0x8000000

# Check ST-Link version
st-info --version

# Probe for connected device
st-info --probe
```

## Summary

✅ **ST-Link is already configured** in your platformio.ini
✅ **Just connect and upload** - no port selection needed
✅ **PlatformIO handles everything** automatically

If you still have issues, the most common problem is **loose wiring** on the debug header.

---

**Ready to flash?** Just run: `pio run -e VARIANT_USART -t upload`
