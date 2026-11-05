# CWSDK license provenance: bounded metadata audit

Current build status (2026-11-04): the shipping ZoneExtractor target now uses
project-owned adapters and no SDK headers/include path. Its compiler command and
dependency audit is `tools/validate_extractor_no_sdk.ps1`. This removes the target's
build dependency; it does not change the historical grant findings below. The
optional legacy RegionProbe/RegionScout sources remain outside that target and
runtime package. See [current qualification](extractor-independent-build.md).

Audit date: 2026-11-03. This records license/provenance evidence, not legal
clearance or a grant of additional rights. The audit inspected license files,
Git repository/tree/commit metadata, public repository/submodule metadata and
safely isolated copyright/SPDX comment matches. No SDK implementation file,
engine, tests, exporter, reconstruction, owner resource/capture or game
execution was used to determine behavior. Nobody was contacted. Existing
source and notices were not changed.

## 1. Confirmed local notice and its identity

The following two local files are byte-identical (source path relative to the
CubeVulkan project root):

* `../SteamCubeWorldMods/vendor/cwmods/LICENSE`
* [licenses/CWSDK-MIT.txt](../licenses/CWSDK-MIT.txt)

Both contain the MIT license and the attribution **Copyright (c) 2021 Gijs**.
Their SHA-256 is
`18CFF3D577EAA9C4FB46F9B7AC177CA6D93167A785076E0C1AE263C4C82C4694`.
This proves which notice was copied. The destination filename CWSDK-MIT.txt
does not independently establish who licensed the original SDK or which
upstream SDK revision that person had authority to license.

