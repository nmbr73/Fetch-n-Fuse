

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Quartz - wip - private" by patu. https://shadertoy.com/view/3tjXRW
// 2021-04-02 00:42:21

vec2 hash( vec2 p ) { p=vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))); return fract(sin(p)*18.5453); }

// return distance, and cell id
vec2 voronoi( in vec2 x )
{
    vec2 n = floor( x );
    vec2 f = fract( x );

	vec3 m = vec3( 8.0 );
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        vec2  g = vec2( float(i), float(j) );
        vec2  o = hash( n + g );
      //vec2  r = g - f + o;
	    vec2  r = g - f + (0.5+0.5*sin(iTime+6.2831*o));
		float d = dot( r, r );
        if( d<m.x )
            m = vec3( d, o );
    }

    return vec2( sqrt(m.x), m.y+m.z );
}


vec3 res;

mat2 rot = mat2(cos(2.399),sin(2.399),-sin(2.399),cos(2.399));

vec3 dof(sampler2D tex,vec2 uv,float rad)
{
	vec3 acc=vec3(0);
    vec2 pixel=vec2(.002*res.y/res.x,.002),angle=vec2(0,rad);;
    rad=1.;
	for (int j=0;j<50;j++)
    {  
        rad += 1./rad;
	    angle*=rot;
        vec4 col=texture(tex,uv+pixel*(rad-1.)*angle);
		acc+=col.xyz;
	}
	return acc/50.;
}

//-------------------------------------------------------------------------------------------
void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    res = iResolution;
	vec2 uv = gl_FragCoord.xy / res.xy;
    vec2 c = voronoi( (14.0+6.0*abs(sin(1.2*iTime))) * uv / res.x * res.y / 4.);
    
    float cell = max(0., floor(abs(sin(iTime)) - fract(c.y * 0.1) + .1));
    //uv += texelFetch(iChannel1, ivec2(gl_FragCoord.xy), 0).b * .04;
    
    //float cell = max(0., floor(abs(sin(iTime + fract(c.y * 0.1) + .9))));
    float sh = (cell / 50.) * texelFetch(iChannel1, ivec2(9, 0), 0).r;//cell > .2 ? c.y * 0.5 : 0.;
    uv.x += sh * 3.;//* hash(uv).x * 2.2;
    
    float a = abs(.7 - pow(texture(iChannel0,uv).w * 1.2, 1.4));
	fragColor=vec4(dof(iChannel0,uv,a),1.);
    fragColor = mix(fragColor, pow(
        max(
            vec4(0.), 
            1.-normalize(fragColor)), vec4(4.)), sh * 40.);
    //fragColor.rbg -= cell * .1;
    //fragColor.rgb -= vec3(.2, .1, 0.) * cell * .5 + (cell > 0.5 ? pow(abs(cell * -c.x * 2.), 10.) : 0.);
    //fragColor = vec4(a);
    
   // fragColor = texelFetch(iChannel1, ivec2(0, 0), 0).aaaa;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PATTERN_TIME (5.485714 / 4.)

vec3 hash33_(vec3 p){     
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}
#define M0 1597334673U
#define M1 3812015801U
#define M2 uvec2(M0, M1)
#define M3 uvec3(M0, M1, 2798796413U)

float hash11( float q )
{
    uvec2 n = uint(q) * M2;
    return float((n.x ^ n.y) * M0) * (1.0/float(0xffffffffU));
}

float hash12( vec2 p ) { uvec2 q = uvec2(ivec2(p)) * M2; uint n = (q.x ^ q.y) * M0; return float(n) * (1./float(0xffffffffU)); }
vec3 hash33(vec3 p) { uvec3 q = uvec3(ivec3(p)) * M3; q = (q.x ^ q.y ^ q.z)*M3; return vec3(q) * (1.0/float(0xffffffffU)); }

