# Original compact binding-name dictionary

The constructor at RVA 0x49080 populates two separate signed-integer-to-wide-string dictionaries. The compact dictionary is the second one, at constructor receiver+0x10. On the normally completing fresh-construction path it contains exactly the 144 keys and strings listed below. Each value is supplied by a fixed original-image UTF-16 literal and an explicit code-unit count. No constructor entry is generated from an operating-system keyboard layout, current binding configuration, imported key-name query or numeric stream formatter.

## Authority and supported original identity

This is an independent original-source contract under the standalone docs/CLEANROOM_AUTHOR.md policy and this isolated assignment. No previous held handoff was read.

The supported original file is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. It is AMD64, machine 0x8664, preferred image base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0 and image size 0x58A000. Its file length is 5,757,952 bytes and its independently checked SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses identifying native routines, literals and instruction sites are RVAs; plus-offsets identify fields within the stated receiver.

G denotes the ordinary Game receiver. Original Game constructor 0x7EA80 preserves its incoming receiver and calls 0x49080 at 0x7EB04 with G+0x30. Thus the name-object receiver K is G+0x30. The first dictionary has its eight-byte head pointer at K and eight-byte count at K+0x8, alias G+0x30/+0x38. The compact dictionary has those words at K+0x10/+0x18, alias G+0x40/+0x48. The formatter 0x4CA10's dictionary branch explicitly reads the head pointer at its receiver+0x10; the other dictionary is not an alternate fallback in that branch.

## Construction, insertion and copy order

Complete constructor 0x49080 first clears the first dictionary's head/count words, calls 0x4C680 to allocate its head and publishes that returned pointer. It then clears the second dictionary's head/count words, allocates its separate head with the same helper and publishes it. Both empty headers therefore exist before the first literal assignment.

Complete 0x4C680 requests a 0x48-byte allocation. It writes the head's left, parent/root and right links at offsets zero, 0x8 and 0x10 back to that head. It writes the two-byte value 0x0101 at offset 0x18: the native one-byte color and sentinel markers are each one. A null allocation takes the reached nonreturning invalid-parameter provider call. This is an allocation/provider boundary, not a proved recoverable empty dictionary.

The constructor then performs all 144 assignments to the first dictionary before performing any assignment to the compact dictionary. Within either dictionary the key order is exactly the order of the compact table below; that order is strictly increasing under signed 32-bit comparison. The first compact key setup begins at 0x4AB3A, its lookup/insertion call is 0x4AB4C and its literal assignment call is 0x4AB64. The final compact lookup/insertion is 0x4C58D, followed by the literal assignment at 0x4C5A5 and the constructor's ordinary return.

Every assignment first supplies a pointer to a four-byte key to 0x48FB0. That complete helper performs a lower-bound tree search using signed 32-bit comparisons of keys at node+0x20. Node+0x19 is the byte sentinel test. On an exact existing key it returns that node and writes a zero insertion-result byte, without allocating another node. On absence it calls 0x48890 to construct a node and 0x48BE0 to link it, then returns the selected node with insertion-result byte one. The constructor does not branch on that result byte. It reads the returned node pointer, advances to its string object at node+0x28 and calls 0x3D4F0 with the selected original literal and explicit length. The table identifies each of those latter assignment calls.

Complete 0x4C6E0 requests a 0x48-byte node and initializes its three tree links from the dictionary head. Complete 0x48890 then writes zero to the two marker bytes at node+0x18/+0x19, copies the four-byte key into node+0x20 and initializes the wide-string object at node+0x28. That string starts with an eight-byte capacity of seven at string+0x18, eight-byte length zero at string+0x10 and a two-byte zero at the start of inline storage. These stores do not establish zero for all node padding or unused string bytes.

On the selected fresh-construction path, the first insertion sees count zero. Each later constructor key is greater than the current maximum. The selected 0x48BE0 branches admit that empty or end-hint insertion directly through 0x488F0. This complete insertion owner increments the eight-byte count, publishes parent/child and head-extreme links, performs its reached color/rotation adjustments and leaves the root color byte one before returning the inserted node. Rotation changes tree links rather than creating another key/string pair. The general hint fallback 0x48E50 was inspected as well; its alternate search/admission and rejected-node cleanup route are not needed to produce any of the 144 fresh compact entries.

