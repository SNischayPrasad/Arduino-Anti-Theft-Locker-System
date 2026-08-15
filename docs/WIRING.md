# Wiring and power guide

## Arduino UNO pin map

| Module | Module pin | Arduino UNO pin | Notes |
|---|---|---:|---|
| MFRC522 | SDA / SS | D10 | SPI chip select |
| MFRC522 | SCK | D13 | Hardware SPI |
| MFRC522 | MOSI | D11 | Hardware SPI |
| MFRC522 | MISO | D12 | Hardware SPI |
| MFRC522 | RST | D9 | Reset |
| MFRC522 | 3.3V | 3.3V | **Never connect to 5 V** |
| MFRC522 | GND | GND | Common ground |
| R305 | TX | D2 | Sensor TX to UNO software RX |
| R305 | RX | D3 | Sensor RX from UNO software TX |
| R305 | VCC/GND | Per module label / GND | Check the exact R305 breakout voltage |
| SIM800L | TXD | D4 | Modem TX to UNO software RX |
| SIM800L | RXD | D5 | Use a divider/level shifter from UNO TX |
| Relay module | IN | D7 | Default code assumes active LOW |
| Relay module | VCC/GND | 5V/GND | Use a suitable 5 V supply |
| I2C LCD | SDA | A4 | I2C data |
| I2C LCD | SCL | A5 | I2C clock |
| I2C LCD | VCC/GND | 5V/GND | Address defaults to `0x27` |

## Solenoid circuit

Connect the relay **COM** and **NO** contacts in series with the solenoid's own rated power supply. Add a flyback diode across the DC solenoid coil (cathode/stripe to positive). Do not power the solenoid from the Arduino 5 V pin.

## SIM800L power warning

A bare SIM800L typically needs about 4.0 V with bursts near 2 A. Do not power it from the UNO 5 V pin. Use a dedicated, stable supply, a large local capacitor recommended by the modem-board vendor, and a common ground. Some SIM800L carrier boards include a regulator and accept 5 V; verify the label/datasheet for the exact board.

## R305 enrollment

The locker authorizes fingerprint templates with IDs 1 through 20 by default. Enroll fingers into the R305's internal memory first, for example with the Adafruit Fingerprint Sensor Library `enroll` example. Change the allowed range in `AntiTheftLocker/LockerConfig.h` if needed.
