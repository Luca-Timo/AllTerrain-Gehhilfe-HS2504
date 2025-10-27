// *******************************************************************
//  ESP32-S3 UART Control Example for Hoverboard FOC Firmware
//  AllTerrain Gehhilfe - HS2504
//
//  This example demonstrates UART communication with the hoverboard
//  using torque control mode via USART3 (right sensor cable)
// *******************************************************************
// HARDWARE CONNECTIONS:
// - Hoverboard TX (PA2) → ESP32-S3 RX (GPIO 16 or your choice)
// - Hoverboard RX (PA3) → ESP32-S3 TX (GPIO 17 or your choice)
// - GND → GND
// - DO NOT CONNECT THE 15V WIRE FROM THE SENSOR CABLE!
//
// HOVERBOARD CONFIGURATION (Inc/config.h):
// - VARIANT_USART is enabled
// - CONTROL_SERIAL_USART3 is enabled
// - FEEDBACK_SERIAL_USART3 is enabled
// - CTRL_TYP_SEL = FOC_CTRL
// - CTRL_MOD_REQ = TRQ_MODE (torque control)
// *******************************************************************

// ########################## DEFINES ##########################
#define HOVER_SERIAL_BAUD   115200      // Baud rate for communication with hoverboard
#define START_FRAME         0xABCD      // Start frame definition for reliable serial communication
#define TIME_SEND           100         // [ms] Sending time interval
#define TORQUE_MAX          1000        // [-] Maximum torque command value (-1000 to +1000)

// Pin definitions (adjust to your ESP32-S3 board)
// GPIO 44 doesn't work for RX on this board - use GPIO 18 instead!
#define RX_PIN              18          // ESP32-S3 RX pin (GPIO 18 works!)
#define TX_PIN              43          // ESP32-S3 TX pin (GPIO 43 works!)

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

// ########################## SETUP ##########################
void setup()
{
  // Initialize debug serial
  Serial.begin(115200);
  Serial.println("ESP32-S3 Hoverboard UART Control - Torque Mode");
  Serial.println("===============================================");

  // Initialize hardware serial for hoverboard communication
  Serial2.begin(HOVER_SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

  // Wait for serial to stabilize
  delay(100);

  Serial.println("Ready! Sending zero torque commands...");
}

// ########################## SEND COMMAND ##########################
void Send(int16_t steer, int16_t torque)
{
  // Clamp torque to valid range
  if (torque > TORQUE_MAX) torque = TORQUE_MAX;
  if (torque < -TORQUE_MAX) torque = -TORQUE_MAX;

  // Clamp steer to valid range
  if (steer > TORQUE_MAX) steer = TORQUE_MAX;
  if (steer < -TORQUE_MAX) steer = -TORQUE_MAX;

  // Create command packet
  Command.start    = START_FRAME;
  Command.steer    = steer;
  Command.speed    = torque;  // In TRQ_MODE, this represents torque
  Command.checksum = (uint16_t)(Command.start ^ Command.steer ^ Command.speed);

  // Write to Serial2 (hoverboard UART)
  Serial2.write((uint8_t *) &Command, sizeof(Command));

  // DEBUG: Print what we're sending
  Serial.print(">> SENT: steer=");
  Serial.print(steer);
  Serial.print(" torque=");
  Serial.print(torque);
  Serial.print(" checksum=0x");
  Serial.println(Command.checksum, HEX);
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

      // Print feedback data to debug serial
      Serial.print("Cmd1: ");   Serial.print(Feedback.cmd1);
      Serial.print(" | Cmd2: "); Serial.print(Feedback.cmd2);
      Serial.print(" | SpeedR: "); Serial.print(Feedback.speedR_meas);
      Serial.print(" | SpeedL: "); Serial.print(Feedback.speedL_meas);
      Serial.print(" | BatV: "); Serial.print(Feedback.batVoltage / 100.0);
      Serial.print("V | Temp: "); Serial.print(Feedback.boardTemp / 10.0);
      Serial.println("°C");
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
int16_t testTorque = 0;
int16_t torqueStep = 10;  // SAFE BENCH TEST: Slow ramp (was 50)

void loop()
{
  unsigned long timeNow = millis();

  // Receive feedback from hoverboard
  Receive();

  // Send commands at specified interval
  if (timeNow >= iTimeSend) {
    iTimeSend = timeNow + TIME_SEND;

    // SAFE BENCH TEST: Slowly ramp up and down torque for testing
    // Limits: ±100 torque (10% of max) with slow 10-step increments
    // Replace this with your actual torque control logic
    Send(0, testTorque);

    // Update test torque - SAFE VALUES FOR FIRST TEST
    testTorque += torqueStep;
    if (testTorque >= 100 || testTorque <= -100) {  // SAFE: Max ±100 (was ±300)
      torqueStep = -torqueStep;
    }
  }
}

// ########################## EXAMPLE FUNCTIONS ##########################

// Set torque directly (call from your control logic)
void setTorque(int16_t steer, int16_t torque) {
  Send(steer, torque);
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

// ########################## END ##########################
