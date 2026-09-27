/* Map section tables ($80:EBAA-$80:ED0D).
 *
 * A map is split into sections described at $7F:D000: $D000 header pointer,
 * $D008 cell data pointer, $D010/$D018 width and height, $D020 the header's
 * first word; $7F:D038 is the running byte offset. Shared by the ordinary
 * map loader ($80:EB40-$80:EBA0) and the Ancient Cave ($83:99A7-$83:99C2).
 */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/system.h"

/* (dp),Y with the live DB. */
static uint32_t FieldDpIndirectY(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint8_t offset) {
    return AbsoluteIndexedAddress(
        cpu, Read16Direct(memory, cpu, offset), cpu->y);
}

Lufia2ExecutionResult Lufia2FieldReadSections(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7fu);                         /* EBAA */
    OpLdy(cpu, 0x0000u);
    OpLda(memory, cpu, FieldDpIndirectY(memory, cpu, 0x5du));
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpStz(memory, cpu, OpDp(cpu, 0x59u));
    OpRep(cpu, 0x30u);                                         /* EBB7 */
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0xd038u)));
    OpLda(memory, cpu, OpDp(cpu, 0x58u));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, 0xd038u));
    OpSta(memory, cpu, OpAbs(cpu, 0xd038u));
    OpLda(memory, cpu, OpDp(cpu, 0x5du));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0006u);
    OpSta(memory, cpu, OpDp(cpu, 0x5du));
    do {
        OpLda(memory, cpu, OpDp(cpu, 0x5du));                  /* EBCD */
        OpSta(memory, cpu, OpAbsX(cpu, 0xd000u));
        OpLdy(cpu, 0x0000u);
        OpLda(memory, cpu, FieldDpIndirectY(memory, cpu, 0x5du));
        OpSta(memory, cpu, OpAbsX(cpu, 0xd020u));
        OpSep(cpu, 0x20u);
        OpLdy(cpu, 0x0002u);                                   /* EBDC */
        OpLda(memory, cpu, FieldDpIndirectY(memory, cpu, 0x5du));
        OpSta(memory, cpu, OpAbsX(cpu, 0xd010u));
        OpSta(memory, cpu, 0x004202u);
        OpIny(cpu);
        OpLda(memory, cpu, FieldDpIndirectY(memory, cpu, 0x5du));
        OpSta(memory, cpu, OpAbsX(cpu, 0xd018u));
        OpSta(memory, cpu, 0x004203u);
        TransferDirectToA(cpu);                                /* EBF2 */
        OpSta(memory, cpu, OpAbsX(cpu, 0xd011u));
        OpSta(memory, cpu, OpAbsX(cpu, 0xd019u));
        OpRep(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, 0x5du));                  /* EBFB */
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0004u);
        OpSta(memory, cpu, OpAbsX(cpu, 0xd008u));
        cpu->carry = 0;
        OpAdc(memory, cpu, 0x004216u);                         /* 2 * w * h */
        OpAdc(memory, cpu, 0x004216u);
        OpSta(memory, cpu, OpDp(cpu, 0x5du));
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);
    } while (!cpu->zero);
    OpSep(cpu, 0x20u);                                         /* EC15 */
    return ExecutionReturned(0x80ec17u);
}

