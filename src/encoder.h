#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef ENCODER_H_
#define ENCODER_H_

struct _bo_stream_record_ {
    struct _bo_stream_record_* next;
    struct _bo_stream_record_* previous;
    uint8_t* data;
    /**
     * data length in bytes
     */
    size_t byte_length;
};

typedef struct _bo_stream_record_ bo_stream_record;

typedef struct {
    uint8_t* final_buffer;
    bo_stream_record* stream;
    bo_stream_record* current;
    size_t total_byte_length;
} bo_encoder;

void push_buffer(bo_encoder* encoder, size_t byte_length, uint8_t* buffer);
void bo_encoder_init(bo_encoder** encoder_ptr);
void destroy_encoder(bo_encoder* encoder);
void write_int32_le(bo_encoder* encoder, int32_t i);
void write_int16_le(bo_encoder* encoder, int16_t i);
void write_uint16_le(bo_encoder* encoder, uint16_t i);
void write_double_le(bo_encoder* encoder, double n);
void write_uint32_le(bo_encoder* encoder, uint32_t i);
void write_bytes(bo_encoder* encoder, size_t byte_length, uint8_t* buffer);
void write_byte(bo_encoder* encoder, uint8_t i);
void write_uint8(bo_encoder* encoder, uint8_t n);
void write_int8(bo_encoder* encoder, int8_t n);
void write_string(bo_encoder* encoder, size_t string_length, const char* string);
void bo_stream_record_init(bo_stream_record** record);
void bo_encoder_finish(bo_encoder* encoder);

#endif

#ifdef __cplusplus
}
#endif