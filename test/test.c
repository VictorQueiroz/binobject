#include "encoder.h"
#include "decoder.h"
#include <assert.h>

int8_t random_int8() {
    int8_t min = -0x80,
            max = 0x7f;

    return min + rand() % ((max + 1) - min);
}

uint8_t random_uint8() {
    uint8_t min = 0,
            max = 0xff;

    return min + rand() % ((max + 1) - min);
}

void should_write_uint32_le() {
    bo_encoder* encoder;
    bo_encoder_init(&encoder);
    write_uint32_le(encoder, 0xffffff);

    bo_encoder_finish(encoder);

    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);
    assert(read_uint32_le(decoder) == 0xffffff);

    destroy_decoder(decoder);
    destroy_encoder(encoder);
}

void should_write_int8() {
    bo_encoder* encoder;
    bo_encoder_init(&encoder);

    const size_t integers_list_size = 0xffff;
    int8_t* integers = malloc(integers_list_size);

    for(int i = 0; i < integers_list_size; i++) {
        int8_t random_int = random_int8();
        write_int8(encoder, random_int);
        integers[i] = random_int;
    }

    bo_encoder_finish(encoder);
    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);
    
    for(int i = 0; i < integers_list_size; i++)
        assert(integers[i] == read_int8(decoder));

    free(integers);
    destroy_decoder(decoder);
    destroy_encoder(encoder);
}

void should_write_bytes() {
    size_t buffer_size = 0xffff;
    uint8_t *buffer = malloc(buffer_size);

    for(int i = 0; i < buffer_size; i++)
        buffer[i] = random_uint8();

    bo_encoder* encoder;
    bo_encoder_init(&encoder);

    write_bytes(encoder, buffer_size, buffer);

    bo_encoder_finish(encoder);

    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);

    for(int i = 0; i < buffer_size; i++) {
        uint8_t output_byte = read_byte(decoder);
        assert(output_byte == buffer[i]);
    }

    assert(decoder->offset == buffer_size);
    destroy_encoder(encoder);
    destroy_decoder(decoder);
}

void should_write_string() {
    bo_encoder* encoder;
    bo_encoder_init(&encoder);

    const char* string = "binary object string text of a few characters";
    uint32_t string_length = strlen(string) + 1;

    // write string length
    write_uint32_le(encoder, string_length);
    write_string(encoder, string_length, string);

    bo_encoder_finish(encoder);

    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);
    assert(read_uint32_le(decoder) == string_length);

    char decoded_string[string_length];

    for(int i = 0; i < string_length; i++)
        decoded_string[i] = read_byte(decoder);

    assert(strcmp(decoded_string, "binary object string text of a few characters") == 0);

    destroy_decoder(decoder);
    destroy_encoder(encoder);
}

void should_write_structures() {
    bo_encoder* encoder;
    bo_encoder_init(&encoder);

    write_uint8(encoder, 255); // max uint8
    write_uint8(encoder, 0); // min uint8
    write_int8(encoder, 127); // max int8
    write_int8(encoder, -128); // min int8

    write_uint32_le(encoder, 0); // min uint32
    write_uint32_le(encoder, 4294967295); // max uint32

    write_int32_le(encoder, -2147483648); // min int32
    write_int32_le(encoder, 2147483647); // max int32

    bo_encoder_finish(encoder);

    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);

    assert(read_uint8(decoder) == 255);
    assert(read_uint8(decoder) == 0);

    assert(read_int8(decoder) == 127);
    assert(read_int8(decoder) == -128);

    assert(read_uint32_le(decoder) == 0);
    assert(read_uint32_le(decoder) == 4294967295);

    assert(read_int32_le(decoder) == -2147483648);
    assert(read_int32_le(decoder) == 2147483647);

    destroy_encoder(encoder);
    destroy_decoder(decoder);
}

void should_write_double() {
    bo_encoder* encoder;
    bo_encoder_init(&encoder);

    write_double_le(encoder, 1524578775412);

    bo_encoder_finish(encoder);

    bo_decoder* decoder;
    bo_decoder_init(&decoder, encoder->total_byte_length, encoder->final_buffer);

    assert(read_double_le(decoder) == 1524578775412);

    destroy_decoder(decoder);
    destroy_encoder(encoder);
}

int main() {
    should_write_uint32_le();
    should_write_bytes();
    should_write_string();
    should_write_int8();
    should_write_double();
    should_write_structures();
}