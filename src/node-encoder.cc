#include <nan.h>
#include "node-encoder.h"
#include "node-binobject.h"

using namespace v8;

Persistent<Function> Encoder::constructor;

Encoder::Encoder() {
    bo_encoder_init(&encoder);
}

Encoder::~Encoder() {
    destroy_encoder(encoder);
}

int WriteInteger(Encoder* encoder, size_t byte_length, double number, bool _unsigned) {
    uint8_t integer_type;

    if(byte_length == 1)
        integer_type = _unsigned ? BO::UInt8 : BO::Int8;
    else if(byte_length == 2)
        integer_type = _unsigned ? BO::UInt16 : BO::Int16;
    else if(byte_length == 4)
        integer_type = _unsigned ? BO::UInt32 : BO::Int32;
    else
        return BO::NumberErrors::InvalidByteLength;

    encoder->WriteUInt8(integer_type);

    switch(integer_type) {
        case BO::UInt8:
            encoder->WriteUInt8(number);
            break;
        case BO::Int8:
            encoder->WriteInt8(number);
            break;
        case BO::UInt16:
            encoder->WriteUInt16LE(number);
            break;
        case BO::Int16:
            encoder->WriteInt16LE(number);
            break;
        case BO::UInt32:
            encoder->WriteUInt32LE(number);
            break;
        case BO::Int32:
            encoder->WriteInt32LE(number);
            break;
    }

    return BO::NumberErrors::Ok;
}

void Encoder::WriteUInt8(uint8_t n) {
    write_uint8(encoder, n);
}

void Encoder::WriteDoubleLE(double n){
    write_double_le(encoder, n);
}

void Encoder::WriteInt8(int8_t n) {
    write_int8(encoder, n);
}

void Encoder::WriteInt16LE(int16_t n) {
    write_int16_le(encoder, n);
}

void Encoder::WriteUInt16LE(uint16_t n) {
    write_uint16_le(encoder, n);
}

void Encoder::WriteUInt32LE(uint32_t n) {
    write_uint32_le(encoder, n);
}

void Encoder::WriteInt32LE(int32_t n) {
    write_int32_le(encoder, n);
}

void Encoder::PushBuffer(size_t string_length, uint8_t* buffer){
    push_buffer(encoder, string_length, buffer);
}

void Encoder::Finish() {
    bo_encoder_finish(encoder);
}

void Encoder::CopyContents(void* target) {
    memcpy(target, encoder->final_buffer, Length());
}

size_t Encoder::Length() {
    return encoder->total_byte_length;
}

void WriteString(Isolate* isolate, Encoder* encoder, Local<String> value) {
    int string_length = value->Utf8Length();
    uint8_t* buffer = (uint8_t*) malloc(string_length + 1);
    value->WriteOneByte(buffer);

    WriteNumber(isolate, encoder, string_length);
    encoder->PushBuffer(string_length, buffer);
}

int WriteNumber(Encoder* encoder, double number) {
    if((number >= -0x80) && (number <= 0x7f))
        return WriteInteger(encoder, 1, number, false);
    else if((number >= 0) && (number <= 0xff))
        return WriteInteger(encoder, 1, number, true);
    else if(number >= -0x8000 && number <= 0x7fff)
        return WriteInteger(encoder, 2, number, false);
    else if((number >= 0) && number <= 0xffff)
        return WriteInteger(encoder, 2, number, true);
    else if(number >= -0x80000000 && number <= 0x7fffffff)
        return WriteInteger(encoder, 4, number, false);
    else if((number >= 0) && number <= 0xffffffff)
        return WriteInteger(encoder, 4, number, true);

    return BO::NumberErrors::InvalidSize;
}

void WriteNumber(Isolate* isolate, Encoder* encoder, double number) {
    int result = WriteNumber(encoder, number);

    if(result != BO::NumberErrors::Ok) {
        if(result == BO::NumberErrors::InvalidSize)
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Number is too big to encode")));
        else
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Could not encode integer")));
    }
}

void WriteNumber(Isolate* isolate, Encoder* encoder, Local<Number> value) {
    WriteNumber(isolate, encoder, value->Value());
}

void WriteArray(Isolate* isolate, Encoder* encoder, Local<Array> array) {
    uint32_t length = array->Length();

    encoder->WriteUInt8(BO::PropertyType::Array);
    WriteNumber(isolate, encoder, length);

    for(uint32_t i = 0; i < length; i++)
        WriteValue(isolate, encoder, array->Get(i));
}

