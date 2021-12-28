
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//__DEVICE__ float3 fract_f3(float3 A){return make_float3(A.x - _floor(A.x), A.y - _floor(A.y), A.z - _floor(A.z));}
__DEVICE__ float3 abs_f3(float3 a) {return (to_float3(_fabs(a.x), _fabs(a.y),_fabs(a.z)));}

#define PI 3.1415926538

__DEVICE__ float2 rot(float2 uv,float a,float2 origin){
    
    float2 p = (uv - origin);
    
    float c = _cosf(a);
    float s = _sinf(a);
    
  return mul_mat2_f2(to_mat2(c,-s,s,c),p) + origin;
}

__DEVICE__ float random (in float2 _st) {
    return fract(_sinf(dot(swi2(_st,x,y),
                 to_float2(12.9898f,78.233f)))*
                 43758.5453123f);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
__DEVICE__ float noise (in float2 _st) {
    float2 i = _floor(_st);
    float2 f = fract_f2(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + to_float2(1.0f, 0.0f));
    float c = random(i + to_float2(0.0f, 1.0f));
    float d = random(i + to_float2(1.0f, 1.0f));

    float2 u = f * f * (3.0f - 2.0f * f);

    return _mix(a, b, u.x) +
           (c - a)* u.y * (1.0f - u.x) +
           (d - b) * u.x * u.y;
}

#define NUM_OCTAVES 2

__DEVICE__ float fbm ( in float2 _st) {
    float v = 0.0f;
    float a = 0.5f;
    float2 shift = to_float2_s(100.0f);
    // Rotate to reduce axial bias
    mat2 rot = to_mat2(_cosf(0.5f), _sinf(0.5f),
                      -_sinf(0.5f), _cosf(0.50f));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = mul_mat2_f2(rot , _st * 2.0f) + shift;
        a *= 0.5f;
    }
    return v;
}

__DEVICE__ float3 mod289(float3 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float2 mod289(float2 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float3 permute(float3 x) { return mod289(((x*34.0f)+1.0f)*x); }

__DEVICE__ float snoise(float2 v) {

    // Precompute values for skewed triangular grid
    const float4 C = to_float4(0.211324865405187f,
                               // (3.0f-_sqrtf(3.0f))/6.0
                               0.366025403784439f,
                               // 0.5f*(_sqrtf(3.0f)-1.0f)
                               -0.577350269189626f,
                               // -1.0f + 2.0f * C.x
                               0.024390243902439f);
                               // 1.0f / 41.0

    // First corner (x0)
    float2 i  = _floor(v + dot(v, swi2(C,y,y)));
    float2 x0 = v - i + dot(i, swi2(C,x,x));

    // Other two corners (x1, x2)
    float2 i1 = to_float2_s(0.0f);
    i1 = (x0.x > x0.y)? to_float2(1.0f, 0.0f):to_float2(0.0f, 1.0f);
    float2 x1 = swi2(x0,x,y) + swi2(C,x,x) - i1;
    float2 x2 = swi2(x0,x,y) + swi2(C,z,z);

    // Do some permutations to avoid
    // truncation effects in permutation
    i = mod289(i);
    float3 p = permute(
               permute( i.y + to_float3(0.0f, i1.y, 1.0f))
               + i.x + to_float3(0.0f, i1.x, 1.0f ));

    float3 m = _fmaxf(0.5f - to_float3(
                                       dot(x0,x0),
                                       dot(x1,x1),
                                       dot(x2,x2)
                                       ), to_float3_s(0.0f));

    m = m*m ;
    m = m*m ;

    // Gradients:
    //  41 pts uniformly over a line, mapped onto a diamond
    //  The ring size 17*17 = 289 is close to a multiple
    //      of 41 (41*7 = 287)

    float3 x = 2.0f * fract_f3(p * swi3(C,w,w,w)) - 1.0f;
    float3 h = abs_f3(x) - 0.5f;
    float3 ox = _floor(x + 0.5f);
    float3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt(a0*a0 + h*h);
    m *= 1.79284291400159f - 0.85373472095314f * (a0*a0+h*h);

    // Compute final noise value at P
    float3 g = to_float3_s(0.0f);
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    //swi2(g,y,z) = swi2(a0,y,z) * to_float2(x1.x,x2.x) + swi2(h,y,z) * to_float2(x1.y,x2.y);
    float2 gyz = swi2(a0,y,z) * to_float2(x1.x,x2.x) + swi2(h,y,z) * to_float2(x1.y,x2.y);
    g.y=gyz.x;g.z=gyz.y;
    
    return 130.0f * dot(m, g);
   
}

#define NUM_OCTAVES2 2

__DEVICE__ float fbms ( in float2 _st) {
    float v = 0.0f;
    float a = 0.5f;
    float2 shift = to_float2_s(100.0f);
    // Rotate to reduce axial bias
    mat2 rot = to_mat2(_cosf(0.5f), _sinf(0.5f),
                       -_sinf(0.5f), _cosf(0.50f));
    for (int i = 0; i < NUM_OCTAVES2; ++i) {
        v += a * snoise(_st);
        _st = mul_mat2_f2(rot , _st * 2.0f) + shift;
        a *= 0.5f;
    }
    return v;
}

//************************************************************************************************
__KERNEL__ void RotationLFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 R = iResolution;
    float2 uv = fragCoord/iResolution;
    uv.x *= R.x/R.y;
    
    uv *= 1.0f;
    
    float sc = 5.0f;
    
    //uv.x += iTime/10.0f;
    
    float ys = smoothstep(0.7f,0.5f,uv.y);
    
    float a = (fbm(uv*sc + to_float2(0.0f,-iTime))*2.0f - 1.0f);
    a = snoise(uv*2.0f+ to_float2(0.0f,-iTime))/1.5f;
    a = (fbms(uv*sc + to_float2(0.0f,-iTime)));
    
    uv = rot(uv,a,(uv - fbms(uv)));

    float l = smoothstep(0.49f,0.51f,uv.y);

    float3 col = to_float3_s(l);

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}


