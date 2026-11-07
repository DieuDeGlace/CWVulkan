# Steam preview tooltip: selection, cached sources and comparison

Status: FINAL / STABLE / HELD. Source-side contract, 2026-11-07.

## Evidence and scope

This contract independently describes the original Steam tooltip's ordinary source selection, source storage, placement class and equipped-item comparison. It also bounds the reachability of its non-item description branches. No existing project specification or implementation material was used.

The authoritative reference is the x64 unpacked original at `C:\Users\DieuDeGlace\Documents\decompiled cube world steam\binary\cubeworld.original.exe.unpacked.exe`, SHA-256 `7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244`, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, image size 0x58A000. Addresses below are original-image RVAs. Native decompilation, assembly, call references and selected binary constants were examined. The packed original and installed Steam executable both have SHA-256 `A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B`. Alpha is outside scope.

The separately authorized Infinite-Cubes Steam reconstruction was used to locate relevant source families and propose semantic bindings. Every rule presented below as native behavior was checked against the original. Reconstruction member names are deliberately absent. Broader completeness and unproved caller bindings remain unresolved.

The main native selection routine is RVA 0xB6B00, called by the game update at RVA 0xA5A50. The tooltip constructor and draw routine are RVAs 0x28C0A0 and 0x28C290. Interface construction at RVA 0xF9AC0 binds the tooltip to the GUI's preview node and parents it directly beneath the overall interface root.

## Selection order and admission

Source selection starts by attempting to resolve the ground-item target. A target with the absent-coordinate sentinel is not resolved. A successful loaded-zone/item lookup at RVA 0x2B8FD0 supplies the ground record, its embedded item, and its quantity. Otherwise the starting item is absent and the starting quantity is one. This contract does not specify how the earlier world-targeting operation chooses that target.

When the quick-item ring's own visibility is set, the selector follows the ring branch and bypasses all panel and map-item hover sources. RVA 0x93E60 enumerates inventory tabs and their entries in ascending storage order, accepting entries whose quantity is nonzero and whose item category is 1. The selected ring index wraps through that list in both directions. A nonempty list supplies a borrowed item from the selected inventory entry. An empty list leaves the initial ground-item choice unchanged.

When the ring is not visible, the first admitted source in this order supplies the item:

1. The map overlay's stored hovered item, provided its category is nonzero. The selector itself does not require that overlay's own visibility or a new pointer hit.
2. An inventory cell, admitted by RVA 0x94600: the inventory panel's own visibility, a successful panel pointer-hit test, valid nonnegative tab and entry indices, and nonzero quantity are required. This helper does not additionally require nonzero item category. Negative quantities are not rejected merely for being negative.
3. An equipped slot, admitted by RVA 0x94260 and the selector's nonempty-category test. The inventory panel must be visible. Slot hit tests are considered in this order: left weapon, right weapon, left ring, right ring, neck, shoulder, chest, hands, feet, pet. The first hit is returned. The selector rejects a returned empty item and continues to the next source family; it does not resume searching later equipped slots.
4. A crafting-list item, admitted by RVA 0x94490: visible crafting panel, successful panel pointer hit, valid tab and entry indices, quantity at least zero, and nonzero item category. A zero quantity is admitted.
5. A vendor-list item, admitted by RVA 0x94750: visible vendor panel, successful panel pointer hit, and valid tab and entry indices. This helper does not add quantity or nonempty-category checks.
6. The shared formula/customization source at RVA 0x94580. A visible formula-details panel supplies its stored hovered item if that item's category is nonzero. If that panel is visible but its stored category is empty, the helper returns no item immediately; it does not try the customization panel. Only when formula details is not visible does it consider the visible customization panel, whose nonnegative stored hover index admits its embedded preview-item copy.
7. The final enchantment-panel helper at RVA 0x94190. It returns no item; no successful item-producing path is established there.
8. The initially resolved ground item, if every hover source failed.

This is a fixed source-family order, not a general rule that whichever panel looks foremost wins. The shared formula/customization helper is lower priority than the vendor list and higher priority than the ground fallback. Its returned item is assigned the crafting-panel placement anchor even when its actual storage belongs to the customization panel.

## Independent ground context, count and visibility

The selected item, the retained ground record and the quantity are distinct state. An admitted map or panel hover clears the tooltip's ground-record association. The ring branch does not clear it. Therefore a ring item can be selected while a previously resolved ground record remains associated with the tooltip.

The quantity is captured from the ground lookup, or defaults to one. The later source overrides do not replace it with the selected inventory stack's, vendor entry's, formula result's or ingredient's quantity. The selector passes that independently captured quantity to the tooltip. These are native state rules, not inferred desired behavior.

Every selection pass resets the skill and specialization indices to -1, stores the selected item pointer and retained ground pointer, stores that quantity, and clears the equipped-comparison pointer. Item-pointer presence initially determines local tooltip visibility; a nonnull pointer is sufficient at this stage, including one returned from a source without a category check.

