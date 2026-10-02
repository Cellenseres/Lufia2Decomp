/* Battle portraits of the party ($81:BAE8). */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    PORTRAITS = 0x153du,                /* portrait id per slot, $FF none */
};

static void SetBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $81:BB75: portrait $153D[$11], pose $12, to $7E slot through WMDATA. */
Lufia2ExecutionResult Lufia2BattlePortraitUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned half, i;

    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xb52cu, cpu->x));
    StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->y);
    Write16Direct(memory, cpu, 0x15u, cpu->y);
    StoreZeroAbsolute8(memory, cpu, SNES_WMADDH, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x12u));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xb534u, cpu->x));
    Write16Direct(memory, cpu, 0x13u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(PORTRAITS, cpu->x)));
    AslA8(cpu);
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(PORTRAITS, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ee5c2u, cpu->x)));
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    IncrementX16(cpu);
    IncrementX16(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8ee5c2u, cpu->x)));
    PushDataBank(memory, cpu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (half = 0; half < 2u; ++half) {
        if (half) {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x15u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0200u);
            StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
        }
        LoadX16(cpu, 0x0100u);
        for (i = 0; i < 0x100u; ++i) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            StoreAAbsolute8(memory, cpu, SNES_WMDATA, 0);
            IncrementY16(cpu);
            LoadX16(cpu, (uint16_t)(cpu->x - 1u));
        }
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bbdfu);
}

/* $81:BAFB: portrait of slot A by status. */
Lufia2ExecutionResult Lufia2BattlePortrait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    StoreADirect16(memory, cpu, 0x00u);
    TransferAToX(cpu);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, PORTRAITS, cpu->x) & 0x00ffu));
    Compare16(cpu, cpu->accumulator, 0x00ffu);
    if (!cpu->zero) {
        uint16_t status;

        TransferXToA(cpu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_PARTY_RECORDS,
            cpu->x));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->x));
        status = cpu->accumulator;
        cpu->zero = (status & 0x0004u) == 0;
        if (cpu->zero)
            cpu->zero = (status & 0x0029u) == 0;
        LoadA16(cpu, (status & 0x0004u) ? 0x0005u : (status & 0x0029u) ? 0x0002u : 0x0000u);
        StoreADirect16(memory, cpu, 0x12u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
        StoreADirect8(memory, cpu, 0x11u);
        SimulateJsrFrame(memory, cpu, 0xbb39u);
        (void)Lufia2BattlePortraitUpload(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        TransferDirectToA(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x01u));
        StoreAAbsolute8(memory, cpu, 0x1547u, cpu->x);
        return ExecutionReturned(0x81bb43u);
    }
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));            /* BB44 */
    AslA16(cpu);
    TransferAToX(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xb52cu, cpu->x));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0x0100u);
    for (i = 0; i < 0x100u; ++i) {
        StoreWordAbsolute(memory, cpu, 0x0000u + cpu->x, 0);
        IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    }
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0100u);
    for (i = 0; i < 0x100u; ++i) {
        StoreZeroAbsolute8(memory, cpu, 0x0000u, cpu->x);
        IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bb74u);
}

/* $81:BAE8: portraits of slots 3 to 0 (DB $97); A kept. */
Lufia2ExecutionResult Lufia2BattlePortraits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushAccumulator8(memory, cpu);
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x97u);
    LoadA8(cpu, 0x03u);
    do {
        PushAccumulator8(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0xbaf3u);
        (void)Lufia2BattlePortrait(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81bafau);
}
