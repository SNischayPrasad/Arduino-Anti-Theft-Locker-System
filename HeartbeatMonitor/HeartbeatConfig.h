#pragma once

// Copy HeartbeatSecrets.example.h to HeartbeatSecrets.h and add the real
// values there. HeartbeatSecrets.h is gitignored. Placeholder values keep the
// sketch compilable before credentials are configured.
#if __has_include("HeartbeatSecrets.h")
#include "HeartbeatSecrets.h"
#else
#define BLYNK_TEMPLATE_ID "TMPL_REPLACE_ME"
#define BLYNK_TEMPLATE_NAME "AI Heartbeat Monitor"
#define BLYNK_AUTH_TOKEN "REPLACE_WITH_DEVICE_AUTH_TOKEN"
constexpr char WIFI_SSID[] = "REPLACE_WITH_WIFI_NAME";
constexpr char WIFI_PASSWORD[] = "REPLACE_WITH_WIFI_PASSWORD";
#endif

// Alert rules requested for the normal deployment.
// Values exactly equal to 60 or 100 BPM are normal. Alerts are sent only when
// BPM is below the low limit or above the high limit.
constexpr int DEFAULT_LOW_BPM_THRESHOLD = 60;
constexpr int DEFAULT_HIGH_BPM_THRESHOLD = 100;

// Adjust this after watching the raw PulseSensor signal in Serial Plotter.
// The ESP32 ADC is set to 10-bit resolution so the PulseSensor Playground
// library and this 0-1023 threshold use the same scale.
constexpr int PULSE_SIGNAL_THRESHOLD = 550;
