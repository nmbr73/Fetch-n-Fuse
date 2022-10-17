

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by sebastien durand - 01/2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// *****************************************************************************
// Add 2 rotations to [iq]  https://www.shadertoy.com/view/3ld3DM
// See also           [dr2] https://www.shadertoy.com/view/3l3GD7
// *****************************************************************************

// Buf B: Calculate distance to scene
// Image: DOF post processing


#define WITH_DOF
#define WITH_CONE_TEST


#ifdef WITH_DOF

const float aperture = 2.;

const float cosAngle = cos(radians(aperture/2.));
const float GA = 2.399;  // golden angle = 2pi/(1+phi)
const mat2 rot = mat2(cos(GA),sin(GA),-sin(GA),cos(GA));
    
bool inCone(vec3 p, vec3 o, vec3 n, float side) {
	return side*dot(normalize(o-p), n) >= cosAngle;
}

//--------------------------------------------------------------------------
// eiffie's code for calculating the aperture size for a given distance...
float coc(float t) {
	return max(t*.08, (2./iResolution.y) * (1.+t));
}

vec3 RD(const vec2 q) {
    return normalize(vec3((2.* q.x - 1.) * iResolution.x/iResolution.y,  (2.* q.y - 1.), 2.));
}

vec3 dof(sampler2D tex, vec2 uv, float fdist) {
    
    const float amount = 1.;
	vec4 colMain = texture(tex, uv);
    
    fdist = min(30., fdist);
    float rad = min(.3, coc(abs(colMain.w-fdist))),//.3; // TODO calculate this for Max distance on picture
    	  r=6.;
    
    vec3 cn = RD(uv),    // Cone axis    
         co = cn*fdist,  // Cone origin
         sum = vec3(0.),  
     	 bokeh = vec3(1),
         acc = vec3(0),
         pixPos;
    vec2 pixScreen,
         pixel = 1./iResolution.xy,        
         angle = vec2(0, rad);
    vec4 pixCol;
    
    bool isInCone = false;
	for (int j=0;j<32;j++) {  
        r += 1./r;
	    angle *= rot;
        pixScreen = uv + pixel*(r-1.)*angle; // Neighbourg Pixel
        pixCol = texture(tex, pixScreen);    // Color of pixel (w is depth)      
        pixPos = pixCol.w * RD(pixScreen);   // Position of 3D point in camera base
#ifdef WITH_CONE_TEST
        if (inCone(pixPos, co, cn, sign(fdist - pixCol.w))) 
#endif            
        {        // true if the point is effectivelly in the cone
            bokeh = pow(pixCol.xyz, vec3(9.)) * amount +.1;
            acc += pixCol.xyz * bokeh;			
            sum += bokeh;
            isInCone = true;
        }
	}
        
 	return (!isInCone) ? colMain.xyz : // Enable to deal with problem of precision when at thin begining of the cone
       acc.xyz/sum;
}


void mainImage(out vec4 fragColor,in vec2 fragCoord) {
	vec2 r = iResolution.xy, m = iMouse.xy / r,
	     q = fragCoord.xy/r.xy;
    
    // Animation
 	float anim = .1*iTime,
   	   aCam = 10. + 4.*anim + 8.*m.x;

    // Camera
	vec3 ro = 1.5*vec3(cos(aCam), 1.2, .2 + sin(aCam));
			
  	// DOF
    float fdist = length(ro-vec3(0,.3,0));
    vec3 c = dof(iChannel0,q,fdist); 
    
    // Vigneting
    c *= pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y), .32); 
    
    fragColor = vec4(c,1.);
}

#else 


void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec4 c = texture(iChannel0,uv);
    c *= pow(16.*uv.x*uv.y*(1.-uv.x)*(1.-uv.y), .5); // Vigneting
	fragColor = c; //*.01; 
}

#endif
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Add second rotation to [iq]  https://www.shadertoy.com/view/3ld3DM
// See also               [dr2] https://www.shadertoy.com/view/3l3GD7

// -------------------------------
// Choose shapes
// -------------------------------

//#define SD2D(uv,w) sdBox(uv, w)
#define SD2D(uv,w) sdRoundedX(uv, w.x, w.y)
//#define SD2D(uv,w) sdCircle(uv, w)
//#define SD2D(uv,w) sdBox2(uv, w)

//#define SD3D(uv,w) sdBox3(uv, vec3(w.x,h,w.x))
#define SD3D(uv,w) .8*sdMessage3D(-uv.yzx+vec3(l+.24,0,0), txt,.5,w.x)

