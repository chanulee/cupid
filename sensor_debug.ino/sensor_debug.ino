/*
  GSR Sensor Raw Value Debugger

  This simple sketch reads the analog value from a specified pin
  and prints it to the Serial Monitor. This is useful for debugging
  and calibrating sensors without any extra logic.

  Instructions:
  1. Upload this sketch to your Arduino.
  2. Open the Serial Monitor (Tools > Serial Monitor).
  3. Make sure the baud rate in the Serial Monitor is set to 115200.
  4. Observe the values when the sensor is touched and untouched.
*/

// The pin your GSR sensor's signal wire is connected to.
const int GSR_PIN = A3;

void setup() {
  // Start serial communication at the same baud rate as our main project.
  Serial.begin(115200);
  Serial.println("--- GSR Sensor Debugger Initialized ---");
  Serial.println("Now sending raw values from pin A3...");
}

void loop() {
  // Read the raw integer value from the sensor (0-1023).
  int sensorValue = analogRead(GSR_PIN);

  // Print the value to the Serial Monitor.
  Serial.println(sensorValue);

  // Wait for 100 milliseconds before the next reading to avoid
  // flooding the monitor and make the output readable.
  delay(100);
} 