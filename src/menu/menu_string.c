/* Menu string byte code ($80:8878). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"
#include "lufia2/menu.h"
#include "system/dp_scratch.h"

enum {
    STRING = 0x5du,                     /* [$5D],Y */
    ATTRIBUTE = 0x0564u,                /* tile high byte */
    WIDTH = 0x0565u,
    LEFT = 0x0566u,                     /* characters before the wrap */
    DIGITS = 0x0567u,                   /* 8 characters */
    NUMBER = 0x0570u,                   /* 24-bit */
    ROW = 0x0573u,
    CURSOR = 0x0575u,
    PALETTE = 0x0577u,
    RAW = 0x0578u,                      /* next byte is a plain tile */
    MARK = 0x0579u,
    FORMAT = 0x057bu,
    NAMES = 0x0a80u,                    /* string pointers, bank 0 */
    RECORD_NAME = 0x0b77u,
    ITEM_ICON = 0x0b87u,
};

enum {
    MENU_RELOAD,                        /* $80:88AA: X = cursor */
    MENU_NEXT,                          /* $80:88AD */
    MENU_END,
    MENU_HANDOFF,
};

typedef struct MenuVisit {
    uint32_t pc;
    uint16_t stack;
    uint32_t count;
} MenuVisit;

typedef struct MenuVm {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2ExecutionCheckpoint checkpoint;
    void *checkpoint_context;
    uint32_t handoff;
    uint32_t dispatches;                /* earlier passes, same pc and S */
    MenuVisit visits[64];
    unsigned visit_count;
} MenuVm;

static int MenuRun(MenuVm *vm);

/* Visits per JMP (table,X) and S, for hand-offs. */
static uint32_t MenuVisitCount(MenuVm *vm, uint32_t pc) {
    unsigned i;

    for (i = 0; i < vm->visit_count; ++i)
        if (vm->visits[i].pc == pc && vm->visits[i].stack == vm->cpu->stack)
            return vm->visits[i].count++;
    if (vm->visit_count < 64u) {
        vm->visits[vm->visit_count].pc = pc;
        vm->visits[vm->visit_count].stack = vm->cpu->stack;
        vm->visits[vm->visit_count].count = 1;
        ++vm->visit_count;
    }
    return 0;
}

/* DEY: Y -= 1 with N and Z. */
static void DecrementY(Lufia2CpuState *cpu) {
    cpu->y = (uint16_t)(cpu->y - 1u);
    SetNz16(cpu, cpu->y);
}

/* Word at an absolute address in the current data bank. */
static uint16_t Absolute16(const MenuVm *vm, uint16_t address) {
    return Read16AbsoluteIndexed(vm->memory, vm->cpu, address, 0);
}

/* Write A to an absolute address in the data bank. */
static void StoreAbsolute16(const MenuVm *vm, uint16_t address) {
    Write16Absolute(vm->memory, vm->cpu, address, vm->cpu->accumulator);
}

/* (dp),Y: DB:pointer + Y. */
static uint32_t Indirect(const MenuVm *vm, uint8_t offset, uint16_t index) {
    return (((uint32_t)vm->cpu->data_bank << 16) +
        Read16Direct(vm->memory, vm->cpu, offset) + index) & 0x00ffffffu;
}

/* Byte at (dp),Y. */
static uint8_t Indirect8(const MenuVm *vm, uint8_t offset, uint16_t index) {
    return Read8(vm->memory, Indirect(vm, offset, index));
}

/* Word at (dp),0. */
static uint16_t Indirect16(const MenuVm *vm, uint8_t offset) {
    return Read16Long(vm->memory, Indirect(vm, offset, 0));
}

/* LDA [$5D],Y */
static void StringByte(const MenuVm *vm) {
    LoadA8(vm->cpu, Read8IndirectLongY(vm->memory, vm->cpu, STRING));
}

/* Word at [STRING],Y. */
static uint16_t StringWord(const MenuVm *vm) {
    return Read16IndirectLongY(vm->memory, vm->cpu, STRING);
}

/* Skips a two-byte operand in the string. */
static void SkipWord(Lufia2CpuState *cpu) {
    IncrementY16(cpu);
    IncrementY16(cpu);
}

/* Pushes a JSR frame for return_address. */
static void Jsr(const MenuVm *vm, uint16_t return_address) {
    SimulateJsrFrame(vm->memory, vm->cpu, return_address);
}

