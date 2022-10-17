
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)
#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)


#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)

#define O 5.0f
#define W 5.0f

#define ORG
#ifdef ORG
__DEVICE__ float2 shape (float2 u, float tex) {
    u = round(u/O)*O;
    float2 q = W*O+_fminf(_floor(u/60.0f)*60.0f,to_float2_s(O*W*8.0f));
    u = clamp(u,O*2.0f+q,2.0f*O+O*W+q);
    return u;
}
#else

__DEVICE__ float2 shape (float2 u, float tex) {
    //u = round(u/O)*O;
    //float2 q = W*O+_fminf(_floor(u/60.0f)*60.0f,to_float2_s(O*W*8.0f));
    //u = clamp(u,O*2.0f+q,2.0f*O+O*W+q);
    
    if (tex==0.0f) u = to_float2_s(0.0f);
    
    return u;
}
#endif

__DEVICE__ float sg (float2 p, float2 a, float2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
    return (length(p-a-(b-a)*i));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b) {
    float2 m = 0.5f*(a+b); // midpoint
    if (length(a-b)<1e-3) return 1e3; // ignore self
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
__KERNEL__ void DestructibleFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  U+=0.5f;
float AAAAAAAAAAAAAAAAAAAAA;
  //Textur 
  float4 tex = texture(iChannel4, U/R);

  float2 u = shape(U,tex.w);
  Q = A(U);
  float2 f = to_float2_s(0), f1 = to_float2_s(0);
  float n = 0.0f, n1 = 0.0f;
  for (int _x = -2; _x <= 2; _x++)
    for (int _y = -2; _y <= 2; _y++){
        float2 v =shape(u+O*to_float2(_x,_y),tex.w);
        swi2S(Q,z,w, swi2(Q,z,w) + swi2(C(swi2(Q,x,y)+to_float2(_x,_y)),x,y));
        float4 t = A(v);
        float2 r = swi2(t,x,y)-swi2(Q,x,y);
        float l = length(r), ll = length(u-v), lll = length(u+O*to_float2(_x,_y)-v);
        if (ll>1.0f&&ll<3.0f*O&&l>0.0f&&lll<1.0f) {
            n++;
            f += 100.0f*r/l/_fmaxf(1.0f,l*l*ll)*sign_f(l-length(u-v));
        }
        t = A(swi2(B(swi2(Q,x,y)+to_float2(_x,_y)),x,y));
        r = swi2(t,x,y)-swi2(Q,x,y);
        l = length(r);
        if (l>0.0f) {
            n1++;
            f1 -= 10.0f*r/l/_fmaxf(1.0f,l*l)*smoothstep(0.9f*O,0.5f*O,l);
        }
    }
    if (n>0.0f) f = f/n;     else f  = to_float2_s(0);
    if (n1>0.0f) f1 = f1/n1; else f1 = to_float2_s(0);
    swi2S(Q,z,w, swi2(Q,z,w) + f + f1);
    swi2S(Q,x,y, swi2(Q,x,y) + f+swi2(Q,z,w)*(1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w)))));
    
    if (Q.x<5.0f)     {Q.x=5.0f;Q.z*=-0.9f;}
    if (Q.y<5.0f)     {Q.y=5.0f;Q.w*=-0.9f;}
    if (R.x-Q.x<5.0f) {Q.x=R.x-5.0f;Q.z*=-0.9f;}
    if (R.y-Q.y<5.0f) {Q.y=R.y-5.0f;Q.w*=-0.9f;}

  if (iFrame < 3)
    {
       Q = to_float4(u.x,u.y,0.2f,0.3f);
    }
    
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


__KERNEL__ void DestructibleFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  U+=0.5f;
  Q = B(U);
  if (iFrame<1) Q=to_float4_s(0.0f);
  
  
    float4 a = A(swi2(Q,x,y)), aa= A(swi2(Q,z,w));
    Xr(&Q,&a,&aa,U,1.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,2.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,4.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,8.0f,R,iChannel0,iChannel1);
    for (int i = 0; i < 10; i++) {
        float2 u = to_float2(
            (10*iFrame+i)%(int)(R.x),
            (10*iFrame+i)/(int)(R.x)%(int)(R.y));
        if (length(U-swi2(A(u),x,y))<length(U-swi2(a,x,y))) Q.x=u.x,Q.y=u.y;// swi2(Q,x,y) = u;
    }
    Init {
        //swi2(Q,x,y) = U;
        Q.x = U.x;
        Q.y = U.y;
        //Q.z = 0.0f;
        //Q.w = 0.0f;
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
__KERNEL__ void DestructibleFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  CONNECT_SLIDER0(Destructive, -1.0f, 1.0f, 0.1f);

  U+=0.5f;
  float4 b = B(U);
  float4 a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));
  float2 r = swi2(a,x,y)-swi2(aa,x,y);
  float l = length(r);
  float2 v = to_float2_s(0);
  if (l>0.0f) 
      swi2S(Q,x,y, 100.0f*r/l/_fmaxf(1.0f,l*l)*smoothstep(0.9f*O,0.5f*O,l));
  if (length(swi2(Q,x,y))>0.1f) 
      //swi2(Q,x,y) = 0.1f*normalize(swi2(Q,x,y)); //!!!!!!!!!!!!
      swi2S(Q,x,y, Destructive*normalize(swi2(Q,x,y))); //!!!!!!!!!!!!
  
  if (iMouse.z>0.0f && length(swi2(a,x,y)-swi2(iMouse,x,y))>0.0f)
        Q -= to_float4(2,2,0,0)*swi4(clamp(0.03f*(swi2(a,x,y)-swi2(iMouse,x,y))/dot((swi2(a,x,y)-swi2(iMouse,x,y)),(swi2(a,x,y)-swi2(iMouse,x,y))),-2e-4,2e-4),x,y,x,y);

  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Foam Simulation" by wyatt. https://shadertoy.com/view/tlyXDG
// 2020-03-08 02:50:08

// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54

__KERNEL__ void DestructibleFuse(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    U+=0.5f;
    
    //Textur 
    float4 tex = texture(iChannel4, U/R);
    
    float4 b = B(U),
         a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));

    float   
          r = length(swi2(a,x,y)-swi2(aa,x,y)), 
          rr = length(shape(swi2(b,x,y),tex.w)-shape(swi2(b,z,w),tex.w)),
          o = length(U-swi2(a,x,y)),
          w = sg(U,swi2(a,x,y),swi2(aa,x,y)),
          v = pie(U,swi2(a,x,y),swi2(aa,x,y));
    
    Q = smoothstep(1.5f*O,O,_fminf(rr,O*o))*
        smoothstep(2.0f,0.0f,w)*
        (0.6f+0.4f*sin_f4(to_float4(1,2,3,4)+6.2f*length(round(10.0f+swi2(b,x,y)/W/O)*W*O/R)));
        
  SetFragmentShaderComputedColor(Q);
}