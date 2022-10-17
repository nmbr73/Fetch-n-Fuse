

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "HAPPY 2016!" by Martijn Steinrucken aka BigWings - 2015
// countfrolic@gmail.com
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode

// Use these to change the effect

// if you have a kick ass GPU then double both of these
#define NUM_SPARKLES 75.
#define NUM_SUB_SPARKLES 3.

#define SUB_SPARKLE_CHANCE .4
#define PRIMARY_PARTICLE_COLOR vec3(1., 0.8, 0.5)
#define SECONDARY_PARTICLE_COLOR vec3(1., 0.5, 0.3)
#define MOTION_BLUR_AMOUNT 0.04
#define SLOW_MOTION_SPEED .05
#define SLOWMO_CYCLE_DURATION 20.
#define NORMAL_MOTION_SPEED .9
#define DOF vec2(1., 1.5)
#define MIN_CAM_DISTANCE 1.5
#define MAX_CAM_DISTANCE 7.




float CAMERA_DISTANCE;


#define PI 3.1415
#define S(x,y,z) smoothstep(x,y,z)
#define B(x,y,z,w) S(x-z, x+z, w)*S(y+z, y-z, w)
#define saturate(x) clamp(x,0.,1.)
float dist2(vec2 P0, vec2 P1) { vec2 D=P1-P0; return dot(D,D); }
float DistSqr(vec3 a, vec3 b) { vec3 D=a-b; return dot(D, D); } 

const vec3 up = vec3(0.,1.,0.);
const float pi = 3.141592653589793238;
const float twopi = 6.283185307179586;

vec4 Noise401( vec4 x ) { return fract(sin(x)*5346.1764); }
vec4 Noise4( vec4 x ) { return fract(sin(x)*5346.1764)*2. - 1.; }
float Noise101( float x ) { return fract(sin(x)*5346.1764); }

#define MOD3 vec3(.1031,.11369,.13787)
//  3 out, 1 in... DAVE HOSKINS
vec3 hash31(float p) {
   vec3 p3 = fract(vec3(p) * MOD3);
   p3 += dot(p3, p3.yzx + 19.19);
   return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}
