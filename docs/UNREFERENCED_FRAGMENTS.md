# Unreferenced bank 83 fragments

The three scoped boundaries have original-ROM differential evidence. They
have no function metadata or runtime selection: their presence does not prove
that ordinary gameplay reaches them, and the triangle parent entry remains
unresolved. These are reconstructed fragments, not three newly bound functions.

`Lufia2RasterizePreparedTriangle` represents `$83:FD0B..FEC3`, rather than an
invented function entry. It starts with an existing raster header and consumes
the saved data-bank/status pair already on the stack. It does not manufacture
a prologue. The return boundary names the original RTS at `$83:FEC3`.

The fragment prepares row bounds for three coordinate pairs at DP `$16..1B`,
orders the other two vertices, prepares three edges, appends the table tail,
and reconciles crossed adjacent bounds. Original children at `$83:FD9F`,
`$83:FDE7` and `$83:FE44` use the existing `$83:FEC4` edge rasterizer through
their original JSR frames. The asymmetric negative edge counts and inherited
carry in the first row-address calculation are retained.

The current continuation contract requires native mode, PB `$83`, DB `$7F`,
M1X0, DP zero, binary arithmetic, stack `$1F04..1FF8` and a child service.
DP `$F1/$F3/$F4/$F6` holds row storage and prepared extent information. DP zero
initially supplies a header byte and is subsequently reused as the edge
stride. No equality between that header byte and the prepared top row is
assumed. Output addressing retains the original distinction between absolute
parent writes and the child's long indexed writes.

`Lufia2LegacyPreserveRegisters` reconstructs `$83:FF31..FF3D`, including its
stack writes and width restoration. It is not equivalent to an empty RTL.
Its complete 13-byte ROM sequence has no second exact copy in the supported
ROM. No ordinary caller has been established.

`Lufia2LegacyInitializeState` reconstructs `$83:FF3E..FFB9`. The neutral state
fields at `$1360..1385` have no asserted gameplay purpose. The original long
call at `$83:FF7A` targets `$84:B51D`, whose current-ROM path ends in RTS.
The child service must preserve that nonlocal result. The implementation does
not replace it with an assumed long return. A compatible returning service
can also exercise the remaining original state writes; incompatible execution
widths hand off at the exact child continuation. The complete 124-byte region
has no second exact ROM copy.

CPU state and ordered accesses remain explicit because these unreferenced
callers do not supply a proven contract permitting their removal. Structured
phases and shared existing operations replace instruction-address dispatch.
No performance improvement or whole-function contract for the raster parent
is claimed.

## Shared verification

MSVC and Clang each pass 14,256 original-ROM boundary comparisons and 17
no-access admission guards. CPU state, complete WRAM, ordered data/MMIO
accesses, divider state and original child frames agree. Four independent
in-memory ROM mutations are detected by each compiler.

Coverage includes 4,096 register-preservation cases across all entry M/X
modes; 512 executions with the actual legacy child and its nonlocal RTS;
1,024 compatible returning-child cases; 8,192 triangle cases across byte
coordinates and both accepted stack endpoints; 48 child unwinds; and 384
child-context handoffs. Controlled returning services test the original tail
contract, not a claim that the original legacy child returns normally.

The triangle resumes in the child's live program bank after an incompatible
return. Width, DB, DP and decimal changes are checked at each of its three
child boundaries. No guessed caller, clock aggregation, batching hook or
performance improvement is introduced. Existing runtime bindings are unchanged.

Strict standalone GCC and Clang library/CLI builds pass. Both archives contain
all three symbols; the documented CLI `list` command produces identical output.
The public headers also compile as C++17. Two existing missing `<stddef.h>`
includes were corrected in transition setup and menu-string code. No shared
semantic helper or consumer source changed, so unrelated full regression and
runtime performance replays were deferred.
