var assert = require("assert"),
    gf2m = require('./index.js');

var val_hex = 'aff3ee09cb429284985849e20de5742e194aa631490f62ba88702505629a65890',
    rv_hex = 'ff3ee09cb429284985849e20de5742e194aa631490f62ba88702505629a60895';

describe("gf2m", function() {
    describe("mod()", function() {
        it("should return mod of value", function() {
            var value_a = new Uint32Array([
                0x29a65890,
                0x87025056,
                0x90f62ba8,
                0x94aa6314,
                0xde5742e1,
                0x85849e20,
                0xb4292849,
                0xff3ee09c,
                0xa
            ]);
            var value_p = new Uint32Array([
                0x00000101, 0x0000000c
            ]);
            var expect = new Uint32Array([
                0x29a60895, 0x87025056, 0x90f62ba8,
                0x94aa6314, 0xde5742e1, 0x85849e20,
                0xb4292849, 0xff3ee09c, 0
            ]);

            var ret, idx;
            ret = gf2m.mod(value_a, value_p);
            assert.equal(ret.length, expect.length);
            for (idx = 0; idx < ret.length; idx++) {
                assert.equal(ret[idx], expect[idx]);
            }
        })

        it('should return mod of value 2', function () {

            var value_a = new Uint32Array([
                0xb0565d6b, 0x3bc2a3b1, 0x77907ef8, 0x8e01fc6e,
                0xb85d7914, 0x3aedc999, 0x7325be9b, 0x0a1af6dd,
                0x4589d7b8
            ]);

            var expect = new Uint32Array([
                0xdc2f76b7, 0x3bc2a19d, 0x77907ef8, 0x8e01fc6e,
                0xb85d7914, 0x3aedc999, 0x7325be9b, 0xa1af6dd,
                0x0
            ]);

            var value_p = new Uint32Array([
                0x00000101, 0x0000000c, 0
            ]);

            var ret, idx;
            ret = gf2m.mod(value_a, value_p);
            assert.equal(ret.length, expect.length);
            for (idx = 0; idx < ret.length; idx++) {
                assert.equal(ret[idx], expect[idx]);
            }

        });
    })
});
