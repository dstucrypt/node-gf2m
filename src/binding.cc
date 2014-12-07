#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include "string.h"

using namespace v8;
using namespace node;

#define BN_BITS2 32
typedef unsigned int BN_ULONG;
#define BN_MASK2	(0xffffffffL)

/* Product of two polynomials a, b each with degree < BN_BITS2 - 1,
 * result is a polynomial r with degree < 2 * BN_BITS - 1
 * The caller MUST ensure that the variables have the right amount
 * of space allocated.
 */
static void bn_GF2m_mul_1x1(BN_ULONG *r1, BN_ULONG *r0, const BN_ULONG a, const BN_ULONG b)
{
	register BN_ULONG h, l, s;
	BN_ULONG tab[8], top2b = a >> 30; 
	register BN_ULONG a1, a2, a4;

	a1 = a & (0x3FFFFFFF); a2 = a1 << 1; a4 = a2 << 1;

	tab[0] =  0; tab[1] = a1;    tab[2] = a2;    tab[3] = a1^a2;
	tab[4] = a4; tab[5] = a1^a4; tab[6] = a2^a4; tab[7] = a1^a2^a4;

	s = tab[b       & 0x7]; l  = s;
	s = tab[b >>  3 & 0x7]; l ^= s <<  3; h  = s >> 29;
	s = tab[b >>  6 & 0x7]; l ^= s <<  6; h ^= s >> 26;
	s = tab[b >>  9 & 0x7]; l ^= s <<  9; h ^= s >> 23;
	s = tab[b >> 12 & 0x7]; l ^= s << 12; h ^= s >> 20;
	s = tab[b >> 15 & 0x7]; l ^= s << 15; h ^= s >> 17;
	s = tab[b >> 18 & 0x7]; l ^= s << 18; h ^= s >> 14;
	s = tab[b >> 21 & 0x7]; l ^= s << 21; h ^= s >> 11;
	s = tab[b >> 24 & 0x7]; l ^= s << 24; h ^= s >>  8;
	s = tab[b >> 27 & 0x7]; l ^= s << 27; h ^= s >>  5;
	s = tab[b >> 30      ]; l ^= s << 30; h ^= s >>  2;

	/* compensate for the top two bits of a */

	if (top2b & 01) { l ^= b << 30; h ^= b >> 2; } 
	if (top2b & 02) { l ^= b << 31; h ^= b >> 1; } 

	*r1 = h; *r0 = l;
} 

/* Product of two polynomials a, b each with degree < 2 * BN_BITS2 - 1,
 * result is a polynomial r with degree < 4 * BN_BITS2 - 1
 * The caller MUST ensure that the variables have the right amount
 * of space allocated.
 */
static void bn_GF2m_mul_2x2(BN_ULONG *r, const BN_ULONG a1, const BN_ULONG a0, const BN_ULONG b1, const BN_ULONG b0)
{
	BN_ULONG m1, m0;
	/* r[3] = h1, r[2] = h0; r[1] = l1; r[0] = l0 */
	bn_GF2m_mul_1x1(r+3, r+2, a1, b1);
	bn_GF2m_mul_1x1(r+1, r, a0, b0);
	bn_GF2m_mul_1x1(&m1, &m0, a0 ^ a1, b0 ^ b1);
	/* Correction on m1 ^= l1 ^ h1; m0 ^= l0 ^ h0; */
	r[2] ^= m1 ^ r[1] ^ r[3];  /* h0 ^= m1 ^ l1 ^ h1; */
	r[1] = r[3] ^ r[2] ^ r[0] ^ m1 ^ m0;  /* l1 ^= l0 ^ h0 ^ m0; */
}

int num_bits(BN_ULONG *a, int i)
{
	int r = 1, t, x, nz;
    nz = i - 1;
    while(a[nz] == 0) {
        nz--;
    }

    x = a[nz];
    if((t=x>>16) != 0) { x = t; r += 16; }
    if((t=x>>8) != 0) { x = t; r += 8; }
    if((t=x>>4) != 0) { x = t; r += 4; }
    if((t=x>>2) != 0) { x = t; r += 2; }
    if((t=x>>1) != 0) { x = t; r += 1; }
    return r + nz * 32;
}

