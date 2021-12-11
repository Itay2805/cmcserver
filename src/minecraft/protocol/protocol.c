#include "protocol.h"

#include <endian.h>

uint8_t protocol_read_u8(uint8_t* buffer) {
    return *buffer;
}

void protocol_write_u8(uint8_t* buffer, uint8_t value) {
    *buffer = value;
}

uint16_t protocol_read_u16(uint8_t* buffer) {
    return be16toh(*((uint16_t*)buffer));
}

void protocol_write_u16(uint8_t* buffer, uint16_t value) {
    *((uint16_t*)buffer) = htobe16(value);
}

uint32_t protocol_read_u32(uint8_t* buffer) {
    return be32toh(*((uint32_t*)buffer));
}

void protocol_write_u32(uint8_t* buffer, uint32_t value) {
    *((uint32_t*)buffer) = htobe32(value);
}

uint64_t protocol_read_u64(uint8_t* buffer) {
    return be64toh(*((uint64_t*)buffer));
}

void protocol_write_u64(uint8_t* buffer, uint64_t value) {
    *((uint64_t*)buffer) = htobe64(value);
}

int8_t protocol_read_i8(uint8_t* buffer) {
    return (int8_t)*buffer;
}

void protocol_write_i8(uint8_t* buffer, int8_t value) {
    *buffer = (int8_t)value;
}

int16_t protocol_read_i16(uint8_t* buffer) {
    return be16toh(*((int16_t*)buffer));
}

void protocol_write_i16(uint8_t* buffer, int16_t value) {
    *((int16_t*)buffer) = htobe16(value);
}

int32_t protocol_read_i32(uint8_t* buffer) {
    return be32toh(*((int32_t*)buffer));
}

void protocol_write_i32(uint8_t* buffer, int32_t value) {
    *((int32_t*)buffer) = htobe32(value);
}

int64_t protocol_read_i64(uint8_t* buffer) {
    return be64toh(*((int64_t*)buffer));
}

void protocol_write_i64(uint8_t* buffer, int64_t value) {
    *((int64_t*)buffer) = htobe64(value);
}

float protocol_read_f32(uint8_t* buffer) {
    return *((float*)buffer);
}

void protocol_write_f32(uint8_t* buffer, float value) {
    *((float*)buffer) = value;
}

double protocol_read_f64(uint8_t* buffer) {
    return *((double*)buffer);
}

void protocol_write_f64(uint8_t* buffer, double value) {
    *((double*)buffer) = value;
}

bool protocol_read_bool(uint8_t* buffer) {
    return *buffer == 0 ? false : true;
}

void protocol_write_bool(uint8_t* buffer, bool value) {
    *buffer = value ? 0x01 : 0x00;
}

int protocol_read_varint(uint8_t* buffer, int size, int32_t* value) {
    int original_size = size;
    int shift = 0;
    while (true) {
        if (size <= 0) return -1;
        uint8_t b = *buffer;
        buffer++;
        size--;
        *value |= ((b & 0x7F) << shift);
        if (!(b & 0x80)) {
            break;
        }
        shift += 7;
        if (shift >= 32) return -1;
    }
    return original_size - size;
}

int protocol_write_varint(uint8_t* buffer, int size, int32_t value) {
    int original_size = size;
    while (value & ~0x7f) {
        if (size <= 0) return -1;
        *buffer = (value & 0xFF) | 0x80;
        buffer++;
        size--;
        value >>= 7;
    }
    return original_size - size;
}

int protocol_read_varlong(uint8_t* buffer, int size, int64_t* value) {
    int original_size = size;
    int shift = 0;
    while (true) {
        if (size <= 0) return -1;
        uint8_t b = *buffer;
        buffer++;
        size--;
        *value |= ((b & 0x7F) << shift);
        if (!(b & 0x80)) {
            break;
        }
        shift += 7;
        if (shift >= 64) return -1;
    }
    return original_size - size;
}

int protocol_write_varlong(uint8_t* buffer, int size, int64_t value) {
    int original_size = size;
    while (value & ~0x7f) {
        if (size <= 0) return -1;
        *buffer = (value & 0xFF) | 0x80;
        buffer++;
        size--;
        value >>= 7;
    }
    return original_size - size;
}

uuid_t protocol_read_uuid(uint8_t* buffer) {
    return *((uuid_t*)buffer);
}

void protocol_write_uuid(uint8_t* buffer, uuid_t uuid) {
    *((uuid_t*)buffer) = uuid;
}

