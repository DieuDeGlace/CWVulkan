Original Steam detailed binding-name dictionary and Controls text publication
Independent original-source contract, 2026-11-10

This contract concerns the first dictionary built by RVA 0x49080, the detailed binding formatter at RVA 0x4C830, and the concretely identified Controls and Help draw consumers. It is an independent reading of the supported original image and its selected original exports. It does not derive physical key identities from an operating-system layout, SDK enumeration, an installed configuration, or another implementation. Native literal names are reported as strings, without claiming that a particular device or locale produces their numeric keys.

The supported executable is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its length is 5,757,952 bytes and its SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. It is an AMD64 PE image, machine word 0x8664, preferred image base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, and SizeOfImage 0x58A000. Addresses below are RVAs. Matching instruction encodings were checked against this file through its PE section-to-file mapping. The executable was not run.

Receiver and dictionary ownership

Let G denote the original Game receiver. Original image RTTI reached from the table at 0x436068 identifies Game. In its constructor at 0x7EA80, the call at 0x7EB04 passes G+0x30 to 0x49080. Let D denote that G+0x30 receiver. D+0 is the first dictionary's eight-byte head pointer and D+8 is its eight-byte count. D+0x10 and D+0x18 are the second dictionary's separate head and count. Thus the ordinary first dictionary is at G+0x30, and the second is at G+0x40. A subsequent, separate construction call at 0x7EB0E uses G+0x50; it must not be conflated with either of these dictionary receivers.

The complete 0x49080 owner first initializes the first head/count pair, obtains its sentinel through 0x4C680, and publishes that head; it then initializes and allocates the second pair. All 144 first-dictionary assignments precede all 144 second-dictionary assignments. Native register and receiver operands distinguish the two batches throughout. Each assignment supplies a 32-bit key to 0x48FB0, then assigns the counted UTF-16 literal to the returned node's string at node+0x28. The inserted/not-inserted indication does not suppress that string assignment. The first batch's last assignment is at 0x4AB35; it does not continue into the second batch's names.

The constructor's complete call-target set consists of the original head allocator 0x4C680, find-or-insert wrapper 0x48FB0, and counted string assignment 0x3D4F0. No call in this owner obtains a key name from a device, OS layout, random provider, localization provider, or an imported name-generating routine. The imported allocation and memory-copy operations underneath the original helpers remain provider boundaries.

The first dictionary has 144 distinct signed 32-bit keys, all positive and between 1 and 237 inclusive, with the gaps explicitly visible in the table below. Its native construction order is ascending key order. There is no zero key, negative key, or duplicate key in this complete constructor batch. The table contains 725 nonterminating UTF-16 code units in total. All 144 instruction-selected literal starts are distinct. Every stated counted length and every code unit, plus the immediately following zero word, was independently read from the original image.

The head allocator obtains 0x48 bytes, installs three self-referencing link pointers, and stores the two-byte value 0x0101 at node+0x18. A regular node has three eight-byte links at +0, +8 and +0x10; the selected tree logic uses flag bytes +0x18/+0x19 and a signed 32-bit key at +0x20. Its mapped string begins at +0x28, with eight-byte length at +0x38 and capacity at +0x40. Allocation through 0x4C6E0 initializes the three links from the head; 0x48890 stores the key, clears its two flag bytes, and establishes an empty mapped string with capacity 7, length 0 and an initial zero word. These stores do not establish that every allocation byte or padding byte is zero.

The complete 0x48FB0 search uses signed comparisons of the supplied key and node+0x20. An exact existing key returns that node and a false insertion byte without allocating a replacement node. A miss constructs a node and passes it through 0x48BE0. The selected hint paths and the full-search fallback at 0x48E50 retain signed key ordering. The insertion core 0x488F0 increments the eight-byte count, links the new node, updates head extrema/root when needed, and performs link rotations and flag changes. The native count comparison uses constant 0x038E38E38E38E38D and admits only an unsigned count below it; equivalently, the overflow path rejects a count above 0x038E38E38E38E38C before incrementing it, disposes the candidate string/node, and invokes the length-error path. The fallback duplicate route disposes its unused candidate through 0x15D5B0 and returns the retained node. Consequently a repeated constructor assignment would replace the existing mapped string while retaining one key entry; no such repeat actually occurs in the first batch.

Complete first-dictionary literal inventory

In this table, the decimal value is the signed key; the eight-digit hexadecimal value gives the same 32-bit operand. The displayed double quotes are delimiters and do not belong to the strings. The code-unit column is authoritative, in original order, without the terminating 0000. A backslash and grave accent in the text column are displayed by character entities solely to keep the table readable. Lengths count 16-bit units, not bytes. Assignment RVAs identify the native counted-assignment calls, independently of the literal addresses.


