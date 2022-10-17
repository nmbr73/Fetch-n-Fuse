
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0f/)

    #define USE_HQ_KERNEL
     
    // #define USE_AXISALIGNED_OBSTACLES

const float GridScale = 100.0f;// vertex spacing; 1 -> meters | 100 -> centimeters 

const float GridSize = 256.0f;// vertex count per dimension



__DEVICE__ float TrigNoise(float3 _x, float a, float b)
{
  float aaaaaaaaaaaaaaaaaaaaaa;
    float4 u = to_float4(dot(_x, to_float3( 1.0f, 1.0f, 1.0f)), 
                         dot(_x, to_float3( 1.0f,-1.0f,-1.0f)), 
                         dot(_x, to_float3(-1.0f, 1.0f,-1.0f)),
                         dot(_x, to_float3(-1.0f,-1.0f, 1.0f))) * a;

    return dot(sin_f3(_x             + cos_f3(swi3(u,x,y,z)) * b), 
               cos_f3(swi3(_x,z,x,y) + sin_f3(swi3(u,z,w,x)) * b));
}

__DEVICE__ float TrigNoise(float3 _x)
{
    return TrigNoise(_x, 2.0f, 1.0f);
}  

__DEVICE__ float EvalTerrainHeight(float2 uv, float time)
{
    uv -= to_float2_s(GridSize * 0.5f);
    
  #ifdef USE_AXISALIGNED_OBSTACLES
    return _fmaxf(_fmaxf((-uv.x-64.0f), uv.y-64.0f), _fminf((uv.x-32.0f), -uv.y-8.0f)) * 0.02f;
  #endif
    
    float w = time * 0.125f;
    //w = 0.0f;
    
    float terr = -(TrigNoise(to_float3_aw(uv * 0.01f, w)) + 1.0f) * 0.5f;
    
    return terr;
}


__DEVICE__ float2 PatchUVfromScreenUV(float2 screenUV, float2 screenResolution)
{
    return to_float2_s(GridSize * 0.5f) + (screenUV - screenResolution*0.5f)/swi2(screenResolution,x,x) * 226.0f;
}


/*
// x: [0, inf], s: (-1, 1] / (soft, hard]
__DEVICE__ float SoftClip(float x, float s)
{
    return (1.0f + x - _sqrtf(1.0f - 2.0f*s*x + x*x)) / (1.0f + s);
}

__DEVICE__ float3 SoftClip(float3 x, float s)
{
    return (1.0f + x - _sqrtf(1.0f - 2.0f*s*x + x*x)) / (1.0f + s);
}

*/


#define rsqrt 1.0f/_sqrtf //inversesqrt
#define clamp01(x) clamp(x, 0.0f, 1.0f)
#define If(cond, resT, resF) _mix(resF, resT, cond)

#define Pi  3.14159265359f
#define Pi05   (Pi * 0.5f)

__DEVICE__ float Pow2(float x) {return x*x;}
__DEVICE__ float Pow3(float x) {return x*x*x;}
__DEVICE__ float Pow4(float x) {return Pow2(Pow2(x));}

#ifdef XXX
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
#endif

__DEVICE__ float3 GammaEncode(float3 _x) {return pow_f3(_x, to_float3_s(1.0f / 2.2f));}

__DEVICE__ float2 AngToVec(float ang)
{  
  return to_float2(_cosf(ang), _sinf(ang));
}


__DEVICE__ float3 AngToVec(float2 ang)
{
    float sinPhi   = _sinf(ang.x);
    float cosPhi   = _cosf(ang.x);
    float sinTheta = _sinf(ang.y);
    float cosTheta = _cosf(ang.y);    
float fffffffffffffff;
    return to_float3(cosPhi * cosTheta, 
                     sinTheta, 
                     sinPhi * cosTheta); 
}

__DEVICE__ float   cubic(float x) {return x*x*(3.0f-2.0f*x);}
__DEVICE__ float2  cubic(float2  x) {return x*x*(3.0f-2.0f*x);}
__DEVICE__ float3  cubic(float3  x) {return x*x*(3.0f-2.0f*x);}
__DEVICE__ float4  cubic(float4  x) {return x*x*(to_float4_s(3.0f)-2.0f*x);}

__DEVICE__ float   quintic(float x){ return ((x * 6.0f - 15.0f) * x + 10.0f) * x*x*x;}
__DEVICE__ float2  quintic(float2  x){ return ((x * 6.0f - 15.0f) * x + 10.0f) * x*x*x;}
__DEVICE__ float3  quintic(float3  x){ return ((x * 6.0f - 15.0f) * x + 10.0f) * x*x*x;}
__DEVICE__ float4  quintic(float4  x){ return ((x * 6.0f - 15.0f) * x + 10.0f) * x*x*x;}


#ifdef XXX
union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

