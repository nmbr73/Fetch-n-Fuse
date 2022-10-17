

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 
        a = A(U);
    Q = 4.*vec4(-a.z,.1*abs(a.z),1.5*a.z,1);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
mat2 r (float a) {
    vec2 e = vec2(cos(a),sin(a));
	return mat2(e.x,-e.y,e.y,e.x);
}
vec2 mirror (vec2 u) {
    if (u.x>1.) u.x = 1.-fract(u.x);
    if (u.x<0.) u.x = fract(u.x);
    if (u.y>1.) u.x = 1.-fract(u.y);
    if (u.y<0.) u.x = fract(u.y);
	return u;
}
#define A(U) texture(iChannel0,mirror((U)/R))
#define B(U) texture(iChannel1,mirror((U)/R))
#define C(U) texture(iChannel2,mirror((U)/R))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    
    Q = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    
    float d = 0.25*(n.y-s.y+e.x-w.x);
    float c = 0.25*(n.x-s.x-e.y+w.y);
    vec2 g = 0.25*vec2(e.z-w.z,n.z-s.z);
    Q.z = mix(m.z,d,.4);
    Q.xy = mix(Q.xy,m.xy,.3);
    Q.xy -= g;
    if (length(Q.xy)>0.) Q.xy = mix(Q.xy,normalize(Q.xy),1.);
    
    if (iFrame < 1) Q = vec4(sin(.1*U.y),cos(.1*U.x),0,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec2 p = 0.5*R+10.*vec2(cos(iTime),sin(iTime));
    if (iMouse.z>0.) p = iMouse.xy;
    
    U -= p;
    U *= (1.-5e-3*(1.-length(U)/R.x))*r(-.001);
    U += p;
    Q = A(U);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    
    Q = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    
    float d = 0.25*(n.y-s.y+e.x-w.x);
    float c = 0.25*(n.x-s.x-e.y+w.y);
    vec2 g = 0.25*vec2(e.z-w.z,n.z-s.z);
    Q.z = mix(m.z,d,.4);
    Q.xy = mix(Q.xy,m.xy,.3);
    Q.xy -= g;
    if (length(Q.xy)>0.) Q.xy = mix(Q.xy,normalize(Q.xy),1.);
    
    if (iFrame < 1) Q = vec4(sin(.1*U.y),cos(.1*U.x),0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec2 p = 0.5*R+10.*vec2(cos(iTime),sin(iTime));
    if (iMouse.z>0.) p = iMouse.xy;
    
    U -= p;
    U *= (1.-5e-3*(1.-length(U)/R.x))*r(-.001);
    U += p;
    Q = A(U);
}