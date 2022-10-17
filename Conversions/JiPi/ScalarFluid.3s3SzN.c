
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define A(U) texture(iChannel0,(U-swi2(C(U),x,y))/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define M 0.125f*(A(U+to_float2(0,1))+A(U-to_float2(0,1))+A(U+to_float2(1,0))+A(U-to_float2(1,0))   +   A(U+to_float2(1,1))+A(U+to_float2(1,-1))+A(U-to_float2(1,1))+A(U-to_float2(1,-1)))
#define F 0.5f*(swi2(m,x,y)-swi2(Q,x,y))


#define P(a,b) to_float2(length(swi2(a,x,y))-length(swi2(b,x,y)),_atan2f(a.x*b.y-a.y*b.x,dot(swi2(a,x,y),swi2(b,x,y))))
#define q 0.0f


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 a)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          swi2S(Q,x,y, _mix(swi2(Q,x,y),(to_float2(_sinf(a.x),_cosf(a.x))+MulOff.y)*MulOff.x,Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4_f2f2((to_float2(_sinf(a.x),_cosf(a.x))+MulOff.y)*MulOff.x,Par),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  Par, Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
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
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void ScalarFluidFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER4(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER5(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT1(Par2, 0.0f, 0.0f);
    
    
    U+=0.5f;

    Q = A(U);
    float4 m = M;
    Q.x -= (F).y+q*(Q.x-m.x);
      
    if (iFrame < 1 || Reset)    Q.x=1.0f, Q.y=0.0f;//swi2(Q,x,y) = to_float2(1,0);
    float2 a = mul_f2_mat2(0.2f*swi2(U,x,y), to_mat2(_cosf(iTime),-_sinf(iTime),_sinf(iTime),_cosf(iTime)));
    if (((iFrame < 1 || Reset) &&length(U-to_float2(0.8f,0.5f)*R)<20.0f)||iMouse.z>0.&&length(U-swi2(iMouse,x,y))<10.0f) Q.x=_sinf(a.x) , Q.y=_cosf(a.x);//swi2(Q,x,y) = to_float2(_sinf(a.x),_cosf(a.x));


    if (Blend2>0.0) Q = Blending(iChannel3, U/R, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, a);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void ScalarFluidFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER1(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    Q = A(U);
    float4 m = M;
    Q.y += (F).x+q*(Q.y-m.y);
    
    float o = -B(U).x;
    swi2S(Q,x,y, mul_f2_mat2(swi2(Q,x,y) , to_mat2(_cosf(o),-_sinf(o),_sinf(o),_cosf(o))));
    
    if (length(swi2(Q,x,y))<1e-6) Q.x=1.0f, Q.y=0.0f;//swi2(Q,x,y) = to_float2(1,0);
    swi2S(Q,x,y, _mix(swi2(Q,x,y),normalize(swi2(Q,x,y)),3e-3));
    
    if (iFrame < 1 || Reset)               Q.x=1.0f, Q.y=0.0f;//swi2(Q,x,y) = to_float2(1,0);
    float2 a = mul_f2_mat2(0.2f*swi2(U,x,y), to_mat2(_cosf(iTime),-_sinf(iTime),_sinf(iTime),_cosf(iTime)));
    if (((iFrame < 1 || Reset) && length(U-to_float2(0.8f,0.5f)*R)<20.0f)||iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) Q.x=_sinf(a.x) , Q.y=_cosf(a.x);//swi2(Q,x,y) = to_float2(_sinf(a.x),_cosf(a.x));

    
    if (Blend1>0.0) Q = Blending(iChannel3, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, a);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void ScalarFluidFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
    float4 
        n = B(U+to_float2(0,1)),
        e = B(U+to_float2(1,0)),
        s = B(U-to_float2(0,1)),
        w = B(U-to_float2(1,0));
    float2 a = P(e,w);
    float2 b = P(n,s);
    float2 g = 0.25f*to_float2(
           a.y,
           b.y
           );
    Q = to_float4(-g.x,-g.y,0,1);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void ScalarFluidFuse__Buffer_D(float4 Q, float2 U, int iFrame, float2 iResolution)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    float4 
        n = C(U+to_float2(0,1)),
        e = C(U+to_float2(1,0)),
        s = C(U-to_float2(0,1)),
        w = C(U-to_float2(1,0)),
        n1 = B(U+to_float2(0,1)),
        e1 = B(U+to_float2(1,0)),
        s1 = B(U-to_float2(0,1)),
        w1 = B(U-to_float2(1,0));
    Q = 0.25f*(n1+e1+s1+w1);
    Q -= to_float4_s(0.25f)*(n.y-s.y+e.x-w.x);
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void ScalarFluidFuse(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);  
    U+=0.5f;
    Q = A(U);
    if(Invers)
      Q = to_float4_s(1.0f)-abs_f4(sin_f4(0.3f*Q+dot(Q,Q)*to_float4(1,2,3,4)));
    else
      Q = abs_f4(sin_f4(0.3f*Q+dot(Q,Q)*to_float4(1,2,3,4)));


    Q.w=Alpha;

  SetFragmentShaderComputedColor(Q);
}