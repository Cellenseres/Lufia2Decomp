#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    TABLE_REPEAT_FLAGS = 0x4au,
    TABLE_FRAME_FLAGS = 0x72u,
    TABLE_FRAME_REQUEST = 0x74u,
    TABLE_REPEAT_DELAY = (WRAM_BOARD_GAME_LINE_TOTALS + 12u) & 0xffffu
};

Lufia2ExecutionResult Lufia2CardTableWaitFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84e9b1u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e9e2u), false};
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!TableServiceCall(&service, 0x84e9b8u, 0x84e9e3u, false) ||
        !TableServiceCall(&service, 0x84e9bbu, 0x84f034u, false))
        return service.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_FRAME_FLAGS));
    OpLda(memory, cpu, OpDp(cpu, TABLE_FRAME_REQUEST));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_FRAME_REQUEST));
    if (!TableServiceCall(&service, 0x84e9c8u, 0x84a5a8u, false))
        return service.result;
    OpLoadA(cpu, 0x10u);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TABLE_REPEAT_FLAGS)));
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, TABLE_REPEAT_DELAY), -1);
        if (cpu->negative) {
            OpLoadA(cpu, 10u);
            OpSta(memory, cpu, OpAbs(cpu, TABLE_REPEAT_DELAY));
            OpLoadA(cpu, 0x10u);
            OpTestBits(memory, cpu, OpDp(cpu, TABLE_REPEAT_FLAGS), true);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return service.result;
}