float voronoi(vec3 p){
	vec3 b, r, g = floor(p);
	p = fract(p); 
	float d = 1.;      
    for(int j = -1; j <= 1; j++) {
	    for(int i = -1; i <= 1; i++) {    		
		    b = vec3(i, j, -1); r = b - p + hash33(g+b); d = min(d, dot(r,r));    		
		    b.z = 0.0; r = b - p + hash33(g+b); d = min(d, dot(r,r)); 
            b.z = 1.; r = b - p + hash33(g+b); d = min(d, dot(r,r));    			
	    }
	}	
	return d;
}

float noiseLayers(in vec3 p) {
    vec3 t = vec3(0., 0., p.z);    
    float tot = 0., sum = 0., amp = 1.;
    for (int i = 0; i < 2; i++) { tot += voronoi(p + t) * amp; p *= 2.0; t *= 1.5; sum += amp; amp *= 0.5; }    
    return tot / sum;
}

float noiseF( in vec2 p )
{
    vec2 i = floor( p ), f = fract( p ), u = f*f*f*(3.-2.*f);

    return mix( mix( hash12( i + vec2(0.,0.) ), 
                     hash12( i + vec2(1.,0.) ), u.x),
                mix( hash12( i + vec2(0.,1.) ), 
                     hash12( i + vec2(1.,1.) ), u.x), u.y);
}

float noiseFF(in vec2 uv) {
    uv *= 2.;
    
 	mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 ) * 1.25;
    
    float f  = .5*noiseF( uv ); uv = m*uv;
    f += .2500*noiseF( uv ); uv = m*uv;
    f += .1250*noiseF( uv ); uv = m*uv;
    f += .0625*noiseF( uv ); uv = m*uv;   
    
    return f;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define getNormal getNormalHex
#define V vec3
#define W vec2
#define F float
#define FAR 330.
#define INF 1e32
//#define IT iTime
#define mt iChannelTime[1]

#define PI 3.14159265
#define PHI (1.618033988749895)

struct Timeline {
    float songTime;
    float rPatternTime;
    float nPatternTime;
    float smoothNPatternTime;
    
    float patternNum;
} timeline;

struct SdfMixer {
 	float ifs1;
    float grid;        
    float star;
} sdfMixer;

F 
    Z = 0., 
    J = 1., 
	vol = 0.,
	noise = 0.;
 

#define H(P) fract(sin(dot(P,vec2(127.1,311.7)))*43758.545)
F n(in vec3 p) {
    V 	i = floor(p), 
        f = fract(p), 	
	    u = f*f*(3.-f-f);
    
    W   ii = i.xy + i.z * 5. + 5.;
    
    #define II(a,b) H(i.xy + i.z * W(5.0) + W(a,b))
    
    F 	v1 = mix(mix(II(Z,Z),II(J,Z),u.x), mix(
            II(Z,J),II(J,J),u.x), u.y);
    
    #define I2(a,b) H(ii + W(a,b))
    return max(mix(v1,mix(mix(I2(Z,Z),I2(J,Z),u.x), 
        		mix(I2(Z,J),I2(J,J),u.x), 
        		u.y),u.z),Z);
}
#define A w *= .5; s *= 2.; r += w * n(s * x);
F B(vec3 x) {
    F 	r = Z, w = J, s = J;
    A A A A;
    return r;
}

#define fromRGB(a, b, c) vec3(F(a), F(b), F(c)) / 255.;
    
vec3 
    light = vec3(0. ,1., 1.),
	lightDir,
	lightColour = normalize(vec3(0.5, .6, .5) ); 

vec3 saturate(vec3 a) { return clamp(a, 0.0, 1.0); }
vec2 saturate(vec2 a) { return clamp(a, 0.0, 1.0); }
float saturate(float a) { return clamp(a, 0.0, 1.0); }

// Repeat only a few times: from indices <start> to <stop> (similar to above, but more flexible)
float pModInterval1(inout float p, float size, float start, float stop) {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = mod(p+halfsize, size) - halfsize;
	if (c > stop) { //yes, this might not be the best thing numerically.
		p += size*(c - stop);
		c = stop;
	}
	if (c <start) {
		p += size*(c - start);
		c = start;
	}
	return c;
}



float smin( float a, float b, float k )
{
    float res = exp( -k*a ) + exp( -k*b );
    return -log( res )/k ;
}

