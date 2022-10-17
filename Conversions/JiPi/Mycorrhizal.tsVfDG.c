
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define o to_float3(1,0,-1)
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out float4 Q, float2 U)
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
  return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,0.9f));
}
#define norm(u) ((u)/(1e-9f+length(u)))



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
          Q = _mix(Q,(to_float4_f2f2(0.51f*R,0.49f*R)+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          Q = _mix(Q, (to_float4_f2f2(to_float2(0.49f,0.51f)*R,to_float2(0.51f,0.49f)*R)+MulOff.y)*MulOff.x, Blend);  
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,(Q+MulOff.y)*MulOff.x,Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix(swi2(Q,x,y),  Par, Blend));
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
// Connect Buffer A 'Previsualization: Buffer C' to iChannel3
// Connect Buffer A 'Texture: Blending' to iChannel2

__DEVICE__ void X (inout float4 *Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
  float4 n = A(U+r);
  if (ln(U,swi2(n,x,y),swi2(n,z,w))<ln(U,swi2(*Q,x,y),swi2(*Q,z,w))) *Q = n;
}


__KERNEL__ void MycorrhizalFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
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
    for (int x = -2;x <=2; x++)
    for (int y = -2;y <=2; y++)
      X(&Q,U,to_float2(x,y),R,iChannel0);
    swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(A(swi2(Q,x,y)),x,y),1.0f));
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(A(swi2(Q,z,w)),z,w),0.01f));
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(D(swi2(Q,x,y)),x,y));
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(D(swi2(Q,z,w)),x,y));
    
    if (length(swi2(Q,x,y)-swi2(Q,z,w)) > 2.5f) {
        float2 m = 0.5f*(swi2(Q,x,y)+swi2(Q,z,w));
        if (length(U-swi2(Q,x,y)) > length(U-swi2(Q,z,w))) 
          Q.x=m.x,Q.y=m.y;//swi2(Q,x,y) = m;
        else 
          Q.z=m.x,Q.w=m.y;//swi2(Q,z,w) = m;
    }
    
    
    if (Blend1>0.0) Q = Blending(iChannel2, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, U, R);
    
    
    if (iFrame<1 || Reset) {
        Q = to_float4_f2f2(0.51f*R,0.49f*R);
        float4 a = to_float4_f2f2(to_float2(0.49f,0.51f)*R,to_float2(0.51f,0.49f)*R);
        if (ln(U,swi2(a,x,y),swi2(a,z,w))<ln(U,swi2(Q,x,y),swi2(Q,z,w)))
            Q = a;
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//Mouse
__KERNEL__ void MycorrhizalFuse__Buffer_B(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    U+=0.5f;

    float4 p = texture(iChannel0,U/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
    else C =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else C = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel3


__DEVICE__ float4 T(float2 U, float2 R, __TEXTURE2D__ iChannel3) {
  U -= 0.5f*swi2(D(U),x,y);
  U -= 0.5f*swi2(D(U),x,y);
  return D(U);
}

__KERNEL__ void MycorrhizalFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    U+=0.5f;
  
    Q = T(U,R,iChannel3);
    float4 
        n = T(U+swi2(o,y,x),R,iChannel3),
        e = T(U+swi2(o,x,y),R,iChannel3),
        s = T(U+swi2(o,y,z),R,iChannel3),
        w = T(U+swi2(o,z,y),R,iChannel3),
        m = 0.25f*(n+e+s+w);
    swi2S(Q,x,y, swi2(m,x,y)-0.25f*to_float2(e.z-w.z,n.z-s.z));
    Q.z = Q.z-0.25f*(n.y+e.x-s.y-w.x);
    float4 a = A(U);
    float l = ln(U,swi2(a,x,y),swi2(a,z,w));
    float v = _expf(-l);
    Q.z -= 0.001f*v;
    swi2S(Q,x,y, _mix(swi2(Q,x,y),-norm(swi2(a,x,y)-swi2(a,z,w)),0.1f*v*_expf(-Q.z*Q.z)));
    Q.x*=0.5f;Q.y*=0.5f; //swi2(Q,x,y) *= 0.5f;
    if (iMouse.z>0.0f) swi2S(Q,x,y, swi2(Q,x,y) + _expf(-0.1f*length(U-swi2(iMouse,x,y)))*(U-swi2(iMouse,x,y)));
    
    
    //if (Blend1>0.0) Q = Blending(iChannel1, U/R, Q, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus);
    
    
    if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.z *= 0.0f;
    if (iFrame < 1 || Reset) Q = to_float4(0,0,0,0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel3


// Fork of "Line Tracking Fluid" by wyatt. https://shadertoy.com/view/tsKXzd
// 2020-12-10 19:40:02

__KERNEL__ void MycorrhizalFuse(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;

    float4 a = A(U);
    float4 d = D(U);
    float l = ln(U,swi2(a,x,y),swi2(a,z,w));
    Q = _fmaxf(cos_f4(d.z*to_float4(1,2,3,4)),0.1f)*(_expf(-l));


  SetFragmentShaderComputedColor(Q);
}