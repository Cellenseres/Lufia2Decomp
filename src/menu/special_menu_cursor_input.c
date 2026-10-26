#include "menu/card_table_services.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"
#include <stddef.h>

enum {
    CURSOR_RECORDS = 0x6860u,
    CURSOR_COLUMNS = 0x689cu,
    CURSOR_ROWS = 0x68a6u,
    CURSOR_COLUMN_COUNT = 0x68b0u,
    CURSOR_ROW_COUNT = 0x68bau,
    CURSOR_SPACING = 0x68ceu,
    CURSOR_PRODUCT_LEFT = 0x6008u,
    CURSOR_PRODUCT_RIGHT = 0x600au,
    CURSOR_PRODUCT = 0x600cu,
    CURSOR_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    CURSOR_WORK = 0u,
    CURSOR_COMMAND = 2u,
    CURSOR_DISPLACEMENT = 4u,
    CURSOR_SELECTION = 6u,
    CURSOR_TILE_SOURCE = 8u,
    CURSOR_UPLOAD = 0x72u,
    CURSOR_INPUT_ITERATIONS = 4096u
};

typedef enum CursorDirection {
    CURSOR_UP, CURSOR_DOWN, CURSOR_RIGHT, CURSOR_LEFT
} CursorDirection;

static bool CursorCall(TableService *service, uint32_t site,
    uint32_t target, bool normalizes_accumulator) {
    if (!service->child) {
        service->result = ExecutionHandoff(service->cpu, site);
        return false;
    }
    return TableServiceCall(service, site, target, normalizes_accumulator);
}

static void LoadCursorSpacing(TableService *service, uint16_t mask) {
    OpLda(service->memory, service->cpu,
        OpDp(service->cpu, CURSOR_DISPLACEMENT));
    OpAndValue(service->cpu, mask);
}

static void NegateCursorDisplacement(Lufia2CpuState *cpu, bool vertical) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (vertical ? 0xff00u : 0x00ffu)));
    if (vertical) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0100u);
    } else {
        OpIncA(cpu);
    }
}

static bool WrapCursor(TableService *service, CursorDirection direction) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    const bool vertical = direction == CURSOR_UP || direction == CURSOR_DOWN;
    const bool backwards = direction == CURSOR_UP || direction == CURSOR_LEFT;
    const unsigned position = vertical ? CURSOR_ROWS : CURSOR_COLUMNS;
    const unsigned count = vertical ? CURSOR_ROW_COUNT : CURSOR_COLUMN_COUNT;
    static const uint32_t sites[] = {
        0x84a157u, 0x84a189u, 0x84a1c1u, 0x84a1f6u
    };
    if (!backwards)
        OpStz(memory, cpu, OpAbsX(cpu, position));
    OpLda(memory, cpu, OpAbsX(cpu, count));
    OpDecA(cpu);
    if (backwards)
        OpSta(memory, cpu, OpAbsX(cpu, position));
    OpSta(memory, cpu, OpAbs(cpu, CURSOR_PRODUCT_LEFT));
    LoadCursorSpacing(service, vertical ? 0xff00u : 0x00ffu);
    OpSta(memory, cpu, OpAbs(cpu, CURSOR_PRODUCT_RIGHT));
    if (!CursorCall(service, sites[direction], 0x84a5dfu, false))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, CURSOR_PRODUCT));
    if (!backwards)
        NegateCursorDisplacement(cpu, vertical);
    return true;
}

static bool MoveCursorSelection(TableService *service, CursorDirection direction) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    const bool vertical = direction == CURSOR_UP || direction == CURSOR_DOWN;
    const bool backwards = direction == CURSOR_UP || direction == CURSOR_LEFT;
    const unsigned position = vertical ? CURSOR_ROWS : CURSOR_COLUMNS;
    OpLda(memory, cpu, OpAbsX(cpu, position));
    if (backwards) {
        OpDecA(cpu);
        if (cpu->negative)
            return WrapCursor(service, direction);
    } else {
        OpIncA(cpu);
        OpCmpValue(cpu, OpReadM(memory, cpu,
            OpAbsX(cpu, vertical ? CURSOR_ROW_COUNT : CURSOR_COLUMN_COUNT)));
        if (!cpu->negative)
            return WrapCursor(service, direction);
    }
    OpSta(memory, cpu, OpAbsX(cpu, position));
    LoadCursorSpacing(service, vertical ? 0xff00u : 0x00ffu);
    if (backwards)
        NegateCursorDisplacement(cpu, vertical);
    return true;
}

static bool MoveCursorSprites(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpSta(memory, cpu, OpDp(cpu, CURSOR_DISPLACEMENT));
    OpTxa(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, CURSOR_WORK));
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, CURSOR_WORK));
    OpTax(cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS)));
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 2u));
    OpSta(memory, cpu, OpDp(cpu, CURSOR_WORK));
    do {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, CURSOR_DISPLACEMENT));
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        OpIny(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, CURSOR_DISPLACEMENT + 1u));
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        for (unsigned byte = 0u; byte < 3u; ++byte)
            OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, CURSOR_WORK), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 0u);
    return CursorCall(service, 0x84a254u, 0x84a607u, true);
}

