

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// The Greatest Adventure by Martijn Steinrucken aka BigWings - 2016
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// Song: Yann Tiersen – Comptine d’un autre été - L’après midi
// https://soundcloud.com/reminiscience/yann-tiersen-comptine-dun-autre-ete-lapres-midi 
//
// My entry for the 2017 ShaderToy competition
//
// I thought I had one more day, luckily I found out in time that I didn't :) Pulled an all-nighter to get this done in time.
// I didn't have time to optimized and beautify so forgive me for the horrible code.
// 
// I figured the greatest adventure there is is life itself. 
//
// https://www.shadertoy.com/view/MsSBD1

#define INVERTMOUSE 1.

#define MAX_STEPS 200
#define MIN_DISTANCE 0.1
#define MAX_DISTANCE 4.
#define RAY_PRECISION 0.0003

#define BG_STEPS 20.

#define S(x,y,z) smoothstep(x,y,z)
#define L(x, y, z) clamp((z-x)/(y-x), 0., 1.)
#define B(x,y,z,w) S(x-z, x+z, w)*S(y+z, y-z, w)
#define sat(x) clamp(x,0.,1.)
#define SIN(x) sin(x)*.5+.5

#define BOKEH_SIZE .03

#define FLAMECOL vec3(.99, .6, .35)
#define FLAMEBLUE vec3(.1, .1, 1.)
#define CANDLECOL  vec3(.2, .5, .2)
#define CANDLE_HEIGHT 10.
#define SONG_LENGTH 134.
#define DEATH_TIME 126.3

const float halfpi = 1.570796326794896619;
const float pi = 3.141592653589793238;
const float twopi = 6.283185307179586;

vec2 m; // mouse

vec3 A;	// 0=birth  1=death  x = 0->1 y = 1->0 z = smoothstep(x)


float gHeight;

float X2(float x) {return x*x;}
float N( float x ) { return fract(sin(x*12.35462)*5346.1764); }
float N2(float x, float y) { return N(x + y*23414.324); }

float N21(vec2 t) {return fract(sin((t.x+t.y*10.)*9e2));}
float N31(vec3 t) {return fract(sin((t.x+t.y*10.+ t.z*100.)*9e2));}
vec4 N14(float t) {return fract(sin(vec4(1., 3., 5., 7.)*9e2));}

float LN(float x) {return mix(N(floor(x)), N(floor(x+1.)), fract(x));}

struct ray {
    vec3 o;
    vec3 d;
};



struct de {
    // data type used to pass the various bits of information used to shade a de object
	float a;// age
    float d;	// distance to the object
    float fh;	// flame height
    float m; 	// material
    float f;	// flame
    float w;	// distance to wick
    float fd;	// distance to flame
    float b;	// bump	
    
    float s; // closest flame pass
    float sd;
    vec2 uv;
    
    // params that only get calculate when marcing is complete
    vec3 id;	// cell id
    vec4 n; 	// cell id based random values
    vec3 p;	// the world-space coordinate of the fragment
    vec3 nor;	// the world-space normal of the fragment	
};
    
struct rc {
    // data type used to handle a repeated coordinate
	vec3 id;	// holds the floor'ed coordinate of each cell. Used to identify the cell.
    vec3 h;		// half of the size of the cell
    vec3 p;		// the repeated coordinate
};


ray GetRay(vec2 uv, vec3 p, vec3 lookAt, float zoom) {
	
    vec3 f = normalize(lookAt-p),
    	 r = cross(vec3(0,1,0), f),
    	 u = cross(f, r),
    	 c = p+f*zoom,
         i = c+r*uv.x+u*uv.y;	// point in 3d space where cam ray intertsects screen
    
    ray cr;
    
    cr.o = p;						
    cr.d = normalize(i-p);		// ray dir is vector from cam pos to screen intersect 
	return cr;
}

float remap01(float a, float b, float t) { return (t-a)/(b-a); }
float remap(float a, float b, float c, float d, float t) { return sat((b-a)/(t-a)) * (d-c) +c; }

float DistLine(vec3 ro, vec3 rd, vec3 p) {
	return length(cross(p-ro, rd));
}

vec2 within(vec2 uv, vec4 rect) {
	return (uv-rect.xy)/rect.zw;
}

// DE functions from IQ
// https://www.shadertoy.com/view/Xds3zN

