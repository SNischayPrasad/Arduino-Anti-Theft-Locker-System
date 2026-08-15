/*
  AI-Enabled Heartbeat Monitor
  Board: ESP32 DevKit V1
  Sensor: PulseSensor connected to GPIO 34
  Cloud: Blynk IoT

  Blynk datastreams:
    V0 - BPM (integer, output)
    V1 - status (string, output)
    V2 - low BPM threshold (integer, input)
    V3 - high BPM threshold (integer, input)
    V4 - raw pulse signal (integer, output)

  Blynk events (enable push notifications for both in the Blynk Console):
    high_heart_rate
    low_heart_rate
*/

#include "HeartbeatConfig.h"

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>
#include <PulseSensorPlayground.h>
#include <WiFi.h>

constexpr uint8_t PULSE_INPUT_PIN = 34;
constexpr uint8_t ALERT_LED_PIN = 2;
constexpr int MIN_VALID_BPM = 30;
constexpr int MAX_VALID_BPM = 220;

enum HeartRateZone : uint8_t {
  ZONE_UNKNOWN,
  ZONE_LOW,
  ZONE_NORMAL,
  ZONE_HIGH
};

PulseSensorPlayground pulseSensor;
BlynkTimer blynkTimer;

int currentBpm = 0;
int latestRawSignal = 0;
int lowBpmThreshold = DEFAULT_LOW_BPM_THRESHOLD;
int highBpmThreshold = DEFAULT_HIGH_BPM_THRESHOLD;
HeartRateZone activeZone = ZONE_UNKNOWN;

const char *zoneName(HeartRateZone zone) {
  switch (zone) {
    case ZONE_LOW:
      return "LOW";
    case ZONE_NORMAL:
      return "NORMAL";
    case ZONE_HIGH:
      return "HIGH";
    default:
      return "WAITING";
  }
}
HeartRateZone classifyBpm(int bpm) {
  if (bpm < lowBpmThreshold) {
    return ZONE_LOW;
  }
  if (bpm > highBpmThreshold) {
    return ZONE_HIGH;
  }
  return ZONE_NORMAL;
}

void sendAlertEvent(HeartRateZone zone, int bpm) {
  char message[96];

  if (zone == ZONE_HIGH) {
    snprintf(message, sizeof(message),
             "High heart rate detected: %d BPM (limit %d BPM).", bpm,
             highBpmThreshold);
    Blynk.logEvent("high_heart_rate", message);
    Serial.print(F("[BLYNK] Push event high_heart_rate: "));
    Serial.println(message);
  } else if (zone == ZONE_LOW) {
    snprintf(message, sizeof(message),
             "Low heart rate detected: %d BPM (limit %d BPM).", bpm,
             lowBpmThreshold);
    Blynk.logEvent("low_heart_rate", message);
    Serial.print(F("[BLYNK] Push event low_heart_rate: "));
    Serial.println(message);
  }
}

void evaluateHeartRate(int bpm) {
  const HeartRateZone newZone = classifyBpm(bpm);

  Serial.print(F("[PULSE] BPM="));
  Serial.print(bpm);
  Serial.print(F(" | thresholds="));
  Serial.print(lowBpmThreshold);
  Serial.print('-');
  Serial.print(highBpmThreshold);
  Serial.print(F(" | status="));
  Serial.println(zoneName(newZone));

  // Notify only when entering an abnormal zone. A normal reading rearms the
  // monitor, preventing repeated push notifications for every sensor sample.
  if (newZone != activeZone) {
    activeZone = newZone;
    digitalWrite(ALERT_LED_PIN, newZone == ZONE_NORMAL ? LOW : HIGH);

    if (newZone == ZONE_LOW || newZone == ZONE_HIGH) {
      sendAlertEvent(newZone, bpm);
    } else {
      Serial.println(F("[SYSTEM] Heart rate is within the configured range."));
    }
  }
}

void publishToBlynk() {
  if (!Blynk.connected()) {
    return;
  }

  Blynk.virtualWrite(V0, currentBpm);
  Blynk.virtualWrite(V1, zoneName(activeZone));
  Blynk.virtualWrite(V4, latestRawSignal);
}

bool thresholdsAreValid(int lowCandidate, int highCandidate) {
  return lowCandidate >= MIN_VALID_BPM &&
         highCandidate <= MAX_VALID_BPM &&
         lowCandidate < highCandidate;
}

void applyThresholds(int lowCandidate, int highCandidate) {
  if (!thresholdsAreValid(lowCandidate, highCandidate)) {
    Serial.println(F("[CONFIG] Rejected invalid threshold update."));
    return;
  }

  lowBpmThreshold = lowCandidate;
  highBpmThreshold = highCandidate;
  activeZone = ZONE_UNKNOWN;  // Re-evaluate and rearm after a rule change.

  Serial.print(F("[CONFIG] Alert range updated to "));
  Serial.print(lowBpmThreshold);
  Serial.print('-');
  Serial.println(highBpmThreshold);

  if (currentBpm >= MIN_VALID_BPM && currentBpm <= MAX_VALID_BPM) {
    evaluateHeartRate(currentBpm);
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2, V3);
  publishToBlynk();
}

BLYNK_WRITE(V2) {
  applyThresholds(param.asInt(), highBpmThreshold);
}

BLYNK_WRITE(V3) {
  applyThresholds(lowBpmThreshold, param.asInt());
}

void setup() {
  Serial.begin(115200);
  pinMode(ALERT_LED_PIN, OUTPUT);
  digitalWrite(ALERT_LED_PIN, LOW);

  Serial.println(F("\nAI-Enabled Heartbeat Monitor starting..."));
  Serial.print(F("[CONFIG] Initial alert range: "));
  Serial.print(lowBpmThreshold);
  Serial.print('-');
  Serial.println(highBpmThreshold);

  analogReadResolution(10);
  pulseSensor.analogInput(PULSE_INPUT_PIN);
  pulseSensor.setThreshold(PULSE_SIGNAL_THRESHOLD);

  if (!pulseSensor.begin()) {
    Serial.println(F("[ERROR] PulseSensor initialization failed."));
  } else {
    Serial.println(F("[PULSE] Sensor initialized; waiting for a beat."));
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  blynkTimer.setInterval(1000L, publishToBlynk);
}

void loop() {
  Blynk.run();
  blynkTimer.run();

  latestRawSignal = analogRead(PULSE_INPUT_PIN);

  if (pulseSensor.sawStartOfBeat()) {
    const int measuredBpm = pulseSensor.getBeatsPerMinute();

    // Discard start-up noise and physically implausible readings.
    if (measuredBpm >= MIN_VALID_BPM && measuredBpm <= MAX_VALID_BPM) {
      currentBpm = measuredBpm;
      evaluateHeartRate(currentBpm);
    } else {
      Serial.print(F("[PULSE] Ignored invalid reading: "));
      Serial.println(measuredBpm);
    }
  }
}
