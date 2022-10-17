

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
	I changed this since I first posted it
	it now advects the velocity which makes it more realistic



	Hello thanks for looking at my shader
	buffer A has the fun stuff
	
	it works by taking a square around the pixel
	and advecting it through the velocity feild
	if the square gets bigger, pressure is decreasing
	and visa-versa

	kind of a contrived way of calculating divergence
	but hey look! it worked! kind of...
	
*/
void mainImage( out vec4 C, in vec2 U )
{
   	vec4 g = texture(iChannel0,U/iResolution.xy);
    vec2 d = vec2(
    	texture(iChannel0,(U+vec2(1,0))/iResolution.xy).w-texture(iChannel0,(U-vec2(1,0))/iResolution.xy).w,
    	texture(iChannel0,(U+vec2(0,1))/iResolution.xy).w-texture(iChannel0,(U-vec2(0,1))/iResolution.xy).w
    );
    vec3 n = normalize(vec3(d,.1));
    float a = acos(dot(n,normalize(vec3(1))))/3.141593;
	C = vec4(
        (.5+0.5*sin(2.*g.w*vec3(1,2,3)))*(0.7+0.5*pow(a,2.))
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
float area (vec2 a, vec2 b, vec2 c) {
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5*(A+B+C);
    return sqrt(s*(s-A)*(s-B)*(s-C));
}
void mainImage( out vec4 Co, in vec2 uu )
{
    U = uu;
    ur = iResolution.xy;
    if (iFrame < 1) {
        // INIT
        float w = 0.5+sin(0.2*U.x)*0.5;
        float q = length(U-0.5*ur);
        Co = vec4(0.1*exp(-0.001*q*q),0,0,w);
    } else {
        
        vec2 v = U,
             A = v + vec2( 1, 1),
             B = v + vec2( 1,-1),
             C = v + vec2(-1, 1),
             D = v + vec2(-1,-1);
        // ADVECT TO LEARN FROM THE PAST
        for (int i = 0; i < 8; i++) {
            v -= t(v).xy;
            A -= t(A).xy;
            B -= t(B).xy;
            C -= t(C).xy;
            D -= t(D).xy;
        }
        vec4 me = t(v,0,0);
        vec4 n = t(v,0,1),
            e = t(v,1,0),
            s = t(v,0,-1),
            w = t(v,-1,0);
        vec4 ne = .25*(n+e+s+w);
		me = mix(t(v),ne,vec4(0.1,0.1,0.9,0.));
        me.z  = me.z  - 0.01*((area(A,B,C)+area(B,C,D))-4.);
		
        // PRESSURE GRADIENT
            vec4 pr = vec4(e.z,w.z,n.z,s.z);
            me.xy = me.xy + 100.*vec2(pr.x-pr.y, pr.z-pr.w)/ur;
        
        // MOUSE MOVEMENT
        	vec4 mouse = texture(iChannel1,vec2(0.5));
            float q = ln(U,mouse.xy,mouse.zw);
            vec2 m = mouse.xy-mouse.zw;
            float l = length(m);
            if (l>0.) m = min(l,10.)*m/l;
            me.xyw += 0.03*exp(-5e-2*q*q*q)*vec3(m,10.);
        Co = me;
        Co.xyz = clamp(Co.xyz, -.4, .4);
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