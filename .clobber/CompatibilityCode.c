

#if defined(DEVICE_IS_METAL)
  #define in
  #define out thread
  #define inout thread
#else
  #define in
  #define out
  #define inout
#endif

/*| POINTERPARAMETER |*/#if defined(DEVICE_IS_METAL)
/*| POINTERPARAMETER |*/  #define POINTERPARAMETER thread
/*| POINTERPARAMETER |*/#else
/*| POINTERPARAMETER |*/  #define POINTERPARAMETER
/*| POINTERPARAMETER |*/#endif


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

/*| swi2          |*/    #define swi2(A,a,b)     (A).a##b
/*| swi3          |*/    #define swi3(A,a,b,c)   (A).a##b##c
/*| swi4          |*/    #define swi4(A,a,b,c,d) (A).a##b##c##d

/*| swi2S         |*/    #define swi2S(a,b,c,d)   #define swi2S(a,b,c,d) a.b##c = d
/*| swi3S         |*/    #define swi3S(a,b,c,d,e) #define swi3S(a,b,c,d,e) a.b##c##d = e

  #else

/*| swi2          |*/    #define swi2(A,a,b)     to_float2((A).a,(A).b)
/*| swi3          |*/    #define swi3(A,a,b,c)   to_float3((A).a,(A).b,(A).c)
/*| swi4          |*/    #define swi4(A,a,b,c,d) to_float4((A).a,(A).b,(A).c,(A).d)

/*| swi2S         |*/    #define swi2S(a,b,c,d)   {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 
/*| swi3S         |*/    #define swi3S(a,b,c,d,e) {float3 tmp = e; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z;} 

  #endif




/*| mat2          |*/// ----------------------------------------------------------------------------------------------------------
/*| mat2          |*/// mat2 implementation
/*| mat2          |*/// ----------------------------------------------------------------------------------------------------------
/*| mat2          |*/
/*| mat2          |*/#if defined(USE_NATIVE_METAL_IMPL)
/*|               |*/
/*| mat2          |*/  typedef float2x2 mat2;
/*|               |*/
/*| to_mat2       |*/  #define to_mat2(A,B,C,D)   mat2((A),(B),(C),(D))
/*| to_mat2_f     |*/  #define to_mat2_f(A)       mat2((A),(A),(A),(A))
/*| to_mat2_s     |*/  #define to_mat2_s(A)       mat2(A)
/*| to_mat2_f2_f2 |*/  #define to_mat2_f2_f2(A,B) mat2((A),(B))
/*| to_mat2_f_f3  |*/  #define to_mat2_f_f3(A,B)  mat2((A),(B).x,(B).y,(B).z)
/*| to_mat2_f3_f  |*/  #define to_mat2_f3_f(A,B)  mat2((A).x,(A).y,(A).z,(B))
/*|               |*/
/*| mul_mat2_mat2 |*/  #define mul_mat2_mat2(A,B) ((A)*(B))
/*| mul_f2_mat2   |*/  #define mul_f2_mat2(A,B)   ((A)*(B))
/*| mul_mat2_f2   |*/  #define mul_mat2_f2(A,B)   ((A)*(B))
/*| mul_mat2_f    |*/  #define mul_mat2_f(A,B)    ((A)*(B))
/*| mul_f_mat2    |*/  #define mul_f_mat2(A,B)    ((A)*(B))
/*|               |*/
/*| mat2          |*/#else
/*|               |*/
/*| mat2          |*/  typedef struct { float2 r0; float2 r1; } mat2;
/*|               |*/
/*| to_mat2       |*/  __DEVICE__ inline mat2 to_mat2      ( float  a, float  b, float c, float d)  { mat2 t; t.r0.x = a; t.r0.y = b; t.r1.x = c; t.r1.y = d;         return t; }
/*| to_mat2_f     |*/  __DEVICE__ inline mat2 to_mat2_f    ( float  a                            )  { mat2 t; t.r0.x = a; t.r0.y = a; t.r1.x = a; t.r1.y = a;         return t; }
/*| to_mat2_s     |*/  __DEVICE__ inline mat2 to_mat2_s    ( float  a                            )  { mat2 t; t.r0.x = a;  t.r0.y = 0.0f; t.r1.x = 0.0f; t.r1.y = a;  return t; }
/*| to_mat2_f2_f2 |*/  __DEVICE__ inline mat2 to_mat2_f2_f2( float2 a, float2 b                  )  { mat2 t; t.r0 = a; t.r1 = b;                                     return t; }
/*| to_mat2_f_f3  |*/  __DEVICE__ inline mat2 to_mat2_f_f3 ( float  a, float3 b                  )  { mat2 t; t.r0.x = a; t.r0.y = b.x; t.r1.x = b.y; t.r1.y = b.z;   return t; }
/*| to_mat2_f3_f  |*/  __DEVICE__ inline mat2 to_mat2_f3_f ( float3 a, float  b                  )  { mat2 t; t.r0.x = a.x; t.r0.y = a.y; t.r1.x = a.z; t.r1.y = b;   return t; }
/*|               |*/
/*| mul_mat2_mat2 |*/  __DEVICE__ inline mat2 mul_mat2_mat2( mat2 a, mat2 b)
/*| mul_mat2_mat2 |*/  {
/*| mul_mat2_mat2 |*/    mat2 t;
/*| mul_mat2_mat2 |*/    t.r0.x = a.r0.x * b.r0.x + a.r0.y * b.r1.x;   t.r0.y = a.r0.x * b.r0.y + a.r0.y * b.r1.y;
/*| mul_mat2_mat2 |*/    t.r1.x = a.r1.x * b.r0.x + a.r1.y * b.r1.x;   t.r1.y = a.r1.x * b.r0.y + a.r1.y * b.r1.y;
/*| mul_mat2_mat2 |*/    return t;
/*| mul_mat2_mat2 |*/  }
/*|               |*/
/*| mul_f2_mat2   |*/  __DEVICE__ inline float2 mul_f2_mat2( float2 v, mat2 m )
/*| mul_f2_mat2   |*/  {
/*| mul_f2_mat2   |*/    float2 t; t.x = v.x*m.r0.x + v.y*m.r0.y; t.y = v.x*m.r1.x + v.y*m.r1.y; return t;
/*| mul_f2_mat2   |*/  }
/*|               |*/
/*| mul_mat2_f2   |*/  __DEVICE__ inline float2 mul_mat2_f2( mat2 m, float2 v )
/*| mul_mat2_f2   |*/  {
/*| mul_mat2_f2   |*/    float2 t; t.x = v.x*m.r0.x + v.y*m.r1.x; t.y = v.x*m.r0.y + v.y*m.r1.y; return t;
/*| mul_mat2_f2   |*/  }
/*|               |*/
/*| mul_mat2_f    |*/  __DEVICE__ inline mat2 mul_mat2_f( mat2 m, float s)
/*| mul_mat2_f    |*/  {
/*| mul_mat2_f    |*/    mat2 t;
/*| mul_mat2_f    |*/    t.r0.x = s * m.r0.x; t.r0.y = s * m.r0.y;
/*| mul_mat2_f    |*/    t.r1.x = s * m.r1.x; t.r1.y = s * m.r1.y;
/*| mul_mat2_f    |*/    return t;
/*| mul_mat2_f    |*/  }
/*|               |*/
/*| mul_f_mat2    |*/  __DEVICE__ inline mat2 mul_f_mat2( float s, mat2 m) { return mul_mat2_f(m,s); }
/*|               |*/
/*| mat2          |*/#endif // end of mat2 implementation


