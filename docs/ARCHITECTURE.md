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
