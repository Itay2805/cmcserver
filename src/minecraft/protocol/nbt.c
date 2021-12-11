#include "nbt.h"

void nbt_free(nbt_t* nbt) {
    // free the key
    arrfree(nbt->key);

    // free nested structures
    switch (nbt->type) {
        case NBT_TAG_BYTE_ARRAY: arrfree(nbt->byte_array); break;
        case NBT_TAG_INT_ARRAY: arrfree(nbt->int_array); break;
        case NBT_TAG_LONG_ARRAY: arrfree(nbt->long_array); break;
        case NBT_TAG_STRING: arrfree(nbt->string); break;

        case NBT_TAG_LIST: {
            for (int i = 0; i < arrlen(nbt->list); i++) {
                nbt_free(&nbt->list[i]);
            }
            arrfree(nbt->list);
        } break;

        case NBT_TAG_COMPOUND: {
            for (int i = 0; i < shlen(nbt->compound); i++) {
                nbt_free(&nbt->compound[i]);
            }
            shfree(nbt->compound);
        } break;

        default: break;
    }

    // clear the data structure just in case
    memset(nbt, 0, sizeof(*nbt));
}

int protocol_read_nbt(uint8_t* buffer, int size, nbt_t* nbt) {
    return -1;
}

int protocol_write_nbt(uint8_t* buffer, int size, nbt_t nbt) {
    return -1;
}
