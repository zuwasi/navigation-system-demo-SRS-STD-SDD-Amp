# Elbit Demo-5-2-2026 - Software Design Description (SDD)

## 1. Overview
This SDD describes the architecture, components, data flow, and configuration required to implement the three fixed-wing guidance enhancements: Smart Landing Flare, Mission Speed Limiter, and Energy-Aware Loiter.

## 2. Architecture Summary
- FixedWingModeManager performs mode-specific guidance and publishes lateral/longitudinal setpoints.
- FwLateralLongitudinalControl consumes setpoints and outputs vehicle attitude setpoints.
- Support modules provide geofence checks, wind estimation, terrain data, and battery status.

## 3. Design Details

### 3.1 Feature A - Smart Landing Flare
Location: FixedWingModeManager landing logic.

Inputs:
- Wind estimate validity and magnitude.
- Terrain confidence or distance-to-ground validity.
- Current altitude, sink rate, and flare parameters.

Behavior:
- Compute wind score based on headwind magnitude if valid; otherwise zero.
- Compute terrain confidence score (1 if valid and recent, 0 if invalid or timed out).
- Combine scores into an adjustment factor, then clamp within [-0.3, +0.3].
- Apply factor to nominal flare time and altitude thresholds.
- Enforce minimum flare altitude and minimum time-to-touchdown constraints.

Outputs:
- Adjusted flare timing and altitude thresholds.
- Logged adjustment factor and flare initiation timestamp.

Configuration:
- SLF_EN (bool)
- SLF_HEADWIND_GAIN (float)
- SLF_TERRAIN_PENALTY (float)
- SLF_ADJ_LIMIT (float)
- SLF_MIN_TTD (float)

### 3.2 Feature B - Mission Speed Limiter
Location: FixedWingModeManager airspeed setpoint logic and speed zone evaluation.

Inputs:
- Current global position.
- Wind estimate (for groundspeed to airspeed mapping).
- Geofence polygon list with speed limits.

Behavior:
- Determine zone membership from position against polygon list.
- If inside zone, compute allowable airspeed for MAX_GS_LIMIT given wind.
- Clamp equivalent airspeed setpoint to computed limit.
- Bypass limiter during takeoff and landing flare modes.
- Log zone entry/exit and active limits.

Outputs:
- Clamped airspeed setpoints and event logs.

Configuration:
- MSL_EN (bool)
- MSL_MAX_GS (float)
- MSL_ZONE_* (polygon definition list)

### 3.3 Feature C - Energy-Aware Loiter
Location: FixedWingModeManager loiter logic and controller configuration handler.

Inputs:
- Battery status (remaining or voltage).
- Loiter setpoint parameters.

Behavior:
- Detect low-energy state with hysteresis to prevent chattering.
- Scale loiter airspeed by EAL_SPEED_SCALE.
- Reduce maximum lateral acceleration via configuration handler.
- Apply slew-rate limiting to smooth transitions within 2 seconds.
- Log activation and deactivation events.

Outputs:
- Reduced airspeed setpoint and lateral acceleration limit.

Configuration:
- EAL_EN (bool)
- EAL_BATT_THRESH (float)
- EAL_SPEED_SCALE (float)
- EAL_LATACC_SCALE (float)

## 4. Data Flow
- Navigator publishes position_setpoint_triplet.
- FixedWingModeManager computes mode logic and publishes fixed_wing_lateral_setpoint and fixed_wing_longitudinal_setpoint.
- Feature hooks adjust flare timing, airspeed limits, and loiter limits.
- FwLateralLongitudinalControl consumes setpoints and publishes vehicle_attitude_setpoint.

## 5. Logging and Telemetry
- Smart Landing Flare: log adjustment factor and flare initiation timestamp.
- Mission Speed Limiter: log zone entry/exit and active MAX_GS_LIMIT.
- Energy-Aware Loiter: log activation and deactivation events.
