import * as assert from 'assert';
import { test } from 'sarg';
import { randomBytes } from 'crypto';
import { expect } from 'chai';

const bo = require('../');

class User {
    id: number; // uint32
    name: string; // string
    constructor(id: number, name: string) {
        this.id = id;
        this.name = name;
    }
}

class UserProcessor extends bo.CustomTypeProcessor<User> {
    encode(value: User) {
        const finalBuffer = Buffer.concat([
            Buffer.alloc(8),
            Buffer.from(value.name, 'utf8')
        ]);

        finalBuffer.writeUInt32LE(value.id, 0);
        finalBuffer.writeUInt32LE(finalBuffer.byteLength - 8, 4);

        return finalBuffer;
    }

    validate(user: any){
        return user instanceof User;
    }

    decode(buffer: Buffer): User {
        let offset: number = 0;

        const id: number = buffer.readUInt32LE(offset);
        offset += 4;

        const nameLength = buffer.readUInt32LE(offset);
        offset += 4;

        const name = buffer.slice(offset, offset + nameLength).toString('utf8');

        return new User(id, name);
    }
}

test('it should encode NaN numbers into NaN', function() {
    const buffer = new bo.ObjectEncoder().encode({
        object: { value: NaN }
    });
    expect(new bo.ObjectDecoder(buffer).decode()).to.be.deep.equal({
        object: { value: NaN }
    });
});

test('it should encode very big numbers', () => {
    const expected = {
        value: Number.MAX_SAFE_INTEGER
    };
    const buffer = new bo.ObjectEncoder().encode(expected);
    const decoded = new bo.ObjectDecoder(buffer).decode();
    expect(decoded).to.be.deep.equal(expected);
});

test('it should encode complex object', function() {
    const buffer = new bo.ObjectEncoder().encode(require('./test.json'));
    const decoded = new bo.ObjectDecoder(buffer).decode();
    expect(require('./test.json')).to.be.deep.equal(require('./test.json'));
    expect(decoded).to.be.deep.equal(require('./test.json'));
});

test('it should encode native boolean parameters', function() {
    const buffer = new bo.ObjectEncoder().encode({ wellTested: true, badTested: false });

    expect(new bo.ObjectDecoder(buffer).decode()).to.be.deep.equal({
        wellTested: true,
        badTested: false
    });
});

test('it should encode undefined', function() {
    assert.deepEqual(new bo.ObjectDecoder(new bo.ObjectEncoder().encode({ undefinedValue: undefined })).decode(), {
        undefinedValue: undefined
    });
});

test('it should encode null value', function() {
    assert.deepEqual(new bo.ObjectDecoder(new bo.ObjectEncoder().encode({ id: null })).decode(), { id: null });
});

test('it should encode buffer value', function() {   
    const id = randomBytes(64);
    const decoded = new bo.ObjectDecoder(new bo.ObjectEncoder().encode({
        id
    })).decode();

    assert.deepEqual(decoded, {
        id
    });
});

test('it should encode date objects', function() {
    const date = new Date("2019-04-08T04:29:22.810Z");

    assert.deepEqual(new bo.ObjectDecoder(new bo.ObjectEncoder().encode([{ createdAt: date }])).decode(), [{
        createdAt: date
    }]);
});

test('it should throw when receive invalid instructions argument', function() {
    assert.throws(function() {
        new bo.ObjectEncoder(<any>{});
    });
});

test('it should encode objects with custom types', function() {
    const instructions = [{
        value: 80,
        processor: new UserProcessor
    }];
    const encoder = new bo.ObjectEncoder(instructions);
    const buffer = encoder.encode({
        users: [new User(1, 'victor'), new User(2, 'gallins')]
    });

    const decoded: { users: any[] } = new bo.ObjectDecoder(buffer, instructions).decode();

    decoded.users.map(user => assert.ok(user instanceof User, 'all results inside `users` property should be equal to `User` instance'));

    assert.deepEqual(decoded, {
        users: [new User(1, 'victor'), new User(2, 'gallins')]
    });
});

test('it should support fractions', function() {
    const buffer = new bo.ObjectEncoder().encode({
        value: 1.79769e+308
    });
    expect(new bo.ObjectDecoder(buffer).decode()).to.be.deep.equal({
        value: 1.79769e+308
    });
});

test('it should support Map<K, V>', function() {
    const encoder = new bo.ObjectEncoder();
    const usersMap = new Map<number, User>();
    usersMap.set(1, new User(1, 'user 1'));
    usersMap.set(2, new User(2, 'user 2'));
    usersMap.set(3, new User(3, 'user 3'));
    usersMap.set(4, new User(4, 'user 3'));
    const buffer = encoder.encode({
        usersMap
    });
    expect(new bo.ObjectDecoder(buffer).decode()).to.be.deep.equal({
        usersMap
    });
});

test('it should support special characters', function() {
    assert.deepEqual(new bo.ObjectDecoder(new bo.ObjectEncoder().encode({ name: 'Cristóvão Galvão' })).decode(), {
        name: 'Cristóvão Galvão'
    });

    assert.deepEqual(new bo.ObjectDecoder(new bo.ObjectEncoder().encode({ specialText: '¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝÞàáâãäåæçèéêëìíîïðñòóôõöùúûüýþÿ' })).decode(), {
        specialText: '¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝÞàáâãäåæçèéêëìíîïðñòóôõöùúûüýþÿ'
    });
});