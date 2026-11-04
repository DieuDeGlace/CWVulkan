# Steam full item names: material, attributes and dictionary composition

Independent native-only specification of **0x1628b0** and the directly reached
selection/composition helpers. Only original native evidence and independent
prose were read. No SDK, engine, exporter, test, reconstruction, owner dictionary
or capture, probe or game execution was used. Selector keys below are executable
constants, not a dump of owner dictionary definitions or generated names.

## 1. Image, entry and original-item boundary

Addresses are RVAs in the analyzed original unpacked AMD64 image
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This does not establish other executable versions or a runtime-name oracle.

At **0x1628b0**, RCX is the dictionary object, RDX the output native UTF-16 string
object, and R8 the input item. The helper returns that output object pointer.
Its first item operation is **0x1092b0 at 0x162901**, producing a semantic copy
of the full native 0xa0-byte item. Naming proceeds from that copy and does not
rewrite the caller's live record.

The normalized fields relevant here are category byte+0, signed32 subtype+4,
32-bit modifier at+8, rarity byte+0x14, wrapped/type-specific word+0x18, material
byte+0x1c, 32 four-byte attributes at+0x1d..+0x9c, and attribute-count byte+0x9d.
The fourth byte of each attribute is its unsigned material value; the preceding
three coordinate bytes are not read or decoded by the naming tally. Quantity
outside this item payload is not an argument to this helper.

