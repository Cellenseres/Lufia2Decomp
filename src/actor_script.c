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

/* Indexing carries into the next bank. */
static uint32_t AbsoluteIndexedAddress(
    const Lufia2ActorFrontendCpu *cpu, uint16_t address, uint16_t index) {
    return (((uint32_t)cpu->data_bank << 16) + address + index) &
           0x00ffffffu;
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

static void AslA16(Lufia2ActorFrontendCpu *cpu) {
    const uint16_t old = cpu->accumulator;
    cpu->carry = (old & 0x8000u) != 0;
    LoadA16(cpu, (uint16_t)(old << 1));
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


static void ExchangeAccumulatorBytes(Lufia2ActorFrontendCpu *cpu) {
    const uint16_t value = cpu->accumulator;
    cpu->accumulator =
        (uint16_t)((value << 8) | (value >> 8));
    SetNz8(cpu, A8(cpu));
}

static void And16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator & value));
}

static void Add16Value(
    Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    const uint32_t sum =
        (uint32_t)cpu->accumulator + value + (cpu->carry ? 1u : 0u);
    cpu->accumulator = (uint16_t)sum;
    cpu->carry = sum > 0xffffu;
    SetNz16(cpu, cpu->accumulator);
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


static void PushIndex(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->index_is_8_bit) {
        Push8(memory, cpu, (uint8_t)cpu->x);
    } else {
        Push8(memory, cpu, (uint8_t)(cpu->x >> 8));
        Push8(memory, cpu, (uint8_t)cpu->x);
    }
}

static void PullAccumulator16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint8_t low = Pull8(memory, cpu);
    const uint8_t high = Pull8(memory, cpu);
    LoadA16(cpu, (uint16_t)(low | ((uint16_t)high << 8)));
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

static void LoadXDirect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    if (cpu->index_is_8_bit) {
        const uint8_t value = Read8(memory, DirectAddress(cpu, offset));
        cpu->x = value;
        SetNz8(cpu, value);
    } else {
        LoadXDirect16(memory, cpu, offset);
    }
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
    const uint32_t low = AbsoluteIndexedAddress(cpu, address, index);
    const uint32_t high = (low + 1u) & 0x00ffffffu;
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

