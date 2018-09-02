#include <nan.h>
#include "custom-type.h"
#include "node-encoder.h"
#include "node-binobject.h"

using namespace v8;

Nan::Persistent<Function> Encoder::constructor;

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

void WriteString(Encoder* encoder, Local<String> value) {
    int string_length = value->Length();
    uint8_t* buffer = (uint8_t*) malloc(string_length + 1);
    value->WriteOneByte(buffer);

    WriteNumber(encoder, string_length);
    encoder->PushBuffer(string_length, buffer);
}

void WriteNumber(Encoder* encoder, double number) {
    int result;

    if((number >= -0x80) && (number <= 0x7f))
        result = WriteInteger(encoder, 1, number, false);
    else if((number >= 0) && (number <= 0xff))
        result = WriteInteger(encoder, 1, number, true);
    else if(number >= -0x8000 && number <= 0x7fff)
        result = WriteInteger(encoder, 2, number, false);
    else if((number >= 0) && number <= 0xffff)
        result = WriteInteger(encoder, 2, number, true);
    else if(number >= -0x80000000 && number <= 0x7fffffff)
        result = WriteInteger(encoder, 4, number, false);
    else if((number >= 0) && number <= 0xffffffff)
        result = WriteInteger(encoder, 4, number, true);
    else
        result = BO::NumberErrors::InvalidSize;

    if(result != BO::NumberErrors::Ok) {
        if(result == BO::NumberErrors::InvalidSize)
            Nan::ThrowError("Number is too big to encode");
        else
            Nan::ThrowError("Could not encode integer");
    }
}

void WriteNumber(Encoder* encoder, Local<Number> value) {
    double number = value->Value();
    if(isnan(number)) {
        encoder->WriteUInt8(BO::Null);
        return;
    }
    WriteNumber(encoder, number);
}

void WriteArray(Encoder* encoder, Local<Array> array) {
    uint32_t length = array->Length();

    encoder->WriteUInt8(BO::PropertyType::Array);
    WriteNumber(encoder, length);

    for(uint32_t i = 0; i < length; i++)
        WriteValue(encoder, array->Get(i));
}

void WriteNativeMap(Encoder* encoder, Local<Map> map) {
    Local<Array> array = map->AsArray();
    size_t array_length = map->Size() * 2;

    WriteNumber(encoder, array_length / 2);

    for(size_t i = 0; i < array_length;) {
        Local<Value> prop = array->Get(Nan::New<Number>(i++));
        Local<Value> value = array->Get(Nan::New<Number>(i++));

        WriteValue(encoder, prop);
        WriteValue(encoder, value);
    }
}

/**
 * Check if this value can be written using a type defined by the user
 */
bool CheckCustomType(Encoder* encoder, Local<Value> value) {
    Local<Array> instructions = Local<Array>::Cast(encoder->GetHolder()->Get(Nan::New("instructions").ToLocalChecked()));
    uint32_t length = instructions->Length();

    for(uint32_t i = 0; i < length; i++) {
        Local<Number> index = Nan::New<Number>(i);
        Local<Object> instruction = instructions->Get(index)->ToObject();

        if(!instructions->Get(index)->IsObject()) {
            Nan::ThrowError("Instruction item should be plain objects");
            return true;
        }

        Local<Object> processor = instruction->Get(Nan::New("processor").ToLocalChecked())->ToObject();
        uint8_t validationResult = CustomType::Validate(processor, value);
        
        if(validationResult == 2)
            return true;
        else if(validationResult == 0)
            continue;

        size_t buffer_length;
        uint8_t* result;

        if(CustomType::Encode(processor, value, &result, &buffer_length) != 0)
            return true;

        uint8_t type = Local<Number>::Cast(instruction->Get(Nan::New("value").ToLocalChecked()))->Value();

        encoder->WriteUInt8(type);
        WriteNumber(encoder, buffer_length);
        encoder->PushBuffer(buffer_length, result);
        return true;
    }

    return false;
}

void WriteValue(Encoder* encoder, Local<Value> value) {
    Local<Context> context = Nan::GetCurrentContext();
    if(CheckCustomType(encoder, value))
        return;

    if(value->IsTypedArray()) {
        size_t byte_length = node::Buffer::Length(value);
        uint8_t* buffer = (uint8_t*)malloc(byte_length);

        memcpy(buffer, node::Buffer::Data(value), byte_length);

        encoder->WriteUInt8(BO::Buffer);
        WriteNumber(encoder, byte_length);
        encoder->PushBuffer(byte_length, buffer);
    } else if(value->IsBoolean()) {
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
        WriteNativeMap(encoder, Local<Map>::Cast(value));
    } else if(value->IsDate()) {
        Local<Date> date = Local<Date>::Cast(value);

        encoder->WriteUInt8(BO::Date);
        encoder->WriteDoubleLE(date->ValueOf());
    } else if(value->IsArray()) {
        Local<Array> list = Local<Array>::Cast(value);
        WriteArray(encoder, list);
    } else if(value->IsNumber()) {
        WriteNumber(encoder, value->ToNumber(context).ToLocalChecked());
    } else if(value->IsString()) {
        Local<String> string = value->ToString(context).ToLocalChecked();

        encoder->WriteUInt8(BO::String);
        WriteString(encoder, string);
    } else if(value->IsObject()) {
        WriteObject(encoder, value->ToObject(context).ToLocalChecked());
    } else {
        Nan::ThrowError("Invalid value type");
    }
}

void WriteObject(Encoder* encoder, Local<Object> object) {
    Local<Array> properties = object->GetOwnPropertyNames();
    Local<Context> context = Nan::GetCurrentContext();
    uint32_t length = properties->Length();
    
    encoder->WriteUInt8(BO::Object);
    WriteNumber(encoder, length);

    for(uint32_t i = 0; i < length; i++){
        Local<String> name = properties->Get(i)->ToString(context).ToLocalChecked();

        WriteString(encoder, name);
        WriteValue(encoder, object->Get(name));
    }
}

void Encoder::SetCurrentHolder(Local<Object> n) {
    holder = n;
}

Local<Object> Encoder::GetHolder() {
    return holder;
}

void Encoder::Encode(const Nan::FunctionCallbackInfo<Value>& args) {
    Local<Value> value = args[0];
    Encoder* encoder = Nan::ObjectWrap::Unwrap<Encoder>(args.Holder());

    encoder->SetCurrentHolder(args.Holder());

    WriteValue(encoder, value);

    encoder->Finish();

    size_t byte_length = encoder->Length();
    char* buffer = (char*) malloc(byte_length);

    encoder->CopyContents(buffer);

    Local<Object> result = Nan::NewBuffer(buffer, byte_length).ToLocalChecked();

    args.GetReturnValue().Set(result);
}

void Encoder::Init(Local<Object> exports) {
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("ObjectEncoder").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "encode", Encode);

    exports->Set(Nan::New("ObjectEncoder").ToLocalChecked(), tpl->GetFunction());
}

void Encoder::New(const Nan::FunctionCallbackInfo<Value>& args) {
    Local<Value> value = args[0];

    if(args.Length() > 0 && !value->IsArray())
        Nan::ThrowError("First argument must be an array or undefined");
    else if(value->IsArray())
        args.This()->Set(Nan::New<String>("instructions").ToLocalChecked(), args[0]);

    Encoder* encoder = new Encoder();
    encoder->Wrap(args.This());

    args.GetReturnValue().Set(args.This());
}