| Order | Signed key / word | Literal text | UTF-16 length | Exact code units | Literal RVA | Counted assignment RVA |
|---:|---|---|---:|---|---|---|
| 1 | 1 / 0x00000001 | "Escape" | 6 | 0045 0073 0063 0061 0070 0065 | 0x430DB0 | 0x490F4 |
| 2 | 2 / 0x00000002 | "1" | 1 | 0031 | 0x430DC0 | 0x49123 |
| 3 | 3 / 0x00000003 | "2" | 1 | 0032 | 0x430DC4 | 0x49152 |
| 4 | 4 / 0x00000004 | "3" | 1 | 0033 | 0x430DC8 | 0x49181 |
| 5 | 5 / 0x00000005 | "4" | 1 | 0034 | 0x430DCC | 0x491B0 |
| 6 | 6 / 0x00000006 | "5" | 1 | 0035 | 0x430DD0 | 0x491DF |
| 7 | 7 / 0x00000007 | "6" | 1 | 0036 | 0x430DD4 | 0x4920E |
| 8 | 8 / 0x00000008 | "7" | 1 | 0037 | 0x430DD8 | 0x4923D |
| 9 | 9 / 0x00000009 | "8" | 1 | 0038 | 0x430DDC | 0x4926C |
| 10 | 10 / 0x0000000A | "9" | 1 | 0039 | 0x430DE0 | 0x4929B |
| 11 | 11 / 0x0000000B | "0" | 1 | 0030 | 0x4300CC | 0x492CA |
| 12 | 12 / 0x0000000C | "-" | 1 | 002D | 0x4300D0 | 0x492F9 |
| 13 | 13 / 0x0000000D | "=" | 1 | 003D | 0x430DE4 | 0x49328 |
| 14 | 14 / 0x0000000E | "Backspace" | 9 | 0042 0061 0063 006B 0073 0070 0061 0063 0065 | 0x430DE8 | 0x49357 |
| 15 | 15 / 0x0000000F | "Tab" | 3 | 0054 0061 0062 | 0x430E00 | 0x49386 |
| 16 | 16 / 0x00000010 | "Q" | 1 | 0051 | 0x430E08 | 0x493B5 |
| 17 | 17 / 0x00000011 | "W" | 1 | 0057 | 0x430E0C | 0x493E4 |
| 18 | 18 / 0x00000012 | "E" | 1 | 0045 | 0x430E10 | 0x49413 |
| 19 | 19 / 0x00000013 | "R" | 1 | 0052 | 0x430E14 | 0x49442 |
| 20 | 20 / 0x00000014 | "T" | 1 | 0054 | 0x430E18 | 0x49471 |
| 21 | 21 / 0x00000015 | "Y" | 1 | 0059 | 0x430E1C | 0x494A0 |
| 22 | 22 / 0x00000016 | "U" | 1 | 0055 | 0x430E20 | 0x494CF |
| 23 | 23 / 0x00000017 | "I" | 1 | 0049 | 0x430E24 | 0x494FE |
| 24 | 24 / 0x00000018 | "O" | 1 | 004F | 0x430E28 | 0x4952D |
| 25 | 25 / 0x00000019 | "P" | 1 | 0050 | 0x430E2C | 0x4955C |
| 26 | 26 / 0x0000001A | "[" | 1 | 005B | 0x430E30 | 0x4958B |
| 27 | 27 / 0x0000001B | "]" | 1 | 005D | 0x430E34 | 0x495BA |
| 28 | 28 / 0x0000001C | "Return" | 6 | 0052 0065 0074 0075 0072 006E | 0x430E38 | 0x495E9 |
| 29 | 29 / 0x0000001D | "Left Control" | 12 | 004C 0065 0066 0074 0020 0043 006F 006E 0074 0072 006F 006C | 0x430E48 | 0x49618 |
| 30 | 30 / 0x0000001E | "A" | 1 | 0041 | 0x430E64 | 0x49647 |
| 31 | 31 / 0x0000001F | "S" | 1 | 0053 | 0x430E68 | 0x49676 |
| 32 | 32 / 0x00000020 | "D" | 1 | 0044 | 0x430E6C | 0x496A5 |
| 33 | 33 / 0x00000021 | "F" | 1 | 0046 | 0x430E70 | 0x496D4 |
| 34 | 34 / 0x00000022 | "G" | 1 | 0047 | 0x430E74 | 0x49703 |
| 35 | 35 / 0x00000023 | "H" | 1 | 0048 | 0x430E78 | 0x49732 |
| 36 | 36 / 0x00000024 | "J" | 1 | 004A | 0x430E7C | 0x49761 |
| 37 | 37 / 0x00000025 | "K" | 1 | 004B | 0x430E80 | 0x49790 |
| 38 | 38 / 0x00000026 | "L" | 1 | 004C | 0x430E84 | 0x497BF |
| 39 | 39 / 0x00000027 | ";" | 1 | 003B | 0x430E88 | 0x497EE |
| 40 | 40 / 0x00000028 | "'" | 1 | 0027 | 0x430E8C | 0x4981D |
| 41 | 41 / 0x00000029 | "&#96;" | 1 | 0060 | 0x430E90 | 0x4984C |
| 42 | 42 / 0x0000002A | "Left Shift" | 10 | 004C 0065 0066 0074 0020 0053 0068 0069 0066 0074 | 0x430E98 | 0x4987B |
| 43 | 43 / 0x0000002B | "&#92;" | 1 | 005C | 0x430EB0 | 0x498AA |
| 44 | 44 / 0x0000002C | "Z" | 1 | 005A | 0x430EB4 | 0x498D9 |
| 45 | 45 / 0x0000002D | "X" | 1 | 0058 | 0x430EB8 | 0x49908 |
| 46 | 46 / 0x0000002E | "C" | 1 | 0043 | 0x430EBC | 0x49937 |
| 47 | 47 / 0x0000002F | "V" | 1 | 0056 | 0x430EC0 | 0x49966 |
| 48 | 48 / 0x00000030 | "B" | 1 | 0042 | 0x430EC4 | 0x49995 |
| 49 | 49 / 0x00000031 | "N" | 1 | 004E | 0x430EC8 | 0x499C4 |
| 50 | 50 / 0x00000032 | "M" | 1 | 004D | 0x430ECC | 0x499F3 |
| 51 | 51 / 0x00000033 | "," | 1 | 002C | 0x430ED0 | 0x49A22 |
| 52 | 52 / 0x00000034 | "." | 1 | 002E | 0x4300C8 | 0x49A51 |
| 53 | 53 / 0x00000035 | "/" | 1 | 002F | 0x430ED4 | 0x49A80 |
| 54 | 54 / 0x00000036 | "Right Shift" | 11 | 0052 0069 0067 0068 0074 0020 0053 0068 0069 0066 0074 | 0x430ED8 | 0x49AAF |
| 55 | 55 / 0x00000037 | "Numpad *" | 8 | 004E 0075 006D 0070 0061 0064 0020 002A | 0x430EF0 | 0x49ADE |
| 56 | 56 / 0x00000038 | "Left Alt" | 8 | 004C 0065 0066 0074 0020 0041 006C 0074 | 0x430F08 | 0x49B0D |
| 57 | 57 / 0x00000039 | "Space" | 5 | 0053 0070 0061 0063 0065 | 0x430F20 | 0x49B3C |
| 58 | 58 / 0x0000003A | "Caps Lock" | 9 | 0043 0061 0070 0073 0020 004C 006F 0063 006B | 0x430F30 | 0x49B6B |
| 59 | 59 / 0x0000003B | "F1" | 2 | 0046 0031 | 0x430F44 | 0x49B9A |
| 60 | 60 / 0x0000003C | "F2" | 2 | 0046 0032 | 0x430F4C | 0x49BC9 |
| 61 | 61 / 0x0000003D | "F3" | 2 | 0046 0033 | 0x430F54 | 0x49BF8 |
| 62 | 62 / 0x0000003E | "F4" | 2 | 0046 0034 | 0x430F5C | 0x49C27 |
| 63 | 63 / 0x0000003F | "F5" | 2 | 0046 0035 | 0x430F64 | 0x49C56 |
| 64 | 64 / 0x00000040 | "F6" | 2 | 0046 0036 | 0x430F6C | 0x49C85 |
| 65 | 65 / 0x00000041 | "F7" | 2 | 0046 0037 | 0x430F74 | 0x49CB4 |
| 66 | 66 / 0x00000042 | "F8" | 2 | 0046 0038 | 0x430F7C | 0x49CE3 |
| 67 | 67 / 0x00000043 | "F9" | 2 | 0046 0039 | 0x430F84 | 0x49D12 |
| 68 | 68 / 0x00000044 | "F10" | 3 | 0046 0031 0030 | 0x430F90 | 0x49D41 |
| 69 | 69 / 0x00000045 | "Num Lock" | 8 | 004E 0075 006D 0020 004C 006F 0063 006B | 0x430F98 | 0x49D70 |
| 70 | 70 / 0x00000046 | "Scroll Lock" | 11 | 0053 0063 0072 006F 006C 006C 0020 004C 006F 0063 006B | 0x430FB0 | 0x49D9F |
| 71 | 71 / 0x00000047 | "Numpad 7" | 8 | 004E 0075 006D 0070 0061 0064 0020 0037 | 0x430FC8 | 0x49DCE |
| 72 | 72 / 0x00000048 | "Numpad 8" | 8 | 004E 0075 006D 0070 0061 0064 0020 0038 | 0x430FE0 | 0x49DFD |
| 73 | 73 / 0x00000049 | "Numpad 9" | 8 | 004E 0075 006D 0070 0061 0064 0020 0039 | 0x430FF8 | 0x49E2C |
| 74 | 74 / 0x0000004A | "Numpad -" | 8 | 004E 0075 006D 0070 0061 0064 0020 002D | 0x431010 | 0x49E5B |
| 75 | 75 / 0x0000004B | "Numpad 4" | 8 | 004E 0075 006D 0070 0061 0064 0020 0034 | 0x431028 | 0x49E8A |
| 76 | 76 / 0x0000004C | "Numpad 5" | 8 | 004E 0075 006D 0070 0061 0064 0020 0035 | 0x431040 | 0x49EB9 |
| 77 | 77 / 0x0000004D | "Numpad 6" | 8 | 004E 0075 006D 0070 0061 0064 0020 0036 | 0x431058 | 0x49EE8 |
| 78 | 78 / 0x0000004E | "Numpad +" | 8 | 004E 0075 006D 0070 0061 0064 0020 002B | 0x431070 | 0x49F17 |
| 79 | 79 / 0x0000004F | "Numpad 1" | 8 | 004E 0075 006D 0070 0061 0064 0020 0031 | 0x431088 | 0x49F46 |
| 80 | 80 / 0x00000050 | "Numpad 2" | 8 | 004E 0075 006D 0070 0061 0064 0020 0032 | 0x4310A0 | 0x49F75 |
| 81 | 81 / 0x00000051 | "Numpad 3" | 8 | 004E 0075 006D 0070 0061 0064 0020 0033 | 0x4310B8 | 0x49FA4 |
| 82 | 82 / 0x00000052 | "Numpad 0" | 8 | 004E 0075 006D 0070 0061 0064 0020 0030 | 0x4310D0 | 0x49FD3 |
| 83 | 83 / 0x00000053 | "Numpad ." | 8 | 004E 0075 006D 0070 0061 0064 0020 002E | 0x4310E8 | 0x4A002 |
| 84 | 86 / 0x00000056 | "<" | 1 | 003C | 0x4310FC | 0x4A031 |
| 85 | 87 / 0x00000057 | "F11" | 3 | 0046 0031 0031 | 0x431100 | 0x4A060 |
| 86 | 88 / 0x00000058 | "F12" | 3 | 0046 0031 0032 | 0x431108 | 0x4A08F |
| 87 | 100 / 0x00000064 | "F13" | 3 | 0046 0031 0033 | 0x431110 | 0x4A0BE |
| 88 | 101 / 0x00000065 | "F14" | 3 | 0046 0031 0034 | 0x431118 | 0x4A0ED |
| 89 | 102 / 0x00000066 | "F15" | 3 | 0046 0031 0035 | 0x431120 | 0x4A11C |
| 90 | 112 / 0x00000070 | "Kana" | 4 | 004B 0061 006E 0061 | 0x431128 | 0x4A14B |
| 91 | 115 / 0x00000073 | "C1" | 2 | 0043 0031 | 0x431134 | 0x4A17A |
| 92 | 121 / 0x00000079 | "Convert" | 7 | 0043 006F 006E 0076 0065 0072 0074 | 0x431140 | 0x4A1A9 |
| 93 | 123 / 0x0000007B | "No Convert" | 10 | 004E 006F 0020 0043 006F 006E 0076 0065 0072 0074 | 0x431150 | 0x4A1D8 |
| 94 | 125 / 0x0000007D | "Yen" | 3 | 0059 0065 006E | 0x431168 | 0x4A207 |
| 95 | 126 / 0x0000007E | "C2" | 2 | 0043 0032 | 0x431170 | 0x4A236 |
| 96 | 141 / 0x0000008D | "Numpad =" | 8 | 004E 0075 006D 0070 0061 0064 0020 003D | 0x431178 | 0x4A265 |
| 97 | 144 / 0x00000090 | "Previous Track" | 14 | 0050 0072 0065 0076 0069 006F 0075 0073 0020 0054 0072 0061 0063 006B | 0x431190 | 0x4A294 |
| 98 | 145 / 0x00000091 | "@" | 1 | 0040 | 0x4311B0 | 0x4A2C3 |
| 99 | 146 / 0x00000092 | ":" | 1 | 003A | 0x4311B4 | 0x4A2F2 |
| 100 | 147 / 0x00000093 | "_" | 1 | 005F | 0x4311B8 | 0x4A321 |
| 101 | 148 / 0x00000094 | "Kanji" | 5 | 004B 0061 006E 006A 0069 | 0x4311C0 | 0x4A350 |
| 102 | 149 / 0x00000095 | "Stop" | 4 | 0053 0074 006F 0070 | 0x4311D0 | 0x4A37F |
| 103 | 150 / 0x00000096 | "AX" | 2 | 0041 0058 | 0x4311DC | 0x4A3AE |
| 104 | 151 / 0x00000097 | "Unlabeled" | 9 | 0055 006E 006C 0061 0062 0065 006C 0065 0064 | 0x4311E8 | 0x4A3DD |
| 105 | 153 / 0x00000099 | "Next Track" | 10 | 004E 0065 0078 0074 0020 0054 0072 0061 0063 006B | 0x431200 | 0x4A40C |
| 106 | 156 / 0x0000009C | "Numpad Enter" | 12 | 004E 0075 006D 0070 0061 0064 0020 0045 006E 0074 0065 0072 | 0x431218 | 0x4A43B |
| 107 | 157 / 0x0000009D | "Right Control" | 13 | 0052 0069 0067 0068 0074 0020 0043 006F 006E 0074 0072 006F 006C | 0x431238 | 0x4A46A |
| 108 | 160 / 0x000000A0 | "Mute" | 4 | 004D 0075 0074 0065 | 0x431258 | 0x4A499 |
| 109 | 161 / 0x000000A1 | "Calculator" | 10 | 0043 0061 006C 0063 0075 006C 0061 0074 006F 0072 | 0x431268 | 0x4A4C8 |
| 110 | 162 / 0x000000A2 | "Play/Pause" | 10 | 0050 006C 0061 0079 002F 0050 0061 0075 0073 0065 | 0x431280 | 0x4A4F7 |
| 111 | 164 / 0x000000A4 | "Media Stop" | 10 | 004D 0065 0064 0069 0061 0020 0053 0074 006F 0070 | 0x431298 | 0x4A526 |
| 112 | 174 / 0x000000AE | "Volume -" | 8 | 0056 006F 006C 0075 006D 0065 0020 002D | 0x4312B0 | 0x4A555 |
| 113 | 176 / 0x000000B0 | "Volume +" | 8 | 0056 006F 006C 0075 006D 0065 0020 002B | 0x4312C8 | 0x4A584 |
| 114 | 178 / 0x000000B2 | "Web Home" | 8 | 0057 0065 0062 0020 0048 006F 006D 0065 | 0x4312E0 | 0x4A5B3 |
| 115 | 179 / 0x000000B3 | "Numpad ," | 8 | 004E 0075 006D 0070 0061 0064 0020 002C | 0x4312F8 | 0x4A5E2 |
| 116 | 181 / 0x000000B5 | "Numpad /" | 8 | 004E 0075 006D 0070 0061 0064 0020 002F | 0x431310 | 0x4A611 |
| 117 | 183 / 0x000000B7 | "SYSRQ" | 5 | 0053 0059 0053 0052 0051 | 0x431328 | 0x4A640 |
| 118 | 184 / 0x000000B8 | "Right Alt" | 9 | 0052 0069 0067 0068 0074 0020 0041 006C 0074 | 0x431338 | 0x4A66F |
| 119 | 197 / 0x000000C5 | "Pause" | 5 | 0050 0061 0075 0073 0065 | 0x431350 | 0x4A69E |
| 120 | 199 / 0x000000C7 | "Home" | 4 | 0048 006F 006D 0065 | 0x431360 | 0x4A6CD |
| 121 | 200 / 0x000000C8 | "Up Arrow" | 8 | 0055 0070 0020 0041 0072 0072 006F 0077 | 0x431370 | 0x4A6FC |
| 122 | 201 / 0x000000C9 | "Page Up" | 7 | 0050 0061 0067 0065 0020 0055 0070 | 0x431388 | 0x4A72B |
| 123 | 203 / 0x000000CB | "Left Arrow" | 10 | 004C 0065 0066 0074 0020 0041 0072 0072 006F 0077 | 0x431398 | 0x4A75A |
| 124 | 205 / 0x000000CD | "Right Arrow" | 11 | 0052 0069 0067 0068 0074 0020 0041 0072 0072 006F 0077 | 0x4313B0 | 0x4A789 |
| 125 | 207 / 0x000000CF | "End" | 3 | 0045 006E 0064 | 0x4313C8 | 0x4A7B8 |
| 126 | 208 / 0x000000D0 | "Down Arrow" | 10 | 0044 006F 0077 006E 0020 0041 0072 0072 006F 0077 | 0x4313D0 | 0x4A7E7 |
| 127 | 209 / 0x000000D1 | "Page Down" | 9 | 0050 0061 0067 0065 0020 0044 006F 0077 006E | 0x4313E8 | 0x4A816 |
| 128 | 210 / 0x000000D2 | "Insert" | 6 | 0049 006E 0073 0065 0072 0074 | 0x431400 | 0x4A845 |
| 129 | 211 / 0x000000D3 | "Delete" | 6 | 0044 0065 006C 0065 0074 0065 | 0x431410 | 0x4A874 |
| 130 | 219 / 0x000000DB | "Windows Left" | 12 | 0057 0069 006E 0064 006F 0077 0073 0020 004C 0065 0066 0074 | 0x431420 | 0x4A8A3 |
| 131 | 220 / 0x000000DC | "Windows Right" | 13 | 0057 0069 006E 0064 006F 0077 0073 0020 0052 0069 0067 0068 0074 | 0x431440 | 0x4A8D2 |
| 132 | 221 / 0x000000DD | "App Menu" | 8 | 0041 0070 0070 0020 004D 0065 006E 0075 | 0x431460 | 0x4A901 |
| 133 | 222 / 0x000000DE | "System Power" | 12 | 0053 0079 0073 0074 0065 006D 0020 0050 006F 0077 0065 0072 | 0x431478 | 0x4A930 |
| 134 | 223 / 0x000000DF | "System Sleep" | 12 | 0053 0079 0073 0074 0065 006D 0020 0053 006C 0065 0065 0070 | 0x431498 | 0x4A95F |
| 135 | 227 / 0x000000E3 | "System Wake" | 11 | 0053 0079 0073 0074 0065 006D 0020 0057 0061 006B 0065 | 0x4314B8 | 0x4A98E |
| 136 | 229 / 0x000000E5 | "Web Search" | 10 | 0057 0065 0062 0020 0053 0065 0061 0072 0063 0068 | 0x4314D0 | 0x4A9BD |
| 137 | 230 / 0x000000E6 | "Web Favorites" | 13 | 0057 0065 0062 0020 0046 0061 0076 006F 0072 0069 0074 0065 0073 | 0x4314E8 | 0x4A9EC |
| 138 | 231 / 0x000000E7 | "Web Refresh" | 11 | 0057 0065 0062 0020 0052 0065 0066 0072 0065 0073 0068 | 0x431508 | 0x4AA1B |
| 139 | 232 / 0x000000E8 | "Web Stop" | 8 | 0057 0065 0062 0020 0053 0074 006F 0070 | 0x431520 | 0x4AA4A |
| 140 | 233 / 0x000000E9 | "Web Forward" | 11 | 0057 0065 0062 0020 0046 006F 0072 0077 0061 0072 0064 | 0x431538 | 0x4AA79 |
| 141 | 234 / 0x000000EA | "Web Back" | 8 | 0057 0065 0062 0020 0042 0061 0063 006B | 0x431550 | 0x4AAA8 |
| 142 | 235 / 0x000000EB | "My Computer" | 11 | 004D 0079 0020 0043 006F 006D 0070 0075 0074 0065 0072 | 0x431568 | 0x4AAD7 |
| 143 | 236 / 0x000000EC | "Mail" | 4 | 004D 0061 0069 006C | 0x431580 | 0x4AB06 |
| 144 | 237 / 0x000000ED | "Media Select" | 12 | 004D 0065 0064 0069 0061 0020 0053 0065 006C 0065 0063 0074 | 0x431590 | 0x4AB35 |

