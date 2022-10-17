

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

//
//
// Stranger Tides   
//
// I like the kinda abstract and fluid looks that go along with when distributing and
// animating voronoi´s to surfaces. I have always wanted to get get it working in one of
// my 50 cents of shadertoy´ing. This mood is not perfect at all, but with surprising 
// ease to get this started...
//
//
// This shader shall exist in its/this form on shadertoy.com only 
// You shall not use this shader in any commercial or non-commercial product, website or project. 
// This shader is not for sale nor can´t be minted as NFT.
//
//
// Related examples
//
//
// IQ´s 2nd order voronoi algorithm:
// https://www.shadertoy.com/view/MsXGzM      
//
//
//


#define PI 3.14159265359
#define MAX_ITER_INTERNAL 0
#define MAX_DIST 10.0
#define MIN_DIST 0.01
#define FOV 1.05
#define FK(k) floatBitsToInt(cos(k))^floatBitsToInt(k)



struct ray {
	vec4 c;
	vec3 p;
	vec3 d;
	vec3 n;
	float t;
	int i;
};




//////////////////////////////////////////////////////////////////////////////////////
// x,y,z rotation(s)
//////////////////////////////////////////////////////////////////////////////////////

mat3 rotx(float a) {
	float c=cos(a);
	float s=sin(a);
	return mat3(1.0,0.0,0.0,0.0,c,-s,0.0,s,c);
}
mat3 roty(float a) {
	float c=cos(a);
	float s=sin(a);
	return mat3(c,0.0,s,0.0,1.0,0.0,-s,0.0,c);
}
mat3 rotz(float a) {
	float c=cos(a);
	float s=sin(a);
	return mat3(c,-s,0.0,s,c,0.0,0.0,0.0,1.0);
}
mat3 rot(vec3 z,float a) {
	float c=cos(a);
	float s=sin(a);
	float b=1.0-c;
	return mat3(
		b*z.x*z.x+c,b*z.x*z.y-z.z*s,b*z.z*z.x+z.y*s,
		b*z.x*z.y+z.z*s,b*z.y*z.y+c,b*z.y*z.z-z.x*s,
		b*z.z*z.x-z.y*s,b*z.y*z.z+z.x*s,b*z.z*z.z+c);
}
mat2 rot2d(float a) {
	float c=cos(a);
	float s=sin(a);
	return mat2(
		c,-s,
		s, c);
}




//////////////////////////////////////////////////////////////////////////////////////
// smooth minimum(s) -> https://iquilezles.org/www/articles/smin/smin.htm
//////////////////////////////////////////////////////////////////////////////////////

float smin(float a,float b,float k) {
	float h=clamp(0.5+0.5*(b-a)/k,0.0,1.0);
    return mix(b,a,h)-k*h*(1.0-h);
}

//Smooth min by IQ
float smin( float a, float b ) {
    float k = 0.95;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}



//////////////////////////////////////////////////////////////////////////////////////
// randomness and hash´ing like a pro
//////////////////////////////////////////////////////////////////////////////////////

float s;

void srand(vec2 p) {
	s=sin(dot(p,vec2(423.62431,321.54323)));
}

float rand() {
	s=fract(s*32322.65432+0.12333);
	return abs(fract(s));
}

float hash( in vec3 p ) {
    return fract(sin(p.x*15.32758341+p.y*39.786792357+p.z*59.4583127+7.5312) * 43758.236237153)-.5;
}

float hash(vec2 k) {
  int x = FK(k.x);int y = FK(k.y);
  return float((x*x-y)*(y*y+x)-x)/3.14e9;
}

float hash3(vec3 k) {
  float h1 = hash(k.xy);
  return hash(vec2(h1, k.z));
}


vec3 hash33(vec3 k) {
  float h1 = hash3(k);
  float h2 = hash3(k*h1);
  float h3 = hash3(k*h2);
  return vec3(h1, h2, h3);
}



//////////////////////////////////////////////////////////////////////////////////////
// As in IQ´s rocks
//////////////////////////////////////////////////////////////////////////////////////

