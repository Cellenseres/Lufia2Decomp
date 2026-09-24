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
    const uint16_t old = cpu->accumulator;
    const uint32_t sum =
        (uint32_t)old + value + (cpu->carry ? 1u : 0u);
    cpu->accumulator = (uint16_t)sum;
    cpu->carry = sum > 0xffffu;
    cpu->overflow =
        ((~(old ^ value) & (old ^ (uint16_t)sum)) & 0x8000u) != 0;
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
    Add16Value(cpu, value);
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

/* Binary mode only; the game never sets D. */
static void Adc8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t old = A8(cpu);
    const uint16_t sum =
        (uint16_t)old + value + (cpu->carry ? 1u : 0u);
    cpu->carry = sum > 0xffu;
    cpu->overflow =
        ((~(old ^ value) & (old ^ (uint8_t)sum)) & 0x80u) != 0;
    LoadA8(cpu, (uint8_t)sum);
}

static void Sbc8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    Adc8(cpu, (uint8_t)~value);
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

static void PrimaryCallRandom(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

static void PrimaryCallRandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

static void PrimaryTargetDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

static uint8_t PrimaryFacingCompare(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t helper_pc);

/* $83:D27F: X = operand8 * $28 + $A7 via the multiplier. */
static void PrimaryTargetRecordIndex(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x4202u, 0), A8(cpu));
    LoadA8(cpu, 0x28u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x4203u, 0), A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x4216u, 0)));
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* 16-bit operand + $A1D4 into $2A, then C85A. */
static Lufia2ActorScriptDispatchResult PrimaryJumpOperand(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t operand_offset) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(
        cpu, Read16AbsoluteIndexed(memory, cpu, operand_offset, cpu->y));
    cpu->carry = 0;
    Add16Immediate(cpu, 0xa1d4u);
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return PrimaryRedispatch(memory, cpu, 1);
}

/* D350 call; zero means unknown D350 path. */
#define PRIMARY_ACTION_OR_BOUNDARY(return_address)                  \
    do {                                                             \
        if (!PrimaryCallActionCore(memory, cpu, (return_address))) { \
            result.handler_pc = cpu->resume_pc;                      \
            return result;                                           \
        }                                                            \
    } while (0)

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

static uint16_t PullIndexValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

static void Compare16(
    Lufia2ActorFrontendCpu *cpu, uint16_t left, uint16_t right) {
    cpu->carry = left >= right;
    SetNz16(cpu, (uint16_t)(left - right));
}

/* $83:C0EF: leader position to $8F/$91. */
static void PrimaryLeaderToProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06bau, 0)));
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06e2u, 0)));
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:C99A: actor position to $8F/$91. */
static void PrimaryActorToProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Probe +/- $54 on both axes into $9F..$A2. */
static void PrimaryProbeBox(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t x_offset,
    uint8_t y_offset) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, x_offset)));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, DirectAddress(cpu, 0x9fu), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    Write8(memory, DirectAddress(cpu, 0xa1u), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, y_offset)));
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, DirectAddress(cpu, 0xa0u), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    Write8(memory, DirectAddress(cpu, 0xa2u), A8(cpu));
}

/* $83:C9A7: radius box around $8F/$91. */
static void PrimaryRadiusBox(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    PrimaryProbeBox(memory, cpu, 0x8fu, 0x91u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:C9F9/$83:CA08: step toward probe on one axis. */
static void PrimaryProbeAxisDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t coordinate_base,
    uint8_t probe_offset,
    uint8_t action_if_ahead,
    uint8_t action_if_behind,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, coordinate_base, cpu->x)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, probe_offset)));
    if (cpu->zero) {
        cpu->carry = 0;                                        /* CA17 */
    } else {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? action_if_ahead : action_if_behind);
        cpu->carry = 1;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:C9C5: step toward $8F/$91; $7F:E5A6 picks axis order. */
static void PrimaryApproachProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);                           /* C9C5 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
    if (!cpu->zero) {
        PrimaryProbeAxisDirection(
            memory, cpu, 0x06e2u, 0x91u, 0x00u, 0x01u, 0xc9cfu);
        if (!cpu->carry)
            PrimaryProbeAxisDirection(
                memory, cpu, 0x06bau, 0x8fu, 0x02u, 0x03u, 0xc9d4u);
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);                     /* C9D7 */
            LoadA8(cpu, 0x00u);
            Write8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x), 0x00u);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
        }
    } else {
        PrimaryProbeAxisDirection(
            memory, cpu, 0x06bau, 0x8fu, 0x02u, 0x03u, 0xc9e4u);
        if (!cpu->carry)
            PrimaryProbeAxisDirection(
                memory, cpu, 0x06e2u, 0x91u, 0x00u, 0x01u, 0xc9e9u);
        if (cpu->carry) {
            ExchangeAccumulatorBytes(cpu);                     /* C9EC */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
            DecrementA8(cpu);
            Write8(
                memory, LongIndexedAddress(0x7fe5a6u, cpu->x), A8(cpu));
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
        }
    }
    SimulateRtsFrame(memory, cpu);
}

typedef enum PrimaryListSearch {
    PRIMARY_LIST_STEPPED = 0,
    PRIMARY_LIST_EXHAUSTED = 1,
    PRIMARY_LIST_BOUNDARY = 2,
} PrimaryListSearch;

/* $83:D0AA: first steppable listed point in radius. */
static PrimaryListSearch PrimaryApproachListedPoint(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    uint32_t guard;

    SimulateJsrFrame(memory, cpu, return_address);
    Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));        /* D0AA */
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);          /* D0AC */
    PushIndex(memory, cpu);                                    /* D0AE */
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));        /* D0B2 */
    PrimaryActorToProbe(memory, cpu, 0xd0b6u);                 /* D0B4 */
    PrimaryRadiusBox(memory, cpu, 0xd0b9u);                    /* D0B7 */
    cpu->x = PullIndexValue(memory, cpu);                      /* D0BA */
    Write8(memory, DirectAddress(cpu, 0x56u), 0x00u);          /* D0BB */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
    TransferAToX(cpu);                                         /* D0C6 */

    for (guard = 0;; ++guard) {
        if (guard >= 0x10000u) {
            cpu->resume_pc = 0x83d0c7u;
            return PRIMARY_LIST_BOUNDARY;
        }
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);                         /* D0CB */
        if (cpu->zero) {
            IncrementY16(cpu);                                 /* D10E */
            cpu->carry = 1;
            SimulateRtsFrame(memory, cpu);
            return PRIMARY_LIST_EXHAUSTED;
        }
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x9fu)));
        if (!cpu->carry)
            goto next;
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa1u)));
        if (cpu->carry)
            goto next;
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa0u)));
        if (!cpu->carry)
            goto next;
        DecrementA8(cpu);                                      /* D0E3 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa2u)));
        if (cpu->carry)
            goto next;

        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7ef001u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));    /* D0F7 */
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7ef002u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));    /* D0FD */
        PrimaryApproachProbe(memory, cpu, 0xd101u);            /* D0FF */
        if (cpu->carry) {
            if (!PrimaryCallActionCore(memory, cpu, 0xd107u))  /* D104 */
                return PRIMARY_LIST_BOUNDARY;
            IncrementY16(cpu);                                 /* D108 */
            IncrementY16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            cpu->carry = 0;
            SimulateRtsFrame(memory, cpu);
            return PRIMARY_LIST_STEPPED;
        }
next:
        SetAccumulatorWidth(cpu, 0);                           /* D0E8 */
        LoadA16(cpu, cpu->x);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x58u));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
}

static void PrimaryMapCoordinateToCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu);

static void IncrementDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint8_t value =
        (uint8_t)(Read8(memory, DirectAddress(cpu, offset)) + 1u);
    Write8(memory, DirectAddress(cpu, offset), value);
    SetNz8(cpu, value);
}

static void DecrementDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint8_t value =
        (uint8_t)(Read8(memory, DirectAddress(cpu, offset)) - 1u);
    Write8(memory, DirectAddress(cpu, offset), value);
    SetNz8(cpu, value);
}

static void CopyDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t from,
    uint8_t to) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, from)));
    Write8(memory, DirectAddress(cpu, to), A8(cpu));
}

/* $83:F9AD / $83:F9B6: X = $8F + $91 * width. */
static void PrimaryCollisionIndex(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address,
    uint8_t from_probe) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (from_probe) {
        Write8(memory, DirectAddress(cpu, 0x90u), 0x00u);      /* F9AD */
        Write8(memory, DirectAddress(cpu, 0x92u), 0x00u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    }
    Write8(memory, 0x004202u, A8(cpu));                        /* F9B6 */
    LoadA8(cpu, Read8(memory, 0x0005b9u));
    Write8(memory, 0x004203u, A8(cpu));
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x004216u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F988: height bits 7-6 of the map cell at $8F/$91. */
static void PrimaryTileHeight(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SimulateJsrFrame(memory, cpu, 0xf98au);                    /* F988 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* F9F2 */
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    PrimaryMapCoordinateToCellOffset(memory, cpu);             /* F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* F98B */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05aau, 0));
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(0x7fd008u, cpu->x)));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7f0001u, cpu->x)));
    AslA8(cpu);                                                /* F99C */
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);                                          /* F9A2 */
    SimulateRtsFrame(memory, cpu);
}

/* Collision byte test: A = mask & $7E:4000+offset,X. */
static void PrimaryCollisionTest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mask,
    uint8_t second_column) {
    LoadA8(cpu, mask);
    And8(
        cpu, Read8(
            memory,
            LongIndexedAddress(
                second_column ? 0x7e4001u : 0x7e4000u, cpu->x)));
}

/* $9A >= 2 means a two-column actor. */
static uint8_t PrimaryWideActor(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9au)));
    Compare8(cpu, A8(cpu), 0x02u);
    return cpu->carry;
}

/* $83:D89E body: Z clear = blocked, 0 = unknown target. */
static uint8_t PrimaryStepBlockedBody(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;

    TransferAToX(cpu);                                         /* D89E */
    target = Read16ProgramIndexed(memory, cpu, 0xd8a3u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xd8a1u);                    /* D89F */

    switch (target) {
    case 0xd8abu:
        IncrementDirect8(memory, cpu, 0x91u);                  /* D8AB */
        PrimaryCollisionIndex(memory, cpu, 0xd8afu, 1);
        PrimaryCollisionTest(memory, cpu, 0x9bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D8BE */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x9bu, 1);           /* D8C0 */
        break;

    case 0xd8c7u:
        PrimaryCollisionIndex(memory, cpu, 0xd8c9u, 1);        /* D8C7 */
        PrimaryCollisionTest(memory, cpu, 0x20u, 0);
        if (!cpu->zero)
            break;
        DecrementDirect8(memory, cpu, 0x8fu);                  /* D8D2 */
        PrimaryCollisionIndex(memory, cpu, 0xd8d6u, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        break;

    case 0xd8deu:
        PrimaryCollisionIndex(memory, cpu, 0xd8e0u, 1);        /* D8DE */
        PrimaryCollisionTest(memory, cpu, 0x10u, 0);
        if (!cpu->zero)
            break;
        if (PrimaryWideActor(memory, cpu)) {
            PrimaryCollisionTest(memory, cpu, 0x10u, 1);       /* D8EF */
            if (!cpu->zero)
                break;
        }
        DecrementDirect8(memory, cpu, 0x91u);                  /* D8F7 */
        PrimaryCollisionIndex(memory, cpu, 0xd8fbu, 1);
        PrimaryCollisionTest(memory, cpu, 0x8bu, 0);
        if (!cpu->zero)
            break;
        if (!PrimaryWideActor(memory, cpu)) {
            TransferDirectToA(cpu);                            /* D90A */
            break;
        }
        PrimaryCollisionTest(memory, cpu, 0x8bu, 1);           /* D90C */
        break;

    case 0xd913u:
        if (PrimaryWideActor(memory, cpu))                     /* D913 */
            IncrementDirect8(memory, cpu, 0x8fu);
        IncrementDirect8(memory, cpu, 0x8fu);                  /* D91B */
        PrimaryCollisionIndex(memory, cpu, 0xd91fu, 1);
        PrimaryCollisionTest(memory, cpu, 0xabu, 0);
        break;

    default:
        cpu->resume_pc = 0x830000u | target;
        return 0;
    }

    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* JSR $D89E. */
static uint8_t PrimaryStepBlocked(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (!PrimaryStepBlockedBody(memory, cpu))
        return 0;
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:CDF6: clear same-height run length into $9D. */
static uint8_t PrimaryMeasureRun(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    uint32_t guard;

    SimulateJsrFrame(memory, cpu, return_address);
    PrimaryTileHeight(memory, cpu, 0xcdf8u);                   /* CDF6 */
    Write8(memory, DirectAddress(cpu, 0x99u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x9du), 0x00u);          /* CDFB */

    for (guard = 0;; ++guard) {
        if (guard >= 0x10000u) {
            cpu->resume_pc = 0x83cdfdu;
            return 0;
        }
        CopyDirect8(memory, cpu, 0x8fu, 0x95u);                /* CDFD */
        CopyDirect8(memory, cpu, 0x91u, 0x96u);
        TransferDirectToA(cpu);                                /* CE05 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
        if (!PrimaryStepBlocked(memory, cpu, 0xce0au))         /* CE08 */
            return 0;
        if (!cpu->zero)
            break;
        CopyDirect8(memory, cpu, 0x95u, 0x8fu);                /* CE0D */
        CopyDirect8(memory, cpu, 0x96u, 0x91u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
        SimulateJslFrame(memory, cpu, 0x83u, 0xce1au);         /* CE17 */
        if (Lufia2ActorMovementStep(memory, cpu) == 0)
            return 0;
        SimulateRtlFrame(memory, cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));  /* CE1B */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x9fu)));
        if (!cpu->carry)
            break;
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa1u)));
        if (cpu->carry)
            break;
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa0u)));
        if (!cpu->carry)
            break;
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa2u)));
        if (cpu->carry)
            break;
        PrimaryTileHeight(memory, cpu, 0xce31u);               /* CE2F */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x99u)));
        if (!cpu->zero)
            break;
        IncrementDirect8(memory, cpu, 0x9du);                  /* CE36 */
    }
    SimulateRtsFrame(memory, cpu);                             /* CE3A */
    return 1;
}

/* $83:CE3B: walk A steps (0 = 256) in direction $94. */
static uint8_t PrimaryWalkSteps(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    uint16_t target;

    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator8(memory, cpu);                             /* CE3B */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));
    TransferAToX(cpu);                                         /* CE3F */
    LoadA8(cpu, Pull8(memory, cpu));                           /* CE40 */
    target = Read16ProgramIndexed(memory, cpu, 0xce48u, cpu->x);
    if (target != 0xce50u && target != 0xce53u &&
        target != 0xce56u && target != 0xce59u) {
        cpu->resume_pc = 0x83ce41u;
        return 0;
    }
    do {
        SimulateJsrFrame(memory, cpu, 0xce43u);                /* CE41 */
        if (target == 0xce50u)
            IncrementDirect8(memory, cpu, 0x91u);
        else if (target == 0xce53u)
            DecrementDirect8(memory, cpu, 0x8fu);
        else if (target == 0xce56u)
            DecrementDirect8(memory, cpu, 0x91u);
        else
            IncrementDirect8(memory, cpu, 0x8fu);
        SimulateRtsFrame(memory, cpu);
        DecrementA8(cpu);                                      /* CE44 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:CE5C: $8F/$91 to $7F:DB4C/$7F:DB4D + $A9. */
static void PrimaryRecordProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa9u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
    Write8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    Write8(memory, LongIndexedAddress(0x7fdb4du, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:CF11: origin $63/$64 back to $8F/$91. */
static void PrimaryResetProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    CopyDirect8(memory, cpu, 0x63u, 0x8fu);
    CopyDirect8(memory, cpu, 0x64u, 0x91u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:CEA8: random walk of $66..$65 steps on one axis. */
static uint8_t PrimaryWanderAxis(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));        /* CEA8 */
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5f6u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9fu), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe61eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0xa0u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe646u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0xa1u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe66eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0xa2u), A8(cpu));
    LoadA8(cpu, 0x02u);                                        /* CEC4 */
    PrimaryCallRandom(memory, cpu, 0xcec9u);
    AslA8(cpu);                                                /* CECA */
    AslA8(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    PrimaryResetProbe(memory, cpu, 0xced2u);                   /* CED0 */
    if (!PrimaryMeasureRun(memory, cpu, 0xced5u))
        return 0;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));     /* CED6 */
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x66u)));
    if (!cpu->carry) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u))); /* CEDC */
        cpu->carry = 0;
        Adc8(cpu, 0x04u);
        And8(cpu, 0x06u);
        Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
        PrimaryResetProbe(memory, cpu, 0xcee7u);
        if (!PrimaryMeasureRun(memory, cpu, 0xceeau))
            return 0;
    }
    PrimaryResetProbe(memory, cpu, 0xceedu);                   /* CEEB */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x66u)));
    if (cpu->carry) {
        Compare8(
            cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x65u)));
        if (cpu->carry) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x65u)));
            cpu->carry = 1;                                    /* CEFA */
            Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x66u)));
        }
        DecrementA8(cpu);                                      /* CEFD */
        PrimaryCallRandom(memory, cpu, 0xcf01u);
        cpu->carry = 0;                                        /* CF02 */
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x66u)));
        if (!PrimaryWalkSteps(memory, cpu, 0xcf07u))
            return 0;
        CopyDirect8(memory, cpu, 0x8fu, 0x63u);                /* CF08 */
        CopyDirect8(memory, cpu, 0x91u, 0x64u);
    }
    SimulateRtsFrame(memory, cpu);                             /* CF10 */
    return 1;
}

static void TestBitsAbsolute8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint8_t set) {
    const uint32_t effective = AbsoluteIndexedAddress(cpu, address, 0);
    const uint8_t value = Read8(memory, effective);
    const uint8_t a = A8(cpu);

    cpu->zero = (value & a) == 0;
    Write8(
        memory, effective,
        set ? (uint8_t)(value | a) : (uint8_t)(value & (uint8_t)~a));
}

static void LsrA16(Lufia2ActorFrontendCpu *cpu) {
    const uint16_t old = cpu->accumulator;
    cpu->carry = old & 1u;
    LoadA16(cpu, (uint16_t)(old >> 1));
}

static void PushAccumulator16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Push8(memory, cpu, (uint8_t)(cpu->accumulator >> 8));
    Push8(memory, cpu, (uint8_t)cpu->accumulator);
}

static void IncrementA16(Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
}

static void LoadAAbsolute8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, address, index)));
}

static void StoreAAbsolute8(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, index), A8(cpu));
}

void Lufia2ActorMarkMapOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* FA3F */
    Write8(memory, DirectAddress(cpu, 0x9eu), 0x00u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);                             /* FA47 */
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);          /* FA4B */
        Compare8(cpu, A8(cpu), 0x71u);
        if (!cpu->zero) {
            Compare8(cpu, A8(cpu), 0x72u);
            if (!cpu->zero) {
                Compare8(cpu, A8(cpu), 0x73u);
                if (!cpu->zero) {
                    LoadA8(cpu, 0xffu);                        /* FA5A */
                    Write8(memory, DirectAddress(cpu, 0x9eu), A8(cpu));
                }
            }
        }
    }
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);              /* FA5E */
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    PrimaryCollisionIndex(memory, cpu, 0xfa67u, 0);            /* FA65 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    Or8(cpu, 0x01u);
    Write8(memory, LongIndexedAddress(0x7e4000u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9eu)));     /* FA72 */
    if (!cpu->zero) {
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7e4001u, cpu->x)));
        Or8(cpu, 0x01u);
        Write8(memory, LongIndexedAddress(0x7e4001u, cpu->x), A8(cpu));
    }
}

void Lufia2ActorLoadPrimaryScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* D416 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x070au, cpu->x);
    AslA8(cpu);
    TransferAToX(cpu);                                         /* D41D */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(
        cpu, Read16Long(memory, LongIndexedAddress(0x91a1d4u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);                           /* D424 */
    cpu->carry = 0;
    Add16Immediate(cpu, 0xa1d4u);
    Write16Long(
        memory, LongIndexedAddress(0x7fe506u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* D42E */
    LoadA8(cpu, 0x91u);
    Write8(memory, LongIndexedAddress(0x7fe508u, cpu->x), A8(cpu));
}

void Lufia2ActorSyncFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* A746 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A74C */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    PushAccumulator16(memory, cpu);                            /* A752 */
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* A755 */
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SetAccumulatorWidth(cpu, 0);                               /* A759 */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    LoadXDirect(memory, cpu, 0xa9u);                           /* A75F */
    Write16Long(
        memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);                            /* A765 */
    Write16Long(
        memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* A76A */
}

void Lufia2QueueDeferredSound(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    ExchangeAccumulatorBytes(cpu);                             /* 8766 */
    LoadA8(cpu, Read8(memory, 0x0005b6u));
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 876F */
        Write8(memory, 0x0017acu, A8(cpu));
    }
}

/* $83:FAFA: sign-extend A.low, leave M=0. */
static void PrimarySignExtend(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Or8(cpu, 0x00u);                                           /* FAFA */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);                         /* FAFE */
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);                               /* FB02 */
    SimulateRtsFrame(memory, cpu);
}

/* Signed operand bytes added to a 16-bit X/Y pair. */
static void PrimaryAddSignedPair(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t pair,
    uint16_t operand,
    uint16_t return_address) {
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, operand, cpu->y);
    PrimarySignExtend(memory, cpu, return_address);
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
    Write16Long(memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
}

void Lufia2ActorAddDisplayOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);                           /* FACB */
    PrimaryAddSignedPair(memory, cpu, 0x7fdc8cu, 0x0001u, 0xfad3u);
    SetAccumulatorWidth(cpu, 1);                               /* FADD */
    PrimaryAddSignedPair(memory, cpu, 0x7fdd1cu, 0x0002u, 0xfae5u);
    LoadA16(cpu, cpu->y);                                      /* FAEF */
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

/* ADC #0 after LSR x4 rounds the 1/16 position. */
static void PrimaryFineToTile(Lufia2ActorFrontendCpu *cpu) {
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Add16Value(cpu, 0x0000u);
    SetAccumulatorWidth(cpu, 1);
}

void Lufia2ActorMoveFinePosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadYDirect16(memory, cpu, 0x2au);                         /* FA81 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    PrimarySignExtend(memory, cpu, 0xfa89u);
    LoadXDirect(memory, cpu, 0xa9u);                           /* FA8A */
    cpu->carry = 0;
    Add16Value(
        cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
    Write16Long(
        memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
    PrimaryFineToTile(cpu);                                    /* FA95 */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    PrimaryAddSignedPair(memory, cpu, 0x7fde3eu, 0x0002u, 0xfaa6u);
    PrimaryFineToTile(cpu);                                    /* FAB0 */
    LoadYDirect16(memory, cpu, 0xa7u);                         /* FAB9 */
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->y);
    SetAccumulatorWidth(cpu, 0);                               /* FAC3 */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    IncrementA16(cpu);
    IncrementA16(cpu);
    IncrementA16(cpu);
}

void Lufia2ActorPrimaryReset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xc94au);             /* C947 */
    Lufia2ActorMarkMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* C94B */
    if (cpu->zero) {
        LoadA8(cpu, 0x02u);                                    /* C94F */
        TestBitsAbsolute8(memory, cpu, 0x0622u, 0);
        LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
        And8(cpu, 0x01u);
        if (cpu->zero) {
            LoadA8(cpu, 0x01u);                                /* C95B */
            TestBitsAbsolute8(memory, cpu, 0x09a6u, 0);
        }
    }
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);              /* C960 */
    And8(cpu, 0xf7u);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    Compare8(cpu, A8(cpu), 0x0au);                             /* C96B */
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0xfdu);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);              /* C977 */
    And8(cpu, 0xfdu);
    StoreAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    LoadA8(cpu, 0x08u);
    Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xc988u);             /* C985 */
    Lufia2ActorLoadPrimaryScript(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

void Lufia2ActorClearSlotLinks(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, 0x0004u);                                     /* CB65 */
    LoadA8(cpu, 0xffu);
    do {
        StoreAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
}

void Lufia2ActorBlockedEvent(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, 0x7fd0a1u));                     /* CA68 */
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero)
        return;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1724u, 0));
    LoadA8(cpu, 0x01u);                                        /* CA73 */
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);                               /* CA79 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1724u, 0));
    Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x56u));         /* CA81 */
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, 0x918f2bu));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8ec7u);
    Write16Long(
        memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* CA90 */
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
            result.handler_pc = cpu->resume_pc;
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
            result.handler_pc = cpu->resume_pc;
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
                result.handler_pc = cpu->resume_pc;
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

    case 0x83c8afu:
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:C8AF */
        PrimaryCallRandom(memory, cpu, 0xc8b5u);       /* $83:C8B2 */
        cpu->carry = 0;                                /* $83:C8B6 */
        Adc8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
                                                        /* $83:C8B7 */
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C8BA */
        Write8(
            memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
                                                        /* $83:C8BC */
        IncrementY16(cpu);                             /* $83:C8C0 */
        IncrementY16(cpu);                             /* $83:C8C1 */
        IncrementY16(cpu);                             /* $83:C8C2 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:C8C3 */

    case 0x83c8d4u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:C8D4 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
                                                        /* $83:C8D6 */
        PrimaryCallRandom(memory, cpu, 0xc8dcu);       /* $83:C8D9 */
        cpu->carry = 0;                                /* $83:C8DD */
        Adc8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:C8DE */
        AslA8(cpu);                                    /* $83:C8E1 */
        AslA8(cpu);                                    /* $83:C8E2 */
        AslA8(cpu);                                    /* $83:C8E3 */
        Write8(
            memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
                                                        /* $83:C8E4 */
        IncrementY16(cpu);                             /* $83:C8E8 */
        IncrementY16(cpu);                             /* $83:C8E9 */
        IncrementY16(cpu);                             /* $83:C8EA */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:C8EB */

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
            result.handler_pc = cpu->resume_pc;        /* $83:CC46 */
            return result;
        }
        IncrementY16(cpu);                             /* $83:CC4A */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83d320u:
        PrimaryCallRandomByte(memory, cpu, 0xd323u);   /* $83:D320 */
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:D324 */
        if (!cpu->carry) {
            IncrementY16(cpu);                         /* $83:D329 */
            IncrementY16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        SetAccumulatorWidth(cpu, 0);                   /* $83:D330 */
        LoadA16(
            cpu, Read16AbsoluteIndexed(
                memory, cpu, 0x0002u, cpu->y));        /* $83:D332 */
        cpu->carry = 0;                                /* $83:D335 */
        Add16Immediate(cpu, 0xa1d4u);                  /* $83:D336 */
        Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
                                                        /* $83:D339 */
        SetAccumulatorWidth(cpu, 1);                   /* $83:D33B */
        dispatch = PrimaryRedispatch(memory, cpu, 1);  /* $83:C85A */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d340u:
        SetAccumulatorWidth(cpu, 0);                   /* $83:D340 */
        LoadA16(
            cpu, Read16AbsoluteIndexed(
                memory, cpu, 0x0001u, cpu->y));        /* $83:D342 */
        cpu->carry = 0;                                /* $83:D345 */
        Add16Immediate(cpu, 0xf000u);                  /* $83:D346 */
        Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
                                                        /* $83:D349 */
        SetAccumulatorWidth(cpu, 1);                   /* $83:D34B */
        dispatch = PrimaryRedispatch(memory, cpu, 1);  /* $83:C85A */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d125u:
        LoadA8(cpu, 0x04u);                            /* $83:D125 */
        PrimaryCallRandom(memory, cpu, 0xd12au);       /* $83:D127 */
        PRIMARY_ACTION_OR_BOUNDARY(0xd12eu);           /* $83:D12B */
        IncrementY16(cpu);                             /* $83:C8C6 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83d132u:
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:D132 */

    case 0x83ccf0u:
    case 0x83cd0du: {
        const uint8_t x_axis = (handler_pc & 0x00ffffffu) == 0x83ccf0u;

        SimulateJsrFrame(
            memory, cpu, x_axis ? 0xccf2u : 0xcd0fu); /* $83:CCF0 */
        if (x_axis)
            PrimaryLeaderDirection(memory, cpu, 0x06bau, 0x02u, 0x03u);
        else
            PrimaryLeaderDirection(memory, cpu, 0x06e2u, 0x00u, 0x01u);
        SimulateRtsFrame(memory, cpu);
        if (cpu->carry) {
            TransferAToX(cpu);                         /* $83:CD00 */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x83cd2au, cpu->x)));
        } else {
            LoadA8(cpu, 0x02u);                        /* $83:CCF5 */
            PrimaryCallRandom(
                memory, cpu, x_axis ? 0xccfau : 0xcd17u);
            cpu->carry = 0;                            /* $83:CCFB */
            Adc8(cpu, x_axis ? 0x02u : 0x00u);         /* $83:CCFC */
        }
        PRIMARY_ACTION_OR_BOUNDARY(x_axis ? 0xcd08u : 0xcd25u);
        IncrementY16(cpu);                             /* $83:CD09 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83cd2eu:
        IncrementY16(cpu);                             /* $83:CD2E */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83cd32u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CD32 */
        TransferDirectToA(cpu);                        /* $83:CD34 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0692u, cpu->x)));
                                                        /* $83:CD35 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));          /* $83:CD38 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));          /* $83:CD39 */
        And8(cpu, 0x07u);                              /* $83:CD3A */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0692u, cpu->x),
            A8(cpu));                                  /* $83:CD3C */
        TransferAToX(cpu);                             /* $83:CD3F */
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x83c1a5u, cpu->x)));
                                                        /* $83:CD40 */
        cpu->carry = 0;                                /* $83:CD44 */
        Adc8(cpu, 0x24u);                              /* $83:CD45 */
        PRIMARY_ACTION_OR_BOUNDARY(0xcd4au);           /* $83:CD47 */
        IncrementY16(cpu);                             /* $83:CD4B */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83cd4fu: {
        uint16_t helper_pc;

        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CD4F */
        TransferDirectToA(cpu);                        /* $83:CD51 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0692u, cpu->x)));
                                                        /* $83:CD52 */
        TransferAToX(cpu);                             /* $83:CD55 */
        helper_pc = Read16ProgramIndexed(memory, cpu, 0xcd66u, cpu->x);
        SimulateJsrFrame(memory, cpu, 0xcd58u);        /* $83:CD56 */
        if (!PrimaryFacingCompare(memory, cpu, helper_pc)) {
            cpu->resume_pc = 0x830000u | helper_pc;
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        SimulateRtsFrame(memory, cpu);
        if (!cpu->zero && cpu->carry)                  /* $83:CD59 */
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CD5D */
        IncrementY16(cpu);                             /* $83:CD60 */
        IncrementY16(cpu);
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83cd92u:
    case 0x83d112u: {
        const uint8_t cd92 = (handler_pc & 0x00ffffffu) == 0x83cd92u;

        TransferDirectToA(cpu);                        /* $83:CD92 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
                                                        /* $83:CD93 */
        And8(cpu, 0x0fu);                              /* $83:CD95 */
        if (!cpu->zero) {
            TransferAToX(cpu);                         /* $83:CD99 */
            LoadA8(
                cpu, Read8(
                    memory,
                    LongIndexedAddress(
                        cd92 ? 0x83d457u : 0x83d447u, cpu->x)));
                                                        /* $83:CD9A */
            PRIMARY_ACTION_OR_BOUNDARY(cd92 ? 0xcda1u : 0xd121u);
        }
        IncrementY16(cpu);                             /* $83:C8C6 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);
    }

    case 0x83ce73u:
        LoadA8(cpu, 0x60u);                            /* $83:CE73 */
        PRIMARY_ACTION_OR_BOUNDARY(0xce78u);           /* $83:CE75 */
        IncrementY16(cpu);                             /* $83:CE79 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83cab9u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CAB9 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
                                                        /* $83:CABB */
        Write8(
            memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
                                                        /* $83:CABE */
        SimulateJsrFrame(memory, cpu, 0xcac4u);        /* $83:CAC2 */
        PrimaryTargetDirection(memory, cpu);           /* $83:CA93 */
        SimulateRtsFrame(memory, cpu);
        if (cpu->carry) {
            IncrementY16(cpu);                         /* $83:CACE */
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        PRIMARY_ACTION_OR_BOUNDARY(0xcacau);           /* $83:CAC7 */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:CACB */

    case 0x83c98au:
    case 0x83ccd7u: {
        const uint8_t flee = (handler_pc & 0x00ffffffu) == 0x83ccd7u;

        PrimaryLeaderToProbe(
            memory, cpu, flee ? 0xccd9u : 0xc98cu);    /* JSR $C0EF */
        PrimaryApproachProbe(
            memory, cpu, flee ? 0xccdcu : 0xc98fu);    /* JSR $C9C5 */
        if (cpu->carry) {
            if (flee) {
                ExchangeAccumulatorBytes(cpu);         /* $83:CCDF */
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                TransferAToX(cpu);                     /* $83:CCE3 */
                LoadA8(
                    cpu, Read8(
                        memory, LongIndexedAddress(0x83cd2au, cpu->x)));
            }
            PRIMARY_ACTION_OR_BOUNDARY(flee ? 0xccebu : 0xc995u);
        }
        IncrementY16(cpu);                             /* $83:C996 */
        if (flee) {
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);
    }

    case 0x83d01eu:
        LoadXDirect(memory, cpu, 0xa9u);               /* $83:D01E */
        TransferDirectToA(cpu);                        /* $83:D020 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
        TransferAToX(cpu);                             /* $83:D025 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x0006bau, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x0006e2u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        PrimaryApproachProbe(memory, cpu, 0xd034u);    /* $83:D032 */
        if (cpu->carry)
            PRIMARY_ACTION_OR_BOUNDARY(0xd03au);       /* $83:D037 */
        IncrementY16(cpu);                             /* $83:D03B */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83cf6eu:
        LoadA8(cpu, 0x5fu);                            /* $83:CF6E */
        PRIMARY_ACTION_OR_BOUNDARY(0xcf73u);           /* $83:CF70 */
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CF74 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
        ExchangeAccumulatorBytes(cpu);                 /* $83:CF79 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
        LoadXDirect(memory, cpu, 0xa9u);               /* $83:CF7D */
        Write8(memory, LongIndexedAddress(0x7fdb9du, cpu->x), A8(cpu));
        ExchangeAccumulatorBytes(cpu);                 /* $83:CF83 */
        Write8(memory, LongIndexedAddress(0x7fdb9cu, cpu->x), A8(cpu));
        IncrementY16(cpu);                             /* $83:CF88 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83cf8cu: {
        uint8_t ahead;

        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CF8C */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        if (!cpu->zero) {
            ahead = cpu->carry;
            LoadA8(cpu, ahead ? 0x02u : 0x03u);        /* $83:CF96 */
        } else {
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
            Compare8(
                cpu, A8(cpu),
                Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
            if (cpu->zero) {
                IncrementY16(cpu);                     /* $83:CFB3 */
                IncrementY16(cpu);
                IncrementY16(cpu);
                dispatch = PrimaryRedispatch(memory, cpu, 0);
                return PrimaryStepRedispatched(dispatch);
            }
            ahead = cpu->carry;
            LoadA8(cpu, ahead ? 0x00u : 0x01u);        /* $83:CFA6 */
        }
        PRIMARY_ACTION_OR_BOUNDARY(0xcfafu);           /* $83:CFAC */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:CFB0 */
    }

    case 0x83cfb9u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CFB9 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        AslA8(cpu);                                    /* $83:CFC0 */
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        for (uint8_t axis = 0; axis < 2u; ++axis) {    /* $83:CFC3 */
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(
                        cpu, axis ? 0x06e2u : 0x06bau, cpu->x)));
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            Write8(
                memory, DirectAddress(cpu, axis ? 0xa0u : 0x9fu), A8(cpu));
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
            Write8(
                memory, DirectAddress(cpu, axis ? 0xa2u : 0xa1u), A8(cpu));
        }
        LoadX16(cpu, 0x0000u);                         /* $83:CFDD */
        do {
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x05d2u, cpu->x)));
            Compare8(cpu, A8(cpu), 0xffu);             /* $83:CFE3 */
            if (cpu->zero)
                goto cfb9_next;
            Compare8(cpu, A8(cpu), 0x80u);             /* $83:CFE7 */
            if (!cpu->carry)
                goto cfb9_next;
            Compare16(cpu, cpu->x, Read16Direct(memory, cpu, 0xa7u));
            if (cpu->zero)                             /* $83:CFEB */
                goto cfb9_next;
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x9fu)));
            if (!cpu->carry)
                goto cfb9_next;
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa1u)));
            if (cpu->carry)
                goto cfb9_next;
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa0u)));
            if (!cpu->carry)
                goto cfb9_next;
            DecrementA8(cpu);                          /* $83:D001 */
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa2u)));
            if (cpu->carry)
                goto cfb9_next;

            TransferXToA(cpu);                         /* $83:D006 */
            LoadXDirect(memory, cpu, 0xa9u);
            Write8(
                memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
            IncrementY16(cpu);                         /* $83:D00D */
            IncrementY16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
cfb9_next:
            LoadX16(cpu, (uint16_t)(cpu->x + 1u));     /* $83:D014 */
            Compare16(cpu, cpu->x, 0x0028u);
        } while (!cpu->zero);
        IncrementY16(cpu);                             /* $83:D01A */
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83d2b4u);

    case 0x83d09au:
    case 0x83ca19u: {
        const uint8_t d09a = (handler_pc & 0x00ffffffu) == 0x83d09au;
        PrimaryListSearch search;

        LoadX16(cpu, d09a ? 0x0002u : 0x0026u);        /* $83:D09A */
        LoadA8(cpu, d09a ? 0x0fu : 0x04u);             /* $83:D09D */
        search = PrimaryApproachListedPoint(
            memory, cpu, d09a ? 0xd0a1u : 0xca20u);    /* JSR $D0AA */
        if (search == PRIMARY_LIST_BOUNDARY) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        if (search == PRIMARY_LIST_EXHAUSTED)
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:D0A4 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83ce7du:
    case 0x83cf1au: {
        const uint8_t around_leader =
            (handler_pc & 0x00ffffffu) == 0x83cf1au;

        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CE7D */
        LoadA8(
            cpu, Read8(
                memory,
                AbsoluteIndexedAddress(
                    cpu, 0x06bau, around_leader ? 0 : cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x63u), A8(cpu));
        LoadA8(
            cpu, Read8(
                memory,
                AbsoluteIndexedAddress(
                    cpu, 0x06e2u, around_leader ? 0 : cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x64u), A8(cpu));
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        if (around_leader) {
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
            Write8(memory, DirectAddress(cpu, 0x65u), A8(cpu));
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
        } else {
            LoadA8(cpu, 0x10u);                        /* $83:CE8F */
            Write8(memory, DirectAddress(cpu, 0x65u), A8(cpu));
            LoadA8(cpu, 0x02u);
        }
        Write8(memory, DirectAddress(cpu, 0x66u), A8(cpu));
        LoadA8(cpu, 0x00u);
        if (!PrimaryWanderAxis(
                memory, cpu, around_leader ? 0xcf3au : 0xce9bu)) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        LoadA8(cpu, 0x02u);
        if (!PrimaryWanderAxis(
                memory, cpu, around_leader ? 0xcf3fu : 0xcea0u)) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        if (!around_leader) {
            PrimaryRecordProbe(memory, cpu, 0xcea3u);  /* $83:CEA1 */
            IncrementY16(cpu);                         /* $83:CEA4 */
            dispatch = PrimaryRedispatch(memory, cpu, 0);
            return PrimaryStepRedispatched(dispatch);
        }
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CF40 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, LongIndexedAddress(0x7fe5f6u, cpu->x)));
        if (cpu->carry) {
            Compare8(
                cpu, A8(cpu),
                Read8(memory, LongIndexedAddress(0x7fe646u, cpu->x)));
            if (!cpu->carry) {
                LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
                Compare8(
                    cpu, A8(cpu),
                    Read8(
                        memory, LongIndexedAddress(0x7fe61eu, cpu->x)));
                if (cpu->carry) {
                    Compare8(
                        cpu, A8(cpu),
                        Read8(
                            memory,
                            LongIndexedAddress(0x7fe66eu, cpu->x)));
                    if (!cpu->carry) {
                        PrimaryRecordProbe(memory, cpu, 0xcf60u);
                        for (int i = 0; i < 5; ++i)    /* $83:CF61 */
                            IncrementY16(cpu);
                        dispatch = PrimaryRedispatch(memory, cpu, 0);
                        return PrimaryStepRedispatched(dispatch);
                    }
                }
            }
        }
        IncrementY16(cpu);                             /* $83:CF69 */
        IncrementY16(cpu);
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83d2b4u);
    }

    case 0x83cda5u: {
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CDA5 */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06bau, 0)));
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06e2u, 0)));
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        IncrementY16(cpu);                             /* $83:CDB7 */
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        Write8(memory, DirectAddress(cpu, 0x93u), A8(cpu));
        TransferDirectToA(cpu);                        /* $83:CDBD */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0692u, 0)));
        TransferAToX(cpu);
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x83ce6bu, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x9fu), 0x00u);  /* $83:CDC8 */
        Write8(memory, DirectAddress(cpu, 0xa0u), 0x00u);
        LoadA8(cpu, 0xffu);
        Write8(memory, DirectAddress(cpu, 0xa1u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0xa2u), A8(cpu));
        if (!PrimaryMeasureRun(memory, cpu, 0xcdd4u)) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9du)));
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x93u)));
        if (!cpu->carry)
            return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, 0x83d2b4u);              /* $83:CDF3 */
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06bau, 0)));
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x06e2u, 0)));
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x93u)));
        if (!PrimaryWalkSteps(memory, cpu, 0xcde9u)) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        PrimaryRecordProbe(memory, cpu, 0xcdecu);      /* $83:CDEA */
        IncrementY16(cpu);                             /* $83:CDED */
        IncrementY16(cpu);
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83d03fu: {
        uint8_t direction;

        PrimaryActorToProbe(memory, cpu, 0xd041u);     /* $83:D03F */
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D042 */
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        TransferDirectToA(cpu);                        /* $83:D04A */
        IncrementY16(cpu);
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        if (!PrimaryStepBlocked(memory, cpu, 0xd051u)) {
            result.handler_pc = cpu->resume_pc;
            return result;
        }
        if (cpu->zero) {
            LoadXDirect(memory, cpu, 0xa7u);           /* $83:D054 */
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
            direction = A8(cpu);
            Compare8(cpu, direction, 0x00u);           /* $83:D059 */
            if (!cpu->zero) {
                Compare8(cpu, direction, 0x04u);       /* $83:D068 */
                if (!cpu->zero)
                    Compare8(cpu, direction, 0x02u);   /* $83:D076 */
            }
            if (direction == 0x00u || direction == 0x02u ||
                direction == 0x04u) {
                const uint8_t y_axis = direction != 0x02u;
                const uint32_t actor = y_axis ? 0x0006e2u : 0x0006bau;
                const uint32_t bound =
                    direction == 0x00u ? 0x7fe66eu :
                    direction == 0x04u ? 0x7fe61eu : 0x7fe5f6u;
                if (direction == 0x00u) {
                    LoadA8(cpu, Read8(memory, LongIndexedAddress(bound, cpu->x)));
                    DecrementA8(cpu);
                    Compare8(
                        cpu, A8(cpu),
                        Read8(memory, LongIndexedAddress(actor, cpu->x)));
                } else {
                    LoadA8(cpu, Read8(memory, LongIndexedAddress(actor, cpu->x)));
                    Compare8(
                        cpu, A8(cpu),
                        Read8(memory, LongIndexedAddress(bound, cpu->x)));
                }
            } else {
                LoadA8(
                    cpu, Read8(
                        memory, LongIndexedAddress(0x7fe646u, cpu->x)));
                DecrementA8(cpu);                      /* $83:D088 */
                Compare8(
                    cpu, A8(cpu),
                    Read8(memory, LongIndexedAddress(0x0006bau, cpu->x)));
            }
            if (!cpu->zero && cpu->carry) {            /* $83:D08D */
                IncrementY16(cpu);                     /* $83:D094 */
                IncrementY16(cpu);
                IncrementY16(cpu);
                dispatch = PrimaryRedispatch(memory, cpu, 0);
                return PrimaryStepRedispatched(dispatch);
            }
        }
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83d2b4u);                  /* $83:D091 */
    }

    case 0x83c918u:
    case 0x83cad3u:
    case 0x83cbb1u: {
        const uint16_t entry = (uint16_t)handler_pc;

        SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
        Lufia2ActorPrimaryReset(memory, cpu);          /* JSR $C947 */
        SimulateRtsFrame(memory, cpu);
        if (entry == 0xc918u) {
            LoadXDirect(memory, cpu, 0xa7u);           /* $83:C91B */
            LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
            Or8(cpu, 0x01u);
            StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
            LoadAAbsolute8(memory, cpu, 0x09a7u, 0);   /* $83:C925 */
            BitImmediate8(cpu, 0x01u);
            if (!cpu->zero) {
                LoadA8(cpu, 0x09u);                    /* $83:C92C */
                StoreAAbsolute8(memory, cpu, 0x070au, cpu->x);
                SimulateJslFrame(memory, cpu, 0x83u, 0xc934u);
                Lufia2ActorLoadPrimaryScript(memory, cpu);
                SimulateRtlFrame(memory, cpu);
            } else {
                LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
                BitImmediate8(cpu, 0x20u);             /* $83:C93A */
                if (cpu->zero) {
                    LoadA8(cpu, 0x04u);
                    Write8(
                        memory, LongIndexedAddress(0x7fe4deu, cpu->x),
                        A8(cpu));
                }
            }
        } else if (entry == 0xcad3u) {
            LoadA8(cpu, 0x01u);                        /* $83:CAD6 */
            TestBitsAbsolute8(memory, cpu, 0x0622u, 0);
            SimulateJsrFrame(memory, cpu, 0xcaddu);
            Lufia2ActorClearSlotLinks(memory, cpu);    /* JSR $CB65 */
            SimulateRtsFrame(memory, cpu);
        }
        result.flow = LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2;
        result.handler_pc = 0x83c8d2u;
        return result;
    }

    case 0x83ca29u:
        LoadA8(cpu, 0x40u);                            /* $83:CA29 */
        TestBitsAbsolute8(memory, cpu, 0x0622u, 1);
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:CA2E */
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xca3du);        /* $83:CA3B */
        PrimaryTargetDirection(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        if (!cpu->carry) {
            Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
            TransferAToX(cpu);                         /* $83:CA42 */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
            SimulateJslFrame(memory, cpu, 0x83u, 0xca4au);
            if (Lufia2ActorMovementStep(memory, cpu) == 0) {
                result.handler_pc = cpu->resume_pc;
                return result;
            }
            SimulateRtlFrame(memory, cpu);
            PrimaryCollisionIndex(memory, cpu, 0xca4du, 1); /* $83:CA4B */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
            BitImmediate8(cpu, 0x0bu);                 /* $83:CA52 */
            if (cpu->zero) {
                TransferDirectToA(cpu);                /* $83:CA56 */
                LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x56u)));
                PRIMARY_ACTION_OR_BOUNDARY(0xca5cu);
                return Lufia2ActorPrimaryScriptExecuteKnownHandler(
                    memory, cpu, 0x83c8c7u);          /* $83:CA5D */
            }
            SimulateJslFrame(memory, cpu, 0x83u, 0xca63u);
            Lufia2ActorBlockedEvent(memory, cpu);      /* $83:CA60 */
            SimulateRtlFrame(memory, cpu);
        }
        IncrementY16(cpu);                             /* $83:CA64 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);

    case 0x83d135u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D135 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
        LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        IncrementY16(cpu);                             /* $83:D143 */
        IncrementY16(cpu);
        IncrementY16(cpu);
        SimulateJslFrame(memory, cpu, 0x83u, 0xd149u);
        Lufia2ActorSyncFinePosition(memory, cpu);      /* $83:A746 */
        SimulateRtlFrame(memory, cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d1b5u:
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);  /* $83:D1B5 */
        SimulateJslFrame(memory, cpu, 0x83u, 0xd1bbu);
        Lufia2QueueDeferredSound(memory, cpu);         /* $84:8766 */
        SimulateRtlFrame(memory, cpu);
        IncrementY16(cpu);                             /* $83:D1BC */
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);
        return PrimaryStepRedispatched(dispatch);

    case 0x83d1feu:
    case 0x83d207u: {
        const uint8_t display = (handler_pc & 0x00ffffffu) == 0x83d1feu;

        SimulateJsrFrame(
            memory, cpu, display ? 0xd200u : 0xd209u);
        if (display)
            Lufia2ActorAddDisplayOffset(memory, cpu);  /* JSR $FACB */
        else
            Lufia2ActorMoveFinePosition(memory, cpu);  /* JSR $FA81 */
        SimulateRtsFrame(memory, cpu);
        TransferAToY(cpu);                             /* $83:D201 */
        SetAccumulatorWidth(cpu, 1);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83d176u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D176 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x1291u, cpu->x)));
        And8(cpu, 0xf8u);                              /* $83:D17B */
        Or8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x1291u, cpu->x),
            A8(cpu));                                  /* $83:D180 */
        IncrementY16(cpu);                             /* $83:D183 */
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d188u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D188 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        Write8(
            memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
        IncrementY16(cpu);                             /* $83:D191 */
        IncrementY16(cpu);
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83d196u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D196 */
        LoadA8(cpu, 0x80u);                            /* $83:D198 */
        Or8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x)));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0622u, cpu->x),
            A8(cpu));                                  /* $83:D19D */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        Write8(
            memory, LongIndexedAddress(0x7fe5a6u, cpu->x), A8(cpu));
        LoadA8(cpu, 0x1cu);                            /* $83:D1A7 */
        Write8(
            memory, LongIndexedAddress(0x7fe466u, cpu->x), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xd1afu);        /* $83:D1AD */
        PrimaryInstallSecondaryScript(memory, cpu);    /* $83:D3F7 */
        SimulateRtsFrame(memory, cpu);
        IncrementY16(cpu);                             /* $83:D1B0 */
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d1c1u:
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x09a1u, 0)));
                                                        /* $83:D1C1 */
        if (cpu->negative) {
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
            PRIMARY_ACTION_OR_BOUNDARY(0xd1ccu);       /* $83:D1C9 */
        }
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);                  /* $83:D1CD */

    case 0x83d1d0u: {
        uint16_t pointer;

        IncrementY16(cpu);                             /* $83:D1D0 */
        Write16Direct(memory, cpu, 0x54u, cpu->y);     /* $83:D1D1 */
        TransferDirectToA(cpu);                        /* $83:D1D3 */
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0692u, 0)));
        TransferAToY(cpu);                             /* $83:D1D7 */
        SetAccumulatorWidth(cpu, 0);                   /* $83:D1D8 */
        pointer = Read16Direct(memory, cpu, 0x54u);
        LoadA16(
            cpu, Read16AbsoluteIndexed(memory, cpu, pointer, cpu->y));
                                                        /* $83:D1DA */
        cpu->carry = 0;                                /* $83:D1DC */
        Add16Immediate(cpu, 0xa1d4u);                  /* $83:D1DD */
        TransferAToY(cpu);                             /* $83:D1E0 */
        SetAccumulatorWidth(cpu, 1);                   /* $83:D1E1 */
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83d1e6u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D1E6 */
        TransferDirectToA(cpu);                        /* $83:D1E8 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0692u, cpu->x)));
        TransferAToX(cpu);                             /* $83:D1EC */
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x83c1a5u, cpu->x)));
        cpu->carry = 0;                                /* $83:D1F1 */
        Adc8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        PRIMARY_ACTION_OR_BOUNDARY(0xd1f8u);           /* $83:D1F5 */
        IncrementY16(cpu);                             /* $83:D1F9 */
        IncrementY16(cpu);
        return Lufia2ActorPrimaryScriptExecuteKnownHandler(
            memory, cpu, 0x83c8c7u);

    case 0x83d210u: {
        uint8_t take;

        SetAccumulatorWidth(cpu, 0);                   /* $83:D210 */
        LoadA16(
            cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->y));
        cpu->carry = 0;                                /* $83:D215 */
        Add16Immediate(cpu, 0xa1d4u);                  /* $83:D216 */
        Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                   /* $83:D21B */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0003u, cpu->y)));
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        PrimaryTargetRecordIndex(memory, cpu, 0xd224u); /* $83:D222 */
        SetAccumulatorWidth(cpu, 0);                   /* $83:D225 */
        LoadA16(cpu, cpu->y);                          /* $83:D227 */
        cpu->carry = 0;                                /* $83:D228 */
        Add16Immediate(cpu, 0x0006u);                  /* $83:D229 */
        Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                   /* $83:D22E */
        LoadA8(
            cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
        ExchangeAccumulatorBytes(cpu);                 /* $83:D234 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
                                                        /* $83:D235 */
        if (cpu->zero) {
            ExchangeAccumulatorBytes(cpu);             /* $83:D270 */
            DecrementA8(cpu);
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
            take = cpu->carry;
        } else {
            const uint8_t mode = A8(cpu);

            for (uint8_t n = 1; n <= 4u; ++n) {        /* $83:D23A */
                Compare8(cpu, mode, n);
                if (cpu->zero)
                    break;
            }
            ExchangeAccumulatorBytes(cpu);
            if (mode == 0x03u)
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));  /* $83:D259 */
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
            if (mode == 0x02u)
                take = cpu->zero;                      /* $83:D263 */
            else if (mode <= 0x04u)
                take = cpu->carry;
            else
                take = !cpu->zero;                     /* $83:D24B */
        }
        if (take) {
            LoadYDirect16(memory, cpu, 0x56u);         /* $83:D278 */
            StoreYDirect16(memory, cpu, 0x2au);
        }
        dispatch = PrimaryRedispatch(memory, cpu, 1);  /* $83:C85A */
        return PrimaryStepRedispatched(dispatch);
    }

    case 0x83d293u:
        LoadXDirect(memory, cpu, 0xa7u);               /* $83:D293 */
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)));
        IncrementY16(cpu);                             /* $83:D298 */
        And8(cpu, Read8(memory, LongIndexedAddress(0x7fe57eu, cpu->x)));
        if (cpu->zero)
            return PrimaryStepRedispatched(
                PrimaryJumpOperand(memory, cpu, 0x0002u)); /* $83:D2A4 */
        IncrementY16(cpu);                             /* $83:D29F */
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);

    case 0x83d2e6u:
    case 0x83d2f6u:
    case 0x83d30bu: {
        const uint16_t entry = (uint16_t)handler_pc;
        const uint32_t target = 0x7fe5a6u;

        PrimaryTargetRecordIndex(
            memory, cpu, (uint16_t)(entry + 2u));      /* JSR $D27F */
        if (entry == 0xd2e6u) {
            LoadA8(
                cpu, Read8(
                    memory,
                    AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
        } else {
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(target, cpu->x)));
            cpu->carry = entry == 0xd30bu;
            if (entry == 0xd2f6u)
                Adc8(
                    cpu, Read8(
                        memory,
                        AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
            else
                Sbc8(
                    cpu, Read8(
                        memory,
                        AbsoluteIndexedAddress(cpu, 0x0002u, cpu->y)));
        }
        Write8(memory, LongIndexedAddress(target, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        dispatch = PrimaryRedispatch(memory, cpu, 0);  /* $83:C85C */
        return PrimaryStepRedispatched(dispatch);
    }

    default:
        cpu->resume_pc = result.handler_pc;
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
        cpu->resume_pc = 0x83fb17u;
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

static uint8_t PackStatus(const Lufia2ActorFrontendCpu *cpu) {
    return (uint8_t)(
        (cpu->negative ? 0x80u : 0u) |
        (cpu->overflow ? 0x40u : 0u) |
        (cpu->accumulator_is_8_bit ? 0x20u : 0u) |
        (cpu->index_is_8_bit ? 0x10u : 0u) |
        (cpu->decimal ? 0x08u : 0u) |
        (cpu->irq_disable ? 0x04u : 0u) |
        (cpu->zero ? 0x02u : 0u) |
        (cpu->carry ? 0x01u : 0u));
}

static void UnpackStatus(Lufia2ActorFrontendCpu *cpu, uint8_t status) {
    cpu->negative = (status & 0x80u) != 0;
    cpu->overflow = (status & 0x40u) != 0;
    cpu->decimal = (status & 0x08u) != 0;
    cpu->irq_disable = (status & 0x04u) != 0;
    cpu->zero = (status & 0x02u) != 0;
    cpu->carry = status & 0x01u;
    SetAccumulatorWidth(cpu, status & 0x20u);
    SetIndexWidth(cpu, status & 0x10u);
}

static void PushY(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->index_is_8_bit)
        Push8(memory, cpu, (uint8_t)(cpu->y >> 8));
    Push8(memory, cpu, (uint8_t)cpu->y);
}

static uint16_t PullIndexValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint8_t low = Pull8(memory, cpu);
    uint16_t value;

    if (cpu->index_is_8_bit) {
        SetNz8(cpu, low);
        return low;
    }
    value = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
    SetNz16(cpu, value);
    return value;
}

static void LoadX8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->x = value;
    SetNz8(cpu, value);
}

static void LoadY8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->y = value;
    SetNz8(cpu, value);
}

/* $80:832D: lagged XOR refill, lags 24 and 31. */
static void RandomRefill(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX8(cpu, 0x00u);                                        /* 832D */
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0540u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x18u);                 /* 8339 */
    } while (!cpu->zero);
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0509u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x37u);                 /* 8347 */
    } while (!cpu->zero);
}

/* PHB/PHK/PLB/PHX/PHY/PHP/SEP #$30 */
static void RandomEnter(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* Next table index in X, refilling past $36. */
static void RandomAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t refill_return) {
    LoadX8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0559u, 0)));
    LoadX8(cpu, (uint8_t)(cpu->x + 1u));
    Compare8(cpu, (uint8_t)cpu->x, 0x37u);
    if (cpu->carry) {
        SimulateJsrFrame(memory, cpu, refill_return);
        RandomRefill(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadX8(cpu, 0x00u);
    }
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0559u, 0),
        (uint8_t)cpu->x);
}

/* PLP/PLY/PLX/PLB */
static void RandomLeave(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
}

void Lufia2RandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    RandomEnter(memory, cpu);                                  /* 82C7 */
    RandomAdvance(memory, cpu, 0x82d9u);                       /* 82CF */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
    RandomLeave(memory, cpu);                                  /* 82E2 */
}

void Lufia2RandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    RandomEnter(memory, cpu);                                  /* 8299 */
    ExchangeAccumulatorBytes(cpu);                             /* 82A1 */
    RandomAdvance(memory, cpu, 0x82acu);                       /* 82A2 */
    ExchangeAccumulatorBytes(cpu);                             /* 82B2 */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x4202u, 0), A8(cpu));
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x4203u, 0), A8(cpu));
    LoadA8(cpu, 0x00u);                                        /* 82BC */
    ExchangeAccumulatorBytes(cpu);                             /* 82BE */
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x4217u, 0)));
    RandomLeave(memory, cpu);                                  /* 82C2 */
}

static void PrimaryCallRandom(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static void PrimaryCallRandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:CA93: step toward $7F:E5A6/E5CE target. */
static void PrimaryTargetDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* CA93 */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06bau, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
    if (!cpu->zero) {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x02u : 0x03u);                    /* CA9E */
        cpu->carry = 0;                                        /* CAB5 */
        return;
    }
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x06e2u, cpu->x)));
    Compare8(
        cpu, A8(cpu),
        Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
    if (!cpu->zero) {
        const uint8_t ahead = cpu->carry;
        LoadA8(cpu, ahead ? 0x00u : 0x01u);                    /* CAAF */
        cpu->carry = 0;
        return;
    }
    cpu->carry = 1;                                            /* CAB7 */
}

/* $83:CD6E..$83:CD91 facing comparators. */
static uint8_t PrimaryFacingCompare(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t helper_pc) {
    uint16_t base;
    uint8_t leader_first;

    switch (helper_pc) {
    case 0xcd6eu: base = 0x06e2u; leader_first = 1; break;
    case 0xcd77u: base = 0x06bau; leader_first = 0; break;
    case 0xcd80u: base = 0x06e2u; leader_first = 0; break;
    case 0xcd89u: base = 0x06bau; leader_first = 1; break;
    default: return 0;
    }

    LoadXDirect(memory, cpu, 0xa7u);
    if (leader_first) {
        LoadA8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, base, 0)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, base, cpu->x)));
    } else {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, base, cpu->x)));
        Compare8(
            cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, base, 0)));
    }
    return 1;
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
        helper_pc != 0xd3d7u && helper_pc != 0xd3e5u) {
        cpu->resume_pc = 0x83d370u;
        return LUFIA2_ACTOR_PRIMARY_ACTION_UNKNOWN_D370_TARGET;
    }

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

    /* X8: F9D4's PHX/PLA pair derails its RTS. */
    if (cpu->index_is_8_bit) {
        cpu->resume_pc = 0x83d38du;
        return LUFIA2_ACTOR_PRIMARY_ACTION_X8_BOUNDARY_D38D;
    }
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

Lufia2ActorPrimaryUpdateResult Lufia2ActorPrimaryUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;
    Lufia2ActorScriptDispatchResult dispatch;
    Lufia2ActorPrimaryFlow flow;
    uint32_t handler;
    uint32_t steps;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83c83bu;
    result.dispatches = 0;

    flow = Lufia2ActorPrimaryUpdateFrontend(memory, cpu);      /* C7F8 */
    if (flow == LUFIA2_ACTOR_PRIMARY_RETURN)
        return result;
    if (flow == LUFIA2_ACTOR_PRIMARY_CONTINUE_C808) {
        DecrementA8(cpu);                                      /* C808 */
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x1291u, cpu->x),
            A8(cpu));
    }

    dispatch = Lufia2ActorPrimaryScriptDispatch(memory, cpu);  /* C83C */
    handler = dispatch.handler_pc;
    result.dispatches = 1;
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        const Lufia2ActorPrimaryScriptStepResult step =
            Lufia2ActorPrimaryScriptExecuteKnownHandler(
                memory, cpu, handler);

        if (step.flow == LUFIA2_ACTOR_PRIMARY_SCRIPT_REDISPATCHED) {
            handler = step.handler_pc;
            ++result.dispatches;
            continue;
        }
        if (step.flow == LUFIA2_ACTOR_PRIMARY_SCRIPT_CONTINUE_C8D2) {
            PullDataBank(memory, cpu);                         /* C8D2 */
            result.pc = 0x83c8d3u;
            return result;
        }
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = step.handler_pc;
        return result;
    }
    cpu->resume_pc = handler;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = handler;
    return result;
}

/* Secondary actor VM ($83:D508). */

static void IncrementX16(Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, (uint16_t)(cpu->x + 1u));
}

static void TransferYToX(Lufia2ActorFrontendCpu *cpu) {
    if (cpu->index_is_8_bit) {
        cpu->x = (uint8_t)cpu->y;
        SetNz8(cpu, (uint8_t)cpu->x);
    } else {
        cpu->x = cpu->y;
        SetNz16(cpu, cpu->x);
    }
}

static void StepMemory8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t address,
    int delta) {
    const uint8_t value = (uint8_t)(Read8(memory, address) + delta);
    Write8(memory, address, value);
    SetNz8(cpu, value);
}

/* $83:D5C3: STX $2A, then two-level table dispatch. */
static Lufia2ActorScriptDispatchResult SecondaryRedispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorScriptDispatchResult result;
    uint32_t first;

    StoreXDirect16(memory, cpu, 0x2au);                        /* D5C3 */
    LoadYDirect16(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    result.opcode = LoadScriptByteX(memory, cpu);
    And8(cpu, 0xf0u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    first = JumpProgramTable(memory, cpu, 0xdf17u);            /* D5D1 */
    if (first == 0x83d5d4u || first == 0x83d5e0u || first == 0x83d5ecu) {
        const uint16_t table = first == 0x83d5d4u ? 0xdf37u :
                               first == 0x83d5e0u ? 0xdf57u : 0xdf77u;
        LoadXDirect16(memory, cpu, 0x2au);
        (void)LoadScriptByteX(memory, cpu);
        And8(cpu, 0x0fu);
        AslA8(cpu);
        TransferAToX(cpu);
        result.handler_pc = JumpProgramTable(memory, cpu, table);
    } else {
        result.handler_pc = first;
    }
    return result;
}

/* $83:DA8A: clear $0622 bit 7 and the walk counter. */
static void SecondaryStopScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    TransferDirectToA(cpu);                                    /* DA94 */
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:DA71: leader stop sets $05B5 bit 4. */
static void SecondaryLeaderStopFlag(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, 0xa7u);                         /* DA71 */
    if (cpu->zero) {
        uint8_t mark;

        LoadA8(cpu, Read8(memory, 0x7fd0a1u));
        BitImmediate8(cpu, 0x04u);
        mark = !cpu->zero;
        if (!mark) {
            LoadAAbsolute8(memory, cpu, 0x0622u, cpu->y);      /* DA7D */
            BitImmediate8(cpu, 0x08u);
            mark = cpu->zero;
        }
        if (mark) {
            LoadA8(cpu, 0x10u);                                /* DA84 */
            TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:DA5B: probe = actor tile moved one step by facing. */
static uint8_t SecondaryAdvanceProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DA5B */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xda6du);             /* DA6A */
    if (Lufia2ActorMovementStep(memory, cpu) == 0)
        return 0;
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DA6E */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:D87D: probe the step in direction A from the actor tile. */
static uint8_t SecondaryStepBlocked(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushAccumulator8(memory, cpu);                             /* D87D */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    TransferYToX(cpu);                                         /* D888 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferDirectToA(cpu);                                    /* D88F */
    TransferXToA(cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);                                    /* D899 */
        Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    }
    LoadA8(cpu, Pull8(memory, cpu));                           /* D89D */
    if (!PrimaryStepBlockedBody(memory, cpu))
        return 0;
    SimulateRtsFrame(memory, cpu);                             /* D8A2 */
    return 1;
}

/* Occupancy bit 0 at the probe tile, both columns if wide. */
static void SecondaryOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address,
    uint8_t set) {
    PrimaryCollisionIndex(memory, cpu, return_address, 1);
    for (uint8_t column = 0; column < 2u; ++column) {
        const uint32_t cell =
            LongIndexedAddress(column ? 0x7e4001u : 0x7e4000u, cpu->x);

        if (column) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x9au)));
            Compare8(cpu, A8(cpu), 0x02u);
            if (!cpu->carry)
                break;
        }
        LoadA8(cpu, Read8(memory, cell));
        if (set)
            Or8(cpu, 0x01u);
        else
            And8(cpu, 0xfeu);
        Write8(memory, cell, A8(cpu));
    }
}

/* $83:D9C6: move occupancy and the actor one step by facing. */
static uint8_t SecondaryCommitStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);              /* D9C6 */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    TransferYToX(cpu);                                         /* D9D0 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
    TransferXToA(cpu);                                         /* D9D7 */
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd0feu));
        if (!cpu->zero) {
            LoadA8(cpu, 0x01u);                                /* D9E0 */
            Write8(memory, DirectAddress(cpu, 0x9au), A8(cpu));
        }
    }
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->y);              /* D9E4 */
    BitImmediate8(cpu, 0x0au);
    if (cpu->zero) {
        SecondaryOccupancy(memory, cpu, 0xd9edu, 0);           /* D9EB */
        if (!SecondaryAdvanceProbe(memory, cpu, 0xda0au))      /* DA08 */
            return 0;
        SecondaryOccupancy(memory, cpu, 0xda0du, 1);           /* DA0B */
    }
    LoadAAbsolute8(memory, cpu, 0x070au, cpu->y);              /* DA28 */
    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x09a6u, 0);               /* DA2F */
        BitImmediate8(cpu, 0x01u);
        if (cpu->zero) {
            /* Shift the follower trail in $09A1.. by one. */
            LoadX16(cpu, 0x0003u);                             /* DA36 */
            do {
                LoadAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
                StoreAAbsolute8(memory, cpu, 0x09a2u, cpu->x);
                LoadX16(cpu, (uint16_t)(cpu->x - 1u));
            } while (!cpu->negative);
            LoadXDirect(memory, cpu, 0xa7u);                   /* DA42 */
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x7fe466u, cpu->x)));
            Or8(cpu, 0x80u);
            StoreAAbsolute8(memory, cpu, 0x09a1u, 0);
        }
    }
    if (!SecondaryAdvanceProbe(memory, cpu, 0xda4fu))          /* DA4D */
        return 0;
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));     /* DA50 */
    StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:DD18 targets: signed step along facing, M=1 on exit. */
static uint8_t SecondaryFineStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;
    uint32_t pair;

    TransferAToX(cpu);                                         /* DCC8 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    SetAccumulatorWidth(cpu, 0);                               /* DCCB */
    target = Read16ProgramIndexed(memory, cpu, 0xdd18u, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xdccfu);                    /* DCCD */
    switch (target) {
    case 0xdd24u: pair = 0x7fde3eu; break;
    case 0xdd20u: pair = 0x7fde3eu; break;
    case 0xdd32u: pair = 0x7fddaeu; break;
    case 0xdd36u: pair = 0x7fddaeu; break;
    default:
        cpu->resume_pc = 0x830000u | target;
        return 0;
    }
    if (target == 0xdd20u || target == 0xdd32u) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));  /* DD20 */
        IncrementA16(cpu);
    }
    LoadXDirect(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
    Write16Long(memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

typedef enum SecondaryStepFlow {
    SECONDARY_STEP_UNKNOWN = 0,
    SECONDARY_STEP_REDISPATCHED = 1,
    SECONDARY_STEP_EXIT_D60D = 2,
} SecondaryStepFlow;

typedef struct SecondaryStep {
    SecondaryStepFlow flow;
    uint32_t handler_pc;
} SecondaryStep;

static SecondaryStep SecondaryRedispatched(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_REDISPATCHED;
    step.handler_pc = SecondaryRedispatch(memory, cpu).handler_pc;
    return step;
}

static SecondaryStep SecondaryExit(void) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_EXIT_D60D;
    step.handler_pc = 0x83d60du;
    return step;
}

static SecondaryStep SecondaryBoundary(const Lufia2ActorFrontendCpu *cpu) {
    SecondaryStep step;
    step.flow = SECONDARY_STEP_UNKNOWN;
    step.handler_pc = cpu->resume_pc;
    return step;
}

/* A16 cursor to $7F:E3EE+record, exit ($83:D605/$83:DD09). */
static SecondaryStep SecondarySaveCursorExit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:DC45: walk one tile over 16 sub-steps. */
static SecondaryStep SecondaryWalk(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* DC45 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    if (cpu->zero) {
        uint8_t commit = 0;

        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* DC4D */
        BitImmediate8(cpu, 0x0au);
        if (!cpu->zero) {
            commit = 1;
        } else {
            LoadAAbsolute8(memory, cpu, 0x057cu, 0);           /* DC54 */
            if (!cpu->zero) {
                LoadA8(cpu, 0x80u);
                And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
                if (!cpu->zero) {
                    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
                    if (cpu->zero)
                        commit = 1;                            /* DC63 */
                }
            }
            if (!commit) {
                LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);  /* DC65 */
                if (!SecondaryStepBlocked(memory, cpu, 0xdc6au))
                    return SecondaryBoundary(cpu);
                if (!cpu->zero) {
                    /* Blocked: skip the opcode and stop. */
                    SetAccumulatorWidth(cpu, 0);               /* DC6D */
                    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
                    IncrementA16(cpu);
                    return SecondarySaveCursorExit(memory, cpu);
                }
                commit = 1;
            }
        }
        if (commit && !SecondaryCommitStep(memory, cpu, 0xdc77u))
            return SecondaryBoundary(cpu);                     /* DC75 */
    }

    LoadXDirect(memory, cpu, 0xa7u);                           /* DC78 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe4deu, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
    And8(cpu, 0xfcu);                                          /* DC89 */
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    if (cpu->negative)
        goto save_cursor;                                      /* DC8D */
    LoadA8(cpu, 0x03u);                                        /* DC8F */
    TrbDirect8(memory, cpu, 0x32u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe48eu, cpu->x)));
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x32u)));
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x32u), A8(cpu));
    cpu->carry = 0;                                            /* DC9D */
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x), A8(cpu));
    Compare8(cpu, A8(cpu), 0x04u);                             /* DCA6 */
    if (cpu->zero || (Compare8(cpu, A8(cpu), 0x0cu), cpu->zero)) {
        const int delta = A8(cpu) == 0x04u ? 1 : -1;
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero)
            StepMemory8(                                       /* DCB1 */
                memory, cpu,
                AbsoluteIndexedAddress(cpu, 0x066au, cpu->x), delta);
    }
    TransferDirectToA(cpu);                                    /* DCC4 */
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->y);
    if (!SecondaryFineStep(memory, cpu))
        return SecondaryBoundary(cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DCD0 */
    LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
    BitImmediate8(cpu, 0x08u);
    if (!cpu->zero) {
        /* Bob offset from $83:DD44. */
        TransferDirectToA(cpu);                                /* DCD9 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
        DecrementA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83dd44u, cpu->x)));
        PrimarySignExtend(memory, cpu, 0xdce6u);
        LoadXDirect(memory, cpu, 0xa9u);                       /* DCE7 */
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect(memory, cpu, 0xa7u);                       /* DCF4 */
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x10u);                             /* DCFA */
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* DCFE */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
        IncrementA16(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);
    }

save_cursor:
    LoadXDirect(memory, cpu, 0xabu);                           /* DD09 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Long(
        memory, LongIndexedAddress(0x7fe3eeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return SecondaryExit();
}

/* $83:D5F8: next byte. */
static SecondaryStep SecondaryNextByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t length) {
    LoadXDirect(memory, cpu, 0x2au);
    while (length--)
        IncrementX16(cpu);
    return SecondaryRedispatched(memory, cpu);
}

/* $83:D7A5: actor tile to $8F/$91. */
static void SecondaryActorToProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $83:D99F: signed operands onto $7F:DC8C/DD1C; M=0 exit. */
static void SecondaryAddDisplayOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadYDirect16(memory, cpu, 0x2au);
    LoadXDirect(memory, cpu, 0xa9u);
    PrimaryAddSignedPair(memory, cpu, 0x7fdc8cu, 0x0001u, 0xd9a9u);
    SetAccumulatorWidth(cpu, 1);
    PrimaryAddSignedPair(memory, cpu, 0x7fdd1cu, 0x0002u, 0xd9bbu);
    SimulateRtsFrame(memory, cpu);
}

static void CopyLong16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t from,
    uint32_t to) {
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(from, cpu->x)));
    Write16Long(memory, LongIndexedAddress(to, cpu->x), cpu->accumulator);
}

static void ClearCellBit0(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t base) {
    const uint32_t address = LongIndexedAddress(base, cpu->x);
    LoadA8(cpu, Read8(memory, address));
    And8(cpu, 0xfeu);
    Write8(memory, address, A8(cpu));
}

/* $83:FA12: clear occupancy bit 0 under the actor. */
static void SecondaryClearOccupancy(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                           /* FA12 */
    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
    PrimaryCollisionIndex(memory, cpu, 0xfa1du, 0);            /* F9B6 */
    ClearCellBit0(memory, cpu, 0x7e4000u);                     /* FA1E */
    PushIndex(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x02u);
    cpu->x = PullIndexValue(memory, cpu);                      /* FA31 */
    if (cpu->carry)
        ClearCellBit0(memory, cpu, 0x7e4001u);                 /* FA34 */
}

/* $80:8450 sine, $80:8486 cosine; sign in bit 7. */
static void SecondaryWave(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t cosine,
    uint16_t return_address) {
    const uint8_t angle = A8(cpu);
    uint8_t index;
    uint8_t negative;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    if (!cosine) {
        if (angle < 0x2du) {
            index = angle;
            negative = 0;
        } else if (angle < 0x5au) {
            index = (uint8_t)(0x5au - angle);
            negative = 0;
        } else if (angle < 0x87u) {
            index = (uint8_t)(angle - 0x5au);
            negative = 1;
        } else {
            index = (uint8_t)(0xb4u - angle);
            negative = 1;
        }
    } else {
        if (angle < 0x2du) {
            index = (uint8_t)(0x2du - angle);
            negative = 0;
        } else if (angle < 0x5au) {
            index = (uint8_t)(angle - 0x2du);
            negative = 1;
        } else if (angle < 0x87u) {
            index = (uint8_t)(0x87u - angle);
            negative = 1;
        } else {
            index = (uint8_t)(angle - 0x87u);
            negative = 0;
        }
    }
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8084bfu, index)));
    if (negative)
        Or8(cpu, 0x80u);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* PLP */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:DE9F: radius times magnitude via Mode 7. */
static void SecondaryScaleRadius(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    And8(cpu, 0x7fu);                                          /* DE9F */
    StoreAAbsolute8(memory, cpu, 0x211cu, 0);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    PrimarySignExtend(memory, cpu, 0xdea9u);
    SetAccumulatorWidth(cpu, 1);                               /* DEAA */
    StoreAAbsolute8(memory, cpu, 0x211bu, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x211bu, 0);
    {
        const uint32_t address = AbsoluteIndexedAddress(cpu, 0x2134u, 0);
        const uint8_t value = Read8(memory, address);          /* DEB3 */

        cpu->carry = (value & 0x80u) != 0;
        Write8(memory, address, (uint8_t)(value << 1));
        SetNz8(cpu, (uint8_t)(value << 1));
    }
    SetAccumulatorWidth(cpu, 0);                               /* DEB6 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x2135u, 0));
    {
        const uint16_t value = cpu->accumulator;               /* DEBB */
        const uint8_t carry = cpu->carry;

        cpu->carry = (value & 0x8000u) != 0;
        LoadA16(cpu, (uint16_t)((value << 1) | carry));
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:DE76: orbit offsets into $54/$56; M=0 exit. */
static void SecondaryOrbitOffsets(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadXDirect(memory, cpu, 0x2au);                           /* DE76 */
    LoadAAbsolute8(memory, cpu, 0x0003u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 0, 0xde83u);
    SecondaryScaleRadius(memory, cpu, 0xde86u);
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* DE89 */
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    LsrA8(cpu);
    SecondaryWave(memory, cpu, 1, 0xde96u);
    SecondaryScaleRadius(memory, cpu, 0xde99u);
    Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
    LoadXDirect(memory, cpu, 0xa9u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:FAF4: signed operand at X+1; M=0 exit. */
static void SecondarySignedOperand(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* FAF4 */
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    Or8(cpu, 0x00u);                                           /* FAFA */
    if (cpu->negative) {
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* A16 = $2A + length, save cursor, exit. */
static SecondaryStep SecondarySaveCursorPlus(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t length) {
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
    while (length--)
        IncrementA16(cpu);
    return SecondarySaveCursorExit(memory, cpu);
}

static void AddLong16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t address) {
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(address, cpu->x)));
    Write16Long(memory, LongIndexedAddress(address, cpu->x), cpu->accumulator);
}

/* $83:AB4F: record offsets for actor $A7. */
static void SecondaryRecordOffsets(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Write8(memory, DirectAddress(cpu, 0xa8u), 0x00u);          /* AB4F */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0xa9u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0xaau), 0x00u);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    Write8(memory, DirectAddress(cpu, 0xabu), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0xacu), 0x00u);
    Write8(memory, DirectAddress(cpu, 0xadu), 0x00u);
}

/* $83:DFFD: script pointer from table $91:8EC7. */
static void SecondarySpawnScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* DFFD */
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x918ec7u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    cpu->carry = 0;
    Add16Immediate(cpu, 0x8ec7u);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x91u);
    Write8(memory, LongIndexedAddress(0x7fdef0u, cpu->x), A8(cpu));
}

/* $83:DFA5: initialise actor X with spawn id $54. */
static void SecondarySpawnInit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint32_t zeroed[4] = {
        0x7fdaecu, 0x7fdb0cu, 0x7fe286u, 0x7fe2ceu};
    static const uint32_t filled[3] = {0x7fe1aeu, 0x7fdb2cu, 0x7fe3a6u};
    static const uint32_t cleared[4] = {
        0x7fdcdcu, 0x7fdcddu, 0x7fdd6cu, 0x7fdd6du};
    unsigned i;

    StoreXDirect16(memory, cpu, 0xa7u);                        /* DFA5 */
    SimulateJslFrame(memory, cpu, 0x83u, 0xdfaau);
    SecondaryRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x84u);                                        /* DFAB */
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    LoadA8(cpu, 0x01u);
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0692u, 0);
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    LoadA8(cpu, 0x20u);
    Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* DFC3 */
    for (i = 0; i < 4u; ++i)
        Write8(memory, LongIndexedAddress(zeroed[i], cpu->x), A8(cpu));
    LoadA8(cpu, 0xffu);                                        /* DFD4 */
    for (i = 0; i < 3u; ++i)
        Write8(memory, LongIndexedAddress(filled[i], cpu->x), A8(cpu));
    LoadXDirect(memory, cpu, 0xa9u);                           /* DFE2 */
    TransferDirectToA(cpu);
    for (i = 0; i < 4u; ++i)
        Write8(memory, LongIndexedAddress(cleared[i], cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* DFF5 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    SimulateJslFrame(memory, cpu, 0x83u, 0xdffbu);
    SecondarySpawnScript(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:DF87: spawn id A into the first free slot. */
static void SecondarySpawnActor(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);                                 /* DF87 */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, 0x0000u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);          /* DF8F */
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            SimulateJsrFrame(memory, cpu, 0xdf9au);
            SecondarySpawnInit(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            break;
        }
        IncrementX16(cpu);                                     /* DF9D */
        Compare16(cpu, cpu->x, 0x0020u);
        if (cpu->zero)
            break;
    }
    PullDataBank(memory, cpu);                                 /* DFA3 */
}

/* $83:D6A6: skip a one-byte operand. */
static void SecondarySkipOperand(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0x2au);
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* $83:D67A: spawn child at $8F/$91 facing $94. */
static void SecondarySpawnChild(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));    /* D67A */
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0x2au);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xd685u);
    SecondarySpawnActor(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);                           /* D686 */
    LoadYDirect16(memory, cpu, 0xa9u);
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x83u, 0xd690u);
    SecondaryRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x94u)));    /* D691 */
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    TransferYToX(cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x8fu));
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x91u));
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
    SecondarySkipOperand(memory, cpu);                         /* D6A6 */
}

/* $83:D661: spawn child at the actor's position. */
static void SecondarySpawnAtActor(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa9u);                           /* D661 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
    Write16Direct(memory, cpu, 0x8fu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
    Write16Direct(memory, cpu, 0x91u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
    SecondarySpawnChild(memory, cpu);
}

static void ObjectAllocSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);
static void ObjectFreeSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

/* 24-bit pointer at DP offset. */
static uint32_t DirectLongPointer(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    return Read16Direct(memory, cpu, offset) |
           ((uint32_t)Read8(memory, DirectAddress(cpu, (uint8_t)(offset + 2u)))
               << 16);
}

/* $83:ABE9: sprite VRAM base (A.high << 4) + $2000; M=0. */
static void SpriteVramBase(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* ABE9 */
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Immediate(cpu, 0x2000u);
    SimulateRtlFrame(memory, cpu);
}

/* $83:AAAF: free the actor's sprite, clear occupancy. */
static void ActorReleaseSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* AAAF */
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    Or8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe25eu, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    ObjectFreeSprite(memory, cpu, 0xaad9u);
    SetIndexWidth(cpu, 0);                                     /* AADA */
    SimulateJslFrame(memory, cpu, 0x83u, 0xaadfu);
    SecondaryClearOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:A9E5: sprite descriptor A from $CF:F000. */
static void ActorSpriteDescriptor(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    uint32_t pointer;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushDataBank(memory, cpu);                                 /* A9E5 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xcff000u, cpu->x)));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* A9FB */
    SetIndexWidth(cpu, 1);
    LoadA8(cpu, 0xcfu);
    Write8(memory, DirectAddress(cpu, 0x5fu), A8(cpu));
    LoadXDirect(memory, cpu, 0xa7u);
    pointer = DirectLongPointer(memory, cpu, 0x5du);
    LoadA8(cpu, Read8(memory, pointer));
    And8(cpu, 0x07u);
    Write8(memory, LongIndexedAddress(0x7fe216u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, pointer));
    And8(cpu, 0xf8u);
    StoreAAbsolute8(memory, cpu, 0x1291u, cpu->x);
    LoadY8(cpu, 0x01u);
    LoadA8(cpu, Read8(memory, (pointer + cpu->y) & 0x00ffffffu));
    AslA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x), A8(cpu));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    SetAccumulatorWidth(cpu, 0);                               /* AA1C */
    LoadXDirect(memory, cpu, 0xabu);
    LoadA16(cpu, Read16Long(memory, (pointer + cpu->y) & 0x00ffffffu));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x12b9u, cpu->x),
        cpu->accumulator);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, (pointer + cpu->y) & 0x00ffffffu));
    StoreAAbsolute8(memory, cpu, 0x12bbu, cpu->x);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA7D: animation tables for the actor's sprite type. */
static void ActorSpriteTables(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA7D */
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83abfcu, cpu->x)));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1381u, cpu->y),
        cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);                               /* AA91 */
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x1471u, cpu->y);
    LoadAAbsolute8(memory, cpu, 0x1291u, cpu->y);
    And8(cpu, 0x18u);
    LsrA8(cpu);
    LsrA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83ac14u, cpu->x)));
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa9u)));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x13d1u, cpu->y),
        cpu->accumulator);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA50: allocate sprite slots for the actor. */
static void ActorAllocSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA50 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    ObjectAllocSprite(memory, cpu, 0xaa62u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* AA63 */
    Write8(memory, LongIndexedAddress(0x7fe25eu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x), A8(cpu));
    SpriteVramBase(memory, cpu, 0xaa71u);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0xa9u)));    /* AA72 */
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1331u, cpu->y),
        cpu->accumulator);
    ActorSpriteTables(memory, cpu, 0xaa7au);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* AA7B */
    SimulateRtsFrame(memory, cpu);
}

/* $83:A9BA: load sprite A for actor $A7. */
static void ActorLoadSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    const uint8_t wide = !cpu->accumulator_is_8_bit;

    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    if (wide)                                                  /* A9BA */
        PushAccumulator16(memory, cpu);
    else
        PushAccumulator8(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    ActorSpriteDescriptor(memory, cpu, 0xa9c6u);
    ActorAllocSprite(memory, cpu, 0xa9c9u);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* A9CA */
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (wide)
        PullAccumulator16(memory, cpu);
    else
        LoadA8(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:AA30: sprite height offset, -16 for odd types. */
static void ActorSpriteOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* AA30 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, 0xf0u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero)
        TransferDirectToA(cpu);                                /* AA43 */
    ExchangeAccumulatorBytes(cpu);
    PrimarySignExtend(memory, cpu, 0xaa47u);
    LoadXDirect(memory, cpu, 0xa9u);
    Write16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:DAE9: reload the actor's sprite, keep its frame. */
static void SecondarySpriteReload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xabu);                           /* DAE9 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe506u, cpu->x)));
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe508u, cpu->x)));
    PushAccumulator8(memory, cpu);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x)));
    PushAccumulator8(memory, cpu);
    ActorReleaseSprite(memory, cpu, 0xdb03u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* DB04 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
    ActorLoadSprite(memory, cpu, 0xdb10u);
    ActorSpriteOffset(memory, cpu, 0xdb14u);
    LoadA8(cpu, Pull8(memory, cpu));                           /* DB15 */
    LoadXDirect(memory, cpu, 0xa7u);
    Write8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
    LoadA8(cpu, Pull8(memory, cpu));
    LoadXDirect(memory, cpu, 0xabu);
    Write8(memory, LongIndexedAddress(0x7fe508u, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    Write16Long(memory, LongIndexedAddress(0x7fe506u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

static SecondaryStep SecondaryExecuteHandler(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t handler_pc) {
    switch (handler_pc & 0x00ffffffu) {
    case 0x83d5fdu:
        SecondaryStopScript(memory, cpu, 0xd5ffu);             /* D5FD */
        SecondaryLeaderStopFlag(memory, cpu, 0xd602u);         /* D600 */
        return SecondaryExit();

    case 0x83d60fu:
        SecondaryStopScript(memory, cpu, 0xd611u);             /* D60F */
        return SecondaryExit();

    case 0x83d615u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D615 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(
            cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x8000u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d626u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D626 */
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->y);
        And8(cpu, 0x06u);
        Or8(
            cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x066au, cpu->y);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6adu:
        LoadXDirect(memory, cpu, 0xa7u);                       /* D6AD */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
        Write8(memory, LongIndexedAddress(0x7fd69cu, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
        Write8(memory, LongIndexedAddress(0x7fd6ccu, cpu->x), A8(cpu));
        LoadXDirect(memory, cpu, 0x2au);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6c7u:
        LoadXDirect(memory, cpu, 0xa7u);                       /* D6C7 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
        AslA8(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 0);                           /* D6D0 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        PushAccumulator16(memory, cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        IncrementA16(cpu);
        LoadXDirect(memory, cpu, 0xa9u);
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        PullAccumulator16(memory, cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                           /* D6E7 */
        LoadXDirect(memory, cpu, 0x2au);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d6efu:
        LoadXDirect(memory, cpu, 0x2au);                       /* D6EF */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        LsrA8(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        if (cpu->carry)
            And8(cpu, 0xfdu);                                  /* D6FC */
        else
            Or8(cpu, 0x02u);                                   /* D700 */
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        LoadXDirect(memory, cpu, 0x2au);                       /* D705 */
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d833u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D833 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fe4deu, cpu->x), A8(cpu));
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d844u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D844 */
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->y);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);      /* D84D */
            StoreAAbsolute8(memory, cpu, 0x066au, cpu->y);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83da9au:
        LoadYDirect16(memory, cpu, 0x2au);                     /* DA9A */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        StoreAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        TransferDirectToA(cpu);                                /* DAA4 */
        Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
        Write8(memory, LongIndexedAddress(0x7fe4b6u, cpu->x), A8(cpu));
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dee9u: {
        const uint32_t wait = 0x7fe48eu;

        LoadYDirect16(memory, cpu, 0x2au);                     /* DEE9 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(wait, cpu->x)));
        if (!cpu->negative) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* DEF3 */
            And8(cpu, 0x0fu);
            Or8(cpu, 0x80u);
            Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
        } else {
            DecrementA8(cpu);                                  /* DF00 */
            Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
            And8(cpu, 0x0fu);
            if (cpu->zero) {
                Write8(memory, LongIndexedAddress(wait, cpu->x), A8(cpu));
                return SecondaryNextByte(memory, cpu, 1);      /* DF0D */
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* DF10 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));
        return SecondarySaveCursorExit(memory, cpu);
    }

    case 0x83d7b2u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D7B2 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* D7BD */
        LoadA16(cpu, cpu->y);
        IncrementA16(cpu);
        IncrementA16(cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdb9cu, cpu->x), cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d7ccu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* D7CC */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
        DecrementA8(cpu);
        Write8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x), A8(cpu));
        if (cpu->zero)
            return SecondaryNextByte(memory, cpu, 1);          /* D7D9 */
        SetAccumulatorWidth(cpu, 0);                           /* D7DF */
        LoadA16(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdb9cu, cpu->x)));
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d80eu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* D80E */
        TransferDirectToA(cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdbecu, cpu->x)));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdc3cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdbecu, cpu->x), cpu->accumulator);
        LsrA16(cpu);                                           /* D820 */
        LsrA16(cpu);
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83d969u:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D969 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        for (uint8_t i = 0; i < 2u; ++i) {
            const uint32_t pair = i ? 0x7fdd1cu : 0x7fdc8cu;
            LoadA16(
                cpu, Read16AbsoluteIndexed(
                    memory, cpu, i ? 0x0003u : 0x0001u, cpu->y));
            cpu->carry = 0;
            Add16Value(
                cpu, Read16Long(memory, LongIndexedAddress(pair, cpu->x)));
            Write16Long(
                memory, LongIndexedAddress(pair, cpu->x), cpu->accumulator);
        }
        LoadA16(cpu, cpu->y);                                  /* D987 */
        cpu->carry = 0;
        Add16Immediate(cpu, 0x0005u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);

    case 0x83db77u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DB77 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        {
            const uint8_t clear = !cpu->zero;
            LoadAAbsolute8(memory, cpu, 0x0736u, cpu->y);
            if (clear)
                And8(cpu, 0xfdu);                              /* DB81 */
            else
                Or8(cpu, 0x02u);                               /* DB8B */
            StoreAAbsolute8(memory, cpu, 0x0736u, cpu->y);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dbdcu:
        LoadXDirect(memory, cpu, 0x2au);                       /* DBDC */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        Write8(memory, LongIndexedAddress(0x7fe35eu, cpu->x), A8(cpu));
        Or8(cpu, 0x00u);
        {
            const uint8_t blink = !cpu->zero;
            LoadA8(
                cpu, Read8(memory, LongIndexedAddress(0x000736u, cpu->x)));
            if (blink)
                Or8(cpu, 0x80u);                               /* DBF7 */
            else
                And8(cpu, 0x7fu);                              /* DBEF */
            Write8(memory, LongIndexedAddress(0x000736u, cpu->x), A8(cpu));
        }
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83dab3u:
        LoadXDirect(memory, cpu, 0xa7u);                       /* DAB3 */
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
        And8(cpu, 0x07u);
        StoreAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dac3u:
        return SecondaryNextByte(memory, cpu, 1);             /* DAC3 */

    case 0x83dac9u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DAC9 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        Compare8(cpu, A8(cpu), 0x01u);
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
        if (cpu->carry)
            Or8(cpu, 0x80u);                                   /* DADC */
        else
            And8(cpu, 0x7fu);                                  /* DAD8 */
        Write8(memory, LongIndexedAddress(0x7fe316u, cpu->x), A8(cpu));
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83dd54u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DD54 */
        cpu->y = cpu->x;                                       /* TXY */
        SetNz16(cpu, cpu->y);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadXDirect(memory, cpu, 0xa7u);
        Write8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
        And8(cpu, 0x07u);
        StoreAAbsolute8(memory, cpu, 0x066au, cpu->x);
        TransferYToX(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83debdu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DEBD */
        SetAccumulatorWidth(cpu, 0);
        CopyLong16(memory, cpu, 0x7fddaeu, 0x7fdb4cu);
        CopyLong16(memory, cpu, 0x7fde3eu, 0x7fdb9cu);
        CopyLong16(memory, cpu, 0x7fdc8cu, 0x7fdbecu);
        CopyLong16(memory, cpu, 0x7fdd1cu, 0x7fdc3cu);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83db54u:
    case 0x83db5au: {
        const uint8_t save = (handler_pc & 0x00ffffffu) == 0x83db54u;

        SimulateJsrFrame(memory, cpu, save ? 0xdb56u : 0xdb5cu);
        Lufia2ActorMoveFinePosition(memory, cpu);              /* FA81 */
        SimulateRtsFrame(memory, cpu);
        if (save)
            return SecondarySaveCursorExit(memory, cpu);       /* DB57 */
        TransferAToX(cpu);                                     /* DB5D */
        SetAccumulatorWidth(cpu, 1);
        return SecondaryRedispatched(memory, cpu);
    }

    case 0x83db6du:
        SimulateJslFrame(memory, cpu, 0x83u, 0xdb70u);         /* DB6D */
        Lufia2ActorMarkMapOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83d760u:
        LoadXDirect(memory, cpu, 0x2au);                       /* D760 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xd768u);
        Lufia2QueueDeferredSound(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d7ebu:
        LoadYDirect16(memory, cpu, 0x2au);                     /* D7EB */
        LoadXDirect(memory, cpu, 0xa9u);
        for (uint8_t i = 0; i < 2u; ++i) {
            LoadAAbsolute8(memory, cpu, i ? 0x0002u : 0x0001u, cpu->y);
            PrimarySignExtend(memory, cpu, i ? 0xd800u : 0xd7f4u);
            Write16Long(
                memory, LongIndexedAddress(i ? 0x7fdc3cu : 0x7fdbecu, cpu->x),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
        }
        IncrementY16(cpu);                                     /* D807 */
        IncrementY16(cpu);
        IncrementY16(cpu);
        TransferYToX(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dbc2u:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DBC2 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x06bau, cpu->y);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb4du, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
        SimulateJslFrame(memory, cpu, 0x83u, 0xdbd5u);
        Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dc04u:
        LoadXDirect(memory, cpu, 0x2au);                       /* DC04 */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        {
            const uint8_t restore = !cpu->zero;
            uint32_t flags;

            LoadXDirect(memory, cpu, 0xa7u);                   /* DC0B */
            flags = LongIndexedAddress(0x7fe316u, cpu->x);
            LoadA8(cpu, Read8(memory, flags));
            if (restore) {
                And8(cpu, 0x7fu);                              /* DC11 */
                Write8(memory, flags, A8(cpu));
                LoadXDirect(memory, cpu, 0xa9u);
                LoadA8(
                    cpu, Read8(memory, LongIndexedAddress(0x7fdb4cu, cpu->x)));
                ExchangeAccumulatorBytes(cpu);
                LoadA8(
                    cpu, Read8(memory, LongIndexedAddress(0x7fdb4du, cpu->x)));
            } else {
                Or8(cpu, 0x80u);                               /* DC2A */
                Write8(memory, flags, A8(cpu));
                TransferDirectToA(cpu);
            }
        }
        LoadXDirect(memory, cpu, 0xa7u);                       /* DC31 */
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x06bau, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xdc3du);
        Lufia2ActorSyncFinePosition(memory, cpu);              /* A746 */
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83d78cu:
    case 0x83d79cu: {
        const uint8_t step = (handler_pc & 0x00ffffffu) == 0x83d78cu;

        SecondaryActorToProbe(memory, cpu, step ? 0xd78eu : 0xd79eu);
        if (step) {
            LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);      /* D78F */
            SimulateJslFrame(memory, cpu, 0x83u, 0xd795u);
            if (Lufia2ActorMovementStep(memory, cpu) == 0)
                return SecondaryBoundary(cpu);
            SimulateRtlFrame(memory, cpu);
        }
        return SecondaryNextByte(memory, cpu, 1);
    }

    case 0x83d95eu:
    case 0x83d992u: {
        const uint8_t save = (handler_pc & 0x00ffffffu) == 0x83d95eu;

        SecondaryAddDisplayOffset(memory, cpu, save ? 0xd960u : 0xd994u);
        if (save) {
            LoadA16(cpu, Read16Direct(memory, cpu, 0x2au));    /* D961 */
            IncrementA16(cpu);
            IncrementA16(cpu);
            IncrementA16(cpu);
            return SecondarySaveCursorExit(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 1);                           /* D995 */
        return SecondaryNextByte(memory, cpu, 3);
    }

    case 0x83db63u:
        SimulateJslFrame(memory, cpu, 0x83u, 0xdb66u);         /* DB63 */
        SecondaryClearOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83ddfbu:
        SimulateJslFrame(memory, cpu, 0x83u, 0xddfeu);         /* DDFB */
        SecondaryClearOccupancy(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* DDFF */
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        Or8(cpu, 0x04u);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, 0x05d2u, cpu->x);
        return SecondaryExecuteHandler(memory, cpu, 0x83d5fdu);

    case 0x83d76eu:
        LoadYDirect16(memory, cpu, 0xa7u);                     /* D76E */
        LoadAAbsolute8(memory, cpu, 0x06bau, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
        SimulateJslFrame(memory, cpu, 0x83u, 0xd77du);
        Lufia2ActorReadMapCellValue(memory, cpu);              /* FB71 */
        SimulateRtlFrame(memory, cpu);
        Compare8(cpu, A8(cpu), 0x09u);                         /* D77E */
        if (cpu->zero) {
            /* $80:E7DF event queue stays LLE. */
            cpu->resume_pc = 0x83d782u;
            return SecondaryBoundary(cpu);
        }
        return SecondaryNextByte(memory, cpu, 1);

    case 0x83dd6eu:
        LoadXDirect(memory, cpu, 0xa9u);                       /* DD6E */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7fddaeu));
        Write16Long(
            memory, LongIndexedAddress(0x7fddaeu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fde3eu));
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        LoadXDirect(memory, cpu, 0x2au);                       /* DD82 */
        SecondarySignedOperand(memory, cpu, 0xdd86u);
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
        IncrementX16(cpu);
        SecondarySignedOperand(memory, cpu, 0xdd8cu);
        IncrementX16(cpu);
        PushIndex(memory, cpu);                                /* DD8E */
        LoadXDirect(memory, cpu, 0xa9u);
        AddLong16(memory, cpu, 0x7fde3eu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));        /* DD9A */
        AddLong16(memory, cpu, 0x7fddaeu);
        cpu->y = PullIndexValue(memory, cpu);                  /* DDA5 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
        /* JSR $FAFA with M=0: ORA/TSB/LDA. */
        SimulateJsrFrame(memory, cpu, 0xddaeu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x1000u));
        {
            const uint16_t value = Read16Direct(memory, cpu, 0xebu);

            cpu->zero = (value & cpu->accumulator) == 0;
            Write16Direct(
                memory, cpu, 0xebu, (uint16_t)(value | cpu->accumulator));
        }
        LoadA16(cpu, 0xebffu);
        SimulateRtsFrame(memory, cpu);
        Write16Long(                                           /* DDAF */
            memory, LongIndexedAddress(0x7fdc8cu, cpu->x), cpu->accumulator);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 1);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        PrimarySignExtend(memory, cpu, 0xddbcu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 0);

    case 0x83de11u:
        SecondaryOrbitOffsets(memory, cpu, 0xde13u);           /* DE11 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        Write16Long(
            memory, LongIndexedAddress(0x7fdc8cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83de29u:
        SecondaryOrbitOffsets(memory, cpu, 0xde2bu);           /* DE29 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdb9cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fde3eu, cpu->x), cpu->accumulator);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        Add16Immediate(cpu, 0x0000u);                          /* DE3B */
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect(memory, cpu, 0xa7u);
        StoreAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
        SetAccumulatorWidth(cpu, 0);                           /* DE45 */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        cpu->carry = 0;
        Add16Value(
            cpu, Read16Long(memory, LongIndexedAddress(0x7fdc3cu, cpu->x)));
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83de5du:
        SecondaryOrbitOffsets(memory, cpu, 0xde5fu);           /* DE5D */
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        IncrementA16(cpu);
        Write16Long(
            memory, LongIndexedAddress(0x7fdd1cu, cpu->x), cpu->accumulator);
        return SecondarySaveCursorPlus(memory, cpu, 4);

    case 0x83d70cu:
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u))); /* D70C */
        PushAccumulator8(memory, cpu);
        LoadXDirect(memory, cpu, 0x2au);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        SimulateJslFrame(memory, cpu, 0x83u, 0xd717u);
        SecondarySpawnActor(memory, cpu);                      /* DF87 */
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa9u);                       /* D718 */
        StoreXDirect16(memory, cpu, 0x54u);
        LoadXDirect(memory, cpu, 0xa7u);
        StoreXDirect16(memory, cpu, 0x56u);
        LoadA8(cpu, Pull8(memory, cpu));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        SimulateJslFrame(memory, cpu, 0x83u, 0xd726u);
        SecondaryRecordOffsets(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* D727 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5a6u, cpu->x)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe5ceu, cpu->x)));
        LoadXDirect(memory, cpu, 0x56u);
        Write8(memory, LongIndexedAddress(0x7fda4cu, cpu->x), A8(cpu));
        ExchangeAccumulatorBytes(cpu);
        Write8(memory, LongIndexedAddress(0x7fda2cu, cpu->x), A8(cpu));
        SetAccumulatorWidth(cpu, 0);                           /* D73D */
        LoadXDirect(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        LoadXDirect(memory, cpu, 0x54u);
        Write16Long(
            memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        Write16Long(
            memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return SecondaryNextByte(memory, cpu, 2);

    case 0x83d638u:
        SimulateJsrFrame(memory, cpu, 0xd63au);                /* D638 */
        SecondarySpawnAtActor(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83d63eu:
        SimulateJsrFrame(memory, cpu, 0xd640u);                /* D63E */
        SecondarySpawnAtActor(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadXDirect(memory, cpu, 0xa7u);                       /* D641 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        Compare8(cpu, A8(cpu), 0x02u);
        TransferDirectToA(cpu);
        TransferYToX(cpu);
        SetAccumulatorWidth(cpu, 0);
        if (!cpu->carry)
            LoadA16(cpu, 0xfff8u);                             /* D64F */
        AddLong16(memory, cpu, 0x7fddfeu);
        SimulateJsrFrame(memory, cpu, 0xd65du);
        SecondarySkipOperand(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83db95u: {
        unsigned i;

        SetAccumulatorWidth(cpu, 0);                           /* DB95 */
        LoadXDirect(memory, cpu, 0xa9u);
        for (i = 0; i < 2u; ++i) {
            LoadA16(cpu, Read16Long(
                memory, LongIndexedAddress(i ? 0x7fdb4du : 0x7fdb4cu, cpu->x)));
            And16(cpu, 0x00ffu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            Write16Direct(memory, cpu, i ? 0x91u : 0x8fu, cpu->accumulator);
        }
        SetAccumulatorWidth(cpu, 1);                           /* DBB3 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
        Write8(memory, DirectAddress(cpu, 0x94u), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xdbbeu);
        SecondarySpawnChild(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        return SecondaryRedispatched(memory, cpu);
    }

    case 0x83dae9u:
        SecondarySpriteReload(memory, cpu);
        LoadXDirect(memory, cpu, 0x2au);                       /* DB34 */
        IncrementX16(cpu);
        return SecondaryRedispatched(memory, cpu);

    case 0x83dc45u:
        return SecondaryWalk(memory, cpu);

    default: {
        SecondaryStep step;
        cpu->resume_pc = handler_pc & 0x00ffffffu;
        step.flow = SECONDARY_STEP_UNKNOWN;
        step.handler_pc = cpu->resume_pc;
        return step;
    }
    }
}

Lufia2ActorPrimaryUpdateResult Lufia2ActorSecondaryUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;
    uint32_t handler;
    uint32_t steps;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83d599u;
    result.dispatches = 0;

    if (Lufia2ActorSecondaryUpdateFrontend(memory, cpu) !=
        LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A)
        return result;

    handler = Lufia2ActorSecondaryScriptDispatch(memory, cpu).handler_pc;
    result.dispatches = 1;
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        const SecondaryStep step =
            SecondaryExecuteHandler(memory, cpu, handler);

        if (step.flow == SECONDARY_STEP_REDISPATCHED) {
            handler = step.handler_pc;
            ++result.dispatches;
            continue;
        }
        if (step.flow == SECONDARY_STEP_EXIT_D60D) {
            PullDataBank(memory, cpu);                         /* D60D */
            result.pc = 0x83d60eu;
            return result;
        }
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = step.handler_pc;
        return result;
    }
    cpu->resume_pc = handler;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = handler;
    return result;
}

/* $83:867B / $83:8674: pressed-bit test, clear latch on hit. */
static void PlayerPressedClear(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t pressed,
    uint8_t latch,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    And8(cpu, Read8(memory, DirectAddress(cpu, pressed)));
    if (!cpu->zero)
        TrbDirect8(memory, cpu, latch);
    SimulateRtsFrame(memory, cpu);
}

/* $83:F9AD cell under $8F/$91. */
static void PlayerProbeCell(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    PrimaryCollisionIndex(memory, cpu, return_address, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
}

/* $83:BA76: solid bit of the probed cell. */
static void PlayerProbeSolid(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PlayerProbeCell(memory, cpu, 0xba78u);
    And8(cpu, 0x01u);
}

/* $83:BA5C..BAAC: step the probe past ledges. */
static void PlayerProbeAhead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t target) {
    switch (target) {
    case 0xba5cu:
        IncrementDirect8(memory, cpu, 0x91u);
        PlayerProbeCell(memory, cpu, 0xba60u);
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero)
            return;                                            /* BA67 */
        BitImmediate8(cpu, 0x10u);
        if (!cpu->zero) {
            IncrementDirect8(memory, cpu, 0x91u);              /* BA6D */
            SimulateJsrFrame(memory, cpu, 0xba71u);
            PlayerProbeSolid(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            if (!cpu->zero)
                return;
            IncrementDirect8(memory, cpu, 0x91u);              /* BA74 */
        }
        PlayerProbeSolid(memory, cpu);
        return;
    case 0xba80u:
    case 0xba96u: {
        const uint8_t along_x = target == 0xba80u;
        const uint8_t axis = along_x ? 0x8fu : 0x91u;

        PlayerProbeCell(memory, cpu, along_x ? 0xba82u : 0xba98u);
        And8(cpu, along_x ? 0x20u : 0x10u);
        if (!cpu->zero) {
            DecrementDirect8(memory, cpu, axis);
            SimulateJsrFrame(memory, cpu, along_x ? 0xba8fu : 0xbaa5u);
            PlayerProbeSolid(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            if (!cpu->zero)
                return;
        }
        DecrementDirect8(memory, cpu, axis);                   /* BA92 */
        PlayerProbeSolid(memory, cpu);
        return;
    }
    default:                                                   /* BAAC */
        IncrementDirect8(memory, cpu, 0x8fu);
        PlayerProbeCell(memory, cpu, 0xbab0u);
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero)
            return;
        BitImmediate8(cpu, 0x20u);
        if (cpu->zero)
            return;
        IncrementDirect8(memory, cpu, 0x8fu);
        PlayerProbeSolid(memory, cpu);
        return;
    }
}

/* $83:BAC2: actor 8..39 on $8F/$91; X and $56. */
static void PlayerFindActorAt(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, 0x0008u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* BAC5 */
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            uint8_t hit = 0;
            unsigned i;

            /* The ROM tests the wide column twice. */
            for (i = 0; i < 2u && !hit; ++i) {
                LoadA8(cpu, Read8(
                    memory, LongIndexedAddress(0x7fe216u, cpu->x)));
                Compare8(cpu, A8(cpu), 0x02u);
                if (cpu->carry) {
                    LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);
                    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                    Compare8(
                        cpu, A8(cpu),
                        Read8(memory, DirectAddress(cpu, 0x8fu)));
                    hit = cpu->zero;
                }
            }
            if (!hit) {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* BAEC */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
                hit = cpu->zero;
            }
            if (hit) {
                LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);  /* BAF3 */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
                cpu->carry = 1;
                if (cpu->zero) {
                    StoreXDirect16(memory, cpu, 0x56u);        /* BB04 */
                    return;
                }
            }
        }
        IncrementX16(cpu);                                     /* BAFB */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero) {
            cpu->carry = 0;
            return;
        }
    }
}

/* $83:BA06: talkable actor ahead; carry = found. */
static uint8_t PlayerFindTalkTarget(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;

    SimulateJsrFrame(memory, cpu, 0xc1d2u);
    SetIndexWidth(cpu, 0);                                     /* BA06 */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    PrimaryTileHeight(memory, cpu, 0xba14u);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    TransferDirectToA(cpu);                                    /* BA17 */
    LoadAAbsolute8(memory, cpu, 0x0692u, 0);
    TransferAToX(cpu);
    target = Read16ProgramIndexed(memory, cpu, 0xba54u, cpu->x);
    if (target != 0xba5cu && target != 0xba80u &&
        target != 0xba96u && target != 0xbaacu) {
        cpu->resume_pc = 0x83ba1cu;
        return 0;
    }
    SimulateJsrFrame(memory, cpu, 0xba1eu);
    PlayerProbeAhead(memory, cpu, target);
    SimulateRtsFrame(memory, cpu);
    cpu->carry = 0;                                            /* BA1F */
    if (!cpu->zero) {
        SimulateJsrFrame(memory, cpu, 0xba24u);
        PlayerFindActorAt(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        if (cpu->carry) {
            uint8_t same_height = 1;

            LoadAAbsolute8(memory, cpu, 0x05d2u, cpu->x);      /* BA27 */
            Compare8(cpu, A8(cpu), 0x70u);
            if (cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
                BitImmediate8(cpu, 0x01u);
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x0692u, 0);   /* BA35 */
                    Compare8(cpu, A8(cpu), 0x04u);
                    cpu->carry = cpu->zero;
                    same_height = 0;
                }
            }
            if (same_height) {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* BA40 */
                Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
                LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
                Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
                PrimaryTileHeight(memory, cpu, 0xba4cu);
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
                cpu->carry = cpu->zero;
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:C161: D-pad to $22/$23; carry = held. */
static void PlayerReadDirection(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc1e5u);
    TransferDirectToA(cpu);                                    /* C161 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
    And8(cpu, 0x0fu);
    cpu->carry = 0;
    if (!cpu->zero) {
        TrbDirect8(memory, cpu, 0x4bu);                        /* C169 */
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83d437u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x22u), A8(cpu));
        Write8(memory, 0x7fd4f6u, A8(cpu));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
        Write8(memory, DirectAddress(cpu, 0x23u), A8(cpu));
        cpu->carry = 1;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:FC56: $057C set, B held, slot 0. */
static void PlayerSkipProbe(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc1eau);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* FC56 */
    cpu->carry = 0;
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
            if (cpu->zero)
                cpu->carry = 1;                                /* FC67 */
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:C246: $83:C1B0 entry for direction $22. */
static void PlayerFacingCode(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* C246 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83c1b0u, cpu->x)));
    SimulateRtsFrame(memory, cpu);
}

/* $83:FBBD: edge bit toward facing A; carry = set. */
static uint8_t PlayerEdgeAhead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t target;
    uint8_t mask;

    SimulateJsrFrame(memory, cpu, 0xc1f7u);
    ExchangeAccumulatorBytes(cpu);                             /* FBBD */
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    TransferAToX(cpu);
    target = Read16ProgramIndexed(memory, cpu, 0xfbc6u, cpu->x);
    if (target != 0xfbceu && target != 0xfbd7u &&
        target != 0xfbdeu && target != 0xfbe5u) {
        cpu->resume_pc = 0x83fbc2u;
        return 0;
    }
    SimulateJsrFrame(memory, cpu, 0xfbc4u);
    if (target == 0xfbceu)
        IncrementDirect8(memory, cpu, 0x91u);
    else if (target == 0xfbe5u)
        IncrementDirect8(memory, cpu, 0x8fu);
    mask = (target == 0xfbceu || target == 0xfbdeu) ? 0x10u : 0x20u;
    SimulateJsrFrame(                                          /* FBF1 */
        memory, cpu,
        target == 0xfbceu ? 0xfbd2u : target == 0xfbd7u ? 0xfbd9u :
        target == 0xfbdeu ? 0xfbe0u : 0xfbe9u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x8fu)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x91u)));
    PrimaryCollisionIndex(memory, cpu, 0xfbf8u, 0);            /* F9B6 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    SimulateRtsFrame(memory, cpu);
    And8(cpu, mask);                                           /* FBEC */
    cpu->carry = !cpu->zero;
    SimulateRtsFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $8E:BBAF: vehicle tile pair; boundary past $8E:BBD1. */
static uint8_t PlayerVehicleTile(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    cpu->program_bank = 0x8eu;
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);                   /* BBAF */
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    SimulateJslFrame(memory, cpu, 0x8eu, 0xbbbcu);
    cpu->program_bank = 0x83u;
    Lufia2ActorReadMapCellValue(memory, cpu);                  /* FB71 */
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x8eu;
    Compare8(cpu, A8(cpu), 0x06u);                             /* BBBD */
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x23u)));
        SimulateJslFrame(memory, cpu, 0x8eu, 0xbbc6u);
        cpu->program_bank = 0x83u;
        if (Lufia2ActorMovementStep(memory, cpu) == 0)
            return 0;
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x8eu, 0xbbcau);
        Lufia2ActorReadMapCellValue(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x8eu;
        Compare8(cpu, A8(cpu), 0x06u);                         /* BBCB */
        if (cpu->zero) {
            cpu->resume_pc = 0x8ebbd1u;
            return 0;
        }
    }
    cpu->carry = 0;                                            /* BBCF */
    return 1;
}

/* $8E:B65A: leader tile inside rectangle at base. */
static uint8_t PlayerInsideRect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t base) {
    unsigned axis;

    for (axis = 0; axis < 2u; ++axis) {
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, (uint8_t)(0x56u + axis))));
        Compare8(
            cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(base + axis), cpu->x)));
        if (!cpu->carry)
            return 0;
        Compare8(
            cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(base + 2u + axis), cpu->x)));
        if (cpu->carry)
            return 0;
    }
    return 1;
}

/* $8E:B63B: door rectangles at $7E:F000; boundary $8E:B6D4. */
static uint8_t PlayerDoorRegion(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint32_t entries;

    cpu->program_bank = 0x8eu;
    PushDataBank(memory, cpu);                                 /* B63B */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x57u), A8(cpu));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);                                     /* B64A */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xf002u, 0));
    LoadA8(cpu, 0xffu);
    Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
    for (entries = 0;; ++entries) {
        /* A table without $FF spins the ROM. */
        if (entries == 0x10000u) {
            cpu->resume_pc = 0x8eb653u;
            return 0;
        }
        LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);          /* B653 */
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        if (PlayerInsideRect(memory, cpu, 0xf005u)) {
            LoadAAbsolute8(memory, cpu, 0xf00du, cpu->x);      /* B672 */
            Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
            break;
        }
        LoadAAbsolute8(memory, cpu, 0xf009u, cpu->x);          /* B679 */
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero && PlayerInsideRect(memory, cpu, 0xf009u)) {
            LoadAAbsolute8(memory, cpu, 0xf00du, cpu->x);      /* B698 */
            Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
            break;
        }
        SetAccumulatorWidth(cpu, 0);                           /* B69F */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Immediate(cpu, 0x000fu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x58u)));     /* B6AB */
    Compare8(cpu, A8(cpu), 0xffu);
    cpu->carry = 0;
    if (!cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd09du));                 /* B6B5 */
        Compare8(cpu, A8(cpu), 0x00u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x57u)));
            Compare8(
                cpu, A8(cpu),
                Read8(memory, AbsoluteIndexedAddress(cpu, 0xf002u, cpu->x)));
        } else if (Compare8(cpu, A8(cpu), 0x01u), cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);      /* B6CD */
            Compare8(
                cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x57u)));
        } else {
            cpu->carry = 0;                                    /* B6C1 */
        }
        if (cpu->carry) {
            cpu->resume_pc = 0x8eb6d4u;
            return 0;
        }
    }
    PullDataBank(memory, cpu);                                 /* B738 */
    return 1;
}

/* $83:FC69: $057C and B pick walk speed $10/$08. */
static void PlayerWalkSpeed(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xc234u);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* FC69 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            LoadA8(cpu, 0x10u);
            Write8(memory, 0x7fe4deu, A8(cpu));
        } else {
            LoadA8(cpu, Read8(memory, 0x7fe4deu));             /* FC7C */
            Compare8(cpu, A8(cpu), 0x10u);
            if (cpu->zero) {
                LoadA8(cpu, 0x08u);
                Write8(memory, 0x7fe4deu, A8(cpu));
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

static Lufia2ActorPrimaryUpdateResult PlayerReturned(uint32_t rts_pc) {
    Lufia2ActorPrimaryUpdateResult result;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = rts_pc;
    result.dispatches = 0;
    return result;
}

static Lufia2ActorPrimaryUpdateResult PlayerBoundary(
    Lufia2ActorFrontendCpu *cpu, uint32_t pc) {
    Lufia2ActorPrimaryUpdateResult result;
    if (pc)
        cpu->resume_pc = pc;
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc;
    result.dispatches = 0;
    return result;
}

/* D350 via JSL; 0 on its exact boundary. */
static uint8_t PlayerCallActionCore(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    if (Lufia2ActorPrimaryActionCore(memory, cpu) !=
        LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

Lufia2ActorPrimaryUpdateResult Lufia2PlayerSlotStandardUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* C1B4 */
    if (cpu->negative)
        return PlayerReturned(0x83c1e2u);
    LoadA8(cpu, 0xa0u);                                        /* C1B9 */
    PlayerPressedClear(memory, cpu, 0x46u, 0x4au, 0xc1bdu);
    if (!cpu->zero) {
        if (!PlayerFindTalkTarget(memory, cpu))                /* C1D0 */
            return PlayerBoundary(cpu, 0);
        if (!cpu->carry)
            return PlayerBoundary(cpu, 0x83c1dcu);             /* $8E:C05F */
        LoadXDirect(memory, cpu, 0x56u);                       /* C1D5 */
        return PlayerBoundary(cpu, 0x83c1d7u);                 /* BB08 */
    }
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* C1C0 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x20u);
        PlayerPressedClear(memory, cpu, 0x47u, 0x4bu, 0xc1c9u);
        if (!cpu->zero)
            return PlayerBoundary(cpu, 0x83c1ccu);             /* 83E0 */
    }

    PlayerReadDirection(memory, cpu);                          /* C1E3 */
    if (!cpu->carry)
        return PlayerReturned(0x83c1e2u);
    PlayerSkipProbe(memory, cpu);                              /* C1E8 */
    if (!cpu->carry) {
        SetIndexWidth(cpu, 0);                                 /* C1ED */
        PrimaryLeaderToProbe(memory, cpu, 0xc1f1u);
        PlayerFacingCode(memory, cpu, 0xc1f4u);
        if (!PlayerEdgeAhead(memory, cpu))
            return PlayerBoundary(cpu, 0);
        SetIndexWidth(cpu, 1);                                 /* C1F8 */
        if (cpu->carry) {
            SetIndexWidth(cpu, 0);                             /* C1FC */
            SimulateJslFrame(memory, cpu, 0x83u, 0xc201u);
            if (!PlayerVehicleTile(memory, cpu))
                return PlayerBoundary(cpu, 0);
            SimulateRtlFrame(memory, cpu);
            cpu->program_bank = 0x83u;
            SetIndexWidth(cpu, 1);                             /* C202 */
            if (cpu->carry)
                goto done;
        } else {
            SetIndexWidth(cpu, 0);                             /* C219 */
            PrimaryLeaderToProbe(memory, cpu, 0xc21du);
            PlayerFacingCode(memory, cpu, 0xc220u);
            SimulateJslFrame(memory, cpu, 0x83u, 0xc224u);
            if (Lufia2ActorMovementStep(memory, cpu) == 0)
                return PlayerBoundary(cpu, 0);
            SimulateRtlFrame(memory, cpu);
            PlayerProbeCell(memory, cpu, 0xc227u);             /* C225 */
            SetIndexWidth(cpu, 1);
            BitImmediate8(cpu, 0x01u);
            if (cpu->zero)
                goto walk;
        }
        /* Blocked: turn toward $22 + $18. */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));  /* C206 */
        Write8(memory, 0x7fd09du, A8(cpu));
        cpu->carry = 0;
        Adc8(cpu, 0x18u);
        if (!PlayerCallActionCore(memory, cpu, 0xc212u))
            return PlayerBoundary(cpu, 0);
        SimulateJslFrame(memory, cpu, 0x83u, 0xc216u);         /* C213 */
        if (!PlayerDoorRegion(memory, cpu))
            return PlayerBoundary(cpu, 0);
        SimulateRtlFrame(memory, cpu);
        cpu->program_bank = 0x83u;
        goto done;
    }

walk:
    PlayerWalkSpeed(memory, cpu);                              /* C232 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x22u)));
    if (!PlayerCallActionCore(memory, cpu, 0xc23au))
        return PlayerBoundary(cpu, 0);
    SimulateJsrFrame(memory, cpu, 0xc23du);                    /* C23B */
    LoadA8(cpu, 0x20u);                                        /* C0FA */
    TestBitsAbsolute8(memory, cpu, 0x099cu, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);
        Write8(memory, 0x7fd0c1u, A8(cpu));
    }
    SimulateRtsFrame(memory, cpu);
    LoadA8(cpu, 0x08u);                                        /* C23E */
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);

done:
    SetIndexWidth(cpu, 1);                                     /* C243 */
    return PlayerReturned(0x83c245u);
}

/* TYA with M=1. */
static void TransferYToA8(Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, (uint8_t)cpu->y);
}

Lufia2ActorPrimaryUpdateResult Lufia2UpdateActorSlots(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorSlotChild child,
    void *child_context) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83bbf2u;
    result.dispatches = 0;

    LoadA8(cpu, Read8(memory, 0x7fd0feu));                     /* BB93 */
    if (!cpu->zero) {
        LoadA8(cpu, 0x01u);
        Write8(memory, 0x7fe216u, A8(cpu));
    }
    Write8(memory, DirectAddress(cpu, 0xa7u), 0x00u);          /* BB9F */
    for (;;) {
        ++result.dispatches;
        /* A child left D set: AB4F's ADC goes BCD. */
        if (cpu->decimal) {
            cpu->resume_pc = 0x83bba1u;
            result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
            result.pc = cpu->resume_pc;
            return result;
        }
        SimulateJslFrame(memory, cpu, 0x83u, 0xbba4u);         /* BBA1 */
        SecondaryRecordOffsets(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (cpu->index_is_8_bit) {                             /* BBA5 */
            cpu->y = Read8(memory, DirectAddress(cpu, 0xa7u));
            SetNz8(cpu, (uint8_t)cpu->y);
        } else {
            LoadYDirect16(memory, cpu, 0xa7u);
        }
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->y);
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            uint32_t primary = 0;
            uint8_t gate = 0;

            BitImmediate8(cpu, 0x18u);                         /* BBAE */
            if (!cpu->zero) {
                gate = 1;
            } else {
                BitImmediate8(cpu, 0x01u);
                if (cpu->zero) {
                    TransferYToA8(cpu);                        /* BBB6 */
                    if (cpu->zero)
                        primary = 0x83bbf3u;
                    else
                        gate = 1;
                }
            }
            if (gate) {
                LoadA8(cpu, Read8(memory, 0x7fd0a1u));         /* BBBE */
                BitImmediate8(cpu, 0x2cu);
                if (!cpu->zero)
                    TransferYToA8(cpu);
                if (cpu->zero)
                    primary = 0x83c7f8u;
            }
            if (primary != 0) {
                if (!child(child_context, cpu, primary,
                        primary == 0x83bbf3u ? 0x83bbb9u : 0x83bbc9u)) {
                    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_CHILD_UNWOUND;
                    return result;
                }
            }
            if (!child(child_context, cpu, 0x83d508u, 0x83bbccu)) {
                result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_CHILD_UNWOUND;
                return result;
            }
            SetAccumulatorWidth(cpu, 1);                       /* BBCF */
            SetIndexWidth(cpu, 1);
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));  /* BBD1 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        Compare8(cpu, A8(cpu), 0x28u);
        if (cpu->zero)
            break;
    }
    LoadA8(cpu, Read8(memory, 0x7fd0feu));                     /* BBDA */
    if (!cpu->zero) {
        LoadA8(cpu, 0x03u);
        Write8(memory, 0x7fe216u, A8(cpu));
    }
    LoadAAbsolute8(memory, cpu, 0x09a1u, 0);                   /* BBE6 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        TestBitsAbsolute8(memory, cpu, 0x09a1u, 0);
    }
    return result;
}

/* $80:CBAE: tick event timers $7F:D18C; 0 = handoff. */
static uint8_t FieldEventTimers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    cpu->program_bank = 0x80u;
    PushDataBank(memory, cpu);                                 /* CBAE */
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x1273u, 0), 0x00u);
    LoadA8(cpu, 0x02u);
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 0);
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = LongIndexedAddress(0x7fd18cu, cpu->x);

        LoadA8(cpu, Read8(memory, timer));                     /* CBBD */
        if (cpu->negative) {
            DecrementA8(cpu);
            Write8(memory, timer, A8(cpu));
            And8(cpu, 0x7fu);
            if (cpu->zero) {
                /* $80:CC35 runs the expired entry. */
                cpu->resume_pc = 0x80cbccu;
                return 0;
            }
        }
        IncrementX16(cpu);                                     /* CBF0 */
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->carry);
    LoadX16(cpu, 0x0007u);                                     /* CBF6 */
    for (;;) {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd18cu, cpu->x)));
        if (cpu->negative) {
            LoadA8(cpu, 0x02u);                                /* CBFF */
            TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);
            break;
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));                 /* CC06 */
        if (cpu->negative)
            break;
    }
    LoadAAbsolute8(memory, cpu, 0x1273u, 0);                   /* CC09 */
    if (!cpu->zero) {
        cpu->resume_pc = 0x80cc0eu;
        return 0;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* CC23 */
    PullDataBank(memory, cpu);
    return 1;
}

/* $83:80CD: field idle test; zero = no event running. */
static void FieldIdleBody(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t gates[5] = {0x09a8u, 0x0622u, 0x05b7u, 0x05b5u,
                                      0x17aau};
    static const uint8_t masks[5] = {0x08u, 0x88u, 0x07u, 0xa2u, 0x00u};
    unsigned i;

    for (i = 0; i < 5u; ++i) {
        LoadAAbsolute8(memory, cpu, gates[i], 0);              /* 80CD */
        if (masks[i])
            BitImmediate8(cpu, masks[i]);
        if (!cpu->zero)
            return;
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 80EE */
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero)
        return;
    LoadX8(cpu, 0x00u);                                        /* 80F5 */
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd057u, cpu->x)));
        if (cpu->negative)
            return;
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* 80FD */
        Compare8(cpu, (uint8_t)cpu->x, 0x08u);
    } while (!cpu->zero);
}

static void FieldIdle(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    FieldIdleBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:D927: edge bit of the cell next to $8F/$91. */
static void FieldEdgeTest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t sites[4] = {0xd936u, 0xd940u, 0xd94au, 0xd956u};
    const unsigned slot = (cpu->x >> 1) & 3u;

    SimulateJsrFrame(memory, cpu, 0xb966u);                    /* B964 */
    if (slot == 0)
        IncrementDirect8(memory, cpu, 0x91u);                  /* D932 */
    else if (slot == 3)
        IncrementDirect8(memory, cpu, 0x8fu);                  /* D952 */
    PrimaryCollisionIndex(memory, cpu, sites[slot], 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e4000u, cpu->x)));
    BitImmediate8(cpu, (slot == 0 || slot == 2) ? 0x10u : 0x20u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:B8BF: actor 8..39 touching the leader; 0 = handoff. */
static uint8_t FieldTouchScan(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x81f8u);
    LoadAAbsolute8(memory, cpu, 0x057cu, 0);                   /* B8BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x80u);
        And8(cpu, Read8(memory, DirectAddress(cpu, 0x47u)));
        if (!cpu->zero) {
            SimulateRtsFrame(memory, cpu);                     /* B8CA */
            return 1;
        }
    }
    SetIndexWidth(cpu, 0);                                     /* B8CB */
    LoadAAbsolute8(memory, cpu, 0x06bau, 0);
    Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
    Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
    PrimaryTileHeight(memory, cpu, 0xb8d9u);
    Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
    LoadX16(cpu, 0x0008u);
    for (;;) {
        uint8_t hit = 0;

        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* B8DF */
        BitImmediate8(cpu, 0x04u);
        if (!cpu->zero)
            goto next;
        BitImmediate8(cpu, 0x80u);
        if (!cpu->zero)
            goto next;
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x14u);
        if (!cpu->zero)
            goto next;
        LoadAAbsolute8(memory, cpu, 0x05fau, cpu->x);
        Compare8(cpu, A8(cpu), 0xfdu);
        if (cpu->zero)
            goto next;
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        And8(cpu, 0x02u);
        LsrA8(cpu);
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);          /* B901 */
        Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
        if (!cpu->zero) {
            LoadA8(cpu, (uint8_t)(A8(cpu) - 2u));              /* B908 */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
            if (cpu->carry)
                goto next;
            LoadA8(cpu, (uint8_t)(A8(cpu) + 3u));              /* B90E */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
            if (!cpu->carry)
                goto next;
            LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);      /* B915 */
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
            if (cpu->zero) {
                hit = 1;
            } else {
                cpu->carry = 0;
                Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
                hit = cpu->zero;
            }
        } else {
            LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);      /* B925 */
            LoadA8(cpu, (uint8_t)(A8(cpu) - 2u));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
            if (cpu->carry)
                goto next;
            LoadA8(cpu, (uint8_t)(A8(cpu) + 3u));              /* B92E */
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
            hit = cpu->carry;
        }
        if (hit) {
            uint8_t side;

            StoreXDirect16(memory, cpu, 0x65u);                /* B942 */
            LoadAAbsolute8(memory, cpu, 0x06e2u, cpu->x);
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x91u)));
            if (!cpu->zero) {
                side = cpu->carry ? 0u : 4u;                   /* B94B */
            } else {
                LoadAAbsolute8(memory, cpu, 0x06bau, cpu->x);  /* B955 */
                Compare8(
                    cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x8fu)));
                if (cpu->zero) {
                    cpu->resume_pc = 0x83b96du;
                    return 0;
                }
                side = cpu->carry ? 6u : 2u;
            }
            LoadX16(cpu, side);
            FieldEdgeTest(memory, cpu);
            if (cpu->zero) {
                cpu->resume_pc = 0x83b96du;                    /* B967 */
                return 0;
            }
            LoadXDirect16(memory, cpu, 0x65u);                 /* B969 */
        }
next:
        IncrementX16(cpu);                                     /* B93A */
        Compare16(cpu, cpu->x, 0x0028u);
        if (cpu->zero)
            break;
    }
    cpu->carry = 0;                                            /* B940 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

Lufia2ActorPrimaryUpdateResult Lufia2FieldTriggerUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 81C6 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x83u, 0x81ceu);
    if (!FieldEventTimers(memory, cpu)) {
        result.pc = cpu->resume_pc;
        return result;
    }
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x83u;
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* 81CF */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        SetIndexWidth(cpu, 1);                                 /* 81D6 */
        FieldIdle(memory, cpu, 0x81dau);
        SetIndexWidth(cpu, 0);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, 0x7fd0a1u));             /* 81DF */
            BitImmediate8(cpu, 0x3cu);
            if (cpu->zero) {
                PushDataBank(memory, cpu);                     /* 81E7 */
                LoadAAbsolute8(memory, cpu, 0x06bau, 0);
                Write8(memory, DirectAddress(cpu, 0x8fu), A8(cpu));
                LoadAAbsolute8(memory, cpu, 0x06e2u, 0);
                Write8(memory, DirectAddress(cpu, 0x91u), A8(cpu));
                LoadA8(cpu, 0x7eu);
                PushAccumulator8(memory, cpu);
                PullDataBank(memory, cpu);
                if (!FieldTouchScan(memory, cpu)) {
                    result.pc = cpu->resume_pc;
                    return result;
                }
                PullDataBank(memory, cpu);                     /* 81F9 */
            }
        }
    }
    LoadAAbsolute8(memory, cpu, 0x0622u, 0);                   /* 81FA */
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            uint8_t armed = 0;

            LoadA8(cpu, Read8(memory, 0x7fd0a1u));             /* 8205 */
            BitImmediate8(cpu, 0x04u);
            if (!cpu->zero) {
                armed = 1;
            } else {
                BitImmediate8(cpu, 0x38u);
                if (cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x09a8u, 0);
                    BitImmediate8(cpu, 0x08u);
                    armed = cpu->zero;
                }
            }
            if (armed) {
                LoadAAbsolute8(memory, cpu, 0x05b5u, 0);       /* 8218 */
                BitImmediate8(cpu, 0x02u);
                if (cpu->zero) {
                    LoadA8(cpu, Read8(memory, 0x7fd0a3u));
                    if (!cpu->zero) {
                        /* $80:E722 queues the event. */
                        result.pc = cpu->resume_pc = 0x838225u;
                        return result;
                    }
                }
            }
        }
    }
    LoadAAbsolute8(memory, cpu, 0x05b5u, 0);                   /* 823B */
    BitImmediate8(cpu, 0x10u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0622u, 0);
        BitImmediate8(cpu, 0x80u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, 0x7fd0a1u));
            BitImmediate8(cpu, 0x08u);
            if (cpu->zero) {
                /* Door/warp handling stays in LLE. */
                result.pc = cpu->resume_pc = 0x838251u;
                return result;
            }
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 829E */
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83829fu;
    return result;
}

/* $83:E200: sprite frame $54 for object $A7. */
static void ObjectSetFrame(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* E200 */
    LoadXDirect(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    And8(cpu, 0x3fu);
    Write8(memory, 0x004202u, A8(cpu));
    TransferDirectToA(cpu);                                    /* E212 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    Write8(memory, 0x004203u, A8(cpu));
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0732u, 0));
    LoadXDirect(memory, cpu, 0xabu);                           /* E223 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe0f0u, cpu->x)));
    Write8(memory, LongIndexedAddress(0x7fd90eu, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, 0x004216u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);                               /* E234 */
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fe0eeu, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x7fd90cu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);                           /* E242 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    AslA8(cpu);
    Adc8(cpu, 0x00u);
    And8(cpu, 0x03u);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);              /* E250 */
    And8(cpu, 0xfcu);
    Or8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Or8(cpu, 0x20u);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    cpu->y = PullIndexValue(memory, cpu);                      /* E25C */
    SimulateRtsFrame(memory, cpu);
}

static void ToggleLong8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t address,
    uint8_t bits) {
    LoadA8(cpu, Read8(memory, address));
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ bits));
    Write8(memory, address, A8(cpu));
}

typedef enum ObjectFlow {
    OBJECT_FLOW_DISPATCH = 0,
    OBJECT_FLOW_RETURN = 1,
    OBJECT_FLOW_BOUNDARY = 2,
} ObjectFlow;

/* $83:E143: store the cursor, PLB, RTS. */
static ObjectFlow ObjectSaveAndReturn(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xabu);                           /* E143 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_RETURN;
}

/* $83:E11C: advance by A; yield on $064A bit 4. */
static ObjectFlow ObjectAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* E11C */
    Write16Direct(memory, cpu, 0x54u, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(memory, LongIndexedAddress(0x7fdeeeu, cpu->x), cpu->accumulator);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);                           /* E12C */
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    BitImmediate8(cpu, 0x10u);
    if (cpu->zero)
        return OBJECT_FLOW_DISPATCH;
    LoadA8(cpu, 0x01u);                                        /* E135 */
    Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    PullDataBank(memory, cpu);
    return OBJECT_FLOW_RETURN;
}

/* $83:FB05: sign-extend nibble A; M=0 exit. */
static void ObjectSignNibble(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    BitImmediate8(cpu, 0x08u);                                 /* FB05 */
    if (!cpu->zero) {
        Or8(cpu, 0xf0u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0xffu);
        ExchangeAccumulatorBytes(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $83:E1AD / $83:E1B9: add A16 to a position word. */
static void ObjectAddPosition(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t base,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(base, cpu->x)));
    Write16Long(memory, LongIndexedAddress(base, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:EED8: random offset around 0 of width operand. */
static void ObjectJitter(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);              /* EED8 */
    LsrA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    PrimaryCallRandom(memory, cpu, 0xeee4u);                   /* $80:8299 */
    cpu->carry = 1;
    Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    PrimarySignExtend(memory, cpu, 0xeeeau);
    SimulateRtsFrame(memory, cpu);
}

/* $83:ED63: low nibble into a handler table. */
static uint16_t ObjectSubDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t table,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* ED63 */
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    IncrementY16(cpu);
    And8(cpu, 0x0fu);
    AslA8(cpu);
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
    return Read16ProgramIndexed(memory, cpu, table, cpu->x);
}

/* $83:EE0E: copy animation state of slot X to $A7. */
static void ObjectTakeAnimation(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe286u, cpu->x)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x)));
    PushAccumulator8(memory, cpu);
    TransferXToA(cpu);
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    LoadXDirect(memory, cpu, 0xa7u);
    Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fe3a6u, cpu->x), A8(cpu));
    LoadA8(cpu, Pull8(memory, cpu));
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:AB7C: claim A free sprite slots in $7E:E100. */
static void ObjectAllocSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    PushDataBank(memory, cpu);                                 /* AB7C */
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX8(cpu, 0x00u);
    LoadY8(cpu, 0x00u);
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x56u), 0x00u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0xe100u, cpu->x);          /* AB8A */
        if (cpu->zero) {
            /* Y keeps counting across used slots. */
            LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            Compare8(cpu, (uint8_t)cpu->y,
                Read8(memory, DirectAddress(cpu, 0x54u)));
            if (cpu->zero)
                break;
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));
            continue;
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x56u))); /* AB97 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        Write8(memory, DirectAddress(cpu, 0x56u), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        TransferAToX(cpu);
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->zero) {
            PullDataBank(memory, cpu);                         /* ABA5 */
            cpu->carry = 1;
            SimulateRtlFrame(memory, cpu);
            return;
        }
    }
    LoadX8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));     /* ABA8 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    Or8(cpu, 0x80u);
    do {
        StoreAAbsolute8(memory, cpu, 0xe100u, cpu->x);
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        DecrementDirect8(memory, cpu, 0x54u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);                                 /* ABB6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    And8(cpu, 0xf0u);
    TrbDirect8(memory, cpu, 0x55u);
    AslA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    {
        const uint8_t carry = cpu->carry;                      /* ROL */

        cpu->carry = 0;
        LoadA8(cpu, (uint8_t)((A8(cpu) << 1) | carry));
    }
    ExchangeAccumulatorBytes(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    cpu->carry = 0;
    SimulateRtlFrame(memory, cpu);
}

/* $83:ECDE: sprite slots and VRAM base for object $A7. */
static void ObjectSpriteSetup(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xecdbu);
    SetAccumulatorWidth(cpu, 1);                               /* ECDE */
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
    ObjectAllocSprite(memory, cpu, 0xecf6u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* ECF7 */
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    SpriteVramBase(memory, cpu, 0xed05u);
    SetIndexWidth(cpu, 0);                                     /* ED06 */
    LoadXDirect(memory, cpu, 0xa9u);
    Write16Long(memory, LongIndexedAddress(0x7fe0aeu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:EC9C: animation A for object X. */
static void ObjectAnimationSetup(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    PushY(memory, cpu);                                        /* EC9C */
    Write8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    TransferAToX(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83f388u, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    Write16Long(memory, LongIndexedAddress(0x7fe0eeu, cpu->x), cpu->accumulator);
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83f38au, cpu->x)));
    LoadXDirect(memory, cpu, 0xabu);
    Write8(memory, LongIndexedAddress(0x7fe0f0u, cpu->x), A8(cpu));
    cpu->x = cpu->y;
    SetNz16(cpu, cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83f38bu, cpu->x)));
    SetIndexWidth(cpu, 1);                                     /* ECC3 */
    LoadXDirect(memory, cpu, 0xa7u);
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    And8(cpu, 0x0fu);
    Write8(memory, LongIndexedAddress(0x7fe1f6u, cpu->x), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
    ObjectSpriteSetup(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* ECDC */
    SimulateRtsFrame(memory, cpu);
}

/* $83:E8A6: spawn child object from the operand block at Y. */
static void ObjectSpawnChild(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* E8A6 */
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    PrimarySignExtend(memory, cpu, 0xe8acu);
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);                                    /* E8B1 */
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);
    PrimarySignExtend(memory, cpu, 0xe8b7u);
    Write16Direct(memory, cpu, 0x63u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);                           /* E8BC */
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8c5u);
    SecondarySpawnActor(memory, cpu);                          /* DF87 */
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);                      /* E8C6 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
    LoadXDirect(memory, cpu, 0xa9u);
    StoreYDirect16(memory, cpu, 0xa7u);
    SimulateJslFrame(memory, cpu, 0x83u, 0xe8d4u);
    SecondaryRecordOffsets(memory, cpu);                       /* AB4F */
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* E8D5 */
    cpu->y = cpu->x;                                           /* TXY */
    SetNz16(cpu, cpu->y);
    LoadXDirect(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Direct(memory, cpu, 0x63u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x)));
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x)));
    cpu->x = cpu->y;                                           /* TYX */
    SetNz16(cpu, cpu->x);
    Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
    PullAccumulator16(memory, cpu);
    Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
    Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PushIndex(memory, cpu);                                    /* E90D */
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
    LoadXDirect(memory, cpu, 0x54u);
    Write8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x), A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    Write8(memory, LongIndexedAddress(0x7fda2cu, cpu->x), A8(cpu));
    TransferDirectToA(cpu);                                    /* E924 */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0xa7u)));
    if (cpu->carry) {
        TransferAToX(cpu);                                     /* E92B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x)));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
    }
    cpu->x = PullIndexValue(memory, cpu);                      /* E935 */
    SimulateRtsFrame(memory, cpu);
}

/* Size-0 sprites can spin $83:AB7C forever. */
static uint8_t ObjectSpriteSizeZero(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t animation) {
    const uint16_t index =
        (uint16_t)(((cpu->direct_page & 0xff00u) | animation) << 2);
    const uint8_t shape = Read8(memory, LongIndexedAddress(0x83f38bu, index));

    return Read8(memory, 0x83abf4u + (uint32_t)(shape >> 4)) == 0;
}

/* $83:ABCC: clear A sprite slots from $54/$55. */
static void ObjectFreeSprite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    ExchangeAccumulatorBytes(cpu);                             /* ABCC */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
    LsrA8(cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    {
        const uint8_t carry = cpu->carry;                      /* ROR */

        cpu->carry = (A8(cpu) & 0x01u) != 0;
        LoadA8(cpu, (uint8_t)((A8(cpu) >> 1) | (carry ? 0x80u : 0u)));
    }
    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
    And8(cpu, 0xf0u);
    TrbDirect8(memory, cpu, 0x54u);
    LsrA8(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
    TransferAToX(cpu);
    ExchangeAccumulatorBytes(cpu);
    TransferAToY(cpu);
    TransferDirectToA(cpu);
    do {
        Write8(memory, LongIndexedAddress(0x7ee100u, cpu->x), A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        LoadY8(cpu, (uint8_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SimulateRtlFrame(memory, cpu);
}

/* $83:F205: despawn object $A7, free its sprite. */
static void ObjectDespawn(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0xe140u);
    LoadXDirect(memory, cpu, 0xa7u);                           /* F205 */
    LoadA8(cpu, 0x04u);
    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
    SetIndexWidth(cpu, 1);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        TransferDirectToA(cpu);                                /* F218 */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe23eu, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe286u, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
            ExchangeAccumulatorBytes(cpu);
            TransferAToX(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf4u, cpu->x)));
            ObjectFreeSprite(memory, cpu, 0xf237u);
        }
    }
    LoadXDirect(memory, cpu, 0xa7u);                           /* F238 */
    LoadA8(cpu, 0xffu);
    Write8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fe3a6u, cpu->x), A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x1559u, cpu->x);
    Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
    Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
    SetIndexWidth(cpu, 0);
    SimulateRtlFrame(memory, cpu);
}

static ObjectFlow ObjectExecute(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t handler) {
    switch (handler) {
    case 0xed45u:
        handler = ObjectSubDispatch(memory, cpu, 0xf368u, 0xed47u);
        break;
    case 0xed4bu:
        handler = ObjectSubDispatch(memory, cpu, 0xf348u, 0xed4du);
        break;
    case 0xed51u:
        handler = ObjectSubDispatch(memory, cpu, 0xf328u, 0xed53u);
        break;
    case 0xed57u:
        handler = ObjectSubDispatch(memory, cpu, 0xf308u, 0xed59u);
        break;
    case 0xed5du:
        handler = ObjectSubDispatch(memory, cpu, 0xf2e8u, 0xed5fu);
        break;
    default:
        break;
    }

    switch (handler) {
    case 0xe831u:                                              /* 4x */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        And8(cpu, 0x0fu);
        Write8(memory, LongIndexedAddress(0x7fdfaeu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        return ObjectSaveAndReturn(memory, cpu);

    case 0xe168u:                                              /* F6 */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xe16fu);
        LoadXDirect(memory, cpu, 0xa9u);                       /* E176 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        ObjectSignNibble(memory, cpu, 0xe181u);
        ObjectAddPosition(memory, cpu, 0x7fdcdcu, 0xe184u);
        TransferDirectToA(cpu);                                /* E185 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        And8(cpu, 0x0fu);
        ObjectSignNibble(memory, cpu, 0xe18cu);
        ObjectAddPosition(memory, cpu, 0x7fdd6cu, 0xe18fu);
        SimulateRtsFrame(memory, cpu);
        TransferDirectToA(cpu);                                /* E170 */
        LoadA8(cpu, 0x01u);
        return ObjectAdvance(memory, cpu);

    case 0xeeb5u:                                              /* F2 */
        LoadXDirect(memory, cpu, 0xa9u);
        ObjectJitter(memory, cpu, 0xeeb9u);
        IncrementY16(cpu);                                     /* EEBA */
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x)));
        Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        ObjectJitter(memory, cpu, 0xeec8u);
        IncrementY16(cpu);                                     /* EEC9 */
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x)));
        Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xe589u:                                              /* 1A */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        BitImmediate8(cpu, 0x20u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* E593 */
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            ObjectSetFrame(memory, cpu, 0xe59au);
        }
        IncrementY16(cpu);                                     /* E59B */
        return OBJECT_FLOW_DISPATCH;

    case 0xead5u:                                              /* 80 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe08eu, cpu->x)));
        DecrementA8(cpu);
        Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
        if (cpu->negative) {
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* EAE2 */
            Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
            return OBJECT_FLOW_DISPATCH;
        }
        LoadXDirect(memory, cpu, 0xabu);                       /* EAEA */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfcfu, cpu->x)));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfceu, cpu->x)));
        TransferAToY(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdfd0u, cpu->x)));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe150u:                                              /* 2F */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7fddaeu));
        Write16Long(memory, LongIndexedAddress(0x7fddfeu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fde3eu));
        Write16Long(memory, LongIndexedAddress(0x7fde8eu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xe270u:                                              /* 2E */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe5b5u:                                              /* 1D */
        LoadXDirect(memory, cpu, 0xa9u);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x1724u, 0), (uint8_t)cpu->x);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x1725u, 0),
            (uint8_t)(cpu->x >> 8));
        return OBJECT_FLOW_DISPATCH;

    case 0xe700u:                                              /* 12 */
        return OBJECT_FLOW_DISPATCH;

    case 0xe810u:                                              /* 13 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x001220u));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x0080u);
        Write16Long(memory, LongIndexedAddress(0x7fda6cu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x001228u));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x0070u);
        Write16Long(memory, LongIndexedAddress(0x7fdaacu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xea7du:                                              /* 22 */
        LoadAAbsolute8(memory, cpu, 0x066au, 0);
        And8(cpu, 0x06u);
        Or8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        StoreAAbsolute8(memory, cpu, 0x066au, 0);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xea8cu:                                              /* 23 */
        TransferDirectToA(cpu);
        Write8(memory, 0x7fd0a1u, A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xea94u:                                              /* 8D */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Or8(cpu, Read8(memory, 0x7fd0a1u));
        Write8(memory, 0x7fd0a1u, A8(cpu));
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xeab5u:                                              /* 7x */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        DecrementA8(cpu);
        Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadXDirect(memory, cpu, 0xabu);                       /* EAC1 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        Write16Long(memory, LongIndexedAddress(0x7fdfceu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        PushDataBank(memory, cpu);                             /* EACC */
        LoadA8(cpu, Pull8(memory, cpu));
        Write8(memory, LongIndexedAddress(0x7fdfd0u, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xeb50u:                                              /* 29 */
    case 0xec6du:                                              /* 2B */
    case 0xed11u: {                                            /* Cx */
        uint8_t set;
        uint32_t target;
        uint8_t bit;

        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        if (handler == 0xeb50u) {
            set = !cpu->zero;
            target = AbsoluteIndexedAddress(cpu, 0x064au, cpu->x);
            bit = 0x04u;
        } else if (handler == 0xec6du) {
            set = cpu->negative;
            target = LongIndexedAddress(0x7fdb0cu, cpu->x);
            bit = 0x40u;
        } else {
            And8(cpu, 0x0fu);
            set = cpu->zero;
            target = AbsoluteIndexedAddress(cpu, 0x064au, cpu->x);
            bit = 0x08u;
        }
        LoadA8(cpu, Read8(memory, target));
        if (set)
            Or8(cpu, bit);
        else
            And8(cpu, (uint8_t)~bit);
        Write8(memory, target, A8(cpu));
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xed9bu:                                              /* 84 */
        LoadA8(cpu, Read8(memory, 0x7fd0a1u));
        Or8(cpu, 0x10u);
        Write8(memory, 0x7fd0a1u, A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xedc2u:                                              /* 89 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, 0xffu);
        Write8(memory, LongIndexedAddress(0x7fe23eu, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe33eu, cpu->x)));
        Or8(cpu, 0x80u);
        Write8(memory, LongIndexedAddress(0x7fe33eu, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xedd7u:                                              /* EA */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        Or8(cpu, 0x80u);
        Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xefd1u:                                              /* F8 */
    case 0xefdeu:                                              /* F9 */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
        if (handler == 0xefd1u)
            Or8(cpu, 0x10u);
        else
            And8(cpu, 0xefu);
        StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
        return OBJECT_FLOW_DISPATCH;

    case 0xf0e8u:                                              /* E2 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
        Write16Long(memory, LongIndexedAddress(0x7fda6cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xf0fau:                                              /* 82 */
    case 0xf10bu:                                              /* 83 */
        LoadXDirect(memory, cpu, 0xa9u);
        SetAccumulatorWidth(cpu, 0);
        if (handler == 0xf0fau)
            CopyLong16(memory, cpu, 0x7fda6cu, 0x7fdaacu);
        else
            CopyLong16(memory, cpu, 0x7fdaacu, 0x7fda6cu);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xf11cu: {                                            /* EB */
        unsigned i;

        LoadXDirect(memory, cpu, 0xa9u);
        /* Each byte negated on its own. */
        for (i = 0; i < 2u; ++i) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdaacu + i, cpu->x)));
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            Write8(memory, LongIndexedAddress(0x7fda6cu + i, cpu->x), A8(cpu));
        }
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xf137u:                                              /* EC */
        LoadXDirect(memory, cpu, 0xa7u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        cpu->carry = 1;
        Sbc8(cpu, Read8(memory, LongIndexedAddress(0x7fe08eu, cpu->x)));
        DecrementA8(cpu);
        Write8(memory, LongIndexedAddress(0x7fe08eu, cpu->x), A8(cpu));
        IncrementY16(cpu);
        SetAccumulatorWidth(cpu, 0);                           /* F147 */
        LoadXDirect(memory, cpu, 0xabu);
        LoadA16(cpu, cpu->y);
        Write16Long(memory, LongIndexedAddress(0x7fdfceu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return OBJECT_FLOW_DISPATCH;

    case 0xf155u:                                              /* ED */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x8ec7u);
        Write16Long(memory, 0x7fddacu, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        PushDataBank(memory, cpu);                             /* F164 */
        PushY(memory, cpu);
        LoadA8(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadX16(cpu, 0x0000u);
        do {
            TransferDirectToA(cpu);                            /* F16D */
            LoadAAbsolute8(memory, cpu, 0xd0a6u, cpu->x);
            if (!cpu->negative) {
                Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
                AslA8(cpu);
                Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
                TransferAToY(cpu);
                LoadAAbsolute8(memory, cpu, 0xddacu, 0);
                StoreAAbsolute8(memory, cpu, 0xdeeeu, cpu->y);
                LoadAAbsolute8(memory, cpu, 0xddadu, 0);
                StoreAAbsolute8(memory, cpu, 0xdeefu, cpu->y);
            }
            IncrementX16(cpu);                                 /* F185 */
            Compare16(cpu, cpu->x, 0x0008u);
        } while (!cpu->zero);
        cpu->y = PullIndexValue(memory, cpu);                  /* F18B */
        PullDataBank(memory, cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe1c5u:                                              /* 3x */
    case 0xe1d3u:                                              /* 2C */
    case 0xe1e6u:                                              /* 25 */
    case 0xe1f4u:                                              /* FD */
    case 0xe59fu: {                                            /* 8F */
        uint16_t back;
        uint8_t skip_cursor = 0;

        if (handler == 0xe1c5u) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            And8(cpu, 0x0fu);
            back = 0xe1ceu;
        } else if (handler == 0xe1d3u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            cpu->carry = 0;
            Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x)));
            back = 0xe1e1u;
        } else if (handler == 0xe1e6u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
            back = 0xe1f0u;
            skip_cursor = 1;
        } else if (handler == 0xe1f4u) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            back = 0xe1fbu;
        } else {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
            BitImmediate8(cpu, 0x20u);
            if (cpu->zero)
                return OBJECT_FLOW_DISPATCH;
            LoadA8(cpu, Read8(memory, 0x7fd4f4u));
            back = 0xe5b1u;
            skip_cursor = 1;
        }
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        ObjectSetFrame(memory, cpu, back);
        if (!skip_cursor)
            IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xe840u:                                              /* 5x */
    case 0xeb38u:                                              /* 28 */
    case 0xed32u: {                                            /* Dx */
        if (handler == 0xe840u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd9ccu, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
            SetAccumulatorWidth(cpu, 0);                       /* E84A */
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            TransferAToY(cpu);
        } else if (handler == 0xeb38u) {
            LoadXDirect(memory, cpu, 0xa7u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda2cu, cpu->x)));
            AslA8(cpu);
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
            SetAccumulatorWidth(cpu, 0);                       /* EB43 */
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
            TransferAToY(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
        SimulateJsrFrame(memory, cpu, 0xed34u);                /* ED32 */
        SetAccumulatorWidth(cpu, 0);                           /* ED38 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->y));
        cpu->carry = 0;
        Add16Immediate(cpu, 0x8ec7u);
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
        SimulateRtsFrame(memory, cpu);
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xee48u:                                              /* FB */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        SimulateJsrFrame(memory, cpu, 0xee4fu);
        LoadXDirect(memory, cpu, 0xa9u);                       /* EE56 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        ObjectSignNibble(memory, cpu, 0xee61u);
        ObjectAddPosition(memory, cpu, 0x7fddfeu, 0xee64u);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        And8(cpu, 0x0fu);
        ObjectSignNibble(memory, cpu, 0xee6cu);
        ObjectAddPosition(memory, cpu, 0x7fde8eu, 0xee6fu);
        SimulateRtsFrame(memory, cpu);
        TransferDirectToA(cpu);                                /* EE50 */
        LoadA8(cpu, 0x01u);
        return ObjectAdvance(memory, cpu);

    case 0xee71u:                                              /* FC */
    case 0xf19fu: {                                            /* E3 */
        const uint8_t fixed = handler == 0xf19fu;

        if (fixed) {
            LoadXDirect(memory, cpu, 0xa9u);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda6cu, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda6du, cpu->x)));
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        } else {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        }
        SimulateJsrFrame(memory, cpu, fixed ? 0xf1afu : 0xee7du);
        LoadXDirect(memory, cpu, 0xa9u);                       /* EE84 */
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        PrimarySignExtend(memory, cpu, 0xee8bu);
        ObjectAddPosition(memory, cpu, 0x7fddfeu, 0xee8eu);
        TransferDirectToA(cpu);                                /* EE91 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
        PrimarySignExtend(memory, cpu, 0xee96u);
        ObjectAddPosition(memory, cpu, 0x7fde8eu, 0xee99u);
        SimulateRtsFrame(memory, cpu);
        TransferDirectToA(cpu);                                /* EE7E/F1B0 */
        if (!fixed)
            LoadA8(cpu, 0x02u);
        return ObjectAdvance(memory, cpu);
    }

    case 0xe191u:                                              /* F7 */
        LoadXDirect(memory, cpu, 0xa9u);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        PrimarySignExtend(memory, cpu, 0xe199u);
        ObjectAddPosition(memory, cpu, 0x7fdcdcu, 0xe19cu);
        TransferDirectToA(cpu);                                /* E19D */
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        PrimarySignExtend(memory, cpu, 0xe1a3u);
        ObjectAddPosition(memory, cpu, 0x7fdd6cu, 0xe1a6u);
        TransferDirectToA(cpu);                                /* E1A7 */
        LoadA8(cpu, 0x02u);
        return ObjectAdvance(memory, cpu);

    case 0xe99au: {                                            /* 20 */
        unsigned i;

        LoadXDirect(memory, cpu, 0xa9u);
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        IncrementY16(cpu);
        TransferDirectToA(cpu);                                /* E9A2 */
        for (i = 0; i < 2u; ++i) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdaacu + i, cpu->x)));
            if (cpu->zero)
                continue;
            if (cpu->negative) {
                cpu->carry = 0;
                Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            } else {
                cpu->carry = 1;
                Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
            }
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            Write8(memory, LongIndexedAddress(0x7fda6cu + i, cpu->x), A8(cpu));
        }
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xede6u: {                                            /* F1 */
        uint8_t found = 0;

        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
        LoadX16(cpu, 0x0000u);
        for (;;) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
            if (cpu->zero) {
                found = 1;
                break;
            }
            IncrementX16(cpu);                                 /* EDF6 */
            Compare16(cpu, cpu->x, 0x0020u);
            if (cpu->zero)
                break;
        }
        if (!found) {
            /* No match: the ROM clears slot 32. */
            TransferDirectToA(cpu);                            /* EDFC */
            Write8(memory, LongIndexedAddress(0x7fe286u, cpu->x), A8(cpu));
            Write8(memory, LongIndexedAddress(0x7fe2ceu, cpu->x), A8(cpu));
        } else {
            ObjectTakeAnimation(memory, cpu, 0xee09u);
        }
        IncrementY16(cpu);                                     /* EE0A */
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xe437u:                                              /* 19 */
    case 0xe445u: {                                            /* 8E */
        const uint8_t animation = handler == 0xe437u
            ? Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y))
            : Read8(memory, 0x7fd4f3u);

        if (ObjectSpriteSizeZero(memory, cpu, animation)) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        if (handler == 0xe437u) {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            IncrementY16(cpu);
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
            IncrementY16(cpu);
        } else {
            LoadA8(cpu, Read8(memory, 0x7fd4f3u));
            Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
            LoadA8(cpu, Read8(memory, 0x7fd4f4u));
            Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        }
        LoadX16(cpu, 0x0000u);                                 /* E453 */
        for (;;) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1aeu, cpu->x)));
            Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x54u)));
            if (cpu->zero) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb2cu, cpu->x)));
                Compare8(cpu, A8(cpu), Read8(memory, DirectAddress(cpu, 0x55u)));
                if (cpu->zero) {
                    ObjectTakeAnimation(memory, cpu, 0xe483u); /* E481 */
                    return OBJECT_FLOW_DISPATCH;
                }
            }
            IncrementX16(cpu);                                 /* E466 */
            Compare16(cpu, cpu->x, 0x0020u);
            if (cpu->zero)
                break;
        }
        LoadXDirect(memory, cpu, 0xa7u);                       /* E46C */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x)));
        Or8(cpu, 0x20u);
        Write8(memory, LongIndexedAddress(0x7fdb0cu, cpu->x), A8(cpu));
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        ObjectAnimationSetup(memory, cpu, 0xe47du);
        return OBJECT_FLOW_DISPATCH;
    }

    case 0xeaffu:                                              /* 26 */
        if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
                LongIndexedAddress(0x7fda4cu, Read16Direct(memory, cpu, 0xa7u))))) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        LoadXDirect(memory, cpu, 0xa7u);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fda4cu, cpu->x)));
        ObjectAnimationSetup(memory, cpu, 0xeb08u);
        return OBJECT_FLOW_DISPATCH;

    case 0xec8eu:                                              /* A0 */
        if (ObjectSpriteSizeZero(memory, cpu, Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y)))) {
            cpu->resume_pc = 0x830000u | handler;
            return OBJECT_FLOW_BOUNDARY;
        }
        LoadXDirect(memory, cpu, 0xa7u);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        ObjectAnimationSetup(memory, cpu, 0xec96u);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe854u:                                              /* 60 */
        IncrementY16(cpu);
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe858u);
        cpu->y = PullIndexValue(memory, cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe860u:                                              /* 85 */
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe863u);
        cpu->y = PullIndexValue(memory, cpu);
        TransferDirectToA(cpu);                                /* E865 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xa7u)));
        Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        ObjectTakeAnimation(memory, cpu, 0xe871u);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x55u)));
        Write8(memory, DirectAddress(cpu, 0xa7u), A8(cpu));
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe87cu:                                              /* 8A */
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe87fu);
        cpu->y = PullIndexValue(memory, cpu);
        TransferDirectToA(cpu);                                /* E881 */
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u)));
        AslA8(cpu);
        TransferAToX(cpu);
        LoadAAbsolute8(memory, cpu, 0x0003u, cpu->y);
        PrimarySignExtend(memory, cpu, 0xe88bu);
        Write16Long(memory, LongIndexedAddress(0x7fdcdcu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        /* No TDC: B keeps the first high byte. */
        LoadAAbsolute8(memory, cpu, 0x0004u, cpu->y);
        PrimarySignExtend(memory, cpu, 0xe897u);
        Write16Long(memory, LongIndexedAddress(0x7fdd6cu, cpu->x), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        return OBJECT_FLOW_DISPATCH;

    case 0xe937u:                                              /* EE */
        PushY(memory, cpu);
        ObjectSpawnChild(memory, cpu, 0xe93au);
        cpu->y = PullIndexValue(memory, cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        TransferDirectToA(cpu);                                /* E93F */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fda6cu, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        Write8(memory, LongIndexedAddress(0x7fda6du, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadX16(cpu, 0x0000u);                                 /* E950 */
        for (;;) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd0a6u, cpu->x)));
            if (cpu->negative)
                break;
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, 0x0008u);
            if (cpu->zero) {
                LoadX16(cpu, 0x0000u);
                break;
            }
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x54u))); /* E962 */
        Write8(memory, LongIndexedAddress(0x7fd0a6u, cpu->x), A8(cpu));
        return OBJECT_FLOW_DISPATCH;

    case 0xe13du:                                              /* 0x */
        ObjectDespawn(memory, cpu);
        PullDataBank(memory, cpu);                             /* E141 */
        return OBJECT_FLOW_RETURN;

    default:
        cpu->resume_pc = 0x830000u | handler;
        return OBJECT_FLOW_BOUNDARY;
    }
}

/* $83:E0FC: run object $A7's script until it yields. */
static uint8_t ObjectRunScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t *dispatches) {
    unsigned steps;

    PushDataBank(memory, cpu);                                 /* E0FC */
    LoadXDirect(memory, cpu, 0xabu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdef0u, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdeefu, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fdeeeu, cpu->x)));
    TransferAToY(cpu);
    /* A script that never yields spins the ROM forever. */
    for (steps = 0; steps < 0x10000u; ++steps) {
        uint16_t handler;
        ObjectFlow flow;

        ++*dispatches;
        TransferDirectToA(cpu);                                /* E10F */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        LsrA8(cpu);
        LsrA8(cpu);
        LsrA8(cpu);
        And8(cpu, 0x1eu);
        TransferAToX(cpu);
        handler = Read16ProgramIndexed(memory, cpu, 0xf2c8u, cpu->x);
        flow = ObjectExecute(memory, cpu, handler);
        if (flow == OBJECT_FLOW_RETURN)
            return 1;
        if (flow == OBJECT_FLOW_BOUNDARY)
            return 0;
    }
    cpu->resume_pc = 0x83e10fu;
    return 0;
}

Lufia2ActorPrimaryUpdateResult Lufia2ObjectSlotsUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83e0fbu;
    result.dispatches = 0;
    SetIndexWidth(cpu, 0);                                     /* E03E */
    TransferDirectToA(cpu);
    Write8(memory, 0x7fddacu, A8(cpu));
    Write8(memory, 0x7fddadu, A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x09a8u, 0), 0x00u);
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);          /* E04F */
        BitImmediate8(cpu, 0x80u);
        if (!cpu->zero) {
            const uint32_t flags = LongIndexedAddress(0x7fdb0cu, cpu->x);
            const uint32_t blink = LongIndexedAddress(0x7fe386u, cpu->x);
            uint8_t animate = 0;

            LoadA8(cpu, Read8(memory, flags));                 /* E059 */
            BitImmediate8(cpu, 0x10u);
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, blink));
                DecrementA8(cpu);
                Write8(memory, blink, A8(cpu));
                And8(cpu, 0x0fu);
                if (cpu->zero) {
                    LoadA8(cpu, Read8(memory, blink));         /* E06E */
                    LsrA8(cpu);
                    LsrA8(cpu);
                    LsrA8(cpu);
                    LsrA8(cpu);
                    Or8(cpu, Read8(memory, blink));
                    Write8(memory, blink, A8(cpu));
                    ToggleLong8(
                        memory, cpu,
                        LongIndexedAddress(0x7fe33eu, cpu->x), 0x80u);
                }
            }
            LoadAAbsolute8(memory, cpu, 0x09a7u, 0);           /* E088 */
            BitImmediate8(cpu, 0x01u);
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory, flags));             /* E08F */
                BitImmediate8(cpu, 0x40u);
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);
                    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
                    TestBitsAbsolute8(memory, cpu, 0x09a8u, 1);
                }
                LoadAAbsolute8(memory, cpu, 0x064au, cpu->x);  /* E09F */
                BitImmediate8(cpu, 0x08u);
                if (!cpu->zero) {
                    BitImmediate8(cpu, 0x40u);
                    animate = cpu->zero;
                }
                if (!animate) {
                    And8(cpu, 0xbfu);                          /* E0AA */
                    StoreAAbsolute8(memory, cpu, 0x064au, cpu->x);
                }
            }
            if (!animate) {
                const uint32_t wait = LongIndexedAddress(0x7fdfaeu, cpu->x);

                LoadA8(cpu, Read8(memory, wait));              /* E0AF */
                DecrementA8(cpu);
                Write8(memory, wait, A8(cpu));
                if (cpu->zero) {
                    StoreXDirect16(memory, cpu, 0xa7u);        /* E0BA */
                    SimulateJslFrame(memory, cpu, 0x83u, 0xe0bfu);
                    SecondaryRecordOffsets(memory, cpu);
                    SimulateRtlFrame(memory, cpu);
                    SimulateJsrFrame(memory, cpu, 0xe0c2u);    /* E0C0 */
                    if (!ObjectRunScript(
                            memory, cpu, &result.dispatches)) {
                        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
                        result.pc = cpu->resume_pc;
                        return result;
                    }
                    SimulateRtsFrame(memory, cpu);
                    LoadXDirect16(memory, cpu, 0xa7u);         /* E0C3 */
                }
            } else {
                const uint32_t step = LongIndexedAddress(0x7fdaecu, cpu->x);

                LoadA8(cpu, Read8(memory, step));              /* E0C7 */
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                Write8(memory, step, A8(cpu));
                Compare8(cpu, A8(cpu), 0x20u);
                if (cpu->zero) {
                    TransferDirectToA(cpu);                    /* E0D4 */
                    Write8(memory, step, A8(cpu));
                    StoreXDirect16(memory, cpu, 0xa7u);
                    SimulateJslFrame(memory, cpu, 0x83u, 0xe0deu);
                    SecondaryRecordOffsets(memory, cpu);
                    SimulateRtlFrame(memory, cpu);
                    ToggleLong8(
                        memory, cpu,
                        LongIndexedAddress(0x7fdb2cu, cpu->x), 0x01u);
                    Write8(memory, DirectAddress(cpu, 0x54u), A8(cpu));
                    ObjectSetFrame(memory, cpu, 0xe0edu);      /* E0EB */
                    LoadXDirect16(memory, cpu, 0xa7u);
                }
            }
        }
        IncrementX16(cpu);                                     /* E0F0 */
        Compare16(cpu, cpu->x, 0x0020u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 1);                                     /* E0F9 */
    return result;
}

static void Write16Absolute(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t value) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)value);
    Write8(memory, AbsoluteIndexedAddress(cpu, (uint16_t)(address + 1u), 0),
        (uint8_t)(value >> 8));
}

static void StoreYIndex(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address) {
    if (cpu->index_is_8_bit)
        Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)cpu->y);
    else
        Write16Absolute(memory, cpu, address, cpu->y);
}

static void StoreA8Absolute(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint8_t value) {
    LoadA8(cpu, value);
    StoreAAbsolute8(memory, cpu, address, 0);
}

/* $83:A052: CGRAM upload on DMA channel 6. */
static void NmiCgramUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    StoreYIndex(memory, cpu, 0x4365u);                         /* A052 */
    StoreAAbsolute8(memory, cpu, 0x2121u, 0);
    Write16Absolute(memory, cpu, 0x4362u, cpu->x);
    StoreA8Absolute(memory, cpu, 0x4360u, 0x00u);
    StoreA8Absolute(memory, cpu, 0x4364u, 0x00u);
    StoreA8Absolute(memory, cpu, 0x4361u, 0x22u);
    StoreA8Absolute(memory, cpu, 0x420bu, 0x40u);
}

/* $83:A033: queued palette uploads, $73 bits 0-1. */
static void NmiPaletteUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fb3u);
    LoadA8(cpu, 0x01u);                                        /* A033 */
    TrbDirect8(memory, cpu, 0x73u);
    if (!cpu->zero) {
        LoadA8(cpu, 0x10u);
        LoadX16(cpu, 0x0340u);
        LoadY16(cpu, 0x00e0u);
        SimulateJsrFrame(memory, cpu, 0xa043u);
        NmiCgramUpload(memory, cpu);
        SimulateRtsFrame(memory, cpu);
    }
    LoadA8(cpu, 0x02u);                                        /* A044 */
    TrbDirect8(memory, cpu, 0x73u);
    if (!cpu->zero) {
        LoadA8(cpu, 0xf0u);
        LoadX16(cpu, 0x0500u);
        LoadY16(cpu, 0x0020u);
        NmiCgramUpload(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* One two-part VRAM tile upload of $A0E1. */
static void NmiTileBlock(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t list,
    uint16_t source,
    uint8_t vmain,
    uint8_t bank,
    uint8_t setup,
    uint16_t second_step) {
    const uint16_t entry = Read16AbsoluteIndexed(memory, cpu, list, cpu->x);

    LoadA16(cpu, entry);
    if (cpu->zero)
        return;
    Write16Absolute(memory, cpu, (uint16_t)(list + cpu->x), 0x0000u);
    LoadY8(cpu, vmain);
    StoreYIndex(memory, cpu, 0x2115u);
    And16(cpu, 0x1fffu);
    LsrA16(cpu);
    Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
    LoadY8(cpu, bank);
    StoreYIndex(memory, cpu, 0x4364u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, source, cpu->x));
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
    if (setup) {
        LoadY8(cpu, 0x01u);
        StoreYIndex(memory, cpu, 0x4360u);
        LoadY8(cpu, 0x18u);
        StoreYIndex(memory, cpu, 0x4361u);
    }
    LoadA16(cpu, 0x0040u);
    Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
    LoadY8(cpu, 0x40u);
    StoreYIndex(memory, cpu, 0x420bu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x0040u);
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
    if (second_step == 1u) {
        IncrementA16(cpu);
    } else {
        cpu->carry = 0;
        Add16Immediate(cpu, second_step);
    }
    Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
    LoadA16(cpu, 0x0040u);
    Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
    LoadY8(cpu, 0x40u);
    StoreYIndex(memory, cpu, 0x420bu);
}

/* $83:A0E1: queued VRAM tile uploads, four slots each list. */
static void NmiTileUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fb6u);
    SetAccumulatorWidth(cpu, 0);                               /* A0E1 */
    SetIndexWidth(cpu, 1);
    LoadY8(cpu, 0x80u);
    StoreYIndex(memory, cpu, 0x2115u);
    LoadY8(cpu, 0x01u);
    StoreYIndex(memory, cpu, 0x4360u);
    LoadY8(cpu, 0x18u);
    StoreYIndex(memory, cpu, 0x4361u);
    LoadX8(cpu, 0x06u);
    for (;;) {
        NmiTileBlock(memory, cpu, 0x1246u, 0x1236u, 0x81u, 0x7fu, 0, 1u);
        NmiTileBlock(memory, cpu, 0x123eu, 0x122eu, 0x80u, 0x7eu, 1,
            0x0020u);                                          /* A13E */
        LoadX8(cpu, (uint8_t)(cpu->x - 2u));                   /* A193 */
        if (cpu->negative)
            break;
    }
    LoadY8(cpu, 0x80u);                                        /* A19A */
    StoreYIndex(memory, cpu, 0x2115u);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A1A2: column uploads listed at $7F:D4F8. */
static void NmiColumnUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fbfu);
    TransferDirectToA(cpu);                                    /* A1A2 */
    LoadA8(cpu, Read8(memory, 0x7fd4f8u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    do {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd4f8u, cpu->x)));
        Write16Direct(memory, cpu, 0x33u, cpu->accumulator);   /* A1A8 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd518u, cpu->x)));
        Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd538u, cpu->x)));
        Write16Direct(memory, cpu, 0x37u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        StoreA8Absolute(memory, cpu, 0x4364u, 0x7eu);
        StoreA8Absolute(memory, cpu, 0x4360u, 0x01u);
        StoreA8Absolute(memory, cpu, 0x4361u, 0x18u);
        do {
            SetAccumulatorWidth(cpu, 0);                       /* A1CD */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0080u);
            And16(cpu, 0x07ffu);
            Write16Direct(memory, cpu, 0x39u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            And16(cpu, 0xf800u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator |
                Read16Direct(memory, cpu, 0x39u)));
            Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));    /* A1E6 */
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0040u);
            And16(cpu, 0x03ffu);
            Write16Direct(memory, cpu, 0x39u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            And16(cpu, 0xfc00u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator |
                Read16Direct(memory, cpu, 0x39u)));
            Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            LoadA16(cpu, 0x0080u);
            Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreA8Absolute(memory, cpu, 0x420bu, 0x40u);
            DecrementDirect8(memory, cpu, 0x37u);              /* A20A */
        } while (!cpu->zero);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));                 /* A20E */
    } while (!cpu->zero);
    TransferDirectToA(cpu);                                    /* A212 */
    Write8(memory, 0x7fd4f8u, A8(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A070: eight queued VRAM block uploads at $05C2. */
static void NmiBlockUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fc2u);
    SetAccumulatorWidth(cpu, 1);                               /* A070 */
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05c2u, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, 0x4362u, cpu->y);     /* A07C */
            Write16Direct(memory, cpu, 0x33u, cpu->y);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x05c2u, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x05c3u, cpu->x), 0x00u);
            LoadAAbsolute8(memory, cpu, 0x11d9u, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x4364u, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e9u, cpu->x));
            Write16Direct(memory, cpu, 0x35u, cpu->y);
            Write16Absolute(memory, cpu, 0x2116u, cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11f9u, cpu->x));
            Write16Direct(memory, cpu, 0x37u, cpu->y);
            Write16Absolute(memory, cpu, 0x4365u, cpu->y);
            StoreA8Absolute(memory, cpu, 0x4360u, 0x01u);
            StoreA8Absolute(memory, cpu, 0x4361u, 0x18u);
            StoreA8Absolute(memory, cpu, 0x420bu, 0x40u);
            SetAccumulatorWidth(cpu, 0);                       /* A0AC */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0100u);
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x37u));
            Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x33u));
            Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreA8Absolute(memory, cpu, 0x4360u, 0x01u);
            StoreA8Absolute(memory, cpu, 0x4361u, 0x18u);
            StoreA8Absolute(memory, cpu, 0x420bu, 0x40u);
        }
        IncrementX16(cpu);                                     /* A0D3 */
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->carry);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), 0x00u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0733u, 0), 0x00u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9FE1: colour window fade via $7F:D0F2/D0F3. */
static void NmiFade(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fc5u);
    LoadAAbsolute8(memory, cpu, 0x09aau, 0);                   /* 9FE1 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd0f2u));
        And8(cpu, 0xf0u);
        Or8(cpu, 0x0fu);
        StoreAAbsolute8(memory, cpu, 0x2106u, 0);
        LoadA8(cpu, Read8(memory, 0x7fd0f2u));
        And8(cpu, 0x0fu);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, 0x7fd0f3u));
        Write8(memory, 0x7fd0f3u, A8(cpu));
        BitImmediate8(cpu, 0x08u);                             /* A002 */
        if (!cpu->zero) {
            uint8_t finished;

            And8(cpu, 0x80u);
            Write8(memory, 0x7fd0f3u, A8(cpu));
            if (!cpu->negative) {
                LoadA8(cpu, Read8(memory, 0x7fd0f2u));         /* A00E */
                cpu->carry = 1;
                Sbc8(cpu, 0x10u);
                Write8(memory, 0x7fd0f2u, A8(cpu));
                finished = !cpu->carry;
            } else {
                LoadA8(cpu, Read8(memory, 0x7fd0f2u));         /* A01D */
                cpu->carry = 0;
                Adc8(cpu, 0x10u);
                Write8(memory, 0x7fd0f2u, A8(cpu));
                finished = cpu->carry;
            }
            if (finished) {
                LoadA8(cpu, 0x01u);                            /* A02A */
                TestBitsAbsolute8(memory, cpu, 0x09aau, 0);
            }
        }
        LoadAAbsolute8(memory, cpu, 0x09aau, 0);               /* A02F */
    }
    SimulateRtsFrame(memory, cpu);
}

Lufia2ActorPrimaryUpdateResult Lufia2FieldNmiUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    Push8(memory, cpu, PackStatus(cpu));                       /* 9FA9 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    NmiPaletteUploads(memory, cpu);
    NmiTileUploads(memory, cpu);
    LoadA8(cpu, Read8(memory, 0x7fd4f8u));                     /* 9FB7 */
    if (!cpu->zero)
        NmiColumnUploads(memory, cpu);
    NmiBlockUploads(memory, cpu);
    NmiFade(memory, cpu);
    LoadA8(cpu, Read8(memory, 0x7fd081u));                     /* 9FC6 */
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
    cpu->carry = 1;
    Adc8(cpu, 0x08u);
    if (cpu->negative)
        TransferDirectToA(cpu);                                /* 9FD1 */
    StoreAAbsolute8(memory, cpu, 0x2126u, 0);
    Adc8(cpu, 0xeeu);
    if (!cpu->negative)
        LoadA8(cpu, 0xffu);                                    /* 9FD9 */
    StoreAAbsolute8(memory, cpu, 0x2127u, 0);
    PullDataBank(memory, cpu);                                 /* 9FDE */
    UnpackStatus(cpu, Pull8(memory, cpu));
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x839fe0u;
    result.dispatches = 0;
    return result;
}

/* $8E:BF88: shift count from $8E:BF93 into $4E. */
static void ScrollShiftCount(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferAToX(cpu);                                         /* BF88 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ebf93u, cpu->x)));
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0x4eu, cpu->accumulator);
    SimulateRtsFrame(memory, cpu);
}

/* $8E:BF70: signed shift right by $4E; C=1 skips. */
static void ScrollShiftRight(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0x4eu)));    /* BF70 */
    if (cpu->zero) {
        TransferDirectToA(cpu);                                /* BF85 */
        cpu->carry = 0;
    } else if (cpu->negative) {
        cpu->carry = 1;                                        /* BF76 */
    } else {
        do {
            const uint16_t old = cpu->accumulator;             /* BF78 */

            SetNz16(cpu, old);
            LoadA16(cpu, (uint16_t)((old >> 1) | (old & 0x8000u)));
            cpu->carry = old & 1u;
            LoadY8(cpu, (uint8_t)(cpu->y - 1u));
        } while (!cpu->zero);
        cpu->carry = 0;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $8E:BFDE: shift left by $4E. */
static void ScrollShiftLeft(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0x4eu)));    /* BFDE */
    while (!cpu->zero) {
        /* ORA/CLC/SEC before ASL are dead. */
        AslA16(cpu);                                           /* BFE9 */
        LoadY8(cpu, (uint8_t)(cpu->y - 1u));
    }
    SimulateRtsFrame(memory, cpu);
}

/* A = base - value, 16-bit SBC after SEC. */
static void Subtract16(Lufia2ActorFrontendCpu *cpu, uint16_t value) {
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~value);
}

/* Step current toward the target by the speed word. */
static void ScrollStepToward(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t current,
    uint8_t out,
    uint32_t speed) {
    const uint8_t down = cpu->negative;

    LoadA16(cpu, Read16Direct(memory, cpu, current));
    if (down) {
        Subtract16(cpu, Read16Long(memory, speed));
    } else {
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, speed));
    }
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* $8E:BE78: layer follows the camera target. */
static void ScrollModeFollow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1261u, 0)); /* BE78 */
    cpu->zero = (cpu->accumulator & 0x0040u) == 0;
    if (!cpu->zero) {
        LoadXDirect(memory, cpu, 0x5du);                       /* BE80 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd0ceu, cpu->x)));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
        if (!cpu->zero)
            ScrollStepToward(memory, cpu, 0x54u, 0x58u,
                LongIndexedAddress(0x7fd0deu, cpu->x));
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd0d6u, cpu->x)));
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);   /* BEA2 */
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
        if (!cpu->zero)
            ScrollStepToward(memory, cpu, 0x56u, 0x5au,
                LongIndexedAddress(0x7fd0e6u, cpu->x));
        return;
    }
    LoadXDirect(memory, cpu, 0x5du);                           /* BEC4 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a8u, 0));
    if (cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    } else {
        const uint32_t speed = AbsoluteIndexedAddress(cpu, 0x05a8u, 0);

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);   /* BED7 */
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
        if (!cpu->zero) {
            ScrollStepToward(memory, cpu, 0x54u, 0x58u, speed);
        } else {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
            Write16Direct(memory, cpu, 0x5au, cpu->accumulator); /* BEF6 */
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
            if (!cpu->zero) {
                const uint8_t down = cpu->negative;

                ScrollStepToward(memory, cpu, 0x56u, 0x5au, speed);
                if (down)
                    return;                                    /* BF09 */
            }
        }
    }
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0)); /* BF12 */
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
    if (!cpu->zero)
        return;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
    if (!cpu->zero)
        return;
    Write16Absolute(memory, cpu, 0x05a8u, 0x0000u);
}

/* $8E:BF25/BF9B tail: camera + $80 unless equal. */
static void ScrollOffsetTarget(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t current,
    uint8_t out) {
    cpu->carry = 0;
    Add16Value(cpu, 0x0080u);
    Subtract16(cpu, Read16Direct(memory, cpu, current));
    if (cpu->zero)
        return;
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, current));
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* Layer nibble of $7F:D021,x. */
static void ScrollLayerNibble(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t high) {
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
    if (high) {
        And16(cpu, 0x00f0u);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
    } else {
        And16(cpu, 0x000fu);
    }
}

/* $8E:BF25: parallax, camera shifted right. */
static void ScrollModeParallax(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0x5du);                           /* BF25 */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbf34u);
    if (!cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        ScrollShiftRight(memory, cpu, 0xbf3cu);
        if (cpu->carry)
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        else
            ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    }
    LoadXDirect(memory, cpu, 0x5du);                           /* BF4D */
    ScrollLayerNibble(memory, cpu, 0);
    ScrollShiftCount(memory, cpu, 0xbf58u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
    ScrollShiftRight(memory, cpu, 0xbf5eu);
    if (cpu->carry)
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    else
        ScrollOffsetTarget(memory, cpu, 0x56u, 0x5au);
}

/* $8E:BF9B: camera shifted left. */
static void ScrollModeScaled(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0x5du);                           /* BF9B */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbfaau);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
    ScrollShiftLeft(memory, cpu, 0xbfb0u);
    ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    /* X still holds the high nibble. */
    ScrollLayerNibble(memory, cpu, 0);                         /* BFBF */
    ScrollShiftCount(memory, cpu, 0xbfc8u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
    ScrollShiftLeft(memory, cpu, 0xbfceu);
    ScrollOffsetTarget(memory, cpu, 0x56u, 0x5au);
}

/* One axis of $8E:BFEE: offset, wrap at map size. */
static void ScrollWrapAxis(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t current,
    uint8_t out,
    uint32_t size) {
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ec04du, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, current));
    And16(cpu, 0x7fffu);
    Write16Direct(memory, cpu, out, cpu->accumulator);
    LoadXDirect(memory, cpu, 0x5du);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(size, cpu->x)));
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, out));
    if (cpu->carry)
        return;
    Subtract16(cpu, Read16Direct(memory, cpu, out));
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* $8E:BFEE: fixed offsets, wrapped by map size. */
static void ScrollModeWrap(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0x5du);                           /* BFEE */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
    And16(cpu, 0x00f0u);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x54u, 0x58u, 0x7fd010u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
    And16(cpu, 0x000fu);                                       /* C01F */
    AslA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x56u, 0x5au, 0x7fd018u);
}

/* [dp],y address: 24-bit pointer plus Y. */
static uint32_t DirectLongIndirectY(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint32_t pointer =
        Read16Direct(memory, cpu, offset) |
        ((uint32_t)Read8(memory, DirectAddress(cpu, (uint8_t)(offset + 2u)))
            << 16);
    return (pointer + cpu->y) & 0x00ffffffu;
}

static void Decrement16Direct(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, offset) - 1u);
    Write16Direct(memory, cpu, offset, value);
    SetNz16(cpu, value);
}

/* $80:F81C: divider settle delay; leaves M=0. */
static void StreamDelay(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x00u)));    /* F81C */
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* A mod divisor via $4204/$4206; negative A wraps. */
static void StreamWrap(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t divisor,
    uint16_t negative_return,
    uint16_t positive_return,
    uint8_t short_cut) {
    cpu->zero = (cpu->accumulator & 0x0800u) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator | 0xf000u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        IncrementA16(cpu);
        Write16Long(memory, 0x004204u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
        Write8(memory, 0x004206u, A8(cpu));
        StreamDelay(memory, cpu, negative_return);
        LoadA16(cpu, Read16Direct(memory, cpu, divisor));
        Subtract16(cpu, Read16Long(memory, 0x004216u));
        return;
    }
    if (short_cut) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, divisor));
        if (!cpu->carry)
            return;
    }
    Write16Long(memory, 0x004204u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
    Write8(memory, 0x004206u, A8(cpu));
    StreamDelay(memory, cpu, positive_return);
    LoadA16(cpu, Read16Long(memory, 0x004216u));
}

/* $80:F734: map cell and buffer offsets for A=x, Y=y. */
static void StreamLocate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LsrA16(cpu);                                               /* F734 */
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Write16Direct(memory, cpu, 0x30u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd010u, cpu->x));
    Write16Direct(memory, cpu, 0x83u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd018u, cpu->x));
    Write16Direct(memory, cpu, 0x85u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
    StreamWrap(memory, cpu, 0x83u, 0xf762u, 0xf77au, 0);
    Write16Direct(memory, cpu, 0x87u, cpu->accumulator);       /* F77F */
    Write16Direct(memory, cpu, 0x26u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x30u));
    AslA16(cpu);
    AslA16(cpu);
    And16(cpu, 0x003eu);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    StreamWrap(memory, cpu, 0x85u, 0xf7adu, 0xf7c9u, 1);
    Write16Direct(memory, cpu, 0x89u, cpu->accumulator);       /* F7CE */
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x00f0u);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Write16Direct(memory, cpu, 0x24u, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x22u));
    Write16Direct(memory, cpu, 0x2du, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x838ff0u, cpu->x)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x28u)));
    Write8(memory, 0x004202u, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x83u)));
    Write8(memory, 0x004203u, A8(cpu));
    LoadA8(cpu, 0x7eu);
    Write8(memory, DirectAddress(cpu, 0x2cu), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, cpu->x));
    Write16Direct(memory, cpu, 0x8du, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, 0x004216u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x26u));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x8du));
    Write16Direct(memory, cpu, 0x30u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd03cu, 0));
    Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
    Compare16(cpu, cpu->x, 0x0004u);
    if (cpu->carry) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd040u, 0));
        Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
    }
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
}

/* $80:F6AA: X = map cell for ($87, $89). */
static void StreamCellIndex(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* F6AA */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x89u)));
    Write8(memory, 0x004202u, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x83u)));
    Write8(memory, 0x004203u, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x87u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x004216u));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x8du));
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* One 16x16 metatile from cell X into [$2A],y. */
static void StreamMetatile(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, 0x3000u);
    Compare16(cpu, cpu->accumulator, 0x3000u);
    if (cpu->zero)
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, 0x03ffu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x65u));
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, 0x003eu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
}

/* Advance a wrapped map coordinate; refresh X on wrap. */
static void StreamStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t coordinate,
    uint8_t limit,
    uint16_t return_address) {
    LoadA16(cpu, Read16Direct(memory, cpu, coordinate));
    IncrementA16(cpu);
    Write16Direct(memory, cpu, coordinate, cpu->accumulator);
    if (!cpu->zero) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, limit));
        if (!cpu->carry)
            return;
        Write16Direct(memory, cpu, coordinate, 0x0000u);
    }
    StreamCellIndex(memory, cpu, return_address);
}

/* $80:F5ED: 16 metatiles down a column. */
static void StreamColumnTiles(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA16(cpu, 0x0010u);                                     /* F5ED */
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    do {
        PushIndex(memory, cpu);                                /* F5F2 */
        StreamMetatile(memory, cpu);
        LoadA16(cpu, cpu->y);                                  /* F62B */
        cpu->carry = 0;
        Add16Value(cpu, 0x003eu);
        And16(cpu, 0x07ffu);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x8bu));
        TransferAToX(cpu);
        StreamStep(memory, cpu, 0x89u, 0x85u, 0xf648u);
        Decrement16Direct(memory, cpu, 0x28u);                 /* F649 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $80:F64E: A metatiles along a row. */
static void StreamRowTiles(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, 0x26u, cpu->accumulator);       /* F64E */
    do {
        PushIndex(memory, cpu);                                /* F650 */
        StreamMetatile(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);                  /* F689 */
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, cpu->y);
        Subtract16(cpu, 0x003eu);
        And16(cpu, 0x07ffu);
        TransferAToY(cpu);
        StreamStep(memory, cpu, 0x87u, 0x83u, 0xf6a4u);
        Decrement16Direct(memory, cpu, 0x26u);                 /* F6A5 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* PHP; PHB; DB=$7F; REP #$30. */
static void StreamEnter(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadXDirect(memory, cpu, 0x5du);
}

/* $80:F4FD/F518: stream a column at the right/left edge. */
static void StreamColumn(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t right) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    if (right) {
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
    }
    StreamLocate(memory, cpu, 0xf52fu);                        /* F52D */
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x83u));
    AslA16(cpu);
    Write16Direct(memory, cpu, 0x8bu, cpu->accumulator);
    LoadXDirect(memory, cpu, 0x30u);
    LoadYDirect16(memory, cpu, 0x2du);
    StreamColumnTiles(memory, cpu, 0xf53cu);
    cpu->x = PullIndexValue(memory, cpu);                      /* F53D */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
    TransferAToY(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Direct(memory, cpu, 0x2du, cpu->accumulator);
    StoreXDirect16(memory, cpu, 0x30u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f581u, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x001236u, cpu->x), cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, 0x0020u);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    do {
        LoadA16(cpu, Read16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
            cpu->accumulator);
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadA16(cpu, Read16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
            cpu->accumulator);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, 0x003eu);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x22u);
    } while (!cpu->zero);
    LoadXDirect(memory, cpu, 0x30u);                           /* F56E */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2du));
    Write16Long(memory, LongIndexedAddress(0x001246u, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f581u, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x001236u, cpu->x), cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $80:F589/F5A2: stream a row at the top/bottom edge. */
static void StreamRow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t down) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
    if (down) {
        cpu->carry = 0;
        Add16Value(cpu, 0x00f0u);
    } else {
        Subtract16(cpu, 0x0010u);
    }
    And16(cpu, 0xfff0u);
    TransferAToY(cpu);                                         /* F5B9 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    StreamLocate(memory, cpu, 0xf5c0u);
    LoadA16(cpu, 0x0040u);                                     /* F5C1 */
    Subtract16(cpu, Read16Direct(memory, cpu, 0x22u));
    LsrA16(cpu);
    LsrA16(cpu);
    LoadXDirect(memory, cpu, 0x30u);
    LoadYDirect16(memory, cpu, 0x2du);
    StreamRowTiles(memory, cpu, 0xf5cfu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));            /* F5D0 */
    LsrA16(cpu);
    LsrA16(cpu);
    if (!cpu->zero) {
        /* X continues from the first run. */
        LoadYDirect16(memory, cpu, 0x24u);
        StreamRowTiles(memory, cpu, 0xf5dau);
    }
    LoadA16(cpu, Read16Direct(memory, cpu, 0x24u));            /* F5DB */
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x2au));
    LoadXDirect(memory, cpu, 0x5du);
    Write16Long(memory, LongIndexedAddress(0x00122eu, cpu->x), cpu->accumulator);
    Write16Long(memory, LongIndexedAddress(0x00123eu, cpu->x), cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* JSL from $8E:BD77 into a $80 tile streamer. */
static void ScrollStream(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address,
    uint16_t entry) {
    SimulateJslFrame(memory, cpu, 0x8eu, return_address);
    switch (entry) {
    case 0xf4fdu: StreamColumn(memory, cpu, 1); break;
    case 0xf518u: StreamColumn(memory, cpu, 0); break;
    case 0xf589u: StreamRow(memory, cpu, 0); break;
    default: StreamRow(memory, cpu, 1); break;
    }
    SimulateRtlFrame(memory, cpu);
}

/* JSR ($BE6E,x); 0 for an unknown mode. */
static uint8_t ScrollLayerMode(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->x > 8u)
        return 0;
    SimulateJsrFrame(memory, cpu, 0xbdd9u);
    switch (cpu->x) {
    case 0: ScrollModeFollow(memory, cpu); break;
    case 2: break;                                             /* BF24 */
    case 4: ScrollModeParallax(memory, cpu); break;
    case 6: ScrollModeWrap(memory, cpu); break;
    default: ScrollModeScaled(memory, cpu); break;
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* dispatches counts completed BDD7 layer calls. */
static Lufia2ActorPrimaryUpdateResult ScrollBoundary(
    Lufia2ActorPrimaryUpdateResult result,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t pc) {
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc = pc;
    return result;
}

Lufia2ActorPrimaryUpdateResult Lufia2FieldScrollUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x8ebe6du;
    result.dispatches = 0;
    /* The field loop always calls with X8. */
    if (!cpu->index_is_8_bit)
        return ScrollBoundary(result, cpu, 0x8ebd77u);
    LoadAAbsolute8(memory, cpu, 0x1261u, 0);                   /* BD77 */
    BitImmediate8(cpu, 0x08u);
    SetAccumulatorWidth(cpu, 0);
    if (!cpu->zero) {
        LoadA16(cpu, Read16Long(memory, 0x7fd08bu));
        Write16Absolute(memory, cpu, 0x05a4u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fd08du));
        Write16Absolute(memory, cpu, 0x05a6u, cpu->accumulator);
    } else {
        LoadX8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0734u, 0)));
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        Subtract16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, 0x05a4u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        Subtract16(cpu, 0x0070u);
        Write16Absolute(memory, cpu, 0x05a6u, cpu->accumulator);
    }
    SetAccumulatorWidth(cpu, 1);                               /* BDA9 */
    LoadX8(cpu, 0x00u);
    do {
        TransferDirectToA(cpu);                                /* BDAD */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd020u, cpu->x)));
        if (!cpu->negative) {
            SetAccumulatorWidth(cpu, 0);                       /* BDB7 */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x121eu, cpu->x));
            Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1226u, cpu->x));
            Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            PushIndex(memory, cpu);
            Write8(memory, DirectAddress(cpu, 0x5du), (uint8_t)cpu->x);
            Write8(memory, DirectAddress(cpu, 0x5eu), 0x00u);
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd020u, cpu->x)));
            AslA8(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 0);
            if (!ScrollLayerMode(memory, cpu))
                return ScrollBoundary(result, cpu, 0x8ebdd7u);
            ++result.dispatches;
            LoadXDirect(memory, cpu, 0x5du);                   /* BDDA */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x121eu, cpu->x));
            And16(cpu, 0x000fu);
            if (cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
                Subtract16(cpu, Read16Direct(memory, cpu, 0x54u));
                if (!cpu->zero) {
                    uint8_t right = 1;

                    if (cpu->negative) {
                        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
                        Compare16(cpu, cpu->accumulator, 0x0100u);
                        right = cpu->carry;
                    }
                    if (right)
                        ScrollStream(memory, cpu, 0xbdfeu, 0xf4fdu); /* BDFB */
                    else
                        ScrollStream(memory, cpu, 0xbdf8u, 0xf518u); /* BDF5 */
                }
            }
            LoadXDirect(memory, cpu, 0x5du);                   /* BDFF */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x121eu + cpu->x), cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0008u);
            PushAccumulator16(memory, cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4edu, cpu->x)));
            TransferAToY(cpu);
            PullAccumulator16(memory, cpu);
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, 0x7fd081u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x0594u + cpu->y), cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));    /* BE19 */
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
            if (!cpu->zero) {
                const uint16_t from = Read16Direct(memory, cpu, 0x56u);

                if (cpu->negative) {
                    LoadA16(cpu, from);                        /* BE21 */
                    LoadA16(cpu, (uint16_t)(from ^ Read16Direct(memory, cpu, 0x5au)));
                    And16(cpu, 0x0010u);
                    if (!cpu->zero)
                        ScrollStream(memory, cpu, 0xbe2du, 0xf589u); /* BE2A */
                } else {
                    uint8_t stream;

                    LoadA16(cpu, from);                        /* BE30 */
                    And16(cpu, 0x000fu);
                    stream = cpu->zero;
                    if (!stream) {
                        LoadA16(cpu, from);
                        LoadA16(cpu, (uint16_t)(from ^ Read16Direct(memory, cpu, 0x5au)));
                        And16(cpu, 0x0010u);
                        if (!cpu->zero) {
                            LoadA16(cpu, from);
                            And16(cpu, 0x0001u);
                            stream = !cpu->zero;
                        }
                    }
                    if (stream)
                        ScrollStream(memory, cpu, 0xbe4au, 0xf5a2u); /* BE47 */
                }
            }
            LoadXDirect(memory, cpu, 0x5du);                   /* BE4B */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x1226u + cpu->x), cpu->accumulator);
            PushAccumulator16(memory, cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4f5u, cpu->x)));
            TransferAToY(cpu);
            PullAccumulator16(memory, cpu);
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, 0x7fd083u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x0596u + cpu->y), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            cpu->x = PullIndexValue(memory, cpu);              /* BE63 */
        }
        LoadX8(cpu, (uint8_t)(cpu->x + 2u));                   /* BE64 */
        Compare8(cpu, (uint8_t)cpu->x, 0x06u);
    } while (!cpu->zero);
    return result;
}

/* $83:80CD from the field loop's JSR (X8 only). */
Lufia2ActorPrimaryUpdateResult Lufia2FieldIdleTest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x838102u;
    result.dispatches = 0;
    if (!cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8380cdu;
        return result;
    }
    FieldIdleBody(memory, cpu);
    return result;
}

/* $83:8682: tick the eight animation slots at $7F:D057. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldAnimationTicks(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint32_t slots = 0x7fd057u;
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x8386e9u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* 8682 */
    SetIndexWidth(cpu, 0);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x17aau, 0), 0x00u);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, 0x17abu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
        if (cpu->negative) {
            ExchangeAccumulatorBytes(cpu);                     /* 8696 */
            LoadAAbsolute8(memory, cpu, 0x17abu, 0);
            TestBitsAbsolute8(memory, cpu, 0x17aau, 1);
            AslA8(cpu);
            StoreAAbsolute8(memory, cpu, 0x17abu, 0);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
            And8(cpu, 0x18u);
            Compare8(cpu, A8(cpu), 0x18u);
            if (cpu->zero) {
                LoadA8(cpu, Read8(memory, LongIndexedAddress(slots, cpu->x)));
                And8(cpu, 0xe7u);                              /* 86B3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                PushIndex(memory, cpu);
                Write8(memory, 0x7fd049u, A8(cpu));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                BitImmediate8(cpu, 0x02u);
                if (!cpu->zero)
                    TransferDirectToA(cpu);                    /* 86C3 */
                Write8(memory, LongIndexedAddress(slots, cpu->x), A8(cpu));
                LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd04fu, cpu->x)));
                Write8(memory, 0x7fd04eu, A8(cpu));
                LoadA8(cpu, Read8(memory, 0x7fd04eu));
                /* The slot handlers $83:8783/87CC stay LLE. */
                result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
                result.pc = cpu->resume_pc =
                    cpu->negative ? 0x8386d6u : 0x8386dbu;
                return result;
            }
        }
        IncrementX16(cpu);                                     /* 86DF */
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 86E8 */
    return result;
}

/* $83:AEED: palette cycles from bank $A1; 0 = cap hit. */
static uint8_t FieldPaletteCycles(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t *visits) {

    SimulateJsrFrame(memory, cpu, 0xaee6u);
    LoadA8(cpu, Read8(memory, 0x0009a9u));                     /* AEED */
    BitImmediate8(cpu, 0x06u);
    if (!cpu->zero) {
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    LoadA8(cpu, Read8(memory, 0x7fd0f7u));
    if (cpu->zero) {
        SimulateRtsFrame(memory, cpu);
        return 1;
    }
    Write8(memory, DirectAddress(cpu, 0x58u), A8(cpu));
    LoadX16(cpu, 0x0000u);
    do {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0xed01u, cpu->x);
        const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

        Write8(memory, timer, left);                           /* AF00 */
        SetNz8(cpu, left);
        if (!cpu->zero)
            goto next;
        SetAccumulatorWidth(cpu, 0);                           /* AF05 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xec00u, cpu->x));
        IncrementA16(cpu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xec00u, cpu->x),
            cpu->accumulator);
        cpu->y = cpu->x;                                       /* TXY */
        SetNz16(cpu, cpu->y);
        for (;;) {
            if (*visits >= 0x10000u) {
                /* Zero-length frames can chain forever. */
                cpu->resume_pc = 0x83af11u;
                return 0;
            }
            ++*visits;
            TransferAToX(cpu);                                 /* AF11 */
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0xa10000u, cpu->x)));
            StoreAAbsolute8(memory, cpu, 0xed01u, cpu->y);
            SetAccumulatorWidth(cpu, 0);
            if (!cpu->zero)
                break;
            LoadA16(cpu, cpu->y);                              /* AF1F */
            cpu->carry = 0;
            Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd0f5u, 0));
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa10003u, cpu->x)));
            cpu->carry = 1;
            Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd0f5u, 0));
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xec00u, cpu->y),
                cpu->accumulator);
        }
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa10001u, cpu->x)));
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);   /* AF36 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xed00u, cpu->y));
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        Write16Long(memory, LongIndexedAddress(0x000320u, cpu->x),
            cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        cpu->x = cpu->y;                                       /* TYX */
        SetNz16(cpu, cpu->x);
        LoadA8(cpu, 0x01u);
        {
            const uint32_t flags = DirectAddress(cpu, 0x73u);  /* TSB $73 */
            const uint8_t value = Read8(memory, flags);

            cpu->zero = (value & 0x01u) == 0;
            Write8(memory, flags, (uint8_t)(value | 0x01u));
        }
next:
        IncrementX16(cpu);                                     /* AF4D */
        IncrementX16(cpu);
        DecrementDirect8(memory, cpu, 0x58u);
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $83:AF54: step the HDMA wave table; set up channel 1. */
static void FieldWaveTable(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xaee9u);
    LoadAAbsolute8(memory, cpu, 0xd0cau, 0);                   /* AF54 */
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero)
        goto done;
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c9u, 0);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, 0x7fd0c8u, A8(cpu));
    Compare8(cpu, A8(cpu), Read8(memory, LongIndexedAddress(0x7e0001u, cpu->x)));
    if (!cpu->zero)
        goto done;
    TransferDirectToA(cpu);                                    /* AF6E */
    StoreAAbsolute8(memory, cpu, 0xd0c8u, 0);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, 0x55u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x54u), 0x00u);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, 0x7fd0c6u));
    Write16Long(memory, 0x004312u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    IncrementX16(cpu);                                         /* AF99 */
    IncrementX16(cpu);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xd0c9u, 0), cpu->x);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7e0000u, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero) {
        LoadX16(cpu, 0xc002u);                                 /* AFA6 */
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0xd0c9u, 0), cpu->x);
    }
    Push8(memory, cpu, 0x83u);                                 /* AFAC */
    PullDataBank(memory, cpu);
    LoadA8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, 0x4310u, 0);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, 0x4314u, 0);
    LoadX16(cpu, 0x01e0u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x4315u, 0), cpu->x);
    LoadA8(cpu, 0x18u);
    StoreAAbsolute8(memory, cpu, 0x4311u, 0);
    LoadX16(cpu, 0x4010u);
    StoreXDirect16(memory, cpu, 0x7bu);
    LoadA8(cpu, 0x42u);
    Write8(memory, DirectAddress(cpu, 0x76u), A8(cpu));
done:
    SimulateRtsFrame(memory, cpu);
}

/* $83:AEB5: per-frame field palette and wave effects. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldColourEffects(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = 0x83aeecu;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* AEB5 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, Read8(memory, 0x0009a9u));
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1280u, 0);
        const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

        Write8(memory, timer, left);                           /* AEC2 */
        SetNz8(cpu, left);
        if (cpu->zero) {
            /* $84:8E07 stays LLE. */
            result.pc = cpu->resume_pc = 0x83aec7u;
            return result;
        }
    }
    BitImmediate8(cpu, 0x04u);                                 /* AECF */
    if (!cpu->zero) {
        /* $84:8D54 stays LLE. */
        result.pc = cpu->resume_pc = 0x83aed3u;
        return result;
    }
    BitImmediate8(cpu, 0x01u);                                 /* AEDB */
    if (!cpu->zero) {
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        if (!FieldPaletteCycles(memory, cpu, &result.dispatches)) {
            result.pc = cpu->resume_pc;
            return result;
        }
        FieldWaveTable(memory, cpu);
        PullDataBank(memory, cpu);                             /* AEEA */
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* AEEB */
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    return result;
}

static Lufia2ActorPrimaryUpdateResult FieldTickBoundary(
    Lufia2ActorPrimaryUpdateResult result,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t pc) {
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc = pc;
    return result;
}

/* $80:9C72: per-frame screen effects, event timer, text gate. */
static Lufia2ActorPrimaryUpdateResult TextEngineStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result);

static Lufia2ActorPrimaryUpdateResult TextPromptTick(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result);

Lufia2ActorPrimaryUpdateResult Lufia2FieldEventTick(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint8_t effects[5] = {0x04u, 0x02u, 0x01u, 0x30u, 0x80u};
    Lufia2ActorPrimaryUpdateResult result;
    unsigned i;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x809cb7u;
    result.dispatches = 0;
    /* Active effects run $84:8000 on LLE. */
    if ((Read8(memory, AbsoluteIndexedAddress(cpu, 0x1261u, 0)) & 0xb7u) ||
        (Read8(memory, AbsoluteIndexedAddress(cpu, 0x1262u, 0)) & 0x01u))
        return FieldTickBoundary(result, cpu, 0x809c72u);
    SimulateJslFrame(memory, cpu, 0x80u, 0x9c75u);             /* 9C72 */
    LoadAAbsolute8(memory, cpu, 0x1261u, 0);                   /* $84:8000 */
    for (i = 0; i < 5u; ++i)
        BitImmediate8(cpu, effects[i]);
    LoadAAbsolute8(memory, cpu, 0x1262u, 0);                   /* $84:809B */
    BitImmediate8(cpu, 0x01u);
    SimulateRtlFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x9c78u);                    /* 9C76 */
    LoadA8(cpu, Read8(memory, 0x7fd0c1u));                     /* C21A */
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
        Write8(memory, 0x7fd0c1u, A8(cpu));
        if (cpu->zero)
            return FieldTickBoundary(result, cpu, 0x80c229u);
    }
    SimulateRtsFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9C79 */
    And8(cpu, 0x0au);
    if (!cpu->zero)
        return TextPromptTick(memory, cpu, result);
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9CB2 */
    if (cpu->negative)
        return TextEngineStep(memory, cpu, result);
    return result;
}

/* $85:8E98: sixteen queued VRAM DMA uploads on channel 6. */
static void BattleVramQueue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8dccu);
    LoadX16(cpu, 0x005au);                                     /* 8E98 */
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, 0x4365u, cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a91u, cpu->x));
            Write16Absolute(memory, cpu, 0x4362u, cpu->y);
            LoadA8(cpu, 0x01u);
            StoreAAbsolute8(memory, cpu, 0x4360u, 0);
            LoadA8(cpu, 0x7eu);
            StoreAAbsolute8(memory, cpu, 0x4364u, 0);
            LoadA8(cpu, 0x18u);
            StoreAAbsolute8(memory, cpu, 0x4361u, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a93u, cpu->x));
            Write16Absolute(memory, cpu, 0x2116u, cpu->y);
            LoadA8(cpu, 0x40u);
            StoreAAbsolute8(memory, cpu, 0x420bu, 0);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a8fu, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a90u, cpu->x), 0x00u);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 6u));                 /* 8EC9 */
    } while (!cpu->negative);
    SimulateRtsFrame(memory, cpu);
}

/* $85:8ED2: latch HDMA channels from $1AEF, enable $420C. */
static void BattleHdmaChannels(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8e24u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdau)));    /* 8ED2 */
    Write8(memory, DirectAddress(cpu, 0xdau), 0x00u);
    {
        const uint32_t d9 = DirectAddress(cpu, 0xd9u);
        const uint8_t value = Read8(memory, d9);

        cpu->zero = (value & A8(cpu)) == 0;                    /* TSB $D9 */
        Write8(memory, d9, (uint8_t)(value | A8(cpu)));
    }
    Or8(cpu, Read8(memory, DirectAddress(cpu, 0xd8u)));
    Or8(cpu, 0x40u);
    And8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));
    Write8(memory, DirectAddress(cpu, 0xd8u), A8(cpu));
    if (!cpu->zero) {
        LoadY16(cpu, 0x4300u);
        LoadX16(cpu, 0x1aefu);
        for (;;) {
            const uint32_t d8 = DirectAddress(cpu, 0xd8u);     /* 8EE8 */
            const uint8_t mask = Read8(memory, d8);

            cpu->carry = mask & 1u;
            Write8(memory, d8, (uint8_t)(mask >> 1));
            SetNz8(cpu, (uint8_t)(mask >> 1));
            if (cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);  /* 8EF2 */
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y),
                    cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0003u, cpu->y),
                    cpu->accumulator);
            } else if (cpu->zero) {
                break;
            } else {
                SetAccumulatorWidth(cpu, 0);
            }
            LoadA16(cpu, cpu->x);                              /* 8F06 */
            cpu->carry = 0;
            Add16Value(cpu, 0x0005u);
            TransferAToX(cpu);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0010u);
            TransferAToY(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));    /* 8F15 */
    StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $85:A8E7: timer 1, battle HDMA wave table at $7E:40CC. */
static void BattleWaveTable(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x1b23u, 0);                   /* A8E7 */
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1b22u, 0);
        PushIndex(memory, cpu);
        SetIndexWidth(cpu, 1);
        TransferAToX(cpu);
        And8(cpu, 0x03u);
        Write8(memory, DirectAddress(cpu, 0x33u), A8(cpu));
        LoadA8(cpu, 0x04u);
        cpu->carry = 1;
        Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x33u)));
        Write8(memory, DirectAddress(cpu, 0x33u), A8(cpu));
        AslA8(cpu);
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x33u)));
        TransferAToY(cpu);
        TransferXToA(cpu);
        AslA8(cpu);
        cpu->carry = 0;
        Adc8(cpu, 0x08u);
        TransferAToX(cpu);
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859ffau, cpu->x)));
        LoadX8(cpu, 0x00u);
        do {
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x40ccu, cpu->x),
                cpu->accumulator);                             /* A915 */
            LoadY8(cpu, (uint8_t)(cpu->y - 1u));
            if (cpu->zero) {
                LoadY8(cpu, 0x0cu);
                cpu->carry = 0;
                Add16Value(cpu, 0x0004u);
            }
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));               /* A921 */
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));
            Compare8(cpu, (uint8_t)cpu->x, 0x90u);
        } while (!cpu->zero);
        PullDataBank(memory, cpu);                             /* A927 */
        SetAccumulatorWidth(cpu, 1);
        SetIndexWidth(cpu, 0);
        cpu->x = PullIndexValue(memory, cpu);
    } else {
        static const uint16_t fills[3][2] = {
            {0x015fu, 0x0008u}, {0x015bu, 0x0038u}, {0x011fu, 0x0008u}};
        unsigned band;

        PushIndex(memory, cpu);                                /* A933 */
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadX16(cpu, 0x0000u);
        SetAccumulatorWidth(cpu, 0);
        for (band = 0; band < 3u; ++band) {
            LoadA16(cpu, fills[band][0]);
            LoadY16(cpu, fills[band][1]);
            do {
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x40ccu, cpu->x),
                    cpu->accumulator);
                IncrementX16(cpu);
                IncrementX16(cpu);
                LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            } while (!cpu->zero);
        }
        SetAccumulatorWidth(cpu, 1);                           /* A968 */
        PullDataBank(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
    }
    LoadA8(cpu, 0x01u);                                        /* A92D */
    StoreAAbsolute8(memory, cpu, 0x1b20u, 0);
}

/* $85:8F1B: eight battle timers; 0 = handler left to LLE. */
static uint8_t BattleTimers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8e78u);
    SetIndexWidth(cpu, 1);                                     /* 8F1B */
    LoadX8(cpu, 0x00u);
    do {
        LoadAAbsolute8(memory, cpu, 0x1b17u, cpu->x);
        if (!cpu->zero) {
            const uint32_t counter =
                AbsoluteIndexedAddress(cpu, 0x1b18u, cpu->x);
            const uint8_t left = (uint8_t)(Read8(memory, counter) - 1u);

            Write8(memory, counter, left);
            SetNz8(cpu, left);
            if (cpu->zero) {
                uint16_t handler;

                LoadAAbsolute8(memory, cpu, 0x1b19u, cpu->x);  /* 8F29 */
                AslA8(cpu);
                TransferAToY(cpu);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x9e05u, cpu->y));
                handler = cpu->accumulator;
                Push8(memory, cpu, 0x8fu);                     /* PEA #$8F3B */
                Push8(memory, cpu, 0x3bu);
                PushAccumulator16(memory, cpu);
                SetAccumulatorWidth(cpu, 1);
                SetIndexWidth(cpu, 0);
                (void)Pull8(memory, cpu);                      /* RTS */
                (void)Pull8(memory, cpu);
                if (handler != 0xa8e6u) {
                    /* Other handlers run on LLE after RTS. */
                    cpu->resume_pc = 0x850000u | (uint16_t)(handler + 1u);
                    return 0;
                }
                BattleWaveTable(memory, cpu);
                SimulateRtsFrame(memory, cpu);                 /* to 8F3C */
                SetIndexWidth(cpu, 1);
            }
        }
        LoadA8(cpu, (uint8_t)cpu->x);                          /* 8F3E */
        cpu->carry = 0;
        Adc8(cpu, 0x08u);
        TransferAToX(cpu);
        Compare8(cpu, (uint8_t)cpu->x, 0x40u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 0);                                     /* 8F47 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $85:8DC5: battle NMI work via the $00:0067 vector. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleNmiUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t scroll_regs[6] = {
        0x210du, 0x210eu, 0x210fu, 0x2110u, 0x2111u, 0x2112u};
    static const uint16_t window_regs[8] = {
        0x2123u, 0x2125u, 0x2127u, 0x2129u, 0x212bu, 0x212du, 0x212fu,
        0x2131u};
    Lufia2ActorPrimaryUpdateResult result;
    unsigned i;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x858e97u;
    result.dispatches = 0;
    /* The NMI handler always enters with M=1. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858dc5u;
        return result;
    }
    PushDataBank(memory, cpu);                                 /* 8DC5 */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);
    BattleVramQueue(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* 8DCD */
    TestBitsAbsolute8(memory, cpu, 0x15b3u, 0);
    if (!cpu->zero) {
        for (i = 0; i < 12u; ++i) {
            LoadAAbsolute8(memory, cpu, (uint16_t)(0x0594u + i), 0);
            StoreAAbsolute8(memory, cpu, scroll_regs[i >> 1], 0);
        }
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x059au, 0));
        Write16Absolute(memory, cpu, 0x1256u, cpu->x);
    }
    BattleHdmaChannels(memory, cpu);                           /* 8E22 */
    for (i = 0; i < 8u; ++i) {
        LoadX16(cpu, Read16AbsoluteIndexed(
            memory, cpu, (uint16_t)(0x123cu + 2u * i), 0));
        Write16Absolute(memory, cpu, window_regs[i], cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1268u, 0);                   /* 8E55 */
    if (cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1249u, 0));
        Write16Absolute(memory, cpu, 0x1252u, cpu->x);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1245u, 0));
        Write16Absolute(memory, cpu, 0x1250u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1254u, 0);                   /* 8E66 */
    StoreAAbsolute8(memory, cpu, 0x1255u, 0);
    LoadAAbsolute8(memory, cpu, 0x0583u, 0);
    StoreAAbsolute8(memory, cpu, 0x2100u, 0);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdbu)));
    if (!cpu->zero && !BattleTimers(memory, cpu)) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* 8E79 */
    LoadAAbsolute8(memory, cpu, 0x12e3u, cpu->y);
    if (!cpu->negative) {
        /* Queued tasks run through JSR ($9E37,x) on LLE. */
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858e7fu;
        return result;
    }
    PullDataBank(memory, cpu);                                 /* 8E96 */
    return result;
}

static void RolA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t old = A8(cpu);
    const uint8_t value = (uint8_t)((old << 1) | (cpu->carry ? 1u : 0u));
    cpu->carry = (old & 0x80u) != 0;
    LoadA8(cpu, value);
}

static void BattleSetDataBank(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t bank) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

static uint8_t DirectByte(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    return Read8(memory, DirectAddress(cpu, offset));
}

static void StoreADirect8(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    Write8(memory, DirectAddress(cpu, offset), A8(cpu));
}

static uint8_t AbsoluteByte(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    return Read8(memory, AbsoluteIndexedAddress(cpu, address, index));
}

static void StoreZeroAbsolute8(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, index), 0x00u);
}

/* Y += step with M=0, back to M=1. */
static void BattleNextRecord(Lufia2ActorFrontendCpu *cpu, uint16_t step) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, step);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:BD4B, $81:BDCC: OAM strips, five bytes per sprite. */
static void BattleOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    if (mirrored) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BDD1 */
        DecrementA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        Adc8(cpu, DirectByte(memory, cpu, 0x06u));
        StoreADirect8(memory, cpu, 0x06u);
    }
    Write8(memory, DirectAddress(cpu, 0x15u), 0x00u);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x07u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x06u));
    TransferAToY(cpu);
    StoreYDirect16(memory, cpu, 0x17u);
    StoreYDirect16(memory, cpu, 0x1cu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
    DecrementA8(cpu);
    StoreADirect8(memory, cpu, 0x16u);
    LoadXDirect16(memory, cpu, 0x08u);
    do {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BD70 */
        StoreADirect8(memory, cpu, 0x13u);
        do {
            static const uint8_t fields[4] = {0x17u, 0x16u, 0x11u, 0x04u};
            unsigned i;

            for (i = 0; i < 4u; ++i) {
                LoadA8(cpu, DirectByte(memory, cpu, fields[i]));
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
            }
            LoadA8(cpu, DirectByte(memory, cpu, 0x18u));
            And8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            IncrementDirect8(memory, cpu, 0x15u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x17u));
            if (mirrored) {
                Subtract16(cpu, 0x0010u);
            } else {
                cpu->carry = 0;
                Add16Value(cpu, 0x0010u);
            }
            Write16Direct(memory, cpu, 0x17u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
            cpu->carry = 0;
            Adc8(cpu, 0x02u);
            BitImmediate8(cpu, 0x0fu);
            if (cpu->zero)
                Adc8(cpu, 0x10u);
            StoreADirect8(memory, cpu, 0x11u);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        LoadA8(cpu, DirectByte(memory, cpu, 0x16u));           /* BDB3 */
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x16u);
        LoadYDirect16(memory, cpu, 0x1cu);
        StoreYDirect16(memory, cpu, 0x17u);
        DecrementDirect8(memory, cpu, 0x03u);
    } while (!cpu->zero);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x15u));
    PullDataBank(memory, cpu);
}

/* JSL $81:BD47 or $81:BDC8 from bank 85. */
static void BattleCallOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    SimulateJsrFrame(memory, cpu, mirrored ? 0xbdcau : 0xbd49u);
    BattleOamStrips(memory, cpu, mirrored);
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $81:BE58: 16x16 tilemap block, rows of $02 cells. */
static void BattleTileBlock(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));               /* BE5D */
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadXDirect16(memory, cpu, 0x08u);
    StoreXDirect16(memory, cpu, 0x19u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x03u));
    StoreADirect8(memory, cpu, 0x14u);
    for (;;) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BE70 */
        StoreADirect8(memory, cpu, 0x13u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
        do {
            SetAccumulatorWidth(cpu, 0);                       /* BE79 */
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
                cpu->accumulator);
            TransferAToY(cpu);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
                cpu->accumulator);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0042u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0002u);
            cpu->zero = (cpu->accumulator & 0x000fu) == 0;
            if (cpu->zero)
                Add16Value(cpu, 0x0010u);
            SetAccumulatorWidth(cpu, 1);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        StoreADirect8(memory, cpu, 0x11u);                     /* BEA5 */
        DecrementDirect8(memory, cpu, 0x14u);
        if (cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x19u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        Write16Direct(memory, cpu, 0x19u, cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
}

/* $85:8B4B: $153C sprites from 13-byte records at $139A. */
static void BattleSpriteRecords(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8B50 */
    StoreAAbsolute8(memory, cpu, 0x15c7u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15c8u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cau, 0);
    LoadAAbsolute8(memory, cpu, 0x153cu, 0);
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x139au, cpu->y);      /* 8B67 */
            if (cpu->negative) {
                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                LoadX16(cpu, 0x0202u);
                StoreXDirect16(memory, cpu, 0x02u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b5u, 0);       /* 8B8D */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x139cu, cpu->y));
                And8(cpu, 0x07u);
                AslA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1475u, 0));
                Or8(cpu, AbsoluteByte(memory, cpu, 0x139bu, cpu->y));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x139du, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                BattleCallOamStrips(memory, cpu, 0, 0x8ba7u);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15cau, 0));
                StoreAAbsolute8(memory, cpu, 0x15cau, 0);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000du);                    /* 8BB0 */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);
}

/* $85:8BC0: the single 3x3 sprite at $13CE. */
static void BattleSpriteSingle(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15ccu, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cbu, 0);
    LoadAAbsolute8(memory, cpu, 0x13ceu, 0);
    if (cpu->negative) {
        LoadA8(cpu, 0xffu);                                    /* 8BD2 */
        StoreAAbsolute8(memory, cpu, 0x15cbu, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x13d8u, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d4u, 0));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0x05u);
        LoadX16(cpu, 0x0303u);
        StoreXDirect16(memory, cpu, 0x02u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d6u, 0));
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d2u, 0));
        And16(cpu, 0x01ffu);
        Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x15b6u, 0);               /* 8BF7 */
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d0u, 0));
        And8(cpu, 0x07u);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13ceu, 0));
        cpu->carry = 1;
        RolA8(cpu);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x1476u, 0));
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13cfu, 0));
        StoreADirect8(memory, cpu, 0x04u);
        LoadAAbsolute8(memory, cpu, 0x13d1u, 0);
        StoreADirect8(memory, cpu, 0x00u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        BitImmediate8(cpu, 0x40u);
        if (cpu->zero)
            BattleCallOamStrips(memory, cpu, 0, 0x8c1bu);
        else
            BattleCallOamStrips(memory, cpu, 1, 0x8c21u);
        StoreAAbsolute8(memory, cpu, 0x15ceu, 0);
    }
    PullDataBank(memory, cpu);                                 /* 8C25 */
}

/* $85:8C27: five 1x1 sprites from $1435, same records as $139A. */
static void BattleSpriteMarkers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, 0x493du);                                     /* 8C2C */
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d2u, 0);
    LoadA8(cpu, 0x05u);
    StoreADirect8(memory, cpu, 0x0du);
    LoadY16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x1435u, cpu->y);          /* 8C3B */
        if (cpu->negative) {
            PushY(memory, cpu);
            TransferDirectToA(cpu);
            LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
            StoreADirect8(memory, cpu, 0x05u);
            LoadX16(cpu, 0x0101u);
            StoreXDirect16(memory, cpu, 0x02u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu,
                Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            And16(cpu, 0x01ffu);
            Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x1478u, 0);           /* 8C64 */
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Or8(cpu, AbsoluteByte(memory, cpu, 0x1436u, cpu->y));
            Or8(cpu, 0x01u);
            StoreADirect8(memory, cpu, 0x04u);
            LoadAAbsolute8(memory, cpu, 0x1438u, cpu->y);
            StoreADirect8(memory, cpu, 0x00u);
            BattleCallOamStrips(memory, cpu, 0, 0x8c79u);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d2u, 0));
            StoreAAbsolute8(memory, cpu, 0x15d2u, 0);
            cpu->y = PullIndexValue(memory, cpu);
        }
        BattleNextRecord(cpu, 0x000du);                        /* 8C82 */
        DecrementDirect8(memory, cpu, 0x0du);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x15d2u, 0);                   /* 8C90 */
    StoreAAbsolute8(memory, cpu, 0x15cfu, 0);
    PullDataBank(memory, cpu);
}

/* $85:8C98: six sprites from 15-byte records at $13DB. */
static void BattleSpriteParty(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8C9D */
    StoreAAbsolute8(memory, cpu, 0x15d3u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15d4u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d6u, 0);
    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x0bu);
    StoreZeroAbsolute8(memory, cpu, 0x1577u, 0);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8CBE */
            if (cpu->negative) {
                uint8_t mirrored;

                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13e5u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13e1u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e3u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x13dfu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b7u, 0);       /* 8CE4 */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13ddu, cpu->y));
                And8(cpu, 0x07u);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x13dbu, cpu->y));
                cpu->carry = 1;
                RolA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1477u, 0));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
                BitImmediate8(cpu, 0x40u);
                mirrored = cpu->zero ? 0u : 1u;
                BattleCallOamStrips(
                    memory, cpu, mirrored, mirrored ? 0x8d0bu : 0x8d05u);
                LoadYDirect16(memory, cpu, 0x0bu);             /* 8D0C */
                StoreAAbsolute8(memory, cpu, 0x157fu, cpu->y);
                IncrementDirect8(memory, cpu, 0x0bu);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d6u, 0));
                StoreAAbsolute8(memory, cpu, 0x15d6u, 0);
                StoreAAbsolute8(memory, cpu, 0x1578u, cpu->y);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8D1E */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8D2C */
}

/* $85:8D2E: party tilemap at $7E:2800 instead of sprites. */
static void BattlePartyTilemap(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);               /* 8D31 */
    LoadX16(cpu, 0x2800u);
    LoadY16(cpu, 0x0200u);
    Write16Absolute(memory, cpu, 0x2181u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x2183u, 0);
    LoadA8(cpu, 0x01u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x2180u, 0);           /* 8D42 */
        StoreAAbsolute8(memory, cpu, 0x2180u, 0);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);                   /* 8D4B */
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x05u);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8D5B */
            if (cpu->negative) {
                PushY(memory, cpu);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13e1u, cpu->y);  /* 8D6A */
                ExchangeAccumulatorBytes(cpu);
                LoadAAbsolute8(memory, cpu, 0x13dfu, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, 0x0100u);
                LsrA16(cpu);
                LsrA16(cpu);
                LsrA16(cpu);
                And16(cpu, 0x1f1fu);
                SetAccumulatorWidth(cpu, 1);
                ExchangeAccumulatorBytes(cpu);                 /* 8D7F */
                Write8(memory, 0x004202u, A8(cpu));
                LoadA8(cpu, 0x40u);
                Write8(memory, 0x004203u, A8(cpu));
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                AslA8(cpu);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x004216u));
                Add16Value(cpu, 0x2800u);
                Write16Direct(memory, cpu, 0x08u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13ddu, cpu->y);  /* 8D9C */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15b7u, 0));
                And8(cpu, 0x07u);
                AslA8(cpu);
                AslA8(cpu);
                Or8(cpu, 0x23u);
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                SimulateJslFrame(memory, cpu, 0x85u, 0x8db3u); /* $81:BE54 */
                SimulateJsrFrame(memory, cpu, 0xbe56u);
                BattleTileBlock(memory, cpu);
                SimulateRtsFrame(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8DB5 */
            DecrementDirect8(memory, cpu, 0x05u);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8DC3 */
}

/* $85:972E: 15 rows of 16 tile ids from $3710. */
static void BattleTileGrid(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    unsigned row;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x3710u);
    for (row = 0; row < 15u; ++row) {
        unsigned column;

        LoadX16(cpu, (uint16_t)(0x2816u + 0x40u * row));
        SimulateJsrFrame(memory, cpu, (uint16_t)(0x9738u + 6u * row));
        for (column = 0; column < 16u; ++column) {
            Write16Long(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(2u * column), cpu->x), cpu->accumulator);
            IncrementA16(cpu);
        }
        SimulateRtsFrame(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 1);
}

/* $85:8A2F body; returns the RTL taken. */
static uint16_t BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x15abu, 0);
    DecrementA8(cpu);
    if (cpu->zero) {
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a6fu);
        BattleSpriteRecords(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a73u);
        BattleSpriteSingle(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a77u);
        BattleSpriteMarkers(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadAAbsolute8(memory, cpu, 0x11deu, 0);               /* 8A78 */
        if (cpu->zero) {
            SimulateJslFrame(memory, cpu, 0x85u, 0x8a80u);
            BattlePartyTilemap(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            return 0x8a95u;
        }
        LoadAAbsolute8(memory, cpu, 0x125fu, 0);               /* 8A83 */
        if (!cpu->zero)
            return 0x8a95u;
        StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);
        BattleSetDataBank(memory, cpu, 0x7eu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a93u);
        BattleTileGrid(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        PullDataBank(memory, cpu);
        return 0x8a95u;
    }
    DecrementA8(cpu);
    if (!cpu->zero)
        return 0x8a38u;
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa1u);             /* 8A9E */
    BattleSpriteRecords(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa5u);
    BattleSpriteSingle(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa9u);
    BattleSpriteMarkers(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aadu);
    BattleSpriteParty(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return 0x8aaeu;
}

/* $85:91A1: refresh the five $147A state bytes. */
static void BattleSlotStates(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0000u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    do {
        SetAccumulatorWidth(cpu, 0);                           /* 91A8 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a64u, cpu->x));
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x147au, cpu->y);
            ExchangeAccumulatorBytes(cpu);
            LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x147au, cpu->y);
            if (cpu->zero) {
                TransferDirectToA(cpu);                        /* 91CC */
                StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
            } else {
                ExchangeAccumulatorBytes(cpu);
                if (cpu->zero) {
                    LoadA8(cpu, 0xffu);
                    StoreAAbsolute8(memory, cpu, 0x147bu, cpu->y);
                    StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
                }
            }
            cpu->x = PullIndexValue(memory, cpu);
        }
        IncrementX16(cpu);                                     /* 91D1 */
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0014u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

static Lufia2ActorPrimaryUpdateResult BattleFrameEntry(
    Lufia2ActorFrontendCpu *cpu, uint32_t entry, uint32_t exit) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = exit;
    result.dispatches = 0;
    /* Only the M=1 X=0 entry is native. */
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = entry;
    }
    return result;
}

/* $85:8A2F: battle sprites, JSL entry. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x858a2fu, 0x858a38u);

    if (result.flow == LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        result.pc = 0x850000u | BattleSprites(memory, cpu);
    return result;
}

/* $85:ECF0: per-frame battle upkeep from the $81:8877 loop. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleFrameUpkeep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x85ecf0u, 0x85ed50u);
    static const uint16_t lists[2][2] = {{0x0a64u, 8u}, {0x0a6eu, 10u}};
    unsigned list;

    if (result.flow != LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        return result;
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf3u);
    BattleSlotStates(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf7u);
    (void)BattleSprites(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* ECF8 */
    Write8(memory, 0x0012f3u, A8(cpu));
    SimulateJslFrame(memory, cpu, 0x85u, 0xed01u);             /* $85:9265 */
    BattleSetDataBank(memory, cpu, 0x7fu);
    LoadX16(cpu, 0x02fbu);
    do {
        StoreZeroAbsolute8(memory, cpu, 0xf44eu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x1b8bu, 0);               /* ED02 */
    LoadX16(cpu, 0x0023u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x1b8cu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    LoadAAbsolute8(memory, cpu, 0x11e8u, 0);                   /* ED0E */
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    if (!cpu->zero)
        StoreAAbsolute8(memory, cpu, 0x11e8u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x11eau, 0);
    for (list = 0; list < 2u; ++list) {
        LoadY16(cpu, lists[list][1]);                          /* ED1A */
        do {
            LoadX16(cpu, Read16AbsoluteIndexed(
                memory, cpu, lists[list][0], cpu->y));
            if (!cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x0010u, cpu->x);
                And8(cpu, 0xfeu);
                StoreAAbsolute8(memory, cpu, 0x0010u, cpu->x);
            }
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->negative);
    }
    LoadX16(cpu, 0x0041u);                                     /* ED42 */
    do {
        LoadAAbsolute8(memory, cpu, 0x14e6u, cpu->x);
        if (!cpu->zero) {
            const uint32_t timer =
                AbsoluteIndexedAddress(cpu, 0x14e6u, cpu->x);
            const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

            Write8(memory, timer, left);
            SetNz8(cpu, left);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    return result;
}

static void RorA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t old = A8(cpu);
    const uint8_t value =
        (uint8_t)((old >> 1) | (cpu->carry ? 0x80u : 0u));
    cpu->carry = old & 1u;
    LoadA8(cpu, value);
}

/* BIT abs, 16-bit: N and V from memory. */
static void BitAbsolute16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address) {
    const uint16_t value = Read16AbsoluteIndexed(memory, cpu, address, 0);

    cpu->zero = (cpu->accumulator & value) == 0;
    cpu->negative = (value & 0x8000u) != 0;
    cpu->overflow = (value & 0x4000u) != 0;
}

/* TSB/TRB dp at the current accumulator width. */
static void TestBitsDirect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset,
    uint8_t set) {
    if (cpu->accumulator_is_8_bit) {
        const uint8_t value = DirectByte(memory, cpu, offset);

        cpu->zero = (value & A8(cpu)) == 0;
        Write8(memory, DirectAddress(cpu, offset), set
            ? (uint8_t)(value | A8(cpu)) : (uint8_t)(value & ~A8(cpu)));
    } else {
        const uint16_t value = Read16Direct(memory, cpu, offset);

        cpu->zero = (value & cpu->accumulator) == 0;
        Write16Direct(memory, cpu, offset, set
            ? (uint16_t)(value | cpu->accumulator)
            : (uint16_t)(value & ~cpu->accumulator));
    }
}

static void LoadXDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadX8(cpu, DirectByte(memory, cpu, offset));
}

static void LoadYDirect8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadY8(cpu, DirectByte(memory, cpu, offset));
}

static void LoadADirect16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadA16(cpu, Read16Direct(memory, cpu, offset));
}

static void StoreADirect16(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    Write16Direct(memory, cpu, offset, cpu->accumulator);
}

static void StoreAAbsolute16(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t index) {
    Write16Long(memory, AbsoluteIndexedAddress(cpu, address, index),
        cpu->accumulator);
}

/* $83:A669: set the size bit for OAM entry $58, then $58++. */
static void FieldOamHighBit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferDirectToA(cpu);                                    /* A669 */
    LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
    And8(cpu, 0x03u);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0xac10u, cpu->x);
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
    IncrementDirect8(memory, cpu, 0x58u);
    LsrA8(cpu);
    LsrA8(cpu);
    And8(cpu, 0x1fu);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, 0x0300u, cpu->x);
    Or8(cpu, DirectByte(memory, cpu, 0x55u));
    StoreAAbsolute8(memory, cpu, 0x0300u, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A591-$83:A633: four 16x16 tiles; bit 1 flips Y, bit 0 X. */
static void FieldOamQuad(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t layout) {
    static const uint8_t tiles[2][4] = {
        {0x02u, 0x06u, 0x0au, 0x0eu}, {0x06u, 0x02u, 0x0eu, 0x0au}};
    const uint8_t *order = tiles[layout & 1u];
    unsigned i;

    SimulateJsrFrame(memory, cpu, 0xa555u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    for (i = 0; i < 4u; ++i) {
        if (i)
            LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, order[i], cpu->x);
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0009u : 0x0001u, cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x000du : 0x0005u, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0001u : 0x0009u, cpu->x);
    StoreAAbsolute8(memory, cpu, (layout & 2u) ? 0x0005u : 0x000du, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
    StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x0008u, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, 0x10u);
    StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x000cu, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A48A handlers: OAM entries by sprite size. */
static void FieldOamEntries(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t size) {
    SimulateJsrFrame(memory, cpu, 0xa463u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    if (size == 0) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A4A2 */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (!cpu->zero)
            FieldOamHighBit(memory, cpu, 0xa4bdu);
    } else if (size == 1) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A4BF */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        BitImmediate8(cpu, 0x80u);
        {
            const uint16_t top = cpu->zero ? 0x0001u : 0x0005u;

            LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
            StoreAAbsolute8(memory, cpu, top, cpu->x);
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            StoreAAbsolute8(memory, cpu, top ^ 0x0004u, cpu->x);
        }
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));           /* A4EE */
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (!cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa4fcu);
            FieldOamHighBit(memory, cpu, 0xa4ffu);
        }
    } else if (size == 2) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));           /* A501 */
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->x);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0005u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreAAbsolute8(memory, cpu, 0x0004u, cpu->x);
        if (!cpu->carry)
            LoadA8(cpu, DirectByte(memory, cpu, 0x90u));
        if (cpu->carry || !cpu->zero) {
            FieldOamHighBit(memory, cpu, 0xa52fu);             /* A52D */
            FieldOamHighBit(memory, cpu, 0xa532u);
        }
    } else {
        TransferDirectToA(cpu);                                /* A534 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x97u));
        StoreAAbsolute8(memory, cpu, 0x0003u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0007u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x000bu, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x000fu, cpu->x);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        And8(cpu, 0x06u);
        TransferAToX(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x9du));
        FieldOamQuad(memory, cpu, (uint8_t)(cpu->x >> 1));
        LoadA8(cpu, DirectByte(memory, cpu, 0x90u));           /* A556 */
        if (!cpu->negative) {
            LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (cpu->carry) {
                IncrementDirect8(memory, cpu, 0x58u);          /* A561 */
                FieldOamHighBit(memory, cpu, 0xa565u);
                IncrementDirect8(memory, cpu, 0x58u);
                FieldOamHighBit(memory, cpu, 0xa56au);
            }
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));       /* A56C */
            cpu->carry = 0;
            Adc8(cpu, 0x10u);
            if (!cpu->carry) {
                FieldOamHighBit(memory, cpu, 0xa575u);
                FieldOamHighBit(memory, cpu, 0xa578u);
                FieldOamHighBit(memory, cpu, 0xa57bu);
                FieldOamHighBit(memory, cpu, 0xa57eu);
            } else {
                FieldOamHighBit(memory, cpu, 0xa582u);         /* A580 */
                IncrementDirect8(memory, cpu, 0x58u);
                FieldOamHighBit(memory, cpu, 0xa587u);
            }
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:FCD1: queue a sprite upload from the object tables. */
static void FieldObjectSpriteUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xa4a0u);
    LoadY8(cpu, AbsoluteByte(memory, cpu, 0x0732u, 0));        /* FCD1 */
    TransferDirectToA(cpu);
    LoadXDirect8(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x83abfcu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadXDirect8(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fe05eu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadXDirect8(memory, cpu, 0xabu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd896u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x11d9u, cpu->y);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd894u, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x05c2u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), (uint8_t)cpu->y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A492: pending reload flag $20 in $0622,x. */
static void FieldObjectReload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);              /* A492 */
    BitImmediate8(cpu, 0x20u);
    if (!cpu->zero) {
        And8(cpu, 0xdfu);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        FieldObjectSpriteUpload(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $83:AAE5: queue the actor's new animation frame upload. */
static void FieldActorFrameUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint32_t pointer;

    SimulateJsrFrame(memory, cpu, 0xa3d2u);
    SetAccumulatorWidth(cpu, 1);                               /* AAE5 */
    SetIndexWidth(cpu, 1);
    LoadXDirect8(memory, cpu, 0xa7u);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, LongIndexedAddress(0x7fe2eeu, cpu->x)));
    SetAccumulatorWidth(cpu, 0);
    LoadYDirect8(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d1u, cpu->y));
    StoreADirect16(memory, cpu, 0x54u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x83u);
    StoreADirect8(memory, cpu, 0x56u);
    LoadXDirect8(memory, cpu, 0xa7u);
    pointer = Read16Direct(memory, cpu, 0x54u) |
        ((uint32_t)DirectByte(memory, cpu, 0x56u) << 16);
    LoadA8(cpu, Read8(memory, pointer));                       /* LDA [$54] */
    And8(cpu, 0x7fu);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x83abf8u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadYDirect8(memory, cpu, 0xabu);
    LoadAAbsolute8(memory, cpu, 0x12bbu, cpu->y);
    StoreADirect8(memory, cpu, 0x54u);
    LoadAAbsolute8(memory, cpu, 0x4216u, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x12b9u, cpu->y));
    StoreADirect16(memory, cpu, 0x56u);
    LoadY8(cpu, AbsoluteByte(memory, cpu, 0x0732u, 0));
    LoadXDirect8(memory, cpu, 0xa9u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1381u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11f9u, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1331u, cpu->x));
    StoreAAbsolute16(memory, cpu, 0x11e9u, cpu->y);
    LoadADirect16(memory, cpu, 0x54u);
    StoreAAbsolute16(memory, cpu, 0x11d9u, cpu->y);
    LoadADirect16(memory, cpu, 0x56u);
    StoreAAbsolute16(memory, cpu, 0x05c2u, cpu->y);
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    LoadY8(cpu, (uint8_t)(cpu->y + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), (uint8_t)cpu->y);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A29B: sort visible actors by Y into $E200/$E300. */
static void FieldSortVisible(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    TransferDirectToA(cpu);                                    /* A295 */
    TransferAToY(cpu);
    LoadX8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 1);
    do {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);          /* A29B */
        BitImmediate8(cpu, 0x04u);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
            BitImmediate8(cpu, 0x80u);
        }
        if (cpu->zero) {
            uint8_t visible = 0;

            SetAccumulatorWidth(cpu, 0);
            TransferXToA(cpu);
            StoreADirect16(memory, cpu, 0xa7u);
            AslA16(cpu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0x0030u);
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
            if (!cpu->negative) {
                Compare16(cpu, cpu->accumulator,
                    Read16Direct(memory, cpu, 0x56u));
                if (cpu->negative) {
                    LoadA16(cpu, Read16Long(
                        memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
                    cpu->carry = 0;
                    Add16Value(cpu, 0x0020u);
                    Compare16(cpu, cpu->accumulator,
                        Read16Direct(memory, cpu, 0x58u));
                    if (!cpu->negative) {
                        Compare16(cpu, cpu->accumulator,
                            Read16Direct(memory, cpu, 0x5au));
                        visible = cpu->negative;
                    }
                }
            }
            if (visible) {
                LoadA16(cpu, (uint16_t)(cpu->accumulator | 0x1000u));
                StoreADirect16(memory, cpu, 0x63u);
                LoadXDirect8(memory, cpu, 0xa7u);
                LoadA16(cpu, Read16Long(
                    memory, LongIndexedAddress(0x7fe316u, cpu->x)));
                cpu->zero = (cpu->accumulator & 0x0001u) == 0;
                if (!cpu->zero) {
                    LoadA16(cpu, 0x1000u);
                    TestBitsDirect(memory, cpu, 0x63u, 0);
                } else {
                    cpu->zero = (cpu->accumulator & 0x0002u) == 0;
                    if (!cpu->zero) {
                        LoadA16(cpu, 0x2000u);
                        TestBitsDirect(memory, cpu, 0x63u, 1);
                    }
                }
                LoadX8(cpu, (uint8_t)cpu->y);                  /* A2F2 */
                while (!cpu->zero) {
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe1feu, cpu->x));
                    Compare16(cpu, cpu->accumulator,
                        Read16Direct(memory, cpu, 0x63u));
                    if (cpu->carry)
                        break;
                    StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                    LoadA16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0xe2feu, cpu->x));
                    StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                    LoadX8(cpu, (uint8_t)(cpu->x - 1u));
                }
                LoadADirect16(memory, cpu, 0x63u);             /* A309 */
                StoreAAbsolute16(memory, cpu, 0xe200u, cpu->x);
                LoadADirect16(memory, cpu, 0xa7u);
                StoreAAbsolute16(memory, cpu, 0xe300u, cpu->x);
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
                LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            }
            SetAccumulatorWidth(cpu, 1);                       /* A315 */
            LoadXDirect8(memory, cpu, 0xa7u);
        }
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));                   /* A319 */
        Compare8(cpu, (uint8_t)cpu->x, 0x48u);
    } while (!cpu->zero);
}

/* $83:A33A: OAM entries for one sorted actor. */
static uint8_t FieldActorOam(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ee300u, cpu->x)));
    StoreADirect8(memory, cpu, 0xa7u);
    TransferAToX(cpu);
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0xa9u);
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0xa7u));
    StoreADirect8(memory, cpu, 0xabu);
    Write8(memory, DirectAddress(cpu, 0x97u), 0x00u);
    Compare8(cpu, (uint8_t)cpu->x, 0x28u);
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x0732u, 0);               /* A34F */
        Compare8(cpu, A8(cpu), 0x10u);
        if (!cpu->carry) {
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe37eu, cpu->x)));
            Compare8(cpu, A8(cpu), 0xffu);
            if (!cpu->zero) {
                PushIndex(memory, cpu);
                cpu->carry = 0;
                Adc8(cpu, 0x28u);
                StoreADirect8(memory, cpu, 0xa7u);
                SimulateJslFrame(memory, cpu, 0x83u, 0xa368u);
                SecondaryRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                LoadXDirect8(memory, cpu, 0xa7u);
                FieldObjectReload(memory, cpu, 0xa36du);
                cpu->x = PullIndexValue(memory, cpu);
                Write8(memory, DirectAddress(cpu, 0xa7u), (uint8_t)cpu->x);
                SimulateJslFrame(memory, cpu, 0x83u, 0xa374u);
                SecondaryRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
            } else {
                FieldObjectReload(memory, cpu, 0xa379u);       /* A377 */
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* A37A */
        LoadXDirect8(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
        Subtract16(cpu, 0x0008u);
        StoreADirect16(memory, cpu, 0x8fu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0xa1u));
        StoreADirect16(memory, cpu, 0x91u);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect8(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
        AslA8(cpu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0x03u);
        LsrA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        RorA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        LsrA8(cpu);
        ExchangeAccumulatorBytes(cpu);
        RorA8(cpu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
    } else {
        LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);          /* A3BB */
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x1471u, cpu->x));
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x0732u, 0);
            Compare8(cpu, A8(cpu), 0x0au);
            if (!cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
                StoreAAbsolute8(memory, cpu, 0x1471u, cpu->x);
                FieldActorFrameUpload(memory, cpu);
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* A3D3 */
        LoadXDirect8(memory, cpu, 0xa9u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdc8cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
        Subtract16(cpu, 0x0008u);
        Subtract16(cpu, Read16Long(memory, 0x7fd081u));
        StoreADirect16(memory, cpu, 0x8fu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x7fdd1cu, cpu->x)));
        Subtract16(cpu, Read16Direct(memory, cpu, 0xa1u));
        cpu->carry = 0;
        Add16Value(cpu, (uint16_t)~Read16Long(memory, 0x7fd083u));
        StoreADirect16(memory, cpu, 0x91u);
        SetAccumulatorWidth(cpu, 1);
        LoadXDirect8(memory, cpu, 0xa7u);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe1ceu, cpu->x)));
        And8(cpu, 0x0eu);
        TestBitsDirect(memory, cpu, 0x97u, 1);
        LoadAAbsolute8(memory, cpu, 0x1291u, cpu->x);
        And8(cpu, 0x18u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x066au, cpu->x);
            And8(cpu, 0x07u);
            Compare8(cpu, A8(cpu), 0x06u);
            if (cpu->carry) {
                LoadA8(cpu, 0x40u);
                TestBitsDirect(memory, cpu, 0x97u, 1);
            }
        }
    }
    LoadXDirect8(memory, cpu, 0xa7u);                          /* A421 */
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe316u, cpu->x)));
    And8(cpu, 0x30u);
    TestBitsDirect(memory, cpu, 0x97u, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe2a6u, cpu->x)));
    TestBitsDirect(memory, cpu, 0x97u, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe25eu, cpu->x)));
    StoreADirect8(memory, cpu, 0x9du);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    TransferAToY(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
    StoreADirect8(memory, cpu, 0x94u);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0xabf4u, cpu->y));
    StoreADirect8(memory, cpu, 0x5fu);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x94u));
    StoreADirect8(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, 0x0100u);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    Compare8(cpu, A8(cpu), 0x04u);
    if (cpu->carry)
        TransferDirectToA(cpu);
    AslA8(cpu);
    TransferAToX(cpu);
    if (cpu->x > 6u) {
        /* D high byte set: LLE takes the JSR. */
        cpu->resume_pc = 0x83a461u;
        return 0;
    }
    FieldOamEntries(memory, cpu, (uint8_t)(cpu->x >> 1));
    return 1;
}

/* $83:A21A: field OAM from the visible, Y-sorted actors. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldActorSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x83a489u;
    result.dispatches = 0;
    /* X is set before any index use; M=1 only. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x83a21au;
        return result;
    }
    BattleSetDataBank(memory, cpu, 0x7eu);                     /* A21A */
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, 0x7fd0b0u));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        unsigned i;

        SetAccumulatorWidth(cpu, 0);                           /* A228 */
        SetIndexWidth(cpu, 0);
        AslA16(cpu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, 0xf000u);
        do {
            StoreAAbsolute16(memory, cpu, 0x0140u, cpu->x);
            LoadX16(cpu, (uint16_t)(cpu->x - 4u));
        } while (!cpu->negative);
        SetIndexWidth(cpu, 1);
        for (i = 0; i < 8u; ++i)
            Write16Long(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(0x0302u + 2u * i), 0), 0x0000u);
    }
    SetAccumulatorWidth(cpu, 0);                               /* A253 */
    SetIndexWidth(cpu, 1);
    LoadA16(cpu, 0x0040u);
    BitAbsolute16(memory, cpu, 0x1261u);
    if (!cpu->zero) {
        LoadA16(cpu, Read16Long(memory, 0x7fd0eeu));
        StoreADirect16(memory, cpu, 0x9fu);
        LoadA16(cpu, Read16Long(memory, 0x7fd0f0u));
        StoreADirect16(memory, cpu, 0xa1u);
    } else {
        LoadA16(cpu, Read16Long(memory, 0x001220u));
        StoreADirect16(memory, cpu, 0x9fu);
        LoadA16(cpu, Read16Long(memory, 0x001228u));
        StoreADirect16(memory, cpu, 0xa1u);
    }
    LoadADirect16(memory, cpu, 0x9fu);                         /* A279 */
    cpu->carry = 0;
    Add16Value(cpu, 0x0020u);
    StoreADirect16(memory, cpu, 0x54u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0110u);
    StoreADirect16(memory, cpu, 0x56u);
    LoadADirect16(memory, cpu, 0xa1u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0010u);
    StoreADirect16(memory, cpu, 0x58u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    StoreADirect16(memory, cpu, 0x5au);
    FieldSortVisible(memory, cpu);
    Write8(memory, DirectAddress(cpu, 0x60u), (uint8_t)cpu->y); /* A321 */
    Push8(memory, cpu, 0x83u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x60u));
    if (!cpu->zero) {
        Write8(memory, DirectAddress(cpu, 0xaau), 0x00u);      /* A32E */
        Write8(memory, DirectAddress(cpu, 0xacu), 0x00u);
        LoadA8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x5fu);
        LoadX8(cpu, 0x00u);
        Write8(memory, DirectAddress(cpu, 0x5du), 0x00u);
        do {
            if (!FieldActorOam(memory, cpu)) {
                result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
                result.pc = cpu->resume_pc;
                return result;
            }
            ++result.dispatches;
            SetAccumulatorWidth(cpu, 1);                       /* A464 */
            SetIndexWidth(cpu, 1);
            LoadXDirect8(memory, cpu, 0x5du);
            LoadX8(cpu, (uint8_t)(cpu->x + 2u));
            Write8(memory, DirectAddress(cpu, 0x5du), (uint8_t)cpu->x);
            Compare8(cpu, (uint8_t)cpu->x, DirectByte(memory, cpu, 0x60u));
        } while (!cpu->zero);
    }
    SetAccumulatorWidth(cpu, 1);                               /* A473 */
    SetIndexWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5fu));
    cpu->carry = 1;
    Sbc8(cpu, 0x11u);
    Write8(memory, 0x7fd0b0u, A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x72u));
    BitImmediate8(cpu, 0x01u);
    if (cpu->zero) {
        LoadA8(cpu, 0x80u);
        StoreADirect8(memory, cpu, 0x72u);
    }
    PullDataBank(memory, cpu);                                 /* A488 */
    return result;
}

/* dp,X: wraps inside bank 0. */
static uint32_t DirectIndexedAddress(
    const Lufia2ActorFrontendCpu *cpu, uint8_t offset, uint16_t index) {
    return (uint16_t)(cpu->direct_page + offset + index);
}

static uint16_t Read16DirectIndexed(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset,
    uint16_t index) {
    const uint16_t address = (uint16_t)DirectIndexedAddress(cpu, offset, index);

    return (uint16_t)(Read8(memory, address) |
        ((uint16_t)Read8(memory, (uint16_t)(address + 1u)) << 8));
}

static void StoreAImmediate8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t value,
    uint16_t address) {
    LoadA8(cpu, value);
    StoreAAbsolute8(memory, cpu, address, 0);
}

static void CopyAbsolute8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t from,
    uint16_t to) {
    LoadAAbsolute8(memory, cpu, from, 0);
    StoreAAbsolute8(memory, cpu, to, 0);
}

/* $86:D1E8: tile column upload, rows of $0100 words. */
static void WorldMapColumnUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd1d8u);
    SetAccumulatorWidth(cpu, 0);                               /* D1E8 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4364u, 0);
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, 0x05u);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    StoreADirect8(memory, cpu, 0x33u);
    LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x33u));
    StoreADirect8(memory, cpu, 0x35u);
    if (!cpu->zero) {
        LoadA8(cpu, 0xc0u);
        StoreADirect8(memory, cpu, 0x05u);
    }
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);              /* D214 */
    StoreADirect8(memory, cpu, 0x37u);
    cpu->carry = 0;
    do {
        SetAccumulatorWidth(cpu, 0);                           /* D21A */
        LoadADirect16(memory, cpu, 0x33u);
        Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
        LoadADirect16(memory, cpu, 0x35u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->accumulator);
        LoadADirect16(memory, cpu, 0x39u);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
        StoreADirect16(memory, cpu, 0x39u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
        StoreAAbsolute8(memory, cpu, 0x420bu, 0);
        DecrementDirect8(memory, cpu, 0x37u);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0xd26cu, cpu->x);              /* D23C */
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x0002u, cpu->y));
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x37u);
        LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
        StoreADirect8(memory, cpu, 0x35u);
        cpu->carry = 0;
        do {
            SetAccumulatorWidth(cpu, 0);                       /* D24D */
            LoadADirect16(memory, cpu, 0x35u);
            Write16Absolute(memory, cpu, 0x4375u, cpu->accumulator);
            LoadADirect16(memory, cpu, 0x39u);
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0100u);
            StoreADirect16(memory, cpu, 0x39u);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
            DecrementDirect8(memory, cpu, 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D271: block upload, two passes of rows $0200 apart. */
static void WorldMapBlockUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    unsigned pass;

    SimulateJsrFrame(memory, cpu, 0xd1ddu);
    SetAccumulatorWidth(cpu, 0);                               /* D271 */
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4364u, 0);
    LoadAAbsolute8(memory, cpu, 0xd2d0u, cpu->x);
    StoreADirect8(memory, cpu, 0x33u);
    LoadAAbsolute8(memory, cpu, 0xd2d1u, cpu->x);
    StoreADirect8(memory, cpu, 0x37u);
    StoreADirect8(memory, cpu, 0x38u);
    for (pass = 0; pass < 2u; ++pass) {
        if (pass)
            IncrementDirect8(memory, cpu, 0x3au);              /* D2B0 */
        LoadXDirect16(memory, cpu, 0x39u);
        cpu->carry = 0;
        do {
            SetAccumulatorWidth(cpu, 0);                       /* D295 */
            LoadADirect16(memory, cpu, 0x33u);
            Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
            TransferXToA(cpu);
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0200u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
            DecrementDirect8(memory, cpu, pass ? 0x38u : 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D1A1: pending map tile uploads, $1365 entries. */
static void WorldMapTileUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xcf14u);
    LoadA8(cpu, 0x18u);                                        /* D1A1 */
    StoreAAbsolute8(memory, cpu, 0x4361u, 0);
    StoreAAbsolute8(memory, cpu, 0x4371u, 0);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x09u, 0x4370u);
    LoadX16(cpu, 0xd1e7u);
    Write16Absolute(memory, cpu, 0x4372u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x86u, 0x4374u);
    Write8(memory, DirectAddress(cpu, 0x34u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x36u), 0x00u);
    LoadX16(cpu, 0x1367u);
    do {
        PushIndex(memory, cpu);                                /* D1C5 */
        LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x40u, cpu->x));
        if (!cpu->zero) {
            StoreYDirect16(memory, cpu, 0x39u);
            LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x00u, cpu->x));
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            if (!cpu->negative)
                WorldMapColumnUpload(memory, cpu);
            else
                WorldMapBlockUpload(memory, cpu);
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* D1DE */
        IncrementX16(cpu);
        IncrementX16(cpu);
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, 0x1365u, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $86:CFC0: $16E7 palette cycles, five bytes each at $16E8. */
static void WorldMapPaletteCycles(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, 0x16e8u);
    do {
        PushAccumulator8(memory, cpu);                         /* CFC8 */
        {
            const uint32_t tick = DirectIndexedAddress(cpu, 0x04u, cpu->x);
            const uint8_t value = (uint8_t)(Read8(memory, tick) + 1u);

            Write8(memory, tick, value);
            SetNz8(cpu, value);
            LoadA8(cpu, value);
        }
        Compare8(cpu, A8(cpu),
            Read8(memory, DirectIndexedAddress(cpu, 0x02u, cpu->x)));
        if (cpu->carry) {
            uint32_t cycle;

            Write8(memory, DirectIndexedAddress(cpu, 0x04u, cpu->x), 0x00u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            StoreAAbsolute8(memory, cpu, 0x2121u, 0);
            cycle = DirectIndexedAddress(cpu, 0x03u, cpu->x);
            LoadA8(cpu, (uint8_t)(Read8(memory, cycle) + 1u));
            Compare8(cpu, A8(cpu),
                Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            if (cpu->carry)
                LoadA8(cpu, 0x00u);
            Write8(memory, cycle, A8(cpu));                    /* CFE1 */
            StoreADirect8(memory, cpu, 0x38u);
            AslA8(cpu);
            StoreADirect8(memory, cpu, 0x33u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, cycle));
            StoreADirect8(memory, cpu, 0x37u);
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            AslA8(cpu);
            Adc8(cpu, DirectByte(memory, cpu, 0x33u));
            TransferAToY(cpu);
            do {
                LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);  /* CFF8 */
                StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                IncrementY16(cpu);
                LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);
                StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                IncrementY16(cpu);
                DecrementDirect8(memory, cpu, 0x37u);
            } while (!cpu->zero);
            LoadA8(cpu, DirectByte(memory, cpu, 0x38u));
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory,
                    DirectIndexedAddress(cpu, 0x00u, cpu->x)));
                AslA8(cpu);
                TransferAToY(cpu);
                do {
                    LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);  /* D012 */
                    StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                    IncrementY16(cpu);
                    LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);
                    StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                    IncrementY16(cpu);
                    DecrementDirect8(memory, cpu, 0x38u);
                } while (!cpu->zero);
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* D024 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0005u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->zero);
}

/* $86:CEF6: world map NMI via the $00:0067 vector. */
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapNmiUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t mode7_regs[8] = {
        0x211bu, 0x211bu, 0x211cu, 0x211cu, 0x211du, 0x211du, 0x211eu,
        0x211eu};
    static const uint16_t scroll_regs[6] = {
        0x210du, 0x210eu, 0x210fu, 0x2110u, 0x2111u, 0x2112u};
    Lufia2ActorPrimaryUpdateResult result;
    unsigned i;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x86d1a0u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* CEF6 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreAImmediate8(memory, cpu, 0x8fu, 0x2100u);
    StoreZeroAbsolute8(memory, cpu, 0x420cu, 0);
    LoadAAbsolute8(memory, cpu, 0x11d9u, 0);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1365u, 0);
        if (!cpu->zero)
            WorldMapTileUploads(memory, cpu);
    }
    StoreZeroAbsolute8(memory, cpu, 0x4370u, 0);               /* CF15 */
    StoreAImmediate8(memory, cpu, 0x18u, 0x4371u);
    LoadAAbsolute8(memory, cpu, 0x1710u, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x2115u, 0);
        StoreAImmediate8(memory, cpu, 0x7fu, 0x4374u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1712u, 0));
        Write16Absolute(memory, cpu, 0x4372u, cpu->accumulator);
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x0100u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        StoreZeroAbsolute8(memory, cpu, 0x1710u, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1711u, 0);                   /* CF48 */
    if (!cpu->zero) {
        StoreAImmediate8(memory, cpu, 0x03u, 0x2115u);
        StoreAImmediate8(memory, cpu, 0x7fu, 0x4374u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1714u, 0));
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        IncrementA16(cpu);
        PushAccumulator16(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0xdf00u);
        Write16Absolute(memory, cpu, 0x4372u, cpu->x);
        LoadX16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        cpu->y = PullIndexValue(memory, cpu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->y);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        StoreZeroAbsolute8(memory, cpu, 0x1711u, 0);
    }
    StoreAImmediate8(memory, cpu, 0x80u, 0x2115u);             /* CF86 */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1702u, 0));
    if (!cpu->zero) {
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        CopyAbsolute8(memory, cpu, 0x1701u, 0x2121u);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1704u, 0));
        Write16Absolute(memory, cpu, 0x4372u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        CopyAbsolute8(memory, cpu, 0x1706u, 0x4374u);
        StoreZeroAbsolute8(memory, cpu, 0x4370u, 0);
        StoreAImmediate8(memory, cpu, 0x22u, 0x4371u);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        LoadX16(cpu, 0x0000u);
        Write16Absolute(memory, cpu, 0x1702u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x16e7u, 0);                   /* CFC0 */
    if (!cpu->zero)
        WorldMapPaletteCycles(memory, cpu);
    Write8(memory, DirectAddress(cpu, 0x33u), 0x00u);          /* D032 */
    LoadAAbsolute8(memory, cpu, 0x11deu, 0);
    if (cpu->zero) {
        for (i = 0; i < 8u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(0x1707u + i), mode7_regs[i]);
    } else {
        LoadAAbsolute8(memory, cpu, 0x11dau, 0);               /* D06B */
        if (!cpu->negative) {
            LoadA8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, 0x4300u, 0);
            StoreAAbsolute8(memory, cpu, 0x4310u, 0);
            StoreAImmediate8(memory, cpu, 0x1bu, 0x4301u);
            StoreAImmediate8(memory, cpu, 0x1du, 0x4311u);
            StoreAImmediate8(memory, cpu, 0x00u, 0x4304u);
            StoreAImmediate8(memory, cpu, 0x00u, 0x4314u);
            LoadX16(cpu, 0x1718u);
            Write16Absolute(memory, cpu, 0x4302u, cpu->x);
            LoadX16(cpu, 0x1a9bu);
            Write16Absolute(memory, cpu, 0x4312u, cpu->x);
            LoadA8(cpu, 0x03u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
        LoadAAbsolute8(memory, cpu, 0x11ddu, 0);               /* D09C */
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x2130u, 0);
            CopyAbsolute8(memory, cpu, 0x170fu, 0x2131u);
            StoreZeroAbsolute8(memory, cpu, 0x4340u, 0);
            StoreAImmediate8(memory, cpu, 0x32u, 0x4341u);
            StoreZeroAbsolute8(memory, cpu, 0x4344u, 0);
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1716u, 0));
            Write16Absolute(memory, cpu, 0x4342u, cpu->x);
            LoadA8(cpu, 0x10u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
    }
    LoadAAbsolute8(memory, cpu, 0x11dfu, 0);                   /* D0BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x43u);
        StoreAAbsolute8(memory, cpu, 0x4340u, 0);
        StoreAAbsolute8(memory, cpu, 0x4350u, 0);
        LoadX16(cpu, 0xde00u);
        Write16Absolute(memory, cpu, 0x4342u, cpu->x);
        LoadX16(cpu, 0xe000u);
        Write16Absolute(memory, cpu, 0x4352u, cpu->x);
        LoadA8(cpu, 0x30u);
        TestBitsDirect(memory, cpu, 0x33u, 1);
    }
    LoadAAbsolute8(memory, cpu, 0x11e1u, 0);                   /* D0DC */
    if (!cpu->zero) {
        LoadA8(cpu, 0x41u);
        StoreAAbsolute8(memory, cpu, 0x4320u, 0);
        StoreAAbsolute8(memory, cpu, 0x4330u, 0);
        StoreAImmediate8(memory, cpu, 0x26u, 0x4321u);
        StoreAImmediate8(memory, cpu, 0x28u, 0x4331u);
        LoadA8(cpu, 0x7fu);
        StoreAAbsolute8(memory, cpu, 0x4324u, 0);
        StoreAAbsolute8(memory, cpu, 0x4327u, 0);
        StoreAAbsolute8(memory, cpu, 0x4334u, 0);
        StoreAAbsolute8(memory, cpu, 0x4337u, 0);
        LoadY16(cpu, 0xd400u);
        LoadAAbsolute8(memory, cpu, 0x11dbu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xd600u);
        Write16Absolute(memory, cpu, 0x4322u, cpu->y);
        LoadY16(cpu, 0xd800u);
        LoadAAbsolute8(memory, cpu, 0x11dcu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xda00u);
        Write16Absolute(memory, cpu, 0x4332u, cpu->y);
        LoadA8(cpu, 0x0cu);
        TestBitsDirect(memory, cpu, 0x33u, 1);
        LoadY16(cpu, 0x00ffu);
        Write16Absolute(memory, cpu, 0x2126u, cpu->y);
        Write16Absolute(memory, cpu, 0x2128u, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, 0x11d8u, 0);                   /* D12C */
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
        StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    }
    for (i = 0; i < 4u; ++i)                                   /* D136 */
        CopyAbsolute8(memory, cpu, (uint16_t)(0x11f8u + i),
            (uint16_t)(0x211fu + (i >> 1)));
    LoadAAbsolute8(memory, cpu, 0x11d9u, 0);
    if (cpu->zero) {
        for (i = 0; i < 12u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(0x0594u + i),
                scroll_regs[i >> 1]);
    }
    StoreZeroAbsolute8(memory, cpu, 0x11d9u, 0);               /* D19B */
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return result;
}

/* $85:ECDB: Y = first free slot in the $1A8F VRAM queue. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleVramQueueSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x85ecefu;
    result.dispatches = 0;
    /* X=1 decodes LDY #imm as two bytes. */
    if (cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x85ecdbu;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* ECDB */
    for (;;) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->y));
        if (cpu->zero)
            return result;
        LoadY16(cpu, (uint16_t)(cpu->y + 6u));
        Compare16(cpu, cpu->y, 0x0060u);
        if (cpu->zero)
            break;
    }
    /* Queue full: BRK #$6B on LLE. */
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc = 0x85eceeu;
    return result;
}

/* [dp],Y: 24-bit pointer at dp, 16-bit read. */
static uint16_t Read16IndirectLongY(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint32_t pointer = Read16Direct(memory, cpu, offset) |
        ((uint32_t)DirectByte(memory, cpu, (uint8_t)(offset + 2u)) << 16);

    return Read16Long(memory, (pointer + cpu->y) & 0x00ffffffu);
}

static uint8_t Read8IndirectLongY(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint32_t pointer = Read16Direct(memory, cpu, offset) |
        ((uint32_t)DirectByte(memory, cpu, (uint8_t)(offset + 2u)) << 16);

    return Read8(memory, (pointer + cpu->y) & 0x00ffffffu);
}

/* $86:ADEE: $0B = map cell offset of ($58, $5A). */
static void WorldMapCellOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, 0x5au);                         /* ADEE */
    And16(cpu, 0x003fu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x0bu);
    LoadADirect16(memory, cpu, 0x58u);
    And16(cpu, 0x003fu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x0bu));
    AslA16(cpu);
    Add16Value(cpu, 0x0000u);
    StoreADirect16(memory, cpu, 0x0bu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AE05: $08 = map block pointer for ($58, $5A). */
static void WorldMapBlockPointer(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, 0x58u);                         /* AE05 */
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x08u);
    LoadADirect16(memory, cpu, 0x5au);
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x08u));
    AslA16(cpu);
    Add16Value(cpu, 0x4040u);
    StoreADirect16(memory, cpu, 0x08u);
    SimulateRtsFrame(memory, cpu);
}

/* Metatile index for the current cell: [$DF] or [$E3] table. */
static void WorldMapMetatile(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16Long(memory, ((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, 0x08u)));                    /* LDA ($08) */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadA16(cpu, Read16IndirectLongY(
        memory, cpu, cpu->negative ? 0xe3u : 0xdfu));
    StoreADirect16(memory, cpu, 0x00u);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadXDirect16(memory, cpu, 0x0bu);
}

/* $86:ACFE: stream one map column into $7F:DF00/$7F:DF80. */
static void WorldMapStreamColumn(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x99eeu);
    BattleSetDataBank(memory, cpu, 0x7fu);                     /* ACFE */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xad07u);
    WorldMapBlockPointer(memory, cpu, 0xad0au);
    LoadADirect16(memory, cpu, 0x0bu);
    And16(cpu, 0xc07fu);
    Write16Long(memory, 0x001714u, cpu->accumulator);
    LoadADirect16(memory, cpu, 0x58u);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x13u);
    LoadADirect16(memory, cpu, 0x5au);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x003fu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x0bu);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, 0x26u);
    do {
        LoadADirect16(memory, cpu, 0x5au);                     /* AD2A */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        StoreAAbsolute16(memory, cpu, 0xdf00u, cpu->x);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute16(memory, cpu, 0xdf80u, cpu->x);
        LoadADirect16(memory, cpu, 0x0bu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        And16(cpu, 0x007fu);
        StoreADirect16(memory, cpu, 0x0bu);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, 0x5au);
        if (cpu->zero) {
            SetAccumulatorWidth(cpu, 0);                       /* AD70 */
            WorldMapBlockPointer(memory, cpu, 0xad74u);
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, 0x5au));
            LsrA8(cpu);
            if (!cpu->carry)
                IncrementDirect8(memory, cpu, 0x09u);
        }
        SetAccumulatorWidth(cpu, 0);                           /* AD75 */
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, 0x26u) - 1u);

            Write16Direct(memory, cpu, 0x26u, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AC6C: stream one map row into the $7F buffer at $0B. */
static void WorldMapStreamRow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9a1fu);
    BattleSetDataBank(memory, cpu, 0x7fu);                     /* AC6C */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xac75u);
    WorldMapBlockPointer(memory, cpu, 0xac78u);
    LoadADirect16(memory, cpu, 0x0bu);
    And16(cpu, 0xff80u);
    Write16Long(memory, 0x001712u, cpu->accumulator);
    LoadADirect16(memory, cpu, 0x5au);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x13u);
    LoadADirect16(memory, cpu, 0x58u);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, 0x26u);
    do {
        LoadADirect16(memory, cpu, 0x58u);                     /* AC93 */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        SetAccumulatorWidth(cpu, 1);
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x0080u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xedu));
        StoreAAbsolute8(memory, cpu, 0x0081u, cpu->x);
        SetAccumulatorWidth(cpu, 0);                           /* ACCB */
        LoadADirect16(memory, cpu, 0x0bu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        cpu->zero = (cpu->accumulator & 0x007fu) == 0;
        if (cpu->zero)
            Subtract16(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x0bu);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, 0x58u);
        SetAccumulatorWidth(cpu, 0);
        if (cpu->zero) {
            WorldMapBlockPointer(memory, cpu, 0xace6u);
        } else {
            LoadADirect16(memory, cpu, 0x58u);                 /* ACEA */
            LsrA16(cpu);
            if (!cpu->carry) {
                uint16_t pointer = Read16Direct(memory, cpu, 0x08u);

                pointer = (uint16_t)(pointer + 2u);
                Write16Direct(memory, cpu, 0x08u, pointer);
                SetNz16(cpu, pointer);
            }
        }
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, 0x26u) - 1u);

            Write16Direct(memory, cpu, 0x26u, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Edge ahead of the move: position +$1F or -$1F. */
static void WorldMapEdge(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t position) {
    Compare8(cpu, A8(cpu), 0x80u);
    LoadAAbsolute8(memory, cpu, position, 0);
    if (!cpu->carry)
        Adc8(cpu, 0x1fu);
    else
        Sbc8(cpu, 0x1fu);
}

/* $86:99BF: stream the map edges the camera moved across. */
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapStreamEdges(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x869a43u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8699bfu;
        return result;
    }
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);          /* 99BF */
    Write8(memory, DirectAddress(cpu, 0x5bu), 0x00u);
    LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x5au);
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11f6u, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, 0x11f2u);
        StoreADirect8(memory, cpu, 0x58u);                     /* 99EA */
        WorldMapStreamColumn(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, 0x1711u);
    }
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);                   /* 99F4 */
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x58u);
    LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11f7u, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, 0x11f4u);
        StoreADirect8(memory, cpu, 0x5au);                     /* 9A1B */
        WorldMapStreamRow(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, 0x1710u);
    }
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);                   /* 9A25 */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x11f6u, 0));
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x11f7u, 0));
    }
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x11e3u, 0);               /* 9A35 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->carry)
            StoreAAbsolute8(memory, cpu, 0x11e3u, 0);
    }
    SimulateJsrFrame(memory, cpu, 0x9a42u);                    /* 9A44 */
    CopyAbsolute8(memory, cpu, 0x11f2u, 0x11f6u);
    CopyAbsolute8(memory, cpu, 0x11f4u, 0x11f7u);
    SimulateRtsFrame(memory, cpu);
    return result;
}

static Lufia2ActorPrimaryUpdateResult FieldLoopResult(uint32_t exit) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = exit;
    result.dispatches = 0;
    return result;
}

static Lufia2ActorPrimaryUpdateResult FieldLoopHandoff(
    Lufia2ActorFrontendCpu *cpu, uint32_t pc) {
    Lufia2ActorPrimaryUpdateResult result = FieldLoopResult(pc);

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    cpu->resume_pc = pc;
    return result;
}

/* $83:83A0: menu request; the menu itself runs on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldMenuRequest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x8383a0u);
    LoadA8(cpu, 0x40u);                                        /* 83A0 */
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 0);
    if (!cpu->zero)
        return FieldLoopHandoff(cpu, 0x8383bdu);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
    BitImmediate8(cpu, 0x02u);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* 83AE */
        LoadA16(cpu, 0x9080u);
        And16(cpu, Read16Direct(memory, cpu, 0x46u));
        if (!cpu->zero) {
            TestBitsDirect(memory, cpu, 0x4au, 0);
            if (!cpu->zero) {
                SetAccumulatorWidth(cpu, 1);
                return FieldLoopHandoff(cpu, 0x8383bdu);
            }
        }
        SetAccumulatorWidth(cpu, 1);                           /* 83DD */
    }
    return FieldLoopResult(0x8383dfu);
}

/* $83:867B: A & pressed buttons; consume them from $4A. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldTakeButtons(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x83867bu);
    And8(cpu, DirectByte(memory, cpu, 0x46u));                 /* 867B */
    if (!cpu->zero)
        TestBitsDirect(memory, cpu, 0x4au, 0);
    return FieldLoopResult(0x838681u);
}

/* $83:8103: $05B7 requests; any set request runs on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldStatusRequests(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x838103u);
    LoadAAbsolute8(memory, cpu, 0x09a8u, 0);                   /* 8103 */
    BitImmediate8(cpu, 0x08u);
    if (cpu->zero) {
        SetIndexWidth(cpu, 0);
        LoadAAbsolute8(memory, cpu, 0x05b7u, 0);               /* 810C */
        BitImmediate8(cpu, 0x02u);
        if (!cpu->zero)
            return FieldLoopHandoff(cpu, 0x838113u);
        BitImmediate8(cpu, 0x04u);                             /* 8119 */
        if (!cpu->zero)
            return FieldLoopHandoff(cpu, 0x83811du);
        BitImmediate8(cpu, 0x01u);                             /* 8123 */
        if (!cpu->zero)
            return FieldLoopHandoff(cpu, 0x838127u);
    }
    SetIndexWidth(cpu, 1);                                     /* 812B */
    return FieldLoopResult(0x83812du);
}

/* $82:E746: JSR $8028 inline table on $30; handlers on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2TitleStateDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint32_t table;
    uint16_t target;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x82e746u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x30u));               /* E746 */
    SimulateJsrFrame(memory, cpu, 0xe74au);
    SetAccumulatorWidth(cpu, 0);                               /* 8028 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    IncrementA16(cpu);
    TransferAToY(cpu);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x5du);
    SetAccumulatorWidth(cpu, 1);
    Push8(memory, cpu, 0x82u);                                 /* PHK */
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    table = Read16Direct(memory, cpu, 0x5du) |
        ((uint32_t)DirectByte(memory, cpu, 0x5fu) << 16);
    LoadA16(cpu, Read16Long(memory, (table + cpu->y) & 0x00ffffffu));
    StoreADirect16(memory, cpu, 0x60u);
    SetAccumulatorWidth(cpu, 1);
    target = (uint16_t)(Read8(memory, 0x000060u) |
        ((uint16_t)Read8(memory, 0x000061u) << 8));            /* JMP ($0060) */
    {
        Lufia2ActorPrimaryUpdateResult result =
            FieldLoopHandoff(cpu, 0x820000u | target);

        /* The ROM passed these at this S already. */
        result.dispatches = target == 0xe746u || target == 0xe748u ||
            (target >= 0x8031u && target <= 0x8041u && (target & 1u));
        return result;
    }
}

/* $83:85DC: field reload setup; loading runs on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldReloadSetup(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t cleared[7] = {
        0x099bu, 0x099cu, 0x1261u, 0x1262u, 0x1254u, 0x09a6u, 0x09adu};
    unsigned i;

    Push8(memory, cpu, PackStatus(cpu));                       /* 85DC */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x83u);                                 /* PHK */
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x4200u, 0);
    TransferDirectToA(cpu);
    Write8(memory, 0x7fd0ffu, A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x0562u, 0);
    StoreAAbsolute8(memory, cpu, 0x0563u, 0);
    LoadA8(cpu, 0x80u);
    StoreAAbsolute8(memory, cpu, 0x0583u, 0);
    StoreAAbsolute8(memory, cpu, 0x2100u, 0);
    TransferDirectToA(cpu);                                    /* 85FA */
    StoreADirect8(memory, cpu, 0x6au);
    StoreADirect8(memory, cpu, 0x6fu);
    Write8(memory, DirectAddress(cpu, 0x72u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x74u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x73u), 0x00u);
    StoreAAbsolute8(memory, cpu, 0x1255u, 0);
    StoreAAbsolute8(memory, cpu, 0x1256u, 0);
    StoreADirect8(memory, cpu, 0x81u);
    StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    LoadA8(cpu, 0xffu);                                        /* 8610 */
    Write8(memory, 0x7fd0b0u, A8(cpu));
    Write8(memory, 0x7fd4f5u, A8(cpu));
    StoreAAbsolute8(memory, cpu, 0x17acu, 0);
    for (i = 0; i < 4u; ++i)
        StoreZeroAbsolute8(memory, cpu, cleared[i], 0);
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x1269u, 0);
    for (i = 4; i < 7u; ++i)
        StoreZeroAbsolute8(memory, cpu, cleared[i], 0);
    /* Map loading from JSR $B062 on. */
    return FieldLoopHandoff(cpu, 0x838637u);
}

/* $82:8DA6: HDMA table [$F4] to [$F7], channel $F3 setup. */
static void MenuHdmaUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DA6 */
    And8(cpu, 0x01u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        LoadY16(cpu, 0x0000u);
        for (;;) {
            LoadA8(cpu, Read8(memory,                          /* 8DB3 */
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write8(memory, DirectLongIndirectY(memory, cpu, 0xf7u), A8(cpu));
            if (cpu->zero)
                break;
            IncrementY16(cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0xf7u),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* 8DC6 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0xf3u));
        And16(cpu, 0x00ffu);
        LoadY16(cpu, cpu->accumulator);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0xf7u));
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x4302u, cpu->y),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x4303u, cpu->y),
            (uint8_t)(cpu->accumulator >> 8));
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0xf9u));
        StoreAAbsolute8(memory, cpu, 0x4304u, cpu->y);
        LoadA8(cpu, DirectByte(memory, cpu, 0xfau));
        StoreAAbsolute8(memory, cpu, 0x4301u, cpu->y);
        LoadA8(cpu, DirectByte(memory, cpu, 0xfbu));
        StoreAAbsolute8(memory, cpu, 0x4300u, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DE9 */
    And8(cpu, 0x02u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        LoadA8(cpu, DirectByte(memory, cpu, 0xf2u));
        StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DF8 */
    And8(cpu, 0x04u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        StoreAImmediate8(memory, cpu, 0x04u, 0x212du);
        StoreAImmediate8(memory, cpu, 0x02u, 0x2130u);
        StoreAImmediate8(memory, cpu, 0x10u, 0x2131u);
    }
    SimulateRtlFrame(memory, cpu);
}

/* A += delta on the $7E:80C0 table entry at X. */
static void MenuTableStep8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    int delta) {
    const uint32_t address = LongIndexedAddress(0x7e80c0u, cpu->x);

    LoadA8(cpu, (uint8_t)(Read8(memory, address) + delta));
    Write8(memory, address, A8(cpu));
}

static void MenuTableStep16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    int delta) {
    const uint32_t address = LongIndexedAddress(0x7e80c0u, cpu->x);

    LoadA16(cpu, (uint16_t)(Read16Long(memory, address) + delta));
    Write16Long(memory, address, cpu->accumulator);
}

/* X += 3, 16-bit index. */
static void MenuTableNext(Lufia2ActorFrontendCpu *cpu) {
    IncrementX16(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* $82:8F14: grow the window rows by three lines. */
static void MenuWindowGrow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));  /* 8F14 */
    MenuTableStep8(memory, cpu, -3);
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
    IncrementX16(cpu);
    do {
        MenuTableStep16(memory, cpu, 3);                       /* 8F2B */
        MenuTableNext(cpu);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
    MenuTableStep8(memory, cpu, 3);
    IncrementX16(cpu);
    SetAccumulatorWidth(cpu, 0);
    MenuTableStep16(memory, cpu, 3);
    SetAccumulatorWidth(cpu, 1);
}

/* BIT #mask; true when set. */
static uint8_t MenuBit(Lufia2ActorFrontendCpu *cpu, uint8_t mask) {
    BitImmediate8(cpu, mask);
    return !cpu->zero;
}

/* $82:8E12: window HDMA line table in $7E:80C0 by $1566 bit. */
static void MenuWindowLines(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x93b4u);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 8E12 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadY16(cpu, 0x0000u);
        for (;;) {
            LoadA8(cpu, Read8(memory,                          /* 8E1F */
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            if (cpu->zero)
                break;
            Write8(memory, DirectLongIndirectY(memory, cpu, 0xf7u), A8(cpu));
            IncrementY16(cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0xf7u),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
    } else if (MenuBit(cpu, 0x02u)) {                      /* 8E33 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1532u, 0));
        Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1534u, 0));
        Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        IncrementY16(cpu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        IncrementX16(cpu);
        do {
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));    /* 8E56 */
            Subtract16(cpu, Read16Direct(memory, cpu, 0x33u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0009u);
            Write16Long(memory, LongIndexedAddress(0x7e80c0u, cpu->x),
                cpu->accumulator);
            MenuTableNext(cpu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            cpu->carry = 0;
            Add16Value(cpu, 0x000cu);
            Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            Compare16(cpu, cpu->accumulator,
                Read16AbsoluteIndexed(memory, cpu, 0x1536u, 0));
            if (cpu->zero) {
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1534u, 0));
                Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            }
            cpu->y = (uint16_t)(cpu->y - 1u);                  /* 8E80 */
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        LoadA8(cpu, 0x03u);
        Write8(memory, LongIndexedAddress(0x7e80c0u, cpu->x), A8(cpu));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
        LoadA8(cpu, 0x09u);
        Write8(memory, LongIndexedAddress(0x7e80c0u, cpu->x), A8(cpu));
    } else if (MenuBit(cpu, 0x04u)) {                      /* 8E98 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        MenuTableStep8(memory, cpu, 3);
        SetAccumulatorWidth(cpu, 0);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        IncrementX16(cpu);
        do {
            MenuTableStep16(memory, cpu, -3);                  /* 8EB8 */
            MenuTableNext(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
        cpu->x = (uint16_t)(cpu->x - 1u);
        SetNz16(cpu, cpu->x);
        MenuTableStep8(memory, cpu, -3);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        MenuTableStep16(memory, cpu, -3);
        SetAccumulatorWidth(cpu, 1);
    } else if (MenuBit(cpu, 0x08u)) {                      /* 8EE8 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
        MenuTableStep8(memory, cpu, -1);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        MenuTableStep16(memory, cpu, 1);
        SetAccumulatorWidth(cpu, 1);
        MenuWindowGrow(memory, cpu);
    } else if (MenuBit(cpu, 0x10u)) {                      /* 8F0D */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        MenuWindowGrow(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $82:97A6: step menu sprites along the $8E:E4D8 offset lists. */
static void MenuSpriteSteps(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x93bcu);
    SetIndexWidth(cpu, 1);                                     /* 97A6 */
    BattleSetDataBank(memory, cpu, 0x8eu);
    Write8(memory, DirectAddress(cpu, 0x33u), 0x00u);
    LoadX16(cpu, 0x0010u);
    do {
        LoadAAbsolute8(memory, cpu, 0x11d8u, cpu->x);          /* 97B1 */
        if (!cpu->zero) {
            cpu->y = Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x1208u, cpu->x));
            SetNz8(cpu, (uint8_t)cpu->y);
            LoadAAbsolute8(memory, cpu, 0xe4d8u, cpu->y);
            Compare8(cpu, A8(cpu), 0xaau);
            if (cpu->zero) {
                StoreZeroAbsolute8(memory, cpu, 0x11d8u, cpu->x);
            } else {
                const uint32_t step =
                    AbsoluteIndexedAddress(cpu, 0x1208u, cpu->x);
                uint8_t next;

                cpu->carry = 0;                                /* 97C5 */
                Adc8(cpu, Read8(memory,
                    AbsoluteIndexedAddress(cpu, 0x13e8u, cpu->x)));
                StoreAAbsolute8(memory, cpu, 0x13e8u, cpu->x);
                next = (uint8_t)(Read8(memory, step) + 1u);
                Write8(memory, step, next);
                SetNz8(cpu, next);
                LoadA8(cpu, 0x01u);
                StoreADirect8(memory, cpu, 0x33u);
            }
        }
        cpu->x = (uint8_t)(cpu->x + 1u);                       /* 97D3 */
        Compare8(cpu, (uint8_t)cpu->x, 0x30u);
    } while (!cpu->zero);
    LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
    if (cpu->zero)
        StoreZeroAbsolute8(memory, cpu, 0x1567u, 0);
    SetIndexWidth(cpu, 0);                                     /* 97DF */
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:8809: patch count/value pairs into a $7E:[$3B] HDMA table. */
static void SelectHdmaPatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t table,
    uint16_t return_address) {
    LoadY16(cpu, table);
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, 0x3bu, cpu->y);                 /* 8809 */
    LoadY16(cpu, 0x0000u);
    for (;;) {
        LoadA8(cpu, Read8(memory,                              /* 880E */
            DirectLongIndirectY(memory, cpu, 0x3bu)));
        if (cpu->zero)
            break;
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x1597u, cpu->x);
        Write8(memory, DirectLongIndirectY(memory, cpu, 0x3bu), A8(cpu));
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x15a7u, cpu->x);
        Write8(memory, DirectLongIndirectY(memory, cpu, 0x3bu), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* 16-bit STX/STY to an MMIO pair. */
static void StoreWordAbsolute(
    const Lufia2ActorFrontendMemory *memory,
    const Lufia2ActorFrontendCpu *cpu,
    uint16_t address,
    uint16_t value) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)value);
    Write8(memory, AbsoluteIndexedAddress(cpu, (uint16_t)(address + 1u), 0),
        (uint8_t)(value >> 8));
}

/* DMA channel 6: $7E:X to VRAM Y, count bytes. */
static void SelectVramDma(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t count) {
    StoreWordAbsolute(memory, cpu, 0x2116u, cpu->y);
    StoreWordAbsolute(memory, cpu, 0x4362u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
    LoadX16(cpu, count);
    StoreWordAbsolute(memory, cpu, 0x4365u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
}

/* $86:87E0: HDMA table patches and the $15B8 row upload. */
static void SelectNmiRedraw(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x81c1u);
    SetAccumulatorWidth(cpu, 1);                               /* 87E0 */
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);
    And8(cpu, 0x01u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, 0x0000u);
        LoadA8(cpu, 0x7eu);
        StoreADirect8(memory, cpu, 0x3du);
        SelectHdmaPatch(memory, cpu, 0x80c0u, 0x87fau);
        SelectHdmaPatch(memory, cpu, 0x81c0u, 0x8800u);
        SelectHdmaPatch(memory, cpu, 0x82c0u, 0x8806u);
    }
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 8823 */
    And8(cpu, 0x02u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1566u, 0);
        StoreAImmediate8(memory, cpu, 0x81u, 0x2115u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15b8u, 0));
        And16(cpu, 0x00ffu);
        cpu->carry = 0;
        Add16Value(cpu, 0x1800u);
        LoadY16(cpu, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0xc3c0u);
        SelectVramDma(memory, cpu, 0x003cu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $82:8D44: tilemap uploads by $1568 bits. */
static void SelectTilemapUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const struct {
        uint8_t mask;
        uint16_t source;
        uint16_t vram;
        uint16_t return_address;
    } uploads[3] = {
        {0xc0u, 0x2000u, 0x1000u, 0x8d5cu},
        {0x30u, 0x2800u, 0x1400u, 0x8d6cu},
        {0x0cu, 0x3000u, 0x1800u, 0x8d7cu}};
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x86u, 0x81cau);
    SetAccumulatorWidth(cpu, 1);                               /* 8D44 */
    SetIndexWidth(cpu, 0);
    StoreAImmediate8(memory, cpu, 0x80u, 0x2115u);
    for (i = 0; i < 3u; ++i) {
        LoadAAbsolute8(memory, cpu, 0x1568u, 0);
        And8(cpu, uploads[i].mask);
        if (cpu->zero)
            continue;
        LoadX16(cpu, uploads[i].source);
        LoadY16(cpu, uploads[i].vram);
        SimulateJsrFrame(memory, cpu, uploads[i].return_address);
        PushAccumulator8(memory, cpu);                         /* 8D83 */
        SelectVramDma(memory, cpu, 0x0800u);
        LoadA8(cpu, Pull8(memory, cpu));
        SimulateRtsFrame(memory, cpu);
    }
    LoadA8(cpu, 0xaau);                                        /* 8D7D */
    TestBitsAbsolute8(memory, cpu, 0x1568u, 0);
    SimulateRtlFrame(memory, cpu);
}

/* $82:939C: menu NMI with its three redraw requests. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->accumulator_is_8_bit)                             /* 939C */
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);
    if (!cpu->zero)
        MenuHdmaUpdate(memory, cpu, 0x82u, 0x93acu);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 93AD */
    if (!cpu->zero)
        MenuWindowLines(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1567u, 0);                   /* 93B5 */
    if (!cpu->zero)
        MenuSpriteSteps(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 93BD */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return FieldLoopResult(0x8293c1u);
}

/* $86:81A9: NMI installed by $86:8000. */
Lufia2ActorPrimaryUpdateResult Lufia2SelectScreenNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->accumulator_is_8_bit)                             /* 81A9 */
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);
    if (!cpu->zero)
        MenuHdmaUpdate(memory, cpu, 0x86u, 0x81b9u);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 81BA */
    if (!cpu->zero)
        SelectNmiRedraw(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1568u, 0);                   /* 81C2 */
    if (!cpu->zero)
        SelectTilemapUploads(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 81CB */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return FieldLoopResult(0x8681cfu);
}

/* $80:9357: DMA channel 6, $700 bytes from $7E:X to VRAM Y. */
static void IntroVramDma(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreWordAbsolute(memory, cpu, 0x4362u, cpu->x);           /* 9357 */
    StoreWordAbsolute(memory, cpu, 0x2116u, cpu->y);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
    LoadX16(cpu, 0x0700u);
    StoreWordAbsolute(memory, cpu, 0x4365u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
    StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
    SimulateRtsFrame(memory, cpu);
}

/* $80:92FE/$80:9346: logo tiles from $7E:X, then the next state. */
static void IntroLogoUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t source,
    uint16_t return_address) {
    LoadX16(cpu, source);
    LoadY16(cpu, 0x0000u);
    IntroVramDma(memory, cpu, return_address);
    IncrementDirect8(memory, cpu, 0x50u);
    Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
}

/* $80:92A4: intro NMI, state $50 through the table $80:92B7. */
Lufia2ActorPrimaryUpdateResult Lufia2IntroNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t handler;

    Push8(memory, cpu, PackStatus(cpu));                       /* 92A4 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x50u));
    AslA8(cpu);
    TransferAToX(cpu);
    handler = (uint16_t)(
        Read8(memory, ProgramAddress(cpu, (uint16_t)(0x92b7u + cpu->x))) |
        (Read8(memory, ProgramAddress(cpu, (uint16_t)(0x92b8u + cpu->x)))
            << 8));
    if (handler != 0x92cbu && handler != 0x92feu && handler != 0x930cu &&
        handler != 0x9320u && handler != 0x9330u && handler != 0x9346u &&
        handler != 0x9354u)
        return FieldLoopHandoff(cpu, 0x8092b1u);
    SimulateJsrFrame(memory, cpu, 0x92b3u);
    switch (handler) {
    case 0x92cbu:                                  /* scroll row upload */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7e4004u));
        cpu->carry = 0;
        Add16Value(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, 0x4362u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7e4006u));
        StoreWordAbsolute(memory, cpu, 0x4365u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, 0x2116u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
        StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
        StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
        StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
        IncrementDirect8(memory, cpu, 0x50u);
        break;
    case 0x92feu:
        IntroLogoUpload(memory, cpu, 0x2000u, 0x9306u);
        break;
    case 0x930cu:                                  /* fade in over 32 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x4eu));
        LsrA8(cpu);
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) + 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        Compare8(cpu, A8(cpu), 0x20u);
        if (cpu->carry) {
            IncrementDirect8(memory, cpu, 0x50u);
            Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
        }
        break;
    case 0x9320u:                                  /* hold 120 frames */
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) + 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        Compare8(cpu, A8(cpu), 0x78u);
        if (cpu->carry) {
            LoadA8(cpu, 0x20u);
            StoreADirect8(memory, cpu, 0x4eu);
            IncrementDirect8(memory, cpu, 0x50u);
        }
        break;
    case 0x9330u:                                  /* fade out */
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) - 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        if (cpu->negative) {
            Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
            IncrementDirect8(memory, cpu, 0x50u);
            StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
        } else {
            LsrA8(cpu);
            StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        }
        break;
    case 0x9346u:
        IntroLogoUpload(memory, cpu, 0x2800u, 0x934eu);
        break;
    default:                                       /* 9354 */
        Write8(memory, DirectAddress(cpu, 0x6au), 0x00u);
        break;
    }
    SimulateRtsFrame(memory, cpu);
    PullDataBank(memory, cpu);                                 /* 92B4 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return FieldLoopResult(0x8092b6u);
}

/* $85:C168: battler byte A to its variable base in X. */
static void BattleScriptSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreAAbsolute8(memory, cpu, 0x09fbu, 0);                  /* C168 */
    BitImmediate8(cpu, 0x3fu);
    if (!cpu->zero) {
        And8(cpu, 0x80u);
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
        StoreAAbsolute8(memory, cpu, 0x09fau, 0);
        LoadA8(cpu, 0xffu);
        do {
            const uint32_t bits = AbsoluteIndexedAddress(cpu, 0x09fbu, 0);
            const uint8_t value = Read8(memory, bits);

            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* C17A */
            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            SetNz8(cpu, (uint8_t)(value >> 1));
        } while (!cpu->carry);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fau, 0)));
    }
    SetAccumulatorWidth(cpu, 0);                               /* C184 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a13u, 0));
    And16(cpu, 0x00ffu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        cpu->zero ? 0x0a80u : 0x0a64u, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $85:C4DE: variable bases $C1 and $BE. */
static void BattleScriptBases(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xb462u);
    LoadA8(cpu, Read8(memory, 0x7ff450u));                     /* C4DE */
    BattleScriptSlot(memory, cpu, 0xc4e4u);
    Write16Direct(memory, cpu, 0xc1u, cpu->x);
    LoadA8(cpu, Read8(memory, 0x7ff44eu));
    BattleScriptSlot(memory, cpu, 0xc4edu);
    Write16Direct(memory, cpu, 0xbeu, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* INC $BB, 16-bit. */
static void BattleScriptAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t pointer = (uint16_t)(Read16Direct(memory, cpu, 0xbbu) + 1u);

    Write16Direct(memory, cpu, 0xbbu, pointer);
    SetNz16(cpu, pointer);
}

/* $85:BFBF: next script byte into A; flags kept. */
static void BattleScriptByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFBF */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    SetAccumulatorWidth(cpu, 0);
    BattleScriptAdvance(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFCA: next script word into X. */
static void BattleScriptWord(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFCA */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    BattleScriptAdvance(memory, cpu);
    BattleScriptAdvance(memory, cpu);
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Selector bit 7: local slot at base $C1 or $BE. */
static uint16_t BattleScriptLocal(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a52u, 0));
    And16(cpu, 0x00ffu);
    {
        const uint8_t base = cpu->zero ? 0xbeu : 0xc1u;

        LoadA16(cpu, (uint16_t)(
            Read8(memory, (uint16_t)(cpu->stack + 1u)) |
            (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
        And16(cpu, 0x007fu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, base));
    }
    return cpu->accumulator;
}

/* $85:BFED: read variable A (bit 7 local) into X. */
static void BattleScriptRead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFED */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x)));
    } else {
        BattleScriptLocal(memory, cpu);                        /* C001 */
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0015u, cpu->x));
    }
    TransferAToX(cpu);                                         /* C01F */
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C023: write X to variable A. */
static void BattleScriptWrite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C023 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushY(memory, cpu);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x),
            cpu->accumulator);
    } else {
        BattleScriptLocal(memory, cpu);                        /* C03A */
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0015u, cpu->x),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0016u, cpu->x),
            (uint8_t)(cpu->accumulator >> 8));
    }
    TransferAToX(cpu);                                         /* C05A */
    PullAccumulator16(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFD8: next word, bit 15 = variable, into X. */
static void BattleScriptValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFD8 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    if (cpu->negative)
        BattleScriptRead(memory, cpu, 0xbfe2u);
    else
        TransferAToX(cpu);
    BattleScriptAdvance(memory, cpu);                          /* BFE6 */
    BattleScriptAdvance(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:B551: $BB = $0A42 + next word. */
static void BattleScriptJump(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* B551 */
    BattleScriptWord(memory, cpu, 0xb555u);
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
    Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
}

/* Operand fetch shared by the compare and math opcodes. */
static void BattleScriptOperands(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 5u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(opcode + 10u));
}

/* Signed $54 - X, 16-bit, overflow kept. */
static void BattleScriptCompare(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
}

/* Store binary result X into the destination variable. */
static void BattleScriptStoreBinary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t value,
    uint16_t return_address) {
    LoadA16(cpu, value);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu, return_address);
}

/* Unary opcodes: destination byte, source variable, result X. */
static void BattleScriptUnary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode,
    uint8_t kind) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    PushAccumulator8(memory, cpu);
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 6u));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    if (kind == 0) {                                           /* abs */
        if (cpu->negative) {
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        }
        TransferAToX(cpu);
    } else if (kind == 1) {                                    /* negate */
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        TransferAToX(cpu);
    } else if (!cpu->zero) {                                   /* sign */
        LoadX16(cpu, cpu->negative ? 0xffffu : 0x0001u);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu,
        (uint16_t)(opcode + (kind == 0 ? 22u : kind == 1 ? 20u : 27u)));
}

/* INC $66, 16-bit. */
static void BattleIncrement66(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, 0x66u) + 1u);

    Write16Direct(memory, cpu, 0x66u, value);
    SetNz16(cpu, value);
}

/* $85:DCA3: $63-$66 = $54 * $56, 16x16 via $4202. */
static void BattleMultiply(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCA3 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    SetAccumulatorWidth(cpu, 0);
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x63u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x55u));
    cpu->carry = 0;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->x);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x64u));
    if (cpu->carry) {
        BattleIncrement66(memory, cpu);
        cpu->carry = 0;
    }
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    if (cpu->carry)
        BattleIncrement66(memory, cpu);
    Write16Direct(memory, cpu, 0x64u, cpu->accumulator);       /* DCE6 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DC6F: $5D-$5F /= $54, 24/8 via $4204. */
static void BattleDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    PushDataBank(memory, cpu);                                 /* DC6F */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5eu));
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    PushIndex(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    LoadAAbsolute8(memory, cpu, 0x4216u, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadY16(cpu, cpu->accumulator);
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    LoadA8(cpu, (uint8_t)cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCEA: A times a random 16-bit fraction. */
static void BattleRandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    static const struct {
        uint8_t limit;
        uint16_t call;
    } rolls[3] = {{0x80u, 0xdcf7u}, {0x80u, 0xdcffu}, {0x04u, 0xdd07u}};
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCEA */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        StoreADirect8(memory, cpu, 0x54u);
    else
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, rolls[i].limit);
        SimulateJslFrame(memory, cpu, 0x85u, rolls[i].call);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (i < 2u)
            StoreADirect8(memory, cpu, i == 0 ? 0x56u : 0x57u);
    }
    for (i = 0; i < 2u; ++i) {
        const uint32_t address = DirectAddress(cpu, (uint8_t)(0x56u + i));
        const uint8_t value = Read8(memory, address);
        const uint8_t in = (uint8_t)(A8(cpu) & 1u);

        LsrA8(cpu);                                            /* DD08 */
        cpu->carry = value >> 7;
        Write8(memory, address, (uint8_t)((value << 1) | in));
        SetNz8(cpu, (uint8_t)((value << 1) | in));
    }
    BattleMultiply(memory, cpu, 0xdd11u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x65u));
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DB6D: 32/16 shift-subtract divide of $63-$66 by $58. */
static void BattleLongDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DB6D */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x66u));
    And16(cpu, 0x00ffu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x64u));
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Direct(memory, cpu, 0x64u, cpu->x);
    for (i = 0; i < 16u; ++i) {
        const uint16_t low = Read16Direct(memory, cpu, 0x63u);
        const uint16_t high = Read16Direct(memory, cpu, 0x65u);
        const uint16_t a = cpu->accumulator;

        Write16Direct(memory, cpu, 0x63u, (uint16_t)(low << 1));
        Write16Direct(memory, cpu, 0x65u,
            (uint16_t)((high << 1) | (low >> 15)));
        LoadA16(cpu, (uint16_t)((a << 1) | (high >> 15)));
        cpu->carry = a >> 15;
        if (!cpu->carry) {
            Compare16(cpu, cpu->accumulator,
                Read16Direct(memory, cpu, 0x58u));
            if (!cpu->carry)
                continue;
        }
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x58u));
        Write16Direct(memory, cpu, 0x63u,
            (uint16_t)(Read16Direct(memory, cpu, 0x63u) + 1u));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* DC6D */
    SimulateRtlFrame(memory, cpu);
}

/* $85:C099: X = $85:9E47 word for index A. */
static void BattleStatOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C099 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadX16(cpu, Read16Long(memory, LongIndexedAddress(0x859e47u, cpu->x)));
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C05F: X = stat A of battler ($BE); bytes at $0E/$BC. */
static void BattleStat(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C05F */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushY(memory, cpu);
    BattleStatOffset(memory, cpu, 0xc066u);
    LoadY16(cpu, cpu->x);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        Read16Direct(memory, cpu, 0xbeu), cpu->y));
    if (cpu->y == 0x00bcu || cpu->y == 0x000eu)
        And16(cpu, 0x00ffu);
    else
        Compare16(cpu, cpu->y, 0x000eu);
    TransferAToX(cpu);
    cpu->y = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C117: mask of battlers without status bit 2. */
static void BattleActiveMask(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    const uint8_t enemies = (uint8_t)(A8(cpu) & 0x80u);

    SimulateJsrFrame(memory, cpu, return_address);
    StoreZeroAbsolute8(memory, cpu, 0x09fau, 0);               /* C117 */
    cpu->zero = enemies == 0;
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, enemies ? 0x000au : 0x0008u);
    do {
        const uint32_t mask = AbsoluteIndexedAddress(cpu, 0x09fau, 0);
        uint16_t bits;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            enemies ? 0x0a6eu : 0x0a64u, cpu->x));
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadY16(cpu, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y));
            cpu->zero = (cpu->accumulator & 0x0004u) == 0;
            cpu->carry = cpu->zero;
        }
        bits = (uint16_t)(Read8(memory, mask) |
            (Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0)) << 8));
        {
            const uint8_t out = (uint8_t)(bits >> 15);

            bits = (uint16_t)((bits << 1) | cpu->carry);
            cpu->carry = out;
        }
        Write8(memory, mask, (uint8_t)bits);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0),
            (uint8_t)(bits >> 8));
        SetNz16(cpu, bits);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x09fau, 0);
    if (enemies)
        Or8(cpu, 0x80u);
    SimulateRtsFrame(memory, cpu);
}

/* $85:B452: battle script VM; other opcodes run on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;
    unsigned opcodes;

    PushDataBank(memory, cpu);                                 /* B452 */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x0a60u, 0);
    for (opcodes = 0;; ++opcodes) {
        uint16_t handler;

        SetAccumulatorWidth(cpu, 1);                           /* B45E */
        BattleScriptBases(memory, cpu);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        BattleScriptAdvance(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        Push8(memory, cpu, 0x85u);
        PullDataBank(memory, cpu);
        handler = (uint16_t)(
            Read8(memory, 0x850000u | (uint16_t)(0xb483u + cpu->x)) |
            (Read8(memory, 0x850000u | (uint16_t)(0xb484u + cpu->x)) << 8));
        switch (opcodes < 4096u ? handler : 0u) {
        case 0xb473u:                                          /* end */
        case 0xb476u:                                          /* end, $FF */
            if (handler == 0xb473u)
                TransferDirectToA(cpu);
            else
                LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x0a5bu, 0);          /* B478 */
            SetAccumulatorWidth(cpu, 0);
            SetIndexWidth(cpu, 0);
            cpu->y = PullIndexValue(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
            PullAccumulator16(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            PullDataBank(memory, cpu);
            return FieldLoopResult(0x85b482u);
        case 0xb551u:                                          /* jump */
            BattleScriptJump(memory, cpu);
            break;
        case 0xb582u:                                          /* random */
            BattleScriptByte(memory, cpu, 0xb584u);
            StoreADirect8(memory, cpu, 0x54u);
            LoadA8(cpu, 0xffu);
            SimulateJslFrame(memory, cpu, 0x85u, 0xb58cu);
            Lufia2RandomScale(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
            if (!cpu->carry)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb594u);
            break;
        case 0xb598u:                                          /* equal */
        case 0xb5adu:                                          /* not equal */
            BattleScriptOperands(memory, cpu, handler);
            Compare16(cpu, cpu->x, Read16Direct(memory, cpu, 0x54u));
            if (cpu->zero == (handler == 0xb598u))
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, (uint16_t)(handler + 17u));
            break;
        case 0xb60au:                                          /* >= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb629u);
            break;
        case 0xb62du:                                          /* <= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->zero || cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb64eu);
            break;
        case 0xb670u:                                          /* set */
            BattleScriptByte(memory, cpu, 0xb672u);
            BattleScriptValue(memory, cpu, 0xb675u);
            BattleScriptWrite(memory, cpu, 0xb678u);
            break;
        case 0xb67cu:                                          /* add */
        case 0xb698u:                                          /* subtract */
            BattleScriptOperands(memory, cpu, handler);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            if (handler == 0xb67cu) {
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            } else {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Direct(memory, cpu, 0x54u, cpu->x);
                cpu->carry = 1;
                Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
            }
            BattleScriptStoreBinary(memory, cpu, cpu->accumulator,
                handler == 0xb67cu ? 0xb694u : 0xb6b3u);
            break;
        case 0xb849u:                                          /* and */
        case 0xb864u:                                          /* or */
        case 0xb87fu: {                                        /* xor */
            uint16_t value;

            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            PushAccumulator8(memory, cpu);
            BattleScriptRead(memory, cpu, (uint16_t)(handler + 6u));
            Write16Direct(memory, cpu, 0x54u, cpu->x);
            BattleScriptValue(memory, cpu, (uint16_t)(handler + 11u));
            SetAccumulatorWidth(cpu, 0);
            value = Read16Direct(memory, cpu, 0x54u);
            value = handler == 0xb849u ? (uint16_t)(cpu->x & value)
                : handler == 0xb864u ? (uint16_t)(cpu->x | value)
                : (uint16_t)(cpu->x ^ value);
            BattleScriptStoreBinary(memory, cpu, value,
                (uint16_t)(handler + 0x17u));
            break;
        }
        case 0xb89au:
            BattleScriptUnary(memory, cpu, handler, 0);
            break;
        case 0xb8b4u:
            BattleScriptUnary(memory, cpu, handler, 1);
            break;
        case 0xb8ccu:
            BattleScriptUnary(memory, cpu, handler, 2);
            break;
        case 0xb8ebu:                                          /* leader id */
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e8u, 0));
            BattleScriptByte(memory, cpu, 0xb8f0u);
            BattleScriptWrite(memory, cpu, 0xb8f3u);
            break;
        case 0xb91fu:                                          /* $7F:F45C */
            BattleScriptWord(memory, cpu, 0xb921u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            break;
        case 0xb92cu:                                          /* $7F:F45E */
            BattleScriptByte(memory, cpu, 0xb92eu);
            Write8(memory, 0x7ff45eu, A8(cpu));
            BattleScriptByte(memory, cpu, 0xb935u);
            Write8(memory, 0x7ff460u, A8(cpu));
            break;
        case 0xb560u:                                          /* jump if F42E */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb56au);
            break;
        case 0xb5c2u:                                          /* > */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (!cpu->zero && cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb5e3u);
            break;
        case 0xb5e7u:                                          /* < */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb606u);
            break;
        case 0xb93du:                                          /* move setup */
            BattleScriptWord(memory, cpu, 0xb93fu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb957u);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb96au);
            break;
        case 0xb9b8u:                                          /* step setup */
            BattleScriptWord(memory, cpu, 0xb9bau);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            break;
        case 0xb9dau:                                          /* F44E word+byte */
            BattleScriptByte(memory, cpu, 0xb9dcu);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            PushAccumulator16(memory, cpu);
            BattleScriptValue(memory, cpu, 0xb9edu);
            LoadA16(cpu, cpu->x);
            cpu->x = PullIndexValue(memory, cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu, 0xb9fau);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba02u:                                          /* F44E byte, clear */
            BattleScriptByte(memory, cpu, 0xba04u);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0004u);
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xba19u);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            cpu->x = (uint16_t)(cpu->x - 2u);
            SetNz16(cpu, cpu->x);
            TransferDirectToA(cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            break;
        case 0xba28u:                                          /* F44E byte */
        case 0xba47u:
            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f0fu, cpu->x)));
            And16(cpu, 0x00ffu);
            if (handler == 0xba28u)
                LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu,
                handler == 0xba28u ? 0xba3fu : 0xba5cu);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba64u:                                          /* action 1 */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            LoadA16(cpu, 0x0000u);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0xffu, 0x1262u);
            StoreZeroAbsolute8(memory, cpu, 0x1269u, 0);
            break;
        case 0xba81u:                                          /* action 4 */
        case 0xba8au:                                          /* action 6 */
        case 0xbb46u:                                          /* action 13 */
        case 0xbd6eu:                                          /* action 12 */
            LoadA8(cpu, handler == 0xba81u ? 0x04u : handler == 0xba8au
                ? 0x06u : handler == 0xbb46u ? 0x0du : 0x0cu);
            Write8(memory, 0x7ff454u, A8(cpu));
            break;
        case 0xba93u:                                          /* action 3 */
            LoadA8(cpu, 0x03u);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptWord(memory, cpu, 0xba9bu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            break;
        case 0xbd5eu:                                          /* action 11 */
            LoadA8(cpu, 0x0bu);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptByte(memory, cpu, 0xbd66u);
            Write8(memory, 0x7ff456u, A8(cpu));
            break;
        case 0xbb4fu:                                          /* party flag count */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            if (cpu->negative) {
                LoadX16(cpu, 0x0000u);
            } else {
                Write8(memory, DirectAddress(cpu, 0x54u), 0x00u);
                LoadY16(cpu, 0x0008u);
                do {
                    LoadX16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0x0a64u, cpu->y));        /* BB5F */
                    if (!cpu->zero) {
                        LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
                        BitImmediate8(cpu, 0x04u);
                        if (!cpu->zero)
                            IncrementDirect8(memory, cpu, 0x54u);
                    }
                    cpu->y = (uint16_t)(cpu->y - 2u);
                    SetNz16(cpu, cpu->y);
                } while (!cpu->negative);
                TransferDirectToA(cpu);
                LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
                TransferAToX(cpu);
            }
            BattleScriptByte(memory, cpu, 0xbb77u);
            BattleScriptWrite(memory, cpu, 0xbb7au);
            break;
        case 0xbb7eu:                                          /* side leader */
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, 0x7ff44eu));
            if (cpu->negative) {
                LoadAAbsolute8(memory, cpu, 0x15feu, 0);
            } else {
                LoadAAbsolute8(memory, cpu, 0x0a13u, 0);
                LoadAAbsolute8(memory, cpu,
                    cpu->zero ? 0x0a7au : 0x153cu, 0);
            }
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xbb9au);
            BattleScriptWrite(memory, cpu, 0xbb9du);
            break;
        case 0xbccfu:                                          /* $0A62 byte */
            BattleScriptByte(memory, cpu, 0xbcd1u);
            StoreAAbsolute8(memory, cpu, 0x0a62u, 0);
            break;
        case 0xbcd8u:                                          /* $0A62 if F42E */
            BattleScriptByte(memory, cpu, 0xbcdau);
            StoreADirect8(memory, cpu, 0x54u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Absolute(memory, cpu, 0x0a62u, cpu->accumulator);
            }
            break;
        case 0xbe22u:                                          /* timer $1264 */
            BattleScriptValue(memory, cpu, 0xbe24u);
            Write16Absolute(memory, cpu, 0x1264u, cpu->x);
            break;
        case 0xbe68u:                                          /* F450 mask */
            LoadAAbsolute8(memory, cpu, 0x0a5du, 0);
            if (!cpu->negative) {
                LoadA8(cpu, (uint8_t)(Read8(memory, 0x7ff450u) | 0xefu));
                LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
                    AbsoluteIndexedAddress(cpu, 0x0a5du, 0))));
                Write8(memory, 0x7ff450u, A8(cpu));
            }
            break;
        case 0xb6b7u:                                          /* multiply */
            BattleScriptOperands(memory, cpu, handler);
            Write16Direct(memory, cpu, 0x56u, cpu->x);
            PushAccumulator8(memory, cpu);
            BattleMultiply(memory, cpu, 0xb6c8u);
            LoadA8(cpu, Pull8(memory, cpu));
            LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
            BattleScriptWrite(memory, cpu, 0xb6ceu);
            break;
        case 0xb6d2u: {                                        /* divide */
            uint8_t negative;

            BattleScriptByte(memory, cpu, 0xb6d4u);
            BattleScriptRead(memory, cpu, 0xb6d7u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            negative = cpu->negative;
            if (negative)
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptValue(memory, cpu, negative ? 0xb701u : 0xb6e4u);
            LoadA8(cpu, (uint8_t)cpu->x);
            StoreADirect8(memory, cpu, 0x54u);
            Write8(memory, DirectAddress(cpu, 0x5fu), 0x00u);
            BattleDivide(memory, cpu, negative ? 0xb70au : 0xb6edu);
            if (negative) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, (uint16_t)(0u - Read16Direct(memory, cpu, 0x5du)));
                TransferAToX(cpu);
                SetAccumulatorWidth(cpu, 1);
            } else {
                LoadX16(cpu, Read16Direct(memory, cpu, 0x5du));
            }
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, negative ? 0xb719u : 0xb6f3u);
            break;
        }
        case 0xb73bu:                                          /* random scale */
            BattleScriptByte(memory, cpu, 0xb73du);
            BattleScriptRead(memory, cpu, 0xb740u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            BattleRandomScale(memory, cpu, 0xb748u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb74fu);
            break;
        case 0xb753u:                                          /* long divide */
            BattleScriptByte(memory, cpu, 0xb755u);
            BattleScriptRead(memory, cpu, 0xb758u);
            PushAccumulator8(memory, cpu);
            Write16Direct(memory, cpu, 0x58u, cpu->x);
            BattleScriptByte(memory, cpu, 0xb75eu);
            BattleScriptRead(memory, cpu, 0xb761u);
            Write16Direct(memory, cpu, 0x65u, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            Write16Direct(memory, cpu, 0x63u, 0x0000u);
            BattleLongDivide(memory, cpu, 0xb76bu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x64u));
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb777u);
            break;
        case 0xb96eu:                                          /* move by stats */
            BattleScriptWord(memory, cpu, 0xb970u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x01dcu);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x000au);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb988u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            LoadA16(cpu, 0x0008u);
            BattleStat(memory, cpu, 0xb990u);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, 0x0011u);
            BattleStat(memory, cpu, 0xb99cu);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb9b4u);
            break;
        case 0xbcedu:                                          /* jump back */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            TransferDirectToA(cpu);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (cpu->zero) {
                LoadA16(cpu, 0x0004u);
                BattleStat(memory, cpu, 0xbd06u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x7ff4cau));
                PushAccumulator16(memory, cpu);
                LoadA16(cpu, 0x000du);
                BattleStat(memory, cpu, 0xbd13u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, (uint16_t)(
                    Read8(memory, (uint16_t)(cpu->stack + 1u)) |
                    (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
                PullAccumulator16(memory, cpu);
            }
            LoadA16(cpu, 0x0040u);                             /* BD21 */
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            break;
        case 0xbd2cu:                                          /* extend jump */
            LoadA8(cpu, 0x08u);
            BattleStat(memory, cpu, 0xbd30u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (!cpu->zero) {
                if (cpu->negative) {
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                } else {
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                }
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
            }
            break;
        case 0xbe56u:                                          /* living mask */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            BattleActiveMask(memory, cpu, 0xbe5cu);
            LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory, 0x7ff450u)));
            Write8(memory, 0x7ff450u, A8(cpu));
            break;
        case 0xbd77u:                                          /* call */
            BattleScriptWord(memory, cpu, 0xbd79u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            AslA16(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadY16(cpu, Read16Direct(memory, cpu, 0xbbu));
            Write16Absolute(memory, cpu, 0x0a48u, cpu->y);
            LoadA8(cpu, DirectByte(memory, cpu, 0xbdu));
            StoreAAbsolute8(memory, cpu, 0x0a4au, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
            Write16Absolute(memory, cpu, 0x0a45u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a44u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a47u, 0);
            LoadA8(cpu, 0x96u);
            StoreADirect8(memory, cpu, 0xbdu);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(0x96faddu, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0xfaddu);
            Write16Absolute(memory, cpu, 0x0a42u, cpu->accumulator);
            Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
            break;
        case 0xbdb2u:                                          /* return */
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a48u, 0));
            Write16Direct(memory, cpu, 0xbbu, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a4au, 0);
            StoreADirect8(memory, cpu, 0xbdu);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a45u, 0));
            Write16Absolute(memory, cpu, 0x0a42u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a47u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            break;
        default:                                               /* B470 */
            result = FieldLoopHandoff(cpu, 0x85b470u);
            result.dispatches = opcodes;
            return result;
        }
    }
}

/* $82:8B4B: menu buttons into $14AB/$14AC; carry = none. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuButtons(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint8_t bits[12] = {
        0x80u, 0x40u, 0x20u, 0x10u,
        0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};
    unsigned i;

    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x828b4bu);
    StoreZeroAbsolute8(memory, cpu, 0x14abu, 0);               /* 8B4B */
    StoreZeroAbsolute8(memory, cpu, 0x14acu, 0);
    for (i = 0; i < 12u; ++i) {
        const uint8_t latch = i < 4u ? 0x4au : 0x4bu;

        LoadA8(cpu, bits[i]);
        And8(cpu, DirectByte(memory, cpu, latch));
        if (cpu->zero)
            continue;
        LoadA8(cpu, bits[i]);
        And8(cpu, DirectByte(memory, cpu, (uint8_t)(latch - 4u)));
        if (cpu->zero)
            continue;
        TestBitsDirect(memory, cpu, latch, 0);
        LoadA8(cpu, bits[i]);
        TestBitsAbsolute8(memory, cpu, i < 4u ? 0x14abu : 0x14acu, 1);
    }
    LoadAAbsolute8(memory, cpu, 0x14abu, 0);                   /* 8C35 */
    if (cpu->zero)
        LoadAAbsolute8(memory, cpu, 0x14acu, 0);
    cpu->carry = cpu->zero;
    return FieldLoopResult(cpu->zero ? 0x828c40u : 0x828c42u);
}

/* $82:9313: menu window refresh request; the upload runs on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuWindowRequest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x829313u);
    LoadAAbsolute8(memory, cpu, 0x156au, 0);                   /* 9313 */
    if (cpu->zero)
        return FieldLoopResult(0x82932fu);
    StoreAImmediate8(memory, cpu, 0x20u, 0x0564u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd4deu);
    LoadX16(cpu, 0x35e0u);
    return FieldLoopHandoff(cpu, 0x829327u);
}

/* $82:C627: menu cursor blink, toggles $1552 every $20 frames. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuCursorBlink(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x82c627u);
    LoadAAbsolute8(memory, cpu, 0x09c8u, 0);                   /* C627 */
    if (!cpu->zero)
        LoadAAbsolute8(memory, cpu, 0x1553u, 0);
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1554u, 0);
        const uint8_t count = (uint8_t)(Read8(memory, timer) + 1u);

        Write8(memory, timer, count);
        LoadAAbsolute8(memory, cpu, 0x1554u, 0);
        Compare8(cpu, A8(cpu), 0x20u);
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x1554u, 0);
            LoadAAbsolute8(memory, cpu, 0x1552u, 0);
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0x01u));
            StoreAAbsolute8(memory, cpu, 0x1552u, 0);
        }
    }
    return FieldLoopResult(0x82c646u);
}

/* $83:B882: first $7E:F000 rectangle holding ($8F, $91); 0 = handoff. */
static uint8_t FieldRectSearch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0);                               /* B882 */
    StoreYDirect16(memory, cpu, 0x54u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
    TransferAToX(cpu);
    for (entries = 0;; ++entries) {
        unsigned i;
        uint8_t inside = 1;

        /* A list without $FF spins the ROM. */
        if (entries == 0x10000u) {
            cpu->resume_pc = 0x83b88bu;
            return 0;
        }
        SetAccumulatorWidth(cpu, 1);                           /* B88B */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7ef000u, cpu->x)));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            cpu->carry = 0;
            break;
        }
        for (i = 0; i < 4u && inside; ++i) {
            if (i == 0)
                LoadA8(cpu, DirectByte(memory, cpu, 0x8fu));
            else if (i == 2)
                LoadA8(cpu, DirectByte(memory, cpu, 0x91u));
            Compare8(cpu, A8(cpu), Read8(memory,
                LongIndexedAddress(0x7ef000u + edges[i], cpu->x)));
            inside = (i & 1u) ? !cpu->carry : cpu->carry;
        }
        if (inside) {
            cpu->carry = 1;                                    /* B8B2 */
            break;
        }
        SetAccumulatorWidth(cpu, 0);                           /* B8B5 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
        TransferAToX(cpu);
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

static Lufia2ActorPrimaryUpdateResult FieldRectHandoff(
    Lufia2ActorFrontendCpu *cpu) {
    return FieldLoopHandoff(cpu, cpu->resume_pc);
}

/* $83:B66E: stair and slope rectangles; sets $05B5 bit 5. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldStairRects(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x83b66eu);
    LoadX16(cpu, 0x0002u);                                     /* B66E */
    LoadY16(cpu, 0x000fu);
    if (!FieldRectSearch(memory, cpu, 0xb676u))
        return FieldRectHandoff(cpu);
    if (cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0xf00eu, cpu->x);          /* B685 */
        if (cpu->zero)
            return FieldLoopResult(0x83b710u);
    } else {
        LoadX16(cpu, 0x000au);                                 /* B679 */
        LoadY16(cpu, 0x0005u);
        if (!FieldRectSearch(memory, cpu, 0xb681u))
            return FieldRectHandoff(cpu);
        if (!cpu->carry)
            return FieldLoopResult(0x83b684u);
    }
    LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);              /* B68D */
    StoreADirect8(memory, cpu, 0x54u);
    LoadA8(cpu, Read8(memory, 0x7fd0bfu));
    And8(cpu, 0x7fu);
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
    if (!cpu->zero) {
        LoadA8(cpu, 0xffu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
    }
    LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);              /* B6A2 */
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x91u));
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x);          /* B6A9 */
        DecrementA8(cpu);
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x91u));
        if (!cpu->zero)
            return FieldLoopResult(0x83b710u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));
        StoreADirect8(memory, cpu, 0x55u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            StoreADirect8(memory, cpu, 0x55u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6BD */
        Or8(cpu, 0x80u);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
        if (cpu->negative)
            goto store;
        LoadAAbsolute8(memory, cpu, 0xf004u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x05beu, 0);
    } else {
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6D3 */
        StoreADirect8(memory, cpu, 0x55u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            Write8(memory, DirectAddress(cpu, 0x55u), 0x00u);
        LoadA8(cpu, Read8(memory, 0x7fd0bfu));                 /* B6DF */
        And8(cpu, 0x7fu);
        Write8(memory, 0x7fd0bfu, A8(cpu));
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
        if (!cpu->negative)
            goto store;
        LoadAAbsolute8(memory, cpu, 0xf002u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x05beu, 0);
    }
    LoadA8(cpu, 0x20u);                                        /* B6F3 */
    TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);
    LoadAAbsolute8(memory, cpu, 0xf001u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x05bdu, 0);
    CopyAbsolute8(memory, cpu, 0x0692u, 0x05bfu);
store:
    LoadA8(cpu, Read8(memory, 0x7fd0bfu));                     /* B704 */
    And8(cpu, 0x80u);
    Or8(cpu, DirectByte(memory, cpu, 0x54u));
    Write8(memory, 0x7fd0bfu, A8(cpu));
    return FieldLoopResult(0x83b710u);
}

/* $83:B711: event rectangles; a hit runs $83:B727 on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldEventRects(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x83b711u);
    LoadX16(cpu, 0x000cu);                                     /* B711 */
    LoadY16(cpu, 0x0005u);
    if (!FieldRectSearch(memory, cpu, 0xb719u))
        return FieldRectHandoff(cpu);
    if (!cpu->carry)
        return FieldLoopResult(0x83b726u);
    LoadAAbsolute8(memory, cpu, 0xf000u, cpu->x);
    LoadX16(cpu, 0x0008u);
    return FieldLoopHandoff(cpu, 0x83b722u);
}

/* $83:B747: area rectangles; a hit runs $83:B76E on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2FieldAreaRects(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x83b747u);
    SetIndexWidth(cpu, 0);                                     /* B747 */
    LoadX16(cpu, 0x0006u);
    LoadY16(cpu, 0x0009u);
    if (!FieldRectSearch(memory, cpu, 0xb751u))
        return FieldRectHandoff(cpu);
    if (!cpu->carry)
        return FieldLoopResult(0x83b76du);
    LoadAAbsolute8(memory, cpu, 0xf005u, cpu->x);              /* B754 */
    And8(cpu, 0x0fu);
    Compare8(cpu, A8(cpu), 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0692u, 0);
        Compare8(cpu, A8(cpu), 0x04u);
        if (!cpu->zero)
            return FieldLoopResult(0x83b76du);
        LoadA8(cpu, 0x08u);
        TestBitsAbsolute8(memory, cpu, 0x05b5u, 1);
    }
    return FieldLoopHandoff(cpu, 0x83b769u);
}

/* $80:86C1: screen fade from $0581 into the $0583 brightness. */
Lufia2ActorPrimaryUpdateResult Lufia2ScreenFade(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x8086c1u);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);                   /* 86C1 */
    if (!cpu->negative)
        return FieldLoopResult(0x808702u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);
    And8(cpu, 0x3fu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x0582u, 0));
    StoreAAbsolute8(memory, cpu, 0x0582u, 0);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 86DB */
        cpu->carry = 1;
        Sbc8(cpu, 0x0fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        DecrementA8(cpu);
        if (!cpu->negative)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
    } else {
        ExchangeAccumulatorBytes(cpu);                         /* 86F2 */
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        Compare8(cpu, A8(cpu), 0x0fu);
        if (!cpu->carry)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x0fu, 0x0583u);
    }
    return FieldLoopResult(0x808702u);
}

/* $86:9EDD: world map region holding ($58, $5A); carry clear = hit. */
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapRegionSearch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x869eddu);
    PushDataBank(memory, cpu);                                 /* 9EDD */
    SimulateJsrFrame(memory, cpu, 0x9ee0u);
    SetAccumulatorWidth(cpu, 0);                               /* 9F35 */
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09ebu, 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    AslA16(cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0xcffcbeu, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreADirect8(memory, cpu, 0x10u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xcffcbcu, cpu->x)));
    TransferAToX(cpu);
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    TransferAToX(cpu);                                         /* 9EE4 */
    for (entries = 0;; ++entries) {
        unsigned i;
        uint8_t inside = 1;

        /* A list without an end marker spins the ROM. */
        if (entries == 0x10000u) {
            SetAccumulatorWidth(cpu, 1);
            return FieldLoopHandoff(cpu, 0x869ee7u);
        }
        SetAccumulatorWidth(cpu, 1);                           /* 9EE5 */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        if (cpu->negative) {
            cpu->carry = 1;                                    /* 9F10 */
            break;
        }
        for (i = 0; i < 4u && inside; ++i) {
            if (i == 0)
                LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
            else if (i == 2)
                LoadA8(cpu, DirectByte(memory, cpu, 0x5au));
            Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, edges[i], cpu->x));
            inside = (i & 1u) ? !cpu->carry : cpu->carry;
        }
        if (inside)
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9F04 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0009u);
        TransferAToX(cpu);
    }
    PullDataBank(memory, cpu);                                 /* 9F11 */
    return FieldLoopResult(0x869f12u);
}

/* $80:C0B7: next text byte from DB:Y; Y past $FFFF moves the bank. */
static void TextNextByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);              /* C0B7 */
    IncrementY16(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0BD */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
        LoadA8(cpu, Pull8(memory, cpu));
        LoadY16(cpu, 0x8000u);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:C7C2: glyph $09AF into $7E:[$09B1], attribute from $09AD. */
static void TextDrawGlyph(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xbd3bu);
    Push8(memory, cpu, PackStatus(cpu));                       /* C7C2 */
    PushDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x09adu, 0);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c815u, cpu->x)));
    StoreADirect8(memory, cpu, 0x57u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09afu, 0));
    Compare16(cpu, cpu->accumulator, 0x0010u);
    if (cpu->zero)
        LoadA16(cpu, 0x0020u);
    Subtract16(cpu, 0x0020u);                                  /* C7DC */
    if (cpu->carry) {
        unsigned row;

        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        cpu->carry = 0;
        TransferAToX(cpu);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b1u, 0));
        Write16Absolute(memory, cpu, 0x09b5u, cpu->y);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadA8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x58u);
        for (row = 0; row < 16u; ++row) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x9af970u, cpu->x)));
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);     /* C7FC */
            LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
            StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
            IncrementX16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            DecrementDirect8(memory, cpu, 0x58u);
        }
        SetAccumulatorWidth(cpu, 0);                           /* C80B */
        LoadA16(cpu, cpu->y);
        Write16Long(memory, 0x0009b1u, cpu->accumulator);
    }
    PullDataBank(memory, cpu);                                 /* C812 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $80:BD38: draw the glyph and queue its 32-byte VRAM upload. */
static void TextGlyphUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xbd07u);
    TextDrawGlyph(memory, cpu);                                /* BD38 */
    StoreAImmediate8(memory, cpu, 0x01u, 0x4300u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b5u, 0));
    Write16Absolute(memory, cpu, 0x4302u, cpu->accumulator);
    Subtract16(cpu, 0xd000u);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x1800u);
    Write16Absolute(memory, cpu, 0x0079u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4304u);
    LoadX16(cpu, 0x0020u);
    Write16Absolute(memory, cpu, 0x4305u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4301u);
    LoadA8(cpu, 0x41u);
    StoreADirect8(memory, cpu, 0x75u);
    SimulateRtsFrame(memory, cpu);
}

/* $80:9DB0: PLP, PLB, RTL. */
static Lufia2ActorPrimaryUpdateResult TextEngineExit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 9DB0 */
    PullDataBank(memory, cpu);
    result.pc = 0x809db2u;
    return result;
}

/* $80:C0EC: previous text byte; Y below $8000 moves the bank back. */
static void TextPrevByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->y = (uint16_t)(cpu->y - 1u);                          /* C0EC */
    SetNz16(cpu, cpu->y);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0EF */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        DecrementA8(cpu);
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
        LoadA8(cpu, Pull8(memory, cpu));
        LoadY16(cpu, 0xffffu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:C1FD: close an open text window. */
static void TextCloseWindow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x099cu, 0);                   /* C1FD */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x125du, 0);
            StoreZeroAbsolute8(memory, cpu, 0x125eu, 0);
            SimulateJslFrame(memory, cpu, 0x80u, 0xc214u);
            Push8(memory, cpu, PackStatus(cpu));               /* $84:8328 */
            PushDataBank(memory, cpu);
            LoadA8(cpu, 0x7eu);
            PushAccumulator8(memory, cpu);
            PullDataBank(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            SetIndexWidth(cpu, 0);
            LoadA16(cpu, 0x07f8u);
            cpu->carry = 1;
            do {
                unsigned i;

                TransferAToX(cpu);                             /* 8334 */
                for (i = 0; i < 8u; i += 2u)
                    Write16Absolute(
                        memory, cpu, (uint16_t)(0x3000u + cpu->x + i), 0);
                Add16Value(cpu, (uint16_t)~0x0008u);
            } while (!cpu->negative);
            SetAccumulatorWidth(cpu, 1);                       /* 8346 */
            LoadAAbsolute8(memory, cpu, 0x099cu, 0);
            And8(cpu, 0xfeu);
            StoreAAbsolute8(memory, cpu, 0x099cu, 0);
            StoreZeroAbsolute8(memory, cpu, 0x059cu, 0);
            StoreZeroAbsolute8(memory, cpu, 0x059du, 0);
            StoreAImmediate8(memory, cpu, 0xfcu, 0x059eu);
            StoreAImmediate8(memory, cpu, 0xffu, 0x059fu);
            PullDataBank(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            SimulateRtlFrame(memory, cpu);
            LoadA8(cpu, 0x08u);                                /* C215 */
            StoreADirect8(memory, cpu, 0x74u);
        }
    }
    SimulateRtsFrame(memory, cpu);
}

enum {
    TEXT_OPCODE_NEXT,
    TEXT_OPCODE_RELOAD,
    TEXT_OPCODE_EXIT,
    TEXT_OPCODE_HANDOFF
};

/* Script opcodes behind JMP ($CA14,x); others hand off. */
static unsigned TextScriptOpcode(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    unsigned i;

    switch (handler) {
    case 0xa80fu:                                  /* $33 wait for actor */
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        if (cpu->negative) {
            *handoff = 0x80a834u;
            return TEXT_OPCODE_HANDOFF;
        }
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        TransferAToX(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0x88u);
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xa822u);
            return TEXT_OPCODE_EXIT;
        }
        LoadA8(cpu, 0xffu);                                    /* A826 */
        StoreAAbsolute8(memory, cpu, 0x1269u, 0);
        TextNextByte(memory, cpu, 0xa82du);
        TextNextByte(memory, cpu, 0xa830u);
        return TEXT_OPCODE_NEXT;
    case 0xb2ebu:                                  /* $37 wait frames */
        LoadA8(cpu, 0x20u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 1);
        if (cpu->zero)
            Write8(memory, DirectAddress(cpu, 0x42u), 0x00u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x42u));           /* B2F4 */
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        if (!cpu->carry) {
            TextPrevByte(memory, cpu, 0xb2fdu);
            return TEXT_OPCODE_EXIT;
        }
        TextNextByte(memory, cpu, 0xb303u);                    /* B301 */
        LoadA8(cpu, 0x20u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
        return TEXT_OPCODE_NEXT;
    case 0xb397u:                                  /* $3C wait for party */
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        if (cpu->negative) {
            *handoff = 0x80b3dfu;
            return TEXT_OPCODE_HANDOFF;
        }
        LoadAAbsolute8(memory, cpu, 0x0622u, 0);
        for (i = 1; i < 5u; ++i)
            Or8(cpu, Read8(memory,
                AbsoluteIndexedAddress(cpu, (uint16_t)(0x0622u + i), 0)));
        BitImmediate8(cpu, 0x08u);
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xb400u);                /* B3FE */
            return TEXT_OPCODE_EXIT;
        }
        LoadAAbsolute8(memory, cpu, 0x09a7u, 0);               /* B3B2 */
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero) {
            LoadX16(cpu, 0x0004u);
            do {
                LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
                Or8(cpu, 0x04u);
                StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
                cpu->x = (uint16_t)(cpu->x - 1u);
                SetNz16(cpu, cpu->x);
            } while (!cpu->zero);
        } else {
            LoadX16(cpu, 0x0004u);                             /* B3C9 */
            LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x1269u, 0);
            do {
                StoreAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
                cpu->x = (uint16_t)(cpu->x - 1u);
                SetNz16(cpu, cpu->x);
            } while (!cpu->negative);
        }
        LoadA8(cpu, 0xffu);                                    /* B3D7 */
        StoreAAbsolute8(memory, cpu, 0x1269u, 0);
        return TEXT_OPCODE_NEXT;
    case 0x9d4cu:                                  /* $00/$42 end */
        LoadAAbsolute8(memory, cpu, 0x1254u, 0);
        if (cpu->zero) {
            *handoff = 0x809d69u;
            return TEXT_OPCODE_HANDOFF;
        }
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);      /* back to caller */
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1252u, 0));
        Write16Absolute(memory, cpu, 0x09b7u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        StoreZeroAbsolute8(memory, cpu, 0x1254u, 0);
        return TEXT_OPCODE_RELOAD;
    case 0xbc3du:                                  /* $68 skip branch */
        LoadAAbsolute8(memory, cpu, 0x05b3u, 0);
        BitImmediate8(cpu, 0x10u);
        if (cpu->zero) {
            *handoff = 0x80bc44u;
            return TEXT_OPCODE_HANDOFF;
        }
        TextNextByte(memory, cpu, 0xbc4eu);
        TextNextByte(memory, cpu, 0xbc51u);
        return TEXT_OPCODE_NEXT;
    default:
        return TEXT_OPCODE_HANDOFF;
    }
}

/* $80:9CB8 text step: plain characters native, the rest on LLE. */
static Lufia2ActorPrimaryUpdateResult TextEngineStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    unsigned opcodes;

    LoadA8(cpu, Read8(memory, 0x7fd0ffu));                     /* 9CB8 */
    if (!cpu->zero) {
        Compare8(cpu, A8(cpu), 0x03u);
        if (!cpu->carry) {
            DecrementA8(cpu);
            Write8(memory, 0x7fd0ffu, A8(cpu));
            result.pc = 0x809cc7u;
            return result;
        }
        TransferDirectToA(cpu);                                /* 9CC8 */
        Write8(memory, 0x7fd0ffu, A8(cpu));
    }
    PushDataBank(memory, cpu);                                 /* 9CCD */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x125du, 0);
    StoreZeroAbsolute8(memory, cpu, 0x125eu, 0);
    opcodes = 0;
reload:
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);               /* 9CD9 */
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b7u, 0));
        LoadAAbsolute8(memory, cpu, 0x1259u, 0);
        if (cpu->zero)
            break;
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, 0x125au, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
        if (!cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9CEB */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1257u, 0));
        Write16Absolute(memory, cpu, 0x09b7u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x1259u, 0);
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        StoreZeroAbsolute8(memory, cpu, 0x1259u, 0);
    }
    for (;; ++opcodes) {
        uint16_t handler;
        uint32_t handoff = 0x809d3bu;

        Write16Absolute(memory, cpu, 0x09b7u, cpu->y);         /* 9D00 */
        StoreZeroAbsolute8(memory, cpu, 0x0563u, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x099bu, 0);
        And8(cpu, 0x01u);
        if (cpu->zero) {
            TextCloseWindow(memory, cpu, 0x9d30u);             /* 9D2E */
        } else {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* 9D0E */
            Compare8(cpu, A8(cpu), 0x10u);
            if (cpu->carry)
                break;
        }
        TransferDirectToA(cpu);                                /* 9D31 */
        TextNextByte(memory, cpu, 0x9d34u);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        handler = (uint16_t)(
            Read8(memory, 0x800000u | (uint16_t)(0xca14u + cpu->x)) |
            (Read8(memory, 0x800000u | (uint16_t)(0xca15u + cpu->x)) << 8));
        switch (TextScriptOpcode(memory, cpu, handler, &handoff)) {
        case TEXT_OPCODE_NEXT:                                 /* 9D00 */
            continue;
        case TEXT_OPCODE_RELOAD:                               /* 9CD9 */
            ++opcodes;
            goto reload;
        case TEXT_OPCODE_EXIT:                                 /* 9DB0 */
            return TextEngineExit(memory, cpu, result);
        default:                                               /* 9D3B */
            result = FieldLoopHandoff(cpu, handoff);
            result.dispatches = handoff == 0x809d3bu ? opcodes : 0;
            return result;
        }
    }
    StoreAAbsolute8(memory, cpu, 0x09afu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09b0u, 0);
    Compare8(cpu, A8(cpu), 0x80u);
    if (cpu->carry)
        return FieldLoopHandoff(cpu, 0x809d22u);               /* words */
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BCE4 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, 0x099cu, 0);
        if (!cpu->zero)
            return FieldLoopHandoff(cpu, 0x80bcf2u);           /* C56E */
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* BCF5 */
    And8(cpu, 0x10u);
    if (!cpu->zero)
        IncrementY16(cpu);
    else
        TextNextByte(memory, cpu, 0xbd01u);
    Write16Absolute(memory, cpu, 0x09b7u, cpu->y);             /* BD02 */
    TextGlyphUpload(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BD08 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0b52u, 0);
        Write8(memory, 0x7fd0ffu, A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x1255u, 0);
        Compare8(cpu, A8(cpu), 0x10u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x1260u, 0);
            if (!cpu->zero) {
                LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd0c0u) ^ 0x01u));
                Write8(memory, 0x7fd0c0u, A8(cpu));
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x1260u, 0);
                    SimulateJslFrame(memory, cpu, 0x80u, 0xbd34u);
                    ExchangeAccumulatorBytes(cpu);             /* $84:8766 */
                    LoadA8(cpu, Read8(memory, 0x0005b6u));
                    BitImmediate8(cpu, 0x02u);
                    if (cpu->zero) {
                        ExchangeAccumulatorBytes(cpu);
                        Write8(memory, 0x0017acu, A8(cpu));
                    }
                    SimulateRtlFrame(memory, cpu);
                }
            }
        }
    }
    return TextEngineExit(memory, cpu, result);
}

/* $80:C11C: clear bit 0 of the actor flags $0622-$0649. */
static void TextReleaseActors(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9ca5u);
    Push8(memory, cpu, PackStatus(cpu));                       /* C11C */
    SetIndexWidth(cpu, 1);
    cpu->x = 0x27u;
    do {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0xfeu);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        cpu->x = (uint8_t)(cpu->x - 1u);
        SetNz8(cpu, (uint8_t)cpu->x);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:9C80: text box waits for its timer or the A/X buttons. */
static Lufia2ActorPrimaryUpdateResult TextPromptTick(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    LoadAAbsolute8(memory, cpu, 0x1265u, 0);                   /* 9C80 */
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1266u, 0);
        const uint8_t count = (uint8_t)(Read8(memory, timer) + 1u);

        Write8(memory, timer, count);
        SetNz8(cpu, count);
        Compare8(cpu, A8(cpu), count);
        if (cpu->carry)
            return result;
    } else {
        LoadA8(cpu, 0xa0u);                                    /* 9C8F */
        SimulateJsrFrame(memory, cpu, 0x9c93u);
        And8(cpu, DirectByte(memory, cpu, 0x46u));             /* C81E */
        if (!cpu->zero)
            TestBitsDirect(memory, cpu, 0x4au, 0);
        SimulateRtsFrame(memory, cpu);
        if (cpu->zero)
            return result;
    }
    LoadA8(cpu, 0x08u);                                        /* 9C96 */
    TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x099bu, 0);
        TextCloseWindow(memory, cpu, 0x9ca2u);
        TextReleaseActors(memory, cpu);
        return result;
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9CA8 */
    And8(cpu, 0xfdu);
    StoreAAbsolute8(memory, cpu, 0x099bu, 0);
    return TextEngineStep(memory, cpu, result);
}

/* $80:9CB8: text engine step, JSL entry. */
Lufia2ActorPrimaryUpdateResult Lufia2TextEngineStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x809cb8u);
    return TextEngineStep(memory, cpu, FieldLoopResult(0x809db2u));
}
