# Steam dictionary grammar: alternative selection, conditions and nested macros

## Image identity

- File: `binary\cubeworld.original.exe.unpacked.exe` (original Steam Cube World, unpacked x64 image).
- SHA-256 (recomputed this session): 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244.
- Preferred base 0x140000000. All addresses below are RVAs.
- Section mapping used for raw reads (from the PE section table): `.text` RVA 0x1000 -> file 0x400;
  `.rdata` RVA 0x42C000 -> file 0x42A800 (file offset = RVA - 0x1800); `.data` RVA 0x530000 -> file 0x52DC00.
- Constants read from `.rdata` (UTF-16, NUL-terminated): 0x44E540 "#default"; 0x44E538 "*";
  0x44D580 "#"; 0x4311B0 "@"; 0x437DDC "!"; 0x430DE4 "="; 0x4378E8 "%"; 0x44D0D0 "%%";
  0x436550 ": " (colon, space); 0x42FF38 the empty string.

## Routines involved

- 0x16D530: template parser (text -> node tree). The only code reference to the "#default"
  literal at 0x44E540 in the whole image is inside this routine (instruction at 0x16DFA1).
- 0x16A790: expansion entry. Copies the caller's qualifier set and string-macro map into a
  fresh expansion context (plus two further empty maps), passes the empty string as the
  category, and calls 0x167C80 with the final input argument unchanged. It does NOT add any
  qualifier of its own (no "#default" insertion on this path). PROVED.
- 0x167C80: key lookup and variant draw, then calls 0x166CB0 on the chosen root node.
- 0x166CB0: recursive node expansion (condition test, text emission, group handling).
- 0x1506A0: ordered-set lower-bound search (used for condition membership tests).
- 0x16C1F0: append to a pointer list (used to collect unconditioned alternatives).
- 0x16CE30: node constructor (all sets/maps empty, group kind 0, no text).
- 0x16BE20: wrapper building a one-element qualifier set and a one-entry macro map.
- 0x1628B0: item full-name builder (calls 0x16A790 and 0x16BE20).

## Node model produced by the parser (0x16D530)

Each node carries: a condition set (tags beginning with "#"), a style-tag set (tags beginning
with "$"), its own text, a group kind (0 = ordinary sequence, 1 = punctuation leaf,
2 = alternatives "{...|...}", 3 = optional "[...]"), an ordered child list, and for macro calls
an argument-qualifier set, a "star" flag and a named-argument map.

Parsing facts relevant to selection (all PROVED from 0x16D530):

- Tokens are split at space, newline, tab, end of text, the punctuation characters
  ! ' ( ) , - . / : ; = ? and at the structural characters { } [ ] |. Each of the listed
  punctuation characters becomes its own one-character leaf node (kind 1).
- "{" appends a kind-2 node to the current container and immediately opens a first
  alternative (a fresh kind-0 node) inside it; both become current (stacked).
- "|" closes the current alternative and opens a new kind-0 alternative as the next child of
  the enclosing node. "}" closes the alternative and the brace node. "[" opens a kind-3 node;
  "]" closes it. Unbalanced closers make the parse fail.
- A token whose first character is "#" is not text: it is added, verbatim including the "#",
  to the condition set of the node that is current at that point (for a token inside braces,
  the alternative it appears in, regardless of where inside that alternative it is written).
  "#default" receives no special treatment here; it is an ordinary condition tag.
- A token whose first character is "$" is added to the current node's style-tag set.
- Any other token becomes a text child (kind 0) of the current node.
- A text token beginning with "@" that is immediately followed by "(" is a macro call. The
  argument list up to ")" is split at "," and space. For each non-empty argument:
  - if it contains "=", the part before is prefixed with "@" and the part after is prefixed
    with "@", and the pair is stored in the node's named-argument map (left -> right);
  - else if it is exactly "*", the node's star flag is set;
  - else it is prefixed with "#" and added to the node's argument-qualifier set.
  After the list, if the argument-qualifier set is still empty, "#default" (0x44E540) is
  inserted into it. This is the only place the image inserts "#default" during parsing.

## a) Condition test — PROVED

