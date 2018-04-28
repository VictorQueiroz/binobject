#include <nan.h>
#include "decoder.h"

#ifndef NODE_DECODER_H_
#define NODE_DECODER_H_

using namespace v8;

class Decoder : public Nan::ObjectWrap {
private:
    bo_decoder* decoder;
    Local<Object> current_holder;
    Decoder(size_t byte_length, uint8_t* buffer);
    ~Decoder();
    static Nan::Persistent<Function> constructor;
    static void Decode(const Nan::FunctionCallbackInfo<Value>& args);
    static void New(const Nan::FunctionCallbackInfo<Value>& args);
public:
    static void Init(Local<Object> exports);
    void SetCurrentHolder(Local<Object> holder);
    Local<Object> GetCurrentHolder();

    uint8_t ReadUInt8();
    int8_t ReadInt8();
    uint16_t ReadUInt16LE();
    int16_t ReadInt16LE();
    uint32_t ReadUInt32LE();
    int32_t ReadInt32LE();
    double ReadDoubleLE();
    void ReadBytes(size_t length, uint8_t* buffer);
};

Local<Value> ReadArray(Decoder* decoder);
Local<Value> ReadValue(Decoder* decoder);
void ReadObject(Decoder* decoder, Local<Object> result);
Local<Value> ReadMapNative(Decoder* decoder);

#endif