

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//-----------------------------------------------------
// Created by sebastien durand - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------

// Lightening, essentially based on one of incredible TekF shaders:
// https://www.shadertoy.com/view/lslXRj

//-----------------------------------------------------


// Change this to improve quality (3 is good)

#define ANTIALIASING 1

#define iTime (1.5*iTime)

// consts
const float tau = 6.2831853;
const float phi = 1.61803398875;

// Isosurface Renderer
const int g_traceLimit=240;
const float g_traceSize=.004;


const vec3 g_boxSize = vec3(.4);

const vec3 g_ptOnBody = vec3(g_boxSize.x*.5, g_boxSize.y*.15, g_boxSize.z*.5); 
const vec3 g_ptOnBody2 = vec3(g_boxSize.x*.5, -g_boxSize.y*.5, -g_boxSize.z*.5); 

// Data to read in Buf A
vec3 g_posBox;
mat3 g_rotBox;

vec3 g_envBrightness = vec3(.5,.6,.9); // Global ambiant color
vec3 g_lightPos1, g_lightPos2;
vec3 g_vConnexionPos, g_posFix; 
vec3 g_vConnexionPos2;
const vec3 g_posFix2 = vec3(0.,1.,0.);
float g_rSpring, g_rSpring2;
bool g_WithSpring2;

// -----------------------------------------------------------------


float hash( float n ) { return fract(sin(n)*43758.5453123); }

// ---------------------------------------------

// Distance from ray to point
float dista(vec3 ro, vec3 rd, vec3 p) {
	return length(cross(p-ro,rd));
}

// Intersection ray / sphere
bool intersectSphere(in vec3 ro, in vec3 rd, in vec3 c, in float r, out float t0, out float t1) {
    ro -= c;
	float b = dot(rd,ro), d = b*b - dot(ro,ro) + r*r;
    if (d<0.) return false;
	float sd = sqrt(d);
	t0 = max(0., -b - sd);
	t1 = -b + sd;
	return (t1 > 0.);
}


// -- Modeling Primitives ---------------------------------------------------
// [iq] https://www.shadertoy.com/view/lsccR8
float sdfStar5( in vec2 p )
{
    // using reflections
    const vec2 k1 = vec2(0.809016994375, -0.587785252292); // pi/5
    const vec2 k2 = vec2(-k1.x,k1.y);
    p.x = abs(p.x);
    p -= 2.0*max(dot(k1,p),0.0)*k1;
    p -= 2.0*max(dot(k2,p),0.0)*k2;
    // draw triangle
    const vec2 k3 = vec2(0.951056516295,  0.309016994375); // pi/10
    return dot( vec2(abs(p.x)-0.3,p.y), k3);
}

float sdPlane( vec3 p ) {
	return p.y;
}

//const int[] txt = int[] (50,48,49,57,0,2,2,2,2,0);


//----------------------------------------------------------
// Adapted from
//  [iq] https://www.shadertoy.com/view/4lyfzw
//       https://iquilezles.org/articles/distfunctions
//----------------------------------------------------------

float opExtrusion( in vec3 p, in float d )
{
    vec2 w = vec2( d, p.z );
    return max(p.z, min(max(w.x,w.y),0.0) + length(max(w,0.0)));
}

//----------------------------------------------------------
// FONT
//----------------------------------------------------------


//----------------------------------------------------------
// Adapted from
//  [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
//----------------------------------------------------------

float sdFont(vec2 p, int c) {
    vec2 uv = (p + vec2(float(c%16), float(15-c/16)) + .5)/16.;
    return max(max(abs(p.x) - .25, max(p.y - .35, -.38 - p.y)), textureLod(iChannel0, uv, 0.).w - 127./255.);
}

float sdMessage(vec2 p, float scale) { 
    p /= scale;
    float d;
    d = sdFont(p, 50);
    p.x-=.5;
    d = min(d, sdFont(p, 48));
    p.x-=.5;
    d = min(d, sdFont(p, 49));
    p.x-=.5;
    d = min(d, sdFont(p, 57));
    return d*scale;
}