/* Pops the frame pushed by Jsr. */
static void Rts(const MenuVm *vm) {
    SimulateRtsFrame(vm->memory, vm->cpu);
}

/* $80:8DB3: one tile; from $CC a two-row glyph. */
static void MenuPutTile(const MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, RAW, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, RAW, 0);               /* 8DCA */
        ExchangeAccumulatorBytes(cpu);
    } else {
        ExchangeAccumulatorBytes(cpu);
        Compare8(cpu, A8(cpu), 0xccu);
        if (cpu->carry) {
            PushIndex(memory, cpu);                            /* 8DD0 */
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
            Sbc8(cpu, 0xcdu);
            TransferAToX(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x808e16u, cpu->x)));
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x808e49u, cpu->x)));
            ExchangeAccumulatorBytes(cpu);
            cpu->x = PullIndexValue(memory, cpu);
            StoreAAbsolute8(memory, cpu, 0x0040u, cpu->x);
            LoadAAbsolute8(memory, cpu, ATTRIBUTE, 0);
            StoreAAbsolute8(memory, cpu, 0x0041u, cpu->x);
            ExchangeAccumulatorBytes(cpu);
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            LoadAAbsolute8(memory, cpu, ATTRIBUTE, 0);
            StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
            IncrementX16(cpu);
            IncrementX16(cpu);
            return;
        }
    }
    StoreAAbsolute8(memory, cpu, 0x0040u, cpu->x);             /* 8DBE */
    LoadAAbsolute8(memory, cpu, ATTRIBUTE, 0);
    StoreAAbsolute8(memory, cpu, 0x0041u, cpu->x);
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* $80:8DF9: next tile row. */
static void MenuNewRow(const MenuVm *vm) {
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Absolute16(vm, ROW));
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    StoreAbsolute16(vm, ROW);
    StoreAbsolute16(vm, CURSOR);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(vm->memory, cpu, RAW, 0);
}

/* $80:8E0F */
static void MenuResetWidth(const MenuVm *vm) {
    LoadAAbsolute8(vm->memory, vm->cpu, WIDTH, 0);
    StoreAAbsolute8(vm->memory, vm->cpu, LEFT, 0);
}

/* $80:88DA: one character; wrap after $0565 of them. */
static void MenuPutChar(const MenuVm *vm) {
    const uint32_t left = AbsoluteIndexedAddress(vm->cpu, LEFT, 0);
    uint8_t value;

    Jsr(vm, 0x88dcu);
    MenuPutTile(vm);
    Rts(vm);
    value = (uint8_t)(Read8(vm->memory, left) - 1u);
    Write8(vm->memory, left, value);
    SetNz8(vm->cpu, value);
    if (value == 0) {
        Jsr(vm, 0x88e4u);
        MenuNewRow(vm);
        Rts(vm);
        Jsr(vm, 0x88e7u);
        MenuNewRow(vm);
        Rts(vm);
    }
}

/* $80:88C8: raw control byte drawn as a tile. */
static void MenuRawControl(const MenuVm *vm) {
    Lufia2CpuState *cpu = vm->cpu;

    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(vm->memory, cpu, RAW, 0);
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        cpu->carry = 0;
        return;
    }
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, Absolute16(vm, CURSOR));
    Jsr(vm, 0x88d4u);
    MenuPutChar(vm);
    Rts(vm);
    cpu->carry = 1;
}

/* $80:8D5D: palette A (0-7) into bits 2-4 of $0564. */
static void MenuPalette(const MenuVm *vm) {
    Lufia2CpuState *cpu = vm->cpu;

    AslA8(cpu);
    AslA8(cpu);
    PushAccumulator8(vm->memory, cpu);
    LoadAAbsolute8(vm->memory, cpu, ATTRIBUTE, 0);
    And8(cpu, 0xe3u);
    Or8(cpu, Read8(vm->memory, (uint16_t)(cpu->stack + 1u)));  /* ORA $01,s */
    StoreAAbsolute8(vm->memory, cpu, ATTRIBUTE, 0);
    LoadA8(cpu, Pull8(vm->memory, cpu));
}

