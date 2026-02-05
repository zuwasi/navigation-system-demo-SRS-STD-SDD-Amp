# PX4 Original Code

This folder contains reference code from the PX4 autopilot project that serves as the baseline for the navigation system enhancements.

## Source

- PX4 Autopilot: https://github.com/PX4/PX4-Autopilot
- License: BSD-3-Clause

## Relevant Modules

- `src/modules/navigator/` - Navigation setpoint generation
- `src/modules/fw_pos_control/` - Fixed-wing position control
- `src/modules/fw_att_control/` - Fixed-wing attitude control
- `src/modules/landing_target_estimator/` - Landing target estimation

## Purpose

This code provides context for understanding the modifications made in the `embedded code/` directory for:
- Smart Landing Flare (Feature A)
- Mission Speed Limiter (Feature B)
- Energy-Aware Loiter (Feature C)
