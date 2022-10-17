
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define co(i) to_float2(mod_f(i,R.x),i/R.x)
#define id(U) (_floor((U).x)+_floor((U).y)*R.x)
//#define A(i) texelFetch(iChannel0,to_int2(co(i)),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)

#define A(i) texture(iChannel0,(make_float2(to_int2_cfloat(co(i)))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)

#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage (out float4 Q, float2 U) { U = swi2(gl_FragCoord,x,y);
#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)

#define O 6.0f
#define W round(0.75f*R.y/O)
#define shape(u) clamp(round((u)/O)*O,O*2.0f,2.0f*O+O*W)

__DEVICE__ float pie (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b); // midpoint
  if (length(a-b)<1e-3) return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return (length(p-a-(b-a)*i));
}
__DEVICE__ float2 cc (float2 a, float2 b, float2 c) {
  float2 ab = 0.5f*(a+b), ac = 0.5f*(a+c);
  float m1 = (a.x-b.x)/(b.y-a.y), m2 = (a.x-c.x)/(c.y-a.y);
  float b1 = ab.y-m1*ab.x, b2 = ac.y-m2*ac.x;
  float _x = (b1-b2)/(m2-m1);
  return to_float2(_x,m1*_x+b1);
}

__DEVICE__ float ssg (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return sign_f(dot(p-a,swi2((b-a),y,x)*to_float2(1,-1)))*length(p-a-(b-a)*i);
}
__DEVICE__ float tri (float2 U, float2 a, float2 aa, float2 aaa) {
    if (length(a-aaa)<1e-3||length(aa-aaa)<1e-3||length(a-aa)<1e-3) return 1e3;
    float ab = ssg(U,a,aa),
          bc = ssg(U,aa,aaa),
          ca = ssg(U,aaa,a),
          l = _fminf(_fabs(ab),_fminf(_fabs(bc),_fabs(ca))),
          s = (ab<0.0f && bc<0.0f && ca<0.0f)||(ab>0.0f && bc>0.0f && ca>0.0f)?-1.0f:1.0f;
  if (s>0.0f) return s*l;
    float2 m = cc(a,aa,aaa);
    float v = length(a-m), w = length(U-m);
    return l*s/v;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Information Storage
__KERNEL__ void DestructibleFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
  U+=0.5f;

  float2 u = shape(U);
  float i = id(U);
  Q = A(i);
  float2 f = to_float2_s(0), f1 = to_float2_s(0);
  float n = 0.0f, n1 = 0.0f;
  for (int _x = -2; _x <= 2; _x++)
    for (int _y = -2; _y <= 2; _y++){
        float2 v = shape(u+O*to_float2(_x,_y));
        // triangulated force
        f += swi2(C(swi2(Q,x,y)+to_float2(_x,_y)),x,y);
        // shape force
        float4 t = A(id(v));
        float2 r = swi2(t,x,y)-swi2(Q,x,y);
        float l = length(r), ll = length(u-v), lll = length(u+O*to_float2(_x,_y)-v);
        if (ll>1.0f && ll<3.0f*O && l>0.0f && lll<1.0f) {
            n++;
            f += 500.0f*r/l/_fmaxf(1.0f,l*l*ll)*sign_f(l-length(u-v));
        }
        // direct contact force
        t = A(B(swi2(Q,x,y)+to_float2(_x,_y)).x);
        r = swi2(t,x,y)-swi2(Q,x,y);
        l = length(r);
        if (l>0.0f) {
            n1++;
            f1 -= 50.0f*r/l/_fmaxf(1.0f,l*l)*smoothstep(0.9f*O,0.5f*O,l);
        }
    }
  if (n>0.0f)  f = f/n;    else f = to_float2_s(0);
  if (n1>0.0f) f1 = f1/n1; else f1 = to_float2_s(0);
  swi2S(Q,z,w, swi2(Q,z,w) + f + f1-to_float2(0,1e-3));
  swi2S(Q,x,y, swi2(Q,x,y) + f+swi2(Q,z,w)*(1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w)))));
    
  if (Q.x<15.0f)     {Q.z=_fabs(Q.z);}
  if (Q.y<15.0f)     {Q.w=_fabs(Q.w);}
  if (R.x-Q.x<15.0f) {Q.z=-_fabs(Q.z);}
  if (R.y-Q.y<15.0f) {Q.w=-_fabs(Q.w);}
  if (iFrame < 3)
    {
        Q = to_float4(u.x,u.y,0.4f,0.5f);
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
__DEVICE__ void X (inout float4 *Q, inout float4 *a, inout float4 *aa, inout float4 *aaa, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float4 b = B(U+r);
    // check b.x
    float4 n = A(b.x);
    float ln = length(swi2(n,x,y)-U), 
          la = length(swi2(*a,x,y)-U);
    if (ln<la) {
        (*Q).x = b.x;
        *a = n;
    }
  
    float pn = pie(U,swi2(*a,x,y),swi2(n,x,y)), 
          pa = pie(U,swi2(*a,x,y),swi2(*aa,x,y));
    if (pn<=pa){
        *aa = n;
        (*Q).y = b.x;
    }

    float tn = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(n,x,y)), 
          ta = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(*aaa,x,y));
    if (tn<ta){
        *aaa = n;
        (*Q).z = b.x;
    }
    // check b.y
    n = A(b.y);
    ln = length(swi2(n,x,y)-U);
    la = length(swi2(*a,x,y)-U);
    
    if (ln<la) {
        (*Q).x = b.y;
        *a = n;
    }
    pn = pie(U,swi2(*a,x,y),swi2(n,x,y));
    pa = pie(U,swi2(*a,x,y),swi2(*aa,x,y));
    if (pn<pa){
        *aa = n;
        (*Q).y = b.y;
    }

    tn = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(n,x,y)),
    ta = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(*aaa,x,y));
    if (tn<ta) {
    	  *aaa = n;
        (*Q).z = b.y;
    }
    // check b.z
	  n = A(b.z);
    ln = length(swi2(n,x,y)-U);
    la = length(swi2(*a,x,y)-U);
    if (ln<la) {
    	  (*Q).x = b.z;
        *a = n;
    }
    pn = pie(U,swi2(*a,x,y),swi2(n,x,y));
    pa = pie(U,swi2(*a,x,y),swi2(*aa,x,y));
    if (pn<pa){
        *aa = n;
        (*Q).y = b.z;
    }
    float zzzzzzzzzzzzzzzzzzzzzzzz;    
    tn = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(n,x,y)),
    ta = tri(U,swi2(*a,x,y),swi2(*aa,x,y),swi2(*aaa,x,y));
    if (tn<ta) {
      	*aaa = n;
        (*Q).z = b.z;
    }
}

