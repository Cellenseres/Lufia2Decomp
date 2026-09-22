#include "lufia2/actor_frontend.h"

static uint8_t Read8(
    const Lufia2ActorFrontendMemory *memory, uint32_t address) {
    return memory->read_byte(memory->context, address & 0x00ffffffu);
}

static void Write8(
    const Lufia2ActorFrontendMemory *memory,
    uint32_t address,
    uint8_t value) {
    memory->write_byte(memory->context, address & 0x00ffffffu, value);
}

static uint32_t DirectAddress(
    const Lufia2ActorFrontendCpu *cpu, uint8_t offset) {
    return (uint16_t)(cpu->direct_page + offset);
}

static uint32_t AbsoluteIndexedAddress(
    const Lufia2ActorFrontendCpu *cpu, uint16_t address, uint16_t index) {
    return ((uint32_t)cpu->data_bank << 16) |
           (uint16_t)(address + index);
}

static uint32_t LongIndexedAddress(uint32_t address, uint16_t index) {
    return (address + index) & 0x00ffffffu;
}

static uint32_t ProgramAddress(
    const Lufia2ActorFrontendCpu *cpu, uint16_t address) {
    return ((uint32_t)cpu->program_bank << 16) | address;
}

static uint16_t Read16Direct(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    return (uint16_t)(
        Read8(memory, address) |
        ((uint16_t)Read8(memory, (uint16_t)(address + 1u)) << 8));
}

static void Write16Direct(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset,
    uint16_t value) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    Write8(memory, address, (uint8_t)value);
    Write8(memory, (uint16_t)(address + 1u), (uint8_t)(value >> 8));
}

static uint16_t Read16Long(
    const Lufia2ActorFrontendMemory *memory, uint32_t address) {
    return (uint16_t)(
        Read8(memory, address) |
        ((uint16_t)Read8(memory, (address + 1u) & 0x00ffffffu) << 8));
}

static uint16_t Read16ProgramIndexed(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t base,
    uint16_t index) {
    const uint16_t address = (uint16_t)(base + index);
    return (uint16_t)(
        Read8(memory, ProgramAddress(cpu, address)) |
        ((uint16_t)Read8(
             memory, ProgramAddress(cpu, (uint16_t)(address + 1u))) << 8));
}

static void SetNz8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->negative = (value & 0x80u) != 0;
    cpu->zero = value == 0;
}

static void SetNz16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    cpu->negative = (value & 0x8000u) != 0;
    cpu->zero = value == 0;
}

static uint8_t A8(const Lufia2ActorFrontendCpu *cpu) {
    return (uint8_t)cpu->accumulator;
}

static void LoadA8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | value);
    SetNz8(cpu, value);
}

static void LoadA16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    cpu->accumulator = value;
    SetNz16(cpu, value);
}

static void LoadX16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    cpu->x = value;
    SetNz16(cpu, value);
}

static void LoadY16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    cpu->y = value;
    SetNz16(cpu, value);
}

static void And8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    LoadA8(cpu, (uint8_t)(A8(cpu) & value));
}

static void AslA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t old = A8(cpu);
    cpu->carry = (old & 0x80u) != 0;
    LoadA8(cpu, (uint8_t)(old << 1));
}

static void LsrA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t old = A8(cpu);
    cpu->carry = old & 1u;
    LoadA8(cpu, (uint8_t)(old >> 1));
}

static void TransferAToX(Lufia2ActorFrontendCpu *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->x = A8(cpu);
        SetNz8(cpu, (uint8_t)cpu->x);
    } else {
        cpu->x = cpu->accumulator;
        SetNz16(cpu, cpu->x);
    }
}

static void TransferAToY(Lufia2ActorFrontendCpu *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->y = A8(cpu);
        SetNz8(cpu, (uint8_t)cpu->y);
    } else {
        cpu->y = cpu->accumulator;
        SetNz16(cpu, cpu->y);
    }
}

static void TransferXToA(Lufia2ActorFrontendCpu *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, (uint8_t)cpu->x);
    else
        LoadA16(cpu, cpu->x);
}

static void TransferDirectToA(Lufia2ActorFrontendCpu *cpu) {
    cpu->accumulator = cpu->direct_page;
    SetNz16(cpu, cpu->accumulator);
}

static void SetIndexWidth(Lufia2ActorFrontendCpu *cpu, uint8_t narrow) {
    cpu->index_is_8_bit = narrow != 0;
    if (narrow) {
        cpu->x &= 0x00ffu;
        cpu->y &= 0x00ffu;
    }
}

static void SetAccumulatorWidth(
    Lufia2ActorFrontendCpu *cpu, uint8_t narrow) {
    cpu->accumulator_is_8_bit = narrow != 0;
}

