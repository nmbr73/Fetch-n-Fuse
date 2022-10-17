

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

float kern_v(float x) { return 1.0-x*x*(3.0-2.0*abs(x)); }
float kern_d(float x) { float o = abs(x)-1.0; return x*(o*o); }

float kern_vD1(float x) { return x*(abs(x)*6.0-6.0); }
float kern_dD1(float x) { return (abs(x)-1.0)*(abs(x)*3.0-1.0); }

float kern_vD2(float x) { return abs(x) * 12.0 - 6.0; }
float kern_dD2(float x) { return x * 6.0 + (x > 0.0 ? -4.0 : 4.0); }


vec4 kern(vec2 p)
{
    return vec4(kern_d(p.x) * kern_v(p.y),
                kern_v(p.x) * kern_d(p.y),
                kern_d(p.x) * kern_d(p.y),
                kern_v(p.x) * kern_v(p.y));
}

mat4 kern4x4(vec2 p)
{
    vec2 v   = vec2(kern_v  (p.x), kern_v  (p.y));
    vec2 d   = vec2(kern_d  (p.x), kern_d  (p.y));
    
    vec2 vD1 = vec2(kern_vD1(p.x), kern_vD1(p.y));
    vec2 dD1 = vec2(kern_dD1(p.x), kern_dD1(p.y));
    
    mat4 m = mat4
    (
        /*   kernDx         kernDy         kernDxy          kern    */
        vec4(dD1.x * v.y  ,  d.x * vD1.y  ,  dD1.x * vD1.y  ,  d.x * v.y),
        vec4(vD1.x * d.y  ,  v.x * dD1.y  ,  vD1.x * dD1.y  ,  v.x * d.y),
        vec4(dD1.x * d.y  ,  d.x * dD1.y  ,  dD1.x * dD1.y  ,  d.x * d.y),
        vec4(vD1.x * v.y  ,  v.x * vD1.y  ,  vD1.x * vD1.y  ,  v.x * v.y)
    );
    
    return m;
}

void kern4x4(vec2 p, out mat4 mA, out mat4 mB)
{
    vec2 v   = vec2(kern_v  (p.x), kern_v  (p.y));
    vec2 d   = vec2(kern_d  (p.x), kern_d  (p.y));
    
    vec2 vD1 = vec2(kern_vD1(p.x), kern_vD1(p.y));
    vec2 dD1 = vec2(kern_dD1(p.x), kern_dD1(p.y));
    
    vec2 vD2 = vec2(kern_vD2(p.x), kern_vD2(p.y));
    vec2 dD2 = vec2(kern_dD2(p.x), kern_dD2(p.y));
    
    mA = mat4
    (
        /*   kernDx         kernDy         kernDxy          kern    */
        vec4(dD1.x * v.y  ,  d.x * vD1.y  ,  dD1.x * vD1.y  ,  d.x * v.y),
        vec4(vD1.x * d.y  ,  v.x * dD1.y  ,  vD1.x * dD1.y  ,  v.x * d.y),
        vec4(dD1.x * d.y  ,  d.x * dD1.y  ,  dD1.x * dD1.y  ,  d.x * d.y),
        vec4(vD1.x * v.y  ,  v.x * vD1.y  ,  vD1.x * vD1.y  ,  v.x * v.y)
    );

    mB = mat4
    (
        /*   kernDxx        kernDyy        kernDxxy         kernDxyy    */
        vec4(dD2.x * v.y  ,  d.x * vD2.y  ,  dD2.x * vD1.y  ,  dD1.x * vD2.y),
        vec4(vD2.x * d.y  ,  v.x * dD2.y  ,  vD2.x * dD1.y  ,  vD1.x * dD2.y),
        vec4(dD2.x * d.y  ,  d.x * dD2.y  ,  dD2.x * dD1.y  ,  dD1.x * dD2.y),
        vec4(vD2.x * v.y  ,  v.x * vD2.y  ,  vD2.x * vD1.y  ,  vD1.x * vD2.y)
    );
}


// BICUBIC SAMPLING ROUTINES =============================================================================================================

