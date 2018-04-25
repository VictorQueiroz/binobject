#include <node.h>
#include "decoder.h"
#include <node_object_wrap.h>

#ifndef NODE_DECODER_H_
#define NODE_DECODER_H_

using namespace v8;

Local<Value> ReadValue(Isolate* isolate, bo_decoder* decoder);

class Decoder : public node::ObjectWrap {
private:
    bo_decoder* decoder;
    Decoder(size_t byte_length, uint8_t* buffer);
    ~Decoder();
    static Persistent<Function> constructor;
    static void Decode(const FunctionCallbackInfo<Value>& args);
    static void New(const FunctionCallbackInfo<Value>& args);
public:
    static void Init(Isolate* isolate);
    static void NewInstance(const FunctionCallbackInfo<Value>& args);
    static void CreateObject(const FunctionCallbackInfo<Value>& args);
};

#endif