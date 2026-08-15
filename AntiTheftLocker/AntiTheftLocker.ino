/*
  Anti-Theft Locker System
  Board: Arduino UNO

  Authentication: MFRC522 RFID OR R305 fingerprint
  User feedback:  16x2 I2C LCD + Serial Monitor
  Lock actuator:  5 V relay module controlling a separately powered solenoid
  Alerts:         SIM800L SMS when an unknown RFID card/fingerprint is presented

  IMPORTANT: The solenoid and SIM800L need suitable external power supplies.
  Connect all supply grounds to Arduino GND. See docs/WIRING.md.
*/

#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "LockerConfig.h"

// ----------------------------- UNO pins -----------------------------
constexpr uint8_t FINGER_RX_PIN = 2;  // UNO RX  <- R305 TX
constexpr uint8_t FINGER_TX_PIN = 3;  // UNO TX  -> R305 RX
constexpr uint8_t GSM_RX_PIN = 4;     // UNO RX  <- SIM800L TX
constexpr uint8_t GSM_TX_PIN = 5;     // UNO TX  -> SIM800L RX (use divider)
constexpr uint8_t RELAY_PIN = 7;
constexpr uint8_t RFID_RST_PIN = 9;
constexpr uint8_t RFID_SS_PIN = 10;
// Hardware SPI: MOSI=11, MISO=12, SCK=13. I2C: SDA=A4, SCL=A5.

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
SoftwareSerial fingerprintSerial(FINGER_RX_PIN, FINGER_TX_PIN);
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);
Adafruit_Fingerprint finger(&fingerprintSerial);

bool fingerprintOnline = false;
bool gsmOnline = false;
bool fingerAttemptLatched = false;
bool alertAttemptedOnce = false;
unsigned long lastAlertAttemptAt = 0;

void setLock(bool unlocked) {
  const uint8_t activeLevel = RELAY_ACTIVE_LOW ? LOW : HIGH;
  const uint8_t inactiveLevel = RELAY_ACTIVE_LOW ? HIGH : LOW;
  digitalWrite(RELAY_PIN, unlocked ? activeLevel : inactiveLevel);
}

