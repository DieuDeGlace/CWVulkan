# Steam dictionary colors and displayed-word boundaries

Independent clean-room facts from the native dictionary parser **0x16d530**,
expansion **0x166cb0 /0x167c80**, word append **0x16c880**, word copy
**0x7d260**, and binding helpers below. Native RVAs and constant data are from
the original supported executable. The owner's `dict_en.xml` supplies the
bounded examples. No engine, tests, exporter or third-party reconstruction
was inspected and no game was executed.

This supplies the word inputs for answer-panel layout in
`remaining-runtime-gaps.md` §5.6. Retaining only the flattened displayed text
cannot, in general, recover these native colors and token boundaries.

## 1. Stored word input to the answer panel

The first list in each 0x1b0-byte dialogue page is an ordered displayed-word
list. Each word payload is 0x38 bytes:

| Word offset | Meaning |
| --- | --- |
|+0x00..+0x1f|Native wide-string header/storage for the displayed word|
|+0x20,+0x24,+0x28,+0x2c|Four float32 components R,G,B,A|
|+0x30|Byte flag set by `$shake`; independent of RGBA|
|+0x31..+0x37|Padding; not an additional style or text field|

The list node adds 16 bytes before this payload. **0x7d260** copies the string,
all four color components and the flag. Word append through **0x16c110**
preserves their order. The answer panel consumes each stored word's RGBA;
its hovered-row cyan override remains the later draw rule from §5.6.

## 2. Color selectors are scoped metadata

The parser stores a token beginning `$` in the **current grammar node's
tag set at +0x28**, instead of emitting it as a visible word. Expansion
**0x166cb0** starts each node with a copy of its inherited RGBA, applies
that node's tags, then expands its value and children. Ordinary dictionary
speech starts with **RGBA (1,1,1,1)** from **0x42e8c0**, flag 0, in 0x167c80.

A tag placed after text inside a group still affects that group's earlier
children: this is grammar-scope metadata, not a color change occurring only
at that point in the final text. Descendants inherit the resulting color;
leaving the scope restores the parent's color for its other children.
There is no global color state leaking into later siblings or speeches.

The tag set is ordered lexically by its wide-string keys. Expansion visits
that order, and each recognized color tag replaces the whole RGBA. With
multiple color tags in one scope, the **lexically last recognized selector**
wins, independently of their written order. Repeated identical tags do not
create repeated styling operations. Comparisons are exact/case-sensitive;
an unrecognized `$...` tag supplies no color assignment and is not visible
text. An arbitrary literal `$colour` has no generic numeric-color behavior.

Verified selectors in 0x166cb0 are:

| Tag | Stored float32 RGBA | Native constant RVA |
| --- | --- | --- |
|`$creature`, `$name`|(1,0.3,0.3,1)|0x4380a0|
|`$object`|(1,0.5,0.3,1)|0x44e3a0|
|`$zone`|(0.6,0.5,1,1)|0x438180|
|`$stress`|(0,1,1,1)|0x4381b0|
|`$realm`, `$special`|(1,0.9,0.5,1)|0x438120|
|`$number`|(0.3,1,0.3,1)|0x4380d0|
|`$green`|(0,1,0,1)|0x43f070|
|`$blue`|(0.3,0.45,1,1)|0x44e3c0|
|`$purple`|(0.55,0.2,1,1)|0x44e3b0|
|`$yellow`|(1,1,0,1)|0x43f080|

All decimals in this table denote their stored float32 values, without an
extra byte-color conversion. `$shake` sets the inherited word flag to 1
without assigning a color. The tag handling spans approximately
**0x166e20–0x1674c0**; unknown tags do not reset either inherited RGBA or flag.

## 3. Context-bound colors can override a tag color

