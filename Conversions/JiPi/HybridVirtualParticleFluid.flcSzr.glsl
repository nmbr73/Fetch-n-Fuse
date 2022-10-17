

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

***Click to paint.***

Automatic mouse movement can be turned off using the ENABLE_AUTO_MOUSE #define
The bounding box can be turned off using the ENABLE_BOUNDS #define.

This is a hybrid fluid simulation that combines both forward and reverse
advection techniques to achieve a high-quality pressure solution,
accurate advection, and decent performance while also remaining 
conservative unless under extreme velocities. The virtual particle
method used here is based on Michael Moroz' Reintegration Tracking 
method, extended to support Gaussian particle kernels:
https://www.shadertoy.com/view/WtfyDj

In the low-velocity regime, forward advection with virtual particles is
used. Virtual particle size is controlled according to the magnitude of
velocity; low velocity particles increase in size while high velocity
particle decrease in size. The virtual particle size scaling measure
can be changed using the VIRTUAL_PARTICLE_SIZE #define.

Virtual particles use a gaussian kernel. In order to conserve velocity
and mass in forward advection, masses and velocities from neighboring
particles are accumulated according to box integrals of the error function
(approximated here as tanh, but a more accurate approximation is provided
by toggling the USE_TANH #define). Both the integral and center of mass
of box intersections with a gaussian kernel are computed here.

When particle velocities exceed the forward advection integration range, 
reverse advection is used, using the RK4 method. Using this method,
it is possible to achieve forward advection without also setting a hard
upper bound on velocity.

The Poisson pressure solver kernel used here is precomputed using a custom solver.
First, a 2D kernel is computed, then a separable kernel is derived using
Singular Value Decomposition. The separable kernel method used here can
achieve a nearly-perfect pressure solve in 4 steps, but a single step is
used here for interactivity. The number of pressure solver steps per
fluid solver steps can be changed with the FRAME_DIVIDER #define (set to 1
by default, but can be changed to 4 for a high quality pressure solve).

Additional methods are implemented here in order to increase fluid detail.
This simulation implements multiscale Vorticity Confinement, a kernel-based
turbulence method based on my earlier Multiscale MIP Fluid simulation:
https://www.shadertoy.com/view/tdVSDh
and Florian Berger's work:
https://www.shadertoy.com/view/MsGSRd
This work also implements multiscale viscosity. The size and shape of
the kernels for these methods can be changed using the
MULTISCALE_KERNEL_POWER and MULTISCALE_KERNEL_STEPS #defines.


*/

#define THIN_FILM
#ifdef NORMAL
void mainImage( out vec4 c, in vec2 p )
{    
    initialize(p, iFrame, iResolution);

    vec4 fluid = texture(iChannel1, uv);
    #ifdef USE_VORTICITY
        float v = 0.5*fluid.w + 0.5;
    #else
        float v = fluid.w;
    #endif
    c = v*(0.5 + 1.0*fluid);
    
    vec4 curlcol =  mix(vec4(1,0,0,0),vec4(0,0,1,0),smoothstep(0.,1.,fluid.w + 0.5));
    curlcol = mix(vec4(1), curlcol, smoothstep(0.,1.,pow(abs(4.0*fluid.w),0.5)));
    
    float p0 = textureLod(iChannel1, uv, 0.).x;
    float p1 = textureLod(iChannel1, uv, 12.).x;
    float h = smoothstep(-1.,1., 0.2*(p0-p1));
    c = vec4(smoothstep(-.4,1.2,2.0*h * length(fluid.xy) * curlcol));
    //c = vec4(length(fluid.xy));
    c = fluid.zzzz;
    
    vec2 comt = textureLod(iChannel2, uv, 0.).zw;
    c = vec4(4.0*comt + 0.5,8.0*length(comt),0);
}
#endif

#ifdef HEIGHT
void mainImage( out vec4 c, in vec2 p )
{    
    initialize(p, iFrame, iResolution);
    
    vec4 tx = texelFetch(iChannel3, ivec2(p), 0);
    vec2 t1 = unpack2x16(tx.x);
    vec2 t2 = unpack2x16(tx.y);
    vec2 t3 = unpack2x16(tx.z);

    float height = texture(iChannel2, uv).x;
    vec4 fluid = texture(iChannel1, uv);
    
    //c = 0.5 + 0.5*vec4(height);
    //c = length(fluid.xy) * (0.5+fluid);
    //c = 0.3*vec4(fluid.zzz,1);
    c = 0.15*vec4(fluid.zzz,1) * vec4(length(fluid.xy));
}
#endif


#ifdef THIN_FILM
/*
	Fast Thin-Film Interference

	This is a performance-optimized version of my previous 
	thin-film interference shader here: https://www.shadertoy.com/view/XddXRj
	This version also fixes a platform-specific bug and has
	a few other tweaks as well.

	Thin-film interference and chromatic dispersion are simulated at
	six different wavelengths and then downsampled to RGB.
*/

// To see just the reflection (no refraction/transmission) uncomment this next line:
//#define REFLECTANCE_ONLY

// performance and raymarching options
#define INTERSECTION_PRECISION 0.01  // raymarcher intersection precision
#define ITERATIONS 20				 // max number of iterations
#define AA_SAMPLES 1				 // anti aliasing samples
#define BOUND 6.0					 // cube bounds check
#define DIST_SCALE 0.9   			 // scaling factor for raymarching position update

// optical properties
#define DISPERSION 0.05			     // dispersion amount
#define IOR 0.9     				 // base IOR value specified as a ratio
#define THICKNESS_SCALE 32.0		 // film thickness scaling factor
#define THICKNESS_CUBEMAP_SCALE 0.1  // film thickness cubemap scaling factor
#define REFLECTANCE_SCALE 3.0        // reflectance scaling factor
#define REFLECTANCE_GAMMA_SCALE 1.0  // reflectance gamma scaling factor
#define FRESNEL_RATIO 0.1			 // fresnel weight for reflectance
#define SIGMOID_CONTRAST 10.0         // contrast enhancement

#define GAMMA_CURVE 1.0
#define GAMMA_SCALE 1.0

#define TWO_PI 6.28318530718
#define WAVELENGTHS 6				 // number of wavelengths, not a free parameter

// iq's cubemap function
vec3 fancyCube( sampler2D sam, in vec3 d, in float s, in float b )
{
    vec3 colx = textureLod( sam, 0.5 + s*d.yz/d.x, b ).xyz;
    vec3 coly = textureLod( sam, 0.5 + s*d.zx/d.y, b ).xyz;
    vec3 colz = textureLod( sam, 0.5 + s*d.xy/d.z, b ).xyz;
    
    vec3 n = d*d;
    
    return (colx*n.x + coly*n.y + colz*n.z)/(n.x+n.y+n.z);
}

// iq's 3D noise function
float hash( float n ){
    return fract(sin(n)*43758.5453);
}

