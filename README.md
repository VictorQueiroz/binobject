# binobject

Easy way to encode / decode JavaScript objects in binary format with support for Date, ArrayBuffer and Node.js Buffer.

## Installation
```
yarn add binobject
```

## Usage
```js
import { ObjectEncoder, ObjectDecoder } from 'binobject';
import { randomBytes } from 'crypto';

const sourceObject = {
    users: [{
        name: 'Astrid',
        birthday: new Date(),
        uniqueSignature: randomBytes(256)
    }]
};
const buffer = new ObjectEncoder().encode(sourceObject);

assert.deepEqual(new ObjectDecoder(buffer).decode(), sourceObject);
```

## Custom types

Both encoder and decoder class give you the possibility to define custom types for encoding and decoding. All you have to do is create a processor to decode and encode your custom type and then define a value. When defining a custom type it's important not to override a default type, you can check a full list starting from [here](https://github.com/VictorQueiroz/binobject/blob/master/src/constants.ts#L1). Also you need to be aware that currently this library support only up to 255 types (custom types included). See an example bellow:

```ts
import { BinaryObject, Processor } from 'binobject';
import { ObjectID } from 'mongodb';
import { deepEqual } from 'assert';

class ProcessorObjectID extends Processor<ObjectID> {
    validate(id: any): boolean {
        return id instanceof ObjectID == true;
    }
    decode(buffer: Buffer): ObjectID {
        return new ObjectID(buffer.toString('hex'));
    }
    encode(input: ObjectID): Buffer {
        return Buffer.from(input.toHexString(), 'hex');
    }
}

const binaryObject = new BinaryObject([{
    processor: new ProcessorObjectID,
    value: 60
}]);
const buffer = binaryObject.encode({
    _id: new ObjectID()
});

assert.deepEqual(binaryObject.decode(buffer), {
    _id: new ObjectID()
});
```

## More examples

Need more examples? Check out our [test.ts](https://github.com/VictorQueiroz/binobject/blob/master/test/test.ts) file