vec2 smin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return vec2(mix( b, a, h ) - k*h*(1.0-h), h);
}
vec2 smin2( float a, float b, float k, float z )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    float g = h*h*(3.-2.*h);
    return vec2(mix( b, a, g ) - k*pow(g*(1.-g), z), g);
}

vec2 smax( float a, float b, float k )
{
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return vec2(mix( a, b, h ) + k*h*(1.0-h), h);
}

float sdSphere( vec3 p, vec3 pos, float s ) { return length(p-pos)-s; }

float sdCapsule( vec3 p, vec3 a, vec3 b, float r ) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float sdCylinder( vec3 p, vec2 h ) {
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdCone( vec3 p, vec3 c ) {
    vec2 q = vec2( length(p.xz), p.y );
    float d1 = -q.y-c.z;
    float d2 = max( dot(q,c.xy), q.y);
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

vec3 Bend( vec3 p, float center, float strength ) {
    p.y-=center;
    float c = cos(strength*p.y);
    float s = sin(strength*p.y);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    q.y+=center;
    return q;
}

float pModPolar(inout vec2 p, float repetitions) {
	float angle = 2.*pi/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	float r = length(p);
	float c = floor(a/angle);
	a = mod(a,angle) - angle/2.;
	p = vec2(cos(a), sin(a))*r;
	if (abs(c) >= (repetitions/2.)) c = abs(c);
	return c;
}

de fmap( vec3 p, de n ) {
    // mapping the flame
     
    float t = iTime;
     
    float y = sat(remap01(.0, CANDLE_HEIGHT, p.y));
    float h = 1.-y;
    float a = atan(p.x, p.z);
    float l = length(p.xz);
    
    float baby = S(.025, 0., h);
    float child = S(.3, .05, h); 				
    float adult = 1.-child;
    float adult2 = S(.2, .5, A.x);
    float death = S(.8, 1., A.z);
    
    float height = gHeight+.055+baby*.05-adult2*.04-adult*.02+death*.02;
   
    de o;
    o.m = 1.;
    
    p.z *= 1.5;
    
    float size = .04-baby*.01+adult2*.04-death*.02;
    
    float flame = MAX_DISTANCE;
    
    float flameHeight = .2-.1*baby+adult2*.1-death*.2;
   
    float smth = .02-baby*.01+adult2*.03-death*.03;
    
    float x = adult*.01-adult2*.03+death*.04;
    float wave = sin(p.y*10.-t*4.);
    wave *= sin(t);
    wave *= .01;
    
    float baseSize = .7+flameHeight*.4;
    float start = S(.01, .0, A.x);
    baseSize *= 1.-start*.5;
    flameHeight *= 1.-start;
    
    float flicker = LN(t+A.x*t);
    flameHeight *= 1.-flicker*flicker*flicker*A.x*.5;
    
    for(float i=0.; i<1.; i+=1./10.) {
    	float fH = height + i*flameHeight;
        size = baseSize * mix(.05, .01, i);
        
        float fX = x+wave*i*i;
        flame = smin(flame, sdSphere(p, vec3(fX, fH, .0), size), smth).x;
    }
    
    o.fh = flameHeight;
    
    o.d = flame/1.5;
    return o;
    
}

vec4 map( vec3 p ) {
   
    float y = sat(remap01(.0, CANDLE_HEIGHT, p.y));
    float h = 1.-y;
    float a = atan(p.x, p.z);
    float l = length(p.xz);
    
    float cRadius = mix(.3, .1, S(0., 1., y)); // overall growth in thickness
    float baby = S(.025, 0., h);
    float child = S(.3, .05, h); 				
    float adult = 1.-child;
    float adult2 = S(.2, .5, A.x);
    float death = S(.8, 1., A.z);
    
    cRadius *= 1.-child*.6;
    
    float inside = l/cRadius;
    float oMask = S(.8, .9, inside);
    
    //candle
    float candle = sdCylinder(p, vec2(cRadius, gHeight));
    if(baby>0.) {
    	float topCone = sdCone(p-vec3(0,gHeight+.075,0), vec3(1,cRadius*12.6,.075));
    	candle = min(candle, topCone);
    }
    
    float d = candle-.003*baby;
    
    float chamfer = 0.06*adult2;//.1*(1.-baby);
    float crRadius = cRadius*(.6+adult2*.3)+ (1.-baby)*.015;
    float crHeight = gHeight+.03*child-.05*adult2+.07*baby;
    crHeight = gHeight+.05+baby*.06-adult*.02-.02*adult2;
    float crBlend = .001+adult2*.02+.02*adult;
    float crater = sdCylinder(p-vec3(0, crHeight+chamfer, 0), vec2(crRadius-chamfer, .05))-chamfer;
    d = smax(d, -crater, crBlend).x;

    // drips
    float id = floor(a*40./twopi);
    float n = N(id);
    float age = n*n-y;
    
    float drips = -abs(sin(a*20.)*sin(p.y*.5+n*pi))*max(0., age);
    drips = max(0., sin(a*20.));
    drips *= S(.85, 1., inside);
    float bump = -max(drips*.7*sin(p.y+n*twopi)-y, 0.);	// add drips
    
    // baby bday candle
    float x = a+p.y*10.;
    float bday = pow(abs(sin(x*4.)),8.);
    float coneMask = S(.9755, .96, y);

    bday = bday*oMask*child*coneMask;
    d -= bday*.005;
    
    // candle getting fucked up toward the bottom
    vec3 q = p*25.;
    bump += sin(p.y)*(sin(q.x+sin(q.y+sin(q.z*pi)))+sin(q.x*2.+q.z+q.y))*.5*S(.3, .1, y);
    bump += sin(sin(p.y*15.)+a*10.)*S(.11, .0, y)*2.;
    d += bump*.01*oMask;
    
    // floor
   	d = smin(d, p.y, .3).x;	
    
    // wick
    float wBottom = crHeight-.05-.01*adult2;
    float wHeight = .08-.01*death-.02*child-baby*.02;
    float wDiam = .008+adult2*.003-.002*child;
    float wBlend = (.04+.05*adult)*(1.-baby)+.1*adult2;
    wBlend = (1.-baby)*.02+.05*adult2-.05*death;
    float wBend = 10.1*adult2*(1.-death)-5.*adult;
    vec3 wPos = Bend(p, wBottom, wBend);
    float wick = sdCapsule(wPos, vec3(0, wBottom, 0), vec3(0, wBottom+wHeight, 0), wDiam);
    
    vec3 wUv = wPos*80.*CANDLE_HEIGHT;
    float wBump = sin(wUv.y*0.2)*sin(wUv.z)*sin(wUv.x);
    wBump *= sat(remap01(wBottom, wBottom+wHeight, p.y));
    wBump *= .001+.001*adult2;
    
    d = smin2(d, wick-wBump, wBlend, 1.+adult2*3.).x;
    
    // floor drips
    float j = pModPolar(p.xz, 4.);
    n = N(j);
    float bottom = sdSphere(p, vec3(1, 0, 0)*.4, .3*(n+.3));
    bottom = smax(bottom, p.y-.05, .05).x;
    d = smin(d, bottom, .02).x;
    
    //d = candle;
    
    return vec4(d, wick, wBottom+wHeight, bump+bday);
}

de castRay( ray r ) {
    
    float t = iTime;
    float dS;
    
    de o;
    o.d = MIN_DISTANCE;
    o.w = MAX_DISTANCE;
    o.m = -1.0;
    o.n = mix(N14(floor(t)), N14(floor(t+1.)), fract(t));
    
    vec4 d;
    for( int i=0; i<MAX_STEPS; i++ ) {
        o.p =  r.o+r.d*o.d;
 
        d = map(o.p);
        
        o.w = min(o.w, d.y);	// keep track of closest wick dist 
        if( d.x<RAY_PRECISION || o.d>MAX_DISTANCE ) break;
        
        o.d += d.x;
    }
    o.a = d.z;
    o.b = d.w;
    
    if(d.x<RAY_PRECISION) o.m = 1.;
    
    
    // marching the flame
    de res;
    
    o.s = 1000.;
    o.fd = 0.;
    float n = 1.;
    for( int i=0; i<MAX_STEPS; i++ )
    {
	    res = fmap( r.o+r.d*o.fd, o );
        o.fh = res.fh;
        if( res.d<RAY_PRECISION || o.fd>MAX_DISTANCE ) break;
        if(res.d<o.s) {
            o.s = res.d;
            o.sd = o.fd;
        }
        
        o.fd += res.d;
    }
    
    if(res.d<RAY_PRECISION)
        o.f=1.;
    
    return o;
}

vec3 calcNormal( de o )
{
	vec3 eps = vec3( 0.01, 0.0, 0.0 );
	vec3 nor = vec3(
	    map(o.p+eps.xyy).x - map(o.p-eps.xyy).x,
	    map(o.p+eps.yxy).x - map(o.p-eps.yxy).x,
	    map(o.p+eps.yyx).x - map(o.p-eps.yyx).x );
	return normalize(nor);
}

vec3 FlameNormal( vec3 p, de n )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    fmap(p+eps.xyy, n).d - fmap(p-eps.xyy, n).d,
	    fmap(p+eps.yxy, n).d - fmap(p-eps.yxy, n).d,
	    fmap(p+eps.yyx, n).d - fmap(p-eps.yyx, n).d );
	return normalize(nor);
}

