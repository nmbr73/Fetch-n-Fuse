

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define S smoothstep
#define AA 1
#define T iTime*4.
#define PI 3.1415926535897932384626433832795
#define TAU 6.283185

#define MAX_STEPS 300
#define MAX_DIST 60.
#define SURF_DIST .0001

mat2 Rot(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}

float smin( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0., 1. );
    return mix( b, a, h ) - k*h*(1.0-h);
}
mat3 rotationMatrixY (float theta)
{
    float c = cos (theta);
    float s = sin (theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}
mat3 rotationMatrixX(float theta){
	float c = cos (theta);
	float s = sin (theta);
	return mat3(
		vec3(1, 0, 0),
		vec3(0, c, -s),
		vec3(0, s, c)
	);
}
mat3 rotationMatrixZ(float theta){
	float c = cos (theta);
	float s = sin (theta);
	return mat3(
		vec3(c, -s, 0),
		vec3(s, c, 0),
		vec3(0, 0, 1)
	);
}
vec3 rotateX (vec3 p, float theta)
{
	return rotationMatrixX(theta) * p;
}
vec3 rotateY (vec3 p, float theta)
{
    return p*rotationMatrixY(theta); 
}
vec3 rotateZ (vec3 p, float theta)
{
	return p*rotationMatrixZ(theta); 
}

float rounding( in float d, in float h )
{
    return d - h;
}


float opUnion( float d1, float d2 )
{
    return min(d1,d2);
}


float opSmoothUnion( float d1, float d2, float k )
{
    float h = max(k-abs(d1-d2),0.0);
    return min(d1, d2) - h*h*0.25/k;
}


float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); 
}
// ================================
// SDF
// ================================
float sdCircle( in vec3 p, in float r )
{
	return length(p)-r;
}
float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
float sdCappedCylinder( vec3 p, float h, float r )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(h,r);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
float sdCappedTorus(in vec3 p, in vec2 sc, in float ra, in float rb)
{
  p.x = abs(p.x);
  float k = (sc.y*p.x>sc.x*p.y) ? dot(p.xy,sc) : length(p.xy);
  return sqrt( dot(p,p) + ra*ra - 2.0*ra*k ) - rb;
}
float ndot(vec2 a, vec2 b ) { return a.x*b.x - a.y*b.y; }
float sdRhombus(vec3 p, float la, float lb, float h, float ra)
{
  p = abs(p);
  vec2 b = vec2(la,lb);
  float f = clamp( (ndot(b,b-2.0*p.xz))/dot(b,b), -1.0, 1.0 );
  vec2 q = vec2(length(p.xz-0.5*b*vec2(1.0-f,1.0+f))*sign(p.x*b.y+p.z*b.x-b.x*b.y)-ra, p.y-h);
  return min(max(q.x,q.y),0.0) + length(max(q,0.0));
}
float sdEllipsoid( vec3 p, vec3 r )
{
  float k0 = length(p/r);
  float k1 = length(p/(r*r));
  return k0*(k0-1.0)/k1;
}
float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}
float sdPlane( vec3 p, vec3 n, float h )
{
  // n must be normalized
  return dot(p,n) + h;
}


// ================================
// FBM
// ===============================

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	vec2 uv  = (p.xy+vec2(37.0,17.0)*p.z);
	vec2 rg1 = textureLod( iChannel2, (uv+ vec2(0.5,0.5))/256.0, 0.0 ).yx;
	vec2 rg2 = textureLod( iChannel2, (uv+ vec2(1.5,0.5))/256.0, 0.0 ).yx;
	vec2 rg3 = textureLod( iChannel2, (uv+ vec2(0.5,1.5))/256.0, 0.0 ).yx;
	vec2 rg4 = textureLod( iChannel2, (uv+ vec2(1.5,1.5))/256.0, 0.0 ).yx;
	vec2 rg  = mix( mix(rg1,rg2,f.x), mix(rg3,rg4,f.x), f.y );
	return mix( rg.x, rg.y, f.z );
}

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

