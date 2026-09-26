/* Menu screen building blocks and screens. */

#include "core/cpu_internal.h"
#include "lufia2/menu.h"
#include "system/system_internal.h"

static void Jsr(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
}

static void Rts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateRtsFrame(memory, cpu);
}

static void SetBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t bank) {
    LoadA8(cpu, bank);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

static void StoreIndexed(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    StoreAAbsolute8(memory, cpu, address, cpu->x);
}

/* JSL $80:8878; 1 when it handed off. */
static int DrawString(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address, Lufia2ExecutionResult *out) {
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    cpu->program_bank = 0x80u;
    *out = Lufia2MenuDrawString(memory, cpu);
    if (out->flow != LUFIA2_EXECUTION_RETURNED)
        return 1;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x82u;
    return 0;
}

/* $82:83B5: clear both layers ($7E:2000/$7E:3000) in a rect; M0. */
static void MenuClearRect(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect16(memory, cpu, 0x54u);
    Write16Direct(memory, cpu, 0x56u, 0);
    Write16Direct(memory, cpu, 0x58u, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x58u);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x56u);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    do {
        LoadY16(cpu, Read16Direct(memory, cpu, 0x56u));
        LoadX16(cpu, Read16Direct(memory, cpu, 0x54u));
        do {
            Write16Absolute(memory, cpu, (uint16_t)(0x2000u + cpu->x), 0);
            Write16Absolute(memory, cpu, (uint16_t)(0x3000u + cpu->x), 0);
            IncrementX16(cpu);
            IncrementX16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        StoreADirect16(memory, cpu, 0x54u);
        Decrement16Direct(memory, cpu, 0x58u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
}

/* $82:88CB: cursor sprite position from row/column steps. */
static void MenuCursorPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kIn[2][2] = {{0x14fdu, 0x14d9u}, {0x1503u, 0x14ebu}};
    static const uint16_t kBase[2][2] = {{0x14cdu, 0x14d3u}, {0x14dfu, 0x14e5u}};
    static const uint16_t kOut[2][2] = {{0x138du, 0x13bdu}, {0x13edu, 0x141du}};
    static const uint16_t kReturn[2] = {0x88e0u, 0x8909u};
    unsigned i;

    for (i = 0; i < 2u; ++i) {
        LoadAAbsolute8(memory, cpu, kIn[i][0], cpu->x);
        StoreAAbsolute8(memory, cpu, 0x1570u, 0);
        StoreZeroAbsolute8(memory, cpu, 0x1571u, 0);
        LoadAAbsolute8(memory, cpu, kIn[i][1], cpu->x);
        StoreAAbsolute8(memory, cpu, 0x1572u, 0);
        StoreZeroAbsolute8(memory, cpu, 0x1573u, 0);
        Lufia2CallMultiply(memory, cpu, 0x82u, kReturn[i]);
        LoadAAbsolute8(memory, cpu, 0x1574u, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, kBase[i][0], cpu->x));
        StoreIndexed(memory, cpu, kOut[i][0]);
        LoadAAbsolute8(memory, cpu, 0x1575u, 0);
        Adc8(cpu, AbsoluteByte(memory, cpu, kBase[i][1], cpu->x));
        StoreIndexed(memory, cpu, kOut[i][1]);
    }
}

/* $82:891E: cursor slot X from the $A6:F518 layout Y. */
static void MenuCursorPlace(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kFields[7] = {
        0x14cdu, 0x14dfu, 0x14f1u, 0x14f7u, 0x14fdu, 0x1503u, 0x14c1u};
    unsigned i;

    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0xa6u);
    for (i = 0; i < 7u; ++i) {
        LoadAAbsolute8(memory, cpu, (uint16_t)(0xf518u + i), cpu->y);
        StoreIndexed(memory, cpu, kFields[i]);
        if (i == 1u) {
            StoreZeroAbsolute8(memory, cpu, 0x14d3u, cpu->x);
            StoreZeroAbsolute8(memory, cpu, 0x14e5u, cpu->x);
            StoreZeroAbsolute8(memory, cpu, 0x14c7u, cpu->x);
        }
    }
    PullDataBank(memory, cpu);
    Jsr(memory, cpu, 0x8959u);
    MenuCursorPosition(memory, cpu);
    Rts(memory, cpu);
}

/* $82:895B: sprite slot X shows cursor animation A. */
static void MenuCursorAnimation(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Write16Absolute(memory, cpu, 0x14a9u, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82897du, cpu->x)));
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x14a9u, 0));
    StoreIndexed(memory, cpu, 0x1448u);
    LoadA8(cpu, 0x01u);
    StoreIndexed(memory, cpu, 0x11d8u);
    ExchangeAccumulatorBytes(cpu);
    SimulateJslFrame(memory, cpu, 0x82u, 0x897bu);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:93F6: window HDMA table A from $8E:E57E/$8E:E5A0. */
static void MenuWindowHdma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ee57eu, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x0596u, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ee5a0u, cpu->x)));
    StoreADirect16(memory, cpu, 0xf4u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0xf6u);
    LoadA8(cpu, 0x10u);
    {
        const uint8_t old = DirectByte(memory, cpu, 0xf2u);    /* TSB */

        cpu->zero = (old & 0x10u) == 0;
        Write8(memory, DirectAddress(cpu, 0xf2u), (uint8_t)(old | 0x10u));
    }
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, 0xf3u);
    LoadA8(cpu, 0x12u);
    StoreADirect8(memory, cpu, 0xfau);
    LoadA8(cpu, 0x02u);
    StoreADirect8(memory, cpu, 0xfbu);
    LoadX16(cpu, 0x80c0u);
    StoreXDirect16(memory, cpu, 0xf7u);
    LoadA8(cpu, 0x7eu);
    StoreADirect8(memory, cpu, 0xf9u);
    LoadA8(cpu, 0x03u);
    StoreAAbsolute8(memory, cpu, 0x1565u, 0);
}

