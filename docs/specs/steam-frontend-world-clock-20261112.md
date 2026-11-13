# Steam World clock on the frontend pages (title, character selection, character creation), 2026-11-12

Status: source-side behavioural contract, code-free. Scope: the supported Steam release only, from process start until a character first enters a world, plus the return-to-title path where it touches the clock. Nothing here is implementation advice.

## 0. Authority, method and exposure

Analysis image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244 (re-hashed for this contract), image base 0x140000000. Installed retail executable C:/Program Files (x86)/Steam/steamapps/common/Cube World/cubeworld.exe was hashed only: 5,939,072 bytes, SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. No executable was run. All addresses are RVAs of the analysis image.

Evidence: the original export (whole-image instruction listing including ownerless code, call-reference table, string and string-reference tables, per-function presentations). Native operands decided wherever a presentation was ambiguous. The Infinite-Cubes reconstruction (release tree) was used only to locate candidate routines; every fact below marked as Steam was re-confirmed in the Steam listing. Facts resting only on the reconstruction are explicitly marked "lead only".

Read for this assignment: docs/CLEANROOM_AUTHOR.md; the three assigned held contracts (World clock units and update order 2026-11-08; Game update complete phase schedule 2026-11-12; World-step phase order 2026-11-07); the original evidence; reconstruction files under the release tree (leads). The coordinating harness injected the repository's top-level project instruction text and branch name into this context at session start without a tool read; it carries workflow policy only. No engine, test, exporter, log, capture, owner asset or runtime information was read. No other agent was used. No prohibited exposure occurred.

Terminology follows the phase-schedule contract: G is the Game object, W the client World at G+0x08, P the local creature at W+0x448, H the in-process Host at G+0x6E8 whose embedded World is HW at H+0x08, CL the network client at G+0xD18. "Wanted identity" means the pair G+0x206C (32-bit seed) and G+0x2070 (string). "World identity" means W+0x220 (32-bit seed) and the string beginning at W+0x200 whose length field is W+0x210.

Correction to a held contract (held file unchanged): the world-step phase-order contract describes W+0x210 as a "pointer" that may be null. Native operands at 0x2E2FB3 (clock updater) and 0x2DA935 (world step) compare the 64-bit length field of the string at W+0x200 with zero. "Null" there means "empty string". The clock-units contract's description (length field) is the correct one.

## 1. Who sets the identities, and when

### 1.1 World constructor 0x29A130

Two direct callers only: the application owner at 0x136163 (this builds the client World later handed to the Game constructor as W) and the Host constructor at 0xF17E1 (HW). The constructor initialises the string at W+0x200 as empty (length 0) and stores zero into the role byte W+0x358 (0x29A276). It does not store the seed W+0x220, the day W+0x08 or the time of day W+0x0C; those receive values only in 0x2B5F20 (section 1.2). No Game update can run before the Game constructor has completed, because the frame driver returns while the Game global is zero.

### 1.2 World initialisation 0x2B5F20

Inputs: the World, a 32-bit seed, a string. In order: the seed is stored at W+0x220 (0x2B5F76); the string is copied into W+0x200 when it is a different object (0x2B5F7D/0x2B5F93); the CRT random generator is seeded with the seed and "SRAND initialize" is written to standard output; seed-derived noise tables are filled. Then, unconditionally, the day W+0x08 is set to 1 (0x2B66F7) and the time of day W+0x0C is set to binary32 0x4C095440, which is exactly 36,000,000.0 (10:00 in the 86,400,000 day) (0x2B66FF). Then, only when the role byte W+0x358 is zero and the supplied string is non-empty (0x2B6707/0x2B6716), it opens the database "Save/world.db" (string 0x472060, literal path, not derived from the name or seed) into W+0x3B8 and calls 0x2B98B0 (section 4.1), which may replace the day and time with saved values.

