

__DEVICE__ float4 A (float2 U, float2 R, __TEXTURE2D__ iChannel0) {return _tex2DVecN(iChannel0,U.x/R.x,U.y/R.y,15);}
__DEVICE__ float4 B (float2 U, float2 R, __TEXTURE2D__ iChannel1) {return _tex2DVecN(iChannel1,U.x/R.x,U.y/R.y,15);}
__DEVICE__ float4 C (float2 U, float2 R, __TEXTURE2D__ iChannel2) {return _tex2DVecN(iChannel2,U.x/R.x,U.y/R.y,15);}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0
// Connect 'Previsualization: Buffer B' to iChannel1
// Connect 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}

__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {return A(swi2(B(U,R,iChannel1),x,y),R,iChannel0);}

__KERNEL__ void EverythingFuse__Buffer_A(float4 C, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

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
   
   if (U.x < 1.0f||R.x-U.x<1.0f)    C.x=0.0f,C.y*=0.0f; //    swi2(C,x,y)*=0.0f;
   if (U.y < 1.0f||R.y-U.y<1.0f)    C.x=0.0f,C.y*=0.0f; //    swi2(C,x,y)*=0.0f;
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

__KERNEL__ void EverythingFuse__Buffer_B(float4 C, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

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
__KERNEL__ void EverythingFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
     
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
__KERNEL__ void EverythingFuse__Buffer_D(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

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

__KERNEL__ void EverythingFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

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