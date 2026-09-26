/* Menu screen building blocks and screens. */

#include "core/cpu_internal.h"
#include "lufia2/item.h"
#include "lufia2/menu.h"
#include "lufia2/party.h"
#include "lufia2/system.h"
#include "party/party_internal.h"
#include "system/system_internal.h"

static void Jsr(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
}

static void Rts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateRtsFrame(memory, cpu);
}

/* (dp),Y 16-bit through DB. */
static uint16_t Indirect16At(const Lufia2Memory *memory,
    const Lufia2CpuState *cpu, uint8_t offset, uint16_t index) {
    return Read16Long(memory, (((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, offset) + index) & 0x00ffffffu);
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

/* $82:83EB: clear layer 2 ($7E:3000) in a rect; M0. */
static void MenuClearRect2(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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

/* $82:831A: tiles Y, Y+1, ... in a rect at A, X = w << 8 | h; M0. */
static void MenuTileBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAccumulator16(memory, cpu);
    Write16Direct(memory, cpu, 0x56u, 0);
    Write16Direct(memory, cpu, 0x58u, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x58u);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x56u);
    SetAccumulatorWidth(cpu, 0);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA16(cpu, cpu->y);                                      /* TYA */
    do {
        LoadY16(cpu, Read16Direct(memory, cpu, 0x56u));
        PushIndex(memory, cpu);
        do {
            Write16Long(memory, LongIndexedAddress(0x7e2000u, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementA16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        Decrement16Direct(memory, cpu, 0x58u);
    } while (!cpu->zero);
}

/* $82:91AB: A (max 999) into hundreds $00, tens $02, ones $04. */
static void MenuDigits3(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t kSlots[3] = {0x00u, 0x02u, 0x04u};
    static const uint16_t kUnits[2] = {0x0064u, 0x000au};
    static const uint16_t kReturns[3] = {0x91c0u, 0x91c8u, 0x91d0u};
    unsigned i;

    for (i = 0; i < 3u; ++i)
        Write16Direct(memory, cpu, kSlots[i], 0);
    Compare16(cpu, cpu->accumulator, 0x03e8u);
    if (cpu->carry)
        LoadA16(cpu, 0x03e7u);
    for (i = 0; i < 2u; ++i) {
        Compare16(cpu, cpu->accumulator, kUnits[i]);
        if (!cpu->carry)
            continue;
        SimulateJsrFrame(memory, cpu, kReturns[i]);
        do {
            Increment16Direct(memory, cpu, kSlots[i]);
            Subtract16(cpu, kUnits[i]);
            Compare16(cpu, cpu->accumulator, kUnits[i]);
        } while (cpu->carry);
        SimulateRtsFrame(memory, cpu);
    }
    Compare16(cpu, cpu->accumulator, 0x0000u);
    if (!cpu->zero) {
        SimulateJsrFrame(memory, cpu, kReturns[2]);
        do {
            Increment16Direct(memory, cpu, 0x04u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
        } while (!cpu->zero);
        SimulateRtsFrame(memory, cpu);
    }
}

/* $82:91F0: tens and ones tile at $7E:3002,X. */
static void MenuNumberTail(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Direct(memory, cpu, 0x02u));
    if (!cpu->zero) {
        StoreADirect16(memory, cpu, 0x15u);
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadA16(cpu, 0x0000u);
        do {
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Decrement16Direct(memory, cpu, 0x15u);
        } while (!cpu->zero);
    }
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x11u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x04u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator |
        Read16AbsoluteIndexed(memory, cpu, 0x155cu, 0)));
    Write16Long(memory, LongIndexedAddress(0x7e3002u, cpu->x), cpu->accumulator);
}

/* $82:9169: small 3-digit number at $7E:3000,X. */
static void MenuSmallNumber(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Jsr(memory, cpu, 0x916bu);
    MenuDigits3(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x0050u);
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    if (!cpu->zero) {
        cpu->carry = 0;
        Add16Value(cpu, 0x0050u);
        LoadA16(cpu, (uint16_t)(cpu->accumulator |
            Read16AbsoluteIndexed(memory, cpu, 0x155cu, 0)));
        Write16Long(memory, LongIndexedAddress(0x7e3000u, cpu->x),
            cpu->accumulator);
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
    }
    Jsr(memory, cpu, 0x9187u);
    MenuNumberTail(memory, cpu);
    Rts(memory, cpu);
}

/* $82:E4D5: seven stats of each member as small numbers; M1. */
static void MenuPartyStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a7au, 0));
    And16(cpu, 0x00ffu);
    StoreADirect16(memory, cpu, 0x26u);
    LoadA16(cpu, 0x3d00u);
    StoreAAbsolute16(memory, cpu, 0x155cu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a80u, cpu->x));
        cpu->carry = 0;
        Add16Value(cpu, 0x0029u);
        StoreADirect16(memory, cpu, 0x2au);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e51au, cpu->x)));
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe4f9u);                             /* E504 */
        LoadY16(cpu, 0x0000u);
        do {
            LoadA16(cpu, Read16Long(memory, (((uint32_t)cpu->data_bank << 16) +
                Read16Direct(memory, cpu, 0x2au) + cpu->y) & 0x00ffffffu));
            Jsr(memory, cpu, 0xe50bu);
            MenuSmallNumber(memory, cpu);
            Rts(memory, cpu);
            TransferXToA(cpu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0040u);
            TransferAToX(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            Compare16(cpu, cpu->y, 0x000eu);
        } while (!cpu->zero);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x26u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

static void TsbDirect(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset, uint8_t bits) {
    const uint8_t old = DirectByte(memory, cpu, offset);

    LoadA8(cpu, bits);
    cpu->zero = (old & bits) == 0;
    Write8(memory, DirectAddress(cpu, offset), (uint8_t)(old | bits));
}

/* $82:D749: shop party screen with member stats. */
Lufia2ExecutionResult Lufia2MenuShopParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0xf7f0u);
    SimulateJslFrame(memory, cpu, 0x82u, 0xd74fu);
    MenuClearSlots(memory, cpu, 0x11d8u);
    SimulateRtlFrame(memory, cpu);
    if (Text8E(memory, cpu, 0xccfdu, 0xd75fu, &text))
        return text;
    StoreA8Absolute(memory, cpu, 0x0564u, 0x21u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xcd27u);
    if (DrawString(memory, cpu, 0xd76fu, &text))
        return text;
    Jsr(memory, cpu, 0xd772u);
    MenuPartyStats(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x01u);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xd77au);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x0070u);
    LoadX16(cpu, 0x0000u);
    Jsr(memory, cpu, 0xd783u);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1542u, 0);
    StoreAAbsolute8(memory, cpu, 0x14f7u, 0);
    LoadX16(cpu, 0x0001u);
    StoreZeroAbsolute8(memory, cpu, 0x14d9u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x14ebu, cpu->x);
    LoadA8(cpu, 0x0au);
    Jsr(memory, cpu, 0xd797u);
    MenuWindowHdma(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x82d79cu);
}

