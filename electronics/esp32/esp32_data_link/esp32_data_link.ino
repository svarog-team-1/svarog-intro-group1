#include "WiFi.h"
#include "HammingCode.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <MockMPU.h>
#include <WebSocketsServer.h>
#include <cassert>

static constexpr int SERIAL_BAUD = 115200;
static constexpr int UART_BAUD   = 115200;
static constexpr int DATABUFSIZE = 128;
static constexpr int CODESIZE    = 8;

const char* ssid = "esp32";
const uint16_t PORT = 400;

WebSocketsServer webSocket = WebSocketsServer(PORT);
static constexpr int BUFSIZE = 10;
static uint8_t batch[BUFSIZE][DATABUFSIZE / 8];
static uint32_t lastSample = 0;
static int batchCount = 0;


MockMPU mpu;
HardwareSerial ESPSerial(2);
// Uncomment when using real sensor,
// 
// Wire.begin(PININ, PINOUT)

void readSensor() {
  if (batchCount >= BUFSIZE) {
    Serial.println("BUG: readSensor called with full buffer");
    return;
  }

  uint8_t read_buf[(DATABUFSIZE - CODESIZE) / 8];
  uint8_t write_buf[DATABUFSIZE / 8]
  size_t ok = ESPSerial.readBytes(&read_buf, DATABUFSIZE - CODESIZE);


  if (ok == 0) {
    return;
  }

  encode(&read_buf, &batch[batchCount++]);
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
  Serial.begin(SERIAL_BAUD);
  ESPSerial.begin(UART_BAUD, SERIAL_8N1, RX_GPIO, TX_GPIO);

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
      webSocket.broadcastBIN((uint8_t*)batch, (DATABUFSIZE / 8) * batchCount);
      batchCount = 0;
    }
  }
}

