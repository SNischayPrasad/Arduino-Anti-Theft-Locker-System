# Blynk Setup

1. In Blynk Console, create a template for an **ESP32 / WiFi** device.
2. Copy `HeartbeatMonitor/HeartbeatSecrets.example.h` to
   `HeartbeatMonitor/HeartbeatSecrets.h`, then add the template ID, template
   name, device auth token, Wi-Fi name, and Wi-Fi password. The real secrets
   file is ignored by Git.
3. Add these template datastreams:

   | Virtual pin | Type | Direction | Suggested range |
   |---|---|---|---|
   | V0 | Integer | Device to cloud | 0-220 BPM |
   | V1 | String | Device to cloud | Heart-rate status |
   | V2 | Integer | Device and app | 30-99, default 60 |
   | V3 | Integer | Device and app | 61-220, default 100 |
   | V4 | Integer | Device to cloud | 0-1023 raw signal |

4. Add template events with codes `high_heart_rate` and `low_heart_rate`.
   Enable **Push notification** for both events. Event codes must match the
   sketch exactly for `Blynk.logEvent(...)` to notify the phone.
5. In the Blynk mobile dashboard, add a gauge/value widget for V0, a label for
   V1, and numeric input or slider widgets for V2 and V3.
6. Install the Arduino libraries **Blynk** and **PulseSensor Playground**,
   select an ESP32 Dev Module, and upload `HeartbeatMonitor.ino`.

The production defaults are 60-100 BPM. For a broader test range, set V2 to 50
and V3 to 120. The sketch rejects invalid values unless
`30 <= low threshold < high threshold <= 220`.
