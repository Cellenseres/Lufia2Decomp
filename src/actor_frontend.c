#include "lufia2/actor_frontend.h"

static uint32_t DirectAddress(const Lufia2ActorFrontendCpu *cpu, uint8_t offset) {
    return (uint16_t)(cpu->direct_page + offset);
}

static uint32_t AbsoluteAddress(
    const Lufia2ActorFrontendCpu *cpu, uint16_t address) {
    return ((uint32_t)cpu->data_bank << 16) | address;
}

/* Indexing carries into the next bank. */
static uint32_t AbsoluteIndexedAddress(
    const Lufia2ActorFrontendCpu *cpu, uint16_t address) {
    return (((uint32_t)cpu->data_bank << 16) + address + cpu->x) &
           0x00ffffffu;
}

static uint32_t LongIndexedAddress(uint32_t address, uint16_t x) {
    return (address + x) & 0x00ffffffu;
}

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

static void LoadXDirect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    const uint32_t address = DirectAddress(cpu, offset);

    if (cpu->index_is_8_bit) {
        const uint8_t value = Read8(memory, address);
        cpu->x = value;
        SetNz8(cpu, value);
    } else {
        const uint16_t value = (uint16_t)(
            Read8(memory, address) |
            ((uint16_t)Read8(memory, (address + 1u) & 0xffffu) << 8));
        cpu->x = value;
        SetNz16(cpu, value);
    }
}

static void LoadAAbsolute(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address) {
    LoadA8(cpu, Read8(memory, AbsoluteAddress(cpu, address)));
}

static void LoadAAbsoluteX(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t address) {
    LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, address)));
}

static void LoadALong(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t address) {
    LoadA8(cpu, Read8(memory, address));
}

static void LoadALongX(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint32_t address) {
    LoadA8(cpu, Read8(memory, LongIndexedAddress(address, cpu->x)));
}

static void LoadADirect(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t offset) {
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, offset)));
}

static void And8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t result = (uint8_t)(A8(cpu) & value);
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | result);
    SetNz8(cpu, result);
}

static void BitImmediate8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    cpu->zero = (A8(cpu) & value) == 0;
}

static void Compare8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t a = A8(cpu);
    const uint8_t result = (uint8_t)(a - value);
    cpu->carry = a >= value;
    SetNz8(cpu, result);
}

static void IncrementA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t result = (uint8_t)(A8(cpu) + 1u);
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | result);
    SetNz8(cpu, result);
}

static void DecrementA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t result = (uint8_t)(A8(cpu) - 1u);
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | result);
    SetNz8(cpu, result);
}

static void TransferXToA8(Lufia2ActorFrontendCpu *cpu) {
    LoadA8(cpu, (uint8_t)cpu->x);
}

static void TransferDirectToA(Lufia2ActorFrontendCpu *cpu) {
    cpu->accumulator = cpu->direct_page;
    SetNz16(cpu, cpu->accumulator);
}

static void LsrA8(Lufia2ActorFrontendCpu *cpu) {
    const uint8_t old = A8(cpu);
    cpu->carry = old & 1u;
    LoadA8(cpu, (uint8_t)(old >> 1));
}

static void ExclusiveOr8(Lufia2ActorFrontendCpu *cpu, uint8_t value) {
    const uint8_t result = (uint8_t)(A8(cpu) ^ value);
    cpu->accumulator =
        (uint16_t)((cpu->accumulator & 0xff00u) | result);
    SetNz8(cpu, result);
}

Lufia2ActorPrimaryFlow Lufia2ActorPrimaryUpdateFrontend(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                 /* $83:C7F8 */
    LoadAAbsoluteX(memory, cpu, 0x0622u);            /* $83:C7FA */
    And8(cpu, 0x80u);                                /* $83:C7FD */
    if (!cpu->zero)                                  /* $83:C7FF */
        return LUFIA2_ACTOR_PRIMARY_RETURN;

    LoadAAbsoluteX(memory, cpu, 0x1291u);            /* $83:C801 */
    BitImmediate8(cpu, 0x07u);                       /* $83:C804 */
    if (!cpu->zero)                                  /* $83:C806 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C808;

    LoadAAbsolute(memory, cpu, 0x09a7u);             /* $83:C80E */
    BitImmediate8(cpu, 0x01u);                       /* $83:C811 */
    if (cpu->zero)                                   /* $83:C813 */
        goto timer;

    LoadAAbsolute(memory, cpu, 0x1269u);             /* $83:C815 */
    if (!cpu->negative)                              /* $83:C818 */
        goto timer;

    LoadADirect(memory, cpu, 0xa7u);                 /* $83:C81A */
    if (cpu->zero)                                   /* $83:C81C */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;

    LoadAAbsoluteX(memory, cpu, 0x0622u);            /* $83:C81E */
    BitImmediate8(cpu, 0x08u);                       /* $83:C821 */
    if (!cpu->zero)                                  /* $83:C823 */
        goto timer;
    BitImmediate8(cpu, 0x40u);                       /* $83:C825 */
    if (cpu->zero)                                   /* $83:C827 */
        return LUFIA2_ACTOR_PRIMARY_RETURN;
    And8(cpu, 0xbfu);                                /* $83:C829 */
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0622u), A8(cpu));
                                                        /* $83:C82B */