/* $82:D1A4: capsule portraits and the seven present marks. */
static void CapsulePortraits(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kBlocks[3][4] = {
        {0x0358u, 0x020au, 0x1100u, 0xd1b1u},
        {0x0378u, 0x020au, 0x1120u, 0xd1bdu},
        {0x055cu, 0x0e02u, 0x1140u, 0xd1c9u}};
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 3u; ++i) {
        LoadA16(cpu, kBlocks[i][0]);
        LoadX16(cpu, kBlocks[i][1]);
        LoadY16(cpu, kBlocks[i][2]);
        Jsr(memory, cpu, kBlocks[i][3]);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
    }
    Write16Direct(memory, cpu, 0x04u, 0);
    do {
        const uint16_t step = Read16Direct(memory, cpu, 0x04u);

        LoadA16(cpu, step);
        LsrA16(cpu);
        LsrA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x11bbu);
        TransferAToX(cpu);
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) & 0x0008u));
        AslA16(cpu);
        StoreADirect16(memory, cpu, 0x00u);
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) & 0x0007u));
        AslA16(cpu);
        StoreADirect16(memory, cpu, 0x02u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x02u));
        LoadY16(cpu, Read16Long(memory, LongIndexedAddress(0x82d230u, cpu->x)));
        LoadA16(cpu, (uint16_t)(step + 0x049cu));
        LoadX16(cpu, 0x0203u);
        Jsr(memory, cpu, 0xd1f9u);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x02u));
        LoadY16(cpu, Read16Long(memory, LongIndexedAddress(0x82d23cu, cpu->x)));
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x03dcu));
        LoadX16(cpu, 0x0203u);
        Jsr(memory, cpu, 0xd20cu);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadY16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x00u) + 0x116cu));
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x035cu));
        LoadX16(cpu, 0x0202u);
        Jsr(memory, cpu, 0xd21fu);
        MenuTileBlock(memory, cpu);
        Rts(memory, cpu);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x04u) + 0x0004u));
        StoreADirect16(memory, cpu, 0x04u);
        Compare16(cpu, cpu->accumulator, 0x001cu);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:A2E3: capsule monster screen. */
Lufia2ExecutionResult Lufia2MenuCapsuleScreen(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x03d8u);
    LoadX16(cpu, 0x1209u);
    Jsr(memory, cpu, 0xa2edu);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x05u);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11a4u, 0));
    StoreAAbsolute8(memory, cpu, 0x14f0u, 0);
    LoadAAbsolute8(memory, cpu, 0x11a3u, 0);
    StoreAAbsolute8(memory, cpu, 0x14deu, 0);
    LoadA8(cpu, 0x08u);
    LoadX16(cpu, 0x000au);
    Jsr(memory, cpu, 0xa306u);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0x005bu);
    LoadX16(cpu, 0x0005u);
    Jsr(memory, cpu, 0xa30fu);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xa312u);
    CapsulePortraits(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x82a317u);
}

/* $82:9189: small 3-digit number, hundreds tile always drawn. */
static void MenuSmallNumberFull(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Jsr(memory, cpu, 0x918bu);
    MenuDigits3(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, 0x0050u);
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator |
        Read16AbsoluteIndexed(memory, cpu, 0x155cu, 0)));
    Write16Long(memory, LongIndexedAddress(0x7e3000u, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    if (!cpu->zero) {
        LoadA16(cpu, 0x0060u);
        StoreADirect16(memory, cpu, 0x11u);
    }
    Jsr(memory, cpu, 0x91a9u);
    MenuNumberTail(memory, cpu);
    Rts(memory, cpu);
}

/* $82:950E: level, HP and MP of member [$2A] at X; HP colour by ratio. */
Lufia2ExecutionResult Lufia2MenuMemberStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kColumns[5] = {
        0x0004u, 0x0084u, 0x0080u, 0x0044u, 0x0040u};
    static const struct {
        uint16_t field;
        int full;
        uint16_t ret;
    } kValues[4] = {
        {0x0027u, 1, 0x956fu}, {0x0013u, 0, 0x9576u},
        {0x0025u, 1, 0x957du}, {0x0011u, 0, 0x9584u}};
    const uint16_t base = cpu->x;
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 5u; ++i) {                                 /* pushed columns */
        LoadA16(cpu, (uint16_t)(base + kColumns[4u - i]));
        cpu->carry = 0;
        PushAccumulator16(memory, cpu);
    }
    LoadY16(cpu, Read16Direct(memory, cpu, 0x2au));
    LoadA16(cpu, 0x3900u);
    StoreAAbsolute16(memory, cpu, 0x155cu, 0);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x000eu, cpu->y) & 0x00ffu));
    Jsr(memory, cpu, 0x953du);
    MenuSmallNumber(memory, cpu);
    Rts(memory, cpu);
    {
        const uint16_t flags = Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y);
        const uint16_t hp = Read16AbsoluteIndexed(memory, cpu, 0x0025u, cpu->y);
        const uint16_t now = Read16AbsoluteIndexed(memory, cpu, 0x0011u, cpu->y);
        uint16_t colour = 0;

        if ((flags & 0x0005u) || now <= (uint16_t)(hp >> 3)) {
            colour = 0x3500u;                                  /* 9563 */
            LoadA16(cpu, (flags & 0x0005u) ? flags : (uint16_t)(hp >> 3));
        } else if (now <= (uint16_t)(hp >> 2)) {
            colour = 0x3100u;
            LoadA16(cpu, (uint16_t)(hp >> 2));
        } else {
            LoadA16(cpu, (uint16_t)(hp >> 2));
        }
        Compare16(cpu, cpu->accumulator, now);
        if (colour) {
            LoadA16(cpu, colour);
            StoreAAbsolute16(memory, cpu, 0x155cu, 0);
        }
    }
    for (i = 0; i < 4u; ++i) {
        cpu->x = PullIndexValue(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, kValues[i].field, cpu->y));
        Jsr(memory, cpu, kValues[i].ret);
        if (kValues[i].full)
            MenuSmallNumberFull(memory, cpu);
        else
            MenuSmallNumber(memory, cpu);
        Rts(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x829587u);
}

