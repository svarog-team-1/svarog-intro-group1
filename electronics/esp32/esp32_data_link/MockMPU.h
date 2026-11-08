// for testing and setup purposes.
// not necessary while using actual MPU.
#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <atomic>
#include <cmath>
#include <cstring>

class MockMPU {
private:
  std::atomic<uint32_t> _startMs;
  const uint32_t _stepMs;
  std::atomic<uint32_t> _step;

  static constexpr uint32_t kNoFault = UINT32_MAX;
  std::atomic<uint32_t> _failAtStep;

public:
  explicit MockMPU(uint32_t stepMs = 2, uint32_t fail_step = kNoFault);

  bool begin();

  void setAccelerometerRange(mpu6050_accel_range_t range);
  void setGyroRange(mpu6050_gyro_range_t range);

  static float accelXAt(uint32_t n);
  static float accelYAt(uint32_t n);
  static float accelZAt(uint32_t n);
  static float gyroXAt(uint32_t n);
  static float gyroYAt(uint32_t n);
  static float gyroZAt(uint32_t n);
  static float tempAt(uint32_t n);

  bool getEvent(sensors_event_t *accel, sensors_event_t *gyro,
                sensors_event_t *temp);
};
