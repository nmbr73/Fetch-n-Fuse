

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

/*| swi2          |*/    #define swi2(A,a,b)     (A).a##b
/*| swi3          |*/    #define swi3(A,a,b,c)   (A).a##b##c
/*| swi4          |*/    #define swi4(A,a,b,c,d) (A).a##b##c##d

  #else

/*| swi2          |*/    #define swi2(A,a,b)     to_float2((A).a,(A).b)
/*| swi3          |*/    #define swi3(A,a,b,c)   to_float3((A).a,(A).b,(A).c)
/*| swi4          |*/    #define swi4(A,a,b,c,d) to_float4((A).a,(A).b,(A).c,(A).d)

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
  __DEVICE__ inline mat3 to_mat3_f3( float3 a, float3 b, float3 c ) { return mat3(a,b,c); }
  __DEVICE__ inline float3 mul_mat3_f3( mat3 B, float3 A) { return (B*A); }
  __DEVICE__ inline float3 mul_f3_mat3( float3 A, mat3 B) { return (A*B); }
  __DEVICE__ inline mat3 mul_mat3_mat3( mat3 A, mat3 B) { return (A*B); }

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

  __DEVICE__ inline mat3 to_mat3_f( float  a )
  {
    mat3 t;
    t.r0.x = t.r0.y = t.r0.z = t.r1.x = t.r1.y = t.r1.z = t.r2.x = t.r2.y = t.r2.z = a;
    return t;
  }

  __DEVICE__ inline mat3 to_mat3_f3( float3 A, float3 B, float3 C)
  {
	mat3 D;
	D.r0 = A;
	D.r1 = B;
	D.r2 = C;
	return D;
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

  /*| fract_f       |*/#define fract_f(A)  fract(A)
  /*| fract_f2      |*/#define fract_f2(A) fract(A)
  /*| fract_f3      |*/#define fract_f3(A) fract(A)
  /*| fract_f4      |*/#define fract_f4(A) fract(A)

  /*| mod_f         |*/#define mod_f(a,b)  fmod((a),(b))
  /*| mod_f2        |*/#define mod_f2(value,divisor) fmod(value,divisor)
  /*| mod_f3        |*/#define mod_f3(value,divisor) fmod(value,divisor)
  /*| mod_f4        |*/#define mod_f4(value,divisor) fmod(value,divisor)
  /*| mod_f2f2      |*/#define mod_f2f2(value,divisor) fmod(value,divisor)
  /*| mod_f3f3      |*/#define mod_f3f3(value,divisor) fmod(value,divisor)
  /*| mod_f4f4      |*/#define mod_f4f4(value,divisor) fmod(value,divisor)

  /*| sin_f2        |*/#define sin_f2(i) sin(i)
  /*| sin_f3        |*/#define sin_f3(i) sin(i)
  /*| sin_f4        |*/#define sin_f4(i) sin(i)
  /*| cos_f2        |*/#define cos_f2(i) cos(i)
  /*| cos_f3        |*/#define cos_f3(i) cos(i)
  /*| cos_f4        |*/#define cos_f4(i) cos(i)
  /*| acos_f3       |*/#define acos_f3(i) acos(i)
  /*| tan_f3        |*/#define tan_f3(i) tan(i)
  /*| tanh_f3       |*/#define tanh_f3(i) tanh(i)
  /*| atan_f2       |*/#define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
  /*| atan_f3       |*/#define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
  /*| atan_f4       |*/#define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
  /*| abs_f2        |*/#define abs_f2(a) _fabs(a)
  /*| abs_f3        |*/#define abs_f3(a) _fabs(a)
  /*| abs_f4        |*/#define abs_f4(a) _fabs(a)
  /*| sqrt_f2       |*/#define sqrt_f2(a) _sqrtf(a)
  /*| sqrt_f3       |*/#define sqrt_f3(a) _sqrtf(a)
  /*| sqrt_f4       |*/#define sqrt_f4(a) _sqrtf(a)
  /*| exp_f2        |*/#define exp_f2(a) _expf((a).x)
  /*| exp_f3        |*/#define exp_f3(a) _expf((a).x)
  /*| exp_f4        |*/#define exp_f4(a) _expf((a).x)
  /*| exp2_f2       |*/#define exp2_f2(a) _exp2f((a).x)
  /*| exp2_f3       |*/#define exp2_f3(a) _exp2f((a).x)
  /*| exp2_f4       |*/#define exp2_f4(a) _exp2f((a).x)
  /*| ceil_f2       |*/#define ceil_f2(a) ceil((a).x)
  /*| mix_f2        |*/#define mix_f2(v,i,m) mix(v,i,m)
  /*| mix_f3        |*/#define mix_f3(v,i,m) mix(v,i,m)
  /*| mix_f4        |*/#define mix_f4_f(v,i,m) mix(v,i,m)
  /*| log_f3        |*/#define log_f3(a) log(a)
  /*| log2_f3       |*/#define log2_f3(a) log2(a)
  /*| round_f2      |*/#define round_f2(a) round(a)
  /*| round_f3      |*/#define round_f3(a) round(a)
  /*| round_f4      |*/#define round_f4(a) round(a)

  /*| sign          |*/#define sign_f (value) sign(value)
  /*| sign          |*/#define sign_f2(a) sign(value)
  /*| sign          |*/#define sign_f3(a) sign(value)
  /*| sign          |*/#define sign_f4(a) sign(value)

  /*| distance_f    |*/#define distance_f ( p1, p2) _fabs(p1 - p2)
  /*| distance_f2   |*/#define distance_f2(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
  /*| distance_f3   |*/#define distance_f3(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))

  /*| pow_f2        |*/#define pow_f2(a,b) pow(a,b)
  /*| pow_f3        |*/#define pow_f3(a,b) pow(a,b)
  /*| pow_f4        |*/#define pow_f4(a,b) pow(a,b)

  /*| lessThan_f2   |*/#define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
  /*| lessThan_f3   |*/#define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
  /*| lessThan_f4   |*/#define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);

  /*| refract_f2    |*/#define refract_f2(I,N,eta) refract(I,N,eta)
  /*| refract_f3    |*/#define refract_f3(I,N,eta) refract(I,N,eta)


#else

  #if defined(USE_NATIVE_OPENCL_IMPL)

    #define reflect(I,N) (I-2.0f*dot(N,I)*N)

    #define fract(a) ((a)-_floor(a))  // oder Pointer bauen: gentype fract(gentype x, gentype *itpr)

    /*| fract_f       |*/#define fract_f(A)  fract(A)
    /*| fract_f2      |*/#define fract_f2(A) to_float2(fract((A).x),fract((A).y))
    /*| fract_f3      |*/#define fract_f3(A) to_float3(fract((A).x),fract((A).y),fract((A).z))
    /*| fract_f4      |*/#define fract_f4(A) to_float4(fract((A).x),fract((A).y),fract((A).z),fract((A).w))

    /*| mod_f         |*/#define mod_f(a,b) _fmod(a,b)
    /*| mod_f2        |*/#define mod_f2(value,divisor) _fmod(value,divisor)
    /*| mod_f3        |*/#define mod_f3(value,divisor) _fmod(value,divisor)
    /*| mod_f4        |*/#define mod_f4(value,divisor) _fmod(value,divisor)
    /*| mod_f2f2      |*/#define mod_f2f2(value,divisor) _fmod(value,divisor)
    /*| mod_f3f3      |*/#define mod_f3f3(value,divisor) _fmod(value,divisor)
    /*| mod_f4f4      |*/#define mod_f4f4(value,divisor) _fmod(value,divisor)

    /*| sin_f2        |*/#define sin_f2(i) sin(i)
    /*| sin_f3        |*/#define sin_f3(i) sin(i)
    /*| sin_f4        |*/#define sin_f4(i) sin(i)
    /*| cos_f2        |*/#define cos_f2(i) cos(i)
    /*| cos_f3        |*/#define cos_f3(i) cos(i)
    /*| cos_f4        |*/#define cos_f4(i) cos(i)
    /*| acos_f3       |*/#define acos_f3(i) acos(i)
    /*| tan_f3        |*/#define tan_f3(i) tan(i)
    /*| tanh_f3       |*/#define tanh_f3(i) tanh(i)
    /*| atan_f2       |*/#define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
    /*| atan_f3       |*/#define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
    /*| atan_f4       |*/#define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
    /*| abs_f2        |*/#define abs_f2(a) _fabs(a)
    /*| abs_f3        |*/#define abs_f3(a) _fabs(a)
    /*| abs_f4        |*/#define abs_f4(a) _fabs(a)
    /*| sqrt_f2       |*/#define sqrt_f2(a) _sqrtf(a)
    /*| sqrt_f3       |*/#define sqrt_f3(a) _sqrtf(a)
    /*| sqrt_f4       |*/#define sqrt_f4(a) _sqrtf(a)
    /*| exp_f2        |*/#define exp_f2(a) _expf((a).x)
    /*| exp_f3        |*/#define exp_f3(a) _expf((a).x)
    /*| exp_f4        |*/#define exp_f4(a) _expf((a).x)
    /*| exp2_f2       |*/#define exp2_f2(a) _exp2f((a).x)
    /*| exp2_f3       |*/#define exp2_f3(a) _exp2f((a).x)
    /*| exp2_f4       |*/#define exp2_f4(a) _exp2f((a).x)
    /*| ceil_f2       |*/#define ceil_f2(a) ceil((a).x)
    /*| mix_f2        |*/#define mix_f2(v,i,m) mix(v,i,m)
    /*| mix_f3        |*/#define mix_f3(v,i,m) mix(v,i,m)
    /*| mix_f4        |*/#define mix_f4_f(v,i,m) mix(v,i,m)
    /*| log_f3        |*/#define log_f3(a) log(a)
    /*| log2_f3       |*/#define log2_f3(a) log2(a)
    /*| round_f2      |*/#define round_f2(a) round(a)
    /*| round_f3      |*/#define round_f3(a) round(a)
    /*| round_f4      |*/#define round_f4(a) round(a)

    /*| sign          |*/#define sign_f (value) sign(value)
    /*| sign          |*/#define sign_f2(a) sign(value)
    /*| sign          |*/#define sign_f3(a) sign(value)
    /*| sign          |*/#define sign_f4(a) sign(value)

    /*| distance_f    |*/#define distance_f ( p1, p2) _fabs(p1 - p2)
    /*| distance_f2   |*/#define distance_f2(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
    /*| distance_f3   |*/#define distance_f3(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))

    /*| pow_f2        |*/#define pow_f2(a,b) pow(a,b)
    /*| pow_f3        |*/#define pow_f3(a,b) pow(a,b)
    /*| pow_f4        |*/#define pow_f4(a,b) pow(a,b)

    /*| lessThan_f2   |*/#define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
    /*| lessThan_f3   |*/#define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
    /*| lessThan_f4   |*/#define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);


    /*|               |*/ //-------refract--------
    /*| refract_f2    |*/__DEVICE__ float2 refract_f2(float2 I, float2 N, float eta) {
    /*| refract_f2    |*/    float dotNI = dot(N, I);
    /*| refract_f2    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f2    |*/    if (k < 0.0f) {
    /*| refract_f2    |*/       return to_float2_s(0.0f);
    /*| refract_f2    |*/    }
    /*| refract_f2    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N;
    /*| refract_f2    |*/ }
    /*|               |*/
    /*| refract_f3    |*/__DEVICE__ float3 refract_f3(float3 I, float3 N, float eta) {
    /*| refract_f3    |*/   float dotNI = dot(N, I);
    /*| refract_f3    |*/   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f3    |*/   if (k < 0.0f) {
    /*| refract_f3    |*/      return to_float3_s(0.0f);
    /*| refract_f3    |*/   }
    /*| refract_f3    |*/   return eta * I - (eta * dotNI * _sqrtf(k)) * N; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
    /*| refract_f3    |*/ }
    /*|               |*/

 #else // Generic

    #if defined(DEVICE_IS_OPENCL)
      __DEVICE__ float3 reflect(float3 I, float3 N) {return I - 2.0f * dot(N, I) * N;}
    #endif

    #if defined(DEVICE_IS_CUDA)
       #define radians(a) a * M_PI/180.0f
    #endif

    #define fract(a) ((a)-_floor(a))

    /*| fract_f       |*/#define fract_f(A)  fract(A)
    /*| fract_f2      |*/#define fract_f2(A) to_float2(fract((A).x),fract((A).y))
    /*| fract_f3      |*/#define fract_f3(A) to_float3(fract((A).x),fract((A).y),fract((A).z))
    /*| fract_f4      |*/#define fract_f4(A) to_float4(fract((A).x),fract((A).y),fract((A).z),fract((A).w))

    /*| mod_f         |*/#define mod_f(a,b) ((a)-(b)*_floor((a)/(b)))
    /*| mod_f2        |*/#define mod_f2(value,divisor) to_float2(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)))
    /*| mod_f3        |*/#define mod_f3(value,divisor) to_float3(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)))
    /*| mod_f4        |*/#define mod_f4(value,divisor) to_float4(mod_f((value).x, (divisor)),mod_f((value).y, (divisor)),mod_f((value).z, (divisor)),mod_f((value).w, (divisor)))
    /*| mod_f2f2      |*/#define mod_f2f2(value,divisor) to_float2(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y));}
    /*| mod_f3f3      |*/#define mod_f3f3(value,divisor) to_float3(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z));}
    /*| mod_f4f4      |*/#define mod_f4f4(value,divisor) to_float4(mod_f((value).x, (divisor).x),mod_f((value).y, (divisor).y),mod_f((value).z, (divisor).z),mod_f((value).w, (divisor).w));}

    /*| sin_f2        |*/#define sin_f2(i) to_float2( _sinf((i).x), _sinf((i).y))
    /*| sin_f3        |*/#define sin_f3(i) to_float3( _sinf((i).x), _sinf((i).y), _sinf((i).z))
    /*| sin_f4        |*/#define sin_f4(i) to_float4( _sinf((i).x), _sinf((i).y), _sinf((i).z), _sinf((i).w))
    /*| cos_f2        |*/#define cos_f2(i) to_float2( _cosf((i).x), _cosf((i).y))
    /*| cos_f3        |*/#define cos_f3(i) to_float3( _cosf((i).x), _cosf((i).y), _cosf((i).z))
    /*| cos_f4        |*/#define cos_f4(i) to_float4( _cosf((i).x), _cosf((i).y), _cosf((i).z), _cosf((i).w))
    /*| acos_f3       |*/#define acos_f3(i) to_float3( _acosf((i).x), _acosf((i).y), _acosf((i).z))
    /*| tan_f3        |*/#define tan_f3(i) to_float3(_tanf((i).x), _tanf((i).y), _tanf((i).z))
    /*| tanh_f3       |*/#define tanh_f3(i) to_float3(_tanhf(i.x), _tanhf((i).y), _tanhf((i).z))
    /*| atan_f2       |*/#define atan_f2(i, j) to_float2( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y))
    /*| atan_f3       |*/#define atan_f3(i, j) to_float3( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z))
    /*| atan_f4       |*/#define atan_f4(i, j) to_float4( _atan2f((i).x,(j).x), _atan2f((i).y,(j).y), _atan2f((i).z,(j).z), _atan2f((i).w,(j).w))
    /*| abs_f2        |*/#define abs_f2(a) to_float2(_fabs((a).x), _fabs((a).y))
    /*| abs_f3        |*/#define abs_f3(a) to_float3(_fabs((a).x), _fabs((a).y),_fabs((a).z))
    /*| abs_f4        |*/#define abs_f4(a) to_float4(_fabs((a).x), _fabs((a).y),_fabs((a).z),_fabs((a).w))
    /*| sqrt_f2       |*/#define sqrt_f2(a) to_float2(_sqrtf((a).x),_sqrtf((a).y))
    /*| sqrt_f3       |*/#define sqrt_f3(a) to_float3(_sqrtf((a).x),_sqrtf((a).y),_sqrtf((a).z))
    /*| sqrt_f4       |*/#define sqrt_f4(a) to_float4(_sqrtf((a).x),_sqrtf((a).y),_sqrtf((a).z),_sqrtf((a).w);)
    /*| exp_f2        |*/#define exp_f2(a) to_float2(_expf((a).x), _expf((a).y))
    /*| exp_f3        |*/#define exp_f3(a) to_float3(_expf((a).x), _expf((a).y),_expf((a).z))
    /*| exp_f4        |*/#define exp_f4(a) to_float4(_expf((a).x), _expf((a).y),_expf((a).z),_expf((a).w))
    /*| exp2_f2       |*/#define exp2_f2(a) to_float2(_exp2f((a).x), _exp2f((a).y))
    /*| exp2_f3       |*/#define exp2_f3(a) to_float3(_exp2f((a).x), _exp2f((a).y), _exp2f((a).z))
    /*| exp2_f4       |*/#define exp2_f4(a) to_float4(_exp2f((a).x), _exp2f((a).y), _exp2f((a).z), _exp2f((a).w))
    /*| ceil_f2       |*/#define ceil_f2(a) to_float2(_ceil((a).x), _ceil((a).y))
    /*| mix_f2        |*/#define mix_f2(v,i,m) to_float2(_mix((v).x,(i).x,m.x),_mix((v).y,(i).y,(m).y))
    /*| mix_f3        |*/#define mix_f3(v,i,m) to_float3(_mix((v).x,(i).x,m.x),_mix((v).y,(i).y,(m).y),_mix((v).z,(i).z,(m).z))
    /*| mix_f4        |*/#define mix_f4_f(v,i,m) to_float4(_mix((v).x,(i).x,m),_mix((v).y,(i).y,m),_mix((v).z,(i).z,m),_mix((v).w,(i).w,m))
    /*| log_f3        |*/#define log_f3(a) to_float3(_logf((a).x), _logf((a).y),_logf((a).z))
    /*| log2_f3       |*/#define log2_f3(a) to_float3(_log2f((a).x), _log2f((a).y),_log2f((a).z))
    /*| round_f2      |*/#define round_f2(a) to_float2(_round((a).x), _round((a).y))
    /*| round_f3      |*/#define round_f3(a) to_float3(_round((a).x), _round((a).y),_round((a).z))
    /*| round_f4      |*/#define round_f4(a) to_float4(_round((a).x), _round((a).y),_round((a).z),_round((a).w))

    /*| sign          |*/#define sign_f (value) (value == 0.0f ? 0.0f : value > 0.0f ? 1.0f : -1.0f)
    /*| sign          |*/#define sign_f2(a) to_float2(sign_f((a).x), sign_f((a).y))
    /*| sign          |*/#define sign_f3(a) to_float3(sign_f((a).x), sign_f((a).y),sign_f((a).z))
    /*| sign          |*/#define sign_f4(a) to_float4(sign_f((a).x), sign_f((a).y),sign_f((a).z),sign_f((a).w))

    /*| distance_f    |*/#define distance_f ( p1, p2) _fabs(p1 - p2)
    /*| distance_f2   |*/#define distance_f2(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))
    /*| distance_f3   |*/#define distance_f3(pt1,pt2) _sqrtf(dot(pt2 - pt1,pt2 - pt1))

    /*| pow_f2        |*/#define pow_f2(a,b) to_float2(_powf((a).x,(b).x),_powf((a).y,(b).y))
    /*| pow_f3        |*/#define pow_f3(a,b) to_float3(_powf((a).x,(b).x),_powf((a).y,(b).y),_powf((a).z,(b).z))
    /*| pow_f4        |*/#define pow_f4(a,b) to_float4(_powf((a).x,(b).x),_powf((a).y,(b).y),_powf((a).z,(b).z),_powf((a).w,(b).w))

    /*| lessThan_f2   |*/#define lessThan_2f(a,b) to_float2((a).x < (b).x,(a).y < (b).y);
    /*| lessThan_f3   |*/#define lessThan_3f(a,b) to_float3((a).x < (b).x,(a).y < (b).y,(a).z < (b).z);
    /*| lessThan_f4   |*/#define lessThan_4f(a,b) to_float4((a).x < (b).x,(a).y < (b).y,(a).z < (b).z,(a).w < (b).w);


    /*|               |*/ //-------refract--------
    /*| refract_f2    |*/__DEVICE__ float2 refract_f2(float2 I, float2 N, float eta) {
    /*| refract_f2    |*/    float dotNI = dot(N, I);
    /*| refract_f2    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f2    |*/    if (k < 0.0f) {
    /*| refract_f2    |*/      return to_float2_s(0.0f);
    /*| refract_f2    |*/    }
    /*| refract_f2    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N;
    /*| refract_f2    |*/ }
    /*|               |*/
    /*| refract_f3    |*/__DEVICE__ float3 refract_f3(float3 I, float3 N, float eta) {
    /*| refract_f3    |*/    float dotNI = dot(N, I);
    /*| refract_f3    |*/    float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
    /*| refract_f3    |*/    if (k < 0.0f) {
    /*| refract_f3    |*/      return to_float3_s(0.0f);
    /*| refract_f3    |*/    }
    /*| refract_f3    |*/    return eta * I - (eta * dotNI * _sqrtf(k)) * N; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
    /*| refract_f3    |*/ }
    /*|               |*/
  #endif

#endif


//#define floor_f2(V) to_float2(_floor((V).x),_floor((V).y)) // not needed?!?
#define to_int2_f2(V) to_int2_cfloat(V)   // float2 zu int2
#define to_int2_ui2(V) to_int2_cuint(V)   // uint2 zu int2
#define to_int2_2f(A,B) to_int2((int)(A),(int)(B))
//#define to_int2_s(V) to_int2((V),(V))   // existiert schon
#define mod_i2(V,I) to_int2( (V).x % (I), (V).y % (I)  )

// #define eq_i2_i2(A,B) ((A).x==(B).x && (A).y==(B).y)
// #define eq_f2_f2(A,B) ((A).x==(B).x && (A).y==(B).y)
// #define eq_i2_1i(A,I) ((A).x==(I) && (A).y==(I))
// #define eq_f2_1f(A,I) ((A).x==(I) && (A).y==(I))

#define to_float2_i2(V) to_float2_cint(V)   // int2 zu float2
#define to_float2_ui2(V) to_float2_cuint(V) // uint2 zu float2
#define to_float3_i3(V) to_float3_cint(V)   // int2 zu float2
#define to_float3_ui3(V) to_float3_cuint(V) // uint2 zu float2
#define to_float4_i4(V) to_float4_cint(V)   // int2 zu float2
#define to_float4_ui4(V) to_float4_cuint(V) // uint2 zu float2

#define to_float4_f2f2(A,B) to_float4((A).x,(A).y,(B).x,(B).y ) // or is there some to_float_..() for that?!? - No - that is missing in DCTL :-) but now we have "one"
