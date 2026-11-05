# Boot NMI wait continuation

`Lufia2BootNmiWaitStep` reconstructs one original operation at `80:8084` or
`80:8086`. The first reads the NMI bank byte through the current direct page;
the second repeats the wait while Z is clear, otherwise continuing at
`80:8088`. It emits a checkpoint before the selected operation. Register
widths, accumulator high byte, flags, data bank, stack and bus order retain
the original behavior. Unsupported PCs, banks or invalid width flags return
without memory access.

The full `Lufia2RunBootSequence` and this continuation share
`BootNmiBankPending`. The full boot loop retains its original reads and
existing continuation budget. This is an internal phase of the existing
boot algorithm, not a new canonical function or algorithm. No metadata entry
or canonical binding is added. Timing and batching belong to the consumer;
the standalone routine removes no waits or accesses.

MSVC and Clang each pass 16,384 original-ROM step comparisons and 1,273
no-access guards. Coverage includes both operations, all M/X combinations,
DP wrapping, varied DB and flags, load results and branch outcomes. CPU,
full WRAM, ordered data accesses and checkpoint state agree. Two altered-ROM
controls per compiler detect operand and branch-target changes. The affected
full boot/reset/field matrix also passes 4,096 cases for each of its three
entries per compiler, including full SRAM and original child-call state.

Strict GCC/Clang standalone builds and actual archive exports pass. The
public C++17 signature is checked. The consumer separately admits the actual
M1X1/DP0 boot context with hardware and callback-ownership guards. Its linked
replay retires 917 instructions through this continuation over 1,800 software
frames while retaining the existing logo wait batches, device/APU clocks,
PCM and frame state. That bounded observation is not a game-wide speed claim.