/*| mat3          |*/// ----------------------------------------------------------------------------------------------------------
/*| mat3          |*/// mat3 implementation
/*| mat3          |*/// ----------------------------------------------------------------------------------------------------------
/*|               |*/
/*| mat3          |*/#if defined(USE_NATIVE_METAL_IMPL)
/*| mat3          |*/
/*| mat3          |*/  typedef float3x3 mat3;
/*| mat3          |*/
/*| to_mat3       |*/  __DEVICE__ inline mat3 to_mat3( float a, float b, float c, float d, float e, float f, float g, float h, float i)
/*| to_mat3       |*/  {
/*| to_mat3       |*/    return mat3(a,b,c,d,e,f,g,h,i);
/*| to_mat3       |*/  }
/*|               |*/
/*| to_mat3_f     |*/  __DEVICE__ inline mat3 to_mat3_f( float a ) { return mat3(a,a,a,a,a,a,a,a,a); }
/*| to_mat3_f3    |*/  __DEVICE__ inline mat3 to_mat3_f3( float3 a, float3 b, float3 c ) { return mat3(a,b,c); }
/*| mul_mat3_f3   |*/  __DEVICE__ inline float3 mul_mat3_f3( mat3 B, float3 A) { return (B*A); }
/*| mul_f3_mat3   |*/  __DEVICE__ inline float3 mul_f3_mat3( float3 A, mat3 B) { return (A*B); }
/*| mul_mat3_mat3 |*/  __DEVICE__ inline mat3 mul_mat3_mat3( mat3 A, mat3 B) { return (A*B); }
/*|               |*/
/*| mat3          |*/#else
/*|               |*/
/*| mat3          |*/  typedef struct { float3 r0; float3 r1; float3 r2; } mat3;
/*|               |*/
/*| to_mat3       |*/  __DEVICE__ inline mat3 to_mat3( float  a, float  b, float c,   float d, float e, float f,   float g, float h, float i)
/*| to_mat3       |*/  {
/*| to_mat3       |*/    mat3 t;
/*| to_mat3       |*/    t.r0.x = a; t.r0.y = b; t.r0.z = c;
/*| to_mat3       |*/    t.r1.x = d; t.r1.y = e; t.r1.z = f;
/*| to_mat3       |*/    t.r2.x = g; t.r2.y = h; t.r2.z = i;
/*| to_mat3       |*/    return t;
/*| to_mat3       |*/  }
/*|               |*/
/*| to_mat3_f     |*/  __DEVICE__ inline mat3 to_mat3_f( float  a )
/*| to_mat3_f     |*/  {
/*| to_mat3_f     |*/    mat3 t;
/*| to_mat3_f     |*/    t.r0.x = t.r0.y = t.r0.z = t.r1.x = t.r1.y = t.r1.z = t.r2.x = t.r2.y = t.r2.z = a;
/*| to_mat3_f     |*/    return t;
/*| to_mat3_f     |*/  }
/*|               |*/
/*| to_mat3_f3    |*/  __DEVICE__ inline mat3 to_mat3_f3( float3 A, float3 B, float3 C)
/*| to_mat3_f3    |*/  {
/*| to_mat3_f3    |*/	   mat3 D;
/*| to_mat3_f3    |*/	   D.r0 = A;
/*| to_mat3_f3    |*/	   D.r1 = B;
/*| to_mat3_f3    |*/	   D.r2 = C;
/*| to_mat3_f3    |*/	   return D;
/*| to_mat3_f3    |*/  }
/*|               |*/
/*| mul_mat3_f3   |*/__DEVICE__ inline float3 mul_mat3_f3( mat3 B, float3 A) {
/*| mul_mat3_f3   |*/	   float3 C;
/*| mul_mat3_f3   |*/
/*| mul_mat3_f3   |*/	   C.x = A.x * B.r0.x + A.y * B.r1.x + A.z * B.r2.x;
/*| mul_mat3_f3   |*/	   C.y = A.x * B.r0.y + A.y * B.r1.y + A.z * B.r2.y;
/*| mul_mat3_f3   |*/	   C.z = A.x * B.r0.z + A.y * B.r1.z + A.z * B.r2.z;
/*| mul_mat3_f3   |*/	   return C;
/*| mul_mat3_f3   |*/  }
/*|               |*/
/*| mul_f3_mat3   |*/__DEVICE__ inline float3 mul_f3_mat3( float3 A, mat3 B) {
/*| mul_f3_mat3   |*/    float3 C;
/*| mul_f3_mat3   |*/
/*| mul_f3_mat3   |*/    C.x = A.x * B.r0.x + A.y * B.r0.y + A.z * B.r0.z;
/*| mul_f3_mat3   |*/    C.y = A.x * B.r1.x + A.y * B.r1.y + A.z * B.r1.z;
/*| mul_f3_mat3   |*/    C.z = A.x * B.r2.x + A.y * B.r2.y + A.z * B.r2.z;
/*| mul_f3_mat3   |*/    return C;
/*| mul_f3_mat3   |*/  }
/*|               |*/
/*| mul_mat3_mat3 |*/__DEVICE__ mat3 mul_mat3_mat3( mat3 A, mat3 B)
/*| mul_mat3_mat3 |*/{
/*| mul_mat3_mat3 |*/   float r[3][3];
/*| mul_mat3_mat3 |*/   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},
/*| mul_mat3_mat3 |*/                    {A.r1.x, A.r1.y, A.r1.z},
/*| mul_mat3_mat3 |*/                    {A.r2.x, A.r2.y, A.r2.z}};
/*| mul_mat3_mat3 |*/   float b[3][3] = {{B.r0.x, B.r0.y, B.r0.z},
/*| mul_mat3_mat3 |*/                    {B.r1.x, B.r1.y, B.r1.z},
/*| mul_mat3_mat3 |*/                    {B.r2.x, B.r2.y, B.r2.z}};
/*| mul_mat3_mat3 |*/
/*| mul_mat3_mat3 |*/  for( int i = 0; i < 3; ++i)
/*| mul_mat3_mat3 |*/  {
/*| mul_mat3_mat3 |*/	  for( int j = 0; j < 3; ++j)
/*| mul_mat3_mat3 |*/	  {
/*| mul_mat3_mat3 |*/		  r[i][j] = 0.0f;
/*| mul_mat3_mat3 |*/		  for( int k = 0; k < 3; ++k)
/*| mul_mat3_mat3 |*/		  {
/*| mul_mat3_mat3 |*/			  r[i][j] = r[i][j] + a[i][k] * b[k][j];
/*| mul_mat3_mat3 |*/		  }
/*| mul_mat3_mat3 |*/	  }
/*| mul_mat3_mat3 |*/  }
/*| mul_mat3_mat3 |*/  mat3 R = to_mat3(r[0][0], r[0][1], r[0][2],
/*| mul_mat3_mat3 |*/                   r[1][0], r[1][1], r[1][2],
/*| mul_mat3_mat3 |*/					         r[2][0], r[2][1], r[2][2]);
/*| mul_mat3_mat3 |*/  return R;
/*| mul_mat3_mat3 |*/}
/*| mat3          |*/#endif // end of mat3 implementation


