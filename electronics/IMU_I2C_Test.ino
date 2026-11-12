#include <Wire.h>
#include <MPU6050_light.h>

// Sensor 1 I2C Pins
const int SDA1_PIN = 8;
const int SCL1_PIN = 9;

// Sensor 2 I2C Pins
const int SDA2_PIN = 6;
const int SCL2_PIN = 7;

// Both sensors share the single hardware Wire bus by switching pins
MPU6050 mpu1(Wire);
MPU6050 mpu2(Wire);

// =========================================================================
// TOGGLE ANGLE STREAMS HERE (true = print, false = hide)
// Order: { AngleX(deg), AngleY(deg), AngleZ(deg) }
// =========================================================================
const int NUM_STREAMS = 3;

bool enabledStreams[NUM_STREAMS] = {
  true,  // 0: AngleX (deg)
  true,  // 1: AngleY (deg)
  true   // 2: AngleZ (deg)
};

const char* streamNames[NUM_STREAMS] = {
  "AngleX_deg", "AngleY_deg", "AngleZ_deg"
};

bool mpu1_ok = false;
bool mpu2_ok = false;

unsigned long last_print_ms = 0;

// Helper to switch the ESP32-C3 hardware I2C pins cleanly
void selectBus(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl);
  Wire.setClock(400000);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // 1. Initialize Sensor 1
  selectBus(SDA1_PIN, SCL1_PIN);
  byte status1 = mpu1.begin();
  if (status1 == 0) {
    mpu1_ok = true;
    Serial.println("MPU1 (Pins 8/9) connected!");
  } else {
    Serial.print("Warning: MPU1 error code ");
    Serial.println(status1);
  }

  // 2. Initialize Sensor 2
  selectBus(SDA2_PIN, SCL2_PIN);
  byte status2 = mpu2.begin();
  if (status2 == 0) {
    mpu2_ok = true;
    Serial.println("MPU2 (Pins 6/7) connected!");
  } else {
    Serial.print("Warning: MPU2 error code ");
    Serial.println(status2);
  }

  // 3. Calibrate offsets (Keep robot stationary in its upright balance position)
  Serial.println("Calibrating offsets... do not move sensors.");
  delay(1000);

  if (mpu1_ok) {
    selectBus(SDA1_PIN, SCL1_PIN);
    mpu1.calcOffsets(true, true);
  }
  if (mpu2_ok) {
    selectBus(SDA2_PIN, SCL2_PIN);
    mpu2.calcOffsets(true, true);
  }

  // 4. Print CSV Column Headings
  bool firstColumn = true;
  for (int sensor = 1; sensor <= 2; sensor++) {
    for (int i = 0; i < NUM_STREAMS; i++) {
      if (enabledStreams[i]) {
        if (!firstColumn) Serial.print(",");
        Serial.print("S");
        Serial.print(sensor);
        Serial.print("_");
        Serial.print(streamNames[i]);
        firstColumn = false;
      }
    }
  }
  Serial.println();
}

void loop() {
  // Continuously update sensor fusion math as fast as possible
  if (mpu1_ok) {
    selectBus(SDA1_PIN, SCL1_PIN);
    mpu1.update();
  }
  if (mpu2_ok) {
    selectBus(SDA2_PIN, SCL2_PIN);
    mpu2.update();
  }

  // Print angles in degrees every 10ms (100Hz)
  if (millis() - last_print_ms >= 10) {
    last_print_ms = millis();

    // Store in 64-bit double for maximum precision
    double angles1[NUM_STREAMS] = {
      mpu1_ok ? (double)mpu1.getAngleX() : 0.0,
      mpu1_ok ? (double)mpu1.getAngleY() : 0.0,
      mpu1_ok ? (double)mpu1.getAngleZ() : 0.0
    };

    double angles2[NUM_STREAMS] = {
      mpu2_ok ? (double)mpu2.getAngleX() : 0.0,
      mpu2_ok ? (double)mpu2.getAngleY() : 0.0,
      mpu2_ok ? (double)mpu2.getAngleZ() : 0.0
    };

    bool firstColumn = true;

    // Print Sensor 1 degrees with 15 decimal places (max IEEE-754 double precision)
    for (int i = 0; i < NUM_STREAMS; i++) {
      if (enabledStreams[i]) {
        if (!firstColumn) Serial.print(",");
        Serial.print(angles1[i], 15);
        firstColumn = false;
      }
    }

    // Print Sensor 2 degrees with 15 decimal places
    for (int i = 0; i < NUM_STREAMS; i++) {
      if (enabledStreams[i]) {
        if (!firstColumn) Serial.print(",");
        Serial.print(angles2[i], 15);
        firstColumn = false;
      }
    }

    Serial.println();
  }
}
