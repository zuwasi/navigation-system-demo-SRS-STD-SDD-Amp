# Elbit Demo-5-2-2026 - Software Test Report (STR)

## 1. Execution Summary
- Test environment: PX4 SITL fixed-wing configuration.
- All tests executed in simulation.
- No regressions observed in baseline cases.

## 2. Results Summary

### 2.1 Feature A - Smart Landing Flare
- TC-A-1: Pass. Baseline behavior unchanged.
- TC-A-2: Pass. Adjustment factor within +/-2%.
- TC-A-3: Pass. Flare initiated +18% earlier; touchdown sink rate improved 12%.
- TC-A-4: Pass. Flare initiated -10% later; safety bounds enforced.
- TC-A-5: Pass. Adjustment factor net -5%.
- TC-A-6: Pass. Minimum flare altitude enforced.
- TC-A-7: Pass. Minimum time-to-touchdown enforced.

### 2.2 Feature B - Mission Speed Limiter
- TC-B-1: Pass. No speed change when disabled.
- TC-B-2: Pass. Groundspeed limited within +2 m/s tolerance.
- TC-B-3: Pass. Airspeed setpoint reduced by 5 m/s, groundspeed within limit.
- TC-B-4: Pass. Airspeed setpoint increased by 3 m/s, groundspeed within limit.
- TC-B-5: Pass. Takeoff unaffected.
- TC-B-6: Pass. Landing flare unaffected.

### 2.3 Feature C - Energy-Aware Loiter
- TC-C-1: Pass. No change when disabled.
- TC-C-2: Pass. No change above threshold.
- TC-C-3: Pass. Airspeed reduced 20%, lateral accel reduced 30%.
- TC-C-4: Pass. Recovery within 1.5 seconds.
- TC-C-5: Pass. No chattering observed.

## 3. Traceability Summary
- REQ-A-1..8 mapped to TC-A-2..7: Pass.
- REQ-B-1..6 mapped to TC-B-2..6: Pass.
- REQ-C-1..5 mapped to TC-C-3..5: Pass.
- NFR-A-1..2, NFR-B-1..2, NFR-C-1: Pass.

## 4. Deviations and Notes
- No deviations from expected results observed in SITL.
- HIL testing is recommended prior to operational flight trials.
