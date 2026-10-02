/* Battle damage popups ($81:C35F). */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    SLOTS = 0x7ff60cu,                  /* 11 slots, $1E bytes */
};

static void Jsl81(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t ret) {
    SimulateJslFrame(memory, cpu, 0x81u, ret);
}

static void SetBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $81:FBC6: A = sprite of character A. */
static void CharacterSprite(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x9685c0u, cpu->x)));
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x9685cfu, cpu->x)));
    cpu->x = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $81:FB8E: A = size of sprite A - 1, $1F1F masked. */
static void SpriteSize(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushIndex(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        DecrementA8(cpu);
    else
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x97ca64u, cpu->x)));
    And16(cpu, 0x1f1fu);
    cpu->x = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* Party member A: sprite size of its character. */
static void MemberSize(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t sprite_return, uint16_t size_return) {
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_ENEMY_RECORDS, cpu->x));
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0050u, cpu->x));
    Jsl81(memory, cpu, sprite_return);
    CharacterSprite(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    Jsl81(memory, cpu, size_return);
    SpriteSize(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* Record A * factor + base; $00/$01 from its $05/$07. */
static void PositionFrom(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t base, uint8_t factor, int add_offsets) {
    Write8(memory, 0x004202u, A8(cpu));
    LoadA8(cpu, factor);
    Write8(memory, 0x004203u, factor);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, base);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x004216u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x00u);
    if (add_offsets) {
        cpu->carry = 0;
        LoadAAbsolute8(memory, cpu, 0x0005u, cpu->x);
        Adc8(cpu, DirectByte(memory, cpu, 0x00u));
        StoreADirect8(memory, cpu, 0x00u);
        cpu->carry = 0;
        LoadAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        Adc8(cpu, DirectByte(memory, cpu, 0x01u));
        StoreADirect8(memory, cpu, 0x01u);
    } else {
        LoadA8(cpu, 0x10u);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x0005u, cpu->x));
        StoreADirect8(memory, cpu, 0x00u);
        LoadA8(cpu, 0x10u);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x0007u, cpu->x));
        StoreADirect8(memory, cpu, 0x01u);
    }
    PullDataBank(memory, cpu);
}

/* $81:B80A: popup position $00/$01 of target A (bit 7: party). */
static void TargetPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        And8(cpu, 0x7fu);
        Compare8(cpu, A8(cpu), 0x04u);
        if (!cpu->zero) {
            PositionFrom(memory, cpu, 0x1399u, 0x0du, 0);
            return;
        }
        LoadA8(cpu, 0x18u);                                    /* B842 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, 0x0013d2u));
        StoreADirect8(memory, cpu, 0x00u);
        LoadA8(cpu, 0x18u);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, 0x0013d4u));
        StoreADirect8(memory, cpu, 0x01u);
        return;
    }
    TransferAToX(cpu);                                         /* B855 */
    LoadAAbsolute8(memory, cpu, 0x11deu, 0);
    if (!cpu->zero) {
        LoadX16(cpu, 0x3c98u);
        StoreXDirect16(memory, cpu, 0x00u);
        return;
    }
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x007fu);
    PushAccumulator16(memory, cpu);
    MemberSize(memory, cpu, 0xb874u, 0xb878u);
    AslA16(cpu);
    AslA16(cpu);
    And16(cpu, 0xfcfcu);
    StoreADirect16(memory, cpu, 0x00u);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    PositionFrom(memory, cpu, 0x13dau, 0x0fu, 1);
}

/* $81:B7D9: popup size $00/$01 of target A. */
static void TargetSize(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        Compare8(cpu, A8(cpu), 0x84u);
        LoadA8(cpu, cpu->zero ? 0x06u : 0x04u);
        StoreADirect8(memory, cpu, 0x00u);
        StoreADirect8(memory, cpu, 0x01u);
        return;
    }
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    MemberSize(memory, cpu, 0xb800u, 0xb804u);
    StoreADirect16(memory, cpu, 0x00u);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:C4E3: signed value $15 as up to four digits. */