The node is linked with its initially empty string before the constructor performs that node's literal copy. The constructor finishes that copy before starting the next key. This is static call/store order, not an atomic publication of the entire dictionary or a claim about observation from another thread.

There is no duplicate signed key in either constructor population. Consequently, there is no constructor-time overwrite of an earlier compact value: on normal successful fresh construction the compact count becomes 144. If 0x48FB0 is independently called on a dictionary already containing the selected key, its existing-node route preserves that node and count; a subsequent 0x3D4F0 call on that node's string replaces its text. That helper behavior does not imply that reentering 0x49080 on an existing object is a supported reset: the constructor starts by clearing its destination header words rather than first destroying old dictionary contents.

The complete explicit-length copy 0x3D4F0 copies two bytes per requested code unit, updates the eight-byte string length and writes a two-byte terminator after those units. It does not scan these constructor literals to determine the requested count. Its native overlap, length-error and storage-growth alternatives were inspected with 0x3D3B0 and 0x334F0. They do not alter the selected literal/code-unit pairs. Every compact value here has length one through four, fits in the new string's seven-code-unit inline capacity and has a separate original-image source. Thus the ordinary fresh compact assignments do not require the string-growth branch. The first dictionary can use that branch for its longer literals; allocation and memory-provider failures are not converted into a normally completed dictionary.

## Complete compact contents

Rows are in native construction order. Decimal keys are signed 32-bit values; the hexadecimal column gives the same positive value. Every listed string consists only of the shown ASCII-range UTF-16 code units. The unit column is authoritative: each four-hex-digit word denotes one unsigned 16-bit code unit in order, stored little-endian in the supported image. Length excludes the terminator. Quotes in the Text column delimit the value and are not stored. Character entities in that column merely display the grave accent, backslash and less-than characters without Markdown ambiguity.

All 144 native key operands, literal-address operands and explicit count operands were independently reconciled with the original C presentation and the matched native instructions. The selected original bytes contain the exact units below and an additional 0x0000 immediately after each literal's counted units. That following original zero is corroboration; the copy's admission and count come from the native operands.

