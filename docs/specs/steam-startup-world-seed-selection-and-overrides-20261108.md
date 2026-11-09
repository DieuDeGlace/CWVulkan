# Supported Steam startup world-seed selection and bounded overrides

This independent, complete, code-free original-side contract audits startup configuration at Game constructor RVA 0x7EA80 and application startup 0x1355A0, the four-byte Game field +0x206C, configuration routine 0x2B5F20 and its World field +0x220. It distinguishes the two World owners, caller literals, retained state, later character and connection routes, and the constructed world-selection UI. It also follows the configuration input into a reached generation call without assigning it universal control over terrain or names.

The supported original is binary/cubeworld.original.exe.unpacked.exe beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE Machine is 0x8664, image base is 0x140000000, and timestamp is 0x5D93AB0D. The original export metadata agrees. Selected instructions for the constructors, configuration, seed-selection handlers, retained-state reader, connection/disconnect work and generation boundary were compared against the file-backed image: all 4117 comparisons matched. The two startup name literals were independently read from the image.

Only the standalone author policy and filename-selected original executable, native/decompiled routines and original metadata were inspected. No implementation, SDK/dependency source, tests/results, project startup/memory/history, owner outputs or prior held artifacts were read. No held artifact was changed. This document contains no implementation guidance.

## Fields and owners

G denotes Game. V is the separately allocated viewed World, held at G+8. H denotes the Host object embedded at G+0x6E8; its embedded World A starts at H+8, therefore G+0x6F0. These are separate World objects. The owner labels identify their construction and reached uses; they do not assign an additional meaning to every value of World byte +0x358.

The following operands are distinct:

| Operand | Width or form | Directly proved role in this audit |
| --- | --- | --- |
| G+0x206C | Four bytes | Initial caller value for V's configuration; later world-selection value and equality checks |
| G+0x2070 | String object | Initial name passed to V's configuration; later selected-name value |
| World+0x220 | Four bytes | Configuration's stored second argument; read together with World+0x200 in selected equality checks |
| World+0x200 | String object | Configuration's copied name |
| World+0x244 | Four bytes | Random-derived configuration field used in a reached generation reseed |
| CharacterData+0x2C8 | Four bytes | Saved/loaded character-associated selection value; separately assigned by the world-selection helper |
| CharacterData+0x2D0 | String object | Corresponding saved/loaded name |

The original UI supplies an explicit seed label for one selected value. That label does not establish that every field above is a noise seed, that writing any one of them updates the others, or that all generated outputs depend on it.

## Normal startup passes 123 to both configurations

Application startup 0x1355A0 allocates a 0x548-byte World and calls World constructor 0x29A130. It retains this object and passes it as the World argument to Game constructor 0x7EA80 at 0x136331. The native argument setup at 0x1362F8 through 0x13632E passes object/configuration pointers. There is no separately decoded numeric seed argument at that call.

Game stores its viewed World pointer at G+8. At 0x7EB36 it constructs H at G+0x6E8 through 0xF17A0, passing the viewed World's +0x70 value as the additional construction input. Inside 0xF17A0, World constructor 0x29A130 constructs A at H+8. The Host constructor then constructs a six-byte name, exactly one leading space followed by world, from the literal at RVA 0x43FC30. At 0xF1969 it calls 0x2B5F20 on A with a literal second argument 0x7B, decimal 123, and that name.

After the Host constructor returns, Game continues its own initialization. At 0x7EF6A it stores four bytes 0x7B to G+0x206C. It constructs G+0x2070 as the five-byte string world, without a leading space, from the literal at RVA 0x436074. At 0x7F26F it loads the current four-byte G+0x206C into the configuration argument register, and at 0x7F279 calls 0x2B5F20 on V, passing G+0x2070 as the name.

Thus the supported normal startup has two independently proved inputs:

| World owner | Configuration call site | Numeric input | Name at that call |
| --- | --- | --- | --- |
| Host's embedded World A | 0xF1969 | Literal 123 | Six bytes: a leading space, then world |
| Viewed World V | 0x7F279 | Four-byte G+0x206C, initialized to 123 earlier in the same constructor | Five bytes: world |

The Host call does not read G+0x206C. It occurs before that Game field's initialization. V's call does read the field. Neither startup choice is a random draw, clock sample or parsed numeric command-line input.

The original call-reference metadata identifies these two direct call sites to 0x2B5F20. This is a bounded static direct-call result, not a claim that no indirect, undiscovered or external caller can invoke the routine.

## Configuration uses the caller's value

At 0x2B5F71, configuration first cleans up the persistence object at World+0x3B8. At 0x2B5F76 it stores its second argument as four bytes to World+0x220. It then copies the supplied name to World+0x200 unless source and destination are the same string object. At 0x2B5F9A it passes the same 32-bit argument to the imported srand entry.

The body has no selection of 123, no seed-text parsing, and no special substitution for a zero input. The 123 values belong to the two callers described above. World constructor 0x29A130 separately constructs the owner and initializes its name empty and byte +0x358 to zero; this audit does not derive a configured seed from an assumed whole-object zero or from object allocation. The concrete seed store under discussion is in 0x2B5F20.

Configuration performs its 64-iteration table preparation and then samples the imported rand facility for scalar configuration fields. At 0x2B61D2 it makes an explicit rand call; its signed remainder by 100000 is stored as four bytes to World+0x244 at 0x2B61F6. Further samples populate the documented four-byte scalar region following World+0x220. World+0x244 is therefore distinct from the stored argument at World+0x220.

This proves that the configuration input both labels the configured World and seeds a random facility used by configuration. It does not establish a particular library sequence, a process-wide interleaving, or that configuration's random-derived fields are equal to the stored input.

## Retained World restore does not supply another seed here

Configuration subsequently initializes World-state collections and its day/time scalars. Its optional retained-state route is admitted only when byte World+0x358 is zero and the supplied name's length is nonzero. Both startup callers supply nonempty names. These gates are separate from selecting or storing the numeric argument; the argument store and random/table work precede them.

The selected route opens Save/world.db and calls 0x2B98B0. That routine requests the state record and, on success, calls reader 0x148E90 with destination World+8. The reader's directly established World destinations are the day/time words and the selected collection state rooted near the beginning of World. It does not decode or store World+0x220, copy a name into World+0x200, or call 0x2B5F20 again.

Consequently this retained-state reader does not replace the configured 123 with a serialized seed in either startup configuration. A successful restore is not equivalent to configuring a different seed. This is a statement about the selected reader and destination, not all persistence formats or arbitrary indirect effects.

## Character startup, Continue and character Select

Game's constructor can load character records through 0x9BB20 and can load the saved last-character selection. The ordinary Continue branch in 0x965E0 also reaches 0x9BB20 on the viewed local player. Character Select 0x9F7C0 has distinct existing-character and new-character work; its existing-character branch loads the selected character and stores the selected character index.

The complete-character decoder 0x147800 does load four bytes into CharacterData+0x2C8 and a following string into CharacterData+0x2D0 when those fields are present. Serializer 0x149760 writes those corresponding fields. This demonstrates persistence of those character-associated selection values.

It does not demonstrate a write to G+0x206C or either World+0x220. The inspected constructor/Continue/character-Select routes do not pass the decoded CharacterData value into 0x2B5F20 or reach the separate world-selection helper 0x8A500. The later player rebuild, placement and refresh calls are not themselves evidence of a seed override.

There are two different Select routes in this evidence. Character Select at 0x9F7C0 is not the world-record Select handler beginning at native 0x9FD10 described below. Their captions alone cannot merge their behaviors.

## Bounded command-line and options evidence