Flight-target and lore-region coordinates are then copied into the tooltip independently of item selection. A valid first coordinate, meaning anything except 0x7FFFFFFF, forces local tooltip visibility for either source. The selector does not perform a second coordinate validity test or a lore-existence test at this stage. A selected flight or lore source does not itself erase the stored item or ground association.

## Draw-time source precedence

RVA 0x28C290 first clears the four rarity-star displays. It then uses this independent presentation priority: valid flight target; otherwise valid lore region; otherwise item; otherwise nonnegative skill index; otherwise nonnegative specialization index. An item, even if comparison has been selected for it, is not rendered when flight or lore has precedence.

The flight branch at RVA 0x28CC30 displays the target's distance and either flight-time information or a price/admission message. The lore branch at RVA 0x28E640 first asks the world whether that region has lore. If that test fails, the branch returns without falling back to the stored item, skill or specialization. A visible tooltip node alone does not prove that a useful lore body was produced.

The map publisher at RVA 0x2829C0 stores a copied hovered item in the overlay, rather than handing the tooltip the original marker item's address. Its draw resets the flight-target coordinates and later clears the stored map-hover item and lore-region coordinates. Its marker-hover publication is admitted while the map is open and the relevant pointer-movement state is clear. A qualifying marker can publish flight coordinates, a copied nonempty item, or lore-region coordinates when the marker allows a region preview and does not supply a nonempty item. The selector subsequently consumes those stored values. Exact marker generation, all marker-kind bindings and the complete map hit-selection algorithm are outside this contract.

## Formula-details producer: resource admission, geometry and overwrite order

The formula-details constructor at RVA 0x26BF70 initializes the stored hovered item as empty. Its draw at RVA 0x26C2D0 begins with a resource-admission lookup for the result item's sprite. A missing sprite returns before clearing the stored hovered item or its ingredient-hover index. The presence of formula data alone does not prove that the hover cache was refreshed.

After a successful resource lookup, that draw resets the ingredient-hover index to -1 and clears the cached item's category. It does not first erase all remaining item bytes. It then tests the result icon and, on a hit, copies the semantic result item into the panel-owned cached item. Thus the result icon is a tooltip source as well as the ingredient icons.

The result hit region uses pointer coordinates relative to panel content. Horizontally, the pointer must be strictly inside the content-width midpoint minus 60 and midpoint plus 60. Vertically, it must be strictly greater than 50 and strictly less than 170. All four boundary edges are excluded.

Ingredient hit tests are considered afterward in ascending ingredient-list order. The column count is the integer conversion of the displayed panel width minus 10, divided by 45. Column position advances by 70 content units; row position advances by 45. The first column begins 20 units from the content origin. The row-area origin is 130 units above the content bottom, with the first row beginning another 10 units below that origin. Content offsets are multiplied by the node scale and added to the screen content origin; each resulting ingredient rectangle's starting screen coordinates are truncated toward zero.

Each ingredient's hit rectangle extends 40 times that node scale horizontally and vertically from its starting screen point. The left and top edges are included; the right and bottom edges are excluded. On a hit, the draw stores that ingredient index and copies that ingredient's semantic item into the same cached item. The copied item does not include a new tooltip quantity.

The result test is first and does not prevent ingredient testing. A later ingredient hit overwrites a result hit or an earlier ingredient hit. If multiple ingredient rectangles admit the pointer, the highest visited ingredient index supplies the final cached item. If the resource-admitted draw finds no hit, the category remains empty and the later selector rejects that cache.

The constants 60, 50, 170, 130, 45, 10, 40 and 20 were checked in the original binary's referenced float data. The routine's result-copy and ingredient-copy calls are native evidence, not reconstruction-only expectations. Degenerate dimensions producing no usable columns are outside the established ordinary layout contract.

## Hidden or skipped draws and borrowing

Formula hover freshness is a draw-time property. A hidden/skipped draw does not execute its reset, result test or ingredient tests. A sprite-admission failure likewise leaves the earlier cache in place. A later selection pass at RVA 0x94580 borrows the embedded cache if the formula-details panel's own visibility is set and the cached category is nonzero. It does not require a new pointer hit, rerun the sprite lookup, or validate that the cached item came from the current pointer position or current formula revision.

If the formula-details panel's own visibility is clear, its cache is not admitted by that helper. This is different from hiding the whole interface root. The F4 handling at RVA 0x96E70 toggles root visibility; the source selector does not test that root visibility, and its panel helpers examine their respective local visibility and hit/cache conditions. A globally hidden interface is therefore not, by itself, a selector-level source-rejection rule. Actual scene drawing remains subject to the scene hierarchy. The tooltip is parented under that overall root.

The same separation applies to other stored hover sources. The customization draw at RVA 0x292990 resets its hover index in the applicable upgrade-list drawing path, copies a hit entry into panel-owned preview storage, and lets RVA 0x94580 borrow that storage when its visibility and index tests pass. This work does not establish that every early return elsewhere in customization drawing refreshes that cache.