vec3 voronoi3d(const in vec3 x) {

  vec3 p = floor(x);
  vec3 f = fract(x);

  float id = 0.0;
  vec2 res = vec2(100.0);
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
        vec3 b = vec3(float(i), float(j), float(k));
        vec3 r = vec3(b) - f + hash(p + b );
         //r.x *= 0.5 + 0.5*sin( iTime );
         //r.z *= 0.5 + 2.5*cos( iTime );
         //r *= rot( r, iTime );


        float d = dot(r, r);

        float cond = max(sign(res.x - d), 0.0);
        float nCond = 1.0 - cond;

        float cond2 = nCond * max(sign(res.y - d), 0.0);
        float nCond2 = 1.0 - cond2;

        id = (dot(p + b, vec3(1.0, 57.0, 113.0)) * cond) + (id * nCond);
        res = vec2(d, res.x) * cond + res * nCond;

        res.y = cond2 * d + nCond2 * res.y;
      }
    }
  }
  return vec3(sqrt(res), abs(id));
}


//////////////////////////////////////////////////////////////////////////////////////
// Voronoi 2d
//////////////////////////////////////////////////////////////////////////////////////

float voronoi2d(vec2 p,float t) {
	float v=0.0;
	vec2 f=floor(p)+vec2(0.25);
	for(float i=-3.0;i<3.0;i++)
        for(float j=-3.0;j<3.0;j++){
            srand(f+vec2(i,j));
            vec2 o;
            o.x=rand();
            o.y=rand();
            o*=rot2d(iTime*(rand()-0.5));
            float r=distance(p,f+vec2(i,j)+o);
            v+=exp(-16.0*r);
        }
	return -smin((1.0/16.0)*log(v),-0.1,0.1);
}



//////////////////////////////////////////////////////////////////////////////////////
// Torus
//////////////////////////////////////////////////////////////////////////////////////

float torus(inout ray r) {
    vec3 vt = r.p + ((voronoi3d(r.p * 2.6 + iTime * .1).x)*0.5 + 0.5);
    vt.x -= 0.5;
    vt.y -= 1.5;
    vt.z -= 0.60;
    vt *= rotz( sin(iTime * 0.5) );
    vt *= rotx( cos(iTime * 0.25) );
    return length(vec2(length(vt.xy) -1.74, vt.z)) -0.55;	
}



//////////////////////////////////////////////////////////////////////////////////////
// Dome
//////////////////////////////////////////////////////////////////////////////////////

float dome(inout ray r) {
	float v=voronoi2d(r.p.xy,r.t);
	float d=dot(r.p,vec3(0.0,0.0,0.75))+0.5-0.5*v;
	if(d<0.0){
		r.c=vec4(1.0);
		r.i=MAX_ITER_INTERNAL+1;
	}
	return d;
}



//////////////////////////////////////////////////////////////////////////////////////
// Bubbles
//////////////////////////////////////////////////////////////////////////////////////

vec3 sphercoord(vec2 p) {
  float l1 = acos(p.x);
  float l2 = acos(-.5)*p.y;
  return vec3(cos(l1), sin(l1)*sin(l2), sin(l1)*cos(l2));
}

vec3 erot(vec3 p, vec3 ax, float ro) {
  return mix(dot(p,ax)*ax, p, cos(ro)) + sin(ro)*cross(p,ax);
}

float comp(vec3 p, vec3 ro, float t) {
  vec3 ax = sphercoord(ro.xy);
  p.z -= t;
  p = erot(p, ax, ro.z*acos(-1.));
  float scale = 7. + hash(ro.xz)*0.25+0.25;
  p = (fract(p/scale)-0.65)*scale;
  return length(p) - 0.2;
}



//////////////////////////////////////////////////////////////////////////////////////
// distance field
//////////////////////////////////////////////////////////////////////////////////////

float dist(inout ray r) {
  
  float d=MAX_DIST;
  float dist = d;
  
  for( int i = 0; i < 4; i++ ) {
       vec3 rot = hash33(vec3(float(i+1), cos(float(i)), sin(float(i))));
       float d_ = comp(r.p, rot, iTime/2.*(float(i+1)));
       dist = smin(dist, d_, .5);
  }
   
  d=smin(d,dome(r));
  d=smin(d,smin(torus(r),dist, .2));
    
  return d;

}



