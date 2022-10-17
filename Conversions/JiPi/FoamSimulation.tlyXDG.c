
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
//#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)

#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define E(U) texture(iChannel4,(U)/R)

#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)

__DEVICE__ float sg (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return (length(p-a-(b-a)*i));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b);   // midpoint
  if (length(a-b)<1e-3)  return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Information Storage
__KERNEL__ void FoamSimulationFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_CHECKBOX2(TexturInv, 0); 

    U+=0.5f;
    
    float2 u = _floor(U/8.0f+0.5f)*8.0f;
    u = clamp(u,to_float2_s(10),R-10.0f);
    Q = A(U);
    //swi2(Q,z,w) = C(swi2(Q,x,y)).zw;
    Q.z = C(swi2(Q,x,y)).z;
    Q.w = C(swi2(Q,x,y)).w;
    for (int _x = -2; _x <= 2; _x++)
    for (int _y = -2; _y <= 2; _y++)
      //swi2(Q,z,w) += C(swi2(Q,x,y)+to_float2(_x,_y)).xy;
      Q.z += C(swi2(Q,x,y)+to_float2(_x,_y)).x,
      Q.w += C(swi2(Q,x,y)+to_float2(_x,_y)).y;
    
    //swi2(Q,z,w) += 1e-6*_sinf(u);
    Q.z += 1e-6*_sinf(u.x);
    Q.w += 1e-6*_sinf(u.y);
    
    if (length(swi2(Q,z,w))>0.8f) swi2S(Q,z,w, 0.8f*normalize(swi2(Q,z,w)));
    //swi2(Q,x,y) += swi2(Q,z,w);
    Q.x += Q.z;
    Q.y += Q.w;
    
    if (Q.x<5.0f)     Q.x=5.0f;
    if (Q.y<5.0f)     Q.y=5.0f;
    if (R.x-Q.x<5.0f) Q.x=R.x-5.0f;
    if (R.y-Q.y<5.0f) Q.y=R.y-5.0f;

    if (Q.x<1.0f && Q.y<1.0f) Q = 10.0f*swi4(R,x,y,x,y);    
    if (init)
    {
        if (Textur)
        {
          float4 tex = E(U);
          
          if (tex.w > 0.0f && !TexturInv)
            Q = to_float4(0,0,0,0);
          
          if (tex.w > 0.0f && TexturInv)
            Q = to_float4(u.x,u.y,0,0);
          
            
        }
        else
          Q = to_float4(u.x,u.y,0,0);
    }
    
    if (Reset) Q = to_float4(0,0,0,0);
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// SORT TRIANGULATION
__DEVICE__ void X (inout float4 *Q, inout float4 *a, inout float4 *aa, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float4 b = B(U+r);
    float4 n = A(swi2(b,x,y));
    float ln = length(swi2(n,x,y)-U), la = length(swi2(*a,x,y)-U);
    if (ln<=la) {
        //swi2(Q,x,y) = swi2(b,x,y);
        (*Q).x = b.x;
        (*Q).y = b.y;
        //swi2(a,x,y) = swi2(n,x,y);
        (*a).x = n.x;
        (*a).y = n.y;
    }
    float pn = pie(U,swi2(*a,x,y),swi2(n,x,y)), 
          pa = pie(U,swi2(*a,x,y),swi2(*aa,x,y));
    if (pn<=pa){
        *aa = n;
        //swi2(Q,z,w) = swi2(b,x,y);
        (*Q).z = b.x;
        (*Q).w = b.y;
    }
    n = A(swi2(b,z,w));
    ln = length(swi2(n,x,y)-U);
    if (ln<la) {
        //swi2(Q,x,y) = swi2(b,z,w);
        (*Q).x = b.z;
        (*Q).y = b.w;
        //swi2(a,x,y) = swi2(n,x,y);
        (*a).x = n.x;
        (*a).y = n.y;
    }
    pn = pie(U,swi2(*a,x,y),swi2(n,x,y));
    if (pn<pa){
        *aa = n;
        //swi2(Q,z,w) = swi2(b,z,w);
        (*Q).z = b.z;
        (*Q).w = b.w;
    }
}
__DEVICE__ void Xr (inout float4 *Q, inout float4 *a, inout float4 *aa, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
     X(Q,a,aa,U,to_float2(r,0),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(0,r),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(0,-r),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(-r,0),R,iChannel0,iChannel1);
}