- Done at the start of 0x166CB0 for every node, before anything is emitted.
- A node with an empty condition set is unconditioned: it always proceeds.
- A node with conditions proceeds if AT LEAST ONE of its condition tags is present in the
  current qualifier set (logical OR). Each tag is looked up with an exact, case-sensitive
  UTF-16 ordinal comparison of the whole string, including the "#" (0x1506A0 lower bound,
  then an equality check).
- If none of its tags is present, the node emits nothing (none of its text, style effects or
  children) and reports "not matched" to its parent.
- There is no negation operator and no all-must-hold mode in the matcher.
- "#default" is not treated specially by the matcher; it matches only if the literal string
  "#default" is a member of the current qualifier set. It is never implicitly present.
  It enters a qualifier set only:
  (1) when a caller supplies it (0x16BE20 builds the set {"#" + category word}; the 0x1628B0
      callers of 0x16BE20 pass the word "default", giving {"#default"});
  (2) when a nested macro call has no plain (non-"*", non-"=") argument (parser rule above).
- The 0x16A790 calls from the gear branch of 0x1628B0 pass a set made only of the base item
  node's own tags each prefixed with "#" (one branch additionally adds "#item"); no
  "#default" is added by 0x1628B0, 0x16A790 or 0x167C80 on that path. PROVED.
- Consequence for the behavioral observation: at the level where the noun entry
  "{ #default Shield | #plural Shields }" is expanded, "#default" must be in the qualifier set
  for "Shield" to appear. The proved mechanism that supplies it on the gear path is a nested
  macro call with no plain argument (for example a call whose only argument is "*", which
  yields {"#default"} plus the caller's whole set). Which template form actually leads to the
  noun entry was not inspected (owner dictionary not read): UNPROVED.

## b) Selection inside a brace group (kind 2) — PROVED

Children of the brace node are visited in written order:

1. An unconditioned alternative is not expanded during this pass; it is appended to a
   candidate list.
2. A conditioned alternative is expanded immediately (it runs the test in a)). If it fails,
   it emits nothing and the pass continues. If it passes, its text is emitted and the pass
   stops at once: later alternatives are not visited and no random draw takes place.
   Therefore the FIRST passing conditioned alternative in written order wins, even if
   unconditioned alternatives were written before it.
3. If the pass ends without a passing conditioned alternative and the candidate list is
   non-empty, exactly one candidate is expanded: index = H(s) mod (number of candidates),
   where s is the node's current seed (unsigned 32-bit) and H is the 32-bit xorshift
   s ^= s<<13; s ^= s>>17; s ^= s<<5 (result as unsigned, then unsigned modulo).
   The chosen child is expanded with the same seed s (the draw does not advance it).
4. If no conditioned alternative passed and there is no unconditioned alternative, the brace
   group contributes nothing (empty text). There is no fallback to the first alternative.

The seed: 0x16A790's final input reaches 0x167C80. If the looked-up dictionary entry owns a
non-empty list of variants, 0x167C80 picks one with the same H(seed) mod count rule and passes
seed + 1 into 0x166CB0; otherwise it passes the seed unchanged. Seeds are passed by value: all
children of one node receive the same seed. All 0x16A790 calls observed in 0x1628B0 and the
call in 0x16BE20 pass 0 as the final input. H(0) = 0, so with seed 0 the first unconditioned
alternative (written order) is always chosen.

Applied to the observation: "{ #plural Swords | Sword }" with no "#plural" present ->
"Swords" fails, "Sword" is the only candidate -> "Sword". "{ #default Shield | #plural Shields }"
-> "Shield" if "#default" is present, "Shields" if only "#plural" is present, and the first of
the two in written order if both are present; nothing if neither is present.

## c) Propagation into nested macro calls — PROVED (except where stated)

- A text node beginning with "@" without an argument list: its text is looked up in the
  current string-macro map; if found, the mapped string is emitted as literal text; if not
  found, nothing.