static void PopupDigits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned i;

    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->carry)
        Sbc8(cpu, 0x02u);
    StoreAAbsolute8(memory, cpu, 0x4f0fu, cpu->y);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x15u));
    if (!cpu->negative) {
        StoreADirect16(memory, cpu, 0x15u);
        LoadA16(cpu, 0x0001u);
        StoreADirect16(memory, cpu, 0x17u);
    } else {
        LoadA16(cpu, (uint16_t)((cpu->accumulator ^ 0xffffu) + 1u));
        StoreADirect16(memory, cpu, 0x15u);
        Write16Direct(memory, cpu, 0x17u, 0);
    }
    SetAccumulatorWidth(cpu, 1);                               /* C503 */
    LoadX16(cpu, cpu->y);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x81u);                                 /* PHK PLB */
    PullDataBank(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x17u));
    Write8(memory, LongIndexedAddress(0x7e4f14u, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x15u));
    Compare16(cpu, cpu->accumulator, 0x2710u);
    if (cpu->carry)
        LoadA16(cpu, 0x270fu);                                 /* 9999 */
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 4u; ++i) {
        StoreWordAbsolute(memory, cpu, SNES_WRDIVL, cpu->y);
        LoadA8(cpu, 0x0au);
        StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
        PushIndex(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        PushAccumulator8(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        LoadAAbsolute8(memory, cpu, SNES_RDMPYL, 0);
        Write8(memory, LongIndexedAddress(0x7e4f13u - i, cpu->x), A8(cpu));
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDDIVL, 0));
        if (cpu->zero)
            break;
    }
    StoreAAbsolute8(memory, cpu, 0x4f13u, cpu->y);             /* DB $81 */
    LoadA8(cpu, 0x00u);
    StoreAAbsolute8(memory, cpu, 0x4f14u, cpu->y);
    PullDataBank(memory, cpu);
}

/* $81:C477: popup record at $1C for slot $12, kind $13. */
static void BuildPopup(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t kind;

    LoadY16(cpu, Read16Direct(memory, cpu, 0x1cu));
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x12u));
    TransferAToX(cpu);
    LoadA8(cpu, (uint8_t)(Read8(memory, LongIndexedAddress(0x95ffedu, cpu->x)) + 1u));
    StoreAAbsolute8(memory, cpu, 0x4f15u, cpu->y);
    DecrementA8(cpu);
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0x80u));
    StoreADirect8(memory, cpu, 0x1bu);
    SimulateJsrFrame(memory, cpu, 0xc48cu);
    TargetPosition(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    StoreAAbsolute8(memory, cpu, 0x4f0bu, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x01u));
    StoreAAbsolute8(memory, cpu, 0x4f0cu, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x1bu));
    SimulateJsrFrame(memory, cpu, 0xc49bu);
    TargetSize(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    StoreAAbsolute8(memory, cpu, 0x4f0du, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x01u));
    StoreAAbsolute8(memory, cpu, 0x4f0eu, cpu->y);
    TransferDirectToA(cpu);
    StoreAAbsolute8(memory, cpu, 0x4f14u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4f0fu, cpu->y);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x4f10u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4f11u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4f12u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x13u));
    kind = A8(cpu);
    if (kind < 0x0bu) {
        PopupDigits(memory, cpu);
        return;
    }
    Compare8(cpu, kind, kind == 0x10u ? 0x10u : 0x0du);
    if (kind > 0x0du && kind != 0x10u)
        return;
    LoadA8(cpu, kind == 0x10u ? 0x0cu : kind == 0x0du ? 0x0au : 0x0bu);
    StoreAAbsolute8(memory, cpu, 0x4f13u, cpu->y);
}

