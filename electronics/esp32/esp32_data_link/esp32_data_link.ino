#include "WiFi.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <MockMPU.h>
#include <WebSocketsServer.h>
#include <cassert>

const int DATA_RATE = 115200;
const char* ssid = "esp32";
const uint16_t PORT = 400;

WebSocketsServer webSocket = WebSocketsServer(PORT);
static constexpr int PACKETBUFLEN = 28;
static constexpr int BUFSIZE = 10;
static uint8_t batch[BUFSIZE][PACKETBUFLEN];
static uint32_t lastSample = 0;
static int batchCount = 0;

MockMPU mpu;
// Uncomment when using real sensor,
// 
// Wire.begin(PININ, PINOUT)

void readSensor() {
  if (batchCount >= BUFSIZE) {
    Serial.println("BUG: readSensor called with full buffer");
    return;
  }

  sensors_event_t acc, gyr, _;
  mpu.getEvent(&acc, &gyr, &_);

  // Note to anyone modifying this:
  // Ensure buffer length always matches the data
  uint32_t ts = acc.timestamp;
  float ax = acc.acceleration.x;
  float ay = acc.acceleration.y;
  float az = acc.acceleration.z;
  float gx = gyr.gyro.x;
  float gy = gyr.gyro.y;
  float gz = gyr.gyro.z;

  uint8_t *p = batch[batchCount++];
  auto put = [&p](const auto &v) { memcpy(p, &v, sizeof(v)); p += sizeof(v); };

  put(ts);
  put(ax); put(ay); put(az);
  put(gx); put(gy); put(gz);

  // Comment out / delete on final build...
  if (p - &batch[batchCount-1][0] != PACKETBUFLEN) {
    Serial.printf("BUG: packet size %d, expected %d\n",
                  (int)(p - &batch[batchCount-1][0]), PACKETBUFLEN);  
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      break;
  }
}

void setup() {
  Serial.begin(DATA_RATE);
  mpu.begin();

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  Serial.print("Setting AP (Access Point)...");
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  if (micros() - lastSample >= 2000) {  // 500 Hz
    lastSample = micros();
    readSensor();
    
    if (batchCount == BUFSIZE) {
      webSocket.broadcastBIN((uint8_t*)batch, PACKETBUFLEN * batchCount);
      batchCount = 0;
    }
  }
}