/*| mat4          |*/// ----------------------------------------------------------------------------------------------------------
/*| mat4          |*/// mat4 implementation
/*| mat4          |*/// ----------------------------------------------------------------------------------------------------------
/*|               |*/
/*| mat4          |*/#if defined(USE_NATIVE_METAL_IMPL)
/*| mat4          |*/
/*| mat4          |*/  typedef float4x4 mat4;
/*| mat4          |*/
/*| to_mat4       |*/  __DEVICE__ inline mat4 to_mat4( float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p)
/*| to_mat4       |*/  {
/*| to_mat4       |*/    return mat4(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p);
/*| to_mat4       |*/  }
/*|               |*/
/*| to_mat4_f     |*/  __DEVICE__ inline mat4 to_mat4_f( float a ) { return mat4(a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a); }
/*| to_mat4_f4    |*/  __DEVICE__ inline mat4 to_mat4_f4( float4 a, float4 b, float4 c, float4 d ) { return mat4(a,b,c,d); }
/*| mul_mat4_f4   |*/  __DEVICE__ inline float4 mul_mat4_f4( mat4 B, float4 A) { return (B*A); }
/*| mul_f4_mat4   |*/  __DEVICE__ inline float4 mul_f4_mat4( float4 A, mat4 B) { return (A*B); }
/*| mul_mat4_mat4 |*/  __DEVICE__ inline mat4 mul_mat4_mat4( mat4 A, mat4 B) { return (A*B); }
/*|               |*/
/*| mat4          |*/#else
/*|               |*/
/*| mat4          |*/  typedef struct { float4 r0; float4 r1; float4 r2; float4 r3; } mat4;
/*|               |*/
/*| to_mat4       |*/  __DEVICE__ inline mat4 to_mat4( float  a, float  b, float c,   float d, float e, float f,   float g, float h, float i, float j, float k, float l, float m, float n, float o, float p)
/*| to_mat4       |*/  {
/*| to_mat4       |*/    mat4 t;
/*| to_mat4       |*/    t.r0.x = a; t.r0.y = b; t.r0.z = c; t.r0.w = d;
/*| to_mat4       |*/    t.r1.x = e; t.r1.y = f; t.r1.z = g; t.r0.w = h;
/*| to_mat4       |*/    t.r2.x = i; t.r2.y = j; t.r2.z = k; t.r0.w = l;
/*| to_mat4       |*/    t.r3.x = m; t.r3.y = n; t.r3.z = o; t.r0.w = p;
/*| to_mat4       |*/    return t;
/*| to_mat4       |*/  }
/*|               |*/
/*| to_mat4_f     |*/  __DEVICE__ inline mat4 to_mat4_f( float  a )
/*| to_mat4_f     |*/  {
/*| to_mat4_f     |*/    mat4 t;
/*| to_mat4_f     |*/    t.r0.x = t.r0.y = t.r0.z = t.r0.w = t.r1.x = t.r1.y = t.r1.z = t.r1.w = t.r2.x = t.r2.y = t.r2.z = t.r2.w = t.r3.x = t.r3.y = t.r3.z = t.r3.w = a;
/*| to_mat4_f     |*/    return t;
/*| to_mat4_f     |*/  }
/*|               |*/
/*| to_mat4_f4    |*/__DEVICE__ inline mat4 to_mat4_f4( float4 A, float4 B, float4 C, float4 D)
/*| to_mat4_f4    |*/  {
/*| to_mat4_f4    |*/    mat4 _ret;
/*| to_mat4_f4    |*/    _ret.r0 = A;
/*| to_mat4_f4    |*/    _ret.r1 = B;
/*| to_mat4_f4    |*/    _ret.r2 = C;
/*| to_mat4_f4    |*/    _ret.r3 = D;
/*| to_mat4_f4    |*/    return _ret;
/*| to_mat4_f4    |*/  }
/*|               |*/
/*| mul_mat4_f4   |*/__DEVICE__ inline float4 mul_mat4_f4( mat4 B, float4 A)
/*| mul_mat4_f4   |*/  {
/*| mul_mat4_f4   |*/    float4 C;
/*| mul_mat4_f4   |*/    C.x = A.x * B.r0.x + A.y * B.r1.x + A.z * B.r2.x + A.w * B.r3.x;
/*| mul_mat4_f4   |*/    C.y = A.x * B.r0.y + A.y * B.r1.y + A.z * B.r2.y + A.w * B.r3.y;
/*| mul_mat4_f4   |*/    C.z = A.x * B.r0.z + A.y * B.r1.z + A.z * B.r2.z + A.w * B.r3.z;
/*| mul_mat4_f4   |*/    C.w = A.x * B.r0.w + A.y * B.r1.w + A.z * B.r2.w + A.w * B.r3.w;
/*| mul_mat4_f4   |*/    return C;
/*| mul_mat4_f4   |*/}
/*|               |*/
/*| mul_f4_mat4   |*/__DEVICE__ float4 mul_f4_mat4( float4 A, mat4 B)
/*| mul_f4_mat4   |*/  {
/*| mul_f4_mat4   |*/    float4 C;
/*| mul_f4_mat4   |*/    C.x = A.x * B.r0.x + A.y * B.r0.y + A.z * B.r0.z + A.w * B.r0.w;
/*| mul_f4_mat4   |*/    C.y = A.x * B.r1.x + A.y * B.r1.y + A.z * B.r1.z + A.w * B.r1.w;
/*| mul_f4_mat4   |*/    C.z = A.x * B.r2.x + A.y * B.r2.y + A.z * B.r2.z + A.w * B.r2.w;
/*| mul_f4_mat4   |*/    C.w = A.x * B.r3.x + A.y * B.r3.y + A.z * B.r3.z + A.w * B.r3.w;
/*| mul_f4_mat4   |*/    return C;
/*| mul_f4_mat4   |*/  }
/*|               |*/
/*| mul_mat4_mat4 |*/__DEVICE__ inline mat4 mat4_multi_mat4( mat4 B, mat4 A)
/*| mul_mat4_mat3 |*/{
/*| mul_mat4_mat3 |*/
/*| mul_mat4_mat4 |*/  float r[4][4];
/*| mul_mat4_mat4 |*/  float a[4][4] = {{A.r0.x, A.r0.y, A.r0.z, A.r0.w},
/*| mul_mat4_mat4 |*/                   {A.r1.x, A.r1.y, A.r1.z, A.r1.w},
/*| mul_mat4_mat4 |*/                   {A.r2.x, A.r2.y, A.r2.z, A.r2.w},
/*| mul_mat4_mat4 |*/                   {A.r3.x, A.r3.y, A.r3.z, A.r3.w}};
/*| mul_mat4_mat4 |*/  float b[4][4] = {{B.r0.x, B.r0.y, B.r0.z, B.r0.w},
/*| mul_mat4_mat4 |*/                   {B.r1.x, B.r1.y, B.r1.z, B.r1.w},
/*| mul_mat4_mat4 |*/                   {B.r2.x, B.r2.y, B.r2.z, B.r2.w},
/*| mul_mat4_mat4 |*/                   {B.r3.x, B.r3.y, B.r3.z, B.r3.w}};
/*| mul_mat4_mat4 |*/
/*| mul_mat4_mat4 |*/  for( int i = 0; i < 4; ++i)
/*| mul_mat4_mat4 |*/  {
/*| mul_mat4_mat4 |*/	  for( int j = 0; j < 4; ++j)
/*| mul_mat4_mat4 |*/	  {
/*| mul_mat4_mat4 |*/		  r[i][j] = 0.0f;
/*| mul_mat4_mat4 |*/		  for( int k = 0; k < 4; ++k)
/*| mul_mat4_mat4 |*/		  {
/*| mul_mat4_mat4 |*/			r[i][j] = r[i][j] + a[i][k] * b[k][j];
/*| mul_mat4_mat4 |*/		  }
/*| mul_mat4_mat4 |*/	  }
/*| mul_mat4_mat4 |*/  }
/*| mul_mat4_mat4 |*/  mat4 R = to_mat4(r[0][0], r[0][1], r[0][2], r[0][3],
/*| mul_mat4_mat4 |*/                   r[1][0], r[1][1], r[1][2], r[1][3],
/*| mul_mat4_mat4 |*/  	                r[2][0], r[2][1], r[2][2], r[2][3],
/*| mul_mat4_mat4 |*/	                  r[3][0], r[3][1], r[3][2], r[3][3]);
/*| mul_mat4_mat4 |*/  return R;
/*| mul_mat4_mat4 |*/}
/*| mat4          |*/#endif // end of mat3 implementation




