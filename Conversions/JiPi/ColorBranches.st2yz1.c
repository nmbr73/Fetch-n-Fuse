
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

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
          Q = _mix(Q,to_float4_f2f2(ceil_f2(U/Par.x)*Par.x,ceil_f2(U/Par.y)*Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
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
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


__DEVICE__ void X (inout float4 *Q, float2 U, float4 q) {
    if (length(U-swi2(q,x,y))<length(U-swi2(*Q,x,y))) *Q=q;
}

__KERNEL__ void ColorBranchesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
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
    X(&Q,U,A(U+to_float2(0,1)));
    X(&Q,U,A(U+to_float2(1,0)));
    X(&Q,U,A(U-to_float2(0,1)));
    X(&Q,U,A(U-to_float2(1,0)));
    float2 v = _mix(swi2(Q,x,y),U,0.000001f);
    float4 n = B(v+to_float2(0,1));
    float4 e = B(v+to_float2(1,0));
    float4 s = B(v-to_float2(0,1));
    float4 w = B(v-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y) - 0.5f*to_float2(e.w-w.w,n.w-s.w));
    if (iFrame < 1 || Reset) {
        U = ceil_f2(U/200.0f)*200.0f;
        Q = to_float4_f2f2(U,U);
    }
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void ColorBranchesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER3(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER5(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT1(Par2, 0.0f, 0.0f);

    U+=0.5f;
    
    float4 a = A(U);
    float4 b = B(U);
    float4 m = to_float4_s(0);
    for (float x = -2.0f; x <= 2.0f; x++)
    for (float y = -2.0f; y <= 2.0f; y++)
    {
        m += 1.0f/25.0f*B(U+to_float2(x,y));
    }
    Q.x = smoothstep(2.0f,1.0f,length(U-swi2(a,x,y)));
    Q.y = _fmaxf(Q.x,0.93f*b.y);
    Q.z = 0.0f;
    Q.w = m.w*0.99f+Q.y;
    
    if (Blend2>0.0) Q = Blending(iChannel2, U/R, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, R);
    
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q *= 0.0f;
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0.0f); 
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2

__KERNEL__ void ColorBranchesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER6(Blend3, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER7(Blend3Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER8(Blend3Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON2(Modus3, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par3, 0.0f, 0.0f);

    U+=0.5f;
    
    float4 b = B(U);
    float4 a = A(U);
    float4 c = C(U);
    Q = b.x*(0.5f+0.5f*sin_f4(a.z+a.w*to_float4(1,2,3,4)));
    Q = _fmaxf(c*0.99f,Q);
    
    if (Blend3>0.0) Q = Blending(iChannel3, U/R, Q, Blend3, Par3, to_float2(Blend3Mul,Blend3Off), Modus3, U, R);
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void ColorBranchesFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  

    U+=0.5f;
    Q = C(U);
    
  SetFragmentShaderComputedColor(Q);    
}