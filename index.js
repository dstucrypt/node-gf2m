var binding = require('./build/Release/binding');

module.exports.mod = function(a, p, inplace) {
    var ret, idx;
    if (inplace) {
        ret = a;
    } else {
        ret = new Uint32Array(a.length);
        for (idx = 0; idx < a.length; idx++) {
            ret[idx] = a[idx];
        }
    }
    binding.mod(ret, p);
    return ret;
};
