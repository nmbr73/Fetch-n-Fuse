

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// concept for voronoi tracking from user stb
//Render particles
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{	R = iResolution.xy;
    C = P(U);
	vec2 
        n = P(U+vec2(0,1)).xy,
        e = P(U+vec2(1,0)).xy,
        s = P(U-vec2(0,1)).xy,
        w = P(U-vec2(1,0)).xy;
 	float d = (length(n-C.xy)-1.+
        length(e-C.xy)-1.+
        length(s-C.xy)-1.+
        length(w-C.xy)-1.);
 	float m1 = 2.*texture(iChannel2,vec2(abs(0.3*C.w),0.)).x,
 	      m2 = 1.5*texture(iChannel2,vec2(abs(0.3*C.z),0.)).x;
 	float p = smoothstep(2.5,2.,length(C.xy-U));
 	C = 0.5-0.5*sin(.2*(1.+m1)*C.z*vec4(1)+.4*(3.+m2)*C.w*vec4(1,3,5,4));

 	C *= 1.-clamp(.1*d,0.,1.);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Fluid Algorithm  https://www.shadertoy.com/view/MtdBDB
vec2 R;float N;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U0, vec2 U, vec2 U1, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, u = T(V).xy,
         V0 = V - u,
         V1 = V + u;
    float P = T (V0).z, rr = length(r);
    Q.xy -= r*(P-Q.z)/rr/N;
    return (0.5*(length(V0-U0)-length(V1-U1))+P)/N;
}
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 U0 = U - T(U).xy,
         U1 = U + T(U).xy;
 	float P = 0.; Q = T(U0);
 	N = 4.;
    P += X (U0,U,U1,Q, vec2( 1, 0) );
 	P += X (U0,U,U1,Q, vec2( 0,-1) );
 	P += X (U0,U,U1,Q, vec2(-1, 0) );
 	P += X (U0,U,U1,Q, vec2( 0, 1) );
 	Q.z = P;
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy *= 0.;
 	
 	if (length(U-vec2(0.1,0.5)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(.5,-.3);
 	if (length(U-vec2(0.7,0.3)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(-.6,.3);
 	if (length(U-vec2(0.2,0.2)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(.4,.6);
 	if (length(U-vec2(0.7,0.5)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(-.1,-.3);
 	if (length(U-vec2(0.5,0.6)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(0,-.7);
    
 	vec4 mo = texture(iChannel2,vec2(0));
 	float l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 10.) Q.xyz += vec3((10.-l)*(mo.xy-mo.zw)/R.y,(10.-l)*(length(mo.xy-mo.zw)/R.y)*0.02);
 
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Voronoi based particle tracking

vec2 R;float N;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void swap (vec2 U, inout vec4 Q, vec2 u) {
    vec4 p = P(U+u);
    float dl = length(U-Q.xy) - length(U-p.xy);
    float e = .1;
    // allows for probabistic reproduction
    Q = mix(Q,p,0.5+0.5*sign(floor(1e5*dl)));//the value next to dl adjusts the proabability of reproduction
    
    //uncomment and comment the line above to make it not self healing 
    //Q = mix(Q,p,dl>0.?1.:0.);
}
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	// go back through the fluid and test the neighbors
 	//  for the closes particles
 	U = U-T(U).xy;
 	Q = P(U);
 	swap(U,Q,vec2(1,0));
 	swap(U,Q,vec2(0,1));
 	swap(U,Q,vec2(0,-1));
 	swap(U,Q,vec2(-1,0));
 
 	
 	// add color from the jets in the fluid
 	if ((length(Q.xy-vec2(0.1,0.5)*R) < .02*R.y))
        Q.zw = vec2(1,1);
    if ((length(Q.xy-vec2(0.7,0.3)*R) < .02*R.y))
        Q.zw = vec2(3,3);
    if ((length(Q.xy-vec2(0.2,0.2)*R) < .02*R.y))
        Q.zw = vec2(6,5);
 	if (length(Q.xy-vec2(0.7,0.5)*R) < .02*R.y)
        Q.zw = vec2(2,7);
 	if (length(Q.xy-vec2(0.5,0.6)*R) < .02*R.y) 
        Q.zw = vec2(5,4);
 	vec4 mo = texture(iChannel2,vec2(0));
 	if (mo.z > 0. && ln(U,mo.xy,mo.zw) < 10.) Q = vec4(U,1,3.*sin(.4*iTime));
 
 	// advect this particle with the fluid
 	Q.xy = Q.xy + T(Q.xy).xy;
 	if (iFrame < 1) Q = vec4(floor(U/10.+0.5)*10.,0.2,-.1);
}
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
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//Fluid Algorithm  https://www.shadertoy.com/view/MtdBDB
vec2 R;float N;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
float X (vec2 U0, vec2 U, vec2 U1, inout vec4 Q, in vec2 r) {
    vec2 V = U + r, u = T(V).xy,
         V0 = V - u,
         V1 = V + u;
    float P = T (V0).z, rr = length(r);
    Q.xy -= r*(P-Q.z)/rr/N;
    return (0.5*(length(V0-U0)-length(V1-U1))+P)/N;
}
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 U0 = U - T(U).xy,
         U1 = U + T(U).xy;
 	float P = 0.; Q = T(U0);
 	N = 4.;
    P += X (U0,U,U1,Q, vec2( 1, 0) );
 	P += X (U0,U,U1,Q, vec2( 0,-1) );
 	P += X (U0,U,U1,Q, vec2(-1, 0) );
 	P += X (U0,U,U1,Q, vec2( 0, 1) );
 	Q.z = P;
 	if (iFrame < 1) Q = vec4(0);
    if (U.x < 1.||U.y < 1.||R.x-U.x < 1.||R.y-U.y < 1.) Q.xy *= 0.;
 	
 	if (length(U-vec2(0.1,0.5)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(.5,-.3);
 	if (length(U-vec2(0.7,0.3)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(-.6,.3);
 	if (length(U-vec2(0.2,0.2)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(.4,.6);
 	if (length(U-vec2(0.7,0.5)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(-.1,-.3);
 	if (length(U-vec2(0.5,0.6)*R) < .03*R.y) 
        Q.xy= Q.xy*.9+.1*vec2(0,-.7);
    
 	vec4 mo = texture(iChannel2,vec2(0));
 	float l = ln(U,mo.xy,mo.zw);
 	if (mo.z > 0. && l < 10.) Q.xyz += vec3((10.-l)*(mo.xy-mo.zw)/R.y,(10.-l)*(length(mo.xy-mo.zw)/R.y)*0.02);
 
}