vec3 Background(ray r) {
    
    float t = iTime;
    
	float x = atan(r.d.x, r.d.z);
    float y = dot(vec3(0,1,0), r.d);
    
    float d = 2.1;
        
    vec2 size = vec2(2);
    vec2 h = size / 2.;
    
    float blur = .3;
    
    vec3 col = vec3(0);
    
    for(float i=0.; i<BG_STEPS; i++) {
    	vec3 p = r.o + r.d*d;
    						
    	vec2 id = floor(p.xz/size);								// used to give a unique id to each cell
   		vec3 q = p;
        q.xz = mod(p.xz, size)-h;								// make grid of flames
        
        vec3 cP = vec3(0, N21(id)*CANDLE_HEIGHT, 0);
        
        float dRayFlame = DistLine(q, r.d, cP);						// closest ray, point dist
        
        float dOriFlame = d + length(cP-p);		// approximate distance from ray origin to flame 
        float bSize = dRayFlame/dOriFlame;
        vec3 flame = FLAMECOL;
        flame *= S(BOKEH_SIZE, BOKEH_SIZE-BOKEH_SIZE*blur, bSize);
        
        flame *= 100.;
        flame /= (dOriFlame*dOriFlame);
        float flicker = LN(t+id.x*100.+id.y*345.);
        //flicker = mix(.3, 1., S(.2, .5, flicker));
        flame *= 1.-flicker*flicker*.7;
        col += flame;
        
        // step to the next cell
        vec2 rC = ((2.*step(0., r.d.xz)-1.)*h-q.xz)/r.d.xz;		// ray to cell boundary
        float dC = min(rC.x, rC.y)+.01;
        
        d += dC;
    }
    
    
    return col;
}


