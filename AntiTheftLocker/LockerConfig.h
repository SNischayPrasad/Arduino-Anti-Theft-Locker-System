#pragma once

// --------------------------- User settings ---------------------------
// Replace this placeholder with the destination for security-alert SMS messages.
// Use international format, for example: "+919876543210".
const char ALERT_PHONE_NUMBER[] = "+910000000000";

// Most common 16x2 I2C backpacks use 0x27. Some use 0x3F.
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;

// Relay boards are commonly active LOW. Set to false for an active-HIGH board.
constexpr bool RELAY_ACTIVE_LOW = true;

// How long the solenoid remains unlocked after valid RFID/fingerprint access.
constexpr unsigned long UNLOCK_DURATION_MS = 5000UL;

// Prevents one bad card/finger from flooding the phone with repeated SMS messages.
constexpr unsigned long ALERT_COOLDOWN_MS = 20000UL;

// Default R305 baud rate. Change this only if your sensor was reconfigured.
constexpr uint32_t FINGERPRINT_BAUD = 57600UL;

// SIM800L serial rate. Configure the modem for a fixed 9600 baud if necessary.
constexpr uint32_t GSM_BAUD = 9600UL;

// Every enrolled template ID in this inclusive range is authorized.
constexpr uint16_t FIRST_AUTHORIZED_FINGER_ID = 1;
constexpr uint16_t LAST_AUTHORIZED_FINGER_ID = 20;

// Replace these example UIDs with UIDs printed by the Serial Monitor.
// UID lengths of 4, 7, and 10 bytes are supported.
struct AuthorizedCard {
  uint8_t length;
  uint8_t uid[10];
};

const AuthorizedCard AUTHORIZED_CARDS[] = {
    {4, {0xDE, 0xAD, 0xBE, 0xEF}},
    {7, {0x04, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}},
};