Direct callers: exactly two, 0x7F279 (Game constructor 0x7EA80) and 0xF1969 (Host constructor 0xF17A0). No jump or data reference to 0x2B5F20 exists in the listing; the "SRAND initialize" string has a single reference inside it.

### 1.3 Startup values

* Host constructor 0xF17A0, reached from the Game constructor at 0x7EB36: HW is initialised with seed 0x7B (123) and the six-character string " world" (leading space; string 0x43FC30). HW's role byte is zero at that point, so HW loads the saved state (section 4.1).
* Game constructor 0x7EA80: stores the active byte G+0x2068 = 1 (0x7EF63), the wanted seed G+0x206C = 0x7B (123) (0x7EF6A), and the wanted string G+0x2070 = "world" (five characters, string 0x436074; 0x7EF74–0x7EFA9). Later at 0x7F279 it calls 0x2B5F20 on W with the wanted seed read back from G+0x206C and the wanted string G+0x2070. W's role byte is still zero (no connect has happened yet), so W also loads the saved state.

Result at the end of construction: wanted identity = (123, "world"); W identity = (123, "world"); HW identity = (123, " world"). All three strings are non-empty.

### 1.4 Later writers

* Wanted identity: the only direct writer after construction is 0x8A500. Under W's World lock it copies the given seed and string into the record reached through P+0x1528 (+0x2C8 seed, +0x2D0 string), then into G+0x206C (0x8A56C) and G+0x2070 (0x8A565), then calls 0x2D2C00 on W (removes every resident of W+0x40 except P and clears the identifier map W+0x450), then leaves the lock. It does not touch W+0x08, W+0x0C, W+0x200 or W+0x220.
  0x8A500 has exactly two references: (a) the world-creation "Create World" button callback 0x9F060 (registered at 0xFD82F), which appends a new world entry from the name and seed edit fields and then calls 0x8A500 with that entry; (b) the world-selection "Select" button callback, ownerless code at 0x9FD10 (registered at 0xFCFB0), which hides the world-selection root and, when the selected index G+0x1770 is inside the world list G+0x1790, tail-jumps to 0x8A500 at 0x9FDC6 with that entry's seed (+0x30) and name (+0x10); otherwise it shows the world-creation root. Neither callback is reachable from the title, character-selection or character-creation pages themselves.
* World identity of W (W+0x220, W+0x200): no direct writer other than 0x2B5F20 was found. The search covered every 32-bit, 64-bit and 128-bit store at +0x200..+0x220 and every address formation of +0x200 in the whole listing, ownerless code included; the remaining hits belong to unrelated objects or to the World destructor 0x29B290. Computed addresses and the undecoded byte ranges listed in the phase-schedule contract are not excluded.
* Role byte W+0x358: set to 1 by the client connect routine 0x3D820 (0x3DB2D, through CL+0xA50 = G, then G+0x08); set to 0 by 0x911E0 only while a direct socket connection (G+0x21E8 nonzero) is torn down (0x91249, 0x91309).

## 2. Question 1: the phase-25 batch on the frontend pages

The Game update steps exactly one World: the object loaded from G+0x08 (W) at 0xA7324 and 0xA7348. HW is never stepped by the Game update; it is stepped by the host worker thread (held clock contract).

Admission (0xA72CE–0xA72FF): the dword at W+0x220 is compared with G+0x206C; on equality the string comparison 0x6DD20(G+0x2070, W+0x200) is made. 0x6DD20 returns a low byte of 1 exactly when both strings have the same length and identical bytes (memcmp over the common length, then a length test); the batch requires that byte to be nonzero.

On the title, character-selection and character-creation pages before any world has been selected or created, 0x8A500 has not run, so the wanted identity (123, "world") equals W's identity (123, "world"). The batch is admitted on every frame. Consequences: the mismatch hide of phase 26 and the renderer flag reset of phase 93 do not occur on these pages.