/* $80:8BCA: nested string $62:$60, one row lower. */
static int MenuSubString(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x62u));
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x5du));
    PushAccumulator16(memory, cpu);
    PushY(memory, cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x60u));
    TransferAToY(cpu);
    LoadA16(cpu, Absolute16(vm, CURSOR));
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    TransferAToX(cpu);
    LoadA16(cpu, Absolute16(vm, ROW));
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    Jsr(vm, 0x8becu);
    if (MenuRun(vm))
        return 1;
    Rts(vm);
    SetAccumulatorWidth(cpu, 0);                               /* 8BED */
    SetIndexWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    StoreAbsolute16(vm, ROW);
    cpu->y = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x5du);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, 0x5fu);
    return 0;
}

/* $80:8C6C: 1-3 bytes from ($56) into $0570. */
static void MenuLoadNumber(const MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    StoreZeroAbsolute8(memory, cpu, NUMBER, 0);
    StoreZeroAbsolute8(memory, cpu, (uint16_t)(NUMBER + 1u), 0);
    StoreZeroAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    LoadY16(cpu, 0x0000u);
    do {
        ExchangeAccumulatorBytes(cpu);                         /* 8C7B */
        LoadA8(cpu, Indirect8(vm, 0x56u, cpu->y));
        StoreAAbsolute8(memory, cpu, NUMBER, cpu->y);
        IncrementY16(cpu);
        ExchangeAccumulatorBytes(cpu);
        AslA8(cpu);
    } while (!cpu->carry);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    And8(cpu, 0x04u);
    if (cpu->zero)
        return;
    ExchangeAccumulatorBytes(cpu);
    if (!cpu->negative)
        return;
    for (;;) {                                                 /* 8C90 */
        Compare16(cpu, cpu->y, 0x0003u);
        if (cpu->zero)
            return;
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, NUMBER, cpu->y);
        IncrementY16(cpu);
    }
}

/* $80:89D0 */
static void MenuDigit(const MenuVm *vm) {
    Lufia2CpuState *cpu = vm->cpu;

    cpu->carry = 0;
    Adc8(cpu, 0x30u);
    StoreAAbsolute8(vm->memory, cpu, DIGITS, cpu->x);
    IncrementX16(cpu);
}

/* $80:89AA: $0570 as hex or decimal digits. */
static void MenuDigits(const MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    PushY(memory, cpu);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    And8(cpu, 0x08u);
    if (!cpu->zero) {
        LoadX16(cpu, 0x0002u);
        LoadY16(cpu, 0x0002u);
        do {
            LoadAAbsolute8(memory, cpu, NUMBER, cpu->y);       /* 89B8 */
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            Jsr(vm, 0x89c1u);
            MenuDigit(vm);
            Rts(vm);
            LoadAAbsolute8(memory, cpu, NUMBER, cpu->y);
            And8(cpu, 0x0fu);
            Jsr(vm, 0x89c9u);
            MenuDigit(vm);
            Rts(vm);
            DecrementY(cpu);
        } while (!cpu->negative);
        cpu->y = PullIndexValue(memory, cpu);
        return;
    }
    LoadY16(cpu, 0x0000u);                                     /* 89D8 */
    LoadX16(cpu, 0x0000u);
    LoadAAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
    if (cpu->zero) {
        LoadY16(cpu, 0x0003u);
        LoadX16(cpu, 0x0009u);
        LoadAAbsolute8(memory, cpu, (uint16_t)(NUMBER + 1u), 0);
        if (cpu->zero) {
            LoadY16(cpu, 0x0005u);
            LoadX16(cpu, 0x000fu);
        }
    }
    PushY(memory, cpu);                                        /* 89F4 */
    LoadY16(cpu, 0x0007u);
    LoadA8(cpu, 0x30u);
    do {
        StoreAAbsolute8(memory, cpu, DIGITS, cpu->y);
        DecrementY(cpu);
    } while (!cpu->negative);
    cpu->y = PullIndexValue(memory, cpu);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x808a57u, cpu->x)));
        StoreADirect8(memory, cpu, 0x60u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x808a58u, cpu->x)));
        StoreADirect8(memory, cpu, 0x61u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x808a59u, cpu->x)));
        StoreADirect8(memory, cpu, 0x62u);
        PushY(memory, cpu);
        LoadY16(cpu, 0x0030u);
        for (;;) {                                             /* 8A17 */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Absolute16(vm, NUMBER));
            Subtract16(cpu, Read16Direct(memory, cpu, 0x60u));
            StoreAbsolute16(vm, NUMBER);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
            Sbc8(cpu, DirectByte(memory, cpu, 0x62u));
            if (!cpu->carry)
                break;
            StoreAAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
            IncrementY16(cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* 8A31 */
        LoadA16(cpu, Absolute16(vm, NUMBER));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x60u));
        StoreAbsolute16(vm, NUMBER);
        SetAccumulatorWidth(cpu, 1);
        TransferYToA8(cpu);
        cpu->y = PullIndexValue(memory, cpu);
        StoreAAbsolute8(memory, cpu, DIGITS, cpu->y);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0007u);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, NUMBER, 0);
    cpu->carry = 0;
    Adc8(cpu, 0x30u);
    StoreAAbsolute8(memory, cpu, (uint16_t)(DIGITS + 7u), 0);
    cpu->y = PullIndexValue(memory, cpu);                      /* 8A55 */
}

