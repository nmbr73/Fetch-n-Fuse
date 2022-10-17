
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

//#define Main void mainImage( out float4 Q, in float2 U )
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w); 
#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
#define div 0.25f*(n.y-s.y+e.x-w.x)

#define N 6.0f
#define For for (float i = -(N); i<=(N); i++)
#define S to_float4(3.5f,1,4,4)
#define Gaussian(i) 0.3989422804f/S*exp_f4(-0.5f*(i)*(i)/S/S)
#define Init if (iFrame < 1) 
#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.0f)
#define Mouse if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<30.0f) 
  

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float iTime, float2 U)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          swi2S(Q,x,y, _mix(swi2(Q,x,y),U,Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //Q.w = _mix(Q.w,(tex.x+MulOff.y)*MulOff.x,Blend);
          //Q = _mix(Q,0.5f+0.5f*sin_f4(to_float4(1,2,3,4)+iTime),Blend);

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix( swi2(Q,x,y), to_float2_s(0.2f), Blend));

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


__DEVICE__ void X (inout float4 *Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float4 n = A(U+r);
    if (length(U-swi2(n,x,y))<length(U-swi2(*Q,x,y))) *Q = n;
}

__KERNEL__ void SmoothParticleHydrodynamicsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, Clear, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);


    U+=0.5f;
    Q = A(U);
    X(&Q,U,to_float2(1,0),R,iChannel0);
    X(&Q,U,to_float2(0,1),R,iChannel0);
    X(&Q,U,to_float2(-1,0),R,iChannel0);
    X(&Q,U,to_float2(0,-1),R,iChannel0);
    X(&Q,U,to_float2(3,0),R,iChannel0);
    X(&Q,U,to_float2(0,3),R,iChannel0);
    X(&Q,U,to_float2(-3,0),R,iChannel0);
    X(&Q,U,to_float2(0,-3),R,iChannel0);
    
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(C(swi2(Q,x,y)),z,w),0.01f));
    Q.w -= 5e-4;
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(B(swi2(Q,x,y)),x,y));
    //swi2(Q,x,y) += swi2(Q,z,w);
    Q.x += Q.z;
    Q.y += Q.w;
    
    if (Q.x<1.0f)     {Q.x = 1.0f; Q.z *= -1.0f;}
    if (Q.y<1.0f)     {Q.y = 1.0f; Q.w *= -1.0f;}
    if (R.x-Q.x<1.0f) {Q.x = R.x-1.0f; Q.z *= -1.0f;}
    if (R.y-Q.y<1.0f) {Q.y = R.y-1.0f; Q.w *= -1.0f;}
    
    if (iMouse.z>0.0f) {
      if (length(U-swi2(iMouse,x,y))<length(U-swi2(Q,x,y)))
            //swi2(Q,x,y) = swi2(iMouse,x,y);
          Q.x = iMouse.x,
          Q.y = iMouse.y;
    }
    
    
    if (Blend1>0.0) Q = Blending(iChannel4, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, iTime, U);        
    
    if (iFrame < 1 && length (U-0.5f*R) < 30.0f) Q.x=U.x,Q.y=U.y;//swi2(Q,x,y) = U;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SmoothParticleHydrodynamicsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    Q = to_float4_s(0);
    For {
        float4 a = A(U+to_float2(i,i));
        float p = _expf(-length(U+to_float2(i,i)-swi2(a,x,y)));
        Q += Gaussian(i) * to_float4(p,p,a.z,a.w);
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void SmoothParticleHydrodynamicsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    Q = to_float4_s(0);
    For Q += Gaussian(i) * A(U+to_float2(-i,i));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void SmoothParticleHydrodynamicsFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    Neighborhood
    Q = A(U);
    swi2S(Q,x,y, (1.5f*(Q.x+0.01f)*to_float2(e.x-w.x,n.x-s.x)-0.7f*to_float2(e.y-w.y,n.y-s.y)));
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel0


__DEVICE__ float c (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
   float4 a = A(U);
   return 0.01f*a.x+smoothstep(0.1f,1.0f,10.0f*a.x + a.y);
}


__KERNEL__ void SmoothParticleHydrodynamicsFuse(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    float4 a = A(U), b=C(U);
    float 
       q = c(U,R,iChannel0),
       n = c(U+to_float2(0,1),R,iChannel0),
       e = c(U+to_float2(1,0),R,iChannel0),
       s = c(U-to_float2(0,1),R,iChannel0),
       w = c(U-to_float2(1,0),R,iChannel0);
    float3 no = normalize(to_float3(e-w,n-s,-0.001f));
    no = reflect(no,to_float3(0,0,-1));
    Q = abs_f4(sin_f4((2.0f*a.x+0.1f)*to_float4(1,2,3,4)+length(swi2(a,z,w))));
    Q *= q*(1.0f+decube_f3(iChannel1,no))*0.5f;
    Q *= 1.0f-0.1f*smoothstep(2.0f,0.0f,length(U-swi2(b,x,y)));
    
  SetFragmentShaderComputedColor(Q);    
}