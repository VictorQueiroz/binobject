#include "node-encoder.h"
#include "node-decoder.h"
#include <nan.h>

void Init(Local<Object> exports, Local<Object> module) {
    Encoder::Init(exports->GetIsolate());
    Decoder::Init(exports->GetIsolate());
    NODE_SET_METHOD(exports, "ObjectDecoder", Decoder::CreateObject);
    NODE_SET_METHOD(exports, "ObjectEncoder", Encoder::CreateObject);
}

NODE_MODULE(encoder, Init);