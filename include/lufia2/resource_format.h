#ifndef LUFIA2_RESOURCE_FORMAT_H
#define LUFIA2_RESOURCE_FORMAT_H

/* Resource stream format of $80:8E9D, independent of the CPU and the bus.
 *
 * $80:8E9D (Lufia2DecompressResource) is the verified CPU adapter: it
 * decodes a stream exactly as the original, bus access by bus access. The
 * functions here decode the same format from a byte buffer for host tools
 * and consumers. They allocate nothing and hold no cache; where the decoded
 * data lives, and whether it is kept, is the caller's decision.
 *
 * A stream is its decoded length (16-bit little endian), then groups of a
 * control byte and tokens. A token byte below $80 is a literal and uses no
 * control bit. A byte of $80 or more takes the next control bit, most
 * significant first: 0 makes it a literal, 1 starts a back reference with
 * the following byte (and a third byte when the low nibble of the second is
 * zero). A new control byte follows after eight control bits are used, unless
 * the output is complete. */

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
    /* The stream ends before the output is complete. */
    LUFIA2_RESOURCE_TRUNCATED,
    /* A back reference points before the start of the output. */
    LUFIA2_RESOURCE_BAD_REFERENCE,
    /* The output would pass the declared length (a back reference longer
     * than the remaining bytes, or an empty resource). The original only
     * stops when its 16-bit destination equals the end, so it would keep
     * writing until the destination wraps; a buffer cannot represent that. */
    LUFIA2_RESOURCE_OVERRUN,
    /* The output buffer is smaller than the declared length. */
    LUFIA2_RESOURCE_BUFFER_TOO_SMALL,
} Lufia2ResourceStatus;

/* SNES address of a stream from its table entry, computed like $80:8E9D:
 * the low word with bit 15 set, the bank $A7 plus the entry's bits 15-22
 * and the carry out of the shift. */
uint32_t Lufia2ResourceStreamAddress(const uint8_t entry[3]);

/* The declared decoded length, the stream's first two bytes. */
Lufia2ResourceStatus Lufia2ResourceDecodedSize(const uint8_t *stream,
                                               size_t stream_size,
                                               size_t *decoded_size);

/* Decode a whole stream into output. On success, *consumed (if not null) is
 * the number of stream bytes read and output holds exactly the declared
 * length. */
Lufia2ResourceStatus Lufia2ResourceDecode(const uint8_t *stream, size_t stream_size,
                                          uint8_t *output, size_t output_size,
                                          size_t *consumed);

const char *Lufia2ResourceStatusName(Lufia2ResourceStatus status);

#ifdef __cplusplus
}
#endif

#endif
