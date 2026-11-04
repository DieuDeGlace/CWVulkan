# Native specification author policy

This document contains role rules only. Specification authors must not read
`CLEANROOM.md`: that combined document also contains implementation history.
These rules preserve the project's source separation; they do not certify legal
clearance or grant rights to game content or third-party code.

The specification author reads only the scoped original native/decompiled game
and binary evidence, plus finalized independent prose contracts. It must not read
the replacement engine, exporter, SDK headers/implementations, compiler layout
dumps, third-party reconstructions, implementation logs, tests, owner captures,
probe outputs or packages. A task handoff must not include implementation history.

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
to the native-only author.

After writing and reading back a bounded contract, the author reports
FINAL / STABLE / HELD and stops editing it. The implementer reads the entire final
contract before using its facts. Freeze source only after author writes are held.

If prohibited material is exposed, stop and report the exposure without repeating
its contents. Do not continue under a claim that disregarding the material
restores separation. Subsequent specification work needs a separate author
context that has not received the implementation material.
