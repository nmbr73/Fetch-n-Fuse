
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float ln (float3 p, float3 a, float3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
__DEVICE__ float hash (float2 p) // Dave H
{
    float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
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
          swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
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
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void WatercolorPercolationFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = A(U+to_float2(0,1));
    Q = A(U+to_float2(0,0.05f*length(swi3(Q,x,y,z))*Q.w));
    float4 q = to_float4_s(0);
    for (int x = -1; x<= 1; x++)
    for (int y = -1; y<= 1; y++)
    if (x!=0||y!=0)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float h = hash(U+0.5f*u);
        float m = (length(swi3(a,x,y,z)));
        m = _fminf(m,1.0f);
        float4 w = to_float4_aw(atan_f3(Q.w*swi3(a,w,w,w),to_float3_s(1.0f)),a.w);
        q += m*_powf(h,6.0f)*(a-Q)/dot(u,u);
    }
    Q += 0.125f*q;
    
    float4 d = D(U);
    if (iMouse.z>0.0f && ln(U,swi2(d,x,y),swi2(d,z,w))<0.025f*R.y){
        Q = 0.5f+0.5f*sin_f4(iTime+to_float4(1,2,3,4));
        Q.w = 1.0f;
    }
    //if (iFrame < 1) Q = 1.0f-B(U);
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0.0f);
    
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void WatercolorPercolationFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;

    Q = B(U);
    Q += 5e-4*A(U);
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void WatercolorPercolationFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    Q = A(U)*(1.0f-5e-4);
    
    if (iFrame < 1 || Reset) Q = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


//Mouse
__KERNEL__ void WatercolorPercolationFuse__Buffer_D(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

   float4 p = texture(iChannel0,U/iResolution);
   if (iMouse.z>0.0f) {
      if (p.z>0.0f) C = to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
   else C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
   }
   else C = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1

__KERNEL__ void WatercolorPercolationFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Invers, 0);
    
    U+=0.5f;
    float4 a = A(U), b = B(U);
    Q = a+b;
    float n = hash(U+to_float2(0,1));
    float e = hash(U+to_float2(1,0));
    float s = hash(U-to_float2(0,1));
    float w = hash(U-to_float2(1,0));
    float3 no = normalize(to_float3(e-s,n-s,1));
    
    if(Invers)
      Q = sqrt_f4(Q);//-to_float4_s(0.9f+0.05f*no.y);
    else
      Q = to_float4_s(0.9f+0.05f*no.y)-sqrt_f4(Q);
    

  SetFragmentShaderComputedColor(Q);    
}