float map(vec3 p) { 

    p.y -=1.2;
    float z = iTime + p.z;
    p.x += .01*cos(z);
	float k = .1*z+.5*cos(z*.2)*(.5+.5*cos(z));
    float c = cos(4.*k);
    float s = sin(4.*k);
    mat2  m = mat2(c,-s,s,c);
    p.xy*=m;
    float sc = .6+.5*cos(z);
    float d2D = sdMessage(p.xy, sc);
    
    vec2 p2 = p.xy-vec2(.5,.75);
    p2 *= m*m;
    sc = 4.+5./sc;
    float sc2 = (.9 +.1*sc)*.5;
    float fstar = sdfStar5((p2+sc2*vec2(.1,.15))*sc)/sc;
     fstar = min(fstar, sdfStar5((p2+sc2*vec2(.1,-.15))*sc)/sc);
     fstar = min(fstar, sdfStar5((p2+sc2*vec2(-.1,0.))*sc)/sc);
    return opExtrusion(p, min(fstar,d2D));
}



//----------------------------------------------------------------------




float isGridLine(vec2 p, vec2 v) {
    vec2 k = smoothstep(.0,1.,abs(mod(p+v*.5, v)-v*.5)/.01);
    return k.x * k.y;
}


// render for color extraction
vec3 colorField(vec3 p) {
    p.y -= 1.2;
    float z = iTime + p.z;
    
    p.x += .01*cos(z);
	float k = .1*z+.5*cos(z*.2)*(.5+.5*cos(z));
    float c = cos(4.*k);
    float s = sin(4.*k);
    mat2  m = mat2(c,-s,s,c);
    p.xy*=m;
    float sc = .6+.5*cos(z);
    
    float d2D = sdMessage(p.xy, sc);
    
    vec2 p2 = p.xy-vec2(.5,.75);
    p2 *= m*m;
    float sc1 = 4.+5./sc;
    float sc2 = (.9 +.1*sc1)*.5;
    float fstar = sdfStar5((p2+sc2*vec2(.1,.15))*sc1)/sc1;
    fstar = min(fstar, sdfStar5((p2+sc2*vec2(.1,-.15))*sc1)/sc1);
    fstar = min(fstar, sdfStar5((p2+sc2*vec2(-.1,0.))*sc1)/sc1);
   
    return (d2D < fstar) ? vec3(p.xy/sc,z) : vec3(.05,2.05,z);
}


// ---------------------------------------------------------------------------

float SmoothMax( float a, float b, float smoothing ) {
	return a-sqrt(smoothing*smoothing + pow(max(.0,a-b),2.0));
}

vec3 Sky( vec3 ray) {
	return g_envBrightness*mix( vec3(.8), vec3(0), exp2(-(1.0/max(ray.y,.01))*vec3(.4,.6,1.0)) );
}

// -------------------------------------------------------------------


