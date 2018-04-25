import { CustomInstruction } from './custom-types';
import { PropertyType } from './constants';

export class ObjectDecoder {
    private offset: number;
    private buffer: Buffer;
    private custom?: CustomInstruction<any>[];

    constructor(buffer: Buffer, custom?: CustomInstruction<any>[]) {
        this.buffer = buffer;
        this.offset = 0;
        this.custom = custom;
    }

    public decode(): any {
        const type: PropertyType = this.readUInt8();

        if(type == PropertyType.Object)
            return this.decodeObject();
        else if(type == PropertyType.Array)
            return this.decodeArray();

        throw new Error(`Invalid initial message: ${PropertyType[type]} || ${type}`);
    }

    private readBytes(byteLength: number): Buffer {
        if((this.offset + byteLength) > this.buffer.byteLength)
            throw new Error('Exceeded maximum size of buffer, can\'t go any further');

        const result = this.buffer.slice(this.offset, this.offset + byteLength);

        this.offset += byteLength;

        return result;
    }

    private readInt8() {
        const result = this.buffer.readInt8(this.offset);

        this.offset += 1;
        return result;
    }

    private readUInt8() {
        const result = this.buffer.readUInt8(this.offset);

        this.offset += 1;
        return result;
    }

    private readInt16LE() {
        const result = this.buffer.readInt16LE(this.offset);

        this.offset += 2;
        return result;
    }

    private readUInt16LE() {
        const result = this.buffer.readUInt16LE(this.offset);

        this.offset += 2;
        return result;
    }

    private readInt32LE() {
        const result = this.buffer.readInt32LE(this.offset);

        this.offset += 4;
        return result;
    }

    private readUInt32LE() {
        const result = this.buffer.readUInt32LE(this.offset);

        this.offset += 4;
        return result;
    }

    private decodeArray() {
        const length = this.readNumber();
        const list = new Array(length);

        for(let i = 0; i < length; i++)
            list[i] = this.decodeValue();

        return list;
    }

    /**
     * Read an integer of any kind
     */
    private readNumber() {
        const type: PropertyType = this.readUInt8();

        return this.readNumberByType(type);
    }

    /**
     * Read number from current offset according to input type
     */
    private readNumberByType(type: PropertyType) {
        if(type == PropertyType.UInt8)
            return this.readUInt8();
        else if(type == PropertyType.Int8)
            return this.readInt8();
        else if(type == PropertyType.UInt16)
            return this.readUInt16LE();
        else if(type == PropertyType.Int16)
            return this.readInt16LE();
        else if(type == PropertyType.Int32)
            return this.readInt32LE();
        else if(type == PropertyType.UInt32)
            return this.readUInt32LE();

        throw new Error(`Invalid integer type: ${PropertyType[type]} || ${type}`);
    }

    public eof() {
        return this.offset == this.buffer.byteLength;
    }

    private readString(): string {
        return this.readBytes(this.readNumber()).toString('utf8');
    }

    private decodeObject(): any {
        const result: any = {};
        const propertiesLength = this.readNumber();

        for(let i = 0; i < propertiesLength; i++) {
            const propertyName = this.readString();
            result[propertyName] = this.decodeValue();
        }

        return result;
    }

    private readDouble(): number {
        const result = this.buffer.readDoubleLE(this.offset);

        this.offset += 8;

        return result;
    }

    private readNativeMap(): Map<any, any> {
        const map = new Map();
        const length = this.readNumber();

        for(let i = 0; i < length; i++) {
            const key = this.decodeValue();
            map.set(key, this.decodeValue());
        }

        return map;
    }

    private readCustom(type: number): any | undefined {
        if(!this.custom)
            return undefined;

        for(let i = 0; i < this.custom.length; i++) {
            const { value, processor } = this.custom[i];

            if(value == type) {
                const customDataLength = this.readNumber();

                return processor.decode(this.readBytes(customDataLength));
            }
        }

        return false;
    }

    private decodeValue(): any {
        const type: PropertyType = this.readUInt8();

        if(type == PropertyType.Null)
            return null;
        else if(type == PropertyType.Undefined)
            return undefined;
        else if(type == PropertyType.Object)
            return this.decodeObject();
        else if(type == PropertyType.String)
            return this.readBytes(this.readNumber()).toString('utf8');
        else if(type == PropertyType.Date)
            return new Date(this.readBytes(this.readNumber()).toString('utf8'));
        else if(type == PropertyType.Boolean)
            return this.readUInt8() == 1 ? true : false;
        else if(type == PropertyType.Array)
            return this.decodeArray();
        else if(type == PropertyType.Map)
            return this.readNativeMap();
        else if(type == PropertyType.Buffer)
            return this.readBytes(this.readNumber());
        else if(type == PropertyType.ArrayBuffer) {
            const output = this.readBytes(this.readNumber());

            return output.buffer.slice(output.byteOffset, output.byteOffset + output.byteLength);
        } else {
            const customResult: any | undefined = this.readCustom(type);

            if(customResult)
                return customResult;
        }
        
        return this.readNumberByType(type);
    }
}

export default ObjectDecoder;