The expansion context has separate maps for variable text **+0x10** and
variable RGBA **+0x30**. At an `@variable` node,0x166cb0 first applies the
scope's tags as above, then looks up that exact variable in the color map.
If a color binding exists, **its whole RGBA replaces the inherited/tag RGBA**
before that value is emitted. Without one, inherited/tag RGBA remains.
The color-map lookup is at **0x167650**; its final RGBA component load is
**0x167681**.

The same color travels into a referenced variable/template expansion. A
nested template's own scopes can subsequently replace it with their tags.
The binding does not mutate its parent's color. Thus styling an enclosing
group cyan does not guarantee that an item variable inside it remains cyan.

The relevant verified binding producers are:

- **0x15e710**, item definition, registers colors for the primary item
  variable (normally `@item`), its `rarity` form (`@itemrarity`), and its
  `@complete...` form (`@completeitem`).
  It obtains each color from **0x10a600**, passing the item's own region
  pair+0x0c/+0x10. This is an item-type/rarity color, not the player's current
  region or a reinterpretation of `$object`.
- **0x1607e0**, land definition, binds the supplied land variable's primary
  color through **0x16b000**, using its supplied biome ID. This applies to
  `@land`/`@land2` when those are the chosen prefixes.
- The separate numeric legacy zone-definition helper **0x15e200** binds
  its primary variable to **(1,0.5,0.25,1)**. Its name helper 0x164880 currently
  returns an empty name. This is not a reason to assign that color to every
  ordinary string-script `@zone` or to `$zone`.

Ordinary `define_number`/`define_number2` and `define_name` set text, without
an automatic variable RGBA binding. Their color follows the containing tags.
For example the inn-price number under `$yellow` is yellow, rather than
implicitly green merely because its variable is named `@number`.

For the item bindings above, 0x10a600 applies these overrides before the
ordinary rarity palette; all alpha components are 1:

| Item type / subtype | RGB |
| --- | --- |
|0x17 or 0x18|(1,0.6,0.25)|
|0x15|(0.7,1,0.25)|
|0|(1,1,1)|
|0x16, or type 1/subtype 13|(0.5,1,1)|
|0x0d|(0.5,0.1,1)|
|Others|Palette 0x64680: rarity 0 white; 1 green(0,1,0); 2 (0.3,0.45,1); 3 (0.55,0.2,1); 4 yellow(1,1,0)|

0x64680 clamps its signed palette input to −1..4; −1 is gray (0.7,0.7,0.7).
For the item-definition calls above, the own-region reference preserves
the item's stored rarity+0x14 in the branches using 0x109720; other ordinary
branches also use that stored rarity. Preserve the explicit type overrides.

The land color table 0x16b000 is fixed by biome, with alpha 1:

| Biome | RGB |
| --- | --- |
|0 or 2|(0.5,1,0.25)|
|1|(0.4,0.65,1)|
|3|(0.5,0.9,0.1)|
|4|(0.2,0.8,0.2)|
|5|(0.4,0.3,0.9)|
|6|(0.6,0.7,1)|
|7|(0.7,0.7,0.7)|
|8|(0.3,0.7,1)|
|9|(1,0.9,0.2)|
|10|(0.9,1,0.2)|
|11|(0.7,0.9,1)|
|12|(0.5,0.1,1)|
|13|(0.7,0.2,0.2)|
|Other|(1,1,1)|

## 4. Literal grammar tokenization precedes word append

**0x16d530** parses dictionary template text into grammar nodes before
expansion. Outside function-argument syntax, its literal punctuation mask
is **0xac00f38200000000**. It ends
the surrounding ordinary run and creates a separate single-character node
for each of these ASCII characters:

**!  '  (  )  ,  -  .  /  :  ;  =  ?**

These correspond to code points **0x21,0x27,0x28,0x29,0x2c,0x2d,0x2e,
0x2f,0x3a,0x3b,0x3d,0x3f**. This does not imply that all are classified as
punctuation by the answer panel's later spacing rule; that shorter set is
already stated in §5.6. Braces, brackets and vertical bar are grammar
structure, not ordinary visible punctuation nodes. Space, LF and tab delimit
runs; NUL terminates/flushes the final run. No Unicode-dash/apostrophe
normalization or generalized punctuation classification occurs in this mask.

