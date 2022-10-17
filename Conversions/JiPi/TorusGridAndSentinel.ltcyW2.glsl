

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// vertical Gaussian blur pass

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize = 1./ iChannelResolution[0].xy;
    vec2 uv = fragCoord.xy * pixelSize;

    float v = pixelSize.y;
	vec4 sum = vec4(0.0);
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 4.0*v)) ) * 0.05;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 3.0*v)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 2.0*v)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y - 1.0*v)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 0.0*v)) ) * 0.16;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 1.0*v)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 2.0*v)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 3.0*v)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x, uv.y + 4.0*v)) ) * 0.05;
    
    fragColor.xyz = sum.xyz/0.98; // normalize
	fragColor.a = 1.;
    
    fragColor = (fragColor + texture(iChannel1, uv)) / 2.0;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
precision highp float;
precision highp int;

#define mortonBuffer iChannel0
#define sortedBuffer iChannel0
#define particleBuffer iChannel0
#define pixelNearestBuffer iChannel1

//#define maxRes min(vec2(800.0, 450.0), iResolution.xy)
#define maxRes min(vec2(512.0, 512.0), iResolution.xy)
//#define maxRes min(vec2(128.0, 128.0), iResolution.xy)
//#define maxRes min(vec2(512.0, 256.0), iResolution.xy)
//#define maxRes min(vec2(iResolution.x, 256.0), iResolution.xy)
//#define maxRes min(vec2(512.0, iResolution.y), iResolution.xy)
//#define maxRes iResolution.xy
#define realRes iResolution.xy
#define powerOfTwoRes vec2(2048.0, 2048.0)
//#define realRes maxRes
//#define maxRes iResolution.xy

// Try this true for more Matrix fun :)
const bool justSentinels = false;

// number of particles will be 2^magicNumberDoNotChange = 64k
// I haven't figured out why it seems to work only when this number is 16
const int magicNumberDoNotChange = 16;
const int MAX_ITER = 12;
const int maxBin = 32;
const int vec4Count = 1;
#define PART part
const float M_PI = 3.14159265358979323846264338327950288;

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

int getMaxPasses2(vec2 res) {
    return int(ceil(log2(res.x * res.y)));
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
    ivec2 futureCoord;
    vec4 futureParticle;
    bool overflow;
};
    
vec2 extractPosition(vec4 data) {
    return data.yz;
}

// BEGIN QUALITY HASHES

uint baseHash(uvec2 p)
{
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}


//---------------------2D input---------------------

float hash12(uvec2 x)
{
    uint n = baseHash(x);
    return float(n)*(1.0/float(0xffffffffU));
}

vec2 hash22(uvec2 x)
{
    uint n = baseHash(x);
    uvec2 rz = uvec2(n, n*48271U);
    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);
}

vec3 hash32(uvec2 x)
{
    uint n = baseHash(x);
    uvec3 rz = uvec3(n, n*16807U, n*48271U);
    return vec3(rz & uvec3(0x7fffffffU))/float(0x7fffffff);
}

vec4 hash42(uvec2 x)
{
    uint n = baseHash(x);
    uvec4 rz = uvec4(n, n*16807U, n*48271U, n*69621U); //see: http://random.mat.sbg.ac.at/results/karl/server/node4.html
    return vec4(rz & uvec4(0x7fffffffU))/float(0x7fffffff);
}

//--------------------------------------------------


//Example taking an arbitrary float value as input
/*
	This is only possible since the hash quality is high enough so that
	floored float input doesn't break the process when the raw bits are used
*/
vec4 hash42(vec2 x)
{
    uint n = baseHash(floatBitsToUint(x));
    uvec4 rz = uvec4(n, n*16807U, n*48271U, n*69621U);
    return vec4(rz & uvec4(0x7fffffffU))/float(0x7fffffff);
}

// END QUALITY HASHES


float hash( uvec2 x )
{
    uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return float(n) * (1.0/float(0xffffffffU));
}

vec2 getRes(vec2 res) {
    //return vec2(exp2(ceil(log2(max(res.x, res.y)))));
    return powerOfTwoRes;
}

