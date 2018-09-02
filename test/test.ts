import * as fs from 'fs';
import * as assert from 'assert';
import { ObjectID } from 'bson';
import { randomBytes } from 'crypto';
import { test } from 'sarg';
import { ObjectEncoder, CustomTypeProcessor, ObjectDecoder } from '../browser';

class ProcessorObjectID extends CustomTypeProcessor<ObjectID> {
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


test('it should support date field objects', function() {
    let buffer: Buffer;
    const sourceObject = {
        user: {
            name: 'Danny Rayburn',
            createdAt: new Date('2018-04-17T21:36:54.033Z'),
        }
    };

    buffer = new ObjectEncoder().encode(sourceObject);

    const decoder = new ObjectDecoder(buffer);

    assert.deepEqual(decoder.decode(), sourceObject);
});

test('it should throw when tries to access beyond buffer length', function() {
    const buffer = new ObjectEncoder().encode({
        id: 10
    });
    const decoder = new ObjectDecoder(buffer);

    assert.deepEqual(decoder.decode(), { id: 10 });
    assert.throws(function() {
        decoder.decode();
    });
});

test('it should encode / decode big and deep javascript object', function() {
    let buffer: Buffer;
    const sourceObject = JSON.parse(fs.readFileSync(__dirname + '/test.json', 'utf8'));

    buffer = new ObjectEncoder().encode(sourceObject);

    assert.deepEqual(new ObjectDecoder(buffer).decode(), sourceObject);
});

test('it should encode / decode boolean fields', function() {
    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({
        done: true
    })).decode(), { done: true });
});

test('it should support undefined values', function() {
    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({
        value: undefined
    })).decode(), { value: undefined });
});

test('it should support null values', function() {
    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({
        value: null
    })).decode(), { value: null });
});

test('it should support ES6 map objects', function() {
    const map = new Map<any, Buffer>();

    for(let i = 0; i < 100; i++) {
        map.set(i, randomBytes(128));
    }

    map.set('(', Buffer.from(')', 'utf8'));
    map.set(Buffer.from('key 1', 'utf8'), Buffer.from('value 1', 'utf8'));
    map.set(Buffer.from('key 2', 'utf8'), Buffer.from('value 2', 'utf8'));

    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({
        mapsList: [map]
    })).decode(), { mapsList: [map] });
});

test('it should support all kinds of buffers', function() {
    const source = {
        nativeArrayBuffer: Buffer.from('simple text for array buffer', 'utf8'),
        bytes: randomBytes(256)
    };

    const decoded = new ObjectDecoder(new ObjectEncoder().encode(source)).decode();

    assert.ok(Buffer.from(source.nativeArrayBuffer).equals(Buffer.from(decoded.nativeArrayBuffer)));
});

test('it should support maximum of uint32', function() {
    const buffer = new ObjectEncoder().encode({
        maxUint32: 4294967295
    });
    assert.deepEqual(new ObjectDecoder(buffer).decode(), {
        maxUint32: 4294967295
    });
});

test('it should support minimum of int32', function() {
    const buffer = new ObjectEncoder().encode({
        maxInt32: -2147483648
    });
    assert.deepEqual(new ObjectDecoder(buffer).decode(), {
        maxInt32: -2147483648
    });
});

test('it should support maximum of int32', function() {
    const buffer = new ObjectEncoder().encode({
        maxInt32: 2147483647
    });
    assert.deepEqual(new ObjectDecoder(buffer).decode(), {
        maxInt32: 2147483647
    });
});

test('it should provide features for custom types', function() {
    const user = { _id: new ObjectID() };
    const instructions = [{
        processor: new ProcessorObjectID,
        value: 128
    }];

    const buffer = new ObjectEncoder(instructions).encode({
        user
    });

    const decoded = new ObjectDecoder(buffer, instructions).decode();

    assert.ok(decoded.user._id instanceof ObjectID);
    assert.ok(decoded.user._id.equals(user._id));
});

test('it should support special characters', function() {
    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({ name: 'Cristóvão Galvão' })).decode(), {
        name: 'Cristóvão Galvão'
    });

    assert.deepEqual(new ObjectDecoder(new ObjectEncoder().encode({ specialText: '¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝÞàáâãäåæçèéêëìíîïðñòóôõöùúûüýþÿ' })).decode(), {
        specialText: '¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝÞàáâãäåæçèéêëìíîïðñòóôõöùúûüýþÿ'
    });
});