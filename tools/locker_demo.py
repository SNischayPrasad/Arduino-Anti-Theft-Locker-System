"""Hardware-free demonstration of the locker event flow.

This is not a replacement for testing the Arduino with the real modules. It makes
the expected LCD, relay, and GSM behavior easy to verify on a development PC.
"""

from __future__ import annotations

import time


def lcd(line1: str, line2: str) -> None:
    print("+----------------+")
    print(f"|{line1[:16]:^16}|")
    print(f"|{line2[:16]:^16}|")
    print("+----------------+")


def run_demo() -> None:
    print("ANTI-THEFT LOCKER SYSTEM - HARDWARE-FREE LOGIC DEMO")
    print("Board target: Arduino UNO | Alert transport: SIM800L SMS\n")

    lcd("Locker secured", "Scan card/finger")
    print("RELAY: OFF (solenoid locked)\n")

    print("EVENT 1: Authorized RFID UID DE:AD:BE:EF")
    lcd("Access granted", "Locker unlocked")
    print("RELAY: ON  -> solenoid unlocked for configured interval")
    print("RELAY: OFF -> locker secured again\n")

    print("EVENT 2: Unknown RFID UID 12:34:56:78")
    lcd("Access denied", "Unknown RFID")
    print("RELAY: OFF (remains locked)")
    print("GSM SMS: SECURITY ALERT - unauthorized RFID access")
    print("ALERT DESTINATION: configured phone number\n")

    print("EVENT 3: Enrolled fingerprint template ID 3")
    lcd("Access granted", "Locker unlocked")
    print("RELAY: ON  -> access granted\n")

    print("EVENT 4: Fingerprint has no enrolled match")
    lcd("Access denied", "Unknown finger")
    print("RELAY: OFF (remains locked)")
    print("GSM SMS: SECURITY ALERT - unauthorized fingerprint access\n")

    lcd("Locker secured", "Scan card/finger")
    print("DEMO COMPLETE: all expected branches executed.")


if __name__ == "__main__":
    run_demo()
