
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0));

#define Border if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;


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


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Cubemap: Forest_0' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2

__KERNEL__ void CelerityFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Seed, 1); 
    CONNECT_POINT1(SeedXY, 0.8f, 0.2f);

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = A(U);
    Neighborhood;
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f*to_float2(e.z-w.z,n.z-s.z)*Q.w);    
    
    if (iMouse.z>0.0f) {
      if (length(U-swi2(iMouse,x,y))<20.0f)            swi3S(Q,x,y,w, to_float3(0.2f*_cosf(iTime),0.2f*_sinf(iTime),0.5f+0.5f*_sinf(iTime)));
    }
    else if (Seed && length(U-SeedXY*R)<10.0f)   Q.x=-0.3f,Q.y=0.8f,Q.z=0.6f;//Q.xyw=to_float3(-0.3f,0.8f,0.6f);
    
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
    
    
    Border;
    if(iFrame < 1 ||Reset) Q = to_float4(0,0,0,_fmaxf(U.x/R.x-0.2f,0.0f));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CelerityFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = A(U);
    Neighborhood;
    Q.z -= 0.25f*(e.x*e.w-w.x*w.w+n.y*n.w-s.y*s.w);
    Border;

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void CelerityFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Neighborhood;
    U -= 0.25f*(swi2(n,x,y)+swi2(e,x,y)+swi2(s,x,y)+swi2(w,x,y));
    Q = A(U);
    Border;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void CelerityFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Neighborhood;
    Q = A(U);
    Q.w -= 0.25f*(n.w*n.y-s.w*s.y+e.x*e.w-w.x*w.w);
    Border;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CelerityFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Neighborhood;
    float2 g = to_float2(e.w*e.z-w.z*w.w,n.z*n.w-s.z*s.w);
    float3 no = normalize(to_float3_aw(g,0.01f));
    float3 re = reflect(no,to_float3(0,0,1));
    Q = 0.5f+0.5f*(1.0f-100.0f*length(g))*sin_f4(to_float4_s(_fabs(A(U).w)*3.1f+3.1f)+to_float4(1,2,3,4));
    Q *= 0.8f+0.2f*decube_f3(iChannel3,re);
    
  SetFragmentShaderComputedColor(Q);    
}