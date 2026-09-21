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