void showStatus(const __FlashStringHelper *line1,
                const __FlashStringHelper *line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void showReady() {
  showStatus(F("Locker secured"), F("Scan card/finger"));
}

void drainGsmInput() {
  while (gsmSerial.available()) {
    gsmSerial.read();
  }
}

bool waitForGsmToken(const char *token, unsigned long timeoutMs) {
  char response[72];
  size_t used = 0;
  const unsigned long startedAt = millis();

  while (millis() - startedAt < timeoutMs) {
    while (gsmSerial.available()) {
      const char incoming = static_cast<char>(gsmSerial.read());
      Serial.write(incoming);

      if (used < sizeof(response) - 1) {
        response[used++] = incoming;
      } else {
        memmove(response, response + 1, sizeof(response) - 2);
        response[sizeof(response) - 2] = incoming;
        used = sizeof(response) - 1;
      }
      response[used] = '\0';

      if (strstr(response, token) != nullptr) {
        return true;
      }
      if (strstr(response, "ERROR") != nullptr) {
        return false;
      }
    }
  }
  return false;
}

bool sendGsmCommand(const __FlashStringHelper *command,
                    const char *expected,
                    unsigned long timeoutMs = 1500UL) {
  drainGsmInput();
  gsmSerial.println(command);
  return waitForGsmToken(expected, timeoutMs);
}

bool phoneNumberConfigured() {
  return strlen(ALERT_PHONE_NUMBER) >= 8 &&
         strcmp(ALERT_PHONE_NUMBER, "+910000000000") != 0;
}

bool initializeGsm() {
  gsmSerial.listen();
  Serial.println(F("[GSM] Initializing SIM800L..."));

  bool responded = false;
  for (uint8_t attempt = 0; attempt < 4 && !responded; ++attempt) {
    responded = sendGsmCommand(F("AT"), "OK", 2000UL);
    if (!responded) {
      delay(500);
    }
  }

  if (responded) {
    sendGsmCommand(F("ATE0"), "OK");
    responded = sendGsmCommand(F("AT+CMGF=1"), "OK");
  }

  fingerprintSerial.listen();
  Serial.println(responded ? F("[GSM] Modem ready.")
                           : F("[GSM] Modem not detected; alerts disabled."));
  return responded;
}

bool sendUnauthorizedSms(const __FlashStringHelper *reason,
                         const char *details) {
  if (!gsmOnline || !phoneNumberConfigured()) {
    Serial.println(F("[ALERT] SMS skipped: check modem and phone number."));
    return false;
  }

  gsmSerial.listen();
  showStatus(F("Unauthorized!"), F("Sending SMS..."));
  Serial.println(F("[ALERT] Sending security SMS..."));

  bool sent = sendGsmCommand(F("AT+CMGF=1"), "OK");
  if (sent) {
    drainGsmInput();
    gsmSerial.print(F("AT+CMGS=\""));
    gsmSerial.print(ALERT_PHONE_NUMBER);
    gsmSerial.println(F("\""));
    sent = waitForGsmToken(">", 3000UL);
  }

  if (sent) {
    gsmSerial.print(F("SECURITY ALERT: Unauthorized locker access via "));
    gsmSerial.print(reason);
    gsmSerial.print(F(". Details: "));
    gsmSerial.print(details);
    gsmSerial.print(F(". Check the locker immediately."));
    gsmSerial.write(26);  // Ctrl+Z sends the SMS.
    sent = waitForGsmToken("+CMGS:", 15000UL);
  }

  fingerprintSerial.listen();
  Serial.println(sent ? F("[ALERT] SMS sent.") : F("[ALERT] SMS failed."));
  return sent;
}

void attemptSecurityAlert(const __FlashStringHelper *reason,
                          const char *details) {
  const unsigned long now = millis();
  if (alertAttemptedOnce && now - lastAlertAttemptAt < ALERT_COOLDOWN_MS) {
    Serial.println(F("[ALERT] Cooldown active; duplicate SMS suppressed."));
    return;
  }

  alertAttemptedOnce = true;
  lastAlertAttemptAt = now;
  sendUnauthorizedSms(reason, details);
}

void unlockLocker(const __FlashStringHelper *method) {
  Serial.print(F("[ACCESS] Granted by "));
  Serial.println(method);
  showStatus(F("Access granted"), F("Locker unlocked"));
  setLock(true);
  delay(UNLOCK_DURATION_MS);
  setLock(false);
  showStatus(F("Locker locked"), F("Secured again"));
  delay(1200);
  showReady();
}

bool isAuthorizedCard(const MFRC522::Uid &candidate) {
  for (size_t cardIndex = 0;
       cardIndex < sizeof(AUTHORIZED_CARDS) / sizeof(AUTHORIZED_CARDS[0]);
       ++cardIndex) {
    const AuthorizedCard &allowed = AUTHORIZED_CARDS[cardIndex];
    if (candidate.size != allowed.length) {
      continue;
    }

    bool matches = true;
    for (uint8_t i = 0; i < candidate.size; ++i) {
      if (candidate.uidByte[i] != allowed.uid[i]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return true;
    }
  }
  return false;
}

void formatUid(const MFRC522::Uid &uid, char *output, size_t outputSize) {
  const char hexDigits[] = "0123456789ABCDEF";
  size_t position = 0;
  for (uint8_t i = 0; i < uid.size && position + 3 < outputSize; ++i) {
    if (i > 0) {
      output[position++] = ':';
    }
    output[position++] = hexDigits[(uid.uidByte[i] >> 4) & 0x0F];
    output[position++] = hexDigits[uid.uidByte[i] & 0x0F];
  }
  output[position] = '\0';
}

void checkRfid() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  char uidText[31];
  formatUid(rfid.uid, uidText, sizeof(uidText));
  const bool authorized = isAuthorizedCard(rfid.uid);

  // Finish communication before any LCD/GSM delay.
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  Serial.print(F("[RFID] UID: "));
  Serial.println(uidText);

  if (authorized) {
    unlockLocker(F("RFID"));
  } else {
    Serial.println(F("[ACCESS] Denied: unknown RFID card."));
    showStatus(F("Access denied"), F("Unknown RFID"));
    attemptSecurityAlert(F("RFID"), uidText);
    delay(1200);
    showReady();
  }
}

void checkFingerprint() {
  if (!fingerprintOnline) {
    return;
  }

  fingerprintSerial.listen();
  uint8_t result = finger.getImage();

  if (result == FINGERPRINT_NOFINGER) {
    fingerAttemptLatched = false;
    return;
  }
  if (fingerAttemptLatched) {
    return;
  }
  fingerAttemptLatched = true;

  if (result != FINGERPRINT_OK) {
    Serial.println(F("[FINGER] Image capture error; retry."));
    return;
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) {
    Serial.println(F("[FINGER] Image was unclear; access denied."));
    showStatus(F("Try finger again"), F("Image unclear"));
    return;
  }

  result = finger.fingerFastSearch();
  if (result == FINGERPRINT_OK &&
      finger.fingerID >= FIRST_AUTHORIZED_FINGER_ID &&
      finger.fingerID <= LAST_AUTHORIZED_FINGER_ID) {
    Serial.print(F("[FINGER] Authorized template ID: "));
    Serial.println(finger.fingerID);
    unlockLocker(F("fingerprint"));
    return;
  }

  char details[18];
  if (result == FINGERPRINT_OK) {
    strcpy(details, "template outside");
  } else {
    strcpy(details, "no match");
  }

  Serial.println(F("[ACCESS] Denied: fingerprint not authorized."));
  showStatus(F("Access denied"), F("Unknown finger"));
  attemptSecurityAlert(F("fingerprint"), details);
  delay(1200);
  showReady();
}

void setup() {
  // Set the safe relay level before switching the pin to output mode.
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  setLock(false);

  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println(F("\nAnti-Theft Locker System starting..."));

  lcd.init();
  lcd.backlight();
  showStatus(F("Anti-Theft"), F("Locker starting"));

  SPI.begin();
  rfid.PCD_Init();
  Serial.println(F("[RFID] Reader initialized."));

  fingerprintSerial.begin(FINGERPRINT_BAUD);
  finger.begin(FINGERPRINT_BAUD);
  fingerprintSerial.listen();
  fingerprintOnline = finger.verifyPassword();
  Serial.println(fingerprintOnline
                     ? F("[FINGER] Sensor verified.")
                     : F("[FINGER] Sensor not detected; RFID still available."));

  gsmSerial.begin(GSM_BAUD);
  gsmOnline = initializeGsm();

  if (!phoneNumberConfigured()) {
    Serial.println(F("[CONFIG] Set ALERT_PHONE_NUMBER before deployment."));
  }

  showReady();
  Serial.println(F("[SYSTEM] Ready. Present a card or finger."));
}

void loop() {
  checkFingerprint();
  checkRfid();
  delay(30);
}