// this is the most basic version which only evaluates the function value
float SampleBicubic(sampler2D channel, vec2 uv)
{
    uv -= vec2(0.5);
    
    vec2 uvi = floor(uv);
    vec2 uvf = uv - uvi;

    ivec2 uv0 = ivec2(uvi);
    
    float r = 0.0;
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        vec4 c = texelFetch(channel, uv0 + ivec2(i, j), 0);
        
        vec2 l = uvf;
        
        if(i != 0) l.x -= 1.0;
        if(j != 0) l.y -= 1.0;
        
        r += dot(c, kern(l));
    }
    
	return r;
}

// ... this version also outputs derivatives (used here to compute normals)
vec4 SampleBicubic2(sampler2D channel, vec2 uv)
{
    uv -= vec2(0.5);
    
    vec2 uvi = floor(uv);
    vec2 uvf = uv - uvi;

    ivec2 uv0 = ivec2(uvi);
    
    vec4 r = vec4(0.0);
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        vec4 c = texelFetch(channel, uv0 + ivec2(i, j), 0);
        
        vec2 l = uvf;
        
        if(i != 0) l.x -= 1.0;
        if(j != 0) l.y -= 1.0;
        
        r += kern4x4(l) * c;
    }
    
    // r = vec4(df/dx, df/dy, ddf/dxy, f)
	return r;
}

// ... this version also outputs higher order derivatives (only used to debug C2 continuity here)
vec4 SampleBicubic3(sampler2D channel, vec2 uv, out vec4 d2)
{
    uv -= vec2(0.5);
    
    vec2 uvi = floor(uv);
    vec2 uvf = uv - uvi;

    ivec2 uv0 = ivec2(uvi);
    
    d2 = vec4(0.0);
    vec4 r = vec4(0.0);
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        vec4 c = texelFetch(channel, uv0 + ivec2(i, j), 0);
        
        vec2 l = uvf;
        
        if(i != 0) l.x -= 1.0;
        if(j != 0) l.y -= 1.0;
        
        mat4 mA, mB;
        kern4x4(l, /*out*/ mA, mB);
        
        r  += mA * c;
        d2 += mB * c;
    }
    
    // r  = vec4(  df/dx,   df/dy,  ddf/dxy ,         f)
    // d2 = vec4(ddf/dxx, ddf/dyy, dddf/dxxy, dddf/dxyy)
	return r;
}


// IMAGE ==========================================================================================================================

float ReadKey(int keyCode) {return texelFetch(iChannel2, ivec2(keyCode, 0), 0).x;}
float ReadKeyToggle(int keyCode) {return texelFetch(iChannel2, ivec2(keyCode, 2), 0).x;}