#if defined(USE_NATIVE_METAL_IMPL)

  /*| fract_f       |*/ #define fract_f(A)  fract(A)
  /*| fract_f2      |*/ #define fract_f2(A) fract(A)
  /*| fract_f3      |*/ #define fract_f3(A) fract(A)
  /*| fract_f4      |*/ #define fract_f4(A) fract(A)

  /*| mod_f         |*/ #define mod_f(a,b)  fmod((a),(b))
  /*| mod_f2        |*/ #define mod_f2(value,divisor) fmod(value,divisor)
  /*| mod_f3        |*/ #define mod_f3(value,divisor) fmod(value,divisor)
  /*| mod_f4        |*/ #define mod_f4(value,divisor) fmod(value,divisor)
  /*| mod_f2f2      |*/ #define mod_f2f2(value,divisor) fmod(value,divisor)
  /*| mod_f3f3      |*/ #define mod_f3f3(value,divisor) fmod(value,divisor)
  /*| mod_f4f4      |*/ #define mod_f4f4(value,divisor) fmod(value,divisor)

  /*| sin_f2        |*/ #define sin_f2(i) sin(i)
  /*| sin_f3        |*/ #define sin_f3(i) sin(i)
  /*| sin_f4        |*/ #define sin_f4(i) sin(i)
  /*| cos_f2        |*/ #define cos_f2(i) cos(i)
  /*| cos_f3        |*/ #define cos_f3(i) cos(i)
  /*| cos_f4        |*/ #define cos_f4(i) cos(i)
  /*| acos_f3       |*/ #define acos_f3(i) acos(i)
  /*| tan_f3        |*/ #define tan_f3(i) tan(i)
  /*| tanh_f3       |*/ #define tanh_f3(i) tanh(i)
  /*| atan_f2       |*/ #define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
  /*| atan_f3       |*/ #define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
  /*| atan_f4       |*/ #define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
  /*| abs_f2        |*/ #define abs_f2(a) _fabs(a)
  /*| abs_f3        |*/ #define abs_f3(a) _fabs(a)
  /*| abs_f4        |*/ #define abs_f4(a) _fabs(a)
  /*| sqrt_f2       |*/ #define sqrt_f2(a) _sqrtf(a)
  /*| sqrt_f3       |*/ #define sqrt_f3(a) _sqrtf(a)
  /*| sqrt_f4       |*/ #define sqrt_f4(a) _sqrtf(a)
  /*| exp_f2        |*/ #define exp_f2(a) _expf((a).x)
  /*| exp_f3        |*/ #define exp_f3(a) _expf((a).x)
  /*| exp_f4        |*/ #define exp_f4(a) _expf((a).x)
  /*| exp2_f2       |*/ #define exp2_f2(a) _exp2f((a).x)
  /*| exp2_f3       |*/ #define exp2_f3(a) _exp2f((a).x)
  /*| exp2_f4       |*/ #define exp2_f4(a) _exp2f((a).x)
  /*| ceil_f2       |*/ #define ceil_f2(a) ceil(a)
  /*| mix_f2        |*/ #define mix_f2(v,i,m) mix(v,i,m)
  /*| mix_f3        |*/ #define mix_f3(v,i,m) mix(v,i,m)
  /*| mix_f4        |*/ #define mix_f4_f(v,i,m) mix(v,i,m)
  /*| log_f3        |*/ #define log_f3(a) log(a)
  /*| log2_f3       |*/ #define log2_f3(a) log2(a)
  /*| round_f2      |*/ #define round_f2(a) round(a)
  /*| round_f3      |*/ #define round_f3(a) round(a)
  /*| round_f4      |*/ #define round_f4(a) round(a)
  /*| sign_f        |*/ #define sign_f(a) sign(a)
  /*| sign_f2       |*/ #define sign_f2(a) sign(a)
  /*| sign_f3       |*/ #define sign_f3(a) sign(a)
  /*| sign_f4       |*/ #define sign_f4(a) sign(a)
  /*| distance_f    |*/ #define distance_f ( p1, p2) _fabs(p1 - p2)
  /*| distance_f2   |*/ #define distance_f2(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
  /*| distance_f3   |*/ #define distance_f3(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
  /*| pow_f2        |*/ #define pow_f2(a,b) pow(a,b)
  /*| pow_f3        |*/ #define pow_f3(a,b) pow(a,b)
  /*| pow_f4        |*/ #define pow_f4(a,b) pow(a,b)
  /*| lessThan_f2   |*/ #define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
  /*| lessThan_f3   |*/ #define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
  /*| lessThan_f4   |*/ #define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);
  /*| refract_f2    |*/ #define refract_f2(I,N,eta) refract(I,N,eta)
  /*| refract_f3    |*/ #define refract_f3(I,N,eta) refract(I,N,eta)

