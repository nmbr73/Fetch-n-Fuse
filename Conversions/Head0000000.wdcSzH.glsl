

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define M_NONE -1.0
#define M_NOISE 1.0

float hash(float h) {
	return fract(sin(h) * 43758.5453123);
}

vec2 hash( vec2 p ){
	p = vec2( dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3)));
	return fract(sin(p)*43758.5453);
}

float noise(vec3 x) {
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(
			mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
					mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
			mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
					mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

#define OCTAVES 2
float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < OCTAVES; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

const int MAX_MARCHING_STEPS = 256;
const float MAX_DIST = 150.0;
const float EPSILON = 0.001;

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0)-2.0;
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}


float sdCapsule( vec3 p, float h, float r )
{
    p.y -= clamp( p.y, 0.0, h );
    return length( p ) - r;
}

float sdCone( in vec3 p, in float h, in float r1, in float r2 )
{
    vec2 q = vec2( length(p.xz), p.y );
    
    vec2 k1 = vec2(r2,h);
    vec2 k2 = vec2(r2-r1,2.0*h);
    vec2 ca = vec2(q.x-min(q.x,(q.y<0.0)?r1:r2), abs(q.y)-h);
    vec2 cb = q - k1 + k2*clamp( dot((k1-q),k2)/dot(k2,k2), 0.0, 1.0 );
    float s = (cb.x<0.0 && ca.y<0.0) ? -1.0 : 1.0;
    return s*sqrt( min(dot(ca,ca),dot(cb,cb)) );
}

float opSmI( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); 
}

float opSmU( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float opSmS( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

mat3 rotateY(float t) {
    float c = cos(t);float s = sin(t);
	return mat3(	vec3(c, 0, s),
        			vec3(0, 1, 0),
        			vec3(-s, 0, c)	);
}

mat3 rotateX(float t) {
    float c = cos(t);float s = sin(t);
	return mat3(	vec3(1, 0, 0),
        			vec3(0, c, -s),
        			vec3(0, s, c)	);
}

mat3 rotateZ(float t) {
    float c = cos(t);float s = sin(t);
	return mat3(	vec3(c, -s, 0),
        			vec3(s, c, 0),
        			vec3(0, 0, 1)	);
}

mat3 scale(float x,float y,float z){
    return mat3(
    vec3(x,0.,0.),vec3(0.,y,0.),vec3(0.,0.,z)
    );
}

float head(vec3 sP){
    float s1 = length(sP*scale(1.2,1.0,1.0)-vec3(0.0,0.5,0.0))-0.5;
    float s2 = length(sP*scale(1.2,1.3,1.2)-vec3(0.0,0.12,0.15))-0.4;
    float s3 = opSmU(s1,s2,0.2);
    //float s5 = opSmU(s3,neck,0.1);
    float s6 = sdCone((sP-vec3(0.,0.25,0.34))*rotateX(-2.1),0.22,0.2,0.0)-0.03;
    float s7 = opSmU(s3,s6,0.1);
    float s8 = length(sP-vec3(0.0,-0.08,0.27))-0.2;
    float s9 = opSmU(s7,s8,0.1);
    float s10 = length((vec3(abs(sP.x),sP.y,sP.z)-vec3(0.4,0.25,-0.03))*rotateY(0.37)*rotateZ(1341.6)*rotateX(1149.15)*scale(8.0,1.0,1.2))-0.12;
    float s11 = opSmU(s9,s10,.05);
    float s12 = length(sP-vec3(0.0,-0.14,0.56))-0.09;
    float s13 = opSmS(s12,s11,0.1);
    return s13;
}

float displace(vec3 sP,float geo){
    float final;
    if(geo<=0.01){
        float disp = geo-(0.1*fbm(sP*3.0*rotateY(iTime+12.0*(0.3*(sin(sP.y*1.0+iTime*0.3))+0.6))+vec3(0.,0.,2.0*iTime))-0.195);
    	final = opSmS(disp,geo,0.3);
    }else{
        final = geo;
    }
    return final;
}


float sceneSDF(vec3 sP) {
    vec3 sPh=sP*rotateY(noise(vec3(iTime*0.5))-0.5)*rotateX(0.1*noise(vec3(iTime*0.5+123.456))-0.09)*rotateZ(0.1*noise(vec3(iTime*0.5+222.111))-0.05);
    
	float head = head(sPh);

    float s4 = sdCapsule(sP-vec3(0.0,-0.5,-0.15),1.0,0.26);
    float s14 = length(sP-vec3(0.0,-0.7,-0.18))-0.4;
    float s15 = opSmU(s4,s14,0.1);
    float s16 = length(vec3((sP.x),sP.y,sP.z)-vec3(0.5,-1.1,-0.18))-0.43;
    float s17 = opSmU(s15,s16,0.5);
    float s16n = length(vec3(-(sP.x),sP.y,sP.z)-vec3(0.5,-1.1,-0.18))-0.43;
    float s18 = opSmU(s17,s16n,0.5);
    float torso = opSmU(s18,s4,0.1);
    torso-=0.03*noise(sP*8.0);
    torso = displace(sP,torso);
    head-=0.03*noise(sPh*8.0);
    head = displace(sPh,head);
    float final=opSmU(torso,head,0.01);

    return final;
}

float shortestDistanceToSurface(vec3 eye, vec3 marchingDirection) {
    float depth;
    float dd=0.0;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = (sceneSDF(eye + dd * marchingDirection));
        dd += dist;
        if (dist<EPSILON) {
            depth=dd;
        }else if(dist>MAX_DIST){
			return dd;
            break;
        }
        
    }
    return depth;
}
            
vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

vec3 eN(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

float AO(vec3 p,vec3 n){
    float ao =0.0;
    float d;
    for(int i=1;i<=3;i++){
        d=3.0*float(i);
        ao+=max(0.0,(d-sceneSDF(p+n*d))/d);
    }
    return ao;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec3 dir = rayDirection(45.0, iResolution.xy, fragCoord);
    vec3 eye = vec3(0.0, 0.0, 5.0);
    //dir*=rotateY(iTime*0.5);eye*=rotateY(iTime*0.5);
    vec3 color=vec3(0.0);
    float sdf = shortestDistanceToSurface(eye, dir);
    if(sdf>MAX_DIST || sdf==0.0){
        fragColor = vec4(0.0);
        return;
    }
    float dist = sdf;
    vec3 p = eye + dist * dir;
    vec3 N = eN(p);
    float occ = 1.0-AO(p,N);

    vec3 ref = (reflect(dir,N));
    color=(texture(iChannel0,ref).xyz*0.5+vec3(0.5))*occ*(1.0-smoothstep(4.5,6.0,dist));
    
    fragColor = vec4(color, 1.0);
}