void pR(inout vec2 p, float a) {
	p = cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

float opU2( float d1, float d2 ) {
    if (d1 < d2) return d1;
    return d2;
}

vec3 opU2( vec3 d1, vec3 d2 ) {
    if (d1.x < d2.x) return d1;
    return d2;
}

struct geometry {
    float dist;
    float materialIndex;
    float specular;
    float diffuse;
    vec3 color;  
    vec3 space;
    float mirror;
    vec3 index;
};

geometry geoU(geometry g1, geometry g2) {
    if (g1.dist < g2.dist) return g1;
    return g2;
}

vec3 opS2( vec3 d1, vec3 d2 ){	
    if (-d2.x > d1.x) return -d2;
    return d1;
}

vec3 opI2( vec3 d1, vec3 d2 ) {
 	if (d1.x > d2.x) return d1;
    return d2;
}

float vmin(vec2 v) {
	return min(v.x, v.y);
}


// Maximum/minumum elements of a vector
float vmax(vec2 v) {
	return max(v.x, v.y);
}

float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
}

float vmax(vec4 v) {
	return max(max(v.x, v.y), max(v.z, v.w));
}

// Sign function that doesn't return 0
float sgn(float x) {
	return (x<0.)?-1.:1.;
}

vec2 sgn(vec2 v) {
	return vec2((v.x<0.)?-1.:1., (v.y<0.)?-1.:1.);
}


// Repeat space along one axis. Use like this to repeat along the x axis:
// <float cell = pMod1(p.x,5);> - using the return value is optional.
float pMod1(inout float p, float size) {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = mod(p + halfsize, size) - halfsize;
	return c;
}


// Repeat in two dimensions
vec2 pMod2(inout vec2 p, vec2 size) {
	vec2 c = floor((p + size*0.5)/size);
	p = mod(p + size*0.5,size) - size*0.5;
	return c;
}
// Repeat around the origin by a fixed angle.
// For easier use, num of repetitions is use to specify the angle.
float pModPolar(inout vec2 p, float repetitions) {
	float angle = 2.*PI/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	float r = length(p);
	float c = floor(a/angle);
	a = mod(a,angle) - angle/2.;
	p = vec2(cos(a), sin(a))*r;
	// For an odd number of repetitions, fix cell index of the cell in -x direction
	// (cell index would be e.g. -5 and 5 in the two halves of the cell):
	if (abs(c) >= (repetitions/2.)) c = abs(c);
	return c;
}

// Mirror at an axis-aligned plane which is at a specified distance <dist> from the origin.
float pMirror (inout float p, float dist) {
	float s = sgn(p);
	p = abs(p)-dist;
	return s;
}

vec2 pMirrorOctant (inout vec2 p, vec2 dist) {
	vec2 s = sgn(p);
	pMirror(p.x, dist.x);
	pMirror(p.y, dist.y);
	if (p.y > p.x)
		p.xy = p.yx;
	return s;
}

vec2 pModMirror2(inout vec2 p, vec2 size) {
	vec2 halfsize = size*0.5;
	vec2 c = floor((p + halfsize)/size);
	p = mod(p + halfsize, size) - halfsize;
	p *= mod(c,vec2(2))*2. - vec2(1.);
	return c;
}

// Box: correct distance to corners
float fBox(vec3 p, vec3 b) {
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}

// Same as above, but in two dimensions (an endless box)
float fBox2Cheap(vec2 p, vec2 b) {
	return vmax(abs(p)-b);
}

float fCross(vec3 p, vec3 size) {
    float obj = fBox(p, size);
    obj = opU2(obj, fBox(p, size.zxy));
    obj = opU2(obj, fBox(p, size.yzx));
               
               return obj;
}


float fSphere(vec3 p, float r) {
	return length(p) - r;
}