float hash12(vec2 p){
	vec3 p3  = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

struct ray {
    vec3 o;
    vec3 d;
};
ray e;				// the eye ray

struct camera {
    vec3 p;			// the position of the camera
    vec3 forward;	// the camera forward vector
    vec3 left;		// the camera left vector
    vec3 up;		// the camera up vector
	
    vec3 center;	// the center of the screen, in world coords
    vec3 i;			// where the current ray intersects the screen, in world coords
    ray ray;		// the current ray: from cam pos, through current uv projected on screen
    vec3 lookAt;	// the lookat point
    float zoom;		// the zoom factor
};
camera cam;

mat4 CamToWorldMatrix(camera c) {
	vec3 x = c.left;
    vec3 y = c.up;
    vec3 z = c.forward;
    vec3 p = c.p;
    
    return mat4( 
        x.x, x.y, x.z, 0.,
        y.x, y.y, y.z, 0.,
        z.x, z.y, z.z, 0.,
        p.x, p.y, p.z, 1.
    );
}
mat4 WorldToCamMatrix(camera c) {
	vec3 x = c.left;
    vec3 y = c.up;
    vec3 z = c.forward;
    vec3 p = c.p;
    
   return mat4( 
        x.x, y.x, z.x, -dot(x, p),
        x.y, y.y, z.y, -dot(y, p),
        x.z, y.z, z.z, -dot(z, p),
         0.,  0.,  0.,          0.
    );
}

void CameraSetup(vec2 uv, vec3 position, vec3 lookAt, float zoom) {
	
    cam.p = position;
    cam.lookAt = lookAt;
    cam.forward = normalize(cam.lookAt-cam.p);
    cam.left = cross(up, cam.forward);
    cam.up = cross(cam.forward, cam.left);
    cam.zoom = zoom;
    
    cam.center = cam.p+cam.forward*cam.zoom;
    cam.i = cam.center+cam.left*uv.x+cam.up*uv.y;
    
    cam.ray.o = cam.p;						// ray origin = camera position
    cam.ray.d = normalize(cam.i-cam.p);	// ray direction is the vector from the cam pos through the point on the imaginary screen
}

float within(vec2 v, float t) {
	return (t-v.x) / (v.y-v.x);
}


vec4 tex3D( in vec3 pos, in vec3 normal, sampler2D sampler ) {
    // by reinder. This is clever as two hamsters feeding three hamsters.
    
	return 	texture( sampler, pos.yz )*abs(normal.x)+ 
			texture( sampler, pos.xz )*abs(normal.y)+ 
			texture( sampler, pos.xy )*abs(normal.z);
}

// DE functions from IQ
// https://www.shadertoy.com/view/Xds3zN

float sdCapsule( vec3 p, vec3 a, vec3 b, float r ) {
	vec3 pa = p-a, ba = b-a;
	float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
	return length( pa - ba*h ) - r;
}

vec4 map( in vec3 p) {
    // returns a vec3 with x = distance, y = bump, z = mat transition w = mat id
    
    float t = iTime*.1;
    vec2 fryInterval = vec2(2., 2.2);
    float transition = saturate(within(fryInterval, p.y));
    transition = smoothstep(0., 1., transition);
    
    vec3 pos = p*3.;
    pos.y -= t;
    
    vec3 normal = normalize(vec3(p.x, 0., p.z));
    
    float newBump = tex3D(pos, normal, iChannel1).x*.003;
    float burnedBump = tex3D(pos, normal, iChannel0).x*.05;
    
    float bump = mix(newBump, burnedBump, transition);
    
    float d = sdCapsule(p+bump*normal, vec3(0., -10., 0.), vec3(0., 10., 0.), .1);
    
    return vec4(d, bump, transition, 2.);
}

vec4 castRay( in vec3 ro, in vec3 rd ) {
    // returns a distance and a material id
    
    float dmin = 1.0;
    float dmax = 20.0;
    
	float precis = 0.002;
    float d = dmin;
    float m = -1.0;
    float b = 0.;
    float t = 0.;
    for( int i=0; i<50; i++ )
    {
	    vec4 res = map( ro+rd*d );
        if( res.x<precis || d>dmax ) break;
        d += res.x;
        b = res.y;
        t = res.z;
	    m = res.w;
    }

    if( d>dmax ) m=-1.0;
    return vec4( d, b, t, m );
}

float calcAO( in vec3 pos, in vec3 nor ) {
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
}


vec3 ClosestPoint(ray r, vec3 p) {
    // returns the closest point on ray r to point p
    return r.o + max(0., dot(p-r.o, r.d))*r.d;
}

vec4 render( in vec3 ro, in vec3 rd, out float d ) {
    // outputs a color
    
    vec3 col = vec3(0.);
    vec4 res = castRay(ro,rd);
    d = res.x;	// distance
    float b = res.y;	// bump
    float t = res.z;	// transition
	float m = res.w;	// mat id
    if( m>0.5 )
    {
        vec3 pos = ro + d*rd;
        vec3 nor = calcNormal( pos );
        vec3 ref = reflect( rd, nor );
        
        // material        
		col = vec3(0.05,0.08,0.10)+mix(.35, .1, t);

        // lighitng        
        float occ = calcAO( pos, nor );
		vec3  lig = normalize( vec3(-0.6, 0.7, -0.5) );
		float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );
        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
        
        float fade = saturate(within(vec2(2., 3.), pos.y));
        fade = smoothstep(0., 1., fade);
        fade = mix(80., 0., fade);
        vec3 afterGlow = pow(abs(b)*fade,2.) * vec3(1., .1, .02)*2.;
        float whiteGlow = B(2.18, 2.45, .05, pos.y+b*10.);

		vec3 lin = vec3(0.0);
        lin += amb;
        lin += dif;
        
		col = col*lin;
        
        col += afterGlow;
		col += whiteGlow;
    }

	return vec4( saturate(col), saturate(m) );
}

float SineWave(vec2 pos, float phase, float frequency, float amplitude, float offset, float thickness, float glow) {
		// returns a sine wave band
    	// takes a position from -pi,-pi to pi, pi
				
    float dist = abs(pos.y-(sin(pos.x*frequency+phase)*amplitude-offset));  // distance to a sine wave
    return smoothstep(thickness+glow, thickness, dist);
}

vec3 background(ray r) {
	float x = atan(r.d.x, r.d.z);		// from -pi to pi	
	float y = pi*0.5-acos(r.d.y);  		// from -1/2pi to 1/2pi		
    
    float t = iTime;
    
	float band1 = SineWave(vec2(x, y), 0., 3., .25, 0., 0.001, .5);
    
    return  mix(vec3(.3, .02, 0.03), vec3(0.), band1);
}