| Order | Signed key, decimal (hex) | Exact text | UTF-16 units, hexadecimal | Length | Literal RVA | Assignment call RVA |
| --- | ---: | --- | --- | ---: | --- | --- |
| 1 | 1 (0x1) | "ESC" | 0045 0053 0043 | 3 | 0x4315B0 | 0x4AB64 |
| 2 | 2 (0x2) | "1" | 0031 | 1 | 0x430DC0 | 0x4AB93 |
| 3 | 3 (0x3) | "2" | 0032 | 1 | 0x430DC4 | 0x4ABC2 |
| 4 | 4 (0x4) | "3" | 0033 | 1 | 0x430DC8 | 0x4ABF1 |
| 5 | 5 (0x5) | "4" | 0034 | 1 | 0x430DCC | 0x4AC20 |
| 6 | 6 (0x6) | "5" | 0035 | 1 | 0x430DD0 | 0x4AC4F |
| 7 | 7 (0x7) | "6" | 0036 | 1 | 0x430DD4 | 0x4AC7E |
| 8 | 8 (0x8) | "7" | 0037 | 1 | 0x430DD8 | 0x4ACAD |
| 9 | 9 (0x9) | "8" | 0038 | 1 | 0x430DDC | 0x4ACDC |
| 10 | 10 (0xA) | "9" | 0039 | 1 | 0x430DE0 | 0x4AD0B |
| 11 | 11 (0xB) | "0" | 0030 | 1 | 0x4300CC | 0x4AD3A |
| 12 | 12 (0xC) | "-" | 002D | 1 | 0x4300D0 | 0x4AD69 |
| 13 | 13 (0xD) | "=" | 003D | 1 | 0x430DE4 | 0x4AD98 |
| 14 | 14 (0xE) | "BS" | 0042 0053 | 2 | 0x4315B8 | 0x4ADC7 |
| 15 | 15 (0xF) | "TAB" | 0054 0041 0042 | 3 | 0x4315C0 | 0x4ADF6 |
| 16 | 16 (0x10) | "Q" | 0051 | 1 | 0x430E08 | 0x4AE25 |
| 17 | 17 (0x11) | "W" | 0057 | 1 | 0x430E0C | 0x4AE54 |
| 18 | 18 (0x12) | "E" | 0045 | 1 | 0x430E10 | 0x4AE83 |
| 19 | 19 (0x13) | "R" | 0052 | 1 | 0x430E14 | 0x4AEB2 |
| 20 | 20 (0x14) | "T" | 0054 | 1 | 0x430E18 | 0x4AEE1 |
| 21 | 21 (0x15) | "Y" | 0059 | 1 | 0x430E1C | 0x4AF10 |
| 22 | 22 (0x16) | "U" | 0055 | 1 | 0x430E20 | 0x4AF3F |
| 23 | 23 (0x17) | "I" | 0049 | 1 | 0x430E24 | 0x4AF6E |
| 24 | 24 (0x18) | "O" | 004F | 1 | 0x430E28 | 0x4AF9D |
| 25 | 25 (0x19) | "P" | 0050 | 1 | 0x430E2C | 0x4AFCC |
| 26 | 26 (0x1A) | "[" | 005B | 1 | 0x430E30 | 0x4AFFB |
| 27 | 27 (0x1B) | "]" | 005D | 1 | 0x430E34 | 0x4B02A |
| 28 | 28 (0x1C) | "RET" | 0052 0045 0054 | 3 | 0x4315C8 | 0x4B059 |
| 29 | 29 (0x1D) | "LC" | 004C 0043 | 2 | 0x4315D0 | 0x4B088 |
| 30 | 30 (0x1E) | "A" | 0041 | 1 | 0x430E64 | 0x4B0B7 |
| 31 | 31 (0x1F) | "S" | 0053 | 1 | 0x430E68 | 0x4B0E6 |
| 32 | 32 (0x20) | "D" | 0044 | 1 | 0x430E6C | 0x4B115 |
| 33 | 33 (0x21) | "F" | 0046 | 1 | 0x430E70 | 0x4B144 |
| 34 | 34 (0x22) | "G" | 0047 | 1 | 0x430E74 | 0x4B173 |
| 35 | 35 (0x23) | "H" | 0048 | 1 | 0x430E78 | 0x4B1A2 |
| 36 | 36 (0x24) | "J" | 004A | 1 | 0x430E7C | 0x4B1D1 |
| 37 | 37 (0x25) | "K" | 004B | 1 | 0x430E80 | 0x4B200 |
| 38 | 38 (0x26) | "L" | 004C | 1 | 0x430E84 | 0x4B22F |
| 39 | 39 (0x27) | ";" | 003B | 1 | 0x430E88 | 0x4B25E |
| 40 | 40 (0x28) | "'" | 0027 | 1 | 0x430E8C | 0x4B28D |
| 41 | 41 (0x29) | "&#96;" | 0060 | 1 | 0x430E90 | 0x4B2BC |
| 42 | 42 (0x2A) | "LS" | 004C 0053 | 2 | 0x4315D8 | 0x4B2EB |
| 43 | 43 (0x2B) | "&#92;" | 005C | 1 | 0x430EB0 | 0x4B31A |
| 44 | 44 (0x2C) | "Z" | 005A | 1 | 0x430EB4 | 0x4B349 |
| 45 | 45 (0x2D) | "X" | 0058 | 1 | 0x430EB8 | 0x4B378 |
| 46 | 46 (0x2E) | "C" | 0043 | 1 | 0x430EBC | 0x4B3A7 |
| 47 | 47 (0x2F) | "V" | 0056 | 1 | 0x430EC0 | 0x4B3D6 |
| 48 | 48 (0x30) | "B" | 0042 | 1 | 0x430EC4 | 0x4B405 |
| 49 | 49 (0x31) | "N" | 004E | 1 | 0x430EC8 | 0x4B434 |
| 50 | 50 (0x32) | "M" | 004D | 1 | 0x430ECC | 0x4B463 |
| 51 | 51 (0x33) | "," | 002C | 1 | 0x430ED0 | 0x4B492 |
| 52 | 52 (0x34) | "." | 002E | 1 | 0x4300C8 | 0x4B4C1 |
| 53 | 53 (0x35) | "/" | 002F | 1 | 0x430ED4 | 0x4B4F0 |
| 54 | 54 (0x36) | "RS" | 0052 0053 | 2 | 0x4315E0 | 0x4B51F |
| 55 | 55 (0x37) | "N*" | 004E 002A | 2 | 0x4315E8 | 0x4B54E |
| 56 | 56 (0x38) | "LA" | 004C 0041 | 2 | 0x4315F0 | 0x4B57D |
| 57 | 57 (0x39) | "SP" | 0053 0050 | 2 | 0x4315F8 | 0x4B5AC |
| 58 | 58 (0x3A) | "CL" | 0043 004C | 2 | 0x431600 | 0x4B5DB |
| 59 | 59 (0x3B) | "F1" | 0046 0031 | 2 | 0x430F44 | 0x4B60A |
| 60 | 60 (0x3C) | "F2" | 0046 0032 | 2 | 0x430F4C | 0x4B639 |
| 61 | 61 (0x3D) | "F3" | 0046 0033 | 2 | 0x430F54 | 0x4B668 |
| 62 | 62 (0x3E) | "F4" | 0046 0034 | 2 | 0x430F5C | 0x4B697 |
| 63 | 63 (0x3F) | "F5" | 0046 0035 | 2 | 0x430F64 | 0x4B6C6 |
| 64 | 64 (0x40) | "F6" | 0046 0036 | 2 | 0x430F6C | 0x4B6F5 |
| 65 | 65 (0x41) | "F7" | 0046 0037 | 2 | 0x430F74 | 0x4B724 |
| 66 | 66 (0x42) | "F8" | 0046 0038 | 2 | 0x430F7C | 0x4B753 |
| 67 | 67 (0x43) | "F9" | 0046 0039 | 2 | 0x430F84 | 0x4B782 |
| 68 | 68 (0x44) | "F10" | 0046 0031 0030 | 3 | 0x430F90 | 0x4B7B1 |
| 69 | 69 (0x45) | "NL" | 004E 004C | 2 | 0x431608 | 0x4B7E0 |
| 70 | 70 (0x46) | "SL" | 0053 004C | 2 | 0x431610 | 0x4B80F |
| 71 | 71 (0x47) | "N7" | 004E 0037 | 2 | 0x431618 | 0x4B83E |
| 72 | 72 (0x48) | "N8" | 004E 0038 | 2 | 0x431620 | 0x4B86D |
| 73 | 73 (0x49) | "N9" | 004E 0039 | 2 | 0x431628 | 0x4B89C |
| 74 | 74 (0x4A) | "N-" | 004E 002D | 2 | 0x431630 | 0x4B8CB |
| 75 | 75 (0x4B) | "N4" | 004E 0034 | 2 | 0x431638 | 0x4B8FA |
| 76 | 76 (0x4C) | "N5" | 004E 0035 | 2 | 0x431640 | 0x4B929 |
| 77 | 77 (0x4D) | "N6" | 004E 0036 | 2 | 0x431648 | 0x4B958 |
| 78 | 78 (0x4E) | "N+" | 004E 002B | 2 | 0x431650 | 0x4B987 |
| 79 | 79 (0x4F) | "N1" | 004E 0031 | 2 | 0x431658 | 0x4B9B6 |
| 80 | 80 (0x50) | "N2" | 004E 0032 | 2 | 0x431660 | 0x4B9E5 |
| 81 | 81 (0x51) | "N3" | 004E 0033 | 2 | 0x431668 | 0x4BA14 |
| 82 | 82 (0x52) | "N0" | 004E 0030 | 2 | 0x431670 | 0x4BA43 |
| 83 | 83 (0x53) | "N." | 004E 002E | 2 | 0x431678 | 0x4BA72 |
| 84 | 86 (0x56) | "&lt;" | 003C | 1 | 0x4310FC | 0x4BAA1 |
| 85 | 87 (0x57) | "F11" | 0046 0031 0031 | 3 | 0x431100 | 0x4BAD0 |
| 86 | 88 (0x58) | "F12" | 0046 0031 0032 | 3 | 0x431108 | 0x4BAFF |
| 87 | 100 (0x64) | "F13" | 0046 0031 0033 | 3 | 0x431110 | 0x4BB2E |
| 88 | 101 (0x65) | "F14" | 0046 0031 0034 | 3 | 0x431118 | 0x4BB5D |
| 89 | 102 (0x66) | "F15" | 0046 0031 0035 | 3 | 0x431120 | 0x4BB8C |
| 90 | 112 (0x70) | "KN" | 004B 004E | 2 | 0x431680 | 0x4BBBB |
| 91 | 115 (0x73) | "C1" | 0043 0031 | 2 | 0x431134 | 0x4BBEA |
| 92 | 121 (0x79) | "CON" | 0043 004F 004E | 3 | 0x431688 | 0x4BC19 |
| 93 | 123 (0x7B) | "NCON" | 004E 0043 004F 004E | 4 | 0x431690 | 0x4BC48 |
| 94 | 125 (0x7D) | "YEN" | 0059 0045 004E | 3 | 0x4316A0 | 0x4BC77 |
| 95 | 126 (0x7E) | "C2" | 0043 0032 | 2 | 0x431170 | 0x4BCA6 |
| 96 | 141 (0x8D) | "N=" | 004E 003D | 2 | 0x4316A8 | 0x4BCD5 |
| 97 | 144 (0x90) | "PT" | 0050 0054 | 2 | 0x4316B0 | 0x4BD04 |
| 98 | 145 (0x91) | "@" | 0040 | 1 | 0x4311B0 | 0x4BD33 |
| 99 | 146 (0x92) | ":" | 003A | 1 | 0x4311B4 | 0x4BD62 |
| 100 | 147 (0x93) | "_" | 005F | 1 | 0x4311B8 | 0x4BD91 |
| 101 | 148 (0x94) | "KJ" | 004B 004A | 2 | 0x4316B8 | 0x4BDC0 |
| 102 | 149 (0x95) | "ST" | 0053 0054 | 2 | 0x4316C0 | 0x4BDEF |
| 103 | 150 (0x96) | "AX" | 0041 0058 | 2 | 0x4311DC | 0x4BE1E |
| 104 | 151 (0x97) | "UL" | 0055 004C | 2 | 0x4316C8 | 0x4BE4D |
| 105 | 153 (0x99) | "NT" | 004E 0054 | 2 | 0x4316D0 | 0x4BE7C |
| 106 | 156 (0x9C) | "NE" | 004E 0045 | 2 | 0x4316D8 | 0x4BEAB |
| 107 | 157 (0x9D) | "RC" | 0052 0043 | 2 | 0x4316E0 | 0x4BEDA |
| 108 | 160 (0xA0) | "MUT" | 004D 0055 0054 | 3 | 0x4316E8 | 0x4BF09 |
| 109 | 161 (0xA1) | "CAL" | 0043 0041 004C | 3 | 0x4316F0 | 0x4BF38 |
| 110 | 162 (0xA2) | "PP" | 0050 0050 | 2 | 0x4316F8 | 0x4BF67 |
| 111 | 164 (0xA4) | "MS" | 004D 0053 | 2 | 0x431700 | 0x4BF96 |
| 112 | 174 (0xAE) | "V-" | 0056 002D | 2 | 0x431708 | 0x4BFC5 |
| 113 | 176 (0xB0) | "V+" | 0056 002B | 2 | 0x431710 | 0x4BFF4 |
| 114 | 178 (0xB2) | "WH" | 0057 0048 | 2 | 0x431718 | 0x4C023 |
| 115 | 179 (0xB3) | "N," | 004E 002C | 2 | 0x431720 | 0x4C052 |
| 116 | 181 (0xB5) | "N/" | 004E 002F | 2 | 0x431728 | 0x4C081 |
| 117 | 183 (0xB7) | "SRQ" | 0053 0052 0051 | 3 | 0x431730 | 0x4C0B0 |
| 118 | 184 (0xB8) | "RA" | 0052 0041 | 2 | 0x431738 | 0x4C0DF |
| 119 | 197 (0xC5) | "PAU" | 0050 0041 0055 | 3 | 0x431740 | 0x4C10E |
| 120 | 199 (0xC7) | "HOM" | 0048 004F 004D | 3 | 0x431748 | 0x4C13D |
| 121 | 200 (0xC8) | "UA" | 0055 0041 | 2 | 0x431750 | 0x4C16C |
| 122 | 201 (0xC9) | "PGU" | 0050 0047 0055 | 3 | 0x431758 | 0x4C19B |
| 123 | 203 (0xCB) | "LA" | 004C 0041 | 2 | 0x4315F0 | 0x4C1CA |
| 124 | 205 (0xCD) | "RA" | 0052 0041 | 2 | 0x431738 | 0x4C1F9 |
| 125 | 207 (0xCF) | "END" | 0045 004E 0044 | 3 | 0x431760 | 0x4C228 |
| 126 | 208 (0xD0) | "DA" | 0044 0041 | 2 | 0x431768 | 0x4C257 |
| 127 | 209 (0xD1) | "PGD" | 0050 0047 0044 | 3 | 0x431770 | 0x4C286 |
| 128 | 210 (0xD2) | "INS" | 0049 004E 0053 | 3 | 0x431778 | 0x4C2B5 |
| 129 | 211 (0xD3) | "DEL" | 0044 0045 004C | 3 | 0x431780 | 0x4C2E4 |
| 130 | 219 (0xDB) | "WL" | 0057 004C | 2 | 0x431788 | 0x4C313 |
| 131 | 220 (0xDC) | "WR" | 0057 0052 | 2 | 0x431790 | 0x4C342 |
| 132 | 221 (0xDD) | "AM" | 0041 004D | 2 | 0x431798 | 0x4C371 |
| 133 | 222 (0xDE) | "SP" | 0053 0050 | 2 | 0x4315F8 | 0x4C3A0 |
| 134 | 223 (0xDF) | "SS" | 0053 0053 | 2 | 0x4317A0 | 0x4C3CF |
| 135 | 227 (0xE3) | "SW" | 0053 0057 | 2 | 0x4317A8 | 0x4C3FE |
| 136 | 229 (0xE5) | "WS" | 0057 0053 | 2 | 0x4317B0 | 0x4C42D |
| 137 | 230 (0xE6) | "WF" | 0057 0046 | 2 | 0x4317B8 | 0x4C45C |
| 138 | 231 (0xE7) | "WR" | 0057 0052 | 2 | 0x431790 | 0x4C48B |
| 139 | 232 (0xE8) | "WST" | 0057 0053 0054 | 3 | 0x4317C0 | 0x4C4BA |
| 140 | 233 (0xE9) | "WF" | 0057 0046 | 2 | 0x4317B8 | 0x4C4E9 |
| 141 | 234 (0xEA) | "WB" | 0057 0042 | 2 | 0x4317C8 | 0x4C518 |
| 142 | 235 (0xEB) | "MC" | 004D 0043 | 2 | 0x4317D0 | 0x4C547 |
| 143 | 236 (0xEC) | "MA" | 004D 0041 | 2 | 0x4317D8 | 0x4C576 |
| 144 | 237 (0xED) | "SEL" | 0053 0045 004C | 3 | 0x4317E0 | 0x4C5A5 |

