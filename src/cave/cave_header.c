/* Ancient Cave room records in the generated map header. */

#include "core/cpu_ops.h"
#include "lufia2/ancient_cave.h"
#include "system/wram.h"

enum {
    CAVE_HEADER_COUNT = 0x58,
    CAVE_ROOM_ID = 0x54,
    CAVE_ROOM_COORDINATE_STEP = 0x55,
};

static void CaveRoomHeaderCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, CAVE_ROOM_ID));
    OpAndValue(cpu, 0x000fu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, CAVE_ROOM_COORDINATE_STEP));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, CAVE_ROOM_COORDINATE_STEP));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, CAVE_ROOM_ID));
    OpAndValue(cpu, 0x00f0u);
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0010u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, CAVE_ROOM_COORDINATE_STEP));
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, CAVE_ROOM_COORDINATE_STEP));
}

Lufia2ExecutionResult Lufia2CaveRoomHeaderCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9b46u);
    CaveRoomHeaderCoordinates(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x839b47u);
}

static void CaveHeaderEntrance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf200u));
    OpLda(memory, cpu, 0x7fe6abu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf201u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf203u));
    OpLda(memory, cpu, 0x7fe6acu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf202u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf204u));
    OpLoadA(cpu, 0x00f0u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf205u));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf206u));
    OpSta(memory, cpu, OpAbs(cpu, 0xf207u));
    OpLoadA(cpu, 0x00f0u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf208u));
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbs(cpu, 0xf209u));
}

static void CaveHeaderRoomPoints(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0x0000u);
    OpLda(memory, cpu, WRAM_CAVE_HEADER_ROOM_POINT_COUNT);
    uint8_t skip_first = cpu->zero;
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, CAVE_HEADER_COUNT));
        OpStz(memory, cpu, OpDp(cpu, CAVE_HEADER_COUNT + 1));
        OpLdx(cpu, 0x0000u);
    }
    for (;;) {
        if (!skip_first) {
            OpLda(memory, cpu, OpLongX(cpu, 0x7fe6b1u));
            if (!cpu->zero) {
                OpSta(memory, cpu, OpAbsY(cpu, 0xf601u));
                OpLda(memory, cpu, OpLongX(cpu, 0x7fe6d1u));
                OpSta(memory, cpu, OpAbsY(cpu, 0xf602u));
                OpTxa(cpu);
                OpIncA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, 0xf600u));
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, 0xf603u));
                OpSta(memory, cpu, OpAbsY(cpu, 0xf604u));
                OpLoadA(cpu, 0x00ffu);
                OpSta(memory, cpu, OpAbsY(cpu, 0xf605u));
                OpSta(memory, cpu, OpAbsY(cpu, 0xf606u));
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, 0xf607u));
                OpRepWidths(cpu, 0x20u);
                OpTya(cpu);
                cpu->carry = 0;
                OpAdcValue(cpu, 0x0008u);
                OpTay(cpu);
                OpSepWidths(cpu, 0x20u);
            }
        }
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_HEADER_COUNT)));
        if (cpu->carry)
            break;
        /* The zero-count branch joins here with the caller's X and count. */
        skip_first = 0;
    }
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf600u));
}

static void CaveHeaderRoomArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf400u));
    OpLda(memory, cpu, OpLongX(cpu, 0x7fe6f1u));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0010u);
    SimulateJslFrame(memory, cpu, 0x8eu, 0xb8e8u);
    (void)Lufia2CaveRoomHeaderCoordinates(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf402u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf406u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf408u));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf404u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf40au));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf40cu));
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf401u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf405u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf409u));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf403u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf407u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xf40bu));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf40du));
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf40eu));
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x000fu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void CaveHeaderObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0x0000u);
    OpLdx(cpu, 0x0000u);
    for (;;) {
        OpTxa(cpu);
        OpCmp(memory, cpu, WRAM_CAVE_HEADER_OBJECT_COUNT);
        if (cpu->carry)
            break;
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0xf800u));
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe736u));
        OpSta(memory, cpu, OpAbsY(cpu, 0xf801u));
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe73eu));
        OpSta(memory, cpu, OpAbsY(cpu, 0xf802u));
        OpPushX(memory, cpu);
        TransferDirectToA(cpu);
        OpTxa(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe747u));
        OpPullX(memory, cpu);
        OpAndValue(cpu, 0x0040u);
        if (!cpu->zero)
            OpLoadA(cpu, 0x0001u);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0xf803u));
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0004u);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpInx(cpu);
    }
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf800u));
}

Lufia2ExecutionResult Lufia2CaveBuildMapHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    CaveHeaderEntrance(memory, cpu);
    CaveHeaderRoomPoints(memory, cpu);
    OpLdy(cpu, 0x0000u);
    OpLdx(cpu, 0x0000u);
    unsigned rooms = 0;
    for (;;) {
        if (rooms++ == 0x10000u)
            return ExecutionHandoff(cpu, 0x8eb8d0u);
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe6f1u));
        if (!cpu->zero) {
            OpCmpValue(cpu, 0x00ffu);
            if (cpu->zero)
                break;
            CaveHeaderRoomArea(memory, cpu);
        }
        OpInx(cpu);
    }
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xf400u));
    CaveHeaderObjects(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0200u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf006u));
    OpLoadA(cpu, 0x0400u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf002u));
    OpLoadA(cpu, 0x0600u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf010u));
    OpLoadA(cpu, 0x0800u);
    OpSta(memory, cpu, OpAbs(cpu, 0xf026u));
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8eb992u);
}
