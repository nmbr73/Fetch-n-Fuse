

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

    #define swi2(a,b,A)     (A).ab
    #define swi3(a,b,c,A)   (A).abc
    #define swi4(a,b,c,d,A) (A).abcd

  #else
    
    #define swi2(a,b,A)     to_float2((A).a,(A).b)
    #define swi3(a,b,c,A)   to_float3((A).a,(A).b,(A).c)
    #define swi4(a,b,c,d,A) to_float4((A).a,(A).b,(A).c,(A).d)

  #endif




// ----------------------------------------------------------------------------------------------------------
// mat2 implementation
// ----------------------------------------------------------------------------------------------------------

#if defined(USE_NATIVE_METAL_IMPL)

  typedef float2x2 mat2;

  __DEVICE__ inline mat2 to_mat2      ( float  a, float  b, float c, float d) { return mat2(a,b,c,d);       }
  __DEVICE__ inline mat2 to_mat2_f    ( float  a                            ) { return mat2(a,a,a,a);       }
  __DEVICE__ inline mat2 to_mat2_s    ( float  a                            ) { return mat2(a);             }
  __DEVICE__ inline mat2 to_mat2_f2_f2 ( float2 a, float2 b                  ) { return mat2(a,b);           }
  __DEVICE__ inline mat2 to_mat2_f_f3 ( float  a, float3 b                  ) { return mat2(a,b.x,b.y,b.z); }
  __DEVICE__ inline mat2 to_mat2_f3_f ( float3 a, float  b                  ) { return mat2(a.x,a.y,a.z,b); }


  __DEVICE__ inline mat2    mul_mat2_mat2 ( mat2   a, mat2   b )  { return a*b; }
  __DEVICE__ inline float2  mul_f2_mat2   ( float2 v, mat2   m )  { return v*m; }
  __DEVICE__ inline float2  mul_mat2_f2    ( mat2   m, float2 v )  { return m*v; }
  __DEVICE__ inline mat2    mul_mat2_f    ( mat2   m, float  s )  { return m*s; }
  __DEVICE__ inline mat2    mul_f_mat2    ( float  s, mat2   m )  { return s*m; }

#else

  typedef struct { float2 r0; float2 r1; } mat2;

  __DEVICE__ inline mat2 to_mat2      ( float  a, float  b, float c, float d)  { mat2 t; t.r0.x = a; t.r0.y = b; t.r1.x = c; t.r1.y = d;         return t; }
  __DEVICE__ inline mat2 to_mat2_f    ( float  a                            )  { mat2 t; t.r0.x = a; t.r0.y = a; t.r1.x = a; t.r1.y = a;         return t; }
  __DEVICE__ inline mat2 to_mat2_s    ( float  a                            )  { mat2 t; t.r0.x = a;  t.r0.y = 0.0f; t.r1.x = 0.0f; t.r1.y = a;  return t; }
  __DEVICE__ inline mat2 to_mat2_f2_f2 ( float2 a, float2 b                  )  { mat2 t; t.r0 = a; t.r1 = b;                                     return t; }
  __DEVICE__ inline mat2 to_mat2_f_f3 ( float  a, float3 b                  )  { mat2 t; t.r0.x = a; t.r0.y = b.x; t.r1.x = b.y; t.r1.y = b.z;   return t; }
  __DEVICE__ inline mat2 to_mat2_f3_f ( float3 a, float  b                  )  { mat2 t; t.r0.x = a.x; t.r0.y = a.y; t.r1.x = a.z; t.r1.y = b;   return t; }


  __DEVICE__ inline mat2 mul_mat2_mat2( mat2 a, mat2 b)
  {
    mat2 t;
    t.r0.x = a.r0.x * b.r0.x + a.r0.y * b.r1.x;   t.r0.y = a.r0.x * b.r0.y + a.r0.y * b.r1.y;
    t.r1.x = a.r1.x * b.r0.x + a.r1.y * b.r1.x;   t.r1.y = a.r1.x * b.r0.y + a.r1.y * b.r1.y;
    return t;
  }


  __DEVICE__ inline float2 mul_f2_mat2( float2 v, mat2 m )
  {
    float2 t; t.x = v.x*m.r0.x + v.y*m.r0.y; t.y = v.x*m.r1.x + v.y*m.r1.y; return t;
  }


  __DEVICE__ inline float2 mul_mat2_f2( mat2 m, float2 v )
  {
    float2 t; t.x = v.x*m.r0.x + v.y*m.r1.x; t.y = v.x*m.r0.y + v.y*m.r1.y; return t;
  }


  __DEVICE__ inline mat2 mul_mat2_f( mat2 m, float s)
  {
    mat2 t;
    t.r0.x = s * m.r0.x; t.r0.y = s * m.r0.y;
    t.r1.x = s * m.r1.x; t.r1.y = s * m.r1.y;
    return t;
  }

  __DEVICE__ inline mat2 mul_f_mat2( float s, mat2 m) { return mul_mat2_f(m,s); }

