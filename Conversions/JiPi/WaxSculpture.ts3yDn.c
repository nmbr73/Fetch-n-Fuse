
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out float4 Q, in float2 U)
__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float ln (float3 p, float3 a, float3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}

#define rate 0.005f



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
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
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
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3
// Connect Buffer A 'Texture: Blending' to iChannel1


__KERNEL__ void WaxSculptureFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
    
    U+=0.5f;

    if (iFrame%2==0) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            #define o 1.3
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*o,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*o,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(o*o);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
            //Q.xyz/=Q.w;
            Q.x/=Q.w,
            Q.y/=Q.w,
            Q.z/=Q.w;
                
        if (iFrame < 1 || Reset) 
        {
            Q = to_float4(0,0,0.1f,0);
        }
   } else {
      Q = A(U);float4 q = Q;
        for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++)
        if (x != 0||y!=0)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u), c = C(U+u);
            u = (u)/dot(u,u);
            swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*0.5f*(c.w+a.w*(a.w*a.z-2.5f))*u);
            Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
        }
    }
    
    swi2S(Q,x,y, swi2(Q,x,y) * (0.2f+0.77f*_fminf(_powf(Q.w,0.01f),1.0f)));
    // Solidify
    Q.w -= Q.w*rate/(1.0f+10.0f*length(swi2(Q,x,y)));
    float4 d = D(U);
    
    if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    if ((iMouse.z>0.0f)&&ln(U,swi2(d,x,y),swi2(d,z,w))<8.0f)
        Q = to_float4(1e-1*clamp((d.x)-(d.z),-0.8f,0.8f),1e-1*clamp((d.y)-(d.w),-0.8f,0.8f),1,1);
    if (U.y<1.0f||U.x<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3
// Connect Buffer B 'Texture: Blending' to iChannel2

__KERNEL__ void WaxSculptureFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
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
      Q.x/=Q.w,
      Q.y/=Q.w,
      Q.z/=Q.w;
    
    if (iFrame < 1)  //?????????????
    {
        Q = to_float4(0,0,0,0);
    }
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    float4 d = D(U);
    if ((iMouse.z>0.0f)&&ln(U,swi2(d,x,y),swi2(d,z,w))<8.0f)
        Q = to_float4_aw(1.0f+0.5f*sin_f3(iTime*0.1f+iTime*to_float3(1,2,3)),1);

    Q -= Q*rate;
    if (iFrame < 1 || Reset) Q = C(U);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2

__KERNEL__ void WaxSculptureFuse__Buffer_C(float4 Q, float2 U, float2 iResolution)
{

    U+=0.5f;
float CCCCCCCCCCCCCCCCC;
    float4 a = A(U), b = B(U);
    Q = C(U);
    float f = 1.0f/(1.0f+10.0f*length(swi2(Q,x,y)));
    Q.w += 6.0f*rate*a.w*f;
    swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z),5.0f*swi3(b,x,y,z),rate*b.w*f));

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


// keep track of mouse
__KERNEL__ void WaxSculptureFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
        if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// Fork of "Tannins" by wyatt. https://shadertoy.com/view/3sdczN
// 2020-09-21 22:04:57

__KERNEL__ void WaxSculptureFuse(float4 Q, float2 U, float2 iResolution)
{

    U+=0.5f;

    float4
        n = C(U+to_float2(0,1))+B(U+to_float2(0,1)),
        e = C(U+to_float2(1,0))+B(U+to_float2(1,0)),
        s = C(U-to_float2(0,1))+B(U-to_float2(0,1)),
        w = C(U-to_float2(1,0))+B(U-to_float2(1,0));
    float3 norm = 
        normalize(to_float3(e.w-w.w,n.w-s.w,0.1f)),
        ref = reflect(to_float3(0,0,-1),norm);
    float4 b = B(U), c = C(U);
    Q = c+b*b.w;
    float3 l = swi3(R,x,y,y);
    float li = ln(to_float3_aw(U,0),to_float3_aw(U,0)+ref,l);
    
    Q *= 0.2f+0.5f*_expf(-li)+0.5f*_expf(-2.0f*li);
    
    Q = atan_f4(2.0f*Q,to_float4_s(1.0f));
    
  SetFragmentShaderComputedColor(Q);    
}