mat2x2 rot(float angle)
{
    return mat2x2(cos(angle), -sin(angle),
				  sin(angle), cos(angle));
}
//IFS iterations : try 2 or 3
#define NIFS 4
//scale and translate for the IFS in-loop transformation
#define SCALE 1.2
#define TRANSLATE 4.2
vec3 sd2d(vec2 p, float o)
{
    p *= 1.7;
    float time = o * .2;//0.2*o+0.6*iTime;
 	float s =.45, d, d2 = 1., d3= 0.;
    p*= s;
    float RADIUS =2.5;//(1.+sin(iTime));
    
    int i;
    vec3 col;  
    
    p = p*rot((mod(timeline.patternNum, 4.) > 1. ? -1. : 1.) * -0.9 * time);// twist

    for ( i = 0; i<NIFS; i++)
    {        
        if (p.x < 0.) { p.x = -p.x; col.r++;}
		//p = p*rot(0.1*sin(time));
        if (p.y < 0.) {p.y = -p.y; col.g++; }
        if (p.x-p.y < 0.){ p.xy = p.yx; col.b++;}        
        
      	p = p * SCALE - TRANSLATE;
        //p = p * rot(0.1 * iTime);
        d = 0.25 * (length(p) - RADIUS) * pow(SCALE, float(-i)) / s;

        if (d < 1.1) {
         	d2 = float(i);
            d3 = 1.;
            break;
        }
    }
    
    
    
    //d = fCross(vec3(p.xy, 1.), vec3(1.)) - RADIUS * pow(SCALE, float(-i)) * 2.;
    //col/=float(NIFS);
    //vec3 oc = mix(vec3(0.7,col.g,0.2),vec3(0.2,col.r,0.7), col.b);
    
    return vec3(d, d2, d3);
}

// Cone with correct distances to tip and base circle. Y is up, 0 is in the middle of the base.
float fCone(vec3 p, float radius, float height) {
	vec2 q = vec2(length(p.xz), p.y);
	vec2 tip = q - vec2(0, height);
	vec2 mantleDir = normalize(vec2(height, radius));
	float mantle = dot(tip, mantleDir);
	float d = max(mantle, -q.y);
	float projected = dot(tip, vec2(mantleDir.y, -mantleDir.x));
	
	// distance to tip
	if ((q.y > height) && (projected < 0.)) {
		d = max(d, length(tip));
	}
	
	// distance to base ring
	if ((q.x > radius) && (projected > length(vec2(height, radius)))) {
		d = max(d, length(q - vec2(radius, 0)));
	}
	return d;
}

vec3 opTwist(in vec3 p, float k)
{
    float c = cos(k*p.y);
    float s = sin(k*p.y);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xz,p.y);
    
    return q;
}

vec3 discoBall(vec3 p, float spikes) {
    
    //p -= ballPos;
    
    //p = opTwist(p, .1);
    //p = opTwist(p.yxz, .1);//sin(length(p) * .01));
    
    //pR(p.xz, iTime * .3 - min(1., iTime) * vol * 1.5);
    //pR(p.yz, iTime * .24 - min(1., iTime) * vol * .5);
    
    //dpb = p;
    
    
    float pxz = pModPolar(p.xz, 7.);//ceil(vol2 * 10.));// * hash12(vec2(floor(part2Time * 3.55))) * min(part2Time, 30.));
    float pyz = pModPolar(p.xy, 7.);
    
    //pR(p.xz, length(p) * .01);
    //pR(p.xy, length(p) * .01);
    
    
    return vec3(
        fCone(p.zxy, 4., spikes), 
        8., 16.
    );

}

