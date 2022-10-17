
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

#define Main void mainImage( out float4 Q, in float2 U )
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w); 
#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
#define grad 0.25f*to_float2(e.x-w.x,n.x-s.x)
#define div 0.25f*(n.y-s.y+e.x-w.x)

#define N 21.0f
#define For for (float i = -(N-5.0f); i<=(N); i+=1.0f)
#define S 4.0f
#define Gaussian(i) 0.3989422804f/S*_expf(-0.5f*(i)*(i)/S/S)
#define Init if (iFrame < 1 ||Reset) 
#define Border if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)
#define Mouse if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<30.0f) 
  

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
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void EvolveFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    
    U -= 0.5f*R;
    U = mul_f2_mat2(U*(1.0f-0.03f+0.015f*_expf(-0.5f*length(U)/R.y)),rot(0.005f*_expf(-length(U)/R.y)));
    
    U += 0.5f*R;
    U += 0.2f*swi2(A(U),y,x)*to_float2(1,-1);
    U += 0.2f*swi2(A(U),x,y);
    Neighborhood   
    Q = A(U);
    float4 c = C(U), b = B(U);
    
    Q.z  = c.z*0.4f-S*div;
    swi2S(Q,x,y, swi2(Q,x,y)*0.99f+swi2(c,x,y)*0.5f);
    
    if (length(swi2(b,x,y))>0.0f)   swi2S(Q,x,y, _mix(swi2(Q,x,y),0.5f*(2.0f-Q.z)*normalize(swi2(b,x,y)),0.1f));
    
    if (Blend1>0.0) Q = Blending(iChannel3, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    Mouse Q *= 0.0f;
    Init  Q = sin_f4(0.01f*swi4(U,x,y,x,x));
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void EvolveFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    Q = to_float4_s(0);
    For Q += Gaussian(i) * A(U+to_float2(i,i)).z;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void EvolveFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    Q = to_float4_s(0);
    For Q += Gaussian(i) * A(U+to_float2(-i,i));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void EvolveFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);  
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    Neighborhood
    //swi2(Q,x,y) = grad;
    Q.x = (grad).x;
    Q.y = (grad).y;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void EvolveFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 0.1f);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
   
    U+=0.5f;
    float4 a = A(U);
    Neighborhood
    float3 no = normalize(to_float3_aw(grad,1)),
           re = reflect(no,to_float3_aw(0.03f*(U-0.5f*R)/R.y,1));
    Q = swi4(a,z,z,z,z)*0.5f+0.5f+0.04f*swi4(a,x,y,y,w);
    Q *= 0.5f+0.5f*decube_f3(iChannel1,re);
    Q *= 1.0f+0.5f*swi4(no,x,y,z,z)*no.z;
    Q *= 3.0f*Q;
    
    if (Invers) Q = to_float4_s(1.0f) - Q;
    if (ApplyColor)
    {
      if (Q.x <= AlphaThres)      Q.w = Color.w;  
      Q = (Q + (Color-0.5f))*Q.w;
    }

    
    
    
  SetFragmentShaderComputedColor(Q);    
}