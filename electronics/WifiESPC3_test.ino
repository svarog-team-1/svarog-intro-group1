#include <Wire.h>
#include <MPU6050_light.h>
#include <WiFi.h>
#include <WebServer.h>

// =========================================================================
// 1. DIRECT WIFI HOTSPOT SETTINGS
// =========================================================================
const char* AP_SSID     = "ESP32C3_Direct";
const char* AP_PASSWORD = "password123";

WebServer server(80);

// =========================================================================
// 2. DUAL MPU6050 SENSOR SETTINGS
// =========================================================================
const int SDA1_PIN = 8;
const int SCL1_PIN = 9;

const int SDA2_PIN = 6;
const int SCL2_PIN = 7;

MPU6050 mpu1(Wire);
MPU6050 mpu2(Wire);

bool mpu1_ok = false;
bool mpu2_ok = false;

// Latest 6 angles in degrees (double precision)
double s1_x = 0.0, s1_y = 0.0, s1_z = 0.0;
double s2_x = 0.0, s2_y = 0.0, s2_z = 0.0;

// =========================================================================
// 3. LIVE BROWSER DASHBOARD (HTML + JAVASCRIPT)
// =========================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dual MPU6050 Live Angles</title>
  <style>
    body { font-family: system-ui, sans-serif; background: #121212; color: #f0f0f0; text-align: center; padding: 20px; }
    h1 { margin-bottom: 5px; font-size: 1.5rem; }
    p.sub { color: #888; margin-top: 0; margin-bottom: 25px; font-size: 0.9rem; }
    .grid { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; max-width: 800px; margin: 0 auto; }
    .card { background: #1e1e1e; border: 1px solid #333; border-radius: 12px; padding: 20px; width: 320px; }
    .card h2 { margin-top: 0; color: #4db8ff; font-size: 1.2rem; border-bottom: 1px solid #333; padding-bottom: 10px; }
    .row { display: flex; justify-content: space-between; font-size: 1.15rem; margin: 14px 0; font-family: monospace; }
    .val { font-weight: bold; color: #00e676; }
  </style>
</head>
<body>
  <h1>Dual MPU6050 Live Telemetry</h1>
  <p class="sub">Direct WiFi Stream &bull; Updating at 20 Hz</p>
  <div class="grid">
    <div class="card">
      <h2>Sensor 1 (Pins 8 / 9)</h2>
      <div class="row"><span>Angle X:</span><span class="val" id="s1x">0.000000&deg;</span></div>
      <div class="row"><span>Angle Y:</span><span class="val" id="s1y">0.000000&deg;</span></div>
      <div class="row"><span>Angle Z:</span><span class="val" id="s1z">0.000000&deg;</span></div>
    </div>
    <div class="card">
      <h2>Sensor 2 (Pins 6 / 7)</h2>
      <div class="row"><span>Angle X:</span><span class="val" id="s2x">0.000000&deg;</span></div>
      <div class="row"><span>Angle Y:</span><span class="val" id="s2y">0.000000&deg;</span></div>
      <div class="row"><span>Angle Z:</span><span class="val" id="s2z">0.000000&deg;</span></div>
    </div>
  </div>
  <script>
    setInterval(() => {
      fetch('/data')
        .then(r => r.text())
        .then(txt => {
          const v = txt.split(',');
          if (v.length === 6) {
            document.getElementById('s1x').innerText = v[0] + '\u00B0';
            document.getElementById('s1y').innerText = v[1] + '\u00B0';
            document.getElementById('s1z').innerText = v[2] + '\u00B0';
            document.getElementById('s2x').innerText = v[3] + '\u00B0';
            document.getElementById('s2y').innerText = v[4] + '\u00B0';
            document.getElementById('s2z').innerText = v[5] + '\u00B0';
          }
        })
        .catch(() => {});
    }, 50); // Fetch every 50ms (20 Hz)
  </script>
</body>
</html>
)rawliteral";

void selectBus(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl);
  Wire.setClock(400000);
}

// Serves the HTML page once when you open http://192.168.4.1
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// Serves a fast, lightweight CSV string of the 6 angles to the browser's JavaScript
void handleData() {
  char buf[160];
  snprintf(buf, sizeof(buf), "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
           s1_x, s1_y, s1_z, s2_x, s2_y, s2_z);
  server.send(200, "text/plain", buf);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // 1. Initialize Sensor 1
  selectBus(SDA1_PIN, SCL1_PIN);
  if (mpu1.begin() == 0) {
    mpu1_ok = true;
    Serial.println("MPU1 (Pins 8/9) connected!");
  } else {
    Serial.println("Warning: MPU1 not detected.");
  }

  // 2. Initialize Sensor 2
  selectBus(SDA2_PIN, SCL2_PIN);
  if (mpu2.begin() == 0) {
    mpu2_ok = true;
    Serial.println("MPU2 (Pins 6/7) connected!");
  } else {
    Serial.println("Warning: MPU2 not detected.");
  }

  // 3. Calibrate offsets (Keep robot stationary in upright position)
  Serial.println("Calibrating offsets... keep sensors still.");
  delay(1000);

  if (mpu1_ok) {
    selectBus(SDA1_PIN, SCL1_PIN);
    mpu1.calcOffsets(true, true);
  }
  if (mpu2_ok) {
    selectBus(SDA2_PIN, SCL2_PIN);
    mpu2.calcOffsets(true, true);
  }

  // 4. Start Direct WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Prevents SuperMini antenna brownouts

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("WiFi Hotspot & Web Dashboard Ready!");
  Serial.print("Connect laptop to WiFi: "); Serial.println(AP_SSID);
  Serial.print("Open browser to: http://"); Serial.println(WiFi.softAPIP());
}

void loop() {
  // Continuously update both MPUs as fast as possible so angle integration stays accurate
  if (mpu1_ok) {
    selectBus(SDA1_PIN, SCL1_PIN);
    mpu1.update();
    s1_x = (double)mpu1.getAngleX();
    s1_y = (double)mpu1.getAngleY();
    s1_z = (double)mpu1.getAngleZ();
  }

  if (mpu2_ok) {
    selectBus(SDA2_PIN, SCL2_PIN);
    mpu2.update();
    s2_x = (double)mpu2.getAngleX();
    s2_y = (double)mpu2.getAngleY();
    s2_z = (double)mpu2.getAngleZ();
  }

  // Handle incoming browser data requests without blocking the MPU loop
  server.handleClient();
}
