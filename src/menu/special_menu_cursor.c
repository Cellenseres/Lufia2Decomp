#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"

enum {
    CURSOR_DEPTH = WRAM_SPECIAL_MENU_CURSOR_DEPTH & 0xffffu,
    CURSOR_DEFINITION = WRAM_SPECIAL_MENU_CURSOR_DEFINITION & 0xffffu,
    CURSOR_OAM_END = WRAM_SPECIAL_MENU_CURSOR_OAM & 0xffffu,
    CURSOR_RECORDS = 0x6860u,
    CURSOR_COLUMNS = 0x689cu,
    CURSOR_ROWS = 0x68a6u,
    CURSOR_COLUMN_COUNT = 0x68b0u,
    CURSOR_ROW_COUNT = 0x68bau,
    CURSOR_TILE_OFFSET = 0x68c4u,
    CURSOR_SPACING = 0x68ceu,
    CURSOR_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    DP_CURSOR_WORK = 0u,
    DP_CURSOR_TILE_OFFSET = 2u,
    DP_CURSOR_DEFINITION = 8u,
    DP_CURSOR_UPLOAD = 0x72u,
    CURSOR_ITERATION_LIMIT = 4096u
};

static bool CursorReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->decimal;
}

static void LoadDefinitionWord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, CURSOR_DEFINITION));
}

static void AdvanceDefinition(Lufia2CpuState *cpu, unsigned bytes) {
    for (unsigned i = 0u; i < bytes; ++i)
        OpIny(cpu);
}

static void SelectCursorRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CURSOR_DEPTH));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_CURSOR_WORK));
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_CURSOR_WORK));
    OpTax(cpu);
}

static Lufia2ExecutionResult UploadCursor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site) {
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_CURSOR_UPLOAD));
    if (!CallChildWithFrame(memory, cpu, child, context, site, 0x84a5a8u, 2u, 0x84u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return result;
    }
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 |
            (uint16_t)(site + 3u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(site + 4u);
}

Lufia2ExecutionResult Lufia2SpecialMenuLoadCursorDefinition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CursorReady(cpu))
        return ExecutionHandoff(cpu, 0x84a000u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLdy(cpu, 0u);
    OpLdx(cpu, 15u);
    do {
        OpLda(memory, cpu, LongIndexedAddress(
            DirectLongPointer(memory, cpu, DP_CURSOR_DEFINITION), cpu->y));
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_DEFINITION));
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, LongIndexedAddress(
        DirectLongPointer(memory, cpu, DP_CURSOR_DEFINITION), cpu->y));
    OpSta(memory, cpu, OpAbsY(cpu, CURSOR_DEFINITION));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    do {
        AdvanceDefinition(cpu, 2u);
        OpLda(memory, cpu, LongIndexedAddress(
            DirectLongPointer(memory, cpu, DP_CURSOR_DEFINITION), cpu->y));
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_DEFINITION));
        OpDex(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84a029u);
}

Lufia2ExecutionResult Lufia2SpecialMenuPushCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CursorReady(cpu))
        return ExecutionHandoff(cpu, 0x84a02au);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    SelectCursorRecord(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, CURSOR_OAM_END));
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS));
    OpLdy(cpu, 0u);
    static const struct { uint16_t address; uint8_t bytes; } fields[] = {
        {CURSOR_RECORDS + 4u, 2u}, {CURSOR_RECORDS + 6u, 1u},
        {CURSOR_RECORDS + 7u, 2u}, {CURSOR_RECORDS + 9u, 1u},
        {CURSOR_RECORDS + 10u, 2u}
    };
    for (unsigned field = 0u; field < sizeof fields / sizeof fields[0]; ++field) {
        if (fields[field].bytes == 1u) OpSepWidths(cpu, 0x20u);
        else OpRepWidths(cpu, 0x20u);
        LoadDefinitionWord(memory, cpu);
        OpSta(memory, cpu, OpAbsX(cpu, fields[field].address));
        AdvanceDefinition(cpu, fields[field].bytes);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CURSOR_WORK)));
    OpStz(memory, cpu, OpAbsX(cpu, CURSOR_COLUMNS));
    OpStz(memory, cpu, OpAbsX(cpu, CURSOR_ROWS));
    static const uint16_t selections[] = {
        CURSOR_COLUMN_COUNT, CURSOR_ROW_COUNT, CURSOR_TILE_OFFSET, CURSOR_SPACING
    };
    for (unsigned field = 0u; field < 4u; ++field) {
        LoadDefinitionWord(memory, cpu);
        OpSta(memory, cpu, OpAbsX(cpu, selections[field]));
        if (field == 2u)
            OpSta(memory, cpu, OpDp(cpu, DP_CURSOR_TILE_OFFSET));
        AdvanceDefinition(cpu, 2u);
    }
    OpTxa(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_CURSOR_WORK));
    OpTax(cpu);
    LoadDefinitionWord(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_CURSOR_WORK));
    AdvanceDefinition(cpu, 2u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CURSOR_OAM_END)));
    unsigned sprites = 0u;
    do {
        if (sprites++ == CURSOR_ITERATION_LIMIT)
            return ExecutionHandoff(cpu, 0x84a0aeu);
        LoadDefinitionWord(memory, cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, DP_CURSOR_TILE_OFFSET));
        OpSta(memory, cpu, OpAbsX(cpu, CURSOR_OAM));
        OpInx(cpu);
        OpInx(cpu);
        AdvanceDefinition(cpu, 2u);
        LoadDefinitionWord(memory, cpu);
        OpSta(memory, cpu, OpAbsX(cpu, CURSOR_OAM));
        OpInx(cpu);
        OpInx(cpu);
        AdvanceDefinition(cpu, 2u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_CURSOR_WORK), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpWriteX(memory, cpu, OpAbs(cpu, CURSOR_OAM_END), cpu->x);
    OpStepMem(memory, cpu, OpAbs(cpu, CURSOR_DEPTH), 1);
    return UploadCursor(memory, cpu, child, context, 0x84a0d5u);
}

Lufia2ExecutionResult Lufia2SpecialMenuPopCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CursorReady(cpu))
        return ExecutionHandoff(cpu, 0x84a0dau);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpStepMem(memory, cpu, OpAbs(cpu, CURSOR_DEPTH), -1);
    SelectCursorRecord(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, CURSOR_RECORDS + 2u));
    OpTax(cpu);
    do {
        OpLoadA(cpu, 0xf000u);
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        AdvanceDefinition(cpu, 2u);
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpAbsY(cpu, CURSOR_OAM));
        AdvanceDefinition(cpu, 2u);
        OpDex(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return UploadCursor(memory, cpu, child, context, 0x84a10du);
}
