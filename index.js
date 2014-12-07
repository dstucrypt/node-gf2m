var binding = require('./build/Release/binding');

module.exports.mod = function(a, p, r) {
    var idx;
    if (!r) {
        r = new Uint32Array(a.length);
        for (idx = 0; idx < a.length; idx++) {
            r[idx] = a[idx];
        }
    }
    binding.mod(r, p);
    return r;
};

module.exports.mul = function (a, b, r) {
    if (!r) {
        r = new Uint32Array(a.length + b.length + 4);
    }
    binding.mul(a, b, r);
    return r;
};

module.exports.inv = function (a, p, r) {
    binding.inv(a, p, r);
    return r;
};
