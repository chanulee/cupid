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
const unsigned long BASELINE_DURATION   = 5000;  // 5s for baseline

// --- sensor
// An untouched sensor should read very low. We set the threshold low to detect
// the jump when a user touches the sensor. If it triggers without touch,
// there might be a hardware issue.
const int IDLE_THRESHOLD = 100;
const int SAMPLING_DELAY = 20;     // 20 ms ≈ 50 Hz

// --- simple moving average filter
const int BUF_LEN = 10;
int   buf[BUF_LEN];
byte  idx = 0;

// --- state machine
enum State { IDLE, DETECTING, BASELINE, COLLECTING, COOLDOWN };
State currentState = IDLE;

// --- timers and data
unsigned long stateChangeTime = 0;
float gsrSum = 0;
int   gsrCount = 0;
float baseAvg = 0;

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
          // Stable touch detected, now establish a baseline
          currentState = BASELINE;
          stateChangeTime = millis();
          gsrSum = 0;
          gsrCount = 0;
        }
        // No delay here to be responsive
      }
      break;

    case BASELINE:
      {
        // Keep LED on solid during baseline capture
        digitalWrite(LED_STATUS, HIGH);
        
        float gsrValue = readGSR();
        gsrSum += gsrValue;
        gsrCount++;

        if (millis() - stateChangeTime >= BASELINE_DURATION) {
          // Baseline captured, now start the main collection phase
          baseAvg = (gsrCount > 0) ? (gsrSum / gsrCount) : 0;
          
          Serial.println(0); // Send 0 to frontend to trigger "measuring" viz
          
          currentState = COLLECTING;
          stateChangeTime = millis();
          gsrSum = 0;
          gsrCount = 0;
        }
        delay(SAMPLING_DELAY);
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
          float hugAvg = (gsrCount > 0) ? (gsrSum / gsrCount) : 0;
          float delta = hugAvg - baseAvg;

          // Map delta to a 0-100 score. A negative delta (relaxation) is good.
          // We map a delta of +10 (more tense) to 0, and -30 (relaxed) to 100.
          // These mapping values might need tweaking for best results.
          float score = map(delta, 10, -30, 0, 100);
          score = constrain(score, 0, 100);

          Serial.println(score);

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
