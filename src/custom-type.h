#include <stdint.h>
#include <v8.h>

#ifndef CUSTOM_TYPE_H_
#define CUSTOM_TYPE_H_

using namespace v8;

namespace CustomType {
    uint8_t Validate(Isolate* isolate, Local<Object> processor, Local<Value> value);
    uint8_t Encode(Isolate* isolate, Local<Object> processor, Local<Value> value, uint8_t** result, size_t* byte_length);
    Local<Value> Decode(Isolate* isolate, size_t byte_length, uint8_t* input_buffer, Local<Object> processor);
}

#endif