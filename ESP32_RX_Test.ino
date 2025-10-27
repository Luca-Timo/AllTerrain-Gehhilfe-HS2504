// Test if ESP32 RX pin is working
// This will echo back anything received

#define RX_PIN  44
#define TX_PIN  43

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 RX Test");
  Serial.println("=============");
  Serial.print("RX Pin: GPIO ");
  Serial.println(RX_PIN);
  Serial.print("TX Pin: GPIO ");
  Serial.println(TX_PIN);
  Serial.println();
  Serial.println("Listening for data from hoverboard...");
  Serial.println("If you see HEX data below, RX is working!");
  Serial.println();

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  // Send command (we know this works)
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 100) {
    lastSend = millis();
    uint8_t cmd[8] = {0xCD, 0xAB, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xAB};
    Serial2.write(cmd, 8);
  }

  // Check for ANY data on RX
  if (Serial2.available()) {
    Serial.print(millis());
    Serial.print(" ms: RECEIVED ");
    Serial.print(Serial2.available());
    Serial.print(" bytes: ");

    while (Serial2.available()) {
      uint8_t b = Serial2.read();
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Print status every 2 seconds
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 2000) {
    lastStatus = millis();
    Serial.print(".");  // Still alive indicator
  }
}