void mainImage( out vec4 fragColor, in vec2 uv0 )
{    
    #if 0
    fragColor = texelFetch(iChannel1, ivec2(uv0-0.5), 0);
    return;
    #endif

    bool isTerrainAnimated = ReadKeyToggle(KEY_N1) == 0.0;

   #if 1
    // mini map
    bool doShowWaveField = ReadKeyToggle(KEY_N3) != 0.0;
   
    if(doShowWaveField)
    if(uv0.x < GridSize && uv0.y < GridSize)
    {
        vec3 col = vec3(1.0) - normalize(vec3(texelFetch(iChannel0, ivec2(uv0 - 0.5), 0).xy, .01)).z;
        
        float d = EvalTerrainHeight(uv0, isTerrainAnimated ? iTime : 0.0);
        
        bool doShowHeightField = ReadKeyToggle(KEY_N4) != 0.0;
        if(doShowHeightField) col = (texelFetch(iChannel0, ivec2(uv0 - 0.5), 0).www * 1.0 + 0.5);
        
        col *= col;

        float l = -min(d, 0.0)*3.;
        col = mix(vec3(0.125, 0.125, 1.0 ), col, 1.0-(exp2(-(l*2.0 + l*l*1.0))));
        col = mix(col, vec3(1.0  , 0.0  , 0.25), smoothstep(-0.05, 0.0, d));

        bool isGridWindowSharp = ReadKeyToggle(KEY_N2) != 0.0;
        if(isGridWindowSharp && (uv0.x == 0.5 || uv0.y == 0.5 || uv0.x == GridSize-0.5 || uv0.y == GridSize-0.5)) col = vec3(0.0, 1.0, 1.0); 

        fragColor = vec4(sqrt(clamp01(col.rgb)), 0.0);
        return;
    }
   #endif
 
    vec3 col;
    
    vec2 uv = uv0;
    vec2 tc = uv0 / iResolution.xx;
    
   #if 0
    if(uv0.x >= iResolution.x*0.5)
    uv.x -= iResolution.x*0.5;
   #endif
    
    col = vec3(texture(iChannel0, uv0/iResolution.xy*0.125).r);
    
    
    vec2 uv2 = PatchUVfromScreenUV(uv0.xy, iResolution.xy);

    float time = isTerrainAnimated ? iTime : 0.0;

    vec3 V = vec3(0.0, 0.0, 1.0);
    vec3 L = normalize(vec3(1.0, 1.0, 1.0));
    vec3 L2 = normalize(vec3(-1.0, -1.0, 2.0));
    vec3 H = normalize(L + V);


    vec3 terrN;
    {
        float s = 1.0/128.0;
        
        float hx0 = EvalTerrainHeight(uv2 - vec2(s, 0.0), time);
        float hx1 = EvalTerrainHeight(uv2 + vec2(s, 0.0), time);
        float hy0 = EvalTerrainHeight(uv2 - vec2(0.0, s), time);
        float hy1 = EvalTerrainHeight(uv2 + vec2(0.0, s), time);
        
        vec2 dxy = vec2(hx1 - hx0, hy1 - hy0) / (2.0 * s);
        
        terrN = normalize(vec3(-dxy, 0.03));
    }
    
    vec3 terrCol = clamp01(dot(terrN, L))*(1.0/(1.0+1.0*(1.0-clamp01(dot(terrN, H)))))*vec3(1.0)*0.05;


    vec4 d2;
    vec4 h = SampleBicubic3(iChannel0, uv2, d2);// sample water surface
    
    float nscale = 32.0;
    vec3 N = normalize(vec3(-h.xy * nscale, 1.0));
    
    vec3 R = 2.0*dot(V, N)*N - V;

    float ct = clamp01(dot(N, L));
    float ct2 = dot(N, L) * 0.5 + 0.5;
    
    float d = h.w - EvalTerrainHeight(uv2-N.xy*4.0, time);
    
    float waterMask = smoothstep(-0.01, 0.01, d);
    
    // diffuse
    float v =  clamp01(ct2+0.15);
    v = 1.0-v;
    v = cubic(v);
    
    col = exp(-(v * 12.0 + 5.) * vec3(0.05, 0.3, 1.))*1.4;
    //col *= mix(0.25, 1.0, clamp01(dot(terrN, L)+0.5));

    float l = max(0.0, d);
    col = mix(terrCol, col, (1.0-exp2(-(l*2.0 + l*l*1.0))));
    col += vec3(0.0, 0.25, 1.0)*0.1;
    
    // specular
    float c = 1.0 - (dot(R, L)*0.5+0.5);
    float c2 = 1.0 - (dot(R, L2)*0.5+0.5);
    float spec = 0.0;
    spec += smoothstep(0.9, 0.99, dot(R, L))*0.5; 
    float spec0 = spec;
    spec += smoothstep(0.7, 0.9, dot(R, L))*0.125; 
    spec += smoothstep(0.8, 0.9, dot(R, L2))*0.02; 
    spec += smoothstep(0.95, 0.99, N.z)*0.02; 
    spec += exp2(-32.0*(c))*0.25;
    
    col += vec3(1.0) * spec * waterMask;
    
    col = GammaEncode(clamp01(col));
    
    fragColor = vec4(col, 0.0);
}


























// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0/)

    #define USE_HQ_KERNEL
     
    // #define USE_AXISALIGNED_OBSTACLES

const float GridScale = 100.0;// vertex spacing; 1 -> meters | 100 -> centimeters 

