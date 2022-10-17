
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define HALF_PI 1.57079632679f


// FBM implementation from
// https://github.com/MaxBittker/glsl-fractal-brownian-noise

__DEVICE__ float3 mod289(float3 x) {
  return x - _floor(x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 mod289(float4 x) {
  return x - _floor(x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 permute(float4 x) {
     return mod289(((x*34.0f)+1.0f)*x);
}

__DEVICE__ float4 taylorInvSqrt(float4 r)
{
  return to_float4_s(1.79284291400159f) - 0.85373472095314f * r;
}

__DEVICE__ float snoise(float3 v)
  {
  const float2  C = to_float2(1.0f/6.0f, 1.0f/3.0f) ;
  const float4  D = to_float4(0.0f, 0.5f, 1.0f, 2.0f);

// First corner
  float3 i  = _floor(v + dot(v, swi3(C,y,y,y)) );
  float3 x0 =   v - i + dot(i, swi3(C,x,x,x)) ;

// Other corners
  float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
  float3 l = 1.0f - g;
  float zzzzzzzzzzzzzzzzzz;
  float3 i1 = _fminf( swi3(g,x,y,z), swi3(l,z,x,y) );
  float3 i2 = _fmaxf( swi3(g,x,y,z), swi3(l,z,x,y) );

  //   x0 = x0 - 0.0f + 0.0f * swi3(C,x,x,x);
  //   x1 = x0 - i1  + 1.0f * swi3(C,x,x,x);
  //   x2 = x0 - i2  + 2.0f * swi3(C,x,x,x);
  //   x3 = x0 - 1.0f + 3.0f * swi3(C,x,x,x);
  float3 x1 = x0 - i1 + swi3(C,x,x,x);
  float3 x2 = x0 - i2 + swi3(C,y,y,y); // 2.0f*C.x = 1/3 = C.y
  float3 x3 = x0 - swi3(D,y,y,y);      // -1.0f+3.0f*C.x = -0.5f = -D.y

// Permutations
  i = mod289(i);
  float4 p = permute( permute( permute(
             i.z + to_float4(0.0f, i1.z, i2.z, 1.0f ))
           + i.y + to_float4(0.0f, i1.y, i2.y, 1.0f ))
           + i.x + to_float4(0.0f, i1.x, i2.x, 1.0f ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857f; // 1.0f/7.0
  float3  ns = n_ * swi3(D,w,y,z) - swi3(D,x,z,x);

  float4 j = p - 49.0f * _floor(p * ns.z * ns.z);  //  mod_f(p,7*7)

  float4 x_ = _floor(j * ns.z);
  float4 y_ = _floor(j - 7.0f * x_ );    // mod_f(j,N)

  float4 x = x_ *ns.x + swi4(ns,y,y,y,y);
  float4 y = y_ *ns.x + swi4(ns,y,y,y,y);
  float4 h = to_float4_s(1.0f) - abs_f4(x) - abs_f4(y);

  float4 b0 = to_float4_f2f2( swi2(x,x,y), swi2(y,x,y) );
  float4 b1 = to_float4_f2f2( swi2(x,z,w), swi2(y,z,w) );

  //vec4 s0 = to_float4_aw(lessThan(b0,0.0f))*2.0f - 1.0f;
  //vec4 s1 = to_float4_aw(lessThan(b1,0.0f))*2.0f - 1.0f;
  float4 s0 = _floor(b0)*2.0f + 1.0f;
  float4 s1 = _floor(b1)*2.0f + 1.0f;
  float4 sh = -1.0f*step(h, to_float4_s(0.0f));

  float4 a0 = swi4(b0,x,z,y,w) + swi4(s0,x,z,y,w)*swi4(sh,x,x,y,y) ;
  float4 a1 = swi4(b1,x,z,y,w) + swi4(s1,x,z,y,w)*swi4(sh,z,z,w,w) ;

  float3 p0 = to_float3_aw(swi2(a0,x,y),h.x);
  float3 p1 = to_float3_aw(swi2(a0,z,w),h.y);
  float3 p2 = to_float3_aw(swi2(a1,x,y),h.z);
  float3 p3 = to_float3_aw(swi2(a1,z,w),h.w);

//Normalise gradients
  float4 norm = taylorInvSqrt(to_float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  float4 m = _fmaxf(to_float4_s(0.6f) - to_float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), to_float4_s(0.0f));
  m = m * m;
  return 42.0f * dot( m*m, to_float4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
  }



__DEVICE__ float fbm3d(float3 x, const in int it) {
    float v = 0.0f;
    float a = 0.5f;
    float3 shift = to_float3_s(100);

    for (int i = 0; i < 32; ++i) {
        if(i<it) {
            v += a * snoise(x);
            x = x * 2.0f + shift;
            a *= 0.5f;
        }
    }
    return v;
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__KERNEL__ void PinkVoidJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  
  CONNECT_CHECKBOX0(AlphaOn, 0);
  CONNECT_SLIDER0(Alpha, -1.0f, 1.0f, 1.0f);

    float t = iTime * 0.2f;
    
    float2 uv = ( fragCoord -0.5f * iResolution ) / iResolution.y;
    float2 st = to_float2(
        length( uv ) * 1.5f,
        _atan2f( uv.y, uv.x )
    );
    
    st.y += st.x * 1.1f;
        
    float x = fbm3d(
        to_float3(
            _sinf( st.y ),
            _cosf( st.y ),
            _powf( st.x, 0.3f ) + t * 0.1f
        ),
        3
    );
  float y = fbm3d(
        to_float3(
            _cosf( 1.0f - st.y ),
            _sinf( 1.0f - st.y ),
            _powf( st.x, 0.5f ) + t * 0.1f
        ),
        4
    );
    
    float r = fbm3d(
        to_float3(
            x,
            y,
            st.x + t * 0.3f
        ),
        5
    );
    r = fbm3d(
        to_float3(
            r - x,
            r - y,
            r + t * 0.3f
        ),
        6
    );
    
    float c = ( r + st.x * 5.0f ) / 6.0f;
    
    fragColor = to_float4(
        smoothstep( 0.3f, 0.4f, c ),
        smoothstep( 0.4f, 0.55f, c ),
        smoothstep( 0.2f, 0.55f, c ),
        1.0f
    );

if(AlphaOn)
  //fragColor *= fragColor;
  fragColor.w *= clamp(fragColor.x*fragColor.y*fragColor.z, 0.0f,1.0f) + Alpha;

  SetFragmentShaderComputedColor(fragColor);
}