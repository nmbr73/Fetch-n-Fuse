
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define mortonBuffer iChannel0
#define sortedBuffer iChannel0
#define particleBuffer iChannel0
#define pixelNearestBuffer iChannel1

//#define maxRes _fminf(to_float2(800.0f, 450.0f), iResolution)
#define maxRes _fminf(to_float2(512.0f, 512.0f), iResolution)
//#define maxRes _fminf(to_float2(128.0f, 128.0f), iResolution)
//#define maxRes _fminf(to_float2(512.0f, 256.0f), iResolution)
//#define maxRes _fminf(to_float2(iResolution.x, 256.0f), iResolution)
//#define maxRes _fminf(to_float2(512.0f, iResolution.y), iResolution)
//#define maxRes iResolution
#define realRes iResolution
#define powerOfTwoRes to_float2(2048.0f, 2048.0f)
//#define realRes maxRes
//#define maxRes iResolution

// Try this true for more Matrix fun :)
const bool justSentinels = false;

// number of particles will be 2^magicNumberDoNotChange = 64k
// I haven't figured out why it seems to work only when this number is 16
const int magicNumberDoNotChange = 16;
const int MAX_ITER = 12;
const int maxBin = 32;
const int vec4Count = 1;
#define PART part
#define M_PI 3.14159265358979323846264338327950288f

__DEVICE__ float3 hsv2rgb(float3 c) {
  float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
  return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ int getMaxPasses2(float2 res) {
    return int(_ceil(_log2f(res.x * res.y)));
}

struct mPartitionData {
    int partitionCount;
    int maxIndex;
    int particlesPerPartition;
    int index;
    int partitionIndex;
    int offset;
    int pastIndex;
    int futureIndex;
    int2 futureCoord;
    float4 futureParticle;
    bool overflow;
};
    
__DEVICE__ float2 extractPosition(float4 data) {
    return swi2(data,y,z);
}

// BEGIN QUALITY HASHES

__DEVICE__ uint baseHash(uint2 p)
{
    //p = 1103515245U*((p >> 1U)^(swi2(p,y,x)));
    p = 1103515245U*(make_uint2((p.x >> 1U)^(p.y),(p.y >> 1U)^(p.x)));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}


//---------------------2D input---------------------

__DEVICE__ float hash12(uint2 x)
{
    uint n = baseHash(x);
    return (float)(n)*(1.0f/(float)(0xffffffffU));
}

__DEVICE__ float2 hash22(uint2 x)
{
    uint n = baseHash(x);
    uint2 rz = make_uint2(n, n*48271U);

    return to_float2(rz.x & (0x7fffffffU),rz.y & (0x7fffffffU))/(float)(0x7fffffff);
}

__DEVICE__ float3 hash32(uint2 x)
{
    uint n = baseHash(x);
    uint3 rz = make_uint3(n, n*16807U, n*48271U);
    return to_float3(rz.x & (0x7fffffffU),rz.y & (0x7fffffffU),rz.z & (0x7fffffffU))/(float)(0x7fffffff);
}

__DEVICE__ float4 hash42(uint2 x)
{
    uint n = baseHash(x);
    uint4 rz = make_uint4(n, n*16807U, n*48271U, n*69621U); //see: http://random.mat.sbg.ac.at/results/karl/server/node4.html
    return to_float4(rz.x & (0x7fffffffU),rz.y & (0x7fffffffU),rz.z & (0x7fffffffU),rz.w & (0x7fffffffU))/(float)(0x7fffffff);
}

//--------------------------------------------------


//Example taking an arbitrary float value as input
/*
  This is only possible since the hash quality is high enough so that
  floored float input doesn't break the process when the raw bits are used
*/

union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };


union Zahl
 {
   float2  _Float; //32bit float
   uint2   _Uint;  //32bit unsigend integer
 };

__DEVICE__ float4 hash42(float2 _x)
{
    Zahl z;
  
    z._Float = _x;

    //uint n = baseHash(floatBitsToUint(_x));
    uint n = baseHash(z._Uint);
    uint4 rz = make_uint4(n, n*16807U, n*48271U, n*69621U);

    return to_float4(rz.x & (0x7fffffffU), rz.y & (0x7fffffffU), rz.z & (0x7fffffffU), rz.w & (0x7fffffffU))/(float)(0x7fffffff);
}

// END QUALITY HASHES


__DEVICE__ float hash( uint2 _x )
{
    uint2 q = 1103515245U * ( make_uint2((_x.x>>1U) ^ (_x.y),(_x.y>>1U) ^ (_x.x) ));
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return (float)(n) * (1.0f/(float)(0xffffffffU));
}

__DEVICE__ float2 getRes(float2 res) {
    //return to_float2(_exp2f(_ceil(_log2f(_fmaxf(res.x, res.y)))));
    return powerOfTwoRes;
}