float fbm4( in vec3 q )
{
    float f  = 0.5000*noise( q ); q = m*q*2.02;
          f += 0.2500*noise( q ); q = m*q*2.03;
          f += 0.1250*noise( q ); q = m*q*2.01;
          f += 0.0625*noise( q );
    return f;
}
const mat2 m2 = mat2(0.8,-0.6,0.6,0.8);
float fbm( vec2 p )
{
    float f = 0.0;
    f += 0.5000*texture( iChannel2, p/256.0 ).x; p = m2*p*2.02;
    f += 0.2500*texture( iChannel2, p/256.0 ).x; p = m2*p*2.03;
    f += 0.1250*texture( iChannel2, p/256.0 ).x; p = m2*p*2.01;
    f += 0.0625*texture( iChannel2, p/256.0 ).x;
    return f/0.9375;
}


// ================================
// SHIP
// ================================
float speed = 1.9;
float amplitude = 2.;
float amplitudeR = 0.55;
float frequency = 0.8;
float lenReactor = 0.4;
float radius = 0.12;

float createReactor(vec3 p, float rad, float len){

	p = vec3(p.x, p.y, abs(p.z)-0.5);
	float reactor1 = sdCappedCylinder(p, rad-0.02, len);
	reactor1 = rounding(reactor1, 0.02);
	vec3 q = p;

	q += vec3(rad *.8, .0,.0);
	float feature1 = sdCappedCylinder(q, rad * 0.5, len * 0.3);
	reactor1 = opUnion(reactor1, feature1);
	q = p;
	q += vec3(.0,-len,.0);
	float fire = sdCircle(q, 0.6 * rad);
	reactor1 = opUnion(reactor1, fire);
    
	return reactor1;
}

vec2 shipMap(vec3 pos, float rad, float len){
    float material;
    
    pos = rotateZ(pos, cos(T/2. * frequency) * amplitudeR);
    
    //Reactor
	vec3 q = pos;
	q = rotateZ(q, PI * 0.5);
	q = rotateX(q, PI * 0.5);
 	float reactor = createReactor(q, rad, lenReactor);

	//LinkBetweenReactors
	q = pos;
	float core = sdRhombus(q, 0.3, 0.1, 0.05, 0.2 );
	float link = opSmoothUnion(reactor, core, 0.1);
    if(abs(link-core)<0.001)
        material = 1.;

	//Guns
	q = vec3(abs(pos.x), pos.y - 0.05, pos.z);
	//q = rotateX(q, PI * 0.5);
	float gun = sdCapsule(q, vec3(0.1,0.0,-0.1), vec3(0.1,0.0,-0.4), 0.01);
	link = opUnion(gun, link);
    if(abs(link-gun)<0.001)
        material = 1.;

 
	//Core 
	q = pos + vec3(0.0,0.,-0.5);
	float core1 = sdEllipsoid(q, vec3(0.2,0.15,0.8));
	float d = opSmoothUnion(core1, link, 0.05);
    if(abs(d-core1)<0.001)
        material = 1.;
    else if(abs(d-link)<0.001)
        material = 1.;

    //Cockpit
	q = pos + vec3(0.0,-0.1,-0.3);
	float cockpit = sdEllipsoid(q, vec3(0.1,0.1,0.2));
	d = opUnion(cockpit, d);
	if(abs(d-cockpit)<0.001){
		material = 3.;
	}

    return vec2(d, material);
}
// ==================================================

//===============================
// TERRAIN
//=============================== 

vec2 terrainMap(vec3 pos){
    float hPlane = smoothstep(-0.5, 0.5,  0.2 * sin(pos.z* 2.) * sin(pos.x));
    float plane = sdPlane(pos, vec3(0.0,2.1,0.0),hPlane);
    //ROCKS
    vec3 q = vec3( mod(abs(pos.x),7.0)-2.5,pos.y,mod(abs(pos.z+3.0),7.0)-3.0);
    vec2 id = vec2( floor(pos.x/7.0)-2.5, floor((pos.z+3.0)/7.0)-3.0);
    float fid = id.x*121.1 + id.y*31.7;
    float h   = 1.8 + 1.0 * sin(fid*21.7);
    float wid = 1.0 + 0.8 * sin(fid*31.7);
    float len = 1.0 + 0.8 * sin(fid*41.7);
    h   = min(max(h, 1.),2.2);
    len = max(len, 1.5);
    wid = max(wid, 1.5);
    float ellip = sdEllipsoid(q, vec3(wid,h,len));
    ellip -= 0.04*smoothstep(-1.0,1.0,sin(5.0*pos.x)+cos(5.0*pos.y)+sin(5.0*pos.z));
    

    //TORUS
    q = vec3( mod(abs(pos.x+5.0),14.0)-5.,pos.y+0.1,mod(abs(pos.z+3.0),14.0)-3.0);
    float torus = sdCappedTorus(q, vec2(1.,0), 1.5, 0.35);
    torus -= 0.05*smoothstep(-1.0,1.0,sin(9.0*pos.x)+cos(5.0*pos.y)+sin(5.0*pos.z));
    
    float d = opSmoothUnion(torus, ellip, 0.5);
    d = opUnion(d, plane);
    
    
    float material;
    if( abs(d) < 0.001)
        material = 4.; 
    if(abs(d -plane) <0.0001) 
        material = 5.;
    return vec2(d, material);
}


