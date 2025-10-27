# Re-Flash Instructions - Using Correct Binary File

## ⚠️ Problem Identified

You flashed the **`.elf`** file, but STM32CubeProgrammer needs the **`.bin`** file!

The `.elf` file may have been rejected or partially flashed, causing the hoverboard to not respond to UART commands.

---

## ✅ Solution: Flash the Correct File

### Correct File Location:
```
/Users/lucabresch/Documents/GitHub/AllTerrain-Gehhilfe-HS2504/.pio/build/VARIANT_USART/firmware.bin
```

---

## Method 1: STM32CubeProgrammer (What You're Using)

### Step-by-Step:

1. **Connect Hardware:**
   - ST-Link → Computer (USB)
   - ST-Link → Hoverboard (SWDIO, SWCLK, GND, 3.3V)
   - Hoverboard battery connected (important!)

2. **Open STM32CubeProgrammer**

3. **Connect to Board:**
   - Port: "USB" (ST-Link should auto-detect)
   - Click **"Connect"**
   - Should show: "Connected" with green checkmark

4. **Load Binary File:**
   - Click **"Open file"** (folder icon)
   - Navigate to: `.pio/build/VARIANT_USART/`
   - Select: **`firmware.bin`** (NOT firmware.elf!)
   - File should show in the path field

5. **Critical Settings:**
   ```
   Start Address: 0x08000000
   File Type: Binary (.bin)
   ```
   ⚠️ **The start address MUST be 0x08000000!**

6. **Program:**
   - Click **"Download"** or **"Start Programming"**
   - Progress bar will show
   - Wait for "File download complete"
   - Should also show "Verification... OK"

7. **Disconnect:**
   - Click **"Disconnect"**
   - Unplug ST-Link from computer
   - Unplug ST-Link from hoverboard

8. **Test:**
   - Power cycle hoverboard (off then on)
   - Connect ESP32
   - Should stop beeping!

---

## Method 2: PlatformIO (Recommended - Easier!)

### Using VS Code:

1. **Connect ST-Link** to hoverboard and computer
2. **Open project** in VS Code
3. **Bottom toolbar** → Click the **→** (Upload) button
4. Wait for "SUCCESS"
5. Done!

### Using Command Line:

```bash
cd /Users/lucabresch/Documents/GitHub/AllTerrain-Gehhilfe-HS2504

# Build (if needed)
platformio run -e VARIANT_USART

# Upload
platformio run -e VARIANT_USART -t upload
```

---

## Method 3: ST-Link Command Line Tools

If you have `st-flash` installed:

```bash
cd /Users/lucabresch/Documents/GitHub/AllTerrain-Gehhilfe-HS2504

# Flash the binary
st-flash write .pio/build/VARIANT_USART/firmware.bin 0x08000000

# Verify
st-flash read read_back.bin 0x08000000 0x40000
```

---

## Common Issues with STM32CubeProgrammer

### Issue: "Cannot connect to target"
**Solutions:**
- Check ST-Link is plugged in
- Check SWDIO/SWCLK/GND connections
- Try holding RESET button while connecting
- Enable "Connect under reset" in settings

### Issue: "Flash erase failed"
**Solutions:**
- Board might have read protection enabled
- Try: "Security" → "Read Out Protection" → "Disable"
- ⚠️ This will erase everything!

### Issue: "Verification failed"
**Solutions:**
- Flash might be corrupt
- Try full chip erase first
- Check start address is 0x08000000

---

## How to Verify Firmware Flashed Correctly

After flashing, you should see in STM32CubeProgrammer:

```
Download in Progress...
File download complete
Time elapsed during download operation: 00:00:05.123

Verifying ...
Download verified successfully
```

### If You See Errors:
```
Error: Failed to erase memory
Error: Data mismatch
```
→ Flash did NOT work, try again

---

## After Successful Flash

### Expected Behavior:

**On Power-Up (without ESP32 connected):**
- Hoverboard beeps (waiting for commands)
- After ~1 second of no commands → continuous beeping
- This is NORMAL! It means UART mode is active

**With ESP32 Connected:**
- Hoverboard powers on
- Brief beep
- Beeping stops (receiving commands)
- Motors respond to torque commands

---

## Verification Checklist

After flashing, verify:

- [ ] STM32CubeProgrammer showed "Verification OK"
- [ ] File size was ~50-100KB
- [ ] Start address was 0x08000000
- [ ] Used firmware.bin (NOT firmware.elf)
- [ ] Hoverboard powered from battery during flash
- [ ] ST-Link disconnected after flash
- [ ] Hoverboard power cycled

---

## Quick Reference: File Types

| File | Purpose | Use With |
|------|---------|----------|
| **firmware.bin** | ✅ Flash image | STM32CubeProgrammer, st-flash |
| **firmware.hex** | ✅ Flash image (Intel HEX) | Some programmers |
| **firmware.elf** | ❌ Debug symbols | Debuggers only, NOT for flashing! |

---

## Expected File Sizes

```
firmware.bin: ~50-80 KB
firmware.elf: ~500 KB - 2 MB (has debug info)
```

If your .elf file is huge compared to .bin, that confirms why flashing .elf didn't work properly!

---

## After Re-Flash

1. **Disconnect ST-Link completely**
2. **Power off hoverboard**
3. **Wait 5 seconds**
4. **Power on hoverboard**
5. **Connect ESP32**
6. **Upload ESP32 test code**
7. **Watch Serial Monitor**

You should now see:
```
47208 ms: SENT 8 bytes: CD AB 00 00 00 00 CD AB
  << RECEIVED: CD AB 00 00 12 34 56 78 ...
47308 ms: SENT 8 bytes: CD AB 00 00 00 00 CD AB
  << RECEIVED: CD AB 00 00 12 34 56 78 ...
```

And beeping should **STOP**! 🎉

---

## Still Not Working After Re-Flash?

If you correctly flash firmware.bin and it still beeps:

1. **Check config.h was built correctly:**
   ```bash
   # Verify the build includes VARIANT_USART
   grep "VARIANT_USART" Inc/config.h
   ```
   Should show: `#define VARIANT_USART`

2. **Try full chip erase first:**
   - In STM32CubeProgrammer
   - "Erasing & Programming" menu
   - "Full chip erase"
   - Then flash firmware.bin again

3. **Verify USART3 pins on hoverboard:**
   - Check PA2 and PA3 are not damaged
   - Check connector is making good contact

---

**Bottom line: Flash the `.bin` file, not the `.elf` file!** This is almost certainly why it's not working.