Elapsed time per frame: the Game update's capped binary64 milliseconds (input from the frame driver's high-resolution interval, capped at 1000.0; held contracts). It is split into binary32 16.0 steps while the binary64 remainder exceeds 16.0, followed by one final step of the remainder narrowed to binary32 (0xA7328, 0xA734C). Each step reaches the clock updater 0x2E2E80 through 0x2DA6C0 unchanged. Neither the frontend pose (phase 9) nor any page visibility gates the batch.

The world-selection page (between character selection and world entry) is also admitted until its "Select" or the creation page's "Create World" runs 0x8A500. After that call the batch is admitted only if the chosen entry's seed is exactly 123 and its name exactly "world"; see section 6 L1.

## 3. Question 2: the World string tested by the clock updater

The clock updater tests the 64-bit length at W+0x210 (0x2E2FB3) and, when it is zero, assigns 32,400,000.0 (binary32 0x4BF73140, 09:00) to W+0x0C on every call (0x2E2FBD).

On the frontend pages W's string is "world" (length 5), written at 0x7F279 through 0x2B5F20 during the Game constructor, before the first frame. It is never empty after construction: no other writer of W+0x200 exists (section 1.4), and 0x8A500 writes only G's wanted string. Therefore the 09:00 override never fires for W on the title, character-selection or character-creation pages, nor at world entry or on return to the title. HW's string " world" is likewise non-empty, so the override does not fire for HW either.

Only the two World objects of section 1.1 exist in the listing. Since both receive non-empty strings during construction, the empty-string 09:00 assignment is not reached on any ordinary route. (Lead only: the reconstruction names the 09:00 value a "preview hour"; no Steam preview World object was found.)

## 4. Question 3: the time of day W+0x0C on the frontend pages

### 4.1 Initial value at game start

1. 0x2B5F20 sets day 1 and time 36,000,000.0 (10:00) for HW (from the Host constructor) and then for W (from the Game constructor).
2. Both then open "Save/world.db" and call 0x2B98B0. That routine reads the record under the key "state" (string 0x4720D0) from W+0x3B8. When the record exists it builds a reader over it (0x147750 sets the expected format version to 0x2C) and calls 0x148E90 with the World's day field as destination. 0x148E90 writes "loadWorldState 1", reads a 32-bit version, then a 32-bit day into W+0x08, then 32 bits into W+0x0C as the binary32 time of day, then writes "loadWorldState 2". Only after that does it compare the version with 0x2C. The day and time are therefore adopted even when the version differs. Each 4-byte read happens only when at least 4 bytes remain; a short record leaves the corresponding field at its earlier value (1 or 36,000,000.0). When the record is missing, both stay at day 1, 10:00.
3. The record is written only by 0x2D38E0, and only when the World's string is non-empty. Its two direct callers are inside the host generation routine 0xF2340, both for HW: a periodic save while the Host running byte H+0x550 is nonzero and at least 60,001 ms (comparison with 0xEA61) have passed since the previous save (call 0xF2D6F), and a final save when that loop ends (call 0xF2F1E, after the label at 0xF2EE6). The saved day and time are HW's (H+0x10 and H+0x14, i.e. HW+0x08 and HW+0x0C).

The title-screen starting time of day is therefore the in-process host world's day and time as last saved to "Save/world.db" in an earlier session, or day 1 at 10:00 when no such record exists. It is not a fixed backdrop hour. The record carries no world identity; it is the same key and path whichever world was last played.

### 4.2 Advancement on the frontend pages

Two mechanisms change W+0x0C while these pages are shown:

* Local stepping (Steam, confirmed). The admitted phase-25 batch (section 2) runs the clock updater on every frame. Each call adds the binary32 step to W+0x440. It then adds the step times binary32 10.0 to W+0x0C, or times binary32 100.0 when some resident of W+0x40 has byte +0x60 equal to zero, byte +0x68 equal to 0x58 and a strictly positive binary32 at +0x180. It then skips the 09:00 override (non-empty string) and wraps strictly above 86,400,000.0, incrementing the day (held clock contract). On the title and character-selection pages phase 9 sets P's ability byte +0x68 to 0x9D before the batch. On the character-creation page phase 9 instead clears an ability of 0x9D to 0. P therefore does not qualify on these pages. Whether any other resident can qualify on these pages was not traced. The ordinary rate is 10 time-of-day units per elapsed millisecond: one 86,400,000 day takes 8,640,000 ms (2.4 hours) of admitted elapsed time.
* Overwrite by the time message from the in-process host (Steam handler confirmed, delivery on these pages partly lead). The client message handler 0x3E450 (time case, 0x3ED0A–0x3ED8A) reads a 32-bit day and a 32-bit integer time. When W's day differs from the received day it first calls 0x2C34C0 on W (0x3ED64). It then stores the day into W+0x08 (0x3ED74) and the integer time converted to binary32 into W+0x0C (0x3ED8A). This runs inside the client service 0x3F4F0 (phase 71 of the Game update), so it overwrites the value the same frame's batch produced. The next frame's batch continues from the received value. On the sending side, the host update 0xF5520 reads HW's time truncated toward zero to an integer (0xF5D64) and HW's day (0xF5D6D) while building its per-connection replies, before it steps HW (the HW lock is entered at 0xF68CD). The value sent therefore lags the HW steps of that same host update.

Connection on the frontend (Steam, confirmed except where marked): the client constructor 0x3AA10 stores zero into CL+0x9EC (time of the last host contact, 0x3AB08) and stores the local Steam identity into the global 0x54DDC8 (0x3ABAC). In every client service, when the millisecond clock minus CL+0x9EC is not below 5,001 (0x1389), CL connects to that local identity through 0x3D820 (0x3F721). Because CL+0x9EC starts at zero, this happens in the first client service, during the first frame on the title. 0x3D820 records the destination at CL+0x70, refreshes CL+0x9EC and sets W+0x358 to 1. The host worker's client test then fails, because the selected peer equals the local destination, so H keeps updating on these pages. Any packet received through 0x3E450 refreshes CL+0x9EC (0x3E4E0). Lead only: the reconstruction shows the connect sending a join message, and the host answering with a per-connection record that receives the time message on every host update. That the in-process host accepts the local join and sends time messages while the frontend pages are shown was not instruction-traced in Steam (section 6 L2).

Side effect of the connect: from the first frame on, W+0x358 is 1. Inside W's clock updater the crossing-reset calls (0x2D2EB0, 0x2C34C0) and the zero record on a midnight wrap are skipped. The day increment and the period subtraction are not skipped (held clock contract). The role byte does not affect the 10 / 100 rate or the 09:00 test.

Net behaviour on the title, character-selection and character-creation pages: the time of day advances and is not frozen. It starts from the saved host state (or 10:00, day 1). It advances at the 10x rule in W. While time messages from the local host arrive, it is re-synchronised every client service to HW's integer time; HW starts from the same saved state and advances under its own 10 / 100 rule in the host thread.

### 4.3 Return to the title from a world

The start-menu routine 0x9FDF0 (reached by tail jump from ownerless code at 0x29234C) calls 0xA27B0, then 0x911E0 (tear-down of a direct socket connection only, which also zeroes W+0x358 when a socket was open), reloads every saved character slot into its preview creature (0x9BB20), shows the title root (G+0x118), clears the map/menu bytes G+0x420 and G+0x668 and hides eight panels. It writes neither W+0x08 nor W+0x0C. It calls neither 0x2B5F20 nor 0x2B98B0 (the only "state" reader), and it does not restore the wanted identity. Within the inspected routines the time of day is therefore kept, neither reset nor reloaded from disk. It goes on changing only through the two mechanisms of 4.2.

Conditional consequence (static, not observed): 0x8A500 changes the wanted identity and nothing on this route restores it. If the world last selected was not exactly (123, "world"), then after returning to the title phase 25 is not admitted, phase 26 hides the HUD-parent mirror node and phase 93 zeroes the renderer flag on each frame. W+0x0C then changes only through time messages. Whether the routine 0xA27B0 or the remote-host path reconnects to the local host was not traced (section 6 L3).

