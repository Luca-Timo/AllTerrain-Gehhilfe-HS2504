# Bench Testing Safety Assessment

## ✅ ESP32 Parameters - NOW SAFE FOR BENCH TESTING

I've adjusted the ESP32 code to be **safe for first bench tests**:

### ESP32 Test Parameters (Updated)

| Parameter | Previous Value | **New SAFE Value** | Change |
|-----------|----------------|-------------------|--------|
| **Max Test Torque** | ±300 (30%) | **±100 (10%)** | ✅ SAFER |
| **Torque Step** | 50/step | **10/step** | ✅ 5x SLOWER |
| **Ramp Time** | 0.6 sec to max | **1.0 sec to max** | ✅ SLOWER |
| **Update Rate** | 100ms (10Hz) | **100ms (10Hz)** | ✅ UNCHANGED |

### What This Means:

**Before adjustment:**
- Motors would ramp to 30% torque in 0.6 seconds
- Fast acceleration - could surprise you

**After adjustment (CURRENT):**
- Motors ramp to only 10% torque in 1.0 second
- Very gentle, slow acceleration
- Easy to react if something goes wrong

---

## ⚠️ Hoverboard Firmware Parameters - NEED CONSIDERATION

The firmware parameters are still **moderate to high**. They won't hurt anything, but you should be aware:

### Hoverboard Limitations (Current Settings)

| Parameter | Current Value | Risk Level | Recommendation |
|-----------|---------------|------------|----------------|
| **I_MOT_MAX** | 15A | ⚠️ MODERATE | Can produce strong torque |
| **I_DC_MAX** | 17A | ⚠️ MODERATE | Total 34A available |
| **N_MOT_MAX** | 1000 RPM | ⚠️ HIGH | ~20 km/h wheel speed |
| **Field Weakening** | Disabled | ✅ GOOD | Keep disabled |

### Why Current Firmware Settings Are Acceptable:

1. **ESP32 limits to 10% torque anyway** - The firmware allows 15A, but ESP32 only requests 10% = ~1.5A effective
2. **Speed limit won't be hit** - At 10% torque, motors won't reach 1000 RPM
3. **Protection is active** - Current and speed protection will prevent damage

---

## 🎯 Bench Test Safety Assessment

### Overall Safety: ✅ **SAFE FOR BENCH TESTING**

With the updated ESP32 code, the system is now safe for bench testing with these provisions:

### ✅ Safe Elements:
- ESP32 torque limited to ±100 (10% of max)
- Slow ramp rate (10 per 100ms)
- Torque control mode (motors can freewheel)
- Field weakening disabled
- Update rate appropriate (10 Hz)

### ⚠️ Safety Precautions Required:

1. **Elevate Hoverboard**
   - Wheels must be off the ground
   - Secure on a stable bench/table
   - Ensure it can't fall or tip

2. **Emergency Stop Ready**
   - Keep hand near power switch
   - Be ready to pull battery connector
   - OR unplug ESP32 USB immediately

3. **Physical Safety**
   - Keep hands/tools away from spinning wheels
   - Wear safety glasses
   - Don't touch motor wires when powered
   - Loose clothing/hair tied back

4. **Electrical Safety**
   - Verify wiring before power-on:
     - ✅ SWDIO, SWCLK, GND connected
     - ✅ ESP32 RX ↔ Hoverboard TX
     - ✅ ESP32 TX ↔ Hoverboard RX
     - ✅ GND connected
     - ⚠️ 15V wire NOT connected to ESP32!
   - Battery charged but not over-charged
   - Check for loose connections

5. **Monitoring During Test**
   - Watch ESP32 serial monitor (115200 baud)
   - Observe motor behavior
   - Listen for unusual sounds
   - Monitor battery voltage in feedback
   - Monitor temperature in feedback

---

## 📊 Test Progression Plan

### Phase 1: First Power-On (CURRENT SETUP - SAFE)
**ESP32 Settings:**
- Torque: ±100 (10%)
- Ramp: 10/step
- Duration: ~20 seconds per cycle

**Expected Behavior:**
- Wheels spin slowly forward, then reverse
- Smooth acceleration/deceleration
- No jerky movements
- Quiet operation

**Success Criteria:**
- Motors respond to commands
- Feedback data received
- No error messages
- Smooth operation

### Phase 2: Increased Torque (If Phase 1 Successful)
**Modify ESP32 code:**
```cpp
if (testTorque >= 200 || testTorque <= -200) {  // 20% torque
```

**Expected Behavior:**
- Faster wheel rotation
- Still controlled
- May hear more motor noise

### Phase 3: Full Range Test (If Phase 2 Successful)
**Modify ESP32 code:**
```cpp
if (testTorque >= 500 || testTorque <= -500) {  // 50% torque
```

