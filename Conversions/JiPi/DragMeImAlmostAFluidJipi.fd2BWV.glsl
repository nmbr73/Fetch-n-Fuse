

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*

	So a litte bit ago I made this:	
		https://www.shadertoy.com/view/lsVfRd

	but I forgiot the convective term...

	I didnt realize the velocity advects through itself
	kinda trippy tbh

	fluids are like multi-dimensional infinitesimal newton's cradles
	
	


*/
void mainImage( out vec4 C, in vec2 U )
{
   	vec4 g = texture(iChannel0,U/iResolution.xy);
    vec2 d = vec2(
    	texture(iChannel0,(U+vec2(1,0))/iResolution.xy).w-texture(iChannel0,(U-vec2(1,0))/iResolution.xy).w,
    	texture(iChannel0,(U+vec2(0,1))/iResolution.xy).w-texture(iChannel0,(U-vec2(0,1))/iResolution.xy).w
    );
    vec3 n = normalize(vec3(d,0.1));
    float a = acos(dot(n,normalize(vec3(1))))/3.141593;
	g.w = 2.*sqrt(g.w);
    vec3 color = 
        1.3*(.5+0.5*sin(abs(g.xyz)*vec3(20,20,1)+2.*g.w*vec3(sin(g.w),cos(g.w*2.),3)))*(abs(a)*.5+0.5);
    C = vec4(
        color*color*1.2
       ,1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5*ur);
        // make a small right pointing velocity in the middle
        Co = vec4(0.1*exp(-0.01*q*q),0,0,2.*exp(-0.01*q*q));
    } else {
        // start where the pixel is and make a box around it
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= t(v).xy;
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        //average the neighbors to allow values to blend
        vec4 ne = .25*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
		me = mix(t(v),ne,vec4(0.06,0.06,1.,0.));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.);
		
        // PRESSURE GRADIENT
            vec4 pr = vec4(e.z,w.z,n.z,s.z);
        	// add the pressure gradient to the velocity
            me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        // MOUSE MOVEMENT
        	vec4 mouse = texture(iChannel1,vec2(0.5));
            float q = ln(U,mouse.xy,mouse.zw);
            vec2 m = mouse.xy-mouse.zw;
            float l = length(m);
            if (l>0.) m = min(l,10.)*m/l;
        	// add a line from the mouse to the velocity field and add some color
            me.xyw += 0.03*exp(-6e-2*q*q*q)*vec3(m,20.);
        Co = me;
        Co.xyz = clamp(Co.xyz, -.6, .6);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// MOUSE
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 p = texture(iChannel0,fragCoord/iResolution.xy);
    if (iMouse.z>0.) {
        if (p.z>0.) fragColor =  vec4(iMouse.xy,p.xy);
    	else fragColor =  vec4(iMouse.xy,iMouse.xy);
    }
    else fragColor = vec4(-iResolution.xy,-iResolution.xy);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5*ur);
        // make a small right pointing velocity in the middle
        Co = vec4(0.1*exp(-0.01*q*q),0,0,2.*exp(-0.01*q*q));
    } else {
        // start where the pixel is and make a box around it
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= t(v).xy;
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        //average the neighbors to allow values to blend
        vec4 ne = .25*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
		me = mix(t(v),ne,vec4(0.06,0.06,1.,0.));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.);
		
        // PRESSURE GRADIENT
            vec4 pr = vec4(e.z,w.z,n.z,s.z);
        	// add the pressure gradient to the velocity
            me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        // MOUSE MOVEMENT
        	vec4 mouse = texture(iChannel1,vec2(0.5));
            float q = ln(U,mouse.xy,mouse.zw);
            vec2 m = mouse.xy-mouse.zw;
            float l = length(m);
            if (l>0.) m = min(l,10.)*m/l;
        	// add a line from the mouse to the velocity field and add some color
            me.xyw += 0.03*exp(-6e-2*q*q*q)*vec3(m,20.);
        Co = me;
        Co.xyz = clamp(Co.xyz, -.6, .6);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// FLUID PART

vec2 ur, U;
float ln (vec2 p, vec2 a, vec2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec4 t (vec2 v, int a, int b) {return texture(iChannel0,fract((v+vec2(a,b))/ur));}
vec4 t (vec2 v) {return texture(iChannel0,fract(v/ur));}
float area (vec2 a, vec2 b, vec2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1) {
        // INIT
        float q = length(U-0.5*ur);
        // make a small right pointing velocity in the middle
        Co = vec4(0.1*exp(-0.01*q*q),0,0,2.*exp(-0.01*q*q));
    } else {
        // start where the pixel is and make a box around it
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 5; i++) {
            // add the velocity at each position to the position
            v -= t(v).xy;
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        // find out where and what the pixel is now and what its neighbors were doing last frame
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        //average the neighbors to allow values to blend
        vec4 ne = .25*(n+e+s+w);
        // mix the velocity and pressure from neighboring cells
		me = mix(t(v),ne,vec4(0.06,0.06,1.,0.));
        // add the change in the area of the advected box to the pressure
        me.z  = me.z  - ((area(A,B,C)+area(B,C,D))-4.);
		
        // PRESSURE GRADIENT
            vec4 pr = vec4(e.z,w.z,n.z,s.z);
        	// add the pressure gradient to the velocity
            me.xy = me.xy + vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        // MOUSE MOVEMENT
        	vec4 mouse = texture(iChannel1,vec2(0.5));
            float q = ln(U,mouse.xy,mouse.zw);
            vec2 m = mouse.xy-mouse.zw;
            float l = length(m);
            if (l>0.) m = min(l,10.)*m/l;
        	// add a line from the mouse to the velocity field and add some color
            me.xyw += 0.03*exp(-6e-2*q*q*q)*vec3(m,20.);
        Co = me;
        Co.xyz = clamp(Co.xyz, -.6, .6);
    }
}