

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
#define T(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
vec4 A(vec2 U) {
	return T(U-v(T(U)));
}
void mainImage( out vec4 Q, vec2 U )
{
    Q = (texture(iChannel2,D(U).xy/R));

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
#define T(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
vec4 A(vec2 U) {
    U-=v(T(U));
	return T(U);
}
void mainImage( out vec4 Q, in vec2  U)
{
    Q = A(U);
    
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    float px = 0.25*(p(e)-p(w));
    float py = 0.25*(p(n)-p(s));
     Q += 0.25*(n.w + e.y + s.z + w.x)-p(Q)-vec4(px,-px,py,-py);
    
    
    vec2 m = vec2(.9,.5+.3*sin(.01*iTime))*R;
    if (iMouse.z>0.) m = iMouse.xy;
    if (length(U-m) < 10.) Q.xy = vec2(.0,.4);
    if (iFrame < 1) Q = vec4(.2);
    if(U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)Q = vec4(p(Q));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define D(U) texture(iChannel1,(U)/R)
#define T(U) texture(iChannel2,(U)/R)
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
void mainImage( out vec4 Q, in vec2  U)
{
    U -= v(A(U));
    Q = D(U);
    if (iFrame < 1) Q = vec4(U,0,0);
}