__DEVICE__ int toIndexCol(in float2 fragCoord, in float2 resolution, inout float3 *col) {
    int xl = (int)(fragCoord.x);
    int yl = (int)(fragCoord.y);
    int2 res = to_int2_cfloat(resolution);
    int div2 = 1;
    /*
    for (int i = 0; i < MAX_ITER; i++) {
        res /= 2;
        div2 *= 2;
        if (res.x == 0 && res.y == 0) break;
    }
    res = to_int2(div2);
  */
    int index = 0;
    int div = 1;
    div2 = 1;
    bool colorDone = false;
    for (int i = 0; i < MAX_ITER; i++) {
        int2 rest = to_int2(res.x % 2, res.y % 2);
        //res /= 2;
        res.x /= 2;
        res.y /= 2;
        
        if (res.x == 0 && res.y == 0) break;
        div *= 4;
        div2 *= 2;
        int x = (int)(xl >= res.x);
        int y = (int)(yl >= res.y);
        xl -= x * res.x;
        yl -= y * res.y;
        //res += x * rest.x;
        //res += y * rest.y;
        int thisIndex = y * 2 + x;
        index = index * 4 + thisIndex;

        if (!colorDone) {
            float2 uv = to_float2(xl, yl) / to_float2_cint(res);
            float2 center = to_float2_s(0.5f);
            float d = distance_f2(uv, center);
            float r = (float)(d < 0.25f);
            bool border = d > 0.25f - 0.02f / (float)(div2) && d < 0.25f;
            if (border) {
                colorDone = true;
            } else {
              //*col = ((float)((int)(*col) ^ (int)(r)));
              *col = to_float3((int)(*col).x ^ (int)(r),(int)(*col).y ^ (int)(r),(int)(*col).z ^ (int)(r));
              
              
            }
        }
    }
    //return res.x * res.y - index - 1;
    return index;
}

__DEVICE__ int toIndexFull(in float2 fragCoord, in float2 resolution) {
    float3 col = to_float3_s(0.0f);
    int index = toIndexCol(fragCoord, resolution, &col);
    //index += 1;
    return index;
}

__DEVICE__ int2 fromIndexFull(in int index, in float2 resolution) {
    //index -= 1;
    
    int2 fc = to_int2(0,0);
    int div = 1;
    int2 div2 = to_int2(1,1);
    int2 res = to_int2_cfloat(resolution);
    //index = res.x * res.y - index - 1;
    for (int i = 0; i < MAX_ITER; i++) {
        //res /= 2;
        res.x /= 2;
        res.y /= 2;
        //int rx = res.x % 2 == 0 ? 2 : 1;
        //int ry = res.y % 2 == 0 ? 2 : 1;

        int thisIndex = index % 4;
        fc.x += div2.x * (thisIndex % 2);
        fc.y += div2.y * (thisIndex / 2);
        index = index / 4;

        div2 *= 2;
        if (index == 0) break;
    }
    return fc;
}

__DEVICE__ int2 fromLinear(in int index, in float2 resolution) {
    //index -= 1;
    return to_int2(index % (int)(resolution.x), index / (int)(resolution.x));
}

__DEVICE__ int toLinear(in float2 fragCoord, in float2 resolution) {
    int index = (int)(fragCoord.x) + (int)(fragCoord.y) * (int)(resolution.x);
    //index += 1;
    return index;
}

#define toIndex(a) toIndex2(mortonBuffer, a, realRes, iResolution)
__DEVICE__ int toIndex2(in __TEXTURE2D__ channel, in float2 fragCoord, in float2 res, float2 iResolution) {
    int2 fc = to_int2_cfloat(fragCoord * res);
    //float4 index = texelFetch(channel, fc, 0);
    float4 index = texture(channel, (make_float2(fc)+0.5f)/iResolution);
    return (int)(index.w);
}

__DEVICE__ float2 getPosition(__TEXTURE2D__ channel, int index, float2 res, float2 iResolution) {
    int2 fc = fromLinear(index, res);
    //float4 data = texelFetch(channel, fc, 0);
    float4 data = texture(channel, (make_float2(fc)+0.5f)/iResolution);
    return fract_f2(extractPosition(data));
}

__DEVICE__ int maxLinear(float2 res) {
    return (int)(_exp2f(_floor(_log2f((float)(toLinear(res - 1.0f, res))))));
}

__DEVICE__ bool isLinearValid(in int index, float2 iResolution) {
    float2 res = iResolution;
    //return true;
  return index < maxLinear(iResolution);
}

__DEVICE__ bool isValid(in float2 fragCoord, float2 iResolution) {
    float2 res = iResolution;
    return isLinearValid(toLinear(fragCoord, res), iResolution);
}

#define getPartitionData(a, b, c) getPartitionData2(a, b, c, realRes, iResolution)
__DEVICE__ mPartitionData getPartitionData2(__TEXTURE2D__ channel, float2 fragCoord, float2 res, float2 rRes, float2 iResolution) {
    //fragCoord = fragCoord / rRes * res;
    mPartitionData mRet; // sehr sehr unglücklicher Name, da gleich eine gleichnamige Struktur erzeugt wird - böse - böse
    //int maxPasses = getMaxPasses(res);
    //mRet.partitionCount = int(_exp2f(_ceil(_log2f(float(maxPasses)))));
    mRet.partitionCount = magicNumberDoNotChange;
    //mRet.maxIndex = toLinear(res - 1.0f, res);
    mRet.maxIndex = maxLinear(res);
    mRet.particlesPerPartition = mRet.maxIndex / mRet.partitionCount;
    mRet.index = toLinear(fragCoord, res);
    mRet.partitionIndex = mRet.index / mRet.particlesPerPartition;
    mRet.offset = mRet.index % mRet.particlesPerPartition;
    mRet.futureIndex = mRet.index - mRet.particlesPerPartition;
    mRet.futureCoord = fromLinear(mRet.futureIndex, res);
    //mRet.futureParticle = texelFetch(channel, mRet.futureCoord, 0);
    mRet.futureParticle = texture(channel, (to_float2_cint(mRet.futureCoord)+0.5f)/iResolution);
    
    mRet.pastIndex = mRet.index + mRet.particlesPerPartition;
    mRet.overflow = mRet.index >= mRet.maxIndex;
  
    //(mRet.partitionIndex - 1) * mRet.particlesPerPartition + mRet.offset;
    return mRet;
}

