/* Battle initialization at $81:8000. Child routines retain their ROM contracts. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult SetupChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SetupCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         Lufia2PushedChildCall child, void *context,
                         uint32_t site, uint32_t target, uint8_t frame_size) {
    if (frame_size == 2u)
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(memory, cpu, 0x81u, (uint16_t)(site + 3u));
    return child(context, cpu, target, site, frame_size);
}

#define SETUP_CALL(site, target, frame) \
    do { \
        if (!SetupCall(memory, cpu, child, child_context, \
                       0x810000u | (site), (target), (frame))) \
            return SetupChildUnwound(0x810000u | (site)); \
    } while (0)

Lufia2ExecutionResult Lufia2BattleSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    TransferDirectToA(cpu);                                 /* 8000 */
    OpSta(memory, cpu, 0x7ff8a5u);
    SETUP_CALL(0x8005u, 0x85edb2u, 3u);
    SETUP_CALL(0x8009u, 0x85edf1u, 3u);
    OpLdx(cpu, 0x0f20u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0562u), cpu->x);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0b5bu)));
    OpInx(cpu);
    if (!cpu->zero)
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0b5bu), cpu->x);

    OpLda(memory, cpu, 0x7ff8a3u);
    if (cpu->zero) {
        OpLda(memory, cpu, 0x7ff8a1u);
        OpCmpValue(cpu, 0x7fu);
        if (cpu->zero) goto special_one;
        OpCmpValue(cpu, 0x80u);
        if (cpu->zero) goto special_two;
        OpCmpValue(cpu, 0x3fu);
        if (cpu->zero) {
            LoadA8(cpu, 2u);
            SETUP_CALL(0x803au, 0x808299u, 3u);
            OpCmpValue(cpu, 0u);
            if (!cpu->zero) goto special_one;
        } else {
            OpCmpValue(cpu, 0xbfu);
            if (cpu->zero) {
                LoadA8(cpu, 2u);
                SETUP_CALL(0x8051u, 0x808299u, 3u);
                OpCmpValue(cpu, 0u);
                if (!cpu->zero) goto special_two;
            }
        }
    }
    OpLdx(cpu, 0u);                                         /* 8066 */
    OpWriteX(memory, cpu, OpAbs(cpu, 0x11e8u), cpu->x);
    LoadA8(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, 0x11e7u));
    goto formation;

special_one:
    OpLdx(cpu, 0xffffu);                                    /* 8042 */
    OpWriteX(memory, cpu, OpAbs(cpu, 0x11e8u), cpu->x);
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x11e7u));
    goto formation;
special_two:
    OpLdx(cpu, 0xffffu);                                    /* 8059 */
    OpWriteX(memory, cpu, OpAbs(cpu, 0x11e8u), cpu->x);
    LoadA8(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x11e7u));