The selected application argument block obtains the Unicode command line and splits it with CommandLineToArgvW. When a first extra argument exists, it compares that argument with the literal server through equality helper 0x6DDA0. A nonmatching argument is converted and retained as a string. The inspected block contains no numeric extraction for World+0x220 or G+0x206C, and neither configuration call takes its numeric argument from that string. This does not establish the meaning of every possible later consumer of the retained string.

Before Game construction, startup calls options loader 0x13AA50 on the selected options object. That routine opens options.cfg and recognizes its explicit display, sound/music, camera, GUI, timing, language and related option keys. Its selected key/assignment list contains no seed key and no assignment to either operand under audit. This bounds the normal options.cfg route inspected here; it is not a claim about all files or every possible configuration mechanism.

## The constructed UI does present a numeric seed entry

The interface construction reached from Game uses 0xF9AC0 with its interface receiver at G+0x118. In the selected block it constructs a control identified by the literal worldSeed and labels it Seed (must be a number). This selected control is retained at G+0x210. The corresponding worldName control is retained at G+0x218. These meanings are supported by original literals and bindings, rather than inferred from offsets.

The same block constructs a Create World button and binds event 3 to handler 0x9F060, with Game as the callback owner. It also constructs a separate world-record Select button and binds event 3 to native handler 0x9FD10. These are concrete constructed bindings. This audit does not establish every route that shows their parent panels or that an ordinary character-selection click necessarily exposes this UI.

On the Create World route, 0x9F060 obtains edit text from G+0x218 and then G+0x210. It constructs a 0x38-byte record. The selected numeric edit text is supplied to a wide-character formatted stream extraction into the four-byte record field +0x30 at 0x9F3F9. The name is converted/copied into the record string at +0x10. The record is appended to the pointer collection at G+0x1790/+0x1798, persisted through 0xA2BE0, and its numeric +0x30 and name +0x10 are passed to 0x8A500 at 0x9F53D.

The record's +0x34 field is explicitly initialized to zero. Its +0x30 field is not given a literal default by the selected allocation initialization before formatted extraction. There is no visible branch on the stream's failure state between the numeric extraction and the later selection call. This contract therefore bounds the numeric result to successfully parsed, representable signed 32-bit input; malformed/out-of-range text and library failure behavior do not gain an invented seed default.

Native handler 0x9FD10 uses the index at G+0x1770 and record collection G+0x1790/+0x1798. For a valid selected record it loads four bytes at record+0x30 and the name at record+0x10, then tail-calls 0x8A500 at 0x9FDC6. The native upper-bound comparison is signed; a separate negative-index rejection is not supplied by this selected block. No supported malformed-index behavior is inferred.

These bindings prove an explicit numeric seed entry and selected-record value path in the constructed original UI. They do not prove that the supplied value reconfigures generation.

## What the later selection helper actually writes

Helper 0x8A500 takes Game, a four-byte selected value and a name string. It first enters the viewed World's critical section through 0x2ABB70. Its directly reached assignment order is:

1. Store the selected four-byte value into the viewed local player's CharacterData+0x2C8 at 0x8A532.
2. Assign the supplied name into CharacterData+0x2D0 unless the string objects alias.
3. Store the selected four-byte value to G+0x206C at 0x8A56C.
4. Assign the supplied name into G+0x2070 unless those string objects alias.
5. Call viewed-World cleanup 0x2D2C00 at 0x8A58A and leave the critical section through 0x2B9890.

The selected cleanup retains the local-player entity while removing other entity-list entries and rebuilding lookup membership. Its directly established World writes are collection/lifetime changes. The selected helper does not store to World+0x220, overwrite World+0x244 or call 0x2B5F20. It does not replace either World object, and it does not directly reconfigure A.

A later viewed-update block compares four-byte G+0x206C with V+0x220 at 0xA72E1 and compares G+0x2070 with V+0x200. These equalities admit the selected World substep path. A later block at 0xAD72E again compares the numeric values and name and reaches renderer work when they differ. Those are consumers of the distinction between selected values and configured values, not assignments synchronizing them.

