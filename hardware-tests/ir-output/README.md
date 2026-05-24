# IR output

## Purpose

- Verify IRremote output on D3.
- Confirm basic IR command sending to the terrarium lamp.
- Validate that D3 remains suitable for the IR LED driver.

## Hardware under test

- IR LED driver circuit
- Terrarium lamp receiver
- Arduino Nano 33 IoT

## Related documentation

- [`../../HARDWARE.md`](../../HARDWARE.md)
- [`../../SPEC.md`](../../SPEC.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
- [`../../libraries.txt`](../../libraries.txt)

## Wiring summary

- IR LED driver input -> D3
- D3 handled by IRremote

## Planned sketch behavior

- Send one or more known test commands.
- Keep command spacing safe.
- Print sent command names/codes.
- Confirm lamp reaction manually.

## Expected result

- Lamp reacts to known IR commands.
- D3 output works with IRremote.
- Command spacing is adequate.

## Safety notes

- Avoid rapid repeated commands that could stress the lamp controller.
- Confirm the IR LED driver circuit is wired correctly before uploading.

## Result notes

TODO: Add measured results after hardware test.