__DEVICE__ uint  asuint2(float x) { 
  Zahl z;
  
  z._Float = x;
float zzzzzzzzzzzzzzzzzzzzzz;  
  //return x == 0.0f ? 0u : floatBitsToUint(x); 
  return x == 0.0f ? 0u : z._Uint; 
  }

__DEVICE__ uint2 asuint2(float2 x) { return make_uint2(asuint2(x.x ), asuint2(x.y)); }
__DEVICE__ uint3 asuint2(float3 x) { return make_uint3(asuint2(swi2(x,x,y)), asuint2(x.z)); }
__DEVICE__ uint4 asuint2(float4 x) { return make_uint4(asuint2(swi2(x,x,y)), asuint2(swi2(x,z,w))); }
#endif

__DEVICE__ float Float01(uint x) { return (float)(    x ) * (1.0f / 4294967296.0f); }
__DEVICE__ float Float11(uint x) { return (float)((int)(x)) * (1.0f / 2147483648.0f); }

__DEVICE__ float2 Float01(uint2 x) { return to_float2_cuint(      x ) * (1.0f / 4294967296.0f); }
__DEVICE__ float2 Float11(uint2 x) { return to_float2_cint(to_int2_cuint(x)) * (1.0f / 2147483648.0f); }

__DEVICE__ float3 Float01(uint3 x) { return to_float3_cuint(      x ) * (1.0f / 4294967296.0f); }
__DEVICE__ float3 Float11(uint3 x) { return to_float3_cint(to_int3_cuint(x)) * (1.0f / 2147483648.0f); }

__DEVICE__ float4 Float01(uint4 x) { return to_float4_cuint(      x ) * (1.0f / 4294967296.0f); }
__DEVICE__ float4 Float11(uint4 x) { return to_float4_cint(to_int4_cuint(x)) * (1.0f / 2147483648.0f); }

// constants rounded to nearest primes
#define rPhi1   2654435761u

#define rPhi2a  3242174893u
#define rPhi2b  2447445397u

#define rPhi3a  3518319149u
#define rPhi3b  2882110339u
#define rPhi3c  2360945581u

#define rPhi4a  3679390609u
#define rPhi4b  3152041517u
#define rPhi4c  2700274807u
#define rPhi4d  2313257579u


__DEVICE__ uint  Roberts(uint  off, uint n) { return off + rPhi1 * n; }
__DEVICE__ uint2 Roberts(uint2 off, uint n) { const uint2 rPhi2 = make_uint2(rPhi2a, rPhi2b); return off + rPhi2 * n; }
__DEVICE__ uint3 Roberts(uint3 off, uint n) { const uint3 rPhi3 = make_uint3(rPhi3a, rPhi3b, rPhi3c); return off + rPhi3 * n; }
__DEVICE__ uint4 Roberts(uint4 off, uint n) { const uint4 rPhi4 = make_uint4(rPhi4a, rPhi4b, rPhi4c, rPhi4d); return off + rPhi4 * n; }

// http://marc-b-reynolds.github.io/math/2016/03/29/weyl_hash.html
__DEVICE__ uint WeylHash(uint2 c) 
{
    return ((c.x * 0x3504f333u) ^ (c.y * 0xf1bbcdcbu)) * 741103597u; 
}

// low bias version https://nullprogram.com/blog/2018/07/31/
__DEVICE__ uint WellonsHash(uint x)
{
    x ^= x >> 16u;
    x *= 0x7feb352dU;
    x ^= x >> 15u;
    x *= 0x846ca68bU;
    x ^= x >> 16u;

    return x;
}

__DEVICE__ uint2 WellonsHash(uint2 v) { return make_uint2(WellonsHash(v.x), WellonsHash(v.y)); }


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2


// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0f/)

/* Simulation */

//__DEVICE__ float ReadKey(int keyCode) {return texelFetch(iChannel1, to_int2(keyCode, 0), 0).x;}
//__DEVICE__ float ReadKeyToggle(int keyCode) {return texelFetch(iChannel1, to_int2(keyCode, 2), 0).x;}

//#define FETCH(uv) texelFetch(iChannel0, uv, 0).x
//#define FETCH2(uv) texelFetch(iChannel2, uv, 0).x
//#define FETCH3(uv) texelFetch(iChannel2, uv, 0)

#define FETCH(uv)  texture(iChannel0, (make_float2(uv)+0.5f)/iResolution).x
#define FETCH2(uv) texture(iChannel2, (make_float2(uv)+0.5f)/iResolution).x
#define FETCH3(uv) texture(iChannel2, (make_float2(uv)+0.5f)/iResolution)