/* $01: number; format bits 7-5 size, 2 signed, 3 hex. */
static int MenuOpNumber(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    StringByte(vm);                                            /* 88EE */
    StoreAAbsolute8(memory, cpu, FORMAT, 0);
    IncrementY16(cpu);
    StringByte(vm);
    StoreADirect8(memory, cpu, DP_SCRATCH_A);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, DP_SCRATCH_C);
    SkipWord(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    And8(cpu, 0x03u);
    Compare8(cpu, A8(cpu), 0x01u);
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* 890C */
        LoadA16(cpu, Indirect16(vm, 0x56u));
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        SetAccumulatorWidth(cpu, 1);
        StringByte(vm);
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, DP_SCRATCH_C));
        StoreADirect8(memory, cpu, DP_SCRATCH_C);
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_D));
        Adc8(cpu, 0x00u);
        StoreADirect8(memory, cpu, DP_SCRATCH_D);
        IncrementY16(cpu);
    }
    if (vm->checkpoint)
        vm->checkpoint(vm->checkpoint_context, cpu, 0x808922u);
    PushY(memory, cpu);                                        /* 8922 */
    Jsr(vm, 0x8925u);
    MenuLoadNumber(vm);
    Rts(vm);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    And8(cpu, 0x04u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
        if (cpu->negative) {
            StoreZeroAbsolute8(memory, cpu, (uint16_t)(NUMBER + 2u), 0);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, (uint16_t)(Absolute16(vm, NUMBER) ^ 0xffffu));
            IncrementA16(cpu);
            StoreAbsolute16(vm, NUMBER);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, 0xffu);
            StoreADirect8(memory, cpu, DP_SCRATCH_B);
        }
    }
    Jsr(vm, 0x894bu);                                          /* 8949 */
    MenuDigits(vm);
    Rts(vm);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadY16(cpu, 0x0000u);                                 /* blanks */
        do {
            LoadAAbsolute8(memory, cpu, DIGITS, cpu->y);
            Compare8(cpu, A8(cpu), 0x30u);
            if (!cpu->zero)
                break;
            LoadA8(cpu, 0x20u);
            StoreAAbsolute8(memory, cpu, DIGITS, cpu->y);
            IncrementY16(cpu);
            Compare16(cpu, cpu->y, 0x0007u);
        } while (!cpu->zero);
    }
    LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_B)); /* 8968 */
    if (!cpu->zero) {
        LoadY16(cpu, 0x0007u);
        for (;;) {
            LoadAAbsolute8(memory, cpu, DIGITS, cpu->y);
            Compare8(cpu, A8(cpu), 0x20u);
            if (cpu->zero)
                break;
            DecrementY(cpu);
            if (cpu->negative)
                break;
        }
        LoadA8(cpu, 0x2du);                                    /* minus */
        StoreAAbsolute8(memory, cpu, DIGITS, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, FORMAT, 0);                    /* 897E */
    And8(cpu, 0x10u);
    if (!cpu->zero) {
        LoadX16(cpu, DIGITS);
        for (;;) {
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            Compare8(cpu, A8(cpu), 0x20u);
            if (!cpu->zero)
                break;
            IncrementX16(cpu);
        }
    } else {
        LoadA8(cpu, 0x00u);                                    /* 8991 */
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, DP_SCRATCH_A));
        SetAccumulatorWidth(cpu, 0);
        cpu->carry = 0;
        Add16Value(cpu, DIGITS);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    StoreXDirect16(memory, cpu, 0x60u);                        /* 899F */
    Write8(memory, DirectAddress(cpu, 0x62u), 0x00u);
    Jsr(vm, 0x89a5u);
    if (MenuSubString(vm))
        return MENU_HANDOFF;
    Rts(vm);
    cpu->y = PullIndexValue(memory, cpu);
    return MENU_NEXT;
}