static void DecrementA8(Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
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


static void SimulateJslFrame(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    Push8(memory, cpu, return_bank);
    Push8(memory, cpu, (uint8_t)(return_address >> 8));
    Push8(memory, cpu, (uint8_t)return_address);
}

static void SimulateRtlFrame(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    (void)Pull8(memory, cpu);
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

/* $83:CBEC/$83:CBFF: leader within radius on one axis. */
static uint8_t PrimaryLeaderWithinRadius(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t coordinate_base) {
    const uint32_t leader = AbsoluteIndexedAddress(cpu, coordinate_base, 0);

    LoadA8(
        cpu, Read8(
            memory,
            AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    cpu->carry = 0;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Compare8(cpu, A8(cpu), Read8(memory, leader));
    if (cpu->carry)
        return 0;
    cpu->carry = 1;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    Compare8(cpu, A8(cpu), Read8(memory, leader));
    return cpu->carry;
}

/* $83:CC4E/$83:CC70: direction toward the leader. */
static void PrimaryLeaderDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t coordinate_base,
    uint8_t action_if_ahead,
    uint8_t action_if_behind) {
    uint8_t ahead;

    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(
        cpu, Read8(
            memory,
            AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, AbsoluteIndexedAddress(cpu, coordinate_base, 0)));
    if (cpu->zero) {
        cpu->carry = 0;
        return;
    }
    ahead = cpu->carry;
    LoadA8(cpu, action_if_ahead);
    if (!ahead)
        LoadA8(cpu, action_if_behind);
    cpu->carry = 1;
}


static void PrimaryInstallSecondaryScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

static uint8_t PrimaryCallActionCore(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    Lufia2ActorPrimaryActionFlow flow;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    flow = Lufia2ActorPrimaryActionCore(memory, cpu);
    if (flow != LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
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


    case 0x83c867u:
        LoadA8(cpu, 0x00u);                            /* $83:C867 */
        goto primary_fixed_action;
    case 0x83c86bu:
        LoadA8(cpu, 0x01u);                            /* $83:C86B */
        goto primary_fixed_action;
    case 0x83c86fu:
        LoadA8(cpu, 0x02u);                            /* $83:C86F */
        goto primary_fixed_action;
    case 0x83c873u:
        LoadA8(cpu, 0x03u);                            /* $83:C873 */
        goto primary_fixed_action;
    case 0x83c87cu:
        LoadA8(cpu, 0x81u);                            /* $83:C87C */
        goto primary_fixed_action;
    case 0x83c880u:
        LoadA8(cpu, 0x82u);                            /* $83:C880 */
        goto primary_fixed_action;
    case 0x83c884u:
        LoadA8(cpu, 0x83u);                            /* $83:C884 */
        goto primary_fixed_action;
    case 0x83c888u:
        LoadA8(cpu, 0x84u);                            /* $83:C888 */
        goto primary_fixed_action;

primary_fixed_action:
        if (!PrimaryCallActionCore(memory, cpu, 0xc88du)) {
            result.handler_pc = 0x83d350u;
            return result;
        }
        IncrementY16(cpu);                             /* $83:C8C6 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83c877u:
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:C877 */
        if (!PrimaryCallActionCore(memory, cpu, 0xc88du)) {
            result.handler_pc = 0x83d350u;
            return result;
        }
        IncrementY16(cpu);                             /* $83:C8C6 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

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
            if (!PrimaryCallActionCore(memory, cpu, 0xd169u)) {
                result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_D166;
                result.handler_pc = 0x83d166u;
                return result;
            }
            LoadA8(cpu, Read8(memory, 0x7fe4deu));     /* $83:D16A */
            Write8(
                memory, LongIndexedAddress(0x7fe4deu, cpu->x),
                A8(cpu));                              /* $83:D16E */
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

    case 0x83c891u:
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:C891 */
        cpu->carry = 0;                                /* $83:C894 */
        Adc8(cpu, 0x18u);                              /* $83:C895 */
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C897 */
        Write8(
            memory, LongIndexedAddress(0x7fe466u, cpu->x), A8(cpu));
                                                        /* $83:C899 */
        SimulateJsrFrame(memory, cpu, 0xc89fu);        /* $83:C89D */
        PrimaryInstallSecondaryScript(memory, cpu);    /* $83:D3F7 */
        SimulateRtsFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C8A0 */
        LoadA8(cpu, 0x80u);                            /* $83:C8A2 */
        Or8(
            cpu,
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
                                                        /* $83:C8A4 */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x),
            A8(cpu));                                  /* $83:C8A7 */
        IncrementY16(cpu);                             /* $83:C8AA */
        IncrementY16(cpu);                             /* $83:C8AB */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:C8AC */

    case 0x83c8eeu:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C8EE */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:C8F0 */
        Write8(
            memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
                                                        /* $83:C8F3 */
        IncrementY16(cpu);                             /* $83:C8F7 */
        IncrementY16(cpu);                             /* $83:C8F8 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83c8fcu:
    case 0x83c90au:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C8FC */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x)));
                                                        /* $83:C8FE */
        if ((handler_pc & 0x00ffffffu) == 0x83c8fcu)
            Or8(cpu, 0x02u);                           /* $83:C901 */
        else
            And8(cpu, 0xfdu);                          /* $83:C90F */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x),
            A8(cpu));                                  /* $83:C903 */
        IncrementY16(cpu);                             /* $83:C906 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83cbb7u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CBB7 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
                                                        /* $83:CBB9 */
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
                                                        /* $83:CBBC */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
                                                        /* $83:CBBE */
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
                                                        /* $83:CBC1 */
        SimulateJsrFrame(memory, cpu, 0xcbc5u);        /* $83:CBC3 */
        Lufia2ActorResolveMapCellOffset(memory, cpu);  /* $83:F9D4 */
        SimulateRtsFrame(memory, cpu);
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
                                                        /* $83:CBC6 */
        And8(cpu, 0x30u);                              /* $83:CBCA */
        Compare8(cpu, A8(cpu), 0x30u);                 /* $83:CBCC */
        if (!cpu->zero) {
            IncrementY16(cpu);                         /* $83:CBDD */
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CBD0 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x)));
                                                        /* $83:CBD2 */
        Or8(cpu, 0x40u);                               /* $83:CBD5 */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x),
            A8(cpu));                                  /* $83:CBD7 */
        result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
        result.handler_pc = 0x83c8d2u;                 /* $83:CBDA */
        return result;

    case 0x83cbe1u:
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:CBE1 */
        IncrementY16(cpu);                             /* $83:CBE4 */
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
                                                        /* $83:CBE5 */
        AslA8(cpu);                                    /* $83:CBE7 */
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
                                                        /* $83:CBE8 */
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CBEA */
        if (!PrimaryLeaderWithinRadius(memory, cpu, 0x06bau) ||
            !PrimaryLeaderWithinRadius(memory, cpu, 0x06e2u))
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CC18 */
        IncrementY16(cpu);                             /* $83:CC12 */
        IncrementY16(cpu);                             /* $83:CC13 */
        IncrementY16(cpu);                             /* $83:CC14 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83cc1bu:
    case 0x83cc2eu: {
        const uint16_t coordinate_base =
            (handler_pc & 0x00ffffffu) == 0x83cc1bu ? 0x06bau : 0x06e2u;

        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CC1B */
        LoadA8(
            cpu, Read8(
                memory,
                AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
                                                        /* $83:CC1D */
        Compare8(
            cpu, A8(cpu),
            Read8(
                memory,
                AbsoluteIndexedAddress(cpu, coordinate_base, 0)));
                                                        /* $83:CC20 */
        if (cpu->zero)
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CC25 */
        IncrementY16(cpu);                             /* $83:CC28 */
        IncrementY16(cpu);                             /* $83:CC29 */
        IncrementY16(cpu);                             /* $83:CC2A */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83cc41u:
    case 0x83cc63u: {
        const uint8_t x_axis = (handler_pc & 0x00ffffffu) == 0x83cc41u;

        SimulateJsrFrame(
            memory, cpu, x_axis ? 0xcc43u : 0xcc65u); /* $83:CC41 */
        if (x_axis)
            PrimaryLeaderDirection(memory, cpu, 0x06bau, 0x02u, 0x03u);
        else
            PrimaryLeaderDirection(memory, cpu, 0x06e2u, 0x00u, 0x01u);
        SimulateRtsFrame(memory, cpu);
        if (cpu->carry &&
            !PrimaryCallActionCore(
                memory, cpu, x_axis ? 0xcc49u : 0xcc6bu)) {
            result.handler_pc = 0x83d350u;             /* $83:CC46 */
            return result;
        }
        IncrementY16(cpu);                             /* $83:CC4A */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    default:
        return result;
    }
}



static void PrimaryMapCoordinateToCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Write8(memory, 0x004202u, A8(cpu));                  /* $83:F9F7 */
    LoadA8(cpu, Read8(memory, 0x0005b9u));              /* $83:F9FB */
    Write8(memory, 0x004203u, A8(cpu));                 /* $83:F9FF */
    LoadA8(cpu, 0x00u);                                 /* $83:FA03 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FA05 */
    SetAccumulatorWidth(cpu, 0);                        /* $83:FA06 */
    cpu->carry = 0;                                     /* $83:FA08 */
    Add16Value(cpu, Read16Long(memory, 0x004216u));     /* $83:FA09 */
    AslA16(cpu);                                        /* $83:FA0D */
    TransferAToX(cpu);                                  /* $83:FA0E */
    SetAccumulatorWidth(cpu, 1);                        /* $83:FA0F */
}

