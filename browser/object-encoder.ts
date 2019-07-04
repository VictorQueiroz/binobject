import { CustomInstruction } from './custom-types';
import { PropertyType } from './constants';

export class ObjectEncoder {
    private buffers: Buffer[];
    private custom?: CustomInstruction<any>[];

    constructor(custom?: CustomInstruction<any>[]) {
        this.buffers = [];
        this.custom = custom;
    }

    private encodeArray(list: any[]) {
        const length = list.length;

        this.writeUInt8(PropertyType.Array);
        this.encodeNumber(length);

        for(let i = 0; i < length; i++)
            this.encodeValue(list[i]);
    }

    private encodeInteger(byteLength: number, value: number, unsigned: boolean) {
        let type: PropertyType;
        const buffer: Buffer = Buffer.allocUnsafe(1 + byteLength);

        if(byteLength == 1)
            type = unsigned ? PropertyType.UInt8 : PropertyType.Int8;
        else if(byteLength == 2)
            type = unsigned ? PropertyType.UInt16 : PropertyType.Int16;
        else if(byteLength == 4)
            type = unsigned ? PropertyType.UInt32 : PropertyType.Int32;
        else
            throw new Error('Integer above 32 bits of length is currently not supported');

        buffer.writeUInt8(type, 0);

        switch(type) {
            case PropertyType.Int8:
                buffer.writeInt8(value, 1);
                break;
            case PropertyType.UInt8:
                buffer.writeUInt8(value, 1);
                break;
            case PropertyType.Int16:
                buffer.writeInt16LE(value, 1);
                break;
            case PropertyType.UInt16:
                buffer.writeUInt16LE(value, 1);
                break;
            case PropertyType.Int32:
                buffer.writeInt32LE(value, 1);
                break;
            case PropertyType.UInt32:
                buffer.writeUInt32LE(value, 1);
        }

        this.buffers.push(buffer);
    }

    private encodeNativeMap(value: Map<any, any>) {
        const keys = value.keys();
        let current;

        this.writeUInt8(PropertyType.Map);
        this.encodeNumber(value.size);

        while((current = keys.next()) && !current.done) {
            const propertyName = current.value;

            this.encodeValue(propertyName);
            this.encodeValue(value.get(propertyName));
        }
    }

    private encodeNumber(value: number) {
        if(isNaN(value)) {
            this.writeUInt8(PropertyType.Null);
            return;
        }
        if(!Number.isInteger(value)) {
            this.encodeDouble(value);
        } else if(value >= -0x80 && value <= 0x7f)
            this.encodeInteger(1, value, false);
        else if(value >= 0 && value <= 0xff)
            this.encodeInteger(1, value, true);
        else if(value >= 0 && value <= 0xffff)
            this.encodeInteger(2, value, true);
        else if(value >= -0x8000 && value <= 0x7fff)
            this.encodeInteger(2, value, false);
        else if(value >= 0 && value <= 0xffffffff)
            this.encodeInteger(4, value, true);
        else if(value >= -0x80000000 && value <= 0x7fffffff)
            this.encodeInteger(4, value, false);
    }

    private encodeDouble(n: number) {
        const buffer = Buffer.allocUnsafe(9);
        buffer.writeUInt8(PropertyType.Double, 0);
        buffer.writeDoubleLE(n, 1);
        this.buffers.push(buffer);
    }

    private writeUInt8(value: number) {
        const buffer = Buffer.allocUnsafe(1);
        buffer.writeUInt8(value, 0);
        this.buffers.push(buffer);
    }

    private writeString(value: string) {
        const buffer = Buffer.from(value, 'utf8');

        this.encodeNumber(buffer.byteLength);
        this.buffers.push(buffer);
    }

    private writeBuffer(buffer: Buffer | ArrayBuffer) {
        let type: PropertyType;
        let output: Buffer;

        if(Buffer.isBuffer(buffer)) {
            type = PropertyType.Buffer;
            output = buffer;
        } else if(buffer instanceof ArrayBuffer) {
            type = PropertyType.ArrayBuffer;
            output = Buffer.from(<any>(new Uint8Array(buffer)));
        } else {
            throw new Error('Invalid buffer input');
        }

        this.writeUInt8(type);
        this.encodeNumber(output.byteLength);
        this.buffers.push(output);
    }

    writeDouble(number: number) {
        const buffer = Buffer.allocUnsafe(8);

        buffer.writeDoubleLE(number, 0);
        this.buffers.push(buffer);
    }

    private encodeValue(value: any) {
        if(value == null){
            this.writeUInt8(PropertyType.Null);
        } else if(value == undefined) {
            this.writeUInt8(PropertyType.Undefined);
        } else if(Buffer.isBuffer(value) || value instanceof ArrayBuffer) {
            this.writeBuffer(value);
        } else if(value instanceof Map) {
            this.encodeNativeMap(value);
        } else if(value instanceof Date) {
            this.writeUInt8(PropertyType.Date);
            this.writeDouble(value.getTime());
        } else if(typeof value == 'string') {
            this.writeUInt8(PropertyType.String);
            this.writeString(value);
        } else if(Array.isArray(value)) {
            this.encodeArray(value);
        } else if(typeof value == 'boolean') {
            this.writeUInt8(PropertyType.Boolean);
            this.writeUInt8(value ? 1 : 0);
        } else if(typeof value == 'number') {
            this.encodeNumber(value);
        } else if(typeof value == 'object' && value.constructor == Object) {
            this.encodeObject(value);
        } else {
            if(!this.encodeCustom(value))
                throw new Error('Invalid property type: ' + (typeof value));
        }
    }

    private addBuffer(buffer: Buffer) {
        this.buffers.push(buffer);
    }

    private encodeCustom(value: any): boolean {
        if(!this.custom)
            return false;

        for(let i = 0; i < this.custom.length; i++) {
            const { processor, value: customType } = this.custom[i];

            if(processor.validate(value)) {
                const result = processor.encode(value);

                this.writeUInt8(customType);
                this.encodeNumber(result.byteLength);
                this.addBuffer(result);
                return true;
            }
        }
        return false;
    }

    private encodeObject(object: any) {
        const keys = Object.keys(object);

        // store properties length in buffer
        this.writeUInt8(PropertyType.Object);
        this.encodeNumber(keys.length);

        for(let prop of keys) {
            const value = object[prop];
            
            this.writeString(prop);
            this.encodeValue(value);
        }
    }

    public encode(object: any) {
        if(Array.isArray(object)) {
            this.encodeArray(object);
        } else if(typeof object === 'object' && object !== null) {
            this.encodeObject(object);
        } else {
            throw new Error('encode() argument must be either an array or an object');
        }
        return Buffer.concat(this.buffers.splice(0, this.buffers.length));
    }
}

export default ObjectEncoder;