const float GridSize = 256.0;// vertex count per dimension



float TrigNoise(vec3 x, float a, float b)
{
    vec4 u = vec4(dot(x, vec3( 1.0, 1.0, 1.0)), 
                  dot(x, vec3( 1.0,-1.0,-1.0)), 
                  dot(x, vec3(-1.0, 1.0,-1.0)),
                  dot(x, vec3(-1.0,-1.0, 1.0))) * a;

    return dot(sin(x     + cos(u.xyz) * b), 
               cos(x.zxy + sin(u.zwx) * b));
}

float TrigNoise(vec3 x)
{
    return TrigNoise(x, 2.0, 1.0);
}  



float EvalTerrainHeight(vec2 uv, float time)
{
    uv -= vec2(GridSize * 0.5);
    
  #ifdef USE_AXISALIGNED_OBSTACLES
    return max(max((-uv.x-64.0), uv.y-64.0), min((uv.x-32.0), -uv.y-8.0)) * 0.02;
  #endif
    
    float w = time * 0.125;
    //w = 0.0;
    
    float terr = -(TrigNoise(vec3(uv * 0.01, w)) + 1.0) * 0.5;
    
    return terr;
}


vec2 PatchUVfromScreenUV(vec2 screenUV, vec2 screenResolution)
{
    return vec2(GridSize * 0.5) + (screenUV - screenResolution.xy*0.5)/screenResolution.xx * 226.0;
}


/*
// x: [0, inf], s: (-1, 1] / (soft, hard]
float SoftClip(float x, float s)
{
    return (1.0 + x - sqrt(1.0 - 2.0*s*x + x*x)) / (1.0 + s);
}

vec3 SoftClip(vec3 x, float s)
{
    return (1.0 + x - sqrt(1.0 - 2.0*s*x + x*x)) / (1.0 + s);
}

*/




#define rsqrt inversesqrt
#define clamp01(x) clamp(x, 0.0, 1.0)
#define If(cond, resT, resF) mix(resF, resT, cond)

const float Pi = 3.14159265359;
const float Pi05 = Pi * 0.5;

float Pow2(float x) {return x*x;}
float Pow3(float x) {return x*x*x;}
float Pow4(float x) {return Pow2(Pow2(x));}

/* http://keycode.info/ */
#define KEY_LEFT  37
#define KEY_UP    38
#define KEY_RIGHT 39
#define KEY_DOWN  40

#define KEY_CTRL 17
#define KEY_SHIFT 16
#define KEY_SPACE 32 
#define KEY_A 0x41
#define KEY_D 0x44
#define KEY_S 0x53
#define KEY_W 0x57

#define KEY_N1 49
#define KEY_N2 50
#define KEY_N3 51
#define KEY_N4 52
#define KEY_N5 53
#define KEY_N6 54
#define KEY_N7 55
#define KEY_N8 56

vec3 GammaEncode(vec3 x) {return pow(x, vec3(1.0 / 2.2));}

vec2 AngToVec(float ang)
{	
	return vec2(cos(ang), sin(ang));
}


vec3 AngToVec(vec2 ang)
{
    float sinPhi   = sin(ang.x);
    float cosPhi   = cos(ang.x);
    float sinTheta = sin(ang.y);
    float cosTheta = cos(ang.y);    

    return vec3(cosPhi * cosTheta, 
                         sinTheta, 
                sinPhi * cosTheta); 
}

float cubic(float x) {return x*x*(3.-2.*x);}
vec2  cubic(vec2  x) {return x*x*(3.-2.*x);}
vec3  cubic(vec3  x) {return x*x*(3.-2.*x);}
vec4  cubic(vec4  x) {return x*x*(3.-2.*x);}

float quintic(float x){ return ((x * 6.0 - 15.0) * x + 10.0) * x*x*x;}
vec2  quintic(vec2  x){ return ((x * 6.0 - 15.0) * x + 10.0) * x*x*x;}
vec3  quintic(vec3  x){ return ((x * 6.0 - 15.0) * x + 10.0) * x*x*x;}
vec4  quintic(vec4  x){ return ((x * 6.0 - 15.0) * x + 10.0) * x*x*x;}


