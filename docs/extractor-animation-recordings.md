# Animation capture boundaries and recording compatibility

The finalized native contract in [creature pose sections2–3](specs/creature-pose.md)
places animation state at Creature+0xc38. Its sprite-pointer span has twelve
eight-byte entries at animation+0xd8, immediately followed by eighteen 64-byte
matrices at animation+0x138. The matrices end at animation+0x5b8; the next fields
include a size factor and weapon trails. They are not additional part matrices.

`NativeCreaturePose.h` reads these documented spans, state and clock directly
through project-owned value types. The extractor no longer uses SDK animation
members or `Matrix4`. Caller-held creature lifetime and the applicable guard
remain required; a value read is not a universal atomic pose snapshot.

The project-owned CWA1/CWA2 recording format predates that bounded adapter. Its
wire shape reserves fourteen sprite slots and thirty-three matrices. Existing
files and playback remain compatible: new CWA2 output retains the exact 2,236-byte
positioned frame, fills the two surplus sprite slots with -1 and the fifteen
surplus matrices with zero. It does not copy adjacent native fields into those
reserved positions. The first twelve slot values and eighteen matrices retain
their native order and float bits. Unwritten/absent native part matrices still
retain native contents; the existing pose-admission checks are separate.

The native animation clock is a float. Its bits occupy the legacy recording's
four-byte timer word without integer conversion. Snapshot diagnostics also report
the clock as milliseconds. Timestamp, fixed position, flags, ability, yaw and
velocity keep their existing wire fields. `RecordingFormat.h` contains this
asset-free wire value and encoder, shared independently of GLM or the SDK.

Synthetic tests exercise every documented pointer/matrix entry, null sprites,
negative-zero clock/yaw, a retained NaN payload, caller metadata and untouched
native bytes. Nonzero neighboring memory cannot become extra recorded matrices.
Two encoded frames round-trip through the existing playback reader, including
its existing partial-next-frame handling. Older CWA1/CWA2 playback remains in the
full engine suite.

This corrects extraction boundaries and removes another SDK dependency. It does
not change the engine's pose algorithms or establish complete native animation
parity. The new DLL still requires supported-game live validation, and the full
SDK-free extractor build remains a separate release gate.
