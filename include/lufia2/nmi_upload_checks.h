#ifndef LUFIA2_NMI_UPLOAD_CHECKS_H
#define LUFIA2_NMI_UPLOAD_CHECKS_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_UPLOAD_CHECKS 1
#define LUFIA2_NMI_UPLOAD_CHECK_OPCODES 3u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiCheckUploadRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
