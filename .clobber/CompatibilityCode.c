

#if defined(DEVICE_IS_METAL)
  #define in
  #define out thread
  #define inout thread
#else
  #define in
  #define out
  #define inout
#endif


#undef USE_NATIVE_METAL_IMPL
#undef USE_NATIVE_CUDA_IMPL
#undef USE_NATIVE_OPENCL_IMPL

  // 0 to use the generic implementations; 1 for Metal, OpenCL, Cuda specific code if existing

  #if 1
    #if defined(DEVICE_IS_METAL)
      #define USE_NATIVE_METAL_IMPL   1
    #elif defined(DEVICE_IS_CUDA)
      #define USE_NATIVE_CUDA_IMPL    1
    #elif defined(DEVICE_IS_OPENCL)
      #define USE_NATIVE_OPENCL_IMPL  1
    #endif
  #endif

  #if defined(USE_NATIVE_METAL_IMPL)

    #define swixz(V) (V).xz
    #define swiyz(V) (V).yz

    #define swixy(V) (V).xy
    #define swiyx(V) (V).yx
    #define swixxy(V) (V).xxy
    #define swixyx(V) (V).xyx
    #define swiyxz(V) (V).yxz
    #define swiyzz(V) (V).yzz
    #define swizyx(V) (V).zyx

    #define swixyz(V) (V).xyz
    #define swixxx(V) (V).xxx
    #define swiwww(V) (V).www

    #define swizxy(V) (V).zxy

  #else
    #define swixz(V) to_float2((V).x,(V).z)
    #define swiyz(V) to_float2((V).x,(V).z)

    #define swixy(V) to_float2((V).x,(V).y)
    #define swiyx(V) to_float2((V).y,(V).x)
    #define swixxy(V) to_float3((V).x,(V).x,(V).y)
    #define swixyx(V) to_float3((V).x,(V).y,(V).x)
    #define swiyxz(V) to_float3((V).y,(V).x,(V).z)
    #define swiyzz(V) to_float3((V).y,(V).z,(V).z)
    #define swizyx(V) to_float3((V).z,(V).y,(V).x)

    #define swixyz(V) to_float3((V).x,(V).y,(V).z)
    #define swixxx(V) to_float3((V).x,(V).x,(V).x)
    #define swiwww(V) to_float3((V).w,(V).w,(V).w)

    #define swizxy(V) to_float3((V).z,(V).x,(V).y)
  #endif

  #if defined(USE_NATIVE_METAL_IMPL)
    __DEVICE__ inline float length1f      ( float  x ) { return abs(x);    }
    __DEVICE__ inline float length_float3 ( float3 v ) { return length(v); }
    __DEVICE__ inline float3 fract_float3(float3 v) { return fract(v); }
    __DEVICE__ inline float4 pow_float4(float4 a, float4 b) { return pow(a,b); }
  #else
    __DEVICE__ inline float length1f      ( float  x ) { return _fabs(x);                                 }
    __DEVICE__ inline float length_float3 ( float3 v ) { return _sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);          }
    __DEVICE__ inline float3 fract_float3(float3 v)          { return to_float3(v.x - _floor(v.x), v.y - _floor(v.y), v.z - _floor(v.z)                   ); }
    __DEVICE__ inline float4 pow_float4(float4 a, float4 b) { return to_float4( pow(a.x,b.x),pow(a.y,b.y),pow(a.z,b.z),pow(a.w,b.w) ); }
  #endif




// ----------------------------------------------------------------------------------------------------------
// mat2 implementation
// ----------------------------------------------------------------------------------------------------------

#if defined(USE_NATIVE_METAL_IMPL)

  typedef float2x2 mat2;

  __DEVICE__ inline mat2 to_mat2    ( float  a, float  b, float c, float d) { return mat2(a,b,c,d);       }
  __DEVICE__ inline mat2 to_mat2_1f ( float  a                            ) { return mat2(a,a,a,a);       }
  __DEVICE__ inline mat2 to_mat2_s  ( float  a                            ) { return mat2(a);             }
  __DEVICE__ inline mat2 to_mat2_f2_f2 ( float2 a, float2 b                  ) { return mat2(a,b);           }
  __DEVICE__ inline mat2 to_mat2_1f_f3 ( float  a, float3 b                  ) { return mat2(a,b.x,b.y,b.z); }
  __DEVICE__ inline mat2 to_mat2_f3_1f ( float3 a, float  b                  ) { return mat2(a.x,a.y,a.z,b); }


  __DEVICE__ inline mat2    prod_mat2_mat2  ( mat2   a, mat2   b )  { return a*b; }
  __DEVICE__ inline float2  prod_f2_mat2    ( float2 v, mat2   m )  { return v*m; }
  __DEVICE__ inline float2  prod_mat2_f2    ( mat2   m, float2 v )  { return m*v; }
  __DEVICE__ inline mat2    prod_mat2_1f    ( mat2   m, float  s )  { return m*s; }
  __DEVICE__ inline mat2    prod_1f_mat2    ( float  s, mat2   m )  { return s*m; }