The category22 subtype63/64/65/66 normalization and recursive category2 recipe
copy are already bounded in
[inventory naming §6](steam-inventory-refresh-events.md#6-narrow-original-item-naming-boundary).
They preserve copied attributes/count; they must not be replaced by an empty
attribute list or an adapted renderer-only item.

## 2. Decision order before the material/gear path

These earlier exits precede the later attribute tally:

| Normalized category/condition | Native naming route |
| --- | --- |
|2|Make the represented-item copy, clear its wrapped-type word, recursively obtain its full name, prepend literal **Formula: **.|
|29/0x1d|Obtain the subtype-dependent key through0x16c5e0, expand its **default** form through0x16be20, prepend literal **Glyph of **.|
|20/0x14|Obtain the subtype-dependent name through0x164760, prepend literal **Pet: **.|
|22 or23|Expand the base item key with an empty qualifier set and string macro **@** containing0x611d0's generated name. Its seed is modifier; family is the integer selector below.|
|Categories1,15,19, or material byte0|Expand the base item key through0x16be20 with qualifier **#default** and macro **@** mapped to the empty string.|

The category1/15/19 admission is the native category-below20 bit-mask condition;
it does not mean every category below20 bypasses material naming. The material0
test also applies to other categories that survived the earlier exits.

Base item-key lookup **0x16af30(dictionary,out,category,subtype)** searches the
dictionary's category/subtype association and supplies a node-ID string. A missing
association supplies an empty ID. This key is not itself an already expanded
English item name. Helper **0x16c750** separately resolves a node ID in the loaded
dictionary. Dictionary/fragment contents remain owner data.

## 3. Attribute tally and the absence of a dominant-spirit outcome

After the preceding exits, **0x162d42–0x162d49** initialize four 32-bit counters
to zero. If attribute count is nonzero, **0x162d64–0x162d82** visits exactly that
unsigned-byte count of consecutive four-byte records, in stored order. It reads
only each fourth/material byte. Values below 0x80 add nothing. A value at or
above 0x80 increments the counter indexed by that value minus 0x80.

For the ordinary four spirit values, the counters correspond to **0x80,0x81,
0x82,0x83**, one count per occurrence. Coordinates, repeated coordinates, cube
position, size and material below 0x80 do not weight, deduplicate or otherwise
alter these increments. There is no division, percentage, floating conversion
or strength calculation in this tally.

**The four tally values do not select a visible name on the verified ordinary
paths of this helper.** Native control/data flow was checked beyond the scan:
the counter storage is overwritten when naming containers are initialized
(including **0x162e12–0x162e2c** and the fallback paths), or the path returns
without consuming the counters. It is not supplied as a naming argument before
that replacement. The later references to the same stack area belong to the
new qualifier/map objects, not retained spirit counts.

Consequently **there is no reached dominant-spirit selection or tie-breaking
rule to implement from this routine**. Equal counts, the largest count and
attribute-count totals do not produce a native suffix or override the item's
own material here. A stream object is constructed after the scan, but no reached
insertion of these counts/spirit names into the returned string follows.

This conclusion is limited to well-formed ordinary item storage. The scan has
no clamp to count 32 and no upper material 0x83 guard. A count above the 32 embedded
records can read beyond them; higher material values can address beyond the
four counters. No meaningful/safe naming result is specified for those malformed
inputs, and no invented clamp or dominant-material recovery is supplied.

## 4. Own material byte and exact key selection

The material used for naming is the normalized item's **own+0x1c byte**, not an
attribute count or elected attribute material. Calls at **0x163011–0x163020**
and **0x164387–0x164396** explicitly **zero-extend** that byte before invoking
**0x16b8a0**. Values 128..131 are therefore positive integers, not negative
signed-byte values.

Helper0x16b8a0 supplies the following node-ID strings. Values are the original
unsigned byte values before that helper's internal decrement. Spelling and case
are part of the identifiers; in particular **Flannell** is the native spelling.

| Material value | Key | Material value | Key |
| --- | --- | --- | --- |
|1|MaterialIron|2|MaterialWood|
|4|MaterialSteel|5|MaterialObsidian|
|6|MaterialClothing|7|MaterialBone|
|10|MaterialCopper|11|MaterialGold|
|12|MaterialSilver|13|MaterialPlatinum|
|14|MaterialEmerald|15|MaterialSapphire|
|16|MaterialRuby|17|MaterialDiamond|
|18|MaterialSandstone|19|MaterialSaurian|
|20|MaterialParrot|21|MaterialMammoth|
|22|MaterialPlant|23 and 130|MaterialIce|
|24|MaterialLight|25|MaterialGlass|
|26|MaterialSilk|27|MaterialLinen|
|28|MaterialCotton|29|MaterialBronze|
|30|MaterialCalico|31|MaterialFelt|
|32|MaterialFlannell|33|MaterialMail|
|34|MaterialRivet|35|MaterialFilament|
|36|MaterialSpike|37|MaterialFiber|
|38|MaterialScale|39|MaterialWeb|
|40|MaterialShadow|41|MaterialDark|
|42|MaterialDemon|43 and 128|MaterialFire|
|44|MaterialDruid|129|MaterialUnholy|
|131|MaterialWind|All unlisted values|Material|

This is a key-selection table, not guaranteed literal visible material wording.
Material0 normally took the earlier default-name exit in0x1628b0; its listing
under the helper's general fallback is not a new full-name material branch.

## 5. Gear-family dictionary context and quality selection

After the tally, **0x162d98–0x162da6** admits normalized categories **3 through 9
inclusive, plus 26/0x1a**, to the gear-family path. It resolves the base item key
and its dictionary node. Missing node resolution falls to the later fallback
path; it does not manufacture a replacement base-item definition.

For a resolved node, each string from its tag list at node+0x28 is prefixed with
literal **#** and inserted into a fresh qualifier set, in that list's traversal
order. Literal# is at native0x44d580. The routine prepares these string macros:

| Exact macro key | String value supplied |
| --- | --- |
|@name|Generated proper-name result from0x611d0, seed **modifier**, family selector below|
|@item|Base category/subtype node-ID string from0x16af30|
|@materialitem|Own-material node-ID string from0x16b8a0|

The family selector first forms **seven times modifier with 32-bit wrap**, then
takes **unsigned remainder 11**. The adjective selector separately takes
**unsigned modifier remainder 10**. These are integer operations on the original
32 modifier bits. The full-name routine contains no scalar float/double
arithmetic for these selections or for attribute/material naming. No attribute
count, coordinates, region pair or quantity replaces either selector.

For rarity bytes 0 through 4, the selected top-level key is in the following table.
Every cell has the literal prefix **item:**; the table displays its suffix.
Row 3's trailing 2 is part of the dictionary key, not a visible numeral appended
to an item name.

| Modifier remainder10 | Rarity0 | Rarity1 | Rarity2 | Rarity3 | Rarity4 |
| --- | --- | --- | --- | --- | --- |
|0|shabby|adjusted|exceptional|exceptional2|brilliant|
|1|plain|balanced|polished|polished2|shining|
|2|battered|battletested|extraordinary|extraordinary2|magnificent|
|3|artless|good|exquisite|exquisite2|sublime|
|4|unwieldy|handmade|superb|superb2|pompous|
|5|used|fair|unique|unique2|glorious|
|6|dusty|neat|handsome|handsome2|splendid|
|7|scratched|clean|grand|grand2|famous|
|8|worn|undamaged|magic|magic2|legendary|
|9|common|flawless|decorated|decorated2|fabulous|

Expansion calls **0x16a790(dictionary,out,selectedKey,qualifierSet,stringMap,0)**.
Its final selection input is 0, not modifier, count or a spirit tally. The helper
copies the supplied context and reaches grammar expansion0x167c80. The existing
plain-string grammar machinery then flattens the expanded words.

No direct English concatenation of adjective, material and base item is performed
by this caller. The owner definitions and grammar determine which macros are
used, their visible ordering, spacing and punctuation. No dedicated attribute
count or spirit suffix is appended after this expansion. A rarity byte outside
0..4 falls through to the material fallback below rather than being clamped to
one of these rows.

## 6. Later fallback contexts

The later fallback at **0x163fec** also handles material-bearing categories
outside the gear family, unresolved gear base nodes, and unrecognized rarity.

**Category 24/0x18** prepares an empty qualifier set and macro **@name**. Its
generated-name seed is the copied 32-bit word at **item+0x18**, while the family
selector still comes from wrapped seven-times-modifier, unsigned remainder 11.
It expands the base item key through0x16a790 with final input 0. It does not
elect an attribute material or insert the generic fallback's#item qualifier.

**Other fallback categories** resolve the base item key/node again. If that node
is absent, this branch returns native literal **unknown**. With a node present,
it copies its#-prefixed tags and also inserts literal **#item**. It supplies
**@name** from modifier/family and **@item** as the base key, then expands the
**own-material key itself** as the top-level node through0x16a790, final input0.
It does not bind the gear path's@materialitem macro in this branch.

Thus the main gear path expands a quality key with a material-key macro, whereas
this generic fallback expands the material key with an item-key macro. They must
not be flattened into one assumed prefix/suffix layout. Missing dictionary
definitions inside the general expansion machinery are separate from this
explicit late **unknown** result; this note does not substitute that literal for
every missing key or empty expansion.

## 7. Visible-string and data boundaries

The three explicit leading literals are **Formula: **, **Glyph of ** and
**Pet: **, including their trailing spaces. Other wording belongs to the loaded
dictionary and native name generator. Stack string/container destruction does
not append output. Caller-specific quantity displays, colors, wrapping, tooltip
layout and renamed-item routes outside0x1628b0 remain separate contracts.

The exact input boundary of0x611d0 is established here, including family0..10;
its full family-specific fragment extraction/data tables are not newly specified
or bundled. The generic family−1 contract in
[identification naming](steam-identification-ui.md) must not be silently used as
an equivalent family0..10 generator. No owner dictionary/fragment text was read
or copied to fill this dependency.

This closes the later helper's material/attribute/quality decision boundary,
including its unused tally. It does not claim malformed-item safety, every
dictionary failure/recovery rule, complete independent name-generator data,
native runtime-captured output or a complete item/dialogue implementation.
