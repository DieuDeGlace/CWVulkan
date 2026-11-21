# Original Steam identity of “Forest Mound”

FINAL / STABLE / HELD. The checked English Steam sources establish neither a landmark class named “Forest Mound” nor a naming template that produces that exact phrase. The phrase alone does not identify a terrain profile or a reproducible location.

## What the naming evidence distinguishes

The English place template for an ordinary Forest puts the individual name before the word “Forest.” The Hill template similarly ends in “Hill.” These are distinct native place categories. The place-name selector at RVA 0x16B390 maps decimal type 25 to Forest, 26 to Dark Forest, 27 to Enchanted Forest and 31 to Hill. A Forest landmark's ordinary English title therefore ends with “Forest.” Calling a wooded rise a mound supplies neither its place type nor its generated proper name. The dictionary has separate Hill, Hills, Barrow and Molehill entries. None is an English alias named “Mound.” A separate native selector at RVA 0x16B210 also distinguishes broad terrain names such as Hills and Woodlands from these individual place categories.

For named places, the native formatter at RVA 0x166AB0 uses two coordinates and the place type to obtain a proper name, then applies the selected localization template. Names do not serve as unique coordinate keys. For example, on the ordinary Forest and Hill naming paths, increasing the first naming coordinate by 114 and decreasing the second by two preserves both proper-name inputs for a fixed type, away from the special origin. This illustrates a naming collision; it does not claim that a landmark is generated at either position. Evidence is at RVAs 0x166AB0, 0x2BA7C0 and 0x2BA810.

To identify a reported site, the useful discriminators are the exact on-screen English map title and its marked position or region coordinates. To identify a shape described informally as a “forest mound,” an identifiable map position plus a view of the rise can distinguish the local landform from a named point of interest. The inspected evidence supplies no coordinates, seed, elevation, footprint or vegetation profile for that phrase.

## Coverage and limits

The original English dictionary, dict_en.xml, has SHA-256 3629A31565DD1F92B2199D22BF3B5CEDC741FD0CA83538BAE79B6AECEB4D06DB. A case-insensitive search of that dictionary and its English localization export found no occurrence of “mound,” including the full phrase. The original unpacked 64-bit Steam image has SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Its exported string inventory and its raw single-byte and UTF-16 text were checked for both terms. The original native place and terrain naming routines above were traced against that image. Neither the words in an arbitrary generated proper name nor the contents of a particular player's world can be exhaustively ruled out by a literal-string search. No game session, save, capture, third-party map or alternate build was used.