#endif // end of mat2 implementation


// ----------------------------------------------------------------------------------------------------------
// mat3 implementation
// ----------------------------------------------------------------------------------------------------------

#if defined(USE_NATIVE_METAL_IMPL)

  typedef float3x3 mat3;

  __DEVICE__ inline mat3 to_mat3( float a, float b, float c, float d, float e, float f, float g, float h, float i)
  {
    return mat3(a,b,c,d,e,f,g,h,i);
  }

  __DEVICE__ inline mat3 to_mat3_f( float a ) { return mat3(a,a,a,a,a,a,a,a,a); }
  __DEVICE__ inline float3 mul_mat3_f3( mat3 B, float3 A) { return (B*A); }  
  __DEVICE__ inline float3 mul_f3_mat3( float3 A, mat3 B) { return (A*B); } 
  __DEVICE__ inline float3 mul_mat3_mat3( mat3 A, mat3 B) { return (A*B); } 

#else

  typedef struct { float3 r0; float3 r1; float3 r2; } mat3;

  __DEVICE__ inline mat3 to_mat3( float  a, float  b, float c,   float d, float e, float f,   float g, float h, float i)
  {
    mat3 t;
    t.r0.x = a; t.r0.y = b; t.r0.z = c;
    t.r1.x = d; t.r1.y = e; t.r1.z = f;
    t.r2.x = g; t.r2.y = h; t.r2.z = i;
    return t;
  }

  __DEVICE__ inline mat3 to_mat3_1( float  a )
  {
    mat3 t;
    t.r0.x = t.r0.y = t.r0.z = t.r1.x = t.r1.y = t.r1.z = t.r2.x = t.r2.y = t.r2.z = a;
    return t;
  }


__DEVICE__ inline float3 mul_mat3_f3( mat3 B, float3 A) {  
	float3 C;  

	C.x = A.x * B.r0.x + A.y * B.r1.x + A.z * B.r2.x;  
	C.y = A.x * B.r0.y + A.y * B.r1.y + A.z * B.r2.y;  
	C.z = A.x * B.r0.z + A.y * B.r1.z + A.z * B.r2.z;  
	return C;  
  }
 
__DEVICE__ inline float3 mul_f3_mat3( float3 A, mat3 B) {  
  float3 C; 
  
  C.x = A.x * B.r0.x + A.y * B.r0.y + A.z * B.r0.z;  
  C.y = A.x * B.r1.x + A.y * B.r1.y + A.z * B.r1.z;  
  C.z = A.x * B.r2.x + A.y * B.r2.y + A.z * B.r2.z; 
  return C;
 }
 
 __DEVICE__ mat3 mul_mat3_mat3( mat3 A, mat3 B)
{
  float r[3][3];  
  float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
                   {A.r1.x, A.r1.y, A.r1.z},  
                   {A.r2.x, A.r2.y, A.r2.z}};  
  float b[3][3] = {{B.r0.x, B.r0.y, B.r0.z},  
                   {B.r1.x, B.r1.y, B.r1.z},  
                   {B.r2.x, B.r2.y, B.r2.z}};  
     
  for( int i = 0; i < 3; ++i)  
  {  
	  for( int j = 0; j < 3; ++j)  
	  {  
		  r[i][j] = 0.0f;  
		  for( int k = 0; k < 3; ++k)  
		  {  
			  r[i][j] = r[i][j] + a[i][k] * b[k][j];  
		  }  
	  }  
  }  
  mat3 R = to_mat3(r[0][0], r[0][1], r[0][2],   
                   r[1][0], r[1][1], r[1][2], 
					         r[2][0], r[2][1], r[2][2]);  
  return R;  
}  
#endif // end of mat3 implementation






#if defined(USE_NATIVE_METAL_IMPL)

  #define fract_f(A)  fract(A)
  #define fract_f2(A) fract(A)
  #define fract_f3(A) fract(A)
  #define fract_f4(A) fract(A)

  #define mod_f(a,b)  modf((a),(b))
  #define mod_f2(value,divisor) modf(value,divisor)
  #define mod_f3(value,divisor) modf(value,divisor)
  #define mod_f4(value,divisor) modf(value,divisor)
  #define mod_f2f2(value,divisor) modf(value,divisor)
  #define mod_f3f3(value,divisor) modf(value,divisor)
  #define mod_f4f4(value,divisor) modf(value,divisor)

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
  #define mod_f3(value,divisor) to_float3(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)))
  #define mod_f4(value,divisor) to_float4(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)),mod_f((value).w, (divisor)))
  
  #define mod_f2f2(value,divisor) to_float2(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y));}
  #define mod_f3f3(value,divisor) to_float3(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z));}
  #define mod_f4f4(value,divisor) to_float4(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z),mod_f((value).w, (divisor).w));}

#endif
