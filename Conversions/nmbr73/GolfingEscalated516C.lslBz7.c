
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/paulaloveless/puryx-re-elevated' to iChannel0


// 516 chars - Xor does the IMPOSSIBLE by removing 13 chars!!

#define V vec3//
#define S 0.1f*_sinf(q.x+_sinf(q.z))//
mat2 m = mat2(8,6,-6,8);//
float h,s,t,x,p,d,v;//
V q,z,w,U;//
#define g(p)(q=p,h=_fabs(S),swi2(q,x,z)*=m*0.1f,h+S)//
__DEVICE__ float n(V p){//
    for(p*=0.1f,s=0.08f,t=0.9f; (s/=0.4f)<4e2; p+=t) t-=g(p)/s, swi2(p,x,z)*=m*0.21f;//
    return 3.0f-_expf(t);}//
__DEVICE__ void mainImage(out float4 O, float2 u) {
    U = V(u,0)/iResolution-1.0f;
    for(d=p=x=0.0f; d++<2e2 && p/5e3<=x;)
        z=V(0,-8.0f*g(V(0,0,v=iTime*0.2f)),v/0.1f)+p*normalize(U-V(_sinf(v),U.y*0.7f-0.1f,-3)),
        p += x = z.y+n(z);
    swi3(O,x,y,z) = d/5e2+0.1f+0.1f*_logf(p)
                -dot(V(w.z=0.01f),
                     normalize(V(n(z-swi3(w,z,y,x)),
                                 x = n(z),
                                 n(z-w)-n(swi3(z,z,y,x)*11.0f)/5e2)
                               -x))
           *n(swi3(z,z,y,x)*6.0f)*V(5,10,15); }/*


// 529 chars - Applying #define trick:

#define V vec3//
#define S.1*_sinf(q.x+_sinf(q.z))//
mat2 m = mat2(8,6,-6,8);//
float h,s,t,x,p,o=0.1f,d,v;//
V q,z,w;//
#define g(p)(q=p,h=_fabs(S),swi2(q,x,z)*=m*0.1f,h+S)//
__DEVICE__ float n(V p){//
    for(p*=0.1f,s=0.08f,t=0.9f; (s/=0.4f)<4e2; p+=t) t-=g(p)/s, p.xz=swi2(p,x,z)*m*0.21f;//
    return 3.0f-_expf(t);}//
#define mainImage(O,u)                                                                  \
    float2 U = u/iResolution;                                                          \
    U--;                                                                                \
    for(d=p=x=0.0f; d++<2e2 && p/5e3<=x; o+=0.002f )                                        \
        z=V(0,-8.0f*g(w=V(0,0,v=iTime*0.2f)),v/0.1f)+p*normalize(V(U.x-_sinf(v),U.y*0.3f+0.1f,2)),  \
        p += x = z.y+n(z);                                                              \
    swi3(O,x,y,z) = o+0.1f*_logf(p)                                                                 \
                -dot(V(w.z=0.01f),                                                        \
                     normalize(V(n(z-swi3(w,z,y,x)),                                            \
                                 x = n(z),                                              \
                                 n(z-w)-n(swi3(z,z,y,x)*11.0f)/5e2)                               \
                               -x))                                                     \
                *n(swi3(z,z,y,x)*6.0f)*V(5,10,15)                                            


        
// 540 chars - Golfing master coyote, does it again ...

#define V float3
#define S.1*_sinf(q.x+_sinf(q.z))

mat2 m = mat2(8,6,-6,8);
float h,s,t,x,p,o=0.1f,d,v;
V q,z,w;

#define g(p)(q=p,h=_fabs(S),swi2(q,x,z)*=m*0.1f,h+S)

__DEVICE__ float n(V p){
    for(p*=0.1f,s=0.08f,t=0.9f; (s/=0.4f)<4e2; p+=t) t-=g(p)/s, p.xz=swi2(p,x,z)*m*0.21f;
    return 3.0f-_expf(t);}

__DEVICE__ void mainImage(out float4 O,float2 U)
{
    U/=iResolution;
    U--;
    for(d=p=x=0.0f; d++<2e2 && p/5e3<=x; o+=0.002f )
        z=V(0,-8.0f*g(w=V(0,0,v=iTime*0.2f)),v/0.1f)+p*normalize(V(U.x-_sinf(v),U.y*0.3f+0.1f,2)),
        p += x = z.y+n(z);
    swi3(O,x,y,z) = o+0.1f*_logf(p)
                -dot(V(w.z=0.01f),
                     normalize(V(n(z-swi3(w,z,y,x)),
                                 x = n(z),
                                 n(z-w)-n(swi3(z,z,y,x)*11.0f)/5e2)
                               -x))
                *n(swi3(z,z,y,x)*6.0f)*V(5,10,15);
}



// 555 chars - stduhpf uses several optimizations to bring this shader DOWN!

#define V float3
#define S _sinf(p.x+_sinf(p.z))

mat2 m = 0.1f*mat2(8,6,-6,8);
float h,s,t,i;

__DEVICE__ float g(V p){
    h=_fabs(S); swi2(p,x,z)*=m;
    return h+S;}

__DEVICE__ float n(V p){
    for(p*=0.1f,s=5.0f,i=t=0.9f; i++<9.0f;) t-=s*0.1f*g(p), s*=0.4f, swi2(p,x,z)*=m*2.1f, p+=t;
    return 3.0f-_expf(t);}

__DEVICE__ void mainImage(out float4 O,float2 U)
{
    float x=0.0f,p=x,o=1.0f,d=x;
    V z, w = V(0,0,iTime*0.2f);
    U/=iResolution;
    for(U--; d++<2e2 && p/5e3<=x;o+=0.02f )
        z=V(0,-0.8f*g(w),w.z/0.1f)+p*normalize(V(U.x-_sinf(w.z),U.y*0.3f+0.1f,2)),
        p += x = z.y+n(z);
    w.z=0.01f;
    swi3(O,x,y,z) = 0.1f*(o+_logf(p)
                -dot(V(0.5f),
                     normalize(V(n(z-swi3(w,z,y,x)),
                                 x = n(z),
                                 n(z-w)-n(swi3(z,z,y,x)*11.0f)/5e2)
                               -x))
                *n(swi3(z,z,y,x)*6.0f)*V(1,2,3));
}



// 598 chars - by Greg Rostami

#define V float3
#define W float2
#define S _sinf(p.x+_sinf(p.y))

mat2 m = 0.1f*mat2(8,6,-6,8);
float h,s,t;

__DEVICE__ float g(W p){
    h=_fabs(S); p*=m;
  return h+S;}

__DEVICE__ float n(W p){
    p*=0.1f;
    s=5.0f,t=0.9f;
  for(int i=0;i++<9;) t-=s*0.1f*g(p), s*=0.4f, p=p*m*2.1f+t;
    return 3.0f-_expf(t);}

__DEVICE__ void mainImage(out float4 O,W U)
{
    float e,v=iTime*0.2f,u=_sinf(v),x=0.0f,p=x,o=x;
  V r=V(U/iResolution-1.0f,0),z,y;
  for(int d=0;d++<200;)        
        if (p/5e3<=x)
      z=V(0,-0.8f*g(W(0,v)),v/0.1f)+p*normalize(V(r.x-u,r.y*0.3f+0.1f,2)),
            x=z.y+n(swi2(z,x,z)),p+=x,o++;
    x=n(swi2(z,x,z));
    swi3(O,x,y,z) = 0.1f*(dot(V(-0.5f),normalize(V(n(swi2(z,x,z)-W(0.01f,0))-x,0,n(swi2(z,x,z)-W(0,0.01f))-x-n(swi2(z,z,x)*11.0f)/5e2)))*
        n(swi2(z,z,x)*6.0f)*V(1,2,3)+1.0f+o/50.0f+_logf(p));

}



// 664 chars: drift's original shader:

mat2 m=mat2(0.8f,-0.6f,0.6f,0.8f);

__DEVICE__ float g(float2 p){
    float e=_fabs(_sinf(p.x+_sinf(p.y)));p=m*p;
  return 0.1f*(e+_sinf(p.x+_sinf(p.y)));
}

__DEVICE__ float n(float2 p){
    p*=0.1f;
    float s=5.0f,t=0.9f;
  for(int i=0;i<9;i++)
        t-=s*g(p),s*=0.4f,p=m*2.1f*p+t;
    return 3.0f-_expf(t);
}

__KERNEL__ void GolfingEscalated516CFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float v=iTime*2.0f,u=_sinf(v*0.1f),x=0.0f,p=0.0f,o=0.0f;
  float3 r=to_float3_aw(fragCoord/iResolution-1.0f,0),z,y;
  for(int d=0;d<288;d++)        
        if (p*0.0002f<=x)
      z=to_float3(0,-8.0f*g(to_float2(0,v)*0.1f),v)+p*normalize(to_float3(r.x-u,r.y*0.3f+0.1f,2)),x=z.y+n(swi2(z,x,z)),p+=x,o++;
    x=n(swi2(z,x,z));
    y=normalize(to_float3(n(swi2(z,x,z)-to_float2(0.01f,0))-x,0,n(swi2(z,x,z)-to_float2(0,0.01f))-x-n(swi2(z,z,x)*11.0f)*0.002f));
    fragColor.xyz=dot(to_float3_aw(-0.5f),y)*n(swi2(z,z,x)*6.0f)*to_float3(0.1f,0.2f,0.3f)+0.1f+o*0.002f+_logf(p)*0.1f;


  SetFragmentShaderComputedColor(fragColor);
}