__DEVICE__ int getMaxPartition(mPartitionData pd) {
    // TODO: optimize / hardcode
    int k = 0;
    for (int i = 0; i <= pd.partitionCount; i++) {
        int n = 1 << i;
    if (2 * n > pd.particlesPerPartition || pd.particlesPerPartition % n != 0) break;
        k = i;
    }
    return k + 1;
    //return k;
}

struct mRet {
    int dIndex;
    int Am;
    float4 vi;
    float4 v;
    float2 pos;
    bool valid;
};

#define getMD(a, b, c) getMD2(particleBuffer, mortonBuffer, a, b, c, realRes, iResolution)
__DEVICE__ mRet getMD2(__TEXTURE2D__ channel, __TEXTURE2D__ mchannel, int part, int m, float2 res, float2 rRes, float2 iResolution) {
    float2 fc = to_float2_cint(fromLinear(m, res));
    //float4 v = texelFetch(channel, to_int2(fc), 0);
    float4 v = texture(channel, (make_float2(to_int2_cfloat(fc))+0.5f)/iResolution);
    
    float2 pos = extractPosition(v);
    int Am = toIndex2(mchannel, pos, rRes, iResolution);
    int maxIndex = toLinear(res - 1.0f, res);
    bool valid = m >= 0 && m <= maxIndex && isLinearValid(m, res);
    //valid = true;
    
    mRet ret = {m, Am, to_float4_s(0.0f), v, pos, valid};

    return ret; //mRet(m, Am, to_float4_s(0.0f), v, pos, valid);
}

#define getM(a, b, c) getM2(sortedBuffer, particleBuffer, mortonBuffer, a, b, c, realRes, iResolution)
__DEVICE__ mRet getM2(__TEXTURE2D__ channel, __TEXTURE2D__ pchannel, __TEXTURE2D__ mchannel, int part, int m, float2 res, float2 rRes, float2 iResolution) {
    float2 fc = to_float2_cint(fromLinear(m, res));
    //float4 v = texelFetch(channel, to_int2(fc), 0);
    //float4 v = texture(channel, (make_float2(to_int2_cfloat(fc)+0.5f)/iResolution);
    A2F v;
    v.F = texture(channel, (make_float2(to_int2_cfloat(fc))+0.5f)/iResolution);
   
    mRet ret2 = getMD2(pchannel, mchannel, part, (int)(v.A[part]), res, rRes, iResolution);
    int maxIndex = toLinear(res - 1.0f, res);
    bool valid = m >= 0 && m <= maxIndex && isLinearValid(m, res);
    //valid = true;
    
    mRet ret = {(int)(v.A[part]), ret2.Am, v.F, ret2.v, ret2.pos, valid && ret2.valid};
    
    return ret;//mRet((int)(v.A[part]), ret2.Am, v.F, ret2.v, ret2.pos, valid && ret2.valid);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Resources:
// https://www.ics.uci.edu/~goodrich/pubs/skip-journal.pdf
// Sorting with GPUs: A Survey: https://arxiv.org/pdf/1709.02520.pdf

// Practice JavaScript implementation: http://jsbin.com/zeyiraw/

// https://www.shadertoy.com/view/XlcGD8
// https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter46.html
// https://stackoverflow.com/questions/26093629/glsl-odd-even-merge-sort
// https://bl.ocks.org/zz85/cafa1b8b3098b5a40e918487422d47f6

//#define resetPressed (texelFetch(iChannel1, to_int2(KEY_LEFT,1),0 ).x > 0.5f)

//const int KEY_LEFT  = 37;
//const int KEY_UP    = 38;
//const int KEY_RIGHT = 39;
//const int KEY_DOWN  = 40;

__DEVICE__ int extractIndex(float4 v, int part) {
    A2F tmp;
    tmp.F = v;
    return (int)(tmp.A[PART]);
}

__DEVICE__ int getIndex(int part, mRet A, float2 res) {  
    return A.Am;
}

__DEVICE__ bool compare(int part, mRet A, mRet B, float2 res) {
    return getIndex(part, A, res) < getIndex(part, B, res);
}

__DEVICE__ bool cutValid(int part, int n1, int n2, int astart, int bstart, int to, int m2, int x, float2 res, float2 iResolution, __TEXTURE2D__ iChannel0) {
    int apos = m2 - 1;
    bool aValid = apos >= 0 && apos < n1;
    int bpos = to - m2 - 1;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);

    int cv11 = getIndex(part, Amret, res);
    int cv12 = getIndex(part, Bmret, res);
    return (
        aValid && bValid && apos >= 0 && bpos >= 0 ? _fmaxf(cv11, cv12) <= x
        : bValid && apos < 0 && bpos >= 0 ? cv12 <= x
        : aValid && apos >= 0 && bpos < 0 ? cv11 <= x
        : Amret.valid && Bmret.valid);
}

__DEVICE__ bool cutCValid(int part, int n1, int n2, int astart, int bstart, int to, int bm2, int x, float2 res, float2 iResolution, __TEXTURE2D__ iChannel0) {
    int apos = to - bm2 - 1;
    int bpos = bm2 - 1;
    bool aValid = apos >= 0 && apos < n1;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);
    int cvc11 = getIndex(part, Amret, res);
    int cvc12 = getIndex(part, Bmret, res);
    return (
      aValid && bValid && apos >= 0 && bpos >= 0 ? _fmaxf(cvc11, cvc12) <= x
        : bValid && apos < 0 && bpos >= 0 ? cvc12 <= x
        : aValid && apos >= 0 && bpos < 0 ? cvc11 <= x
        : Amret.valid && Bmret.valid);
}

