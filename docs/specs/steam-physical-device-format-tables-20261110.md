Original Steam physical sampling format and object tables
Independent original-source contract, 2026-11-10

This contract reports the complete initial image contents at format headers RVA 0x473C60 and 0x473C40, their referenced record arrays at 0x546FC0 and 0x546F10, and the concrete original startup calls that pass those headers. Field names below are deliberately based on offsets and widths. No SDK enumeration or provider implementation was consulted to assign meanings to the numeric words, the identifier literals, or the virtual slots.

The supported file is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. It is 5,757,952 bytes, SHA256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, AMD64 machine 0x8664, preferred base 0x140000000, PE timestamp 0x5D93AB0D, entry RVA 0x3939F0, and image size 0x58A000. All RVAs and absolute pointer values below refer to that supported file and preferred base. The original executable was not run.

The two headers and their exact footprint

Each header occupies 32 bytes. The first word is 32 and the second is 24. The native caller passes a pointer to the header; it does not individually load or interpret these fields. The full 32-byte footprint, including the zero word before the final eight-byte pointer, is reported here rather than silently discarding alignment bytes or assigning a guessed property name.

| Header-relative offset | Width | Header 0x473C60 | Header 0x473C40 |
|---|---:|---|---|
| +0 | 32 bits | 0x00000020 (32) | 0x00000020 (32) |
| +4 | 32 bits | 0x00000018 (24) | 0x00000018 (24) |
| +8 | 32 bits | 0x00000002 (2) | 0x00000002 (2) |
| +0xC | 32 bits | 0x00000100 (256) | 0x00000010 (16) |
| +0x10 | 32 bits | 0x00000100 (256) | 0x00000007 (7) |
| +0x14 | 32 bits | 0x00000000 | 0x00000000 |
| +0x18 | 64 bits | 0x0000000140546FC0 | 0x0000000140546F10 |

Header 0x473C60 is exactly the inclusive RVA range 0x473C60..0x473C7F, file offsets 0x472460..0x47247F. Header 0x473C40 is 0x473C40..0x473C5F, file offsets 0x472440..0x47245F. Both lie in the original PE's .rdata section. This section placement and the reported file bytes do not establish that every possible runtime alias or provider leaves memory unchanged.

The array enumerations use the header's 24-byte entry word and its count word. Every byte of each bounded array was read directly from the image. A record's first eight bytes are a pointer word, followed by four consecutive 32-bit words at +8, +0xC, +0x10 and +0x14. The tables include the final zero word within each 24-byte footprint; it is not a proved independent provider property. No unreported byte exists inside these stated footprints.

The 256-record array occupies 6,144 bytes at 0x546FC0..0x5487BF, file offsets 0x544BC0..0x5463BF. Its last record begins at 0x5487A8. The seven-record array occupies 168 bytes at 0x546F10..0x546FB7, file offsets 0x544B10..0x544BB7. Its last record begins at 0x546FA0. Both arrays lie in the original .data section. The eight bytes at 0x546FB8..0x546FBF are outside both enumerations; they are not added as an eighth record, nor treated as part of the first 256-record entry.

Complete seven-record array referenced by header 0x473C40

All numbers in the following record tables are exact unsigned bit patterns. In particular, the high bit in some +0xC words is preserved; no signed conversion, optional/required interpretation, axis mode, button classification or provider validation is inferred from that bit. A zero pointer remains zero and is not replaced with a guessed identifier.


| Record | Record RVA | Eight-byte +0 pointer | 32-bit +8 | 32-bit +0xC | 32-bit +0x10 | 32-bit +0x14 |
|---:|---|---|---|---|---|---|
| 0 | 0x546F10 | 0x0000000140473C90 | 0x00000000 | 0x00FFFF03 | 0x00000000 | 0x00000000 |
| 1 | 0x546F28 | 0x0000000140473CA0 | 0x00000004 | 0x00FFFF03 | 0x00000000 | 0x00000000 |
| 2 | 0x546F40 | 0x0000000140473CB0 | 0x00000008 | 0x80FFFF03 | 0x00000000 | 0x00000000 |
| 3 | 0x546F58 | 0x0000000000000000 | 0x0000000C | 0x00FFFF0C | 0x00000000 | 0x00000000 |
| 4 | 0x546F70 | 0x0000000000000000 | 0x0000000D | 0x00FFFF0C | 0x00000000 | 0x00000000 |
| 5 | 0x546F88 | 0x0000000000000000 | 0x0000000E | 0x80FFFF0C | 0x00000000 | 0x00000000 |
| 6 | 0x546FA0 | 0x0000000000000000 | 0x0000000F | 0x80FFFF0C | 0x00000000 | 0x00000000 |

Complete 256-record array referenced by header 0x473C60

For every one of these 256 directly read records, the pointer is exactly 0x0000000140473CC0 and the +0x10 and +0x14 words are zero. The +8 word equals the record index from 0 through 255. The +0xC word starts at 0x8000000C and increases by 0x00000100 between adjacent records, ending at 0x8000FF0C. These are independently checked facts for the complete array, not values supplied by a provider or guessed from a key enumeration. Every field is repeated below so the inventory is complete without relying on an unstated pattern.