//===============================

vec2 path(in float z){ 
    //return vec2(0);
    float a = sin(z * 0.1);
    float b = cos(z * frequency/2.0);
    return vec2(a*1.5 - b*1., b + a*1.5); 
}


vec2 map(in vec3 pos)
{
    
    float material;

    vec3 terrainPos = pos;
    terrainPos.xz -= path(pos.z);
    vec2 terrain = terrainMap(terrainPos);

    vec3 p = pos;
    p.z += T;
    p.x -= path(pos.z).x;
    p.y -= 0.3;
    vec2 ship = shipMap(p, radius, lenReactor);

    float d = min(ship.x, terrain.x);
    if(abs(d - ship.x) < 0.001){
        material = ship.y;
    }
    else if(abs(d - terrain.x) < 0.001){
        material = terrain.y;
    }
    
    return vec2(d, material);
}


vec2 RayMarch(vec3 ro, vec3 rd, out int mat) {
	float dO=0.;
    float dM=MAX_DIST;
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        vec2 res = map(p);
        float dS = 0.75*res.x;
        mat = int(map(p).y);
        if(dS<dM) dM = dS;
        dO += dS;
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return vec2(dO, dM);
}

vec3 GetNormal(vec3 p) {
    int mat = 0;
	float d = map(p).x;
    vec2 e = vec2(.001, 0);
    vec3 n = d - vec3(
        map(p-e.xyy).x,
        map(p-e.yxy).x,
        map(p-e.yyx));
    
    return normalize(n);
}

vec3 R(vec2 uv, vec3 p, vec3 l, float z) {
    vec3 f = normalize(l-p),
        r = normalize(cross(vec3(0,1,0), f)),
        u = cross(f,r),
        c = p+f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i-p);
    return d;
}

float calcAO( in vec3 pos, in vec3 nor, in float time )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = map( pos+h*nor).x;
        occ += (h-d)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );
}

