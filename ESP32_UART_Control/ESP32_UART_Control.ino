// *******************************************************************
//  ESP32-S3 UART Control Example for Hoverboard FOC Firmware
//  AllTerrain Gehhilfe - HS2504
//
//  This example demonstrates UART communication with the hoverboard
//  using torque control mode via USART3 (right sensor cable)
//
//  Load Cell Integration: Uses HX711 24-bit ADC for precise force measurement
// *******************************************************************
// REQUIRED LIBRARY:
// Install "HX711" by Bogdan Necula (bogde)
// GitHub: https://github.com/bogde/HX711
// Arduino Library Manager -> Search "HX711 Arduino Library" by Bogdan Necula -> Install
// *******************************************************************
// HARDWARE CONNECTIONS:
// - Hoverboard TX (PA2) → ESP32-S3 RX (GPIO 18)
// - Hoverboard RX (PA3) → ESP32-S3 TX (GPIO 43)
// - GND → GND
// - DO NOT CONNECT THE 15V WIRE FROM THE SENSOR CABLE!
//
// JOYSTICK (Analog):
// - GPIO 1: X-axis (steering) 0-3.3V (left=0V, center=1.65V, right=3.3V)
// - GPIO 2: Y-axis (speed) 0-3.3V (back=0V, center=1.65V, forward=3.3V)
//
// BUTTONS (Digital):
// - GPIO 4: START button (normally open, goes HIGH to start)
// - GPIO 5: STOP button (normally HIGH at 3.3V):
//     * Stays HIGH (3.3V): Normal operation / waiting for start
//     * Pulse HIGH→LOW→HIGH (<500ms): Normal stop command
//     * Stays LOW (GND, >500ms): Emergency stop
//
// HX711 LOAD CELL AMPLIFIERS (Two Modules):
// Left HX711 Module:
//   - VCC → 3.3V or 5V (both work with ESP32-S3, 5V recommended for better accuracy)
//   - GND → GND
//   - DT (DOUT) → GPIO 11
//   - SCK (CLK) → GPIO 12
//   - E+ (Red) → Load Cell Red wire (Excitation+)
//   - E- (Black) → Load Cell Black wire (Excitation-)
//   - A+ (Green) → Load Cell Green wire (Signal+)
//   - A- (White) → Load Cell White wire (Signal-)
//
// Right HX711 Module:
//   - VCC → 3.3V or 5V (match left module voltage)
//   - GND → GND
//   - DT (DOUT) → GPIO 13
//   - SCK (CLK) → GPIO 14
//   - E+, E-, A+, A- → Connect to right load cell (same color scheme as left)
//
// NOTE: Load cell wire colors may vary by manufacturer. Standard colors are:
//   Red = E+, Black = E-, Green = A+, White = A-
//
// HOVERBOARD CONFIGURATION (Inc/config.h):
// - VARIANT_USART is enabled
// - CONTROL_SERIAL_USART3 is enabled
// - FEEDBACK_SERIAL_USART3 is enabled
// - CTRL_TYP_SEL = FOC_CTRL
// - CTRL_MOD_REQ = SPD_MODE (speed control - IMPORTANT!)
//
// NOTE: This code uses SPD_MODE for proper speed limiting.
// The firmware will enforce the commanded RPM limits regardless of load.
// *******************************************************************

#include "HX711.h"

// ########################## USER CONFIGURATION ##########################
// DEBUG / LOGGING SETTINGS (true = enable, false = disable)
#define DEBUG_STARTUP_INFO      false   // Print configuration at startup
#define DEBUG_BUTTON_EVENTS     false   // Print button press events (start/stop/emergency)
#define DEBUG_CONTROL_OUTPUT    false   // Print steer/speed commands
#define DEBUG_JOYSTICK_RAW      false   // Print raw joystick ADC values
#define DEBUG_LOAD_CELLS        true    // Print load cell measurements
#define DEBUG_HX711_RAW         true    // Print raw HX711 ADC counts (for troubleshooting)
#define DEBUG_HOVERBOARD_FB     false   // Print hoverboard feedback (speed, voltage, temp)
#define DEBUG_UART_COMMANDS     false   // Print raw UART commands being sent
#define DEBUG_TARE_PROCESS      false   // Print tare offset values

