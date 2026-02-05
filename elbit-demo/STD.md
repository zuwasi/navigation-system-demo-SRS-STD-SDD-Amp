# Elbit Demo-5-2-2026 - Software Test Description (STD)

## 1. Overview
This STD defines test design for the Elbit demo features, targeting SITL test execution with optional HIL extensions.

## 2. Test Environment
- PX4 SITL fixed-wing airframe.
- Wind simulation enabled.
- Terrain data enabled.
- Battery drain simulation enabled.

## 3. Test Cases

### 3.1 Feature A - Smart Landing Flare
TC-A-1: Baseline landing in calm conditions with feature disabled.
Expected: baseline flare timing and touchdown rate.

TC-A-2: Feature enabled, calm wind, valid terrain.
Expected: adjustment factor near 0, no significant change.

TC-A-3: Feature enabled, strong headwind, valid terrain.
Expected: flare starts earlier within +30% limit; touchdown sink rate reduced.

TC-A-4: Feature enabled, wind invalid, terrain invalid.
Expected: flare starts later within -30% limit; safety constraints prevent late flare below minimum altitude.

TC-A-5: Feature enabled, wind valid, terrain invalid.
Expected: adjustment factor penalized by terrain, net factor near 0 or negative.

TC-A-6: Minimum flare altitude enforcement.
Expected: flare not initiated below configured minimum.

TC-A-7: Minimum time-to-touchdown enforcement.
Expected: flare initiated before minimum TTD constraint is violated.

### 3.2 Feature B - Mission Speed Limiter
TC-B-1: Limiter disabled, verify mission speed unchanged.
Expected: no speed changes in or out of zones.

TC-B-2: Limiter enabled, enter speed limit zone at steady wind.
Expected: groundspeed clamped within MAX_GS_LIMIT + tolerance.

TC-B-3: Limiter enabled, tailwind present.
Expected: airspeed setpoint reduced to keep groundspeed below limit.

TC-B-4: Limiter enabled, headwind present.
Expected: airspeed setpoint may increase but groundspeed remains below limit.

TC-B-5: Limiter enabled during takeoff.
Expected: limiter not applied.

TC-B-6: Limiter enabled during landing flare.
Expected: limiter not applied.

### 3.3 Feature C - Energy-Aware Loiter
TC-C-1: Feature disabled, loiter at nominal airspeed and bank.
Expected: unchanged behavior.

TC-C-2: Feature enabled, battery above threshold.
Expected: no change.

TC-C-3: Feature enabled, battery drops below threshold.
Expected: airspeed reduced by scale factor; lateral accel reduced.

TC-C-4: Feature enabled, battery recovers above threshold.
Expected: return to nominal airspeed/bank within 2 seconds.

TC-C-5: Battery fluctuates around threshold.
Expected: hysteresis prevents rapid toggling.