__DEVICE__ mRet checkIndex(int part, int n1, int n2, int astart, int bstart, int to, int apos, float2 res, float2 iResolution, __TEXTURE2D__ iChannel0) {
    bool aValid = apos >= 0 && apos < n1;
    int bpos = to - apos;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);

    int candA = getIndex(part, Amret, res);
    bool candAv = cutValid(part, n1, n2, astart, bstart, to, apos, candA, res, iResolution,iChannel0) && aValid;
    Amret.valid = Amret.valid && candAv;

    int candB = getIndex(part, Bmret, res);
    bool candBv = cutCValid(part, n1, n2, astart, bstart, to, bpos, candB, res,iResolution,iChannel0) && bValid;
    Bmret.valid = Bmret.valid && candBv;

    if (candAv && candBv) {
        if (candA < candB) {
            return Amret;
        } else {
            return Bmret;
        }
    } else if (candAv) {
        return Amret;
    }
    return Bmret;
}

__DEVICE__ mRet binarySearchForMergeSlim(
    int part,
    int targetOffset, int n1, int n2, float2 res,
    int astart, int bstart, float2 iResolution,
    __TEXTURE2D__ iChannel0) {

    int L1 = _fminf(max(targetOffset + 1 - n1, 0), n1 - 1);
    int R1 = _fminf(targetOffset + 1, n1);
    int L2 = _fminf(max(targetOffset + 1 - n2, 0), n2 - 1);
    int R2 = _fminf(targetOffset + 1, n2);

    int OL1 = L1;
    int OR2 = R2;

    int i = 0;

    mRet ret;

    bool bValid = true;

    for (i = 0; i < maxBin && L1 < R1 && (L2 < R2 || !bValid); i++) {
        int m = (L1 + R1) / 2 + (L1 + R1) % 2;
        int bm = targetOffset - m;
        int apos = m;
        bool aValid = apos >= 0 && apos < n1;
        int bpos = bm;
        bValid = bpos >= 0 && bpos < n2;

        mRet Amret = getM(part, astart + apos, res);
        aValid = aValid && Amret.valid;
        mRet Bmret = getM(part, bstart + bpos, res);
        bValid = bValid && Bmret.valid;

        bool comparison = compare(part, Amret, Bmret, res) && aValid && bValid;
        bool inUpperHalf = comparison;

        // m + 1 to R1
        L1 = inUpperHalf ? m : L1;
        // L1 to m
        R1 = !inUpperHalf ? m - 1 : R1;
        // bm + 1 to R2
        L2 = !inUpperHalf ? bm : L2;
        // L2 to bm
        R2 = inUpperHalf ? bm : R2;
    }
    mRet error = {-1, -1, to_float4_s(-1.0f), to_float4_s(-1.0f), to_float2_s(-1.0f), false};
    //mRet error = mRet(-1, to_float4_aw(-1.0f), false);

    int apos = L1;
    int bpos = targetOffset - L1;
    bValid = bpos >= 0 && bpos < n2;

    mRet AL1ret = getM(part, astart + apos, res);
    mRet BL1ret = getM(part, bstart + bpos, res);
    //return AL1ret;

    // XXX: AL1ret and BL1ret should be valid I hope
    int m2 = getIndex(part, AL1ret, res) < getIndex(part, BL1ret, res) && bValid ? L1 + 1 : L1;
    int bm2 = OR2 - (m2 - OL1);
    bool bm2Valid = bm2 >= 0 && bm2 < n2;
    bool bm2Min1Valid = bm2 - 1 >= 0 && bm2 - 1 < n2;

    int to = targetOffset;

    mRet cand1 = checkIndex(part, n1, n2, astart, bstart, to, m2, res, iResolution, iChannel0);
    mRet cand2 = checkIndex(part, n1, n2, astart, bstart, to, bm2, res, iResolution, iChannel0);
    cand2.valid = cand2.valid && bm2Valid;
    mRet cand3 = checkIndex(part, n1, n2, astart, bstart, to, m2 - 1, res, iResolution, iChannel0);
    mRet cand4 = checkIndex(part, n1, n2, astart, bstart, to, bm2 - 1, res, iResolution, iChannel0);
    cand4.valid = cand4.valid && bm2Min1Valid;

    ret = cand1;
    if (cand2.valid && (compare(part, cand2, ret, res) || !ret.valid)) {
        ret = cand2;
    }
    if (cand3.valid && (compare(part, cand3, ret, res) || !ret.valid)) {
        ret = cand3;
    }
    if (cand4.valid && (compare(part, cand4, ret, res) || !ret.valid)) {
        ret = cand4;
    }
    mRet AnMin1 = getM(part, astart + n1 - 1, res);
    mRet BtoMinN = getM(part, bstart + to - n1, res);
    mRet BnMin1 = getM(part, bstart + n2 - 1, res);
    mRet AtoMinN = getM(part, astart + to - n2, res);
    if (targetOffset >= n1 && compare(part, AnMin1, BtoMinN, res) && BtoMinN.valid) {
        ret = BtoMinN;
    }
    if (targetOffset >= n2 && compare(part, BnMin1, AtoMinN, res) && AtoMinN.valid) {
        ret = AtoMinN;
    }

    if (i >= maxBin - 1) {
        ret = error;
    }
    return ret;
}