// CALIBRATION MODE - Set to true to find correct scale factors
#define CALIBRATION_MODE        false   // Enable this to see raw counts for calibration

// SPEED SETTINGS (in km/h)
#define MAX_SPEED_FORWARD_KMH   5.0    // Maximum forward speed in km/h
#define MAX_SPEED_BACKWARD_KMH  2.5     // Maximum backward speed in km/h (for safety)

// WHEEL SETTINGS
#define WHEEL_DIAMETER_MM       175.0   // Wheel diameter in millimeters

// STEERING SETTINGS
#define MAX_STEERING_PERCENT    80.0    // Maximum steering ratio (0-100% of total speed difference)

// JOYSTICK SETTINGS
#define DEADZONE_PERCENT        5.0     // Deadzone around neutral position (percentage)
#define ADC_SAMPLES             4       // Number of ADC samples to average for smoothing

// HX711 LOAD CELL SETTINGS
// Left Load Cell Specifications
#define LOAD_CELL_LEFT_MAX_N    200.0   // Maximum force rating in Newtons (left)
#define LOAD_CELL_LEFT_SENS_MV  2.0002  // Sensitivity in mV/V (left)

// Right Load Cell Specifications
#define LOAD_CELL_RIGHT_MAX_N   200.0   // Maximum force rating in Newtons (right)
#define LOAD_CELL_RIGHT_SENS_MV 2.0002  // Sensitivity in mV/V (right)

// HX711 Configuration
#define HX711_SAMPLES           10      // Number of samples to average for load cells
#define HX711_GAIN              128     // Gain: 128 for channel A (default)

// HX711 Calibration Factors (will be calculated based on load cell specs)
// These are computed automatically from sensitivity and max load
// HX711 reading units per Newton will be calculated at runtime

// ########################## SYSTEM DEFINES ##########################
#define HOVER_SERIAL_BAUD   115200      // Baud rate for communication with hoverboard
#define START_FRAME         0xABCD      // Start frame definition for reliable serial communication
#define TIME_SEND           100         // [ms] Sending time interval
#define SPEED_COEFFICIENT   1000        // [-] Maximum speed command value (-1000 to +1000)
#define ADC_MAX             4095        // 12-bit ADC resolution
#define ADC_NEUTRAL         2048        // Neutral position (3.3V / 2)
#define PI                  3.14159265359

// Pin definitions - UART
#define RX_PIN              18          // ESP32-S3 RX pin (GPIO 18 works!)
#define TX_PIN              43          // ESP32-S3 TX pin (GPIO 43 works!)

// Pin definitions - Joystick
#define JOYSTICK_X_PIN      1           // GPIO 1: X-axis (steering) 0-3.3V
#define JOYSTICK_Y_PIN      2           // GPIO 2: Y-axis (speed) 0-3.3V

// Pin definitions - Buttons
#define START_BUTTON_PIN    4           // GPIO 4: Start button (normally open, high active)
#define STOP_BUTTON_PIN     5           // GPIO 5: Stop button - 3 states:
                                        //   HIGH (3.3V): normal operation or waiting
                                        //   Pulse HIGH→LOW→HIGH: stop command
                                        //   Stays LOW (GND): emergency stop

// Pin definitions - HX711 Load Cell Amplifiers
#define HX711_LEFT_DOUT       11        // GPIO 11: Left HX711 data out
#define HX711_LEFT_SCK        12        // GPIO 12: Left HX711 clock (shared or separate)
#define HX711_RIGHT_DOUT      13        // GPIO 13: Right HX711 data out
#define HX711_RIGHT_SCK       14        // GPIO 14: Right HX711 clock
// Note: HX711 modules are powered separately (VCC to 3.3V or 5V, GND to GND)

// Button debounce and timing
#define DEBOUNCE_TIME       50          // ms - debounce time for button reads
#define EMERGENCY_THRESHOLD 500         // ms - time LOW to trigger emergency stop

