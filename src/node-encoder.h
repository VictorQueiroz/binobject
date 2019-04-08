#ifndef NODE_ENCODER_H_
#define NODE_ENCODER_H_

#include <nan.h>
#include "encoder.h"

using namespace v8;

class Encoder : public Nan::ObjectWrap {
private:
    bo_encoder* encoder;
    Encoder();
    ~Encoder();
    void Finish();
    size_t Length();
    static Nan::Persistent<Function> constructor;
    static void New(const Nan::FunctionCallbackInfo<Value>& args);
    static void Encode(const Nan::FunctionCallbackInfo<Value>& args);
    Local<Object> holder;

public:
    static void Init(Local<Object> exports);
    static void NewInstance(const Nan::FunctionCallbackInfo<Value>& args);
    static void CreateObject(const Nan::FunctionCallbackInfo<Value>& args);

    void SetCurrentHolder(Local<Object> holder);
    Local<Object> GetHolder();

    void CopyContents(void* buffer);
    void WriteInt8(int8_t n);
    void WriteDoubleLE(double n);
    void WriteUInt8(uint8_t n);
    void WriteUInt16LE(uint16_t n);
    void WriteUInt32LE(uint32_t n);
    void WriteInt16LE(int16_t n);
    void WriteInt32LE(int32_t n);
    void PushBuffer(size_t string_length, uint8_t* buffer);
};

void WriteCompressedNumber(Encoder*, double);
void WriteNumber(Encoder* encoder, Local<Number> value);

void WriteValue(Encoder* encoder, Local<Value> value);
void WriteObject(Encoder* encoder, Local<Object> object);

bool CheckCustomType(Encoder* encoder, Local<Value> value);

#endif