vec3 Shade( vec3 pos, vec3 ray, vec3 normal, vec3 lightDir1, vec3 lightDir2, vec3 lightCol1, vec3 lightCol2, float shadowMask1, float shadowMask2, float distance )
{

	vec3 ambient = g_envBrightness*mix( vec3(.2,.27,.4), vec3(.4), (-normal.y*.5+.5) ); // ambient
    
    // ambient occlusion, based on my DF Lighting: https://www.shadertoy.com/view/XdBGW3
	float aoRange = distance/20.0;
	
	float occlusion = max( 0.0, 1.0 - map( pos + normal*aoRange )/aoRange ); // can be > 1.0
	occlusion = exp2( -2.0*pow(occlusion,2.0) ); // tweak the curve
    
	ambient *= occlusion*.8+.2; // reduce occlusion to imply indirect sub surface scattering

	float ndotl1 = max(.0,dot(normal,lightDir1));
	float ndotl2 = max(.0,dot(normal,lightDir2));
    
	float lightCut1 = smoothstep(.0,.1,ndotl1);
	float lightCut2 = smoothstep(.0,.1,ndotl2);

	vec3 light = vec3(0);
    

	light += lightCol1*shadowMask1*ndotl1;
	light += lightCol2*shadowMask2*ndotl2;

    
	// And sub surface scattering too! Because, why not?
    float transmissionRange = distance/10.0; // this really should be constant... right?
    float transmission1 = map( pos + lightDir1*transmissionRange )/transmissionRange;
    float transmission2 = map( pos + lightDir2*transmissionRange )/transmissionRange;
    
    vec3 sslight = lightCol1 * smoothstep(0.0,1.0,transmission1) + 
                   lightCol2 * smoothstep(0.0,1.0,transmission2);
    vec3 subsurface = vec3(1,.8,.5) * sslight;

    float specularity = .012; 
	vec3 h1 = normalize(lightDir1-ray);
	vec3 h2 = normalize(lightDir2-ray);
    
	float specPower;
    specPower = exp2(3.0+5.0*specularity);

    vec3 albedo;

    if (pos.y<-.48) {  
        pos.z+=iTime;
       	float f = mod( floor(2.*pos.z) + floor(2.*pos.x), 2.0);
        albedo = (0.4 + 0.1*f)*vec3(.7,.6,.8);
        albedo *= .2*(.3+.5*isGridLine(pos.xz, vec2(.5)));
      	specPower *= 5.;

    } else {
    	vec3 colorId = colorField(pos);
        vec3 col = colorId.y > .5 ? vec3(.96,.96,0) : 
        			colorId.x < .2 ? vec3(.6,.3,.0) : 
                   colorId.x < .7 ? vec3(.3,.6,.0) : 
        			colorId.x < 1.2 ? vec3(0.,.6,.3) : vec3(0,.3,.6);
        float grid = .7+.3*isGridLine(vec2(colorId.z),vec2(.1))*isGridLine(colorId.xy,vec2(.1));
        albedo = grid * 2.*col; 
    }       
    
	vec3 specular1 = lightCol1*shadowMask1*pow(max(.0,dot(normal,h1))*lightCut1, specPower)*specPower/32.0;
	vec3 specular2 = lightCol2*shadowMask2*pow(max(.0,dot(normal,h2))*lightCut2, specPower)*specPower/32.0;
    
	vec3 rray = reflect(ray,normal);
	vec3 reflection = Sky( rray );
	
	// specular occlusion, adjust the divisor for the gradient we expect
	float specOcclusion = max( 0.0, 1.0 - map( pos + rray*aoRange )/(aoRange*max(.01,dot(rray,normal))) ); // can be > 1.0
	specOcclusion = exp2( -2.0*pow(specOcclusion,2.0) ); // tweak the curve
	
	// prevent sparkles in heavily occluded areas
	specOcclusion *= occlusion;

	reflection *= specOcclusion; // could fire an additional ray for more accurate results
    
	float fresnel = pow( 1.0+dot(normal,ray), 5.0 );
	fresnel = mix( mix( .0, .01, specularity ), mix( .4, 1.0, specularity ), fresnel );

    light += ambient;
	light += subsurface;

    vec3 result = light*albedo;
	result = mix( result, reflection, fresnel );
	result += specular1;
    result += specular2;

	return result;
}


float Trace( vec3 pos, vec3 ray, float traceStart, float traceEnd ) {
    float t0=0.,t1=100.;
    float t2=0.,t3=100.;
    // trace only if intersect bounding spheres
  
	float t = max(traceStart, min(t2,t0));
	traceEnd = min(traceEnd, max(t3,t1));
	float h;
	for( int i=0; i < g_traceLimit; i++) {
		h = map( pos+t*ray );
		if (h < g_traceSize || t > traceEnd)
			return t>traceEnd?100.:t;
		t = t+h*.45;
	}
        
	return 100.0;
}



