

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
#define a(U) texture(iChannel0,(U)/R)
#define b(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    vec4 A = a(U);
    vec4 
        n = a(U+vec2(0,1)),
        e = a(U+vec2(1,0)),
        s = a(U-vec2(0,1)),
        w = a(U-vec2(1,0)),
        u = 0.25*(n+e+s+w), 
        l = u-A;
   	vec3 g = normalize(vec3(e.w-w.w,n.w-s.w,.4)),
         r = reflect(g,vec3(0,0,1));
	float q = max(0.,dot(r,normalize(vec3(1,1,-1))));
    Q = exp(-2.*q*q)*(.3+max(sin(5.31+l+2.*atan(4.*A.w)*vec4(-1,1,2,4)),-.2));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
#define a(U) texture(iChannel0,(U)/R)
vec4 A (vec2 U) {
	return a(U-a(U).xy);
}
#define B(U) texture(iChannel1,(U)/R)
void mainImage( out vec4 Q, vec2 U )
{
    Q = A(U);
    vec4 
        b = B(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        nb = B(U+vec2(0,1)),
        eb = B(U+vec2(1,0)),
        sb = B(U-vec2(0,1)),
        wb = B(U-vec2(1,0));
    float dw = 0.25*(e.w*e.x-w.w*w.x+n.w*n.y-s.w*s.y);
    Q.z = 0.25*(n.z+e.z+s.z+w.z+e.x-w.x+n.y-s.y);
    Q.xy += .05*(0.25*(n+e+s+w).xy-Q.xy);
    Q.x += 0.25*(e.z-w.z);
    Q.y += 0.25*(n.z-s.z);
    
    Q.w -= dw;
    Q.y += c*(Q.w);
    Q.xy *= 1.-.01*step(abs(Q.w),.01);
    Q.xy += Q.w*k*b.xy;
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) {Q.xyw = .7*vec3(cos(.5*iTime),sin(.5*iTime),sin(iTime));}
    if (iFrame < 1) Q.w = 0.5-0.5*sign(U.y-80.);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)  Q.xyw*= 0.;
    if (iFrame<1&&R.y-U.y<80.) Q.w = -1.;
    Q.xy = clamp(Q.xy,-1.,1.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define R iResolution.xy
#define aa(U) texture(iChannel0,(U)/R)
vec4 A (vec2 U) {
	return aa(U-aa(U).xy);
}
#define b(U) texture(iChannel1,(U)/R)
vec4 B (vec2 U) {
	return b(U-aa(U).xy);
}
void mainImage( out vec4 Q, vec2 U )
{
    Q = B(U);
    vec4 a = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    Q.xy += a.w*k*(0.25*(n+e+s+w).xy - a.xy);
    
    Q.w = (a.w+n.w+e.w+s.w+w.w)/5.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define k .6
#define c 0.003
#define f .5