| Record | Record RVA | Eight-byte +0 pointer | 32-bit +8 | 32-bit +0xC | 32-bit +0x10 | 32-bit +0x14 |
|---:|---|---|---|---|---|---|
| 0 | 0x546FC0 | 0x0000000140473CC0 | 0x00000000 | 0x8000000C | 0x00000000 | 0x00000000 |
| 1 | 0x546FD8 | 0x0000000140473CC0 | 0x00000001 | 0x8000010C | 0x00000000 | 0x00000000 |
| 2 | 0x546FF0 | 0x0000000140473CC0 | 0x00000002 | 0x8000020C | 0x00000000 | 0x00000000 |
| 3 | 0x547008 | 0x0000000140473CC0 | 0x00000003 | 0x8000030C | 0x00000000 | 0x00000000 |
| 4 | 0x547020 | 0x0000000140473CC0 | 0x00000004 | 0x8000040C | 0x00000000 | 0x00000000 |
| 5 | 0x547038 | 0x0000000140473CC0 | 0x00000005 | 0x8000050C | 0x00000000 | 0x00000000 |
| 6 | 0x547050 | 0x0000000140473CC0 | 0x00000006 | 0x8000060C | 0x00000000 | 0x00000000 |
| 7 | 0x547068 | 0x0000000140473CC0 | 0x00000007 | 0x8000070C | 0x00000000 | 0x00000000 |
| 8 | 0x547080 | 0x0000000140473CC0 | 0x00000008 | 0x8000080C | 0x00000000 | 0x00000000 |
| 9 | 0x547098 | 0x0000000140473CC0 | 0x00000009 | 0x8000090C | 0x00000000 | 0x00000000 |
| 10 | 0x5470B0 | 0x0000000140473CC0 | 0x0000000A | 0x80000A0C | 0x00000000 | 0x00000000 |
| 11 | 0x5470C8 | 0x0000000140473CC0 | 0x0000000B | 0x80000B0C | 0x00000000 | 0x00000000 |
| 12 | 0x5470E0 | 0x0000000140473CC0 | 0x0000000C | 0x80000C0C | 0x00000000 | 0x00000000 |
| 13 | 0x5470F8 | 0x0000000140473CC0 | 0x0000000D | 0x80000D0C | 0x00000000 | 0x00000000 |
| 14 | 0x547110 | 0x0000000140473CC0 | 0x0000000E | 0x80000E0C | 0x00000000 | 0x00000000 |
| 15 | 0x547128 | 0x0000000140473CC0 | 0x0000000F | 0x80000F0C | 0x00000000 | 0x00000000 |
| 16 | 0x547140 | 0x0000000140473CC0 | 0x00000010 | 0x8000100C | 0x00000000 | 0x00000000 |
| 17 | 0x547158 | 0x0000000140473CC0 | 0x00000011 | 0x8000110C | 0x00000000 | 0x00000000 |
| 18 | 0x547170 | 0x0000000140473CC0 | 0x00000012 | 0x8000120C | 0x00000000 | 0x00000000 |
| 19 | 0x547188 | 0x0000000140473CC0 | 0x00000013 | 0x8000130C | 0x00000000 | 0x00000000 |
| 20 | 0x5471A0 | 0x0000000140473CC0 | 0x00000014 | 0x8000140C | 0x00000000 | 0x00000000 |
| 21 | 0x5471B8 | 0x0000000140473CC0 | 0x00000015 | 0x8000150C | 0x00000000 | 0x00000000 |
| 22 | 0x5471D0 | 0x0000000140473CC0 | 0x00000016 | 0x8000160C | 0x00000000 | 0x00000000 |
| 23 | 0x5471E8 | 0x0000000140473CC0 | 0x00000017 | 0x8000170C | 0x00000000 | 0x00000000 |
| 24 | 0x547200 | 0x0000000140473CC0 | 0x00000018 | 0x8000180C | 0x00000000 | 0x00000000 |
| 25 | 0x547218 | 0x0000000140473CC0 | 0x00000019 | 0x8000190C | 0x00000000 | 0x00000000 |
| 26 | 0x547230 | 0x0000000140473CC0 | 0x0000001A | 0x80001A0C | 0x00000000 | 0x00000000 |
| 27 | 0x547248 | 0x0000000140473CC0 | 0x0000001B | 0x80001B0C | 0x00000000 | 0x00000000 |
| 28 | 0x547260 | 0x0000000140473CC0 | 0x0000001C | 0x80001C0C | 0x00000000 | 0x00000000 |
| 29 | 0x547278 | 0x0000000140473CC0 | 0x0000001D | 0x80001D0C | 0x00000000 | 0x00000000 |
| 30 | 0x547290 | 0x0000000140473CC0 | 0x0000001E | 0x80001E0C | 0x00000000 | 0x00000000 |
| 31 | 0x5472A8 | 0x0000000140473CC0 | 0x0000001F | 0x80001F0C | 0x00000000 | 0x00000000 |
| 32 | 0x5472C0 | 0x0000000140473CC0 | 0x00000020 | 0x8000200C | 0x00000000 | 0x00000000 |
| 33 | 0x5472D8 | 0x0000000140473CC0 | 0x00000021 | 0x8000210C | 0x00000000 | 0x00000000 |
| 34 | 0x5472F0 | 0x0000000140473CC0 | 0x00000022 | 0x8000220C | 0x00000000 | 0x00000000 |
| 35 | 0x547308 | 0x0000000140473CC0 | 0x00000023 | 0x8000230C | 0x00000000 | 0x00000000 |
| 36 | 0x547320 | 0x0000000140473CC0 | 0x00000024 | 0x8000240C | 0x00000000 | 0x00000000 |
| 37 | 0x547338 | 0x0000000140473CC0 | 0x00000025 | 0x8000250C | 0x00000000 | 0x00000000 |
| 38 | 0x547350 | 0x0000000140473CC0 | 0x00000026 | 0x8000260C | 0x00000000 | 0x00000000 |
| 39 | 0x547368 | 0x0000000140473CC0 | 0x00000027 | 0x8000270C | 0x00000000 | 0x00000000 |
| 40 | 0x547380 | 0x0000000140473CC0 | 0x00000028 | 0x8000280C | 0x00000000 | 0x00000000 |
| 41 | 0x547398 | 0x0000000140473CC0 | 0x00000029 | 0x8000290C | 0x00000000 | 0x00000000 |
| 42 | 0x5473B0 | 0x0000000140473CC0 | 0x0000002A | 0x80002A0C | 0x00000000 | 0x00000000 |
| 43 | 0x5473C8 | 0x0000000140473CC0 | 0x0000002B | 0x80002B0C | 0x00000000 | 0x00000000 |
| 44 | 0x5473E0 | 0x0000000140473CC0 | 0x0000002C | 0x80002C0C | 0x00000000 | 0x00000000 |
| 45 | 0x5473F8 | 0x0000000140473CC0 | 0x0000002D | 0x80002D0C | 0x00000000 | 0x00000000 |
| 46 | 0x547410 | 0x0000000140473CC0 | 0x0000002E | 0x80002E0C | 0x00000000 | 0x00000000 |
| 47 | 0x547428 | 0x0000000140473CC0 | 0x0000002F | 0x80002F0C | 0x00000000 | 0x00000000 |
| 48 | 0x547440 | 0x0000000140473CC0 | 0x00000030 | 0x8000300C | 0x00000000 | 0x00000000 |
| 49 | 0x547458 | 0x0000000140473CC0 | 0x00000031 | 0x8000310C | 0x00000000 | 0x00000000 |
| 50 | 0x547470 | 0x0000000140473CC0 | 0x00000032 | 0x8000320C | 0x00000000 | 0x00000000 |
| 51 | 0x547488 | 0x0000000140473CC0 | 0x00000033 | 0x8000330C | 0x00000000 | 0x00000000 |
| 52 | 0x5474A0 | 0x0000000140473CC0 | 0x00000034 | 0x8000340C | 0x00000000 | 0x00000000 |
| 53 | 0x5474B8 | 0x0000000140473CC0 | 0x00000035 | 0x8000350C | 0x00000000 | 0x00000000 |
| 54 | 0x5474D0 | 0x0000000140473CC0 | 0x00000036 | 0x8000360C | 0x00000000 | 0x00000000 |
| 55 | 0x5474E8 | 0x0000000140473CC0 | 0x00000037 | 0x8000370C | 0x00000000 | 0x00000000 |
| 56 | 0x547500 | 0x0000000140473CC0 | 0x00000038 | 0x8000380C | 0x00000000 | 0x00000000 |
| 57 | 0x547518 | 0x0000000140473CC0 | 0x00000039 | 0x8000390C | 0x00000000 | 0x00000000 |
| 58 | 0x547530 | 0x0000000140473CC0 | 0x0000003A | 0x80003A0C | 0x00000000 | 0x00000000 |
| 59 | 0x547548 | 0x0000000140473CC0 | 0x0000003B | 0x80003B0C | 0x00000000 | 0x00000000 |
| 60 | 0x547560 | 0x0000000140473CC0 | 0x0000003C | 0x80003C0C | 0x00000000 | 0x00000000 |
| 61 | 0x547578 | 0x0000000140473CC0 | 0x0000003D | 0x80003D0C | 0x00000000 | 0x00000000 |
| 62 | 0x547590 | 0x0000000140473CC0 | 0x0000003E | 0x80003E0C | 0x00000000 | 0x00000000 |
| 63 | 0x5475A8 | 0x0000000140473CC0 | 0x0000003F | 0x80003F0C | 0x00000000 | 0x00000000 |
| 64 | 0x5475C0 | 0x0000000140473CC0 | 0x00000040 | 0x8000400C | 0x00000000 | 0x00000000 |
| 65 | 0x5475D8 | 0x0000000140473CC0 | 0x00000041 | 0x8000410C | 0x00000000 | 0x00000000 |
| 66 | 0x5475F0 | 0x0000000140473CC0 | 0x00000042 | 0x8000420C | 0x00000000 | 0x00000000 |
| 67 | 0x547608 | 0x0000000140473CC0 | 0x00000043 | 0x8000430C | 0x00000000 | 0x00000000 |
| 68 | 0x547620 | 0x0000000140473CC0 | 0x00000044 | 0x8000440C | 0x00000000 | 0x00000000 |
| 69 | 0x547638 | 0x0000000140473CC0 | 0x00000045 | 0x8000450C | 0x00000000 | 0x00000000 |
| 70 | 0x547650 | 0x0000000140473CC0 | 0x00000046 | 0x8000460C | 0x00000000 | 0x00000000 |
| 71 | 0x547668 | 0x0000000140473CC0 | 0x00000047 | 0x8000470C | 0x00000000 | 0x00000000 |
| 72 | 0x547680 | 0x0000000140473CC0 | 0x00000048 | 0x8000480C | 0x00000000 | 0x00000000 |
| 73 | 0x547698 | 0x0000000140473CC0 | 0x00000049 | 0x8000490C | 0x00000000 | 0x00000000 |
| 74 | 0x5476B0 | 0x0000000140473CC0 | 0x0000004A | 0x80004A0C | 0x00000000 | 0x00000000 |
| 75 | 0x5476C8 | 0x0000000140473CC0 | 0x0000004B | 0x80004B0C | 0x00000000 | 0x00000000 |
| 76 | 0x5476E0 | 0x0000000140473CC0 | 0x0000004C | 0x80004C0C | 0x00000000 | 0x00000000 |
| 77 | 0x5476F8 | 0x0000000140473CC0 | 0x0000004D | 0x80004D0C | 0x00000000 | 0x00000000 |
| 78 | 0x547710 | 0x0000000140473CC0 | 0x0000004E | 0x80004E0C | 0x00000000 | 0x00000000 |
| 79 | 0x547728 | 0x0000000140473CC0 | 0x0000004F | 0x80004F0C | 0x00000000 | 0x00000000 |
| 80 | 0x547740 | 0x0000000140473CC0 | 0x00000050 | 0x8000500C | 0x00000000 | 0x00000000 |
| 81 | 0x547758 | 0x0000000140473CC0 | 0x00000051 | 0x8000510C | 0x00000000 | 0x00000000 |
| 82 | 0x547770 | 0x0000000140473CC0 | 0x00000052 | 0x8000520C | 0x00000000 | 0x00000000 |
| 83 | 0x547788 | 0x0000000140473CC0 | 0x00000053 | 0x8000530C | 0x00000000 | 0x00000000 |
| 84 | 0x5477A0 | 0x0000000140473CC0 | 0x00000054 | 0x8000540C | 0x00000000 | 0x00000000 |
| 85 | 0x5477B8 | 0x0000000140473CC0 | 0x00000055 | 0x8000550C | 0x00000000 | 0x00000000 |
| 86 | 0x5477D0 | 0x0000000140473CC0 | 0x00000056 | 0x8000560C | 0x00000000 | 0x00000000 |
| 87 | 0x5477E8 | 0x0000000140473CC0 | 0x00000057 | 0x8000570C | 0x00000000 | 0x00000000 |
| 88 | 0x547800 | 0x0000000140473CC0 | 0x00000058 | 0x8000580C | 0x00000000 | 0x00000000 |
| 89 | 0x547818 | 0x0000000140473CC0 | 0x00000059 | 0x8000590C | 0x00000000 | 0x00000000 |
| 90 | 0x547830 | 0x0000000140473CC0 | 0x0000005A | 0x80005A0C | 0x00000000 | 0x00000000 |
| 91 | 0x547848 | 0x0000000140473CC0 | 0x0000005B | 0x80005B0C | 0x00000000 | 0x00000000 |
| 92 | 0x547860 | 0x0000000140473CC0 | 0x0000005C | 0x80005C0C | 0x00000000 | 0x00000000 |
| 93 | 0x547878 | 0x0000000140473CC0 | 0x0000005D | 0x80005D0C | 0x00000000 | 0x00000000 |
| 94 | 0x547890 | 0x0000000140473CC0 | 0x0000005E | 0x80005E0C | 0x00000000 | 0x00000000 |
| 95 | 0x5478A8 | 0x0000000140473CC0 | 0x0000005F | 0x80005F0C | 0x00000000 | 0x00000000 |
| 96 | 0x5478C0 | 0x0000000140473CC0 | 0x00000060 | 0x8000600C | 0x00000000 | 0x00000000 |
| 97 | 0x5478D8 | 0x0000000140473CC0 | 0x00000061 | 0x8000610C | 0x00000000 | 0x00000000 |
| 98 | 0x5478F0 | 0x0000000140473CC0 | 0x00000062 | 0x8000620C | 0x00000000 | 0x00000000 |
| 99 | 0x547908 | 0x0000000140473CC0 | 0x00000063 | 0x8000630C | 0x00000000 | 0x00000000 |
| 100 | 0x547920 | 0x0000000140473CC0 | 0x00000064 | 0x8000640C | 0x00000000 | 0x00000000 |
| 101 | 0x547938 | 0x0000000140473CC0 | 0x00000065 | 0x8000650C | 0x00000000 | 0x00000000 |
| 102 | 0x547950 | 0x0000000140473CC0 | 0x00000066 | 0x8000660C | 0x00000000 | 0x00000000 |
| 103 | 0x547968 | 0x0000000140473CC0 | 0x00000067 | 0x8000670C | 0x00000000 | 0x00000000 |
| 104 | 0x547980 | 0x0000000140473CC0 | 0x00000068 | 0x8000680C | 0x00000000 | 0x00000000 |
| 105 | 0x547998 | 0x0000000140473CC0 | 0x00000069 | 0x8000690C | 0x00000000 | 0x00000000 |
| 106 | 0x5479B0 | 0x0000000140473CC0 | 0x0000006A | 0x80006A0C | 0x00000000 | 0x00000000 |
| 107 | 0x5479C8 | 0x0000000140473CC0 | 0x0000006B | 0x80006B0C | 0x00000000 | 0x00000000 |
| 108 | 0x5479E0 | 0x0000000140473CC0 | 0x0000006C | 0x80006C0C | 0x00000000 | 0x00000000 |
| 109 | 0x5479F8 | 0x0000000140473CC0 | 0x0000006D | 0x80006D0C | 0x00000000 | 0x00000000 |
| 110 | 0x547A10 | 0x0000000140473CC0 | 0x0000006E | 0x80006E0C | 0x00000000 | 0x00000000 |
| 111 | 0x547A28 | 0x0000000140473CC0 | 0x0000006F | 0x80006F0C | 0x00000000 | 0x00000000 |
| 112 | 0x547A40 | 0x0000000140473CC0 | 0x00000070 | 0x8000700C | 0x00000000 | 0x00000000 |
| 113 | 0x547A58 | 0x0000000140473CC0 | 0x00000071 | 0x8000710C | 0x00000000 | 0x00000000 |
| 114 | 0x547A70 | 0x0000000140473CC0 | 0x00000072 | 0x8000720C | 0x00000000 | 0x00000000 |
| 115 | 0x547A88 | 0x0000000140473CC0 | 0x00000073 | 0x8000730C | 0x00000000 | 0x00000000 |
| 116 | 0x547AA0 | 0x0000000140473CC0 | 0x00000074 | 0x8000740C | 0x00000000 | 0x00000000 |
| 117 | 0x547AB8 | 0x0000000140473CC0 | 0x00000075 | 0x8000750C | 0x00000000 | 0x00000000 |
| 118 | 0x547AD0 | 0x0000000140473CC0 | 0x00000076 | 0x8000760C | 0x00000000 | 0x00000000 |
| 119 | 0x547AE8 | 0x0000000140473CC0 | 0x00000077 | 0x8000770C | 0x00000000 | 0x00000000 |
| 120 | 0x547B00 | 0x0000000140473CC0 | 0x00000078 | 0x8000780C | 0x00000000 | 0x00000000 |
| 121 | 0x547B18 | 0x0000000140473CC0 | 0x00000079 | 0x8000790C | 0x00000000 | 0x00000000 |
| 122 | 0x547B30 | 0x0000000140473CC0 | 0x0000007A | 0x80007A0C | 0x00000000 | 0x00000000 |
| 123 | 0x547B48 | 0x0000000140473CC0 | 0x0000007B | 0x80007B0C | 0x00000000 | 0x00000000 |
| 124 | 0x547B60 | 0x0000000140473CC0 | 0x0000007C | 0x80007C0C | 0x00000000 | 0x00000000 |
| 125 | 0x547B78 | 0x0000000140473CC0 | 0x0000007D | 0x80007D0C | 0x00000000 | 0x00000000 |
| 126 | 0x547B90 | 0x0000000140473CC0 | 0x0000007E | 0x80007E0C | 0x00000000 | 0x00000000 |
| 127 | 0x547BA8 | 0x0000000140473CC0 | 0x0000007F | 0x80007F0C | 0x00000000 | 0x00000000 |
| 128 | 0x547BC0 | 0x0000000140473CC0 | 0x00000080 | 0x8000800C | 0x00000000 | 0x00000000 |
| 129 | 0x547BD8 | 0x0000000140473CC0 | 0x00000081 | 0x8000810C | 0x00000000 | 0x00000000 |
| 130 | 0x547BF0 | 0x0000000140473CC0 | 0x00000082 | 0x8000820C | 0x00000000 | 0x00000000 |
| 131 | 0x547C08 | 0x0000000140473CC0 | 0x00000083 | 0x8000830C | 0x00000000 | 0x00000000 |
| 132 | 0x547C20 | 0x0000000140473CC0 | 0x00000084 | 0x8000840C | 0x00000000 | 0x00000000 |
| 133 | 0x547C38 | 0x0000000140473CC0 | 0x00000085 | 0x8000850C | 0x00000000 | 0x00000000 |
| 134 | 0x547C50 | 0x0000000140473CC0 | 0x00000086 | 0x8000860C | 0x00000000 | 0x00000000 |
| 135 | 0x547C68 | 0x0000000140473CC0 | 0x00000087 | 0x8000870C | 0x00000000 | 0x00000000 |
| 136 | 0x547C80 | 0x0000000140473CC0 | 0x00000088 | 0x8000880C | 0x00000000 | 0x00000000 |
| 137 | 0x547C98 | 0x0000000140473CC0 | 0x00000089 | 0x8000890C | 0x00000000 | 0x00000000 |
| 138 | 0x547CB0 | 0x0000000140473CC0 | 0x0000008A | 0x80008A0C | 0x00000000 | 0x00000000 |
| 139 | 0x547CC8 | 0x0000000140473CC0 | 0x0000008B | 0x80008B0C | 0x00000000 | 0x00000000 |
| 140 | 0x547CE0 | 0x0000000140473CC0 | 0x0000008C | 0x80008C0C | 0x00000000 | 0x00000000 |
| 141 | 0x547CF8 | 0x0000000140473CC0 | 0x0000008D | 0x80008D0C | 0x00000000 | 0x00000000 |
| 142 | 0x547D10 | 0x0000000140473CC0 | 0x0000008E | 0x80008E0C | 0x00000000 | 0x00000000 |
| 143 | 0x547D28 | 0x0000000140473CC0 | 0x0000008F | 0x80008F0C | 0x00000000 | 0x00000000 |
| 144 | 0x547D40 | 0x0000000140473CC0 | 0x00000090 | 0x8000900C | 0x00000000 | 0x00000000 |
| 145 | 0x547D58 | 0x0000000140473CC0 | 0x00000091 | 0x8000910C | 0x00000000 | 0x00000000 |
| 146 | 0x547D70 | 0x0000000140473CC0 | 0x00000092 | 0x8000920C | 0x00000000 | 0x00000000 |
| 147 | 0x547D88 | 0x0000000140473CC0 | 0x00000093 | 0x8000930C | 0x00000000 | 0x00000000 |
| 148 | 0x547DA0 | 0x0000000140473CC0 | 0x00000094 | 0x8000940C | 0x00000000 | 0x00000000 |
| 149 | 0x547DB8 | 0x0000000140473CC0 | 0x00000095 | 0x8000950C | 0x00000000 | 0x00000000 |
| 150 | 0x547DD0 | 0x0000000140473CC0 | 0x00000096 | 0x8000960C | 0x00000000 | 0x00000000 |
| 151 | 0x547DE8 | 0x0000000140473CC0 | 0x00000097 | 0x8000970C | 0x00000000 | 0x00000000 |
| 152 | 0x547E00 | 0x0000000140473CC0 | 0x00000098 | 0x8000980C | 0x00000000 | 0x00000000 |
| 153 | 0x547E18 | 0x0000000140473CC0 | 0x00000099 | 0x8000990C | 0x00000000 | 0x00000000 |
| 154 | 0x547E30 | 0x0000000140473CC0 | 0x0000009A | 0x80009A0C | 0x00000000 | 0x00000000 |
| 155 | 0x547E48 | 0x0000000140473CC0 | 0x0000009B | 0x80009B0C | 0x00000000 | 0x00000000 |
| 156 | 0x547E60 | 0x0000000140473CC0 | 0x0000009C | 0x80009C0C | 0x00000000 | 0x00000000 |
| 157 | 0x547E78 | 0x0000000140473CC0 | 0x0000009D | 0x80009D0C | 0x00000000 | 0x00000000 |
| 158 | 0x547E90 | 0x0000000140473CC0 | 0x0000009E | 0x80009E0C | 0x00000000 | 0x00000000 |
| 159 | 0x547EA8 | 0x0000000140473CC0 | 0x0000009F | 0x80009F0C | 0x00000000 | 0x00000000 |
| 160 | 0x547EC0 | 0x0000000140473CC0 | 0x000000A0 | 0x8000A00C | 0x00000000 | 0x00000000 |
| 161 | 0x547ED8 | 0x0000000140473CC0 | 0x000000A1 | 0x8000A10C | 0x00000000 | 0x00000000 |
| 162 | 0x547EF0 | 0x0000000140473CC0 | 0x000000A2 | 0x8000A20C | 0x00000000 | 0x00000000 |
| 163 | 0x547F08 | 0x0000000140473CC0 | 0x000000A3 | 0x8000A30C | 0x00000000 | 0x00000000 |
| 164 | 0x547F20 | 0x0000000140473CC0 | 0x000000A4 | 0x8000A40C | 0x00000000 | 0x00000000 |
| 165 | 0x547F38 | 0x0000000140473CC0 | 0x000000A5 | 0x8000A50C | 0x00000000 | 0x00000000 |
| 166 | 0x547F50 | 0x0000000140473CC0 | 0x000000A6 | 0x8000A60C | 0x00000000 | 0x00000000 |
| 167 | 0x547F68 | 0x0000000140473CC0 | 0x000000A7 | 0x8000A70C | 0x00000000 | 0x00000000 |
| 168 | 0x547F80 | 0x0000000140473CC0 | 0x000000A8 | 0x8000A80C | 0x00000000 | 0x00000000 |
| 169 | 0x547F98 | 0x0000000140473CC0 | 0x000000A9 | 0x8000A90C | 0x00000000 | 0x00000000 |
| 170 | 0x547FB0 | 0x0000000140473CC0 | 0x000000AA | 0x8000AA0C | 0x00000000 | 0x00000000 |
| 171 | 0x547FC8 | 0x0000000140473CC0 | 0x000000AB | 0x8000AB0C | 0x00000000 | 0x00000000 |
| 172 | 0x547FE0 | 0x0000000140473CC0 | 0x000000AC | 0x8000AC0C | 0x00000000 | 0x00000000 |
| 173 | 0x547FF8 | 0x0000000140473CC0 | 0x000000AD | 0x8000AD0C | 0x00000000 | 0x00000000 |
| 174 | 0x548010 | 0x0000000140473CC0 | 0x000000AE | 0x8000AE0C | 0x00000000 | 0x00000000 |
| 175 | 0x548028 | 0x0000000140473CC0 | 0x000000AF | 0x8000AF0C | 0x00000000 | 0x00000000 |
| 176 | 0x548040 | 0x0000000140473CC0 | 0x000000B0 | 0x8000B00C | 0x00000000 | 0x00000000 |
| 177 | 0x548058 | 0x0000000140473CC0 | 0x000000B1 | 0x8000B10C | 0x00000000 | 0x00000000 |
| 178 | 0x548070 | 0x0000000140473CC0 | 0x000000B2 | 0x8000B20C | 0x00000000 | 0x00000000 |
| 179 | 0x548088 | 0x0000000140473CC0 | 0x000000B3 | 0x8000B30C | 0x00000000 | 0x00000000 |
| 180 | 0x5480A0 | 0x0000000140473CC0 | 0x000000B4 | 0x8000B40C | 0x00000000 | 0x00000000 |
| 181 | 0x5480B8 | 0x0000000140473CC0 | 0x000000B5 | 0x8000B50C | 0x00000000 | 0x00000000 |
| 182 | 0x5480D0 | 0x0000000140473CC0 | 0x000000B6 | 0x8000B60C | 0x00000000 | 0x00000000 |
| 183 | 0x5480E8 | 0x0000000140473CC0 | 0x000000B7 | 0x8000B70C | 0x00000000 | 0x00000000 |
| 184 | 0x548100 | 0x0000000140473CC0 | 0x000000B8 | 0x8000B80C | 0x00000000 | 0x00000000 |
| 185 | 0x548118 | 0x0000000140473CC0 | 0x000000B9 | 0x8000B90C | 0x00000000 | 0x00000000 |
| 186 | 0x548130 | 0x0000000140473CC0 | 0x000000BA | 0x8000BA0C | 0x00000000 | 0x00000000 |
| 187 | 0x548148 | 0x0000000140473CC0 | 0x000000BB | 0x8000BB0C | 0x00000000 | 0x00000000 |
| 188 | 0x548160 | 0x0000000140473CC0 | 0x000000BC | 0x8000BC0C | 0x00000000 | 0x00000000 |
| 189 | 0x548178 | 0x0000000140473CC0 | 0x000000BD | 0x8000BD0C | 0x00000000 | 0x00000000 |
| 190 | 0x548190 | 0x0000000140473CC0 | 0x000000BE | 0x8000BE0C | 0x00000000 | 0x00000000 |
| 191 | 0x5481A8 | 0x0000000140473CC0 | 0x000000BF | 0x8000BF0C | 0x00000000 | 0x00000000 |
| 192 | 0x5481C0 | 0x0000000140473CC0 | 0x000000C0 | 0x8000C00C | 0x00000000 | 0x00000000 |
| 193 | 0x5481D8 | 0x0000000140473CC0 | 0x000000C1 | 0x8000C10C | 0x00000000 | 0x00000000 |
| 194 | 0x5481F0 | 0x0000000140473CC0 | 0x000000C2 | 0x8000C20C | 0x00000000 | 0x00000000 |
| 195 | 0x548208 | 0x0000000140473CC0 | 0x000000C3 | 0x8000C30C | 0x00000000 | 0x00000000 |
| 196 | 0x548220 | 0x0000000140473CC0 | 0x000000C4 | 0x8000C40C | 0x00000000 | 0x00000000 |
| 197 | 0x548238 | 0x0000000140473CC0 | 0x000000C5 | 0x8000C50C | 0x00000000 | 0x00000000 |
| 198 | 0x548250 | 0x0000000140473CC0 | 0x000000C6 | 0x8000C60C | 0x00000000 | 0x00000000 |
| 199 | 0x548268 | 0x0000000140473CC0 | 0x000000C7 | 0x8000C70C | 0x00000000 | 0x00000000 |
| 200 | 0x548280 | 0x0000000140473CC0 | 0x000000C8 | 0x8000C80C | 0x00000000 | 0x00000000 |
| 201 | 0x548298 | 0x0000000140473CC0 | 0x000000C9 | 0x8000C90C | 0x00000000 | 0x00000000 |
| 202 | 0x5482B0 | 0x0000000140473CC0 | 0x000000CA | 0x8000CA0C | 0x00000000 | 0x00000000 |
| 203 | 0x5482C8 | 0x0000000140473CC0 | 0x000000CB | 0x8000CB0C | 0x00000000 | 0x00000000 |
| 204 | 0x5482E0 | 0x0000000140473CC0 | 0x000000CC | 0x8000CC0C | 0x00000000 | 0x00000000 |
| 205 | 0x5482F8 | 0x0000000140473CC0 | 0x000000CD | 0x8000CD0C | 0x00000000 | 0x00000000 |
| 206 | 0x548310 | 0x0000000140473CC0 | 0x000000CE | 0x8000CE0C | 0x00000000 | 0x00000000 |
| 207 | 0x548328 | 0x0000000140473CC0 | 0x000000CF | 0x8000CF0C | 0x00000000 | 0x00000000 |
| 208 | 0x548340 | 0x0000000140473CC0 | 0x000000D0 | 0x8000D00C | 0x00000000 | 0x00000000 |
| 209 | 0x548358 | 0x0000000140473CC0 | 0x000000D1 | 0x8000D10C | 0x00000000 | 0x00000000 |
| 210 | 0x548370 | 0x0000000140473CC0 | 0x000000D2 | 0x8000D20C | 0x00000000 | 0x00000000 |
| 211 | 0x548388 | 0x0000000140473CC0 | 0x000000D3 | 0x8000D30C | 0x00000000 | 0x00000000 |
| 212 | 0x5483A0 | 0x0000000140473CC0 | 0x000000D4 | 0x8000D40C | 0x00000000 | 0x00000000 |
| 213 | 0x5483B8 | 0x0000000140473CC0 | 0x000000D5 | 0x8000D50C | 0x00000000 | 0x00000000 |
| 214 | 0x5483D0 | 0x0000000140473CC0 | 0x000000D6 | 0x8000D60C | 0x00000000 | 0x00000000 |
| 215 | 0x5483E8 | 0x0000000140473CC0 | 0x000000D7 | 0x8000D70C | 0x00000000 | 0x00000000 |
| 216 | 0x548400 | 0x0000000140473CC0 | 0x000000D8 | 0x8000D80C | 0x00000000 | 0x00000000 |
| 217 | 0x548418 | 0x0000000140473CC0 | 0x000000D9 | 0x8000D90C | 0x00000000 | 0x00000000 |
| 218 | 0x548430 | 0x0000000140473CC0 | 0x000000DA | 0x8000DA0C | 0x00000000 | 0x00000000 |
| 219 | 0x548448 | 0x0000000140473CC0 | 0x000000DB | 0x8000DB0C | 0x00000000 | 0x00000000 |
| 220 | 0x548460 | 0x0000000140473CC0 | 0x000000DC | 0x8000DC0C | 0x00000000 | 0x00000000 |
| 221 | 0x548478 | 0x0000000140473CC0 | 0x000000DD | 0x8000DD0C | 0x00000000 | 0x00000000 |
| 222 | 0x548490 | 0x0000000140473CC0 | 0x000000DE | 0x8000DE0C | 0x00000000 | 0x00000000 |
| 223 | 0x5484A8 | 0x0000000140473CC0 | 0x000000DF | 0x8000DF0C | 0x00000000 | 0x00000000 |
| 224 | 0x5484C0 | 0x0000000140473CC0 | 0x000000E0 | 0x8000E00C | 0x00000000 | 0x00000000 |
| 225 | 0x5484D8 | 0x0000000140473CC0 | 0x000000E1 | 0x8000E10C | 0x00000000 | 0x00000000 |
| 226 | 0x5484F0 | 0x0000000140473CC0 | 0x000000E2 | 0x8000E20C | 0x00000000 | 0x00000000 |
| 227 | 0x548508 | 0x0000000140473CC0 | 0x000000E3 | 0x8000E30C | 0x00000000 | 0x00000000 |
| 228 | 0x548520 | 0x0000000140473CC0 | 0x000000E4 | 0x8000E40C | 0x00000000 | 0x00000000 |
| 229 | 0x548538 | 0x0000000140473CC0 | 0x000000E5 | 0x8000E50C | 0x00000000 | 0x00000000 |
| 230 | 0x548550 | 0x0000000140473CC0 | 0x000000E6 | 0x8000E60C | 0x00000000 | 0x00000000 |
| 231 | 0x548568 | 0x0000000140473CC0 | 0x000000E7 | 0x8000E70C | 0x00000000 | 0x00000000 |
| 232 | 0x548580 | 0x0000000140473CC0 | 0x000000E8 | 0x8000E80C | 0x00000000 | 0x00000000 |
| 233 | 0x548598 | 0x0000000140473CC0 | 0x000000E9 | 0x8000E90C | 0x00000000 | 0x00000000 |
| 234 | 0x5485B0 | 0x0000000140473CC0 | 0x000000EA | 0x8000EA0C | 0x00000000 | 0x00000000 |
| 235 | 0x5485C8 | 0x0000000140473CC0 | 0x000000EB | 0x8000EB0C | 0x00000000 | 0x00000000 |
| 236 | 0x5485E0 | 0x0000000140473CC0 | 0x000000EC | 0x8000EC0C | 0x00000000 | 0x00000000 |
| 237 | 0x5485F8 | 0x0000000140473CC0 | 0x000000ED | 0x8000ED0C | 0x00000000 | 0x00000000 |
| 238 | 0x548610 | 0x0000000140473CC0 | 0x000000EE | 0x8000EE0C | 0x00000000 | 0x00000000 |
| 239 | 0x548628 | 0x0000000140473CC0 | 0x000000EF | 0x8000EF0C | 0x00000000 | 0x00000000 |
| 240 | 0x548640 | 0x0000000140473CC0 | 0x000000F0 | 0x8000F00C | 0x00000000 | 0x00000000 |
| 241 | 0x548658 | 0x0000000140473CC0 | 0x000000F1 | 0x8000F10C | 0x00000000 | 0x00000000 |
| 242 | 0x548670 | 0x0000000140473CC0 | 0x000000F2 | 0x8000F20C | 0x00000000 | 0x00000000 |
| 243 | 0x548688 | 0x0000000140473CC0 | 0x000000F3 | 0x8000F30C | 0x00000000 | 0x00000000 |
| 244 | 0x5486A0 | 0x0000000140473CC0 | 0x000000F4 | 0x8000F40C | 0x00000000 | 0x00000000 |
| 245 | 0x5486B8 | 0x0000000140473CC0 | 0x000000F5 | 0x8000F50C | 0x00000000 | 0x00000000 |
| 246 | 0x5486D0 | 0x0000000140473CC0 | 0x000000F6 | 0x8000F60C | 0x00000000 | 0x00000000 |
| 247 | 0x5486E8 | 0x0000000140473CC0 | 0x000000F7 | 0x8000F70C | 0x00000000 | 0x00000000 |
| 248 | 0x548700 | 0x0000000140473CC0 | 0x000000F8 | 0x8000F80C | 0x00000000 | 0x00000000 |
| 249 | 0x548718 | 0x0000000140473CC0 | 0x000000F9 | 0x8000F90C | 0x00000000 | 0x00000000 |
| 250 | 0x548730 | 0x0000000140473CC0 | 0x000000FA | 0x8000FA0C | 0x00000000 | 0x00000000 |
| 251 | 0x548748 | 0x0000000140473CC0 | 0x000000FB | 0x8000FB0C | 0x00000000 | 0x00000000 |
| 252 | 0x548760 | 0x0000000140473CC0 | 0x000000FC | 0x8000FC0C | 0x00000000 | 0x00000000 |
| 253 | 0x548778 | 0x0000000140473CC0 | 0x000000FD | 0x8000FD0C | 0x00000000 | 0x00000000 |
| 254 | 0x548790 | 0x0000000140473CC0 | 0x000000FE | 0x8000FE0C | 0x00000000 | 0x00000000 |
| 255 | 0x5487A8 | 0x0000000140473CC0 | 0x000000FF | 0x8000FF0C | 0x00000000 | 0x00000000 |

