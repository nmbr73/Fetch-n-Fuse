
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3
// Connect Buffer A 'Texture: Blending' to iChannel1

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
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void TanninsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
  
    U+=0.5f;
    
    Q = to_float4_s(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        #define q 1.1f
        float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*q,U - 0.5f,U + 0.5f),
               w2 = clamp(U+u+swi2(a,x,y)+0.5f*q,U - 0.5f,U + 0.5f);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
        Q.w += m*a.w;
    }
    if (Q.w>0.0f)
      //Q.xyz/=Q.w;
      Q.x/=Q.w, Q.y/=Q.w, Q.z/=Q.w;
      
    swi2S(Q,x,y, clamp(swi2(Q,x,y),to_float2_s(-1),to_float2_s(1)));
    if (iFrame < 1 || Reset) 
    {
        Q = to_float4(0,0,0.1f,0);
    }
    float4 d = D(U);
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    if ((iFrame < 10 || iMouse.z>0.0f)&&ln(U,swi2(d,x,y),swi2(d,z,w))<2.0f)
        Q = to_float4_f2f2(clamp(1e-2*(swi2(d,x,y)-swi2(d,z,w)),0.5f,0.5f), to_float2(0.5f,0.5f));
    if (U.x<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3
// Connect Buffer B 'Texture: Blending' to iChannel2


__KERNEL__ void TanninsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    U+=0.5f;
    
    Q = to_float4_s(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u), b = B(U+u);
        #define q 1.1f
        float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*q,U - 0.5f,U + 0.5f),
               w2 = clamp(U+u+swi2(a,x,y)+0.5f*q,U - 0.5f,U + 0.5f);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(b,x,y,z));
        Q.w += m*a.w;
    }
    if (Q.w>0.0f)
      //Q.xyz/=Q.w;
      Q.x/=Q.w, Q.y/=Q.w, Q.z/=Q.w;
    
    if (iFrame < 1 || Reset) 
    {
        Q = to_float4(0,0,0,0);
    }
    float4 d = D(U);
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    if ((iFrame < 10||iMouse.z>0.0f)&&ln(U,swi2(d,x,y),swi2(d,z,w))<2.0f)
        Q = to_float4_aw(1.0f+0.5f*sin_f3(iTime+iTime*to_float3(1,2,3)),0.5f);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f; //swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void TanninsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
float BBBBBBBBBBBBBBBBBBB;    
    Q = A(U);float4 _q = Q;
    for (int x = -1; x<=1; x++)
    for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        u = (u)/dot(u,u);
        swi2S(Q,x,y, swi2(Q,x,y) - _q.w*0.125f*a.w*(0.6f*a.w*a.z+a.w-(1.0f-0.6f*a.w))*u);
        Q.z -= _q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(_q,x,y)));
    }
    if (Q.w < 1e-3) Q.z *= 0.0f;
    Q.y -= 1e-2*Q.w;
    swi2S(Q,x,y, swi2(Q,x,y) * (0.5f+0.5f*_powf(Q.w,0.1f)));  //Klammern
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


// keep track of mouse
__KERNEL__ void TanninsFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;

    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);
    if (iFrame < 10 || Reset) fragColor = to_float4(0.4f,0.5f+0.1f*_sinf((float)(iFrame)),0.6f,0.5f)*swi4(R,x,y,x,y);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// Connect Image 'Texture: Blending' to iChannel2
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void TanninsFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
  
    U+=0.5f;
    
    float4
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    float3 norm = 
        normalize(to_float3(e.w-w.w,n.w-s.w,3)),
        ref = reflect(to_float3(0,0,-1),norm);
   
    float4 b = B(U);
    Q = b*b.w;
    float4 t = decube_f3(iChannel3,ref);
    Q *= 0.8f+30.0f*t*t*t*t;
    
  SetFragmentShaderComputedColor(Q);
}