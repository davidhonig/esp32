# AGENTS.md

## Project Scope
This repository is for firmware and supporting project files for an **ESP-H2-Zero-M** based automatic watering device with:

- **4 capacitive soil moisture sensors**
- **1 MOSFET-controlled micro water pump output**
- **Zigbee-exposed resources** for sensing, control, status, and diagnostics

The project should be treated as an embedded firmware codebase first, with hardware-aware constraints and conservative safety defaults.

## Primary Goals
When working in this repository, optimize for:

1. **Safe watering behavior** — never risk uncontrolled pump activation.
2. **Clear Zigbee modeling** — every sensor and actuator should map cleanly to Zigbee-visible state.
3. **Maintainable embedded design** — small modules, explicit interfaces, minimal hidden behavior.
4. **Field debuggability** — useful logs, fault states, and recoverable behavior.
5. **Hardware realism** — do not assume peripherals or pin capabilities without checking the board and schematic.

## Platform Assumptions
- Target board: **ESP-H2-Zero-M**
- Connectivity: **Zigbee / 802.15.4** is the primary external interface
- Firmware stack should prefer **ESP-IDF** and the **ESP Zigbee SDK** where appropriate
- The device likely operates as a **Zigbee End Device** unless the surrounding design explicitly requires a router role

If any implementation detail depends on board-level capabilities that are not yet documented in this repo, stop guessing and add a TODO or configuration point instead of hard-coding assumptions.

## Hardware Model
Assume the product contains these logical subsystems:

- `soil_sensor[0..3]`: four independent soil moisture inputs
- `pump_output`: one digital output controlling a MOSFET gate for a small water pump
- `water_control`: logic coordinating watering requests, time limits, lockouts, and interlocks
- `zigbee_interface`: endpoints/clusters/attributes/commands representing the device externally
- `health_monitor`: power, sensor validity, pump runtime, faults, and restart reason reporting

Keep hardware mapping centralized. Prefer one place for:

- GPIO assignments
- ADC or external ADC channel assignments
- inversion logic for outputs
- calibration constants
- timing limits
- feature flags

## GPIO layout
- GPIO0 fast spi functions
- GPIO1 ADC1_CH1 analog-to-digital converter
- GPIO2 ADC1_CH2 analog-to-digital converter
- GPIO3 ADC1_CH3 analog-to-digital converter
- GPIO4 ADC1_CH4 analog-to-digital converter
- GPIO10-14 PWM enabled pins

## Important Hardware Safety Rules
- Default the pump to **OFF** during boot, reset, OTA, Zigbee rejoin, and fault conditions.
- Never enable the pump until GPIO direction, idle level, and safety timers are initialized.
- Enforce a **maximum continuous pump runtime** in firmware.
- Enforce a **minimum cool-down / lockout period** between pump activations.
- Reject or clamp invalid watering commands.
- Treat missing, noisy, or out-of-range sensor readings as faults, not as “dry soil”.
- If moisture sensing is unavailable, default to **no automatic watering** unless the user explicitly overrides it.

## Soil Sensor Guidance
Because capacitive soil sensors vary widely, do not hard-code a single interpretation without calibration support.

Design for:

- raw reading capture per sensor
- filtered reading per sensor
- calibration endpoints or config for dry/wet range
- derived moisture percentage per sensor
- sensor health state per sensor

If the selected ESP-H2 board cannot directly support the intended sensor electrical interface, document the gap and introduce the correct interface layer (for example an external ADC or alternate front-end) rather than forcing a misleading abstraction.

## Zigbee Modeling Guidance
Expose the device as a small, understandable Zigbee model. Prefer explicit resources over clever compression.

Recommended logical exposure:

- **4 moisture resources** — one per plant/zone/sensor
- **1 pump control resource** — on/off, manual run request, runtime limit, busy/fault state
- **1 aggregate controller resource** — watering mode, automation enable, lockout, last watering result
- **1 diagnostics resource** — uptime, reboot reason, link status, sensor faults, pump fault, calibration status

Where the Zigbee stack requires choosing concrete endpoints/clusters/attributes:

- Keep endpoint layout simple and documented.
- Prefer standards-based clusters when they fit.
- Use manufacturer-specific attributes only when the standard model is insufficient.
- Document every non-obvious attribute and command in the repo.
- Keep attribute names aligned with firmware field names where practical.

## Firmware Architecture Expectations
Prefer code organized into small modules such as:

- `board` or `hardware`
- `soil_sensors`
- `pump_control`
- `watering_logic`
- `zigbee_device`
- `diagnostics`
- `storage` or `settings`

Keep these boundaries:

- sensor drivers should not directly manipulate Zigbee attributes
- Zigbee handlers should call application logic, not drive GPIOs directly
- watering decisions should be testable without hardware access
- calibration and persisted settings should be separated from real-time control logic

## Configuration and Persistence
Persist only operator-relevant state, such as:

- calibration data per sensor
- automation enable/disable
- watering thresholds
- max runtime / lockout settings
- Zigbee manufacturer-specific configuration if used

Do not persist transient fault states unless there is a specific recovery reason.

## Coding Preferences
- Use conservative C/C++ embedded patterns consistent with the existing codebase.
- Keep diffs small and targeted.
- Avoid introducing large frameworks unless clearly justified.
- Prefer named constants over magic numbers.
- Use explicit units in names where helpful, such as `_ms`, `_sec`, `_pct`.
- Add logs for state transitions, faults, and external commands.
- Avoid blocking delays in control paths; prefer timers, tasks, or event-driven flow.

## Validation Expectations
When making meaningful changes, validate at the smallest useful level first.

Prefer checks such as:

- build succeeds for the target board/config
- sensor read path handles disconnected and out-of-range inputs
- pump output remains off at boot and on errors
- Zigbee attributes update consistently with internal state
- manual watering respects runtime and lockout limits
- automatic watering never runs when sensor state is invalid

If hardware validation cannot be performed in-repo, clearly state what remains unverified.

## Documentation Expectations
Update documentation when adding or changing:

- pin assignments
- endpoint/cluster layout
- calibration flow
- watering decision rules
- safety limits
- manufacturing or deployment assumptions

## Avoid These Mistakes
- Do not assume analog-capable pins or peripherals without verification.
- Do not map all four sensors into one opaque blob if individual resources are needed.
- Do not allow direct Zigbee commands to bypass pump safety interlocks.
- Do not treat boot-time transient readings as valid calibration data.
- Do not silently ignore sensor faults.
- Do not implement “temporary” hard-coded GPIO mappings in multiple places.

## If the Repository Is Still Empty
If starting from scratch, begin with this order:

1. define board/pin/config skeleton
2. define moisture sensor abstraction and calibration model
3. define safe pump control module
4. define Zigbee resource model and endpoint mapping
5. connect application logic and persistence
6. add diagnostics/logging
7. validate boot safety and command paths

## Decision Rule
When choosing between convenience and safety, choose the option that makes accidental watering less likely.
