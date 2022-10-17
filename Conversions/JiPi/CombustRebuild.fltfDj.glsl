

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U)
{
    vec4 f = A(U), c = B(U);
    Q = c*min(f.w,1.3);
    Q = ((.5-.5*cos(5.*c.x+c.y*vec4(1.,2,3,4))))*atan(f.wwww);
    Q += f.z*f.w*vec4(1,.5,0,1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
//#define Main void mainImage(out vec4 Q, in vec2 U)
#define box for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)
#define dt .2
#define _a .05
#define _b .05
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 b = B(U);
    Q.w = b.w;
    vec4 T = Q;
    box if(abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        float f = dt*.25*(a.w*(a.w-1.+a.z));
        Q.xy -= f*u.xy;
        Q.z  += .5*.25*(a.z-T.z)-Q.w*f*dot(a.xy-T.xy+u.xy,u.xy); 
    }
    float M = dt*Q.z*Q.w*b.x*b.y;
    Q.z += 10.*M;
    Q.y -= .1/R.y;
    if (iMouse.z>0.&&length(U-iMouse.xy)<50.)
        Q.x = .5;
    if (iFrame < 1)Q = vec4(0,0,.1,.1);
    if (U.x < 4.||R.x-U.x<4.) Q.xyz *= 0.;
    if (U.y < 4.||R.y-U.y<4.) Q.xyz *= 0.;
    Q = clamp(Q,vec4(-.5,-.5,0,0),vec4(.5,.5,2,Q.w));
    if (length(Q.xy)>.5) Q.xy = .5*normalize(Q.xy);
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
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
       float j = .5+.5*max(1.-5.*Q.w*q.w,0.);
       float k = .5+.5*max(1.-5.*Q.w*q.w,0.);
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
       float j = .5;//+.2*max(1.-5.*Q.w*q.w,0.);
       float k = .5;//+.2*max(1.-5.*Q.w*q.w,0.);
       float wa = 0.25*Q.w*min(i,j)/j;
       float wb = 0.25*q.w*max(k+i-1.,0.)/k;
        dQ.xyz += Qb.xyz*wa+qb.xyz*wb;
        dQ.w += wa+wb;
        
    }
    if (dQ.w>0.)dQ.xyz/=dQ.w;
    Q = dQ;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Reaction
void mainImage(out vec4 Q, in vec2 U)
{
    Q = A(U);
    vec4 b = B(U);
    Q.w = b.w;
    
    float M = dt*b.z*b.w*Q.x*Q.y;
    Q.x += -_a*M;
    Q.y += -_b*M;
    Q.z += M;
    Q.w += (1.-_a-_b)*M;
    
    
    if (iFrame<1) {
        float l = length(U-vec2(.5,.1)*R);
        Q = vec4(step(l,.1*R.y),
                 1,
                 0,.5);
    }
}