/* $82:CCFE: $11 = learned-bit mask of slot A for the form. */
static void CapsuleSkillMask(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ed8c3u, cpu->x)));
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a4u, 0) & 0x00ffu));
    for (;;) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));       /* CD19 */
        if (cpu->zero)
            break;
        {
            uint16_t mask = Read16Direct(memory, cpu, 0x11u);

            mask = (uint16_t)(mask << 3);
            Write16Direct(memory, cpu, 0x11u, mask);
        }
    }
    SetAccumulatorWidth(cpu, 1);
}

/* $82:C4B3: skill id of slot A (learned or not). */
static void CapsuleSkill(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x19u);
    Write8(memory, DirectAddress(cpu, 0x1au), 0);
    Jsr(memory, cpu, 0xc4b9u);
    CapsuleSkillMask(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc4beu);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc4c1u);                                 /* C4A2 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a3u, 0) & 0x00ffu));
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x11cau);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    Rts(memory, cpu);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x1bu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09c4u, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x19u));
    StoreADirect16(memory, cpu, 0x19u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    {
        const uint16_t mask = Read16Direct(memory, cpu, 0x11u);

        cpu->zero = (cpu->accumulator & mask) == 0;            /* BIT $11 */
        cpu->negative = (mask & 0x8000u) != 0;
        cpu->overflow = (mask & 0x4000u) != 0;
    }
    LoadY16(cpu, cpu->zero ? 0x000fu : 0x0012u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x19u));
}

/* $82:C3B1: X = name of skill A ($A5:DF00). */
static void CapsuleSkillName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa5df00u, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xdf00u);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
}

static int String8E(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t string, uint16_t return_address, Lufia2ExecutionResult *out) {
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, string);
    return DrawString(memory, cpu, return_address, out);
}

/* $82:D07B: capsule status: stats, class and three skills. */
Lufia2ExecutionResult Lufia2MenuCapsuleStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kSkillReturns[3] = {0xd0e8u, 0xd0f2u, 0xd0fcu};
    static const uint16_t kSkillRows[3] = {0x3364u, 0x32e4u, 0x3264u};
    Lufia2ExecutionResult text;
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0098u);
    LoadX16(cpu, 0x140eu);
    Jsr(memory, cpu, 0xd085u);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreA8Absolute(memory, cpu, 0x0564u, 0x3cu);
    if (String8E(memory, cpu, 0xc6cdu, 0xd097u, &text))
        return text;
    Jsr(memory, cpu, 0xd09au);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);                     /* PHK PLA */
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, 0x0au);
    LoadX16(cpu, 0x10dfu);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xc8dbu);
    LoadX16(cpu, 0x3064u);
    if (DrawString(memory, cpu, 0xd0b1u, &text))
        return text;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09c4u, 0));
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x0au);
    StoreXDirect16(memory, cpu, 0x08u);
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x97u);
    LoadAAbsolute8(memory, cpu, 0x000eu, cpu->x);
    StoreADirect8(memory, cpu, 0x00u);
    LoadAAbsolute8(memory, cpu, 0x000du, cpu->x);
    StoreADirect8(memory, cpu, 0x01u);
    PullDataBank(memory, cpu);
    if (String8E(memory, cpu, 0xc85eu, 0xd0d5u, &text))
        return text;
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xc8dbu);
    LoadX16(cpu, 0x31e4u);
    if (DrawString(memory, cpu, 0xd0e3u, &text))
        return text;
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, (uint8_t)(2u - i));
        Jsr(memory, cpu, kSkillReturns[i]);
        CapsuleSkill(memory, cpu);
        Rts(memory, cpu);
        LoadX16(cpu, kSkillRows[i]);
        PushIndex(memory, cpu);
        PushAccumulator8(memory, cpu);
    }
    LoadA8(cpu, 0xa5u);
    StoreADirect8(memory, cpu, 0x0au);
    LoadA8(cpu, 0x03u);
    StoreADirect8(memory, cpu, 0x15u);
    do {
        LoadA8(cpu, Pull8(memory, cpu));
        Jsr(memory, cpu, 0xd10du);
        CapsuleSkillName(memory, cpu);
        Rts(memory, cpu);
        StoreXDirect16(memory, cpu, 0x08u);
        LoadA8(cpu, 0x8eu);
        StoreADirect8(memory, cpu, 0x5fu);
        LoadY16(cpu, 0xc8dbu);
        LoadX16(cpu, PullIndexValue(memory, cpu));             /* PLX */
        if (DrawString(memory, cpu, 0xd11bu, &text))
            return text;
        {
            const uint8_t left = (uint8_t)(DirectByte(memory, cpu, 0x15u) - 1u);

            Write8(memory, DirectAddress(cpu, 0x15u), left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    return ExecutionReturned(0x82d120u);
}

/* $82:E46B / $82:E484: count words to 0 or bytes to $FF at [$08]. */
static void ShopCount(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    int bytes) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0xffffu);
    do {
        IncrementY16(cpu);
        LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0x08u)));
        if (bytes)
            And16(cpu, 0x00ffu);
        Increment16Direct(memory, cpu, 0x08u);
        if (!bytes)
            Increment16Direct(memory, cpu, 0x08u);
        Compare16(cpu, cpu->accumulator, bytes ? 0x00ffu : 0x0000u);
    } while (!cpu->zero);
    LoadA16(cpu, cpu->y);
    Write16Long(memory, LongIndexedAddress(0x7e93d0u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:E319: list X starts at [$08]. */
static void ShopList(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t lists = AbsoluteIndexedAddress(cpu, 0x1542u, 0);

    Write8(memory, lists, (uint8_t)(Read8(memory, lists) + 1u));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x08u));
    Write16Long(memory, LongIndexedAddress(0x7e93c0u, cpu->x), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:FC7D: A = owned count nibble of item X ($7F:F080). */
static void ItemCount(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferXToA(cpu);
    And16(cpu, 0x01ffu);
    if (cpu->zero)
        return;
    LsrA16(cpu);
    TransferAToX(cpu);
    {
        const int high = cpu->carry;

        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ff080u, cpu->x)));
        if (!high) {
            And16(cpu, 0x000fu);
            return;
        }
        And16(cpu, 0x00f0u);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
    }
}