#else

  #if defined(USE_NATIVE_OPENCL_IMPL)

    #define reflect(I,N) (I-2.0f*dot(N,I)*N)

    #define fract(a) ((a)-_floor(a))  // oder Pointer bauen: gentype fract(gentype x, gentype *itpr)

    /*| fract_f       |*/ #define fract_f(A)  fract(A)
    /*| fract_f2      |*/ #define fract_f2(A) to_float2(fract((A).x),fract((A).y))
    /*| fract_f3      |*/ #define fract_f3(A) to_float3(fract((A).x),fract((A).y),fract((A).z))
    /*| fract_f4      |*/ #define fract_f4(A) to_float4(fract((A).x),fract((A).y),fract((A).z),fract((A).w))
    /*| mod_f         |*/ #define mod_f(a,b) _fmod(a,b)
    /*| mod_f2        |*/ #define mod_f2(value,divisor) _fmod(value,divisor)
    /*| mod_f3        |*/ #define mod_f3(value,divisor) _fmod(value,divisor)
    /*| mod_f4        |*/ #define mod_f4(value,divisor) _fmod(value,divisor)
    /*| mod_f2f2      |*/ #define mod_f2f2(value,divisor) _fmod(value,divisor)
    /*| mod_f3f3      |*/ #define mod_f3f3(value,divisor) _fmod(value,divisor)
    /*| mod_f4f4      |*/ #define mod_f4f4(value,divisor) _fmod(value,divisor)
    /*| sin_f2        |*/ #define sin_f2(i) sin(i)
    /*| sin_f3        |*/ #define sin_f3(i) sin(i)
    /*| sin_f4        |*/ #define sin_f4(i) sin(i)
    /*| cos_f2        |*/ #define cos_f2(i) cos(i)
    /*| cos_f3        |*/ #define cos_f3(i) cos(i)
    /*| cos_f4        |*/ #define cos_f4(i) cos(i)
    /*| acos_f3       |*/ #define acos_f3(i) acos(i)
    /*| tan_f3        |*/ #define tan_f3(i) tan(i)
    /*| tanh_f3       |*/ #define tanh_f3(i) tanh(i)
    /*| atan_f2       |*/ #define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
    /*| atan_f3       |*/ #define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
    /*| atan_f4       |*/ #define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
    /*| abs_f2        |*/ #define abs_f2(a) fabs(a)
    /*| abs_f3        |*/ #define abs_f3(a) fabs(a)
    /*| abs_f4        |*/ #define abs_f4(a) fabs(a)
    /*| sqrt_f2       |*/ #define sqrt_f2(a) sqrtf(a)
    /*| sqrt_f3       |*/ #define sqrt_f3(a) sqrtf(a)
    /*| sqrt_f4       |*/ #define sqrt_f4(a) sqrtf(a)
    /*| exp_f2        |*/ #define exp_f2(a) _expf((a).x)
    /*| exp_f3        |*/ #define exp_f3(a) _expf((a).x)
    /*| exp_f4        |*/ #define exp_f4(a) _expf((a).x)
    /*| exp2_f2       |*/ #define exp2_f2(a) _exp2f((a).x)
    /*| exp2_f3       |*/ #define exp2_f3(a) _exp2f((a).x)
    /*| exp2_f4       |*/ #define exp2_f4(a) _exp2f((a).x)
    /*| ceil_f2       |*/ #define ceil_f2(a) ceil(a)
    /*| mix_f2        |*/ #define mix_f2(v,i,m) mix(v,i,m)
    /*| mix_f3        |*/ #define mix_f3(v,i,m) mix(v,i,m)
    /*| mix_f4        |*/ #define mix_f4_f(v,i,m) mix(v,i,m)
    /*| log_f3        |*/ #define log_f3(a) log(a)
    /*| log2_f3       |*/ #define log2_f3(a) log2(a)
    /*| sign_f        |*/ #define sign_f(a) sign(a)
    /*| sign_f2       |*/ #define sign_f2(a) sign(a)
    /*| sign_f3       |*/ #define sign_f3(a) sign(a)
    /*| sign_f4       |*/ #define sign_f4(a) sign(a)
    /*| distance_f    |*/ #define distance_f ( p1, p2) distance(p1, p2)
    /*| distance_f2   |*/ #define distance_f2(pt1,pt2) distance(p1, p2)
    /*| distance_f3   |*/ #define distance_f3(pt1,pt2) distance(p1, p2)
    /*| pow_f2        |*/ #define pow_f2(a,b) pow(a,b)
    /*| pow_f3        |*/ #define pow_f3(a,b) pow(a,b)
    /*| pow_f4        |*/ #define pow_f4(a,b) pow(a,b)
    /*| lessThan_f2   |*/ #define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
    /*| lessThan_f3   |*/ #define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
    /*| lessThan_f4   |*/ #define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);

    /*| refract_f2    |*/__DEVICE__ float2 refract_f2(float2 I, float2 N, float eta) {
    /*| refract_f2    |*/    float dotNI = dot(N, I);
    /*| refract_f2    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f2    |*/    if (k < 0.0f) {
    /*| refract_f2    |*/       return to_float2_s(0.0f);
    /*| refract_f2    |*/    }
    /*| refract_f2    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N;
    /*| refract_f2    |*/ }
    /*| refract_f3    |*/__DEVICE__ float3 refract_f3(float3 I, float3 N, float eta) {
    /*| refract_f3    |*/   float dotNI = dot(N, I);
    /*| refract_f3    |*/   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f3    |*/   if (k < 0.0f) {
    /*| refract_f3    |*/      return to_float3_s(0.0f);
    /*| refract_f3    |*/   }
    /*| refract_f3    |*/   return eta * I - (eta * dotNI * _sqrtf(k)) * N; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
    /*| refract_f3    |*/ }

 #else // Generic

    /*| reflect       |*/ #if defined(DEVICE_IS_OPENCL)
    /*| reflect       |*/   __DEVICE__ float3 reflect(float3 I, float3 N) {return I - 2.0f * dot(N, I) * N;}
    /*| reflect       |*/ #endif

    /*| radians       |*/ #if defined(DEVICE_IS_CUDA)
    /*| radians       |*/   #define radians(a) a * M_PI/180.0f
    /*| radians       |*/ #endif

    /*| length_f      |*/ #if defined(DEVICE_IS_CUDA) //Besonderheit bei Cuda - length(float) nicht definiert - alles andere ja
    /*| length_f      |*/   #define length_f(a) fabs(a);
    /*| length_f      |*/ #endif

    #define fract(a) ((a)-_floor(a))

    /*| fract_f       |*/ #define fract_f(A)  fract(A)
    /*| fract_f2      |*/ #define fract_f2(A) to_float2(fract((A).x),fract((A).y))
    /*| fract_f3      |*/ #define fract_f3(A) to_float3(fract((A).x),fract((A).y),fract((A).z))
    /*| fract_f4      |*/ #define fract_f4(A) to_float4(fract((A).x),fract((A).y),fract((A).z),fract((A).w))
    /*| mod_f         |*/ #define mod_f(a,b) ((a)-(b)*_floor((a)/(b)))
    /*| mod_f2        |*/ #define mod_f2(value,divisor) to_float2(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)))
    /*| mod_f3        |*/ #define mod_f3(value,divisor) to_float3(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)))
    /*| mod_f4        |*/ #define mod_f4(value,divisor) to_float4(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)),mod_f((value).w, (divisor)))
    /*| mod_f2f2      |*/ #define mod_f2f2(value,divisor) to_float2(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y))
    /*| mod_f3f3      |*/ #define mod_f3f3(value,divisor) to_float3(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z))
    /*| mod_f4f4      |*/ #define mod_f4f4(value,divisor) to_float4(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z),mod_f((value).w, (divisor).w))
    /*| sin_f2        |*/ #define sin_f2(i) to_float2( _sinf((i).x), _sinf((i).y))
    /*| sin_f3        |*/ #define sin_f3(i) to_float3( _sinf((i).x), _sinf((i).y), _sinf((i).z))
    /*| sin_f4        |*/ #define sin_f4(i) to_float4( _sinf((i).x), _sinf((i).y), _sinf((i).z), _sinf((i).w))
    /*| cos_f2        |*/ #define cos_f2(i) to_float2( _cosf((i).x), _cosf((i).y))
    /*| cos_f3        |*/ #define cos_f3(i) to_float3( _cosf((i).x), _cosf((i).y), _cosf((i).z))
    /*| cos_f4        |*/ #define cos_f4(i) to_float4( _cosf((i).x), _cosf((i).y), _cosf((i).z), _cosf((i).w))
    /*| acos_f3       |*/ #define acos_f3(i) to_float3( _acosf((i).x), _acosf((i).y), _acosf((i).z))
    /*| tan_f3        |*/ #define tan_f3(i) to_float3(_tanf((i).x), _tanf((i).y), _tanf((i).z))
    /*| tanh_f3       |*/ #define tanh_f3(i) to_float3(_tanhf(i.x), _tanhf((i).y), _tanhf((i).z))
    /*| atan_f2       |*/ #define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
    /*| atan_f3       |*/ #define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
    /*| atan_f4       |*/ #define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
    /*| abs_f2        |*/ #define abs_f2(a) to_float2(_fabs((a).x), _fabs((a).y))
    /*| abs_f3        |*/ #define abs_f3(a) to_float3(_fabs((a).x), _fabs((a).y),_fabs((a).z))
    /*| abs_f4        |*/ #define abs_f4(a) to_float4(_fabs((a).x), _fabs((a).y),_fabs((a).z),_fabs((a).w))
    /*| sqrt_f2       |*/ #define sqrt_f2(a) to_float2(_sqrtf((a).x),_sqrtf((a).y))
    /*| sqrt_f3       |*/ #define sqrt_f3(a) to_float3(_sqrtf((a).x),_sqrtf((a).y),_sqrtf((a).z))
    /*| sqrt_f4       |*/ #define sqrt_f4(a) to_float4(_sqrtf((a).x),_sqrtf((a).y),_sqrtf((a).z),_sqrtf((a).w);)
    /*| exp_f2        |*/ #define exp_f2(a) to_float2(_expf((a).x), _expf((a).y))
    /*| exp_f3        |*/ #define exp_f3(a) to_float3(_expf((a).x), _expf((a).y),_expf((a).z))
    /*| exp_f4        |*/ #define exp_f4(a) to_float4(_expf((a).x), _expf((a).y),_expf((a).z),_expf((a).w))
    /*| exp2_f2       |*/ #define exp2_f2(a) to_float2(_exp2f((a).x), _exp2f((a).y))
    /*| exp2_f3       |*/ #define exp2_f3(a) to_float3(_exp2f((a).x), _exp2f((a).y), _exp2f((a).z))
    /*| exp2_f4       |*/ #define exp2_f4(a) to_float4(_exp2f((a).x), _exp2f((a).y), _exp2f((a).z), _exp2f((a).w))
    /*| ceil_f2       |*/ #define ceil_f2(a) to_float2(_ceil((a).x), _ceil((a).y))
    /*| mix_f2        |*/ #define mix_f2(v,i,m) to_float2(_mix((v).x,(i).x,(m).x),_mix((v).y,(i).y,(m).y))
    /*| mix_f3        |*/ #define mix_f3(v,i,m) to_float3(_mix((v).x,(i).x,(m).x),_mix((v).y,(i).y,(m).y),_mix((v).z,(i).z,(m).z))
    /*| mix_f4        |*/ #define mix_f4_f(v,i,m) to_float4(_mix((v).x,(i).x,m),_mix((v).y,(i).y,m),_mix((v).z,(i).z,m),_mix((v).w,(i).w,m))
    /*| log_f3        |*/ #define log_f3(a) to_float3(_logf((a).x), _logf((a).y),_logf((a).z))
    /*| log2_f3       |*/ #define log2_f3(a) to_float3(_log2f((a).x), _log2f((a).y),_log2f((a).z))  
    /*| sign_f        |*/ #define sign_f(a) (a==0.0f?0.0f:a>0.0f?1.0f:-1.0f)
    /*| sign_f2       |*/ #define sign_f2(a) to_float2((a).x==0.0f?0.0f:a>0.0f?1.0f:-1.0f, (a).y==0.0f?0.0f:a>0.0f?1.0f:-1.0f)
    /*| sign_f3       |*/ #define sign_f3(a) to_float3((a).x==0.0f?0.0f:a>0.0f?1.0f:-1.0f, (a).y==0.0f?0.0f:a>0.0f?1.0f:-1.0f,(a).z==0.0f?0.0f:a>0.0f?1.0f:-1.0f)
    /*| sign_f4       |*/ #define sign_f4(a) to_float4((a).x==0.0f?0.0f:a>0.0f?1.0f:-1.0f, (a).y==0.0f?0.0f:a>0.0f?1.0f:-1.0f,(a).z==0.0f?0.0f:a>0.0f?1.0f:-1.0f,(a).w==0.0f?0.0f:a>0.0f?1.0f:-1.0f)
    /*| distance_f    |*/ #define distance_f ( p1, p2) _fabs(p1 - p2)
    /*| distance_f2   |*/ #define distance_f2(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
    /*| distance_f3   |*/ #define distance_f3(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
    /*| pow_f2        |*/ #define pow_f2(a,b) to_float2(_powf((a).x,(b).x),_powf((a).y,(b).y))
    /*| pow_f3        |*/ #define pow_f3(a,b) to_float3(_powf((a).x,(b).x),_powf((a).y,(b).y),_powf((a).z,(b).z))
    /*| pow_f4        |*/ #define pow_f4(a,b) to_float4(_powf((a).x,(b).x),_powf((a).y,(b).y),_powf((a).z,(b).z),_powf((a).w,(b).w))
    /*| lessThan_f2   |*/ #define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
    /*| lessThan_f3   |*/ #define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
    /*| lessThan_f4   |*/ #define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);
    /*| refract_f2    |*/ __DEVICE__ float2 refract_f2(float2 I, float2 N, float eta) {
    /*| refract_f2    |*/    float dotNI = dot(N, I);
    /*| refract_f2    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f2    |*/    if (k < 0.0f) {
    /*| refract_f2    |*/      return to_float2_s(0.0f);
    /*| refract_f2    |*/    }
    /*| refract_f2    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N;
    /*| refract_f2    |*/ }
    /*|               |*/
    /*| refract_f3    |*/ __DEVICE__ float3 refract_f3(float3 I, float3 N, float eta) {
    /*| refract_f3    |*/    float dotNI = dot(N, I);
    /*| refract_f3    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f3    |*/    if (k < 0.0f) {
    /*| refract_f3    |*/      return to_float3_s(0.0f);
    /*| refract_f3    |*/    }
    /*| refract_f3    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
    /*| refract_f3    |*/ }

  #endif