#else

  typedef struct { float2 r0; float2 r1; } mat2;

  __DEVICE__ inline mat2 to_mat2    ( float  a, float  b, float c, float d)  { mat2 t; t.r0.x = a; t.r0.y = b; t.r1.x = c; t.r1.y = d;         return t; }
  __DEVICE__ inline mat2 to_mat2_1f ( float  a                            )  { mat2 t; t.r0.x = a; t.r0.y = a; t.r1.x = a; t.r1.y = a;         return t; }
  __DEVICE__ inline mat2 to_mat2_s  ( float  a                            )  { mat2 t; t.r0.x = a;  t.r0.y = 0.0f; t.r1.x = 0.0f; t.r1.y = a;  return t; }
  __DEVICE__ inline mat2 to_mat2_f2_f2 ( float2 a, float2 b                  )  { mat2 t; t.r0 = a; t.r1 = b;                                     return t; }
  __DEVICE__ inline mat2 to_mat2_1f_f3 ( float  a, float3 b                  )  { mat2 t; t.r0.x = a; t.r0.y = b.x; t.r1.x = b.y; t.r1.y = b.z;   return t; }
  __DEVICE__ inline mat2 to_mat2_f3_1f ( float3 a, float  b                  )  { mat2 t; t.r0.x = a.x; t.r0.y = a.y; t.r1.x = a.z; t.r1.y = b;   return t; }


  __DEVICE__ inline mat2 prod_mat2_mat2( mat2 a, mat2 b)
  {
    mat2 t;
    t.r0.x = a.r0.x * b.r0.x + a.r0.y * b.r1.x;   t.r0.y = a.r0.x * b.r0.y + a.r0.y * b.r1.y;
    t.r1.x = a.r1.x * b.r0.x + a.r1.y * b.r1.x;   t.r1.y = a.r1.x * b.r0.y + a.r1.y * b.r1.y;
    return t;
  }


  __DEVICE__ inline float2 prod_float2_mat2( float2 v, mat2 m )
  {
    float2 t; t.x = v.x*m.r0.x + v.y*m.r0.y; t.y = v.x*m.r1.x + v.y*m.r1.y; return t;
  }


  __DEVICE__ inline float2 prod_mat2_float2( mat2 m, float2 v )
  {
    float2 t; t.x = v.x*m.r0.x + v.y*m.r1.x; t.y = v.x*m.r0.y + v.y*m.r1.y; return t;
  }


  __DEVICE__ inline mat2 prod_mat2_1f( mat2 m, float s)
  {
    mat2 t;
    t.r0.x = s * m.r0.x; t.r0.y = s * m.r0.y;
    t.r1.x = s * m.r1.x; t.r1.y = s * m.r1.y;
    return t;
  }

  __DEVICE__ inline mat2 prod_1f_mat2( float s, mat2 m) { return prod_mat2_1f(m,s); }

#endif // end of mat2 implementation


// ----------------------------------------------------------------------------------------------------------
// mat3 implementation
// ----------------------------------------------------------------------------------------------------------

#if defined(USE_NATIVE_METAL_IMPL)

  typedef float3x3 mat3;

  __DEVICE__ inline mat3 to_mat3    ( float a, float b, float c, float d, float e, float f, float g, float h, float i)
  {
    return mat3(a,b,c,d,e,f,g,h,i);
  }

  __DEVICE__ inline mat3 to_mat3_1f ( float a ) { return mat3(a,a,a,a,a,a,a,a,a); }

#else

  typedef struct { float3 r0; float3 r1; float3 r2; } mat3;

  __DEVICE__ inline mat3 to_mat3    ( float  a, float  b, float c,   float d, float e, float f,   float g, float h, float i)
  {
    mat3 t;
    t.r0.x = a; t.r0.y = b; t.r0.z = c;
    t.r1.x = d; t.r1.y = e; t.r1.z = f;
    t.r2.x = g; t.r2.y = h; t.r2.z = i;
    return t;
  }

  __DEVICE__ inline mat3 to_mat3_1f ( float  a )
  {
    mat3 t;
    t.r0.x = t.r0.y = t.r0.z = t.r1.x = t.r1.y = t.r1.z = t.r2.x = t.r2.y = t.r2.z = a;
    return t;
  }


#endif // end of mat2 implementation






#if defined(USE_NATIVE_METAL_IMPL)

  #define fract_f(A)  fract(A)
  #define fract_f2(A) fract(A)
  #define fract_f3(A) fract(A)
  #define fract_f4(A) fract(A)

  #define mod_f(a,b)  mod((a),(b))
  #define mod_f2(value,divisor) mod(value,divisor)

#else

  #define radians(a) ((a) * M_PI/180.0f)

  #if defined(DEVICE_IS_CUDA)
    #define fract(a) ((a)-_floor(a))
  #endif

  #define fract_f(A)  fract(A)
  #define fract_f2(A) to_float2(fract((A).x),fract((A).y))
  #define fract_f3(A) to_float3(fract((A).x),fract((A).y),fract((A).z))
  #define fract_f4(A) to_float4(fract((A).x),fract((A).y),fract((A).z,fract((A).w))

  #define mod_f(a,b) ((a)-(b)*_floor((a)/(b)))
  #define mod_f2(value,divisor) to_float2(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)))

#endif
