# Installed Steam mod loader: independently observed capture ABI

This contract is derived only from the installed original loader binary, not
CWSDK headers/source, an existing exporter, either engine or captures. The
binary was read, not executed. No implementation or replacement class is
provided. RVAs below are within this loader, whose preferred image base is
0x180000000; they are not Steam game RVAs unless expressly labeled.

## 1. Exact binary scope

| Identity | Observed value |
| --- | --- |
| Inspected file | D:/SteamLibrary/steamapps/common/Cube World/CubeModLoader.fip |
| Size |145408 bytes|
| SHA-256 |`3b0c333bf733d158211c22918fe2dfa9605bae8eb7fe4f7363bcb6b27e59c71c`|
| PE machine / optional-header type |AMD64 0x8664 / PE32+ 0x20b|
| Internal export-library name |CubeModLoader.dll|
| Loader's own ModMajorVersion |7, returned by0x15500|
| Loader's own ModMinorVersion |3, returned by0x15510|

This establishes one installed binary advertising7.3, not compatibility with
every loader called7.3 or a separately identified upstream release commit.
No SDK license grant follows from these ABI observations.

## 2. DLL discovery, exports and initialization order

The DLL route in discovery0xe090 enumerates Mods\\*.dll and loads modules
through the Windows library-loader path. For each discovered DLL it resolves
these exact, case-sensitive export names with GetProcAddress:

| Export | Loader use |
| --- | --- |
|ModMajorVersion|Callable function; low32 return consumed as signed version integer.|
|ModMinorVersion|Callable function; low32 return consumed as signed version integer.|
|ModPreInitialize|Callable function; no input argument is deliberately supplied; return ignored.|
|MakeMod|Callable function; no input argument is deliberately supplied; RAX is the returned object pointer.|

Resolution/storage is at0xe3e0–0xe43d. There is no alternate decorated name
or missing-export default on this route. Missing any required export enters
the error path0xe86d–0xe930 and ultimately calls CRT exit with status1 at
0xe9cb–0xe9d0. A data symbol in place of a callable version/factory export
does not match the observed invocation.

Version checking0xe460–0xe480 requires major exactly7 and tests minor as a
signed32 integer less than4. Thus normal nonnegative versions7.0–7.3 pass.
The machine comparison does not additionally reject a negative minor.
Unsupported versions enter the fatal error paths0xe935–0xe9d0 rather than
silently skipping the module. A version declaration is not an inspection
of the returned object's actual layout.

The loader then performs its module-list/configuration processing and removes
entries whose loader-record byte+0x58 is zero. Export/version validation
precedes this filtering. For each remaining entry, in retained-vector order,
it calls ModPreInitialize and then MakeMod, storing the result. These calls
are at0xe5e3 and0xe5e6. After **all** those factories have run, a separate
pass0xe610–0xe623 invokes initialization on each returned object.

The discovery/initialization routine is reached synchronously from the inline
bridge0xf610, whose call to0xe090 is at0xf633. The calls above run on that
bridge's executing thread; no separate dispatch thread is introduced by the
two factory/initialization loops. Frequency and the full game's admission of
that initialization bridge were not proved from this binary-only scope.
The later legacy Mods\\*.cwmod route is separate and is not a DLL-adapter
factory contract supplied by this note.

## 3. Returned object, vptr, priorities and lifetime

MakeMod's returned pointer is stored at **loader module-record+0x50**. The
initialization pass loads that pointer into RCX, reads its first qword as a
vptr and calls **vptr+0x00**, at0xe613–0xe61a. No other argument is deliberately
supplied and the return is ignored. This slot is the reached initialization
operation, not a reserved destructor slot. The pass has no null-object,
null-vptr or null-initializer fallback.

Ordinary event dispatch uses the same returned object pointer as RCX. It
reads **one-byte priority fields at object+0x08 through object+0x24**. For
each event, the dispatcher visits priority values0,1,2,3,4 in that order;
within a pass it walks the retained module vector forward and calls only
objects whose corresponding byte equals the current value. Values outside
those five do not match a pass. These are byte equality tests, not signed
integer ranges or a four-byte enum read. Constructor/default priority names
and values are not inferred from SDK conventions.

