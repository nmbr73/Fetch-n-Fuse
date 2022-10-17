

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U),
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    vec3 no = normalize(vec3(e.z+e.w-w.z-w.w,n.z+n.w-s.z-s.w,1));
    Q = 0.7+0.7*sin(a+4.*a.z+100.*a.w+vec4(1,2,3,4));
    Q *= .5+.5*texture(iChannel2,no);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define r 12.
#define v vec2(-0.1,0)

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 T(vec2 U) {
	U -= A(U).xy;
    return A(U);
}
void mainImage( out vec4 Q, in vec2 U)
{
    Q = T(U);
    vec4 	n = T(U+vec2(0,1)),
        	e = T(U+vec2(1,0)),
        	s = T(U-vec2(0,1)),
        	w = T(U-vec2(1,0)),
        	m = 0.25*(n+e+s+w);
    float div = 0.25*(n.y+e.x-s.y-w.x);
	Q.xy -= 0.25/max(1.,Q.w)*vec2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w);
	Q.z = m.z-div;
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q = vec4(0,.3,Q.z,1.05);
    if (abs(U.x-0.2*R.x)<20.&&abs(U.y-0.5*R.y)<20.) Q = vec4(v,Q.z,1.05);
    if (iFrame < 1||U.x<2.||R.x-U.x<2.) Q = vec4(.3,0,1,1);
    if (U.y<2.||R.y-U.y<2.) Q = vec4(0,0,Q.z,1);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 T(vec2 U) {
	U -= A(U).xy;
    return A(U);
}
void mainImage( out vec4 Q, in vec2 U)
{
    Q = T(U);
    vec4 	n = T(U+vec2(0,1)),
        	e = T(U+vec2(1,0)),
        	s = T(U-vec2(0,1)),
        	w = T(U-vec2(1,0)),
        	m = 0.25*(n+e+s+w);
    float div = 0.25*(n.y+e.x-s.y-w.x);
	Q.xy -= 0.25/max(1.,Q.w)*vec2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w);
	Q.z = m.z-div;
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q = vec4(0,.3,Q.z,1.05);
    if (abs(U.x-0.2*R.x)<20.&&abs(U.y-0.5*R.y)<20.) Q = vec4(v,Q.z,1.05);
    if (iFrame < 1||U.x<2.||R.x-U.x<2.) Q = vec4(.3,0,1,1);
    if (U.y<2.||R.y-U.y<2.) Q = vec4(0,0,Q.z,1);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec4 T(vec2 U) {
	U -= A(U).xy;
    return A(U);
}
void mainImage( out vec4 Q, in vec2 U)
{
    Q = T(U);
    vec4 	n = T(U+vec2(0,1)),
        	e = T(U+vec2(1,0)),
        	s = T(U-vec2(0,1)),
        	w = T(U-vec2(1,0)),
        	m = 0.25*(n+e+s+w);
    float div = 0.25*(n.y+e.x-s.y-w.x);
	Q.xy -= 0.25/max(1.,Q.w)*vec2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w);
	Q.z = m.z-div;
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q = vec4(0,.3,Q.z,1.05);
    if (abs(U.x-0.2*R.x)<20.&&abs(U.y-0.5*R.y)<20.) Q = vec4(v,Q.z,1.05);
    if (iFrame < 1||U.x<2.||R.x-U.x<2.) Q = vec4(.3,0,1,1);
    if (U.y<2.||R.y-U.y<2.) Q = vec4(0,0,Q.z,1);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 T(vec2 U) {
	U -= A(U).xy;
    return A(U);
}
void mainImage( out vec4 Q, in vec2 U)
{
    Q = T(U);
    vec4 	n = T(U+vec2(0,1)),
        	e = T(U+vec2(1,0)),
        	s = T(U-vec2(0,1)),
        	w = T(U-vec2(1,0)),
        	m = 0.25*(n+e+s+w);
    float div = 0.25*(n.y+e.x-s.y-w.x);
	Q.xy -= 0.25/max(1.,Q.w)*vec2(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w);
	Q.z = m.z-div;
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q = vec4(0,.3,Q.z,1.05);
    if (abs(U.x-0.2*R.x)<20.&&abs(U.y-0.5*R.y)<20.) Q = vec4(v,Q.z,1.05);
    if (iFrame < 1||U.x<2.||R.x-U.x<2.) Q = vec4(.3,0,1,1);
    if (U.y<2.||R.y-U.y<2.) Q = vec4(0,0,Q.z,1);
}