// -------------------------------
// Render options
// -------------------------------

#define WITH_TEXTURE
#define WITH_BUMPMAP
#define WITH_GRIB
//#define WITH_AA

// -------------------------------
#define GROUND 0
#define BIDULE 1
// -------------------------------

mat2 rot(float a) {
    float ca = cos(a), sa = sin(a);
    return mat2( ca, -sa, sa, ca );
}


// --------------------------------------------------------
// Inspired by [iq] https://www.shadertoy.com/view/3ld3DM
// --------------------------------------------------------

vec3 opCurveSpace( in vec3 p, in float h, in vec3 r, out vec2 q, out float ra) {
    float s = sign(r.x);
    if (s*r.x<.001) r.x = .001;
    if (abs(r.y)<.001) r.y = .001;
    if (abs(r.z)<.001) r.z = .001;
    vec2 sc = vec2(sin(r.x),cos(r.x)); // could de precalculated
    mat2 rot2 = rot(r.y);            // could de precalculated
    ra = .5*h/r.x;           // Distance
    p.xz *= rot2;          // Apply 2nd rotation
    p.x -= ra;             // Recenter
    q = p.xy - 2.*sc*max(0.,dot(sc,p.xy));  // Reflect
    vec3 uvw = vec3(ra-length(q)*s,         // New space coordinates 
                    ra*atan(s*p.y,-s*p.x),
                    p.z);
    uvw.zx *= rot(r.y+r.z*(uvw.y/h));        // Inverse 2nd rotation
    return uvw;
}


// -- Text ------------------------------------------------
// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
// --------------------------------------------------------

//int[] gtxt = int[] (83,80,65,67,69);
int[] gtxt1 = int[] (32,74,105,80,105,32);
int[] gtxt2 = int[] (110,109,98,114,55,51);

float sdFont(in vec2 p, in int c) {
    vec2 uv = (p + vec2(float(c%16), float(15-c/16)) + .5)/16.;
    return max(max(abs(p.x) - .25, max(p.y - .35, -.38 - p.y)), textureLod(iChannel2, uv, 0.).w - 127./255.);
}

float sdMessage2D(in vec2 p, in int[6] txt) { 
 	float d = 99., w = .45; // letter width  
    for (int id = 0; id<6; id++){
    	d = min(d, sdFont(p, txt[id]));   
    	p.x -= w; 
    }
    return d-.015; //0.015
}

float sdMessage3D(in vec3 p, in int[6] txt, in float scale, in float h) { 
    return opExtrussion(p, sdMessage2D(p.xy/scale, txt)*scale, h);
}

// Based on [iq] https://www.shadertoy.com/view/3ld3DM
vec4 sdJoint3D( in vec3 p, in float l, in vec3 rot, in vec2 w,in int[6] txt) {
    vec2 q; float ra;
    vec3 uvw = opCurveSpace(p, l, rot, q, ra);
#ifdef SD3D   
    float d = SD3D(uvw, w);
#else // 
    // 2D Profile
    float ww = 1.2*max(w.x,w.y);
    float dTop = length(vec2(q.x+ra-clamp(q.x+ra,-ww,ww), q.y))*sign(-q.y);
    
    // Profile  
    float d = SD2D(uvw.xz, w);
	d = max(dTop, d);
#endif
    return vec4(d, uvw );
}

vec4 sdJoint3DSphere( in vec3 p, in float h, in vec3 rot, in float w) {   
    vec2 q; float ra; // only use in 2D
    vec3 uvw = opCurveSpace(p, h, rot, q, ra);
    float d = sdVerticalCapsule(uvw, h, w);
 	return vec4(d, uvw );
}

// --------------------------------------------------------
//   The Scene
// --------------------------------------------------------

vec4 map4( in vec3 pos ) {
    float a = 1.*sin(iTime*1.5), b = 1.3*sin(iTime*2.5);
    //vec4 d1 = sdJoint3DSphere(pos-vec3(0.,0., .4), .8, vec3(b,a,a*b), .2 ),
    vec4 d1 = sdJoint3D(pos-vec3(0.,0., .4), 1.1, vec3(b,a,a*b),vec2(.2,.04), gtxt1 ),
         d2 = sdJoint3D(pos-vec3(0.,0.,-.4), .8, vec3(a,b,1.5*sin(a+iTime*2.1)), vec2(.2,.04), gtxt2 );
    d1.w += .4;
    d2.w -= .4;
    return d1.x<d2.x ? d1 : d2; // Without ground
}

