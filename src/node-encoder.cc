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

int WriteInteger(bo_encoder* encoder, size_t byte_length, double number, bool _unsigned) {
    uint8_t integer_type;

    if(byte_length == 1)
        integer_type = _unsigned ? BO::UInt8 : BO::Int8;
    else if(byte_length == 2)
        integer_type = _unsigned ? BO::UInt16 : BO::Int16;
    else if(byte_length == 4)
        integer_type = _unsigned ? BO::UInt32 : BO::Int32;
    else
        return BO::NumberErrors::InvalidByteLength;

    write_uint8(encoder, integer_type);

    switch(integer_type) {
        case BO::UInt8:
            write_uint8(encoder, number);
            break;
        case BO::Int8:
            write_int8(encoder, number);
            break;
        case BO::UInt16:
            write_uint16_le(encoder, number);
            break;
        case BO::Int16:
            write_int16_le(encoder, number);
            break;
        case BO::UInt32:
            write_uint32_le(encoder, number);
            break;
        case BO::Int32:
            write_int32_le(encoder, number);
            break;
    }

    return BO::NumberErrors::Ok;
}

void WriteString(Isolate* isolate, bo_encoder* encoder, Local<String> value) {
    int string_length = value->Utf8Length();
    uint8_t* buffer = (uint8_t*) malloc(string_length + 1);
    value->WriteOneByte(buffer);

    WriteNumber(isolate, encoder, string_length);
    push_buffer(encoder, string_length, buffer);
}

int WriteNumber(bo_encoder* encoder, double number) {
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

void WriteNumber(Isolate* isolate, bo_encoder* encoder, double number) {
    int result = WriteNumber(encoder, number);

    if(result != BO::NumberErrors::Ok) {
        if(result == BO::NumberErrors::InvalidSize)
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Number is too big to encode")));
        else
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Could not encode integer")));
    }
}

void WriteNumber(Isolate* isolate, bo_encoder* encoder, Local<Number> value) {
    WriteNumber(isolate, encoder, value->Value());
}

void WriteArray(Isolate* isolate, bo_encoder* encoder, Local<Array> array) {
    uint32_t length = array->Length();

    write_uint8(encoder, BO::PropertyType::Array);
    WriteNumber(isolate, encoder, length);

    for(uint32_t i = 0; i < length; i++)
        WriteValue(isolate, encoder, array->Get(i));
}

void WriteNativeMap(Isolate* isolate, bo_encoder* encoder, Local<Map> map) {
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

void WriteValue(Isolate* isolate, bo_encoder* encoder, Local<Value> value) {
    if(value->IsMap()) {
        write_uint8(encoder, BO::Map);
        WriteNativeMap(isolate, encoder, Local<Map>::Cast(value));
    } else if(value->IsDate()) {
        Local<Date> date = Local<Date>::Cast(value);

        write_uint8(encoder, BO::Date);
        write_double_le(encoder, date->ValueOf());
    } else if(value->IsArray()) {
        Local<Array> list = Local<Array>::Cast(value);
        WriteArray(isolate, encoder, list);
    } else if(value->IsNumber()) {
        WriteNumber(isolate, encoder, value->ToNumber(isolate));
    } else if(value->IsString()) {
        Local<String> string = value->ToString(isolate);

        write_uint8(encoder, BO::String);
        WriteString(isolate, encoder, string);
    } else if(value->IsObject()) {
        WriteObject(isolate, encoder, value->ToObject(isolate));
    } else {
        Local<String> error_message = String::NewFromUtf8(isolate, "Invalid value type");
        isolate->ThrowException(Exception::Error(error_message));
    }
}

void WriteObject(Isolate* isolate, bo_encoder* encoder, Local<Object> object) {
    Local<Array> properties = object->GetOwnPropertyNames();
    uint32_t length = properties->Length();
    
    write_uint8(encoder, BO::Object);
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
    Encoder* wrapped_encoder = ObjectWrap::Unwrap<Encoder>(args.Holder());

    WriteValue(isolate, wrapped_encoder->encoder, value);

    bo_encoder_finish(wrapped_encoder->encoder);

    size_t byte_length = wrapped_encoder->encoder->total_byte_length;
    char* buffer = (char*) malloc(byte_length);

    memcpy(buffer, wrapped_encoder->encoder->final_buffer, byte_length);

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
    Encoder* encoder = new Encoder();
    encoder->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void Encoder::CreateObject(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Context> context = isolate->GetCallingContext();
    Local<Object> instance = cons->NewInstance(context).ToLocalChecked();
    args.GetReturnValue().Set(instance);
}
