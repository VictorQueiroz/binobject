#include "decoder.h"
#include "ieee754.h"
#include <string.h>

int8_t read_int8(bo_decoder* decoder){
    return read_byte(decoder);
}

uint8_t read_uint8(bo_decoder* decoder){
    return read_byte(decoder);
}

const char* read_string(bo_decoder* decoder, size_t byte_length) {
    uint8_t* buffer = malloc(byte_length);

    read_bytes(decoder, byte_length, buffer);

    return (char*) buffer;
}

void read_bytes(bo_decoder* decoder, size_t byte_length, uint8_t* buffer) {
    memcpy(buffer, &decoder->buffer[decoder->offset], byte_length);
    decoder->offset += byte_length;
}

/**
 * Read one single byte from 
 * decoder and update offset
 */
uint8_t read_byte(bo_decoder* decoder) {
    return decoder->buffer[decoder->offset++];
}

int32_t read_int32_le(bo_decoder* decoder) {
    return read_uint32_le(decoder);
}

int bo_decoder_check_offset(bo_decoder* decoder, size_t byte_length) {
    if((decoder->offset + byte_length) > decoder->byte_length)
        fprintf(stderr, "Overlapping offset, max byte length is %d\n", (int) decoder->byte_length);
}

double read_double_le(bo_decoder* decoder) {
    bo_decoder_check_offset(decoder, 8);

    uint8_t* buffer = malloc(8);
    memcpy(buffer, &decoder->buffer[decoder->offset], 8);
    decoder->offset += 8;

    double r = read_ieee754(buffer, 0);
    free(buffer);

    return r;
}

int16_t read_int16_le(bo_decoder* decoder){
    return read_uint16_le(decoder);
}

uint16_t read_uint16_le(bo_decoder* decoder) {
    uint16_t result = 0;

    result += read_byte(decoder);
    result += read_byte(decoder) * pow(2, 8);

    return result;
}

uint32_t read_uint32_le(bo_decoder* decoder) {
    uint32_t result = 0;

    result += read_byte(decoder);
    result += read_byte(decoder) * pow(2, 8);
    result += read_byte(decoder) * pow(2, 16);
    result += read_byte(decoder) * pow(2, 24);

    return result;
}

void bo_decoder_init(bo_decoder** decoder_ptr, size_t byte_length, uint8_t* buffer){
    *decoder_ptr = malloc(sizeof(bo_decoder));

    bo_decoder* decoder = *decoder_ptr;
    decoder->buffer = buffer;
    decoder->offset = 0;
    decoder->byte_length = byte_length;
}

void destroy_decoder(bo_decoder* decoder) {
    free(decoder);
}