/* $80:8BA7: $56 = pointer operand. */
static void MenuPointer(const MenuVm *vm) {
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(vm->memory, cpu, 0x56u);
    SetAccumulatorWidth(cpu, 1);
    SkipWord(cpu);
}

/* $80:8BB2: $56 = (pointer operand) + offset operand. */
static void MenuPointerOffset(const MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, 0x60u);
    SkipWord(cpu);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, DP_SCRATCH_C);
    SkipWord(cpu);
    LoadA16(cpu, Indirect16(vm, 0x60u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
    StoreADirect16(memory, cpu, DP_SCRATCH_C);
    SetAccumulatorWidth(cpu, 1);
}

/* $80:8B3F: member name from the $0A80 table. */
static int MenuMemberName(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Indirect8(vm, 0x56u, 0));
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, NAMES, cpu->x));
    StoreADirect16(memory, cpu, 0x60u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x00u);
    StoreADirect8(memory, cpu, 0x62u);
    Jsr(vm, 0x8b55u);
    if (MenuSubString(vm))
        return 1;
    Rts(vm);
    return 0;
}

/* $80:8B57: name of spell ($56) through $81:F414. */
static int MenuSpellName(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Indirect16(vm, 0x56u));
    StoreAbsolute16(vm, 0x0a0bu);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0x8b64u);
    (void)Lufia2LoadSpellRecord(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    LoadX16(cpu, RECORD_NAME);
    StoreXDirect16(memory, cpu, 0x60u);
    LoadA8(cpu, 0x00u);
    StoreADirect8(memory, cpu, 0x62u);
    Jsr(vm, 0x8b71u);
    if (MenuSubString(vm))
        return 1;
    Rts(vm);
    return 0;
}

/* $80:8B73: icon and name of item ($56) through $81:F1C5. */
static int MenuItemName(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Indirect16(vm, 0x56u));
    StoreAbsolute16(vm, 0x0a06u);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0x8b80u);
    (void)Lufia2LoadItemRecord(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    LoadX16(cpu, RECORD_NAME);
    StoreXDirect16(memory, cpu, 0x60u);
    LoadA8(cpu, 0x00u);
    StoreADirect8(memory, cpu, 0x62u);
    LoadX16(cpu, Absolute16(vm, CURSOR));                      /* 8B8B */
    LoadAAbsolute8(memory, cpu, ITEM_ICON, 0);
    StoreAAbsolute8(memory, cpu, 0x0040u, cpu->x);
    LoadAAbsolute8(memory, cpu, ATTRIBUTE, 0);
    And8(cpu, 0xe0u);
    Or8(cpu, 0x1cu);
    StoreAAbsolute8(memory, cpu, 0x0041u, cpu->x);
    IncrementX16(cpu);
    IncrementX16(cpu);
    Write16Absolute(memory, cpu, CURSOR, cpu->x);
    Jsr(vm, 0x8ba5u);
    if (MenuSubString(vm))
        return 1;
    Rts(vm);
    return 0;
}

