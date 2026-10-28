# Source publication review

CubeVulkan is an unofficial replacement-engine project. Its intended users supply a legitimate Steam installation and their own local exports. The engine verifies the active Steam account's online connection and Cube World license. The original game, its content and Steam runtime remain the property of their respective rightsholders.

The source packager excludes game executables, DLLs, sprites, music, sounds, extracted terrain, object records, character recordings, saves and reference screenshots. Do not attach those files to a repository or release. The extractor runs locally against the user's game; the package includes extractor source, not extracted content or a dumped executable. CWSDK headers and CubeModLoader are separate dependencies and are not bundled here.

These technical boundaries do not establish legal clearance. The [Steam Subscriber Agreement, section 2.G](https://store.steampowered.com/subscriber_agreement/) restricts reverse engineering, modification and distribution, subject to its stated permission and applicable-law exceptions. The applicability of any exception, game-specific terms, interoperability work and the planned distribution method needs review for the relevant jurisdiction before a public release. No Cube World rightsholder permission has been established in this project.

The [OpenMW FAQ](https://openmw.org/faq/) describes both its requirement for user-supplied game content and permission it obtained from Bethesda. That permission does not apply to CubeVulkan. An analogous architecture is not an equivalent permission grant.

The project's original engine and extractor source is licensed under GNU GPL version 3 only (`GPL-3.0-only`), selected by the project owner on 2026-10-28. The full text is in `LICENSE`; third-party components and art retain their existing licenses and notices. This source license does not cover Cube World's content or the Steam runtime.

Before publication, review the provenance of engine and extractor source and obtain appropriate legal advice or rightsholder permission for the intended use. A project license cannot license the game's content or grant rights that the contributor does not hold.

`tools/package_source.ps1` produces a separate source-review tree, an archive and a hash manifest. Inspect the staged tree before uploading its contents; do not upload the development directory. The only bundled bitmap is the separately MIT-licensed OverworldEvents quest art, with its notice. Packaging checks do not establish copyright ownership, contractual permission or trademark rights.
