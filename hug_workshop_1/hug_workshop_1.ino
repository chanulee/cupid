#include <Arduino.h>

/* ------------------------------------------------------------
 *  Cupid - GSR Interaction Logic
 *  Board  : Arduino Uno
 *  Sensor : GSR Sensor V2  → A3
 * ---------------------------------------------------------- */
const int GSR_PIN      = A3;    // Signal pin
const int LED_STATUS   = 13;    // On-board LED

// --- timing (ms)
const unsigned long DETECTION_DURATION  = 2000;  // 2s
const unsigned long COLLECTION_DURATION = 15000; // 15s
const unsigned long COOLDOWN_DURATION   = 40000; // 40s

// --- sensor
const int IDLE_THRESHOLD = 400;
const int SAMPLING_DELAY = 20;     // 20 ms ≈ 50 Hz

// --- simple moving average filter
const int BUF_LEN = 10;
int   buf[BUF_LEN];
byte  idx = 0;

// --- state machine
enum State { IDLE, DETECTING, COLLECTING, COOLDOWN };
State currentState = IDLE;

// --- timers and data
unsigned long stateChangeTime = 0;
float gsrSum = 0;
int   gsrCount = 0;

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_STATUS, OUTPUT);
  for (int i = 0; i < BUF_LEN; i++) buf[i] = 0; // initialize buffer
}

// ------------------------------------------------------------
void loop() {
  switch (currentState) {
    case IDLE:
      {
        digitalWrite(LED_STATUS, LOW); // LED off during idle
        float gsrValue = readGSR();
        if (gsrValue >= IDLE_THRESHOLD) {
          currentState = DETECTING;
          stateChangeTime = millis();
        }
      }
      break;

    case DETECTING:
      {
        digitalWrite(LED_STATUS, HIGH); // LED on to show it's detecting
        float gsrValue = readGSR();
        if (gsrValue < IDLE_THRESHOLD) {
          currentState = IDLE; // False alarm, go back to idle
        } else if (millis() - stateChangeTime >= DETECTION_DURATION) {
          Serial.println(0); // Send 0 to frontend
          currentState = COLLECTING;
          stateChangeTime = millis();
          gsrSum = 0;
          gsrCount = 0;
        }
        // No delay here to be responsive
      }
      break;

    case COLLECTING:
      {
        // LED blinks during collection to show activity
        digitalWrite(LED_STATUS, (millis() / 500) % 2); 
        
        float gsrValue = readGSR();
        gsrSum += gsrValue;
        gsrCount++;

        if (millis() - stateChangeTime >= COLLECTION_DURATION) {
          float gsrAvg = (gsrCount > 0) ? (gsrSum / gsrCount) : 0;
          Serial.println(gsrAvg);
          currentState = COOLDOWN;
          stateChangeTime = millis();
        }
        delay(SAMPLING_DELAY); // Sample at defined rate
      }
      break;

    case COOLDOWN:
      {
        digitalWrite(LED_STATUS, LOW); // LED off during cooldown
        if (millis() - stateChangeTime >= COOLDOWN_DURATION) {
          currentState = IDLE;
        }
        // No action or delay, just waiting
      }
      break;
  }
}

/* ========== Functions ========== */

// Read GSR with moving average filter
float readGSR() {
  buf[idx] = analogRead(GSR_PIN);          // 0-1023
  idx = (idx + 1) % BUF_LEN;
  long sum = 0;
  for (int i = 0; i < BUF_LEN; i++) sum += buf[i];
  return sum / float(BUF_LEN);
}