/* $82:E433: owned items (count << 9 | id) into $7E:97E0, 0-terminated. */
static void ShopOwnedItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    SetBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    Write16Direct(memory, cpu, 0x00u, 0);
    LoadY16(cpu, 0x0000u);
    do {
        LoadX16(cpu, Read16Direct(memory, cpu, 0x00u));
        Jsr(memory, cpu, 0xe443u);
        ItemCount(memory, cpu);
        Rts(memory, cpu);
        Compare16(cpu, cpu->accumulator, 0x0000u);
        if (!cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            AslA16(cpu);
            LoadA16(cpu, (uint16_t)(cpu->accumulator | Read16Direct(memory, cpu, 0x00u)));
            StoreAAbsolute16(memory, cpu, 0x97e0u, cpu->y);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
        Increment16Direct(memory, cpu, 0x00u);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
        Compare16(cpu, cpu->accumulator, 0x0200u);
    } while (!cpu->zero);
    LoadA16(cpu, cpu->y);
    LsrA16(cpu);
    Write16Long(memory, 0x7e93d0u, cpu->accumulator);
    LoadA16(cpu, 0x0000u);
    StoreAAbsolute16(memory, cpu, 0x97e0u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

/* $82:E297: shop $30: kind, flags and item lists. */
Lufia2ExecutionResult Lufia2MenuShopSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[4][2] = {
        {0xe2cau, 0xe2cdu}, {0xe2dau, 0xe2ddu}, {0xe2eau, 0xe2edu},
        {0xe2fau, 0xe2fdu}};
    unsigned i;

    Jsr(memory, cpu, 0xe299u);                                 /* E729 */
    LoadX16(cpu, 0xee9fu);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x0au);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x30u) & 0x00ffu));
    AslA16(cpu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0x08u));
    cpu->carry = 0;
    Add16Value(cpu, 0xee9fu);
    StoreADirect16(memory, cpu, 0x08u);
    SetAccumulatorWidth(cpu, 1);
    Rts(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0x08u)));
    StoreAAbsolute8(memory, cpu, 0x1543u, 0);
    SetAccumulatorWidth(cpu, 0);
    Increment16Direct(memory, cpu, 0x08u);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0x08u)));
    StoreAAbsolute16(memory, cpu, 0x1540u, 0);
    Increment16Direct(memory, cpu, 0x08u);
    Increment16Direct(memory, cpu, 0x08u);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x1542u, 0);
    LoadAAbsolute8(memory, cpu, 0x1540u, 0);
    if (A8(cpu) & 0x01u) {
        const uint32_t lists = AbsoluteIndexedAddress(cpu, 0x1542u, 0);

        Write8(memory, lists, (uint8_t)(Read8(memory, lists) + 1u));
        Jsr(memory, cpu, 0xe2bdu);
        ShopOwnedItems(memory, cpu);
        Rts(memory, cpu);
    }
    for (i = 0; i < 5u; ++i) {                                 /* bits 1-5 */
        LoadAAbsolute8(memory, cpu, 0x1540u, 0);
        if (!(A8(cpu) & (0x02u << i)))
            continue;
        LoadX16(cpu, (uint16_t)(2u + 2u * i));
        Jsr(memory, cpu, i < 4u ? kReturns[i][0] : 0xe30au);
        ShopList(memory, cpu);
        Rts(memory, cpu);
        Jsr(memory, cpu, i < 4u ? kReturns[i][1] : 0xe30du);
        ShopCount(memory, cpu, i == 4u);
        Rts(memory, cpu);
    }
    LoadAAbsolute8(memory, cpu, 0x1540u, 0);
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        const uint32_t lists = AbsoluteIndexedAddress(cpu, 0x1542u, 0);
        const uint8_t count = (uint8_t)(Read8(memory, lists) + 1u);

        Write8(memory, lists, count);
        SetNz8(cpu, count);
    }
    return ExecutionReturned(0x82e318u);
}

