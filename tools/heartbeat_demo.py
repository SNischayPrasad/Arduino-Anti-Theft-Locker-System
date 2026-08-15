"""Deterministic PC test for the Arduino heartbeat-monitor alert rules."""

from dataclasses import dataclass
from enum import Enum


class Zone(str, Enum):
    LOW = "LOW"
    NORMAL = "NORMAL"
    HIGH = "HIGH"


@dataclass(frozen=True)
class Scenario:
    name: str
    bpm: int
    low: int
    high: int
    expected: Zone


SCENARIOS = (
    Scenario("Normal resting rate", 72, 60, 100, Zone.NORMAL),
    Scenario("High rate", 108, 60, 100, Zone.HIGH),
    Scenario("Repeated high (notification suppressed)", 110, 60, 100, Zone.HIGH),
    Scenario("Recovery", 80, 60, 100, Zone.NORMAL),
    Scenario("Low rate", 52, 60, 100, Zone.LOW),
    Scenario("Low boundary is normal", 60, 60, 100, Zone.NORMAL),
    Scenario("High boundary is normal", 100, 60, 100, Zone.NORMAL),
    Scenario("Adjusted range makes 108 normal", 108, 50, 120, Zone.NORMAL),
    Scenario("Adjusted low alert", 45, 50, 120, Zone.LOW),
    Scenario("Adjusted high alert", 125, 50, 120, Zone.HIGH),
)


def classify_bpm(bpm: int, low: int, high: int) -> Zone:
    if not 30 <= low < high <= 220:
        raise ValueError("thresholds must satisfy 30 <= low < high <= 220")
    if bpm < low:
        return Zone.LOW
    if bpm > high:
        return Zone.HIGH
    return Zone.NORMAL


def main() -> None:
    print("=== Heartbeat Monitor Virtual Test ===")
    print("Default rule: alert below 60 BPM or above 100 BPM")
    print("BPM values are simulated; no physical sensor is claimed.\n")

    active_zone: Zone | None = None
    passed = 0

    for scenario in SCENARIOS:
        actual = classify_bpm(scenario.bpm, scenario.low, scenario.high)
        result = "PASS" if actual is scenario.expected else "FAIL"
        passed += result == "PASS"
        print(
            f"[TEST] {scenario.name} | BPM={scenario.bpm} | "
            f"range={scenario.low}-{scenario.high} | status={actual.value} | {result}"
        )

        if actual is not active_zone:
            active_zone = actual
            if actual is Zone.HIGH:
                print(
                    "[BLYNK MOCK] logEvent(high_heart_rate, "
                    f"High heart rate: {scenario.bpm} BPM)"
                )
            elif actual is Zone.LOW:
                print(
                    "[BLYNK MOCK] logEvent(low_heart_rate, "
                    f"Low heart rate: {scenario.bpm} BPM)"
                )
            else:
                print("[SYSTEM] Normal zone; alerts rearmed.")
        elif actual is not Zone.NORMAL:
            print("[BLYNK MOCK] Duplicate abnormal-zone alert suppressed.")

    print(f"\n[RESULT] {passed}/{len(SCENARIOS)} scenarios passed.")
    if passed != len(SCENARIOS):
        raise SystemExit("[RESULT] Virtual heartbeat alert test FAILED.")
    print("[RESULT] Virtual heartbeat alert test SUCCESS.")


if __name__ == "__main__":
    main()