static void Push8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t value) {
    Write8(memory, cpu->stack, value);
    cpu->stack = (uint16_t)(cpu->stack - 1u);
}

static uint8_t Pull8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    cpu->stack = (uint16_t)(cpu->stack + 1u);
    return Read8(memory, cpu->stack);
}

static void PushDataBank(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Push8(memory, cpu, cpu->data_bank);
}

static void PushAccumulator8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Push8(memory, cpu, A8(cpu));
}

static void PullDataBank(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
}

static void LoadXDirect16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadX16(cpu, Read16Direct(memory, cpu, offset));
}

static void LoadYDirect16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadY16(cpu, Read16Direct(memory, cpu, offset));
}

static void StoreXDirect16(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->x);
}

static void StoreYDirect16(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->y);
}

static void TrbDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint32_t address = DirectAddress(cpu, offset);
    const uint8_t value = Read8(memory, address);
    const uint8_t a = A8(cpu);
    cpu->zero = (value & a) == 0;
    Write8(memory, address, (uint8_t)(value & (uint8_t)~a));
}

static uint8_t LoadScriptByteY(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint8_t value = Read8(
        memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y));
    LoadA8(cpu, value);
    return value;
}

static uint8_t LoadScriptByteX(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint8_t value = Read8(
        memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x));
    LoadA8(cpu, value);
    return value;
}

static uint32_t JumpProgramTable(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t base) {
    const uint16_t pc = Read16ProgramIndexed(memory, cpu, base, cpu->x);
    return ((uint32_t)cpu->program_bank << 16) | pc;
}

Lufia2ActorScriptDispatchResult Lufia2ActorPrimaryScriptDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint16_t actor_record;

    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x)));
    And8(cpu, 0xebu);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x), A8(cpu));
    PushDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);
    LoadXDirect16(memory, cpu, 0xabu);
    actor_record = cpu->x;

    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe508u, actor_record)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe506u, actor_record)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadYDirect16(memory, cpu, 0x2au);
    StoreYDirect16(memory, cpu, 0x2au);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteY(memory, cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    result.handler_pc = JumpProgramTable(memory, cpu, 0xd467u);
    return result;
}

Lufia2ActorScriptDispatchResult Lufia2ActorSecondaryScriptDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint16_t actor_record;
    uint32_t first_handler;

    TransferXToA(cpu);
    if (cpu->zero) {
        LoadA8(cpu, 0xc0u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            TrbDirect8(memory, cpu, 0x4bu);
            if (!cpu->zero)
                Write8(memory, 0x7fd0aeu, A8(cpu));
        }
    }

    SetIndexWidth(cpu, 0);
    PushDataBank(memory, cpu);
    LoadXDirect16(memory, cpu, 0xabu);
    actor_record = cpu->x;
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe3f0u, actor_record)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe3eeu, actor_record)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect16(memory, cpu, 0x2au);
    StoreXDirect16(memory, cpu, 0x2au);
    LoadYDirect16(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first_handler = JumpProgramTable(memory, cpu, 0xdf17u);

    if (first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5d4u) ||
        first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5e0u) ||
        first_handler == (((uint32_t)cpu->program_bank << 16) | 0xd5ecu)) {
        uint16_t table;

        LoadXDirect16(memory, cpu, 0x2au);
        (void)LoadScriptByteX(memory, cpu);
        And8(cpu, 0x0fu);
        AslA8(cpu);
        TransferAToX(cpu);

        if ((uint16_t)first_handler == 0xd5d4u)
            table = 0xdf37u;
        else if ((uint16_t)first_handler == 0xd5e0u)
            table = 0xdf57u;
        else
            table = 0xdf77u;
        result.handler_pc = JumpProgramTable(memory, cpu, table);
    } else {
        result.handler_pc = first_handler;
    }

    return result;
}


static uint16_t Read16AbsoluteIndexed(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    const uint16_t effective = (uint16_t)(address + index);
    const uint32_t low =
        ((uint32_t)cpu->data_bank << 16) | effective;
    const uint32_t high =
        ((uint32_t)cpu->data_bank << 16) |
        (uint16_t)(effective + 1u);
    return (uint16_t)(
        Read8(memory, low) |
        ((uint16_t)Read8(memory, high) << 8));
}

