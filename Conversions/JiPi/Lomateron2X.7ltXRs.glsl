

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main 
{
    vec4 a = A(U), b = B(U); 
    Q = b.wwww;
    Q += a.w*max(cos(1.7+5.*a.z+vec4(1,2,3,4)),0.);
    Q = 1.-Q;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)


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
Main
{   std = 2.;
    vec4 dQ = Q = vec4(0);
    for (float x = -4.; x<=4.;x++)
    for (float y = -4.; y<=4.;y++)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec2 v = u+a.xy;
        float w = erfstep(-.5,.5,v.x)*
                  erfstep(-.5,.5,v.y);
        dQ.xyz += w*a.w*a.xyz;
        dQ.w   += w*a.w;
    }
    if (dQ.w>0.)
    {
        dQ.xyz/=dQ.w;
        Q = dQ;
    }
    
    
    if (iFrame < 1) {Q = vec4(0,0,0,.1);}
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
        
    Q = A(U);
    vec4 dQ = vec4(0);
    for (float x = -1.; x<=1.;x++)
    for (float y = -1.; y<=1.;y++)
    if(x!=0.||y!=0.)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 b = B(U+u);
        float f = 0.125*(
        a.w+b.w);
        dQ.xy -= f*u/dot(u,u);
    }
    Q += dQ;
    Q.y -= .5/R.y;
    vec2 M = 1.5*R;
    if (iMouse.z>0.) M = iMouse.xy;
    if(length(U-M)<.02*R.y)Q = vec4(.1*normalize(M-0.5*R),-1,3.);
    if (iFrame < 1) Q = vec4(0,0,U.x/R.x,.2+.1*cos(U.x));
    if (U.x<1.||U.y<1.||R.x-U.x<1.) Q.xy*=0.;
    if (R.y-U.y<1.)Q.w *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
{
    std = 1.;
    vec4 dQ = Q = vec4(0);
    for (float x = -4.; x<=4.;x++)
    for (float y = -4.; y<=4.;y++)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec2 v = u+a.xy;
        float w = erfstep(-.5,.5,v.x)*
                  erfstep(-.5,.5,v.y);
        dQ.xyz += w*a.w*a.xyz;
        dQ.w   += w*a.w;
    }
    if (dQ.w>0.)
    {
        dQ.xyz/=dQ.w;
        Q = dQ;
    }
    
    
    if (iFrame < 1) {Q = vec4(0,0,0,.1);}
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
        
    Q = A(U);
    vec4 dQ = vec4(0);
    for (float x = -1.; x<=1.;x++)
    for (float y = -1.; y<=1.;y++)
    if(x!=0.||y!=0.)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 b = B(U+u);
        float f = 0.1*(
        a.w*(a.w-1.)+b.w);
        dQ.xy -= f*u/dot(u,u);
    }
    Q += dQ;
    Q.y -= .5/R.y;
    vec2 M = 1.5*R;
    if (iMouse.z>0.) M = iMouse.xy;
    if(length(U-M)<.02*R.y)Q = vec4(.1*normalize(M-0.5*R),-1,3.);
    if (iFrame < 1) Q = vec4(0,0,0,.5+.1*sin(U.x));
    if (U.x<1.||U.y<1.||R.x-U.x<1.) Q.xy*=0.;
    if (R.y-U.y<1.)Q.w *= 0.;
}