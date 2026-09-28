/* Resource stream format of $80:8E9D on byte buffers. */

#include "lufia2/resource_format.h"

uint32_t Lufia2ResourceStreamAddress(const uint8_t entry[3]) {
    /* $80:8EB2-$80:8EC6. */
    const uint16_t low = (uint16_t)(entry[0] | ((unsigned)entry[1] << 8));
    const uint16_t high = (uint16_t)(entry[1] | ((unsigned)entry[2] << 8));
    const unsigned carry = high >> 15;
    const uint8_t bank =
        (uint8_t)(0xa7u + (uint8_t)(((unsigned)high << 1) >> 8) + carry);

    return ((uint32_t)bank << 16) | (uint16_t)(low | 0x8000u);
}

Lufia2ResourceStatus Lufia2ResourceDecodedSize(const uint8_t *stream,
                                               size_t stream_size,
                                               size_t *decoded_size) {
    if (stream_size < 2u)
        return LUFIA2_RESOURCE_TRUNCATED;
    *decoded_size = (size_t)stream[0] | ((size_t)stream[1] << 8);
    return LUFIA2_RESOURCE_OK;
}

Lufia2ResourceStatus Lufia2ResourceDecode(const uint8_t *stream, size_t stream_size,
                                          uint8_t *output, size_t output_size,
                                          size_t *consumed) {
    size_t length;
    size_t position = 2;
    size_t written = 0;
    Lufia2ResourceStatus status;

    status = Lufia2ResourceDecodedSize(stream, stream_size, &length);
    if (status != LUFIA2_RESOURCE_OK)
        return status;
    if (output_size < length)
        return LUFIA2_RESOURCE_BUFFER_TOO_SMALL;
    /* The original never stops on an empty resource. */
    if (length == 0)
        return LUFIA2_RESOURCE_OVERRUN;
    for (;;) { /* $80:8EEF */
        uint8_t flags;
        unsigned flags_left;

        if (position >= stream_size)
            return LUFIA2_RESOURCE_TRUNCATED;
        flags = stream[position++];
        for (flags_left = 8; flags_left != 0; --flags_left) {
            uint8_t token;
            int reference;

            if (position >= stream_size)
                return LUFIA2_RESOURCE_TRUNCATED;
            token = stream[position++]; /* $80:8EFA */
            if (token < 0x80u) {
                /* A plain literal uses no control bit. */
                output[written++] = token;
                if (written == length)
                    goto done;
                ++flags_left;
                continue;
            }
            reference = (flags & 0x80u) != 0;
            flags = (uint8_t)(flags << 1);
            if (!reference) {
                output[written++] = token;
            } else {
                /* Low nibble zero selects the three-byte form. */
                uint16_t offset;
                size_t count;
                size_t distance;
                uint8_t second;

                if (position >= stream_size)
                    return LUFIA2_RESOURCE_TRUNCATED;
                second = stream[position++];
                offset = (uint16_t)(0xf000u | ((((unsigned)token << 8) | second) >> 4));
                if (second & 0x0fu) {
                    count = (size_t)(second & 0x0fu) + 2u;
                } else {
                    uint8_t third;

                    if (position >= stream_size)
                        return LUFIA2_RESOURCE_TRUNCATED;
                    third = stream[position++];
                    count = (size_t)(third & 0x3fu) + 3u;
                    offset = (uint16_t)((offset << 2) | (third >> 6));
                }
                /* The 16-bit offset is negative: $10000 - offset back. */
                distance = 0x10000u - offset;
                if (distance > written)
                    return LUFIA2_RESOURCE_BAD_REFERENCE;
                if (count > length - written)
                    return LUFIA2_RESOURCE_OVERRUN;
                /* $80:8F5C MVN copies ascending, so overlaps repeat. */
                while (count--) {
                    output[written] = output[written - distance];
                    ++written;
                }
            }
            if (written == length) /* $80:8F14 */
                goto done;
        }
    }
done:
    if (consumed)
        *consumed = position;
    return LUFIA2_RESOURCE_OK;
}

const char *Lufia2ResourceStatusName(Lufia2ResourceStatus status) {
    switch (status) {
    case LUFIA2_RESOURCE_OK:
        return "ok";
    case LUFIA2_RESOURCE_TRUNCATED:
        return "truncated stream";
    case LUFIA2_RESOURCE_BAD_REFERENCE:
        return "back reference before the output";
    case LUFIA2_RESOURCE_OVERRUN:
        return "back reference past the declared length";
    case LUFIA2_RESOURCE_BUFFER_TOO_SMALL:
        return "output buffer too small";
    }
    return "unknown status";
}
