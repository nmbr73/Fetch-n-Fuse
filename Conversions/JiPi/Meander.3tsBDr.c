
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)


#define r 1.15f
#define N 15.0f
#define S to_float4(4,7,1,1)
#define Gaussian(i) 0.3989422804f/S*exp_f4(-0.5f*(i)*(i)/S/S)

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
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MeanderFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Start, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    if (iFrame%2<1) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*r,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*r,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
          Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w; //Q.xyz/=Q.w;
          
        if (iFrame < 1 || Reset) 
        {
            Q = to_float4(0,0,1,0);
            if (length(U-to_float2_s(0.5f)*R)<0.3f*R.y && Start)  Q.w = 0.3f;
        }
        if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<20.0f)  Q.x=0.25f,Q.w=0.3f;  //swi2(Q,x,w) = to_float2(0.25f,0.3f);
        if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)   Q.x*=0.0f,Q.y*=0.0f;  //swi2(Q,x,y) *= 0.0f;
    } else {
      Q = A(U);float4 q = Q, dd = D(U);
      for (int x = -1; x<=1; x++)
      for (int y = -1; y<=1; y++)
      if (x!=0||y!=0)
      {
          float2 u = to_float2(x,y);
          float4 a = A(U+u), b = B(U+u), d = D(U+u);
          u = (u)/dot(u,u);
          swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*(-d.w*a.w+a.w*(a.w*a.z-1.0f-3.0f*a.w))*u);
          Q.z  -= q.w*0.125f*a.w*dot(u,swi2(a,x,y)-swi2(q,x,y));
      }
      swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(D(U),x,y),Q.w));
      if (Q.w < 1e-3)  Q.z *= 0.0f;
    }
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void MeanderFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    float4 a = A(U);
    Q = _mix(D(U),a,a.w);
    
    float4 m = 0.25f*(D(U+to_float2(0,1))+D(U+to_float2(1,0))+D(U-to_float2(0,1))+D(U-to_float2(1,0)));
    Q = _mix(Q,m,to_float4(0,0,1,0.1f));
    
    if (length(swi2(Q,x,y))>0.0f) 
        swi2S(Q,x,y, 0.2f*normalize(swi2(Q,x,y))*Q.w);
      
    if (iFrame < 1 || Reset) 
    {
      Q = to_float4(0,0,0,0);
    }  
      
  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel3


__KERNEL__ void MeanderFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Start, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    
    if (iFrame%2<1) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*r,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*r,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
            Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w;//Q.xyz/=Q.w;
          
        if (iFrame < 1 || Reset) 
        {
            Q = to_float4(0,0,1,0);
            if (length(U-to_float2_s(0.5f)*R)<0.3f*R.y && Start)        Q.w = 0.3f;
        }
        if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<20.0f)     Q.x=0.25f,Q.w=0.3f;   //swi2(Q,x,w) = to_float2(0.25f,0.3f);
        
        
        if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
        
        
        if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)    Q.x*=0.0f,Q.y*=0.0f;  //swi2(Q,x,y) *= 0.0f;
    } else {
        Q = A(U);float4 q = Q, dd = D(U);
        for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++)
          if (x!=0||y!=0)
          {
              float2 u = to_float2(x,y);
              float4 a = A(U+u), b = B(U+u), d = D(U+u);
              u = (u)/dot(u,u);
              swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*(-d.w*a.w+a.w*(a.w*a.z-1.0f-3.0f*a.w))*u);
              Q.z  -= q.w*0.125f*a.w*dot(u,swi2(a,x,y)-swi2(q,x,y));
          }
        swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(D(U),x,y),Q.w));
        if (Q.w < 1e-3) Q.z *= 0.0f;
   }
   
  SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel3
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void MeanderFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
  
    U+=0.5f;
    
    float4 a = A(U);
    Q = _mix(D(U),a,a.w);
    
    float4 m = 0.25f*(D(U+to_float2(0,1))+D(U+to_float2(1,0))+D(U-to_float2(0,1))+D(U-to_float2(1,0)));
    Q = _mix(Q,m,to_float4(0,0,1,0.1f));
    
    if (length(swi2(Q,x,y))>0.0f) 
        swi2S(Q,x,y, 0.2f*normalize(swi2(Q,x,y))*Q.w);
      
  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Fluid Reaction" by wyatt. https://shadertoy.com/view/3tfBWr
// 2020-08-03 18:33:18

// Fork of "4-Substance" by wyatt. https://shadertoy.com/view/3lffzM
// 2020-08-03 02:14:45

// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

__KERNEL__ void MeanderFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX2(Invers, 0);
    
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
    if(Invers)
      Q = 2.5f*sin_f4(to_float4_s(A(U).w)*((to_float4_s(1.3f)+0.2f*to_float4(1,2,3,4) + (Color-0.5f)) ));
    else
      Q = to_float4_s(1.0f)-2.5f*sin_f4(to_float4_s(A(U).w)*(to_float4_s(1.3f)+0.2f*to_float4(1,2,3,4)));



  SetFragmentShaderComputedColor(Q);
}