// https://iquilezles.org/articles/rmshadows
float calcSoftshadow( in vec3 ro, in vec3 rd, float tmin, float tmax, const float k )
{
	float res = 1.0;
    float t = tmin;
    for( int i=0; i<50; i++ )
    {
		float h = map( ro + rd*t).x;
        res = min( res, k*h/t );
        t += clamp( h, 0.02, 0.20 );
        if( res<0.005 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}




void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 m = iMouse.xy/iResolution.xy;
    
    vec3 col = vec3(0.,0.,0.);
    vec3 ro = vec3(0, 1., 1.)*3.5;
    ro.yz *= Rot(-m.y*3.14+1.);
    //ro.xz *= Rot(-m.x*6.2831);
    ro.z = ro.z - T;
    
    ro.x += path(ro.z).x;
    
    ro.y = max(ro.y, -0.1);
    ro.y = min(ro.y, 1.);
    

    for(int x=0; x<AA; x++) {
        for(int y=0; y<AA; y++) {
            
            vec2 offs = vec2(x, y)/float(AA) -.5;

            vec2 uv = (fragCoord+offs-.5*iResolution.xy)/iResolution.y;
            vec3 dir = vec3(ro.x, 1, path(ro.z).y - T);
            vec3 rd = R(uv, ro, dir, 1.);
            
            // Sky
            col = vec3(0.7,0.7,0.9);
            col -= max(rd.y,0.0)*1.; 
            // clouds
            vec2 sc = ro.xz + rd.xz*(200.0-ro.y)/rd.y;
            col = mix( col, vec3(1.0,0.95,1.0), 0.5*smoothstep(0.4,0.9,fbm(0.0005*sc)) );

            int mat = -1;
            float dist = RayMarch(ro, rd, mat).x;
            
            
            vec3 p = ro + rd * dist;
            vec3 movingPos = p;
            movingPos.z += T;
            movingPos.xz = path(movingPos.z);
            vec3 f0;
            switch(mat){
                //Metal
                case 1:
                    vec3 te = 0.5 * texture(iChannel0, movingPos.xy* 2.0).xyz
                            + 0.5 * texture(iChannel0, movingPos.xz).xyz;
                    te = 0.4 * te;
                    col = te;
                    f0 = te;
                    break;
                //Reactor
                case 2:
                    f0= vec3(0.4,0.8,1.);
                    col = f0;
                    break;
                //Cockpit
                case 3:
                    col = vec3(0.0,0.0,0.0);
                    break;
                //Ground
                case 4:
                    f0 = vec3(0.);
                    vec3 gd = 0.33 * texture(iChannel1, p.xy* 2.0).xyz
                            + 0.33 * texture(iChannel1, p.yz).xyz
                            + 0.33 * texture(iChannel1, p.xz).xyz;
                    gd= 0.5 * gd;
                    col = gd;
                    
                    break;
                case 5: 
                    col *= vec3(0.5, 0.4, 0.2);
                case -1:
                    //col *= vec3(1.,1.,1.);
                    break;
            }
            

            if(dist<MAX_DIST) {
                
                vec3 lightPos = vec3(0.,10.,4.);
                //vec3 lightPos = movingPos + vec3(0.,10.,4.);
                vec3 l = normalize(lightPos);
                vec3 n = GetNormal(p);
                
                float occ = calcAO(p, n, iTime);
                //Top Light
                {
                    
                    float dif = clamp(dot(n, l), 0., 1.);
                    vec3 ref = reflect(rd, n);
                    vec3 spe = vec3(1.0) * smoothstep(0.4,0.6,ref.y);
                     float fre = clamp(1.0+dot(rd, n), 0., 1.);
                    spe *= f0; + (1.-f0) * pow(fre,5.0);
                    spe *= 6.0;
                    //float shadow = calcSoftshadow(p, l, 0.1, 2.0, 32.0 );
                   // dif *= shadow;
                    col += 0.55*vec3(0.7,0.7,0.9)*dif*occ;
                    col += vec3(0.7,0.7,0.9)*spe*dif*f0;  
                }
            
                //Side Light
                {
                    vec3 lightPos = normalize(vec3(-2.7,1.2,-0.4));
                    float dif = clamp(dot(n, lightPos), 0., 1.);
                    float shadow = calcSoftshadow(p, lightPos, 0.001, 2.0, 16.0 );

                    vec3 hal = normalize(lightPos-rd);
                    vec3 spe = vec3(1.) * pow(clamp(dot(hal, n), 0., 1.),32.0);
                    spe *= f0 + (1.-f0) * pow(1.-+clamp(dot(hal, lightPos), 0., 1.),5.0);

                    dif *= shadow;
                    col += 0.5*vec3(1.0,0.6,0.3)*dif*occ;
                    col += 1.0*vec3(1.0,0.6,0.3)*spe*f0;
                }
                
                //Bottom light
                {
                    float dif = clamp(0.5 -0.5 * n.y,0.0 ,1.);
                    col += 0.15*dif*occ;
                }
                //Reactor Light
                {
                    //vec3 lightPos = normalize(vec3(abs(movingPos.x) - 0.5,0.0, lenReactor));
                    //float dif = clamp(dot(n, lightPos), 0., 1.);
                    
                    //float shadow = calcSoftshadow(p, lightPos, 0.001, 0.5, 8.0 );
                    
                    //col += (0.7 + 0.3 * sin(iTime))*vec3(1.0,1.0,2.) * dif * shadow;
                    
                }
                col = mix( col, 0.9*vec3(0.5, 0.4, 0.2), 1.0-exp( -0.00001*dist*dist*dist ) );
            }
            
            
        }
    }
    
    
    col /= float(AA*AA);
    
    col = clamp(col,0.0,1.0);
    col = col*col*(3.0-2.0*col);
    
    
    fragColor = vec4(col,1.0);
}