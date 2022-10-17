

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U).yzwx;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

#define dt .66
#define W dt*(m-Q+Q*d.x)

#define D(a,b) atan(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y)
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 
        d = B(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        a = A(U+vec2(1,1)),
        b = A(U+vec2(1,-1)),
        c = A(U-vec2(1,1)),
        f = A(U-vec2(1,-1)),
        m = 1./6.*(n+e+s+w+.5*(a+b+c+f));
    Q.xz += (W).yw;
    vec2 p = .6*U*mat2(cos(iTime),-sin(iTime),sin(iTime),cos(iTime));
    if (iFrame < 1) Q = vec4(0,.5,0,.5);
    if ((iFrame<1&&length(U-0.5*R)<40.)||(iMouse.z>0.&&length(U-iMouse.xy)<30.)) 
        Q=vec4(sin(p.x),cos(p.x),cos(p.x),-sin(p.x));
	
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 
        d = B(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        a = A(U+vec2(1,1)),
        b = A(U+vec2(1,-1)),
        c = A(U-vec2(1,1)),
        f = A(U-vec2(1,-1)),
        m = 1./6.*(n+e+s+w+.5*(a+b+c+f));
    Q.yw -= (W).xz;
    vec2 p = .6*U*mat2(cos(iTime),-sin(iTime),sin(iTime),cos(iTime));
    if (length(Q)<1e-4||iFrame < 1) Q = vec4(0,.5,0,.5);
    if ((iFrame<1&&length(U-0.5*R)<40.)||(iMouse.z>0.&&length(U-iMouse.xy)<30.)) 
        Q=vec4(sin(p.x),cos(p.x),cos(p.x),-sin(p.x));
	
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        a = A(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    float xy = dot(a.xy,a.xy), zw = dot(a.zw,a.zw);
    Q = 0.25*vec4 (
    	D (e.xy,w.xy),
        D (n.xy,s.xy),
    	D (e.zw,w.zw),
        D (n.zw,s.zw)
    );
    Q.xy = (Q.zw*zw+Q.xy*xy)/(xy+zw);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 b (vec2 U) {return B(U-A(U).xy);}
void mainImage( out vec4 Q, in vec2 U )
{
    Q.x = 0.25*(b(U+vec2(0,1))+b(U+vec2(1,0))+b(U-vec2(0,1))+b(U-vec2(1,0))).x;
    vec4 
        a = A(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    Q.x += 0.25*(n.y-s.y+e.x-w.x);
    
    Q.yzw = b(U).yzw;
    vec2 p = .05*U*mat2(cos(iTime),-sin(iTime),sin(iTime),cos(iTime));
    
    if (iFrame < 1) Q.yzw = vec3(0);
    if ((iFrame<1&&length(U-0.5*R)<40.)||(iMouse.z>0.&&length(U-iMouse.xy)<30.)) Q.yzw=abs(vec3(sin(p.x),cos(.5*p.x),cos(p.x)));
	
    
}