
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define swi4S(a,b,c,d,e,f) {float4 tmp = f; (a).b = tmp.x; (a).c = tmp.y; (a).d = tmp.z; (a).e = tmp.w;}

__DEVICE__ float4 F( float4 Q, float2 fragCoord) {
    float l = length(swi2(Q,x,y)),
          g = length(swi2(Q,z,w));
    float4 c = abs_f4(to_float4_s(length(Q))-to_float4(0.55f,0.55f,0.5f,0.5f));
  return to_float4(g,g,l,l)+c+3e-3f*fragCoord.y;
}

#define dt   0.8f
#define K    0.5f
#define Loss 0.01f

#define Init if (Reset || iFrame < 1 || length(U-(0.5f+0.1f*sin_f2(to_float2(1,2)*_floor((float)(iFrame)/4.0f)*0.1f))*R)<10.0f) Q = _expf(-0.005f*length(U-0.5f*R))
#define Mouse(V) if ((iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<30.0f )|| length(U-(0.8f+0.05f*cos_f2(to_float2(1,2)*_floor((float)(iFrame)/4.0f)*0.1f))*R)<10.0f) swi4S(Q,x,y,z,w, swi4(Q,x,y,z,w) + 0.3f*V);

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
          Q = _mix(Q,to_float4(Par.x,Par.y,-1,0),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  Par, Blend));
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


__KERNEL__ void D2FluidFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
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

    Mouse(to_float4(0,0,-1,0));
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    Init*to_float4(1,0,0,0);
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void D2FluidFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
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
         M = 0.25f*(
           A(U+to_float2(0,1))+A(U+to_float2(1,0))+
           A(U-to_float2(0,1))+A(U-to_float2(1,0))
         ),
         a = A(U);
    Q = B(U) + dt*(M-a-K*a*F(a,U));
    
    Mouse(to_float4(0,0,0,0.3f));
    Init*to_float4(0,-0.3f,0,0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void D2FluidFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
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
    
    Mouse(to_float4(0,0,-1,0));
    Init*to_float4(1,0,0,0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void D2FluidFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
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
         M = 0.25f*(
           A(U+to_float2(0,1))+A(U+to_float2(1,0))+
           A(U-to_float2(0,1))+A(U-to_float2(1,0))
         ),
         a = A(U);
    Q = B(U) + dt*(M-a-K*a*F(a,U));
    
    Mouse(to_float4(0,0,0,0.3f));
    Init*to_float4(0,-0.3f,0,0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void D2FluidFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    U+=0.5f;
    float4 a = A(U)+C(U), b = B(U)+D(U), c = a*a+b*b;
    Q = to_float4(length(swi2(c,x,y)),length(swi2(c,z,w)),length(c),1);
    Q = atan_f4(Q,to_float4_s(1.0f));
    Q = (sin_f4(Q.x-Q.y+Q.z*to_float4(1,2,3,4)));
    
  SetFragmentShaderComputedColor(Q);    
}