struct mcData {
    int pass;
    int n;
    bool overflow;
    int index;
    int base;
    int astart;
    int bstart;
    int targetOffset;
};

__DEVICE__ mcData getMCData(int part,mPartitionData pd) {
    mcData ret;
    ret.pass = _fmaxf(0, pd.partitionIndex - 1);
    ret.n = (1 << ret.pass);
    ret.overflow = 2 * ret.n > pd.particlesPerPartition || pd.particlesPerPartition % ret.n != 0;
    ret.index = pd.index - pd.particlesPerPartition;
    ret.base = ret.index - ret.index % (2 * ret.n);
    ret.astart = ret.base;
    ret.bstart = ret.base + ret.n;
    ret.targetOffset = ret.index - ret.base;
    return ret;
}

__DEVICE__ float4 mergeSort(in float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) {
    
    A2F fragColor;
    fragColor.F = to_float4_s(0.0f);
    
    float2 res = maxRes;
    mPartitionData pd = getPartitionData(sortedBuffer, fragCoord, res);

    //fragColor.F.x = texelFetch(sortedBuffer, to_int2(fragCoord), 0).x;

    bool overflow = false;
    for (int part = 0; part < vec4Count; part++) {
        mcData ret = getMCData(PART, pd);
        overflow = overflow || ret.overflow;
      
        //fragColor.A[PART] = binarySearchForMergeSlim( PART, ret.targetOffset, ret.n, ret.n,
        //                                              res, ret.astart, ret.bstart, iResolution,iChannel0).vi[PART];
        
        A2F bSFMS;
        bSFMS.F = binarySearchForMergeSlim( PART, ret.targetOffset, ret.n, ret.n,res, ret.astart, ret.bstart, iResolution,iChannel0).vi;
        
        fragColor.A[PART] = bSFMS.A[PART];

    }
    if (pd.partitionIndex + 1 < pd.partitionCount) {
        fragColor.F.x += (float)(pd.particlesPerPartition);
    }

    if (overflow) {
        //fragColor.x = pd.futureParticle.x;
        fragColor.F.x = 0.0f;
        return fragColor.F;
    }

    if (pd.partitionIndex == 0) {
        fragColor.F.x = (float)(pd.index);
        fragColor.F.x += (float)(pd.particlesPerPartition);
    }

    return fragColor.F;
}





// BEGIN PARTICLES

const float E = 1.0e-10;

__DEVICE__ float2 transformPos(float2 pos) {
    pos = (pos - 0.5f) * 4.0f + 0.5f;
    pos = mod_f2(pos, 1.0f);
    return pos;
}

__DEVICE__ float2 getSpring(float2 res, float4 particle, float2 pos) {
    float2 dv = swi2(particle,x,y) - pos;
    float l = length(dv);
    float k = 0.1f;
    float s = sign_f(k - l);
    float2 dvn = dv / (E + l);
    l = _fminf(_fabs(k - l), l);
    
    float SPRING_COEFF = 1.0e2;
    float SPRING_LENGTH = 0.001f;
    float X = _fabs(SPRING_LENGTH - l);
    float F_spring = SPRING_COEFF * X;
    
    if (l >= SPRING_LENGTH) {
      dv = dvn * SPRING_LENGTH;
    }
        
    float2 a = to_float2_s(0.0f);
    
    // Spring force
    a += -dv * F_spring;

    return a;
}

__DEVICE__ float2 getGravity(float2 res, float4 particle, float2 pos) {
    // Anti-gravity
    float MIN_DIST = 0.01f;
    float G = 5.0e-1;
    float m = 1.0f / (MIN_DIST * MIN_DIST);
    float2 dvg = swi2(particle,x,y) - swi2(pos,x,y); 
    float l2 = length(dvg);
    float2 dvgn = dvg / l2;
    
    float2 a = G * dvg / (MIN_DIST + m * l2 * l2);
    
    return a;
}

__DEVICE__ float4 updateParticle(in float4 particle, float2 a) {
    float2 v = swi2(particle,x,y) - swi2(particle,z,w);
    
    v += a;
    v *= 0.5f;
    
    if (particle.x + v.x < 0.0f || particle.x + v.x >= 1.0f) {
        v.x = -v.x;
        v *= 0.5f;
    }
    if (particle.y + v.y < 0.0f || particle.y + v.y >= 1.0f) {
        v.y = -v.y;
        v *= 0.5f;
    }
    
    float maxSpeed = 0.01f;
    v = length(v) > maxSpeed ? maxSpeed * v / length(v) : v;
    
    //swi2(particle,z,w) = swi2(particle,x,y);
    particle.z = particle.x;
    particle.w = particle.y;
    //swi2(particle,x,y) += v;
    particle.x += v.x;
    particle.y += v.y;
        
    return particle;
}

