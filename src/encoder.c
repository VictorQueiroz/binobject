#include <string.h>
#include "encoder.h"
#include "ieee754.h"

void push_buffer(bo_encoder* encoder, size_t byte_length, uint8_t* buffer) {
    bo_stream_record* next;
    bo_stream_record* previous = encoder->current;

    bo_stream_record_init(&next);

    next->data = buffer;
    next->byte_length = byte_length;
    next->previous = previous;

    previous->next = next;
    encoder->current = next;
    encoder->total_byte_length += byte_length;
}

void write_byte(bo_encoder* encoder, uint8_t value) {
    uint8_t* buffer = malloc(1);

    if(buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    buffer[0] = value;

    push_buffer(encoder, 1, buffer);
}

void write_string(bo_encoder* encoder, size_t string_length, const char* string) {
    uint8_t* buffer = malloc(string_length);

    if(buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    memcpy(buffer, string, string_length);
    push_buffer(encoder, string_length, buffer);
}

void write_double_le(bo_encoder* encoder, double n) {
    uint8_t* buffer = malloc(8);

    write_ieee754(n, buffer, 0);
    push_buffer(encoder, 8, buffer);
}

void write_uint8(bo_encoder* encoder, uint8_t n) {
    write_byte(encoder, n);
}

void write_int8(bo_encoder* encoder, int8_t n){
    write_byte(encoder, n);
}

void write_bytes(bo_encoder* encoder, size_t byte_length, uint8_t* buffer) {
    push_buffer(encoder, byte_length, buffer);
}

void write_int16_le(bo_encoder* encoder, int16_t value) {
    uint8_t* buffer = malloc(2);

    buffer[0] = value;
    buffer[1] = value >> 8;

    push_buffer(encoder, 2, buffer);
}

void write_uint16_le(bo_encoder* encoder, uint16_t value) {
    uint8_t* buffer = malloc(2);

    buffer[0] = value;
    buffer[1] = value >> 8;

    push_buffer(encoder, 2, buffer);
}

void write_int32_le(bo_encoder* encoder, int32_t value) {
    uint8_t* buffer = malloc(4);

    if(buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    size_t offset = 0;

    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    push_buffer(encoder, 4, buffer);
}

void write_uint32_le(bo_encoder* encoder, uint32_t value) {
    uint8_t* buffer = malloc(4);

    if(buffer == NULL) {
        free(buffer);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    size_t offset = 0;

    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    value = value >> 8;
    buffer[offset++] = value;

    push_buffer(encoder, 4, buffer);
}

void bo_encoder_finish(bo_encoder* encoder) {
    size_t offset = 0;
    uint8_t* buffer = malloc(encoder->total_byte_length);

    if(buffer == NULL){
        free(buffer);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    bo_stream_record* next = encoder->stream;

    while(next) {
        memcpy(&buffer[offset], next->data, next->byte_length);
        offset += next->byte_length;
        next = next->next;
    }

    encoder->final_buffer = buffer;
}

void destroy_encoder(bo_encoder* encoder) {
    bo_stream_record* previous = encoder->current;

    while(previous) {
        if(previous->data != NULL)
            free(previous->data);

        if(previous->previous == NULL)
            break;

        previous = previous->previous;
        free(previous->next);
    }

    free(encoder->stream);
    free(encoder->final_buffer);
    free(encoder);
}

void bo_encoder_init(bo_encoder** encoder_ptr) {
    *encoder_ptr = malloc(sizeof(bo_encoder));

    if(*encoder_ptr == NULL) {
        free(*encoder_ptr);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    bo_stream_record* initial_stream;
    bo_stream_record_init(&initial_stream);


    bo_encoder* encoder = *encoder_ptr;
    encoder->current = initial_stream;
    encoder->stream = initial_stream;
    encoder->total_byte_length = 0;
    encoder->final_buffer = NULL;
}

void bo_stream_record_init(bo_stream_record** record_ptr) {
    *record_ptr = malloc(sizeof(bo_stream_record));

    if(*record_ptr == NULL) {
        free(*record_ptr);
        fprintf(stderr, "Not enough memory");
        exit(1);
    }

    bo_stream_record* new_record = *record_ptr;
    new_record->next = NULL;
    new_record->data = NULL;
    new_record->previous = NULL;
    new_record->byte_length = 0;
}