# House builder notes (0x2ff6f0 cell loop, working notes)

Working notes while transcribing the game's cell loop (0x3025b0..0x316xxx). Slot names are
rbp offsets in 0x2ff6f0.

## Zone-level values
- `[rbp+0x1568]` Place of the zone's 1024-cell (0x31a5d0 = `cellPlace`), c1 = zone's 1024-cell, made even if big: `[rbp+0x80]`=c1x, `[rbp+0x78]`=c1y.
- `[rbp+0xa0]` = bigStructure(c1x, c1y, kind, big) (0x2aa270).
- `[rbp+0x1610]` = structureAnchor (0x2a9df0) or 0x7fffffff,0x7fffffff when no structure.
- `[rbp-0x54]` style = 0x2bb060(c1x, c1y, big): if big c1 even; s = floor(c1/16); v = rndAt(c1y*57+c1x, 0x4f7cff) % 30 - 1; offs {0:0x1e44621,1:0x1e44623,2:0x1e445e9,3:0x1e4465b,4:0x1e445e8,5:0x1e445ea,6:0x1e4465a,7:0x1e4465c, else 0x1e44622}; return rnd(sy*57 + sx + off).
- `[rbp-0x60]` race = 12 + xorshift(style + 0x7b2650) % 11 (table 12..22, 0x10cee0).
- `[rbp-0x4c]` culture = 0x2b9080(regionAt(place.centre)): (x,y) in [-1,1]^2 -> 2; else v = rndAt(y*57+x, 0xb3f58) % 50 - 1: 0-3:2, 4-6:0, 7-8:9, 9-10:10, 11-12:4, 13:13, 14-15:11, 16-17:6, 18-21:3, 22-24:8, 25-26:7, 27-28:12, 29-30:5, else 1.

## Per cell (i, j in -2..5, x outer)
- cx = zx*4+i ([rbp-0x78], r15), cy ([rbp-0x68], r13). dil = outside zone's own 4x4 ([rbp-0x44]).
- castle = castleCell(cx, cy) (0x2a8290) dword -> esi / [rbp+0x74] / [rbp-0x28]; sil = castle.kind.
- `[rbp+0x198]` = 0x2ab0a0(cx, cy); `[rbp+0x194]` = 0x2ab0a0(cx,cy) + (castle.style == 1 ? 20 : 16) (0x2aa250).
- type = cellObject(cx, cy) (r12, [rbp-0x64]).
- inside && type == 92: creature spawn, rand()&7, rand()&7, rand()%4 (+ groundHeight).
- reach test with 0x25f790(type) -> next cell.
- point 0x2d8630 -> [rbp+0x1590]; if type == 0 && !castle.kind -> next cell.
- srand(rnd(cy*57+cx) % 10000000) for every remaining cell.
- then per-type specials (0x3c, 0x45/0x46, 0x42, 0x44, 0x61, 0x43, 0x34, 0x2c, ...) up to 0x30509d.
- 0x3050a1: isBuilding = 0x25f540(type) > 0 ([rbp+0x7d]); type 0x2e && kind 8: object.
- 0x305101: if castle.kind || isBuilding -> house builder 0x3063c3; else if type in 27..34 (0x31abe0): crop objects (rand); then -> 0x3148d1 tree switch.

## House builder (0x3063c3)
- base = 0x2d87f0(cx, cy, type) -> [rbp+8]   (`buildingBase`)
- height = 0x25f6a0(type) -> [rbp+0x54]; if !0x25e9a0(type, style, race, culture): height += 2.
- mason M [rbp+0x1680] = 0xba8c0(zone, cx*16, cy*16, 0, 16, 16, rot 0, mirror 0).
- if [rbp-0x34] (i) >= 0 and 0x25ec30(type, style): M.type(+0x74 water) = 10.
- colA [rbp+0x15d0] = 0x2600f0(type, cx, cy, style, race, culture); colB [rbp+0x1658] = 0x25ece0(...).
- roof [rbp+4] = 0x25fff0(type) (table); [rbp+0x38] = -1, or 0x25ea50(style, race, culture) when roof == 1.
- [rbp+0x7c] = roof==1 ? (sub == 4) : roof in {3,4,6,10}.
- [rbp+0xbc] wall pattern: 1 default; roof 3,6,10 -> 4; roof 4,9 -> 2; roof 1: sub 1 -> 2, sub 3,4 -> 2, sub 5 -> 0, sub 6 -> 4.

