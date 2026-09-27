#ifndef LUFIA2_ITEM_H
#define LUFIA2_ITEM_H

/* Item and spell records. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $81:F1C5 item $0A06: name $0B77, record $0B84; any width. */
Lufia2ExecutionResult Lufia2LoadItemRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F414 spell $0A0B: name and record $0B77; M1X0. */
Lufia2ExecutionResult Lufia2LoadSpellRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F194 A = first record byte of item $0A06; any width. */
Lufia2ExecutionResult Lufia2ItemRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F3F4 A = spell record byte $0C; M1X0. */
Lufia2ExecutionResult Lufia2SpellRecordByteC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F404 A = spell record byte 8; M1X0. */
Lufia2ExecutionResult Lufia2SpellRecordByte8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F0A2: packed request $0A06 / quantity $09F4; M1X0, binary ADC. */
Lufia2ExecutionResult Lufia2InventoryAdd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F057 A = count of item $0A06 held; M1X0. */
Lufia2ExecutionResult Lufia2InventoryCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F2A9 item $0A06 name to $0B77, spaces cut; M1X0. */
Lufia2ExecutionResult Lufia2ItemNameTrimmed(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F291 $0A09 = text pointer of item $0A06; any width. */
Lufia2ExecutionResult Lufia2ItemTextPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F446 $0A0D = text pointer of spell $0A0B; M1X0. */
Lufia2ExecutionResult Lufia2SpellTextPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