/* $82:C4A2: X = skill flags of the capsule ($11CA + 2n). */
static void CapsuleSkillFlags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a3u, 0) & 0x00ffu));
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x11cau);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:CD41: learn skill slot A if the form has it; carry = no. */
static void CapsuleLearn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x13u);
    Write8(memory, DirectAddress(cpu, 0x14u), 0);
    Jsr(memory, cpu, 0xcd47u);
    CapsuleSkillMask(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xcd4au);
    CapsuleSkillFlags(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    {
        const uint16_t mask = Read16Direct(memory, cpu, 0x11u);  /* BIT $11 */

        cpu->zero = (cpu->accumulator & mask) == 0;
        cpu->negative = (mask & 0x8000u) != 0;
        cpu->overflow = (mask & 0x4000u) != 0;
    }
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 1);
        cpu->carry = 1;
        return;
    }
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0xcd58u);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x1bu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
    StoreADirect16(memory, cpu, 0x19u);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0012u);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x19u));
    if (cpu->zero) {
        cpu->carry = 1;
        return;
    }
    Jsr(memory, cpu, 0xcd70u);
    CapsuleSkillFlags(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) |
        Read16Direct(memory, cpu, 0x11u)));
    StoreAAbsolute16(memory, cpu, 0x0000u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    cpu->carry = 0;
}

static void RandomBelow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t limit, uint16_t return_address) {
    LoadA8(cpu, limit);
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:CD1F: 1 in 8, learn a random skill; carry clear, A = skill. */
Lufia2ExecutionResult Lufia2CapsuleTryLearn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    RandomBelow(memory, cpu, 0x08u, 0xcd24u);
    Compare8(cpu, A8(cpu), 0x00u);
    if (!cpu->zero) {
        cpu->carry = 1;
        return ExecutionReturned(0x82cd40u);
    }
    RandomBelow(memory, cpu, 0x03u, 0xcd2eu);
    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->carry)
        return ExecutionReturned(0x82cd40u);
    PushAccumulator8(memory, cpu);
    Jsr(memory, cpu, 0xcd36u);
    CapsuleLearn(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    if (cpu->carry)
        return ExecutionReturned(0x82cd40u);
    Jsr(memory, cpu, 0xcd3cu);
    CapsuleSkill(memory, cpu);
    Rts(memory, cpu);
    cpu->carry = 0;
    return ExecutionReturned(0x82cd3eu);
}

/* $82:E624: stats of the new item vs now, green or red. */
static void ShopCompareStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadY16(cpu, 0x0000u);
    do {
        uint16_t colour = 0;

        LoadA16(cpu, Indirect16At(memory, cpu, 0xedu, 0));
        And16(cpu, 0xff00u);
        if (!cpu->zero) {
            const uint16_t now = Indirect16At(memory, cpu, 0xe7u, cpu->y);
            const uint16_t next = Indirect16At(memory, cpu, 0xeau, cpu->y);

            LoadA16(cpu, now);
            Compare16(cpu, now, next);
            if (!cpu->zero)
                colour = cpu->carry ? 0x3500u : 0x3100u;
        }
        if (!colour) {
            LoadA16(cpu, 0x0000u);                             /* E640 */
            Write16Long(memory, LongIndexedAddress(0x7e3000u, cpu->x), 0);
            Write16Long(memory, LongIndexedAddress(0x7e3002u, cpu->x), 0);
        } else {
            LoadA16(cpu, colour);
            StoreAAbsolute16(memory, cpu, 0x155cu, 0);
            LoadA16(cpu, Indirect16At(memory, cpu, 0xeau, cpu->y));
            Jsr(memory, cpu, 0xe655u);
            MenuSmallNumberFull(memory, cpu);
            Rts(memory, cpu);
        }
        TransferXToA(cpu);                                     /* E656 */
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x000eu);
    } while (!cpu->zero);
}

/* $82:E664: member sprite X pose by equip flags. */
static void ShopMemberPose(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t pose;

    LoadA16(cpu, Indirect16At(memory, cpu, 0xedu, 0));
    SetAccumulatorWidth(cpu, 1);
    if (A8(cpu) == 0) {
        pose = 0x00u;
    } else {
        ExchangeAccumulatorBytes(cpu);
        pose = A8(cpu) == 0x02u ? 0x03u : A8(cpu) == 0x01u ? 0x00u : 0x02u;
    }
    LoadA8(cpu, pose);
    StoreZeroAbsolute8(memory, cpu, 0x1448u, cpu->x);
    SimulateJslFrame(memory, cpu, 0x82u, 0xe685u);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
}

/* $82:E5E1: equipment comparison for each member. */
Lufia2ExecutionResult Lufia2MenuShopCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t kPointers[3] = {0xe7u, 0xeau, 0xedu};
    static const uint16_t kOffsets[3] = {0x0029u, 0x0037u, 0x0048u};
    unsigned i;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0a7au, 0) & 0x00ffu));
    StoreADirect16(memory, cpu, 0x26u);
    LoadX16(cpu, 0x0000u);
    do {
        PushIndex(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a80u, cpu->x));
        StoreADirect16(memory, cpu, 0x2au);
        for (i = 0; i < 3u; ++i) {
            LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x2au) + kOffsets[i]));
            cpu->carry = 0;
            StoreADirect16(memory, cpu, kPointers[i]);
        }
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x82e689u, cpu->x)));
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe611u);
        ShopCompareStats(memory, cpu);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        TransferXToA(cpu);
        LsrA16(cpu);
        PushIndex(memory, cpu);
        TransferAToX(cpu);
        Jsr(memory, cpu, 0xe619u);
        ShopMemberPose(memory, cpu);
        Rts(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x26u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x82e623u);
}

