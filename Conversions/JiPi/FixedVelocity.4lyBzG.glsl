

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T (vec2 U) {return texture(iChannel0,U/R);}
vec4 D (vec2 U) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{   R = iResolution.xy;
 	vec4 i = D(U),
         t = T(U);
 	vec2 d = vec2(
    	D(U+vec2(1,0)).x-D(U-vec2(1,0)).x,
    	D(U+vec2(0,1)).x-D(U-vec2(0,1)).x
    );
 	C = abs(sin(.2*sqrt(i)*vec4(1.,1.3,1.5,4)));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 R;float N;
float hash(vec2 p)
{ // Dave H
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U0, vec2 U, vec2 U1, inout vec4 Q, in vec2 r) {
    vec2 V = U + r;
    vec4 t = T(V);
    vec2 V0 = V - t.xy,
         V1 = V + t.xy;
    float P = t.z, rr = length(r);
    Q.xy -= r*(P-Q.z)/rr/N;
    return (0.5*(length(V0-U0)-length(V1-U1))+P)/N;
}

void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 U0 = U - T(U).xy,
         U1 = U + T(U).xy;
 	float P = 0.; Q = T(U0);
 if (length(Q.xy)==0.||iFrame < 1) {
     	float h = 6.3*hash(U);
     	Q = vec4(0.4*vec2(cos(h),sin(h)),0,0);
        	
 } else {
 	N = 4.;;
    P += X (U0,U,U1,Q, vec2( 1, 0));
 	P += X (U0,U,U1,Q, vec2( 0,-1));
 	P += X (U0,U,U1,Q, vec2(-1, 0));
 	P += X (U0,U,U1,Q, vec2( 0, 1));
 	Q.z = P;
 	Q.xy=mix(Q.xy,0.4*normalize(Q.xy),0.01);
 	vec4 mo = texture(iChannel2,vec2(0));
 	float l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 10.) Q.z -= .1*(10.-l);
 
 }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Voronoi based particle tracking

vec2 R;float N;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void swap (vec2 U, inout vec4 Q, vec2 u) {
    vec4 p = P(U+u);
    float dl = length(U-Q.xy) - length(U-p.xy);
    float e = .1;
    // allows for probabistic reproduction
    Q = mix(Q,p,0.5+0.5*sign(floor(1e3*dl+0.5)));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	U = U-2.*T(U).xy;
 	U = U-2.*T(U).xy;
 	Q = P(U);
 	swap(U,Q,vec2(1,0));
 	swap(U,Q,vec2(0,1));
 	swap(U,Q,vec2(0,-1));
 	swap(U,Q,vec2(-1,0));
 	Q.xy = Q.xy + 2.*T(Q.xy).xy;
 	Q.xy = Q.xy + 2.*T(Q.xy).xy;
 	if (Q.z == 0.) Q = vec4(floor(U/10.)*10.,U);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//Render particles
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
vec4 D ( vec2 U ) {return texture(iChannel2,U/R);}
void mainImage( out vec4 C, in vec2 U )
{	R = iResolution.xy;
    C = P(U);
 	C = vec4(vec3(smoothstep(1.5,0.5,length(C.xy-U))),1);
 	C = C+vec4(0.995,0.98,0.95,1.)*(D(U));
 	if(iFrame < 1) C = vec4(0);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
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