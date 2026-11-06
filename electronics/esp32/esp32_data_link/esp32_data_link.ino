#include "WiFi.h"
#include <string>

const int DATA_RATE = 115200;
const char* ssid = "esp32";
const uint16_t PORT = 400;
const int TEST_VAL = 42;

String header;

String output;

WiFiServer server(PORT);

void setup() {
  Serial.begin(DATA_RATE);
  pinMode(22, INPUT);
  pinMode(24, OUTPUT);
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void handleClientTask(void *pvParameters) {
  WiFiClient client = *(WiFiClient*)pvParameters;
  delete (WiFiClient*)pvParameters;

  Serial.println("Task started for new client.");

  String currentLine = "";
  String header = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      header += c;
      if (c == '\n') {
        if (currentLine.length() == 0) {
          // ---- Your original HTTP response logic ----
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();

          // Example: only return a simple page to verify concurrency
          
          client.println("<!DOCTYPE html><html><body><h1> Test Value " + String(TEST_VAL) + "</h1></body></html>");
          client.println();
          break; // One request per connection
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
    vTaskDelay(1); // Give up CPU to prevent starving other tasks
  }
}

void loop() {
  if (server.hasClient()) {
    WiFiClient newClient = server.available();
    WiFiClient lastAcceptedClient;
    if (newClient && newClient != lastAcceptedClient) {
      lastAcceptedClient = newClient;

      // Allocate heap memory on the heap to pass client to the task
      WiFiClient *clientPtr = new WiFiClient(newClient);

      xTaskCreatePinnedToCore(
        handleClientTask,   // Task function
        "HTTPClientTask",   // Task name
        4096,               // Stack size (adjust based on your HTML length)
        (void*)clientPtr,   // Parameters
        1,                  // Priority
        NULL,               // Task handle
        1                   // Pin to core 1 (core 1 typically handles application logic)
      );
    }
  }
}