__DEVICE__ void Xr (inout float4 *Q, inout float4 *a, inout float4 *aa, inout float4 *aaa, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
     X(Q,a,aa,aaa,U,to_float2(r,0),R,iChannel0,iChannel1);
     X(Q,a,aa,aaa,U,to_float2(0,r),R,iChannel0,iChannel1);
     X(Q,a,aa,aaa,U,to_float2(0,-r),R,iChannel0,iChannel1);
     X(Q,a,aa,aaa,U,to_float2(-r,0),R,iChannel0,iChannel1);
}


__KERNEL__ void DestructibleFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
  U+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBBB;  
  
  Q = B(U);
    float4 a = A(Q.x), aa= A(Q.y), aaa = A(Q.z);
    Xr(&Q,&a,&aa,&aaa,U,1.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,&aaa,U,2.0f,R,iChannel0,iChannel1);
    for (int i = 0; i < 10; i++) {
        float j = mod_f((float)(iFrame + i),R.x*R.y);
        if (length(U-swi2(A(j),x,y))<length(U-swi2(a,x,y))) Q.x = (float)(j);
    }
    Init {
        Q.z = Q.y = Q.x = (float)(id(shape(U)));
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
__KERNEL__ void DestructibleFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
  U+=0.5f; 
float CCCCCCCCCCCCCCCCC;  
  float4 b = B(U);
  float4 a = A(b.x), aa = A(b.y), aaa = A(b.z);
  float2 r = swi2(a,x,y)-swi2(aa,x,y), r1 = swi2(a,x,y)-swi2(aaa,x,y);
  float l = length(r), l1 = length(r1);
  if (l>0.0f && l1>0.0f) 
      swi2(Q,x,y) = 
                   50.0f*r/l/_fmaxf(1.0f,l*l)*smoothstep(0.9f*O,0.8f*O,l)+
                   50.0f*r1/l1/_fmaxf(1.0f,l1*l1)*smoothstep(0.9f*O,0.8f*O,l1);
  if (length(swi2(Q,x,y))>0.1f)   swi2S(Q,x,y, 0.1f*normalize(swi2(Q,x,y)));
  if (iMouse.z>0.0f && length(swi2(a,x,y)-swi2(iMouse,x,y))>0.0f)
        Q += to_float4(120,120,0,0)*swi4(clamp(0.03f*(swi2(a,x,y)-swi2(iMouse,x,y))/dot((swi2(a,x,y)-swi2(iMouse,x,y)),(swi2(a,x,y)-swi2(iMouse,x,y))),-2e-4,2e-4),x,y,x,y);
  
  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/3405e48f74815c7baa49133bdc835142948381fbe003ad2f12f5087715731153.ogv' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Destructible " by wyatt. https://shadertoy.com/view/3lKSDV
// 2020-03-13 18:10:34

// Fork of "Foam Simulation" by wyatt. https://shadertoy.com/view/tlyXDG
// 2020-03-08 02:50:08

// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54
__DEVICE__ float4 color (float i, float2 R) {
  
  float2 temp = shape(co(i))/O/W/2.0f;
  return to_float4(temp.x,temp.y,0,1);
}


__KERNEL__ void DestructibleFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
float IIIIIIIIIIIIIIIIIII;  
    
    float4 b = B(U),
    a = A(b.x), aa = A(b.y), aaa = A(b.z),
    col1 = color(b.x,R), col2 = color(b.y,R), col3 = color(b.z,R);
    float3 by = to_float3(
         sg(U,swi2(aa,x,y),swi2(aaa,x,y))/sg(swi2(a,x,y),swi2(aa,x,y),swi2(aaa,x,y)),
         sg(U,swi2(a,x,y),swi2(aaa,x,y))/sg(swi2(aa,x,y),swi2(a,x,y),swi2(aaa,x,y)),
         sg(U,swi2(a,x,y),swi2(aa,x,y))/sg(swi2(aaa,x,y),swi2(a,x,y),swi2(aa,x,y))
        );
    float o = length(U-swi2(a,x,y)),
    s = _fminf(sg(U,swi2(a,x,y),swi2(aa,x,y)),_fminf(sg(U,swi2(a,x,y),swi2(aaa,x,y)),sg(U,swi2(aa,x,y),swi2(aaa,x,y))));
    float2 w = co(b.x);
    col1 = by.x*col1+by.y*col2+by.z*col3;
    
    col1.x *= R.y/R.x;
    
    Q = _fmaxf(0.3f*smoothstep(O/(1.0f+0.1f*o),0.0f,s),
        smoothstep(1.1f*O,0.9f*O,o))*
        texture(iChannel2,1.6f*swi2(col1,x,y));
    Q = _fmaxf(Q,0.1f*abs_f4(sin_f4(to_float4_s(1+2+3+4)+0.1f*tri(U,swi2(a,x,y),swi2(aa,x,y),swi2(aaa,x,y)))));
    
  SetFragmentShaderComputedColor(Q);    
}