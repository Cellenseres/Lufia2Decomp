#ifndef LUFIA2_EXECUTION_H
#define LUFIA2_EXECUTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 24-bit bus access supplied by the consumer. */
typedef uint8_t (*Lufia2BusReadByte)(void *context, uint32_t address);
typedef void (*Lufia2BusWriteByte)(
    void *context, uint32_t address, uint8_t value);

typedef struct Lufia2Memory {
    Lufia2BusReadByte read_byte;
    Lufia2BusWriteByte write_byte;
    void *context;
} Lufia2Memory;

/* 65816 state in native mode (E=0). */
typedef struct Lufia2CpuState {
    uint16_t accumulator;
    uint16_t x;
    uint16_t y;
    uint16_t stack;
    uint16_t direct_page;
    uint8_t data_bank;
    uint8_t program_bank;
    uint8_t carry;
    uint8_t negative;
    uint8_t zero;
    uint8_t accumulator_is_8_bit;
    uint8_t index_is_8_bit;
    uint8_t overflow;
    uint8_t decimal;
    uint8_t irq_disable;
    /* Exact ROM PC when a run stops early. */
    uint32_t resume_pc;
} Lufia2CpuState;

typedef enum Lufia2ExecutionFlow {
    /* Routine reached its return; pc names that instruction. */
    LUFIA2_EXECUTION_RETURNED = 0,
    /* State exact at resume_pc; consumer finishes in LLE. */
    LUFIA2_EXECUTION_BOUNDARY = 1,
    /* A child call did not return; propagate. */
    LUFIA2_EXECUTION_CHILD_UNWOUND = 2,
} Lufia2ExecutionFlow;

typedef struct Lufia2ExecutionResult {
    Lufia2ExecutionFlow flow;
    uint32_t pc;
    /* Script dispatches; verifiers count visits, runtime ignores. */
    uint32_t dispatches;
} Lufia2ExecutionResult;

#ifdef __cplusplus
}
#endif

#endif
