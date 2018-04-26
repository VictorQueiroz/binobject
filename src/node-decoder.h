#include <node.h>
#include "decoder.h"
#include <node_object_wrap.h>

#ifndef NODE_DECODER_H_
#define NODE_DECODER_H_

using namespace v8;

class Decoder : public node::ObjectWrap {
private:
    bo_decoder* decoder;
    Local<Object> current_holder;
    Decoder(size_t byte_length, uint8_t* buffer);
    ~Decoder();
    static Persistent<Function> constructor;
    static void Decode(const FunctionCallbackInfo<Value>& args);
    static void New(const FunctionCallbackInfo<Value>& args);
public:
    static void Init(Isolate* isolate);
    static void NewInstance(const FunctionCallbackInfo<Value>& args);
    static void CreateObject(const FunctionCallbackInfo<Value>& args);
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

Local<Value> ReadValue(Isolate* isolate, Decoder* decoder);

#endif