// ########################## STRUCTURES ##########################
typedef struct{
   uint16_t start;
   int16_t  steer;
   int16_t  speed;
   uint16_t checksum;
} SerialCommand;

typedef struct{
   uint16_t start;
   int16_t  cmd1;
   int16_t  cmd2;
   int16_t  speedR_meas;
   int16_t  speedL_meas;
   int16_t  batVoltage;
   int16_t  boardTemp;
   uint16_t cmdLed;
   uint16_t checksum;
} SerialFeedback;

// Global variables
SerialCommand Command;
SerialFeedback Feedback;
SerialFeedback NewFeedback;

uint8_t idx = 0;
uint16_t bufStartFrame;
byte *p;
byte incomingByte;
byte incomingBytePrev;

// Control state variables
bool systemEnabled = false;           // System operational state
bool emergencyStop = false;           // Emergency stop flag
unsigned long stopButtonLowTime = 0;  // Time when stop button went LOW
bool stopButtonWasHigh = true;        // Previous state of stop button
unsigned long lastDebounceTime = 0;   // Last time button state changed
int16_t maxSpeedForward = 0;          // Calculated RPM from km/h and wheel diameter
int16_t maxSpeedBackward = 0;         // Calculated RPM from km/h and wheel diameter
int16_t maxSteeringSpeed = 0;         // Calculated from steering percent

// HX711 Load Cell Amplifier Objects (using HX711 class from bogde library)
HX711 hx711_left;    // Left load cell amplifier
HX711 hx711_right;   // Right load cell amplifier

// Load cell variables
float loadCellLeftForce = 0.0;        // Measured force on left load cell in Newtons
float loadCellRightForce = 0.0;       // Measured force on right load cell in Newtons

// ########################## HELPER FUNCTIONS ##########################

// Calculate maximum speed (RPM) values from km/h and wheel diameter
void calculateSpeedLimits() {
  // Convert km/h to RPM using wheel diameter
  // Formula: RPM = (km/h * 1000 * 60) / (PI * wheel_diameter_mm * 60)
  // Simplified: RPM = (km/h * 1000) / (PI * wheel_diameter_mm)

  float wheelCircumference_mm = PI * WHEEL_DIAMETER_MM;  // mm per revolution
  float wheelCircumference_m = wheelCircumference_mm / 1000.0;  // m per revolution

  // Convert km/h to m/min, then divide by wheel circumference to get RPM
  // km/h → m/min: multiply by 1000/60 = 16.6667
  // m/min → RPM: divide by wheelCircumference_m

  float rpmForward = (MAX_SPEED_FORWARD_KMH * 1000.0 / 60.0) / wheelCircumference_m;
  float rpmBackward = (MAX_SPEED_BACKWARD_KMH * 1000.0 / 60.0) / wheelCircumference_m;

  // Convert RPM to speed command value
  // The hoverboard firmware typically uses a coefficient where ±1000 represents max RPM
  // Max RPM set to 300 for this application
  // Speed command = (desired_RPM / max_RPM) * SPEED_COEFFICIENT

  float MAX_RPM = 300.0;  // Maximum RPM for this hoverboard motor

  maxSpeedForward = (int16_t)((rpmForward / MAX_RPM) * SPEED_COEFFICIENT);
  maxSpeedBackward = (int16_t)((rpmBackward / MAX_RPM) * SPEED_COEFFICIENT);

  // Clamp to valid range
  if (maxSpeedForward > SPEED_COEFFICIENT) maxSpeedForward = SPEED_COEFFICIENT;
  if (maxSpeedBackward > SPEED_COEFFICIENT) maxSpeedBackward = SPEED_COEFFICIENT;

  // Calculate max steering speed from percentage of max speed
  maxSteeringSpeed = (int16_t)((MAX_STEERING_PERCENT / 100.0) * maxSpeedForward);
}

// Read ADC with averaging for noise reduction
int readADC(uint8_t pin) {
  int sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(100);
  }
  return sum / ADC_SAMPLES;
}