There are 262 counted code units across these 144 stored values. No compact value is empty, contains an embedded zero, space, non-ASCII code unit or surrogate. No negative key, zero key or unlisted positive key is populated by this constructor. The holes in the numeric key domain remain absent; they are not filled by an inferred enumeration or an operating-system layout.

Keys are unique, but text is not. The complete repeated-value groups are LA at keys 56 and 203, SP at 57 and 222, RA at 184 and 205, WR at 220 and 231, and WF at 230 and 233. There are 139 distinct compact strings and 139 distinct selected literal addresses. Equal strings at different keys neither collide nor overwrite each other, because lookup/insertion compares the signed integer key, not the text.

## Distinction from the first dictionary

The first dictionary and compact dictionary have independent head/count words and independently allocated nodes. Their constructor key sets and key order are the same, but their text assignments can differ. The compact values are not generated by shortening the first dictionary at runtime; the constructor separately selects each compact literal and count.

Examples independently checked in this constructor include the following first-dictionary versus compact pairs:

| Signed key | First dictionary text and length | Compact text and length |
| ---: | --- | --- |
| 1 | Escape, 6 | ESC, 3 |
| 14 | Backspace, 9 | BS, 2 |
| 15 | Tab, 3 | TAB, 3 |
| 28 | Return, 6 | RET, 3 |
| 29 | Left Control, 12 | LC, 2 |
| 56 | Left Alt, 8 | LA, 2 |
| 123 | No Convert, 10 | NCON, 4 |
| 144 | Previous Track, 14 | PT, 2 |
| 203 | Left Arrow, 10 | LA, 2 |
| 220 | Windows Right, 13 | WR, 2 |
| 231 | Web Refresh, 11 | WR, 2 |
| 233 | Web Forward, 11 | WF, 2 |