## 5. Question 4: other title-screen time sources

* No fixed hour for the title backdrop was found. The Game update only reads W+0x0C on these pages, for the night lamp of phase 10: below 18,000,000 or at least 68,400,000, binary32 constants at 0x438010/0x438020. The renderer 0xE2E00 reads the time of day from its World (for example 0xE3011, doubled and divided by the 86,400,000 constant at 0x438028). Its title-only scene flag 0x40 (test at 0xE4006) drives a separate animation from the millisecond tick modulo 10,000; it does not substitute a time of day. This renderer check is bounded to the flag-0x40 branch and the shown time read; the renderer was not audited completely.
* Saved world time: yes, through "Save/world.db" / "state" (section 4.1). That record holds HW's day and time; it is not a per-world or per-character time.
* The 10:00 constant 36,000,000.0 (0x2B66FF) applies only when no usable saved record exists. The 09:00 constant 32,400,000.0 (0x2E2FBD) does not apply on the frontend (non-empty strings).
* Network time messages (section 4.2) are the only other writers of W+0x08 and W+0x0C found in the routines examined. A complete census of writers of W+0x0C was not performed. The world-clear routines 0x2D2C00, 0x2D22E0, 0x2D2EB0, 0x2C34C0 and 0x2DA090 and the character routines 0x9BB20 and 0x9AFB0 were checked and store neither field.

## 6. Unresolved and limits

* L1. Static tension, outside the asked pages: W's world identity is set only once, to (123, "world"), yet world selection and creation change the wanted identity to the chosen entry. No later writer of W+0x200/W+0x220 was found. How in-world admission of phase 25 is reached for worlds of other identities is unresolved: it could be a computed or undecoded writer, or the batch may genuinely be skipped. This contract makes no claim about in-world stepping.
* L2. Delivery of host time messages on the frontend pages: the Steam send site (0xF5D64–0xF5D70, inside 0xF5520's per-connection reply construction) and the Steam receive site (0x3E450) are confirmed. The in-process host's acceptance of the local client's join, the per-connection send call and the message identifier were not instruction-traced. The cadence statement rests on the reconstruction lead.
* L3. 0xA27B0 (first call of the return-to-title routine) and the remote-peer leave/reconnect path through 0x3DBF0 and 0x27B0F0 were not audited for clock effects.
* L4. HW's own advancement (its residents' qualification for the 100x rate, its host-thread scheduling) follows the held clock contract and was not re-audited for the frontend.
* L5. Whether any resident of W other than P can satisfy the 100x predicate while these pages are shown (for example creatures received from the local host) was not traced.
* L6. Behaviour when "Save/world.db" is missing, unreadable or corrupt beyond "no record → defaults" (database open semantics of 0x67070, read semantics of 0x670F0) was not audited. Finite, ordinary saved values are assumed.
* L7. No runtime observation confirms any statement. Computed writes, aliases, the undecoded switch ranges of 0xA5A50 and concurrent access by other threads are not excluded.

Original routines cited: 0x29A130, 0x29AF50, 0x2B5F20, 0x2B98B0, 0x147750, 0x148E90, 0x2D38E0, 0xF2340, 0xF17A0, 0x7EA80, 0x136163 (call site in 0x1355A0), 0x8A500, 0x9F060, 0x9FD10 (ownerless), 0xF9AC0, 0x2D2C00, 0x6DD20, 0xA5A50, 0x2DA6C0, 0x2E2E80, 0x3AA10, 0x3D820, 0x3E450, 0x3F4F0, 0xF5520, 0x911E0, 0x9FDF0, 0xE2E00, 0x29B290.

The implementer must read this artifact completely before using its facts. After final full readback the author holds all writes.

FINAL / STABLE / HELD
