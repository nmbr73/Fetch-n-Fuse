

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Colors!" by wyatt. https://shadertoy.com/view/7dX3z7
// 2021-04-29 06:26:31

// Fork of "Transport Dynamics II" by wyatt. https://shadertoy.com/view/sdl3RN
// 2021-03-14 01:41:52

// Display

void mainImage(out vec4 Q, in vec2 U)
{
    U+=0.5;
    float h = Paper(U);
    Q = 1.5*D(U)+.8*C(U)*A(U).w;
    Q = .8+h-Q;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
//#define Main void mainImage(out vec4 Q, in vec2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118
float hash(vec2 p) // Dave H
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
float Paper (vec2 U) { //https://www.shadertoy.com/view/NsfXWs
    float h = .005*(sin(.6*U.x+.1*U.y)+sin(.7*U.y-.1*U.x));
    for (float x = -1.; x<=1.;x++)
    for (float y = -1.; y<=1.;y++){
        h += .15*.125*hash(U+vec2(x,y));
    }
    return h;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Forces
Main void mainImage(out vec4 Q, in vec2 U)
{
    U+=0.5;
    Q = A(U);
    vec4 m = C(U);
    float d = dot(m,vec4(-1,1,-1,1));
    box if(x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        vec4 c = C(U+u);
        float p = .1*Paper(U+u);
        float f = length(m-c)*d;
        Q.xy -= 0.05*a.w*(p+a.w-Q.w*f-1.)*u/length(u);  
    }
    Q.w *= 1.-1e-4;
    //Q.y -= 1e-4*Q.w;
    Q.xy *= 1.-exp(-10.*Q.w);
    if (iMouse.z>0.&&length(U-iMouse.xy)<20.)
        Q.w = 1.;
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    if (iFrame < 1)Q = vec4(0,0,0.,.05);
    if (U.x < 4.||R.x-U.x<4.) Q.xy *= 0.;
    if (U.y < 4.||R.y-U.y<4.) Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Advect
void mainImage(out vec4 Q, in vec2 U)
{
    U+=0.5;
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
       float j = .5+.5*max(1.-Q.w*q.w,0.);
       float k = .5+.5*max(1.-Q.w*q.w,0.);
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Q.xyz*wa+q.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    
    if (U.x < 4.||R.x-U.x<4.) Q.xy *= 0.;
    if (U.y < 4.||R.y-U.y<4.) Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Advect
void mainImage(out vec4 Q, in vec2 U)
{
    U+=0.5;
    Q = A(U);
    vec4 Qb = B(U);
    vec4 dQ = vec4(0);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 q = A(U+u),
             qb = B(U+u);
        vec2 a = Q.xy,
             b = q.xy+u;
       float ab = dot(u,b-a);
       float i = dot(u,(0.5*u-a))/ab;
       float j = .5;
       float k = .5;
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Qb.xyz*wa+qb.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
    if (iMouse.z>0.&&length(U-iMouse.xy)<20.)
        Q = 0.5+0.5*sin(vec4(1,2,3,4)+iTime);
    if (iFrame<1)
        Q = 0.5+0.5*sin(vec4(1,2,3,4)*4.*length(U-0.5*R)/R.x);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U)
{
    U+=0.5;
    Q = D(U);
    vec4 a = A(U);
    vec4 c = C(U)*a.w;
    
    Q = mix(Q,c,.001*a.w*a.w);
    
}