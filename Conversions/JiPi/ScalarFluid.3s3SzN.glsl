

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    Q = abs(sin(.3*Q+dot(Q,Q)*vec4(1,2,3,4)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define C(U) texture(iChannel2,(U)/R)
#define A(U) texture(iChannel0,(U-C(U).xy)/R)
#define B(U) texture(iChannel1,(U)/R)
#define M 0.125*(A(U+vec2(0,1))+A(U-vec2(0,1))+A(U+vec2(1,0))+A(U-vec2(1,0))   +   A(U+vec2(1,1))+A(U+vec2(1,-1))+A(U-vec2(1,1))+A(U-vec2(1,-1)))
#define F .5*(m.xy-Q.xy)


#define P(a,b) vec2(length(a.xy)-length(b.xy),atan(a.x*b.y-a.y*b.x,dot(a.xy,b.xy)))
#define q 0.
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M;
    Q.x -= (F).y+q*(Q.x-m.x);
      
    if (iFrame < 1) Q.xy = vec2(1,0);
    vec2 a = .2*U.xy*mat2(cos(iTime),-sin(iTime),sin(iTime),cos(iTime));
	if ((iFrame < 1&&length(U-vec2(.8,.5)*R)<20.)||iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xy = vec2(sin(a.x),cos(a.x));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 m = M;
    Q.y += (F).x+q*(Q.y-m.y);
    
    float o = -B(U).x;
    Q.xy *= mat2(cos(o),-sin(o),sin(o),cos(o));
    
    if (length(Q.xy)<1e-6) Q.xy = vec2(1,0);
    Q.xy = mix(Q.xy,normalize(Q.xy),3e-3);
    
    if (iFrame < 1) Q.xy = vec2(1,0);
    vec2 a = .2*U.xy*mat2(cos(iTime),-sin(iTime),sin(iTime),cos(iTime));
	if ((iFrame < 1&&length(U-vec2(.8,.5)*R)<20.)||iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xy = vec2(sin(a.x),cos(a.x));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = B(U+vec2(0,1)),
        e = B(U+vec2(1,0)),
        s = B(U-vec2(0,1)),
        w = B(U-vec2(1,0));
    vec2 a = P(e,w);
    vec2 b = P(n,s);
 	vec2 g = 0.25*vec2(
        a.y,
        b.y
    );
    Q = vec4(-g,0,1);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        n = C(U+vec2(0,1)),
        e = C(U+vec2(1,0)),
        s = C(U-vec2(0,1)),
        w = C(U-vec2(1,0)),
        n1 = B(U+vec2(0,1)),
        e1 = B(U+vec2(1,0)),
        s1 = B(U-vec2(0,1)),
        w1 = B(U-vec2(1,0));
    Q = 0.25*(n1+e1+s1+w1);
    Q -= vec4(.25)*(n.y-s.y+e.x-w.x);
    
    if (iFrame < 1) Q = vec4(0);
}