vec3 Normal( vec3 pos, vec3 ray, float t) {

	float pitch = .2 * t / iResolution.x;   
	pitch = max( pitch, .005 );
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx; // tetrahedral offsets
	vec3 p1 = pos+d.xyy;
	vec3 p2 = pos+d.yxy;
	vec3 p3 = pos+d.yyx;

	float f0 = map(p0), f1 = map(p1), f2 = map(p2),	f3 = map(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}

// Camera
vec3 Ray( float zoom, in vec2 fragCoord) {
	return vec3( fragCoord.xy-iResolution.xy*.5, iResolution.x*zoom );
}


// Camera Effects

void BarrelDistortion( inout vec3 ray, float degree ){
	// would love to get some disperson on this, but that means more rays
	ray.z /= degree;
	ray.z = ( ray.z*ray.z - dot(ray.xy,ray.xy) ); // fisheye
	ray.z = degree*sqrt(ray.z);
}


mat2 matRot(in float a) {
    float ca = cos(a), sa = sin(a);
    return mat2(ca,sa,-sa,ca);
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr) {
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    vec2 m = iMouse.xy/iResolution.y - .5;

 	float time = 15.0 + iTime;

    

// Positon du point lumineux
    float distLightRot =  .7;
                              
    float lt = 3.*(time-1.);
    
   
    g_lightPos1 = g_posBox + distLightRot*vec3(cos(lt*.5), .4+.15*sin(2.*lt), sin(lt*.5));
    g_lightPos2 = g_posBox + distLightRot*vec3(cos(-lt*.5), .4+.15*sin(-2.*lt), sin(-lt*.5));
	
	// Ambiant color
	g_envBrightness = vec3(.6,.65,.9);
    
// intensitee et couleur du point
    vec3 lightCol1 = vec3(1.05,.95,.95)*.5;//*.2*g_envBrightness;
	vec3 lightCol2 = vec3(.95,1.,1.05)*.5;//*.2*g_envBrightness;
	

    
	float lightRange1 = .4, 
          lightRange2 = .4; 
	float traceStart = .2;

    float t, s1, s2;
    
    vec3 col, colorSum = vec3(0.);
	vec3 pos;
    vec3 ro, rd;
	
#if (ANTIALIASING == 1)	
	int i=0;
        vec2 q = (fragCoord.xy)/iResolution.xy;
#else
	for (int i=0;i<ANTIALIASING;i++) {
        float randPix = hash(iTime);
        vec2 subPix = .4*vec2(cos(randPix+6.28*float(i)/float(ANTIALIASING)),
                              sin(randPix+6.28*float(i)/float(ANTIALIASING)));        
    	// camera	
        vec2 q = (fragCoord.xy+subPix)/iResolution.xy;
#endif
        vec2 p = -1.0+2.0*q;
        p.x *= iResolution.x/iResolution.y;

        float dis = 7.*(1.2+.6*cos(.41*iTime)); 
        ro = vec3( dis*cos(.2*time),6.5, dis*sin(.2*time) );
        vec3 ta = vec3( -1., 1., 0. );

        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta, 0.0);

        // ray direction
         rd = ca * normalize( vec3(p.xy,4.5) );

        float tGround = -(ro.y+.5) / rd.y;
        float traceEnd = min(tGround+1.,100.); 
        col = vec3(0);
        vec3 n;
        t = Trace(ro, rd, traceStart, traceEnd);
        if ( t > tGround ) {
            pos = ro + rd*tGround;   
            n = vec3(0,1.,0);
            t = tGround;
        } else {
            pos = ro + rd*t;
            n = Normal(pos, rd, t);
        }

        // Shadows
        vec3 lightDir1 = g_lightPos1-pos;
        float lightIntensity1 = length(lightDir1);
        lightDir1 /= lightIntensity1;
        
        vec3 lightDir2 = g_lightPos2-pos;
        float lightIntensity2 = length(lightDir2);
        lightDir2 /= lightIntensity2;

        s1 = Trace(pos, lightDir1, .04, lightIntensity1 );
        s2 = Trace(pos, lightDir2, .01, lightIntensity2 );

        lightIntensity1 = lightRange1/(.1+lightIntensity1*lightIntensity1);
        lightIntensity2 = lightRange2/(.1+lightIntensity2*lightIntensity2);

        col = Shade(pos, rd, n, lightDir1, lightDir2, lightCol1*lightIntensity1, lightCol2*lightIntensity2,
                    (s1<40.0)?0.0:1.0, (s2<40.0)?0.0:1.0, t );

#if (ANTIALIASING > 1)	
        colorSum += col;
	}
    
    col = colorSum/float(ANTIALIASING);
#endif
    
    // fog
    float f = 100.0;
    col = mix( vec3(.8), col, exp2(-t*vec3(.4,.6,1.0)/f) );
    
    // Draw light
    s1 = .5*max(dista(ro, rd, g_lightPos1)+.05,0.);
    float dist = .5*length(g_lightPos1-ro);
    if (dist < t*.5) {
        vec3 col1 = 2.*lightCol1*exp( -.03*dist*dist );
        float BloomFalloff = 50000.;
        col += col1*col1/(1.+s1*s1*s1*BloomFalloff);
    }

    s2 = .5*max(dista(ro, rd, g_lightPos2)+.05,0.);
    dist = .5*length(g_lightPos2-ro);
    if (dist < t*.5) {
        vec3 col2 = 2.*lightCol2*exp( -.03*dist*dist );
        float BloomFalloff = 50000.;
        col += col2*col2/(1.+s2*s2*s2*BloomFalloff);
    }
        
    // Compress bright colours, (because bloom vanishes in vignette)
    vec3 c = (col-1.0);
    c = sqrt(c*c+.05); // soft abs
    col = mix(col,1.0-c,.48); // .5 = never saturate, .0 = linear
	
	// compress bright colours
	float l = max(col.x,max(col.y,col.z));//dot(col,normalize(vec3(2,4,1)));
	l = max(l,.01); // prevent div by zero, darker colours will have no curve
	float l2 = SmoothMax(l,1.0,.01);
	col *= l2/l;
    
	fragColor =  vec4(pow(col,vec3(1./2.)),1);
}