static void AnimateCursorSprites(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 11u));
    OpDecA(cpu);
    if (cpu->negative)
        return;
    OpStepMem(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 10u), -1);
    if (!cpu->zero)
        return;
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 11u));
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 10u));
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 6u));
    OpSta(memory, cpu, OpDp(cpu, CURSOR_TILE_SOURCE + 2u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 4u)));
    OpWriteX(memory, cpu, OpDp(cpu, CURSOR_TILE_SOURCE), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 7u)));
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 2u));
    OpSta(memory, cpu, OpDp(cpu, CURSOR_WORK));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, LongIndexedAddress(
        DirectLongPointer(memory, cpu, CURSOR_TILE_SOURCE), cpu->y));
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 7u));
        OpLdy(cpu, 0u);
    }
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS));
    PushIndex(memory, cpu);
    OpTax(cpu);
    OpInx(cpu);
    OpInx(cpu);
    do {
        OpLda(memory, cpu, LongIndexedAddress(
            DirectLongPointer(memory, cpu, CURSOR_TILE_SOURCE), cpu->y));
        OpSta(memory, cpu, OpAbsX(cpu, CURSOR_OAM));
        OpIny(cpu);
        OpIny(cpu);
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, CURSOR_WORK), -1);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 7u));
}

static void InitializeCursorInput(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, CURSOR_SELECTION));
}

static bool PollCursorInput(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpDp(cpu, CURSOR_SELECTION));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_SPACING));
    OpSta(memory, cpu, OpDp(cpu, CURSOR_DISPLACEMENT));
    if (!CursorCall(service, 0x84a124u, 0x849f1au, false))
        return false;
    return true;
}

static bool ApplyCursorInput(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpSta(memory, cpu, OpDp(cpu, CURSOR_COMMAND));
    ExchangeAccumulatorBytes(cpu);
    bool moved = false;
    for (unsigned direction = CURSOR_UP; direction <= CURSOR_LEFT; ++direction) {
        RorA16(cpu);
        if (!cpu->carry)
            continue;
        if (!MoveCursorSelection(service, (CursorDirection)direction))
            return false;
        if (!service->child) {
            service->result = ExecutionHandoff(cpu, 0x84a222u);
            return false;
        }
        if (!MoveCursorSprites(service))
            return false;
        moved = true;
        break;
    }
    if (!moved) {
        OpLda(memory, cpu, OpDp(cpu, CURSOR_COMMAND));
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!CursorCall(service, 0x84a208u, 0x84a607u, false))
                return false;
            OpLoadA(cpu, 0u);
            UnpackStatus(cpu, Pull8(memory, cpu));
            service->result = ExecutionReturned(0x84a2b2u);
            return false;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 1u);
            if (!CursorCall(service, 0x84a219u, 0x84a607u, false))
                return false;
            OpLoadA(cpu, 1u);
            UnpackStatus(cpu, Pull8(memory, cpu));
            service->result = ExecutionReturned(0x84a2b2u);
            return false;
        }
    }
    return true;
}

static bool FinishCursorFrame(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    AnimateCursorSprites(service);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, CURSOR_UPLOAD));
    if (!CursorCall(service, 0x84a2a9u, 0x84a5a8u, true))
        return false;
    OpRepWidths(cpu, 0x20u);
    return true;
}

Lufia2ExecutionResult Lufia2SpecialMenuRunCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, true))
        return ExecutionHandoff(cpu, 0x84a112u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84a2b2u), true};
    InitializeCursorInput(memory, cpu);
    for (unsigned frame = 0u; frame < CURSOR_INPUT_ITERATIONS; ++frame) {
        if (!PollCursorInput(&service) || !ApplyCursorInput(&service) ||
            !FinishCursorFrame(&service))
            return service.result;
    }
    return ExecutionHandoff(cpu, 0x84a11au);
}

Lufia2ExecutionResult Lufia2SpecialMenuCursorPhase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (!TableServiceReady(cpu, true) ||
        (entry != 0x84a112u && cpu->index_is_8_bit) ||
        ((entry == 0x84a127u || entry == 0x84a222u) && cpu->accumulator_is_8_bit))
        return ExecutionHandoff(cpu, entry);
    TableService service = {memory, cpu, NULL, NULL,
        ExecutionHandoff(cpu, entry), true};
    switch (entry) {
    case 0x84a112u:
        InitializeCursorInput(memory, cpu);
        PollCursorInput(&service);
        break;
    case 0x84a127u:
        if (ApplyCursorInput(&service))
            service.result = ExecutionHandoff(cpu, 0x84a257u);
        break;
    case 0x84a222u:
        MoveCursorSprites(&service);
        break;
    case 0x84a257u:
        FinishCursorFrame(&service);
        break;
    case 0x84a2acu:
        OpRepWidths(cpu, 0x20u);
        PollCursorInput(&service);
        break;
    default:
        break;
    }
    return service.result;
}
