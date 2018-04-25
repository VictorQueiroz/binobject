#include <node.h>
#include "encoder.h"
#include <node_object_wrap.h>

#ifndef NODE_ENCODER_H_
#define NODE_ENCODER_H_

using namespace v8;

int WriteNumber(bo_encoder* encoder, double number);
void WriteNumber(Isolate* isolate, bo_encoder* encoder, double number);
void WriteNumber(Isolate* isolate, bo_encoder* encoder, Local<Number> value);

void WriteValue(Isolate* isolate, bo_encoder* encoder, Local<Value> value);
void WriteObject(Isolate* isolate, bo_encoder* encoder, Local<Object> object);

class Encoder : public node::ObjectWrap {
private:
    bo_encoder* encoder;
    Encoder();
    ~Encoder();
    static Persistent<Function> constructor;
    static void New(const FunctionCallbackInfo<Value>& args);
    static void Encode(const FunctionCallbackInfo<Value>& args);

public:
    static void Init(Isolate* isolate);
    static void NewInstance(const FunctionCallbackInfo<Value>& args);
    static void CreateObject(const FunctionCallbackInfo<Value>& args);
};

#endif