Handle<Value> Fmul(const Arguments &args) {
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
    Local<Object> bo = args[1]->ToObject();
    if (!bo->HasIndexedPropertiesInExternalArrayData()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Second argument must be a Uint32Array")));
    }

    if (!args[2]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Third argument must be a Uint32Array")));
    }
    Local<Object> so = args[2]->ToObject();
    if (!so->HasIndexedPropertiesInExternalArrayData()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Third argument must be a Uint32Array")));
    }

    unsigned int* a = (unsigned int*)ao->GetIndexedPropertiesExternalArrayData();
    unsigned int* b = (unsigned int*)bo->GetIndexedPropertiesExternalArrayData();
    unsigned int* s = (unsigned int*)so->GetIndexedPropertiesExternalArrayData();


    int i, j, k, b_len, a_len, s_len;
	unsigned int x1, x0, y1, y0, zz[4];

    a_len = ao->GetIndexedPropertiesExternalArrayDataLength();
    b_len = bo->GetIndexedPropertiesExternalArrayDataLength();
    s_len = so->GetIndexedPropertiesExternalArrayDataLength();

    memset(s, 0, s_len * 4);

	for (j = 0; j < b_len ; j += 2)
	{
		y0 = b[j];
		y1 = ((j+1) == b_len) ? 0 : b[j+1];
		for (i = 0; i < a_len; i += 2)
		{
			x0 = a[i];
			x1 = ((i+1) == a_len) ? 0 : a[i+1];
			bn_GF2m_mul_2x2(zz, x1, x0, y1, y0);
			for (k = 0; k < 4; k++) s[i+j+k] ^= zz[k];
		}
	}

    return scope.Close(Undefined());
}

Handle<Value> Finv(const Arguments &args) {
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
    if (!args[2]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Third argument must be a Uint32Array")));
    }
    Local<Object> reto = args[2]->ToObject();
    if (!reto->HasIndexedPropertiesInExternalArrayData()) {
        return ThrowException(Exception::TypeError(String::New(
                      "Third argument must be a Uint32Array")));
    }

    BN_ULONG *bdp, *cdp, *udp, *vdp, *tmp;
    BN_ULONG *a = (unsigned int*)ao->GetIndexedPropertiesExternalArrayData();
    BN_ULONG* p = (unsigned int*)po->GetIndexedPropertiesExternalArrayData();
    BN_ULONG* ret = (unsigned int*)reto->GetIndexedPropertiesExternalArrayData();

    int len, words, i;


    words = po->GetIndexedPropertiesExternalArrayDataLength();
    len = sizeof(BN_ULONG) * words;
    bdp = (BN_ULONG *)malloc(len);
    udp = (BN_ULONG *)malloc(len);
    vdp = (BN_ULONG *)malloc(len);
    cdp = (BN_ULONG *)malloc(len);
    memcpy(udp, a, len);
    memcpy(vdp, p, len);
    memset(bdp, 0, len);
    memset(cdp, 0, len);
    *bdp = 1;

    int ubits = num_bits(udp, words),
        vbits = num_bits(vdp, words);

    while (1)
	{
		while (ubits && !(udp[0]&1))
		{
			BN_ULONG u0,u1,b0,b1,mask;

			u0   = udp[0];
			b0   = bdp[0];
			mask = (BN_ULONG)0-(b0&1);
			b0  ^= p[0]&mask;
			for (i=0;i<words-1;i++)
			{
				u1 = udp[i+1];
				udp[i] = ((u0>>1)|(u1<<(BN_BITS2-1)))&BN_MASK2;
				u0 = u1;
				b1 = bdp[i+1]^(p[i+1]&mask);
				bdp[i] = ((b0>>1)|(b1<<(BN_BITS2-1)))&BN_MASK2;
				b0 = b1;
			}
			udp[i] = u0>>1;
			bdp[i] = b0>>1;
			ubits--;
		}

		if (ubits<=BN_BITS2 && udp[0]==1) break;

		if (ubits<vbits)
		{
			i = ubits; ubits = vbits; vbits = i;
			tmp = udp; udp = vdp; vdp = tmp;
			tmp = bdp; bdp = cdp; cdp = tmp;
		}
		for(i=0;i<words;i++)
		{
			udp[i] ^= vdp[i];
			bdp[i] ^= cdp[i];
		}
		if (ubits==vbits)
		{
            ubits = num_bits(udp, words);
		}
	}

    memcpy(ret, bdp, len);

    free(bdp);
    free(cdp);
    free(udp);
    free(vdp);

    return scope.Close(Undefined());
}


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
  target->Set(String::NewSymbol("mul"),
          FunctionTemplate::New(Fmul)->GetFunction());
  target->Set(String::NewSymbol("inv"),
          FunctionTemplate::New(Finv)->GetFunction());

}
NODE_MODULE(binding, init)