Referenced identifier literals and exact associations

The following seven adjacent 16-byte literal blocks occupy 0x473C80..0x473CEF in .rdata. The component columns are a display of the first four little-endian bytes, the next two little-endian bytes, the next two little-endian bytes and the final eight octets. The complete raw octets are also given. This presentation does not import a symbolic identifier name from a provider header. The original native caller passes pointers and does not supply an identifier-length argument on those calls; the contract closes the specified literal blocks, not an uninspected provider's read footprint.


| Literal RVA | Little-endian 32-bit component | 16-bit component +4 | 16-bit component +6 | Octets +8..+15, in order | Complete 16 bytes, in address order |
|---|---|---|---|---|---|
| 0x473C80 | BF798031 | 483A | 4DA2 | AA 99 5D 64 ED 36 97 00 | 31 80 79 BF 3A 48 A2 4D AA 99 5D 64 ED 36 97 00 |
| 0x473C90 | A36D02E0 | C9F3 | 11CF | BF C7 44 45 53 54 00 00 | E0 02 6D A3 F3 C9 CF 11 BF C7 44 45 53 54 00 00 |
| 0x473CA0 | A36D02E1 | C9F3 | 11CF | BF C7 44 45 53 54 00 00 | E1 02 6D A3 F3 C9 CF 11 BF C7 44 45 53 54 00 00 |
| 0x473CB0 | A36D02E2 | C9F3 | 11CF | BF C7 44 45 53 54 00 00 | E2 02 6D A3 F3 C9 CF 11 BF C7 44 45 53 54 00 00 |
| 0x473CC0 | 55728220 | D33C | 11CF | BF C7 44 45 53 54 00 00 | 20 82 72 55 3C D3 CF 11 BF C7 44 45 53 54 00 00 |
| 0x473CD0 | 6F1D2B60 | D5A0 | 11CF | BF C7 44 45 53 54 00 00 | 60 2B 1D 6F A0 D5 CF 11 BF C7 44 45 53 54 00 00 |
| 0x473CE0 | 6F1D2B61 | D5A0 | 11CF | BF C7 44 45 53 54 00 00 | 61 2B 1D 6F A0 D5 CF 11 BF C7 44 45 53 54 00 00 |