float noise( in vec3 x ) {
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

vec3 noise3(vec3 x) {
	return vec3( noise(x+vec3(123.456,.567,.37)),
				 noise(x+vec3(.11,47.43,19.17)),
				 noise(x) );
}

// a sphere with a little bit of warp
float sdf( vec3 p ) {
	vec3 n = vec3(sin(iDate.w * 0.5), sin(iDate.w * 0.3), cos(iDate.w * 0.2));
	vec3 q = 0.1 * (noise3(p + n) - 0.5);
  
	return length(q + p) - 3.5;
}

vec3 fresnel( vec3 rd, vec3 norm, vec3 n2 ) {
   vec3 r0 = pow((1.0-n2)/(1.0+n2), vec3(2));
   return r0 + (1. - r0)*pow(clamp(1. + dot(rd, norm), 0.0, 1.0), 5.);
}

vec3 calcNormal( in vec3 pos ) {
    const float eps = INTERSECTION_PRECISION;

    const vec3 v1 = vec3( 1.0,-1.0,-1.0);
    const vec3 v2 = vec3(-1.0,-1.0, 1.0);
    const vec3 v3 = vec3(-1.0, 1.0,-1.0);
    const vec3 v4 = vec3( 1.0, 1.0, 1.0);

	return normalize( v1*sdf( pos + v1*eps ) + 
					  v2*sdf( pos + v2*eps ) + 
					  v3*sdf( pos + v3*eps ) + 
					  v4*sdf( pos + v4*eps ) );
}

vec3 filmic_gamma(vec3 x) {
	return log(GAMMA_CURVE * x + 1.0) / GAMMA_SCALE;    
}

vec3 filmic_gamma_inverse(vec3 y) {
	return (1.0 / GAMMA_CURVE) * (exp(GAMMA_SCALE * y) - 1.0); 
}

// sample weights for the cubemap given a wavelength i
// room for improvement in this function
#define GREEN_WEIGHT 2.8
vec3 texCubeSampleWeights(float i) {
	vec3 w = vec3((1.0 - i) * (1.0 - i), GREEN_WEIGHT * i * (1.0 - i), i * i);
    return w / dot(w, vec3(1.0));
}

vec3 sampleCubeMap(vec3 i, vec3 rd) {
	vec3 col = textureLod(iChannel0, rd * vec3(1.0,-1.0,1.0), 0.0).xyz; 
    return vec3(
        dot(texCubeSampleWeights(i.x), col),
        dot(texCubeSampleWeights(i.y), col),
        dot(texCubeSampleWeights(i.z), col)
    );
}

vec3 sampleCubeMap(vec3 i, vec3 rd0, vec3 rd1, vec3 rd2) {
	vec3 col0 = textureLod(iChannel0, rd0 * vec3(1.0,-1.0,1.0), 0.0).xyz;
    vec3 col1 = textureLod(iChannel0, rd1 * vec3(1.0,-1.0,1.0), 0.0).xyz; 
    vec3 col2 = textureLod(iChannel0, rd2 * vec3(1.0,-1.0,1.0), 0.0).xyz; 
    return vec3(
        dot(texCubeSampleWeights(i.x), col0),
        dot(texCubeSampleWeights(i.y), col1),
        dot(texCubeSampleWeights(i.z), col2)
    );
}



vec3 sampleWeights(float i) {
	return vec3((1.0 - i) * (1.0 - i), GREEN_WEIGHT * i * (1.0 - i), i * i);
}

vec3 resample(vec3 wl0, vec3 wl1, vec3 i0, vec3 i1) {
	vec3 w0 = sampleWeights(wl0.x);
    vec3 w1 = sampleWeights(wl0.y);
    vec3 w2 = sampleWeights(wl0.z);
    vec3 w3 = sampleWeights(wl1.x);
    vec3 w4 = sampleWeights(wl1.y);
    vec3 w5 = sampleWeights(wl1.z);
    
    return i0.x * w0 + i0.y * w1 + i0.z * w2
         + i1.x * w3 + i1.y * w4 + i1.z * w5;
}

// downsample to RGB
vec3 resampleColor(vec3[WAVELENGTHS] rds, vec3 refl0, vec3 refl1, vec3 wl0, vec3 wl1) {

    
    #ifdef REFLECTANCE_ONLY
    	vec3 intensity0 = refl0;
    	vec3 intensity1 = refl1;
    #else
        vec3 cube0 = sampleCubeMap(wl0, rds[0], rds[1], rds[2]);
    	vec3 cube1 = sampleCubeMap(wl1, rds[3], rds[4], rds[5]);
    
        vec3 intensity0 = filmic_gamma_inverse(cube0) + refl0;
    	vec3 intensity1 = filmic_gamma_inverse(cube1) + refl1;
    #endif
    vec3 col = resample(wl0, wl1, intensity0, intensity1);

    return col / float(WAVELENGTHS);
}

// compute the wavelength/IOR curve values.
vec3 iorCurve(vec3 x) {
	return x;
}

vec3 attenuation(float filmThickness, vec3 wavelengths, vec3 normal, vec3 rd) {
	return 0.5 + 0.5 * cos(((THICKNESS_SCALE * filmThickness)/(wavelengths + 1.0)) * dot(normal, rd));    
}

vec3 contrast(vec3 x) {
	return 1.0 / (1.0 + exp(-SIGMOID_CONTRAST * (x - 0.5)));    
}

void doCamera( out vec3 camPos, out vec3 camTar, in float time, in vec4 m ) {
    camTar = vec3(0.0,0.0,0.0); 
    if (max(m.z, m.w) <= 0.0) {
    	float an = 1.5 + sin(time * 0.05) * 4.0;
		camPos = vec3(6.5*sin(an), 0.0 ,6.5*cos(an));   
    } else {
    	float an = 10.0 * m.x - 5.0;
		camPos = vec3(6.5*sin(an),10.0 * m.y - 5.0,6.5*cos(an)); 
    }
}

mat3 calcLookAtMatrix( in vec3 ro, in vec3 ta, in float roll )
{
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(sin(roll),cos(roll),0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
    return mat3( uu, vv, ww );
}

void mainImage( out vec4 c, in vec2 p )
{   
    initialize(p, iFrame, iResolution);
    vec3 col = vec3(0.0);
    
    Vec4Neighborhood pn = GetVec4Neighborhood(iChannel2);
    vec2 dp = Delta(pn, 0);
    
    vec3 wavelengths0 = vec3(1.0, 0.8, 0.6);
    vec3 wavelengths1 = vec3(0.4, 0.2, 0.0);
    vec3 iors0 = IOR + iorCurve(wavelengths0) * DISPERSION;
    vec3 iors1 = IOR + iorCurve(wavelengths1) * DISPERSION;
    
    vec3 rds[WAVELENGTHS];
    

    vec3 normal = normalize(vec3(dp,10.0));
    vec3 nggx = normalize(vec3(dp,0.1));
    
    /*
    mat3 camMat = calcLookAtMatrix( vec3(1.0*(uv-0.5),1), vec3(1,0,-1), 0.0 );
    vec3 rd = camMat*vec3(0,0,1);*/
    
    #define TIME (0.05*(15.0*sin(iTime/30.0)+60.0))
    //#define TIME 16.2+sin(0.05*51.2)
    vec2 lookat = vec2(sin(TIME*1.1), cos(TIME));
    mat3 camMat = calcLookAtMatrix( vec3(0,0,0), vec3(lookat,-1), PI );
    vec3 rd = camMat*vec3(uv-0.5,1.0);
    
    float spec = 1.0*ggx(nggx, normalize(rd), vec3(0,1,8), 0.02, 1.0);

    float filmThickness = 0.1+.2*textureLod(iChannel1, uv, 0.).z;

    vec3 att0 = attenuation(filmThickness, wavelengths0, normal, rd);
    vec3 att1 = attenuation(filmThickness, wavelengths1, normal, rd);

    vec3 rrd = reflect(rd, normal);
    vec3 f0 = (1.0 - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0 / iors0);
    vec3 f1 = (1.0 - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0 / iors1);

    //vec3 rrd = reflect(rd, normal);

    vec3 cube0 = REFLECTANCE_GAMMA_SCALE * att0 * filmic_gamma_inverse(sampleCubeMap(wavelengths0, rrd));
    vec3 cube1 = REFLECTANCE_GAMMA_SCALE * att1 * filmic_gamma_inverse(sampleCubeMap(wavelengths1, rrd));

    vec3 refl0 = REFLECTANCE_SCALE * mix(vec3(0), cube0, f0);
    vec3 refl1 = REFLECTANCE_SCALE * mix(vec3(0), cube1, f1);

    rds[0] = refract(rd, normal, iors0.x);
    rds[1] = refract(rd, normal, iors0.y);
    rds[2] = refract(rd, normal, iors0.z);
    rds[3] = refract(rd, normal, iors1.x);
    rds[4] = refract(rd, normal, iors1.y);
    rds[5] = refract(rd, normal, iors1.z);

    col += resampleColor(rds, refl0, refl1, wavelengths0, wavelengths1);
        
    //c = vec4( contrast(col)+spec*col, 1.0 );
    //c = vec4(contrast(0.6*filmic_gamma(spec*col)),1);
    //c = vec4(contrast(filmic_gamma(col/1.0)),4);
    c = vec4(contrast(filmic_gamma(col/2.0)),1);
    //c += 0.25*spec;
}
#endif
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 c, in vec2 p )
{
    initialize(p, iFrame, iResolution);
    if (iFrame <= 1) {
        c = vec4(0,0,INIT_MASS,0);
    } else {
        if (FRAME_MOD(0)) {
            Fluid(c, uv*R, iChannel0, iChannel1, iMouse);
        } else {
            c = texelFetch(iChannel0, ivec2(p), 0);
        }
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.14159265
#define TWO_PI 6.28318530718

#define TURBULENCE_SCALE 0.1
#define VORTICITY_SCALE 0.005
#define VISCOSITY_SCALE 0.01
#define MAX_CONSERVATIVE_DISTANCE 4.0

#define MULTISCALE_KERNEL_POWER 3.0
#define MULTISCALE_KERNEL_STEPS 1

#define ENABLE_BOUNDS
#define USE_TANH
#define ENABLE_AUTO_MOUSE

//#define VIRTUAL_PARTICLE_SIZE mix(0.4, 0.01, smoothstep(0., 3., mass * length(v)))
#define VIRTUAL_PARTICLE_SIZE mix(1.0, 0.15, smoothstep(0., 3., length(v)))
//#define VIRTUAL_PARTICLE_SIZE 0.15

#define INIT_MASS 0.01
#define FRAME_DIVIDER 1
#define FRAME_MOD(x) ((iFrame % FRAME_DIVIDER)==0)




vec2 R;
int F;
vec2 uv;
vec2 texel;

vec4 bounds;

//internal RNG state 
uvec4 s0; 

void initialize(inout vec2 p, int frame, vec3 res)
{
    uv = p / res.xy;
    p = floor(p);
    R = res.xy;
    texel = 1.0/R;
    F = frame;
    
    bounds = vec4(2.0*texel,1.-2.0*texel);

    //white noise seed
    s0 = uvec4(p, uint(frame), uint(p.x) + uint(p.y));
}

// https://www.pcg-random.org/
uvec4 pcg4d(inout uvec4 v)
{
	v = v * 1664525u + 1013904223u;
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
    v = v ^ (v>>16u);
    v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
    return v;
}

float rand(){ return float(pcg4d(s0).x)/float(0xffffffffu); }
vec2 rand2(){ return vec2(pcg4d(s0).xy)/float(0xffffffffu); }
vec3 rand3(){ return vec3(pcg4d(s0).xyz)/float(0xffffffffu); }
vec4 rand4(){ return vec4(pcg4d(s0))/float(0xffffffffu); }


#define _PH_COMP xy

vec2 normz(vec2 x) {
	return length(x) < 1e-6 ? vec2(0) : normalize(x);
}

vec4 normz(vec4 x) {
	return length(x) < 1e-6 ? vec4(0) : normalize(x);
}

#define pack2x16(d) uintBitsToFloat(packHalf2x16(d))
#define unpack2x16(d) unpackHalf2x16(floatBitsToUint(d))

bool reset(sampler2D ch) {
    return texture(ch, vec2(32.5/256.0, 0.5) ).x > 0.5;
}


float G1V(float dnv, float k){
    return 1.0/(dnv*(1.0-k)+k);
}

float ggx(vec3 n, vec3 v, vec3 l, float rough, float f0){
    float alpha = rough*rough;
    vec3 h = normalize(v+l);
    float dnl = clamp(dot(n,l), 0.0, 1.0);
    float dnv = clamp(dot(n,v), 0.0, 1.0);
    float dnh = clamp(dot(n,h), 0.0, 1.0);
    float dlh = clamp(dot(l,h), 0.0, 1.0);
    float f, d, vis;
    float asqr = alpha*alpha;
    const float pi = 3.14159;
    float den = dnh*dnh*(asqr-1.0)+1.0;
    d = asqr/(pi * den * den);
    dlh = pow(1.0-dlh, 5.0);
    f = f0 + (1.0-f0)*dlh;
    float k = alpha/1.0;
    vis = G1V(dnl, k)*G1V(dnv, k);
    float spec = dnl * d * f * vis;
    return spec;
}



struct Vec4Neighborhood {
    vec4 c; vec4 n; vec4 e; vec4 w; vec4 s; vec4 ne; vec4 nw; vec4 sw; vec4 se;
};

vec4 GetCenter(Vec4Neighborhood n) {
    return n.c;
}

struct Vec4Kernel {
    vec4 c; vec4 n; vec4 e; vec4 w; vec4 s; vec4 ne; vec4 nw; vec4 sw; vec4 se;
};

vec4 ApplyVec4KernelVector(Vec4Neighborhood n, Vec4Kernel k) {
    return n.c*k.c + n.n*k.n + n.e*k.e + n.w*k.w + n.s*k.s + n.ne*k.ne + n.nw*k.nw + n.sw*k.sw + n.se*k.se;
}

float ApplyVec4KernelScalar(Vec4Neighborhood n, Vec4Kernel k) {
    return dot(n.c,k.c) + dot(n.n,k.n) + dot(n.e,k.e) + dot(n.w,k.w) + dot(n.s,k.s) 
         + dot(n.ne,k.ne) + dot(n.nw,k.nw) + dot(n.sw,k.sw) + dot(n.se,k.se);
}

vec4 ApplyVec4KernelPermutationVector(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return n.c[v]*k.c + n.n[v]*k.n + n.e[v]*k.e + n.w[v]*k.w + n.s[v]*k.s 
         + n.ne[v]*k.ne + n.nw[v]*k.nw + n.sw[v]*k.sw + n.se[v]*k.se;
}

vec4 ApplyVec4KernelPermutationVectorAbs(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return abs(n.c[v])*k.c + abs(n.n[v])*k.n + abs(n.e[v])*k.e + abs(n.w[v])*k.w + abs(n.s[v])*k.s 
         + abs(n.ne[v])*k.ne + abs(n.nw[v])*k.nw + abs(n.sw[v])*k.sw + abs(n.se[v])*k.se;
}

float ApplyVec4KernelPermutationScalar(Vec4Neighborhood n, Vec4Kernel k, int v) {
    return dot(vec4(n.c[v]),k.c) + dot(vec4(n.n[v]),k.n) + dot(vec4(n.e[v]),k.e) 
         + dot(vec4(n.w[v]),k.w) + dot(vec4(n.s[v]),k.s) + dot(vec4(n.ne[v]),k.ne) 
         + dot(vec4(n.nw[v]),k.nw) + dot(vec4(n.sw[v]),k.sw) + dot(vec4(n.se[v]),k.se);
}


bool BoundsCheck(vec2 ouv) {
    #ifdef ENABLE_BOUNDS
        return (ouv.x < bounds.x || ouv.y < bounds.y || ouv.x > bounds.z || ouv.y > bounds.w);
    #else
        return false;
    #endif
}

vec2 BoundsClamp(vec2 ouv) {
    return clamp(ouv, bounds.xy, bounds.zw);
}

vec4 BoundedTex(sampler2D ch, vec2 p) {
    if (BoundsCheck(p)) {
        return vec4(0,0,0,0);
    } else {
        return textureLod(ch, p, 0.);
    }
}

vec4 BoundedTex(sampler2D ch, vec2 off, int x, int y) {
    vec2 ouv = uv + texel * (off + vec2(x,y));
    return BoundedTex(ch, ouv);
}

vec4 BoundedTex(sampler2D ch, vec2 off, float x, float y) {
    vec2 ouv = uv + texel * (off + vec2(x,y));
    return BoundedTex(ch, ouv);
}

#define U(name,x,y) vec3 name = BoundedTex(ch, vec2(0), x, y)
#define S(name,x,y) name = BoundedTex(ch, vec2(0), x, y)
#define COM(name,x,y) name = BoundedTex(ch_com, vec2(0), x, y).zw
#define SO(name,x,y) name = BoundedTex(ch, off, float(x), float(y))
#define SR(name,x,y) name = BoundedTex(ch, off - RK4(ch, uv + texel*(off+vec2(x,y)), 1.0).xy, float(x), float(y))
#define K(name,x,y,z,w) name = vec4(x,y,z,w)
#define KV(name,x) name = vec4(x)

vec2 RK4(sampler2D ch, vec2 p, float h){
    vec2 k1 = BoundedTex(ch,p).xy;
    vec2 k2 = BoundedTex(ch,p - texel*0.5*h*k1).xy;
    vec2 k3 = BoundedTex(ch,p - texel*0.5*h*k2).xy;
    vec2 k4 = BoundedTex(ch,p - texel*h*k3).xy;
    return h/3.*(0.5*k1+k2+k3+0.5*k4);
}

vec2 RK4(sampler2D ch, float h){
    return RK4(ch, uv, h);
}

Vec4Neighborhood GetVec4Neighborhood(sampler2D ch) {
    Vec4Neighborhood n;
    S(n.c,0,0); S(n.n,0,1); S(n.e,1,0); S(n.s,0,-1); S(n.w,-1,0);
    S(n.nw,-1,1); S(n.sw,-1,-1); S(n.ne,1,1); S(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetVec4Neighborhood(sampler2D ch, vec2 off) {
    Vec4Neighborhood n;
    SO(n.c,0,0); SO(n.n,0,1); SO(n.e,1,0); SO(n.s,0,-1); SO(n.w,-1,0);
    SO(n.nw,-1,1); SO(n.sw,-1,-1); SO(n.ne,1,1); SO(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetVec4NeighborhoodRK4(sampler2D ch) {
    Vec4Neighborhood n;
    vec2 off = vec2(0);
    SR(n.c,0,0); SR(n.n,0,1); SR(n.e,1,0); SR(n.s,0,-1); SR(n.w,-1,0);
    SR(n.nw,-1,1); SR(n.sw,-1,-1); SR(n.ne,1,1); SR(n.se,1,-1);
    return n;
}

Vec4Neighborhood GetStridedVec4Neighborhood(sampler2D ch, float stride) {
    Vec4Neighborhood n;
    vec2 off = vec2(0);
    float s = stride;
    SO(n.c,0,0); SO(n.n,0,s); SO(n.e,s,0); SO(n.s,0,-s); SO(n.w,-s,0);
    SO(n.nw,-s,1); SO(n.sw,-s,-s); SO(n.ne,s,s); SO(n.se,s,-s);
    return n;
}

Vec4Neighborhood GetStridedVec4NeighborhoodRK4(sampler2D ch, float stride) {
    Vec4Neighborhood n;
    vec2 off = vec2(0);
    float s = stride;
    SR(n.c,0,0); SR(n.n,0,s); SR(n.e,s,0); SR(n.s,0,-s); SR(n.w,-s,0);
    SR(n.nw,-s,1); SR(n.sw,-s,-s); SR(n.ne,s,s); SR(n.se,s,-s);
    return n;
}


Vec4Neighborhood GetVec4NeighborhoodRK4(sampler2D ch, vec2 off) {
    Vec4Neighborhood n;
    SR(n.c,0,0); SR(n.n,0,1); SR(n.e,1,0); SR(n.s,0,-1); SR(n.w,-1,0);
    SR(n.nw,-1,1); SR(n.sw,-1,-1); SR(n.ne,1,1); SR(n.se,1,-1);
    return n;
}

Vec4Kernel Vec4NeighborhoodToVec4KernelTransform(Vec4Neighborhood n, Vec4Kernel k) {
    Vec4Kernel k2;
    KV(k2.nw,n.nw*k.nw); KV(k2.n,n.n*k.n); KV(k2.ne,n.ne*k.ne);
    KV(k2.w,n.w*k.w); KV(k2.c,n.c*k.c); KV(k2.e,n.e*k.e);
    KV(k2.sw,n.sw*k.sw); KV(k2.s,n.s*k.s); KV(k2.se,n.se*k.se);
    return k2;
}

Vec4Kernel GetCurlKernel() {
    const float D = 0.5;
    Vec4Kernel k;
    K(k.c, 0, 0, 0, 0);
    K(k.n, 1, 0, 0, 0);
    K(k.s,-1, 0, 0, 0);
    K(k.e, 0,-1, 0, 0);
    K(k.w, 0, 1, 0, 0);
    K(k.nw, D, D, 0, 0);
    K(k.ne, D,-D, 0, 0);
    K(k.sw,-D, D, 0, 0);
    K(k.se,-D,-D, 0, 0);
    return k;
}


Vec4Kernel GetDivKernel() {
    const float D = 0.5;
    Vec4Kernel k;
    K(k.c, 0, 0, 0, 0);
    K(k.n, 0,-1, 0, 0);
    K(k.s, 0, 1, 0, 0);
    K(k.e,-1, 0, 0, 0);
    K(k.w, 1, 0, 0, 0);
    K(k.nw, D,-D, 0, 0);
    K(k.ne,-D,-D, 0, 0);
    K(k.sw, D, D, 0, 0);
    K(k.se,-D, D, 0, 0);
    return k;
}

vec2 Turbulence(Vec4Neighborhood n) {
    return  - 4.0 * n.c.xy 
            + 2.0 * vec2(n.n.x + n.s.x, n.e.y + n.w.y)
            + (n.se - n.ne - n.sw + n.nw).yx;
}


Vec4Kernel GetScalarKernel(float center, float edge, float vertex) {
    Vec4Kernel k;
    KV(k.c, center);
    KV(k.n, edge);
    KV(k.s, edge);
    KV(k.e, edge);
    KV(k.w, edge);
    KV(k.nw, vertex);
    KV(k.ne, vertex);
    KV(k.sw, vertex);
    KV(k.se, vertex);
    return k;
}

Vec4Kernel GetGaussianKernel() {
    const float G0 = 0.25;
    const float G1 = 0.125;
    const float G2 = 0.0625;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetNeighborAvgKernel() {
    const float G0 = 0.0;
    const float G1 = 1.0/6.0;
    const float G2 = 1.0/12.0;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetNeighborAvgVonNeumannKernel() {
    const float G0 = 0.0;
    const float G1 = 0.25;
    const float G2 = 0.0;
    return GetScalarKernel(G0, G1, G2);
}

Vec4Kernel GetLaplacianKernel() {
    const float L0 = -20.0/6.0;
    const float L1 = 4.0/6.0;
    const float L2 = 1.0/6.0;
    return GetScalarKernel(L0, L1, L2);
}

vec4 Advect(sampler2D ch, float timestep) {
    return textureLod(ch,fract(uv - texel*RK4(ch,timestep)), 0.);
}

vec2 Rotate(vec2 v, float r) {
    float s = sin(r);
    float c = cos(r);
    return mat2(c, -s, s, c) * v;
}

vec2 SoftBound(vec2 x, float p) {
    vec2 soft = normz(x) * pow(dot(x,x),1.5);
    return x - p * soft;
}

float SoftBound(float x, float p) {
    float soft = sign(x) * pow(abs(x),3.0);
    return x - p * soft;
}

vec2 SoftBound(vec2 x, float s, float p) {
    vec2 soft = normz(x) * pow(dot(s*x,s*x),1.5);
    return x - p * soft;
}

float SoftBound(float x, float s, float p) {
    float soft = sign(x) * pow(abs(s*x),3.0);
    return x - p * soft;
}

vec2 HardBound(vec2 x, float p) {
    return max(min((length(x) / p) > 1.0 ? (p * normz(x)) : x, p), -p);
}

float HardBound(float x, float p) {
    return max(min(x, p), -p);
}

vec2 Vorticity(Vec4Neighborhood n, float curl) {
    return  -curl * normz(ApplyVec4KernelPermutationVectorAbs(n, GetCurlKernel(), 3).xy);
}

vec2 Delta(Vec4Neighborhood n, int channel) {
    return ApplyVec4KernelPermutationVector(n, GetDivKernel(), channel).xy;
}

vec4 getAutoMouse() {
    int stage = (F/120)%4;
    vec4 auto = vec4(0);
    switch(stage) {
        case 0:
            auto = vec4(0.2, 0.5, 1.0, 0.0); break;
        case 1:
            auto = vec4(0.5, 0.2, 0.0, 1.0); break;
        case 2:
            auto = vec4(0.8, 0.5, -1.0, 0.0); break;
        case 3:
            auto = vec4(0.5, 0.8, 0.0, -1.0); break;
    }
    return auto * vec4(R,1,1);
}

vec4 MouseSpace(vec4 mouse, vec4 phase, vec2 p, float width, float strength) {
    if (mouse.z > 0.) {
        phase.xy += strength * exp(-length(p-mouse.xy) / width) * normz(mouse.xy-abs(mouse.zw));
    } else {
        #ifdef ENABLE_AUTO_MOUSE
            vec4 auto = getAutoMouse();
            phase.xy += strength * exp(-length(p-auto.xy) / width) * auto.zw;
        #endif
    }
    return phase;
}

vec4 MouseMass(vec4 mouse, vec4 phase, vec2 p, float width, float strength) {
    if (mouse.z > 0.) {
        phase.z += strength * exp(-length(p-mouse.xy) / width);
    } else {
        #ifdef ENABLE_AUTO_MOUSE
            vec4 auto = getAutoMouse();
            phase.z += strength * exp(-length(p-auto.xy) / width);
        #endif
    }
    return phase;
}

#undef T
#undef V


float erf(float x) {
    #ifdef USE_TANH
        return tanh(1.22848*x);
    #elif USE_SMOOTHSTEP
        return -1.+2.*smoothstep(-1.657,1.657,sign(x)*pow(abs(x),0.85715));
    #else
    if (x > 9.0) {
        return 1.0;
    } else if (x < -9.0) {
        return -1.0;
    } else if (abs(x) < 1e-9) {
        return x;
    }
    const float p = 0.3275911;
    const float a1 = 0.254829592;
    const float a2 = -0.284496736;
    const float a3 = 1.421413741;
    const float a4 = -1.453152027;
    const float a5 = 1.061405429;
    float sx = sign(x);
    x *= sx;
    float t = 1.0 / (1.0 + p * x);
    return clamp(sx * (1.0 - (a1*t + a2*t*t + a3*t*t*t + a4*t*t*t*t + a5*t*t*t*t*t) * exp(-x*x)),-1.0,1.0);
    #endif
}


float safeexp(float x) {
    return exp(clamp(x, -87.0, 87.0));
}

//https://www.wolframalpha.com/input/?i=%28%28sqrt%28k%29+e%5E%28-%28a%5E2+%2B+b%5E2%29%2Fk%29+%28e%5E%28a%5E2%2Fk%29+-+e%5E%28b%5E2%2Fk%29%29+%28erf%28c%2Fsqrt%28k%29%29+-+erf%28d%2Fsqrt%28k%29%29%29%29%2F%284+sqrt%28%CF%80%29%29%29++%2F+%281%2F4+%28erf%28a%2Fsqrt%28k%29%29+-+erf%28b%2Fsqrt%28k%29%29%29+%28erf%28c%2Fsqrt%28k%29%29+-+erf%28d%2Fsqrt%28k%29%29%29%29
float center_of_mass(vec2 b, float K) {
    float sqK = sqrt(K);
    float sqP = sqrt(PI);
    float erax = erf(b.x/sqK);
    float erbx = erf(b.y/sqK);
    float exabx = safeexp((b.x*b.x + b.y*b.y)/K);
    float exax = safeexp(-(b.x*b.x)/K);
    float exbx = safeexp(-(b.y*b.y)/K);
    
    //return clamp((sqK * (exax - exbx)) / (sqP * (erbx - erax)),-4.0,4.0);
    return HardBound((sqK * (exax - exbx)) / (sqP * (erbx - erax)),16.0);
}

vec3 distribution(vec2 x, vec2 p, float K)
{
    vec2 omin = p - 0.5;
    vec2 omax = p + 0.5; 
    
    float sqK = sqrt(K);
    float sqP = sqrt(PI);
    
    //https://www.wolframalpha.com/input/?i=integral+of+%28integral+of+exp%28-%28x%5E2%2By%5E2%29%2Fk%29+with+respect+to+x+from+a+to+b%29+with+respect+to+y+from+c+to+d
    float masst = 0.25 *
                    ((erf((omin.x - x.x)/sqK) - erf((omax.x - x.x)/sqK)) * 
                    (erf((omin.y - x.y)/sqK) - erf((omax.y - x.y)/sqK)));
    
    vec2 com2 = x-p+vec2(center_of_mass(vec2(omin.x - x.x,omax.x - x.x), K), center_of_mass(vec2(omin.y - x.y,omax.y - x.y), K));
    return vec3(com2, masst);
}

#define range(i, r) for(int i = -r; i < r; i++)




vec2 com(vec2 p, sampler2D ch, sampler2D ch_com) {
    float mass_t = 0.0;
    vec2 com_t = vec2(0);
    range(i, 5) {
        range(j, 5) {
            vec2 off = vec2(0);
            S(vec4 u,i,j);
            COM(vec2 com_p,i,j);
            float mass = u.z;
            float curl = u.w;
            vec2 v = u.xy;
            vec2 p0 = com_p + vec2(i,j) + v;
            vec3 d = distribution(p0, vec2(0), VIRTUAL_PARTICLE_SIZE);
            float mass_p = mass * d.z;
            mass_t += mass_p;
            com_t += mass_p * d.xy;
        }
    }
    if (mass_t != 0.0) {
        com_t /= mass_t;
    }
    return vec2(com_t);
}

vec4 ForwardAdvection(vec2 p, sampler2D ch, sampler2D ch_com) {
    float mass_t = 0.0;
    vec2 vel_t = vec2(0);
    float curl_t = 0.0;
    range(i, 5) {
        range(j, 5) {
            vec2 off = vec2(0);
            S(vec4 u,i,j);
            COM(vec2 com_p,i,j);
            float mass = u.z;
            float curl = u.w;
            vec2 v = u.xy;
            vec2 p0 = com_p + vec2(i,j) + v;
            vec3 d = distribution(p0, vec2(0), VIRTUAL_PARTICLE_SIZE);
            float mass_p = mass * d.z;
            mass_t += mass_p;
            vel_t += v * mass_p;
            curl_t += curl * mass_p;
        }
    }
    if (mass_t != 0.0) {
        vel_t /= mass_t;
        curl_t /= mass_t;
    }
    return vec4(vel_t, mass_t, curl_t);
}


vec2 MultiscaleTurbulence(sampler2D ch) {
    vec2 turbulence = vec2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        vec4 U = GetCenter(n);
        float M = length(U.xy);
        turbulence += M*(1.0/pow(stride,MULTISCALE_KERNEL_POWER))*Turbulence(n);  
    }
    return turbulence;
}

vec2 MultiscaleVorticity(sampler2D ch) {
    vec2 vorticity = vec2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        vec4 U = GetCenter(n);
        float M = length(U.xy);
        float curl = ApplyVec4KernelScalar(n, GetCurlKernel());
        vorticity += M*(1.0/pow(stride,MULTISCALE_KERNEL_POWER))*Vorticity(n, curl);
    }
    return vorticity;
}

vec4 MultiscaleViscosity(sampler2D ch) {
    vec4 viscosity = vec4(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        vec4 U = GetCenter(n);
        vec4 laplacian = ApplyVec4KernelVector(n, GetLaplacianKernel());
        viscosity += (1.0/pow(stride,MULTISCALE_KERNEL_POWER))*laplacian;
    }
    return viscosity;
}

void MultiscaleKernels(sampler2D ch, out vec2 turbulence, out vec2 vorticity, out vec2 viscosity) {
    turbulence = vec2(0);
    vorticity = vec2(0);
    viscosity = vec2(0);
    for (int i = 1; i <= MULTISCALE_KERNEL_STEPS; i++) {
        float stride = float(i);
        Vec4Neighborhood n = GetStridedVec4NeighborhoodRK4(ch, stride);
        vec4 U = GetCenter(n);
        float M = length(U.xy);
        float curl = ApplyVec4KernelScalar(n, GetCurlKernel());
        vec4 laplacian = ApplyVec4KernelVector(n, GetLaplacianKernel());
        float W = (1.0/pow(stride,MULTISCALE_KERNEL_POWER));
        viscosity += W*laplacian.xy;
        turbulence += M*W*Turbulence(n);  
        vorticity += M*W*Vorticity(n, curl);
    }
}


void Fluid( out vec4 U, in vec2 p, sampler2D ch, sampler2D ch_com, vec4 mouse )
{
    vec2 turbulence, viscosity, vorticity;
    MultiscaleKernels(ch, turbulence, vorticity, viscosity);

    Vec4Neighborhood neighborhood = GetVec4NeighborhoodRK4(ch, TURBULENCE_SCALE * turbulence);

    vec4 dist = ForwardAdvection(p, ch, ch_com);
    U = GetCenter(neighborhood);
    //U = mix(dist,U,smoothstep(0.0,MAX_CONSERVATIVE_DISTANCE,length(U.xy)));
    U = mix(dist,U,smoothstep(MAX_CONSERVATIVE_DISTANCE - 1.0,MAX_CONSERVATIVE_DISTANCE,length(U.xy)));
    
    //float lU = length(U.xy);

    // Laplacian/Viscosity
    U.xy += VISCOSITY_SCALE*viscosity;

    // Curl/Vorticity
    U.w = ApplyVec4KernelScalar(neighborhood, GetCurlKernel());
    U.xy += VORTICITY_SCALE*vorticity; 
    
    //U.xy = lU*normz(U.xy);

    // Add mass with the mouse
    U = MouseMass(mouse, U, p, 10.0, 0.2);
    
    // Mouse interaction in phase domain/space domain
    U = MouseSpace(mouse, U, p, 20.0, 0.3);

    U.xy = SoftBound(U.xy, 1.0, 0.00001);
    U.xy = HardBound(U.xy, 16.0);
    U.z = max(0.0, SoftBound(U.z, 0.00001));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
bool reset() {
    return iFrame <= 1 || texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec4 Po(int m, int n) {
    vec2 ouv = uv + texel * vec2(m,n);
    if (BoundsCheck(ouv)) {
        return vec4(pack2x16(vec2(0)),pack2x16(vec2(0)),pack2x16(vec2(0)),0);
    } else {
        return textureLod(iChannel0, ouv, 0.0);
    }
}

float Go(int m, int n) {
    vec2 ouv = uv + texel * vec2(m,n);
    if (BoundsCheck(ouv)) {
        return 0.0;
    } else {
        return textureLod(iChannel1, ouv, 0.0).x;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    initialize(fragCoord, iFrame, iResolution);
    
    vec3 p_y[151] = vec3[](vec3(-0.00039936512386562484,-0.0013037830496811509,-0.003024369266497462),vec3(-0.00043479272952107184,-0.001418247837094988,-0.0032841431420175815),vec3(-0.00047311175208775147,-0.0015418984185112672,-0.0035640171624620187),vec3(-0.0005145378703601011,-0.0016753999013442086,-0.003865351345832546),vec3(-0.0005593015430286112,-0.00181946044562425,-0.004189581775882141),vec3(-0.0006076489826017296,-0.001974833797626846,-0.004538223620654535),vec3(-0.0006598431999115183,-0.002142321974855929,-0.004912874168389093),vec3(-0.0007161651258619417,-0.0023227781140663494,-0.005315215867981123),vec3(-0.0007769148178819725,-0.0025171094952469407,-0.00574701935895483),vec3(-0.0008424127594587307,-0.002726280755872174,-0.0062101464732849465),vec3(-0.0009130012621639432,-0.002951317311279131,-0.006706553188329675),vec3(-0.0009890459807688295,-0.003193308998756704,-0.007238292506523368),vec3(-0.0010709375533892544,-0.0034534139648667857,-0.007807517233220254),vec3(-0.001159093380140449,-0.003732862817675909,-0.008416482619054973),vec3(-0.0012539595555377625,-0.004032963067987648,-0.009067548827244675),vec3(-0.0013560129718921017,-0.004355103886359663,-0.009763183179221676),vec3(-0.0014657636132559054,-0.004700761205697934,-0.010505962123640127),vec3(-0.0015837570621268875,-0.0050715032025820585,-0.011298572863887848),vec3(-0.001710577244168271,-0.005468996194229675,-0.01214381456744096),vec3(-0.0018468494397243054,-0.005895010992201898,-0.01304459906634993),vec3(-0.0019932435949786592,-0.006351429758636426,-0.014003950941382284),vec3(-0.0021504779703164487,-0.006840253416027218,-0.015025006862317022),vec3(-0.0023193231689242525,-0.007363609667412167,-0.016111014032915174),vec3(-0.002500606595033479,-0.007923761690353888,-0.017265327560358507),vec3(-0.002695217398648073,-0.008523117575376556,-0.018491406534454295),vec3(-0.002904111972299025,-0.009164240587639544,-0.019792808560418648),vec3(-0.0031283200755794476,-0.00984986033967067,-0.021173182439071275),vec3(-0.003368951675232755,-0.01058288497304438,-0.022636258627945997),vec3(-0.0036272046027564153,-0.01136641445806573,-0.02418583704385647),vec3(-0.0039043731482907218,-0.012203755132905171,-0.025825771679025524),vec3(-0.004201857729535902,-0.013098435617310009,-0.027559951395473018),vec3(-0.00452117579826187,-0.01405422425106731,-0.029392276131583135),vec3(-0.00486397417548751,-0.015075148223856253,-0.0313266275951637),vec3(-0.005232043040666073,-0.016165514581003473,-0.03336683332200887),vec3(-0.00562733184154224,-0.017329933308865297,-0.03551662273933928),vec3(-0.0060519674414129645,-0.018573342723915864,-0.037779573578614306),vec3(-0.006508274881439065,-0.01990103741075051,-0.040159046618245986),vec3(-0.006998801210110017,-0.021318698975493443,-0.0426581062860751),vec3(-0.007526342923405148,-0.022832429901508428,-0.04527942409155754),vec3(-0.008093977672062791,-0.024448790812264518,-0.048025161159515134),vec3(-0.008705101032420682,-0.026174841459272888,-0.05089682526368315),vec3(-0.009363469312043276,-0.028018185757524506,-0.05389509666073895),vec3(-0.010073249580677117,-0.029987021181332632,-0.05701961564096375),vec3(-0.01083907839404583,-0.03209019280176492,-0.06026872295763385),vec3(-0.011666131030046848,-0.03433725218081176,-0.0636391420650306),vec3(-0.012560203507485332,-0.03673852121908325,-0.06712558924087116),vec3(-0.013527810238371974,-0.03930516085637037,-0.07072029400179898),vec3(-0.014576300919535715,-0.04204924420785956,-0.07441240748450588),vec3(-0.015714001257992345,-0.04498383322316428,-0.07818727031588318),vec3(-0.016950383431117873,-0.04812305719009568,-0.08202550346745388),vec3(-0.018296273925712297,-0.051482190232598785,-0.08590187504816119),vec3(-0.01976410874874513,-0.05507772316147769,-0.0897838820674849),vec3(-0.02136824920173702,-0.05892742230067349,-0.09362996770370406),vec3(-0.02312537581916243,-0.06305036371943105,-0.09738726988920325),vec3(-0.025054984222680283,-0.06746692484105196,-0.10098876378585503),vec3(-0.02718001534382911,-0.07219870536957633,-0.10434961578501721),vec3(-0.02952766495752478,-0.07726833375162516,-0.10736250560624028),vec3(-0.03213043568796663,-0.08269909044537457,-0.10989158977713533),vec3(-0.03502752169832732,-0.08851423913888098,-0.11176466592987913),vec3(-0.03826665722222464,-0.09473589144745072,-0.11276294193733785),vec3(-0.041906623421512364,-0.10138312131867837,-0.11260760327593745),vec3(-0.04602070835716154,-0.10846885934496983,-0.11094209169468648),vec3(-0.050701578024441495,-0.1159947726771164,-0.10730865033138975),vec3(-0.05606828993205902,-0.12394275393722222,-0.10111727591157052),vec3(-0.06227665518595623,-0.13226056303078285,-0.09160486258770702),vec3(-0.06953501099043398,-0.14083708737623918,-0.077782417479591),vec3(-0.07812908130749861,-0.149458503599576,-0.058369977232929894),vec3(-0.08846282054472075,-0.15772775580732623,-0.031725854990398726),vec3(-0.10112895949827036,-0.1649097516191365,0.004199908847829801),vec3(-0.11703860203562352,-0.16961595360355963,0.05178318985988212),vec3(-0.13767855574664672,-0.16911182932471974,0.11316073490042394),vec3(-0.16567645398825667,-0.15763971441508162,0.18793345990415217),vec3(-0.2062192509611813,-0.12179330842214287,0.2642830614724927),vec3(-0.2713074094027817,-0.02537420724641723,0.28182569573805677),vec3(-0.395760620255607,0.2520387049261719,-0.04827629938430805),vec3(-0.4794057541719356,0.45941736433304614,-0.35998402950121244),vec3(-0.39576062025560704,0.25203870492617186,-0.04827629938430784),vec3(-0.27130740940278164,-0.02537420724641726,0.2818256957380568),vec3(-0.20621925096118135,-0.12179330842214289,0.2642830614724926),vec3(-0.16567645398825667,-0.15763971441508165,0.18793345990415228),vec3(-0.13767855574664675,-0.1691118293247198,0.1131607349004239),vec3(-0.11703860203562354,-0.1696159536035597,0.05178318985988209),vec3(-0.10112895949827036,-0.1649097516191365,0.004199908847829665),vec3(-0.0884628205447208,-0.1577277558073263,-0.03172585499039877),vec3(-0.07812908130749864,-0.14945850359957605,-0.05836997723292993),vec3(-0.06953501099043398,-0.14083708737623918,-0.07778241747959105),vec3(-0.06227665518595623,-0.13226056303078287,-0.09160486258770703),vec3(-0.05606828993205902,-0.12394275393722226,-0.10111727591157058),vec3(-0.050701578024441495,-0.11599477267711644,-0.10730865033138978),vec3(-0.04602070835716156,-0.10846885934496989,-0.1109420916946865),vec3(-0.04190662342151239,-0.10138312131867841,-0.11260760327593748),vec3(-0.03826665722222467,-0.09473589144745076,-0.11276294193733784),vec3(-0.03502752169832737,-0.08851423913888105,-0.1117646659298792),vec3(-0.032130435687966606,-0.0826990904453746,-0.10989158977713538),vec3(-0.029527664957524794,-0.0772683337516252,-0.10736250560624033),vec3(-0.027180015343829116,-0.07219870536957636,-0.1043496157850173),vec3(-0.025054984222680304,-0.06746692484105203,-0.10098876378585511),vec3(-0.023125375819162436,-0.06305036371943108,-0.09738726988920338),vec3(-0.02136824920173702,-0.058927422300673514,-0.09362996770370416),vec3(-0.019764108748745155,-0.05507772316147772,-0.08978388206748496),vec3(-0.01829627392571232,-0.05148219023259884,-0.08590187504816127),vec3(-0.01695038343111789,-0.04812305719009571,-0.08202550346745398),vec3(-0.015714001257992355,-0.0449838332231643,-0.07818727031588321),vec3(-0.014576300919535724,-0.04204924420785958,-0.07441240748450599),vec3(-0.013527810238371971,-0.039305160856370404,-0.07072029400179902),vec3(-0.012560203507485332,-0.036738521219083255,-0.06712558924087117),vec3(-0.011666131030046859,-0.03433725218081179,-0.0636391420650307),vec3(-0.01083907839404584,-0.03209019280176495,-0.06026872295763393),vec3(-0.010073249580677119,-0.029987021181332653,-0.05701961564096378),vec3(-0.009363469312043281,-0.02801818575752456,-0.05389509666073898),vec3(-0.008705101032420694,-0.026174841459272933,-0.0508968252636832),vec3(-0.008093977672062803,-0.024448790812264518,-0.04802516115951517),vec3(-0.007526342923405146,-0.02283242990150845,-0.04527942409155761),vec3(-0.006998801210110013,-0.021318698975493443,-0.04265810628607512),vec3(-0.006508274881439066,-0.01990103741075051,-0.040159046618246),vec3(-0.00605196744141298,-0.018573342723915892,-0.03777957357861436),vec3(-0.005627331841542247,-0.017329933308865328,-0.03551662273933936),vec3(-0.005232043040666078,-0.016165514581003487,-0.033366833322008904),vec3(-0.004863974175487524,-0.015075148223856267,-0.031326627595163734),vec3(-0.004521175798261876,-0.01405422425106733,-0.029392276131583166),vec3(-0.004201857729535902,-0.013098435617310021,-0.027559951395473042),vec3(-0.0039043731482907213,-0.012203755132905178,-0.025825771679025535),vec3(-0.003627204602756424,-0.01136641445806575,-0.024185837043856497),vec3(-0.003368951675232753,-0.010582884973044387,-0.022636258627946024),vec3(-0.0031283200755794494,-0.009849860339670675,-0.021173182439071295),vec3(-0.002904111972299031,-0.009164240587639563,-0.019792808560418675),vec3(-0.002695217398648074,-0.00852311757537658,-0.01849140653445433),vec3(-0.002500606595033485,-0.007923761690353899,-0.017265327560358527),vec3(-0.0023193231689242495,-0.007363609667412189,-0.016111014032915188),vec3(-0.002150477970316447,-0.006840253416027219,-0.01502500686231705),vec3(-0.001993243594978657,-0.006351429758636433,-0.014003950941382294),vec3(-0.0018468494397243095,-0.005895010992201904,-0.013044599066349954),vec3(-0.0017105772441682716,-0.005468996194229684,-0.012143814567440982),vec3(-0.0015837570621268916,-0.0050715032025820655,-0.011298572863887869),vec3(-0.0014657636132559086,-0.004700761205697936,-0.010505962123640147),vec3(-0.0013560129718921034,-0.004355103886359678,-0.009763183179221696),vec3(-0.0012539595555377642,-0.004032963067987664,-0.009067548827244698),vec3(-0.0011590933801404499,-0.003732862817675913,-0.00841648261905499),vec3(-0.0010709375533892564,-0.003453413964866797,-0.007807517233220275),vec3(-0.0009890459807688297,-0.0031933089987567142,-0.007238292506523384),vec3(-0.0009130012621639455,-0.002951317311279133,-0.006706553188329679),vec3(-0.0008424127594587336,-0.002726280755872182,-0.006210146473284956),vec3(-0.0007769148178819765,-0.002517109495246948,-0.005747019358954837),vec3(-0.0007161651258619422,-0.002322778114066351,-0.005315215867981134),vec3(-0.0006598431999115193,-0.0021423219748559342,-0.004912874168389099),vec3(-0.0006076489826017314,-0.0019748337976268505,-0.004538223620654547),vec3(-0.0005593015430286124,-0.001819460445624252,-0.004189581775882171),vec3(-0.0005145378703601023,-0.001675399901344218,-0.00386535134583253),vec3(-0.00047311175208774605,-0.001541898418511293,-0.003564017162461541),vec3(-0.0004347927295209686,-0.0014182478370946185,-0.003284143142016918),vec3(-0.0003993651238658139,-0.0013037830496811053,-0.003024369266498018));
    vec3 p_x[151] = vec3[](vec3(-0.002137124133264062,-0.0045652949750816674,-0.007944999489624014),vec3(-0.002300156955533428,-0.004908806940460253,-0.008525633362203022),vec3(-0.0024740180160609526,-0.0052745508615946735,-0.009141755140060714),vec3(-0.0026593024187144994,-0.005663677685852353,-0.009794948578414675),vec3(-0.0028566298846672803,-0.006077377342767512,-0.010486819494099436),vec3(-0.0030666454073479945,-0.006516878738501725,-0.011218991293007716),vec3(-0.0032900199322388997,-0.0069834496606139915,-0.011993099926857181),vec3(-0.0035274510673012965,-0.007478396589859675,-0.012810788233390612),vec3(-0.0037796638307912127,-0.008003064415882877,-0.013673699609992897),vec3(-0.004047411444335345,-0.008558836053786144,-0.014583470965923965),vec3(-0.004331476180377802,-0.009147131958638085,-0.015541724892891462),vec3(-0.00463267027449884,-0.009769409534995865,-0.01655006098737826),vec3(-0.004951836914666999,-0.010427162438453347,-0.017610046250856503),vec3(-0.005289851321237063,-0.011121919766049194,-0.018723204485601174),vec3(-0.005647621933474761,-0.011855245132051603,-0.019891004594067244),vec3(-0.006026091720604429,-0.012628735625135244,-0.02111484767848965),vec3(-0.0064262396378738365,-0.013444020642236373,-0.02239605282424208),vec3(-0.006849082250952507,-0.014302760593353525,-0.02373584143523893),vec3(-0.007295675555175712,-0.015206645470184573,-0.025135319971921),vec3(-0.0077671170197740995,-0.01615739326966947,-0.026595460921701564),vec3(-0.008264547891358274,-0.01715674826113821,-0.02811708180766433),vec3(-0.008789155795641856,-0.0182064790827157,-0.02970082201319753),vec3(-0.009342177681784285,-0.019308376648754934,-0.031347117167421494),vec3(-0.00992490315993552,-0.02046425184516241,-0.03305617079787943),vec3(-0.010538678289711445,-0.021675932983309377,-0.03482792291202483),vec3(-0.011184909885595262,-0.022945262975493885,-0.036662015116369455),vec3(-0.011865070414853763,-0.024274096185269295,-0.03855775182034264),vec3(-0.01258070357473496,-0.025664294893927936,-0.040514056999271006),vec3(-0.013333430648782561,-0.02711772530945963,-0.042529425905391674),vec3(-0.01412495775744369,-0.028636253025678345,-0.04460187101503015),vec3(-0.014957084136223639,-0.030221737816030457,-0.04672886138108205),vec3(-0.01583171159602881,-0.031876027617728446,-0.04890725441919534),vec3(-0.016750855345744924,-0.03360095152585251,-0.051133218989284704),vec3(-0.01771665638740403,-0.03539831157208638,-0.0534021484360339),vec3(-0.018731395730599044,-0.03726987300644707,-0.055708562016558806),vec3(-0.019797510716488533,-0.0392173527296855,-0.05804599286274144),vec3(-0.020917613794531456,-0.04124240543505191,-0.06040686029055546),vec3(-0.02209451415920214,-0.043346606905710634,-0.06278232386756345),vec3(-0.023331242732158364,-0.045531433771570005,-0.06516211616871623),vec3(-0.024631081071251157,-0.04779823884785927,-0.06753435057251908),vec3(-0.02599759490599191,-0.05014822094579368,-0.06988529975365311),vec3(-0.02743467314560627,-0.05258238774762672,-0.07219913968862342),vec3(-0.028946573388404322,-0.05510150995351754,-0.07445765297670581),vec3(-0.03053797519013025,-0.05770606440794075,-0.07663988405127538),vec3(-0.03221404263776634,-0.0603961632607468,-0.07872173737035694),vec3(-0.033980498142029227,-0.06317146536031418,-0.08067550787447306),vec3(-0.035843709830794705,-0.06603106494154067,-0.08246933081865009),vec3(-0.03781079552984435,-0.0689733511594637,-0.08406653544718634),vec3(-0.03988974710170204,-0.07199582998922682,-0.08542488379813157),vec3(-0.042089579940160424,-0.07509489726503166,-0.08649567210767374),vec3(-0.044420513774308154,-0.0782655478780229,-0.0872226677458533),vec3(-0.046894192744119025,-0.0815010009801992,-0.08754084929384119),vec3(-0.04952395514522684,-0.08479221383871625,-0.08737491127852688),vec3(-0.052325166556904924,-0.08812724684632407,-0.0866374883735367),vec3(-0.05531563463635159,-0.09149042775415551,-0.08522704702612285),vec3(-0.05851613023760088,-0.09486124236558141,-0.0830253865465934),vec3(-0.06195104853165206,-0.09821284847174405,-0.0798946889222969),vec3(-0.06564925675340012,-0.10151006459451122,-0.07567406140614437),vec3(-0.06964519409927278,-0.1047066168725813,-0.07017553687965904),vec3(-0.07398031737724835,-0.10774132259935602,-0.06317955075217202),vec3(-0.0787050285543731,-0.11053272464789683,-0.054430032064937746),vec3(-0.08388128622421473,-0.11297142788834234,-0.0436294939520135),vec3(-0.08958620751898465,-0.1149089568361548,-0.03043501110604133),vec3(-0.09591713730219445,-0.11614122538666281,-0.014456986894158296),vec3(-0.10299894760598283,-0.11638344275691273,0.004735321117216877),vec3(-0.11099482783905858,-0.11523099884933204,0.027593267367261202),vec3(-0.12012272629777299,-0.1120965987947297,0.054535916053032214),vec3(-0.13068130718798532,-0.10610553463683693,0.08582732348023998),vec3(-0.1430926878027598,-0.09591364237278834,0.12130023611322262),vec3(-0.15797644463814361,-0.07937430036055836,0.15972950012046713),vec3(-0.17628590900450966,-0.05289019694443883,0.1973636686263485),vec3(-0.19957914170178254,-0.010050447660950014,0.22428672225770246),vec3(-0.23061230582092926,0.06152568473621712,0.21465485784075855),vec3(-0.2748091985304043,0.18844826204726473,0.09754352781288295),vec3(-0.3445128239871801,0.4367329316543684,-0.34280145660982947),vec3(3.01878863857207e-17,2.1657391732312188e-17,2.1073950360688117e-17),vec3(0.3445128239871802,-0.4367329316543684,0.3428014566098292),vec3(0.27480919853040436,-0.18844826204726473,-0.09754352781288317),vec3(0.23061230582092923,-0.061525684736217036,-0.21465485784075863),vec3(0.19957914170178256,0.01005044766095007,-0.2242867222577026),vec3(0.17628590900450966,0.0528901969444389,-0.19736366862634852),vec3(0.15797644463814361,0.07937430036055836,-0.15972950012046708),vec3(0.14309268780275983,0.09591364237278836,-0.1213002361132226),vec3(0.13068130718798535,0.10610553463683695,-0.08582732348023968),vec3(0.12012272629777299,0.11209659879472973,-0.054535916053032235),vec3(0.11099482783905856,0.11523099884933206,-0.027593267367261167),vec3(0.10299894760598281,0.11638344275691272,-0.004735321117216822),vec3(0.09591713730219444,0.11614122538666284,0.014456986894158358),vec3(0.08958620751898468,0.11490895683615483,0.03043501110604141),vec3(0.08388128622421473,0.11297142788834233,0.04362949395201367),vec3(0.0787050285543731,0.11053272464789683,0.054430032064937794),vec3(0.07398031737724835,0.10774132259935604,0.06317955075217199),vec3(0.06964519409927278,0.10470661687258127,0.07017553687965912),vec3(0.06564925675340012,0.10151006459451126,0.07567406140614436),vec3(0.06195104853165206,0.09821284847174405,0.0798946889222969),vec3(0.05851613023760088,0.09486124236558141,0.08302538654659344),vec3(0.05531563463635159,0.09149042775415551,0.08522704702612297),vec3(0.05232516655690491,0.08812724684632407,0.08663748837353664),vec3(0.04952395514522684,0.08479221383871625,0.08737491127852687),vec3(0.046894192744119025,0.08150100098019919,0.0875408492938412),vec3(0.044420513774308154,0.0782655478780229,0.0872226677458533),vec3(0.04208957994016041,0.07509489726503163,0.08649567210767378),vec3(0.03988974710170204,0.07199582998922681,0.08542488379813157),vec3(0.037810795529844336,0.0689733511594637,0.08406653544718634),vec3(0.03584370983079468,0.06603106494154064,0.0824693308186501),vec3(0.03398049814202922,0.06317146536031418,0.08067550787447306),vec3(0.03221404263776634,0.0603961632607468,0.07872173737035698),vec3(0.03053797519013025,0.05770606440794075,0.07663988405127538),vec3(0.02894657338840431,0.055101509953517515,0.07445765297670578),vec3(0.02743467314560627,0.05258238774762672,0.07219913968862343),vec3(0.025997594905991905,0.05014822094579368,0.06988529975365308),vec3(0.024631081071251146,0.04779823884785925,0.06753435057251908),vec3(0.02333124273215836,0.04553143377157,0.06516211616871614),vec3(0.022094514159202137,0.04334660690571063,0.06278232386756344),vec3(0.020917613794531453,0.041242405435051886,0.060406860290555434),vec3(0.019797510716488522,0.039217352729685476,0.05804599286274142),vec3(0.01873139573059904,0.03726987300644707,0.05570856201655882),vec3(0.017716656387404026,0.035398311572086366,0.05340214843603387),vec3(0.016750855345744917,0.0336009515258525,0.051133218989284704),vec3(0.015831711596028804,0.03187602761772843,0.04890725441919533),vec3(0.01495708413622363,0.030221737816030454,0.04672886138108203),vec3(0.014124957757443688,0.028636253025678342,0.04460187101503013),vec3(0.013333430648782552,0.02711772530945961,0.04252942590539163),vec3(0.012580703574734953,0.025664294893927922,0.040514056999270985),vec3(0.011865070414853759,0.024274096185269285,0.03855775182034264),vec3(0.011184909885595262,0.022945262975493878,0.036662015116369455),vec3(0.010538678289711445,0.02167593298330937,0.0348279229120248),vec3(0.009924903159935513,0.020464251845162408,0.03305617079787941),vec3(0.00934217768178428,0.019308376648754923,0.03134711716742148),vec3(0.00878915579564185,0.01820647908271569,0.029700822013197525),vec3(0.008264547891358267,0.017156748261138197,0.028117081807664302),vec3(0.007767117019774098,0.016157393269669456,0.026595460921701543),vec3(0.0072956755551757,0.015206645470184556,0.02513531997192098),vec3(0.0068490822509524995,0.014302760593353517,0.023735841435238908),vec3(0.006426239637873832,0.013444020642236359,0.022396052824242063),vec3(0.006026091720604427,0.012628735625135236,0.02111484767848964),vec3(0.0056476219334747595,0.011855245132051592,0.019891004594067244),vec3(0.005289851321237059,0.01112191976604919,0.018723204485601167),vec3(0.0049518369146669934,0.010427162438453336,0.017610046250856482),vec3(0.004632670274498837,0.009769409534995856,0.016550060987378257),vec3(0.004331476180377796,0.009147131958638074,0.015541724892891445),vec3(0.004047411444335342,0.008558836053786137,0.014583470965923953),vec3(0.0037796638307912088,0.008003064415882869,0.013673699609992889),vec3(0.0035274510673012917,0.007478396589859665,0.012810788233390602),vec3(0.003290019932238895,0.006983449660613981,0.011993099926857178),vec3(0.0030666454073479924,0.006516878738501718,0.011218991293007703),vec3(0.002856629884667277,0.006077377342767509,0.01048681949409938),vec3(0.0026593024187144963,0.005663677685852356,0.00979494857841503),vec3(0.0024740180160610133,0.0052745508615947585,0.009141755140060302),vec3(0.0023001569555331914,0.004908806940460417,0.00852563336220343),vec3(0.0021371241332640567,0.0045652949750816605,0.007944999489623995));
    float s_i[3] = float[](0.2424503566193514,0.10170785486914974,0.03723335168874466);
    float g_x[151] = float[](-0.012893115592183313,-0.013698670060406285,-0.01454270853033401,-0.015426186340698034,-0.01635001813564313,-0.01731507233271321,-0.018322165451038992,-0.01937205631983224,-0.020465440189183726,-0.021602942767020888,-0.022785114207886127,-0.024012423080935617,-0.025285250346210764,-0.02660388336978562,-0.02796851000982456,-0.029379212806879423,-0.030835963312895207,-0.03233861659436575,-0.03388690594586321,-0.03548043785074925,-0.03711868722624115,-0.03880099299014412,-0.04052655398645646,-0.0422944253066969,-0.04410351504318662,-0.04595258150963188,-0.047840230963193016,-0.0497649158607877,-0.0517249336806611,-0.05371842633826146,-0.05574338022319264,-0.057797626881478534,-0.059878844364578875,-0.06198455926355096,-0.06411214944346842,-0.06625884748970856,-0.068421744874013,-0.07059779684434063,-0.07278382803848439,-0.07497653881724314,-0.07717251230864798,-0.0793682221503752,-0.08156004091305374,-0.08374424918274104,-0.0859170452764106,-0.08807455555991833,-0.09021284533361659,-0.09232793024659676,-0.0944157881965094,-0.09647237166805361,-0.09849362045958987,-0.10047547474393566,-0.10241388840629213,-0.10430484259943712,-0.10614435945385038,-0.10792851587831802,-0.109653457384833,-0.11131541187027436,-0.11291070328643685,-0.11443576512950307,-0.11588715368001017,-0.1172615609247814,-0.11855582709315698,-0.11976695274118251,-0.12089211031918369,-0.12192865516037081,-0.12287413583076501,-0.12372630378379504,-0.12448312226638165,-0.12514277442715682,-0.1257036705816586,-0.12616445459385237,-0.126524009338131,-0.12678146121101333,-0.12693618366704282,-0.1269877997588608,-0.12693618366704285,-0.12678146121101333,-0.12652400933813102,-0.12616445459385237,-0.12570367058165863,-0.12514277442715685,-0.12448312226638174,-0.12372630378379511,-0.12287413583076504,-0.12192865516037085,-0.1208921103191837,-0.11976695274118256,-0.11855582709315705,-0.11726156092478153,-0.11588715368001025,-0.11443576512950313,-0.1129107032864369,-0.11131541187027441,-0.10965345738483308,-0.10792851587831806,-0.10614435945385046,-0.10430484259943716,-0.10241388840629222,-0.1004754747439357,-0.09849362045958993,-0.09647237166805367,-0.09441578819650946,-0.09232793024659687,-0.09021284533361669,-0.08807455555991844,-0.08591704527641067,-0.08374424918274113,-0.08156004091305383,-0.07936822215037528,-0.07717251230864812,-0.07497653881724321,-0.07278382803848449,-0.07059779684434074,-0.06842174487401309,-0.06625884748970862,-0.06411214944346849,-0.06198455926355104,-0.05987884436457899,-0.057797626881478596,-0.05574338022319272,-0.05371842633826156,-0.05172493368066117,-0.04976491586078776,-0.047840230963193085,-0.04595258150963194,-0.04410351504318669,-0.04229442530669699,-0.040526553986456555,-0.03880099299014417,-0.0371186872262412,-0.03548043785074932,-0.033886905945863265,-0.03233861659436582,-0.03083596331289527,-0.029379212806879485,-0.02796851000982462,-0.026603883369785666,-0.02528525034621083,-0.02401242308093568,-0.022785114207886165,-0.02160294276702093,-0.020465440189183767,-0.019372056319832277,-0.018322165451039044,-0.017315072332713243,-0.016350018135643175,-0.015426186340698074,-0.014542708530334052,-0.013698670060406317,-0.012893115592183577);

    
    #define RANGE 75
    
    vec2 P1 = vec2(0);
    vec2 P2 = vec2(0);
    vec2 P3 = vec2(0);
    float G = 0.0;
    float Gw = 0.0;
    for (int i = -RANGE; i <= RANGE; i++) {
        int index = RANGE + i;

        vec2 t = Po(i,0).xy;
        float g = Go(i,0);
        
        vec3 py = p_y[index];
        vec3 px = p_x[index];
        
        P1 += vec2(px.x, py.x) * t;
        P2 += vec2(px.y, py.y) * t;
        P3 += vec2(px.z, py.z) * t;
        
        Gw += abs(g_x[index]);
        G  += abs(g_x[index]) * g;
    }
    
    G /= Gw;
    
    if(reset()) {
        fragColor = vec4(0);
    } else {
        fragColor = vec4(pack2x16(P1),pack2x16(P2),pack2x16(P3), G);
    }

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
bool reset() {
    return iFrame <= 1 || texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec4 Po(int m, int n) {
    vec2 ouv = uv + texel * vec2(m,n);
    if (BoundsCheck(ouv)) {
        return vec4(pack2x16(vec2(0)),pack2x16(vec2(0)),pack2x16(vec2(0)),0);
    } else {
        return textureLod(iChannel0, ouv, 0.);
    }
}

float Go(int m, int n) {
    vec2 ouv = uv + texel * vec2(m,n);
    if (BoundsCheck(ouv)) {
        return 0.0;
    } else {
        return textureLod(iChannel1, ouv, 0.0).x;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    initialize(fragCoord, iFrame, iResolution);
    
    
    vec3 p_y[151] = vec3[](vec3(-0.00039936512386562484,-0.0013037830496811509,-0.003024369266497462),vec3(-0.00043479272952107184,-0.001418247837094988,-0.0032841431420175815),vec3(-0.00047311175208775147,-0.0015418984185112672,-0.0035640171624620187),vec3(-0.0005145378703601011,-0.0016753999013442086,-0.003865351345832546),vec3(-0.0005593015430286112,-0.00181946044562425,-0.004189581775882141),vec3(-0.0006076489826017296,-0.001974833797626846,-0.004538223620654535),vec3(-0.0006598431999115183,-0.002142321974855929,-0.004912874168389093),vec3(-0.0007161651258619417,-0.0023227781140663494,-0.005315215867981123),vec3(-0.0007769148178819725,-0.0025171094952469407,-0.00574701935895483),vec3(-0.0008424127594587307,-0.002726280755872174,-0.0062101464732849465),vec3(-0.0009130012621639432,-0.002951317311279131,-0.006706553188329675),vec3(-0.0009890459807688295,-0.003193308998756704,-0.007238292506523368),vec3(-0.0010709375533892544,-0.0034534139648667857,-0.007807517233220254),vec3(-0.001159093380140449,-0.003732862817675909,-0.008416482619054973),vec3(-0.0012539595555377625,-0.004032963067987648,-0.009067548827244675),vec3(-0.0013560129718921017,-0.004355103886359663,-0.009763183179221676),vec3(-0.0014657636132559054,-0.004700761205697934,-0.010505962123640127),vec3(-0.0015837570621268875,-0.0050715032025820585,-0.011298572863887848),vec3(-0.001710577244168271,-0.005468996194229675,-0.01214381456744096),vec3(-0.0018468494397243054,-0.005895010992201898,-0.01304459906634993),vec3(-0.0019932435949786592,-0.006351429758636426,-0.014003950941382284),vec3(-0.0021504779703164487,-0.006840253416027218,-0.015025006862317022),vec3(-0.0023193231689242525,-0.007363609667412167,-0.016111014032915174),vec3(-0.002500606595033479,-0.007923761690353888,-0.017265327560358507),vec3(-0.002695217398648073,-0.008523117575376556,-0.018491406534454295),vec3(-0.002904111972299025,-0.009164240587639544,-0.019792808560418648),vec3(-0.0031283200755794476,-0.00984986033967067,-0.021173182439071275),vec3(-0.003368951675232755,-0.01058288497304438,-0.022636258627945997),vec3(-0.0036272046027564153,-0.01136641445806573,-0.02418583704385647),vec3(-0.0039043731482907218,-0.012203755132905171,-0.025825771679025524),vec3(-0.004201857729535902,-0.013098435617310009,-0.027559951395473018),vec3(-0.00452117579826187,-0.01405422425106731,-0.029392276131583135),vec3(-0.00486397417548751,-0.015075148223856253,-0.0313266275951637),vec3(-0.005232043040666073,-0.016165514581003473,-0.03336683332200887),vec3(-0.00562733184154224,-0.017329933308865297,-0.03551662273933928),vec3(-0.0060519674414129645,-0.018573342723915864,-0.037779573578614306),vec3(-0.006508274881439065,-0.01990103741075051,-0.040159046618245986),vec3(-0.006998801210110017,-0.021318698975493443,-0.0426581062860751),vec3(-0.007526342923405148,-0.022832429901508428,-0.04527942409155754),vec3(-0.008093977672062791,-0.024448790812264518,-0.048025161159515134),vec3(-0.008705101032420682,-0.026174841459272888,-0.05089682526368315),vec3(-0.009363469312043276,-0.028018185757524506,-0.05389509666073895),vec3(-0.010073249580677117,-0.029987021181332632,-0.05701961564096375),vec3(-0.01083907839404583,-0.03209019280176492,-0.06026872295763385),vec3(-0.011666131030046848,-0.03433725218081176,-0.0636391420650306),vec3(-0.012560203507485332,-0.03673852121908325,-0.06712558924087116),vec3(-0.013527810238371974,-0.03930516085637037,-0.07072029400179898),vec3(-0.014576300919535715,-0.04204924420785956,-0.07441240748450588),vec3(-0.015714001257992345,-0.04498383322316428,-0.07818727031588318),vec3(-0.016950383431117873,-0.04812305719009568,-0.08202550346745388),vec3(-0.018296273925712297,-0.051482190232598785,-0.08590187504816119),vec3(-0.01976410874874513,-0.05507772316147769,-0.0897838820674849),vec3(-0.02136824920173702,-0.05892742230067349,-0.09362996770370406),vec3(-0.02312537581916243,-0.06305036371943105,-0.09738726988920325),vec3(-0.025054984222680283,-0.06746692484105196,-0.10098876378585503),vec3(-0.02718001534382911,-0.07219870536957633,-0.10434961578501721),vec3(-0.02952766495752478,-0.07726833375162516,-0.10736250560624028),vec3(-0.03213043568796663,-0.08269909044537457,-0.10989158977713533),vec3(-0.03502752169832732,-0.08851423913888098,-0.11176466592987913),vec3(-0.03826665722222464,-0.09473589144745072,-0.11276294193733785),vec3(-0.041906623421512364,-0.10138312131867837,-0.11260760327593745),vec3(-0.04602070835716154,-0.10846885934496983,-0.11094209169468648),vec3(-0.050701578024441495,-0.1159947726771164,-0.10730865033138975),vec3(-0.05606828993205902,-0.12394275393722222,-0.10111727591157052),vec3(-0.06227665518595623,-0.13226056303078285,-0.09160486258770702),vec3(-0.06953501099043398,-0.14083708737623918,-0.077782417479591),vec3(-0.07812908130749861,-0.149458503599576,-0.058369977232929894),vec3(-0.08846282054472075,-0.15772775580732623,-0.031725854990398726),vec3(-0.10112895949827036,-0.1649097516191365,0.004199908847829801),vec3(-0.11703860203562352,-0.16961595360355963,0.05178318985988212),vec3(-0.13767855574664672,-0.16911182932471974,0.11316073490042394),vec3(-0.16567645398825667,-0.15763971441508162,0.18793345990415217),vec3(-0.2062192509611813,-0.12179330842214287,0.2642830614724927),vec3(-0.2713074094027817,-0.02537420724641723,0.28182569573805677),vec3(-0.395760620255607,0.2520387049261719,-0.04827629938430805),vec3(-0.4794057541719356,0.45941736433304614,-0.35998402950121244),vec3(-0.39576062025560704,0.25203870492617186,-0.04827629938430784),vec3(-0.27130740940278164,-0.02537420724641726,0.2818256957380568),vec3(-0.20621925096118135,-0.12179330842214289,0.2642830614724926),vec3(-0.16567645398825667,-0.15763971441508165,0.18793345990415228),vec3(-0.13767855574664675,-0.1691118293247198,0.1131607349004239),vec3(-0.11703860203562354,-0.1696159536035597,0.05178318985988209),vec3(-0.10112895949827036,-0.1649097516191365,0.004199908847829665),vec3(-0.0884628205447208,-0.1577277558073263,-0.03172585499039877),vec3(-0.07812908130749864,-0.14945850359957605,-0.05836997723292993),vec3(-0.06953501099043398,-0.14083708737623918,-0.07778241747959105),vec3(-0.06227665518595623,-0.13226056303078287,-0.09160486258770703),vec3(-0.05606828993205902,-0.12394275393722226,-0.10111727591157058),vec3(-0.050701578024441495,-0.11599477267711644,-0.10730865033138978),vec3(-0.04602070835716156,-0.10846885934496989,-0.1109420916946865),vec3(-0.04190662342151239,-0.10138312131867841,-0.11260760327593748),vec3(-0.03826665722222467,-0.09473589144745076,-0.11276294193733784),vec3(-0.03502752169832737,-0.08851423913888105,-0.1117646659298792),vec3(-0.032130435687966606,-0.0826990904453746,-0.10989158977713538),vec3(-0.029527664957524794,-0.0772683337516252,-0.10736250560624033),vec3(-0.027180015343829116,-0.07219870536957636,-0.1043496157850173),vec3(-0.025054984222680304,-0.06746692484105203,-0.10098876378585511),vec3(-0.023125375819162436,-0.06305036371943108,-0.09738726988920338),vec3(-0.02136824920173702,-0.058927422300673514,-0.09362996770370416),vec3(-0.019764108748745155,-0.05507772316147772,-0.08978388206748496),vec3(-0.01829627392571232,-0.05148219023259884,-0.08590187504816127),vec3(-0.01695038343111789,-0.04812305719009571,-0.08202550346745398),vec3(-0.015714001257992355,-0.0449838332231643,-0.07818727031588321),vec3(-0.014576300919535724,-0.04204924420785958,-0.07441240748450599),vec3(-0.013527810238371971,-0.039305160856370404,-0.07072029400179902),vec3(-0.012560203507485332,-0.036738521219083255,-0.06712558924087117),vec3(-0.011666131030046859,-0.03433725218081179,-0.0636391420650307),vec3(-0.01083907839404584,-0.03209019280176495,-0.06026872295763393),vec3(-0.010073249580677119,-0.029987021181332653,-0.05701961564096378),vec3(-0.009363469312043281,-0.02801818575752456,-0.05389509666073898),vec3(-0.008705101032420694,-0.026174841459272933,-0.0508968252636832),vec3(-0.008093977672062803,-0.024448790812264518,-0.04802516115951517),vec3(-0.007526342923405146,-0.02283242990150845,-0.04527942409155761),vec3(-0.006998801210110013,-0.021318698975493443,-0.04265810628607512),vec3(-0.006508274881439066,-0.01990103741075051,-0.040159046618246),vec3(-0.00605196744141298,-0.018573342723915892,-0.03777957357861436),vec3(-0.005627331841542247,-0.017329933308865328,-0.03551662273933936),vec3(-0.005232043040666078,-0.016165514581003487,-0.033366833322008904),vec3(-0.004863974175487524,-0.015075148223856267,-0.031326627595163734),vec3(-0.004521175798261876,-0.01405422425106733,-0.029392276131583166),vec3(-0.004201857729535902,-0.013098435617310021,-0.027559951395473042),vec3(-0.0039043731482907213,-0.012203755132905178,-0.025825771679025535),vec3(-0.003627204602756424,-0.01136641445806575,-0.024185837043856497),vec3(-0.003368951675232753,-0.010582884973044387,-0.022636258627946024),vec3(-0.0031283200755794494,-0.009849860339670675,-0.021173182439071295),vec3(-0.002904111972299031,-0.009164240587639563,-0.019792808560418675),vec3(-0.002695217398648074,-0.00852311757537658,-0.01849140653445433),vec3(-0.002500606595033485,-0.007923761690353899,-0.017265327560358527),vec3(-0.0023193231689242495,-0.007363609667412189,-0.016111014032915188),vec3(-0.002150477970316447,-0.006840253416027219,-0.01502500686231705),vec3(-0.001993243594978657,-0.006351429758636433,-0.014003950941382294),vec3(-0.0018468494397243095,-0.005895010992201904,-0.013044599066349954),vec3(-0.0017105772441682716,-0.005468996194229684,-0.012143814567440982),vec3(-0.0015837570621268916,-0.0050715032025820655,-0.011298572863887869),vec3(-0.0014657636132559086,-0.004700761205697936,-0.010505962123640147),vec3(-0.0013560129718921034,-0.004355103886359678,-0.009763183179221696),vec3(-0.0012539595555377642,-0.004032963067987664,-0.009067548827244698),vec3(-0.0011590933801404499,-0.003732862817675913,-0.00841648261905499),vec3(-0.0010709375533892564,-0.003453413964866797,-0.007807517233220275),vec3(-0.0009890459807688297,-0.0031933089987567142,-0.007238292506523384),vec3(-0.0009130012621639455,-0.002951317311279133,-0.006706553188329679),vec3(-0.0008424127594587336,-0.002726280755872182,-0.006210146473284956),vec3(-0.0007769148178819765,-0.002517109495246948,-0.005747019358954837),vec3(-0.0007161651258619422,-0.002322778114066351,-0.005315215867981134),vec3(-0.0006598431999115193,-0.0021423219748559342,-0.004912874168389099),vec3(-0.0006076489826017314,-0.0019748337976268505,-0.004538223620654547),vec3(-0.0005593015430286124,-0.001819460445624252,-0.004189581775882171),vec3(-0.0005145378703601023,-0.001675399901344218,-0.00386535134583253),vec3(-0.00047311175208774605,-0.001541898418511293,-0.003564017162461541),vec3(-0.0004347927295209686,-0.0014182478370946185,-0.003284143142016918),vec3(-0.0003993651238658139,-0.0013037830496811053,-0.003024369266498018));
    vec3 p_x[151] = vec3[](vec3(-0.002137124133264062,-0.0045652949750816674,-0.007944999489624014),vec3(-0.002300156955533428,-0.004908806940460253,-0.008525633362203022),vec3(-0.0024740180160609526,-0.0052745508615946735,-0.009141755140060714),vec3(-0.0026593024187144994,-0.005663677685852353,-0.009794948578414675),vec3(-0.0028566298846672803,-0.006077377342767512,-0.010486819494099436),vec3(-0.0030666454073479945,-0.006516878738501725,-0.011218991293007716),vec3(-0.0032900199322388997,-0.0069834496606139915,-0.011993099926857181),vec3(-0.0035274510673012965,-0.007478396589859675,-0.012810788233390612),vec3(-0.0037796638307912127,-0.008003064415882877,-0.013673699609992897),vec3(-0.004047411444335345,-0.008558836053786144,-0.014583470965923965),vec3(-0.004331476180377802,-0.009147131958638085,-0.015541724892891462),vec3(-0.00463267027449884,-0.009769409534995865,-0.01655006098737826),vec3(-0.004951836914666999,-0.010427162438453347,-0.017610046250856503),vec3(-0.005289851321237063,-0.011121919766049194,-0.018723204485601174),vec3(-0.005647621933474761,-0.011855245132051603,-0.019891004594067244),vec3(-0.006026091720604429,-0.012628735625135244,-0.02111484767848965),vec3(-0.0064262396378738365,-0.013444020642236373,-0.02239605282424208),vec3(-0.006849082250952507,-0.014302760593353525,-0.02373584143523893),vec3(-0.007295675555175712,-0.015206645470184573,-0.025135319971921),vec3(-0.0077671170197740995,-0.01615739326966947,-0.026595460921701564),vec3(-0.008264547891358274,-0.01715674826113821,-0.02811708180766433),vec3(-0.008789155795641856,-0.0182064790827157,-0.02970082201319753),vec3(-0.009342177681784285,-0.019308376648754934,-0.031347117167421494),vec3(-0.00992490315993552,-0.02046425184516241,-0.03305617079787943),vec3(-0.010538678289711445,-0.021675932983309377,-0.03482792291202483),vec3(-0.011184909885595262,-0.022945262975493885,-0.036662015116369455),vec3(-0.011865070414853763,-0.024274096185269295,-0.03855775182034264),vec3(-0.01258070357473496,-0.025664294893927936,-0.040514056999271006),vec3(-0.013333430648782561,-0.02711772530945963,-0.042529425905391674),vec3(-0.01412495775744369,-0.028636253025678345,-0.04460187101503015),vec3(-0.014957084136223639,-0.030221737816030457,-0.04672886138108205),vec3(-0.01583171159602881,-0.031876027617728446,-0.04890725441919534),vec3(-0.016750855345744924,-0.03360095152585251,-0.051133218989284704),vec3(-0.01771665638740403,-0.03539831157208638,-0.0534021484360339),vec3(-0.018731395730599044,-0.03726987300644707,-0.055708562016558806),vec3(-0.019797510716488533,-0.0392173527296855,-0.05804599286274144),vec3(-0.020917613794531456,-0.04124240543505191,-0.06040686029055546),vec3(-0.02209451415920214,-0.043346606905710634,-0.06278232386756345),vec3(-0.023331242732158364,-0.045531433771570005,-0.06516211616871623),vec3(-0.024631081071251157,-0.04779823884785927,-0.06753435057251908),vec3(-0.02599759490599191,-0.05014822094579368,-0.06988529975365311),vec3(-0.02743467314560627,-0.05258238774762672,-0.07219913968862342),vec3(-0.028946573388404322,-0.05510150995351754,-0.07445765297670581),vec3(-0.03053797519013025,-0.05770606440794075,-0.07663988405127538),vec3(-0.03221404263776634,-0.0603961632607468,-0.07872173737035694),vec3(-0.033980498142029227,-0.06317146536031418,-0.08067550787447306),vec3(-0.035843709830794705,-0.06603106494154067,-0.08246933081865009),vec3(-0.03781079552984435,-0.0689733511594637,-0.08406653544718634),vec3(-0.03988974710170204,-0.07199582998922682,-0.08542488379813157),vec3(-0.042089579940160424,-0.07509489726503166,-0.08649567210767374),vec3(-0.044420513774308154,-0.0782655478780229,-0.0872226677458533),vec3(-0.046894192744119025,-0.0815010009801992,-0.08754084929384119),vec3(-0.04952395514522684,-0.08479221383871625,-0.08737491127852688),vec3(-0.052325166556904924,-0.08812724684632407,-0.0866374883735367),vec3(-0.05531563463635159,-0.09149042775415551,-0.08522704702612285),vec3(-0.05851613023760088,-0.09486124236558141,-0.0830253865465934),vec3(-0.06195104853165206,-0.09821284847174405,-0.0798946889222969),vec3(-0.06564925675340012,-0.10151006459451122,-0.07567406140614437),vec3(-0.06964519409927278,-0.1047066168725813,-0.07017553687965904),vec3(-0.07398031737724835,-0.10774132259935602,-0.06317955075217202),vec3(-0.0787050285543731,-0.11053272464789683,-0.054430032064937746),vec3(-0.08388128622421473,-0.11297142788834234,-0.0436294939520135),vec3(-0.08958620751898465,-0.1149089568361548,-0.03043501110604133),vec3(-0.09591713730219445,-0.11614122538666281,-0.014456986894158296),vec3(-0.10299894760598283,-0.11638344275691273,0.004735321117216877),vec3(-0.11099482783905858,-0.11523099884933204,0.027593267367261202),vec3(-0.12012272629777299,-0.1120965987947297,0.054535916053032214),vec3(-0.13068130718798532,-0.10610553463683693,0.08582732348023998),vec3(-0.1430926878027598,-0.09591364237278834,0.12130023611322262),vec3(-0.15797644463814361,-0.07937430036055836,0.15972950012046713),vec3(-0.17628590900450966,-0.05289019694443883,0.1973636686263485),vec3(-0.19957914170178254,-0.010050447660950014,0.22428672225770246),vec3(-0.23061230582092926,0.06152568473621712,0.21465485784075855),vec3(-0.2748091985304043,0.18844826204726473,0.09754352781288295),vec3(-0.3445128239871801,0.4367329316543684,-0.34280145660982947),vec3(3.01878863857207e-17,2.1657391732312188e-17,2.1073950360688117e-17),vec3(0.3445128239871802,-0.4367329316543684,0.3428014566098292),vec3(0.27480919853040436,-0.18844826204726473,-0.09754352781288317),vec3(0.23061230582092923,-0.061525684736217036,-0.21465485784075863),vec3(0.19957914170178256,0.01005044766095007,-0.2242867222577026),vec3(0.17628590900450966,0.0528901969444389,-0.19736366862634852),vec3(0.15797644463814361,0.07937430036055836,-0.15972950012046708),vec3(0.14309268780275983,0.09591364237278836,-0.1213002361132226),vec3(0.13068130718798535,0.10610553463683695,-0.08582732348023968),vec3(0.12012272629777299,0.11209659879472973,-0.054535916053032235),vec3(0.11099482783905856,0.11523099884933206,-0.027593267367261167),vec3(0.10299894760598281,0.11638344275691272,-0.004735321117216822),vec3(0.09591713730219444,0.11614122538666284,0.014456986894158358),vec3(0.08958620751898468,0.11490895683615483,0.03043501110604141),vec3(0.08388128622421473,0.11297142788834233,0.04362949395201367),vec3(0.0787050285543731,0.11053272464789683,0.054430032064937794),vec3(0.07398031737724835,0.10774132259935604,0.06317955075217199),vec3(0.06964519409927278,0.10470661687258127,0.07017553687965912),vec3(0.06564925675340012,0.10151006459451126,0.07567406140614436),vec3(0.06195104853165206,0.09821284847174405,0.0798946889222969),vec3(0.05851613023760088,0.09486124236558141,0.08302538654659344),vec3(0.05531563463635159,0.09149042775415551,0.08522704702612297),vec3(0.05232516655690491,0.08812724684632407,0.08663748837353664),vec3(0.04952395514522684,0.08479221383871625,0.08737491127852687),vec3(0.046894192744119025,0.08150100098019919,0.0875408492938412),vec3(0.044420513774308154,0.0782655478780229,0.0872226677458533),vec3(0.04208957994016041,0.07509489726503163,0.08649567210767378),vec3(0.03988974710170204,0.07199582998922681,0.08542488379813157),vec3(0.037810795529844336,0.0689733511594637,0.08406653544718634),vec3(0.03584370983079468,0.06603106494154064,0.0824693308186501),vec3(0.03398049814202922,0.06317146536031418,0.08067550787447306),vec3(0.03221404263776634,0.0603961632607468,0.07872173737035698),vec3(0.03053797519013025,0.05770606440794075,0.07663988405127538),vec3(0.02894657338840431,0.055101509953517515,0.07445765297670578),vec3(0.02743467314560627,0.05258238774762672,0.07219913968862343),vec3(0.025997594905991905,0.05014822094579368,0.06988529975365308),vec3(0.024631081071251146,0.04779823884785925,0.06753435057251908),vec3(0.02333124273215836,0.04553143377157,0.06516211616871614),vec3(0.022094514159202137,0.04334660690571063,0.06278232386756344),vec3(0.020917613794531453,0.041242405435051886,0.060406860290555434),vec3(0.019797510716488522,0.039217352729685476,0.05804599286274142),vec3(0.01873139573059904,0.03726987300644707,0.05570856201655882),vec3(0.017716656387404026,0.035398311572086366,0.05340214843603387),vec3(0.016750855345744917,0.0336009515258525,0.051133218989284704),vec3(0.015831711596028804,0.03187602761772843,0.04890725441919533),vec3(0.01495708413622363,0.030221737816030454,0.04672886138108203),vec3(0.014124957757443688,0.028636253025678342,0.04460187101503013),vec3(0.013333430648782552,0.02711772530945961,0.04252942590539163),vec3(0.012580703574734953,0.025664294893927922,0.040514056999270985),vec3(0.011865070414853759,0.024274096185269285,0.03855775182034264),vec3(0.011184909885595262,0.022945262975493878,0.036662015116369455),vec3(0.010538678289711445,0.02167593298330937,0.0348279229120248),vec3(0.009924903159935513,0.020464251845162408,0.03305617079787941),vec3(0.00934217768178428,0.019308376648754923,0.03134711716742148),vec3(0.00878915579564185,0.01820647908271569,0.029700822013197525),vec3(0.008264547891358267,0.017156748261138197,0.028117081807664302),vec3(0.007767117019774098,0.016157393269669456,0.026595460921701543),vec3(0.0072956755551757,0.015206645470184556,0.02513531997192098),vec3(0.0068490822509524995,0.014302760593353517,0.023735841435238908),vec3(0.006426239637873832,0.013444020642236359,0.022396052824242063),vec3(0.006026091720604427,0.012628735625135236,0.02111484767848964),vec3(0.0056476219334747595,0.011855245132051592,0.019891004594067244),vec3(0.005289851321237059,0.01112191976604919,0.018723204485601167),vec3(0.0049518369146669934,0.010427162438453336,0.017610046250856482),vec3(0.004632670274498837,0.009769409534995856,0.016550060987378257),vec3(0.004331476180377796,0.009147131958638074,0.015541724892891445),vec3(0.004047411444335342,0.008558836053786137,0.014583470965923953),vec3(0.0037796638307912088,0.008003064415882869,0.013673699609992889),vec3(0.0035274510673012917,0.007478396589859665,0.012810788233390602),vec3(0.003290019932238895,0.006983449660613981,0.011993099926857178),vec3(0.0030666454073479924,0.006516878738501718,0.011218991293007703),vec3(0.002856629884667277,0.006077377342767509,0.01048681949409938),vec3(0.0026593024187144963,0.005663677685852356,0.00979494857841503),vec3(0.0024740180160610133,0.0052745508615947585,0.009141755140060302),vec3(0.0023001569555331914,0.004908806940460417,0.00852563336220343),vec3(0.0021371241332640567,0.0045652949750816605,0.007944999489623995));
    float s_i[3] = float[](0.2424503566193514,0.10170785486914974,0.03723335168874466);
    float g_x[151] = float[](-0.012893115592183313,-0.013698670060406285,-0.01454270853033401,-0.015426186340698034,-0.01635001813564313,-0.01731507233271321,-0.018322165451038992,-0.01937205631983224,-0.020465440189183726,-0.021602942767020888,-0.022785114207886127,-0.024012423080935617,-0.025285250346210764,-0.02660388336978562,-0.02796851000982456,-0.029379212806879423,-0.030835963312895207,-0.03233861659436575,-0.03388690594586321,-0.03548043785074925,-0.03711868722624115,-0.03880099299014412,-0.04052655398645646,-0.0422944253066969,-0.04410351504318662,-0.04595258150963188,-0.047840230963193016,-0.0497649158607877,-0.0517249336806611,-0.05371842633826146,-0.05574338022319264,-0.057797626881478534,-0.059878844364578875,-0.06198455926355096,-0.06411214944346842,-0.06625884748970856,-0.068421744874013,-0.07059779684434063,-0.07278382803848439,-0.07497653881724314,-0.07717251230864798,-0.0793682221503752,-0.08156004091305374,-0.08374424918274104,-0.0859170452764106,-0.08807455555991833,-0.09021284533361659,-0.09232793024659676,-0.0944157881965094,-0.09647237166805361,-0.09849362045958987,-0.10047547474393566,-0.10241388840629213,-0.10430484259943712,-0.10614435945385038,-0.10792851587831802,-0.109653457384833,-0.11131541187027436,-0.11291070328643685,-0.11443576512950307,-0.11588715368001017,-0.1172615609247814,-0.11855582709315698,-0.11976695274118251,-0.12089211031918369,-0.12192865516037081,-0.12287413583076501,-0.12372630378379504,-0.12448312226638165,-0.12514277442715682,-0.1257036705816586,-0.12616445459385237,-0.126524009338131,-0.12678146121101333,-0.12693618366704282,-0.1269877997588608,-0.12693618366704285,-0.12678146121101333,-0.12652400933813102,-0.12616445459385237,-0.12570367058165863,-0.12514277442715685,-0.12448312226638174,-0.12372630378379511,-0.12287413583076504,-0.12192865516037085,-0.1208921103191837,-0.11976695274118256,-0.11855582709315705,-0.11726156092478153,-0.11588715368001025,-0.11443576512950313,-0.1129107032864369,-0.11131541187027441,-0.10965345738483308,-0.10792851587831806,-0.10614435945385046,-0.10430484259943716,-0.10241388840629222,-0.1004754747439357,-0.09849362045958993,-0.09647237166805367,-0.09441578819650946,-0.09232793024659687,-0.09021284533361669,-0.08807455555991844,-0.08591704527641067,-0.08374424918274113,-0.08156004091305383,-0.07936822215037528,-0.07717251230864812,-0.07497653881724321,-0.07278382803848449,-0.07059779684434074,-0.06842174487401309,-0.06625884748970862,-0.06411214944346849,-0.06198455926355104,-0.05987884436457899,-0.057797626881478596,-0.05574338022319272,-0.05371842633826156,-0.05172493368066117,-0.04976491586078776,-0.047840230963193085,-0.04595258150963194,-0.04410351504318669,-0.04229442530669699,-0.040526553986456555,-0.03880099299014417,-0.0371186872262412,-0.03548043785074932,-0.033886905945863265,-0.03233861659436582,-0.03083596331289527,-0.029379212806879485,-0.02796851000982462,-0.026603883369785666,-0.02528525034621083,-0.02401242308093568,-0.022785114207886165,-0.02160294276702093,-0.020465440189183767,-0.019372056319832277,-0.018322165451039044,-0.017315072332713243,-0.016350018135643175,-0.015426186340698074,-0.014542708530334052,-0.013698670060406317,-0.012893115592183577);


    #define RANGE 75
    
    vec2 P = vec2(0);
    float G = 0.0;
    float Gw = 0.0;
    for (int i = -RANGE; i <= RANGE; i++) {
        int index = RANGE + i;
        
        vec4 tx = Po(0,i);
        vec2 t1 = unpack2x16(tx.x);
        vec2 t2 = unpack2x16(tx.y);
        vec2 t3 = unpack2x16(tx.z);

        float g = tx.w;
        
        vec3 py = p_y[index];
        vec3 px = p_x[index];
        
        P += s_i[0] * vec2(px.x, py.x).yx * t1;
        P += s_i[1] * vec2(px.y, py.y).yx * t2;
        P += s_i[2] * vec2(px.z, py.z).yx * t3;
        Gw += abs(g_x[index]);
        G  += abs(g_x[index]) * g;
    }
    
    G /= Gw;

    if(reset()) {
        fragColor = vec4(0);
    } else {
        vec2 com_n;
        if (FRAME_MOD(0)) {
            com_n = com(fragCoord, iChannel2, iChannel1);
        } else {
            com_n = textureLod(iChannel1, uv, 0.0).zw;
        }
        fragColor = vec4(vec2((P.x + P.y) + G),com_n);
    }

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
bool reset() {
    return iFrame <= 1 || texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

void mainImage( out vec4 c, in vec2 p )
{
    initialize(p, iFrame, iResolution);
    Vec4Neighborhood pn = GetVec4Neighborhood(iChannel0);
    vec4 U = texelFetch(iChannel1, ivec2(p), 0);
    vec2 dp = Delta(pn, 0);
    c = U + vec4(dp,0,0)/2.0;
    
    if (reset()) {
        c = vec4(0,0,INIT_MASS,0);
    }
}