/* $82:9918: price X halved (rounded up) unless a member holds item $167. */
static void ShopPriceHalve(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1544u, 0);
    if (!cpu->zero) {
        int halve = 0;

        LoadY16(cpu, 0x0000u);
        for (;;) {
            LoadAAbsolute8(memory, cpu, 0x0a7bu, cpu->y);
            Compare8(cpu, A8(cpu), 0x01u);
            if (cpu->zero) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0cd9u, 0) & 0x01ffu));
                Compare16(cpu, cpu->accumulator, 0x0167u);
                halve = cpu->zero;
                break;
            }
            IncrementY16(cpu);
            TransferYToA8(cpu);
            Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x0a7au, 0));
            if (cpu->zero)
                break;
        }
        if (!halve) {
            UnpackStatus(cpu, Pull8(memory, cpu));
            cpu->y = PullIndexValue(memory, cpu);
            return;
        }
    }
    SetAccumulatorWidth(cpu, 0);                               /* 9943 */
    TransferXToA(cpu);
    LsrA16(cpu);
    Add16Value(cpu, 0x0000u);
    TransferAToX(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
}

/* $82:98C7: 24-bit $09BD = X * 200. */
static void ShopPriceTimes200(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t kProducts[3] = {0x54u, 0x56u, 0x58u};
    unsigned i;

    Write16Absolute(memory, cpu, 0x09bdu, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x09bfu, 0);
    StoreA8Absolute(memory, cpu, 0x4202u, 0xc8u);
    for (i = 0; i < 3u; ++i) {
        if (i < 2u)
            LoadAAbsolute8(memory, cpu, (uint16_t)(0x09bdu + i), 0);
        else
            LoadA8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x4203u, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
        StoreXDirect16(memory, cpu, kProducts[i]);
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x56u));
    StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    Adc8(cpu, DirectByte(memory, cpu, 0x58u));
    StoreADirect8(memory, cpu, 0x56u);
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, DirectByte(memory, cpu, (uint8_t)(0x54u + i)));
        StoreAAbsolute8(memory, cpu, (uint16_t)(0x09bdu + i), 0);
    }
}

/* $82:85D2: $15 shop rows from list [$FC] at index $00: name, price. */
static int ShopRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreA8Absolute(memory, cpu, 0x0564u, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    AslA16(cpu);
    TransferAToX(cpu);
    do {
        StoreXDirect16(memory, cpu, 0x19u);                    /* 85E1 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7e93e0u, cpu->x)));
        StoreADirect16(memory, cpu, 0x02u);
        LoadY16(cpu, cpu->x);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xfcu));
        And16(cpu, 0x01ffu);
        if (cpu->zero)
            break;
        StoreADirect16(memory, cpu, 0x00u);
        SetAccumulatorWidth(cpu, 1);
        LoadY16(cpu, 0xc9b1u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x85fdu, text))
            return 1;
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0b89u, 0));
        Jsr(memory, cpu, 0x8603u);
        ShopPriceHalve(memory, cpu);
        Rts(memory, cpu);
        Write16Absolute(memory, cpu, 0x0b89u, cpu->x);
        LoadAAbsolute8(memory, cpu, 0x154cu, 0);
        Compare8(cpu, A8(cpu), 0x01u);
        if (cpu->zero) {
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0b89u, 0));
            Jsr(memory, cpu, 0x8613u);
            ShopPriceTimes200(memory, cpu);
            Rts(memory, cpu);
        }
        LoadY16(cpu, 0xc9c2u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x861cu, text))
            return 1;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x15u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);                               /* 862F */
    TsbDirect(memory, cpu, 0x74u, 0x08u);
    return 0;
}

/* $82:DCF4: one shop row at $3708; M0. */
Lufia2ExecutionResult Lufia2MenuShopRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0x3708u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0001u);
    StoreXDirect16(memory, cpu, 0x15u);
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0xdd02u);
    if (ShopRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    return ExecutionReturned(0x82dd05u);
}

/* $82:DCC1: five shop rows from $3488. */
Lufia2ExecutionResult Lufia2MenuShopRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0x3488u);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0005u);
    StoreXDirect16(memory, cpu, 0x15u);
    Jsr(memory, cpu, 0xdccdu);
    if (ShopRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    return ExecutionReturned(0x82dcceu);
}

/* $82:F703: Y = equipment slot A offset ($66 + 2A), also $1523. */
static void EquipSlot(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0066u);
    StoreAAbsolute16(memory, cpu, 0x1523u, 0);
    TransferAToY(cpu);
}

/* $82:F893: carry = slot A holds an item with record flag 8. */
static void EquipFlagged(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t slot_return) {
    Jsr(memory, cpu, slot_return);
    EquipSlot(memory, cpu);
    Rts(memory, cpu);
    LoadA16(cpu, (uint16_t)(Indirect16At(memory, cpu, 0x2au, cpu->y) & 0x01ffu));
    StoreAAbsolute16(memory, cpu, 0x0a06u, 0);
    if (cpu->zero) {
        cpu->carry = 0;
        return;
    }
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x82u, 0xf8a4u);
    (void)Lufia2ItemRecordByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    And16(cpu, 0x0008u);
    cpu->carry = !cpu->zero;
}

/* $82:F846: equipment bonuses of member [$2A] from its six slots. */
static void EquipBonuses(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned slot;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    Jsr(memory, cpu, 0xf84bu);
    Lufia2BonusClear(memory, cpu, 0x0086u, 7u);
    Rts(memory, cpu);
    for (slot = 0; slot < 6u; ++slot) {
        LoadA16(cpu, (uint16_t)slot);
        PushAccumulator16(memory, cpu);
        Jsr(memory, cpu, 0xf852u);
        EquipSlot(memory, cpu);
        Rts(memory, cpu);
        Jsr(memory, cpu, 0xf855u);                             /* F710 */
        LoadA16(cpu, Indirect16At(memory, cpu, 0x2au, cpu->y));
        if (cpu->zero) {
            cpu->carry = 1;
            Rts(memory, cpu);
        } else {
            And16(cpu, 0x01ffu);
            StoreAAbsolute16(memory, cpu, 0x0a06u, 0);
            SimulateJslFrame(memory, cpu, 0x82u, 0xf71du);
            (void)Lufia2LoadItemRecord(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 0;
            Rts(memory, cpu);
            Jsr(memory, cpu, 0xf85au);                         /* F722 */
            {
                unsigned i;

                LoadX16(cpu, Read16Direct(memory, cpu, 0x2au));
                for (i = 0; i < 7u; ++i) {
                    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                        (uint16_t)(0x0086u + 2u * i), cpu->x));
                    cpu->carry = 0;
                    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu,
                        (uint16_t)(0x0b95u + 2u * i), 0));
                    StoreAAbsolute16(memory, cpu, (uint16_t)(0x0086u + 2u * i), cpu->x);
                }
            }
            Rts(memory, cpu);
        }
        PullAccumulator16(memory, cpu);
        IncrementA16(cpu);
        Compare16(cpu, cpu->accumulator, 0x0006u);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $82:B2C5: with $0A62 = $25, upgrade flagged equipment of member
   $14B3; carry clear when something changed. */