The corresponding public
[CubeMegaMod project LICENSE](https://github.com/gijsgroenewegen/CubeMegaMod/blob/main/LICENSE)
identifies MIT with the same 2021 Gijs attribution. Its permission covers the
software supplied under that notice, subject to retaining its copyright and
permission notice in copies or substantial portions, and includes the stated
warranty/liability disclaimer. The notice does not enumerate an original
CWSDK revision, identify its separate contributors or expressly document an
SDK-specific permission chain.

## 2. Local repository revision versus original SDK revision

The enclosing local repository is SteamCubeWorldMods, whose Git origin is
`https://github.com/DieuDeGlace/SteamCubeWorldMods.git`. At inspection:

| Metadata | Recorded value |
| --- | --- |
| Enclosing repository HEAD | `9dedd95b6d7b32fafa56b15fd61c2191f58abdf7` |
| Tracked vendor/cwmods entry | Regular Git tree, mode040000, object `aa7b679b220684b4c0562d358ab0418bcf902cd3` |
| Tracked vendor/cwmods/LICENSE blob | `41af1278e1ae87b4a7b2047ff8bf4a0c798cb60a` |
| Latest path-affecting commit shown for vendor/cwmods and its LICENSE | `325a073a48399b42b3e6ef140a8cd439aba9d458`, 2026-10-09T23:01:40-05:00 |

That commit's message is a publication message for the mod sources/build
instructions. These identifiers locate the local vendored snapshot and
notice; they are not an original CWSDK upstream commit identifier.

The inspected vendor/cwmods directory has neither nested `.git` metadata nor
`.gitmodules`. Its enclosing tracked entry is a directory tree, not a
mode160000 submodule gitlink. Thus this local metadata does not preserve an
SDK remote/revision association. The original SDK revision, acquisition route
and any subsequent per-file provenance cannot be established from those
identifiers alone. No implementation comparison was performed to fabricate
such an association.

## 3. Separate upstream dependency and bounded license search

CubeMegaMod's public
[.gitmodules](https://github.com/gijsgroenewegen/CubeMegaMod/blob/main/.gitmodules)
declares a submodule named CWSDK, path CWSDK, with URL
`https://github.com/ChrisMiuchiz/CWSDK.git`. The
[original URL](https://github.com/ChrisMiuchiz/CWSDK) now redirects to
[coremaze/CWSDK](https://github.com/coremaze/CWSDK). This is affirmative
metadata identifying a separate upstream repository. It does not establish
that CubeMegaMod's root MIT notice relicenses that dependency, or that the
current local vendor tree matches a particular dependency commit.

The inspected coremaze/CWSDK root inventory shows master as its default
branch and the repository as archived. No LICENSE/COPYING/NOTICE file is
visible in that root inventory. This is only a bounded inventory observation,
not proof that every historical revision, nested path, release archive or
author permission statement lacks licensing evidence.

The local recursive LICENSE/LICENCE/COPYING/NOTICE filename inventory found
only vendor/cwmods/LICENSE, the Gijs notice above. An isolated search for
copyright/SPDX comment markers in the local C/C++ header/source extensions
`.h`, `.hpp`, `.c` and `.cpp` produced no additional matching license
metadata. Only license-comment matches were eligible for output; no
implementation was examined. This search is not a full contributor-rights
audit or an assertion about every possible file extension/comment format.

No original CWSDK-specific license text or express permission statement
establishing a reusable grant for the exact local SDK material was obtained
from this bounded metadata work. Public availability, a separate project's
license badge, a copied notice or the existence of a fork is not substituted
for that missing evidence.

## 4. Exact notice scope and unresolved boundary

Established: the local notice is an unchanged MIT/2021 Gijs notice matching
the identified CubeMegaMod project notice; its local storage and Git
provenance are recorded. CubeMegaMod also names a separate original SDK
repository in its submodule metadata.

Unresolved: the original SDK revision represented by the local files; the
original SDK authors' applicable grant or permissions; any per-file exceptions
or third-party notices; and whether the Gijs notice was supplied with authority
to cover all material now placed beneath the local vendor directory.

Accordingly this audit does **not** establish that the user's SDK-reuse
license condition has been satisfied. It also does not claim that permission
cannot exist elsewhere. A confirmed SDK-specific license/permission and
revision/content scope would be additional evidence, not a conclusion implied
by the existing copied notice. Work stops at this metadata boundary; no
implementation reuse, source modification or contact action is authorized by
this document itself.

## 5. Additional upstream history metadata, 2026-11-03

A follow-up public metadata query records coremaze/CWSDK's only returned branch,
master, at `083e67b5e301483c4a222989c046fc2041d134a8`. Repository metadata marks it
archived and returns no detected license identifier. A separate bare clone used
`blob:none` filtering with no checkout. A rename-disabled, names-only history
inventory covered50 reachable commits and found no added/modified path whose
filename begins with the common UNLICENSE/LICENSE/LICENCE/LICENSING/COPYING/NOTICE/
COPYRIGHT names, including nested paths. No SDK implementation or header content
was read. This is bounded path/commit metadata, not an exhaustive search of every
possible prose grant or a proof that permission cannot exist. It neither pins the
local vendor snapshot to this upstream revision nor resolves the permission gate.
No author was contacted and no additional SDK implementation reuse was authorized.
The public source is the [upstream repository](https://github.com/coremaze/CWSDK)
and its [repository metadata](https://api.github.com/repos/coremaze/CWSDK).

## 6. Bounded independent data-view progress

Primary sprite-table export now reads its catalog pointer range and raw dimension
fields through an independently authored guarded byte view. Its sole native
layout source is [the independent catalog contract](specs/steam-native-sprite-catalog.md).
Malformed-memory tests and live byte-identical owner metadata exports pass. This
component includes no SDK header, but other extractor code still does. It does
not resolve the existing SDK-wide grant, local revision or complete-adapter gate.

The sprite vertex-buffer view is now independently authored from
[ordinary-backend facts](specs/steam-sprite-buffer-resources.md). It reads the
wrapper's context and actual COM buffer fields without invoking its bind methods.
This further component does not include SDK headers; the rest of the extractor
still does, so the grant/revision/complete-adapter gate remains open.

The active terrain-list view and its documented lock addresses now come from
[separate native-only prose](specs/steam-terrain-buffer-resources.md). The capture
code's World-container enumeration was replaced by the later independent element
view described below. These bounded readers do not supply a complete independent
adapter or resolve the SDK grant/revision for final release.

Terrain-buffer capture now also replaces its SDK loaded-zone iterator with the
[independent element-sequence view](specs/steam-loaded-zone-container.md). The
ordered capture lease is original project code using documented native lock
addresses and Windows APIs. Other extractor paths remain SDK-dependent; this
change does not establish the missing SDK grant, revision or complete replacement.


The DLL factory's callback record is also independently authored from the complete
[loader ABI](specs/steam-mod-loader-capture-abi.md) and its separate
[lifetime observations](specs/steam-mod-loader-object-lifetime.md). Its header uses
only standard library types; the exporter no longer inherits SDK GenericMod or
uses its export macro. The rest of the exporter still includes SDK game views.
Raw-host tests and required export checks pass; this new callback integration is
not yet live-validated. No SDK-wide grant, exact revision or complete replacement
is established by this bounded change.

The stored-terrain export path now also reads primitive Zone identity/column
fields through [independent native-only prose](specs/steam-zone-terrain-storage.md)
and an original guarded view. Its enumeration/lease no longer uses SDK members.
The owner-format encoder is project code, and its fixtures pass the actual engine
consumer. Other sidecar and game views still depend on the SDK. This further
bounded change supplies neither a grant/revision nor complete replacement; its
new native integration is not yet live-validated.

## 7. Opaque snapshot comparison and README follow-up, 2026-11-04

A further metadata-only comparison hashed all 248 local vendor files and compared
path/blob identities against all 50 commits reachable in the isolated upstream
metadata clone. It did not display or interpret SDK implementation/header text.
Raw byte hashes and a separate CRLF-to-LF-only comparison were retained. No
complete file-set/byte match was found. Seven commits tie for the maximum overlap:
204 raw matching paths among 229 common paths, 25 different common paths, 19 local-only
paths and 5 upstream-only paths. The current master has 149 raw matching paths.
These counts include dependency files; they are not a claim that 204 core SDK
implementations were identified or that the most similar tree is the local origin.
A partial match does not establish an exact upstream revision or acquisition chain.

The official README at the previously recorded master revision was also checked
for licensing paragraphs, with fenced code suppressed. None matched license,
copyright, permission, public-domain or SPDX terms. An official GitHub issue
search for this repository and `license` returned zero results. These bounded
queries do not prove that no permission statement exists elsewhere. No SDK source
or header content was used for behavior, no author was contacted, and no new grant
was inferred. The grant/revision/complete-replacement gate remains open.

Sources: [the inspected upstream tree](https://github.com/coremaze/CWSDK/tree/083e67b5e301483c4a222989c046fc2041d134a8),
[its README](https://github.com/coremaze/CWSDK/blob/083e67b5e301483c4a222989c046fc2041d134a8/README.md)
and [the official issue-search endpoint](https://api.github.com/search/issues?q=repo%3Acoremaze%2FCWSDK%20license).


Proper-name fragment capture also now uses an original guarded data-only reader
and the independent literal RVA/length manifest. It includes no SDK header or
fragment text and performs no instruction scan. This bounded asset-access path
does not supply the missing SDK grant/revision or a complete replacement. Other
exporter views remain SDK-dependent; its new live integration is unqualified.