The factory call uses the literal at 0x473C80. Its first device-creation call uses 0x473CE0, and its second uses 0x473CD0. All 256 record pointers reference 0x473CC0. The first three seven-record pointers reference 0x473C90, 0x473CA0 and 0x473CB0 in that order; its final four pointers are null. There are no other nonnull identifier pointers in either complete object array. In the seven-record array the first two +0xC words are 0x00FFFF03, the third is 0x80FFFF03, the fourth and fifth are 0x00FFFF0C, and the sixth and seventh are 0x80FFFF0C. Thus replacing that table with uniformly high-bit-set entries would change original numeric data. This is a table distinction, without assigning unproved provider meanings to those words.

Concrete startup owner and setup call order

The complete native owning body is RVA 0x1355A0, inclusive range 0x1355A0..0x137E1B. Its selected original C presentation was read completely, including the later sampling loop and normal exit path, and all of its 2,217 instructions and 10,364 instruction bytes were matched continuously against the original image. The behavioral scope here is format setup, retained receiver aliases, the corresponding sample-call widths/order, and the selected exit calls. The rest of the large owner's display enumeration, rendering, audio, Game/controller/UI work and transitive helper internals are not promoted into a complete behavioral contract.

Earlier startup can exit before this setup: the original Steam initialization's zero result, failed main-window registration and a null main-window creation result have early returns. The selected graphics setup helper 0x134CC0 must return a nonzero byte to enter the branch containing these input setup calls; its zero branch reports the graphics failure and bypasses them. The selected audio initialization helper's zero byte reports failure but continues toward the factory call. These statements are caller branches, not claims about any imported subsystem's success convention or internals.