**Expected Behavior:**
- Significant wheel speed
- Powerful acceleration
- Motor noise increases

---

## 🚨 Stop Testing Immediately If:

| Symptom | Possible Cause | Action |
|---------|----------------|--------|
| **Jerky/uncontrolled movement** | Bad connection, wrong settings | STOP, check wiring |
| **Smoke or burning smell** | Overheating, short circuit | CUT POWER IMMEDIATELY |
| **Unusual noises (grinding, clicking)** | Mechanical issue | STOP, inspect hardware |
| **Motors don't respond** | Communication failure | Check UART connections |
| **Battery voltage drops rapidly** | Over-current draw | STOP, reduce torque |
| **Temperature > 60°C** | Overheating | STOP, cool down |
| **Checksum errors** | Communication issues | STOP, check wiring/GND |
| **One motor spins, other doesn't** | Hardware failure | STOP, inspect board |

---

## 📈 Expected Telemetry Values

### Normal Operating Values:

| Parameter | Safe Range | Warning Level | Critical |
|-----------|------------|---------------|----------|
| **Battery Voltage** | 36-42V (10S) | <36V | <33V |
| **Board Temp** | 20-40°C | 40-60°C | >60°C |
| **Motor Speed** | 0-300 RPM | 300-600 RPM | >800 RPM |
| **Cmd Echo** | Matches sent | Lag <200ms | No response |

### What You'll See in Serial Monitor:

```
ESP32-S3 Hoverboard UART Control - Torque Mode
===============================================
Ready! Sending zero torque commands...
Cmd1: 0 | Cmd2: 0 | SpeedR: 0 | SpeedL: 0 | BatV: 40.50V | Temp: 25.3°C
Cmd1: 0 | Cmd2: 10 | SpeedR: 15 | SpeedL: 15 | BatV: 40.48V | Temp: 25.4°C
Cmd1: 0 | Cmd2: 20 | SpeedR: 28 | SpeedL: 29 | BatV: 40.46V | Temp: 25.5°C
...
Cmd1: 0 | Cmd2: 100 | SpeedR: 145 | SpeedL: 143 | BatV: 40.35V | Temp: 26.2°C
Cmd1: 0 | Cmd2: 90 | SpeedR: 132 | SpeedL: 130 | BatV: 40.38V | Temp: 26.3°C
...
```

---

## ✅ Pre-Test Checklist

Before powering on:

### Hardware:
- [ ] Hoverboard elevated and secured
- [ ] Wheels can spin freely (nothing blocking them)
- [ ] All UART connections verified (TX↔RX, RX↔TX, GND)
- [ ] 15V wire is NOT connected to ESP32
- [ ] ST-Link disconnected (after flashing firmware)
- [ ] Battery connector secured
- [ ] No loose wires or tools near moving parts

### Software:
- [ ] Hoverboard firmware flashed successfully (VARIANT_USART)
- [ ] ESP32 code uploaded with SAFE parameters (±100 torque)
- [ ] Serial monitor open (115200 baud)
- [ ] Emergency stop plan ready

### Environment:
- [ ] Clear workspace
- [ ] Fire extinguisher nearby (just in case)
- [ ] Other people informed of testing
- [ ] Good lighting to observe motors

### You:
- [ ] Safety glasses on
- [ ] Loose clothing/hair secured
- [ ] Well-rested and focused
- [ ] Read this safety document completely

---

## 🎓 Final Assessment

### Current Configuration Safety Rating: ✅ **8/10 - SAFE**

**Why 8/10 and not 10/10?**
- Firmware allows up to 15A (though ESP32 limits usage)
- 1000 RPM speed limit is high (though unlikely to be reached at 10% torque)

**Why it's still SAFE:**
- ESP32 torque is limited to 10% (±100)
- Slow ramp rate prevents sudden movements
- Torque mode allows freewheeling
- Multiple protection layers active
- You have physical emergency stop capability

### Recommendation: ✅ **APPROVED FOR BENCH TESTING**

The current parameters are suitable for:
- ✅ First power-on test
- ✅ Communication verification
- ✅ Basic motor response testing
- ✅ Telemetry validation
- ✅ Learning system behavior

**Proceed with testing using the updated ESP32 code!**

---

## 📞 If Something Goes Wrong

1. **Cut power immediately**
2. **Document what happened** (before forgetting)
3. **Inspect hardware** for damage
4. **Review serial output** for clues
5. **Ask for help** if needed

**Remember:** It's better to be overly cautious than to damage equipment or hurt yourself!

---

**Last Updated:** 2025-10-27
**Configuration:** VARIANT_USART, TRQ_MODE, Safe ESP32 Test Values
