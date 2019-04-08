#ifndef NODE_ENCODER_H_
#define NODE_ENCODER_H_

#include <nan.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <mffcodec.h>
#ifdef __cplusplus
}
#endif

using namespace v8;

class Encoder : public Nan::ObjectWrap {
private:
    mff_serializer* encoder = nullptr;
    Encoder();
    ~Encoder();
    size_t Length();
    static Nan::Persistent<Function> constructor;
    static NAN_METHOD(New);
    static NAN_METHOD(Encode);
    Local<Object> holder;

public:
    static void Init(Local<Object> exports);
    static NAN_METHOD(NewInstance);
    static NAN_METHOD(CreateObject);

    void SetCurrentHolder(Local<Object> holder);
    Local<Object> GetHolder();

    /**
     * Copy current serializer contents to buffer and move
     * offset to zero
     */
    void FlushContents(void*);
    void WriteInt8(int8_t n);
    void WriteDoubleLE(double n);
    void WriteFloatLE(float n);
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