float map(in vec3 p) {
    return min(map4(p).x, p.y); // Distance with ground
}

// --------------------------------------
// Shading Tools
// --------------------------------------

vec3 normal(in vec3 p, in vec3 ray, in float t) {
	float pitch = .4 * t / iResolution.x;
    vec2 d = vec2(-1,1) * pitch;
	vec3 p0 = p+d.xxx, p1 = p+d.xyy, p2 = p+d.yxy, p3 = p+d.yyx;
	float f0 = map(p0), f1 = map(p1), f2 = map(p2), f3 = map(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - p*(f0+f1+f2+f3);
	return normalize(grad - max(.0, dot(grad,ray))*ray);
}

float SoftShadow(in vec3 ro, in vec3 rd) {
    float r = 1., h, t = .005+hash13(ro)*.02, dt = .01;
    for(int i=0; i<48; i++ ) {
		h = map4(ro + rd*t).x;
		r = min(r, 3.*h/t);
		t += dt;
        dt += .0015;
        if (h<1e-4) break;
    }
    return clamp(r, 0., 1.);
}

float CalcAO(in vec3 p, in vec3 n) {
    float d, h=.01, a=.0, s=1.;
    for(int i=0; i<4; i++) {
        d = map(n * h + p);
        a += (h-d)*s;
        s *= .8;
        h += .03;
    }
    return clamp(1.-4.*a, 0., 1.);
}

float isGridLine(vec3 p, vec3 v) {
    vec3 k = smoothstep(.2,.8,abs(mod(p+v*.5, v)-v*.5)/.01);
    return k.x * k.y * k.z;
}

// See https://iquilezles.org/articles/palettes for more information
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d ) {
    return a + b*cos( 6.28318*(c*t+d) );
}

#ifdef WITH_BUMPMAP
//----------------------------------
// Texture 3D
//----------------------------------
// Need to be in UVW space !
vec3 normalUVW(in vec3 p, in vec3 n, in float t) {   
	float pitch = .4 * t / iResolution.x;
    return normalize(map4(p+n*pitch).yzw - map4(p).yzw);
}

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){  
    n = max(n*n, .001);
    n /= n.x + n.y + n.z;  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, in vec3 nUVW, float bf){   
    const vec2 e = vec2(.001, 0);
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, nUVW), tex3D(tx, p - e.yxy, nUVW), tex3D(tx, p - e.yyx, nUVW));
    vec3 g = vec3(.299, .587, .114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , nUVW), vec3(.299, .587, .114)) )/e.x; 
    g -= nUVW*dot(nUVW, g);
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
}
#endif

//----------------------------------
// Shading
//----------------------------------

vec3 render(in vec3 ro, in vec3 rd, in float res, in vec3 pos, in vec3 n, in vec3 cobj, in vec3 light) {
    float 
         ao = CalcAO(pos, n),
    	 sh = SoftShadow( pos, light),
         amb = clamp(.5+.5*n.y, .0, 1.),
         dif = sh*clamp(dot( n, light ), 0., 1.),
         pp = clamp(dot(reflect(-light,n), -rd),0.,1.),
         fre = (.7+.3*dif)* ao*pow( clamp(1.+dot(n,rd),0.,1.), 2.);
    vec3 brdf = ao*.5*(amb)+ sh*ao*1.*dif*vec3(1.,.9,.7)*vec3(1.,.25,.05),
         sp = sh*5.*pow(pp,9.)*vec3(1., .6, .2),
	     col = cobj*(brdf + sp) + fre*(.5*cobj+.5);
    return mix(.1*vec3(1.,1.,.8),col,2.*dot(n,-rd));
}

// --------------------------------------
// Main
// --------------------------------------

