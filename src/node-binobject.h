#ifndef NODE_BINOBJECT_H_
#define NODE_BINOBJECT_H_

namespace BO {
    enum PropertyType {
        Object = 1,
        String = 2,
        Date = 3,
        Array = 4,
        Null = 5,
        Int8 = 6,
        UInt8 = 7,
        Int16 = 8,
        UInt16 = 9,
        Int32 = 10,
        UInt32 = 11,
        Int64 = 12,
        UInt64 = 13,
        Float = 14,
        Double = 15,
        Boolean = 16,
        Undefined = 17,
        Map = 18,
        Buffer = 19,
        ArrayBuffer = 20
    };
    namespace NumberErrors {
        enum NumberErrors {
            Ok = 1,
            InvalidType = 2,
            InvalidSize = 3,
            InvalidByteLength = 4
        };
    }
}

#endif