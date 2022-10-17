

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Display
void mainImage(out vec4 Q, in vec2 U)
{
    vec4 f = A(U), c = B(U);
    Q = c*min(f.w,1.3);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)

#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define r2 0.70710678118
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Forces
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        Q.xy -= 0.05*a.w*(a.w-.9)*u;  
    }
    Q.xy -= 4e-4*(U-0.5*R)/(1.+length(U-0.5*R));
    if (iMouse.z>0.) 
    Q.xy += -1e-3*(iMouse.xy-U)/(1.+length(iMouse.xy-U));
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    if (iFrame < 1)Q = vec4(0,0,0.,.5);
    if (U.x < 4.||R.x-U.x<4.) Q.xy *= 0.;
    if (U.y < 4.||R.y-U.y<4.) Q.xy *= 0.;
    if (Q.w>2.) Q.w = 2.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Advect
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
//Advect
void mainImage(out vec4 Q, in vec2 U)
{
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
    if (iFrame<1)
        Q = vec4(U/R,1.-(U.x+U.y)/R.x,1);
}