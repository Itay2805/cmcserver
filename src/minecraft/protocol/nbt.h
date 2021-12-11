#pragma once

#include <stdint.h>
#include <stddef.h>
#include <lib/stb_ds.h>
#include "protocol.h"

typedef enum nbt_type {
    NBT_TAG_END = 0,
    NBT_TAG_BYTE = 1,
    NBT_TAG_SHORT = 2,
    NBT_TAG_INT = 3,
    NBT_TAG_LONG = 4,
    NBT_TAG_FLOAT = 5,
    NBT_TAG_DOUBLE = 6,
    NBT_TAG_BYTE_ARRAY = 7,
    NBT_TAG_STRING = 8,
    NBT_TAG_LIST = 9,
    NBT_TAG_COMPOUND = 10,
    NBT_TAG_INT_ARRAY = 11,
    NBT_TAG_LONG_ARRAY = 12,
} nbt_type_t;

typedef struct nbt {
    nbt_type_t type;
    char* key;
    union {
        int8_t byte;
        int16_t short_value;
        int32_t int_value;
        int64_t long_value;
        float float_value;
        double double_value;
        int8_t* byte_array;
        char* string;
        struct nbt* list;
        struct nbt* compound;
        int32_t* int_array;
        int64_t* long_array;
    };
} nbt_t;

void nbt_free(nbt_t* nbt);

int protocol_read_nbt(uint8_t* buffer, int size, nbt_t* nbt);
int protocol_write_nbt(uint8_t* buffer, int size, nbt_t nbt);