/* Scan slot words from $19 for the first nonzero one. */
static void ScanValues(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
    for (;;) {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(SLOTS + 4u, cpu->x)));
        if (!cpu->zero)
            break;
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x13u);
        if (cpu->zero)
            break;
    }
    StoreADirect16(memory, cpu, 0x15u);
    LoadA16(cpu, 0x000du);
    Subtract16(cpu, Read16Direct(memory, cpu, 0x13u));
    StoreADirect16(memory, cpu, 0x13u);
}

/* $81:C3EE: value $15 and kind $13 of slot $19, then its popup. */
static void SlotValue(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t mode;

    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x15u);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(SLOTS + 2u, cpu->x)));
    if (A8(cpu) == 0x08u) {
        LoadA8(cpu, 0x10u);                                    /* C46D */
        StoreADirect8(memory, cpu, 0x13u);
    } else {
        LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
        mode = A8(cpu);
        if (mode == 0x01u) {                                   /* C453 */
            uint8_t type;

            LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
            LoadA8(cpu, Read8(memory, LongIndexedAddress(SLOTS, cpu->x)));
            type = A8(cpu);
            Compare8(cpu, type, type == 0x03u ? 0x03u : 0x02u);
            if (type != 0x03u && type != 0x02u)
                return;
            LoadA8(cpu, 0x0eu);
            StoreADirect8(memory, cpu, 0x13u);
        } else if (mode == 0u || mode == 0x03u || mode == 0x04u) {
            SetAccumulatorWidth(cpu, 0);                       /* C401 */
            LoadA16(cpu, 0x000du);
            StoreADirect16(memory, cpu, 0x13u);
            ScanValues(memory, cpu);
        } else {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(SLOTS + 2u, cpu->x)));   /* C432 */
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x85a6c6u, cpu->x)));
            And16(cpu, 0x00ffu);
            StoreADirect16(memory, cpu, 0x13u);
            AslA16(cpu);
            Add16Value(cpu, Read16Direct(memory, cpu, 0x19u));
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(SLOTS + 4u, cpu->x)));
            if (cpu->zero)
                ScanValues(memory, cpu);                       /* C408 */
            else
                StoreADirect16(memory, cpu, 0x15u);
        }
    }
    SetAccumulatorWidth(cpu, 1);                               /* C471 */
    SimulateJsrFrame(memory, cpu, 0xc475u);
    BuildPopup(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Slot type against popup mode $11. */
static int ShowsSlot(uint8_t type, uint8_t mode) {
    switch (type) {
    case 0x80u: return mode != 0x01u && mode != 0x02u;
    case 0x04u: return mode != 0x01u && mode != 0x02u && mode != 0x03u;
    case 0x03u: return mode == 0x01u;
    case 0x02u: return mode == 0x02u;
    default: return 0;
    }
}

/* $81:C35F: popups at $7E:4F0B of the slots matching mode A. */
Lufia2ExecutionResult Lufia2BattlePopups(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x11u);
    SetBank(memory, cpu, 0x7eu);
    LoadA8(cpu, 0x0bu);
    StoreADirect8(memory, cpu, 0x12u);
    StoreZeroAbsolute8(memory, cpu, 0x4f0au, 0);
    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x1cu);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(SLOTS, cpu->x)));
        if (ShowsSlot(A8(cpu), DirectByte(memory, cpu, 0x11u))) {
            PushIndex(memory, cpu);                            /* C3C1 */
            StoreXDirect16(memory, cpu, 0x19u);
            SimulateJsrFrame(memory, cpu, 0xc3c6u);
            SlotValue(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x1cu));
            cpu->carry = 0;
            Add16Value(cpu, 0x000bu);
            StoreADirect16(memory, cpu, 0x1cu);
            StoreWordAbsolute(memory, cpu, 0x4f0au,
                (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x4f0au, 0) + 1u));
            cpu->x = PullIndexValue(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* C3D5 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x001eu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, 0x12u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81c3edu);
}
