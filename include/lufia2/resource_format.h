#ifndef LUFIA2_RESOURCE_FORMAT_H
#define LUFIA2_RESOURCE_FORMAT_H

/* Resource stream format of $80:8E9D on byte buffers. */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Resources in the table at $A7:8000. */
    LUFIA2_RESOURCE_COUNT = 680,
    /* Bytes per table entry: a 24-bit offset from $A7:8000. */
    LUFIA2_RESOURCE_ENTRY_SIZE = 3,
};

/* SNES address of the resource table read by $80:8E9D. */
#define LUFIA2_RESOURCE_TABLE_ADDRESS 0xa78000u

typedef enum Lufia2ResourceStatus {
    LUFIA2_RESOURCE_OK = 0,
    LUFIA2_RESOURCE_TRUNCATED,
    /* Reference before the output start. */
    LUFIA2_RESOURCE_BAD_REFERENCE,
    /* Output would pass the declared length. */
    LUFIA2_RESOURCE_OVERRUN,
    LUFIA2_RESOURCE_BUFFER_TOO_SMALL,
} Lufia2ResourceStatus;

/* SNES address of a stream, as $80:8E9D computes it. */
uint32_t Lufia2ResourceStreamAddress(const uint8_t entry[3]);

/* Declared length: the first two stream bytes. */
Lufia2ResourceStatus Lufia2ResourceDecodedSize(const uint8_t *stream,
                                               size_t stream_size,
                                               size_t *decoded_size);

/* Decode a whole stream; *consumed gets the bytes read. */
Lufia2ResourceStatus Lufia2ResourceDecode(const uint8_t *stream, size_t stream_size,
                                          uint8_t *output, size_t output_size,
                                          size_t *consumed);

const char *Lufia2ResourceStatusName(Lufia2ResourceStatus status);

#ifdef __cplusplus
}
#endif

#endif