The call at 0x135EAC invokes the original six-byte thunk at 0x31DBB2. That thunk is a single indirect transfer through import slot 0x42C0D8. The original PE import descriptor independently identifies DINPUT8.dll and the name DirectInput8Create; its descriptor is at 0x52AE50, lookup entry at 0x52B120 and hint/name bytes at 0x52BF0A, with hint 0. The same import identity is present in the original export metadata. This identifies a provider boundary; its implementation was not read.

At that call the caller supplies its original first application argument as an eight-byte first argument, the 32-bit value 0x800 as the second, the full pointer to literal 0x473C80 as the third, and the address of an eight-byte stack output slot at frame-pointer displacement -0x40 as the fourth. The fifth stack argument is an eight-byte zero. The first argument was retained in R14 from entry; the zero came from R13 before that register's later reuse. No provider signature or enumeration is inferred beyond these concrete native argument operands.

Only the low 32-bit return is tested. Exactly zero reaches the setup sequence; every other 32-bit word takes the failure branch, which reports the input-factory failure, joins the selected audio cleanup at 0x137D78 and returns through the owner's common exit. The guard is not a signed-negative-only test. The zero branch also does not itself establish that the output pointer is nonnull or otherwise valid.

Let F denote the current eight-byte value in that factory output slot. Let A denote the value in a second eight-byte slot at frame displacement -0x50, and B the value in a third slot at -0x78. These names identify source-side stack aliases. They do not identify the provider's concrete class, prove A and B distinct, or make either receiver the original Game or GUI manager.

