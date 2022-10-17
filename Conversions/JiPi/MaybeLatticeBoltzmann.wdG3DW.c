
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Lichen' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

#define T(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define E(U) texture(iChannel3,(U)/R)



__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix( swi2(Q,x,y), to_float2_s(0.2f), Blend));

      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
    }
  
  return Q;
}




__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
__DEVICE__ float p (float4 b) {
  return 0.25f*(b.x+b.y+b.z+b.w);
}
__DEVICE__ float4 A(float2 U, float2 R, __TEXTURE2D__ iChannel0) {
    U-=v(T(U));
  return T(U);
}

__KERNEL__ void MaybeLatticeBoltzmannFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);


    U+=0.5f;
    Q = A(U,R,iChannel0);
    
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    float px = 0.25f*(p(e)-p(w));
    float py = 0.25f*(p(n)-p(s));
    Q += to_float4_s(0.25f*(n.w + e.y + s.z + w.x)-p(Q))-to_float4(px,-px,py,-py);

    
    float2 m = to_float2(0.9f,0.5f+0.3f*_sinf(0.01f*iTime))*R;
    if (iMouse.z>0.0f)                             m = swi2(iMouse,x,y);
    if (length(U-m) < 10.0f)                       Q.x=0.0f,Q.y=0.4f;    //swi2(Q,x,y) = to_float2(0.0f,0.4f);

    if (Blend1>0.0) Q = Blending(iChannel3,U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);

    if (iFrame < 1 || Reset)                             Q = to_float4_s(0.2f);
    if(U.x<3.0f||R.x-U.x<3.0f||U.y<3.0f||R.y-U.y<3.0f)   Q = to_float4_s(p(Q));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Lichen' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1

#ifdef XXX
#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
#define T(U) texture(iChannel2,(U)/R)
__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
__DEVICE__ float p (float4 b) {
  return 0.25f*(b.x+b.y+b.z+b.w);
}
#endif

__KERNEL__ void MaybeLatticeBoltzmannFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0); 
  
    U+=0.5f;
    U -= v(A(U,R,iChannel0));
    Q = D(U);
    if (iFrame < 1 || Reset) Q = to_float4(U.x,U.y,0,0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Stars' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

#ifdef XXX
#define R iResolution
#define T(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
__DEVICE__ float2 v (float4 b) {
  return to_float2(b.x-b.y,b.z-b.w);
}
__DEVICE__ float p (float4 b) {
  return 0.25f*(b.x+b.y+b.z+b.w);
}
__DEVICE__ float4 A(float2 U) {
  return T(U-v(T(U)));
}
#endif

__KERNEL__ void MaybeLatticeBoltzmannFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    U+=0.5f;
    Q = (texture(iChannel2,swi2(D(U),x,y)/R));

  SetFragmentShaderComputedColor(Q);
}