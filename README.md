# Arduino Anti-Theft Locker System

> Also included: [AI-Enabled Heartbeat Monitor with ESP32, PulseSensor, and
> Blynk](HeartbeatMonitor/README.md), with a virtual Arduino test and adjustable
> low/high BPM alert thresholds.

An Arduino UNO locker controller that grants access when **either** an authorized MFRC522 RFID card or an enrolled R305 fingerprint matches. It drives a relay/solenoid lock, reports state on a 16x2 I2C LCD, and uses a SIM800L GSM module to send an immediate SMS after an unknown card or fingerprint attempt.

## Features

- RFID or fingerprint authentication
- Five-second unlock pulse with automatic relocking
- 16x2 status messages for ready, granted, denied, and GSM alert states
- SMS details for unknown RFID UIDs and unmatched fingerprints
- Alert cooldown to prevent SMS flooding
- Safe locked relay state established at startup
- Serial diagnostics at 115200 baud
- Hardware-free event-flow demo for development PCs

## Hardware

- Arduino UNO
- MFRC522 RFID reader (3.3 V)
- R305-compatible fingerprint sensor
- 16x2 LCD with I2C backpack
- 5 V relay module and a solenoid lock with its own supply
- SIM800L GSM module, SIM card, antenna, and a suitable external power supply

See [docs/WIRING.md](docs/WIRING.md) for the complete pin map and important power/safety guidance.

## Configure before uploading

Edit `AntiTheftLocker/LockerConfig.h`:

1. Replace `ALERT_PHONE_NUMBER` with the recipient's number in international format.
2. Replace the two sample entries in `AUTHORIZED_CARDS` with your RFID UIDs. Unknown UIDs are printed to the Serial Monitor, making enrollment easy.
3. Confirm `LCD_I2C_ADDRESS` (`0x27` or sometimes `0x3F`).
4. Confirm whether the relay module is active LOW.
5. Enroll authorized fingers into R305 template IDs 1-20, or change the range.

## Required Arduino libraries

Install these through Arduino Library Manager:

- `MFRC522`
- `Adafruit Fingerprint Sensor Library`
- `LiquidCrystal I2C`

`SPI`, `Wire`, and `SoftwareSerial` are supplied with the Arduino AVR core.

## Build and upload

Arduino IDE:

1. Open `AntiTheftLocker/AntiTheftLocker.ino`.
2. Select **Arduino UNO** and the correct COM port.
3. Click **Verify**, then **Upload**.
4. Open Serial Monitor at **115200 baud**.

Arduino CLI:

```powershell
arduino-cli core install arduino:avr
arduino-cli lib install "MFRC522" "Adafruit Fingerprint Sensor Library" "LiquidCrystal I2C"
arduino-cli compile --fqbn arduino:avr:uno AntiTheftLocker
arduino-cli upload -p COM3 --fqbn arduino:avr:uno AntiTheftLocker
```

Replace `COM3` with the port reported for the connected UNO. Uploading cannot be completed unless a physical UNO is connected.

## Hardware-free demonstration

Run this on a PC to exercise the four principal behavior branches without pretending that hardware I/O occurred:

```powershell
python tools/locker_demo.py
```

The demo covers authorized RFID, unauthorized RFID with SMS, authorized fingerprint, and unauthorized fingerprint with SMS.

## Security notes

- MFRC522 UID-only authentication can be cloned; use it for prototypes or pair it with stronger card technology for higher-security deployments.
- The default design uses **OR** authentication, as requested: either factor unlocks. For two-factor security, change the state machine to require both within a short time window.
- Keep the relay and lock wiring inaccessible from outside the enclosure.
- GSM/SMS latency depends on the cellular network; the code begins sending the alert immediately after denial.