timer:
    LoadALongX(memory, cpu, 0x7fe3c6u);              /* $83:C82E */
    if (cpu->zero)                                   /* $83:C832 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;
    DecrementA8(cpu);                                /* $83:C834 */
    Write8(memory, LongIndexedAddress(0x7fe3c6u, cpu->x), A8(cpu));
                                                        /* $83:C835 */
    if (cpu->zero)                                   /* $83:C839 */
        return LUFIA2_ACTOR_PRIMARY_CONTINUE_C83C;
    return LUFIA2_ACTOR_PRIMARY_RETURN;              /* $83:C83B */
}

Lufia2ActorSecondaryFlow Lufia2ActorSecondaryUpdateFrontend(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadXDirect(memory, cpu, 0xa7u);                 /* $83:D508 */
    LoadALongX(memory, cpu, 0x000736u);              /* $83:D50A */
    BitImmediate8(cpu, 0x80u);                       /* $83:D50E */
    if (!cpu->zero) {
        const uint32_t timer = LongIndexedAddress(0x7fe35eu, cpu->x);
        const uint32_t flags = LongIndexedAddress(0x7fe316u, cpu->x);

        LoadA8(cpu, Read8(memory, timer));           /* $83:D512 */
        DecrementA8(cpu);
        Write8(memory, timer, A8(cpu));
        And8(cpu, 0x0fu);                            /* $83:D51B */
        if (cpu->zero) {
            /* Reload the period from the high nibble. */
            LoadA8(cpu, Read8(memory, timer));       /* $83:D51F */
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            LsrA8(cpu);
            LoadA8(cpu, (uint8_t)(A8(cpu) | Read8(memory, timer)));
            Write8(memory, timer, A8(cpu));
            LoadA8(cpu, Read8(memory, flags));       /* $83:D52F */
            ExclusiveOr8(cpu, 0x80u);
            Write8(memory, flags, A8(cpu));
        }
    }

    LoadAAbsoluteX(memory, cpu, 0x0622u);            /* $83:D539 */
    And8(cpu, 0x80u);                                /* $83:D53C */
    if (!cpu->zero)                                  /* $83:D53E */
        return LUFIA2_ACTOR_SECONDARY_CONTINUE_D59A;

    LoadAAbsolute(memory, cpu, 0x09a7u);             /* $83:D540 */
    BitImmediate8(cpu, 0x01u);                       /* $83:D543 */
    if (cpu->zero)                                   /* $83:D545 */
        goto final_gate;

    TransferXToA8(cpu);                              /* $83:D547 */
    if (!cpu->zero)                                  /* $83:D548 */
        goto state_gate;

    LoadALong(memory, cpu, 0x7fd0feu);               /* $83:D54A */
    if (cpu->zero)                                   /* $83:D54E */
        goto final_gate;
    goto walk_counter;                               /* $83:D550 */

state_gate:
    LoadAAbsoluteX(memory, cpu, 0x0736u);            /* $83:D552 */
    BitImmediate8(cpu, 0x42u);                       /* $83:D555 */
    if (!cpu->zero)                                  /* $83:D557 */
        goto final_gate;

    LoadAAbsoluteX(memory, cpu, 0x05d2u);            /* $83:D559 */
    Compare8(cpu, 0x80u);                            /* $83:D55C */
    if (!cpu->carry)                                 /* $83:D55E */
        goto final_gate;
    Compare8(cpu, 0xa4u);                            /* $83:D560 */
    if (cpu->zero)                                   /* $83:D562 */
        goto final_gate;

walk_counter:
    LoadALongX(memory, cpu, 0x7fe48eu);              /* $83:D564 */
    IncrementA8(cpu);                                /* $83:D568 */
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
                                                        /* $83:D569 */
    Compare8(cpu, 0x20u);                            /* $83:D56D */
    if (!cpu->zero)                                  /* $83:D56F */
        goto final_gate;

    TransferDirectToA(cpu);                          /* $83:D571 */
    Write8(memory, LongIndexedAddress(0x7fe48eu, cpu->x), A8(cpu));
                                                        /* $83:D572 */
    LoadAAbsoluteX(memory, cpu, 0x066au);            /* $83:D576 */
    ExclusiveOr8(cpu, 0x01u);                        /* $83:D579 */
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x066au), A8(cpu));
                                                        /* $83:D57B */

final_gate:
    LoadAAbsoluteX(memory, cpu, 0x0736u);            /* $83:D57E */
    BitImmediate8(cpu, 0x04u);                       /* $83:D581 */
    if (!cpu->zero) {
        LoadALongX(memory, cpu, 0x7fe48eu);          /* $83:D585 */
        BitImmediate8(cpu, 0x02u);
        if (!cpu->zero) {
            LoadXDirect(memory, cpu, 0xa9u);         /* $83:D58D */
            LoadALongX(memory, cpu, 0x7fddaeu);
            ExclusiveOr8(cpu, 0x01u);
            Write8(
                memory, LongIndexedAddress(0x7fddaeu, cpu->x), A8(cpu));
        }
    }
    return LUFIA2_ACTOR_SECONDARY_RETURN;            /* $83:D599 */
}
