

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define PI 			3.14159265359
#define SPHERE 		vec4 (0.0, 0.0, 0.0, 2.0)
#define FOV 		60.0

#define RI_AIR		1.000293
#define RI_SPH		1.55

#define ETA 		(RI_AIR/RI_SPH)
#define R			-0.02

#define FR_BIAS		0.0
#define FR_SCALE	1.0
#define FR_POWER	0.7

#define FR0			vec3 (0.0, 1.0, 0.7)

#define PI 3.14159265359

float noise (vec2 co) {
  return length (texture (iChannel2, co));
}

mat2 rotate (float fi) {
	float cfi = cos (fi);
	float sfi = sin (fi);
	return mat2 (-sfi, cfi, cfi, sfi);
}

vec3 hsv2rgb (vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float fbm ( vec2 uv) {
	return (
		+noise (uv*2.0)/2.0
		+noise (uv*4.0)/4.0
		+noise (uv*8.0)/8.0
		+noise (uv*16.0)/16.0
		+noise (uv*32.0)/32.0
	);
}

vec4 compute (vec2 uv, float iTime) {	
	uv *= rotate (PI * 0.5 * fbm (uv/256.0) * length (uv) + iTime);
	uv = (iTime+uv)/196.0;
	vec3 col = vec3 (fbm (uv)*PI*2.0, 1.0, 1.0);	
	return vec4 (hsv2rgb (col),1.0) ;
}
			  
mat3 rotate_x (float fi) {
	float cfi = cos (fi);
	float sfi = sin (fi);
	return mat3 (
		1.0, 0.0, 0.0,
		0.0, cfi, -sfi,
		0.0, sfi, cfi);
}

mat3 rotate_y (float fi) {
	float cfi = cos (fi);
	float sfi = sin (fi);
	return mat3 (
		cfi, 0.0, sfi,
		0.0, 1.0, 0.0,
		-sfi, 0.0, cfi);
}

mat3 rotate_z (float fi) {
	float cfi = cos (fi);
	float sfi = sin (fi);
	return mat3 (
		cfi, -sfi, 0.0,
		sfi, cfi, 0.0,
		0.0, 0.0, 1.0);
}

vec4 noise4v (vec2 p) {
	return texture (iChannel2, p);
}

vec4 fbm4v (vec2 p) {
	vec4 f = vec4 (0.0);
	f += 0.5000 * noise4v (p); p *= 2.01;
	f += 0.2500 * noise4v (p); p *= 2.02;
	f += 0.1250 * noise4v (p); p *= 2.03;
	f += 0.0625 * noise4v (p); p *= 2.04;
	f /= 0.9375;
	return f;
}

vec4 fbm3d4v (vec3 p, float s) {	
	return 
		compute (p.xy, iTime/s) * abs (p.z) +
		compute (p.xz, iTime/s) * abs (p.y) +
		compute (p.yz, iTime/s) * abs (p.x);
}

float sphere_intersect (in vec3 o, in vec3 d, in vec4 c, out float t0, out float t1) {
	vec3 oc = o - c.xyz;
	float A = dot (d, d);
	float B = 2.0 * dot (oc, d);
	float C = dot (oc, oc) - c.w;
	float D = B*B - 4.0*A*C;
	float q = (-B - sqrt (D) * sign (B))/2.0;
	float _t0 = q/A;
	float _t1 = C/q;
	t0 = min (_t0, _t1);
	t1 = max (_t0, _t1);
	return step (0.0, D);
}

float fresnel_step (vec3 I, vec3 N, vec3 f) {
	return clamp (f.x + f.y * pow (1.0 + dot (I, N), f.z), 0.0, 1.0);
}

vec2 to_spherical_normalized (vec3 pt) {
	float r = length (pt);
	return vec2 (acos (pt.z / r)/PI, atan (pt.y, pt.x)/PI + 0.5); 
}

vec3 spherical (vec3 cart) {
	float r = length (cart);
	float i = (acos (cart.z/r)/(PI/2.0) - 0.5)*2.0;
	float a = atan (cart.y, cart.x)/PI;
	return vec3 (r, a, i);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = (2.0*fragCoord.xy - iResolution.xy)/min (iResolution.x, iResolution.y) * tan (radians (FOV)/2.0);
	vec2 mo = PI * iMouse.xy / iResolution.xy;
	
	vec3 up = vec3 (0.0, 1.0, 0.0); 			// up 
	vec3 fw = vec3 (0.0, 0.0, 1.0) * 			// forward
		rotate_y (mo.x * PI); 				
	vec3 lf = cross (up, fw); 					// left
	
	vec3 ro = -fw * 5.0; 						// ray origin
	vec3 rd = normalize (uv.x * lf + uv.y * up + fw) ; 		// ray direction
	vec3 rn = rd;
	vec3 dr = fbm4v (uv/64.0 + sin (iTime/128.0)).xyz - 0.5;
	//rd = normalize (rd + dr/32.0);
	
	vec4 sp = SPHERE + 					
		vec4 (0.0, 1.0, 0.0, 0.0)*sin (iTime); 							
	
	float t0 = 0.0, t1 = 0.0;					// sphere intersection points
	
	float d = sphere_intersect (				// initial intersection
		ro, rd, sp, t0, t1); 
	
	vec4 color = texture (iChannel0, rn);
	
	if (d > 0.0) {
		vec3 pt0 = ro + rd*t0;
		vec3 pt1 = ro + rd*t1;
		vec3 pn0 = normalize (pt0 - sp.xyz);	
		vec3 pn1 = normalize (sp.xyz - pt1);
		
		
		vec3 r0 = reflect (rd, pn0);			
		vec3 r1 = reflect (rd, pn1);
		
		
		
		//vec4 s0 = fbm3d4v (normalize (pn0), 8.0);
		//vec4 s1 = fbm3d4v (normalize (pn1), 8.0);
		vec4 s0 = compute (spherical (pn0.zxy).yz/2.0, iTime/8.0);
		vec4 s1 = compute (spherical (pn1.zxy).yz/2.0, iTime/8.0);
		vec4 c0 = texture (iChannel1, r0);
		vec4 c1 = texture (iChannel1, r1);		
		
		color = mix (color,c1 + c1*s1, fresnel_step (rd, pn1, FR0));
		color = mix (color,c0 + c0*s0, fresnel_step (rd, pn0, FR0));

		
	}
	
	
	fragColor = color;
}