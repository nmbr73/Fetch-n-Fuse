
  #define SHADER_PREAMBLE                                                                           \
    DEFINE_KERNEL_ITERATORS_XY(x, y);                                                               \
    if (x >= params->width || y >= params->height)                                                  \
      return;                                                                                       \
                                                                                                    \
    float2 iResolution = to_float2(params->width, params->height);                                  \
    float  iTime       = params->iTime * params->frequency;                                         \
    float2 fragCoord   = to_float2(x, y);                                                           \
    float4 iMouse      = to_float4(params->mouse_x,params->mouse_y,params->mouse_z,params->mouse_w); \
    float4 fragColor   = to_float4_s(0.0f);

   #define SHADER_EPILOGUE \
    _tex2DVec4Write(dst, x, y, fragColor);

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
    #define swixy(V) (V).xy
    #define swiyx(V) (V).yx
    #define swixxy(V) (V).xxy
    #define swixyx(V) (V).xyx
    #define swiyxz(V) (V).yxz
    #define swiyzz(V) (V).yzz
    #define swizyx(V) (V).zyx
  #else
    #define swixy(V) to_float2((V).x,(V).y)
    #define swiyx(V) to_float2((V).y,(V).x)
    #define swixxy(V) to_float3((V).x,(V).x,(V).y)
    #define swixyx(V) to_float3((V).x,(V).y,(V).x)
    #define swiyxz(V) to_float3((V).y,(V).x,(V).z)
    #define swiyzz(V) to_float3((V).y,(V).z,(V).z)
    #define swizyx(V) to_float3((V).z,(V).y,(V).x)
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

  #if defined(USE_NATIVE_METAL_IMPL)

    typedef float2x2 mat2;

    __DEVICE__ inline mat2    to_mat2(float  a, float  b, float c, float d) { return mat2(a,b,c,d); }
    __DEVICE__ inline float2  prod_float2_mat2(float2 v, mat2   m )  { return v*m; }

  #else

    typedef struct { float2 r0; float2 r1; } mat2;

    __DEVICE__ inline mat2   to_mat2(float  a, float  b, float c, float d)  { mat2 t; t.r0.x = a; t.r0.y = b; t.r1.x = c; t.r1.y = d; return t; }
    __DEVICE__ inline float2 prod_float2_mat2(float2 v, mat2 m) { float2 t; t.x = v.x*m.r0.x + v.y*m.r0.y; t.y = v.x*m.r1.x + v.y*m.r1.y; return t; }

  #endif // end of mat2 implementation
