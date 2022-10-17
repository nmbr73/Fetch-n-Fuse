

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, vec2 U )
{
    vec4 a = A(U);
    Q = .7-.03*B(U)+3.*abs(hash(a.z))*smoothstep(2.,0.,abs(length(U-a.xy)-a.w));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void swap (inout vec4 Q, vec2 U, vec2 r) {
	vec4 n = A(U+r);
    if (abs(length(n.xy-U)-n.w)<abs(length(Q.xy-U)-Q.w)) Q = n;
}
void mainImage( out vec4 Q, vec2 U )
{
    
    Q = A(U);
    swap(Q,U, vec2(0,1));
    swap(Q,U, vec2(1,0));
    swap(Q,U,-vec2(0,1));
    swap(Q,U,-vec2(1,0));
    swap(Q,U, vec2(1,1));
    swap(Q,U, vec2(1,-1));
    swap(Q,U,-vec2(1,1));
    swap(Q,U,-vec2(1,-1));
    swap(Q,U, vec2(4,4));
    swap(Q,U, vec2(-4,4));
    swap(Q,U,-vec2(4,4));
    swap(Q,U,-vec2(-4,4));
    vec4 
        id = hash(Q.z);
    vec4
        n = B(Q.xy+vec2(0,1)),
        e = B(Q.xy+vec2(1,0)),
        s = B(Q.xy-vec2(0,1)),
        w = B(Q.xy-vec2(1,0));
    vec2 
        x = vec2(e.x-w.x,n.x-s.x),
        y = vec2(e.y-w.y,n.y-s.y),
        z = vec2(e.z-w.z,n.z-s.z),
        a = vec2(e.w-w.w,n.w-s.w);
    Q.xy -= id.x*x+id.y*y+id.z*z+id.w*a;
    if (iMouse.z>0.&&length(U-iMouse.xy) < 10.) {
        vec2 u = floor(U/5.+0.5)*5.;
        vec4 n = vec4(
        	u,
            u.x+R.x*u.y,1.+5.*hash(u.y+R.y*u.x).x
        );
        if (abs(length(n.xy-U)-n.w)<abs(length(Q.xy-U)-Q.w)) Q = n;
    }
    if (iFrame < 1) {
        vec2 u = floor(U/20.+0.5)*20.;
        float h = hash(u.y+R.y*u.x).x;
    	Q = vec4(
        	u,
            u.x+R.x*u.y,1.+20.*h*h
        );
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, vec2 U )
{
    
    Q = B(U)*0.5;
    
    for (float i = -I; i < I; i++) {
        vec2 x = U+2.*vec2(i,0); 
        vec4 a = A(x);
        float r = smoothstep(1.,.5,abs(length(a.xy-x)-a.w));
    	Q += O*exp(-M*i*i)*r*hash(a.z);
    }
    
    if (iFrame < 1) {
    	Q = vec4(0);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 Q, vec2 U )
{
    
    Q = B(U)*0.5;
    
    for (float i = -I; i < I; i++) {
        vec2 x = U+2.*vec2(0,i); 
        vec4 a = A(x);
    	Q += exp(-M*i*i)*a;
    }
    
    if (iFrame < 1) {
    	Q = vec4(0);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define M .01*vec4(1,2,4,8)
#define I 15.
#define O .5*sqrt(vec4(1,2,4,8))
vec4 hash (float p) // Dave (Hash)kins
{
	vec4 p4 = fract(vec4(p) * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+19.19);
    return floor(fract((p4.xxyz+p4.yzzw)*p4.zywx)*10.)/10.-.25;
    
}
#define R iResolution.xy
#define A(U) texture(iChannel0, (U)/R)
#define B(U) texture(iChannel1, (U)/R)