These names are original literal strings, not claims about the active operating-system layout, a physical keyboard's legends or a supported imported input enumeration. Shared literal addresses for equal text do not merge the two dictionary nodes or turn one dictionary's header into the other.

## Formatter admission and outputs that are not entries

Complete formatter 0x4CA10 accepts a receiver K, an output-wide-string address and a pointer to two consecutive signed 32-bit words. It initially reads the first word as the key. If both words are zero, it initializes the output and supplies the one-code-unit question mark from original literal RVA 0x431820. That branch does not insert key zero.

Otherwise, a second word equal to one selects a stream-formatting branch. It constructs the reached temporary wide-stream objects, rereads the first 32-bit word at 0x4CB17, supplies the one-code-unit literal M at RVA 0x430ECC through 0x486B0, then performs a 32-bit addition of one at 0x4CB2D before passing the resulting signed integer to the imported wide integer-stream insertion. That native addition wraps at 32 bits. It next calls 0x415A0 for the current stream text and tears down the temporary stream objects through the reached helpers/providers. Imported integer formatting, locale, stream state, buffering and failures are outside this dictionary contract. No particular numeric string from that provider is assumed here. This branch does not read or insert a compact dictionary entry, and its composed output must not be counted among the 144 constructor values.

Every other second-word value selects the compact dictionary lookup. The native path at 0x4CB61 reads the head at K+0x10 and performs a signed 32-bit lower-bound search, using node+0x20 and the sentinel byte at node+0x19. It requires an exact key match. It initializes the output with capacity seven, length zero and a two-byte initial zero. A miss supplies the same one-code-unit question mark through 0x3D4F0. A match copies the current string object at found-node+0x28 through 0x3D3B0 with source offset zero and an unbounded requested count, which is limited to that source's actual eight-byte length. Neither the hit nor miss route changes a dictionary node, count or binding record.