The exact-displacement native search for G+0x206C establishes its constructor store and the store in 0x8A500, plus the selected reads. This is not a universal alias/bulk-write census. In the selected paths there is no proved later operation that transfers a changed G+0x206C into V+0x220 before 0x2AE0C0.

## Connection and disconnect are separate transitions

Shared connection helper 0x3D820 takes a peer value, including the local fallback peer value used by the selected wrapper/dispatcher paths. Its reached operations reset client packet/core state, clear selected client collections, clean up viewed-World entities/chunks, reset selected Host state, send the connection request and publish the selected peer/timestamp. It then stores byte 1 to the viewed World+0x358.

Socket/thread disconnect 0x911E0 separately clears its connection state and stores byte 0 to viewed World+0x358 along its admitted disconnect work. Neither selected helper decodes a numeric seed, stores G+0x206C or World+0x220, or calls configuration 0x2B5F20. The local fallback peer is not a replacement seed argument.

Some client-core transfers cover a client-relative +0x220 address. That address is not World+0x220. Receiver identity, not a shared offset spelling, establishes which field changes. Reached virtual entity destruction and all possible asynchronous callbacks are not promoted here into a proof of additional seed writes.

## Bounded generation connection and its limits

The viewed generation worker 0x84F70 produces coordinate requests and reaches 0x7D5C0. Its directly inspected single-request path calls 0x2AE0C0 at 0x7D618 with V, the first coordinate and the second coordinate. This gives a concrete viewed owner for the generation boundary. Other batch/Host generation callers exist in the original call metadata, but a universal generation-caller census and all their admissions are outside this audit.

For an already-present nonnull cached object, 0x2AE0C0 returns the existing object before reseeding or constructing another. On a miss, at 0x2AE12F through 0x2AE13D it forms a 32-bit seeding argument from the second coordinate shifted left ten bits, then adds four-byte World+0x244, then adds the first coordinate, and calls the imported srand entry. The native operations retain their low 32-bit result. It then allocates 0x58518 bytes, constructs the object through 0x2F9400 with the same World and coordinate pair, and calls generation routine 0x2FF6F0 at 0x2AE179.

This is a proved connection from configuration's random-derived World+0x244 field into a random-facility call immediately before reached generation. It is not a direct read of World+0x220 by 0x2AE0C0. The configuration argument's coupling to the random-derived field is separate from the stored identifier used in Game equality checks.

A bounded nearby terrain-query path also cautions against a universal seed claim. Query 0x358E0 reaches leaf routines 0x2B9080 and 0x2B9210. Those leaves compute their returned values/positions from the two integer coordinates and fixed constants; they do not read the supplied World receiver, World+0x220 or World+0x244, and make no random call. Their selected calculations therefore do not establish seed-dependent output.

This audit proves the concrete configuration and generation seeding arguments. It does not prove which particular generated terrain heights, names, layouts or object values change when a different configuration argument is supplied. The large generator's complete internal data flow, intervening reseeding, random backend and worker interleavings were not resolved. A specific generated-output dependence remains unresolved rather than being inferred from the field's seed label.

## Boundaries of the startup conclusion

For the supported normal static startup, both configuration calls receive 123. The later routes inspected here establish character-associated selection storage, an explicit seed-entry UI path, selected Game-field replacement and role/connection transitions. They do not establish a subsequent replacement of either configured World's +0x220 before generation.

The conclusion is limited to the named routines, concrete receivers, original metadata and supported image. It does not assert that the world-selection panels are universally reachable or unreachable, that all command-line/configuration mechanisms have been enumerated, that all indirect writers are known, or that every generated feature is controlled by this value. Selected callbacks, bulk copies through other aliases, alternate process modes, malformed persistence, foreign callers and concurrency remain outside the proved bounds. Static construction order does not guarantee a wall-clock ordering between the Host worker and later Game initialization.
