/* ------------------------------------------------------------
 *  Hug Workshop – GSR Δ-Oxytocin Proxy
 *  Board  : Arduino Nano / Uno
 *  Sensor : GSR Sensor V2  → A0
 *  Author : Hanna’s workshop, 2025
 * ---------------------------------------------------------- */
const int GSR_PIN      = A3;    // Signal pin
const int LED_STATUS   = 13;    // On-board LED

// --- timing (ms)
const unsigned long BASELINE_MS = 10000;   // 10초(= 10 000 ms)
const unsigned long HUG_MS      = 30000;   // 30초(= 30 000 ms)
const int SAMPLING_DELAY        = 20;     // 20 ms ≈ 50 Hz

// --- simple moving average filter
const int BUF_LEN = 10;
int   buf[BUF_LEN];
byte  idx = 0;

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_STATUS, OUTPUT);
  for (int i = 0; i < BUF_LEN; i++) buf[i] = 0;

  Serial.println(F("=== GSR Hug Experiment – ready ==="));
  Serial.println(F(">> 10 s baseline (stay calm)"));
}

// ------------------------------------------------------------
void loop() {
  // -------- 1) Baseline 10 s --------
  digitalWrite(LED_STATUS, LOW);        // LED off = baseline
  float baseSum = 0;  int baseCnt = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < BASELINE_MS) {
    float v = readGSR();
    baseSum += v;  baseCnt++;
    delay(SAMPLING_DELAY);
  }
  float baseAvg = baseSum / baseCnt;
  Serial.print(F("Baseline avg : ")); Serial.println(baseAvg, 1);

  // -------- 2) Hug 30 s -------------
  Serial.println(F(">> Please start hugging (15 s)"));
  digitalWrite(LED_STATUS, HIGH);       // LED on = hug phase
  float hugSum = 0;    int hugCnt = 0;
  t0 = millis();
  while (millis() - t0 < HUG_MS) {
    float v = readGSR();
    hugSum += v;   hugCnt++;
    delay(SAMPLING_DELAY);
  }
  float hugAvg = hugSum / hugCnt;
  Serial.print(F("Hug avg      : ")); Serial.println(hugAvg, 1);

  // -------- 3) Δ 계산 & 점수 ----------
  float delta = hugAvg - baseAvg;     // 음수면 이완
  float score = (-delta) * 0.5;       // 스케일 팩터(0.5) – 자유 조정
  if (score < 0) score = 0;

  Serial.print(F("Δ (hug-base) : ")); Serial.println(delta, 1);
  Serial.print(F("Calm-Score   : ")); Serial.println(score, 1);
  Serial.println(F("-----------------------------------"));

  // LED 깜빡여 결과 알림
  blinkStatus(int(constrain(score / 20, 1, 5)));

  // 다음 참가자를 위해 15 초 대기
  delay(15000);
}

/* ========== 함수들 ========== */

// GSR 읽기 + 이동평균
float readGSR() {
  buf[idx] = analogRead(GSR_PIN);          // 0-1023
  idx = (idx + 1) % BUF_LEN;
  long sum = 0;
  for (int i = 0; i < BUF_LEN; i++) sum += buf[i];
  return sum / float(BUF_LEN);
}

// n번 LED 점멸로 점수 피드백
void blinkStatus(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(200);
    digitalWrite(LED_STATUS, LOW);
    delay(200);
  }
}