The dictionary lookup is therefore not restricted to a second word of zero; non-one values other than zero also reach it unless the earlier both-zero test already returned. Negative or missing keys on that branch miss against the constructor state. The output on a hit is the current stored string, not necessarily its constructor value if an independent later writer has changed it.

These distinctions yield source-only conditional examples without a runtime capture. With constructor state retained, a lookup of key 14 under second word zero supplies BS. Key 84 is absent and supplies the question mark. Keys 56 and 203 both supply LA without either key overwriting the other. Key 123 supplies the four units NCON. A second word of two with key 1 still selects the dictionary branch and supplies ESC. A second word of one selects the stream path instead, even if its first word equals a populated dictionary key.

## Correspondence manifest and bounded coverage

Original filenames were discovered before content searches. The selected original evidence resides beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. The native binary is the supported file identified above. Export metadata used were decompiled/functions.csv, decompiled/function_ranges.csv and decompiled/calls.csv; instruction evidence was scoped by selected owner in decompiled/assembly.txt. Individual C presentation filenames beneath decompiled/functions are the full preferred image VA for the selected RVA followed by .c: 140049080.c, 14004ca10.c, 14004c680.c, 140048fb0.c, 140048890.c, 14004c6e0.c, 140048be0.c, 1400488f0.c, 140048e50.c, 14003d4f0.c, 14003d3b0.c, 1400334f0.c and 14007ea80.c.

