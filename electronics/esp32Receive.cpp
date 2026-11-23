#include <Arduino.h>

#define RX_PIN 21  // Connected to STM32 PA9 (TX)
#define TX_PIN 20  // Connected to STM32 PA10 (RX)

void setup() {
    Serial.begin(115200); // USB serial monitor
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial1.setTimeout(20); // Prevent blocking if a packet is incomplete
}

void loop() {
    if (Serial1.available()) {
        String data = Serial1.readStringUntil('\n');
        
        float pitch, yaw, roll;
        if (sscanf(data.c_str(), "%f,%f,%f", &pitch, &yaw, &roll) == 3) {
            Serial.printf("Pitch: %.2f | Yaw: %.2f | Roll: %.2f\n", pitch, yaw, roll);
        }
    }
}