The object-field reads are a lower-bound ABI observation: an eight-byte
vptr followed by29 individually addressed bytes must be readable when these
dispatchers reach them. They do **not** prove a full C++ object size, tail
padding, further members, RTTI layout or constructor. The loader does not
initialize those priority bytes in the reached factory/initialization loop.
The highest reached event call is vptr+0xe8: the complete observed positional
range is30 eight-byte entries through that offset, totaling0xf0 bytes of
function-pointer positions when all these events are admitted. A two-entry
initialization/tick table is not the observed general dispatch contract.

The loader retains the raw returned pointer for later callbacks. Its
loader-record fields are distinct from the plugin object's fields:

| Loader-owned record offset | Meaning on this route |
| --- | --- |
|+0x28|Loaded Windows module handle.|
|+0x30|Resolved ModPreInitialize function pointer.|
|+0x38 / +0x40|Resolved major/minor version function pointers.|
|+0x48|Resolved MakeMod function pointer.|
|+0x50|Factory-returned object pointer.|
|+0x58|Loader-side inclusion byte tested before factory calls.|

The reached retained-vector cleanup0x1760 frees its pointer-array allocation;
it does not call a plugin-object destructor or free each factory result.
Neither this cleanup nor the load pass establishes a promised ownership
transfer through deletion. Object storage and callback code must remain
valid throughout their later uses. Other unload/reload/termination paths,
DLL lifetime policies and recovery after a plugin failure remain unproved.

## 4. Initialization and tick calling boundary

The observed calls follow the Microsoft Windows x64 machine calling
convention. Member callbacks receive the object in RCX. Subsequent integer
or pointer-shaped arguments occupy RDX, R8 and R9; a fifth total argument
uses stack space. The inline bridges align the call stack and provide the
ordinary shadow space. No x86 thiscall/stdcall stack-pop convention or
mangled-name discovery is involved.

The tick event dispatcher is **0x2340**, using **vptr+0x20** and the priority
byte **object+0x0b**. Its bridge begins at0x24f0, takes the pre-hook RAX value
as the context passed to0x2340, and calls that dispatcher at0x2516. The
dispatcher supplies that same context as RDX to the plugin, with RCX=this.
The event callback return is ignored. The dispatcher has its own first-use
setup before reaching the event passes; that setup is not a plugin-object
constructor or a priority initializer.

Installer0x2560 selects **main-module base+0x136458** as the bridge site and
stores **main-module base+0x13646b** as the continuation. Address addition is
through0x1bd0. This is the directly proved main-module hook location; it is
not a direct call from game0xa5a50 or a proved World-substep callback.
The bridge and plugin call execute synchronously on the thread reaching
that game site. This audit does not infer a rendered-frame, input-poll or
simulation frequency by inspecting another binary. The context is kept as
an opaque game-context pointer here; its full pointed-to layout is not an
ABI fact recovered by this note.

## 5. Complete reached event slot/register census

Every row receives RCX=this. Q means a64-bit general-register argument with
pointed-to type/ownership unspecified here; U32 means the low32 value
deliberately supplied in that register. Out pointers below are separate
mutable caller state, not callback return values. No by-value floating
argument is established by these call windows. A numeric slot is retained
where a semantic callback name was not independently proved.