int toIndexCol(in vec2 fragCoord, in vec2 resolution, inout vec3 col) {
    int xl = int(fragCoord.x);
    int yl = int(fragCoord.y);
    ivec2 res = ivec2(resolution);
    int div2 = 1;
    /*
    for (int i = 0; i < MAX_ITER; i++) {
        res /= 2;
        div2 *= 2;
        if (res.x == 0 && res.y == 0) break;
    }
    res = ivec2(div2);
	*/
    int index = 0;
    int div = 1;
    div2 = 1;
    bool colorDone = false;
    for (int i = 0; i < MAX_ITER; i++) {
        ivec2 rest = res % 2;
        res /= 2;
        if (res.x == 0 && res.y == 0) break;
        div *= 4;
        div2 *= 2;
        int x = int(xl >= res.x);
        int y = int(yl >= res.y);
        xl -= x * res.x;
        yl -= y * res.y;
        //res += x * rest.x;
        //res += y * rest.y;
        int thisIndex = y * 2 + x;
        index = index * 4 + thisIndex;

        if (!colorDone) {
            vec2 uv = vec2(xl, yl) / vec2(res);
            vec2 center = vec2(0.5);
            float d = distance(uv, center);
            float r = float(d < 0.25);
            bool border = d > 0.25 - 0.02 / float(div2) && d < 0.25;
            if (border) {
                colorDone = true;
            } else {
            	col = vec3(float(int(col) ^ int(r)));
            }
        }
    }
    //return res.x * res.y - index - 1;
    return index;
}

int toIndexFull(in vec2 fragCoord, in vec2 resolution) {
    vec3 col = vec3(0.0);
    int index = toIndexCol(fragCoord, resolution, col);
    //index += 1;
    return index;
}

