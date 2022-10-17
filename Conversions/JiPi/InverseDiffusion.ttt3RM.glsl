

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
mat2 r (float a) {
    vec2 e = vec2(cos(a),sin(a));
	return mat2(e.x,-e.y,e.y,e.x);
}
void mainImage( out vec4 Q, in vec2 U )
{
    vec3 
        p = vec3(0.5*R,.62*R.y),
        d = normalize(vec3((U-0.5*R)/R.y,-1)),
        o = vec3(0.5,.1,.5)*R.xyy;
    if (iMouse.z>0.) o.xy = iMouse.xy;
    mat2 m = r(.44);
    p.y -= .19*R.y;
    d.yz *= m;
    p.yz *= m;
    for (int i = 0; i<30; i++){ 
    	p += .2*d*(p.z-4.*A(p.xy).z);
    }
    d = normalize(o-p);
    float z = A(p.xy).z;
    vec3 n = normalize(vec3(B(p.xy).xy,-.25));
    vec3 q = d;
    p += .1*d;
    for (int i = 0; i<30; i++){ 
    	p += .5*d*min(p.z-4.*A(p.xy).z,length(p-o)-1.);
    }
    Q = (exp(-length(p-o)+1.))*(cos(-.1*iTime+.1*z+.5*vec4(1,2,3,4)))*.5*(dot(reflect(n,d),q)-dot(n,d));
	Q*=Q;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
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
        n = B(U+vec2(0,1)),
        e = B(U+vec2(1,0)),
        s = B(U-vec2(0,1)),
        w = B(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    
    float d = 0.25*(n.y-s.y+e.x-w.x);
    float c = 0.25*(n.x-s.x-e.y+w.y);
    
    Q.z = m.z*.999 - mix(d,c,length(U-0.5*R)/R.y);
    Q.w = d;
    if (iFrame < 1) Q = vec4(sin(U.x)*cos(U.y));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    Q.xy = 0.25*vec2(e.z-w.z,n.z-s.z);
    if (length(Q.xy)>0.) Q.xy = mix(Q.xy,normalize(Q.xy),.2);
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
    vec4 
        n = B(U+vec2(0,1)),
        e = B(U+vec2(1,0)),
        s = B(U-vec2(0,1)),
        w = B(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    
    float d = 0.25*(n.y-s.y+e.x-w.x);
    float c = 0.25*(n.x-s.x-e.y+w.y);
    
    Q.z = m.z*.999 - mix(d,c,.2);
    
    if (iFrame < 1) Q = vec4(sin(U.x)*cos(U.y));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec2 c = 0.5*R;
    if (iMouse.z>0.) c = iMouse.xy;
    
    U -= c;
    U *= .99;
    U += c;
    Q = A(U);
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    Q.xy = 0.25*vec2(e.z-w.z,n.z-s.z);
    if (length(Q.xy)>0.) Q.xy = mix(Q.xy,normalize(Q.xy),.2);
    
}