geometry map(vec3 p) {
    geometry box, fl;
    vec3 bp = p, bp2 = p;
	//p.x *= sin(timeline.smoothNPatternTime + p.x);
    p *= (sin(length(bp)) * .2 - 1.) * (1. + timeline.nPatternTime);
    
    //p += sin(iTime);
    
    //pR(bp.yx, p.y * .2 + sin(iTime));
    
    //pR(bp.xy, PI / 4.);
    pR(bp.xy, timeline.songTime * .02 + n(vec3(iTime * .1)) * 5.);
    //pModPolar(p.xy, 4.);
    //pModPolar(p.xz, 4.);
    vec3 tb = bp;
    vec3 reps = vec3(
        pModInterval1(bp.x, 5., -2., 2.),
        pModInterval1(bp.y, 5., -2., 2.),
        pModInterval1(bp.z, 5., -2., 2.)
    );
    
    box.dist = fSphere(bp, 1.5);
    
    bp = tb;
    
    vec3 reps2 = vec3(
        pModInterval1(bp.x, 5., -1.5, 1.5),
        pModInterval1(bp.y, 5., -1.5, 1.5),
        pModInterval1(bp.z, 5., -1.5, 1.5)
    );
    
    box.dist = mix(box.dist, fBox(bp, vec3(1.5)), mod(timeline.patternNum, 2.));//fSphere(bp, 1.5);
 
	box.diffuse = 4.;
    box.specular = 5.;
    box.mirror = 1.;
    box.color = vec3(0.0);
    
    //pR(p.xz, timeline.songTime * .1);
    //p = mix(p, p.xzy, .9);
    vec3 s = sd2d(p.xz, p.y) * .9;
    
    float a = smin(s.x, box.dist, pow(sin(iTime / 2.) / 2. + .5, 3.) + .8);
    
    box.dist = mix(box.dist, a, sdfMixer.ifs1);    
    
    reps = vec3(s.y);
    
    //box.dist = smin(box.dist, fSphere(p, 5.), abs(sin(iTime * .1)));
    box.index = ceil(reps + (n(reps + timeline.patternNum + timeline.nPatternTime * 10.) - .5) * 3.);
    
    
  	//box.dist = mix(box.dist, fBox(p, vec3(10.)), sin(iTime) / 2. + .5);
    //!!!  box.dist = mix(box.dist, fBox(p, vec3(10.)), 1.-n(vec3(p) * .5));
    //pR(bp2.zx, p.y * .2 + sin(iTime));
    
    //db.yzx = opTwist(db.yzx, sin(length(db)) * .001);
    float di = discoBall(bp2, 25. * (1.-timeline.smoothNPatternTime / 4.)).x;
    box.dist = smin(box.dist, di, .9);
    
   // box.dist = min(box.dist, -(length(bp2) -140.));
    box.dist = mix(fSphere(bp2, 15.), box.dist, min(timeline.songTime * .1, 1.));
    
    p= bp2;
    //p.zxy = opTwist(p.zxy, .03);
    //float m = pModPolar(p.xz, 10.);
    //p.x -= 35. ;//+ sin(p.y * .3+ IT) * 3.;;
    float ss = sd2d(-p.zx * 1.4, p.y).x;
    //p.y += sin(m + IT) * 10.;
    box.dist = mix(ss, box.dist, min(1., timeline.songTime*.1));
    
    return box;
    box.diffuse = 4.;
    box.specular = 5.;
    box.mirror = 1.;
    box.color = vec3(.0, .0, .0);
    box.index = vec3(1.);
    
    bp.x += sin(timeline.songTime * .04) * 20.;
    pR(bp.zy, sin(timeline.songTime * .03));
    pModPolar(bp.xy, 5.);
    box.dist = discoBall(bp * 2., 25.).x;
    bp = bp2;
    bp.x += sin(timeline.songTime * .02) * 20.;
    bp.y += cos(timeline.songTime * .02) * 20.;
    pR(bp.xy, sin(timeline.songTime * .1));
    pR(bp.zy, sin(timeline.songTime * .1));
    
    box.dist = smin(box.dist, discoBall(bp * 2., 25.).x,.1);
    
    
    return box;
}



float t_max = FAR;

float minDist = 1e3;
float glow = 0.;
bool firstpass = true;