Tokens starting `$` become the current node's metadata set; tokens starting
`#` become grammar tags. Ordinary textual runs become child value nodes.
Their displayed words are created later, after grammar choices and variable
expansion; copying every lexical token straight to the answer panel would
incorrectly display metadata and grammar controls.

## 5. Exact final word append, including apostrophes and glue

**0x16c880** takes one emitted string fragment, its current RGBA and flag.
It uses wide-stream whitespace extraction **0x6d6f0** to visit non-whitespace
tokens. It **does not re-run the dictionary punctuation lexer** on those
tokens. For each token, decisions use the last word in the current page's
already accumulated word list, even if that word came from a different
grammar scope, variable or preceding speech fragment:

1. If there is no stored word, append a new word with this token's RGBA/flag.
2. If the last stored word is **exactly the one-character `=`**, remove that
   word. If a preceding word now exists, concatenate the incoming token onto
   its string. Otherwise the incoming token is discarded by this branch.
3. Otherwise, if the last stored word ends in **ASCII apostrophe 0x27**, or
   the incoming token is **exactly one ASCII apostrophe**, concatenate the
   incoming token onto the last word.
4. Otherwise append a new stored word with the incoming RGBA/flag.

Concatenation adds **no space** and preserves the **existing word's RGBA and
flag**; it does not replace them by the incoming token's metadata. The `=`
marker's metadata is discarded when it is removed. These are single-step
last-word rules, not a general expression-joining or repeated-marker parser.
Hyphens have no comparable merge rule and remain separate words when the
literal grammar lexer separated them. The glue/apostrophe decisions are
around **0x16ca00–0x16cb85**; word-field copy is 0x7d260.

The helper tests stream state **before** extraction, not immediately after
it. The extraction call at **0x16c9cd** is followed directly by the word-list
decisions at **0x16c9d2**, without a successful-extraction guard. Thus an
empty string or an attempted extraction after trailing whitespace can feed
the initialized **empty token** through these same rules. With no existing
word, that can create an empty stored word; with a normal final word it can
append an empty word. Do not add a general empty-token filter while claiming
the exact native word count. This is derived from the traced stream/append
control flow, not a new runtime capture.

This distinction matters for substitutions. A bound `@name` value containing
`a-b` is passed to the append helper as a string and can stay one word
`a-b`; literal dictionary text `a-b` has three grammar values and becomes
three words. Do not tokenize the final flattened text globally and assume
it is equivalent. A substituted string's spaces can create multiple words,
but its internal punctuation is not automatically reparsed as grammar.

## 6. Derived examples useful for independent layout checks

These are consequences of the verified parser/append operations, not new
native runtime captures:

| Emitted source kind | Final stored word strings |
| --- | --- |
|Literal `don't!`|`don't`, `!`|
|Literal `well-known`|`well`, `-`, `known`|
|Literal `18=:=00`|`18:00`|
|One substituted fragment `a-b`|`a-b`|
|One substituted fragment `a-b.`|`a-b.`|

For the owner's inn answer `rest_inn?`, with number 10, the final strings are
**I / want / to / rest / here / until / next / morning / . / 10 / ®**.
The first nine words are white; **10 and ® are yellow** from their enclosing
`$yellow` group. There is no visible `$yellow` word and no price action is
performed by this expansion. The answer-choice ID remains separate metadata.

If a yellow word is followed by an apostrophe and suffix emitted under a
different color, the merged word keeps yellow. These stored word boundaries
and colors should reach the existing choice-panel measurement/drawing rules
directly; their punctuation spacing and hovered cyan are later UI operations.

## 7. Postfix `@variable=s` is ordinary glue, not a possessive operator

