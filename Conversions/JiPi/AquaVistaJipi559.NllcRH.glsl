

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Temperatures" by wyatt. https://shadertoy.com/view/fsf3zS
// 2021-03-22 22:23:14

// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-18 22:39:28

// Display
void mainImage(out vec4 Q, in vec2 U)
{
    vec4 f = A(U);
    Q = 1.-(.5-0.5*(sin(3.5-.3*(f.z)+vec4(1,2,3,4))))*f.w;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
//#define Main void mainImage(out vec4 Q, in vec2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Forces
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        float f = 0.07*a.w*((a.w-1.)+.3);
        dQ.xy -= f*u;
    }
    Q += dQ;
    if (abs(U.y-0.6*R.y)<.3*R.y) {
        Q.xy -= Q.xy*.1*exp(-16.*Q.w*Q.w);
    }
    Q.y -= 3e-4;
    vec2 M = 1.5*R;
    if (iMouse.z>0.) M = iMouse.xy;
    if(length(U-M)<.02*R.y)Q = vec4(.1*normalize(iMouse.xy-0.5*R),-1,1.);
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    if (iFrame < 1) Q = vec4(0,0,0,.12);
    if (R.y-U.y<2.) Q.zw = mix(Q.zw,vec2(-8.*step(U.x,.5*R.x)*U.x/R.x,1.5-U.x/R.x*1.),.005);
    if (U.x < 1.) Q.xy *= 0.;
    if (U.y < 1.||R.y-U.y<1.) Q.xy *= 0.;
    if (R.x-U.x<1.) Q.w *= 0.;
    if (abs(R.x*.5-U.x)<1.&&U.y<.15*R.y) Q.xy *= 0.;
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
       float j = .5+.5*max(1.-3.*Q.w*q.w,0.);
       float k = .5+.5*max(1.-3.*Q.w*q.w,0.);
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Q.xyz*wa+q.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
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
       float j = .5;
       float k = .5;
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Q.xyz*wa+q.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
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
       float j = .5;
       float k = .5;
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Q.xyz*wa+q.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
}