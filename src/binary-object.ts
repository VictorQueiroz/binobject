import ObjectEncoder from './object-encoder';
import ObjectDecoder from './object-decoder';
import { CustomInstruction } from './custom-types';

class BinaryObject {
    custom?: CustomInstruction<any>[];
    constructor(custom?: CustomInstruction<any>[]) {
        this.custom = custom;
    }

    encode(object: any): Buffer {
        return new ObjectEncoder(this.custom).encode(object);
    }

    decode(buffer: Buffer): any {
        return new ObjectDecoder(buffer, this.custom).decode();
    }
}

export default BinaryObject;