- A text node beginning with "@" with an argument list (argument-qualifier set non-empty,
  which the parser guarantees for any "@name(...)" form):
  1. The node text (for example "@materialitem") is looked up in the current macro map; if
     absent, nothing is emitted.
  2. The mapped string is used as a dictionary key; if the key is absent or has no root,
     nothing is emitted.
  3. A new macro map is made as a full copy of the current one. If the current map contains
     the key "!" followed by the macro name (for example "!@materialitem"), its value is stored
     in the new map under "@". For each named argument left=right, if the current map contains
     "@right", the new map's "@left" is set to that value.
  4. A new qualifier set is made from the node's argument-qualifier set (each plain argument
     prefixed with "#", or {"#default"} if there were none). If the star flag is set, every
     member of the CURRENT qualifier set is added to it. Without "*", the caller's qualifiers
     do NOT propagate.
  5. The dictionary entry's base root is expanded with the new set and map, the same seed,
     the current style, the current shake flag and an empty category. This path does not
     perform 0x167C80's variant draw; whether that base root differs from the variants when
     an entry has several is UNPROVED.
  - Example: "@materialitem(*,item)" -> nested set = {"#item"} united with the caller's whole
    set; "#default" is NOT added (a plain argument exists). "@x(*)" -> {"#default"} plus the
    caller's set. "@x()" -> {"#default"} only.
- A text node beginning with "*" names a sub-tree (looked up without the "*" in a separate
  expansion-context table); it is expanded with the current qualifier set and maps unchanged.
- Outside macro calls the qualifier set is never modified during descent: alternatives,
  sequences and optional groups all pass the caller's set unchanged.

## d) Order of evaluation inside 0x166CB0 — PROVED

For each node:
1. Condition test (a). Failure -> stop, nothing emitted, "not matched".
2. Style tags ($creature, $name, $object, $zone, $stress, $realm, $special, $number, $green,
   $blue, $purple, $yellow select a text style; $shake sets the shake flag), in set order.
3. The node's own text: "%" alone starts a new output segment; "@..." macro handling (c);
   "*..." sub-tree; "%%" emits "%"; any other text is emitted literally. Before the first text
   of an empty segment, if a non-empty category string is in effect, the category followed by
   ": " is emitted first.
4. If the node is optional (kind 3): t = s ^ (s<<13); t ^= t>>17; if bit 0 of t is 0 the
   children are skipped; the node's seed becomes s + 1 for its children either way. (With
   seed 0 an optional group is always skipped.)
5. Children: kind 2 as in (b). For every other kind each child is expanded in order, but if a
   child that itself carries conditions passes, the remaining siblings are not expanded.
6. Result: "matched" only when the node carried conditions and passed; otherwise "not matched".

## Evidence boundaries

- Established from the decompiled routines 0x167C80, 0x166CB0, 0x16A790, 0x16D530, 0x16BE20,
  0x1506A0, 0x16C1F0, 0x16CE30 and part of 0x1628B0, one assembly confirmation (0x167B20-0x167B3E:
  the collected item is the child node, only for kind 2 and an empty child condition set),
  the cross-reference list for 0x44E540, and raw `.rdata` reads.
- Not inspected: routines that look up macro/dictionary maps by key (0x166A20), map insertion
  helpers, output emission (0x16C880, 0x16C310, 0x15C380) and 0x16AF30/0x16B8A0 key builders;
  their roles above are inferred from call pattern and arguments.
- Not established: the dictionary template text that leads from the item name templates to the
  noun entry (owner data not read), hence which proved mechanism supplies "#default" there.
- No runtime test was performed; no game executable was run.

## Exposure statement

Read in this session: docs/CLEANROOM_AUTHOR.md (role policy, in full); decompiled function files
140167c80.c, 140166cb0.c, 14016a790.c, 14016d530.c, 14016be20.c, 1401506a0.c, 14016c1f0.c,
14016ce30.c in full and 1401628b0.c lines 90-389 plus a search for its call lines; search hits in
assembly.txt for the constant addresses listed above and the 14 instructions before the 0x16C1F0
call in 0x166CB0; strings.csv header and the row for 0x44E540; a directory filename listing of
decompiled\functions; PE headers, section table and the listed `.rdata` constants of the unpacked
image, and its SHA-256. Placed in context by the harness before the task: the project AGENTS.md
policy text (project requirements, no implementation detail), the repository branch name and an
unavailable git status, and the task prompt with its stated known facts and behavioral
observation. No implementation source, tests, tools, logs, memory, handoffs, other specs, SDKs,
reconstruction source, owner game data or web content was read. No prohibited material exposure.
