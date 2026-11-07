# Steam preview addendum: ingredient sprite admission

Status: FINAL / STABLE / HELD. Independent original-evidence clarification, 2026-11-07. The previously held tooltip-selection contract is unchanged.

## Original image and scope

The evidence is the original Steam x64 unpacked image at `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\binary\cubeworld.original.exe.unpacked.exe`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`, preferred image base 0x140000000. All addresses below are RVAs in that image. Only its native decompilation, assembly and call references were consulted for this clarification. No reconstruction or implementation material was used for this addendum.

## Established behavior

An ingredient's own missing sprite does not prevent publication of its hovered ingredient index or semantic item. After the formula result's initial sprite admission has succeeded, an ingredient geometry hit publishes those values independently of that ingredient's resource availability.

The original formula-details draw at RVA 0x26C2D0 performs the ingredient rectangle tests first. On a geometry hit it stores the ingredient index in the panel's four-byte field at offset 0x2E8 and copies that ingredient's semantic item into the panel-owned hover item at offset 0x2EC. The native index store is at RVA 0x26C6E8; the item-copy call is at RVA 0x26C707, using the item-copy routine at RVA 0x9CD0.

Only afterward does the caller request drawing of that ingredient icon through the item renderer at RVA 0xDDBC0. That later call is at RVA 0x26C7B9. There is no intervening ingredient-sprite admission test guarding the earlier index store or semantic item copy. The resource lookup inside the item renderer occurs after those publications. A missing nonempty-item sprite can suppress icon rendering there; the caller does not undo the earlier hover publication when that renderer returns normally.

The formula result's admission remains different: its initial sprite lookup at RVA 0x26C36C precedes the panel's hover reset and ingredient loop. Failure at that earlier gate prevents reaching the ingredient publication path in that draw. This addendum does not remove that result-resource prerequisite.

The later selector at RVA 0x94580 borrows the panel-owned cached item when the formula-details panel's own visibility is set and the cached item category is nonzero. It does not test the ingredient's sprite resource. Therefore a nonempty semantic ingredient can remain an admitted tooltip source even when its own icon could not be rendered. If the copied semantic category is empty, the index and item copy can still have been published by the geometry hit, but this selector rejects that empty cached item.

Later geometry hits retain the previously specified overwrite order. This clarification concerns normal resource absence and normal renderer return, not exceptions, corrupted data or a complete rendering-resource audit.

FINAL / STABLE / HELD. Read back by the source-side author; further writes are held.
