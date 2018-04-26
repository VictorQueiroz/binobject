const bo = require('bindings')('binobject');

class CustomTypeProcessor() {

}

class BinaryObject {
    constructor(custom) {
        this.custom = custom;
    }
    encode(object) {
        return new bo.ObjectEncoder(this.custom).encode(object);
    }
    decode(buffer) {
        return new bo.ObjectDecoder(buffer, this.custom).decode();
    }
}

bo.BinaryObject = BinaryObject;
bo.CustomTypeProcessor = CustomTypeProcessor;

module.exports = bo;