Detailed formatter, complete ordered behavior

RVA 0x4C830 receives D, a fresh output string object, and a pointer B to an eight-byte binding record. B+0 is a 32-bit code; B+4 is a 32-bit category. The output string representation has two-byte elements, an eight-byte length at +0x10 and eight-byte capacity at +0x18. Capacity below 8 selects the inline units; otherwise the first eight bytes hold the data pointer. Each formatter return path establishes capacity 7, length 0 and a zero first word before the selected string construction or move. No whole-object or padding zeroing is proved.

At entry the formatter reads B+0 into a retained 32-bit register before reading any category. If that initial code is zero, it reads the category for a zero comparison. If that category is also zero, it constructs the exact ten-unit literal "Unassigned" at 0x4317E8 and returns the output object. It does not proceed to the other category comparison or dictionary lookup. A nonzero initial code skips this first category read.

On every remaining path it reads B+4 again for comparison with 1. Category 1 selects the stream path. All other 32-bit category words select the first dictionary, including categories other than zero and negative categories. The formatter does not validate a category enumeration or restrict the non-1 route to a supposed keyboard category.

On the category-1 path, original instructions first construct a local wide stream using imported MSVCP140 initialization operations. After that initialization they reread B+0 as a 32-bit code at 0x4C937. This later code is retained across the prefix insertion. The original prefix is "Mouse ", six UTF-16 units, at 0x431800. The formatter inserts that prefix through 0x486B0, then forms the numeric argument by adding one in a 32-bit destination at 0x4C94D and passes it to the imported signed-int stream insertion at 0x4C950. There is no code-range test before this increment. The native operation wraps modulo 2 to the power 32: original code -1 produces argument 0; 2147483647 produces -2147483648; -2147483648 produces -2147483647. This is an instruction-level width fact, without an inference about a source-language overflow rule.