uint  asuint2(float x) { return x == 0.0 ? 0u : floatBitsToUint(x); }
uvec2 asuint2(vec2 x) { return uvec2(asuint2(x.x ), asuint2(x.y)); }
uvec3 asuint2(vec3 x) { return uvec3(asuint2(x.xy), asuint2(x.z)); }
uvec4 asuint2(vec4 x) { return uvec4(asuint2(x.xy), asuint2(x.zw)); }

float Float01(uint x) { return float(    x ) * (1.0 / 4294967296.0); }
float Float11(uint x) { return float(int(x)) * (1.0 / 2147483648.0); }

vec2 Float01(uvec2 x) { return vec2(      x ) * (1.0 / 4294967296.0); }
vec2 Float11(uvec2 x) { return vec2(ivec2(x)) * (1.0 / 2147483648.0); }

vec3 Float01(uvec3 x) { return vec3(      x ) * (1.0 / 4294967296.0); }
vec3 Float11(uvec3 x) { return vec3(ivec3(x)) * (1.0 / 2147483648.0); }

vec4 Float01(uvec4 x) { return vec4(      x ) * (1.0 / 4294967296.0); }
vec4 Float11(uvec4 x) { return vec4(ivec4(x)) * (1.0 / 2147483648.0); }

// constants rounded to nearest primes
const uint rPhi1  = 2654435761u;

const uint rPhi2a = 3242174893u;
const uint rPhi2b = 2447445397u;

const uint rPhi3a = 3518319149u;
const uint rPhi3b = 2882110339u;
const uint rPhi3c = 2360945581u;

const uint rPhi4a = 3679390609u;
const uint rPhi4b = 3152041517u;
const uint rPhi4c = 2700274807u;
const uint rPhi4d = 2313257579u;

const uvec2 rPhi2 = uvec2(rPhi2a, rPhi2b);
const uvec3 rPhi3 = uvec3(rPhi3a, rPhi3b, rPhi3c);
const uvec4 rPhi4 = uvec4(rPhi4a, rPhi4b, rPhi4c, rPhi4d);

uint  Roberts(uint  off, uint n) { return off + rPhi1 * n; }
uvec2 Roberts(uvec2 off, uint n) { return off + rPhi2 * n; }
uvec3 Roberts(uvec3 off, uint n) { return off + rPhi3 * n; }
uvec4 Roberts(uvec4 off, uint n) { return off + rPhi4 * n; }

// http://marc-b-reynolds.github.io/math/2016/03/29/weyl_hash.html
uint WeylHash(uvec2 c) 
{
    return ((c.x * 0x3504f333u) ^ (c.y * 0xf1bbcdcbu)) * 741103597u; 
}

// low bias version https://nullprogram.com/blog/2018/07/31/
uint WellonsHash(uint x)
{
    x ^= x >> 16u;
    x *= 0x7feb352dU;
    x ^= x >> 15u;
    x *= 0x846ca68bU;
    x ^= x >> 16u;

    return x;
}

uvec2 WellonsHash(uvec2 v) { return uvec2(WellonsHash(v.x), WellonsHash(v.y)); }


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0/)

/* Simulation */

float ReadKey(int keyCode) {return texelFetch(iChannel1, ivec2(keyCode, 0), 0).x;}
float ReadKeyToggle(int keyCode) {return texelFetch(iChannel1, ivec2(keyCode, 2), 0).x;}

#define FETCH(uv) texelFetch(iChannel0, uv, 0).r
#define FETCH2(uv) texelFetch(iChannel2, uv, 0).r
#define FETCH3(uv) texelFetch(iChannel2, uv, 0)

