

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "In Air" by wyatt. https://shadertoy.com/view/7tVXDw
// 2022-06-09 22:26:36

// Fork of "Water Fall" by wyatt. https://shadertoy.com/view/NtKGWD
// 2021-12-30 04:31:49

void mainImage(out vec4 Q, in vec2 U)
{
    vec4 f = A(U),b=B(U);
    Q = .6*f.wwww*vec4(1,2,3,4);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
//#define Main void mainImage(out vec4 Q, in vec2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118
// oneshade:
//https://www.shadertoy.com/view/7sKSRh
float std;
float erf(in float x) {
    x *= std;
    //return sign(x) * sqrt(1.0 - exp(-1.239192 * x * x));
    return sign(x) * sqrt(1.0 - exp2(-1.787776 * x * x)); // likely faster version by @spalmer
}
float erfstep (float a, float b, float x) {
    return .5*(erf(b-x)-erf(a-x));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Forces
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u),b=B(U+u);
        float f = 0.05*(a.w*(a.w-.8)+.2*b.w);
        dQ.xy -= f*u;
    }
    Q += dQ;
    Q.y -= .1/R.y;
    Q = clamp(Q,-2.,2.);
    vec2 M = 1.5*R;
    if (iMouse.z>0.) M = iMouse.xy;
    if(length(U-M)<.02*R.y)Q = vec4(0,0,0.*sin(iTime),1.);
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    if (iFrame < 1) Q = vec4(0);
    
    if (abs(U.x-.5*R.x)<.005*R.x&&U.y<.3*R.y)
        Q.x = 0.,Q.w = .2, Q.y = 1.;
    if (abs(U.x-.25*R.x)<.005*R.x&&U.y<.2*R.y)
        Q.x = 0.,Q.w = .2, Q.y = 1.;
    if (abs(U.x-.75*R.x)<.005*R.x&&U.y<.2*R.y)
        Q.x = 0.,Q.w = .2, Q.y = 1.;
    
    if (U.x<1.||U.y<1.||R.y-U.y<1.) Q.xy*=0.;
    if(R.x-U.x<1.)Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Advect
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 q = A(U+u);
        vec2 a = Q.xy,
             b = q.xy+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5*u-a))/ab;
       {
           float j = 1.;
           float k = 1.;
           float wa = 0.25*Q.w*min(i,j)/j;
           float wb = 0.25*q.w*max(k+i-1.,0.)/k;
            dQ.w += wa+wb;
      }
      {
           float j = 1.;
           float k = 1.;
           float wa = 0.25*Q.w*min(i,j)/j;
           float wb = 0.25*q.w*max(k+i-1.,0.)/k;
            dQ.xyz += Q.xyz*wa+q.xyz*wb;
      }
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Forces
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u),b=B(U+u);
        float f = 0.1*(a.w+b.w);
        dQ.xy -= f*u;
    }
    Q += dQ;
    Q = clamp(Q,-2.,2.);
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    if (iFrame < 1) Q = vec4(0,0,0,.1);
    if (U.x<1.||U.y<1.||R.y-U.y<1.) Q.xy*=0.;
    if(R.x-U.x<1.)Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Advect
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 q = A(U+u);
        vec2 a = Q.xy,
             b = q.xy+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5*u-a))/ab;
       float j = .6;
       float k = .6;
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Q.xyz*wa+q.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
}