__DEVICE__ float4 computeParticles(in float2 fragCoord, float2 iResolution, float iTime, int iFrame, bool resetPressed, __TEXTURE2D__ particleBuffer )
{
    float4 fragColor = to_float4_s(0.0f);
    float2 res = maxRes;
    mPartitionData pd = getPartitionData(particleBuffer, fragCoord, res);
    
    if (iFrame == 0 || resetPressed) {
        fragColor = to_float4_s(0.0f);
        
        float2 particle = to_float2_s(0.0f);
        if (pd.partitionIndex == 0) {
            // position
            float2 fc = to_float2_cint(fromLinear(pd.index, res));
            float4 data = hash42(fc);
            particle = transformPos(swi2(data,x,y));
        } else {
            // velocity
            float2 fc = to_float2_cint(fromLinear(pd.futureIndex, res));
            float4 data = hash42(fc);

            float2 pos = transformPos(swi2(data,x,y));
            float2 vel = 10.0f * (swi2(data,z,w) - 0.5f) / res;
            float maxSpeed = 1.0f;
            vel = length(vel) > maxSpeed ? maxSpeed * vel / length(vel) : vel;
            vel = to_float2_s(0.0f);
            float2 oldPos = pos - vel;
            particle = oldPos;
        }

        if (pd.overflow) {
            particle = to_float2_s(0.0f);            
        }
        
        //swi2(fragColor,y,z) = particle;
        fragColor.y = particle.x;
        fragColor.z = particle.y;
        
        return fragColor;
    }
    
    float4 particle1 = to_float4_s(0.0f);
    swi2S(particle1,x,y, getPosition(particleBuffer, pd.index, res, iResolution));
    swi2S(particle1,z,w, getPosition(particleBuffer, pd.pastIndex, res, iResolution));
    
    const int k = 16;
    const int k2 = 4;
    int w = (int)(_sqrtf((float)(k)));
    float2 a1 = to_float2_s(0.0f);
    float2 a2 = to_float2_s(0.0f);
    int torusCount = (int)(_powf(2.0f, (float)((int)(iTime / 4.0f) % 10)));
    int particlesPerTorus = pd.particlesPerPartition / torusCount;
    int wp = int(_sqrtf((float)(particlesPerTorus)));
    int torus = pd.index / particlesPerTorus;
    for (int i = 0; i < k; i++) {
        {
            int index = pd.index % particlesPerTorus;
            float2 fc = to_float2_cint(fromLinear(index, to_float2_s(wp)));
            float2 offset = to_float2(i % w - w / 2, i / w - w / 2);
            if (torus % 3 == 0 && !justSentinels) {
                // Torus
                fc = fc + offset;
                fc = mod_f2f2(fc, to_float2_s(wp));
            } else if (torus % 3 == 1 && !justSentinels) {
                // Cloth
                fc = fc + offset;
                fc = clamp(fc, to_float2_s(0.0f), to_float2_s(wp));
            } else {
                // Sentinel
                offset.x = -1.0f;
                offset.y = 0.0f;
                fc = fc + offset;
                fc = clamp(fc, to_float2_s(0.0f), to_float2_s(wp));
                if (index % wp == 0) {
                    fc = to_float2_s(0.0f);
                }
            }
            int j = toLinear(fc, to_float2_s(wp)) + pd.index - index;
            float2 p2 = getPosition(particleBuffer, j, res, iResolution);
            a1 += getSpring(res, particle1, swi2(p2,x,y)) / (float)(w);
        }
        for (int i2 = 0; i2 < k2; i2++) {
            int w = (int)(_sqrtf((float)(k)));
            int index = pd.index % particlesPerTorus;

            int j = (int)((float)(particlesPerTorus) * 
                    hash(make_uint2(fragCoord.x + (float)(i * k + i2) * (13.0f) * (iFrame), fragCoord.y + (float)(i * k + i2) * (29.0f) * (iFrame))));
                    //hash(make_uint2(fragCoord + (float)(i * k + i2) * to_float2(13.0f, 29.0f) * to_float2_s(iFrame))));
            j += pd.index - index;
            float2 p2 = getPosition(particleBuffer, j, res, iResolution);
            a1 += getGravity(res, particle1, swi2(p2,x,y)) / (float)(w * k2);
        }
    }
    
    float2 updatedParticle = swi2(updateParticle(particle1, a1),x,y);
  
    swi2S(fragColor,y,z, pd.partitionIndex == 0 ? swi2(updatedParticle,x,y) : extractPosition(pd.futureParticle));
    swi2S(fragColor,y,z, pd.overflow ? to_float2_s(0.0f) : swi2(fragColor,y,z));
    
    return fragColor;
}

// END PARTICLES


__DEVICE__ float computeZOrder(in float2 fragCoord, float2 iResolution) {
    float2 res = realRes;
    float2 pres = getRes(res);
    float2 fc = fragCoord / res * pres;
    int index = toIndexFull(fc, pres);
    return float(index);
}