Lufia2ExecutionResult Lufia2MenuEquipUpgrade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint16_t item = Read16AbsoluteIndexed(memory, cpu, 0x0a06u, 0);
    int changed = 0;

    LoadX16(cpu, item);
    PushIndex(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0a62u, 0);
    Compare8(cpu, A8(cpu), 0x25u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x14b3u, 0);
        StoreADirect8(memory, cpu, 0x22u);
        Jsr(memory, cpu, 0xb2d7u);                             /* 9971 */
        PushY(memory, cpu);
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 0);
        SetIndexWidth(cpu, 0);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x22u) & 0x00ffu));
        AslA16(cpu);
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a80u, cpu->y));
        StoreADirect16(memory, cpu, 0x2au);
        UnpackStatus(cpu, Pull8(memory, cpu));
        cpu->y = PullIndexValue(memory, cpu);
        Rts(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        Jsr(memory, cpu, 0xb2dcu);                             /* F87D count */
        LoadX16(cpu, 0x0000u);
        TransferXToA(cpu);
        do {
            PushIndex(memory, cpu);
            PushAccumulator16(memory, cpu);
            TransferXToA(cpu);
            Jsr(memory, cpu, 0xf886u);
            EquipFlagged(memory, cpu, 0xf895u);
            Rts(memory, cpu);
            PullAccumulator16(memory, cpu);
            Add16Value(cpu, 0x0000u);
            cpu->x = PullIndexValue(memory, cpu);
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, 0x0006u);
        } while (!cpu->zero);
        Rts(memory, cpu);
        Compare16(cpu, cpu->accumulator, 0x0000u);
        if (!cpu->zero) {
            Jsr(memory, cpu, 0xb2e4u);                         /* F8C6 */
            LoadA16(cpu, 0x0000u);
            do {
                PushAccumulator16(memory, cpu);
                Jsr(memory, cpu, 0xf8ccu);
                EquipFlagged(memory, cpu, 0xf895u);
                Rts(memory, cpu);
                if (cpu->carry) {
                    const uint32_t at = (((uint32_t)cpu->data_bank << 16) +
                        Read16Direct(memory, cpu, 0x2au) + cpu->y) & 0x00ffffffu;
                    const uint16_t next = (uint16_t)(Read16Long(memory, at) + 1u);

                    Write16Long(memory, at, next);
                    SetNz16(cpu, next);
                }
                PullAccumulator16(memory, cpu);
                IncrementA16(cpu);
                Compare16(cpu, cpu->accumulator, 0x0006u);
            } while (!cpu->zero);
            Rts(memory, cpu);
            Jsr(memory, cpu, 0xb2e7u);
            EquipBonuses(memory, cpu);
            Rts(memory, cpu);
            LoadX16(cpu, Read16Direct(memory, cpu, 0x2au));
            SimulateJslFrame(memory, cpu, 0x82u, 0xb2edu);
            (void)Lufia2PartyDerivedStats(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            changed = 1;
        }
    }
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, PullIndexValue(memory, cpu));
    Write16Absolute(memory, cpu, 0x0a06u, cpu->x);
    cpu->carry = !changed;
    return ExecutionReturned(changed ? 0x82b2f5u : 0x82b2fdu);
}

/* $82:89C4: two cursor sprites: X (animation $57, or off), $5A. */
static void MenuCursorPair(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreXDirect16(memory, cpu, 0x58u);
    StoreYDirect16(memory, cpu, 0x5au);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    if (cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x11d8u, cpu->x);
    } else {
        Jsr(memory, cpu, 0x89d3u);
        MenuCursorAnimation(memory, cpu);
        Rts(memory, cpu);
    }
    LoadX16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x5au) - 5u));
    StoreXDirect16(memory, cpu, 0x63u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5au));
    Jsr(memory, cpu, 0x89e3u);
    MenuCursorAnimation(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x54u));
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    Jsr(memory, cpu, 0x89eau);
    MenuCursorPlace(memory, cpu);
    Rts(memory, cpu);
}

/* $82:8C9C: scrollbar thumb sprite (slot 11) at Y. */
static void MenuScrollThumb(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x000bu);
    StoreAAbsolute8(memory, cpu, 0x13e8u, cpu->x);
    StoreAAbsolute8(memory, cpu, 0x150au, 0);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x1388u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x1418u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x13b8u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x1509u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x1448u, cpu->x);
    LoadA8(cpu, 0x04u);
    SimulateJslFrame(memory, cpu, 0x82u, 0x8cbfu);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:8CC1: thumb step = $150B / rows past the window. */
static void MenuScrollStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x00u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x152eu, 0));
    Compare16(cpu, cpu->x, 0x0000u);
    if (!cpu->negative && !cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x150bu, 0));
        StoreXDirect16(memory, cpu, 0x4eu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x152eu, 0));
        StoreXDirect16(memory, cpu, 0x51u);
        SimulateJslFrame(memory, cpu, 0x82u, 0x8cdau);
        (void)Lufia2Divide16(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x4eu));
        Write16Absolute(memory, cpu, 0x150du, cpu->x);
        LoadA8(cpu, 0x01u);
    }
    LoadX16(cpu, 0x000bu);
    StoreAAbsolute8(memory, cpu, 0x11d8u, cpu->x);
}

