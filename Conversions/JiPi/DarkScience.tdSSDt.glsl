

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
    U.x = abs(U.x-0.5*R.x);
	Q = T(U).xxxx;  
    Q.yz = T(U+sin(iTime)).xx;
    vec3 n = vec3(
       T(U+vec2(1,0)).x-T(U-vec2(1,0)).x,
       T(U+vec2(0,1)).x-T(U-vec2(0,1)).x,1);
    Q += 10.*(Q.x-0.5)*length(n.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define R iResolution.xy
vec4 F (vec2 U) {return texture(iChannel1,U/R);}
vec4 T (vec2 U) {
    U -= F(U).xy;
    return texture(iChannel0,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{
	Q = T(U);
    #define o 1.5
    
    float a = 1.61803398875*(float(iFrame) + U.x + R.x*U.y),
        si = sin(a),
        co = cos(a);
    mat2 m = mat2(co,-si,si,co);
    
    vec4 
        n = T(U+vec2(0,o)*m),
        e = T(U+vec2(o,0)*m),
        s = T(U-vec2(0,o)*m),
        w = T(U-vec2(o,0)*m),
        mu = 0.25*(n+e+s+w),
        la = mu - Q;
    
    Q += la*vec4(1,.21,0,0);
    
    float x = Q.x*Q.y*(1.-Q.y);
    Q.y += x-.023 - 0.02*Q.y;
    Q.x += -x+.015*(1.-Q.x);
    
    
    Q = max(Q,vec4(0));
    
    if (length (U-0.5*R) < 10.&&iFrame < 3) 
        Q.y = 1.;
    
   
    if (iFrame < 1) {
    	Q = vec4 (1,0,0,0);
    }
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
   #define o 1.2
   vec4 // neighborhood
        n = T(U+vec2(0,o)),
        e = T(U+vec2(o,0)),
        s = T(U-vec2(0,o)),
        w = T(U-vec2(o,0));
   // xy : velocity, z : pressure, w : spin
   C.x -= 0.25*(e.z-w.z+C.w*(n.w-s.w));
   C.y -= 0.25*(n.z-s.z+C.w*(e.w-w.w));
   C.z  = 0.25*((s.y-n.y+w.x-e.x)+(n.z+e.z+s.z+w.z));
   C.w  = 0.25*((n.x-s.x+e.y-w.y)-(n.w+e.w+s.w+w.w));
   float a = A(U).x;
   float an = 1.61803398875*(float(iFrame) + U.x + R.x*U.y),
        si = sin(an),
        co = cos(an);
   mat2 m = mat2(co,-si,si,co);
   C.zy += vec2(0.01,-.002)*A(U).x;
   C.z *= 0.995;
   C.xy += .15*(
       x (a, U, vec2(1,0)*m)+
       x (a, U, vec2(-1,0)*m)+
       x (a, U, vec2(0,1)*m)+
       x (a, U, vec2(0,-1)*m));
   // boundary conditions
    if( U.x < 2. || U.y < 2. || R.x-U.x < 2. || R.y-U.y < 2.)
        C.xy *= 0.;
   vec4 mo = texture(iChannel1,vec2(0));
   float l = ln(U,mo.xy,mo.zw);
   C.xy = mix(C.xy,10.*(mo.xy-mo.zw)/R.y,exp(-.1*l));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// mouse
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{   R = iResolution.xy;
 		vec4 M = iMouse;
 		M.x = abs(M.x-0.5*R.x);
        vec4 p = P(U);
    	if (M.z>0.) {
        	if (p.z>0.) C =  vec4(M.xy,p.xy);
    		else C =  vec4(M.xy,M.xy);
    	}
    	else C = vec4(-iResolution.xy,-iResolution.xy);
}