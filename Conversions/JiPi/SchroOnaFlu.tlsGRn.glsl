

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy
vec4 T (vec2 U) {return texture(iChannel0,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = sin(vec4(1,2,3,4)*length(T(U)));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
vec4 F (vec2 U) {return texture(iChannel2,U/R);}
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T (vec2 U) {
    U -= 0.5*F(U).xy;
    U -= 0.5*F(U).xy;
    return texture(iChannel0,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{
    Q = T(U);
 	if (iFrame < 1) Q = vec4(0);
    #define e  vec2 (1,0)
    float dQ = ((T(U+e.xy)+T(U-e.xy)+T(U+e.yx)+T(U-e.yx)).x/4.-Q.x);
    
    //Schr Eq :
      Q.y += dQ-Q.x;
	  Q.x +=    Q.y;
     /*
		Q.y += dQ;
		Q.x += Q.y;
	 */
    //Wave Eq ^
    
    vec4 mo = texture(iChannel1,vec2(0));
    float l = ln(U,mo.xy,mo.zw);
    if (iMouse.z>1.) Q.x = mix(Q.x,1.,exp(-.1*l));
    if (iFrame < 1 && length(0.5*R-U) < 10.) Q.x = 10.*sin(0.5*U.x);
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Fluid
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 A (vec2 U) {return texture(iChannel2,U/R);}
vec4 t (vec2 U) { // access buffer
	return texture(iChannel0,U/R);
}
vec4 T (vec2 U) {
    // sample things where they were, not where they are
	U -= 0.5*t(U).xy;
	U -= 0.5*t(U).xy;
    return t(U);
}
vec2 x (float a, vec2 U, vec2 r) {
	return r * (A(U+r).x-a);
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = T(U);
   #define o 1.
   vec4 // neighborhood
        n = T(U+vec2(0,o)),
        e = T(U+vec2(o,0)),
        s = T(U-vec2(0,o)),
        w = T(U-vec2(o,0));
   // xy : velocity, z : pressure, w : spin
   C.x -= 0.25*(e.z-w.z+C.w*(n.w-s.w));
   C.y -= 0.25*(n.z-s.z+C.w*(e.w-w.w));
   C.z  = 0.25*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   C.w  = 0.25*((n.x-s.x+e.y-w.y)-C.w);
   
  
   // boundary conditions
   if( U.x < 2. || U.y < 2. || R.x-U.x < 2. || R.y-U.y < 2.)
        C.xy *= 0.;
   vec4 mo = texture(iChannel1,vec2(0));
   float l = ln(U,mo.xy,mo.zw);
   if (iFrame < 1 && length(0.5*R-U) < 10.) C.x = sin(0.5*U.x);
   if (iMouse.z>0.) C.xy = mix(C.xy,10.*(mo.xy-mo.zw)/R.y,exp(-.5*l));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// mouse
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
void mainImage( out vec4 C, in vec2 U )
{   R = iResolution.xy;
 		vec4 M = iMouse;
        vec4 p = T(U);
    	if (M.z>0.) {
        	if (p.z>0.) C =  vec4(M.xy,p.xy);
    		else C =  vec4(M.xy,M.xy);
    	}
    	else C = vec4(-iResolution.xy,-iResolution.xy);
}