Prefix insertion precedes signed-int insertion; the second code read precedes both. The exact imported numeric formatting, locale state, error handling and possible internal callbacks have not been traced. After the insertion, 0x415A0 calls 0x414A0 on the stream's buffer subobject to obtain the output string. The native stream cleanup at 0x40600 and the imported wide-ios destruction occur before the formatter returns the output pointer. Failure, allocation, exception and imported destructor behavior do not establish an unconditional successful result.

The complete 0x486B0 prefix helper measures the supplied UTF-16 prefix to its zero word, obtains stream width/state, admits the stream operation through the imported prefix/state machinery, and performs the selected padding and buffer write operations. It sets stream error state on failed selected buffer writes and performs the original output suffix work. It does not supply a different key-name lookup. The complete 0x414A0 extraction selects buffer spans from the stream's flags and indirect current pointers: its first path requires flag bit 2 clear and a nonzero pointed current put position, and uses the larger of that position and the saved buffer endpoint; its alternative requires flag bit 4 clear and a nonzero pointed get position and adds twice a sign-extended 32-bit remaining count. The selected start/end byte difference is arithmetically shifted right by one for the UTF-16 count passed to 0x3D4F0. If neither span is admitted, extraction constructs an empty temporary. It then transfers that string to the fresh output through 0x3BF00. A successful mouse-category route can therefore depend on stream state; the original formatter does not replace an extracted empty result with "Unknown" or "Unassigned".

On the dictionary path, the search uses the initial code retained at formatter entry, not a second code read. It reads the first head at D+0, starts from the head's +8 root link, tests sentinel byte +0x19, and uses signed comparisons against each node's +0x20 key. It finds the lower-bound candidate and retains it only for an exact key; otherwise it selects the head as the miss indication. A miss constructs "Unknown", seven units, from 0x431810. An exact hit copies the current mapped string at node+0x28 with 0x3D3B0, starting at zero and requesting all remaining units with the all-ones 64-bit count. Lookup neither inserts a missing key nor invokes an OS/device naming fallback. A valid mapped empty string would copy as empty, rather than becoming "Unknown".

The record is not read as one atomic eight-byte snapshot. For initial code zero, the first zero-category admission may precede a different category value at the later comparison. If the first category read is zero, the immediate "Unassigned" return wins. If it is nonzero, the later category read selects the next route. For category 1, the later code read follows stream initialization; for every other category the initial code remains the lookup key. The selected original code supplies no lock or retry around these reads.

The counted assignment at 0x3D4F0 preserves the exact count, grows capacity through the selected original allocation path when necessary, copies twice that count in bytes and writes a zero word after it. It rejects excessive counts through the original length-error route. Its overlap path delegates a substring operation to 0x3D3B0. The complete copy and move helpers retain the native inline/heap distinction and length/capacity widths; a heap transfer clears the source pointer before resetting the source to its empty inline state. Their original allocator, memory-runtime and error providers remain boundaries. The ordinary formatter pattern supplies fresh outputs and well-formed strings; no safe behavior for arbitrary live output aliases, null bindings, corrupted heads, invalid capacities or malformed buffer spans is asserted.

The separately checked compact formatter at 0x4CA10 searches D+0x10 instead of D+0. Its zero/zero and miss literal is the single unit "?" at 0x431820, and its category-1 prefix is the single unit "M" at 0x430ECC. Its other receiver must not silently replace the detailed dictionary above. This contract independently inspected that complete owner to establish the distinction; it does not import or reproduce an earlier compact-dictionary handoff.

Concrete Controls construction and current binding ownership

Let I denote the interface object constructed by 0xF8690 at G+0x118. That complete constructor stores G as the eight-byte pointer I+0x5C0. In the selected setup at 0xF9AC0, call site 0xFCA5E constructs a 0x2E8-byte receiver C through 0x275A70, passing that same G and a selected cloned root. The result is published at I+0x540, equivalently G+0x658. Original RTTI reached from C's installed primary table 0x46E468 identifies ControlsWidget. The table's +8 slot points to 0x276200. This is concrete image construction/table evidence, not an identity guessed from a field offset.

C+0x1E8 stores G. C+0x1F0 through C+0x2A7 hold 23 eight-byte pending binding records. The complete constructor calls 0x41DD0 on this pending array, sets 32-bit scroll C+0x2D8 to zero, and uses one eight-byte all-ones store at C+0x2DC to establish both 32-bit candidate C+0x2DC and capture C+0x2E0 as -1. Optional root pointers are retained at C+0x2C0, +0x2C8 and +0x2D0.

The original base construction through 0x268A70 and 0x328FF0 retains the GUI manager at receiver+8 and the supplied root at +0x178. The selected attachment path 0x334410, ending in 0x32D190, establishes the reciprocal root+0x70 widget pointer and widget+0x178 root pointer for a nonnull root. Existing root attachments have their selected layout state transferred, are passed with the manager to 0x33E0C0, and then have the root's +0x70 pointer cleared before the new attachment join; the internals of 0x33E0C0 remain outside this contract. This does not establish a general ownership or callback-safety guarantee for arbitrary replacement or deletion during a draw.

The constructor requests the resource named "resource1.dat" through 0x269130 and the selected manager/resource provider; its result is stored at C+0x1A8. Asset contents, font availability and resulting glyph geometry were not read. At the setup caller, the selected size helper receives binary32 500 and 600 and a byte value 1; the following 0xA3A20 invocation writes zero to the current selected integer of the new root. The latter helper resolves node+0x68, sign-extends its index+0xAC, and stores a 32-bit zero or one into its array+0xE0 according to the supplied byte. External GUI traversal and subsequent root admissions determine whether the draw slot is called; the Controls draw body itself does not first retest this selected integer.

C has three separately cloned button nodes: C+0x2B8, +0x2A8 and +0x2B0. In that order, construction sends the literal captions "Defaults" (8 units), "OK" (2 units, at 0x46E5EC) and "Cancel" (6 units) through 0x334C50. It then registers event-number 3 callbacks on the corresponding node+0x70 receivers, with C as the callback receiver: 0x276A30, 0x276C80 and the native target at 0x276A00 respectively. This contract closes the first two reached callback bodies and does not invent a separate decoded body for 0x276A00.

The complete caption helper 0x334C50 first requires bit 2 of the node's 32-bit flags+0x80 to be clear. A node with nonnull +0x58 receiver and that receiver's 32-bit +0x70 equal to 3 selects a string at receiver array+0x108 with signed 32-bit index+0xD4 and stride 0x20. It compares the current and supplied text through the complete 0x89470 helper, which compares unsigned 16-bit units in order and then the unsigned 64-bit lengths; only equal lengths with equal units return zero. There is no case folding or locale conversion in that selected comparison. It copies the whole supplied text only if they differ and the string objects are distinct. It then traverses the node's current child list in list order and applies the same operation recursively. The flag gate prevents both the local change and this recursive descent. Therefore button caption publication is conditional on the actual constructed node/property tree; asset-dependent descendants were not inspected. This is distinct from the row binding-text draw calls below. The related complete named helper 0x3348E0 compares the current node name, can delegate recursively to this caption helper when its last byte is nonzero, and otherwise follows its selected single-text receiver/update route; it is not called to install each row's formatted binding text in 0x276200.

The complete 0x41DD0 initializer clears exactly 23 consecutive eight-byte records, then calls 0x43760. The latter writes whole eight-byte code/category pairs in the native index order 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3, 13, 2, 14, 15, 16, 17, 20, 18, 19. Rows 21 and 22 retain the cleared pair. Game construction also calls this initializer at 0x7ED12 on G+0x1BDC. That Game array is the active 23-record array, distinct from C's pending array. These are constructor/default assignments; they do not prove that later ordinary opens or a configured installation retain those initial values.

