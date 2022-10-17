
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)
//#define E(U) _tex2DVecN(iChannel4,(U).x/R.x,(U).y/R.y,15)
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define Neighborhood2 float4 N = B(U+to_float2(0,1)), E = B(U+to_float2(1,0)), S = B(U-to_float2(0,1)), W = B(U-to_float2(1,0)), M = 0.25f*(n+e+s+w);
#define grd 0.25f*to_float2(e.w-w.w,n.w-s.w);
#define div 0.25f*(e.x-w.x+n.y-s.y);


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void LastFluidFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    U+=0.5f;
    Q = B(U-swi2(B(U),x,y));
    Neighborhood;
    swi2S(Q,x,y, swi2(Q,x,y) - grd);   
    if ((iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)||(iFrame<1&&length(U-0.5f*R)<6.0f))
       swi3S(Q,x,y,w, to_float3(0.4f*_sinf(0.2f*iTime),0.4f*_cosf(0.2f*iTime),1));
    
    if(Reset) Q = to_float4_s(0.0f);
    
    SetFragmentShaderComputedColor(Q); 
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void LastFluidFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
   
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    Q = B(U-swi2(A(U),x,y));
    Neighborhood;
    Q.w  -= div;
    Neighborhood2; 
    swi3S(Q,x,y,z, swi3(Q,x,y,z) - 0.25f*swi3((N*n.y-S*s.y+E*e.x-W*w.x),x,y,z));
    if ((iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)||(iFrame<1&&length(U-0.5f*R)<6.0f))
       swi3S(Q,x,y,z, abs_f3(to_float3(_sinf(0.2121f*iTime),_cosf(0.43f*iTime),_sinf(0.333f*iTime))));
    
float BBBBBBBBBBBBBBBBBBBBBBBBBBBB;    
    //Blending
    if (Blend1 > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = C(U);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          Q = _mix(Q,(tex+Blend1Off)*Blend1Mul,Blend1);

        if ((int)Modus&4)
          swi2S(Q,x,y, _mix( swi2(Q,x,y), Par1, Blend1));
        
        if ((int)Modus&8)
          swi2S(Q,z,w, _mix( swi2(Q,z,w), Par1, Blend1));

        if ((int)Modus&16) //Masse
          Q = _mix(Q, to_float4_s(0.0f), Blend1);

        if ((int)Modus&32) //Special
          Q.w = _mix(Q.w, (tex.x+Blend1Off)*Blend1Mul, Blend1);
      }
    }
    
    
    if(Reset) Q = to_float4_s(0.0f);
    
    SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void LastFluidFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    float4 b = B(U);
    Q = abs_f4(sin_f4(b+b.w));
    
    SetFragmentShaderComputedColor(Q);    
}