__KERNEL__ void WatersurfacesimulationJipiFuse__Buffer_A(float4 col, float2 uv0, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_CHECKBOX0(KEY_N1, 0);
    CONNECT_CHECKBOX1(KEY_N2, 0);
    CONNECT_CHECKBOX2(KEY_N3, 0);
    CONNECT_CHECKBOX3(KEY_N4, 0);
    CONNECT_CHECKBOX4(KEY_N5, 0);
    CONNECT_CHECKBOX5(KEY_SHIFT, 0);
    CONNECT_CHECKBOX6(KEY_SPACE, 0);

    uv0+=0.5f;

    // =================================================================================== //
    // program state
    bool isGrid = uv0.x < GridSize && uv0.y < GridSize;
    bool isStateColumn = uv0.x == iResolution.x - 0.5f;
    
    if(!isGrid && !isStateColumn) { 
      //discard; 
      SetFragmentShaderComputedColor(col);
      return;
    }
float AAAAAAAAAAAAAAAAAAA;
    int2 uv = to_int2_cfloat(uv0 - 0.5f);
    int stateColumnX = (int)(iResolution.x - 1.0f);
    
    float iFrameTest     = texture(iChannel0, (make_float2(to_int2(stateColumnX, 0))+0.5f)/iResolution).x;
    float4  iMouseLast   = texture(iChannel0, (make_float2(to_int2(stateColumnX, 1))+0.5f)/iResolution);
    float iTimeDeltaLast = texture(iChannel0, (make_float2(to_int2(stateColumnX, 2))+0.5f)/iResolution).x;

    bool isInit = (float)(iFrame) == iFrameTest;
    if( !isInit)
    {
      iTimeDeltaLast = iTimeDelta;
    }
    
    if(isStateColumn)
    {
      if(uv.y == 0) col = to_float4((float)(iFrame) + 1.0f, 0.0f, 0.0f, 0.0f);
      if(uv.y == 1) col = iMouse;
      if(uv.y == 2) col = to_float4(iTimeDelta, 0.0f, 0.0f, 0.0f);
      
      SetFragmentShaderComputedColor(col);
      return;
    }
    
    if(!isGrid) 
    {
      SetFragmentShaderComputedColor(col);
      return;
    }
    // =================================================================================== //

    col = to_float4_s(0.0f);
    
    float2 h12 = swi2(texture(iChannel0, (make_float2(uv)+0.5f)/iResolution),x,y);

    bool isTerrainAnimated = KEY_N1;
    
    float terrH = EvalTerrainHeight(uv0, isTerrainAnimated ? iTime : 0.0f);
    float mask = smoothstep(0.0f, -0.05f, terrH);
    float D = clamp01(-terrH);
    float lD2 = clamp01(-terrH-1.0f);
    
   
#ifdef USE_HQ_KERNEL

    // 30 tabs version (vertical pass; horizontal pass in Buffer B)
    float lowp3[4] = {5.0f/16.0f, 15.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f};
    float lowp7[8] = {429.0f/2048.0f, 3003.0f/16384.0f, 1001.0f/8192.0f, 1001.0f/16384.0f, 91.0f/4096.0f, 91.0f/16384.0f, 7.0f/8192.0f, 1.0f/16384.0f};
    float lapl7[8] = {3.22f, -1.9335988099476562f, 0.4384577800334821f, -0.1637450351359609f, 0.07015324480535713f, -0.02963974593026339f, 0.010609595665007715f, -0.0022370294899667453f};

    float lowpass3  = 0.0f;
    float lowpass7  = 0.0f;
    float laplacian = 0.0f;
    
    for(int y = -7; y <= 7; ++y)
    {
        float3 f = swi3(FETCH3(uv + to_int2(0, y)),x,y,z);
    
        int i = _fabs(y);

        lowpass3  += f.x * (i < 4 ? lowp3[i] : 0.0f);
        lowpass7  += f.y * lowp7[i];
        laplacian += f.z * lapl7[i];
    }

    float4 f0 = FETCH3(uv);
    
    laplacian += f0.w;
    
    float highpass = f0.z - _mix(lowpass3, lowpass7, 0.772f * lD2);
    float halfLaplacian =   _mix(highpass, laplacian, 0.19f)*1.255f;

    float Aa = laplacian;
    float Ab = halfLaplacian;
   
#else

   #if 0
    // 21 tabs version
    const int r  = 4;
    const int r1 = r + 1;

    float kernA[r1] = {3.14f, -1.8488262937460072f, 0.3538769077873216f, -0.0913000638886917f, 0.016249449847377015f};

    float kernB[r1*r1] = {2.269921105564736f    , -0.4505893247500618f, 0.01789846106075618f, -0.01027660288590306f, 0.0034772145111404747f, 
                         -0.4505893247500618f   , -0.1279900243271159f, 0.0f                 ,  0.0f                 , 0.0f                   , 
                          0.01789846106075618f  ,  0.0f                , 0.0f                 ,  0.0f                 , 0.0f                   , 
                         -0.01027660288590306f  ,  0.0f                , 0.0f                 ,  0.0f                 , 0.0f                   , 
                          0.0034772145111404747f,  0.0f                , 0.0f                 ,  0.0f                 , 0.0f                   };  
   #else
    // 13 tabs version
    const int r  = 2;
    const int r1 = r + 1;
 
    float kernA[r1] = {2.85f, -1.5792207792207793f, 0.15422077922077923f};
 
    float kernB[r1*r1] = {2.0933782605117255f  , -0.32987120049780483f, -0.026408964879028916f, 
                         -0.32987120049780483f , -0.1670643997510976f ,  0.0f                 ,
                         -0.026408964879028916f,  0.0f                ,  0.0f                 };
   #endif
     
      
    float Aa = 0.0f;
    float Ab = 0.0f;

    for(int y = -r; y <= r; ++y)
    for(int x = -r; x <= r; ++x)
    {
        float w = (kernB[_fabs(x) + _fabs(y) * r1]);

        if(w == 0.0f) continue;

        float f = FETCH(uv + to_int2(x, y));

        Ab += f * w;

        if(y == 0) Aa += f * kernA[_fabs(x)];                
        if(x == 0) Aa += f * kernA[_fabs(y)];                
    }     

#endif

  #ifndef USE_AXISALIGNED_OBSTACLES
    // mitigate erroneous simulation behavior along shorelines
    D = _mix(0.25f, 1.0f, D);
  #endif
  
    float A = _mix(Aa, Ab, (D*D) / (2.0f/7.0f + 5.0f/7.0f * (D*D))) * D;
    
   // A = Ab;// deep
   // A = Aa * 0.5f;// shallow

    A *= -9.81f*GridScale;


    // painting
    bool isSingleDrop = KEY_SHIFT;
    
    if(iMouse.w > 0.0f || (!isSingleDrop && iMouse.z > 0.0f || (iMouse.x != iMouseLast.x && iMouse.y != iMouseLast.y)))
    {
        float2 c = PatchUVfromScreenUV(swi2(iMouse,x,y), iResolution);
        
        float2 vec = (uv0 - c);
  
        if(!isSingleDrop)
        if(iMouseLast.z > 0.0f || iMouseLast.w > 0.0f)
        {
            float2 c2 = PatchUVfromScreenUV(swi2(iMouseLast,x,y), iResolution);
            
            vec = uv0 - (c2 + (c-c2)*clamp01(dot(c-c2, uv0-c2)/dot(c-c2,c-c2)));
        }
  
        float v = _exp2f(-dot(vec, vec) * 1.0f);
        h12 = _mix(h12, to_float2_s(0.75f), v);
    }
    
    if(iFrame == 0) h12 = to_float2_s(0.0f);

    // rain drops
    if(KEY_N5 == false)
    if(WellonsHash((uint)(iFrame)) < 100000000u)
    {
        float2 c = Float01(WellonsHash((uint)(iFrame) * make_uint2(3242174893u, 2447445397u) + 3u)) * GridSize;
        float2 vec = (uv0 - c);
    
        float v = _exp2f(-dot(vec, vec) * 1.0f);
        h12 = _mix(h12, to_float2_s(0.75f), v);
    }

    float dt = 0.016667f;
    float dt2 =  dt * dt;

    float h0 = 0.0f;
    float h1 = h12.x;
    float h2 = h12.y;

#if 1
    // Verlet integration
    h0 = (2.0f * h1 - h2) + A * dt2;

#else

    // ...damped version
    float a = 1.0f/2.0f;
    float adt = a * dt;
    
    h0 = (((2.0f + adt) * h1 - h2) + A * dt2) / (1.0f + adt);

#endif

    float2 h01 = to_float2(h0, h1);

    // exponential state buffer smoothing
    float beta = 2.0f;
    h01 = _mix(h01, h12, 1.0f-_exp2f(-dt*beta));

    // mask out obstacles
    h01 *= mask;

    // grid windowing
    bool isGridWindowed = KEY_N2;
    if(isGridWindowed)
    {
        float r = 32.0f;

        float2 u = _fminf((to_float2_s(GridSize*0.5f) - abs_f2(uv0 - to_float2_s(GridSize*0.5f))) / r, to_float2_s(1.0f));

        u = 1.0f - u;
        u *= u;
        u *= u;
        u = 1.0f - u;

        float s = u.x*u.y;

        h01 *= _mix(0.75f, 1.0f, s);        
    }
    
    if(KEY_SPACE) { h01 *= 0.95f; } 
    
    col = to_float4(h01.x, h01.y, 0.0f, 0.0f);

  SetFragmentShaderComputedColor(col);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0f/)