#endif


/*| to_int2_f2    |*/#define to_int2_f2(V) to_int2_cfloat(V)   // float2 zu int2
/*| to_int2_ui2   |*/#define to_int2_ui2(V) to_int2_cuint(V)   // uint2 zu int2
/*| to_int2_2f    |*/#define to_int2_2f(A,B) to_int2((int)(A),(int)(B))
/*| mod_i2        |*/#define mod_i2(V,I) to_int2( (V).x % (I), (V).y % (I)  )

// #define eq_i2_i2(A,B) ((A).x==(B).x && (A).y==(B).y)
// #define eq_f2_f2(A,B) ((A).x==(B).x && (A).y==(B).y)
// #define eq_i2_1i(A,I) ((A).x==(I) && (A).y==(I))
// #define eq_f2_1f(A,I) ((A).x==(I) && (A).y==(I))

/*|  to_float2_i2 |*/#define to_float2_i2(V) to_float2_cint(V)   // int2 zu float2
/*|  to_float2_ui2|*/#define to_float2_ui2(V) to_float2_cuint(V) // uint2 zu float2
/*|  to_float3_i3 |*/#define to_float3_i3(V) to_float3_cint(V)   // int2 zu float2
/*|  to_float3_ui3|*/#define to_float3_ui3(V) to_float3_cuint(V) // uint2 zu float2
/*|  to_float4_i4 |*/#define to_float4_i4(V) to_float4_cint(V)   // int2 zu float2
/*|  to_float4_ui4|*/#define to_float4_ui4(V) to_float4_cuint(V) // uint2 zu float2