__KERNEL__ void TorusGridAndSentinelFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(resetPressed, 0);

    fragCoord+=0.5f;

    /*
    int maxLinear = toLinear(res - 1.0f, res);
    if (frag
  */
    if (iFrame == 0 || resetPressed) {
        //swi3(fragColor,x,y,z) = to_float3_s(0.0f);
        //fragColor.w = computeZOrder(fragCoord, iResolution);
        
        fragColor = to_float4(0.0f,0.0f,0.0f, computeZOrder(fragCoord, iResolution));
        
    } else {
        //fragColor.w = texelFetch(mortonBuffer, to_int2(fragCoord), 0).w;
        fragColor.w = texture(mortonBuffer, fragCoord/iResolution).w;
    }
    float2 res = maxRes;
    if (fragCoord.x >= res.x || fragCoord.y >= res.y) {
        //discard;
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    fragColor.x = mergeSort(fragCoord, iResolution, iChannel0).x;
    swi2S(fragColor,y,z, swi2(computeParticles(fragCoord,iResolution,iTime,iFrame,resetPressed, particleBuffer),y,z));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__DEVICE__ int binarySearchLeftMost(int part, int T, float2 res, float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) {
    mPartitionData pd = getPartitionData(sortedBuffer, fragCoord, res);
    int n = pd.particlesPerPartition;
    int maxPartition = getMaxPartition(pd);
    int L = maxPartition * n;
    int R = L + n;

    int i = 0;
    for (i = 0; i < maxBin && L < R; i++) {
        int m = (L + R) / 2;
        int Am = getM(part, m, res).Am;
        L = Am < T ? m + 1 : L;
        R = Am >= T ? m : R;
    }
    int ret = i < maxBin - 1 ? L : -1;
    return ret;
}

__DEVICE__ int binarySearchRightMost(int part, int T, float2 res, float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) {
    mPartitionData pd = getPartitionData(sortedBuffer, fragCoord, res);
    int n = pd.particlesPerPartition;
    int maxPartition = getMaxPartition(pd);
    int L = maxPartition * n;
    int R = L + n;

    int i = 0;
    for (i = 0; i < maxBin && L < R; i++) {
        int m = (L + R) / 2;
        int Am = getM(part, m, res).Am;
        L = Am <= T ? m + 1 : L;
        R = Am > T ? m : R;
    }
    int ret = i < maxBin - 1 ? L - 1 : -1;
    return ret;
}

__DEVICE__ float doDistance(int part, in float2 fragCoord, float2 colorUV, float2 iResolution, __TEXTURE2D__ iChannel0) {
    float2 res = maxRes;
    //vec2 oc = fragCoord / realRes * res;
    float2 oc = fragCoord;

    int uvIndex = toIndex(colorUV);
    int index3 = binarySearchLeftMost(part, uvIndex, res, oc, iResolution, iChannel0);
    int index4 = binarySearchRightMost(part, uvIndex, res, oc, iResolution, iChannel0);

    mRet mret = getM(part, index3, res);
    int foundIndex = mret.Am;
    float4 v = mret.v;
    float d = distance_f2(colorUV, mret.pos);

    int j = 0;
    int a = _fminf(index3, index4);
    int b = _fmaxf(index3, index4);
    int maxIter = 10;
    int retIndex = -1;
    for (int j = 0; j < maxIter; j++) {
        int i = a + j - maxIter / 2;
        mRet mret = getM(part, i, res);
        int foundIndex = mret.Am;
        float4 v = mret.v;
        float d2 = distance_f2(colorUV, mret.pos);
        if (d2 < d) {
            d = d2;
            retIndex = i;
        }
    }
    return float(retIndex);
}



__KERNEL__ void TorusGridAndSentinelFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution)
{

    fragCoord+=0.5f;

    //fragCoord = _floor(fragCoord / iResolution * maxRes);
    
    float2 res = maxRes;
    // TODO: try +0.5
    float2 colorUV = (fragCoord + 0.0f) / realRes;
    
    //float4 old = texelFetch(pixelNearestBuffer, to_int2(fragCoord), 0);
    A2F old; 
    old.F = texture(pixelNearestBuffer, fragCoord/iResolution);

    A2F _fragColor;
    _fragColor.F = fragColor;

    for (int part = 0; part < vec4Count; part++) {
        float oldIndex = old.A[part];

        mRet mret1 = getM(part, (int)(oldIndex), res);
        float d2 = distance_f2(colorUV, mret1.pos);
        float index = doDistance(part, fragCoord, colorUV, iResolution, iChannel0);
        mRet mret2 = getM(part, (int)(index), res);
        float d3 = distance_f2(colorUV, mret2.pos);
        index = d3 < d2 ? index : oldIndex;

        _fragColor.A[PART] = index;
    }


  SetFragmentShaderComputedColor(_fragColor.F);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


// Fork of "Z Particle Sort Pipeline" by emh. https://shadertoy.com/view/Mtdyzs
// 2018-08-09 11:43:19

__DEVICE__ void lookup(out float4 *fragColor, in float2 fragCoord, float2 iResolution, __TEXTURE2D__ pixelNearestBuffer, __TEXTURE2D__ iChannel0) {
    float2 res = maxRes;
    int k = 1;
    float mul = 1.0f;
    const float MAX = 1.0e10;
    float rd = 0.0f;
    float mind = MAX;
    float2 colorUV = (fragCoord + 0.0f) / realRes;
    float3 color = to_float3_s(1.0f);
    int minIndex = -1;
    bool firstHalf = true;
    mPartitionData pd = getPartitionData(particleBuffer, fragCoord, res);

    //float4 indexAndVelNearest = texelFetch(pixelNearestBuffer, to_int2(fragCoord), 0);
    float4 indexAndVelNearest = texture(pixelNearestBuffer, fragCoord/iResolution);

    for (int dx = -k; dx <= k; dx++) {
        for (int dy = -k; dy <= k; dy++) {
            float2 delta = to_float2(dx, dy);
            //vec2 delta2 = to_float2(sign_f((float)(dx)) * _exp2f(_fabs(float(dx))), sign_f((float)(dy)) * _exp2f(_fabs(float(dy))));

            int2 fc = to_int2_cfloat(fragCoord + mul * delta);
            //float4 indexAndVel = texelFetch(pixelNearestBuffer, fc, 0);
            //float4 indexAndVel = texture(pixelNearestBuffer, (make_float2(fc)+0.5f)/iResolution);
            A2F indexAndVel;
            indexAndVel.F = texture(pixelNearestBuffer, (make_float2(fc)+0.5f)/iResolution);


            for (int part = 0; part < vec4Count; part++) {
                int i = (int)(indexAndVel.A[part]);
                mRet iret = getM(part, i, res);
                float2 newPos = iret.pos;
                //vec2 newPos = to_float2(fc) / realRes;
                float d = distance_f2(colorUV, newPos);
                if (i >= 0 && d < mind) {
                    minIndex = iret.dIndex;
                    firstHalf = part == 0;
                }
                //mind = i < 0 ? mind : _fminf(d, mind);
                mind = _fminf(d, mind);
                //float f = 0.00005f  / d;
                float f = d;
                rd = i < 0 ? rd : (d < ((float)(k) / realRes.x) ? f + rd : rd);
                if (i >= 0 && (d < ((float)(k) / realRes.x))) {
                    float h = (float)(iret.dIndex % pd.particlesPerPartition) / (float)(pd.particlesPerPartition);
                    color = hsv2rgb(to_float3(h, 1.0f, 1.0f));
                    color = _mix(to_float3_s(1.0f), color, d * iResolution.x / 10.0f);
                  //*fragColor += clamp(0.01f * to_float4_aw(color, 1.0f) * to_float4(1.0f / (d * realRes.x)), 0.0f, 1.0f);
                }
            }
        }
    }

    float h = (float)(minIndex % pd.particlesPerPartition) / (float)(pd.particlesPerPartition);
    color = hsv2rgb(to_float3(h, 1.0f, 1.0f));
    color = _mix(to_float3_s(1.0f), color, 100.0f * mind);

    float size = minIndex >= 0 ? (float)(minIndex % 10 + 1) : 1.0f;

    float brightness = 1.0f;
    //*fragColor += clamp(brightness * to_float4_aw(color, 1.0f) * to_float4(1.0f / (mind * 1000.0f)), 0.0f, 1.0f);
    *fragColor += clamp(brightness * to_float4_aw(color, 1.0f) * to_float4_s(1.0f / (mind * realRes.x)), 0.0f, 1.0f);
    //*fragColor += clamp(brightness * to_float4_aw(color, 1.0f) * to_float4(1.0f / (rd * realRes.x)), 0.0f, 1.0f);
    //*fragColor = to_float4_aw(1.0f * rd);
}

__DEVICE__ void debug(out float4 *fragColor, in float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3) {
    //float4 v0 = texelFetch(iChannel0, to_int2(fragCoord), 0);
    //float4 v1 = texelFetch(iChannel1, to_int2(fragCoord), 0);
    //float4 v2 = texelFetch(iChannel2, to_int2(fragCoord), 0);
    //float4 v3 = texelFetch(iChannel3, to_int2(fragCoord), 0);

    float4 v0 = texture(iChannel0, fragCoord/iResolution);
    float4 v1 = texture(iChannel1, fragCoord/iResolution);
    float4 v2 = texture(iChannel2, fragCoord/iResolution);
    float4 v3 = texture(iChannel3, fragCoord/iResolution);

    
    //float val = float(int(v.w) % 1000) / 1000.0f;
    float val = (float)((int)(v0.x) % 10000) / 10000.0f;
    *fragColor = to_float4_s(val);
    //fragColor.rb = swi2(v0,y,z);
}

__KERNEL__ void TorusGridAndSentinelFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    fragCoord+=0.5f;

    //debug(fragColor, fragCoord);
    lookup(&fragColor, fragCoord, iResolution, pixelNearestBuffer, iChannel0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// horizontal Gaussian blur pass

__KERNEL__ void TorusGridAndSentinelFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  fragCoord+=0.5f;

  float2 pixelSize = 1.0f/ iResolution;
  float2 uv = fragCoord * pixelSize;
   
  float h = pixelSize.x;
  float4 sum = to_float4_s(0.0f);
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 4.0f*h, uv.y)) ) * 0.05f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 3.0f*h, uv.y)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 2.0f*h, uv.y)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x - 1.0f*h, uv.y)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 0.0f*h, uv.y)) ) * 0.16f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 1.0f*h, uv.y)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 2.0f*h, uv.y)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 3.0f*h, uv.y)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x + 4.0f*h, uv.y)) ) * 0.05f;
   
  fragColor = to_float4_aw( swi3(sum,x,y,z)/0.98f, 1.0f);
  //fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


