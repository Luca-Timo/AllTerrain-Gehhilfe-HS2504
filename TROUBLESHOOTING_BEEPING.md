# Troubleshooting: Continuous Beeping

## Problem: Hoverboard beeps continuously, ESP32 shows "Ready! Sending zero torque commands..." but no feedback

This means **the hoverboard is NOT receiving valid commands from ESP32**.

---

## 🔧 Step-by-Step Troubleshooting

### Step 1: Re-Upload ESP32 Code with Debug Output ✅

I've added debug output to show what's being sent. Re-upload the code to ESP32.

**After re-uploading, you should see:**
```
Ready! Sending zero torque commands...
>> SENT: steer=0 torque=0 checksum=0xABCD
>> SENT: steer=0 torque=10 checksum=0xABC7
>> SENT: steer=0 torque=20 checksum=0xABDD
...
```

**If you DON'T see ">> SENT:" lines:**
- ESP32 is not running the loop properly
- Check ESP32 is powered and running

---

### Step 2: Check Wiring (MOST LIKELY ISSUE!)

#### Correct Wiring:
```
ESP32-S3                    Hoverboard Right Cable
--------                    -----------------------
TX (GPIO 17)  ────────►     RX (PA3 / Pin 4)
RX (GPIO 16)  ◄────────     TX (PA2 / Pin 3)
GND           ─────────     GND (Pin 1)
                            15V (Pin 2) - NOT CONNECTED!
```

#### Common Mistakes:
❌ **TX connected to TX** (should be TX → RX)
❌ **RX connected to RX** (should be RX → TX)
❌ **Wrong pins on ESP32** (check your board pinout)
❌ **Loose connections** (wiggle test each wire)
❌ **No GND connection** (MUST have common ground!)

#### How to Verify:
1. **Visual check:** Follow each wire from ESP32 to hoverboard
2. **Continuity test:** Use multimeter to verify connections
3. **Swap test:** If TX→RX doesn't work, try swapping them

---

### Step 3: Verify Pin Numbers Match Your ESP32 Board

Different ESP32-S3 boards have different pinouts. Check your board:

**In the code (lines 29-30):**
```cpp
#define RX_PIN              16          // ESP32-S3 RX pin
#define TX_PIN              17          // ESP32-S3 TX pin
```

**Common ESP32-S3 boards:**

**ESP32-S3 DevKitC:**
- ✅ RX: GPIO16, TX: GPIO17 (default in code)

**ESP32-S3-WROOM:**
- ✅ RX: GPIO44, TX: GPIO43 (if using Serial2)

**If unsure, try common alternatives:**
```cpp
// Option 1 (default):
#define RX_PIN  16
#define TX_PIN  17

// Option 2:
#define RX_PIN  44
#define TX_PIN  43

// Option 3 (Serial1):
#define RX_PIN  9
#define TX_PIN  10
```

---

### Step 4: Check Hoverboard Cable Pinout

Verify which pin is which on the hoverboard 4-pin connector:

**Standard hoverboard right cable (looking at connector):**
```
    ┌─────────────┐
    │ 1  2  3  4  │
    └─────────────┘
     │  │  │  │
     │  │  │  └── Pin 4: RX (PA3) - Connect to ESP32 TX
     │  │  └───── Pin 3: TX (PA2) - Connect to ESP32 RX
     │  └──────── Pin 2: 15V - DON'T CONNECT TO ESP32!
     └─────────── Pin 1: GND - Connect to ESP32 GND
```

**Use a multimeter to verify:**
- GND should have continuity to hoverboard metal frame
- TX/RX pins will show ~3.3V when powered

---

### Step 5: Test UART Communication

**Quick loopback test on ESP32:**

Temporarily modify the code to test ESP32 UART is working:

```cpp
void setup() {
  Serial.begin(115200);
  Serial2.begin(HOVER_SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

  Serial.println("UART Test - connect TX to RX on ESP32");
  delay(100);
}

void loop() {
  // Send data
  Serial2.write("TEST");

  // Try to receive (if TX connected to RX)
  if (Serial2.available()) {
    char c = Serial2.read();
    Serial.print("Received: ");
    Serial.println(c);
  }

  delay(100);
}
```

**If this doesn't work:** Problem is with ESP32 pins/UART configuration

