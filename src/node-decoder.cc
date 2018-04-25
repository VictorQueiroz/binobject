#include <nan.h>
#include "decoder.h"
#include "node-decoder.h"
#include "node-binobject.h"

using namespace v8;

Persistent<Function> Decoder::constructor;

Decoder::Decoder(size_t byte_length, uint8_t* buffer) {
    bo_decoder_init(&decoder, byte_length, buffer);
}

Decoder::~Decoder() {
    destroy_decoder(decoder);
}

Local<Value> ReadNumberAsValue(Isolate* isolate, bo_decoder* decoder, uint8_t type) {
    switch(type) {
        case BO::UInt8:
            return Number::New(isolate, read_uint8(decoder));
        case BO::Int8:
            return Number::New(isolate, read_int8(decoder));
        case BO::UInt16:
            return Number::New(isolate, read_uint16_le(decoder));
        case BO::Int16:
            return Number::New(isolate, read_int16_le(decoder));
        case BO::UInt32:
            return Number::New(isolate, read_uint32_le(decoder));
        case BO::Int32:
            return Number::New(isolate, read_int32_le(decoder));
    }

    return isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Got invalid integer type")));
}

double ReadNumber(Isolate* isolate, bo_decoder* decoder) {
    uint8_t type = read_uint8(decoder);
    double n = 0;

    switch(type) {
        case BO::UInt8:
            n += read_uint8(decoder);
            break;
        case BO::Int8:
            n += read_int8(decoder);
            break;
        case BO::UInt16:
            n += read_uint16_le(decoder);
            break;
        case BO::Int16:
            n += read_int16_le(decoder);
            break;
        case BO::UInt32:
            n += read_uint32_le(decoder);
            break;
        case BO::Int32:
            n += read_int32_le(decoder);
            break;
        default:
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Got invalid integer type")));
    }

    return n;
}

void ReadObject(Isolate* isolate, bo_decoder* decoder, Local<Object> result) {
    size_t properties_length = ReadNumber(isolate, decoder);

    for(size_t i = 0; i < properties_length; i++) {
        size_t string_length = ReadNumber(isolate, decoder);
        uint8_t* buffer = (uint8_t*)malloc(string_length);

        read_bytes(decoder, string_length, buffer);

        Local<String> name = String::NewFromOneByte(isolate, buffer, NewStringType::kNormal, string_length).ToLocalChecked();
        Local<Value> value = ReadValue(isolate, decoder);

        result->Set(name, value);
    }
}

Local<Value> ReadArray(Isolate* isolate, bo_decoder* decoder) {
    int array_length = ReadNumber(isolate, decoder);
    Local<Array> list = Array::New(isolate, array_length);

    for(int i = 0; i < array_length; i++)
        list->Set(Number::New(isolate, i), ReadValue(isolate, decoder));

    return list;
}

Local<Value> ReadMapNative(Isolate* isolate, bo_decoder* decoder) {
    Local<Map> map = Map::New(isolate);
    Local<Context> context = isolate->GetCallingContext();
    size_t map_length = ReadNumber(isolate, decoder);

    for(size_t i = 0; i < map_length; i++) {
        Local<Value> prop = ReadValue(isolate, decoder);
        map = map->Set(context, prop, ReadValue(isolate, decoder)).ToLocalChecked();
    }

    return map;
}

Local<Value> ReadValue(Isolate* isolate, bo_decoder* decoder) {
    uint8_t type = read_uint8(decoder);

    if(type == BO::Buffer) {
        size_t byte_length = ReadNumber(isolate, decoder);
        uint8_t* buffer = (uint8_t*) malloc(byte_length);

        read_bytes(decoder, byte_length, buffer);

        Local<Object> nodejs_buffer = node::Buffer::New(isolate, (char*) buffer, byte_length).ToLocalChecked();

        return nodejs_buffer;
    } else if(type == BO::Boolean) {
        bool value = read_uint8(decoder) == 0 ? false : true;
        Local<Boolean> boolean = Boolean::New(isolate, value);

        return boolean;
    } else if(type == BO::Undefined) {
        return Undefined(isolate);
    } else if(type == BO::Null) {
        return Null(isolate);
    } else if(type == BO::Object){
        Local<Object> result = Object::New(isolate);
        ReadObject(isolate, decoder, result);

        return result;
    } else if(type == BO::String){
        int string_length = ReadNumber(isolate, decoder);
        uint8_t* buffer = (uint8_t*) malloc(string_length);

        read_bytes(decoder, string_length, buffer);

        Local<String> string = String::NewFromOneByte(isolate, buffer, NewStringType::kNormal, string_length).ToLocalChecked();

        free(buffer);
        return string;
    } else if(type == BO::Date) {
        double date = read_double_le(decoder);

        return Date::New(isolate->GetCallingContext(), date).ToLocalChecked();
    } else if(type == BO::Array) {
        return ReadArray(isolate, decoder);
    } else if(type == BO::Map) {
        return ReadMapNative(isolate, decoder);
    }

    return ReadNumberAsValue(isolate, decoder, type);
}

void Decoder::Decode(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Decoder* wrapped_decoder = ObjectWrap::Unwrap<Decoder>(args.Holder());
    Local<Value> result = ReadValue(isolate, wrapped_decoder->decoder);

    args.GetReturnValue().Set(result);
}

void Decoder::CreateObject(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Context> context = isolate->GetCallingContext();
    Local<Object> instance = cons->NewInstance(context).ToLocalChecked();

    Local<Value> value = args[0];

    if(value->IsUndefined()){
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Expected buffer but got undefined instead")));
        return;
    }

    size_t byte_length = node::Buffer::Length(value);
    uint8_t* buffer = (uint8_t*) node::Buffer::Data(value);
    Decoder* decoder = new Decoder(byte_length, buffer);
    decoder->Wrap(instance);

    args.GetReturnValue().Set(instance);
}

void Decoder::Init(Isolate* isolate) {
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Decoder"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "decode", Decode);

    constructor.Reset(isolate, tpl->GetFunction());
}

void Decoder::New(const FunctionCallbackInfo<Value>& args) {
    args.GetReturnValue().Set(args.This());
}