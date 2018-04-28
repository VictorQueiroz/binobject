#include "node-encoder.h"
#include "node-decoder.h"
#include <nan.h>

void Init(Local<Object> exports) {
    Encoder::Init(exports);
    Decoder::Init(exports);
}

NODE_MODULE(binobject, Init);