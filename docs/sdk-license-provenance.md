# CWSDK license provenance: bounded metadata audit

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
