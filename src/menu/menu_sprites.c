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
