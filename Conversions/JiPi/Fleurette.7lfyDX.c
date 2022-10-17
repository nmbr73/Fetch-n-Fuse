
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)



__DEVICE__ float2 hash23(float3 p3)
{  // Dave H
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}


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

__KERNEL__ void FleuretteFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    U+=0.5f;
    float2 uv = U/R;
    
    Q = to_float4_s(0);
    U = 4.0f*(U-0.5f*R)/R.y;
    for (float i = 0.0f; i < 8.0f; i++) {
        U = to_float2(U.x*U.x-U.y*U.y,2.0f*U.x*U.y)-to_float2(-0.6f,_sinf(0.05f*iTime));
        U /= 0.5f+0.3f*dot(U,U);
        Q += 0.1f*length(U)*to_float4_aw(0.5f+sin_f3(2.0f*U.x+to_float3(1,2,3)),1);
    }

  if (Blend1>0.0) Q = Blending(iChannel0, uv, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);

  if(iFrame<1 || Reset) Q = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void FleuretteFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER5(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT3(Par2, 0.0f, 0.0f);

    U+=0.5f;
    float2 uv = U/R;
    
    Q = 0.99f*B(U);
    for (float i = 0.0f; i < 30.0f; i+=1.0f) {
        float2 h = 20.0f*(hash23(to_float3_aw(U+R*i,iFrame))*2.0f-1.0f);
        float4 c = A(U+h),
             n = A(U+h+to_float2(0,1)),
             e = A(U+h+to_float2(1,0)),
             s = A(U+h-to_float2(0,1)),
             w = A(U+h-to_float2(1,0));

        float2 g = 2.0f*R.x*to_float2(e.w-w.w,n.w-s.w);

        Q += _expf(-length(h-g))*c;
   }
   
   if (Blend2>0.0) Q = Blending(iChannel2, uv, Q, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, U, R);
   
   if(iFrame<1 || Reset) Q = to_float4_s(0.0f);
   
  SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void FleuretteFuse(float4 Q, float2 U, float2 iResolution)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 1.0f);
   
    U+=0.5f;
    Q = 0.05f*B(U);

    if (Invers) Q = to_float4_s(1.0f) - Q;
    if (ApplyColor)
    {
      Q = (Q + (Color-0.5f))*Q.w;
      if (Q.x <= AlphaThres)      Q.w = Color.w;  
    }

  SetFragmentShaderComputedColor(Q);
}