RVA 0x42670 supplies the following row labels. The 23 original jump-table entries at 0x429FC..0x42A57 were checked directly and matched to their owning branch destinations and original literals. Its unsigned index admission is 0 through 22 inclusive; other 32-bit indices use its separate "Unknown " plus numeric-helper route. The ordinary clamped Controls loop does not reach that fallback. The last two columns give the initializer values just established, without treating them as a snapshot of a later draw.


| Row | Label | Length | Literal RVA | Initial code | Initial category |
|---:|---|---:|---|---:|---:|
| 0 | "Normal Attack" | 13 | 0x4303B8 | 0 | 1 |
| 1 | "Special Attack" | 14 | 0x4303D8 | 1 | 1 |
| 2 | "Class Skill" | 11 | 0x4304D8 | 42 | 0 |
| 3 | "Ultimate Skill" | 14 | 0x4304B8 | 19 | 0 |
| 4 | "Use Quick Item" | 14 | 0x430530 | 16 | 0 |
| 5 | "Move Forward" | 12 | 0x4303F8 | 17 | 0 |
| 6 | "Move Backward" | 13 | 0x430418 | 31 | 0 |
| 7 | "Move Left" | 9 | 0x430438 | 30 | 0 |
| 8 | "Move Right" | 10 | 0x430450 | 32 | 0 |
| 9 | "Jump" | 4 | 0x430468 | 57 | 0 |
| 10 | "Dodge" | 5 | 0x430478 | 2 | 1 |
| 11 | "Use/Interact" | 12 | 0x430488 | 18 | 0 |
| 12 | "Lamp" | 4 | 0x4304A8 | 33 | 0 |
| 13 | "Walk" | 4 | 0x4304F0 | 29 | 0 |
| 14 | "Call Pet" | 8 | 0x430500 | 20 | 0 |
| 15 | "World Map" | 9 | 0x430518 | 50 | 0 |
| 16 | "Ring Menu" | 9 | 0x430550 | 15 | 0 |
| 17 | "Inventory" | 9 | 0x430568 | 48 | 0 |
| 18 | "Crafting Menu" | 13 | 0x4305A8 | 46 | 0 |
| 19 | "System Menu" | 11 | 0x4305C8 | 24 | 0 |
| 20 | "Multiplayer Menu" | 16 | 0x430580 | 36 | 0 |
| 21 | "Zoom In" | 7 | 0x4305E0 | 0 | 0 |
| 22 | "Zoom Out" | 8 | 0x4305F0 | 0 | 0 |

Pending changes and active changes

On invocation, 0x276A30 builds a separate default temporary with 0x41DD0 and copies all 23 pending records to C+0x1F0 in increasing destination order. Native copies use consecutive 16-byte blocks for the first 22 records and a final eight-byte record. This callback does not itself format or draw a row, change the detailed dictionary, or publish the pending array to G's active array. It does not reset the capture or candidate fields in this selected body.

The complete capture-record helper 0x2769A0 admits only a capture index whose 32-bit unsigned value is at most 22. It visits all 23 pending records in increasing index order, reading the supplied category before comparing a row's category; only on a category match does it read the supplied code and compare the row's code. Matching rows receive an eight-byte zero store. After the pass, it rereads the incoming eight-byte record, sign-extends a fresh C+0x2E0 index, stores that record to the selected pending slot, and sets only candidate C+0x2DC to -1. It leaves capture C+0x2E0 unchanged. Invalid initial indices leave these fields unchanged. The original Game adapter at 0x8A120 obtains C through G+0x658 and jumps to this helper, retaining the incoming record argument in the native argument register. Input/device admission above that adapter is outside this text-publication contract. There is no formatter call inside this helper. If an unusual caller aliases the incoming record into the pending array, the comparison/clearing pass is not a preserved incoming snapshot.

On invocation, 0x276F30 copies the current 32-bit candidate to the 32-bit capture field. The complete 0x2761F0 writes capture -1. These effects are stated at the selected helpers' entry boundary; no full input or universal caller census for capture admission is claimed here.

The event-3 callback 0x276C80 first calls 0x89B80 with G and C+0x1F0. The native 0x89B80 writes eleven consecutive 16-byte blocks and one final eight-byte block, in increasing offset order, into active G+0x1BDC..G+0x1C93. It then jumps to 0x438E0 on that active array. The complete original 0x438E0 routes the current 32-bit code/category fields to a narrow output stream with the literal filename "controls.cfg"; it does not edit either name dictionary. This is a source literal and call route, not a reading of any installed configuration. Provider open/write/flush/close success, filesystem ordering and recovery are not proved. Only after this route returns does 0x276C80 reread C+0x178, its +0x68 property receiver, signed index+0xAC and array+0xE0, then store a 32-bit zero to that selected integer. There is no atomic all-record publication, rollback, success test or dictionary-name regeneration in these complete selected bodies. The callback does not report a status. Imported failures or exceptions are not a guarantee that the final root store occurs.

The callback registration helper 0x2758B0 looks up the signed event number in the selected receiver's dictionary at +0x188, creates an entry when absent, frees a previous stored callback allocation when present, and allocates a 0x28-byte callback object retaining the supplied receiver and 16-byte member-call representation. The selected byte controls recursive registration through its obtained descendant list. Constructor-local callback descriptions are thus copied into registrations; their later dispatch, C's destruction, arbitrary replacement during a callback, and imported allocation failure remain separate lifetime boundaries. No ownership claim that the registration keeps Game or C alive is inferred from retaining those pointers.

Controls draw and binding-text publication

The complete Controls draw owner 0x276200 first obtains current corner outputs for the three button nodes in order C+0x2B8, C+0x2A8 and C+0x2B0 through 0x332AD0. Each pair's binary32 coordinates are added, multiplied by binary32 0.5, and converted to signed 32-bit integers. Each resulting pair is appended to G's list at +0x6C8, with eight-byte count +0x6D0. After staging each node through 0x6ECE0, the owner subtracts the current count from unsigned 64-bit 0x0AAAAAAAAAAAAAA9 and requires the resulting unsigned difference to be at least one before incrementing the count and publishing the links. With an ordinary count at or below that limit, equality takes the length-error/trap branch. This branch precedes all row text work; no malformed-count recovery is inferred. The matrix/corner helper closure and allocation provider are not a universal screen-geometry or lifetime guarantee.

It next queries current height with 0x329E00. From that binary32 result it subtracts 120, converts toward zero to a signed 32-bit integer, performs signed division by 30, and adds the 32-bit scroll value C+0x2D8. The result is the exclusive row end after clamping to 0..23; the independently clamped scroll is the row start. The 32-bit addition retains its native width. The finite, representable conversion domain supports the ordinary row description; out-of-range or unordered conversions and unmasked floating exceptions are not supplied a guessed language-level result. The width/height wrappers return the first/second binary32 result of the complete local dimension owner 0x32A110. That owner reads the widget's current +0x70..+0x9C fields and performs its native comparisons/additions/subtractions; these are current widget dimensions, not a newly read OS screen rectangle.

After obtaining a local point through 0x32B180, the draw writes candidate -1, installs the four-component color at 0x42E8C0, installs the second color vector at 0x42E660, sets binary32 style size 12 and the binary32 auxiliary style value 3. Their scalar words are 0x41400000 and 0x40400000. The first color vector is four 0x3F800000 words; the second is three zero words followed by 0x3F800000. It then reads the current byte G+0x1958; if nonzero it clears capture to -1. No gameplay or device meaning is assigned to that byte in this contract.

The local-point helper first obtains the selected root matrix through 0x100EE0, samples current manager binary32 coordinates at +0x180/+0x184, obtains the widget position through 0x32CF50, subtracts that position, and applies the selected matrix components. Those matrix/position helpers and earlier point producers remain explicit dependencies. The Controls loop uses that returned point. It does not itself request current device data.

Rows are visited in increasing index order from the clamped start to the exclusive end. The first visited row uses baseline binary32 27, word 0x41D80000 from 0x46DEB8, even when the first index is not zero. After each visited row, baseline is increased by binary32 30, then the row index and pending-record pointer advance. It is not baseline 27 plus an absolute row-index offset.

At each row entry the current capture value is loaded and compared with the row index. For the matching row the draw installs the four words at 0x46E640, sets 32-bit text-style flags to 0 through 0x269190, constructs the row label through 0x42670, and submits it to 0x269210 at X=20 and the current baseline. It disposes that label, sets the style flags to 1, constructs the exact eleven-unit literal "Press a key" from 0x46E5F8, freshly queries the width, and submits the prompt at the same baseline. No call to the detailed formatter is made for this captured row.