/* $02 n: nested string (0-3), member/item/spell name (4-9). */
static int MenuOpString(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;
    unsigned kind;
    int stop = 0;

    LoadA8(cpu, 0x00u);                                        /* 8A6C */
    ExchangeAccumulatorBytes(cpu);
    StringByte(vm);
    StoreAAbsolute8(memory, cpu, FORMAT, 0);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    kind = cpu->x >> 1;
    vm->dispatches = MenuVisitCount(vm, 0x808a7eu);
    if (kind > 9u) {
        vm->handoff = 0x808a7eu;                               /* JMP ($8A81,x) */
        return MENU_HANDOFF;
    }
    switch (kind) {
    case 0:                                                    /* 8A99 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, StringWord(vm));
        StoreADirect16(memory, cpu, 0x60u);
        SkipWord(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
        StoreADirect8(memory, cpu, 0x62u);
        Jsr(vm, 0x8aa9u);
        break;
    case 1:                                                    /* 8AAD */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, StringWord(vm));
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        SkipWord(cpu);
        LoadA16(cpu, Indirect16(vm, 0x56u));
        StoreADirect16(memory, cpu, 0x60u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
        StoreADirect8(memory, cpu, 0x62u);
        Jsr(vm, 0x8ac1u);
        break;
    case 2:                                                    /* 8AC5 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, StringWord(vm));
        StoreADirect16(memory, cpu, 0x60u);
        SkipWord(cpu);
        SetAccumulatorWidth(cpu, 1);
        StringByte(vm);
        StoreADirect8(memory, cpu, 0x62u);
        IncrementY16(cpu);
        Jsr(vm, 0x8ad6u);
        break;
    case 3:                                                    /* 8ADA */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, StringWord(vm));
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        SkipWord(cpu);
        PushY(memory, cpu);
        LoadA16(cpu, Indirect16(vm, 0x56u));
        StoreADirect16(memory, cpu, 0x60u);
        SetAccumulatorWidth(cpu, 1);
        LoadY16(cpu, 0x0002u);
        LoadA8(cpu, Indirect8(vm, 0x56u, cpu->y));
        StoreADirect8(memory, cpu, 0x62u);
        cpu->y = PullIndexValue(memory, cpu);
        Jsr(vm, 0x8af3u);
        break;
    default: {
        static const uint16_t kOperandReturn[6] = {
            0x8b0bu, 0x8b1du, 0x8b2fu, 0x8b14u, 0x8b26u, 0x8b38u};
        const unsigned name = (kind - 4u) % 3u;                /* member, item, spell */

        Jsr(vm, kOperandReturn[kind - 4u]);
        if (kind < 7u)
            MenuPointer(vm);
        else
            MenuPointerOffset(vm);
        Rts(vm);
        Jsr(vm, (uint16_t)(kOperandReturn[kind - 4u] + 3u));
        if (name == 0)
            stop = MenuMemberName(vm);
        else if (name == 1)
            stop = MenuItemName(vm);
        else
            stop = MenuSpellName(vm);
        if (stop)
            return MENU_HANDOFF;
        Rts(vm);
        return MENU_RELOAD;
    }
    }
    if (MenuSubString(vm))
        return MENU_HANDOFF;
    Rts(vm);
    return MENU_NEXT;
}

/* $80:8C9E: branch operands into $54 and $56. */
static void MenuOperands(const MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    LoadA16(cpu, Indirect16(vm, 0x54u));
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    SkipWord(cpu);
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, DP_SCRATCH_C);
    SkipWord(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, FORMAT, 0);
    BitImmediate8(cpu, 0x08u);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* 8CB9 */
        LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_C));
        StoreADirect16(memory, cpu, DP_SCRATCH_A);
        LoadA16(cpu, Indirect16(vm, 0x54u));
        StoreADirect16(memory, cpu, DP_SCRATCH_A);
        LoadA16(cpu, StringWord(vm));
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        SkipWord(cpu);
        LoadA16(cpu, Indirect16(vm, 0x56u));
        cpu->carry = 0;
        Add16Value(cpu, StringWord(vm));
        SkipWord(cpu);
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        LoadA16(cpu, Indirect16(vm, 0x56u));
        StoreADirect16(memory, cpu, DP_SCRATCH_C);
        SetAccumulatorWidth(cpu, 1);
    } else {
        BitImmediate8(cpu, 0x40u);                             /* 8CDD */
        if (cpu->zero) {
            BitImmediate8(cpu, 0x20u);
            if (!cpu->zero) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Indirect16(vm, 0x56u));
                StoreADirect16(memory, cpu, DP_SCRATCH_C);
                SetAccumulatorWidth(cpu, 1);
            }
        }
    }
    LoadAAbsolute8(memory, cpu, FORMAT, 0);                    /* 8CF0 */
    BitImmediate8(cpu, 0x80u);
    if (cpu->zero) {
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_D), 0x00u);
    }
}