// Apply cubic curve for more precision at low speeds
// Input: -1.0 to 1.0, Output: -1.0 to 1.0
float applyCurve(float input) {
  // Cubic curve: y = x^3 gives more precision near zero
  // You can adjust the curve by mixing linear and cubic: y = a*x + (1-a)*x^3
  float mixFactor = 0.3;  // 30% linear, 70% cubic for good low-speed control
  return mixFactor * input + (1.0 - mixFactor) * input * input * input;
}

// Map joystick ADC value to -1.0 to 1.0 range with deadzone
float mapJoystick(int adcValue, bool invertDirection = false) {
  // Calculate deviation from neutral
  int deviation = adcValue - ADC_NEUTRAL;

  // Apply deadzone
  int deadzoneRange = (int)(ADC_NEUTRAL * DEADZONE_PERCENT / 100.0);
  if (abs(deviation) < deadzoneRange) {
    return 0.0;
  }

  // Map to -1.0 to 1.0 range
  float normalized;
  if (deviation > 0) {
    // Positive direction
    normalized = (float)(deviation - deadzoneRange) / (float)(ADC_NEUTRAL - deadzoneRange);
  } else {
    // Negative direction
    normalized = (float)(deviation + deadzoneRange) / (float)(ADC_NEUTRAL - deadzoneRange);
  }

  // Clamp to valid range
  if (normalized > 1.0) normalized = 1.0;
  if (normalized < -1.0) normalized = -1.0;

  // Invert if requested
  if (invertDirection) {
    normalized = -normalized;
  }

  return normalized;
}

// Tare (zero) the HX711 load cells using bogde library
void tareLoadCells() {
  Serial.println("Taring load cells...");
  hx711_left.tare();
  hx711_right.tare();
  Serial.println("Taring complete!");
}

// Update load cell readings from HX711 modules using bogde library
void updateLoadCells() {
  // Read raw values and convert to Newtons
  // The bogde library returns raw ADC values that need to be scaled

  // For now, read raw values. You'll need to calibrate the scale factor.
  // Typical calibration: place known weight, measure reading, calculate scale = reading / known_weight

  // Left load cell
  if (hx711_left.is_ready()) {
    long raw_left = hx711_left.read();
    // TODO: Apply calibration scale factor
    // For now, just store raw value (you'll calibrate this)
    loadCellLeftForce = raw_left / 1000.0;  // Placeholder - needs calibration
  }

  // Right load cell
  if (hx711_right.is_ready()) {
    long raw_right = hx711_right.read();
    // TODO: Apply calibration scale factor
    loadCellRightForce = raw_right / 1000.0;  // Placeholder - needs calibration
  }
}

// Check button states and update system state
void checkButtons() {
  unsigned long currentTime = millis();

  // Read stop button current state
  int stopButtonState = digitalRead(STOP_BUTTON_PIN);

  // STOP BUTTON LOGIC - 3 states:
  // 1. HIGH (3.3V): Normal operation or waiting for start
  // 2. Pulse HIGH→LOW→HIGH: Stop command (normal stop)
  // 3. Stays LOW: Emergency stop

  if (stopButtonState == LOW) {
    // Button is currently LOW (GND)

    if (stopButtonWasHigh) {
      // HIGH to LOW transition detected - start timing
      stopButtonLowTime = currentTime;
      stopButtonWasHigh = false;
    } else {
      // Button has been LOW for some time
      unsigned long lowDuration = currentTime - stopButtonLowTime;

      if (lowDuration >= EMERGENCY_THRESHOLD && !emergencyStop) {
        // Button held LOW for >500ms = EMERGENCY STOP
        emergencyStop = true;
        systemEnabled = false;
        #if DEBUG_BUTTON_EVENTS
          Serial.println("!!! EMERGENCY STOP ACTIVATED !!!");
          Serial.println("!!! BUTTON HELD LOW !!!");
        #endif
      }
    }

  } else {
    // Button is currently HIGH (3.3V)

    if (!stopButtonWasHigh) {
      // LOW to HIGH transition detected - button released
      unsigned long lowDuration = currentTime - stopButtonLowTime;

      if (lowDuration < EMERGENCY_THRESHOLD) {
        // Short pulse detected (<500ms) = NORMAL STOP
        if (systemEnabled) {
          systemEnabled = false;
          #if DEBUG_BUTTON_EVENTS
            Serial.println(">> STOP: Normal stop pulse detected");
          #endif
        }
      }

      stopButtonWasHigh = true;
    }
    // Otherwise button is just sitting at HIGH - normal state
  }

  // START BUTTON LOGIC
  // Read start button (normally open, high active)
  int startButtonState = digitalRead(START_BUTTON_PIN);

  if (startButtonState == HIGH) {
    // Debounce the start button
    if (currentTime - lastDebounceTime > DEBOUNCE_TIME) {

      if (!systemEnabled && !emergencyStop) {
        // Normal start
        systemEnabled = true;
        Serial.println(">>> SYSTEM STARTED <<<");
        lastDebounceTime = currentTime;
      }
      else if (emergencyStop) {
        // Clear emergency stop and restart
        emergencyStop = false;
        systemEnabled = true;
        Serial.println(">>> EMERGENCY STOP CLEARED - SYSTEM RESTARTED <<<");
        lastDebounceTime = currentTime;
      }
    }
  }
}

