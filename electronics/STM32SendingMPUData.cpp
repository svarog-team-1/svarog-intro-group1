#include <Arduino.h>
#include <Wire.h>

// 1. Configure I2C3 on PB4 (SDA) and PB8 (SCL)
TwoWire WireMPU(PB4, PB8);

// 2. Configure USART1 on PA10 (RX) and PA9 (TX)
HardwareSerial SerialESP(PA10, PA9);

const uint8_t MPU6050_ADDR    = 0x68;
const uint8_t PWR_MGMT_1_REG  = 0x6B;
const uint8_t GYRO_CONFIG_REG = 0x1B;
const uint8_t GYRO_XOUT_H_REG = 0x43;

float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
unsigned long last_time = 0;

void setup() {
    // Initialize UART to ESP32-C3 at 115200 baud
    SerialESP.begin(115200);

    // Initialize I2C3 on PB4 & PB8
    WireMPU.begin();
    WireMPU.setClock(400000); // 400 kHz Fast Mode I2C

    // Wake up MPU6050
    WireMPU.beginTransmission(MPU6050_ADDR);
    WireMPU.write(PWR_MGMT_1_REG);
    WireMPU.write(0x00);
    WireMPU.endTransmission(true);

    // Set Gyro full-scale range to ±250 °/s (sensitivity = 131.0 LSB/°/s)
    WireMPU.beginTransmission(MPU6050_ADDR);
    WireMPU.write(GYRO_CONFIG_REG);
    WireMPU.write(0x00);
    WireMPU.endTransmission(true);

    last_time = millis();
}

void loop() {
    // Request 6 bytes starting at GYRO_XOUT_H (0x43)
    WireMPU.beginTransmission(MPU6050_ADDR);
    WireMPU.write(GYRO_XOUT_H_REG);
    if (WireMPU.endTransmission(false) == 0) {
        WireMPU.requestFrom(MPU6050_ADDR, (uint8_t)6, (uint8_t)true);

        if (WireMPU.available() == 6) {
            int16_t gx_raw = (WireMPU.read() << 8) | WireMPU.read();
            int16_t gy_raw = (WireMPU.read() << 8) | WireMPU.read();
            int16_t gz_raw = (WireMPU.read() << 8) | WireMPU.read();

            // Convert raw readings to degrees/second
            float gx = gx_raw / 131.0f;
            float gy = gy_raw / 131.0f;
            float gz = gz_raw / 131.0f;

            // Integrate angular velocity over elapsed time (dt)
            unsigned long now = millis();
            float dt = (now - last_time) / 1000.0f;
            last_time = now;

            roll  += gx * dt;
            pitch += gy * dt;
            yaw   += gz * dt;

            // Send formatted CSV line over PA9 (TX) to ESP32
            char tx_buffer[48];
            snprintf(tx_buffer, sizeof(tx_buffer), "%.2f,%.2f,%.2f\n", pitch, yaw, roll);
            SerialESP.print(tx_buffer);
        }
    }

    delay(10); // ~100 Hz update rate
}
