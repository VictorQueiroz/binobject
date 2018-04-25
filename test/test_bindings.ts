import * as bo from '../';
import * as assert from 'assert';

export default function() {
    return {
        'it should encode complex object': function() {
            const buffer = new bo.ObjectEncoder().encode(require('./test.json'));
            assert.deepEqual(new bo.ObjectDecoder(buffer).decode(), require('./test.json'));
        },

        'it should encode native boolean parameters': function() {
            const buffer = new bo.ObjectEncoder().encode({ wellTested: true, badTested: false });

            assert.deepEqual(new bo.ObjectDecoder(buffer).decode(), { wellTested: true, badTested: false });
        }
    };
}