

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// IMAGE
vec2 R;
vec3 E (vec3 U) {
	return texture(iChannel0,U.xy/R).xyz;
}
vec3 M (vec3 U) {
	return texture(iChannel1,U.xy/R).xyz;
}

vec3 X (vec3 U, vec3 R) {
	return cross(R,E(U+R));
}
void mainImage( out vec4 c, in vec2 u )
{	R = iResolution.xy;
 
 	vec3 U = vec3(u,0);
 	vec3 e = E(U);
 	vec3 m = M(U);
 
 	float a = atan(e.y,e.x);
 	vec3 cr = cross(e,m);
 	vec3 C = 0.5+0.5*sin(10.*cr.x*vec3(1,2,3));
 	c = vec4(C,1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// ELECTRIC FIELD
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec3 E (vec3 U) {
	return texture(iChannel0,U.xy/R).xyz;
}
vec3 M (vec3 U) {
	return texture(iChannel1,U.xy/R).xyz;
}

vec3 X (vec3 U, vec3 R) {
	return cross(R,M(U+R));
}

void mainImage( out vec4 c, in vec2 u )
{	R = iResolution.xy;
 
 	vec3 U = vec3(u,0);
 	# define l inversesqrt (2.)
 	vec3 mu = 0.25*(E(U+vec3(1,0,0))+E(U-vec3(1,0,0))+E(U+vec3(0,1,0))+E(U-vec3(0,1,0)));
    vec3 C = mix(E(U),mu,dx)
        + dt*(- M(U) + 
      ( X(U, vec3( 1, 0,0)) + 
        X(U, vec3(-1, 0,0)) + 
        X(U, vec3( 0, 1,0)) + 
        X(U, vec3( 0,-1,0)) + 
        X(U, vec3( l, l,0)) + 
        X(U, vec3( l,-l,0)) +
        X(U, vec3(-l,-l,0)) +
        X(U, vec3(-l, l,0)) ) / 8.);
 	
 	if (iFrame < 1) C = 1e-1*vec3(-1.+2.*smoothstep(-10.,10.,U.x-0.5*R.x));
 	vec4 mouse = texture(iChannel2,vec2(.5));
 	if (iMouse.z > 0. ) C = mix(C,normalize(vec3(mouse.xy-mouse.zw,1)),smoothstep(7.,0.,ln(U.xy,mouse.xy,mouse.zw)));
 	c = vec4(C,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// MAGNETIC FIELD
vec2 R;
vec3 E (vec3 U) {
	return texture(iChannel0,U.xy/R).xyz;
}
vec3 M (vec3 U) {
	return texture(iChannel1,U.xy/R).xyz;
}

vec3 X (vec3 U, vec3 R) {
	return cross(R,E(U+R));
}
void mainImage( out vec4 c, in vec2 u )
{	R = iResolution.xy;
 
 	vec3 U = vec3(u,0);
 	# define l inversesqrt (2.)
 	vec3 mu = 0.25*(M(U+vec3(1,0,0))+M(U-vec3(1,0,0))+M(U+vec3(0,1,0))+M(U-vec3(0,1,0)));
    vec3 C = mix(M(U),mu,dx) + dt*( E(U) -
      ( X(U, vec3( 1, 0,0)) + 
        X(U, vec3(-1, 0,0)) + 
        X(U, vec3( 0, 1,0)) + 
        X(U, vec3( 0,-1,0)) + 
        X(U, vec3( l, l,0)) + 
        X(U, vec3( l,-l,0)) +
        X(U, vec3(-l,-l,0)) +
        X(U, vec3(-l, l,0)) ) / 8.);
 	if (iFrame < 1) C = vec3(0);
 	c = vec4(C,0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// these values are improperly named dt = d/dt and dx = dispersion
#define dt 1.
#define dx 0.001
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// keep track of mouse
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 p = texture(iChannel0,fragCoord/iResolution.xy);
    if (iMouse.z>0.) {
        if (p.z>0.) fragColor =  vec4(iMouse.xy,p.xy);
    	else fragColor =  vec4(iMouse.xy,iMouse.xy);
    }
    else fragColor = vec4(-iResolution.xy,-iResolution.xy);
}