For every other visited row, it starts with the base color. It checks the pending code for zero before checking its category for zero; when both are zero it selects 0x46E630. Its hover admission uses the capture value already loaded at this row entry, requiring it negative, then rereads G+0x1958 and requires zero. It requires local X at least zero, then makes a current width call and requires local X strictly less than width minus 40. It requires local Y at least baseline minus 20 and strictly less than baseline plus 10. If all tests pass it stores this row index as candidate and selects vector 0x4381C0. The native comparisons reject unordered operands along these point-admission branches if the comparison completes; no NaN point is silently admitted. A callback changing a previously tested field does not cause that row branch to restart.

The draw then installs the selected color, sets style flags 0, creates and draws that row's label at X=20, and destroys the label. It next sets style flags 1, reloads G from C+0x1E8, and calls the detailed formatter at 0x27677F with D=G+0x30 and the pending record C+0x1F0 plus eight bytes per current row index. After the formatter returns, it retains the returned temporary pointer, makes a fresh width query, and draws that formatted text at the same baseline. It frees a heap-backed temporary after the draw. It does not first substitute G's active binding array, the compact dictionary or a cached constructor caption.

For both the capture prompt and ordinary binding text, right X is produced by subtracting binary32 250 from the fresh width, multiplying that result by binary32 0.5, then adding binary32 240. Those three operations are ordered separately; this contract does not replace them with a reassociated expression. The constants are at 0x43293C, 0x4AC964 and 0x432D94, words 0x437A0000, 0x3F000000 and 0x43700000 respectively. The style-flag change is a native input to drawing; no complete font alignment semantics are inferred merely from the values 0 and 1.

This order matters when original providers or callbacks change state. The label draw occurs before the formatter's binding/category reads. The formatter completes before the subsequent width sample and the binding-text draw. The next row samples its capture again. The selected loop does not cache all 23 labels or all 23 binding strings together, and it does not revalidate its original clamped loop extent after each callback. The Game receiver is reloaded at each ordinary binding-format call; no whole-Game or whole-array coherent snapshot is established. The selected ordinary body contains no persistent caption assignment for these row strings.

After all rows, the owner separately handles each nonnull optional node C+0x2C0, +0x2C8 and +0x2D0, in that order. It calls 0x32B0C0 on the corresponding node+0x70 receiver and selects either the base vector or 0x4381B0 according to the returned byte. It writes four 32-bit components to the current node+0x68 property receiver's array+0x1F0 at signed index+0x1BC and stride 16. These are reached vector stores, not binding-text stores. The complete 0x32B0C0 is an adapter to 0x32AE90 with its receiver's current root; that query's transitive hit/geometry closure is not reinterpreted as a generic rectangle test.

For exact color distinctions, 0x46E630 contains words 3E4CCCCD 3F800000 3E4CCCCD 3F800000; 0x46E640 contains 3F800000 3EB33333 3EB33333 3F800000; 0x4381C0 contains 3E4CCCCD 3F800000 3F800000 3F800000; and 0x4381B0 contains 00000000 3F800000 3F800000 3F800000. These are original binary32 words, with no decimal-rounding substitution.

What text publication reaches

The complete 0x269210 text bridge checks receiver+0x1A8 first. A null resource writes two binary32 zero outputs and returns without its manager/text-resource draw route. For a nonnull resource it enters the selected manager/root setup calls, obtains the current manager matrix, and reads current style and root-transform inputs. Supplied X/Y and the selected root offsets are combined and scaled in the original binary32 order before resource drawing. The ordinary style value 3 set by the Controls owner takes the two-pass route: the first selected 0x34FA60 call uses the auxiliary style and zero color inputs; the following call uses zero auxiliary style and the current color inputs. The first returned pair is retained for the output conversion. The other admitted style route has a single resource call. The native wrapper completes its manager/root closing call, rereads the root's selected offset inputs, and converts that retained pair for its caller.

The complete 0x34FA60 takes the selected text resource's critical section at resource+0x168. It obtains the current manager matrix, configures the resource through 0x354600, and supplies the selected text to 0x34FE40. Its flag-bit-0x10 route first copies the text and calls 0x354F20 for wrapping, then draws that copy and disposes it. The other route supplies the original string. After the selected output pair is copied back, it restores the saved manager matrix and leaves that critical section on the normal return path. This lock begins after binding formatting and does not protect the dictionary, the binding records, or the enclosing row traversal.

The complete original 0x34FE40 was read and byte-matched to close the immediate publication boundary. It obtains original metric results, iterates selected 16-bit text units, handles its line-break path separately, obtains current glyph data or the selected stored glyph entry, and invokes its selected resource/manager draw paths when the required returned glyph resources are present. It also uses original kerning providers and current style/matrix data and returns an advanced coordinate pair. It does not install these Controls row strings into a node's selected caption array. Font configuration, glyph lookup, kerning, wrapped-layout and imported renderer internals remain unclosed numerical/provider dependencies. The evidence establishes supplied strings, admission and call order; it does not guarantee visible pixels, glyph presence, a deterministic font/locale, a particular width, an exception-safe unlock, or retention behavior inside an imported provider.

The temporary strings remain available through their immediate draw calls and are disposed afterward on normal returns. Neither this caller pattern nor raw pointer retention proves a general lifetime guarantee if arbitrary callbacks replace C, G, their root, resource, dictionary, child lists or property arrays while those objects are still in use. No such replacement or cross-thread safety is presumed.

Separate Help consumer and detailed-formatter caller census

The selected original direct-call metadata contains exactly eleven calls to 0x4C830: one in Controls draw at 0x27677F and ten in the complete owner 0x278040, at 0x27817A, 0x278502, 0x27859E, 0x278A0F, 0x2790CC, 0x279265, 0x2793F2, 0x27957D, 0x279809 and 0x279994. This is a census of the selected original export's resolved direct calls, not a claim about every indirect, unrecognized or alternative native caller.

At setup call site 0xFF76A, 0x277EC0 constructs a separate 0x1F0-byte receiver H with the same G and a root supplied by 0x33CD30. I+0x278, equivalently G+0x390, retains H. Its original installed table 0x46E8A8 and RTTI identify HelpWidget; its +8 slot is 0x278040. The constructor stores G at H+0x1E8, requests the same "resource1.dat" name, and the caller sets the selected root integer to zero through 0xA3A20. Help is not an alias for the Controls pending array. The complete Help draw owner has no preliminary internal selected-root or key-input visibility check; its invocation by the surrounding GUI remains an external admission boundary.

Help prepares its current parent/root matrix and style, including size 12, auxiliary value 3 and the second color vector, then samples G+0x1A72 as a byte and passes the current G+8 owner's entity at +0x448 with that byte to 0x5FC40. Call the returned 32-bit value A. The selected entity/provider internals are outside this binding-text contract. The first binding string is formatted from active record 0 at G+0x1BDC and drawn bare at Y=0. Only afterward does the owner obtain/draw its separate descriptive text and optional annotation. Imported initialization guards also establish three selected color globals in this owner; their provider synchronization does not constitute a binding-array lock.

The next binding row, at Y=20, uses active record 1 at G+0x1BE4. The native unsigned 32-bit subtraction/range test and mask 0x880000000B0001 select exactly A values 9, 25, 26, 28, 60 and 64 for the prefix "HOLD " from 0x46EA38, five units. That branch uses the format call at 0x278502. Every other A takes the bare-format call at 0x27859E and joins the same following description work. The saved byte G+0x1A72 is also supplied to a separate current-entity query at 0x60120; its result controls surrounding descriptive/annotation text, not a different name dictionary.

The third binding row, Y=40, uses active record 2 at G+0x1BEC and always prepends "Hold " from 0x46EA78, five units, with its different case preserved. Its following descriptive text has branches on current entity bytes +0x14C and +0x14D; those branches do not select a different binding record for this row.

The row at Y=60 uses active record 10 at G+0x1C2C and prepends the seven units "Move + " at 0x46EAA0. The owner then has a conditional extra binding row at Y=80, using that same active record 10 with "Tap " from 0x46EAB0, four units. That extra row is admitted for current entity byte +0x14C equal to 1, 2 or 3, and also for value 4 when current byte +0x14D is zero. The four corresponding format-call sites are 0x279265, 0x2793F2, 0x27957D and 0x279809. Its byte-dependent descriptive strings and numeric annotations are distinct provider/literal work; no action meaning is inferred from these byte values.

The final binding row uses active record 14 at G+0x1C4C, bare, through 0x279994. Its Y is 100 when one of those extra rows was admitted and 80 otherwise. The following separate literal is "Call/Ride Pet". Each admitted binding row is submitted before its associated descriptive text and the original temporary disposal. No path in this owner commits Controls pending records, changes a dictionary entry, or copies its formatted binding output into the persistent node caption helper.

Before each selected Help format call, the native owner freshly obtains G from H+0x1E8 and derives both D and that row's active-record address from that one obtained G. It does not retain one full binding-array snapshot for all rows. The intervening descriptive/text-resource/provider calls can precede later Game/entity rereads. The 0x36690 prefix helper measures each original UTF-16 prefix, inserts it at position zero into the formatted temporary through 0x3E110, initializes a fresh result and transfers the combined string through 0x3BF00. It does not trim spaces, change case, query another dictionary or reread the binding. Its original overlapping-string/growth/error branches retain separate provider limits. No inference about Help dismissal, gameplay controls or frame timing follows from this draw-only consumer.

