
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0



__DEVICE__ float2 mod289_f2(float2 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float3 mod289_f3(float3 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float4 mod289_f4(float4 x) { return x - _floor(x * (1.0f / 289.0f)) * 289.0f; }
__DEVICE__ float3 permute_f3(float3 x) { return mod289_f3(((x*34.0f)+1.0f)*x); }
__DEVICE__ float4 permute_f4(float4 x) { return mod289_f4(((x*34.0f)+1.0f)*x); }

  __DEVICE__ float4 taylorInvSqrt(float4 r)
  {
    return to_float4_s(1.79284291400159f) - 0.85373472095314f * r;
  }

  __DEVICE__ float snoise_f3(float3 v)
    {
    const float2  C = to_float2(1.0f/6.0f, 1.0f/3.0f) ;
    const float4  D = to_float4(0.0f, 0.5f, 1.0f, 2.0f);

  // First corner
    float3 i  = _floor(v + dot(v, swi3(C,y,y,y)) );
    float3 x0 =   v - i + dot(i, swi3(C,x,x,x)) ;

  // Other corners
    float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
    float3 l = 1.0f - g;
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
    i = mod289_f3(i);
    float4 p = permute_f4( permute_f4( permute_f4(
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
float zzzzzzzzzzzzzzzz;
  // Mix final noise value
    float4 m = _fmaxf(to_float4_s(0.6f) - to_float4( dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), to_float4_s(0.0f));
    m = m * m;
    return 42.0f * dot( m*m, to_float4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
  }

  __DEVICE__ float snoise_f2(float2 v) {

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
      i = mod289_f2(i);
      float3 p = permute_f3(
                 permute_f3( i.y + to_float3(0.0f, i1.y, 1.0f))
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
      swi2S(g,y,z, swi2(a0,y,z) * to_float2(x1.x,x2.x) + swi2(h,y,z) * to_float2(x1.y,x2.y));
      return 130.0f * dot(m, g);
  }

  __DEVICE__ float turbulence (in float2 st, in int OCTAVES, in float2 parallax, in float evolution) {
      // Initial values
      float value = 0.0f;
      float amplitude = 0.5f;
      float frequency = 0.0f;
      //
      // Loop of octaves
      for (int i = 0; i < OCTAVES; i++) {
          value += amplitude * _fabs(snoise_f3(to_float3_aw(st,evolution)));
          st *= 2.0f;
          st += parallax;
          amplitude *= 0.5f;
      }
      return value;
  }





__KERNEL__ void TurbulenceJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
	#ifdef ORG
	float evolution =0.0f;//-50~50
	float seed = 0.0f;//0~5
	float intensity = 0.85f;//0  1
	float octaves = 5.0f;//0 8
	float2 position = to_float2(250.0f,250.0f);//0~500.0
    #endif
	
	CONNECT_SLIDER0(evolution, -50.0f, 50.0f, 0.0f);
	CONNECT_SLIDER1(seed, -10.0f, 10.0f, 0.0f);
	CONNECT_SLIDER2(intensity, -1.0f, 1.0f, 0.85f);
	CONNECT_SLIDER3(octaves, -10.0f, 10.0f, 5.0f);
	//CONNECT_POINT0(position, 250.0f, 250.0f );
  CONNECT_POINT0(_Position, 0.0f, 0.0f );
  
  float2 position = to_float2_s(250.0f) + _Position*iResolution;


    float2 uv =  fragCoord / iResolution;
    float2 st =  fragCoord / iResolution;
    st.x *= iResolution.x/iResolution.y;
    st += position*to_float2(-1.0f,1.0f)/500.0f - 0.5f;

    float2 parallax = to_float2(0.0f,0.0f);
   
    float p = 0.0f;
    p += turbulence(st, (int)(octaves), parallax, evolution + 7391.935f*seed);
    p *= intensity;
    uv.x += p;
    
    
    float2 parallay = to_float2(0.0f,0.0f);
    float q = 0.0f;
    q += turbulence(st, (int)(octaves), parallay, evolution + 7391.935f*seed);
    q *= intensity;
    uv.y += q;

    float4 texColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    fragColor = texColor;
  
  SetFragmentShaderComputedColor(fragColor);
}