All selected owner bodies were read completely. The full constructor population, selected node/header/insertion/copy behavior and complete formatter branches were interpreted to the provider boundaries stated here. The larger Game constructor is used only for the concrete K=G+0x30 initialization join and construction context; no closure of its unrelated world, interface, persistence or worker behavior is claimed.

The following inclusive ranges are the original exported owner ranges. Every instruction's exported bytes was compared at the matching raw-image offset using the supported PE section table. Every listed range also had continuous instruction-byte coverage from its first byte through its inclusive end. Separate exported ranges are listed separately; bytes between them were not silently counted as instructions.

| Owner RVA | Inclusive exported ranges | Instructions | Matched bytes |
| --- | --- | ---: | ---: |
| 0x334F0 | 0x334F0 through 0x33573; 0x33588 through 0x33607 | 78 | 260 |
| 0x3D3B0 | 0x3D3B0 through 0x3D4E1 | 88 | 306 |
| 0x3D4F0 | 0x3D4F0 through 0x3D634 | 100 | 325 |
| 0x48890 | 0x48890 through 0x488EB | 25 | 92 |
| 0x488F0 | 0x488F0 through 0x48BD6 | 207 | 743 |
| 0x48BE0 | 0x48BE0 through 0x48E43 | 162 | 612 |
| 0x48E50 | 0x48E50 through 0x48FAC | 100 | 349 |
| 0x48FB0 | 0x48FB0 through 0x49072 | 54 | 195 |
| 0x49080 | 0x49080 through 0x4C5BA | 2909 | 13627 |
| 0x4C680 | 0x4C680 through 0x4C6AB; 0x4C6AD through 0x4C6D2 | 21 | 82 |
| 0x4C6E0 | 0x4C6E0 through 0x4C70E; 0x4C710 through 0x4C73C | 27 | 92 |
| 0x4CA10 | 0x4CA10 through 0x4CBEA | 110 | 475 |
| 0x7EA80 | 0x7EA80 through 0x7F35D; 0x7F35F through 0x7F36D; 0x7F36F through 0x7F37D; 0x7F37F through 0x7F38A; 0x7F38C through 0x7F9DF | 806 | 3932 |