geometry trace(vec3 o, vec3 d, int maxI) {
    float omega = .3;
    float t = .01;
    float candidate_error = INF;
    float candidate_t = t;
    float previousRadius = 0.;
    float stepLength = 0.;
    float pixelRadius = 1. / 250.;
    float functionSign = map(o).dist < 0. ? -1. : +1.;
    
    geometry mp;

    for (int i = 0; i < 200; ++i) {
        if (maxI > 0 && i > maxI) break; 
        mp = map(d * t + o);
        
        if (mp.index.x == 0. && firstpass) {
            minDist = min(minDist, mp.dist * 1.);
            glow = pow( 1. / minDist, .5);
        } 
        
        float signedRadius = functionSign * mp.dist;
        float radius = abs(signedRadius);
        bool sorFail = omega > 1. &&
        (radius + previousRadius) < stepLength;
        
        if (sorFail) {
            stepLength -= omega * stepLength;
            omega = .1;
        } else {
            stepLength = signedRadius * omega;
        }
        
        previousRadius = radius;
        
        float error = radius / t;
        
        if (!sorFail && error < candidate_error) {
            candidate_t = t;
            candidate_error = error;
        }
        
        if (!sorFail && error < pixelRadius || t > t_max) break;
        
        t += stepLength;
   	}
    
    mp.dist = candidate_t;
    
    if (
        (t > t_max || candidate_error > pixelRadius)
    	) mp.dist = INF;
    
    
    return mp;
}


float softShadow(vec3 ro, vec3 lp, float k) {
    const int maxIterationsShad = 15;
    vec3 rd = (lp - ro); 

    float shade = 1.;
    float dist = 2.05;
    float end = max(length(rd), .01);
    float stepDist = end / float(maxIterationsShad);

    rd /= end;
    for (int i = 0; i < maxIterationsShad; i++) {
        float h = map(ro + rd * dist).dist;
        shade = min(shade, k*h/dist);
        dist += min(h, stepDist * 2.); 
        if (h < 0.001 || dist > end) break;
    }
    return min(max(shade, 0.0), 1.0);
}


//	normal calculation
vec3 normal(vec3 p) {
    float e=.001, d = map(p).dist; return normalize(vec3(map(p+vec3(e,0,0)).dist-d,map(p+vec3(0,e,0)).dist-d,map(p+vec3(0,0,e)).dist-d));
}

float getAO(vec3 h, vec3 n, float d) { return clamp(map(h + n * d).dist / d, .3, 1.); }

vec3 clouds(vec3 d, vec3 o) {
    vec2 u = d.xz / d.y;
   
    return vec3(
        B(
            vec3(
                u + vec2(0., o.z  * .05), 9.
            )
        ) * vec3(1., .5, 0.)

    ) * max(0., d.y); 	
}

vec3 Sky(in vec3 rd, bool showSun, vec3 lightDir, vec3 ro)
{
   
    float sunSize = .5;
    float sunAmount = max(dot(rd, lightDir), 0.);
    float v = pow(1. - max(rd.y, 0.0), 1.1);
    vec3 cl = vec3(1.);//fromRGB(0,136,254);
    //cl.b *= sin(p.z * 0.3);
    vec3 sky = mix(cl, vec3(.1, .2, .3), v);
 	//vec3 lightColour = vec3(.1, .2, .3);
    
    sky += lightColour * sunAmount * sunAmount * 1. + lightColour * min(pow(sunAmount, 122.0)* sunSize, 1.2 * sunSize);
    //sky += vec3(0., 0., 0.) * max(0.,rd.y);
    return clamp(sky, 0.3, 1.0) + clouds(rd, ro);// * H(vec2(IT)) * floor(B(rd * 12.1) * 1.+ .5) * 1.;
}

vec3 doColor( in vec3 sp, in vec3 rd, in vec3 sn, in vec3 lp, geometry obj) {
	vec3 sceneCol = obj.color;
    lp = sp + lp;
    vec3 ld = lp - sp; // Light direction vector.
    float lDist = max(length(ld / 2.), 0.01); // Light to surface distance.
    ld /= lDist; // Normalizing the light vector.

    float atten = 1. / (1.0 + lDist * 0.025 + lDist * lDist * 0.5);

    float diff = max(dot(sn, ld), obj.diffuse);
    float spec = max(dot(reflect(-ld, sn), -rd), obj.specular);

    
    vec3 objCol = obj.color;//getObjectColor(sp, sn, obj);
	return objCol * (diff + .15) * spec * .1;
}


