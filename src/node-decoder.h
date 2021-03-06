#ifndef NODE_DECODER_H_
#define NODE_DECODER_H_

#include <nan.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <mffcodec.h>
#ifdef __cplusplus
}
#endif

using namespace v8;

class Decoder : public Nan::ObjectWrap {
private:
    mff_deserializer* decoder;
    Local<Object> current_holder;
    Decoder(size_t byte_length, uint8_t* buffer);
    ~Decoder();
    static Nan::Persistent<Function> constructor;
    static NAN_METHOD(Decode);
    static NAN_METHOD(New);
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
    float ReadFloatLE();
    void ReadBytes(size_t length, uint8_t* buffer);
};

Local<Value> ReadArray(Decoder* decoder);
Local<Value> ReadValue(Decoder* decoder);
void ReadObject(Decoder* decoder, Local<Object> result);
Local<Value> ReadMapNative(Decoder* decoder);
Local<Object> ReadBuffer(Decoder* decoder);

#endif