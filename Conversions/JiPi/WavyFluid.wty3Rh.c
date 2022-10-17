
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define swi4S(a,b,c,d,e,f) {float4 tmp = f; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z; (a).e = tmp.w;}

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)



#define F(x) (_fabs((x)-0.5f)+U.y*1e-3)

#define dt   0.75f
#define K    0.75f
#define Loss 0.01f

#define Border if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q *= 0.0f;
#define Init if (iFrame < 1 || Reset) Q = _expf(-0.04f*length(U-0.5f*R))
#define Mouse(V) if (iMouse.z>0.0f)   swi4S(Q,x,y,z,w, swi4(Q,x,y,z,w) + 0.5f*_expf(-0.001f*dot(U-swi2(iMouse,x,y),U-swi2(iMouse,x,y))) * V)
  

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
          Q = _mix(Q,to_float4(Par.x*tex.x,Par.y*tex.y,MulOff.x*tex.z,MulOff.y*tex.w),Blend);
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
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2

__KERNEL__ void WavyFluidFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 
         N = A(U+to_float2(0,1)), 
         E = A(U+to_float2(1,0)), 
         S = A(U-to_float2(0,1)), 
         W = A(U-to_float2(1,0)),
         M = 0.25f*(N+E+S+W),
         a = A(U), b = B(U);
    Q = a + dt*b + Loss*(M-a);
    
    Mouse(to_float4(1,0,-1,0));
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    Init*to_float4(1,0,-1,0);
    Border
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void WavyFluidFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 
         M = 0.125f*(
           A(U+to_float2(0,1))+A(U+to_float2(1,0))+
           A(U-to_float2(0,1))+A(U-to_float2(1,0))+
           A(U+to_float2(1,1))+A(U+to_float2(1,-1))+
           A(U-to_float2(1,1))+A(U-to_float2(1,-1))
           ),
         a = A(U);
    float P = F(length(a));
    Q = B(U) + dt*(M-a-K*a*P);
    
    Mouse(to_float4(0,K,0,-K));
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    Init*to_float4(0,K,0,-K);
    Border
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void WavyFluidFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 
         N = A(U+to_float2(0,1)), 
         E = A(U+to_float2(1,0)), 
         S = A(U-to_float2(0,1)), 
         W = A(U-to_float2(1,0)),
         M = 0.25f*(N+E+S+W),
         a = A(U), b = B(U);
    Q = a + dt*b + Loss*(M-a);
    
    Mouse(to_float4(1,0,-1,0));
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    Init*to_float4(1,0,-1,0);
    Border
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void WavyFluidFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 
         M = 0.125f*(
           A(U+to_float2(0,1))+A(U+to_float2(1,0))+
           A(U-to_float2(0,1))+A(U-to_float2(1,0))+
           A(U+to_float2(1,1))+A(U+to_float2(1,-1))+
           A(U-to_float2(1,1))+A(U-to_float2(1,-1))
         ),
         a = A(U);
    float P = F(length(a));
    Q = B(U) + dt*(M-a-K*a*P);
    
    Mouse(to_float4(0,K,0,-K));
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    Init*to_float4(0,K,0,-K);
    Border
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel3


__KERNEL__ void WavyFluidFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    
    CONNECT_CHECKBOX1(ChangeColor, 0);
    CONNECT_COLOR0(Color, 1.0f, 2.0f, 3.0f, 1.0f); 
    
    U+=0.5f;
    
    float4 a = A(U);
    float4 b = B(U);
    float4 c = C(U);
    float4 d = D(U);
    float e = dot(a,a)+dot(b,b);
    
    if(ChangeColor)
      Q = sin_f4(a*a+e*Color), Q.w=Color.w;
    else
      Q = sin_f4(a*a+e*to_float4(1,2,3,4));
    
    
  SetFragmentShaderComputedColor(Q);    
}