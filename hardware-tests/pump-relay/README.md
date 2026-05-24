# Pump relay

## Purpose

- Verify pump relay module logic input on D11.
- Verify safe default state.
- Test relay switching without mains load first.

## Hardware under test

- 3.3 V logic relay module
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- Relay input -> D11
- D11 = `OUTPUT`
- Safe default = pump off

## Planned sketch behavior

- Initialize D11 to safe pump-off state.
- Allow controlled toggling.
- Print relay command state.
- Optionally test active-high or active-low behavior if not already known.

## Expected result

- Relay input responds predictably.
- Safe default state is confirmed.
- Active logic level is documented.

## Safety notes

- Do not test with 230 V mains load until relay wiring, enclosure, isolation, strain relief, and safety precautions are complete.
- Firmware can only command the relay; it cannot make mains wiring safe.

## Result notes

TODO: Add measured results after hardware test.
