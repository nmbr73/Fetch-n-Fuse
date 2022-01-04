

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

    #define swi2(A,a,b)     (A).a##b

  #else

    #define swi2(A,a,b)     to_float2((A).a,(A).b)

  #endif

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

#else

  #if defined(USE_NATIVE_OPENCL_IMPL)

    #define reflect(I,N) (I-2.0f*dot(N,I)*N)

    #define fract(a) ((a)-_floor(a))  // oder Pointer bauen: gentype fract(gentype x, gentype *itpr)

 //-------refract--------

 #else // Generic

    #if defined(DEVICE_IS_OPENCL)
      __DEVICE__ float3 reflect(float3 I, float3 N) {return I - 2.0f * dot(N, I) * N;}
    #endif

    #if defined(DEVICE_IS_CUDA)
       #define radians(a) a * M_PI/180.0f
    #endif

    #define fract(a) ((a)-_floor(a))

 //-------refract--------

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




// ##################################################################################




__DEVICE__ float4 A (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return _tex2DVecN(iChannel0,U.y/R.x,U.y/R.y,15);}
__DEVICE__ float4 B (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return _tex2DVecN(iChannel1,U.y/R.x,U.y/R.y,15);}
__DEVICE__ float4 C (float2 U, float2 R, __TEXTURE2D__ iChannel2) {return _tex2DVecN(iChannel2,U.y/R.x,U.y/R.y,15);}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0
// Connect 'Previsualization: Buffer B' to iChannel1
// Connect 'Previsualization: Buffer D' to iChannel3


// Fluid
//float2 R;
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}

__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {return A(swi2(B(U,R,iChannel1),x,y),R,iChannel0);}

__KERNEL__ void EverythingFuse__Buffer_A(__CONSTANTREF__ ShaderParameters*  _shaderParameters,  __TEXTURE2D__ iChannel0,  __TEXTURE2D__ iChannel1,  __TEXTURE2D__ iChannel3, __TEXTURE2D_WRITE__ _shaderDestinationTexture)
{
   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);

   if (fusion_x >= _shaderParameters->width || fusion_y >= _shaderParameters->height)
     return;

  float4 C   = to_float4_s(0.0f);
  float2 U   = to_float2(fusion_x,fusion_y);
  float2 iResolution = to_float2(_shaderParameters->iResolution[0], _shaderParameters->iResolution[1]);
  float  iTime       = _shaderParameters->iTime;
  int    iFrame      = _shaderParameters->iFrame;


  // --------



   float2 R = iResolution;
   C = T(U,R,iChannel0,iChannel1);
   float4
        n = T(U+to_float2(0,1),R,iChannel0,iChannel1),
        e = T(U+to_float2(1,0),R,iChannel0,iChannel1),
        s = T(U-to_float2(0,1),R,iChannel0,iChannel1),
        w = T(U-to_float2(1,0),R,iChannel0,iChannel1);
   C.x -= 0.25f*(e.z-w.z+(n.w*C.w-s.w*C.w));
   C.y -= 0.25f*(n.z-s.z+(e.w*C.w-w.w*C.w));
   C.z  = 0.25f*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   C.w  = 0.25f*((s.x-n.x+w.y-e.y)-(n.w+e.w+s.w+w.w));


   float2 Cxy = swi2(C,x,y) + _expf(-length(swi2(U,x,y)-0.5f*R))*(0.9f*to_float2(_sinf(0.2f*iTime),_cosf(0.2f*iTime))-swi2(C,x,y));
   C.x=Cxy.x;C.y=Cxy.y;

   if (U.x < 1.0f||R.x-U.x<1.0f)    C.x==0.0f,C.y*=0.0f; //    swi2(C,x,y)*=0.0f;
   if (U.y < 1.0f||R.y-U.y<1.0f)    C.x==0.0f,C.y*=0.0f; //    swi2(C,x,y)*=0.0f;
   if (iFrame < 1) C = to_float4_s(0);
   float4 mo = _tex2DVecN(iChannel3,0.0f,0.0f,15);
   float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
   if (mo.z > 0.0f && l < 2.0f) C += to_float4((3.0f*(2.0f-l)*(swi2(mo,x,y)-swi2(mo,z,w))/R.y).x,(3.0f*(2.0f-l)*(swi2(mo,x,y)-swi2(mo,z,w))/R.y).y,0,0);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0
// Connect 'Previsualization: Buffer B' to iChannel1

//vec2 R;


__KERNEL__ void EverythingFuse__Buffer_B(__CONSTANTREF__ ShaderParameters*  _shaderParameters,  __TEXTURE2D__ iChannel0,  __TEXTURE2D__ iChannel1, __TEXTURE2D_WRITE__ _shaderDestinationTexture)
{
   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);

   if (fusion_x >= _shaderParameters->width || fusion_y >= _shaderParameters->height)
     return;

  float4 C   = to_float4_s(0.0f);
  float2 U   = to_float2(fusion_x,fusion_y);
  float2 iResolution = to_float2(_shaderParameters->iResolution[0], _shaderParameters->iResolution[1]);


  // --------



    float2 R = iResolution;
    float
      n = A(U+to_float2(0,1),R,iChannel0).z,
      e = A(U+to_float2(1,0),R,iChannel0).z,
      s = A(U-to_float2(0,1),R,iChannel0).z,
      w = A(U-to_float2(1,0),R,iChannel0).z;
    #define N 2.0f
    for (float i = 0.0f; i < N; i+=1.0f)
        U -= swi2(A(U,R,iChannel0),x,y)/N;
    C.x = U.x;
    C.y = U.y;
    C.z = e-w;
    C.w = n-s;


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0
// Connect 'Previsualization: Buffer B' to iChannel1
// Connect 'Previsualization: Buffer C' to iChannel2


