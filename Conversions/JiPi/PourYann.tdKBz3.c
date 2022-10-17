
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define o to_float3(1,0,-1)
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3,(U).x/R.x,(U).y/R.y,15)

__DEVICE__ float ln (float2 p, float2 a, float2 b) {
  return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,0.9f));
}
#define norm(u) ((u)/(1e-9+length(u)))
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel3


__DEVICE__ void X (inout float4 *Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
    float4 n = A(U+r);
    if (ln(U,swi2(n,x,y),swi2(n,z,w))<ln(U,swi2(*Q,x,y),swi2(*Q,z,w))) *Q = n;
}


__KERNEL__ void PourYannFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 

    U+=0.5f;
    Q = A(U);
    for (int _x = -2;_x <=2; _x++)
    for (int _y = -2;_y <=2; _y++)
      X(&Q,U,to_float2(_x,_y),R,iChannel0);
  
    swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(A(swi2(Q,x,y)),x,y),0.3f));
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(A(swi2(Q,z,w)),z,w),0.05f));
    swi2S(Q,x,y, swi2(Q,x,y) + swi2(D(swi2(Q,x,y)),x,y));
    swi2S(Q,z,w, swi2(Q,z,w) + swi2(D(swi2(Q,z,w)),x,y));
    
    if (length(swi2(Q,x,y)-swi2(Q,z,w)) > 2.5f) {
        float2 m = 0.5f*(swi2(Q,x,y)+swi2(Q,z,w));
        if (length(U-swi2(Q,x,y)) > length(U-swi2(Q,z,w))) 
          Q.x=m.x,Q.y=m.y;//swi2(Q,x,y) = m;
        else 
          Q.z=m.x,Q.w=m.y;//swi2(Q,z,w) = m;
    }
    if (iMouse.z>0.0f) {
        float4 n = B(to_float2_s(0));
      if (ln(U,swi2(n,x,y),swi2(n,z,w))<ln(U,swi2(Q,x,y),swi2(Q,z,w))) Q = n;
    }
    if (iFrame<1 || Reset) {
        Q = to_float4_f2f2(0.7f*R,0.3f*R);
        float4 a = to_float4_f2f2(to_float2(0.3f,0.7f)*R,to_float2(0.7f,0.3f)*R);
        
        if (Textur)
        {
           float4 tex = C(U);
           if (tex.w>0.0f)
           {
             Q = a;
           }
        }
        else  
          if (ln(U,swi2(a,x,y),swi2(a,z,w))<ln(U,swi2(Q,x,y),swi2(Q,z,w)))
              Q = a;
    }
    
    //Q=C(U);
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//Mouse
__KERNEL__ void PourYannFuse__Buffer_B(float4 C, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    U+=0.5f;

    float4 p = texture(iChannel0,U/iResolution);
    if (iMouse.z>0.0f) {
      if (p.z>0.0f) C = to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
    else            C = to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else            C = to_float4_f2f2(-iResolution,-iResolution);


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

__KERNEL__ void PourYannFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 

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
    float v = smoothstep(1.0f,0.0f,l);
    Q.z += 0.01f*v;
    swi2S(Q,x,y, _mix(swi2(Q,x,y),norm(swi2(a,x,y)-swi2(a,z,w)),0.1f*v));
    //swi2(Q,x,y) *= 0.99f-0.5f*v;
    Q.x *= 0.99f-0.5f*v;
    Q.y *= 0.99f-0.5f*v;
    
    if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
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

__KERNEL__ void PourYannFuse(float4 Q, float2 U, float2 iResolution)
{

    float4 a = A(U);
    float4 d = D(U);
    float l = ln(U,swi2(a,x,y),swi2(a,z,w));
    Q = (0.8f+0.2f*swi4(d,x,x,x,x))*smoothstep(0.0f,1.0f,l);

  SetFragmentShaderComputedColor(Q);
}