void WriteNativeMap(Isolate* isolate, Encoder* encoder, Local<Map> map) {
    Local<Array> array = map->AsArray();
    size_t array_length = map->Size() * 2;

    WriteNumber(isolate, encoder, array_length / 2);

    for(size_t i = 0; i < array_length;) {
        Local<Value> prop = array->Get(Number::New(isolate, i++));
        Local<Value> value = array->Get(Number::New(isolate, i++));

        WriteValue(isolate, encoder, prop);
        WriteValue(isolate, encoder, value);
    }
}

void WriteValue(Isolate* isolate, Encoder* encoder, Local<Value> value) {
    if(value->IsTypedArray()) {
        size_t byte_length = node::Buffer::Length(value);
        uint8_t* buffer = (uint8_t*)malloc(byte_length);

        memcpy(buffer, node::Buffer::Data(value), byte_length);

        encoder->WriteUInt8(BO::Buffer);
        WriteNumber(isolate, encoder, byte_length);
        encoder->PushBuffer(byte_length, buffer);
    } if(value->IsBoolean()) {
        Local<Boolean> boolean = Local<Boolean>::Cast(value);

        encoder->WriteUInt8(BO::Boolean);

        if(boolean->Value())
            encoder->WriteUInt8(1);
        else
            encoder->WriteUInt8(0);
    } else if(value->IsUndefined()) {
        encoder->WriteUInt8(BO::Undefined);
    } else if(value->IsNull()) {
        encoder->WriteUInt8(BO::Null);
    } else if(value->IsMap()) {
        encoder->WriteUInt8(BO::Map);
        WriteNativeMap(isolate, encoder, Local<Map>::Cast(value));
    } else if(value->IsDate()) {
        Local<Date> date = Local<Date>::Cast(value);

        encoder->WriteUInt8(BO::Date);
        encoder->WriteDoubleLE(date->ValueOf());
    } else if(value->IsArray()) {
        Local<Array> list = Local<Array>::Cast(value);
        WriteArray(isolate, encoder, list);
    } else if(value->IsNumber()) {
        WriteNumber(isolate, encoder, value->ToNumber(isolate));
    } else if(value->IsString()) {
        Local<String> string = value->ToString(isolate);

        encoder->WriteUInt8(BO::String);
        WriteString(isolate, encoder, string);
    } else if(value->IsObject()) {
        WriteObject(isolate, encoder, value->ToObject(isolate));
    } else {
        Local<String> error_message = String::NewFromUtf8(isolate, "Invalid value type");
        isolate->ThrowException(Exception::Error(error_message));
    }
}

void WriteObject(Isolate* isolate, Encoder* encoder, Local<Object> object) {
    Local<Array> properties = object->GetOwnPropertyNames();
    uint32_t length = properties->Length();
    
    encoder->WriteUInt8(BO::Object);
    WriteNumber(encoder, length);

    for(uint32_t i = 0; i < length; i++){
        Local<String> name = properties->Get(i)->ToString();

        WriteString(isolate, encoder, name);
        WriteValue(isolate, encoder, object->Get(name));
    }
}

void Encoder::SetCurrentHolder(Local<Object> n) {
    holder = n;
}

Local<Object> Encoder::GetHolder() {
    return holder;
}

void Encoder::Encode(const FunctionCallbackInfo<Value>& args) {
    Local<Value> value = args[0];
    Isolate* isolate = args.GetIsolate();
    Encoder* encoder = ObjectWrap::Unwrap<Encoder>(args.Holder());

    encoder->SetCurrentHolder(args.Holder());

    WriteValue(isolate, encoder, value);

    encoder->Finish();

    size_t byte_length = encoder->Length();
    char* buffer = (char*) malloc(byte_length);

    encoder->CopyContents(buffer);

    Local<Object> result = node::Buffer::Copy(isolate, buffer, byte_length).ToLocalChecked();
    args.GetReturnValue().Set(result);

    free(buffer);
}

void Encoder::Init(Isolate* isolate) {
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Encoder"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "encode", Encode);

    constructor.Reset(isolate, tpl->GetFunction());
}

void Encoder::New(const FunctionCallbackInfo<Value>& args){
    args.GetReturnValue().Set(args.This());
}

void Encoder::CreateObject(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Context> context = isolate->GetCallingContext();
    Local<Object> instance = cons->NewInstance(context).ToLocalChecked();

    Encoder* encoder = new Encoder();
    encoder->Wrap(instance);

    args.GetReturnValue().Set(instance);
}