//vec2 R;
#define D 5
__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
//__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
//__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}

__DEVICE__ float dI (float2 U, float3 me, float3 light, float mu, float2 R, __TEXTURE2D__ iChannel1)  {
    float3 r = to_float3_aw(U,100);
    float3 n = normalize(to_float3_aw(swi2(B(swi2(r,x,y),R,iChannel1),z,w),mu));
    float3 li = reflect((r-light),n);
    float len = ln(me,r,li);

    return 2.5f*_expf(-1.7f*len);
}
__DEVICE__ float I (float2 U, float3 me, float3 light, float mu, float2 R, __TEXTURE2D__ iChannel1) {
    float intensity = 0.0f;

    for (int x = -D; x <= D; x+=1.0f)
    for (int y = -D; y <= D; y+=1.0f)
      intensity += dI(U+to_float2(x,y),me,light,0.1f*mu,R,iChannel1);
    return intensity;
}
__DEVICE__ float3 S (float2 U, float3 me, float3 light, float mu, float2 R, __TEXTURE2D__ iChannel1) {

    return I (U,me,light,mu,R,iChannel1)*to_float3(_expf(-(mu-0.5f)*(mu-0.5f)), _expf(-(mu-1.0f)*(mu-1.0f)), _expf(-(mu-1.4f)*(mu-1.4f)));
}
__KERNEL__ void EverythingFuse__Buffer_C(__CONSTANTREF__ ShaderParameters*  _shaderParameters,  __TEXTURE2D__ iChannel0,  __TEXTURE2D__ iChannel1,  __TEXTURE2D__ iChannel2, __TEXTURE2D_WRITE__ _shaderDestinationTexture)
{
   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);

   if (fusion_x >= _shaderParameters->width || fusion_y >= _shaderParameters->height)
     return;

  float4 Q   = to_float4_s(0.0f);
  float2 U   = to_float2(fusion_x,fusion_y);
  float2 iResolution = to_float2(_shaderParameters->iResolution[0], _shaderParameters->iResolution[1]);


  // --------



    float2 R = iResolution;
    float3 light = to_float3_aw(0.5f*R,1e5);
    float3 me    = to_float3_aw(U,0);

    float3 c = to_float3_s(0);
    for (float mu = 0.4f; mu <= 1.6f; mu+=0.4f)
        c += S(U,me,light,mu,R,iChannel1);
    Q = to_float4_aw(0.03f*c,1);
    if (R.x >= 800.0f) Q = _mix(Q,C(U,R,iChannel2),0.5f);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer D' to iChannel0


//Mouse
__KERNEL__ void EverythingFuse__Buffer_D(__CONSTANTREF__ ShaderParameters*  _shaderParameters,  __TEXTURE2D__ iChannel0, __TEXTURE2D_WRITE__ _shaderDestinationTexture)
{
   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);

   if (fusion_x >= _shaderParameters->width || fusion_y >= _shaderParameters->height)
     return;

  float4 C   = to_float4_s(0.0f);
  float2 U   = to_float2(fusion_x,fusion_y);
  float2 iResolution = to_float2(_shaderParameters->iResolution[0], _shaderParameters->iResolution[1]);
  float4 iMouse      = to_float4(_shaderParameters->iMouse[0],_shaderParameters->iMouse[1],_shaderParameters->iMouse[2],_shaderParameters->iMouse[3]);


  // --------



    float4 p = _tex2DVecN(iChannel0,U.x/iResolution.x,U.y/iResolution.y,15);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else C = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0
// Connect 'Previsualization: Buffer B' to iChannel1
// Connect 'Previsualization: Buffer C' to iChannel2


//vec2 R;
//__DEVICE__ float4 A (float2 U) {return texture(iChannel0,U/R);}
//__DEVICE__ float4 B (float2 U) {return texture(iChannel1,U/R);}
//__DEVICE__ float4 C (float2 U) {return texture(iChannel2,U/R);}
__KERNEL__ void EverythingFuse(__CONSTANTREF__ ShaderParameters*  _shaderParameters,  __TEXTURE2D__ iChannel0,  __TEXTURE2D__ iChannel1,  __TEXTURE2D__ iChannel2, __TEXTURE2D_WRITE__ _shaderDestinationTexture)
{
   DEFINE_KERNEL_ITERATORS_XY(fusion_x, fusion_y);

   if (fusion_x >= _shaderParameters->width || fusion_y >= _shaderParameters->height)
     return;

  float4 Q   = to_float4_s(0.0f);
  float2 U   = to_float2(fusion_x,fusion_y);
  float2 iResolution = to_float2(_shaderParameters->iResolution[0], _shaderParameters->iResolution[1]);
  float4 iMouse      = to_float4(_shaderParameters->iMouse[0],_shaderParameters->iMouse[1],_shaderParameters->iMouse[2],_shaderParameters->iMouse[3]);


  // --------



    float2 R = iResolution;
    float2 M = iMouse.z>0.0f?swi2(iMouse,x,y):0.5f*R;
    float2 r = 2.0f*(U-M)/R.y;
    r = r/_sqrtf(length(r));
    Q = to_float4_s(0);
    for (float i = 1.0f; i < 10.0f; i+=1.0f) {
        float4 c = C(U-i*r,R,iChannel2);
        Q += c*c*_expf(-0.2f*i);
    }
    Q = _mix(C(U,R,iChannel2),0.8f*Q*_expf(-1.5f*length(r)),0.5f);


  SetFragmentShaderComputedColor(Q);
}