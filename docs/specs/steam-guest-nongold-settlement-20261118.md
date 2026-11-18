# Steam guest inventory settlement after a pickup notice

**FINAL / STABLE / HELD**

## Supported image and boundary

These observations cover the native Steam guest receiving an ordinary world-item pickup in message 7. All RVAs are relative to the original unpacked image, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. The reference packaged image and installed Steam executable share SHA-256 A1EFBBD41EEB8DF08395C4D6511194D416CB70A14054B2A2000B9F5CDBCFA65B. The observations are static. Receipts are distinguished below from the earlier interaction that requested the pickup.

## Recipient and quantity

The ninth counted section of message 7 carries pickup notices. Each 0xD0-byte record includes a 64-bit creature ID at +0x00, the source item at +0x08, an unsigned 16-bit count at +0xA8, three 64-bit positions at +0xB0, +0xB8 and +0xC0, and a 32-bit flag field at +0xC8. The receiver looks up the named creature and skips the record if it cannot find it. It resolves the item for that creature before comparing the creature with the local player. Only the local recipient reaches inventory, personal currency, collected-position tracking, or special-item presentation. A bystander with that creature in view of its world lookup can display the receipt in chat. The receive path has no separate XP award for an ordinary bag item. See RVAs 0x39AF0, 0x3AC10, 0x3F078, 0xAA620–0xAA657 and 0xAAD68–0xAADA9.

For flag mask 0x02, the local recipient first obtains the three integer block coordinates from the notice position, using floor at 65,536 position units per block. If that triple already exists in the recipient's collected-position set, processing advances directly to the next notice. The first notice records the triple before awarding the item. This also suppresses repeated local chat and special presentation for that position. A bystander takes the observer chat path before this test. The ordinary unflagged receiver takes no position-based duplicate check. See RVAs 0xAA65D–0xAA721 and 0x93870.

## Bag selection and stacking

The native insertion routine receives the **effective** item and the full unsigned count. Each tab entry holds a 32-bit quantity followed by a 0xA0-byte item. Zero quantity in the notice or item category zero makes no bag-entry change. The routine extends the tab array if required. The following table gives its automatic tab selection before the counter exceptions below. Category numbers are hexadecimal. The item routing routine is at RVA 0x5F100 and the visible tab names are selected at 0x27DC5C–0x27DC89.

| Tab | Display name | Effective item categories |
| --- | --- | --- |
| 0 | Equipment | 03–09, 0F, 19, 1A |
| 1 | Specials | 16, 18, 1D |
| 2 | Items | All other categories |
| 3 | Ingredients | 02, 0B |
| 4 | Pets | 14, 15 |
| 5 | Artifacts | 17 |

Only categories **01, 0A, 0B, 0C, 0D, 16 and 1C** can merge into a matching entry. The match requires the category byte, 32-bit fields at item +0x04, +0x08, +0x0C, +0x10 and +0x18, the bytes at +0x14 and +0x1C, and the active-entry count at +0x9D to agree. When the latter count is positive, the four bytes of each active entry beginning at +0x1D must agree, in order. This includes the item's region coordinate pair at +0x0C/+0x10. Padding and unused entries after the active count do not participate. See RVAs 0x5F0E0, 0x4DF60 and 0x35400.

The chosen tab is scanned from its first entry. The first matching stack receives the entire count. A zero-count entry with matching retained item data also qualifies. While scanning, the routine remembers the first zero-count entry. If no stack match occurs, that remembered entry takes the new item and count. If there is no empty entry, a new entry is appended. Nonmergeable items follow the empty-entry or append path even when identical equipment is already present. A single notice with a count greater than one produces one quantity-bearing entry for a nonmergeable item. Separate notices for the same nonmergeable item produce separate entries. There is no per-stack cap, splitting, inventory-full rejection or incoming negative-quantity sentinel in this receipt path. The generic insertion helper has a signed minus-one case for other callers, whereas pickup count 65,535 arrives as positive 65,535. See RVAs 0xAA7AB–0xAA7C5, 0x4E810, 0x4E94E–0x4E9DC and 0x4EA82.

For example, if the Ingredients tab contains a zero-count slot followed by four units of the same category 0B ingredient, a receipt for three adds to the latter stack, producing seven. A different item region prevents that merge. Two separate one-unit receipts for the same category 03 weapon leave two equipment entries.

## Counter and conversion exceptions