On the zero-factory-return path, the setup order is as follows. Each virtual slot is loaded through the current receiver's current vtable.

| Call RVA | Concrete caller receiver and explicit arguments |
|---|---|
| 0x135F2C | Current F, virtual +0x18; full pointer to identifier 0x473CE0; full address of the A output slot; 32-bit fourth argument zero |
| 0x135F3D | Current A reloaded from its slot, virtual +0x58; full pointer to header 0x473C60 |
| 0x135F47 | Current A reloaded again, virtual +0x38 |
| 0x135F5F | Current F reloaded, virtual +0x18; full pointer to identifier 0x473CD0; full address of the B output slot; 32-bit fourth argument zero |
| 0x135F70 | Current B reloaded from its slot, virtual +0x58; full pointer to header 0x473C40 |
| 0x135F87 | Current B reloaded again, virtual +0x68; current eight-byte window value from global RVA 0x551AC8; 32-bit argument 0xA |
| 0x135F91 | Current B reloaded again, virtual +0x38 |

The caller does not test any return from those seven calls before making the following one. In particular, it does not first check either device-creation result, either format-call result or either +0x38 result. It dereferences the slot values without a local null check in this sequence. A provider failure therefore does not imply a proved safe recovery, valid buffer setup or cleanup sequence; the original caller simply has no success guard at these joins.