static void Write16Long(
    const Lufia2ActorFrontendMemory *memory,
    uint32_t address,
    uint16_t value) {
    Write8(memory, address, (uint8_t)value);
    Write8(memory, (address + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
}

static void Or8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    LoadA8(cpu, (uint8_t)(A8(cpu) | value));
}

static void Add16Immediate(
    Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    const uint32_t sum =
        (uint32_t)cpu->accumulator + value + (cpu->carry ? 1u : 0u);
    cpu->accumulator = (uint16_t)sum;
    cpu->carry = sum > 0xffffu;
    SetNz16(cpu, cpu->accumulator);
}

static void IncrementY16(Lufia2ActorFrontendCpu *cpu) {
    cpu->y = (uint16_t)(cpu->y + 1u);
    SetNz16(cpu, cpu->y);
}

static void BitImmediate8(
    Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->zero = (A8(cpu) & value) == 0;
}

static void Compare8(
    Lufia2ActorFrontendCpu *cpu, uint8_t left, uint8_t right) {
    cpu->carry = left >= right;
    SetNz8(cpu, (uint8_t)(left - right));
}

static void Adc8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t old = A8(cpu);
    const uint16_t sum =
        (uint16_t)old + value + (cpu->carry ? 1u : 0u);
    cpu->carry = sum > 0xffu;
    LoadA8(cpu, (uint8_t)sum);
}

static void Sbc8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t old = A8(cpu);
    const uint16_t borrow = cpu->carry ? 0u : 1u;
    const uint16_t rhs = (uint16_t)value + borrow;
    cpu->carry = (uint16_t)old >= rhs;
    LoadA8(cpu, (uint8_t)((uint16_t)old - rhs));
}

static void SimulateJsrFrame(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    Push8(memory, cpu, (uint8_t)(return_address >> 8));
    Push8(memory, cpu, (uint8_t)return_address);
}

static void SimulateRtsFrame(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    (void)Pull8(memory, cpu);
    (void)Pull8(memory, cpu);
}

static void PrimaryIntervalCheck(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);

    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu)); /* $83:CCC1 */
    cpu->carry = 0;                                     /* $83:CCC3 */
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
                                                         /* $83:CCC4 */
    Compare8(
        cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x56u)));
                                                         /* $83:CCC6 */
    if (!cpu->carry) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
                                                         /* $83:CCCA */
        cpu->carry = 0;                                 /* $83:CCCC */
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
                                                         /* $83:CCCD */
        Compare8(
            cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x56u)));
                                                         /* $83:CCCF */
        cpu->carry = cpu->carry ? 0u : 1u;             /* CLC/SEC */
    } else {
        cpu->carry = 1;                                /* $83:CCD5 SEC */
    }

    SimulateRtsFrame(memory, cpu);
}

static Lufia2ActorScriptDispatchResult PrimaryRedispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t reload_y) {
    Lufia2ActorScriptDispatchResult result;

    if (reload_y)
        LoadYDirect16(memory, cpu, 0x2au);            /* $83:C85A */
    StoreYDirect16(memory, cpu, 0x2au);               /* $83:C85C */
    TransferDirectToA(cpu);                           /* $83:C85E */
    result.opcode = LoadScriptByteY(memory, cpu);     /* $83:C85F */
    AslA8(cpu);                                       /* $83:C862 */
    TransferAToX(cpu);                                /* $83:C863 */
    result.handler_pc =
        JumpProgramTable(memory, cpu, 0xd467u);       /* $83:C864 */
    return result;
}

static Lufia2ActorPrimaryScriptStepResult PrimaryStepRedispatched(
    Lufia2ActorScriptDispatchResult dispatch) {
    Lufia2ActorPrimaryScriptStepResult result;
    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_REDISPATCHED;
    result.opcode = dispatch.opcode;
    result.handler_pc = dispatch.handler_pc;
    return result;
}

