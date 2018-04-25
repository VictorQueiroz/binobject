#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef DECODER_H_
#define DECODER_H_

typedef struct {
    uint8_t* buffer;
    size_t offset;
    size_t byte_length;
} bo_decoder;

double read_double_le(bo_decoder* decoder);
uint8_t read_uint8(bo_decoder* decoder);
int16_t read_int16_le(bo_decoder* decoder);
uint16_t read_uint16_le(bo_decoder* decoder);
int32_t read_int32_le(bo_decoder* decoder);
uint32_t read_uint32_le(bo_decoder* decoder);
uint8_t read_byte(bo_decoder* decoder);
int8_t read_int8(bo_decoder* decoder);
int32_t read_int32_le(bo_decoder* decoder);
const char* read_string(bo_decoder* decoder, size_t byte_length);
void read_bytes(bo_decoder* decoder, size_t byte_length, uint8_t* buffer);
void destroy_decoder(bo_decoder* decoder);
void bo_decoder_init(bo_decoder** decoder_ptr, size_t byte_length, uint8_t* buffer);

#endif

#ifdef __cplusplus
}
#endif