The sequence passes the 256-record header to A and the seven-record header to B. It does not pass a header to F's +0x58 slot. The +0x68 call with window and 0xA occurs only for B in this selected setup sequence; there is no corresponding A call here. No API meaning such as acquisition, cooperative policy, relative motion or object optionality is assigned from the virtual offsets or the numeric words alone. Equal virtual offsets on two receivers do not prove an equal concrete target address or implementation.

The setup is before the original Game construction call at 0x136331 and before the ordinary message/sampling loop. These header pointers are image addresses, not temporary descriptors constructed on that stack. The caller does not copy, allocate or free either bounded header/array in the selected setup. Whether the provider copies, retains, validates or modifies any supplied memory remains unproved. Each receiver reload also means that a value changed through an external alias can affect a later call; the original does not preserve one object identity through all those calls.

Reached sample-call width and scheduling limits

In the normal loop of the same owner, already-dispatched window messages and selected current-window/cursor work precede the pair of sample calls. A selected global byte at RVA 0x536CE8 must be nonzero and the imported foreground-window result must equal the current eight-byte window value at 0x551AC8 to reach them. A failing gate takes the selected sleep path with argument 16 and bypasses this sampling pair. The owner's message-queue and current GUI-point helper results can affect how that gate is reached; no elapsed-time or device-delivery guarantee is inferred.

At 0x13665C the caller reloads A from its frame slot, obtains its current vtable and calls virtual +0x48 with a 32-bit count of 256 and a full pointer to a current 256-byte destination at frame displacement +0x480. At 0x136672 it reloads B and its vtable, then calls its +0x48 with the 32-bit count 16 and a full pointer to the destination beginning at frame displacement +0x1B0. Thus the A sample call precedes the B sample call. They are separate calls; the original does not make them one simultaneous input snapshot.

The native first count is supplied from R14D. The owner initializes that register to 0x100 before the loop, and the selected temporary reuse on its capture-scan route rejoins through 0x136729, which restores 0x100 before the common continuation. The complete native writer/branch check is why that call's ordinary count is stated as 256 rather than inferred from a header name. The second call uses immediate 0x10. These counts equal the respective header +0xC words, but no original provider implementation was read to prove that a header field enforces a later call's width.

Neither sample return is checked before the caller proceeds to its Game-state helper and selected record processing. The complete owner initializes a separate previous 256-byte area at frame +0x580 to zero and a separate previous 16-byte area at +0x180 to zero before the loop. It does not issue a per-call clearing of the current destinations immediately before the sampling pair. At the reached common continuation 0x137B04 it copies the current 256-byte region into the previous 256-byte area in ordered 16-byte loads/stores, then copies the current 16-byte region into the previous 16-byte area. This is caller memory work, not evidence that a failing or partial provider return fills, clears or preserves every destination byte.

The same original owner uses its own category/index tests when consuming sampled bytes and when preparing binding-capture records. A table entry for numeric index zero or any other index is not itself proof that an action binding or a capture input is admitted. The detailed action/Controls callback semantics, other physical providers in this owner, and downstream Game/creature consumers are outside this bounded table contract.

On the selected normal message-loop exit, after the reached Game and graphics-related teardown calls, native call sites 0x137D1A, 0x137D24 and 0x137D2E invoke virtual +0x10 on the current A, current B and current F slot values, in that order. Each pointer and vtable is reloaded. These three sites have no local null or return-value test. The caller then performs other subsystem calls and common cleanup. The +0x10 calls are reached teardown operations; their internal release/reference-count behavior, exception behavior and lifetime effects were not imported from a provider interface definition.

