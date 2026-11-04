# Owned inventory and conversation captures

The extractor uses the native layouts in the
[inventory contract section1](specs/steam-inventory-slot-lifecycle.md) and the
[source-only conversation audit](specs/steam-conversation-generation-protection.md).
These adapters remove SDK container access while retaining the existing exported
file formats. They do not establish complete SDK independence or live acceptance.

Inventory capture copies ordered tabs and complete 164-byte slots: a signed
four-byte quantity followed by the 160-byte item. The item is copied into an owned
value, so a C++ alignment requirement cannot insert padding into the native slot.
Empty slots, negative quantities, duplicate items, attributes and padding remain
capture data. Six tabs is the native default, not an imposed capture length.
Malformed, unreadable, changed or over-budget vectors publish no partial inventory.
The capture policy permits 256 tabs and 262,144 total slots; these are tool budgets.
Metadata rechecks do not pin buffers or prove an atomic in-place item snapshot.

Gear surveys plan from an owned inventory. Empty slots are excluded from trials.
Before applying a queued trial, the tool verifies its current physical slot and
exact captured item bytes. Missing, empty or changed slots are skipped; unavailable
capture defers the trial. This conservative identity check is distinct from native
semantic item equality. Equipment changes remain a recording aid. The caller's
World guard covers local actor access during survey work. Loot diagnostics retain
their last valid inventory baseline on failed reads, sum signed quantities in
64 bits and track signed gold separately from baseline availability.

Conversation generation follows the observed native client caller's World actor
guard. Each native result is consumed and disposed through the original destructor
while that scope remains held. A temporary day override is restored before further
work, including C++ exception unwinding. Cleanup inside a failing native generator
before it returns is not established. The result decoder copies 32-byte string
records, handling inline and heap representations, explicit lengths and embedded
NUL bytes. It rejects changed/unreadable/bounded malformed metadata without partial
tokens. Its per-result budgets are 16,384 tokens and 8 MiB of text.

The formatting phase uses owned actor metadata and tokens after the capture
lease ends. Native name/key enrichment now has its own observed caller guard
interval; its owning text results are disposed before file formatting. Only
owned strings reach output. See [text ownership](extractor-generated-text.md).
It retains no actor pointers or native result allocations. Capture batches are bounded
to 256 people, 262,144 tokens and 32 MiB of text. Completed people are marked exported
only after successful output and flushing; deferred people remain eligible for
later capture. The 64-day window uses defined signed32 wrapping.

Native generation can initialize runtime state and change CRT random state. It is
not a read-only World query; reference collection belongs in an isolated session.
The known native caller guard is not a universal lifetime pin, complete transitive
lock analysis or guarantee against every external writer. New code and fixture
checks do not substitute for supported-game live validation.

Tests cover physical widths, alignment, order/empty/negative values, bounded nested
ranges and metadata changes. Mock ABI callbacks check result disposal and day
restoration during normal consumption and exceptions. Sparse text fixtures cover
inline/heap boundaries and unavailable/malformed/changed results without invoking
the original game. Broader acceptance and remaining SDK fields stay open on the
[independence workboard](extractor-independence.md).
