# Source-side specification author policy

This document contains role rules only. Specification authors must not read
`CLEANROOM.md`: that combined document also contains implementation history.
These rules preserve the project's source separation; they do not certify legal
clearance or grant rights to game content or third-party code.

The source-side specification author runs in a separate agent context from the
implementer. It may read the scoped original native/decompiled game and binary
evidence, plus finalized independent prose contracts. On 2026-11-07 the user
explicitly authorized isolated Infinite-Cubes source research as an additional
reference. Treat reconstruction findings as leads: confirm Steam behavior against
original Steam evidence before presenting them as Steam facts, and distinguish
unconfirmed reconstruction observations. This permission does not authorize any
reference-source access by the implementer.

The author must not read the replacement engine, exporter, SDK headers or
implementations, compiler layout dumps, implementation logs, tests, owner captures,
probe outputs or packages. A task handoff must not include implementation history
or a fork of the implementation chat. Read this policy first, then only the
individually assigned reference sources. Deliver code-free behavioral prose only.

A directory name or FINAL label does not establish that an older document is
suitable for this role. Older prose may mix native facts with implementation or
verification material. Default to original evidence only unless a document or
excerpt has been individually checked for author-only suitability; task-specific
stricter source restrictions take precedence.

Identify reference filenames before searching contents. Restrict content searches
to individually verified original or reconstruction-owned files. Do not recursively
search a reference/reconstruction root that also contains SDKs, headers from
dependencies, or other excluded third-party material.

The author describes observable contracts in plain language: inputs, outputs,
decision and random-draw order, constants, identifiers, field offsets and widths,
ownership, protection scopes and explicit limits. Cite original routines by RVA
and identify the supported image. Do not transcribe source, pseudocode, decompiler
names or another implementation's expression or structure. Leave unproved facts
unresolved rather than filling gaps from implementation material.

The implementer reads finalized contracts and project-owned implementation only;
it does not inspect original game implementation, SDK source or reconstructions.
Verification against owner data and runtime behavior belongs to that separate
implementation/verification role and must not feed implementation details back
to the source-side author.

After writing and reading back a bounded contract, the author reports
FINAL / STABLE / HELD and stops editing it. The implementer reads the entire final
contract before using its facts. Freeze source only after author writes are held.

If prohibited material is exposed, stop and report the exposure without repeating
its contents. Do not continue under a claim that disregarding the material
restores separation. Subsequent specification work needs a separate author
context that has not received the implementation material.
