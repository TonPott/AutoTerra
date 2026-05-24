# Hardware tests

Manual hardware validation sketches for AutoTerra live here.

These tests are separate from production firmware. They validate wiring, libraries, pin assignments, signal levels, timing behavior, and module assumptions before those assumptions are relied on by production modules.

Hardware tests normally run locally on real hardware. They can document expected compile behavior and intended Serial output, but they cannot be fully validated in CI because the relevant sensors, actuators, signal levels, and timing behavior depend on the physical build.

## Related documentation

- [`../HARDWARE.md`](../HARDWARE.md) for wiring and pin map
- [`../SPEC.md`](../SPEC.md) for functional requirements
- [`../ROADMAP.md`](../ROADMAP.md) for validation tracking
- [`../libraries.txt`](../libraries.txt) for required libraries

## Planned test order

1. `i2c-bus-scan`
2. `rtc-ds3231-at24c32`
3. `fan-pwm-tach`
4. `sht45`
5. `ntc-water-temp`
6. `liquid-sensor`
7. `tcs34725`
8. `pump-relay`
9. `ir-output`

## General rules

- Keep tests small.
- Cover one hardware topic per test.
- Print clear Serial output.
- Document wiring before uploading a test sketch.
- Never test mains wiring from a sketch without a safe physical setup.
- Production firmware must not depend on test-only code.
- Findings from tests should be copied back into `ROADMAP.md`, `HARDWARE.md`, `SPEC.md`, or `DECISIONS.md` when they change requirements or validate open assumptions.