- **Gold and platinum coins.** Effective category 0C, material 0B adds the receipt count to the local player's gold at +0xAAC. Category 0C, material 0D instead credits the full count to the local player's regional platinum account. Its key comes from the player's region at +0x150/+0x154 at settlement. The item region and world-drop position do not supply that key. Other category 0C materials pass through standard tab and stack processing. See RVAs 0xAA721–0xAA784, 0x4E897–0x4E89E, 0x60EC0, 0x16B8A0, 0xAD251 and 0xAD445.
- **Mana Cube.** Effective category 0E adds the receipt count to the local player's 32-bit counter at +0xAB0. It uses no tab entry. The original item-name map identifies category 0E, subtype zero as Mana Cube. The insertion branch applies to the category regardless of subtype. Category 0D itself follows the normal Items-tab merge rule on receipt, including subtype zero, whose item-name key is PlatinumCoin. Regional platinum has the separate category/material condition above. See RVAs 0x4E8AA–0x4E8AE, 0x155804–0x155836 and 0x155C0F–0x155C33.
- **Gold Bag.** Effective category 1F credits a single seed- and rarity-derived value to local gold. The receipt count does not multiply this value. Its registered item-name key is GoldBag. See RVAs 0x109070, 0xAA786–0xAA7A9 and 0x155C53–0x155C70.
- **Source category 1E.** Other source categories pass through as copies. Category 1E creates the effective reward using its own seed and the named creature, including that creature's class on class-dependent branches. Subtype 1 yields a gold bag. Other supported outcomes are equipment categories 03–09. The selected effective item supplies the bag category, stack identity, presentation and chat name. The local recipient additionally records the drop's horizontal 16-block cell coordinate pair in a separate personal set after award. This record is distinct from the earlier flag-02 triple check. See RVAs 0x10A720, 0xAA63C–0xAA657, 0xAA7EA–0xAA840, 0x137E0 and 0x7D390.

**Correction to the earlier pickup contract.** Its statement that input makes no currency award was too broad. The stock pre-request bag check itself writes to two counters. For category 0C/material 0B it adds its supplied amount to gold, and for category 0D it increments the local +0xAB0 counter by one. The normal interaction calls this helper with one during the prompt and again on the accepted-input path. Ordinary bag items still enter inventory on receipt. The category 0D pre-request increment and the category 0E receipt increment are separate native decisions. RVAs 0x8A3DA–0x8A401, 0x8E329, 0x8E58F and 0x4E8AA–0x4E8AE establish the distinction. The frequency with which either exceptional item is available through this particular prompt in live play was not measured.

## Visible effects

After settlement the recipient sorts inventory, shrinks each tab to its nonzero-entry count, and refreshes the inventory widget. Insert location therefore need not be the final visual position. Equipment gives priority to the displayed region, suitability for the recipient and descending equipment score. Specials give priority to the displayed region, then ascending category and subtype. Tabs 2 and higher sort by category then subtype. Empty slots sort behind occupied slots. No tie order is guaranteed here. See RVAs 0xAA7CA–0xAA7E5, 0xB3ED0, 0x7B580, 0x74FB0, 0x83D90, 0x85770 and 0xA2540.

The standard chat line uses “You receive” for the local recipient and the known character's name followed by “receives” for another recipient. Effective category 0C skips this chat output. Positive counts are shown explicitly for categories 01, 0A, 0B, 0D, 16 and 1C. A normal nonstackable item displays the item name without a count prefix. Other people's receipts do not change the observer's tabs or counters. See RVAs 0xAAD94–0xAADA9 and 0x9D6F0.

An additional local item message is triggered by source category 1E or effective categories 17, 18 and 1D. The message uses a definite item form for categories 17 and 18, and for equipment categories 03–09 or 1A with rarity above two when the message is otherwise triggered. The remaining cases take the indefinite form. Category 18 adds its item's land and a subtype-specific description where defined, including hang glider, boat, reins and climbing spikes. The local player can also enter a presentation action when their current action permits it. Native action 0x78 is selected for category 18 subtype 1, 0x79 for subtype 2 when player flag +0x5C mask 0x02 is set, and 0x7C for other qualifying items. The normal item line and this special message have independent eligibility. See RVAs 0xAA840–0xAA8FB, 0xAAC9A–0xAAD54 and 0x9AE90.

This contract covers receipt-side ownership, tab changes, quantity, exceptions and presentation. It does not establish transport replay behavior, save or reconnect timing, live frame counts, a total ordering for equal sort keys, or every procedural equipment roll for a category 1E source.
