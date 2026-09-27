/* Menu sprite animations ($86:8CF5). */

#include "core/cpu_internal.h"
#include "lufia2/menu.h"

static void LoadIndexed(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    LoadAAbsolute8(memory, cpu, address, cpu->x);
}

static void PointerFrom(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t low, uint16_t high) {
    LoadIndexed(memory, cpu, low);
    StoreADirect8(memory, cpu, 0x5du);
    LoadIndexed(memory, cpu, high);
    StoreADirect8(memory, cpu, 0x5eu);
}

/* [$5D],Y twice into two slot tables. */
static void PointerPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t low, uint16_t high) {
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));
    StoreAAbsolute8(memory, cpu, low, cpu->x);
    IncrementY16(cpu);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));
    StoreAAbsolute8(memory, cpu, high, cpu->x);
}

/* $86:8CF5: slot X plays animation A from frame $1448,X. */
Lufia2ExecutionResult Lufia2SpriteSetAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    StoreAAbsolute8(memory, cpu, 0x1208u, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    PointerFrom(memory, cpu, 0x1268u, 0x1298u);
    LoadIndexed(memory, cpu, 0x1238u);
    StoreADirect8(memory, cpu, 0x5fu);
    PointerPair(memory, cpu, 0x12c8u, 0x12f8u);                /* animation */
    PointerFrom(memory, cpu, 0x12c8u, 0x12f8u);
    LoadIndexed(memory, cpu, 0x1448u);
    AslA8(cpu);
    TransferAToY(cpu);
    PointerPair(memory, cpu, 0x1328u, 0x1358u);                /* frame */
    PointerFrom(memory, cpu, 0x1328u, 0x1358u);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0x5du)));
    StoreAAbsolute8(memory, cpu, 0x1478u, cpu->x);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x868d46u);
}

/* $86:8CDA: slot X animation list from $8E:D9A9,Y. */
Lufia2ExecutionResult Lufia2SpriteSetTable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x8eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0xd9a9u, cpu->y);
    Write8(memory, LongIndexedAddress(0x001268u, cpu->x), A8(cpu));
    LoadAAbsolute8(memory, cpu, 0xd9aau, cpu->y);
    Write8(memory, LongIndexedAddress(0x001298u, cpu->x), A8(cpu));
    LoadA8(cpu, 0x8eu);
    Write8(memory, LongIndexedAddress(0x001238u, cpu->x), A8(cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x868cf4u);
}

enum {
    SPRITE_SLOTS = 0x30u,
    OAM = 0x0100u,                      /* 128 entries, then 32 high bytes */
};

static void IncrementY8(Lufia2CpuState *cpu) {
    cpu->y = (uint8_t)(cpu->y + 1u);
    SetNz8(cpu, (uint8_t)cpu->y);
}

/* STZ abs,X with a 16-bit accumulator. */
static void StoreAAbsolute16Zero(const Lufia2Memory *memory,
    const Lufia2CpuState *cpu, uint16_t address, uint16_t index) {
    const uint32_t at = AbsoluteIndexedAddress(cpu, address, index);

    Write8(memory, at, 0);
    Write8(memory, (at + 1u) & 0x00ffffffu, 0);
}