vec3 applyFog( in vec3  rgb,      // original color of the pixel
               in float distance, // camera to point distance
               in vec3  rayOri,   // camera position
               in vec3  rayDir ) {  // camera to point vector
    
    float c = .08;
    float b = .1;
    //rayOri.y -= 14.;
    float fogAmount = c * exp(-rayOri.y * b) * (1.0-exp( -distance*rayDir.y*b ))/rayDir.y;
    
    vec3  fogColor  = vec3(1., 1., 1.);//Sky(rayDir, false, normalize(light)) * 1.;//
    
    return mix( rgb, fogColor, fogAmount );
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {

    timeline.songTime = texelFetch(iChannel0, ivec2(0, 0), 0).r;
    timeline.rPatternTime = texelFetch(iChannel0, ivec2(0, 0), 0).g;
    timeline.nPatternTime = texelFetch(iChannel0, ivec2(0, 0), 0).b;
    timeline.smoothNPatternTime = texelFetch(iChannel0, ivec2(0, 0), 0).a;
    
    timeline.patternNum = texelFetch(iChannel0, ivec2(1, 0), 0).x;
    
    vec3 campos = texelFetch(iChannel0, ivec2(5, 0), 0).rgb;
    sdfMixer.grid = texelFetch(iChannel0, ivec2(11, 0), 0).r;
    sdfMixer.star = texelFetch(iChannel0, ivec2(13, 0), 0).g;
    sdfMixer.ifs1 = texelFetch(iChannel0, ivec2(15, 0), 0).r;
    
    
    //if (fragCoord.x < 100.) {
        //fragColor = vec4(timeline.rPatternTime);
        //return;
    //}
    F 	mat = 0.,
        camShY = 0.;
    
    //vol = (texture(iChannel0, vec2(.92, .15)).r) * 2.;
    
    vec2 uv = (2. * fragCoord.xy - iResolution.xy) / iResolution.x * 1.5;
    //if (length(uv) < .9) pR(uv, PI);
    light = vec3(0., 0., 100.);        
    float rr = timeline.songTime + (n(vec3(timeline.songTime * .4)) * 2.) - .5;
    vec3 
        vuv = vec3(0., 1., 0.4 ), // up
    	
        ro = campos;
    
    vec3
        vrp =  vec3(0., 0., 0.),//+ ro,
		
    	vpn = normalize(vrp - ro),
    	u = normalize(cross(vuv, vpn)),
    	rd = normalize(
            vpn + uv.x * u  + uv.y * cross(vpn, u)
        ),
        hit,
        ord = rd;
        
	
    vec3 sceneColor = vec3(1.);
    
    geometry tr = trace(ro, rd, 90);    
    
    //float fog = smoothstep(FAR * FOG, 0., tr.dist) * 1.;
    hit = ro + rd * tr.dist;
	
    float odist = tr.dist;
    
    vec3 sn = normal(hit);	
    
   // float sh = softShadow(hit, hit + light, 3.);
    
    float 
        ao = getAO(hit, sn, .6);
	
    //ao *= saturate(getAO(hit + sn * .2, sn, .5));
    //ao *= saturate(getAO(hit + sn * 1., sn, 3.0));
    
    float alpha = 1.;
	vec3 sky = Sky(rd, true, normalize(light), ro);
    
    if (tr.dist < FAR) { 
        sceneColor = (doColor(hit, rd, sn, light, tr) * 1.) * 1.;
       // sceneColor *= ao; 
        //sceneColor *= sh;
        sceneColor = mix(sceneColor, sky, saturate(tr.dist * 2. / FAR));
        //sceneColor = mix(sceneColor, lightColour, 0.1);        
        sceneColor *= 0.9 + vec3(length(
            max(
                vec2(.0),
                .7 * max(
                    0.,
                    length(normalize(light.y) * max(0., sn.y))
                )
            )
        ));
        
        firstpass = false;
        
        glow *= 1.- timeline.nPatternTime;
        
        sceneColor += pow(glow, 2.);
        
        if (glow < 1.) {
            if (tr.mirror > 0.) {   
                float mirror = tr.mirror;
                vec3 refSceneColor = sceneColor;
                rd = reflect(rd, sn);// + sin(t));
                //hit += rd * 3.;
                //rd += n(rd * 3.) * .2;
                //rd = normalize(rd);

                tr = trace(hit, rd, 69);
                hit = hit + rd * tr.dist;
                
                if (tr.dist < FAR) {
                    sn = normal(hit);
                    refSceneColor = mix(sceneColor, abs(doColor(hit, rd, sn, light, tr)), mirror);                
                } else {
                    sky = mix(Sky(rd, true, normalize(light), hit), vec3(0.), .3);
                    sky = Sky(rd, true, normalize(light), hit);
                    refSceneColor = mix(refSceneColor, sky, mirror);
                }

                sceneColor = mix(sceneColor, refSceneColor, mirror);
                
            } else {
                sceneColor = mix(sceneColor, sky, 1.0);
            }
        }
	 sceneColor *= ao; 
    	//sceneColor = mix(sceneColor, vec3(sceneColor.r + sceneColor.g + sceneColor.b)/ 3., odist / 10.);
    
    	alpha = odist / 25.;        
    } else {
        alpha = 1.;
        sceneColor = sky;        
    }

    sceneColor += pow(glow, 3.) * vec3(1., .5, 0.);
    
    
    vec3 bsceneColor = texture(iChannel1, (fragCoord.xy + vec2(1., 0.)) / iResolution.xy).rgb;
    
    sceneColor = mix(sceneColor, bsceneColor, .2);
    //sceneColor *= timeline.smoothNPatternTime;
    //if (length(uv) < .5) sceneColor = 1.- sceneColor;//pR(uv, PI);
    fragColor = vec4(clamp(sceneColor * (1. - length(uv) / 2.), 0.0, 1.0), 1.0) * 1.4;
    //fragColor = pow(fragColor, 1./vec4(1.2));
	fragColor.a = alpha;
    
    //fragColor.rgb = vec3(timeline.nPatternTime);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec3 U[2] = vec3[2](vec3(0), vec3(0));

void compute(in ivec2 coord, out vec4 color) {
    float time = U[0].x,
          pt = mod(time, PATTERN_TIME),
          pt2 = mod(pt - PATTERN_TIME / 4., PATTERN_TIME) / 3.,
          npt = pt / PATTERN_TIME,
    	  pn = floor(time / PATTERN_TIME); // pattern num
                
    switch (coord.x) {
        // R - songtime, G - pattern time (1->0), B - pattern time(0->1), smootstep patterntime) 
        case 0:
        	color = vec4(U[1].x, (PATTERN_TIME - pt) / PATTERN_TIME, npt, smoothstep(0., 1., pt / PATTERN_TIME));
            
        	break;
        case 1:
            // R - pattern number;
        	color = vec4(pn, 0., 0., 0.);
        	break;
        case 5: 
        	// RG - cam position 
        	vec2 x = texture(iChannel1, vec2(.5, fract(time * .2))).rg * 0.4 * npt;
        	vec2 r = time * .95 + x - sin(pt) * npt * 6.14 * x;
        
        	r += pn * .5;
        
        	color = vec4(
                sin(r.x) * 29., 
                -10., 
                cos(r.x) * 26., 
                0.
            );
        	
        	break;

        case 9:
        	color = vec4(pt);
        	break;
        case 11:
        	/// sdf mixer GRID,
        	color = vec4(pt, sin(pt), 0., 0.);
        	break;
        case 13: 
        	break;
        
        case 15:
        	/// sdf mixer IFS1
        	color = vec4(max(0., 1.- pt2));
        	break;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	U[1].x = float(iFrame);
    U[0].x = max(22., iChannelTime[0]);
    
    ivec2 store = ivec2(fragCoord.xy);
	vec2 uv = gl_FragCoord.xy / U[0].yz;   
    
    fragColor = vec4(0.0);

    float ck = cos(iTime * .1) * .1;
    int index = 0;

    if (store.y != 0 || store.x > 15) {        
        if (U[1].x < 10.) {
            fragColor.r = noiseLayers(vec3(fragCoord * .03, 2.));
            fragColor.b = noiseFF(fragCoord * .01);
            return;
        } else {
        	discard;
        }
    } else {
        compute(store, fragColor);         
    }
}