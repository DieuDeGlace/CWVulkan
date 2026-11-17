# Steam stock graphical entry routes

FINAL / STABLE / HELD — isolated original-image evidence, 2026-11-17.

The stock graphical client has no resume launch argument in the traced startup
path. No keyboard-only title or selection activation was established. The
ordinary title menu does offer Continue when the remembered character slot is
valid. An XInput controller can activate it through the same pointer controls
as the mouse.

Evidence: stock cubeworld.exe SHA-256
a1efbbd41eeb8df08395c4d6511194d416cb70a14054b2a2000b9f5cdbcfa65b.
The matching unpacked image and export report agree on SHA-256
7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244.
Addresses below are RVAs from the unpacked image, base 0x140000000.

The stock PE entry is in the packaging section, at RVA 0x58a310. The unpacked
game entry at 0x3939f0 reaches startup at 0x393884, which calls the graphical
entry 0x1355a0 at 0x393996. The CRT supplies a narrow command line, but that
parameter is unused by the graphical entry. The game reads the Windows wide
command line and splits it at 0x135fb2 and 0x135fc0 after successful Steam
initialization, main-window and graphics creation, and DirectInput creation.

If splitting succeeds with at least two tokens, only the first operand is
examined. The comparison at 0x135ff3, via 0x06dda0 and 0x089470, requires the
whole operand to be exactly lowercase `server`. That match skips the narrow
string conversion. A nonmatch is converted at 0x136029 and retained in a local
string whose only subsequent use is teardown. Both cases converge at 0x136055
and proceed to the same graphical game construction at 0x136331. No operand
also goes to that construction. There is no server-mode dispatch on the
`server` match in this client, and the parsed value does not select a
character, world, or startup screen. Later operands are not read here.

The graphical construction at 0x07ea80 opens `Save/characters.db`; its `slot`
record supplies the remembered character index at game offset 0x2264. Local
world setup at 0x2b5f20 includes opening `Save/world.db`. The menu at 0x291210
offers Continue only when the remembered index is nonnegative and below the
loaded character count. A primary release at 0x0965e0 requires the title and
its menu group to be visible and a valid pointer choice. The Continue choice
reloads that slot into the active player via 0x09bb20, closes the title, and
refreshes the active world/character state without opening a world chooser.
The frame update at 0x0a5a50 applies the preview marker while the title or
character chooser is visible and clears that marker once both are closed.

Start Game opens the ordinary character chooser. Its Select release, wired at
0x0fcbbb, reaches 0x09f7c0. A valid existing row loads and records that slot and
closes the chooser; an invalid character selection opens character creation.
A separately constructed world Select widget is wired at 0x0fcfc7 to 0x09fd10.
If that chooser is open and an existing world row is valid, its release applies
the row's world seed and name through 0x08a500. Its presence does not establish
a second compulsory screen on the ordinary Continue route.

The main window at 0x133c70 routes key down, key up and translated characters
to GUI focus handling (0x340df0, 0x340e60, 0x340dd0) and separate game input
handlers. The title widget's three focused keyboard methods point to the
no-op at 0x0e8a20 (table at 0x470630). The Return handling at 0x096e70 is a
separately gated text/chat action. The inspected title and selection callbacks
provide no Enter, Space, Tab, or arrow-key path that invokes their primary
release actions.

The foreground client polls XInput index 0 at 0x136881. With successful
polling, no binding capture and the menu pointer branch active (cursor
requested by the UI and game byte 0x668 clear), the left stick moves or snaps
the pointer when either axis magnitude exceeds 0x3d52 (0x136ebb through
0x1372a8). A-button edges, mask 0x1000, dispatch primary press 0x094900 and
release 0x0965e0 from 0x1372d6 and 0x137334 after more than 200 ms in that
branch. Title choices and the selection buttons still require the ordinary
pointer hit and visibility conditions; A is not a menu bypass or default
selection key.

This is static evidence for the ordinary graphical startup and input paths in
the named image. It does not cover launcher or service behavior outside the
client, external input remapping, other builds, or runtime verification on a
particular save or device. The executable was neither launched nor changed.