/* $86:8B87: slot X timer; at 0 the next frame ($FE holds, $FF loops). */
static void SpriteStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1478u, cpu->x);
    uint8_t value = (uint8_t)(Read8(memory, timer) - 1u);

    Write8(memory, timer, value);
    SetNz8(cpu, value);
    if (value != 0)
        return;
    LoadAAbsolute8(memory, cpu, 0x12c8u, cpu->x);
    StoreADirect8(memory, cpu, 0x5du);
    LoadAAbsolute8(memory, cpu, 0x12f8u, cpu->x);
    StoreADirect8(memory, cpu, 0x5eu);
    LoadAAbsolute8(memory, cpu, 0x1238u, cpu->x);
    StoreADirect8(memory, cpu, 0x5fu);
    StoreADirect8(memory, cpu, 0x62u);
    {
        const uint32_t frame = AbsoluteIndexedAddress(cpu, 0x1448u, cpu->x);

        Write8(memory, frame, (uint8_t)(Read8(memory, frame) + 1u));
        for (;;) {
            LoadAAbsolute8(memory, cpu, 0x1448u, cpu->x);      /* 8BA1 */
            AslA8(cpu);
            TransferAToY(cpu);
            for (;;) {
                LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));   /* 8BA6 */
                StoreAAbsolute8(memory, cpu, 0x1328u, cpu->x);
                StoreADirect8(memory, cpu, 0x60u);
                IncrementY8(cpu);
                LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));
                StoreAAbsolute8(memory, cpu, 0x1358u, cpu->x);
                StoreADirect8(memory, cpu, 0x61u);
                LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0x60u)));
                Compare8(cpu, A8(cpu), 0xfeu);
                if (cpu->zero)
                    break;
                Compare8(cpu, A8(cpu), 0xffu);
                if (!cpu->zero) {
                    StoreAAbsolute8(memory, cpu, 0x1478u, cpu->x);
                    return;
                }
                StoreZeroAbsolute8(memory, cpu, 0x1448u, cpu->x);  /* 8BC8 */
                LoadY8(cpu, 0x00u);
            }
            Write8(memory, frame, (uint8_t)(Read8(memory, frame) - 1u));  /* 8BC3 */
        }
    }
}

/* $86:8B73: step every active slot's animation. */
Lufia2ExecutionResult Lufia2SpriteAnimateAll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetIndexWidth(cpu, 1);
    LoadX8(cpu, 0x00u);
    do {
        LoadAAbsolute8(memory, cpu, 0x11d8u, cpu->x);
        if (!cpu->zero) {
            SimulateJsrFrame(memory, cpu, 0x8b7eu);
            SpriteStep(memory, cpu);
            SimulateRtsFrame(memory, cpu);
        }
        cpu->x = (uint8_t)(cpu->x + 1u);
        Compare8(cpu, (uint8_t)cpu->x, SPRITE_SLOTS);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 0);
    return ExecutionReturned(0x868b86u);
}

/* $86:8BCF: all OAM entries off screen, high table clear. */
Lufia2ExecutionResult Lufia2SpriteClearOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0x0080u);
    LoadX16(cpu, 0x0000u);
    do {
        LoadA16(cpu, 0xf000u);
        StoreAAbsolute16(memory, cpu, OAM, cpu->x);
        IncrementX16(cpu);
        IncrementX16(cpu);
        StoreAAbsolute16Zero(memory, cpu, OAM, cpu->x);
        IncrementX16(cpu);
        IncrementX16(cpu);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    } while (!cpu->zero);
    LoadY16(cpu, 0x0010u);
    do {
        StoreAAbsolute16Zero(memory, cpu, OAM, cpu->x);
        IncrementX16(cpu);
        IncrementX16(cpu);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x868bf4u);
}