/* $82:9F6F: equipment commands window. */
Lufia2ExecutionResult Lufia2MenuEquipCommands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0442u);
    LoadX16(cpu, 0x1e0bu);
    Jsr(memory, cpu, 0x9f79u);
    MenuClearRect(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x045eu);
    LoadX16(cpu, 0x0f0au);
    Jsr(memory, cpu, 0x9f82u);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreA8Absolute(memory, cpu, 0x0564u, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd2aeu);
    {
        Lufia2ExecutionResult text;

        if (DrawString(memory, cpu, 0x9f94u, &text))
            return text;
    }
    LoadA8(cpu, 0x01u);
    LoadX16(cpu, 0x0007u);
    Jsr(memory, cpu, 0x9f9cu);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x0015u);
    LoadX16(cpu, 0x0002u);
    Jsr(memory, cpu, 0x9fa5u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x02u);
    Jsr(memory, cpu, 0x9faau);
    MenuWindowHdma(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x88u);
    {
        const uint8_t old = DirectByte(memory, cpu, 0x74u);    /* TSB */

        cpu->zero = (old & 0x88u) == 0;
        Write8(memory, DirectAddress(cpu, 0x74u), (uint8_t)(old | 0x88u));
    }
    return ExecutionReturned(0x829fafu);
}

static void Window(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t position, uint16_t size, uint16_t return_address) {
    LoadA16(cpu, position);
    LoadX16(cpu, size);
    Jsr(memory, cpu, return_address);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
}

/* JSL $80:8878 with the string at $8E:Y, attribute $20. */
static int Text8E(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t string, uint16_t return_address, Lufia2ExecutionResult *out) {
    StoreA8Absolute(memory, cpu, 0x0564u, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, string);
    return DrawString(memory, cpu, return_address, out);
}

/* $82:9214 / $82:922D: clear flag bytes for set bits of X. */
static void MenuClearSlots(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t table) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    TransferXToA(cpu);
    LoadX16(cpu, 0x0000u);
    do {
        LsrA16(cpu);
        if (cpu->carry)
            Write8(memory, AbsoluteIndexedAddress(cpu, table, cpu->x), 0);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $82:D721: shop windows; the first only with $1540 bit 1. */
Lufia2ExecutionResult Lufia2MenuShopWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1540u, 0));
    if (cpu->accumulator & 0x0002u)
        Window(memory, cpu, 0x0382u, 0x0f03u, 0xd733u);
    Window(memory, cpu, 0x03a0u, 0x0f03u, 0xd73cu);
    Window(memory, cpu, 0x0442u, 0x1e0au, 0xd745u);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82d748u);
}

/* $82:E49E: shop kind title and its window. */
Lufia2ExecutionResult Lufia2MenuShopTitle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    StoreA8Absolute(memory, cpu, 0x0564u, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd366u);
    LoadX16(cpu, 0x30f0u);
    if (DrawString(memory, cpu, 0xe4b0u, &text))
        return text;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x1542u, 0) - 1u));
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e4d0u, cpu->x)));
    And16(cpu, 0x00ffu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0900u);
    TransferAToX(cpu);
    LoadA16(cpu, 0x00acu);
    Jsr(memory, cpu, 0xe4ccu);
    (void)Lufia2MenuDrawWindow(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82e4cfu);
}

/* $82:EFC5: "Game saved." window; carry picks the layout. */
Lufia2ExecutionResult Lufia2MenuSavedWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    SetAccumulatorWidth(cpu, 0);
    if (!cpu->carry) {
        Window(memory, cpu, 0x0584u, 0x1105u, 0xefd1u);
        LoadX16(cpu, 0x3648u);
    } else {
        Window(memory, cpu, 0x0090u, 0x1703u, 0xefdfu);
        LoadX16(cpu, 0x30d4u);
    }
    SetAccumulatorWidth(cpu, 1);
    if (Text8E(memory, cpu, 0xcae1u, 0xeff4u, &text))
        return text;
    return ExecutionReturned(0x82eff5u);
}

/* $82:F0A2: name entry windows and cursor. */
Lufia2ExecutionResult Lufia2MenuNameEntryWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0xffffu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf0aau);
    MenuClearSlots(memory, cpu, 0x11d8u);
    SimulateRtlFrame(memory, cpu);
    LoadX16(cpu, 0xffffu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf0b1u);
    MenuClearSlots(memory, cpu, 0x11e8u);
    SimulateRtlFrame(memory, cpu);
    Window(memory, cpu, 0x0108u, 0x0803u, 0xf0bau);
    Window(memory, cpu, 0x0118u, 0x1003u, 0xf0c3u);
    Window(memory, cpu, 0x01c8u, 0x1811u, 0xf0ccu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x14deu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x14f0u, 0);
    LoadA8(cpu, 0x07u);
    LoadX16(cpu, 0x000au);
    Jsr(memory, cpu, 0xf0dcu);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x00afu);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xf0e5u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadX16(cpu, 0x0000u);
    Write16Absolute(memory, cpu, 0x14bbu, 0);
    Write16Absolute(memory, cpu, 0x14b9u, 0);
    LoadX16(cpu, 0x3162u);
    Write16Absolute(memory, cpu, 0x14adu, 0x3162u);
    return ExecutionReturned(0x82f0f5u);
}
