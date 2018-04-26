#include <nan.h>
#include "decoder.h"
#include "custom-type.h"
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

uint8_t Decoder::ReadUInt8() {
    return read_uint8(decoder);
}

int8_t Decoder::ReadInt8() {
    return read_int8(decoder);
}

uint16_t Decoder::ReadUInt16LE() {
    return read_uint16_le(decoder);
}

int16_t Decoder::ReadInt16LE() {
    return read_int16_le(decoder);
}

uint32_t Decoder::ReadUInt32LE() {
    return read_uint32_le(decoder);
}

int32_t Decoder::ReadInt32LE() {
    return read_int32_le(decoder);
}

double Decoder::ReadDoubleLE() {
    return read_double_le(decoder);
}

void Decoder::ReadBytes(size_t length, uint8_t* buffer) {
    read_bytes(decoder, length, buffer);
}

Local<Value> ReadNumberAsValue(Isolate* isolate, Decoder* decoder, uint8_t type) {
    switch(type) {
        case BO::UInt8:
            return Number::New(isolate, decoder->ReadUInt8());
        case BO::Int8:
            return Number::New(isolate, decoder->ReadInt8());
        case BO::UInt16:
            return Number::New(isolate, decoder->ReadUInt16LE());
        case BO::Int16:
            return Number::New(isolate, decoder->ReadInt16LE());
        case BO::UInt32:
            return Number::New(isolate, decoder->ReadUInt32LE());
        case BO::Int32:
            return Number::New(isolate, decoder->ReadInt32LE());
    }

    return isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Got invalid integer type")));
}

double ReadNumber(Isolate* isolate, Decoder* decoder) {
    uint8_t type = decoder->ReadUInt8();
    double n = 0;

    switch(type) {
        case BO::UInt8:
            n += decoder->ReadUInt8();
            break;
        case BO::Int8:
            n += decoder->ReadInt8();
            break;
        case BO::UInt16:
            n += decoder->ReadUInt16LE();
            break;
        case BO::Int16:
            n += decoder->ReadInt16LE();
            break;
        case BO::UInt32:
            n += decoder->ReadUInt32LE();
            break;
        case BO::Int32:
            n += decoder->ReadInt32LE();
            break;
        default:
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Got invalid integer type")));
    }

    return n;
}

void ReadObject(Isolate* isolate, Decoder* decoder, Local<Object> result) {
    size_t properties_length = ReadNumber(isolate, decoder);

    for(size_t i = 0; i < properties_length; i++) {
        size_t string_length = ReadNumber(isolate, decoder);
        uint8_t* buffer = (uint8_t*)malloc(string_length);

        decoder->ReadBytes(string_length, buffer);

        Local<String> name = String::NewFromOneByte(isolate, buffer, NewStringType::kNormal, string_length).ToLocalChecked();
        Local<Value> value = ReadValue(isolate, decoder);

        result->Set(name, value);
    }
}

Local<Value> ReadArray(Isolate* isolate, Decoder* decoder) {
    int array_length = ReadNumber(isolate, decoder);
    Local<Array> list = Array::New(isolate, array_length);

    for(int i = 0; i < array_length; i++)
        list->Set(Number::New(isolate, i), ReadValue(isolate, decoder));

    return list;
}

Local<Value> ReadMapNative(Isolate* isolate, Decoder* decoder) {
    Local<Map> map = Map::New(isolate);
    Local<Context> context = isolate->GetCallingContext();
    size_t map_length = ReadNumber(isolate, decoder);

    for(size_t i = 0; i < map_length; i++) {
        Local<Value> prop = ReadValue(isolate, decoder);
        map = map->Set(context, prop, ReadValue(isolate, decoder)).ToLocalChecked();
    }

    return map;
}

bool CheckCustomType(Isolate* isolate, Decoder* decoder, uint8_t type, Local<Object>& processor) {
    Local<Object> holder = decoder->GetCurrentHolder();
    Local<Array> instructions = Local<Array>::Cast(holder->Get(String::NewFromUtf8(isolate, "instructions")));

    uint32_t length = instructions->Length();

    for(uint32_t i = 0; i < length; i++) {
        Local<Object> instruction = instructions->Get(i)->ToObject();
        Local<Uint32> value = Local<Uint32>::Cast(instruction->Get(String::NewFromUtf8(isolate, "value")));

        if(value->Value() != type)
            continue;

        processor = instruction->Get(String::NewFromUtf8(isolate, "processor"))->ToObject();
        return true;
    }

    return false;
}

Local<Value> ReadValue(Isolate* isolate, Decoder* decoder) {
    uint8_t type = decoder->ReadUInt8();

    if(type == BO::Buffer) {
        size_t byte_length = ReadNumber(isolate, decoder);
        uint8_t* buffer = (uint8_t*) malloc(byte_length);

        decoder->ReadBytes(byte_length, buffer);

        Local<Object> nodejs_buffer = node::Buffer::New(isolate, (char*) buffer, byte_length).ToLocalChecked();

        return nodejs_buffer;
    } else if(type == BO::Boolean) {
        bool value = decoder->ReadUInt8() == 0 ? false : true;
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

        decoder->ReadBytes(string_length, buffer);

        Local<String> string = String::NewFromOneByte(isolate, buffer, NewStringType::kNormal, string_length).ToLocalChecked();

        free(buffer);
        return string;
    } else if(type == BO::Date) {
        double date = decoder->ReadDoubleLE();

        return Date::New(isolate->GetCallingContext(), date).ToLocalChecked();
    } else if(type == BO::Array) {
        return ReadArray(isolate, decoder);
    } else if(type == BO::Map) {
        return ReadMapNative(isolate, decoder);
    } else {
        Local<Object> processor;

        if(CheckCustomType(isolate, decoder, type, processor)) {
            size_t byte_length = ReadNumber(isolate, decoder);
            uint8_t* buffer = (uint8_t*) malloc(byte_length);

            decoder->ReadBytes(byte_length, buffer);

            return CustomType::Decode(isolate, byte_length, buffer, processor);
        }
    }

    return ReadNumberAsValue(isolate, decoder, type);
}

void Decoder::SetCurrentHolder(Local<Object> holder) {
    current_holder = holder;
}

Local<Object> Decoder::GetCurrentHolder() {
    return current_holder;
}

void Decoder::Decode(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Decoder* decoder = ObjectWrap::Unwrap<Decoder>(args.Holder());

    decoder->SetCurrentHolder(args.Holder());

    Local<Value> result = ReadValue(isolate, decoder);

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

    instance->Set(String::NewFromUtf8(isolate, "instructions"), args[1]);

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