Lufia2ExecutionResult Lufia2FieldPackSectionAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* EC18 */
    OpLda(memory, cpu, 0x0005aau);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0xd010u));
    OpSta(memory, cpu, 0x004202u);
    OpLda(memory, cpu, OpAbsX(cpu, 0xd018u));
    OpSta(memory, cpu, 0x004203u);
    OpRep(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0xd008u));                  /* EC2E */
    OpTax(cpu);
    OpLda(memory, cpu, 0x004216u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0003u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpSep(cpu, 0x20u);
    OpLdy(cpu, 0x0000u);                                       /* EC40 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0001u));              /* EC43 */
        OpAndValue(cpu, 0x30u);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x0001u));              /* EC50 */
        OpAndValue(cpu, 0x30u);
        OpLsrA(cpu);
        OpLsrA(cpu);
        OpTestBits(memory, cpu, OpDp(cpu, 0x54u), 1);
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x0001u));              /* EC5B */
        OpAndValue(cpu, 0x30u);
        OpTestBits(memory, cpu, OpDp(cpu, 0x54u), 1);
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x0001u));              /* EC64 */
        OpAndValue(cpu, 0x30u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpOra(memory, cpu, OpDp(cpu, 0x54u));
        OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpCompareIndex(cpu, cpu->y,
            OpReadX(memory, cpu, OpDp(cpu, 0x58u)));
    } while (!cpu->zero);                                      /* EC75 */
    return ExecutionReturned(0x80ec77u);
}

Lufia2ExecutionResult Lufia2FieldSectionSize(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSep(cpu, 0x20u);                                         /* EC78 */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, 0x0005aau);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0xd010u));
    OpSta(memory, cpu, 0x0005b9u);
    OpLda(memory, cpu, OpAbsX(cpu, 0xd018u));
    OpSta(memory, cpu, 0x0005bbu);
    TransferDirectToA(cpu);                                    /* EC8E */
    OpSta(memory, cpu, 0x0005bau);
    OpSta(memory, cpu, 0x0005bcu);
    return ExecutionReturned(0x80ec97u);
}

/* JSL $80:8E9D from bank $80. */
static void FieldDecompress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJslFrame(memory, cpu, 0x80u, (uint16_t)(site + 3u));
    (void)Lufia2DecompressResource(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:ECF2: $2D += $58 (the decompressed size). */
static void FieldAdvanceDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpRep(cpu, 0x20u);                                         /* ECF2 */
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, 0x58u));
    OpSta(memory, cpu, OpDp(cpu, 0x2du));
    OpSep(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* ECFD */
}

/* $80:ECFE (M0): A = [$7F:(A + $D03A)] + $D03A. */
static void FieldRelativeWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    cpu->carry = 0;                                            /* ECFE */
    OpAdc(memory, cpu, 0x7fd03au);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    cpu->carry = 0;
    OpAdc(memory, cpu, 0x7fd03au);
    SimulateRtsFrame(memory, cpu);                             /* ED0D */
}

Lufia2ExecutionResult Lufia2FieldDecompressMapData(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, 0x54u));                      /* EC98 */
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    OpSta(memory, cpu, 0x7fd03au);
    OpSta(memory, cpu, OpDp(cpu, 0x60u));
    OpSep(cpu, 0x20u);
    LoadA8(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    FieldDecompress(memory, cpu, 0xeca8u);
    FieldAdvanceDestination(memory, cpu, 0xecacu);
    OpRep(cpu, 0x20u);                                         /* ECAF */
    OpLda(memory, cpu, 0x7fd03au);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0010u);
    OpSta(memory, cpu, 0x7fd03cu);
    LoadA16(cpu, 0x0004u);
    FieldRelativeWord(memory, cpu, 0xecc0u);
    OpSta(memory, cpu, 0x7fd03eu);
    OpSep(cpu, 0x20u);                                         /* ECC7 */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x29u));
    if (!cpu->zero) {
        OpAndValue(cpu, 0x0fu);                                /* ECCE */
        OpRep(cpu, 0x20u);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0166u);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpLda(memory, cpu, OpDp(cpu, 0x2du));
        OpSta(memory, cpu, OpDp(cpu, 0x60u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0010u);
        OpSta(memory, cpu, 0x7fd040u);
        OpSep(cpu, 0x20u);
        LoadA8(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, 0x62u));
        FieldDecompress(memory, cpu, 0xeceau);
        FieldAdvanceDestination(memory, cpu, 0xeceeu);
    }
    return ExecutionReturned(0x80ecf1u);                       /* ECF1 */
}