Conditional source-only cases

These are independent conditional cases, not observations from an executing original, an owner capture or a replacement implementation. With the supported bytes, record 57 of the large array has +8 word 0x00000039, +0xC word 0x8000390C, pointer 0x0000000140473CC0 and both final words zero. Its last record, index 255, is at 0x5487A8 with +8 word 0x000000FF and +0xC word 0x8000FF0C. The seven-record entry at index 2 instead points to 0x0000000140473CB0, has +8 word 8 and +0xC word 0x80FFFF03. Entry 3 has a null pointer, +8 word 12 and +0xC word 0x00FFFF0C. These numeric distinctions do not establish a provider interpretation or availability.

A factory return whose low 32 bits are 1 bypasses both header-setting calls. A low word of zero reaches the unguarded first-device sequence, provided execution continues normally through the preceding calls. If that first device's +0x58 call returns a nonzero word normally, the caller still reloads A and makes its +0x38 call; the original does not branch on that result. If both foreground/global admissions pass, the two later +0x48 invocations still occur in A-then-B order; a nonzero first return does not suppress the second call in this owner. None of these conditions promises that a provider wrote the requested memory.



Coverage, byte correspondence and limits

The assignment used only the standalone source-author policy at C:/Users/DieuDeGlace/Documents/Cubeworld-Vulkanengine-Steam/docs/CLEANROOM_AUTHOR.md and individually selected original material beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Reference filenames were discovered before scoped content searches. The complete selected original C owners are decompiled/functions/1401355a0.c and decompiled/functions/14031dbb2.c. The application owner has 1,294 exported text lines and its complete owning native range was checked before interpretation. No provider DLL, SDK declaration, game SDK or reconstructed structure definition was used.

The whole startup owner at 0x1355A0..0x137E1B contributes 2,217 instructions and 10,364 bytes. The complete thunk at 0x31DBB2..0x31DBB7 contributes one instruction and six bytes. All encoded instruction bytes in both complete exported ranges match the supported original image, and every instruction joins continuously to the next through its inclusive range end: zero mismatches and zero internal gaps. The native setup is specifically 0x135E94..0x135F93, containing the factory arguments/guard and the selected receiver calls. The source owner's later sampling sites are 0x13665C and 0x136672; selected exit sites are 0x137D1A, 0x137D24 and 0x137D2E.

The original resolved-direct-call metadata identifies one direct call into 0x1355A0 at 0x393996, owned by 0x393884. That five-byte call, inclusive 0x393996..0x39399A, was independently byte-matched. The rest of that runtime startup caller was not treated as a closed owner or provider implementation. Accordingly the complete-owner instruction total is 2,218 instructions and 10,370 bytes; the separately selected caller join adds one checked instruction and five bytes. None of those numbers counts data bytes as instructions.

Scoped native operand discovery across the two header/identifier and object-array ranges found five explicit absolute-address operands: 0x135E9D references 0x473C80, 0x135F25 references 0x473CE0, 0x135F36 references 0x473C60, 0x135F58 references 0x473CD0, and 0x135F69 references 0x473C40. All are in the verified startup owner. The object-array pointers are present in the image headers rather than requiring a direct code reference to each record. This discovery does not prove that no computed address, indirect caller, relocation, imported write or other alias reaches these data. No universal writer census or permanent runtime immutability is asserted.

The data manifest below covers all 64 header bytes, all 6,312 object-record bytes, all 112 identifier-literal bytes and the eight-byte import slot in the original file. The six blocks total 6,496 bytes and have independently calculated hashes. The .rdata/.data placement describes the PE image sections; preferred-base pointer values and raw little-endian bytes are not a guarantee about loader relocation, page protection or current runtime memory.


| Original image block | Inclusive RVA range | File offset | Bytes | SHA256 |
|---|---|---|---:|---|
| header16 | 0x473C40..0x473C5F | 0x472440 | 32 | 007F7DFB9D187B954DDCE249C992BCD3B286067A8178B408E6670E27E53133FA |
| header256 | 0x473C60..0x473C7F | 0x472460 | 32 | 24413087880A6D637F96422E77272A59FC024092CB3DED60832CDBD4E6CE78AD |
| objects16 | 0x546F10..0x546FB7 | 0x544B10 | 168 | 2A022DBE6E1A69E7BB6B3A30468D293B0B7565799CEE1CDB99D7FB214CD88E02 |
| objects256 | 0x546FC0..0x5487BF | 0x544BC0 | 6144 | BC230B490EB0808E469FDAA70A310613ADE602C8D16633CA6452A94D4B73124D |
| identifiers | 0x473C80..0x473CEF | 0x472480 | 112 | 73CBA438BC09B121F702D46873768E369F47C1855454379D24FAFA5F4A5FA015 |
| importSlot | 0x42C0D8..0x42C0DF | 0x42A8D8 | 8 | C3149D71E691807592CA70262D72E1A5C0F4682D2BC5821347EE3D8B3C1A238D |

The original metadata sources are decompiled/functions.csv, decompiled/function_ranges.csv, decompiled/calls.csv and decompiled/assembly.txt. The byte checker mapped selected RVAs through the original PE section table, compared the full exported instruction encodings, and checked continuous coverage rather than relying on a decompiler's C type guesses. Data values were read as exact little-endian 32-bit/eight-byte words at each listed record, with all 263 records covered. Identifier octets and PE import-name data were read directly. The following file hashes identify the original export evidence, not any replacement exporter or project output.


| Original evidence file beneath decompiled | Bytes | SHA256 |
|---|---:|---|
| functions.csv | 1551545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |
| function_ranges.csv | 509127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| functions/1401355a0.c | 62220 | 1150BE55AC2A7CC4D32E504213FBE3381715E05FA514AF5BD29EA12DC2690F20 |
| functions/14031dbb2.c | 352 | 7D451D4E417CCDB176824EDAA17365AA48F170FEE67539A147D0A8B48E949F66 |

The exact provider interpretation of header word 2, object words 0x00FFFF03/0x80FFFF03/0x00FFFF0C/0x80FFFF0C, and the 0x8000000C..0x8000FF0C family remains outside this contract. Their numerical values and associations are closed; optionality, category enumeration, axis/button/key meanings, unit scaling, physical device availability, key layout, sample freshness, result-buffer filling and success conventions are not imported from names. The original call order is static source order, not a wall-clock guarantee or proof of atomic/concurrent behavior.

No selected code adds runtime records to these initial arrays, but the bounded discovery is not proof against every untraced writer. There is no table lock or lifetime lease proved by these caller sites. Provider-held pointers, changes through external aliases, caller reentry, provider exceptions, partial or failed operations and process/loader concurrency remain unresolved. Other startup paths, input devices, Game/controller behavior and general teardown are not universally enumerated by this bounded table audit.

Exposure statement: this research read only the standalone author policy and selected supported original executable/export evidence, plus this assignment. It did not read active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined project history, replacement implementation/exporter/extractor, tests or results, owner files/data/outputs/captures/configuration/assets, packages, SDK/dependency/provider/reconstruction source, or any prior held handoff. The original game was not executed. No implementation or runtime-verification feedback was requested or used. All previous held artifacts remained unread and unchanged. This document contains code-free behavioral prose, numeric literal inventories and evidence metadata, with no pseudocode or implementation guidance.

The entire saved artifact must be read back before its exact byte length and SHA256 are delivered with FINAL / STABLE / HELD. All writes stop at that hold.