Inventory, ring, vendor, equipped and ground sources are borrowed from their respective current storage. The map-hover, formula-hover and customization-hover sources are borrowed from copies owned by their source panels. Selection does not transfer ownership or create a durable tooltip snapshot. On each item draw the tooltip derives a separate temporary displayed item with the local player through RVA 0x10A720. That temporary's category is also used during comparison selection. The ordinary non-category-30 path copies the item; the generated-item path can derive different displayed equipment from the original source item. Full generated-item derivation is outside scope.

Source ownership must not be confused with unlimited pointer lifetime. These routines show no tooltip-owned lifetime extension for an inventory allocation or source object. The contract establishes what is referenced and what is copied; it does not promise validity across arbitrary source-container replacement or destruction. Exact update-versus-draw frame latency and every publisher's caller schedule remain unproved.

## Comparison eligibility and ordered replacement

RVA 0xB6B00 computes at most one equipped comparison pointer whenever the selected item pointer is nonnull. It first derives the temporary displayed item with the local player. Category matching uses that displayed item's category. Self-exclusion uses the original selected item's address, not item-value equality and not the temporary copy's address. Consequently, an equal-valued panel-owned copy is not excluded merely because it describes currently equipped gear.

The comparison candidate starts absent. The following tests are applied in order; a later eligible test replaces an earlier candidate:

1. Right weapon, provided the selected address is neither weapon slot and the displayed category equals the right-slot category. There is an extra restriction for displayed subtype 13: that subtype admits the right slot only when the equipped right item has category 3 and subtype 5, 6, 7, 8, 10, 11, 15, 16, 17, 18 or 23. For displayed subtypes other than 13, that extra restriction does not apply.
2. Left weapon, under the same two-weapon-address exclusion and matching left category, but considered for replacement only when Shift is held or no comparison candidate exists.
3. Pet, shoulder, chest, hands, feet and neck, in that order. Each test requires matching displayed category and excludes only that slot's exact address.
4. Right ring, excluding both ring addresses and requiring category equality.
5. Left ring, excluding both ring addresses and requiring category equality, with replacement admitted only when Shift is held or no candidate exists.

The right-weapon restriction above is stated numerically because those native subtype values are proved directly by the selector. No additional weapon-name enumeration is required by this contract. Shift is the key-state entry for virtual key 0x10, consistent with the state publication in RVA 0x96E70.

This routine does not choose the strongest gear, compare item values to decide eligibility, test class usability, or apply a general nonempty/equipment-category filter before these category comparisons. It does not add lamp or eye-patch comparison passes. In ordinary valid equipment the categories distinguish the slots; the literal ordered rules also cover empty or unusual category contents without silently adding restrictions.

The comparison pointer refers to live equipped storage. In the item branch of RVA 0x28C290 it is supplied to the primary item-information rendering and, when nonnull, is then shown below the selected item's information beneath the literal “Currently equipped:”. That lower item uses quantity one and a smaller text scale. The selected displayed item is the derived temporary copy; the lower comparison is the equipped record. Flight/lore precedence prevents both item sections from rendering in those branches.

## Placement category

Placement is selected after source and comparison state are published. With the menu/interface-ownership predicate at RVA 0x1014F0 active and no retained ground record, the default is beside the pointer with offsets of 30 and 30 interface units. Admitted inventory/equipment, crafting, vendor and shared formula/customization sources also nominate a panel anchor. The pointer's side of that panel's horizontal midpoint chooses the panel's left or right edge: the tooltip sits 10 units outside that edge, at pointer height with a 30-unit vertical offset. The map-hover item has no nominated panel anchor and keeps the pointer-relative placement in this case.

Otherwise the tooltip uses the lower-world-view placement: the window's horizontal midpoint and the window bottom minus tooltip view height, with offsets 210 and -23. This includes a retained ground association even while a menu is open. A ring item can therefore receive the ground-associated placement when a ground record was retained. Flight/lore precedence does not independently recompute this category. The native constants were checked; complete scene-transform and screen-clamping behavior is not specified here.

## Non-item branches: existence versus ordinary reachability

The constructor at RVA 0x28C0A0 initializes skill and specialization indices to -1 and skill level to zero. The ordinary selector at RVA 0xB6B00 resets both indices to -1 on each pass. No native ordinary caller publishing a nonnegative skill or specialization index into this tooltip after that reset was identified in this bounded investigation. The scoped reconstruction search likewise exposed resets but no positive publication; that reconstruction absence is not proof of complete native absence.

The draw branches are real: with no higher-priority flight, lore or item source, RVA 0x28C290 can call the skill renderer at RVA 0x28D320 for a nonnegative skill index, otherwise the specialization renderer at RVA 0x28DB50 for a nonnegative specialization index. The specialization branch also uses the current local-player class. These conditional rendering facts do not establish an ordinary reachable hover feature, its producer, its admission or identity rules, or a caller that makes the tooltip visible for it.

Missing evidence is an original caller that publishes those nonnegative indices after the reset, establishes their identity and visibility, and reaches drawing before another reset. No skill-button, character-panel or specialization-control binding is asserted from labels or branch existence alone. Indirect, inlined or unrecognized producers remain possible; a whole-game audit was not attempted.

FINAL / STABLE / HELD. The author has read back this bounded contract and holds further writes.
