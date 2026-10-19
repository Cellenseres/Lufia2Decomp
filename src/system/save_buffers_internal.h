#ifndef LUFIA2_SYSTEM_SAVE_BUFFERS_INTERNAL_H
#define LUFIA2_SYSTEM_SAVE_BUFFERS_INTERNAL_H

#include "lufia2/save_buffers.h"

void Lufia2SaveCopyBuffer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         Lufia2SaveBuffer buffer,
                         Lufia2SaveBufferDirection direction);

void Lufia2SaveCopyRecordTail(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2SaveRecordTail tail,
                             Lufia2SaveBufferDirection direction);

#endif