vec3 sparkle(ray r, vec3 p, float size, vec3 color) {
	float camDist = length(cam.p-p);
    float focus = smoothstep(DOF.y, DOF.x, abs(camDist-CAMERA_DISTANCE));
    
    vec3 closestPoint = ClosestPoint(r, p);
    float dist = DistSqr(closestPoint, p)*10000.;
   
    size = mix(size*5., size, focus);
    float brightness = size/dist;
    brightness = clamp(brightness,0., 10.);
    
    float bokeh = smoothstep(.01, .04, brightness)*saturate(dist*.005+.4)*.15;
    
    brightness = mix(bokeh, brightness, focus);
    return color * brightness;
}

vec3 sparkles(ray r, vec2 uv, float time, float timeFactor, float dist) {
	vec3 col = vec3(0.);
    
    float n2 = fract(sin(uv.x*123134.2345)*1231.234255);
    float n3 = fract(sin((n2+uv.y)*234.978)*789.234);
    
    float motionBlur = (n3-.5)*timeFactor*MOTION_BLUR_AMOUNT;
    
    for(float i=0.; i<NUM_SPARKLES; i++) {					
    	float t = time+(i/NUM_SPARKLES) + motionBlur;
        float ft = floor(t);
        t -= ft;
        vec3 n = hash31(i+ft*123.324);			// per particle noise / per cycle noise
        
        
        vec3 pStart = vec3(0., 2.1+n.y*.15, 0.);
        pStart.y -= t*t*.6;	// gravity
        pStart.y += t;		// account for slow scroll down the stick
        
        vec3 pEnd = pStart + (n-.5) * vec3(1., .6, 1.)*4.;
        vec3 p = mix(pStart, pEnd, t);
       	
        if(length(p-cam.p)<dist) {
            float size = mix(10., .5, smoothstep(0., .2, t)); // in the first 20% it gets smaller very fast
            size *= smoothstep(1., .2, t);					// in the remaining 80% it slowly fades out

            if(t>n.z && abs(n.z-.55)<SUB_SPARKLE_CHANCE) {
                for(float x=0.; x<NUM_SUB_SPARKLES; x++) {
                    vec3 ns = hash31(x+i);			// per particle noise
                    vec3 sStart = mix(pStart, pEnd, n.z);
                    vec3 sEnd = sStart + (ns-.5) *2.;
                    float st = saturate(within(vec2(n.z, 1.), t));
                    vec3 sp = mix(sStart, sEnd, st);

                    size = mix(10., 0.5, smoothstep(0., .1, st));	// explosion in the first 10%
                    size *= smoothstep(1., .9, st);					// fade over the next 90%

                    col += sparkle(r, sp, size, SECONDARY_PARTICLE_COLOR);
                }
            } else
                 col += sparkle(r, p, size, PRIMARY_PARTICLE_COLOR);
        }
    }
    
    return col;
}

vec3 Rainbow(vec3 c) {
	
    float t=iTime;
    
    //float avg = (c.r+c.g+c.b)/3.;
    //c = avg + (c-avg)*sin(vec3(0., .333, .666)+t);
    
    c += sin(vec3(.4, .3, .3)*t + vec3(1.1244,3.43215,6.435))*vec3(.4, .1, .5);
    
    return c;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = (fragCoord.xy / iResolution.xy) - 0.5;
   	uv.y *= iResolution.y/iResolution.x;
    vec2 m = iMouse.xy/iResolution.xy;
    
	float t = iTime;
    float timeFactor = fract(t/SLOWMO_CYCLE_DURATION)>.5 ? SLOW_MOTION_SPEED : NORMAL_MOTION_SPEED;
    t *= timeFactor;
    
    float turn = -m.x*pi*2.+iTime*.1;
    float s = sin(turn);
    float c = cos(turn);
    mat3 rot = mat3(	  c,  0., s,
                   		  0., 1., 0.,
                   		  s,  0., -c);
    
    CAMERA_DISTANCE = mix(MIN_CAM_DISTANCE, MAX_CAM_DISTANCE, sin(iTime*.0765)*.5+.5);
    vec3 pos = vec3(0., 0.4, -CAMERA_DISTANCE)*rot;
   	
    CameraSetup(uv, pos, vec3(0., 2.3, 0.), 1.);
    
    vec3 bg = background(cam.ray);
    float dist;										// the distance of the current pixel from the camera
    vec4 stick = render(cam.ray.o, cam.ray.d, dist);
    dist += .08; // add some distance to make sure particles render on top of the stick when they first come to life
    
    vec3 col = mix(bg, stick.rgb, stick.a);	// composite stick onto bg
    
    col += sparkles(cam.ray, uv, t, timeFactor, dist);
    
    col = Rainbow(col);
    
    fragColor = vec4(col, .1);
}