void mainImage2(out vec4 fragColor, in vec2 fragCoord ) {

    vec2 r = iResolution.xy, 
         m = iMouse.xy / r,
	     q = fragCoord.xy/r.xy, pix = q+q-1.;
	pix.x *= r.x/r.y;
    
    float anim = .1*iTime,
     	  tTensionCol = smoothstep(.8,1.2,anim),
		  aCam = 10. + 4.*anim + 8.*m.x;
    
    // Camera
	vec3 ro = 1.5*vec3(cos(aCam), 1.2, .2 + sin(aCam)),
         w = normalize(vec3(0,.3,0) - ro), 
         u = normalize(cross(w, vec3(0,1,0))), 
         v = cross(u, w),
         rd = normalize(pix.x * u + pix.y * v + w+w);
			
    // Ground intersection (faster than ray marching)
    float tg = -ro.y/rd.y; 
    float tmax = min(tg,5.5);
	// Ray marching
    int obj = GROUND;
    float h = .1, t = .01*hash13(q.xyx);
    for(int i=0;i<200;i++) { 
        if (h<5.e-5 || t>tmax) break;
        t += h = map4(ro + rd*t).x;
    }
    if (h<5.e-5) {
        obj = BIDULE;
    } else {
        t = tg;
    }
    
	// Light
    vec3 lightPos = vec3(0.,1.2, .7);
            
    // Calculate color on point
    vec3 pos = ro + t * rd;
    vec3 n = obj == GROUND ? vec3(0,1,0) : normal(pos,rd,t);
    vec3 uvw = obj == GROUND ? pos : map4(pos).yzw; 		
    vec3 cobj = vec3(1.);
	float grib = 1.;
#ifdef WITH_TEXTURE
    float k = hash13(floor((uvw+.15)/.3));
    cobj = pal(k, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,0.5),vec3(0.8,0.90,0.30));
    cobj = mix(vec3(1), sqrt(cobj), step(.5,k));
#endif      
#ifdef WITH_GRIB
    grib = isGridLine(uvw+.15, vec3(.3));
    cobj = mix(vec3(.1),cobj,grib);
#endif
#ifdef WITH_BUMPMAP
    vec3 nuvw = obj == GROUND ? n : normalUVW(pos,n,t);
    n = grib < .7 ? n : doBumpMap(iChannel1, uvw, n, nuvw, .003);
    // keep in visible side
    n = normalize(n - max(.0,dot(n,rd))*rd);
#endif   
 
    // Shading
    vec3 c = render(ro, rd, t, pos, n, cobj, normalize(lightPos-pos));

#ifndef WITH_TEXTURE
    // Add light
    vec3 col = mix(vec3(0.,.25,1.), vec3(1.,.25,0.), smoothstep(-.1,.1,cos(.5*iTime)));
    c += .1*(.03+col/pow(.1+length(pos-vec3(0,.4,0)),1.5));
#endif        

	fragColor = vec4(pow(clamp(c,0.,1.), vec3(.43)), t);	
}


// -- Anti aliasing ----------------------------------

#ifdef WITH_AA
void mainImage(out vec4 O, in vec2 U ) {
    vec4 T;                                     
    for (int k=0; k<4; k++, O+=T)               
        mainImage2(T, U+.25*vec2(k%2-1,k/2-1));  
    O /= 4.;
}
#else
void mainImage(out vec4 O, in vec2 U ) {
    mainImage2(O, U);  
}
#endif
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

float hash13( const in vec3 p ) {
	float h = dot(p,vec3(127.1,311.7,758.5453123));	
    return fract(sin(h)*43758.5453123);
}


// [iq] https://www.shadertoy.com/view/llGSzw
vec3 hash3( uint n ) 
{
    // integer hash copied from Hugo Elias
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    uvec3 k = n * uvec3(n,n*16807U,n*48271U);
    return vec3( k & uvec3(0x7fffffffU))/float(0x7fffffff);
}

float hash1( uint n ) 
{
    // integer hash copied from Hugo Elias
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return float( n & uvec3(0x7fffffffU))/float(0x7fffffff);
}


// --------------------------------------------------------
// [iq] https://iquilezles.org/articles/distfunctions
// --------------------------------------------------------

float opExtrussion( in vec3 p, in float sdf, in float h) {
    vec2 w = vec2(sdf, abs(p.z) - h);
  	return min(max(w.x,w.y),0.) + length(max(w,0.));
}

float sdCircle( in vec2 p, in vec2 w) {
    float d = length(p)- w.x;
    return max(d, -w.y-d);
}

float sdRoundedX( in vec2 p, in float w, in float r ) {
    p = abs(p);
    return length(p-min(p.x+p.y,w)*0.5) - r;
}

float sdVerticalCapsule( vec3 p, float h, float r ) {
  p.y -= clamp( p.y, 0.0, h );
  return length( p ) - r;
}

float sdBox( in vec2 p, in vec2 b ) {
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdBox2( in vec2 p, in vec2 b ) {
    float d = sdBox(p, vec2(.8*b.x))-.01;
    return max(d, -b.y-d);
}

float sdBox3( vec3 p, vec3 b ) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