/* $03 n: branch if equal, >=, <, or always. */
static int MenuOpBranch(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;
    unsigned kind;
    int take = 1;

    StringByte(vm);                                            /* 8BFD */
    IncrementY16(cpu);
    PushAccumulator8(memory, cpu);
    And8(cpu, 0xf8u);
    StoreAAbsolute8(memory, cpu, FORMAT, 0);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    And8(cpu, 0x07u);
    AslA8(cpu);
    TransferAToX(cpu);
    kind = cpu->x >> 1;
    vm->dispatches = MenuVisitCount(vm, 0x808c0eu);
    if (kind > 3u) {
        vm->handoff = 0x808c0eu;                               /* JMP ($8C11,x) */
        return MENU_HANDOFF;
    }
    if (kind < 3u) {
        static const uint16_t kReturn[3] = {0x8c1bu, 0x8c40u, 0x8c50u};

        Jsr(vm, kReturn[kind]);
        MenuOperands(vm);
        Rts(vm);
        if (kind == 0) {
            int any_bit;

            LoadAAbsolute8(memory, cpu, FORMAT, 0);            /* 8C1C */
            BitImmediate8(cpu, 0x10u);
            any_bit = !cpu->zero;
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
            if (any_bit) {
                const uint16_t mask = Read16Direct(memory, cpu, DP_SCRATCH_C);

                cpu->zero = (cpu->accumulator & mask) == 0;    /* BIT $56 */
                cpu->negative = (mask & 0x8000u) != 0;
                cpu->overflow = (mask & 0x4000u) != 0;
                take = !cpu->zero;
            } else {
                Compare16(cpu, cpu->accumulator,
                          Read16Direct(memory, cpu, DP_SCRATCH_C));
                take = cpu->zero;
            }
            SetAccumulatorWidth(cpu, 1);
        } else {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_SCRATCH_C));
            SetAccumulatorWidth(cpu, 1);
            take = kind == 1u ? cpu->carry : !cpu->carry;
        }
    }
    if (!take) {
        SkipWord(cpu);                                         /* 8C39 */
        return MENU_RELOAD;
    }
    SetAccumulatorWidth(cpu, 0);                               /* 8C5E */
    LoadA16(cpu, StringWord(vm));
    StoreADirect16(memory, cpu, STRING);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0000u);
    return MENU_RELOAD;
}