vec3 render( vec2 uv, ray cam ) {
    
    float t = iChannelTime[0];
    
    vec3 col = vec3(0.);
    de o = castRay(cam);
    
    vec3 n = calcNormal(o);
    
    float death2 = S(.9999, .998, A.x);
    
    if(o.m==-1.)
        col = Background(cam)*S(0.25, .45, A.x)*S(1., .8, A.x);//*X2(A.x*A.y)*4.;
    else if(o.m==1.) {
    	
        float y = sat(remap01(.0, CANDLE_HEIGHT, o.p.y));
    	float h = 1.-y;
    
    	float baby = S(.025, 0., h);
    	float child = S(.3, .05, h); 				
    	float adult = 1.-child;
    	float adult2 = S(.2, .5, A.x);
    	float death = S(.8, 1., A.z);
        
        vec3 sssColor = mix(mix(vec3(1), vec3(1, 0, 0), o.b), CANDLECOL, adult);
        
        float cRadius = mix(.3, .1, S(0., 1., y));
        
        float l = length(o.p.xz);
        float inside = l/cRadius;
        
        col = vec3(dot(normalize(vec3(1, 1,1)), n)*.5+.5);
    
        float height = mix(CANDLE_HEIGHT, .1, A.z);

        // add wick glow
        y = o.p.y-height;
        float wickH = y-o.p.x*.4-.02-baby*.15;
        float wickGlow = S(.009, .025, wickH)*S(.2, .15, inside);
        
        vec3 fPos = vec3(0,height,0);
        fPos.y += .2;
        fPos -= o.p;
        float lambert = sat(dot(normalize(fPos), n))/dot(fPos, fPos);
        lambert *= S(1.0005, .998, A.x);
        lambert *= 1.-S(.85, .999, A.x)*.75;
        lambert *= .1;
        vec3 light = FLAMECOL*lambert;

        col *= S(1.5, 0., length(o.p.xz));
        col *= .1;
        col += light*(1.-wickGlow*.7);
        
        col += FLAMECOL*FLAMECOL*wickGlow*2.;
        
        vec3 r = reflect(cam.d, n);
        float phong = sat(dot(normalize(fPos), r));
        phong = pow(phong, 10.);
        phong *= S(1.5, .8, inside);
        phong *= S(1.0005, .998, A.x);
        col += phong*FLAMECOL*o.fh*2.;
        
        
        
        // add sss
        float sssHeight = mix(.4, 1., adult)*o.fh;
        float sss = S(-sssHeight, -.0, y);
        sss *= inside*S(.3, .1, A.x)*2.+S(.7, .98, inside);
        sss *= 1.+o.b*.3;
        sss *= S(1., .9, A.x);
        col += sss*sssColor;
        
        //col = vec3(phong);
        
    }
    
     if(o.f>0.&&o.fd<o.d) {
        vec3 p = cam.o+cam.d*o.fd;
         
        float y = sat(remap01(.0, CANDLE_HEIGHT, p.y));
    	float h = 1.-y;
    
    	float baby = S(.025, 0., h);
    	float child = S(.3, .05, h); 				
    	float adult = 1.-child;
    	float adult2 = S(.2, .5, A.x);
    	float death = S(.8, 1., A.z);
        
         
        vec3 n = FlameNormal(p, o);
        float fresnel = sat(dot(n, -cam.d));
        float flame = 1.;//fresnel;
        
        y = p.y-gHeight;
       
        float topFade = S(o.fh, o.fh*.5, y-baby*.13);
        float bottomFade = S(.0, .1, y);
        float wickFade = S(.0, .03, o.w)*.8 +.2;
         
         float spikes = sin(uv.x*200.+t*15.)*.25;
         spikes += sin(uv.x*324.-t*5.)*.5;
         spikes += sin(uv.x*145.+t);
         spikes *= S(0.1, .6, y);
         spikes = 1.-spikes;
         
        col = mix(col, FLAMECOL*3., bottomFade*wickFade*fresnel*topFade*death2*spikes);
        
         float blue = S(.4, -.0, y);
         blue *= fresnel;//S(.7, .3, fresnel);
         
         topFade = S(.1, .0, y);
         bottomFade = S(-.04, .01, y);
         wickFade = mix(wickFade, 1., 1.-bottomFade);
         col += FLAMEBLUE*blue*bottomFade*topFade*wickFade*death2;
        
         //col = vec3(spikes);
    }
    
    
    // add some glow
    float gw = S(0., .3, A.x);
    gw *= S(1., .8, A.x);
    float glow = S(.1*gw, 0., o.s)*.15*gw;
    glow*=death2;
    col += glow*FLAMECOL;
    
    return col;
}