uint32_t Lufia2ActorMovementStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;
    uint32_t address;
    uint8_t value;

    ExchangeAccumulatorBytes(cpu);                      /* $83:FB12 */
    LoadA8(cpu, 0x00u);                                 /* $83:FB13 */
    ExchangeAccumulatorBytes(cpu);                      /* $83:FB15 */
    TransferAToX(cpu);                                  /* $83:FB16 */
    target = Read16ProgramIndexed(memory, cpu, 0xfb1au, cpu->x);
                                                               /* $83:FB17 */

    switch (target) {
    case 0xfb22u:
        address = DirectAddress(cpu, 0x91u);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb24u;
    case 0xfb25u:
        address = DirectAddress(cpu, 0x8fu);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb27u;
    case 0xfb28u:
        address = DirectAddress(cpu, 0x91u);
        value = (uint8_t)(Read8(memory, address) - 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2au;
    case 0xfb2bu:
        address = DirectAddress(cpu, 0x8fu);
        value = (uint8_t)(Read8(memory, address) + 1u);
        Write8(memory, address, value);
        SetNz8(cpu, value);
        return 0x83fb2du;
    default:
        return 0;
    }
}

void Lufia2ActorResolveMapCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* F9D4 */
    ExchangeAccumulatorBytes(cpu);                             /* F9D6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));     /* F9D7 */

    SimulateJsrFrame(memory, cpu, 0xf9dbu);                    /* F9D9 */
    PrimaryMapCoordinateToCellOffset(memory, cpu);             /* F9F7 */
    SimulateRtsFrame(memory, cpu);

    SetAccumulatorWidth(cpu, 0);                               /* F9DC */
    PushIndex(memory, cpu);                                    /* F9DE */
    LoadA16(cpu, Read16Long(memory, 0x0005aau));               /* F9DF */
    TransferAToX(cpu);                                         /* F9E3 */
    PullAccumulator16(memory, cpu);                            /* F9E4 */
    cpu->carry = 0;                                            /* F9E5 */
    Add16Value(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x7fd008u, cpu->x)));   /* F9E6 */
    TransferAToX(cpu);                                         /* F9EA */
    SetAccumulatorWidth(cpu, 1);                               /* F9EB */
}

