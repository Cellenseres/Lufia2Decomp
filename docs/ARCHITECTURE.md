# Architecture

Lufia2Decomp answers one question: what did the original Lufia II SNES program
do? It owns reconstructed semantics, conservative symbols, memory maps, and
function metadata.

The library target `lufia2_decomp` (alias `Lufia2::Decomp`) is portable C. Its
public API must remain independent of SNESRecomp ABI types and desktop/Vita
platform libraries. The consuming Lufia2SNESRecomp repository owns CPU-state
adaptation, generated `hle_func` selection, differential CPU/bus tests,
platform integration, and enhancements.

Only functions whose metadata status is `verified` are eligible for native
selection by a consumer. Consumers independently bind an address to a bridge;
verification without a binding is informational and never activates code.

Partial semantic front-ends may stop at named continuation boundaries when a
whole routine is not reconstructed yet. They can be marked `draft` in symbol
status, but stay out of `metadata/functions.toml` until they describe a whole
replacement candidate with a representable entry/exit ABI.

The actor script virtual machines are reconstructed incrementally as explicit
dispatcher prefixes plus handler semantics. Indirect jump-table words remain
ROM-backed reads through the portable memory callback instead of copied ROM
data. This keeps the standalone source descriptive while preserving the exact
original dispatch behavior, including malformed or currently unseen opcodes.

CPU helpers have one direction of reuse: `core/cpu_internal.h` owns scalar
register, flag, arithmetic, transfer and stack primitives; `core/cpu_ops.h`
adapts them to the live M/X widths and explicit operand addresses. New
instruction adapters should reuse these primitives. The older fixed-width
idioms remain for existing modules; do not introduce competing arithmetic or
status implementations. Memory RMW adapters retain their verified byte-write
order rather than being folded into a superficially similar store helper.
`OpRepWidths` and `OpSepWidths` accept only width masks `$10/$20/$30`;
they are not arbitrary REP/SEP status-mask implementations.

Shared implementation children belong in subsystem internal headers. The T14
section readers and party-list search are declared in `field/field_internal.h`
and `party/party_internal.h`; consumer differential tests explicitly include
those private headers. They do not expand the portable consumer API.