| Vptr byte offset | Priority byte at object+ | Dispatcher RVA | Deliberately supplied event arguments | Return handling |
| --- | --- | --- | --- | --- |
|0x08|0x08|0x18c0|RDX=Q|First nonzero EAX stops dispatch; dispatcher reports1.|
|0x10|0x09|0x1eb0|RDX=Q, R8=Q|First nonzero EAX stops dispatch and is retained.|
|0x18|0x0a|0x1be0|RDX=Q|First nonzero EAX stops dispatch and is retained.|
|0x20|0x0b|0x2340|RDX=tick context from bridge RAX|Ignored.|
|0x28|0x0c|0x25b0|RDX=Q|Ignored.|
|0x30|0x0d|0x27e0|RDX=Q|Ignored.|
|0x38|0x0e|0x2a10|RDX=Q, R8=U32, R9=Q, one stacked Q|First nonzero EAX stops dispatch and is retained.|
|0x40|0x0f|0x2cd0|RDX=Q|Ignored.|
|0x48|0x10|0x2f10|RDX=Q|Ignored.|
|0x50|0x11|0x3150|RDX=Q, R8=U32, R9=U32|Ignored.|
|0x58|0x12|0xc1f0|RDX=Q, R8=Q|Ignored.|
|0x60|0x13|0x3390|RDX=Q, R8=Q|Ignored.|
|0x68|0x14|0x35a0|RDX=Q, R8=Q|Ignored.|
|0x70|0x15|0x37b0|RDX=Q, R8=Q|Ignored.|
|0x78|0x16|0x39c0|RDX=Q, R8=Q|Ignored.|
|0x80|0x17|0x3c20|RDX=Q, R8=Q|Ignored.|
|0x88|0x18|0x3e30|RDX=Q, R8=Q|Ignored.|
|0x90|0x19|0x4060|RDX=Q, R8=Q|Ignored.|
|0x98|0x1a|0x4270|RDX=Q, R8=Q|Ignored.|
|0xa0|0x1b|0x44d0|RDX=Q|Ignored.|
|0xa8|0x1c|0x4710|RDX=Q|Ignored.|
|0xb0|0x1d|0xc550 /0xc780 /0xc9c0|RDX=Q, R8=Q, R9=U32 discriminator0/1/2 respectively|Ignored.|
|0xb8|0x1e|0xcbf0|RDX=Q, R8=Q, R9=Q|Ignored.|
|0xc0|0x1f|0xd360|RDX=Q|Ignored.|
|0xc8|0x20|0xd5b0|RDX=Q, R8=Q|Ignored.|
|0xd0|0x21|0xd7b0|RDX=Q, R8=Q|Ignored.|
|0xd8|0x22|0xce00|RDX=Q, R8=Q, R9=byte out-state pointer|Ignored.|
|0xe0|0x23|0xda20|RDX=Q, R8=signed32 out-state pointer|Ignored.|
|0xe8|0x24|0xdc40|RDX=Q, R8=U32, R9=byte out-state pointer|Ignored.|

For slot0x38, the stacked Q is at caller RSP+0x20 immediately before CALL,
hence callee-entry RSP+0x28 after the return address is pushed. This is an
argument-position fact, not a source-level SDK class declaration.

## 6. Neutral-result and mutable-state obligations

For the integer-return-controlled rows0x08,0x10,0x18 and0x38, **EAX zero**
means continue looking through later matching objects/priorities. If none
returns nonzero, their dispatcher result remains zero. An unspecified
integer return could therefore intercept an event; it is not equivalent
to ignoring an event. This audit does not assign every possible nonzero
value the same game meaning merely because it ends the dispatch.

The remaining rows do not inspect the callback return in these dispatchers.
Some carry mutable values by address, so return handling alone is not their
whole contract. In particular:

* 0xce00 initializes its byte out-state from the result of its own ordinary
  helper0x16040 before the priority passes. It later converts the retained byte
  to its final boolean result. A callback need not supply a fresh baseline.
* 0xda20 initializes its admitted signed32 out-state to100 before invoking
  slot0xe0, then consumes the resulting state. Its separate non-admitted path
  does not create an event obligation for that callback.
* 0xdc40 computes its byte out-state before invoking slot0xe8, then consumes
  the resulting byte. Its preceding classification is outside this minimum
  callback ABI; no universal true/false baseline is substituted here.

Other opaque argument pointers can likewise carry the caller's existing
mutable state. Their complete record schemas and ownership are not
established merely by register position. Ignoring a result is distinct
from overwriting pointed-to state with zero. No generic reset of output
buffers or guessed default item/creature record belongs to this ABI note.

## 7. Compatibility boundary

The binary establishes callable exports, version acceptance, raw object/vptr
and priority reads, initialization/tick routing, the full reached numeric
event-dispatch census and the return/state distinctions above. It provides
enough evidence to describe that machine protocol without importing an SDK
implementation. It does not prove a complete SDK C++ class, plugin constructor,
all semantic event names, all pointed-to record layouts, exception recovery,
concurrent registration, unload ownership or compatibility with a different
loader/game binary. Those unclosed facts must not be filled by a truncated
class inferred from older examples. No adapter was implemented or executed.