__KERNEL__ void FoamSimulationFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 

    U+=0.5f;
 
    Q = B(U);
    float4 a = A(swi2(Q,x,y)), aa = A(swi2(Q,z,w));
    Xr(&Q,&a,&aa,U,1.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,2.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,3.0f,R,iChannel0,iChannel1);
    for (int i = 0; i < 10; i++) {
        float2 u = to_float2(
            (10*iFrame+i)%(int)(R.x),
            (10*iFrame+i)/(int)(R.x)%(int)(R.y));
        if (length(U-swi2(A(u),x,y))<length(U-swi2(a,x,y)))  Q.x=u.x, Q.y=u.y;// swi2(Q,x,y) = u;
    }
    Init {
         //swi2(Q,x,y) = U;
         Q.x = U.x;
         Q.y = U.y;
    }
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// COMPUTE FORCE FIELD
__KERNEL__ void FoamSimulationFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 

    U+=0.5f; 
    float4 b = B(U);
    float4 a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));
    float2 r = swi2(a,x,y)-swi2(aa,x,y);
    float l = length(r);
    float2 v = to_float2_s(0);
    if (l>2.3f) 
      swi2S(Q,x,y, 0.1f*(-r/l/l+20.0f*r/l/l/l/l));

    
    //swi2(Q,z,w) = C(U-C(U).zw).zw;
    Q.z = C(U-swi2(C(U),z,w)).z;
    Q.w = C(U-swi2(C(U),z,w)).w;
    
    float4 
    n = D(U+to_float2(0,1)),
    e = D(U+to_float2(1,0)),
    s = D(U-to_float2(0,1)),
    w = D(U-to_float2(1,0));
    swi2S(Q,z,w, _mix(swi2(Q,z,w),
           _mix(swi2(aa,z,w),swi2(a,z,w),0.5f+0.5f*pie(U,swi2(a,x,y),swi2(aa,x,y))),
           smoothstep(2.5f,1.5f,sg(U,swi2(a,x,y),swi2(aa,x,y)))*
           smoothstep(12.0f,8.0f,_fminf(length(swi2(aa,x,y)-swi2(a,x,y)),10.0f*length(U-swi2(a,x,y)))) ));
    //swi2(Q,z,w) -= 0.25f*to_float2(e.z-w.z,n.z-s.z);
    Q.z -= 0.25f*(e.z-w.z);
    Q.w -= 0.25f*(n.z-s.z);
    
    
    if (length(swi2(Q,z,w))>0.8f)  swi2S(Q,z,w, 0.8f*normalize(swi2(Q,z,w)));
    if (iMouse.z>0.&&length(swi2(a,x,y)-swi2(iMouse,x,y))>0.0f)
        Q -= to_float4(1,1,-1,-1)*swi4(clamp(0.03f*(swi2(a,x,y)-swi2(iMouse,x,y))/dot((swi2(a,x,y)-swi2(iMouse,x,y)),(swi2(a,x,y)-swi2(iMouse,x,y))),-2e-4,2e-4),x,y,x,y);
  
    if (U.x < 2.0f||U.y < 2.0f||R.x-U.x<2.0f||R.y-U.y<2.0f) Q.z*=0.0f,Q.w*=0.0f;//swi2(Q,z,w) *= 0.0f;
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void FoamSimulationFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 

    U+=0.5f;
    Q = D(U-swi2(C(U),z,w));
    float4 
        n = C(U+to_float2(0,1)),
        e = C(U+to_float2(1,0)),
        s = C(U-to_float2(0,1)),
        w = C(U-to_float2(1,0));
    swi2S(Q,z,w, to_float2(Q.z,0)-0.25f*(e.z-w.z+n.w-s.w));
    //Q.z = (Q.z)-0.25f*(e.z-w.z+n.w-s.w);
    //Q.w = (0.0f)-0.25f*(e.z-w.z+n.w-s.w);
    
    SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54

__KERNEL__ void FoamSimulationFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 

    U+=0.5f;
    float4 b = B(U),
           a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));
    
    float   
          r = length(swi2(a,x,y)-swi2(aa,x,y)), 
          o = length(U-swi2(a,x,y)),
          w = sg(U,swi2(a,x,y),swi2(aa,x,y)),
          v = pie(U,swi2(a,x,y),swi2(aa,x,y));
    
    Q = 0.5f+0.5f*smoothstep(12.0f,8.0f,_fminf(r,10.0f*o))*
        smoothstep(3.0f,0.0f,w)*
        to_float4(0.9f,0.8f,1,1);
    Q = 0.5f+0.5f*sin_f4(1.0f+3.1f*Q+2.0f*_fabs(D(U).z));
    
    SetFragmentShaderComputedColor(Q);
}
