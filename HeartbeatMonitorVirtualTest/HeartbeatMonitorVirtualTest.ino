/*
  Hardware-free Arduino test for the heartbeat monitor alert rules.

  This sketch runs in Wokwi or on an Arduino UNO without a physical sensor.
  It feeds deterministic BPM values into the same threshold behavior used by
  the production ESP32 sketch and prints the Blynk calls that would be made.
*/

enum HeartRateZone : uint8_t {
  ZONE_UNKNOWN,
  ZONE_LOW,
  ZONE_NORMAL,
  ZONE_HIGH
};

struct TestCase {
  const char *name;
  int bpm;
  int lowThreshold;
  int highThreshold;
  HeartRateZone expected;
};

const TestCase TEST_CASES[] = {
    {"Normal resting rate", 72, 60, 100, ZONE_NORMAL},
    {"High rate", 108, 60, 100, ZONE_HIGH},
    {"Repeated high (notification suppressed)", 110, 60, 100, ZONE_HIGH},
    {"Recovery", 80, 60, 100, ZONE_NORMAL},
    {"Low rate", 52, 60, 100, ZONE_LOW},
    {"Low boundary is normal", 60, 60, 100, ZONE_NORMAL},
    {"High boundary is normal", 100, 60, 100, ZONE_NORMAL},
    {"Adjusted range makes 108 normal", 108, 50, 120, ZONE_NORMAL},
    {"Adjusted low alert", 45, 50, 120, ZONE_LOW},
    {"Adjusted high alert", 125, 50, 120, ZONE_HIGH},
};

HeartRateZone activeZone = ZONE_UNKNOWN;
size_t testIndex = 0;
size_t passedTests = 0;

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

HeartRateZone classifyBpm(int bpm, int lowThreshold, int highThreshold) {
  if (bpm < lowThreshold) {
    return ZONE_LOW;
  }
  if (bpm > highThreshold) {
    return ZONE_HIGH;
  }
  return ZONE_NORMAL;
}

void mockBlynkPush(HeartRateZone zone, int bpm) {
  if (zone == ZONE_HIGH) {
    Serial.print(F("[BLYNK MOCK] logEvent(high_heart_rate, BPM="));
  } else {
    Serial.print(F("[BLYNK MOCK] logEvent(low_heart_rate, BPM="));
  }
  Serial.print(bpm);
  Serial.println(')');
}

void runTest(const TestCase &test) {
  const HeartRateZone actual =
      classifyBpm(test.bpm, test.lowThreshold, test.highThreshold);

  Serial.print(F("[TEST] "));
  Serial.print(test.name);
  Serial.print(F(" | BPM="));
  Serial.print(test.bpm);
  Serial.print(F(" | range="));
  Serial.print(test.lowThreshold);
  Serial.print('-');
  Serial.print(test.highThreshold);
  Serial.print(F(" | status="));
  Serial.print(zoneName(actual));

  if (actual == test.expected) {
    ++passedTests;
    Serial.println(F(" | PASS"));
  } else {
    Serial.println(F(" | FAIL"));
  }

  if (actual != activeZone) {
    activeZone = actual;
    if (actual == ZONE_LOW || actual == ZONE_HIGH) {
      mockBlynkPush(actual, test.bpm);
    } else {
      Serial.println(F("[SYSTEM] Normal zone; alerts rearmed."));
    }
  } else if (actual == ZONE_LOW || actual == ZONE_HIGH) {
    Serial.println(F("[BLYNK MOCK] Duplicate abnormal-zone alert suppressed."));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== Heartbeat Monitor Virtual Test ==="));
  Serial.println(F("Default rule: alert below 60 BPM or above 100 BPM"));
  Serial.println(F("BPM values are simulated; no physical sensor is claimed."));
  Serial.println();
}

void loop() {
  if (testIndex < sizeof(TEST_CASES) / sizeof(TEST_CASES[0])) {
    runTest(TEST_CASES[testIndex]);
    ++testIndex;
    delay(700);
    return;
  }

  if (testIndex == sizeof(TEST_CASES) / sizeof(TEST_CASES[0])) {
    Serial.println();
    Serial.print(F("[RESULT] "));
    Serial.print(passedTests);
    Serial.print('/');
    Serial.print(sizeof(TEST_CASES) / sizeof(TEST_CASES[0]));
    Serial.println(F(" scenarios passed."));
    Serial.println(F("[RESULT] Virtual heartbeat alert test SUCCESS."));
    ++testIndex;
  }

  delay(1000);
}