/* horizontal pass of hq deep water kernel */

//#define FETCH(uv) texelFetch(iChannel0, uv, 0).r

__KERNEL__ void WatersurfacesimulationJipiFuse__Buffer_B(float4 col, float2 uv0, float2 iResolution, sampler2D iChannel0)
{

  uv0+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBB;
  #ifndef USE_HQ_KERNEL
    //discard;
    SetFragmentShaderComputedColor(col);
    return;
  #endif
  
    bool isGrid = uv0.x < GridSize && uv0.y < GridSize;

    if(!isGrid) {
      SetFragmentShaderComputedColor(col);
      return;
    }

    int2 uv = to_int2_cfloat(uv0 - 0.5f);
    
    float lowp3[4] = {5.0f/16.0f, 15.0f/64.0f, 3.0f/32.0f, 1.0f/64.0f};   
    float lowp7[8] = {429.0f/2048.0f, 3003.0f/16384.0f, 1001.0f/8192.0f, 1001.0f/16384.0f, 91.0f/4096.0f, 91.0f/16384.0f, 7.0f/8192.0f, 1.0f/16384.0f};
    float lapl7[8] = {3.22f, -1.9335988099476562f, 0.4384577800334821f, -0.1637450351359609f, 0.07015324480535713f, -0.02963974593026339f, 0.010609595665007715f, -0.0022370294899667453f};

    float lowpass3  = 0.0f;
    float lowpass7  = 0.0f;
    float laplacian = 0.0f;
    
    for(int x = -7; x <= 7; ++x)
    {
        float f = FETCH(uv + to_int2(x, 0));
    
        int i = _fabs(x);

        lowpass3  += f * (i < 4 ? lowp3[i] : 0.0f);
        lowpass7  += f * lowp7[i];
        laplacian += f * lapl7[i];
    }

    float f0 = FETCH(uv);
    
    col = to_float4(lowpass3, lowpass7, f0, laplacian);

  SetFragmentShaderComputedColor(col);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


// Lincense: CC0 (https://creativecommons.org/publicdomain/zero/1.0f/)

/*
    pre-filter pass (prepares height field for rendering)
    https://www.shadertoy.com/view/WtsBDH ("Bicubic C2 cont. Interpolation")
*/

//#define FETCH(uv) (texelFetch(iChannel0, uv, 0).r)

__KERNEL__ void WatersurfacesimulationJipiFuse__Buffer_C(float4 fragColor, float2 uv0, float2 iResolution, sampler2D iChannel0)
{
    uv0+=0.5f;
    
    if(uv0.x > GridSize || uv0.y > GridSize) 
    {
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
float CCCCCCCCCCCCCCCCCCCC;
    int2 uv = to_int2_cfloat(uv0 - 0.5f);
    
    float4 col = to_float4_s(0.0f);
    
    // ======================================================= GENERALIZED CUBIC BSPLINE =======================================================
    
    float kernD0[3];
    float kernD1[3];
    
    kernD0[0] = 2.0f/3.0f; kernD0[1] = 1.0f/6.0f; kernD0[2] = 0.0f;
    kernD1[0] =      0.0f; kernD1[1] =     -0.5f; kernD1[2] = 0.0f;
    
    float sw;// side lobes weight

    sw = 0.0f;// cubic BSpline
    
   #if 0
   
    sw = 0.25f;// similar to 1/3 but less ringing
    
   #elif 0
   
    sw = 1.0f/3.0f;// kernD0[0] == 1
    
   #elif 0
    
    sw = 0.186605f;// max abs derivative == 1
    
   #elif 0
    
    sw = 1.0f/6.0f;// maximaly flat pass band
    
   #elif 1
    
    sw = -0.25f;// spectrum falls off to 0 at Nyquist frequency
    
   #endif
    
    // add a pair of side lobes:
    kernD0[0] += 1.0f * sw; kernD0[1] += -1.0f/3.0f * sw; kernD0[2] += -1.0f/6.0f * sw;
                            kernD1[1] += -1.0f      * sw; kernD1[2] +=  0.5f      * sw;
    
    int r = sw == 0.0f ? 1 : 2;
    for(int j = -r; j <= r; ++j)
    for(int i = -r; i <= r; ++i)
    {
      float f = FETCH(uv + to_int2(i, j));
        
        int x = _fabs(i);
        int y = _fabs(j);
        
        float kAx = kernD0[x];
        float kAy = kernD0[y];
        
        float kBx = kernD1[x] * (i > 0 ? -1.0f : 1.0f);
        float kBy = kernD1[y] * (j > 0 ? -1.0f : 1.0f);
        
        col += f * to_float4(kBx * kAy, 
                             kAx * kBy, 
                             kBx * kBy,
                             kAx * kAy);
    }
    
    // col = to_float4(df/dx, df/dy, d^2f/dxy, f)
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Background' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel0



__DEVICE__ float kern_v(float x) { return 1.0f-x*x*(3.0f-2.0f*_fabs(x)); }
__DEVICE__ float kern_d(float x) { float o = _fabs(x)-1.0f; return x*(o*o); }

__DEVICE__ float kern_vD1(float x) { return x*(_fabs(x)*6.0f-6.0f); }
__DEVICE__ float kern_dD1(float x) { return (_fabs(x)-1.0f)*(_fabs(x)*3.0f-1.0f); }

__DEVICE__ float kern_vD2(float x) { return _fabs(x) * 12.0f - 6.0f; }
__DEVICE__ float kern_dD2(float x) { return x * 6.0f + (x > 0.0f ? -4.0f : 4.0f); }


__DEVICE__ float4 kern(float2 p)
{
    return to_float4(kern_d(p.x) * kern_v(p.y),
                     kern_v(p.x) * kern_d(p.y),
                     kern_d(p.x) * kern_d(p.y),
                     kern_v(p.x) * kern_v(p.y));
}

__DEVICE__ mat4 kern4x4(float2 p)
{
    float2 v   = to_float2(kern_v  (p.x), kern_v  (p.y));
    float2 d   = to_float2(kern_d  (p.x), kern_d  (p.y));
    
    float2 vD1 = to_float2(kern_vD1(p.x), kern_vD1(p.y));
    float2 dD1 = to_float2(kern_dD1(p.x), kern_dD1(p.y));
    
    mat4 m = to_mat4_f4
    (
        /*   kernDx         kernDy         kernDxy          kern    */
        to_float4(dD1.x * v.y  ,  d.x * vD1.y  ,  dD1.x * vD1.y  ,  d.x * v.y),
        to_float4(vD1.x * d.y  ,  v.x * dD1.y  ,  vD1.x * dD1.y  ,  v.x * d.y),
        to_float4(dD1.x * d.y  ,  d.x * dD1.y  ,  dD1.x * dD1.y  ,  d.x * d.y),
        to_float4(vD1.x * v.y  ,  v.x * vD1.y  ,  vD1.x * vD1.y  ,  v.x * v.y)
    );
    
    return m;
}

__DEVICE__ void kern4x4(float2 p, out mat4 *mA, out mat4 *mB)
{
    float2 v   = to_float2(kern_v  (p.x), kern_v  (p.y));
    float2 d   = to_float2(kern_d  (p.x), kern_d  (p.y));
    
    float2 vD1 = to_float2(kern_vD1(p.x), kern_vD1(p.y));
    float2 dD1 = to_float2(kern_dD1(p.x), kern_dD1(p.y));
float mmmmmmmmmmmmmmmm;    
    float2 vD2 = to_float2(kern_vD2(p.x), kern_vD2(p.y));
    float2 dD2 = to_float2(kern_dD2(p.x), kern_dD2(p.y));
    
    *mA = to_mat4_f4
    (
        /*   kernDx         kernDy         kernDxy          kern    */
        to_float4(dD1.x * v.y  ,  d.x * vD1.y  ,  dD1.x * vD1.y  ,  d.x * v.y),
        to_float4(vD1.x * d.y  ,  v.x * dD1.y  ,  vD1.x * dD1.y  ,  v.x * d.y),
        to_float4(dD1.x * d.y  ,  d.x * dD1.y  ,  dD1.x * dD1.y  ,  d.x * d.y),
        to_float4(vD1.x * v.y  ,  v.x * vD1.y  ,  vD1.x * vD1.y  ,  v.x * v.y)
    );

    *mB = to_mat4_f4
    (
        /*   kernDxx        kernDyy        kernDxxy         kernDxyy    */
        to_float4(dD2.x * v.y  ,  d.x * vD2.y  ,  dD2.x * vD1.y  ,  dD1.x * vD2.y),
        to_float4(vD2.x * d.y  ,  v.x * dD2.y  ,  vD2.x * dD1.y  ,  vD1.x * dD2.y),
        to_float4(dD2.x * d.y  ,  d.x * dD2.y  ,  dD2.x * dD1.y  ,  dD1.x * dD2.y),
        to_float4(vD2.x * v.y  ,  v.x * vD2.y  ,  vD2.x * vD1.y  ,  vD1.x * vD2.y)
    );
}


// BICUBIC SAMPLING ROUTINES =============================================================================================================

// this is the most basic version which only evaluates the function value
__DEVICE__ float SampleBicubic(__TEXTURE2D__ channel, float2 uv, float2 iResolution)
{
    uv -= to_float2_s(0.5f);
    
    float2 uvi = _floor(uv);
    float2 uvf = uv - uvi;

    int2 uv0 = to_int2_cfloat(uvi);
    
    float r = 0.0f;
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        float4 c = texture(channel, (make_float2(uv0 + to_int2(i, j))+0.5f)/iResolution);
        
        float2 l = uvf;
        
        if(i != 0) l.x -= 1.0f;
        if(j != 0) l.y -= 1.0f;
        
        r += dot(c, kern(l));
    }
    
  return r;
}

// ... this version also outputs derivatives (used here to compute normals)
__DEVICE__ float4 SampleBicubic2(__TEXTURE2D__ channel, float2 uv, float2 iResolution)
{
    uv -= to_float2_s(0.5f);
    
    float2 uvi = _floor(uv);
    float2 uvf = uv - uvi;

    int2 uv0 = to_int2_cfloat(uvi);
    
    float4 r = to_float4_s(0.0f);
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        float4 c = texture(channel, (make_float2(uv0 + to_int2(i, j))+0.5f)/iResolution);
        
        float2 l = uvf;
        
        if(i != 0) l.x -= 1.0f;
        if(j != 0) l.y -= 1.0f;
        
        r += mul_mat4_f4(kern4x4(l) , c);
    }
    
    // r = to_float4(df/dx, df/dy, ddf/dxy, f)
  return r;
}

// ... this version also outputs higher order derivatives (only used to debug C2 continuity here)
__DEVICE__ float4 SampleBicubic3(__TEXTURE2D__ channel, float2 uv, out float4 *d2, float2 iResolution)
{
    uv -= to_float2_s(0.5f);
    
    float2 uvi = _floor(uv);
    float2 uvf = uv - uvi;

    int2 uv0 = to_int2_cfloat(uvi);
    
    *d2 = to_float4_s(0.0f);
    float4 r = to_float4_s(0.0f);
    for(int j = 0; j < 2; ++j)
    for(int i = 0; i < 2; ++i)
    {
        float4 c = texture(channel, (make_float2(uv0 + to_int2(i, j))+0.5f)/iResolution);
        
        float2 l = uvf;
        
        if(i != 0) l.x -= 1.0f;
        if(j != 0) l.y -= 1.0f;
        
        mat4 mA, mB;
        kern4x4(l, /*out*/ &mA, &mB);
        
        r  += mul_mat4_f4(mA , c);
        *d2 += mul_mat4_f4(mB , c);
    }
    
    // r  = to_float4(  df/dx,   df/dy,  ddf/dxy ,         f)
    // *d2 = to_float4(ddf/dxx, ddf/dyy, dddf/dxxy, dddf/dxyy)
  return r;
}


// IMAGE ==========================================================================================================================

//__DEVICE__ float ReadKey(int keyCode) {return texelFetch(iChannel2, to_int2(keyCode, 0), 0).x;}
//__DEVICE__ float ReadKeyToggle(int keyCode) {return texelFetch(iChannel2, to_int2(keyCode, 2), 0).x;}

__KERNEL__ void WatersurfacesimulationJipiFuse(float4 fragColor, float2 uv0, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX7(Texture, 0);
  
    CONNECT_CHECKBOX0(KEY_N1, 0); //Hintergrundanimation - schwarze Felder werden animiert
    CONNECT_CHECKBOX1(KEY_N2, 0); // GridWindow -> keine sichtbaren Änderungen
    CONNECT_CHECKBOX2(KEY_N3, 0); // eine Minimap der Simulation wird eingeblendet
    CONNECT_CHECKBOX3(KEY_N4, 0); // in der Minimap wird die Höhe der Wellen gezeigt
    
    // Im Buffer A verwendet
    //CONNECT_CHECKBOX3(KEY_N5, 0);    // Tropfen an/aus
    //CONNECT_CHECKBOX5(KEY_SHIFT, 0); // Soll einzelne Tropfen bei der Mausverwendung erzeugen - hat aber keine Auswirkungen 
    //CONNECT_CHECKBOX6(KEY_SPACE, 0); // Wellenhöhe wird verstärkt ( h01 )
    
    
    CONNECT_COLOR0(Color1, 0.05f, 0.3f, 1.0f, 1.0f);
    CONNECT_SLIDER0(TexBright, -1.0f, 100.0f, 1.4f);
    CONNECT_SLIDER1(TexWave1, -1.0f, 30.0f, 12.0f);
    CONNECT_SLIDER2(TexWave2, -1.0f, 10.0f, 5.0f);
    
    CONNECT_SLIDER3(TexWaveOff, -1.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(TexWaveMul, -1.0f, 1.0f, 1.0f);
  
    #if 0
      fragColor = texture(iChannel1, (make_float2(uv0 + to_int2(i, j))+0.5f)/iResolution);
      SetFragmentShaderComputedColor(fragColor);
      return;
    #endif

    bool isTerrainAnimated = KEY_N1;

   #if 1
    // mini map
    bool doShowWaveField = KEY_N3;
   
    if(doShowWaveField)
    if(uv0.x < GridSize && uv0.y < GridSize)
    {
        float3 col = to_float3_s(1.0f) - normalize(to_float3_aw(swi2(texture(iChannel0, (make_float2(to_int2_cfloat(uv0 - 0.5f))+0.5f)/iResolution),x,y), 0.01f)).z;
        
        float d = EvalTerrainHeight(uv0, isTerrainAnimated ? iTime : 0.0f);
        
        bool doShowHeightField = KEY_N4;
        if(doShowHeightField) col = (swi3(texture(iChannel0, (make_float2(to_int2_cfloat(uv0 - 0.5f))+0.5f)/iResolution),w,w,w) * 1.0f + 0.5f);
        
        col *= col;

        float l = -_fminf(d, 0.0f)*3.0f;
        col = _mix(to_float3(0.125f, 0.125f, 1.0f ), col, 1.0f-(_exp2f(-(l*2.0f + l*l*1.0f))));
        col = _mix(col, to_float3(1.0f  , 0.0f  , 0.25f), smoothstep(-0.05f, 0.0f, d));

        bool isGridWindowSharp = KEY_N2;
        if(isGridWindowSharp && (uv0.x == 0.5f || uv0.y == 0.5f || uv0.x == GridSize-0.5f || uv0.y == GridSize-0.5f)) col = to_float3(0.0f, 1.0f, 1.0f); 

        fragColor = to_float4_aw(sqrt_f3(clamp01(swi3(col,x,y,z))), 0.0f);
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
   #endif

    float3 col;
    
    float2 uv = uv0;
    float2 tc = uv0 / swi2(iResolution,x,x);
    
   #if 0
    if(uv0.x >= iResolution.x*0.5f)
    uv.x -= iResolution.x*0.5f;
   #endif
    
    col = to_float3_s(texture(iChannel0, uv0/iResolution*0.125f).x);
    
    
    float2 uv2 = PatchUVfromScreenUV(swi2(uv0,x,y), iResolution);

    float time = isTerrainAnimated ? iTime : 0.0f;

    float3 V = to_float3(0.0f, 0.0f, 1.0f);
    float3 L = normalize(to_float3(1.0f, 1.0f, 1.0f));
    float3 L2 = normalize(to_float3(-1.0f, -1.0f, 2.0f));
    float3 H = normalize(L + V);


    float3 terrN;
    {
        float s = 1.0f/128.0f;
        
        float hx0 = EvalTerrainHeight(uv2 - to_float2(s, 0.0f), time);
        float hx1 = EvalTerrainHeight(uv2 + to_float2(s, 0.0f), time);
        float hy0 = EvalTerrainHeight(uv2 - to_float2(0.0f, s), time);
        float hy1 = EvalTerrainHeight(uv2 + to_float2(0.0f, s), time);
        
        float2 dxy = to_float2(hx1 - hx0, hy1 - hy0) / (2.0f * s);
        
        terrN = normalize(to_float3_aw(-dxy, 0.03f));
    }
    
    float3 terrCol = clamp01(dot(terrN, L))*(1.0f/(1.0f+1.0f*(1.0f-clamp01(dot(terrN, H)))))*to_float3_s(1.0f)*0.05f;

    float4 d2;
    float4 h = SampleBicubic3(iChannel0, uv2, &d2, iResolution);// sample water surface
    
    float nscale = 32.0f;
    float3 N = normalize(to_float3_aw(-1.0f*swi2(h,x,y) * nscale, 1.0f));
    
    float3 R = 2.0f*dot(V, N)*N - V;

    float ct = clamp01(dot(N, L));
    float ct2 = dot(N, L) * 0.5f + 0.5f;
    
    float d = h.w - EvalTerrainHeight(uv2-swi2(N,x,y)*4.0f, time);
    
    float waterMask = smoothstep(-0.01f, 0.01f, d);
    
    // diffuse
    float v =  clamp01(ct2+0.15f);
    v = 1.0f-v;
    v = cubic(v);
    
    float2 tuv = (uv0+((v+TexWaveOff)*TexWaveMul)) / iResolution;
    
    
    float4 tex = texture(iChannel2, tuv);//uv0/iResolution);
    
    //col = exp_f3(-(v * 12.0f + 5.0f) * to_float3(0.05f, 0.3f, 1.0f))*1.4f;
    col = exp_f3(-(v * 12.0f + 5.0f) * swi3(Color1,x,y,z))*1.4f;
    if(Texture)
      //col = 1.0f-exp_f3(-(v * 12.0f + 5.0f) * swi3(tex,x,y,z))*TexBright;//1.4f; // Naja
    //col = _expf(-(v * TexWave1 + TexWave2)) * swi3(tex,x,y,z)*TexBright;//1.4f; // Naja
    col = swi3(tex,x,y,z)*TexBright;//1.4f; // Naja
    
    
    //col *= _mix(0.25f, 1.0f, clamp01(dot(terrN, L)+0.5f));

    float l = _fmaxf(0.0f, d);
    col = _mix(terrCol, col, (1.0f-_exp2f(-(l*2.0f + l*l*1.0f))));
    col += to_float3(0.0f, 0.25f, 1.0f)*0.1f;
    
    // specular
    float c = 1.0f - (dot(R, L)*0.5f+0.5f);
    float c2 = 1.0f - (dot(R, L2)*0.5f+0.5f);
    float spec = 0.0f;
    spec += smoothstep(0.9f, 0.99f, dot(R, L))*0.5f; 
    float spec0 = spec;
    spec += smoothstep(0.7f, 0.9f, dot(R, L))*0.125f; 
    spec += smoothstep(0.8f, 0.9f, dot(R, L2))*0.02f; 
    spec += smoothstep(0.95f, 0.99f, N.z)*0.02f; 
    spec += _exp2f(-32.0f*(c))*0.25f;
    
    col += to_float3_s(1.0f) * spec * waterMask;
    
    col = GammaEncode(clamp01(col));
    
    fragColor = to_float4_aw(col, Color1.w);

  SetFragmentShaderComputedColor(fragColor);
}