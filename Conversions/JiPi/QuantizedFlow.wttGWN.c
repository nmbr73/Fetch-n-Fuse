
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)


#define Me Q = A(U);
#define Them float4 M = 0.25f*(A(U+to_float2(0,1))+A(U+to_float2(1,0))+A(U-to_float2(0,1))+A(U-to_float2(1,0)));

#define F (0.75f*(M-Q-0.03f*Q*(dot(swi2(Q,x,y),swi2(Q,x,y))-0.5f*Q.z)))

#define Mouse if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<10.0f)   Q.x=_sinf(U.x+U.y), Q.y=_cosf(U.x+U.y);
//      swi2(Q,x,y) = to_float2(_sinf(U.x+U.y),_cosf(U.x+U.y));


#define First if (iFrame < 1 || Reset) {Q = to_float4(1,0,0,0); if (length(U-Start*R)<30.0f)    Q.x=_sinf(U.x), Q.y=_cosf(U.x);}
//      swi2(Q,x,y) = to_float2(_sinf(U.x),_cosf(U.x));}


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R, float iTime)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),to_float2(_sinf(U.x+U.y),_cosf(U.x+U.y)) ,Blend));
          Q = _mix(Q,to_float4(_sinf(U.x+U.y),_cosf(U.x+U.y), _sinf(iTime), _cosf(iTime)),Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4(_sinf(U.x+U.y),_cosf(U.x+U.y), Par.x, Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          // swi2S(Q,z,w, _mix(swi2(Q,z,w),  Par, Blend));
          Q = _mix( Q,  (tex+MulOff.y)*MulOff.x, Blend);
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


__KERNEL__ void QuantizedFlowFuse__Buffer_A(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    CONNECT_POINT1(Start, 0.2f, 0.5f);

    U+=0.5f;
    
    Me
    Them
    Q.x += (F).y;
    Mouse 
    First
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R, iTime);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void QuantizedFlowFuse__Buffer_B(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    U+=0.5f;
    Me
    
    Them
        
    Q.y -= (F).x;
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


__DEVICE__ float angle (float2 a, float2 b) {
  return _atan2f(a.x*b.y-a.y*b.x,dot(swi2(a,x,y),swi2(b,x,y)));
}

__KERNEL__ void QuantizedFlowFuse__Buffer_C(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_POINT1(Start, 0.2f, 0.5f);    
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    U+=0.5f;
    
    Me
    float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)),w = A(U-to_float2(1,0));
    
    Q.x = angle(swi2(e,x,y),swi2(w,x,y));
    Q.y = angle(swi2(n,x,y),swi2(s,x,y));
    
    swi2S(Q,z,w, swi2(B(U-0.15f*swi2(Q,x,y)),z,w));
    
    //if (iFrame < 1 && length(U-to_float2(0.2f,0.5f)*R)<30.0f)   Q.z=_sinf(0.1f*U.x) , Q.w=_cosf(0.1f*U.x) ;// swi2(Q,z,w) = to_float2(_sinf(0.1f*U.x),_cosf(0.1f*U.x));   
    if (iFrame < 2 && length(U-Start*R)<30.0f)   Q.z=_sinf(0.1f*U.x) , Q.w=_cosf(0.1f*U.x) ;// swi2(Q,z,w) = to_float2(_sinf(0.1f*U.x),_cosf(0.1f*U.x));   
    if (iFrame < 1 || Reset) Q.x = 0.0f, Q.y=0.0f, Q.z =0.0f, Q.w=0.0f;
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)        Q.z=_sinf(iTime) , Q.w=_cosf(iTime) ;// swi2(Q,z,w) = to_float2(_sinf(iTime),_cosf(iTime));   
    
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R, iTime);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void QuantizedFlowFuse__Buffer_D(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    U+=0.5f;
    
    Me
    Them
    float4 n = B(U+to_float2(0,1)), e = B(U+to_float2(1,0)),s = B(U-to_float2(0,1)), w = B(U-to_float2(1,0));
    Q.z = M.z+0.25f*(n.y+e.x-s.y-w.x);
  
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void QuantizedFlowFuse(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    U+=0.5f;
    
    Me
    Q = (to_float4_s(0.5f)+0.5f*swi4(Q,x,x,x,x))*abs_f4(atan_f4(10.0f*to_float4(Q.z,0.5f*(Q.z+Q.w),Q.w,1),to_float4_s(1.0f)));
    Q = sin_f4(Q*to_float4(1,2,3,4));
    
  SetFragmentShaderComputedColor(Q);    
}