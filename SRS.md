# Elbit Demo-5-2-2026 - Software Requirements Specification (SRS)

## 1. Purpose
This SRS defines the functional and non-functional requirements for the Elbit demo features derived from the commercialization process demo. The scope is limited to fixed-wing navigation and control enhancements in PX4-style architecture.

## 2. Scope
Features included:
- Feature A: Smart Landing Flare
- Feature B: Mission Speed Limiter
- Feature C: Energy-Aware Loiter

## 3. System Context
- Navigation setpoints originate in navigator and are consumed by fixed-wing mode manager and fixed-wing lateral/longitudinal control modules.
- Safety-critical behavior remains unchanged unless explicitly stated.

## 4. Requirements

### 4.1 Feature A - Smart Landing Flare
REQ-A-1: The system shall compute a flare adjustment factor derived from wind estimate validity and terrain confidence.
REQ-A-2: The flare adjustment factor shall increase flare time (start earlier) when wind estimate is valid and indicates strong headwind.
REQ-A-3: The flare adjustment factor shall delay flare time (start later) when terrain confidence is low and wind estimate is invalid.
REQ-A-4: The flare adjustment factor shall be bounded between -30% and +30% of nominal flare time.
REQ-A-5: The system shall not initiate flare below the configured minimum flare altitude.
REQ-A-6: The system shall not initiate flare later than the time-to-touchdown threshold derived from sink rate and altitude.
REQ-A-7: The system shall log the computed adjustment factor and flare initiation timestamp for post-flight analysis.
REQ-A-8: The system shall provide a parameter to enable or disable Smart Landing Flare without requiring firmware rebuild.

Non-functional:
NFR-A-1: Added computation shall not increase fixed-wing control loop execution time by more than 5%.
NFR-A-2: Feature shall be disabled by default.

### 4.2 Feature B - Mission Speed Limiter
REQ-B-1: The system shall allow defining geofenced speed limit zones using a polygon list.
REQ-B-2: When the vehicle position is inside a speed limit zone, the system shall constrain commanded groundspeed to MAX_GS_LIMIT.
REQ-B-3: The system shall apply the speed limit in mission auto mode and loiter modes.
REQ-B-4: The system shall not apply the speed limit during takeoff or landing flare phases.
REQ-B-5: The system shall log entry/exit of speed limit zones and active limit value.
REQ-B-6: The system shall provide parameters to enable/disable the limiter and set MAX_GS_LIMIT per zone group.

Non-functional:
NFR-B-1: Speed limit evaluation shall not exceed 2 ms per control cycle.
NFR-B-2: Zone transitions shall be detected within 1 second.

### 4.3 Feature C - Energy-Aware Loiter
REQ-C-1: The system shall detect a low-energy state using battery remaining or voltage threshold.
REQ-C-2: When low-energy state is active, the system shall reduce loiter airspeed by ENERGY_LOITER_SPEED_SCALE.
REQ-C-3: When low-energy state is active, the system shall reduce maximum lateral acceleration (bank angle limit).
REQ-C-4: The system shall revert to nominal loiter behavior when energy state returns to normal.
REQ-C-5: The system shall log energy-aware loiter activation and deactivation events.

Non-functional:
NFR-C-1: Transition between normal and energy-aware loiter shall be smoothed within 2 seconds.