void mainImage( out vec4 col, in vec2 uv0 )
{

    // =================================================================================== //
    // program state
    bool isGrid = uv0.x < GridSize && uv0.y < GridSize;
    bool isStateColumn = uv0.x == iResolution.x - 0.5;
    
    if(!isGrid && !isStateColumn) { discard; }

    ivec2 uv = ivec2(uv0 - 0.5);
    int stateColumnX = int(iResolution.x - 1.0);
    
    float iFrameTest     = texelFetch(iChannel0, ivec2(stateColumnX, 0), 0).x;
    vec4  iMouseLast     = texelFetch(iChannel0, ivec2(stateColumnX, 1), 0);
    float iTimeDeltaLast = texelFetch(iChannel0, ivec2(stateColumnX, 2), 0).x;

    bool isInit = float(iFrame) == iFrameTest;
    if( !isInit)
    {
        iTimeDeltaLast = iTimeDelta;
    }
    
    if(isStateColumn)
    {
        if(uv.y == 0) col = vec4(float(iFrame) + 1.0, 0.0, 0.0, 0.0);
        if(uv.y == 1) col = iMouse;
        if(uv.y == 2) col = vec4(iTimeDelta, 0.0, 0.0, 0.0);
        
        return;
    }
    
    if(!isGrid) return;
    // =================================================================================== //

    col = vec4(0.0);
    
    vec2 h12 = texelFetch(iChannel0, uv, 0).xy;

    bool isTerrainAnimated = ReadKeyToggle(KEY_N1) == 0.0;
    
    float terrH = EvalTerrainHeight(uv0, isTerrainAnimated ? iTime : 0.0);
    float mask = smoothstep(0.0, -0.05, terrH);
    float D = clamp01(-terrH);
    float lD2 = clamp01(-terrH-1.0);
    
   
#ifdef USE_HQ_KERNEL

    // 30 tabs version (vertical pass; horizontal pass in Buffer B)
    float lowp3[4] = float[4](5.0/16.0, 15.0/64.0, 3.0/32.0, 1.0/64.0);   
    float lowp7[8] = float[8](429.0/2048.0, 3003.0/16384.0, 1001.0/8192.0, 1001.0/16384.0, 91.0/4096.0, 91.0/16384.0, 7.0/8192.0, 1.0/16384.0);   
    float lapl7[8] = float[8](3.22, -1.9335988099476562, 0.4384577800334821, -0.1637450351359609, 0.07015324480535713, -0.02963974593026339, 0.010609595665007715, -0.0022370294899667453);   

    float lowpass3  = 0.0;
    float lowpass7  = 0.0;
    float laplacian = 0.0;
    
    for(int y = -7; y <= 7; ++y)
    {
        vec3 f = FETCH3(uv + ivec2(0, y)).xyz;
    
        int i = abs(y);

        lowpass3  += f.x * (i < 4 ? lowp3[i] : 0.0);
        lowpass7  += f.y * lowp7[i];
        laplacian += f.z * lapl7[i];
    }

    vec4 f0 = FETCH3(uv);
    
    laplacian += f0.w;
    
    float highpass = f0.z - mix(lowpass3, lowpass7, 0.772 * lD2);
    float halfLaplacian =   mix(highpass, laplacian, 0.19)*1.255;

    float Aa = laplacian;
    float Ab = halfLaplacian;
   
#else

   #if 0
    // 21 tabs version
    const int r  = 4;
    const int r1 = r + 1;

    float kernA[r1] = float[r1](3.14, -1.8488262937460072, 0.3538769077873216, -0.0913000638886917, 0.016249449847377015);

    float kernB[r1*r1] = float[r1*r1](2.269921105564736    , -0.4505893247500618, 0.01789846106075618, -0.01027660288590306, 0.0034772145111404747, 
                                     -0.4505893247500618   , -0.1279900243271159, 0.                 ,  0.                 , 0.                   , 
                                      0.01789846106075618  ,  0.                , 0.                 ,  0.                 , 0.                   , 
                                     -0.01027660288590306  ,  0.                , 0.                 ,  0.                 , 0.                   , 
                                      0.0034772145111404747,  0.                , 0.                 ,  0.                 , 0.                   );  
   #else
    // 13 tabs version
    const int r  = 2;
    const int r1 = r + 1;
 
    float kernA[r1] = float[r1](2.85, -1.5792207792207793, 0.15422077922077923);
 
    float kernB[r1*r1] = float[r1*r1](2.0933782605117255  , -0.32987120049780483, -0.026408964879028916, 
                                     -0.32987120049780483 , -0.1670643997510976 ,  0.0                 ,
                                     -0.026408964879028916,  0.0                ,  0.0                 );
   #endif
     
      
    float Aa = 0.0;
    float Ab = 0.0;

    for(int y = -r; y <= r; ++y)
    for(int x = -r; x <= r; ++x)
    {
        float w = (kernB[abs(x) + abs(y) * r1]);

        if(w == 0.0) continue;

        float f = FETCH(uv + ivec2(x, y));

        Ab += f * w;

        if(y == 0) Aa += f * kernA[abs(x)];                
        if(x == 0) Aa += f * kernA[abs(y)];                
    }     

#endif

  #ifndef USE_AXISALIGNED_OBSTACLES
    // mitigate erroneous simulation behavior along shorelines
    D = mix(0.25, 1.0, D);
  #endif
  
    float A = mix(Aa, Ab, (D*D) / (2.0/7.0 + 5.0/7.0 * (D*D))) * D;
    
   // A = Ab;// deep
   // A = Aa * 0.5;// shallow

    A *= -9.81*GridScale;


    // painting
    bool isSingleDrop = ReadKey(KEY_SHIFT) != 0.0;
    
    if(iMouse.w > 0.0 || (!isSingleDrop && iMouse.z > 0.0 || (iMouse.x != iMouseLast.x && iMouse.y != iMouseLast.y)))
    {
        vec2 c = PatchUVfromScreenUV(iMouse.xy, iResolution.xy);
        
        vec2 vec = (uv0 - c);
  
        if(!isSingleDrop)
        if(iMouseLast.z > 0.0 || iMouseLast.w > 0.0)
        {
            vec2 c2 = PatchUVfromScreenUV(iMouseLast.xy, iResolution.xy);
            
            vec = uv0 - (c2 + (c-c2)*clamp01(dot(c-c2, uv0-c2)/dot(c-c2,c-c2)));
        }
  
        float v = exp2(-dot(vec, vec) * 1.0);
        h12 = mix(h12, vec2(0.75), v);
    }
    
    if(iFrame == 0) h12 = vec2(0.0);

    // rain drops
    if(ReadKeyToggle(KEY_N5) == 0.0)
    if(WellonsHash(uint(iFrame)) < 100000000u)
    {
        vec2 c = Float01(WellonsHash(uint(iFrame) * uvec2(3242174893u, 2447445397u) + 3u)) * GridSize;
        vec2 vec = (uv0 - c);
    
        float v = exp2(-dot(vec, vec) * 1.0);
        h12 = mix(h12, vec2(0.75), v);
    }

    float dt = 0.016667;
    float dt2 =  dt * dt;

    float h0 = 0.0;
    float h1 = h12.x;
    float h2 = h12.y;

#if 1
    // Verlet integration
    h0 = (2.0 * h1 - h2) + A * dt2;

#else

    // ...damped version
    float a = 1.0/2.0;
    float adt = a * dt;
    
    h0 = (((2.0 + adt) * h1 - h2) + A * dt2) / (1.0 + adt);

#endif

    vec2 h01 = vec2(h0, h1);

    // exponential state buffer smoothing
    float beta = 2.0;
    h01 = mix(h01, h12, 1.0-exp2(-dt*beta));

    // mask out obstacles
    h01 *= mask;

    // grid windowing
    bool isGridWindowed = ReadKeyToggle(KEY_N2) == 0.0;
    if(isGridWindowed)
    {
        float r = 32.0;

        vec2 u = min((vec2(GridSize*0.5) - abs(uv0 - vec2(GridSize*0.5))) / r, vec2(1.0));

        u = 1.0 - u;
        u *= u;
        u *= u;
        u = 1.0 - u;

        float s = u.x*u.y;

        h01 *= mix(0.75, 1.0, s);        
    }
    
    if(ReadKey(KEY_SPACE) != 0.0) { h01 *= 0.95; } 
    
    col = vec4(h01, 0.0, 0.0);
}










// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0/)

/* horizontal pass of hq deep water kernel */

#define FETCH(uv) texelFetch(iChannel0, uv, 0).r

void mainImage(out vec4 col, in vec2 uv0)
{
  #ifndef USE_HQ_KERNEL
    discard; return;
  #endif
  
    bool isGrid = uv0.x < GridSize && uv0.y < GridSize;

    if(!isGrid) return;

    ivec2 uv = ivec2(uv0 - 0.5);
    
    float lowp3[4] = float[4](5.0/16.0, 15.0/64.0, 3.0/32.0, 1.0/64.0);   
    float lowp7[8] = float[8](429.0/2048.0, 3003.0/16384.0, 1001.0/8192.0, 1001.0/16384.0, 91.0/4096.0, 91.0/16384.0, 7.0/8192.0, 1.0/16384.0);   
    float lapl7[8] = float[8](3.22, -1.9335988099476562, 0.4384577800334821, -0.1637450351359609, 0.07015324480535713, -0.02963974593026339, 0.010609595665007715, -0.0022370294899667453);   

    float lowpass3  = 0.0;
    float lowpass7  = 0.0;
    float laplacian = 0.0;
    
    for(int x = -7; x <= 7; ++x)
    {
        float f = FETCH(uv + ivec2(x, 0));
    
        int i = abs(x);

        lowpass3  += f * (i < 4 ? lowp3[i] : 0.0);
        lowpass7  += f * lowp7[i];
        laplacian += f * lapl7[i];
    }

    float f0 = FETCH(uv);
    
    col = vec4(lowpass3, lowpass7, f0, laplacian);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0/)

