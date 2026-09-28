/* lufia2-resource list|check <rom>, extract <rom> <id> <out>,
 * decode <stream> <out>. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lufia2/resource_format.h"

enum {
    ROM_SIZE = 0x280000,
    COPIER_HEADER = 0x200,
    MAX_DECODED = 0x10000,
};

static const char kTitle[] = "Lufia II(Estpolis II)";

static unsigned char *ReadFile(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    unsigned char *data = 0;
    long length;

    if (!file) {
        perror(path);
        return 0;
    }
    if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) > 0 &&
        fseek(file, 0, SEEK_SET) == 0) {
        data = (unsigned char *)malloc((size_t)length);
        if (data && fread(data, 1, (size_t)length, file) != (size_t)length) {
            free(data);
            data = 0;
        }
        *size = (size_t)length;
    }
    fclose(file);
    if (!data)
        fprintf(stderr, "%s: cannot read\n", path);
    return data;
}

static int WriteFile(const char *path, const unsigned char *data, size_t size) {
    FILE *file = fopen(path, "wb");
    int ok = file && fwrite(data, 1, size, file) == size;

    if (file && fclose(file) != 0)
        ok = 0;
    if (!ok)
        fprintf(stderr, "%s: cannot write\n", path);
    return ok;
}

/* The headerless US ROM (a 512-byte copier header is skipped). */
static const unsigned char *CheckRom(const unsigned char *data, size_t size) {
    if (size == ROM_SIZE + COPIER_HEADER) {
        data += COPIER_HEADER;
        size -= COPIER_HEADER;
    }
    if (size != ROM_SIZE || memcmp(data + 0x7fc0, kTitle, sizeof kTitle - 1u) != 0 ||
        data[0x7fd9] != 0x01u) {
        fprintf(stderr, "not the supported Lufia II (USA) ROM\n");
        return 0;
    }
    return data;
}

/* LoROM file offset of a stream address, or ROM_SIZE when outside. */
static size_t FileOffset(uint32_t address) {
    const size_t offset =
        ((size_t)((address >> 16) & 0x7fu) << 15) | (address & 0x7fffu);

    return (address & 0x8000u) && offset < ROM_SIZE ? offset : ROM_SIZE;
}

typedef struct Resource {
    uint32_t address;
    size_t offset;
    size_t decoded_size;
    size_t stream_size;
    Lufia2ResourceStatus status;
} Resource;

static Resource Decode(const unsigned char *rom, unsigned id, uint8_t *output) {
    Resource resource;

    memset(&resource, 0, sizeof resource);
    resource.address =
        Lufia2ResourceStreamAddress(rom + FileOffset(LUFIA2_RESOURCE_TABLE_ADDRESS) +
                                    (size_t)id * LUFIA2_RESOURCE_ENTRY_SIZE);
    resource.offset = FileOffset(resource.address);
    if (resource.offset >= ROM_SIZE) {
        resource.status = LUFIA2_RESOURCE_TRUNCATED;
        return resource;
    }
    resource.status = Lufia2ResourceDecodedSize(
        rom + resource.offset, ROM_SIZE - resource.offset, &resource.decoded_size);
    if (resource.status == LUFIA2_RESOURCE_OK)
        resource.status =
            Lufia2ResourceDecode(rom + resource.offset, ROM_SIZE - resource.offset,
                                 output, MAX_DECODED, &resource.stream_size);
    return resource;
}

static int List(const unsigned char *rom, int print) {
    static uint8_t output[MAX_DECODED];
    unsigned id;
    unsigned good = 0;

    if (print)
        puts("id   address  offset   stream  decoded  status");
    for (id = 0; id < LUFIA2_RESOURCE_COUNT; ++id) {
        const Resource resource = Decode(rom, id, output);

        good += resource.status == LUFIA2_RESOURCE_OK;
        if (print || resource.status != LUFIA2_RESOURCE_OK)
            printf("%3u  %02X:%04X  %06zX  %6zu  %7zu  %s\n", id,
                   (unsigned)(resource.address >> 16),
                   (unsigned)(resource.address & 0xffffu), resource.offset,
                   resource.stream_size, resource.decoded_size,
                   Lufia2ResourceStatusName(resource.status));
    }
    printf("%u of %u resources decoded\n", good, (unsigned)LUFIA2_RESOURCE_COUNT);
    return good == LUFIA2_RESOURCE_COUNT ? 0 : 1;
}

static int Usage(void) {
    fputs("usage: lufia2-resource list <rom>\n"
          "       lufia2-resource check <rom>\n"
          "       lufia2-resource extract <rom> <id> <output>\n"
          "       lufia2-resource decode <stream-file> <output>\n",
          stderr);
    return 2;
}

int main(int argc, char **argv) {
    static uint8_t output[MAX_DECODED];
    unsigned char *file;
    const unsigned char *rom;
    size_t size = 0;
    int status = 1;

    if (argc < 3)
        return Usage();
    file = ReadFile(argv[2], &size);
    if (!file)
        return 1;
    if (strcmp(argv[1], "decode") == 0 && argc == 4) {
        size_t decoded = 0;
        const Lufia2ResourceStatus result =
            Lufia2ResourceDecode(file, size, output, sizeof output, 0);

        if (result == LUFIA2_RESOURCE_OK &&
            Lufia2ResourceDecodedSize(file, size, &decoded) == LUFIA2_RESOURCE_OK)
            status = !WriteFile(argv[3], output, decoded);
        else
            fprintf(stderr, "%s: %s\n", argv[2], Lufia2ResourceStatusName(result));
    } else if ((rom = CheckRom(file, size)) == 0) {
        status = 1;
    } else if (strcmp(argv[1], "list") == 0 && argc == 3) {
        status = List(rom, 1);
    } else if (strcmp(argv[1], "check") == 0 && argc == 3) {
        status = List(rom, 0);
    } else if (strcmp(argv[1], "extract") == 0 && argc == 5) {
        const unsigned long id = strtoul(argv[3], 0, 0);

        if (id >= LUFIA2_RESOURCE_COUNT) {
            fprintf(stderr, "resource id must be below %u\n",
                    (unsigned)LUFIA2_RESOURCE_COUNT);
        } else {
            const Resource resource = Decode(rom, (unsigned)id, output);

            if (resource.status == LUFIA2_RESOURCE_OK)
                status = !WriteFile(argv[4], output, resource.decoded_size);
            else
                fprintf(stderr, "resource %lu: %s\n", id,
                        Lufia2ResourceStatusName(resource.status));
        }
    } else {
        status = Usage();
    }
    free(file);
    return status;
}