void mainImage( out vec4 o, in vec2 uv )
{
    float t = fract(iTime/SONG_LENGTH)*SONG_LENGTH;
    
    uv = (2.*uv - (o.xy=iResolution.xy) ) / o.y ;  	// -1 <> 1
   	m = iMouse.xy/iResolution.xy;					// 0 <> 1
    
    float t2 = sat(t/DEATH_TIME);
    A = vec3(t2, 1.-t2, S(0., 1., t2));
    //A = vec3(m.y);
    
    gHeight = mix(CANDLE_HEIGHT, .1, A.z);
    
    float turn = (.1-m.x)*twopi+t*.06;
    float s = sin(turn);
    float c = cos(turn);
    mat3 rotX = mat3(	  c,  0., s,
                   		  0., 1., 0.,
                   		  s,  0., -c);
    
    float y = mix(CANDLE_HEIGHT+.1, .1, A.z);
    //y-=.1;
    vec3 lookAt = vec3(0., y, 0.);
    //lookAt.y += S(.2, 0., A.x)*.1;
    
    y += A.x*A.x*A.x*A.z*.5;					// raise cam towards the end
    
    
    float dist = .5+A.z+S(.3, 0., A.x)*.25;
   // dist = .45; //y-=.2;
    vec3 pos = vec3(0., y, -dist)*rotX;
   	
    ray r = GetRay(uv, pos, lookAt, 2.);

    vec3 col = render(uv, r);
    
    o = vec4(col, 1.);
    
    
    o *= S(0., 1., t)*S(SONG_LENGTH, 130., t);	// fade in/out
}