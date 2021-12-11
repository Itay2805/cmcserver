#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct uuid {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_hi_and_reserved;
    uint8_t clock_seq_low;
    uint8_t node[6];
} uuid_t;

typedef enum protocol_phase {
    PROTOCOL_HANDSHAKING = 0,
    PROTOCOL_STATUS = 1,
    PROTOCOL_LOGIN = 2,
    PROTOCOL_PLAY = 3
} protocol_state_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t protocol_read_u8(uint8_t* buffer);
void protocol_write_u8(uint8_t* buffer, uint8_t value);
uint16_t protocol_read_u16(uint8_t* buffer);
void protocol_write_u16(uint8_t* buffer, uint16_t value);
uint32_t protocol_read_u32(uint8_t* buffer);
void protocol_write_u32(uint8_t* buffer, uint32_t value);
uint64_t protocol_read_u64(uint8_t* buffer);
void protocol_write_u64(uint8_t* buffer, uint64_t value);
int8_t protocol_read_i8(uint8_t* buffer);
void protocol_write_i8(uint8_t* buffer, int8_t value);
int16_t protocol_read_i16(uint8_t* buffer);
void protocol_write_i16(uint8_t* buffer, int16_t value);
int32_t protocol_read_i32(uint8_t* buffer);
void protocol_write_i32(uint8_t* buffer, int32_t value);
int64_t protocol_read_i64(uint8_t* buffer);
void protocol_write_i64(uint8_t* buffer, int64_t value);
float protocol_read_f32(uint8_t* buffer);
void protocol_write_f32(uint8_t* buffer, float value);
double protocol_read_f64(uint8_t* buffer);
void protocol_write_f64(uint8_t* buffer, double value);
bool protocol_read_bool(uint8_t* buffer);
void protocol_write_bool(uint8_t* buffer, bool value);
int protocol_read_varint(uint8_t* buffer, int size, int32_t* value);
int protocol_write_varint(uint8_t* buffer, int size, int32_t value);
int protocol_read_varlong(uint8_t* buffer, int size, int64_t* value);
int protocol_write_varlong(uint8_t* buffer, int size, int64_t value);
uuid_t protocol_read_uuid(uint8_t* buffer);
void protocol_write_uuid(uint8_t* buffer, uuid_t uuid);
