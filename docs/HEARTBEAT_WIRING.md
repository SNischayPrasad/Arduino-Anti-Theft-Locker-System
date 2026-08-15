# Heartbeat Monitor Wiring

## ESP32 and PulseSensor

| PulseSensor wire | ESP32 DevKit V1 | Purpose |
|---|---|---|
| Purple / Signal | GPIO 34 | Analog pulse waveform input |
| Red / VCC | 3V3 | Sensor power |
| Black / GND | GND | Common ground |

The built-in ESP32 LED on GPIO 2 turns on while the measured heart rate is in
the low or high alert zone. Some boards use a different built-in LED pin; an
external LED with a 220-ohm resistor can be used instead.

Use this as a prototype and learning system, not as a medical device. A hobby
PulseSensor and this code are not validated for diagnosis or emergencies.
