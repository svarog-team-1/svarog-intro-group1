#include <Wire.h>
#include <AS5600.h>

// Instantiate the encoder object
AS5600 encoder;

//• VCC \(\rightarrow \) 3.3V (Powers the sensor)
//• GND \(\rightarrow \) GND (Common Ground)
//• SDA \(\rightarrow \) GPIO 16 / D16 (I2C Data line)
//• SCL \(\rightarrow \) GPIO 17 / D17 (I2C Clock line)

const int MY_SDA_PIN = 16; // Connect AS5600 SDA to physical pin 16
const int MY_SCL_PIN = 17; // Connect AS5600 SCL to physical pin 17

void setup() {
  // Start the Serial monitor for debugging at 115200 baud
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for serial monitor to open

  Serial.println("--- AS5600 Encoder Initialization ---");

  // Initialize the I2C bus using your custom pins (SDA, SCL)
  Wire.begin(MY_SDA_PIN, MY_SCL_PIN);

  // Initialize the encoder module
  if (!encoder.begin()) {
    Serial.println("Error: AS5600 encoder not detected! Check your wiring.");
    while (1) {
      delay(1000);
    }
  }

  // Check if the sensor detects the magnet safely
  if (encoder.isConnected()) {
    Serial.println("AS5600 connected successfully.");
  } else {
    Serial.println("Warning: Magnet missing or out of operational distance!");
  }
}

void loop() {
  // 1. Fetch the raw 12-bit position register value (0 to 4095)
  uint16_t raw_position = encoder.readAngle(); 

  // 2. Convert the raw value into degrees (0.00° to 360.00°)
  float angle_degrees = raw_position * (360.0 / 4096.0); 

  // Output formatting to the Serial Monitor
  Serial.println(angle_degrees, 2); // Print with 2 decimal precision

  // Small delay to keep the data flood manageable
  delay(100); 
}