There is **no special English-possessive transformation** for this syntax
in the native parser/expander/word append path. Outside function arguments,
`@variable=s` parses as the variable value node, a separate literal `=` node,
and a literal `s` node. The parser's ordinary punctuation-node construction
is at **0x16e016–0x16e0e5**, within 0x16d530; `=` is one of its mask bits.
Plain variable expansion in **0x166cb0** looks up the variable's exact name
and sends its bound string to 0x16c880. It does not inspect a following `=s`,
the final character of the bound name, or an English singular/plural rule.

The later ordinary glue rule in **0x16ca3d–0x16caf0** removes the standalone
`=` word and appends the incoming `s` to the preceding stored word. It inserts
**no apostrophe**. The appended suffix retains that preceding word's RGBA and
flag. There is no special case for a preceding name already ending in `s`.
The owner's `mob_names`, `mob2_names` and `mob3_names` dictionary macros use
this spelling with their corresponding name variables and `$name`; their
macro names do not change the generic parser/append behavior.

For nonempty, single-word bound values without trailing whitespace, these
are derived exact string results, not new runtime captures:

| Template fragment | Bound value | Stored result |
| --- | --- | --- |
|`@name=s`|`Robin`|`Robins`|
|`@name=s`|`James`|`Jamess`|
|`@name's`|`Robin`|`Robin's`|
|`@name's`|`James`|`James's`|
|`@name'`|`James`|`James'`|

An apostrophe must be present in the template or substituted value to appear
in the output. For explicit `@name's`, the punctuation lexer separates the
apostrophe; the two generic apostrophe-joining rules in §5 then attach it
and the following `s`. These rules also do not select a different suffix
according to the name's last letter. With a multiword substituted value,
glue and apostrophe joins operate on its **last stored word**, as in §5.

## 8. Dictionary XML normalizes source line endings before grammar parsing

The ordinary dictionary import path **does normalize literal CRLF and CR
to LF before 0x16d530 sees the text**. The grammar tokenizer itself has no
separate CR-skipping rule: its whitespace separators remain space, LF and
tab, as stated in §4. Feeding a raw XML body's CR characters straight into
that tokenizer therefore does not reproduce the native import boundary.

The dictionary constructor **0x1510f0** sets XML parse options **0x74** at
**0x15bc2d** and calls file loader **0x13c6a0** at **0x15bc48**. The file is
opened in binary mode, so this is not a C-runtime text-file conversion.
The loader passes its options through **0x13c580 / 0x13d3e0** to XML parser
**0x13cbb0**. That parser selects its ordinary text processor by bits 4–5
of the options: for 0x74 the selector is 3, choosing **0x13c990**.

In 0x13c990, a literal source CR becomes LF. If the source character
immediately following that CR is LF, that second character is removed:
CRLF becomes **one** LF, while a standalone CR also becomes one LF. An
existing standalone LF remains LF. The processor preserves other ordinary
text characters and line indentation; this is not blanket trimming or
collapsing all text whitespace to spaces. XML entity/reference decoding is
also performed in this text processor through **0x13ed60**, rather than by
the dialogue word append helper.

The dictionary then obtains the parsed XML text with **0x13bbe0**, copies
that wide text through **0x41d80**, and invokes grammar parser 0x16d530.
For example, the ordinary top-level template import follows this sequence
at **0x15c2a4**, **0x15c2b3**, **0x15c2d2**. There is no intervening restoration
of source-file CR characters.

Consequently the owner's CRLF indentation around the inn-answer body
reaches the grammar parser as LF plus indentation spaces. Those delimit
ordinary grammar runs without creating standalone CR value nodes. They
do not add leading/trailing empty displayed words to the eleven-word inn
example in §6. This does **not** remove the distinct §5 behavior when an
actually emitted fragment is empty or ends in whitespace; preserve that
append behavior independently of XML source normalization.