vec4 trace(inout ray r) {
	r.c=vec4(1.0);
	for(int i=0;i<32;i++){
		float d=dist(r);
		if(r.i>MAX_ITER_INTERNAL)break;
		r.p+=r.d*max(d,MIN_DIST);
	}

    
	return vec4(2.0/exp(abs(r.p.z)));
}




//////////////////////////////////////////////////////////////////////////////////////
// particles ( --> by Andrew Baldwin)
//////////////////////////////////////////////////////////////////////////////////////

float particles(vec3 direction)
{
	float help = 0.0;
	const mat3 p = mat3(13.323122,23.5112,21.71123,21.1212,28.7312,11.9312,21.8112,14.7212,61.3934);
	vec2 uvx = vec2(direction.x,direction.z)-vec2(1.,iResolution.y/iResolution.x)*gl_FragCoord.xy / iResolution.xy;

    float acc = 0.0;
	float DEPTH = direction.y*direction.y-0.3;
	float WIDTH =0.1;
	float SPEED = 0.09;
	for (int i=0;i<10;i++) 
	{
		float fi = float(i);
		vec2 q = uvx*(1.+fi*DEPTH);
		q += vec2(q.y*(WIDTH*mod(fi*7.238917,1.)-WIDTH*.5),SPEED*iTime/(1.+fi*DEPTH*.03));
		vec3 n = vec3(floor(q),31.189+fi);
		vec3 m = floor(n)*.00001 + fract(n);
		vec3 mp = (31415.9+m)/fract(p*m);
		vec3 r = fract(mp);
		vec2 s = abs(mod(q,1.)-.5+.9*r.xy-.45);
		float d = .7*max(s.x-s.y,s.x+s.y)+max(s.x,s.y)-.01;
		float edge = .04;
		acc += smoothstep(edge,-edge,d)*(r.x/1.0);
		help = acc;
	}
	return help;
}



void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
    float t = iTime;
	float r = iResolution.x/iResolution.y;
    
	vec2 m=vec2(
		(iMouse.x-iResolution.x/2.0)/iResolution.x*r,
		(iMouse.y-iResolution.y/2.0)/iResolution.y);
	vec2 s=vec2(
		(fragCoord.x-iResolution.x/2.0)/iResolution.x*r,
		(fragCoord.y-iResolution.y/2.0)/iResolution.y);	    
	
    vec3 l=vec3(0.0,0.0,0.0);
	vec3 tmp=vec3(2.0,3.0,2.0);//2.0
	tmp*=roty((PI*m.y)/4.0-PI/8.0);
	tmp*=rotz(2.0*PI*m.x);

	vec3 e=l+tmp;
	vec3 u=vec3(0.0,0.0,1.0);
	vec3 d=normalize(l-e);
	vec3 h=normalize(cross(d,u));
	vec3 v=normalize(cross(h,d));
	float f=0.75;
	d*=rot(v,FOV*s.x);
	d*=rot(h,FOV*s.y);
	ray a=ray(vec4(0.0),e,d,vec3(0.0),t,0);
	
    vec4 col = trace(a);
    col.r -= 0.98;
    col.g -= 0.48;
    col.b -= .018;
    
    vec2 position = fragCoord.xy / iResolution.xy;   
	position.y *=-1.0;

	col *= 0.27 + 5.5 * position.x * position.y * ( 1.0 - position.x ) * ( -1.0 - position.y );
    col.rgb = col.rgb + particles(a.c.xyz);
  
    vec2 rnd1  = vec2(12.9898,78.233);
    float rnd2 = 43758.5453;
    float c = fract(sin(dot(s.xy ,rnd1)) * rnd2+(iTime*0.5));
    col.xyz = (col.xyz*0.85)+(col.xyz*0.12*vec3(c));    

	col.xyz *= 1.1;

    fragColor = col * col * 2.1;
    
}