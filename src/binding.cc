#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include "string.h"

using namespace v8;
using namespace node;

#define BN_BITS2 32

Handle<Value> Fmod(const Arguments &args) {
    HandleScope scope;
    if (!args[0]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New(
                      "First argument must be a Uint32Array")));
    }
    Local<Object> ao = args[0]->ToObject();
    if (!ao->HasIndexedPropertiesInExternalArrayData()) {
        return ThrowException(Exception::TypeError(String::New(
                      "First argument must be a Uint32Array")));
    }

    if (!args[1]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Second argument must be a Uint32Array")));
    }
    Local<Object> po = args[1]->ToObject();
    if (!po->HasIndexedPropertiesInExternalArrayData()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Second argument must be a Uint32Array")));
    }

    unsigned int* ret = (unsigned int*)ao->GetIndexedPropertiesExternalArrayData();
    unsigned int* p = (unsigned int*)po->GetIndexedPropertiesExternalArrayData();

    int j, k;
	int n, dN, d0, d1;
	unsigned int zz;
    int ret_len = ao->GetIndexedPropertiesExternalArrayDataLength();
    /* start reduction */
    dN = p[0] / BN_BITS2;
    for (j = ret_len - 1; j > dN;)
    {
        zz = ret[j];
        if (ret[j] == 0) { j--; continue; }
        ret[j] = 0;

        for (k = 1; p[k]; k++)
        {
            /* reducing component t^p[k] */
			n = p[0] - p[k];
			d0 = n % BN_BITS2;  d1 = BN_BITS2 - d0;
			n /= BN_BITS2; 
			ret[j-n] ^= (zz>>d0);
			if (d0) ret[j-n-1] ^= (zz<<d1);
        }

        /* reducing component t^0 */
        n = dN;
        d0 = p[0] % BN_BITS2;
        d1 = BN_BITS2 - d0;
        ret[j-n] ^= (zz >> d0);
        if (d0) ret[j-n-1] ^= (zz << d1);

    }

    /* final round of reduction */
    while (j == dN)
    {
        d0 = p[0] % BN_BITS2;
        zz = ret[dN] >> d0;
        if (zz == 0) break;
        d1 = BN_BITS2 - d0;

        /* clear up the top d1 bits */
        if (d0)
            ret[dN] = (ret[dN] << d1) >> d1;
        else
            ret[dN] = 0;
        ret[0] ^= zz; /* reduction t^0 component */

        for (k = 1; p[k]; k++)
        {
            unsigned int tmp_ulong;
            /* reducing component t^p[k]*/
            n = p[k] / BN_BITS2;   
            d0 = p[k] % BN_BITS2;
            d1 = BN_BITS2 - d0;
            ret[n] ^= (zz << d0);
            tmp_ulong = zz >> d1;
            if (d0 && tmp_ulong)
                    ret[n+1] ^= tmp_ulong;
         }
    }

    return scope.Close(Undefined());
}

extern "C"
void init(Handle<Object> target) {
  HandleScope scope;
  target->Set(String::NewSymbol("mod"),
          FunctionTemplate::New(Fmod)->GetFunction());

}
NODE_MODULE(binding, init)