void Lufia2ActorReadMapCellValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xfb73u);                    /* FB71 */
    Lufia2ActorResolveMapCellOffset(memory, cpu);              /* F9D4 */
    SimulateRtsFrame(memory, cpu);

    SetAccumulatorWidth(cpu, 0);                               /* FB74 */
    LoadA16(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x7f0000u, cpu->x)));   /* FB76 */
    And16(cpu, 0x03ffu);                                       /* FB7A */
    cpu->carry = 0;                                            /* FB7D */
    Add16Value(cpu, Read16Long(memory, 0x7fd03eu));            /* FB7E */
    TransferAToX(cpu);                                         /* FB82 */
    SetAccumulatorWidth(cpu, 1);                               /* FB83 */
    TransferDirectToA(cpu);                                    /* FB85 */
    LoadA8(
        cpu, Read8(
            memory, LongIndexedAddress(0x7f0000u, cpu->x)));   /* FB86 */
}

static void PrimaryActionBoundaryHelper(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t helper_pc) {
    uint8_t coordinate;

    LoadXDirect(memory, cpu, 0xa7u);

    switch (helper_pc) {
    case 0xd3b7u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(memory, LongIndexedAddress(0x7fe61eu, cpu->x)));
        }
        break;

    case 0xd3c5u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadA8(
                cpu,
                Read8(memory, LongIndexedAddress(0x7fe66eu, cpu->x)));
            DecrementA8(cpu);
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
        }
        break;

    case 0xd3d7u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(memory, LongIndexedAddress(0x7fe5f6u, cpu->x)));
        }
        break;

    case 0xd3e5u:
        coordinate = Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x));
        LoadA8(cpu, coordinate);
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadA8(
                cpu,
                Read8(memory, LongIndexedAddress(0x7fe646u, cpu->x)));
            DecrementA8(cpu);
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu),
                Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
        }
        break;

    default:
        break;
    }
}

static void PrimaryInstallSecondaryScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint8_t action = A8(cpu);

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, action);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(
        cpu, Read16Long(
            memory, LongIndexedAddress(0x918000u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8000u);
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x91u);
    Write8(
        memory, LongIndexedAddress(0x7fe3f0u, cpu->x), A8(cpu));
}

Lufia2ActorPrimaryActionFlow Lufia2ActorPrimaryActionCore(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t helper_pc;

    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));       /* D350 */
    LoadXDirect(memory, cpu, 0xa7u);                          /* D352 */
    Write8(
        memory, LongIndexedAddress(0x7fe466u, cpu->x), A8(cpu));
                                                               /* D354 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x)));
                                                               /* D358 */
    And8(cpu, 0xefu);                                         /* D35B */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x),
        A8(cpu));                                              /* D35D */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
                                                               /* D360 */
    And8(cpu, 0x28u);                                         /* D363 */
    if (!cpu->zero)
        goto install_secondary_script;                         /* D365 */

    TransferDirectToA(cpu);                                   /* D367 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D368 */
    Compare8(cpu, A8(cpu), 0x04u);                            /* D36A */
    if (cpu->carry)
        goto install_secondary_script;                         /* D36C */

    AslA8(cpu);                                               /* D36E */
    TransferAToX(cpu);                                        /* D36F */
    helper_pc = Read16ProgramIndexed(memory, cpu, 0xd3afu, cpu->x);
                                                               /* D370 */

    if (helper_pc != 0xd3b7u && helper_pc != 0xd3c5u &&
        helper_pc != 0xd3d7u && helper_pc != 0xd3e5u)
        return LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET;

    SimulateJsrFrame(memory, cpu, 0xd372u);
    PrimaryActionBoundaryHelper(memory, cpu, helper_pc);
    SimulateRtsFrame(memory, cpu);

    if (!cpu->carry)
        return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;         /* D373 */

    LoadXDirect(memory, cpu, 0xa7u);                           /* D375 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
                                                               /* D377 */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));        /* D37A */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
                                                               /* D37C */
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));        /* D37F */
    TransferDirectToA(cpu);                                   /* D381 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D382 */
    TransferAToX(cpu);                                        /* D384 */
    LoadA8(
        cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
                                                               /* D385 */

    SimulateJslFrame(memory, cpu, 0x83u, 0xd38cu);             /* D389 */
    if (Lufia2ActorMovementStep(memory, cpu) == 0)
        return LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET;
    SimulateRtlFrame(memory, cpu);

    SimulateJslFrame(memory, cpu, 0x83u, 0xd390u);             /* D38D */
    Lufia2ActorReadMapCellValue(memory, cpu);                   /* FB71 */
    SimulateRtlFrame(memory, cpu);

    Compare8(cpu, A8(cpu), 0x07u);                             /* D391 */
    if (cpu->zero)
        goto install_secondary_script;
    Compare8(cpu, A8(cpu), 0x01u);                             /* D395 */
    if (cpu->zero)
        goto install_secondary_script;
    Or8(cpu, 0x00u);                                          /* D399 */
    if (!cpu->zero)
        return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;         /* D39B */

install_secondary_script:
    LoadXDirect(memory, cpu, 0xa7u);                           /* D39D */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));     /* D39F */
    SimulateJsrFrame(memory, cpu, 0xd3a3u);                   /* D3A1 */
    PrimaryInstallSecondaryScript(memory, cpu);                /* D3F7 */
    SimulateRtsFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* D3A4 */
    LoadA8(cpu, 0x80u);                                       /* D3A6 */
    Or8(
        cpu,
        Read8(memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
                                                               /* D3A8 */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x),
        A8(cpu));                                              /* D3AB */
    return LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE;
}
