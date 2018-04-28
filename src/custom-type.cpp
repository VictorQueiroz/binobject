#include <nan.h>
#include "custom-type.h"

/**
 * 0 - false
 * 1 - true
 * 2 - invalid result
 */
uint8_t CustomType::Validate(Local<Object> processor, Local<Value> value) {
    Local<Value> args[1] = { value };
    Local<Context> context = Nan::GetCurrentContext();
    Local<Function> validate = Local<Function>::Cast(processor->Get(Nan::New("validate").ToLocalChecked()));

    Local<Value> result = validate->Call(context, processor, 1, args).ToLocalChecked();

    if(!result->IsBoolean()){
        Nan::ThrowError("Result of validation function must be boolean");
        return 2;
    }

    return result->ToBoolean(context).ToLocalChecked()->Value() == true ? 1 : 0;
}

Local<Value> CustomType::Decode(size_t byte_length, uint8_t* input_buffer, Local<Object> processor) {
    Local<Context> context = Nan::GetCurrentContext();
    Local<Function> decodeFunction = Local<Function>::Cast(processor->Get(Nan::New("decode").ToLocalChecked()));
    Local<Object> nodejs_buffer = Nan::NewBuffer((char*) input_buffer, byte_length).ToLocalChecked();
    Local<Value> args[1] = { nodejs_buffer };

    return decodeFunction->Call(context, processor, 1, args).ToLocalChecked();
}

uint8_t CustomType::Encode(Local<Object> processor, Local<Value> value, uint8_t** result, size_t* byte_length) {
    Local<Value> args[1] = { value };
    Local<Context> context = Nan::GetCurrentContext();
    Local<Function> encodeFunction = Local<Function>::Cast(processor->Get(Nan::New("encode").ToLocalChecked()));
    Local<Value> buffer = encodeFunction->Call(context, processor, 1, args).ToLocalChecked();

    if(!buffer->IsTypedArray()){
        Nan::ThrowError("Result from encoder method must be an typed array");
        return 1;
    }

    *byte_length = node::Buffer::Length(buffer);

    size_t buffer_length = *byte_length;
    *result = (uint8_t*)malloc(buffer_length);

    memcpy(*result, node::Buffer::Data(buffer), buffer_length);
    return 0;
}