/* Ancient Cave room records in the generated map header. */

#include "cave/wram.h"
#include "core/cpu_ops.h"
#include "lufia2/ancient_cave.h"
#include "system/wram.h"

/* Generated map header sections, each ended by $FF. */
enum {
    CAVE_HEADER_ENTRANCE = 0xf200u,    /* one ten-byte entry at the stairs */
    CAVE_HEADER_ROOM_AREAS = 0xf400u,  /* fifteen bytes per linked room */
    CAVE_HEADER_ROOM_POINTS = 0xf600u, /* eight bytes per room point */
    CAVE_HEADER_OBJECTS = 0xf800u,     /* four bytes per chest */
    CAVE_HEADER_OBJECT_FLAGS =
        0x7fe747u, /* word per chest; bit 6 picks the kind byte */
};

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
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE));
    OpLda(memory, cpu, CAVE_STAIR_COLUMN_LONG);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 1));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 3));
    OpLda(memory, cpu, CAVE_STAIR_ROW_LONG);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 2));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 4));
    OpLoadA(cpu, 0x00f0u);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 5));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 6));
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 7));
    OpLoadA(cpu, 0x00f0u);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 8));
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbs(cpu, CAVE_HEADER_ENTRANCE + 9));
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
            OpLda(memory, cpu, OpLongX(cpu, CAVE_OBJECT_COLUMNS_LONG));
            if (!cpu->zero) {
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 1));
                OpLda(memory, cpu, OpLongX(cpu, CAVE_OBJECT_ROWS_LONG));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 2));
                OpTxa(cpu);
                OpIncA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS));
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 3));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 4));
                OpLoadA(cpu, 0x00ffu);
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 5));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 6));
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS + 7));
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
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_POINTS));
}

static void CaveHeaderRoomArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS));
    OpLda(memory, cpu, OpLongX(cpu, (0x7f0000u | CAVE_LINKS)));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0010u);
    SimulateJslFrame(memory, cpu, 0x8eu, 0xb8e8u);
    (void)Lufia2CaveRoomHeaderCoordinates(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 2));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 6));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 8));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 4));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 10));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 12));
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 1));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 5));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 9));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 3));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 7));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 11));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 13));
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS + 14));
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
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_OBJECTS));
        OpLda(memory, cpu, OpLongX(cpu, CAVE_CHEST_COLUMNS_LONG));
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_OBJECTS + 1));
        OpLda(memory, cpu, OpLongX(cpu, CAVE_CHEST_ROWS_LONG));
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_OBJECTS + 2));
        OpPushX(memory, cpu);
        TransferDirectToA(cpu);
        OpTxa(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, CAVE_HEADER_OBJECT_FLAGS));
        OpPullX(memory, cpu);
        OpAndValue(cpu, 0x0040u);
        if (!cpu->zero)
            OpLoadA(cpu, 0x0001u);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_OBJECTS + 3));
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0004u);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpInx(cpu);
    }
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_OBJECTS));
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
        OpLda(memory, cpu, OpLongX(cpu, (0x7f0000u | CAVE_LINKS)));
        if (!cpu->zero) {
            OpCmpValue(cpu, 0x00ffu);
            if (cpu->zero)
                break;
            CaveHeaderRoomArea(memory, cpu);
        }
        OpInx(cpu);
    }
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_HEADER_ROOM_AREAS));
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
