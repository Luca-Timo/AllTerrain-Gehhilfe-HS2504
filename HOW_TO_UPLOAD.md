# How to Upload - ST-Link Has NO Port!

## Important: ST-Link Does NOT Use a Serial Port!

**You will NOT see an "upload port" option for ST-Link** because it communicates directly via USB using the SWD protocol, not through a COM/serial port.

## What You Need to Do

### Step 1: Connect ST-Link Hardware

**Physical connections:**
```
ST-Link USB → Your Computer (plug it in)

ST-Link Pins → Hoverboard Debug Header:
  SWDIO → SWDIO
  SWCLK → SWCLK
  GND   → GND
  3.3V  → 3.3V (optional)
```

### Step 2: Just Click Upload!

**In VS Code PlatformIO:**

There are NO port selections needed. Just:

1. **Bottom toolbar** → Look for the **→** (right arrow) icon
2. **Click the arrow** → This uploads automatically
3. PlatformIO will find the ST-Link automatically

**OR:**

1. **PlatformIO icon** (left sidebar, looks like an alien/ant head)
2. **Project Tasks** → **VARIANT_USART** → **Upload**
3. Click it!

### Step 3: Watch the Output

You'll see output in the terminal like:
```
Uploading .pio/build/VARIANT_USART/firmware.elf
xPack Open On-Chip Debugger...
Info : STLINK V2J45S7 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.261514
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
========================= [SUCCESS] =========================
```

## VS Code Interface Guide

### Bottom Toolbar (Most Important)

Look at the **very bottom** of VS Code window. You'll see icons:

```
[✓] [→] [•] [🗑] [⚙] [🔌] VARIANT_USART
 |   |   |   |    |    |        |
 |   |   |   |    |    |        └─ Environment selector
 |   |   |   |    |    └─ Serial Monitor (NOT for upload!)
 |   |   |   |    └─ PlatformIO Settings
 |   |   |   └─ Clean
 |   |   └─ Test
 |   └─ Upload (USE THIS!)
 └─ Build
```

**The → (Upload) button is what you need!**

### What About "Serial Monitor"?

The 🔌 (Serial Monitor) in the bottom toolbar is ONLY for:
- **After** you upload the firmware
- **Debugging** via UART (if you enable DEBUG_SERIAL in config.h)
- **NOT** for uploading with ST-Link!

## If You Don't See ST-Link

### Check 1: Is ST-Link Plugged In?

Physically check:
- ST-Link USB cable is plugged into your computer
- ST-Link LED should be lit (usually red)

### Check 2: System Detection (macOS)

Open Terminal and run:
```bash
# See if macOS detects the ST-Link
system_profiler SPUSBDataType | grep -A 10 "STM\|STLink"
```

You should see something like:
```
STMicroelectronics ST-LINK/V2:
  Product ID: 0x3748
  Vendor ID: 0x0483
```

### Check 3: Install ST-Link Software (if needed)

**macOS:**
```bash
brew install stlink

# Test detection
st-info --probe
```

If you see "Found 1 stlink programmers" - you're good!

### Check 4: PlatformIO Extension Installed?

In VS Code:
1. Click **Extensions** icon (left sidebar)
2. Search for "PlatformIO"
3. Should show **PlatformIO IDE** as installed
4. If not, install it and reload VS Code

## Common Misconceptions

### ❌ WRONG: "I need to select a COM port"
**ST-Link doesn't use COM ports!** It uses USB direct communication.

### ❌ WRONG: "I need to set upload_port in platformio.ini"
**No!** ST-Link is auto-detected. The `monitor_port = COM5` is ONLY for serial debugging, not upload.

### ✅ CORRECT: "I just click Upload and it works"
**Yes!** That's exactly how ST-Link works.

## The Configuration is Already Correct

Your [platformio.ini](platformio.ini) already has everything needed:

```ini
[env:VARIANT_USART]
upload_protocol = stlink    ← This tells PlatformIO to use ST-Link
debug_tool      = stlink    ← This is for debugging
```

**You do NOT need:**
- ❌ `upload_port` setting
- ❌ To select a COM port
- ❌ Any additional configuration

## Terminal/Command Line Upload

If you prefer command line:

```bash
# Navigate to project folder
cd /Users/lucabresch/Documents/GitHub/AllTerrain-Gehhilfe-HS2504

# Upload (PlatformIO will find ST-Link automatically)
platformio run -e VARIANT_USART -t upload

# Or shorter version if pio is in PATH
pio run -e VARIANT_USART -t upload
```

**No port specification needed!**

## What If I Get "No ST-Link Found"?

### Solution 1: Install Drivers/Tools

**macOS:**
```bash
# Install ST-Link tools
brew install stlink

# Verify detection
st-info --probe
```

**Windows:**
- Download ST-Link drivers from STMicroelectronics website
- Or use Zadig to install WinUSB driver for ST-Link

**Linux:**
```bash
sudo apt install stlink-tools

# Add udev rules
sudo sh -c 'echo "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"0483\", ATTRS{idProduct}==\"3748\", MODE=\"0666\"" > /etc/udev/rules.d/99-stlink.rules'
sudo udevadm control --reload-rules
```

### Solution 2: Check USB Cable

- Some USB cables are power-only (no data)
- Try a different USB cable
- Try a different USB port on your computer

### Solution 3: Check Wiring to Hoverboard

The 4 wires must be connected:
- SWDIO to SWDIO
- SWCLK to SWCLK
- GND to GND
- (3.3V optional)

**Loose or wrong connections are the #1 cause of upload failures!**

## Still Can't Find Upload?

### Visual Guide - Where to Click in VS Code

**Bottom Toolbar Method (Fastest):**
```
Look at the very bottom of VS Code window:
┌─────────────────────────────────────────────┐
│ [✓] [→] [•] [🗑]  VARIANT_USART            │
│      ↑                                       │
│   CLICK HERE TO UPLOAD!                     │
└─────────────────────────────────────────────┘
```

**Sidebar Method:**
```
1. Click PlatformIO icon (left sidebar)
2. Expand "Project Tasks"
3. Expand "VARIANT_USART"
4. Click "Upload"
```

**Menu Method:**
```
Terminal → Run Task → PlatformIO: Upload (VARIANT_USART)
```

## Summary Checklist

Before clicking Upload:

- [ ] ST-Link is plugged into computer USB
- [ ] ST-Link LED is on
- [ ] 4 wires connected to hoverboard (SWDIO, SWCLK, GND, 3.3V)
- [ ] Hoverboard is powered (from battery or ST-Link 3.3V)
- [ ] PlatformIO extension is installed in VS Code
- [ ] Environment is set to VARIANT_USART (bottom toolbar)

Then just:
- [ ] Click the → (Upload) button

**That's it! No port selection needed!**

---

## Quick Answer

**Q: Where is the upload port?**

**A: There is no upload port to select! ST-Link uses USB directly. Just click the → Upload button in the bottom toolbar.**

If the upload button doesn't work, the issue is either:
1. ST-Link not plugged in / detected
2. Wiring to hoverboard is wrong/loose
3. ST-Link drivers not installed

Not a port selection issue!