formation:
    OpLda(memory, cpu, OpAbs(cpu, 0x0a7au));                /* 8071 */
    OpSta(memory, cpu, OpAbs(cpu, 0x153cu));
    OpLdx(cpu, 3u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a7bu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x153du));
        OpDex(cpu);
    } while (!cpu->negative);

    OpLdx(cpu, 3u);
    do {
        OpRepWidths(cpu, 0x20u);                            /* 8086 */
        OpTxa(cpu);
        OpAslA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0x0a80u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x0a64u));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
        OpSta(memory, cpu, OpAbsX(cpu, 0x1542u));
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0010u));
        OpDex(cpu);
    } while (!cpu->negative);

    OpLda(memory, cpu, OpAbs(cpu, 0x11e7u));
    OpBitValue(cpu, 1u);
    if (cpu->zero) SETUP_CALL(0x80a8u, 0x859419u, 3u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1104u));
    OpSta(memory, cpu, OpAbs(cpu, 0x10f0u));
    OpStz(memory, cpu, OpAbs(cpu, 0x160au));
    OpStz(memory, cpu, OpAbs(cpu, 0x1607u));
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, 0x1608u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1605u));
    OpStz(memory, cpu, OpAbs(cpu, 0x160bu));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x859ec8u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x0a6eu));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 12u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    SETUP_CALL(0x80d8u, 0x8181e6u, 2u);

    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x1345u));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, 0x0a6eu));
            OpSta(memory, cpu, OpAbsY(cpu, 0x0a6fu));
        } else {
            OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
            PushY(memory, cpu);
            OpPushX(memory, cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0x0a6eu)));
            OpWriteX(memory, cpu, OpAbs(cpu, 0x00b2u), cpu->x);
            SETUP_CALL(0x80fau, 0x81fc0bu, 3u);
            OpPullX(memory, cpu);
            OpPullY(memory, cpu);
        }
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, 6u);
    } while (!cpu->zero);

    OpLda(memory, cpu, OpAbs(cpu, 0x11e0u));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        LoadA8(cpu, 1u);
        OpSta(memory, cpu, 0x7ff8a5u);
    } else {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            const uint8_t slots[] = {2u, 2u, 1u, 0xffu, 0xffu};
            const uint16_t words[] = {0x0d29u, 0x0c6bu, 0u, 0u};
            for (uint16_t i = 0; i < 5u; ++i) {
                LoadA8(cpu, slots[i]);
                OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x153cu + i)));
            }
            for (uint16_t i = 0; i < 4u; ++i) {
                OpLdx(cpu, words[i]);
                OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(0x0a64u + 2u * i)), cpu->x);
            }
            OpLda(memory, cpu, OpAbs(cpu, 0x0d38u));
            OpSta(memory, cpu, OpAbs(cpu, 0x1542u));
            OpLda(memory, cpu, OpAbs(cpu, 0x0c7au));
            OpSta(memory, cpu, OpAbs(cpu, 0x1543u));
        } else {
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                const uint8_t slots[] = {2u, 0u, 4u, 0xffu, 0xffu};
                const uint16_t words[] = {0x0badu, 0x0ea5u, 0u, 0u};
                for (uint16_t i = 0; i < 5u; ++i) {
                    LoadA8(cpu, slots[i]);
                    OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x153cu + i)));
                }
                for (uint16_t i = 0; i < 4u; ++i) {
                    OpLdx(cpu, words[i]);
                    OpWriteX(memory, cpu, OpAbs(cpu, (uint16_t)(0x0a64u + 2u * i)), cpu->x);
                }
                OpLda(memory, cpu, OpAbs(cpu, 0x0bbcu));
                OpSta(memory, cpu, OpAbs(cpu, 0x1542u));
                OpLda(memory, cpu, OpAbs(cpu, 0x0eb4u));
                OpSta(memory, cpu, OpAbs(cpu, 0x1543u));
            }
        }
    }

    SETUP_CALL(0x8191u, 0x858a03u, 3u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1144u));
    OpSta(memory, cpu, OpAbs(cpu, 0x154du));
    OpLdx(cpu, 0x10dfu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0a6cu), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, 0x0a7fu));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x154cu));
    if (cpu->zero) {
        OpLdx(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a6cu), cpu->x);
        LoadA8(cpu, 4u);
        OpTestBits(memory, cpu, OpAbs(cpu, 0x10eeu), 1u);
    }
    SETUP_CALL(0x81b5u, 0x85eddbu, 3u);
    SETUP_CALL(0x81b9u, 0x81851eu, 2u);
    SETUP_CALL(0x81bcu, 0x8591a1u, 3u);
    SETUP_CALL(0x81c0u, 0x858a2fu, 3u);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    OpLda(memory, cpu, OpDp(cpu, 0x40u));
    OpSta(memory, cpu, OpDp(cpu, 0xd4u));
    SETUP_CALL(0x81ceu, 0x81c339u, 2u);
    for (;;) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1264u)));
        if (cpu->zero) break;
        SETUP_CALL(0x81d6u, 0x81d9d0u, 2u);
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, 0x0012f3u);
        SETUP_CALL(0x81dfu, 0x85ec81u, 3u);
    }
    return ExecutionReturned(0x8181e5u);                  /* RTS */
}
