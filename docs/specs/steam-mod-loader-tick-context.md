# Steam loader tick-context provenance

Independent native-only supplement to
[the installed loader ABI §4](steam-mod-loader-capture-abi.md#4-initialization-and-tick-calling-boundary).
Only the original Steam native image, the installed loader binary and existing
independent specifications were inspected. Neither binary was executed. No SDK,
engine, exporter, test, reconstruction or owner capture was used. This identifies
one pointer-producing route; it is not a replacement class declaration.

## 1. Binary and address scope

Game addresses below are RVAs in the analyzed original unpacked AMD64 image,
whose preferred base is 0x140000000. Its SHA-256 is
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This hash identifies that analyzed unpacked image, not every distributed or
modified executable bearing the same game name.

Explicitly labeled loader RVAs refer to the installed CubeModLoader.fip with
SHA-256
`3b0c333bf733d158211c22918fe2dfa9605bae8eb7fe4f7363bcb6b27e59c71c`,
the binary scoped in the existing ABI note. Its bridge at loader 0x24f0 captures
the pre-hook RAX value and passes it to loader dispatcher 0x2340 at loader
0x2516. The dispatcher supplies the same 64-bit value as the callback's second
machine argument, in RDX; the callback object occupies RCX. The bridge does not
derive or validate a World pointer from that value.

## 2. Established normal identity and construction

The hook at game **0x136458** lies in the ordinary startup/input-loop routine
**0x1355a0**. At **0x136440**, RAX receives the eight-byte pointer stored in the
main-module global slot at **RVA 0x551a80**. Thus the normal value is the slot's
contents, not the address of the slot.

That slot is published at **0x13633e** from the result of native constructor
**0x7ea80**. The successful startup allocation requests **0x2288 bytes** at
**0x1362e9–0x1362ee**; **0x136331** invokes the constructor on that allocation.
The constructor returns its input object base. Consequently the normal captured
value denotes the separately allocated native controller object that the
independent game specifications call Game. This name describes its demonstrated
native role; it does not establish an SDK class identity or any SDK layout.

Its **+0x08** field is a separate **64-bit World pointer**, not an inline World.
Startup first constructs that World through **0x29a130 at 0x136163**, retaining
the result at **0x136168**. The same result is passed as the constructor's World
argument at **0x13632b**, and **0x7eade** stores that argument at controller+0x08.
It is the client/rendered World described in
[the independent World-ownership contract §1](steam-local-world-zone-ownership.md#1-bounded-conclusion-and-object-identities).
In contrast, the host member is initialized at controller+0x6e8 through
**0xf17a0 at 0x7eb36**; **0x7eb2b–0x7eb2f** obtain the client World's +0x70
catalog input for that initialization. Those relationships distinguish the
captured controller, its client World and the embedded host; they do not define
the controller's remaining layout.

## 3. Immediate admission and the shutdown exception

Between the global-pointer read and the hook, the game tests the controller's
**byte +0x1d68** at **0x136447**. Constructor **0x7ed17** initializes this byte
to zero. With the byte zero, the branch at **0x13644e** reaches **0x136458**
without another call or replacement of RAX: the loader therefore captures the
controller pointer just described.

With the byte nonzero, **0x136452** calls the imported **USER32 PostQuitMessage**
with argument zero before falling through to the same hook. There is **no reload
of RAX from the global slot** after that call. PostQuitMessage returns no value,
and RAX is a volatile register under the machine calling convention. Its value
after this call is therefore not established as the controller pointer. The
loader bridge forwards the then-current bits; it does not repair this provenance
gap. No particular nonpointer value, preserved pointer or null result is claimed.
The reachability of every producer of the nonzero quit byte is outside this
bounded audit; the native branch itself is present and cannot be silently
excluded from the context contract.

There is also **no null test of the global controller pointer before the byte
dereference at 0x136447**. Startup's allocation-failure branch publishes zero at
**0x13633a–0x13633e**, but this does not establish a supported null-context
callback route: the original game would encounter the dereference first. The
verified normal pointer identity concerns successful completed construction.

The overwritten original span **0x136458–0x13646a** prepares the initial
PeekMessageW arguments; the original API call follows at **0x13646b**. The hook
therefore precedes that queue poll at this cut. It is not, merely by its placement
or callback name, a proved rendered-frame or World-substep boundary.

## 4. Title, World readiness and lifetime limits

The immediate hook admission has no title-versus-play, selected-character,
loaded-zone or client-World-ready test. The controller and its World are
constructed before this loop. A normal context value consequently identifies
the controller even during frontend operation; its presence does not establish
that a saved character has been selected or that a requested playable World or
Zone is ready. Title Continue, existing/New Character selection and Create
Character operate on that controller's existing embedded Interface, as already
bounded in
[the GUI-root lifetime contract §4](steam-gui-root-construction-lifetime.md#4-frontend-to-play-transitions-retain-interface-and-root).
No new controller or World replacement policy is inferred from those UI actions.

The reached quit cleanup in **0x1355a0** reloads the global controller at
**0x137ba5**, tests it for null at **0x137bac**, and at **0x137bc1** invokes its
first virtual entry with second argument **1**. This is a teardown boundary,
not ownership transferred to a plugin by the earlier raw context argument.
Retaining the numerical context value does not establish continued object or
World validity after that boundary. Other replacements, external hooks,
concurrent mutation, recovery and all shutdown producers were not exhaustively
audited.

The resulting bounded guarantee is conditional: **on the successful-construction,
zero-quit-byte path at this exact native hook, the captured 64-bit value is the
published native controller base and its +0x08 field is the separately constructed
client World pointer**. Neither every hook invocation's RAX identity nor any
rendering frequency, simulation cadence, broad object layout or readiness policy
is established beyond the explicit evidence above.