## Builder body (continued)
- constants in registers: xmm9 = 0, xmm11 = 50, xmm13 = 1.
- colour ops: 0x16fd0 make, 0x172b0 out=b+a, 0x2fa2b0 out=b*a, 0x17730 a+=b, 0x171c0 out=a*k, 0xc7c40 out=k*a, 0x4e070 a*=k, 0x16fb0 copy.
- 0x306587 first scheme (i >= 0 only, else default):
  - type 25: setScheme(colB,0,0, colA+50,0,10, rel)
  - roof 7 or sub in {2,3}: setScheme(colB,6,0, colA,6,10, rel)
  - sub 6 && type != 26: setScheme(colB,7,0, colA,7,10, !rel)
  - roof 8: setScheme(colB*(0.8,1,0.8),1,0, colA,1,10, rel)
  - roof 5: setScheme(colB,1,0, colA,1,25, rel)
  - default: setScheme(colB,1,0, colA,1,10, rel)
- S1 = M.s; S2 = S1 with c0 += 30, c1 += 30 (0x306744).
- isBuilding: types 63-65: setColour(colA,0); z = base-2 (+3 for 63,64); foundations 2x2 at corners not joined (0x31a7f0 of E/N neighbours), pattern rand&1 + 4, M.type 23, box(0,0,base-1,16,16,1).
  roof in {3,6,10} or type in {16,26,50,51,55,57,58,59,61}: foundation(0,0,base-1,16,16,1,0,0).
  else floorTop = min over self and building neighbours of base + 0x2d82c0(cell,0); foundation(0,0,floorTop,16,16,height-floorTop+base,0,0).
- S3 = M.s * 0.75 (all three colours).
- [rbp+0x24] = roof 1 && sub in {4,6}.
- [rbp+0x90] = 0x2af7b0(cx, cy, kind, &anchor); [rbp+0x9c] = {2:0x2e,3:0x2f,4:0x31,5:0x30,6:0x32,7:0x33,8:0x34,9:0x35}[that] else -1.
- [rbp+0x8c] = 5 if sub==4 or roof==4 else 4; roof 3..6, 10 or sub 4: [rbp-0x2c]=7, [rbp+0x124]=4 (else 6, 2). [rbp-0x38]=4; roof 5: [rbp-0x38]=10, [rbp+0x8c]= 0x31a720(c1x,c1y) ? 0x119800(c1x) : 8; roof 4: [rbp-0x38]=6, [rbp+0x8c] = rand()%4==0 && 0x93480(c1x,c1y) not in {10,12} ? 0x12a600(cx,cy) : 5; roof 3,6,9,10: [rbp-0x38]=6, [rbp+0x8c]=-1. type 76/81: -1 then 0x31a770(c1x,c1y) ? 0x12a600(cx,cy); type 100: 0x12a600.
- sub in {0,2} or roof in {7,8}, and 0x25e9a0(type,...): save S4=M.s; colour: sub 0 -> 0x260a00(type,style,race,culture); sub 2 -> setScheme(0.5*255+0.5*colB,0,0, 0.5*255+0.5*colA,0,0,!rel); roof 8 -> (60,50,30); else colA*0.85. S3 = M.s. floors: for k in 0..height/floorH: box(0,0,base-1+k*floorH,16,16,1). corner posts with attached neighbours (0x31ac20). restore S4.
- rails on each side not joined (0x25eb60(type, nbType)) when 0x25e940(type,...): p=[rbp+0xbc]; box + 0xbb170 (x sides) / 0xbaef0 (y sides).
- [rbp+0xac] = 0x25ea00(type).
- rooms: if castle.kind || (isBuilding && floorH): floors = height/floorH; neighbours' types and bases; door side: list of sides without building neighbour (W1,E2,S4,N8) when 0x31abb0(type); pick rand()%n, else rand()%4 -> 1,2,4,8.
- floor loop r14 = castle ? -1 : 0 .. floors-1: (r14>=0 needs 0x2b8b10(cx,cy,r14)); z = r14*floorH + base + 0x2d82c0(cx,cy,r14) (castle: [rbp+0x198]); colours 0x2600f0 (castle 0x2fec70/0x2feb30); rand tint (r3,r2,r1)%150+50, then %100+150; [rbp+0xa4] rand%4==0 for one-floor roof 1/8/9 houses (not 26); rand%5+8 twice.
