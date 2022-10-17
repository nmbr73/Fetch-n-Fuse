

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Line Tracking Fluid" by wyatt. https://shadertoy.com/view/tsKXzd
// 2020-12-10 19:40:02

void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U);
    vec4 d = D(U);
    float l = ln(U,a.xy,a.zw);
    Q = (.8+.2*d.xxxx)*smoothstep(0.,1.,l);
    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define o vec3(1,0,-1)
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, vec2 U)
float ln (vec2 p, vec2 a, vec2 b) {
	return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,.9));
}
#define norm(u) ((u)/(1e-9+length(u)))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec2 r) {
    vec4 n = A(U+r);
	if (ln(U,n.xy,n.zw)<ln(U,Q.xy,Q.zw)) Q = n;
}
Main {
    Q = A(U);
    for (int x = -2;x <=2; x++)
    for (int y = -2;y <=2; y++)
    X(Q,U,vec2(x,y));
    Q.xy = mix(Q.xy,A(Q.xy).xy,.3);
    Q.zw = mix(Q.zw,A(Q.zw).zw,.05);
    Q.xy += D(Q.xy).xy;
    Q.zw += D(Q.zw).xy;
    
    if (length(Q.xy-Q.zw) > 2.5) {
        vec2 m = 0.5*(Q.xy+Q.zw);
        if (length(U-Q.xy) > length(U-Q.zw)) 
        	Q.xy = m;
        else Q.zw = m;
    }
    if (iMouse.z>0.) {
        vec4 n = B(vec2(0));
    	if (ln(U,n.xy,n.zw)<ln(U,Q.xy,Q.zw)) Q = n;
    }
    if (iFrame<1) {
        Q = vec4(0.7*R,0.3*R);
        vec4 a =vec4(vec2(0.3,.7)*R,vec2(.7,.3)*R);
        if (ln(U,a.xy,a.zw)<ln(U,Q.xy,Q.zw))
            Q = a;
    }

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Mouse
void mainImage( out vec4 C, in vec2 U )
{
    vec4 p = texture(iChannel0,U/iResolution.xy);
   	if (iMouse.z>0.) {
      if (p.z>0.) C =  vec4(iMouse.xy,p.xy);
    else C =  vec4(iMouse.xy,iMouse.xy);
   }
    else C = vec4(-iResolution.xy,-iResolution.xy);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 T(vec2 U) {
	U -= .5*D(U).xy;
	U -= .5*D(U).xy;
    return D(U);
}
Main {
    Q = T(U);
    vec4 
        n = T(U+o.yx),
        e = T(U+o.xy),
        s = T(U+o.yz),
        w = T(U+o.zy),
        m = 0.25*(n+e+s+w);
    Q.xy = m.xy-0.25*vec2(e.z-w.z,n.z-s.z);
	Q.z = Q.z-0.25*(n.y+e.x-s.y-w.x);
    vec4 a = A(U);
    float l = ln(U,a.xy,a.zw);
    float v = smoothstep(1.,0.,l);
    Q.z += 0.01*v;
    Q.xy = mix(Q.xy,norm(a.xy-a.zw),.1*v);
    Q.xy *= .99-.5*v;
    if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.) Q.xy *= 0.;
	if (iFrame < 1) Q = vec4(0,0,0,0);
}