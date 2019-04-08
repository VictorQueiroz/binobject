#include <nan.h>
#include "decoder.h"
#include "custom-type.h"
#include "node-decoder.h"
#include "node-binobject.h"

using namespace v8;

Nan::Persistent<Function> Decoder::constructor;

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

Local<Value> ReadNumberAsValue(Decoder* decoder, uint8_t type) {
    switch(type) {
        case BO::UInt8:
            return Nan::New<Number>(decoder->ReadUInt8());
        case BO::Int8:
            return Nan::New<Number>(decoder->ReadInt8());
        case BO::UInt16:
            return Nan::New<Number>(decoder->ReadUInt16LE());
        case BO::Int16:
            return Nan::New<Number>(decoder->ReadInt16LE());
        case BO::UInt32:
            return Nan::New<Number>(decoder->ReadUInt32LE());
        case BO::Int32:
            return Nan::New<Number>(decoder->ReadInt32LE());
    }

    Nan::ThrowError("Got invalid integer type");
}

double ReadNumber(Decoder* decoder) {
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
            Nan::ThrowError("Got invalid integer type");
    }

    return n;
}

void ReadObject(Decoder* decoder, Local<Object> result) {
    size_t properties_length = ReadNumber(decoder);

    for(size_t i = 0; i < properties_length; i++) {
        size_t string_length = ReadNumber(decoder);
        uint8_t* buffer = (uint8_t*)malloc(string_length);

        decoder->ReadBytes(string_length, buffer);

        Local<String> name = Nan::NewOneByteString(buffer, string_length).ToLocalChecked();
        Local<Value> value = ReadValue(decoder);

        result->Set(name, value);
    }
}

Local<Value> ReadArray(Decoder* decoder) {
    int array_length = ReadNumber(decoder);
    Local<Array> list = Nan::New<Array>(array_length);

    for(int i = 0; i < array_length; i++)
        list->Set(Nan::New<Number>(i), ReadValue(decoder));

    return list;
}

Local<Value> ReadMapNative(Decoder* decoder) {
    Local<Context> context = Nan::GetCurrentContext();
    Local<Map> map = Map::New(context->GetIsolate());
    size_t map_length = ReadNumber(decoder);

    for(size_t i = 0; i < map_length; i++) {
        Local<Value> prop = ReadValue(decoder);
        map = map->Set(context, prop, ReadValue(decoder)).ToLocalChecked();
    }

    return map;
}

bool CheckCustomType(Decoder* decoder, uint8_t type, Local<Object>& processor) {
    Local<Object> holder = decoder->GetCurrentHolder();
    Local<Array> instructions = Local<Array>::Cast(holder->Get(Nan::New("instructions").ToLocalChecked()));

    uint32_t length = instructions->Length();

    for(uint32_t i = 0; i < length; i++) {
        Local<Object> instruction = instructions->Get(i)->ToObject();
        Local<Uint32> value = Local<Uint32>::Cast(instruction->Get(Nan::New("value").ToLocalChecked()));

        if(value->Value() != type)
            continue;

        processor = instruction->Get(Nan::New("processor").ToLocalChecked())->ToObject();
        return true;
    }

    return false;
}

Local<Value> ReadValue(Decoder* decoder) {
    uint8_t type = decoder->ReadUInt8();

    if(type == BO::Buffer) {
        size_t byte_length = ReadNumber(decoder);
        uint8_t* buffer = (uint8_t*) malloc(byte_length);

        decoder->ReadBytes(byte_length, buffer);

        Local<Object> nodejs_buffer = Nan::NewBuffer((char*) buffer, byte_length).ToLocalChecked();

        return nodejs_buffer;
    } else if(type == BO::Boolean) {
        bool value = decoder->ReadUInt8() == 0 ? false : true;
        Local<Boolean> boolean = Nan::New(value);

        return boolean;
    } else if(type == BO::Undefined) {
        return Nan::Undefined();
    } else if(type == BO::Null) {
        return Nan::Null();
    } else if(type == BO::Object){
        Local<Object> result = Nan::New<Object>();
        ReadObject(decoder, result);

        return result;
    } else if(type == BO::String){
        int string_length = ReadNumber(decoder);
        uint8_t* buffer = (uint8_t*) malloc(string_length);

        decoder->ReadBytes(string_length, buffer);

        Local<String> string = Nan::NewOneByteString(buffer, string_length).ToLocalChecked();

        free(buffer);
        return string;
    } else if(type == BO::Date) {
        double date = decoder->ReadDoubleLE();
        Local<Context> context = Nan::GetCurrentContext();

        return Nan::New<Date>(date).ToLocalChecked();
    } else if(type == BO::Array) {
        return ReadArray(decoder);
    } else if(type == BO::Map) {
        return ReadMapNative(decoder);
    } else {
        Local<Object> processor;

        if(CheckCustomType(decoder, type, processor)) {
            size_t byte_length = ReadNumber(decoder);
            uint8_t* buffer = (uint8_t*) malloc(byte_length);

            decoder->ReadBytes(byte_length, buffer);

            return CustomType::Decode(byte_length, buffer, processor);
        }
    }

    return ReadNumberAsValue(decoder, type);
}

void Decoder::SetCurrentHolder(Local<Object> holder) {
    current_holder = holder;
}

Local<Object> Decoder::GetCurrentHolder() {
    return current_holder;
}

NAN_METHOD(Decoder::Decode) {
    Decoder* decoder = ObjectWrap::Unwrap<Decoder>(info.Holder());

    decoder->SetCurrentHolder(info.Holder());

    Local<Value> result = ReadValue(decoder);

    info.GetReturnValue().Set(result);
}

NAN_METHOD(Decoder::New) {
    Local<Object> instance = info.This();
    Local<Value> value = info[0];

    if(value->IsUndefined()){
        Nan::ThrowError("Expected buffer but got undefined instead");
        return;
    }

    size_t byte_length = node::Buffer::Length(value);
    uint8_t* buffer = (uint8_t*) node::Buffer::Data(value);

    instance->Set(Nan::New("instructions").ToLocalChecked(), info[1]);

    Decoder* decoder = new Decoder(byte_length, buffer);
    decoder->Wrap(instance);

    info.GetReturnValue().Set(instance);
}

void Decoder::Init(Local<Object> exports) {
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("ObjectDecoder").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "decode", Decode);

    exports->Set(Nan::New("ObjectDecoder").ToLocalChecked(), tpl->GetFunction());
}