/* $86:8C1F: OAM pieces of slot X from frame [$1328/$1358]. */
static void SpritePieces(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x1328u, cpu->x);
    StoreADirect8(memory, cpu, 0x5du);
    LoadAAbsolute8(memory, cpu, 0x1358u, cpu->x);
    StoreADirect8(memory, cpu, 0x5eu);
    LoadAAbsolute8(memory, cpu, 0x1238u, cpu->x);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0x0001u);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x56u);                         /* size bit */
    Write8(memory, DirectAddress(cpu, 0x57u), 0);
    IncrementY16(cpu);
    for (;;) {
        SetAccumulatorWidth(cpu, 0);                           /* 8C3B */
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0x5du));
        StoreADirect16(memory, cpu, 0x58u);                    /* y, x offset */
        IncrementY16(cpu);
        IncrementY16(cpu);
        SetAccumulatorWidth(cpu, 1);
        PushY(memory, cpu);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x5au));
        LoadAAbsolute8(memory, cpu, 0x1388u, cpu->x);
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, 0x59u));
        StoreAAbsolute8(memory, cpu, OAM, cpu->y);
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x13e8u, cpu->x);
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, 0x58u));
        StoreAAbsolute8(memory, cpu, OAM, cpu->y);
        IncrementY16(cpu);
        StoreYDirect16(memory, cpu, 0x5au);
        cpu->y = PullIndexValue(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0x5du));  /* tile, attributes */
        IncrementY16(cpu);
        IncrementY16(cpu);
        PushY(memory, cpu);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x5au));
        StoreAAbsolute16(memory, cpu, OAM, cpu->y);
        IncrementY16(cpu);
        IncrementY16(cpu);
        StoreYDirect16(memory, cpu, 0x5au);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));        /* high table */
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        And16(cpu, 0x00feu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0200u);
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, OAM, cpu->y));
        StoreADirect16(memory, cpu, 0x63u);
        LoadA16(cpu, (uint16_t)((Read16Direct(memory, cpu, 0x54u) & 0x000fu) + 1u));
        StoreADirect16(memory, cpu, 0x65u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x59u));
        And8(cpu, 0x80u);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x1388u, cpu->x);
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x59u));
            LoadAAbsolute8(memory, cpu, 0x13b8u, cpu->x);
            Sbc8(cpu, 0x00u);
        } else {
            LoadA8(cpu, (uint8_t)((DirectByte(memory, cpu, 0x59u) ^ 0xffu) + 1u));
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x1388u, cpu->x));
            LoadAAbsolute8(memory, cpu, 0x13b8u, cpu->x);
            Adc8(cpu, 0x00u);
        }
        cpu->carry = 0;
        Adc8(cpu, 0xffu);                                      /* carry = x bit 8 */
        SetAccumulatorWidth(cpu, 0);
        RolA16(cpu);
        And16(cpu, 0x0001u);
        Or16(cpu, Read16Direct(memory, cpu, 0x56u));
        cpu->carry = 0;
        RorA16(cpu);
        do {
            RolA16(cpu);                                       /* 8CBF */
            Decrement16Direct(memory, cpu, 0x65u);
        } while (!cpu->zero);
        Or16(cpu, Read16Direct(memory, cpu, 0x63u));
        StoreAAbsolute16(memory, cpu, OAM, cpu->y);
        Increment16Direct(memory, cpu, 0x54u);
        Increment16Direct(memory, cpu, 0x54u);
        SetAccumulatorWidth(cpu, 1);
        cpu->y = PullIndexValue(memory, cpu);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x5du));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            return;
    }
}

/* $86:8BF5: OAM for slots 12-47, then 0-11. */
Lufia2ExecutionResult Lufia2SpriteBuildOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t kRanges[2][2] = {{0x0cu, 0x30u}, {0x00u, 0x0cu}};
    static const uint16_t kReturns[2] = {0x8c06u, 0x8c17u};
    unsigned pass;

    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x54u);
    StoreXDirect16(memory, cpu, 0x5au);
    for (pass = 0; pass < 2u; ++pass) {
        LoadX16(cpu, kRanges[pass][0]);
        do {
            LoadAAbsolute8(memory, cpu, 0x11d8u, cpu->x);
            if (!cpu->zero) {
                SimulateJsrFrame(memory, cpu, kReturns[pass]);
                SpritePieces(memory, cpu);
                SimulateRtsFrame(memory, cpu);
            }
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, kRanges[pass][1]);
        } while (!cpu->zero);
    }
    return ExecutionReturned(0x868c1eu);
}

/* $86:8B55: animate, clear and build OAM, request it ($72), then the
   frame wait $86:8B48 on LLE. */
Lufia2ExecutionResult Lufia2SpriteFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[3] = {0x8b5fu, 0x8b63u, 0x8b67u};

    PushY(memory, cpu);
    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x86u, kReturns[0]);
    (void)Lufia2SpriteAnimateAll(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x86u, kReturns[1]);
    (void)Lufia2SpriteClearOam(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x86u, kReturns[2]);
    (void)Lufia2SpriteBuildOam(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x80u);
    StoreADirect8(memory, cpu, 0x72u);
    SimulateJsrFrame(memory, cpu, 0x8b6eu);
    return ExecutionHandoff(cpu, 0x868b48u);
}