/* $82:8C43: thumb y = $1509 + row * step; frame 4 at the ends. */
static void MenuScrollPlace(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x000bu);
    LoadAAbsolute8(memory, cpu, 0x11d8u, cpu->x);
    if (cpu->zero)
        return;
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x150fu, 0));
    Write16Absolute(memory, cpu, 0x1570u, cpu->y);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x150du, 0));
    Write16Absolute(memory, cpu, 0x1572u, cpu->x);
    Lufia2CallMultiply(memory, cpu, 0x82u, 0x8c5cu);
    LoadX16(cpu, 0x000bu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1509u, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1574u, 0));
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x13e8u, cpu->x);
    LoadA8(cpu, 0x04u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x150fu, 0));
    if (!cpu->zero) {
        Compare16(cpu, cpu->y, Read16AbsoluteIndexed(memory, cpu, 0x152eu, 0));
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
    }
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x1208u, cpu->x));
    if (cpu->zero)
        return;
    SimulateJslFrame(memory, cpu, 0x82u, 0x8c83u);
    (void)Lufia2SpriteSetAnimation(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:8C85: scroll range from X rows (carry: halve), then thumb. */
static void MenuScrollbar(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    if (cpu->carry) {
        IncrementA16(cpu);
        LsrA16(cpu);
    }
    Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
    StoreAAbsolute16(memory, cpu, 0x152eu, 0);
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0x8c97u);
    MenuScrollStep(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0x8c9au);
    MenuScrollPlace(memory, cpu);
    Rts(memory, cpu);
}

/* $82:8680: $15 rows of $7E:943C string pointers ($8E) from index $00. */
static int MenuPointerRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *text) {
    StoreA8Absolute(memory, cpu, 0x0564u, 0x20u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x0au);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x00u));
    AslA16(cpu);
    TransferAToX(cpu);
    do {
        StoreXDirect16(memory, cpu, 0x19u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7e943cu, cpu->x)));
        Compare16(cpu, cpu->accumulator, 0xffffu);
        if (cpu->zero)
            break;
        StoreADirect16(memory, cpu, 0x08u);
        SetAccumulatorWidth(cpu, 1);
        LoadY16(cpu, 0xd062u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x11u));
        if (DrawString(memory, cpu, 0x86aau, text))
            return 1;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x11u);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x19u));
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x15u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    TsbDirect(memory, cpu, 0x74u, 0x08u);
    return 0;
}

/* $82:9CB2: warp destinations list with scrollbar. */
Lufia2ExecutionResult Lufia2MenuWarpList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult text;

    LoadX16(cpu, 0xffb0u);
    SimulateJslFrame(memory, cpu, 0x82u, 0x9cb8u);
    MenuClearSlots(memory, cpu, 0x11d8u);
    SimulateRtlFrame(memory, cpu);
    StoreA8Absolute(memory, cpu, 0x153fu, 0x01u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0342u);
    LoadX16(cpu, 0x1e13u);
    Jsr(memory, cpu, 0x9cc8u);
    MenuClearRect(memory, cpu);
    Rts(memory, cpu);
    Window(memory, cpu, 0x0348u, 0x1b03u, 0x9cd1u);
    Window(memory, cpu, 0x0408u, 0x1b0bu, 0x9cdau);
    SetAccumulatorWidth(cpu, 1);
    if (Text8E(memory, cpu, 0xd045u, 0x9cecu, &text))
        return text;
    LoadX16(cpu, 0x0003u);
    StoreZeroAbsolute8(memory, cpu, 0x14d9u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x14ebu, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x150fu, 0));
    Write16Long(memory, 0x7e93c0u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x14b5u, 0));
    Write16Long(memory, 0x7e93c2u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x0000u);
    Write16Absolute(memory, cpu, 0x150fu, 0);
    Write16Absolute(memory, cpu, 0x14b5u, 0);
    LoadX16(cpu, 0x003fu);
    StoreXDirect16(memory, cpu, 0x54u);
    LoadX16(cpu, 0x0001u);
    StoreXDirect16(memory, cpu, 0x56u);
    LoadX16(cpu, 0x0009u);
    LoadY16(cpu, 0x0008u);
    Jsr(memory, cpu, 0x9d23u);
    MenuCursorPair(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, 0xf08au);
    Jsr(memory, cpu, 0x9d29u);
    MenuScrollThumb(memory, cpu);
    Rts(memory, cpu);
    LoadX16(cpu, 0x4800u);
    Write16Absolute(memory, cpu, 0x150bu, 0x4800u);
    LoadAAbsolute8(memory, cpu, 0x14b9u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x14b7u, 0);
    cpu->carry = 1;
    Sbc8(cpu, 0x06u);
    if (!cpu->negative) {
        StoreAAbsolute8(memory, cpu, 0x14b7u, 0);
    } else {
        LoadAAbsolute8(memory, cpu, 0x14b9u, 0);
        StoreAAbsolute8(memory, cpu, 0x14fau, 0);
    }
    cpu->carry = 0;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x14b9u, 0));
    Jsr(memory, cpu, 0x9d4cu);
    MenuScrollbar(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0x9d4fu);                                 /* B052 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x040eu);
    LoadX16(cpu, 0x1410u);
    Jsr(memory, cpu, 0xb05cu);
    MenuClearRect2(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0x344eu);
    StoreXDirect16(memory, cpu, 0x11u);
    LoadX16(cpu, 0x0006u);
    StoreXDirect16(memory, cpu, 0x15u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x14b5u, 0));
    StoreXDirect16(memory, cpu, 0x00u);
    Jsr(memory, cpu, 0xb070u);
    if (MenuPointerRows(memory, cpu, &text))
        return text;
    Rts(memory, cpu);
    Rts(memory, cpu);
    TsbDirect(memory, cpu, 0x74u, 0x88u);
    return ExecutionReturned(0x829d54u);
}
