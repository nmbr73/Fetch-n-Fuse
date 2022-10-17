

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    Q = B(U).z*C(U+2.).zzzz;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
//Texture lookups :
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Main void mainImage( out vec4 Q, in vec2 U )
#define N 7.
#define For for (float i = -(N); i<=(N); i++)
#define S 2.
#define Gaussian(i) 0.3989422804/S*exp(-.5*(i)*(i)/S/S)
#define W 25.

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
}
void Xr (inout vec4 Q, vec2 U, float r) {
	 X(Q,U,vec2(r,0));
     X(Q,U,vec2(0,r));
     X(Q,U,vec2(0,-r));
     X(Q,U,vec2(-r,0));
}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = vec4(0);
	Xr(Q,U,1.); 
	Xr(Q,U,2.); 
	Xr(Q,U,3.); 
    
    Q.xy += B(Q.xy).xy;
    Q.zw += B(Q.zw).xy;
    
    if (length(U-Q.zw)<length(U-Q.xy)) {
    	vec2 u = Q.xy;
        Q.xy = Q.zw;
        Q.zw = u;
    }
    
    if (Q.x< 1.) Q.xy = Q.zw;
    if (Q.z< 1.) Q.zw = Q.xy;
    
    if (iMouse.z>0.) {
        vec4 n = iMouse.xyxy;
        if (ln(U,n.xy,n.zw).x<ln(U,Q.xy,Q.zw).x) Q = n;
    	if (ln(U,Q.xy,n.xy).x<ln(U,Q.xy,Q.zw).x) Q.zw = n.xy;
    }
   
    if (iFrame < 1) {
        if (length(U-0.5*R)<55.)
    	Q = floor(U/10.+0.5).xyxy*10.;
    }
    if (length(Q.xy-Q.zw) > W) Q.zw = Q.xy;
    
    if (iFrame%40==0) if (length(U-0.5*R+7.*sin(float(iFrame)))<length(U-Q.xy)) Q.xy = 0.5*R; 
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 Q, in vec2 U )
{
    vec4 a = A(U), b = B(U);
	vec2 u = a.xy-a.zw;
    float r = length(u);
    vec2 l = ln(U,a.xy,a.zw);
	if (r>1.) Q.xy =  250.*u/r/r*smoothstep(3.,1.,l.x)*exp(-5.*l.y);
	else Q.xy = vec2(0);
    
    Q.zw = vec2(1);
    Q *= smoothstep(1.,2.,length(U-a.xy));
    Q *= smoothstep(0.,1.,ln(U,a.xy,a.zw).x);
	Q.zw = mix(Q.zw,b.zw,0.7);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main
{
    Q = vec4(0);
    For Q += Gaussian(i) * A(U+vec2(0,i));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main
{
    Q = vec4(0);
    For Q += Gaussian(i) * A(U+vec2(i,0));

}