Independent conditional examples

The following are source-only conditional cases, not observations from an executing original or an implementation. They assume the completed first dictionary above, well-formed fresh output strings and normal provider returns. Where decimal spelling is given for a category-1 result, ordinary signed decimal insertion is an additional explicit provider condition.

A record with code 1 and category 0 returns "Escape". The same code with category -7 still uses the dictionary and returns "Escape". Code 85 with category 0 is a constructor-table miss and returns "Unknown"; it is not inferred from a neighboring key or an OS name. Code zero and category zero returns "Unassigned"; zero and category 2 instead searches key zero and returns "Unknown". Zero and category 1 supplies numeric argument 1 after prefix "Mouse ", conditionally giving "Mouse 1". Code -1 and category 1 supplies numeric argument 0; code 2147483647 and category 1 supplies -2147483648. A category-0 code -1 is instead a dictionary miss. Changing the category between the two zero-code category reads can change the selected later route only if the first read did not already return "Unassigned".

For a finite Controls dimension result height 240, scroll 1 and ordinary signed arithmetic, the loop's exclusive end is 5 and the first visited row is 1. Its baseline is 27; the later visited rows 2, 3 and 4 use baselines 57, 87 and 117. With fresh width 500, the right-side operation sequence produces X=365. For a normal noncaptured row at baseline 27, local point X=0,Y=7 passes the inclusive lower point bounds when width permits it; Y=37 fails the strict upper bound. X equal to width minus 40 also fails. An active captured row draws "Press a key" instead of invoking 0x4C830 for that row, independent of the pending code/category it would otherwise format. A null C+0x1A8 can still allow label/formatter construction in the caller but makes the immediate 0x269210 submission return zero outputs without its text-resource draw path.

For Help with provider result A=25, active record 1 receives prefix "HOLD "; A=24 receives the bare detailed result. An entity +0x14C byte of 4 and +0x14D byte of 1 omits the extra "Tap " row and places the final active-record-14 row at Y=80; the same first byte with the second byte zero admits the extra row and places the final row at Y=100. These examples describe native branch inputs, not names for the entity-byte categories or guarantees about the untraced entity providers.



Bounded ownership, writers and provider limits

The complete constructor fixes the first dictionary's initial population. Neither detailed formatter nor either inspected draw consumer writes those mapped names. Mouse numeric text is built outside that table by a formatter branch; it is not a set of generated constructor entries. The generic insertion wrapper also has resolved direct calls in the original owners 0x1510F0 and 0x15CFF0. The latter merely returns the mapped-string address after invoking that wrapper, and its resolved callers in the selected metadata are in 0x1510F0. Those generic helper references do not by themselves prove an alias to ordinary G+0x30. This audit does not claim a universal later-writer census, permanent immutability, a single process-global dictionary, or complete destruction/reconfiguration ownership. Another receiver or an untraced indirect writer remains possible.

The complete Controls constructor, draw, selected default/capture/apply helpers and immediate text/caption bridges establish the publication described here. This is not the whole Controls input system or an inventory of every writer to pending or active bindings. Ordinary menu-opening copies, device sampling, all focus/capture/message admissions, other configuration routes and the undecoded 0x276A00 target remain outside the selected behavioral closure. No key sequence is manufactured to reach a callback. The surrounding GUI visibility, clipping, z-order, pick/update/draw scheduling, imported render resources, asset-defined node/property availability and full general geometry closure remain unresolved. A callback table association proves the stored target and receiver, not that every root or input route invokes it.

The additional Help owner is included because it is a concretely resolved detailed-formatter consumer. Its reached binding-row selection/prefix/order is closed here; its descriptive localization, entity-query arithmetic, help-state transitions and gameplay meanings are not. All row positions above are the selected binary32 arguments supplied to the original widget text bridge, not a claim that the same numbers are physical screen pixels. The full metric, glyph, font and wrapping closures are not derived by this contract.

There is no explicit dictionary or pending/active-array lock in the selected formatting and publication paths. The resource critical section is narrower than those operations. Imported streams, font systems, allocators, critical-section implementations, runtime guards and virtual manager callbacks may have their own internals, which were not inspected. The native call/read order supports conditional reentrant or interleaved outcomes only to the extent stated; it does not guarantee that a callback occurs, that an external thread runs at a particular point, or that unsynchronized mutation has a defined safe lifetime. No wall-clock, OS device-layout, locale, font, atomic-copy, scheduler or provider-success guarantee is inferred.

Behavior is stated for correctly constructed original receivers and normal returns where necessary. Corrupted sentinels, null receiver arguments, arbitrary string aliases, invalid signed indices outside their selected admission, malformed object lifetimes and unavailable providers are not repaired by an invented fallback. Error calls, cleanup calls and branch destinations are source facts; untraced imported exceptions, aborts and recovery are explicitly outside the contract.

Evidence manifest and correspondence

The standalone policy read for this assignment was C:/Users/DieuDeGlace/Documents/Cubeworld-Vulkanengine-Steam/docs/CLEANROOM_AUTHOR.md. Original evidence resides only beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Filenames were identified before content searches. The selected C presentations are under decompiled/functions, named by each listed full virtual entry address: preferred base plus the owner RVA, as in 140049080.c for 0x49080. The full original first constructor, detailed and compact formatter owners and selected original helper/consumer bodies were read; their relevant C presentation was reconciled with native operands rather than used to assign guessed semantics.

The large Game construction and interface-setup owners are expressly bounded context: 0x7EA80 is used for its dictionary/interface/active-array constructor joins; 0xF9AC0 for the Controls and Help construction, publication, size-input and initial selected-integer joins. No interpretation of the whole of those large owners is claimed. The separately byte-checked context owner 0x103200 supplies no additional detailed-formatter caller or name-table mutation claim in this contract. Complete raw instruction correspondence for each listed exported range was nevertheless checked. This distinction prevents local setup facts from becoming a universal Game/UI lifecycle claim.

The selected metadata sources are decompiled/functions.csv for owners/files and sizes, decompiled/function_ranges.csv for inclusive native extents, decompiled/calls.csv for the bounded resolved-direct-call census, and decompiled/assembly.txt for instruction addresses, encodings and operands. The checker mapped each selected instruction's RVA through the original PE section table and compared every encoded byte with the original image. It also required exact adjacency of the instructions within every separately exported range, with the last instruction ending at the inclusive range end. All 71 listed owners match: 19,238 instructions and 90,632 instruction bytes, zero mismatches and zero internal range gaps. Separate exported ranges are listed separately; intervening bytes are not silently counted or assigned to an owner. This is full correspondence for these selected exports, not proof that every possible undiscovered entry or indirect edge in the executable is represented.


| Original export file | Bytes | SHA256 |
|---|---:|---|
| decompiled/functions.csv | 1551545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |
| decompiled/function_ranges.csv | 509127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| decompiled/calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| decompiled/assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |


| Native owner RVA | Inclusive instruction ranges, RVA | Instructions | Matched bytes | Scope |
|---|---|---:|---:|---|
| 0x334F0 | 0x334F0..0x33573; 0x33588..0x33607 | 78 | 260 | Complete selected owner |
| 0x33B10 | 0x33B10..0x33B28; 0x33B2A..0x33B40; 0x33B42..0x33B50; 0x33B52..0x33B60; 0x33B62..0x33B6D; 0x33B6F..0x33B7D | 26 | 105 | Complete selected owner |
| 0x36690 | 0x36690..0x366F2 | 30 | 99 | Complete selected owner |
| 0x3BF00 | 0x3BF00..0x3BF8C | 41 | 141 | Complete selected owner |
| 0x3D3B0 | 0x3D3B0..0x3D4E1 | 88 | 306 | Complete selected owner |
| 0x3D4F0 | 0x3D4F0..0x3D634 | 100 | 325 | Complete selected owner |
| 0x3E110 | 0x3E110..0x3E2C2 | 127 | 435 | Complete selected owner |
| 0x40600 | 0x40600..0x40673 | 23 | 116 | Complete selected owner |
| 0x414A0 | 0x414A0..0x41598 | 65 | 249 | Complete selected owner |
| 0x415A0 | 0x415A0..0x415C2 | 10 | 35 | Complete selected owner |
| 0x41D80 | 0x41D80..0x41DC8 | 19 | 73 | Complete selected owner |
| 0x41DD0 | 0x41DD0..0x41E00 | 16 | 49 | Complete selected owner |
| 0x42670 | 0x42670..0x429FA | 186 | 907 | Complete selected owner |
| 0x43760 | 0x43760..0x438D4 | 71 | 373 | Complete selected owner |
| 0x438E0 | 0x438E0..0x4401F | 377 | 1856 | Complete selected owner |
| 0x486B0 | 0x486B0..0x48816; 0x48825..0x48884 | 138 | 455 | Complete selected owner |
| 0x48890 | 0x48890..0x488EB | 25 | 92 | Complete selected owner |
| 0x488F0 | 0x488F0..0x48BD6 | 207 | 743 | Complete selected owner |
| 0x48BE0 | 0x48BE0..0x48E43 | 162 | 612 | Complete selected owner |
| 0x48E50 | 0x48E50..0x48FAC | 100 | 349 | Complete selected owner |
| 0x48FB0 | 0x48FB0..0x49072 | 54 | 195 | Complete selected owner |
| 0x49080 | 0x49080..0x4C5BA | 2909 | 13627 | Complete selected owner |
| 0x4C680 | 0x4C680..0x4C6AB; 0x4C6AD..0x4C6D2 | 21 | 82 | Complete selected owner |
| 0x4C6E0 | 0x4C6E0..0x4C70E; 0x4C710..0x4C73C | 27 | 92 | Complete selected owner |
| 0x4C830 | 0x4C830..0x4CA09 | 110 | 474 | Complete selected owner |
| 0x4CA10 | 0x4CA10..0x4CBEA | 110 | 475 | Complete selected owner |
| 0x7EA80 | 0x7EA80..0x7F35D; 0x7F35F..0x7F36D; 0x7F36F..0x7F37D; 0x7F37F..0x7F38A; 0x7F38C..0x7F9DF | 806 | 3932 | Caller/setup context; semantic scope stated above |
| 0x89470 | 0x89470..0x894A7; 0x894B0..0x894F9 | 39 | 130 | Complete selected owner |
| 0x89B80 | 0x89B80..0x89C01 | 26 | 130 | Complete selected owner |
| 0x8A120 | 0x8A120..0x8A12B | 2 | 12 | Complete selected owner |
| 0xA3A20 | 0xA3A20..0xA3A3F | 8 | 32 | Complete selected owner |
| 0xF8690 | 0xF8690..0xF87C9 | 67 | 314 | Complete selected owner |
| 0xF9AC0 | 0xF9AC0..0x10089D | 5496 | 28126 | Caller/setup context; semantic scope stated above |
| 0x103200 | 0x103200..0x106C18 | 3103 | 14873 | Caller/setup context; semantic scope stated above |
| 0x15CFF0 | 0x15CFF0..0x15D00E | 8 | 31 | Complete selected owner |
| 0x15D5B0 | 0x15D5B0..0x15D605 | 24 | 86 | Complete selected owner |
| 0x268A70 | 0x268A70..0x268B39 | 36 | 202 | Complete selected owner |
| 0x269130 | 0x269130..0x269152 | 10 | 35 | Complete selected owner |
| 0x269160 | 0x269160..0x269183 | 9 | 36 | Complete selected owner |
| 0x269190 | 0x269190..0x269196 | 2 | 7 | Complete selected owner |
| 0x2691B0 | 0x2691B0..0x2691B8 | 2 | 9 | Complete selected owner |
| 0x2691C0 | 0x2691C0..0x2691E3 | 9 | 36 | Complete selected owner |
| 0x2691F0 | 0x2691F0..0x2691F8 | 2 | 9 | Complete selected owner |
| 0x269210 | 0x269210..0x26958E | 158 | 895 | Complete selected owner |
| 0x2758B0 | 0x2758B0..0x275A6D | 111 | 446 | Complete selected owner |
| 0x275A70 | 0x275A70..0x2761AC | 425 | 1853 | Complete selected owner |
| 0x2761F0 | 0x2761F0..0x2761FA | 2 | 11 | Complete selected owner |
| 0x276200 | 0x276200..0x276993 | 429 | 1940 | Complete selected owner |
| 0x2769A0 | 0x2769A0..0x2769FA | 23 | 91 | Complete selected owner |
| 0x276A30 | 0x276A30..0x276AED | 39 | 190 | Complete selected owner |
| 0x276C80 | 0x276C80..0x276CC1 | 14 | 66 | Complete selected owner |
| 0x276CD0 | 0x276CD0..0x276E95 | 107 | 454 | Complete selected owner |
| 0x276F30 | 0x276F30..0x276F3C | 3 | 13 | Complete selected owner |
| 0x276F40 | 0x276F40..0x27705C | 70 | 285 | Complete selected owner |
| 0x277EC0 | 0x277EC0..0x277FF6 | 74 | 311 | Complete selected owner |
| 0x278040 | 0x278040..0x279A72 | 1494 | 6707 | Complete selected owner |
| 0x328FF0 | 0x328FF0..0x329206 | 96 | 535 | Complete selected owner |
| 0x329E00 | 0x329E00..0x329E33 | 12 | 52 | Complete selected owner |
| 0x32A110 | 0x32A110..0x32A1CE | 45 | 191 | Complete selected owner |
| 0x32A1D0 | 0x32A1D0..0x32A202 | 12 | 51 | Complete selected owner |
| 0x32B0C0 | 0x32B0C0..0x32B0CB | 2 | 12 | Complete selected owner |
| 0x32B180 | 0x32B180..0x32B248 | 45 | 201 | Complete selected owner |
| 0x32D190 | 0x32D190..0x32D1A0 | 5 | 17 | Complete selected owner |
| 0x332AD0 | 0x332AD0..0x332B96 | 47 | 199 | Complete selected owner |
| 0x334410 | 0x334410..0x334506 | 60 | 247 | Complete selected owner |
| 0x3348E0 | 0x3348E0..0x334A12 | 82 | 307 | Complete selected owner |
| 0x334C50 | 0x334C50..0x334D24 | 58 | 213 | Complete selected owner |
| 0x3356D0 | 0x3356D0..0x335773 | 42 | 164 | Complete selected owner |
| 0x335830 | 0x335830..0x3358D3 | 42 | 164 | Complete selected owner |
| 0x34FA60 | 0x34FA60..0x34FE37 | 223 | 984 | Complete selected owner |
| 0x34FE40 | 0x34FE40..0x350BF3 | 749 | 3508 | Complete selected owner |

The literal inventory's per-row original addresses cover each counted first-dictionary string plus its following zero word, a total of 1,738 addressed literal bytes without padding. The raw 92-byte action jump table is 0x429FC..0x42A57; its 23 four-byte RVA entries were read directly and reconciled with the complete 0x42670 owner. The action-label literal starts and lengths are recorded in the row table. Other directly checked UTF-16 data are 0x4317E8 ("Unassigned"), 0x431800 ("Mouse "), 0x431810 ("Unknown"), 0x431820 ("?"), 0x430ECC ("M"), 0x46E5D8 ("Defaults"), 0x46E5EC ("OK"), 0x437350 ("Cancel"), 0x46E5F8 ("Press a key"), 0x46EA38 ("HOLD "), 0x46EA78 ("Hold "), 0x46EAA0 ("Move + "), 0x46EAB0 ("Tap "), and 0x436340 ("resource1.dat"). The selected constructor also reaches literal names "upbutton" at 0x441058, "downbutton" at 0x441070 and "scrollbutton" at 0x441088 in its optional-node callback setup; those names do not establish missing asset contents or geometry.

The original RTTI/table joins are: Game table 0x436068, locator 0x4AE118, type descriptor 0x549730; ControlsWidget table 0x46E468, locator 0x4B02D8, type descriptor 0x54B5F0; HelpWidget table 0x46E8A8, locator 0x4B04E8, type descriptor 0x54B6A8. Their original image pointers, relative type-descriptor addresses, terminated type names and selected +8 function pointers were checked from the raw image. This establishes the selected concrete class names without consulting recovered headers or SDK types. The table assignments inside the complete original constructors independently match those data.

The binary32 literal operands were checked directly at their instruction-selected image addresses: 0x46DEB8=41D80000 (27), 0x42E7C8=42F00000 (120), 0x42E59C=41400000 (12), 0x42E580=40400000 (3), 0x42E5A0=41A00000 (20), 0x42E5A8=41F00000 (30), 0x42E5AC=42200000 (40), 0x42E598=41200000 (10), 0x43293C=437A0000 (250), 0x4AC964=3F000000 (0.5), 0x432D94=43700000 (240), 0x42E7B8=42700000 (60), 0x42E5B8=42A00000 (80), 0x42E7EC=44160000 (600), and 0x42E5C8=43FA0000 (500). The relevant four-word color constants are stated in the publication prose above. Float arithmetic refers to the reached scalar instruction order. No full numerical closure of font/geometry providers, nondefault floating-control modes or concurrent current-data changes is supplied.

Exposure statement: this assignment read only the standalone author policy and filename-selected original executable/export material, plus this source-side assignment. It did not read active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, project history, replacement engine/exporter/extractor code, tests or results, installed or owner configuration, owner files/data/assets/outputs/captures, SDK/dependency/reconstruction sources, packages, or previous held handoffs. Original runtime execution and implementation/verification feedback were not used. All earlier held artifacts were left unread and unchanged. The document contains behavioral prose, literal data and evidence metadata, without source code, pseudocode or implementation instructions.

Delivery is finalized only after the entire saved UTF-8 artifact is read back and its exact byte length and SHA256 are reported separately with FINAL / STABLE / HELD. No further writes are authorized to this artifact after that hold.