Lufia2ActorPrimaryScriptStepResult
Lufia2ActorPrimaryScriptExecuteKnownHandler(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t handler_pc) {
    Lufia2ActorPrimaryScriptStepResult result;
    Lufia2ActorScriptDispatchResult dispatch;

    result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_UNKNOWN_HANDLER;
    result.opcode = 0;
    result.handler_pc = handler_pc & 0x00ffffffu;

    switch (handler_pc & 0x00ffffffu) {

    case 0x83cc85u:
        IncrementY16(cpu);                             /* $83:CC85 */
        LoadXDirect16(memory, cpu, 0xa7u);             /* $83:CC86 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06bau, 0)));
                                                        /* $83:CC88 */
        Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
                                                        /* $83:CC8B */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
                                                        /* $83:CC8D */
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
                                                        /* $83:CC90 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
                                                        /* $83:CC92 */
        PrimaryIntervalCheck(memory, cpu, 0xcc97u);    /* JSR $CCC1 */
        if (!cpu->carry)
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CC9A */
        IncrementY16(cpu);                             /* $83:CC9D */
        IncrementY16(cpu);                             /* $83:CC9E */
        IncrementY16(cpu);                             /* $83:CC9F */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83cca3u:
        IncrementY16(cpu);                             /* $83:CCA3 */
        LoadXDirect16(memory, cpu, 0xa7u);             /* $83:CCA4 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06e2u, 0)));
                                                        /* $83:CCA6 */
        Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
                                                        /* $83:CCA9 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
                                                        /* $83:CCAB */
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
                                                        /* $83:CCAE */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
                                                        /* $83:CCB0 */
        PrimaryIntervalCheck(memory, cpu, 0xccb5u);    /* JSR $CCC1 */
        if (!cpu->carry)
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CCB8 */
        IncrementY16(cpu);                             /* $83:CCBB */
        IncrementY16(cpu);                             /* $83:CCBC */
        IncrementY16(cpu);                             /* $83:CCBD */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d14du:
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x09a1u, 0)));
                                                        /* $83:D14D */
        if (!cpu->negative)                            /* $83:D150 */
            goto d14d_commit;

        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x09a6u, 0)));
                                                        /* $83:D152 */
        BitImmediate8(cpu, 0x01u);                    /* $83:D155 */
        if (!cpu->zero)                               /* $83:D157 */
            goto d14d_commit;

        LoadXDirect16(memory, cpu, 0xa7u);             /* $83:D159 */
        TransferDirectToA(cpu);                       /* $83:D15B */
        LoadA8(
            cpu, Read8(
                memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
                                                        /* $83:D15C */
        TransferAToX(cpu);                            /* $83:D160 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x09a1u, cpu->x)));
                                                        /* $83:D161 */
        if (!cpu->negative) {                         /* $83:D164 */
            result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_D166;
            result.handler_pc = 0x83d166u;
            return result;
        }

d14d_commit:
        IncrementY16(cpu);                            /* $83:D172 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                 /* $83:D173 */

    case 0x83c8c7u:
        LoadXDirect16(memory, cpu, 0xabu);             /* $83:C8C7 */
        SetAccumulatorWidth(cpu, 0);                   /* $83:C8C9 */
        LoadA16(cpu, cpu->y);                          /* $83:C8CB TYA */
        Write16Long(
            memory, LongIndexedAddress(0x7fe506u, cpu->x),
            cpu->accumulator);                         /* $83:C8CC */
        SetAccumulatorWidth(cpu, 1);                   /* $83:C8D0 */
        result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
        result.handler_pc = 0x83c8d2u;
        return result;

    case 0x83d2b4u:
        SetAccumulatorWidth(cpu, 0);                   /* $83:D2B4 */
        LoadA16(
            cpu, Read16AbsoluteIndexed(
                memory, cpu, 0x0001u, cpu->y));        /* $83:D2B6 */
        cpu->carry = 0;                                /* $83:D2B9 CLC */
        Add16Immediate(cpu, 0xa1d4u);                  /* $83:D2BA */
        Write16Direct(
            memory, cpu, 0x2au, cpu->accumulator);    /* $83:D2BD */
        SetAccumulatorWidth(cpu, 1);                   /* $83:D2BF */
        dispatch = PrimaryRedispatch(memory, cpu, 1);  /* $83:C85A */
        return PrimaryStepRedispatched(dispatch);


    case 0x83d2bdu:
        Write8(memory, DirectAddress(cpu, 0x2au), A8(cpu));
                                                        /* $83:D2BD */
        SetAccumulatorWidth(cpu, 1);                   /* $83:D2BF */
        dispatch = PrimaryRedispatch(memory, cpu, 1);  /* $83:C85A */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d2c4u:
        LoadXDirect16(memory, cpu, 0xa7u);             /* $83:D2C4 */
        LoadA8(
            cpu, Read8(
                memory,
                AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:D2C6 */
        Or8(
            cpu,
            Read8(memory, LongIndexedAddress(0x7fe57eu, cpu->x)));
                                                        /* $83:D2C9 */
        Write8(
            memory, LongIndexedAddress(0x7fe57eu, cpu->x), A8(cpu));
                                                        /* $83:D2CD */
        IncrementY16(cpu);                             /* $83:D2D1 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d2d5u:
        LoadXDirect16(memory, cpu, 0xa7u);             /* $83:D2D5 */
        LoadA8(
            cpu, Read8(
                memory,
                AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:D2D7 */
        And8(
            cpu,
            Read8(memory, LongIndexedAddress(0x7fe57eu, cpu->x)));
                                                        /* $83:D2DA */
        Write8(
            memory, LongIndexedAddress(0x7fe57eu, cpu->x), A8(cpu));
                                                        /* $83:D2DE */
        IncrementY16(cpu);                             /* $83:D2E2 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    default:
        return result;
    }
}
