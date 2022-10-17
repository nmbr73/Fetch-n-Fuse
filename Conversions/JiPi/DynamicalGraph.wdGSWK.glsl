

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = vec4(0);
    for (int x = -2; x <= 2; x++)
        for (int y = -2; y <= 2; y++) {
        vec4 a = A(U+vec2(x,y));
        vec2 l = ln(U,a.xy,a.zw);
        vec3 r = vec3(length(U-a.xy),length(U-a.zw),length(a.xy-a.zw));
        Q += .5*exp(-.04*r.z)*(exp(-l.x)-.25*exp(-r.x)-.25*exp(-r.y));
     }
	 Q *= 0.5+0.5*cos(B(U).z*3.+vec4(1,2,3,4));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
//Texture lookups :
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

// Distance to line
vec2 ln (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return vec2(length(p-a-(b-a)*i),i);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec2 r) {
    vec4 n = A(U+r);
    if (ln(U,n.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q = n;
    else if (ln(U,Q.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.zw;
    else if (ln(U,Q.xy,n.xy).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.xy;
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
	 X(Q,U,vec2(1,0));
     X(Q,U,vec2(0,1));
     X(Q,U,vec2(0,-1));
     X(Q,U,vec2(-1,0));
     X(Q,U,vec2(2,0));
     X(Q,U,vec2(0,2));
     X(Q,U,vec2(0,-2));
     X(Q,U,vec2(-2,0));
    
    Q.xy += B(Q.xy).xy;
    Q.zw += B(Q.zw).xy;
    
    if (Q.x< 1.) Q.xy = Q.zw;
    if (Q.z< 1.) Q.zw = Q.xy;
    
    if (iMouse.z>0.) {
        vec4 n = iMouse.xyxy;
        if (ln(U,n.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q = n;
    	if (ln(U,Q.xy,n.xy).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.xy;
    }
   
    if (iFrame < 1) {
    	Q = clamp(floor(U/30.+0.5).xyxy*30.,.2*R.xyxy,.8*R.xyxy);
    }
    if (length(Q.xy-Q.zw) > 100.) Q.zw = Q.xy;
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 T (vec2 U) {
	U -= B(U).xy;
    return B(U);
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = T(U);
    vec4 
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    float div = 0.25*(e.x-w.x+n.y-s.y);
    vec2 grad = 0.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy -= grad;
    Q.z  = m.z-div;
    Q.xy = mix(Q.xy,vec2(0.7,0),exp(-.3*length(U-vec2(.3,.4)*R)));
    Q.xy = mix(Q.xy,vec2(-.7,0),exp(-.3*length(U-vec2(.7,.6)*R)));
    if (length(U-vec2(.7,.6)*R) < 10.) Q.xy = vec2(-.7,0);
    if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec2 r) {
    vec4 n = A(U+r);
    if (ln(U,n.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q = n;
    else if (ln(U,Q.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.zw;
    else if (ln(U,Q.xy,n.xy).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.xy;
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = A(U);
	 X(Q,U,vec2(1,0));
     X(Q,U,vec2(0,1));
     X(Q,U,vec2(0,-1));
     X(Q,U,vec2(-1,0));
     X(Q,U,vec2(2,0));
     X(Q,U,vec2(0,2));
     X(Q,U,vec2(0,-2));
     X(Q,U,vec2(-2,0));
    
    Q.xy += B(Q.xy).xy;
    Q.zw += B(Q.zw).xy;
    
    if (Q.x< 1.) Q.xy = Q.zw;
    if (Q.z< 1.) Q.zw = Q.xy;
    
    if (iMouse.z>0.) {
        vec4 n = iMouse.xyxy;
        if (ln(U,n.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q = n;
    	if (ln(U,Q.xy,n.xy).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.xy;
    }
   
    if (iFrame < 1) {
    	Q = clamp(floor(U/30.+0.5).xyxy*30.,.2*R.xyxy,.8*R.xyxy);
    }
    if (length(Q.xy-Q.zw) > 100.) Q.zw = Q.xy;
    
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
vec4 T (vec2 U) {
	U -= B(U).xy;
    return B(U);
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = T(U);
    vec4 
        n = T(U+vec2(0,1)),
        e = T(U+vec2(1,0)),
        s = T(U-vec2(0,1)),
        w = T(U-vec2(1,0)),
        m = 0.25*(n+e+s+w);
    float div = 0.25*(e.x-w.x+n.y-s.y);
    vec2 grad = 0.25*vec2(e.z-w.z,n.z-s.z);
    Q.xy -= grad;
    Q.z  = m.z-div;
    Q.xy = mix(Q.xy,vec2(0.7,0),exp(-.3*length(U-vec2(.3,.4)*R)));
    Q.xy = mix(Q.xy,vec2(-.7,0),exp(-.3*length(U-vec2(.7,.6)*R)));
    if (length(U-vec2(.7,.6)*R) < 10.) Q.xy = vec2(-.7,0);
    if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
}