/* $80:888C: string at $5F:Y; ops $00-$0F, $00 ends. */
static int MenuRun(MenuVm *vm) {
    const Lufia2Memory *memory = vm->memory;
    Lufia2CpuState *cpu = vm->cpu;
    int step = MENU_RELOAD;

    PushDataBank(memory, cpu);                                 /* 888C */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreYDirect16(memory, cpu, STRING);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    Subtract16(cpu, 0x0040u);
    StoreAbsolute16(vm, ROW);
    StoreAbsolute16(vm, CURSOR);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0000u);
    for (;;) {
        if (step == MENU_RELOAD)
            LoadX16(cpu, Absolute16(vm, CURSOR));              /* 88AA */
        for (;;) {
            StringByte(vm);                                    /* 88AD */
            IncrementY16(cpu);
            Compare8(cpu, A8(cpu), 0x10u);
            if (!cpu->carry)
                break;
            Jsr(vm, 0x88b6u);
            MenuPutChar(vm);
            Rts(vm);
        }
        Write16Absolute(memory, cpu, CURSOR, cpu->x);          /* 88B9 */
        Jsr(vm, 0x88beu);
        MenuRawControl(vm);
        Rts(vm);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        AslA8(cpu);
        TransferAToX(cpu);
        vm->dispatches = MenuVisitCount(vm, 0x8088c5u);
        if (cpu->x >= 0x20u) {
            vm->handoff = 0x8088c5u;                           /* JMP ($8E7C,x) */
            return 1;
        }
        switch (cpu->x >> 1) {
        case 0x0:                                              /* 88E9 end */
            LoadX16(cpu, Absolute16(vm, CURSOR));
            PullDataBank(memory, cpu);
            return 0;
        case 0x1:
            step = MenuOpNumber(vm);
            break;
        case 0x2:
            step = MenuOpString(vm);
            break;
        case 0x3:
            step = MenuOpBranch(vm);
            break;
        case 0x4:                                              /* 8CFC */
            StoreA8Absolute(memory, cpu, RAW, 0xffu);
            step = MENU_RELOAD;
            break;
        case 0x5:                                              /* 8D6C mark */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Absolute16(vm, CURSOR));
            StoreAbsolute16(vm, MARK);
            SetAccumulatorWidth(cpu, 1);
            step = MENU_RELOAD;
            break;
        case 0x6:                                              /* 8D79 mark + n */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Absolute16(vm, MARK));
            cpu->carry = 0;
            Add16Value(cpu, StringWord(vm));
            SkipWord(cpu);
            StoreAbsolute16(vm, CURSOR);
            StoreAbsolute16(vm, ROW);
            SetAccumulatorWidth(cpu, 1);
            Jsr(vm, 0x8d8du);
            MenuResetWidth(vm);
            Rts(vm);
            step = MENU_RELOAD;
            break;
        case 0x7:                                              /* 8D91 save palette */
            LoadAAbsolute8(memory, cpu, ATTRIBUTE, 0);
            LsrA8(cpu);
            LsrA8(cpu);
            And8(cpu, 0x07u);
            StoreAAbsolute8(memory, cpu, PALETTE, 0);
            step = MENU_RELOAD;
            break;
        case 0x8:                                              /* 8D9E restore */
            LoadAAbsolute8(memory, cpu, PALETTE, 0);
            Jsr(vm, 0x8da3u);
            MenuPalette(vm);
            Rts(vm);
            step = MENU_RELOAD;
            break;
        case 0x9:                                              /* 8DA7 attribute */
            StringByte(vm);
            StoreAAbsolute8(memory, cpu, ATTRIBUTE, 0);
            IncrementY16(cpu);
            step = MENU_RELOAD;
            break;
        case 0xa:                                              /* 8D04 new line */
            Jsr(vm, 0x8d06u);
            MenuNewRow(vm);
            Rts(vm);
            Jsr(vm, 0x8d09u);
            MenuResetWidth(vm);
            Rts(vm);
            step = MENU_NEXT;
            break;
        case 0xb:                                              /* 8D0D position */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, StringWord(vm));
            SkipWord(cpu);
            StoreAbsolute16(vm, ROW);
            StoreAbsolute16(vm, CURSOR);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            Jsr(vm, 0x8d1eu);
            MenuResetWidth(vm);
            Rts(vm);
            step = MENU_NEXT;
            break;
        case 0xc:                                              /* 8D22 palette n */
            StringByte(vm);
            IncrementY16(cpu);
            Jsr(vm, 0x8d27u);
            MenuPalette(vm);
            Rts(vm);
            step = MENU_RELOAD;
            break;
        case 0xd:                                              /* 8D2B table[(p)] */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, StringWord(vm));
            StoreADirect16(memory, cpu, DP_SCRATCH_C);
            SkipWord(cpu);
            LoadA16(cpu, StringWord(vm));
            StoreADirect16(memory, cpu, DP_SCRATCH_E);
            SkipWord(cpu);
            SetAccumulatorWidth(cpu, 1);
            PushY(memory, cpu);
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Indirect8(vm, 0x58u, 0));
            TransferAToY(cpu);
            LoadA8(cpu, Indirect8(vm, 0x56u, cpu->y));
            Jsr(vm, 0x8d46u);
            MenuPalette(vm);
            Rts(vm);
            cpu->y = PullIndexValue(memory, cpu);
            step = MENU_RELOAD;
            break;
        case 0xe:                                              /* 8D4B (p) */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, StringWord(vm));
            StoreADirect16(memory, cpu, DP_SCRATCH_C);
            SkipWord(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Indirect8(vm, 0x56u, 0));
            Jsr(vm, 0x8d59u);
            MenuPalette(vm);
            Rts(vm);
            step = MENU_RELOAD;
            break;
        default:                                               /* 8DB0 */
            step = MENU_RELOAD;
            break;
        }
        if (step == MENU_HANDOFF)
            return 1;
    }
}

/* $80:8878: 32 characters a line; any width. */
Lufia2ExecutionResult Lufia2MenuDrawStringWithCheckpoint(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2ExecutionCheckpoint checkpoint, void *context) {
    MenuVm vm;

    vm.checkpoint = checkpoint;
    vm.checkpoint_context = context;
    vm.memory = memory;
    vm.cpu = cpu;
    vm.handoff = 0;
    vm.dispatches = 0;
    vm.visit_count = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 8878 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    PushY(memory, cpu);
    LoadA8(cpu, 0x20u);
    StoreAAbsolute8(memory, cpu, WIDTH, 0);
    StoreAAbsolute8(memory, cpu, LEFT, 0);
    Jsr(&vm, 0x8888u);
    if (MenuRun(&vm)) {
        Lufia2ExecutionResult result = ExecutionHandoff(cpu, vm.handoff);

        result.dispatches = vm.dispatches;
        return result;
    }
    Rts(&vm);
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80888bu);
}

Lufia2ExecutionResult Lufia2MenuDrawString(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Lufia2MenuDrawStringWithCheckpoint(memory, cpu, 0, 0);
}
