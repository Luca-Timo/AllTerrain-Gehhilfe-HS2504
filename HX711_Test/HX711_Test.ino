// *******************************************************************
//  HX711 Load Cell Test Program for ESP32-S3
//  AllTerrain Gehhilfe - HS2504
//
//  Simple diagnostic program to test HX711 load cell amplifiers
//  This will help identify wiring and hardware issues
// *******************************************************************
// REQUIRED LIBRARY:
// Install "HX711" by Bogdan Necula (bogde)
// GitHub: https://github.com/bogde/HX711
// Arduino Library Manager -> Search "HX711 Arduino Library" -> Install
// *******************************************************************



// Pin definitions - HX711 Load Cell Amplifiers
#define HX711_LEFT_DOUT       11        // GPIO 11: Left HX711 data out
#define HX711_LEFT_SCK        12        // GPIO 12: Left HX711 clock
#define HX711_RIGHT_DOUT      13        // GPIO 13: Right HX711 data out
#define HX711_RIGHT_SCK       14        // GPIO 14: Right HX711 clock



#include "Adafruit_HX711.h"

// Define the pins for the HX711 communication
const uint8_t DATA_PIN = 13;  // Can use any pins!
const uint8_t CLOCK_PIN = 14; // Can use any pins!


Adafruit_HX711 hx711(DATA_PIN, CLOCK_PIN);
void setup() {
  Serial.begin(115200);
  // wait for serial port to connect. Needed for native USB port only
  while (!Serial) {
    delay(10);
  }

  Serial.println("Adafruit HX711 Test!");
  // Initialize the HX711
  hx711.begin();
  // read and toss 3 values each
  Serial.println("Tareing....");
  for (uint8_t t=0; t<3; t++) {
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
  }
}

void loop() {
  // Read from Channel A with Gain 128, can also try CHAN_A_GAIN_64 or CHAN_B_GAIN_32
  // since the read is blocking this will not be more than 10 or 80 SPS (L or H switch)
  int32_t weightA128 = hx711.readChannelBlocking(CHAN_A_GAIN_128);
  Serial.print("Channel A (Gain 128): ");
  Serial.println(weightA128);
  // Read from Channel A with Gain 128, can also try CHAN_A_GAIN_64 or CHAN_B_GAIN_32
  int32_t weightB32 = hx711.readChannelBlocking(CHAN_B_GAIN_32);
  Serial.print("Channel B (Gain 32): ");
  Serial.println(weightB32);
}