/*
    pre-filter pass (prepares height field for rendering)
    https://www.shadertoy.com/view/WtsBDH ("Bicubic C2 cont. Interpolation")
*/

#define FETCH(uv) (texelFetch(iChannel0, uv, 0).r)

void mainImage( out vec4 fragColor, in vec2 uv0 )
{    
    if(uv0.x > GridSize || uv0.y > GridSize) return;

    ivec2 uv = ivec2(uv0 - 0.5);
    
    vec4 col = vec4(0.0);
    
    // ======================================================= GENERALIZED CUBIC BSPLINE =======================================================
    
    float kernD0[3];
    float kernD1[3];
    
    kernD0[0] = 2.0/3.0; kernD0[1] = 1.0/6.0; kernD0[2] = 0.0;
    kernD1[0] =     0.0; kernD1[1] =    -0.5; kernD1[2] = 0.0;
    
    float sw;// side lobes weight

    sw = 0.0;// cubic BSpline
    
   #if 0
   
    sw = 0.25;// similar to 1/3 but less ringing
    
   #elif 0
   
    sw = 1.0/3.0;// kernD0[0] == 1
    
   #elif 0
    
    sw = 0.186605;// max abs derivative == 1
    
   #elif 0
    
    sw = 1.0/6.0;// maximaly flat pass band
    
   #elif 1
    
    sw = -0.25;// spectrum falls off to 0 at Nyquist frequency
    
   #endif
    
    // add a pair of side lobes:
    kernD0[0] += 1.0 * sw; kernD0[1] += -1.0/3.0 * sw; kernD0[2] += -1.0/6.0 * sw;
    	                   kernD1[1] += -1.0     * sw; kernD1[2] +=  0.5     * sw;
    
    int r = sw == 0.0 ? 1 : 2;
    for(int j = -r; j <= r; ++j)
    for(int i = -r; i <= r; ++i)
    {
    	float f = FETCH(uv + ivec2(i, j));
        
        int x = abs(i);
        int y = abs(j);
        
        float kAx = kernD0[x];
        float kAy = kernD0[y];
        
        float kBx = kernD1[x] * (i > 0 ? -1.0 : 1.0);
        float kBy = kernD1[y] * (j > 0 ? -1.0 : 1.0);
        
        col += f * vec4(kBx * kAy, 
                        kAx * kBy, 
                        kBx * kBy,
                        kAx * kAy);
    }
    
    // col = vec4(df/dx, df/dy, d^2f/dxy, f)
    fragColor = col;
}

