#include "custom-type.h"
#include "node-decoder.h"
#include "node-binobject.h"

#include <nan.h>

using namespace v8;

Nan::Persistent<Function> Decoder::constructor;

Decoder::Decoder(size_t byte_length, uint8_t* buffer) {
    mff_deserializer_init(&decoder, buffer, byte_length);
}

Decoder::~Decoder() {
    mff_deserializer_destroy(decoder);
}

uint8_t Decoder::ReadUInt8() {
    uint8_t n = 0;
    mff_deserializer_read_uint8(decoder, &n);
    return n;
}

int8_t Decoder::ReadInt8() {
    int8_t n = 0;
    mff_deserializer_read_int8(decoder, &n);
    return n;
}

uint16_t Decoder::ReadUInt16LE() {
    uint16_t n = 0;
    mff_deserializer_read_uint16(decoder, &n);
    return n;
}

int16_t Decoder::ReadInt16LE() {
    int16_t n = 0;
    mff_deserializer_read_int16(decoder, &n);
    return n;
}

uint32_t Decoder::ReadUInt32LE() {
    uint32_t n = 0;
    mff_deserializer_read_uint32(decoder, &n);
    return n;
}

int32_t Decoder::ReadInt32LE() {
    int32_t n = 0;
    mff_deserializer_read_int32(decoder, &n);
    return n;
}

double Decoder::ReadDoubleLE() {
    double n = 0;
    mff_deserializer_read_double(decoder, &n);
    return n;
}

float Decoder::ReadFloatLE() {
    float n = 0;
    mff_deserializer_read_float(decoder, &n);
    return n;
}

void Decoder::ReadBytes(size_t length, uint8_t* buffer) {
    mff_deserializer_read_buffer(decoder, buffer, length);
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
        case BO::Float:
            return Nan::New<Number>(decoder->ReadFloatLE());
        case BO::Double:
            return Nan::New<Number>(decoder->ReadDoubleLE());
    }

    Nan::ThrowError(
        std::string("Got invalid integer type: " + std::to_string(type)).c_str()
    );
    return Nan::Undefined();
}

double ReadNumber(Decoder* decoder) {
    uint8_t type = decoder->ReadUInt8();
    double n = 0;

    switch(type) {
        case BO::UInt8:
            n = decoder->ReadUInt8();
            break;
        case BO::Int8:
            n = decoder->ReadInt8();
            break;
        case BO::UInt16:
            n = decoder->ReadUInt16LE();
            break;
        case BO::Int16:
            n = decoder->ReadInt16LE();
            break;
        case BO::UInt32:
            n = decoder->ReadUInt32LE();
            break;
        case BO::Int32:
            n = decoder->ReadInt32LE();
            break;
        case BO::Float:
            n = decoder->ReadFloatLE();
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
        uint8_t buffer[string_length];

        decoder->ReadBytes(string_length, buffer);

        Local<String> name = Nan::NewOneByteString(buffer, string_length).ToLocalChecked();
        Local<Value> value = ReadValue(decoder);

        Nan::Set(result, name, value);
    }
}

Local<Value> ReadArray(Decoder* decoder) {
    double array_length = ReadNumber(decoder);
    Local<Array> list = Nan::New<Array>(array_length);

    for(int i = 0; i < array_length; i++)
        Nan::Set(list, Nan::New<Number>(i), ReadValue(decoder));

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
    Local<Array> instructions = Local<Array>::Cast(Nan::Get(holder, Nan::New("instructions").ToLocalChecked()).ToLocalChecked());

    uint32_t length = instructions->Length();

    for(uint32_t i = 0; i < length; i++) {
        Local<Object> instruction = Nan::To<Object>(Nan::Get(instructions, i).ToLocalChecked()).ToLocalChecked();
        Local<Uint32> value = Local<Uint32>::Cast(Nan::Get(instruction, Nan::New("value").ToLocalChecked()).ToLocalChecked());

        if(value->Value() != type)
            continue;

        processor = Nan::To<Object>(Nan::Get(instruction, Nan::New("processor").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
        return true;
    }

    return false;
}

Local<Object> ReadBuffer(Decoder* decoder) {
    size_t byte_length = ReadNumber(decoder);
    // Allocation ownership is taken by `Nan::NewBuffer`
    uint8_t* buffer = (uint8_t*) malloc(byte_length);

    decoder->ReadBytes(byte_length, buffer);

    Local<Object> nodejs_buffer = Nan::NewBuffer((char*) buffer, byte_length).ToLocalChecked();

    return nodejs_buffer;
}

Local<Value> ReadValue(Decoder* decoder) {
    uint8_t type = decoder->ReadUInt8();

    if(type == BO::Buffer) {
        return ReadBuffer(decoder);
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
        uint32_t string_length = ReadNumber(decoder);
        uint8_t buffer[string_length];

        decoder->ReadBytes(string_length, buffer);

        Local<String> string = Nan::NewOneByteString(buffer, string_length).ToLocalChecked();
        return string;
    } else if(type == BO::Date) {
        double date = decoder->ReadDoubleLE();

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

    Local<Value> v = ReadNumberAsValue(decoder, type);
    // printf("Number type is %d. Number value is %.6f\n", type, v->NumberValue());
    return v;
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

    Nan::Set(instance, Nan::New("instructions").ToLocalChecked(), info[1]);

    Decoder* decoder = new Decoder(byte_length, buffer);
    decoder->Wrap(instance);

    info.GetReturnValue().Set(instance);
}

void Decoder::Init(Local<Object> exports) {
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("ObjectDecoder").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "decode", Decode);

    Nan::Set(exports, Nan::New("ObjectDecoder").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}