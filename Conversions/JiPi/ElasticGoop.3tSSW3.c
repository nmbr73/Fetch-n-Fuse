
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


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
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          swi2S(Q,x,y, _mix( swi2(Q,x,y), (swi2(tex,x,y)+MulOff.y)*MulOff.x, Blend));  
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix(swi2(Q,x,y),  Par, Blend));

      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
    }
  
  return Q;
}






#define k 0.6f
#define c 0.003f
#define f 0.5f
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


#define R iResolution
#define a(U) texture(iChannel0,(U)/R)
__DEVICE__ float4 A (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
  return a(U-swi2(a(U),x,y));
}
#define B(U) texture(iChannel1,(U)/R)
__KERNEL__ void ElasticGoopFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    CONNECT_CHECKBOX1(Vertical, 0); 
    CONNECT_SLIDER3(Orange, -1500.0f, 1500.0f, 80.0f);
    CONNECT_SLIDER4(Green, -1500.0f, 1500.0f, 0.0f);

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = A(U,R,iChannel0);
    float4 
        b = B(U),
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0),
        nb = B(U+to_float2(0,1)),
        eb = B(U+to_float2(1,0)),
        sb = B(U-to_float2(0,1)),
        wb = B(U-to_float2(1,0));
    float dw = 0.25f*(e.w*e.x-w.w*w.x+n.w*n.y-s.w*s.y);
    Q.z = 0.25f*(n.z+e.z+s.z+w.z+e.x-w.x+n.y-s.y);
    swi2S(Q,x,y, swi2(Q,x,y) + 0.05f*(0.25f*swi2((n+e+s+w),x,y)-swi2(Q,x,y)));
    Q.x += 0.25f*(e.z-w.z);
    Q.y += 0.25f*(n.z-s.z);
    
    Q.w -= dw;
    Q.y += c*(Q.w);
    swi2S(Q,x,y, swi2(Q,x,y) * (1.0f-0.01f*step(_fabs(Q.w),0.01f))); //!!
    swi2S(Q,x,y, swi2(Q,x,y) + Q.w*k*swi2(b,x,y));
    if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<10.0f) {swi3S(Q,x,y,w, 0.7f*to_float3(_cosf(0.5f*iTime),_sinf(0.5f*iTime),_sinf(iTime)));}
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
    
    if (Vertical)
    {
      if (iFrame < 1 || Reset)                            Q.w = 0.5f-0.5f*sign_f(U.x-Orange-Green);
      if ((iFrame<1 || Reset) && R.y-U.x<Orange)          Q.w = -1.0f;
    }
    else    
    {
      //Original
      if (iFrame < 1 || Reset)                            Q.w = 0.5f-0.5f*sign_f(U.y-80.0f);
      if ((iFrame<1 || Reset) && R.y-U.y<80.0f)           Q.w = -1.0f;
    }
    
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*= 0.0f,Q.y*=0.0f,Q.w*=0.0f;//swi3(Q,x,y,w)*= 0.0f;
    
    swi2S(Q,x,y, clamp(swi2(Q,x,y),-1.0f,1.0f));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1

#ifdef XXX
#define R iResolution
#define aa(U) texture(iChannel0,(U)/R)
__DEVICE__ float4 A (float2 U) {
  return aa(U-swi2(aa(U),x,y));
}
#endif

#define b(U) texture(iChannel1,(U)/R)
__DEVICE__ float4 BB (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
  return b(U-swi2(a(U),x,y));
}
__KERNEL__ void ElasticGoopFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
  
    U+=0.5f;
    Q = BB(U,R,iChannel0,iChannel1);
    float4 a = A(U,R,iChannel0);
    float4 
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    swi2S(Q,x,y, swi2(Q,x,y) + a.w*k*(0.25f*swi2((n+e+s+w),x,y) - swi2(a,x,y)));
    
    Q.w = (a.w+n.w+e.w+s.w+w.w)/5.0f;

  if (iFrame < 1 || Reset) Q=to_float4_s(0.0f);                 

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//#define R iResolution
//#define a(U) texture(iChannel0,(U)/R)
//#define b(U) texture(iChannel1,(U)/R)

__KERNEL__ void ElasticGoopFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    U+=0.5f;
    float4 A = a(U);
    float4 
        n = a(U+to_float2(0,1)),
        e = a(U+to_float2(1,0)),
        s = a(U-to_float2(0,1)),
        w = a(U-to_float2(1,0)),
        u = 0.25f*(n+e+s+w), 
        l = u-A;
     float3 g = normalize(to_float3(e.w-w.w,n.w-s.w,0.4f)),
            r = reflect(g,to_float3(0,0,1));
     float q = _fmaxf(0.0f,dot(r,normalize(to_float3(1,1,-1))));
     Q = _expf(-2.0f*q*q)*(0.3f+_fmaxf(sin_f4(5.31f+l+2.0f*_atan2f(4.0f*A.w,1.0f)*to_float4(-1,1,2,4)),to_float4_s(-0.2f)));

  SetFragmentShaderComputedColor(Q);
}