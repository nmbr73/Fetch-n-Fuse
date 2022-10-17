
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)

#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)


__DEVICE__ float map (float2 u,float2 r) {
    u = 4.0f*(u-to_float2(0.3f,0.5f)*r)/r.y;
    float a = 2.5f;
    u = mul_f2_mat2(u,to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)));
    float2 c = to_float2(1.0f,0.4f);
    for (int i = 0; i < 10; i++) 
      u = to_float2(u.x*u.x-u.y*u.y,2.0f*u.x*u.y)-c;
  return length(u);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0
// Connect Buffer A 'Texture: Tex' to iChannel1

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
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
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}

// divergence step

__KERNEL__ void FluidAutomataFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Tex, 0);
    CONNECT_CHECKBOX2(MapOff, 0);
    CONNECT_SLIDER0(AlphaLevel, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(MouseSize, -1.0f, 10.0f, 1.0f);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
        
    
    U+=0.5f;

    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    Q = A(U);
    Q.z -= 0.25f*(n.y-s.y+e.x-w.x);
    Q.w -= 0.125f*(n.y*n.w-s.y*s.w+e.x*e.w-w.x*w.w);
    
    if (isnan(Q.x)) Q.x = 0.00001f;
    if (isnan(Q.y)) Q.y = 0.00001f;
    if (isnan(Q.z)) Q.z = 0.00001f;
    if (isnan(Q.w)) Q.w = 0.00001f;
    
    if (isinf(Q.x)) Q.x = 100000.0000f;
    if (isinf(Q.y)) Q.y = 100000.0000f;
    if (isinf(Q.z)) Q.z = 100000.0000f;
    if (isinf(Q.w)) Q.w = 100000.0000f;  
    
float AAAAAAAAAAAAAA;    
    //Boundary Conditions
    float M = 0.5f;
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q = to_float4(M,0,Q.z,0);
    if (iFrame < 1 || Reset)     Q = to_float4(M,0,0,0);
    if (iMouse.z>0.0f)  Q = _mix(Q,to_float4(0.4f*_cosf(0.1f*iTime),0.4f*_sinf(0.1f*iTime),Q.z,-1),smoothstep(6.0f,4.0f,length(U-swi2(iMouse,x,y))*MouseSize)*MouseSize);
    
    if(Tex)
    {
      float4 tex = texture(iChannel1, U/R);
      if(tex.w>AlphaLevel)
        Q = to_float4(0,0,Q.z,1);      
    }
    else
    {
      if(MapOff == false)
      {
        float d = map(U,R);
        Q = _mix(Q,to_float4(0,0,Q.z,1),smoothstep(6.0f,5.0f,d));
      }
    }

  if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);

    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// gradient step
__KERNEL__ void FluidAutomataFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{

    U+=0.5f;

    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    Q = A(U);
    swi2S(Q,x,y, swi2(Q,x,y) - 0.25f*to_float2(e.z-w.z,n.z-s.z));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// blur pass
__KERNEL__ void FluidAutomataFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    U+=0.5f;
float CCCCCCCCCCCCCCCCCC;
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    Q = m;
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1


// advection
__KERNEL__ void FluidAutomataFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    U+=0.5f;
float DDDDDDDDDDDDDDDDDD;
    float4 
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        m = 0.25f*(n+e+s+w);
    Q = A(U);
    Q = 0.25f*(
      _mix(Q,n,_fmaxf(0.0f,-m.y))+
      _mix(Q,e,_fmaxf(0.0f,-m.x))+
      _mix(Q,s,_fmaxf(0.0f,+m.y))+
      _mix(Q,w,_fmaxf(0.0f,+m.x))
    );

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Tex' to iChannel1

__KERNEL__ void FluidAutomataFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Tex, 0);
    CONNECT_CHECKBOX2(MapOff, 0);
    CONNECT_SLIDER0(AlphaLevel, 0.0f, 1.0f, 0.0f);
    
    CONNECT_SLIDER5(MixAlpha, 0.0f, 1.0f, 0.0f);
    
    
    
    U+=0.5f;
float IIIIIIIIIIIIIIII;
    float4 a = A(U);
    Q = abs_f4(sin_f4(0.3f+a.w+a));
    if(Tex)
    {
      float4 tex = texture(iChannel1, U/R);
      if(tex.w>AlphaLevel)
        Q = tex;
    }
    else
    {
      if(MapOff == false)
      {
        float d = map(U,R);
        Q = _mix(Q,to_float4_s(0),smoothstep(6.0f,5.0f,d));
      }
    }

  Q.w = _mix(Q.w,1.0f,MixAlpha);

  SetFragmentShaderComputedColor(Q);
}