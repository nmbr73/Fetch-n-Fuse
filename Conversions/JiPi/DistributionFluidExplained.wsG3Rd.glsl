

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
	A,B,C,and D are the same and have have the fluid code

	Each pixel stores +Energy,-Energy in each direction
	
	Q.xyzw = ( +x,-x,+y,-y )
	
	At each step, the fluid completely exchanges its state
	with its neighborhood. 

	+x goes to the +x cell, -x goes to the -x cell
	+y goes to the +y cell, -y goes to the -y cell

	Then the fluid accelerates
	
	The gradient of pressure polarizes the Energy

	(+x,-x) += dP/dx * (-,+)
	(+y,-y) += dP/dy * (-,+)

*/


// This part makes the lines on the fluid.


#define X 3.
#define L 3.
float time;
vec2 v (vec4 b) {
	return 10.*(vec2(b.x-b.y,b.z-b.w));
}
float ln (vec2 p, vec2 a, vec2 d,float i) {
    float r = clamp(dot(p-a,d)/dot(d,d),0.,1.);
	return length(p-a-d*r);
}
void mainImage( out vec4 Q, vec2 U )
{
    vec2 a = v(T(U));
    Q =  vec4(1);
    for (int x = -3; x <= 3; x++)
    for (int y = -3; y <= 3; y++) {
        vec2 V = floor(U/X+0.5+vec2(x,y))*X;
        V += X*h(V)-.5;
        V += X*sin(10.*h(2.*V).x+vec2(0,3.14/2.)+iTime)-.5;
        a = v(T(V));
        for (float i = 0.; i < L; i++) {
            Q += smoothstep(1.5,0.25,ln(U,V,X*a.xy/L,i));
            V += X*a.xy/L;
            a = v(T(V));
        }   Q += smoothstep(1.5,0.25,ln(U,V,X*a.xy/L,L));
    }
    Q  *= .05+.2*T(U).xzyw;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// FLUID EVOLUTION
// Velocity
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
// Pressure
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
vec4 A(vec2 U) {
    U-=.5*v(T(U));
    U-=.5*v(T(U));
	return T(U);
}
void mainImage( out vec4 Q, in vec2  U)
{
    // THIS PIXEL
    Q = A(U);
    float pq = p(Q);
    // NEIGHBORHOOD
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    // GRADIENT of PRESSURE
    float px = 0.25*(p(e)-p(w));
    float py = 0.25*(p(n)-p(s)); 
    		// boundary Energy exchange in :   
    Q += 0.25*(n.w + e.y + s.z + w.x)
        	// boundary Energy exchange out :
        	-pq
        	// dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
        	-vec4(px,-px,py,-py);
    
    
    // boundary conditions
   	if (iFrame < 1) Q = vec4(.2);
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) {
    	Q = pq+.1*vec4(sin(iTime),-sin(iTime),cos(iTime),-cos(iTime));;
    }
    else if (length(U-vec2(.1,.5)*R)<15.) {
    	Q.xy += 0.01*vec2(1,-1)/(1.+pq);
    }
    if(U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)Q = vec4(p(Q));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define T(U) texture(iChannel0,(U)/R)
vec2 h(vec2 p) // Dave H
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// FLUID EVOLUTION
// Velocity
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
// Pressure
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
vec4 A(vec2 U) {
    U-=.5*v(T(U));
    U-=.5*v(T(U));
	return T(U);
}
void mainImage( out vec4 Q, in vec2  U)
{
    // THIS PIXEL
    Q = A(U);
    float pq = p(Q);
    // NEIGHBORHOOD
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    // GRADIENT of PRESSURE
    float px = 0.25*(p(e)-p(w));
    float py = 0.25*(p(n)-p(s)); 
    		// boundary Energy exchange in :   
    Q += 0.25*(n.w + e.y + s.z + w.x)
        	// boundary Energy exchange out :
        	-pq
        	// dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
        	-vec4(px,-px,py,-py);
    
    
    // boundary conditions
   	if (iFrame < 1) Q = vec4(.2);
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) {
    	Q = pq+.1*vec4(sin(iTime),-sin(iTime),cos(iTime),-cos(iTime));;
    }
    else if (length(U-vec2(.1,.5)*R)<15.) {
    	Q.xy += 0.01*vec2(1,-1)/(1.+pq);
    }
    if(U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)Q = vec4(p(Q));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// FLUID EVOLUTION
// Velocity
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
// Pressure
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
vec4 A(vec2 U) {
    U-=.5*v(T(U));
    U-=.5*v(T(U));
	return T(U);
}
void mainImage( out vec4 Q, in vec2  U)
{
    // THIS PIXEL
    Q = A(U);
    float pq = p(Q);
    // NEIGHBORHOOD
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    // GRADIENT of PRESSURE
    float px = 0.25*(p(e)-p(w));
    float py = 0.25*(p(n)-p(s)); 
    		// boundary Energy exchange in :   
    Q += 0.25*(n.w + e.y + s.z + w.x)
        	// boundary Energy exchange out :
        	-pq
        	// dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
        	-vec4(px,-px,py,-py);
    
    
    // boundary conditions
   	if (iFrame < 1) Q = vec4(.2);
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) {
    	Q = pq+.1*vec4(sin(iTime),-sin(iTime),cos(iTime),-cos(iTime));;
    }
    else if (length(U-vec2(.1,.5)*R)<15.) {
    	Q.xy += 0.01*vec2(1,-1)/(1.+pq);
    }
    if(U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)Q = vec4(p(Q));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// FLUID EVOLUTION
// Velocity
vec2 v (vec4 b) {
	return vec2(b.x-b.y,b.z-b.w);
}
// Pressure
float p (vec4 b) {
	return 0.25*(b.x+b.y+b.z+b.w);
}
// TRANSLATE COORD BY Velocity THEN LOOKUP STATE
vec4 A(vec2 U) {
    U-=.5*v(T(U));
    U-=.5*v(T(U));
	return T(U);
}
void mainImage( out vec4 Q, in vec2  U)
{
    // THIS PIXEL
    Q = A(U);
    float pq = p(Q);
    // NEIGHBORHOOD
    vec4 
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    // GRADIENT of PRESSURE
    float px = 0.25*(p(e)-p(w));
    float py = 0.25*(p(n)-p(s)); 
    		// boundary Energy exchange in :   
    Q += 0.25*(n.w + e.y + s.z + w.x)
        	// boundary Energy exchange out :
        	-pq
        	// dV/dt = dP/dx,  dEnergy In dTime = dEnergy in dSpace
        	-vec4(px,-px,py,-py);
    
    
    // boundary conditions
   	if (iFrame < 1) Q = vec4(.2);
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) {
    	Q = pq+.1*vec4(sin(iTime),-sin(iTime),cos(iTime),-cos(iTime));;
    }
    else if (length(U-vec2(.1,.5)*R)<15.) {
    	Q.xy += 0.01*vec2(1,-1)/(1.+pq);
    }
    if(U.x<3.||R.x-U.x<3.||U.y<3.||R.y-U.y<3.)Q = vec4(p(Q));
}