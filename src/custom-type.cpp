#include <nan.h>
#include "custom-type.h"

/**
 * 0 - false
 * 1 - true
 * 2 - invalid result
 */
uint8_t CustomType::Validate(Isolate* isolate, Local<Object> processor, Local<Value> value) {
    Local<Value> args[1] = { value };
    Local<Context> context = isolate->GetCallingContext();
    Local<Function> validate = Local<Function>::Cast(processor->Get(String::NewFromUtf8(isolate, "validate")));

    Local<Value> result = validate->Call(context, processor, 1, args).ToLocalChecked();

    if(!result->IsBoolean()){
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Result of validation function must be boolean")));
        return 2;
    }

    return result->ToBoolean(context).ToLocalChecked()->Value() == true ? 1 : 0;
}

Local<Value> CustomType::Decode(Isolate* isolate, size_t byte_length, uint8_t* input_buffer, Local<Object> processor) {
    const char* buffer = (char*) input_buffer;
    Local<Context> context = isolate->GetCallingContext();
    Local<Function> decodeFunction = Local<Function>::Cast(processor->Get(String::NewFromUtf8(isolate, "decode")));
    Local<Object> nodejs_buffer = node::Buffer::Copy(isolate, buffer, byte_length).ToLocalChecked();
    Local<Value> args[1] = { nodejs_buffer };

    return decodeFunction->Call(context, processor, 1, args).ToLocalChecked();
}

uint8_t CustomType::Encode(Isolate* isolate, Local<Object> processor, Local<Value> value, uint8_t** result, size_t* byte_length) {
    Local<Value> args[1] = { value };
    Local<Context> context = isolate->GetCallingContext();
    Local<Function> encoderFunction = Local<Function>::Cast(processor->Get(String::NewFromUtf8(isolate, "encode")));
    Local<Value> buffer = encoderFunction->Call(context, processor, 1, args).ToLocalChecked();

    if(!buffer->IsTypedArray()){
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Result from encoder method must be an typed array")));
        return 1;
    }

    size_t buffer_length = node::Buffer::Length(buffer);
    *result = (uint8_t*)malloc(buffer_length);

    memcpy(*result, node::Buffer::Data(buffer), buffer_length);
    return 0;
}