---

### Step 6: Verify Hoverboard is in USART Mode

Check your hoverboard firmware was flashed correctly:

**In config.h, verify:**
```c
#define VARIANT_USART              // Line 14 ✅
#define CONTROL_SERIAL_USART3  0   // Line 319 ✅
#define FEEDBACK_SERIAL_USART3     // Line 320 ✅
```

**Re-flash if needed:**
```bash
pio run -e VARIANT_USART -t upload
```

---

### Step 7: Check Baud Rate Match

**ESP32 code (line 23):**
```cpp
#define HOVER_SERIAL_BAUD   115200
```

**Hoverboard config.h (line 652):**
```c
#define USART3_BAUD           115200
```

**Both must match!**

---

### Step 8: Try Different UART Pins

If GPIO 16/17 don't work, try these alternatives:

```cpp
// Try Serial1 instead of Serial2
Serial1.begin(HOVER_SERIAL_BAUD, SERIAL_8N1, 18, 17);
// Then use Serial1.write() instead of Serial2.write()

// Or try other GPIO pairs:
// RX=18, TX=19
// RX=20, TX=21
// RX=44, TX=43 (on some ESP32-S3 boards)
```

---

### Step 9: Measure Signals with Multimeter/Oscilloscope

**Voltage test:**
1. Power everything on
2. Measure voltage on hoverboard TX pin (PA2): Should be ~3.3V or toggling
3. Measure voltage on ESP32 TX pin: Should be toggling ~3.3V

**If hoverboard TX is always low (0V):**
- Hoverboard might not be in UART mode
- Wrong firmware variant
- Hardware issue

**If ESP32 TX is always high (3.3V):**
- ESP32 not sending data
- Wrong pin configuration
- Code not running

---

## 🎯 Most Likely Solutions (Try in Order)

### 1. **Swap TX and RX wires** (50% of cases)
```
If currently:
  ESP32 TX → Hoverboard Pin 3
  ESP32 RX → Hoverboard Pin 4

Try:
  ESP32 TX → Hoverboard Pin 4
  ESP32 RX → Hoverboard Pin 3
```

### 2. **Check GND is connected** (30% of cases)
- UART needs common ground to work
- Verify continuity between ESP32 GND and Hoverboard GND

### 3. **Wrong GPIO pins on ESP32** (15% of cases)
- Check your ESP32-S3 board pinout
- Try different GPIO pairs

### 4. **Hoverboard not in USART mode** (5% of cases)
- Re-flash hoverboard with VARIANT_USART
- Verify config.h settings

---

## ✅ Expected Behavior When Working

**Serial Monitor Output:**
```
ESP32-S3 Hoverboard UART Control - Torque Mode
===============================================
Ready! Sending zero torque commands...
>> SENT: steer=0 torque=0 checksum=0xABCD
>> SENT: steer=0 torque=10 checksum=0xABC7
Cmd1: 0 | Cmd2: 0 | SpeedR: 0 | SpeedL: 0 | BatV: 40.5V | Temp: 25.3°C
>> SENT: steer=0 torque=20 checksum=0xABDD
Cmd1: 0 | Cmd2: 10 | SpeedR: 15 | SpeedL: 15 | BatV: 40.48V | Temp: 25.4°C
```

**Hoverboard:**
- Beeping stops
- Motors start spinning slowly
- LED lights up green

---

## 🆘 If Nothing Works

1. **Take photos of your wiring** - share them for review
2. **Post your Serial Monitor output** - shows what ESP32 is doing
3. **Verify hoverboard works** - test with Arduino example first
4. **Try a different ESP32 board** - rule out hardware issues

---

## Quick Diagnostic Checklist

Run through this quickly:

- [ ] ESP32 shows ">> SENT:" messages (re-upload debug code first!)
- [ ] TX wire goes from ESP32 to hoverboard RX pin
- [ ] RX wire goes from ESP32 to hoverboard TX pin
- [ ] GND is connected
- [ ] 15V is NOT connected to ESP32
- [ ] Hoverboard firmware is VARIANT_USART
- [ ] Baud rates match (115200)
- [ ] Correct GPIO pins for your ESP32 board
- [ ] Wires are firmly connected (no loose connections)

---

**After fixing, the beeping should stop within 1 second!**
