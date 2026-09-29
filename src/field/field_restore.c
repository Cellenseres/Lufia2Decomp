/* Field display restoration at $83:B062. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"

Lufia2ExecutionResult Lufia2FieldRestore(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    PushAccumulator8(memory, cpu);                         /* B062 */
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    Push8(memory, cpu, 0x83u);                             /* PHK */
    PullDataBank(memory, cpu);
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, 0xbf00u);
    OpLdy(cpu, 0x0420u);
    LoadA16(cpu, 0x00ffu);
    OpMoveNext(memory, cpu, 0x00u, 0xa6u);
    OpLdx(cpu, 0xfb0du);
    OpLdy(cpu, 0x0320u);
    LoadA16(cpu, 0x001fu);
    OpMoveNext(memory, cpu, 0x00u, 0xa6u);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);                             /* DB = $83 */
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, 0u);
    OpSta(memory, cpu, 0x7fd4f8u);
    OpSta(memory, cpu, 0x7fd59au);
    static const uint16_t zero_words[] = {
        0x1246u, 0x1248u, 0x124au, 0x124cu,
        0x123eu, 0x1240u, 0x1242u, 0x1244u, 0x125du};
    for (unsigned i = 0; i < sizeof(zero_words) / sizeof(zero_words[0]); ++i)
        OpSta(memory, cpu, OpAbs(cpu, zero_words[i]));
    OpLdx(cpu, 14u);
    OpLdy(cpu, 0u);
    do {
        TransferDirectToA(cpu);
        OpStz(memory, cpu, OpAbsX(cpu, 0x05c2u));
        OpSta(memory, cpu, OpLongX(cpu, 0x7fd55au));
        OpSta(memory, cpu, OpLongX(cpu, 0x7fd57au));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, 0x09a9u), 0u);
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x73u));
    OpStz(memory, cpu, OpDp(cpu, 0x72u));
    OpStz(memory, cpu, OpDp(cpu, 0x81u));
    OpStz(memory, cpu, OpAbs(cpu, 0x420cu));
    OpStz(memory, cpu, OpDp(cpu, 0x74u));
    OpStz(memory, cpu, OpAbs(cpu, 0x09aau));
    OpStz(memory, cpu, OpAbs(cpu, 0x1209u));
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x71u));
    OpStz(memory, cpu, OpDp(cpu, 0x6au));
    OpLdx(cpu, 0x9fa9u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x68u), cpu->x);      /* Field NMI target */
    LoadA8(cpu, 0x83u);
    OpSta(memory, cpu, OpDp(cpu, 0x6au));                  /* publish bank */

    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0xb15eu));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) break;
        OpSta(memory, cpu, OpDp(cpu, 0x5du));
        OpLda(memory, cpu, OpAbsX(cpu, 0xb15fu));
        OpSta(memory, cpu, OpDp(cpu, 0x5eu));
        OpInx(cpu);
        OpInx(cpu);
        OpLdy(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, OpAbsX(cpu, 0xb15eu));
            OpInx(cpu);
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero) break;
            OpSta(memory, cpu, OpAbsY(cpu, OpRead16(memory, OpDp(cpu, 0x5du))));
            OpIny(cpu);
        }
    }

    OpLdx(cpu, 6u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0x210du));
        OpStz(memory, cpu, OpAbsX(cpu, 0x210du));
        OpStz(memory, cpu, OpAbsX(cpu, 0x210eu));
        OpStz(memory, cpu, OpAbsX(cpu, 0x210eu));
        OpStz(memory, cpu, OpAbsX(cpu, 0x0594u));
        OpStz(memory, cpu, OpAbsX(cpu, 0x0595u));
        OpStz(memory, cpu, OpAbsX(cpu, 0x0596u));
        OpStz(memory, cpu, OpAbsX(cpu, 0x0597u));
        OpDex(cpu);
    } while (!cpu->negative);
    LoadA8(cpu, 0x33u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2123u));
    OpSta(memory, cpu, OpAbs(cpu, 0x2124u));
    OpSta(memory, cpu, OpAbs(cpu, 0x2125u));
    OpStz(memory, cpu, OpAbs(cpu, 0x212au));
    OpStz(memory, cpu, OpAbs(cpu, 0x212bu));
    LoadA8(cpu, 8u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2126u));
    LoadA8(cpu, 0xf7u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2127u));
    LoadA8(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x212eu));
    OpSta(memory, cpu, OpAbs(cpu, 0x212fu));

    SimulateJslFrame(memory, cpu, 0x83u, 0xb157u);
    if (!child(child_context, cpu, 0x83b007u, 0x83b154u, 3u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x83b154u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83b15du);
}