// vertical Gaussian blur pass

__KERNEL__ void TorusGridAndSentinelFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(Sum, -1.0f, 2.0f, 0.98f);
  CONNECT_SLIDER1(Lookup, -1.0f, 5.0f, 2.0f);
  CONNECT_SLIDER2(AlphaThreshold, 0.0f, 1.0f, 0.5f);

  float2 pixelSize = 1.0f/ iResolution;
  float2 uv = fragCoord * pixelSize;

  float v = pixelSize.y;
  float4 sum = to_float4_s(0.0f);
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 4.0f*v)) ) * 0.05f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y - 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 0.0f*v)) ) * 0.16f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 1.0f*v)) ) * 0.15f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 2.0f*v)) ) * 0.12f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 3.0f*v)) ) * 0.09f;
  sum += texture(iChannel0, fract_f2(to_float2(uv.x, uv.y + 4.0f*v)) ) * 0.05f;
    
  fragColor = to_float4_aw( swi3(sum,x,y,z)/Sum, 1.0f);  
  fragColor = (fragColor + _tex2DVecN(iChannel1,uv.x,uv.y,15)) / Lookup;

  if (AlphaThreshold)
    if(fragColor.w > AlphaThreshold) fragColor.w = Color.w;
    else                             fragColor.w = 0.0f; 

  fragColor = to_float4_aw(swi3(fragColor,x,y,z) + (swi3(Color,x,y,z)-0.5f)*fragColor.w, fragColor.w);
  
  SetFragmentShaderComputedColor(fragColor);
}