Across these thirteen complete owners, 4,687 instructions and 21,090 bytes matched the original image with zero mismatches and zero internal coverage gaps. The dictionary constructor itself is one continuous 13,627-byte owner with 2,909 matched instructions. Its native population contains 288 ordered lookup/insertion-plus-literal-copy assignments: 144 for the first dictionary and 144 for the compact dictionary. The complete sequence was reconciled against the C presentation with no key, receiver, literal-address, string-text or explicit-length discrepancy.

For the compact table, all 139 distinct source literal addresses were read from the raw original image, using every row's exact count and checking the following two-byte unit. The question-mark literal at 0x431820 and the M prefix at 0x430ECC are separately identified formatter operands. The table's assignment RVAs identify all 144 selected original 0x3D4F0 calls, rather than treating a nearby literal or decompiler display name as an association.

## Limits, lifetime and exposure

This is a complete constructor-population census for the identified compact dictionary, not a census of every writer in the executable. It does not exclude later edits, reassignment, dictionary replacement, independent construction, destruction, indirect calls or aliases. The exact lookup returns current storage; later name changes must be established from their own original writers. No installation controls.cfg, input configuration, asset, owner data or runtime output was opened.

No imported allocator, memory routine, wide-stream provider or operating-system input implementation was inspected. Native allocation sizes, initialization/copy operands and branches are established; successful storage allocation and normally returning copy/provider calls are conditions of the completed-state statements. Imported failure propagation, allocation exhaustion, corrupted trees/strings, invalid receivers, interrupted construction, exception unwind behavior and malformed pointer aliasing are not replaced by an invented fallback table. The inspected alternative insertion fallback reaches rejected-node cleanup 0x15D5B0; that cleanup's transitive closure is not needed or claimed for the fresh ascending constructor path.

The selected constructor and dictionary helpers contain no explicit native critical-section acquisition protecting this population as one transaction. No encompassing caller lock, other-thread access, atomic whole-table visibility, stream-provider reentrancy guarantee or scheduler/lifetime guarantee is inferred. The ordering described is native synchronous call/store order when the reached calls return normally.

Exposure statement: only the standalone source-author policy, this isolated assignment and filename-selected supported original executable/export evidence and metadata were read for this contract. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined project history, replacement implementation/exporter/extractor, SDK, dependency or reconstruction source, engine tests/results, owner files/data/outputs/captures, installation configuration, assets or packages was read. All prior held handoffs remained unread and unchanged. No original executable was run, no agent was spawned and no implementation or verification feedback was requested or consumed.

The complete code-free contract is released only after exclusive creation, whole saved-file readback, exact length/hash verification and rechecking the supported original identity. All artifact edits stop at the separately reported FINAL / STABLE / HELD boundary.