// Read joystick and calculate speed values
void readJoystickAndCalculateSpeed(int16_t &steer, int16_t &speed) {
  if (!systemEnabled) {
    steer = 0;
    speed = 0;
    return;
  }

  // Read joystick axes
  int xRaw = readADC(JOYSTICK_X_PIN);
  int yRaw = readADC(JOYSTICK_Y_PIN);

  // Map to normalized values (-1.0 to 1.0)
  // X-axis: left=0V=-1.0, neutral=1.65V=0.0, right=3.3V=1.0
  // Y-axis: backward=0V=-1.0, neutral=1.65V=0.0, forward=3.3V=1.0
  float xNorm = mapJoystick(xRaw, false);
  float yNorm = mapJoystick(yRaw, false);

  // Apply curve for better low-speed control
  float xCurved = applyCurve(xNorm);
  float yCurved = applyCurve(yNorm);

  // Calculate base speed from Y-axis
  if (yCurved >= 0) {
    // Forward
    speed = (int16_t)(yCurved * maxSpeedForward);
  } else {
    // Backward
    speed = (int16_t)(yCurved * maxSpeedBackward);
  }

  // Calculate steering from X-axis
  steer = (int16_t)(xCurved * maxSteeringSpeed);
}

// ########################## SETUP ##########################
void setup()
{
  // Initialize debug serial
  Serial.begin(115200);
  Serial.println("ESP32-S3 Hoverboard UART Control - Joystick Mode");
  Serial.println("==================================================");

  // Configure GPIO pins
  pinMode(START_BUTTON_PIN, INPUT);        // Start button: normally open, high active
  pinMode(STOP_BUTTON_PIN, INPUT);         // Stop button: expects external pull-up to 3.3V
                                           // HIGH = normal, LOW pulse = stop, stays LOW = emergency

  // Configure ADC resolution (ESP32 supports 12-bit)
  analogReadResolution(12);

  // Set ADC attenuation to 11dB for full 0-3.3V range (per pin on ESP32-S3)
  analogSetPinAttenuation(JOYSTICK_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOYSTICK_Y_PIN, ADC_11db);

  // Initialize HX711 Load Cell Amplifiers using bogde library
  Serial.println("\n=== INITIALIZING HX711 MODULES ===");

  // Initialize with pin assignments (DOUT pin, SCK pin)
  hx711_left.begin(HX711_LEFT_DOUT, HX711_LEFT_SCK);
  hx711_right.begin(HX711_RIGHT_DOUT, HX711_RIGHT_SCK);

  Serial.println("  Left HX711 initialized on pins:");
  Serial.print("    DOUT: GPIO "); Serial.println(HX711_LEFT_DOUT);
  Serial.print("    SCK:  GPIO "); Serial.println(HX711_LEFT_SCK);
  Serial.println("  Right HX711 initialized on pins:");
  Serial.print("    DOUT: GPIO "); Serial.println(HX711_RIGHT_DOUT);
  Serial.print("    SCK:  GPIO "); Serial.println(HX711_RIGHT_SCK);

  // Wait for HX711 modules to stabilize
  delay(100);

  // Test ADC readings immediately after configuration
  delay(100);
  Serial.println("\n=== ADC TEST READINGS ===");
  Serial.print("  GPIO 1 raw: "); Serial.println(analogRead(JOYSTICK_X_PIN));
  Serial.print("  GPIO 2 raw: "); Serial.println(analogRead(JOYSTICK_Y_PIN));
  Serial.println("  (Should be ~2048 if joystick is centered at 1.65V)");
  Serial.println("  (If reading 4095, check: 1) Joystick wiring, 2) Power supply, 3) Pin connections)");

  // Initialize hardware serial for hoverboard communication
  Serial2.begin(HOVER_SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

  // Calculate speed limits based on km/h settings and wheel diameter
  calculateSpeedLimits();

  // Wait for serial to stabilize
  delay(100);

  // Print configuration
  Serial.println("\nConfiguration:");
  Serial.print("  Max Speed Forward: "); Serial.print(MAX_SPEED_FORWARD_KMH); Serial.println(" km/h");
  Serial.print("  Max Speed Backward: "); Serial.print(MAX_SPEED_BACKWARD_KMH); Serial.println(" km/h");
  Serial.print("  Wheel Diameter: "); Serial.print(WHEEL_DIAMETER_MM); Serial.println(" mm");
  Serial.print("  Max Steering: "); Serial.print(MAX_STEERING_PERCENT); Serial.println(" %");
  Serial.print("  Deadzone: "); Serial.print(DEADZONE_PERCENT); Serial.println(" %");
  Serial.print("\nCalculated Speed Commands (RPM based):");
  Serial.print("  Forward: "); Serial.println(maxSpeedForward);
  Serial.print("  Backward: "); Serial.println(maxSpeedBackward);
  Serial.print("  Steering: "); Serial.println(maxSteeringSpeed);

  Serial.println("\n=== LOAD CELL CONFIGURATION ===");
  Serial.print("  Left: Max "); Serial.print(LOAD_CELL_LEFT_MAX_N);
  Serial.print("N, Sens "); Serial.print(LOAD_CELL_LEFT_SENS_MV); Serial.println(" mV/V");
  Serial.print("  Right: Max "); Serial.print(LOAD_CELL_RIGHT_MAX_N);
  Serial.print("N, Sens "); Serial.print(LOAD_CELL_RIGHT_SENS_MV); Serial.println(" mV/V");

  // Show raw readings before tare
  Serial.println("\n=== RAW READINGS (BEFORE TARE) ===");
  if (hx711_left.is_ready()) {
    long raw_left = hx711_left.read();
    Serial.print("  Left raw: "); Serial.println(raw_left);
  } else {
    Serial.println("  Left HX711 not ready");
  }

  if (hx711_right.is_ready()) {
    long raw_right = hx711_right.read();
    Serial.print("  Right raw: "); Serial.println(raw_right);

    // Test multiple reads to see if values change
    Serial.println("  Testing right HX711 - reading 5 times:");
    for (int i = 0; i < 5; i++) {
      delay(200);
      if (hx711_right.is_ready()) {
        long test_read = hx711_right.read();
        Serial.print("    Read "); Serial.print(i+1); Serial.print(": ");
        Serial.println(test_read);
      }
    }
  } else {
    Serial.println("  Right HX711 not ready");
  }

  Serial.println("\n=== BUTTON OPERATION ===");
  Serial.println("START (GPIO 4): Press to start system or clear emergency stop");
  Serial.println("STOP (GPIO 5):");
  Serial.println("  - Normal: Pulse LOW (<500ms) to stop");
  Serial.println("  - Emergency: Hold LOW (>500ms) for emergency stop");

  // Tare the load cells
  Serial.println("\n=== TARING LOAD CELLS ===");
  Serial.println("Please ensure no load on sensors...");
  delay(2000);  // Give user time to ensure no load
  tareLoadCells();

  Serial.println("\nSystem ready! Waiting for START command...");
}

// ########################## SEND COMMAND ##########################
void Send(int16_t steer, int16_t speed)
{
  // Clamp speed to valid range
  if (speed > SPEED_COEFFICIENT) speed = SPEED_COEFFICIENT;
  if (speed < -SPEED_COEFFICIENT) speed = -SPEED_COEFFICIENT;

  // Clamp steer to valid range
  if (steer > SPEED_COEFFICIENT) steer = SPEED_COEFFICIENT;
  if (steer < -SPEED_COEFFICIENT) steer = -SPEED_COEFFICIENT;

  // Create command packet
  Command.start    = START_FRAME;
  Command.steer    = steer;
  Command.speed    = speed;  // In SPD_MODE, this represents desired RPM
  Command.checksum = (uint16_t)(Command.start ^ Command.steer ^ Command.speed);

  // Write to Serial2 (hoverboard UART)
  Serial2.write((uint8_t *) &Command, sizeof(Command));

  // DEBUG: Print what we're sending (reduced output)
  // Uncomment the lines below for detailed debug output
  // Serial.print(">> SENT: steer=");
  // Serial.print(steer);
  // Serial.print(" speed=");
  // Serial.print(speed);
  // Serial.print(" checksum=0x");
  // Serial.println(Command.checksum, HEX);
}

// ########################## RECEIVE FEEDBACK ##########################
void Receive()
{
  // Check for new data availability in the Serial buffer
  if (Serial2.available()) {
    incomingByte = Serial2.read();
    bufStartFrame = ((uint16_t)(incomingByte) << 8) | incomingBytePrev;
  }
  else {
    return;
  }

  // Copy received data
  if (bufStartFrame == START_FRAME) {
    p       = (byte *)&NewFeedback;
    *p++    = incomingBytePrev;
    *p++    = incomingByte;
    idx     = 2;
  } else if (idx >= 2 && idx < sizeof(SerialFeedback)) {
    *p++    = incomingByte;
    idx++;
  }

  // Check if we reached the end of the package
  if (idx == sizeof(SerialFeedback)) {
    uint16_t checksum;
    checksum = (uint16_t)(NewFeedback.start ^ NewFeedback.cmd1 ^ NewFeedback.cmd2
                        ^ NewFeedback.speedR_meas ^ NewFeedback.speedL_meas
                        ^ NewFeedback.batVoltage ^ NewFeedback.boardTemp ^ NewFeedback.cmdLed);

    // Check validity of the new data
    if (NewFeedback.start == START_FRAME && checksum == NewFeedback.checksum) {
      // Copy the new data
      memcpy(&Feedback, &NewFeedback, sizeof(SerialFeedback));

          // Print feedback data to debug serial (only every 10th message to reduce spam)
      static int feedbackCounter = 0;
      feedbackCounter++;
      if (feedbackCounter >= 10) {
        feedbackCounter = 0;
        Serial.print("<< FB: SpeedR: "); Serial.print(Feedback.speedR_meas);
        Serial.print(" | SpeedL: "); Serial.print(Feedback.speedL_meas);
        Serial.print(" | BatV: "); Serial.print(Feedback.batVoltage / 100.0);
        Serial.print("V | Temp: "); Serial.print(Feedback.boardTemp / 10.0);
        Serial.println("°C");
      }
    } else {
      Serial.println("Invalid feedback data received");
    }
    idx = 0;
  }

  // Update previous states
  incomingBytePrev = incomingByte;
}

// ########################## MAIN LOOP ##########################
unsigned long iTimeSend = 0;

void loop()
{
  unsigned long timeNow = millis();

  // Check button states (start/stop/emergency)
  checkButtons();

  // Receive feedback from hoverboard
  Receive();

  // Send commands at specified interval
  if (timeNow >= iTimeSend) {
    iTimeSend = timeNow + TIME_SEND;

    // Update load cell readings
    updateLoadCells();

    // Read joystick raw values for debugging
    int xRaw = readADC(JOYSTICK_X_PIN);
    int yRaw = readADC(JOYSTICK_Y_PIN);

    // Read joystick and calculate speed values
    int16_t steer = 0;
    int16_t speed = 0;
    readJoystickAndCalculateSpeed(steer, speed);

    // Send command to hoverboard
    Send(steer, speed);

    // Debug output (reduced frequency to avoid spam)
    static int debugCounter = 0;
    debugCounter++;
    if (debugCounter >= 5) {  // Print every 5th send (every ~500ms)
      debugCounter = 0;

      #if DEBUG_JOYSTICK_RAW
        Serial.print("Joystick X:");
        Serial.print(xRaw);
        Serial.print(" Y:");
        Serial.print(yRaw);
      #endif

      #if DEBUG_CONTROL_OUTPUT
        #if DEBUG_JOYSTICK_RAW
          Serial.print(" | ");
        #endif
        Serial.print(">> ");
        if (systemEnabled) Serial.print("ACTIVE");
        else Serial.print("STOPPED");
        Serial.print(" | Steer: ");
        Serial.print(steer);
        Serial.print(" | Speed: ");
        Serial.print(speed);
      #endif

      #if DEBUG_LOAD_CELLS || DEBUG_HX711_RAW
        Serial.print("Load: ");

        // Left load cell
        if (hx711_left.is_ready()) {
          #if DEBUG_LOAD_CELLS
            Serial.print("L: ");
            Serial.print(loadCellLeftForce, 2);
            Serial.print("N");
          #endif

          #if DEBUG_HX711_RAW
            long raw_left = hx711_left.read();
            Serial.print(" (raw:");
            Serial.print(raw_left);
            Serial.print(")");
          #endif
        } else {
          Serial.print("L: --");
        }

        Serial.print(" | ");

        // Right load cell
        if (hx711_right.is_ready()) {
          #if DEBUG_LOAD_CELLS
            Serial.print("R: ");
            Serial.print(loadCellRightForce, 2);
            Serial.print("N");
          #endif

          #if DEBUG_HX711_RAW
            long raw_right = hx711_right.read();
            Serial.print(" (raw:");
            Serial.print(raw_right);
            Serial.print(")");
          #endif
        } else {
          Serial.print("R: --");
        }
      #endif

      #if DEBUG_CONTROL_OUTPUT || DEBUG_LOAD_CELLS || DEBUG_JOYSTICK_RAW
        Serial.println();  // End the line
      #endif
    }
  }
}

// ########################## EXAMPLE FUNCTIONS ##########################

// Set speed directly (call from your control logic)
void setSpeed(int16_t steer, int16_t speed) {
  Send(steer, speed);
}

// Get battery voltage in volts
float getBatteryVoltage() {
  return Feedback.batVoltage / 100.0;
}

// Get board temperature in Celsius
float getBoardTemperature() {
  return Feedback.boardTemp / 10.0;
}

// Get measured speed (RPM)
int16_t getSpeedLeft() {
  return Feedback.speedL_meas;
}

int16_t getSpeedRight() {
  return Feedback.speedR_meas;
}

// ########################## HX711 CALIBRATION HELPER ##########################
// Calibration procedure for HX711 load cells using bogde library:
//
// STEP 1: Find raw reading with no load
// 1. Upload this code with DEBUG_HX711_RAW = true
// 2. Open Serial Monitor at 115200 baud
// 3. Ensure NO load on sensors
// 4. Read the raw values displayed (should be stable numbers)
// 5. Note these values - they will be used for taring
//
// STEP 2: Find scale factor
// 1. Keep sensors with NO load, press START button to tare
// 2. Place a KNOWN weight on the sensor (e.g., 10 kg = ~98 Newtons)
// 3. Read the raw value change
// 4. Calculate scale: scale = raw_reading_change / known_force_in_newtons
// 5. Update the code in updateLoadCells() function:
//    loadCellLeftForce = raw_left / CALIBRATION_SCALE_LEFT;
//
// Example:
//   No load: raw = 50000
//   With 10kg (98N): raw = 150000
//   Change = 150000 - 50000 = 100000
//   Scale = 100000 / 98 = 1020.4 counts per Newton
//   Then use: loadCellLeftForce = raw_left / 1020.4;
//
// TIPS:
// - The bogde library is very simple - just use read() for raw values
// - You can also use read_average(N) to average N readings for stability
// - Make sure HX711 modules are powered properly (3.3V or 5V)
// - Check wiring if you only get 0 or constant values
// ########################## END ##########################
