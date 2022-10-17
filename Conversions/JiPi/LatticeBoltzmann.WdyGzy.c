
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// FLUID EVOLUTION
#define R iResolution
#define T(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
#define B(U) texture(iChannel2,(U)/R)
// Velocity
__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
// Pressure
__DEVICE__ float p (float4 b) {
  return 0.25f*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
__DEVICE__ float4 A(float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    U-=0.5f*v(T(U));
    U-=0.5f*v(T(U));
  return T(U);
}
__KERNEL__ void LatticeBoltzmannFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  float AAAAAAAAAAAAAAAAAA;
    U+=0.5f;
    // THIS PIXEL
    Q = A(U,R,iChannel0);
    // NEIGHBORHOOD
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // GRADIENT of PRESSURE
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s)); 
    
        // boundary Energy exchange in :   
    Q = Q + to_float4_s(0.25f*(n.w + e.y + s.z + w.x))
          // boundary Energy exchange out :
          -to_float4_s(p(Q))
          // dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
          -to_float4(px,-px,py,-py);
    
    // get value from picture buffer
    float z = 0.8f-length(swi3(B(U),x,y,z));
    // some kind of viscolsity thing 
    Q = _mix(_mix(Q,0.25f*(n+e+s+w),0.01f),to_float4_s(p(Q)),0.01f*(1.0f-z));
    // gravity polarizes energy! pretty cool imo
    //swi2(Q,z,w) -= 0.001f*z*to_float2(1,-1);
    Q.z -= 0.001f*z*(1);
    Q.w -= 0.001f*z*(-1);
        
    // Init with no velocity and some pressure
    if (iFrame < 1||(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<R.y/5.0f))  Q = to_float4_s(0.2f);
    // At boundarys turn all kinetic energy into potential energy
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)  Q = to_float4_s(p(Q));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Lichen' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


// LOOK UP PICTURE IN LOCATION FROM BUFFER D
//#define R iResolution
//#define T(U) texture(iChannel0,(U)/R)
//#define D(U) texture(iChannel1,(U)/R)
__KERNEL__ void LatticeBoltzmannFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
float BBBBBBBBBBBBBBBBBBB;    
    Q = texture(iChannel2,swi2(D(U),x,y)/R);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Lighting on Buffer B
//#define R iResolution
//#define T(U) texture(iChannel0,(U)/R)

__KERNEL__ void LatticeBoltzmannFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
   U+=0.5f;
float CCCCCCCCCCCCCCCCCCCC;
   Q =  to_float4_s(1.2f)-2.2f*T(U);
   swi3S(Q,x,y,z, swi3(Q,x,y,z)+0.5f*normalize(swi3(Q,x,y,z)));
   float
       n = length(T(U+to_float2(0,1))),
       e = length(T(U+to_float2(1,0))),
       s = length(T(U-to_float2(0,1))),
       w = length(T(U-to_float2(1,0)));
    float3 no = normalize(to_float3(e-w,n-s,1));
    float d = dot(reflect(no,to_float3(0,0,1)),normalize(to_float3_s(1)));
    Q *= 8.0f*_expf(-3.0f*d*d);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// TRANSLATE LOCATION FIELD WITH v(A(coord)), INIT WITH FragCoord

#ifdef XXX
#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define d(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
#endif

__DEVICE__ float4 DD(float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    U-=0.5f*v(T(U));
    U-=0.5f*v(T(U));
  return D(U);
}

__KERNEL__ void LatticeBoltzmannFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
float DDDDDDDDDDDDDDDDDDDDD;     
    Q = DD(U, R,iChannel0,iChannel1);
    
    float4 
        q = T(U),
        n = T(U+to_float2(0,1)),
        e = T(U+to_float2(1,0)),
        s = T(U-to_float2(0,1)),
        w = T(U-to_float2(1,0)),
        N = DD(U+to_float2(0,1), R,iChannel0,iChannel1),
        E = DD(U+to_float2(1,0), R,iChannel0,iChannel1),
        S = DD(U-to_float2(0,1), R,iChannel0,iChannel1),
        W = DD(U-to_float2(1,0), R,iChannel0,iChannel1);
    Q = Q + 0.25f*((n.w-q.z)*(N-Q) + (e.y-q.x)*(E-Q) + (s.z-q.w)*(S-Q) + (w.x-q.y)*(W-Q));
    
    if (iFrame < 1||(iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<R.y/5.0f))  Q = to_float4(U.x,U.y,0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


// LENS FLAIR EFFECT
//#define R iResolution
//#define T(U) texture(iChannel0,(U)/R)
__DEVICE__ float4 F (float2 U,float2 r, float2 R, __TEXTURE2D__ iChannel0) {
  float4 t = T(U+r);
  float FFFFFFFFFFFFFFFFFFFFFFF;
    //return clamp(_expf(-0.01f*dot(r,r))*(exp_f4(2.0f*t)-1.0f) , -20.0f,20.0f);
    return _expf(-0.01f*dot(r,r))*(exp_f4(2.0f*t)-1.0f);
}
__KERNEL__ void LatticeBoltzmannFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
  CONNECT_SLIDER0(Convolve1, 0.0f, 2.0f, 1.1f);
  CONNECT_SLIDER1(Convolve2, -1.0f, 3.0f, 1.0f);
  
   U+=0.5f;
float IIIIIIIIIIIIIIIIIIIIII;
   Q = to_float4_s(0);
    for (float i = 0.0f; i < 7.0f; i+=Convolve1) {
      Q += F(U,to_float2(-i,i), R,iChannel0);
      Q += F(U,to_float2(i,i), R,iChannel0);
      Q += F(U,to_float2(i,-i), R,iChannel0);
      Q += F(U,to_float2(-i,-i), R,iChannel0);
    }
    Q = T(U)*0.15f + 1e-5*Q*Convolve2;
    Q = atan_f4(Q, to_float4_s(1.0f));


  SetFragmentShaderComputedColor(Q);
}