// for testing and setup purposes.
// not necessary while using actual MPU.

#include "MockMPU.h"
#include <Arduino.h> // for millis()
#include <atomic>
#include <cmath>
#include <cstring>

MockMPU::MockMPU(uint32_t stepMs, uint32_t fail_step)
    : _startMs(0), _stepMs(stepMs), _step(0), _failAtStep(fail_step) {}

bool MockMPU::begin() {
  _step.store(0, std::memory_order_release);
  _failAtStep.store(kNoFault, std::memory_order_release);
  _startMs.store(millis(), std::memory_order_release);
  return true;
}

void MockMPU::setAccelerometerRange(mpu6050_accel_range_t) {}
void MockMPU::setGyroRange(mpu6050_gyro_range_t) {}

float MockMPU::accelXAt(uint32_t n) { return 1.0f * n; }
float MockMPU::accelYAt(uint32_t n) { return 2.0f * n; }
float MockMPU::accelZAt(uint32_t n) { return 9.81f + 0.5f * n; }
float MockMPU::gyroXAt(uint32_t n) { return 0.1f * n; }
float MockMPU::gyroYAt(uint32_t n) { return -0.2f * n; }
float MockMPU::gyroZAt(uint32_t n) { return 0.3f * n; }
float MockMPU::tempAt(uint32_t n) { return 25.0f + 0.01f * n; }

bool MockMPU::getEvent(sensors_event_t *accel, sensors_event_t *gyro,
                       sensors_event_t *temp) {
  const uint32_t n = _step.fetch_add(1, std::memory_order_acq_rel);

  const uint32_t startMs = _startMs.load(std::memory_order_acquire);
  const uint32_t failAt = _failAtStep.load(std::memory_order_acquire);

  if (n == failAt) {
    return false;
  }

  const uint64_t tsUs =
      (uint64_t)startMs * 1000ULL + (uint64_t)n * _stepMs * 1000ULL;

  if (accel) {
    memset(accel, 0, sizeof(sensors_event_t));
    accel->version = sizeof(sensors_event_t);
    accel->sensor_id = 0x01;
    accel->type = SENSOR_TYPE_ACCELEROMETER;
    accel->timestamp = tsUs;
    accel->acceleration.x = accelXAt(n);
    accel->acceleration.y = accelYAt(n);
    accel->acceleration.z = accelZAt(n);
  }
  if (gyro) {
    memset(gyro, 0, sizeof(sensors_event_t));
    gyro->version = sizeof(sensors_event_t);
    gyro->sensor_id = 0x02;
    gyro->type = SENSOR_TYPE_GYROSCOPE;
    gyro->timestamp = tsUs;
    gyro->gyro.x = gyroXAt(n);
    gyro->gyro.y = gyroYAt(n);
    gyro->gyro.z = gyroZAt(n);
  }
  if (temp) {
    memset(temp, 0, sizeof(sensors_event_t));
    temp->version = sizeof(sensors_event_t);
    temp->sensor_id = 0x03;
    temp->type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
    temp->timestamp = tsUs;
    temp->temperature = tempAt(n);
  }
  return true;
}