/*|  to_float4_f2f2 |*/#define to_float4_f2f2(A,B) to_float4((A).x,(A).y,(B).x,(B).y ) // or is there some to_float_..() for that?!? - No - that is missing in DCTL :-) but now we have "one"



/*|  decube_3f    |*/
/*|  decube_3f    |*/__DEVICE__ float4 decube_3f(__TEXTURE2D__ t, float x, float y, float z)
/*|  decube_3f    |*/{
/*|  decube_3f    |*/  float ax=_fabs(x);
/*|  decube_3f    |*/  float ay=_fabs(y);
/*|  decube_3f    |*/  float az=_fabs(z);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (x>0.0f && ax>=ay && ax>=az) // +X, Face 0, right
/*|  decube_3f    |*/    return _tex2DVecN(t,(-z/ax+1.0f)/8.0f + 0.5f,(y/ax+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (y>0.0f && ay>=ax && ay>=az) // +Y, Face 2, top
/*|  decube_3f    |*/    return _tex2DVecN(t,(x/ay+1.0f)/8.0f + 0.25f,(-z/ay+1.0f)/6.0f + (2.0f/3.0f),15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (z>0.0f && az>=ax && az>=ay) // +Z, Face 4, front
/*|  decube_3f    |*/    return _tex2DVecN(t,(x/az+1.0f)/8.0f + 0.25f,(y/az+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (x<0.0f && ax>=ay && ax>=az) // -X, Face 1, left
/*|  decube_3f    |*/    return _tex2DVecN(t,(z/ax+1.0f)/8.0f,(y/ax+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (y<0.0f && ay>=ax && ay>=az) // -Y, Face 3, bottom
/*|  decube_3f    |*/    return _tex2DVecN(t,(x/ay+1.0f)/8.0f + 0.25f,(z/ay+1.0f)/6.0f,15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  if (z<0.0f && az>=ax && az>=ay) // -Z, Face 5, back
/*|  decube_3f    |*/    return _tex2DVecN(t,(-x/az+1.0f)/8.0f + 0.75f,(y/az+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_3f    |*/
/*|  decube_3f    |*/  return to_float4(1.0f,0.0f,0.0f,1.0f); // error
/*|  decube_3f    |*/}
/*|  decube_f3    |*/
/*|  decube_f3    |*/__DEVICE__ float4 decube_f3(__TEXTURE2D__ t, float3 xyz)
/*|  decube_f3    |*/{
/*|  decube_f3    |*/  float ax=_fabs(xyz.x);
/*|  decube_f3    |*/  float ay=_fabs(xyz.y);
/*|  decube_f3    |*/  float az=_fabs(xyz.z);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.x>0.0f && ax>=ay && ax>=az) // +X, Face 0, right
/*|  decube_f3    |*/    return _tex2DVecN(t,(-xyz.z/ax+1.0f)/8.0f + 0.5f,(xyz.y/ax+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.y>0.0f && ay>=ax && ay>=az) // +Y, Face 2, top
/*|  decube_f3    |*/    return _tex2DVecN(t,(xyz.x/ay+1.0f)/8.0f + 0.25f,(-xyz.z/ay+1.0f)/6.0f + (2.0f/3.0f),15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.z>0.0f && az>=ax && az>=ay) // +Z, Face 4, front
/*|  decube_f3    |*/    return _tex2DVecN(t,(xyz.x/az+1.0f)/8.0f + 0.25f,(xyz.y/az+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.x<0.0f && ax>=ay && ax>=az) // -X, Face 1, left
/*|  decube_f3    |*/    return _tex2DVecN(t,(xyz.z/ax+1.0f)/8.0f,(xyz.y/ax+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.y<0.0f && ay>=ax && ay>=az) // -Y, Face 3, bottom
/*|  decube_f3    |*/    return _tex2DVecN(t,(xyz.x/ay+1.0f)/8.0f + 0.25f,(xyz.z/ay+1.0f)/6.0f,15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  if (xyz.z<0.0f && az>=ax && az>=ay) // -Z, Face 5, back
/*|  decube_f3    |*/    return _tex2DVecN(t,(-xyz.x/az+1.0f)/8.0f + 0.75f,(xyz.y/az+1.0f)/6.0f + (1.0f/3.0f),15);
/*|  decube_f3    |*/
/*|  decube_f3    |*/  return to_float4(1.0f,0.0f,0.0f,1.0f); // error
/*|  decube_f3    |*/}