ivec2 fromIndexFull(in int index, in vec2 resolution) {
    //index -= 1;
    ivec2 fc = ivec2(0);
    int div = 1;
    ivec2 div2 = ivec2(1);
    ivec2 res = ivec2(resolution);
    //index = res.x * res.y - index - 1;
    for (int i = 0; i < MAX_ITER; i++) {
        res /= 2;
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

ivec2 fromLinear(in int index, in vec2 resolution) {
    //index -= 1;
    return ivec2(index % int(resolution.x), index / int(resolution.x));
}

int toLinear(in vec2 fragCoord, in vec2 resolution) {
    int index = int(fragCoord.x) + int(fragCoord.y) * int(resolution.x);
    //index += 1;
    return index;
}

#define toIndex(a) toIndex2(mortonBuffer, a, realRes)
int toIndex2(in sampler2D channel, in vec2 fragCoord, in vec2 res) {
    ivec2 fc = ivec2(fragCoord * res);
    vec4 index = texelFetch(channel, fc, 0);
    return int(index.w);
}

vec2 getPosition(sampler2D channel, int index, vec2 res) {
    ivec2 fc = fromLinear(index, res);
    vec4 data = texelFetch(channel, fc, 0);
    return fract(extractPosition(data));
}

int maxLinear(vec2 res) {
    return int(exp2(floor(log2(float(toLinear(res - 1.0, res))))));
}

bool isLinearValid(in int index, vec2 iResolution) {
    vec2 res = iResolution.xy;
    //return true;
	return index < maxLinear(iResolution);
}

bool isValid(in vec2 fragCoord, vec2 iResolution) {
    vec2 res = iResolution.xy;
    return isLinearValid(toLinear(fragCoord, res), iResolution);
}

#define getPartitionData(a, b, c) getPartitionData2(a, b, c, realRes)
mPartitionData getPartitionData2(sampler2D channel, vec2 fragCoord, vec2 res, vec2 rRes) {
    //fragCoord = fragCoord / rRes * res;
    mPartitionData mRet;
    //int maxPasses = getMaxPasses(res);
    //mRet.partitionCount = int(exp2(ceil(log2(float(maxPasses)))));
    mRet.partitionCount = magicNumberDoNotChange;
    //mRet.maxIndex = toLinear(res - 1.0, res);
    mRet.maxIndex = maxLinear(res);
    mRet.particlesPerPartition = mRet.maxIndex / mRet.partitionCount;
    mRet.index = toLinear(fragCoord, res);
    mRet.partitionIndex = mRet.index / mRet.particlesPerPartition;
    mRet.offset = mRet.index % mRet.particlesPerPartition;
    mRet.futureIndex = mRet.index - mRet.particlesPerPartition;
    mRet.futureCoord = fromLinear(mRet.futureIndex, res);
    mRet.futureParticle = texelFetch(channel, mRet.futureCoord, 0);
    mRet.pastIndex = mRet.index + mRet.particlesPerPartition;
    mRet.overflow = mRet.index >= mRet.maxIndex;
    
    //(mRet.partitionIndex - 1) * mRet.particlesPerPartition + mRet.offset;


    return mRet;
}

int getMaxPartition(mPartitionData pd) {
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
    vec4 vi;
    vec4 v;
    vec2 pos;
    bool valid;
};

#define getMD(a, b, c) getMD2(particleBuffer, mortonBuffer, a, b, c, realRes)
mRet getMD2(sampler2D channel, sampler2D mchannel, int part, int m, vec2 res, vec2 rRes) {
    vec2 fc = vec2(fromLinear(m, res));
    vec4 v = texelFetch(channel, ivec2(fc), 0);
    vec2 pos = extractPosition(v);
    int Am = toIndex2(mchannel, pos, rRes);
    int maxIndex = toLinear(res - 1.0, res);
    bool valid = m >= 0 && m <= maxIndex && isLinearValid(m, res);
    //valid = true;
    return mRet(m, Am, vec4(0.0), v, pos, valid);
}

#define getM(a, b, c) getM2(sortedBuffer, particleBuffer, mortonBuffer, a, b, c, realRes)
mRet getM2(sampler2D channel, sampler2D pchannel, sampler2D mchannel, int part, int m, vec2 res, vec2 rRes) {
    vec2 fc = vec2(fromLinear(m, res));
    vec4 v = texelFetch(channel, ivec2(fc), 0);
    mRet ret2 = getMD2(pchannel, mchannel, part, int(v[part]), res, rRes);
    int maxIndex = toLinear(res - 1.0, res);
    bool valid = m >= 0 && m <= maxIndex && isLinearValid(m, res);
    //valid = true;
    return mRet(int(v[part]), ret2.Am, v, ret2.v, ret2.pos, valid && ret2.valid);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Resources:
// https://www.ics.uci.edu/~goodrich/pubs/skip-journal.pdf
// Sorting with GPUs: A Survey: https://arxiv.org/pdf/1709.02520.pdf

// Practice JavaScript implementation: http://jsbin.com/zeyiraw/

// https://www.shadertoy.com/view/XlcGD8
// https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter46.html
// https://stackoverflow.com/questions/26093629/glsl-odd-even-merge-sort
// https://bl.ocks.org/zz85/cafa1b8b3098b5a40e918487422d47f6

#define resetPressed (texelFetch(iChannel1, ivec2(KEY_LEFT,1),0 ).x > 0.5)

const int KEY_LEFT  = 37;
const int KEY_UP    = 38;
const int KEY_RIGHT = 39;
const int KEY_DOWN  = 40;

int extractIndex(vec4 v, int part) {
    return int(v[PART]);
}

int getIndex(int part, mRet A, vec2 res) {	
    return A.Am;
}

bool compare(int part, mRet A, mRet B, vec2 res) {
    return getIndex(part, A, res) < getIndex(part, B, res);
}

bool cutValid(int part, int n1, int n2, int astart, int bstart, int to, int m2, int x, vec2 res) {
    int apos = m2 - 1;
    bool aValid = apos >= 0 && apos < n1;
    int bpos = to - m2 - 1;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);

    int cv11 = getIndex(part, Amret, res);
    int cv12 = getIndex(part, Bmret, res);
    return (
        aValid && bValid && apos >= 0 && bpos >= 0 ? max(cv11, cv12) <= x
        : bValid && apos < 0 && bpos >= 0 ? cv12 <= x
        : aValid && apos >= 0 && bpos < 0 ? cv11 <= x
        : Amret.valid && Bmret.valid);
}

bool cutCValid(int part, int n1, int n2, int astart, int bstart, int to, int bm2, int x, vec2 res) {
    int apos = to - bm2 - 1;
    int bpos = bm2 - 1;
    bool aValid = apos >= 0 && apos < n1;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);
	int cvc11 = getIndex(part, Amret, res);
    int cvc12 = getIndex(part, Bmret, res);
    return (
    	aValid && bValid && apos >= 0 && bpos >= 0 ? max(cvc11, cvc12) <= x
        : bValid && apos < 0 && bpos >= 0 ? cvc12 <= x
        : aValid && apos >= 0 && bpos < 0 ? cvc11 <= x
        : Amret.valid && Bmret.valid);
}

mRet checkIndex(int part, int n1, int n2, int astart, int bstart, int to, int apos, vec2 res) {
    bool aValid = apos >= 0 && apos < n1;
    int bpos = to - apos;
    bool bValid = bpos >= 0 && bpos < n2;

    mRet Amret = getM(part, astart + apos, res);
    mRet Bmret = getM(part, bstart + bpos, res);

    int candA = getIndex(part, Amret, res);
    bool candAv = cutValid(part, n1, n2, astart, bstart, to, apos, candA, res) && aValid;
    Amret.valid = Amret.valid && candAv;

    int candB = getIndex(part, Bmret, res);
    bool candBv = cutCValid(part, n1, n2, astart, bstart, to, bpos, candB, res) && bValid;
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

mRet binarySearchForMergeSlim(
    int part,
    int targetOffset, int n1, int n2, vec2 res,
    int astart, int bstart) {

    int L1 = min(max(targetOffset + 1 - n1, 0), n1 - 1);
    int R1 = min(targetOffset + 1, n1);
    int L2 = min(max(targetOffset + 1 - n2, 0), n2 - 1);
    int R2 = min(targetOffset + 1, n2);

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
    mRet error = mRet(-1, -1, vec4(-1.0), vec4(-1.0), vec2(-1.0), false);
    //mRet error = mRet(-1, vec4(-1.0), false);

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

    mRet cand1 = checkIndex(part, n1, n2, astart, bstart, to, m2, res);
    mRet cand2 = checkIndex(part, n1, n2, astart, bstart, to, bm2, res);
    cand2.valid = cand2.valid && bm2Valid;
    mRet cand3 = checkIndex(part, n1, n2, astart, bstart, to, m2 - 1, res);
    mRet cand4 = checkIndex(part, n1, n2, astart, bstart, to, bm2 - 1, res);
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

mcData getMCData(int part,mPartitionData pd) {
    mcData ret;
    ret.pass = max(0, pd.partitionIndex - 1);
    ret.n = (1 << ret.pass);
    ret.overflow = 2 * ret.n > pd.particlesPerPartition || pd.particlesPerPartition % ret.n != 0;
    ret.index = pd.index - pd.particlesPerPartition;
    ret.base = ret.index - ret.index % (2 * ret.n);
    ret.astart = ret.base;
    ret.bstart = ret.base + ret.n;
    ret.targetOffset = ret.index - ret.base;
    return ret;
}

vec4 mergeSort(in vec2 fragCoord) {
    vec4 fragColor = vec4(0.0);
    vec2 res = maxRes;
    mPartitionData pd = getPartitionData(sortedBuffer, fragCoord, res);

    //fragColor.x = texelFetch(sortedBuffer, ivec2(fragCoord), 0).x;

    bool overflow = false;
    for (int part = 0; part < vec4Count; part++) {
        mcData ret = getMCData(PART, pd);
    	overflow = overflow || ret.overflow;
        fragColor[PART] = binarySearchForMergeSlim(
            PART, ret.targetOffset, ret.n, ret.n,
            res, ret.astart, ret.bstart).vi[PART];
    }
    if (pd.partitionIndex + 1 < pd.partitionCount) {
        fragColor.x += float(pd.particlesPerPartition);
    }

    if (overflow) {
        //fragColor.x = pd.futureParticle.x;
        fragColor.x = 0.0;
        return fragColor;
    }

    if (pd.partitionIndex == 0) {
        fragColor.x = float(pd.index);
        fragColor.x += float(pd.particlesPerPartition);
    }

    return fragColor;
}





// BEGIN PARTICLES

const float E = 1.0e-10;

vec2 transformPos(vec2 pos) {
    pos = (pos - 0.5) * 4.0 + 0.5;
    pos = mod(pos, 1.0);
    return pos;
}

vec2 getSpring(vec2 res, vec4 particle, vec2 pos) {
    vec2 dv = particle.xy - pos;
    float l = length(dv);
    float k = 0.1;
    float s = sign(k - l);
    vec2 dvn = dv / (E + l);
    l = min(abs(k - l), l);
    
    float SPRING_COEFF = 1.0e2;
    float SPRING_LENGTH = 0.001;
    float X = abs(SPRING_LENGTH - l);
    float F_spring = SPRING_COEFF * X;
    
    if (l >= SPRING_LENGTH) {
    	dv = dvn * SPRING_LENGTH;
    }
    
    
    vec2 a = vec2(0.0);
    
    // Spring force
    a += -dv * F_spring;
    
    return a;
}

vec2 getGravity(vec2 res, vec4 particle, vec2 pos) {
    // Anti-gravity
    float MIN_DIST = 0.01;
    float G = 5.0e-1;
    float m = 1.0 / (MIN_DIST * MIN_DIST);
    vec2 dvg = particle.xy - pos.xy; 
    float l2 = length(dvg);
    vec2 dvgn = dvg / l2;
    
    vec2 a = G * dvg / (MIN_DIST + m * l2 * l2);
    
    return a;
}

vec4 updateParticle(in vec4 particle, vec2 a) {
    vec2 v = particle.xy - particle.zw;
    
    v += a;
    v *= 0.5;
    
    if (particle.x + v.x < 0.0 || particle.x + v.x >= 1.0) {
        v.x = -v.x;
        v *= 0.5;
    }
    if (particle.y + v.y < 0.0 || particle.y + v.y >= 1.0) {
        v.y = -v.y;
        v *= 0.5;
    }
    
    float maxSpeed = 0.01;
    v = length(v) > maxSpeed ? maxSpeed * v / length(v) : v;
    
    particle.zw = particle.xy;
    particle.xy += v;
        
    return particle;
}

vec4 computeParticles(in vec2 fragCoord )
{
    vec4 fragColor = vec4(0.0);
    vec2 res = maxRes;
    mPartitionData pd = getPartitionData(particleBuffer, fragCoord, res);
    
    if (iFrame == 0 || resetPressed) {
        fragColor = vec4(0.0);
        
        vec2 particle = vec2(0.0);
        if (pd.partitionIndex == 0) {
            // position
            vec2 fc = vec2(fromLinear(pd.index, res));
            vec4 data = hash42(fc);
            particle = transformPos(data.xy);
        } else {
            // velocity
            vec2 fc = vec2(fromLinear(pd.futureIndex, res));
            vec4 data = hash42(fc);

            vec2 pos = transformPos(data.xy);
            vec2 vel = 10.0 * (data.zw - 0.5) / res;
            float maxSpeed = 1.0;
            vel = length(vel) > maxSpeed ? maxSpeed * vel / length(vel) : vel;
            vel = vec2(0.0);
            vec2 oldPos = pos - vel;
            particle = oldPos;
        }

        if (pd.overflow) {
            particle = vec2(0.0);            
        }
        
        fragColor.yz = particle;
        
        return fragColor;
    }
    
    vec4 particle1 = vec4(0.0);
    particle1.xy = getPosition(particleBuffer, pd.index, res);
    particle1.zw = getPosition(particleBuffer, pd.pastIndex, res);
    
    const int k = 16;
    const int k2 = 4;
    int w = int(sqrt(float(k)));
    vec2 a1 = vec2(0.0);
    vec2 a2 = vec2(0.0);
    int torusCount = int(pow(2.0, float(int(iTime / 4.0) % 10)));
    int particlesPerTorus = pd.particlesPerPartition / torusCount;
    int wp = int(sqrt(float(particlesPerTorus)));
    int torus = pd.index / particlesPerTorus;
    for (int i = 0; i < k; i++) {
        {
            int index = pd.index % particlesPerTorus;
            vec2 fc = vec2(fromLinear(index, vec2(wp)));
            vec2 offset = vec2(i % w - w / 2, i / w - w / 2);
            if (torus % 3 == 0 && !justSentinels) {
                // Torus
                fc = fc + offset;
            	fc = mod(fc, vec2(wp));
            } else if (torus % 3 == 1 && !justSentinels) {
                // Cloth
                fc = fc + offset;
            	fc = clamp(fc, vec2(0.0), vec2(wp));
            } else {
                // Sentinel
                offset.x = -1.0;
                offset.y = 0.0;
                fc = fc + offset;
                fc = clamp(fc, vec2(0.0), vec2(wp));
                if (index % wp == 0) {
                    fc = vec2(0.0);
                }
            }
            int j = toLinear(fc, vec2(wp)) + pd.index - index;
            vec2 p2 = getPosition(particleBuffer, j, res);
            a1 += getSpring(res, particle1, p2.xy) / float(w);
        }
        for (int i2 = 0; i2 < k2; i2++) {
            int w = int(sqrt(float(k)));
            int index = pd.index % particlesPerTorus;
            int j =
                int(float(particlesPerTorus) * 
                    hash(uvec2(fragCoord + float(i * k + i2) * vec2(13.0, 29.0) * vec2(iFrame))));
            j += pd.index - index;
            vec2 p2 = getPosition(particleBuffer, j, res);
            a1 += getGravity(res, particle1, p2.xy) / float(w * k2);
        }
    }
    
    vec2 updatedParticle = updateParticle(particle1, a1).xy;
	
    fragColor.yz = pd.partitionIndex == 0 ? updatedParticle.xy : extractPosition(pd.futureParticle);
    fragColor.yz = pd.overflow ? vec2(0.0) : fragColor.yz;
    
    return fragColor;
}

// END PARTICLES


float computeZOrder(in vec2 fragCoord) {
    vec2 res = realRes;
    vec2 pres = getRes(res);
    vec2 fc = fragCoord / res * pres;
    int index = toIndexFull(fc, pres);
    return float(index);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    /*
    int maxLinear = toLinear(res - 1.0, res);
    if (frag
	*/
    if (iFrame == 0 || resetPressed) {
        fragColor.xyz = vec3(0.0);
        fragColor.w = computeZOrder(fragCoord);
    } else {
        fragColor.w = texelFetch(mortonBuffer, ivec2(fragCoord), 0).w;
    }
    vec2 res = maxRes;
    if (fragCoord.x >= res.x || fragCoord.y >= res.y) {
        //discard;
        return;
    }
    fragColor.x = mergeSort(fragCoord).x;
    fragColor.yz = computeParticles(fragCoord).yz;
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
int binarySearchLeftMost(int part, int T, vec2 res, vec2 fragCoord) {
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

int binarySearchRightMost(int part, int T, vec2 res, vec2 fragCoord) {
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

float doDistance(int part, in vec2 fragCoord, vec2 colorUV) {
    vec2 res = maxRes;
    //vec2 oc = fragCoord / realRes * res;
    vec2 oc = fragCoord;

    int uvIndex = toIndex(colorUV);
    int index3 = binarySearchLeftMost(part, uvIndex, res, oc);
    int index4 = binarySearchRightMost(part, uvIndex, res, oc);

    mRet mret = getM(part, index3, res);
    int foundIndex = mret.Am;
    vec4 v = mret.v;
    float d = distance(colorUV, mret.pos);

    int j = 0;
    int a = min(index3, index4);
    int b = max(index3, index4);
    int maxIter = 10;
    int retIndex = -1;
    for (int j = 0; j < maxIter; j++) {
        int i = a + j - maxIter / 2;
        mRet mret = getM(part, i, res);
        int foundIndex = mret.Am;
        vec4 v = mret.v;
        float d2 = distance(colorUV, mret.pos);
        if (d2 < d) {
            d = d2;
            retIndex = i;
        }
    }

    return float(retIndex);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    //fragCoord = floor(fragCoord / iResolution.xy * maxRes);
    
    vec2 res = maxRes;
    // TODO: try +0.5
    vec2 colorUV = (fragCoord + 0.0) / realRes;
    
	vec4 old = texelFetch(pixelNearestBuffer, ivec2(fragCoord), 0);

    for (int part = 0; part < vec4Count; part++) {
    	float oldIndex = old[part];

        mRet mret1 = getM(part, int(oldIndex), res);
        float d2 = distance(colorUV, mret1.pos);

        float index = doDistance(part, fragCoord, colorUV);

        mRet mret2 = getM(part, int(index), res);

        float d3 = distance(colorUV, mret2.pos);

        index = d3 < d2 ? index : oldIndex;

        fragColor[PART] = index;
    }
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Fork of "Z Particle Sort Pipeline" by emh. https://shadertoy.com/view/Mtdyzs
// 2018-08-09 11:43:19

void lookup(out vec4 fragColor, in vec2 fragCoord) {
	vec2 res = maxRes;
    int k = 1;
    float mul = 1.0;
    const float MAX = 1.0e10;
    float rd = 0.0;
    float mind = MAX;
    vec2 colorUV = (fragCoord + 0.0) / realRes;
    vec3 color = vec3(1.0);
    int minIndex = -1;
    bool firstHalf = true;
    mPartitionData pd = getPartitionData(particleBuffer, fragCoord, res);

    vec4 indexAndVelNearest = texelFetch(pixelNearestBuffer, ivec2(fragCoord), 0);

    for (int dx = -k; dx <= k; dx++) {
        for (int dy = -k; dy <= k; dy++) {
            vec2 delta = vec2(dx, dy);
            //vec2 delta2 = vec2(sign(float(dx)) * exp2(abs(float(dx))), sign(float(dy)) * exp2(abs(float(dy))));

            ivec2 fc = ivec2(fragCoord + mul * delta);
            vec4 indexAndVel = texelFetch(pixelNearestBuffer, fc, 0);

            for (int part = 0; part < vec4Count; part++) {
                int i = int(indexAndVel[part]);
                mRet iret = getM(part, i, res);
                vec2 newPos = iret.pos;
                //vec2 newPos = vec2(fc) / realRes;
                float d = distance(colorUV, newPos);
                if (i >= 0 && d < mind) {
                    minIndex = iret.dIndex;
                    firstHalf = part == 0;
                }
                //mind = i < 0 ? mind : min(d, mind);
                mind = min(d, mind);
                //float f = 0.00005  / d;
                float f = d;
                rd = i < 0 ? rd : (d < (float(k) / realRes.x) ? f + rd : rd);
                if (i >= 0 && (d < (float(k) / realRes.x))) {
                    float h = float(iret.dIndex % pd.particlesPerPartition) / float(pd.particlesPerPartition);
                    color = hsv2rgb(vec3(h, 1.0, 1.0));
                    color = mix(vec3(1.0), color, d * iResolution.x / 10.0);
                	//fragColor += clamp(0.01 * vec4(color, 1.0) * vec4(1.0 / (d * realRes.x)), 0.0, 1.0);
                }
            }
        }
    }

    float h = float(minIndex % pd.particlesPerPartition) / float(pd.particlesPerPartition);
    color = hsv2rgb(vec3(h, 1.0, 1.0));
    color = mix(vec3(1.0), color, 100.0 * mind);

    float size = minIndex >= 0 ? float(minIndex % 10 + 1) : 1.0;

    float brightness = 1.0;
    //fragColor += clamp(brightness * vec4(color, 1.0) * vec4(1.0 / (mind * 1000.0)), 0.0, 1.0);
    fragColor += clamp(brightness * vec4(color, 1.0) * vec4(1.0 / (mind * realRes.x)), 0.0, 1.0);
    //fragColor += clamp(brightness * vec4(color, 1.0) * vec4(1.0 / (rd * realRes.x)), 0.0, 1.0);
    //fragColor = vec4(1.0 * rd);
}

void debug(out vec4 fragColor, in vec2 fragCoord) {
    vec4 v0 = texelFetch(iChannel0, ivec2(fragCoord), 0);
    vec4 v1 = texelFetch(iChannel1, ivec2(fragCoord), 0);
    vec4 v2 = texelFetch(iChannel2, ivec2(fragCoord), 0);
    vec4 v3 = texelFetch(iChannel3, ivec2(fragCoord), 0);
    //float val = float(int(v.w) % 1000) / 1000.0;
    float val = float(int(v0.x) % 10000) / 10000.0;
    fragColor = vec4(val);
    //fragColor.rb = v0.yz;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    //debug(fragColor, fragCoord);
    lookup(fragColor, fragCoord);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// horizontal Gaussian blur pass

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize = 1./ iChannelResolution[0].xy;
    vec2 uv = fragCoord.xy * pixelSize;
    
    float h = pixelSize.x;
	vec4 sum = vec4(0.0);
	sum += texture(iChannel0, fract(vec2(uv.x - 4.0*h, uv.y)) ) * 0.05;
	sum += texture(iChannel0, fract(vec2(uv.x - 3.0*h, uv.y)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x - 2.0*h, uv.y)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x - 1.0*h, uv.y)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x + 0.0*h, uv.y)) ) * 0.16;
	sum += texture(iChannel0, fract(vec2(uv.x + 1.0*h, uv.y)) ) * 0.15;
	sum += texture(iChannel0, fract(vec2(uv.x + 2.0*h, uv.y)) ) * 0.12;
	sum += texture(iChannel0, fract(vec2(uv.x + 3.0*h, uv.y)) ) * 0.09;
	sum += texture(iChannel0, fract(vec2(uv.x + 4.0*h, uv.y)) ) * 0.05;
    
    fragColor.xyz = sum.xyz/0.98; // normalize
	fragColor.a = 1.;
    
    //fragColor = texture(iChannel0, uv);
}