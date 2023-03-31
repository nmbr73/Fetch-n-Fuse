

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//-----------------------------------------------------
// Created by sebastien durand - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------
// inspired by nguyen nhut work https://twitter.com/nguyenhut_art
// Teapot alone: https://www.shadertoy.com/view/XsSGzG
//-----------------------------------------------------

#define PI 3.14159265

#define WITH_AO

#define ZERO (min(0, iFrame))


#define GROUND 0.
#define SKIN 1.
#define SHOES1 2.
#define SHOES2 2.5
#define SHORT 3.
#define BAG 4.
#define SHIRT 5.
#define TEAPOT 6.
#define BED 7.
#define METAL 8.
//#define PANEL 9.
#define BONE 10.


float gTime;

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------

//                       Contact           Down               Pass               Up      

vec3[9] HEAD = vec3[9]( vec3(50,24,0),  vec3(73,30,0),   vec3(94,20,0),   vec3(117,15,0),  
                        vec3(135,24,0), vec3(158,30,0),  vec3(179,20,0),  vec3(202,15,0), vec3(218,24,0));

vec3[9] SHOULDER = vec3[9](vec3(44,47,16),vec3(66,53,16), vec3(91,43,16), vec3(115,38,16), 
                         vec3(136,50,16), vec3(158,55,16), vec3(176,43,16), vec3(85+111,37,16), vec3(212,47,16));

vec3[9] ELBOW = vec3[9](vec3(25,64,25), vec3(46,67,25),  vec3(88,70,25),  vec3(120,65,25),
                        vec3(139,72,25),vec3(172,67,25), vec3(176,71,25), vec3(177,61,25), vec3(193,64,25));

vec3[9] WRIST = vec3[9](vec3(20,85,15), vec3(35,76,20), vec3(88,100,25), vec3(128,89,25), 
                        vec3(164,85,15), vec3(187,81,20),vec3(85+88,98,25),vec3(85+82,81,20), vec3(188,85,15));

vec3[9] HIP = vec3[9](  vec3(42,90,10),  vec3(62,95,10),   vec3(83,88,10),   vec3(107,83,10),  
                        vec3(127,92,10), vec3(147,94,10),  vec3(168,91,10),  vec3(192,85,10), vec3(210,90,10));

vec3[9] KNEE = vec3[9]( vec3(29,118,7),  vec3(48,120,8),   vec3(97,117,10),  vec3(130,107,10), 
                        vec3(144,120,7), vec3(167,118,7),  vec3(167,118,7),  vec3(181,111,7), vec3(197,118,7));

vec3[9] ANKLE = vec3[9](vec3(5,134,5),   vec3(22,132,6),   vec3(71,122,10),  vec3(113,127,10), 
                        vec3(162,146,5), vec3(164,146,5),  vec3(164,146,5),  vec3(168,137,5), vec3(173,134,5));

vec3[9] FOOT = vec3[9]( vec3(14,150,10), vec3(16,150,10),  vec3(63,139,10),  vec3(119,143,10), 
                        vec3(178,139,10),vec3(182,150,10), vec3(182,150,10), vec3(182,150,10), vec3(182,150,10));


vec3 shoulder1, elbow1, wrist1, head,
     shoulder2, elbow2, wrist2;
vec3 foot1, ankle1, knee1, hip1,
     foot2, ankle2, knee2, hip2;

mat2 rot, rot1, rot2;


#define U(a,b) (a.x*b.y-b.x*a.y)

vec2 A[15];
vec2 T1[5];
vec2 T2[5];

float smin(float a, float b, float k){
    float h = clamp(.5+.5*(b-a)/k, 0., 1.);
    return mix(b,a,h)-k*h*(1.-h);
}

vec2 min2(vec2 a, vec2 b) {
    return a.x<b.x ? a : b; 
}

#ifdef BONE
// Adated from gaz [Bones] https://www.shadertoy.com/view/ldG3zc
vec2 sdBone(in vec3 p) {
    p.x -= 80.;
    const float m = 200.;
    float scale = .5 + floor(abs(p.x)/m);
    p.x = mod(p.x+m*.5,m)-m*.5;
    p.xz *= rot;
    p /= scale;
    p -= vec3(0.,.05,2.5);
    float d = length(p-vec3(1.2,.2,-.35)) - .2;
    p.y -= .2*p.x*p.x;
    p.y *= cos(atan(.6*p.x));
    float n = clamp(p.x,-.7,1.);
    vec2 sg = vec2(length(p.xy-vec2(n,0)),(n+.7)/1.7),
         p0 = pow(abs(vec2(sg.x, p.z)), vec2(3));
    d = smin(d,pow(p0.x+p0.y, 1./3.) -(.3*pow(sg.y-.5,2.)+.2), .3);
    return vec2(.7*scale*d, BONE);
}
#endif

// Distance to Bezier
// inspired by [iq:https://www.shadertoy.com/view/ldj3Wh]
// calculate distance to 2D bezier curve on xy but without forgeting the z component of p
// total distance is corrected using pytagore just before return
vec2 sdBezier(vec2 m, vec2 n, vec2 o, vec3 p) {
	vec2 q = p.xy;
	m-= q; n-= q; o-= q;
	float x = U(m, o), y = 2. * U(n, m), z = 2. * U(o, n);
	vec2 i = o - m, j = o - n, k = n - m, 
		 s = 2. * (x * i + y * j + z * k), 
		 r = m + (y * z - x * x) * vec2(s.y, -s.x) / dot(s, s);
	float t = clamp((U(r, i) + 2. * U(k, r)) / (x + x + y + z), 0.,1.); // parametric position on curve
	r = m + t * (k + k + t * (j - k)); // distance on 2D xy space
	return vec2(sqrt(dot(r, r) + p.z * p.z), t); // distance on 3D space
}

//-----------------------------------------------------------------------------------
// iq - https://www.shadertoy.com/view/ldj3Wh
vec2 sdBezier(in vec3 p,in vec3 b0,in vec3 b1,in vec3 b2 ) {
    b0 -= p; b1 -= p; b2 -= p;
    vec3 b01 = cross(b0,b1), b12 = cross(b1,b2), b20 = cross(b2,b0),
         n =  b01+b12+b20;
    float a = -dot(b20,n), b = -dot(b01,n), d = -dot(b12,n), m = -dot(n,n);
    vec3  g =  (d-b)*b1 + (b+a*.5)*b2 + (-d-a*.5)*b0;
    float t = clamp((a*.5+b-.5*(a*a*.25-b*d)*dot(g,b0-2.*b1+b2)/dot(g,g))/m, 0., 1.);
    return vec2(length(mix(mix(b0,b1,t), mix(b1,b2,t),t)),t);
}

// Distance to scene
float sdTeapot(vec3 p) {
// Distance to Teapot --------------------------------------------------- 
	// precalcul first part of teapot spout
	vec2 h = sdBezier(T1[2],T1[3],T1[4], p);
	float a = 99., 
    // distance to teapot handle (-.06 => make the thickness) 
		b = min(min(sdBezier(T2[0],T2[1],T2[2], p).x, sdBezier(T2[2],T2[3],T2[4], p).x) - .06, 
    // max p.y-.9 => cut the end of the spout 
                max(p.y - .9,
    // distance to second part of teapot spout (abs(dist,r1)-dr) => enable to make the spout hole 
                    min(abs(sdBezier(T1[0],T1[1],T1[2], p).x - .07) - .01, 
    // distance to first part of teapot spout (tickness incrase with pos on curve) 
                        h.x * (1. - .75 * h.y) - .08)));
    // distance to teapot body => use rotation symetry to simplify calculation to a distance to 2D bezier curve
    vec3 qq= vec3(sqrt(dot(p,p)-p.y*p.y), p.y, 0);
    // the substraction of .015 enable to generate a small thickness arround bezier to help convergance
    // the .8 factor help convergance  
	for(int i=ZERO;i<13;i+=2) 
		a = min(a, (sdBezier(A[i], A[i + 1], A[i + 2], qq).x - .035) * .9); 
    // smooth minimum to improve quality at junction of handle and spout to the body
	return smin(a,b,.02);
}

// Interpolate pos of articulations
vec3 getPos(vec3 arr[9], int it, float kt, float z) {
    it = it%8;
    vec3 p = mix(arr[it], arr[it+1], kt);
	return .02*vec3(p.x+floor(gTime/8.)*168., 150.-p.y, p.z*z);
}

//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

vec2 hash22(vec2 p) {
    p = fract(p * vec2(5.3983, 5.4427));
    p += dot(p.yx, p.xy + vec2(21.5351, 14.3137));
    return fract(vec2(p.x * p.y * 95.4337, p.x * p.y * 97.597));
}

//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
//---------------------------------------------------------------------

float sdCap(vec3 p, vec3 a, vec3 b) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0., 1. );
    return length( pa - ba*h );
}

float sdCap2(vec3 p, vec3 a, vec3 b, float r1, float r2) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. );
    return length( pa - ba*h ) - mix(r1,r2,h);
}

float udRoundBox( vec3 p, vec3 b, float r ) {
  return length(max(abs(p)-b,0.))-r;
}

float sdCappedCylinder(vec3 p, vec2 h ) {
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.) + length(max(d,0.));
}

// approximated
float sdEllipsoid( in vec3 p, in vec3 r ) {
    float k0 = length(p/r);
    return k0*(k0-1.)/length(p/(r*r));
}

float sdEar(in vec3 p) {
    vec3 p_ear = 1.5*p;
    float d = max(-sdEllipsoid(p_ear-vec3(.005,.015,.02), vec3(.07,.1,.07)), 
                   sdEllipsoid(p_ear, vec3(.08,.12,.09)));
    d = max(p_ear.z, d); 
    d = smin(d, sdEllipsoid(p_ear+vec3(.035,.045,.01), vec3(.04,.04,.018)), .01);
    return d/1.5;
}


mat3 baseArm1, baseArm2, baseBag, baseFoot1, baseFoot2;

vec2 sdMan(in vec3 pos){
    vec3 p0 = pos;
    vec2 res = vec2(999,0);
    
    // Legs
    float dSkin = min(
        min(sdCap(pos, ankle1, knee1), 
            sdCap(pos, knee1, hip1)),
        min(sdCap(pos, ankle2, knee2),
            sdCap(pos, knee2, hip2)))-.1;
            
    // Foot1 flat part - vector base linked to leg 1
    float dShoes1 = max(sdCap(pos, foot1, ankle1) - .15, -dot((pos-ankle1)*baseFoot1-vec3(0,0,-.13), vec3(0,0,1))); 

    // Leg 2
    float dShoes2 = max(sdCap(pos, foot2, ankle2) - .15, -dot((pos-ankle2)*baseFoot2-vec3(0,0,-.13), vec3(0,0,1)));  
    
    vec3 ep0 = mix(shoulder1,shoulder2,.5),
         ha0 = mix(hip1,hip2,.5);

    // Head
    vec3 h1 = vec3(0,.17,0), h2 = vec3(.02,-.11,0),
         h = mix(h1,h2,.2);
    dSkin = min(dSkin, sdCap(pos, head - h, head - h2)-.25);
    
    vec3 posHead = pos-head;
    posHead.xz *= rot1;
    vec3 posEar = posHead;
    posEar.z = abs(posEar.z);
    posEar-=vec3(0.,-.08,.29);
    posEar.zx *= rot;
 
     // ear / noze
    dSkin = smin(dSkin, min(sdEar(posEar.zyx),
                            sdCap(posHead, - mix(h1,h2,.4), - mix(h1,h2,.4) + vec3(.28,0,0))- .04),.02);
    // Torso
    vec3 a = mix(ha0,ep0,.15), b = mix(ha0,ep0,.78);
    
    // Neck
    float dNeck = sdCap(pos, ep0-vec3(.08,0,0), head-vec3(.08,.1,0))- .1;
    dSkin = smin(dSkin, dNeck,.06);
  
    float dTorso = smin(sdCap(pos,shoulder1,shoulder2)-.11, sdCap2(pos, a, b, .23,.28),.095);
    dSkin = min(dSkin, dTorso);
   
    dTorso = smin(dTorso, sdCap(pos, shoulder1, mix(shoulder1,elbow1,.3))- .1,.05);

    // Arm 1
    dSkin = smin(dSkin, sdCap(pos, shoulder1, elbow1)- .1,.05);
    dSkin = min(dSkin, sdCap2(pos, elbow1, wrist1-.05*normalize(wrist1-elbow1), .1, .08));
   
    vec3 p2 = (pos-wrist1)*baseArm1; // change to hand base
    float d2 = sdCap(p2,vec3(-.1,.12,.04),vec3(-.04,.18,.06))-.05;
    p2.z -= 1.5*p2.x*p2.x;
    d2 = min(d2,sdEllipsoid(p2-vec3(.02,.05,0), vec3(.17,.14,.07)));

    // Arm 2
    dTorso = smin(dTorso, sdCap(pos, shoulder2, mix(shoulder2,elbow2,.3))- .1,.05);
    dSkin = smin(dSkin, sdCap2(pos, shoulder2, elbow2, .11,.1),.05);
    dSkin = min(dSkin, sdCap2(pos, elbow2, wrist2-.105*normalize(wrist2-elbow2), .1, .08));
    
    p2 = (pos - wrist2)*baseArm2; // change to hand base
    d2 = min(d2,sdCap(p2,vec3(-.1,.12,.04),vec3(-.04,.18,.06))-.05);
    p2.z -= 1.5*p2.x*p2.x;
    d2 = min(d2,sdEllipsoid(p2-vec3(.02,.05,0), vec3(.17,.14,.07)));
    
   	dSkin = smin(d2, dSkin, .1);

    // Short
    float dShort = min(sdCap(pos, hip1, mix(hip1,knee1,.7)), 
                       sdCap(pos, hip2, mix(hip2,knee2,.7)))-.14;  
    dShort = min(dShort, smin(dShort, sdEllipsoid(pos-ha0-vec3(.01,.06,0),vec3(.23,.3,.3)), .05));
 
    // Belt
    vec3 p3 = p0;
    p3.y -= ha0.y+.2;
    float dBelt = max(max(min(dTorso-.05,dShort-.02), p3.y), -p3.y-.16);
    dShoes1 = min(dShoes1, dBelt); 

    float dMetal = mix(dBelt,sdCappedCylinder((p0-ha0-vec3(.2,.11,0)).zxy, vec2(.07,.14)),.5);
    float dNeck2 = length(pos - ep0)-.25;
    dTorso = max(max(dTorso-.03, -dNeck2), -dSkin + .005);
    dTorso = max(dTorso, -pos.y + a.y);

    pos.x += .2;
    pos = pos - ep0;
    pos *= baseBag;
    vec3 pos0 = pos;
    
    // Backpack
    float ta = cos(2.8+.25*PI*gTime);

    // Water
    vec3 p = pos - vec3(-.50,-.5,-.4);
    p.xy *= rot2;
    dMetal = min(dMetal, min(sdCap(p,vec3(0), vec3(0,-.4-.04*ta,-.04*ta))- .1,
                             sdCap(p,vec3(0,.1,.01*ta), vec3(0,.15,.015*ta))-.07)); 
    pos.z = abs(pos.z);
    float dPack = sdBezier(pos, vec3(-.2,.2,.17),vec3(.9,.2,.36),vec3(-.2,-.6,.3)).x-.04;
   
    dPack = mix(dPack, dTorso,.2);

    pos = pos0;
  
    float ta2 = cos(2.5+.25*PI*gTime);
    pos.y += .15*ta2*ta2;
    pos.z += .06*ta2*ta2*ta2;
  
    float dBed = sdCappedCylinder(pos.yzx+vec3(-.35,0,.5), vec2(.2,.5)); 
    
    // Teapot
    vec3 posTeapot = 3.*(pos - vec3(-.9,.25,.4)).yxz;
    posTeapot.xy *= rot2;
    posTeapot.yz *= rot;
    posTeapot.yx *= -1.; 
    float dTeapot = .7*sdTeapot(posTeapot-vec3(1.,-1.,0))/3.;
    
    dPack = min(dPack, sdCappedCylinder(pos-vec3(-.75,.23,.26), vec2(.05,.03))); 
    pos.z = abs(pos.z);
    dPack = min(dPack, sdCappedCylinder(pos.yzx-vec3(.35,.25,-.5), vec2(.22,.05))); 
    dPack = min(dPack, udRoundBox(pos0-vec3(-.33,-.25,-.35), vec3(.08,.1,.1),.04)); 
    dPack = smin(dPack, udRoundBox(pos0-vec3(-.33,-.2,0), vec3(.1,.3,.2), .15), .12);
    
    // Little box
    dBed = min(dBed, udRoundBox(pos0-vec3(-.33,-.1,-.35), vec3(.08,.1,.1),.0)); 
    
    // Cap
    pos = posHead + h2;
    float d = pos.x*.2-pos.y+.15*cos(5.*pos.z)-.12;
    float dHat = max(sdCap(pos, vec3(0), vec3(.2,0,0))-.27, -d);
    dHat = min(dHat, mix(dSkin, sdCappedCylinder(pos, vec2(.25,.23)),.4)-.01);      
    dHat = max(dHat, d-.02);
    dPack = min(dPack, dHat);

    // Asssociate distance to materials
    res = min2(res, vec2(dTeapot, TEAPOT));
    res = min2(res, vec2(dTorso, SHIRT));
    res = min2(res, vec2(dShort, SHORT));
    res = min2(res, vec2(dShoes1, SHOES1));
    res = min2(res, vec2(dShoes2, SHOES2));
    res = min2(res, vec2(dSkin, SKIN));
    res = min2(res, vec2(dMetal, METAL));
    res = min2(res, vec2(dPack, BAG));
    res = min2(res, vec2(dBed, BED));
    
    // Distance field is not percise for cap, teapot and hands
    res.x *= .8;
    return res;
}

#ifdef PANEL

float sdFont(in vec2 p, in int c) {
    vec2 uv = (p + vec2(float(c%16), float(15-c/16)) + .5)/16.;
    return max(max(abs(p.x) - .25, max(p.y - .35, -.38 - p.y)), textureLod(iChannel1, uv, 0.).w - 127./255.);
}

float sdMessage2D(in vec2 p, int i0, in int[15] txt, in float scale) { 
    p /= scale;
 	float d = 999., w = .45; // letter width  
    p.x += w*float(txt.length()-1)*.5; // center text arround 0
    for (int id = i0; id<15; id++){
        if (txt[id] == 0) break;
    	d = min(d, sdFont(p, txt[id]));   
    	p.x -= w; 
    }
    return scale*d;
}

vec2 sdPanel(vec3 p) {
    p.x -= 300.;
    p.z += 2.5;
    float d = udRoundBox(p, vec3(.05,1.9,.05),.01);
    d = min(d, udRoundBox(p-vec3(0,1.5,.07), vec3(.7,.25,.02),.01));  
    return vec2(d, PANEL);
}

#endif

vec2 map(in vec3 p0){
    // Little stones    
    vec2 size = vec2(35.,20.),
         id = floor((p0.xz + size*0.5)/size);
    vec3 pos = p0;
    pos.xz = mod(p0.xz + size*0.5,size) - size*0.5;
    vec2 h = 1.-2.*hash22(id);
    float r = .15+.25*abs(h.x),
          d = length(pos - vec3(h.x*5.,-r*.4,7.*h.y))-r;
    vec2 res = vec2(d,GROUND);
#ifdef PANEL
    res = min2(res, sdPanel(p0)); 
#endif
#ifdef BONE
    res = min2(res, sdBone(p0));
#endif    
    d = length(p0-hip1)-2.;
    if (d<0.) {
        return min2(sdMan(p0), res);
    } else {    
        return min2(vec2(d+.1,999.), res);
    }
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------

vec2 Trace( in vec3 ro, in vec3 rd) {
    vec2 res = vec2(999,0);
    float t = .5;
    for( int i=ZERO; i<128 && t<100.; i++ ) {
        vec2 h = map( ro+rd*t);
        if( abs(h.x)<.0005*t ) { 
            res = vec2(t,h.y); 
            break;
        }
        t += h.x;
    }
    return res;
}

//------------------------------------------------------------------------
// [Shane] - Desert Canyon - https://www.shadertoy.com/view/Xs33Df
//------------------------------------------------------------------------
// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
float tex3D(sampler2D tex, in vec3 p, in vec3 n){
    n = max(n*n, .001);
    n /= (n.x + n.y + n.z );  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).x;
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
vec3 doBumpMap( sampler2D tex, in vec3 p, in vec3 n, float k){
    const float ep = .001;
    vec3 grad = vec3( tex3D(tex, vec3(p.x-ep, p.y, p.z), n),
                      tex3D(tex, vec3(p.x, p.y-ep, p.z), n),
                      tex3D(tex, vec3(p.x, p.y, p.z-ep), n));
    grad = (grad - tex3D(tex, p, n))/ep;             
    grad -= n*dot(n, grad);          
    return normalize(n + grad*k);
}

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
float calcAO( in vec3 pos, in vec3 nor ){
	float dd, hr, sca = 1., totao = 0.;
    vec3 aopos; 
    for( int aoi=ZERO; aoi<5; aoi++ ) {
        hr = .01 + .05*float(aoi);
        aopos = nor * hr + pos;
        totao += -(map(aopos).x-hr)*sca;
        sca *= .75;
    }
    return clamp(1. - 4.*totao, 0., 1.);
}
#endif

float textureInvader(vec2 uv) {
	float y = 7.-floor((uv.y)*16.+4.);
	if (y<0. || y>7.) return 0.;
	float x = floor((abs(uv.x))*16.),
	      v=y>6.5? 6.:y>5.5? 40.:y>4.5? 47.:y>3.5?63.:
			y>2.5? 27.:y>1.5? 15.:y>0.5? 4.:8.;
	return floor(mod(v/pow(2.,x), 2.0)) == 0. ? 0.: 1.;
}

vec3 doColor(in vec3 p, in vec3 rd, in vec3 n, in vec2 res){
    // sky dome
    vec3 skyColor =  .5*vec3(.5, .6, .9),
         col = skyColor - max(rd.y,0.)*.5;
  
    float ss = .5, sp = 0.;
#ifdef BONE    
    if (res.y == GROUND || res.y == BONE) {
#else    
    if (res.y == GROUND) {
#endif    
        col = .7+ .5 *vec3(texture(iChannel0,.1*p.zx).x)+p.y;
        col.r *= .8;
        n = doBumpMap(iChannel0, .1*p, n, .002);
        ss = 0.;
        sp = .3;
    } else 
    if (res.y == SHOES1) {
        sp = .1;
        vec3 pFoot = (p-ankle1)*baseFoot1;
        col = mix(vec3(.1,.1,0), vec3(0,0,.1), smoothstep(-.1,-.09,pFoot.z));
    } else if (res.y == SHOES2) {
        col = vec3(0,0,.1);
        sp = .1;
        vec3 pFoot = (p-ankle2)*baseFoot2;
        col = mix(vec3(.1,.1,0), vec3(0,0,.1), smoothstep(-.1,-.09,pFoot.z));
    } else if (res.y == SKIN) {
        col = vec3(222,177,144)/255.;
        ss = 1.;
        sp = .1;
        if (p.x>head.x) {
			// Draw simple face
            vec3 phead = (p - head);
            phead.xz *= rot1;
			vec2 p2 = phead.zy;
            p2.x = abs(p2.x);
            float d = length(p2-vec2(.1,0))-.02;
            d = min(d, max(length(p2-vec2(0.,-.18))-.05, -length(p2-vec2(0.,-.14))+.07))            ;
            col = mix(vec3(0), col, smoothstep(.0,.01,d));
		}
    } else if (res.y == METAL) {
        col = vec3(.8,.9,1.);
        sp = 2.;
    } else if (res.y == SHORT) {
        col = vec3(.35,.7,.85);
    } else if (res.y == BAG) {
//        vec3 ep0 = mix(shoulder1,shoulder2,.5);
//        p.x += .2;
//        p -= ep0;
        col = vec3(.3,.5,.2);
//        nor = doBumpMap(iChannel0, .1*(p*baseBag), nor, .002);
        sp = .1;
    } else if (res.y == SHIRT) {
        col = vec3(.3,.4,.5);
        vec2 p2 = p.zy;
        p2.y -= mix(mix(shoulder2,shoulder1,.5),
                    mix(hip2,hip1,.5),.5).y;
        col *= 1.-.5*textureInvader(p2*1.9-vec2(0,.3));
        ss = .2;
    } else if (res.y == TEAPOT) {
        col = vec3(1.,.01,.01);
        sp = .5;
    } else if (res.y == BED) {
        col = vec3(1.,.5,.01);
        ss = 1.;
        sp = .3;
    }
#ifdef PANEL
    else if (res.y == PANEL) {
        col = vec3(.8,.4,.1);
        if (p.z > -2.4) {
            int[] gtxt = int[] (72,69,76,76,79,0,0,70,65,66,82,73,67,69,33);
            float d = min(sdMessage2D(p.xy-vec2(300.66,1.6),0, gtxt, .3),
                          sdMessage2D(p.xy-vec2(300.5,1.4),7, gtxt, .3));
            col = mix(vec3(0), col, smoothstep(.0,.01,d));
        }
    }
#endif
    else {
        return col;
    }
#ifdef BONE    
    if (res.y == BONE) {
        ss = 1.;
        sp = 1.;
    }
#endif    
    vec2 d = vec2(res.x, res.y);
    vec3 ld = -normalize(vec3(50,100,-100)-p);
    // IQ sss version
    float sss = ss*0.2*clamp(0.5+0.5*dot(ld,n),0.0,1.0)*(2.0+dot(rd,n));
    vec3 r = reflect(rd,n);
    float diff = max(0.,dot(n,ld)),
         amb = dot(n,ld)*.45+.55,
         spec = pow(max(0.,dot(r,ld)),40.),
         fres = pow(abs(.7+dot(rd,n)),3.),   
         ao = calcAO(p, n);
    // ligthing     
    col = col*mix(1.2*vec3(.25,.08,.13),vec3(.984,.996,.804), mix(amb,diff,.75)) + 
          spec*sp+fres*mix(col,vec3(1),.7)*.4;
    // kind of sub surface scatering      
    col += sss*vec3(1.,.3,.2);
    // sky light reflected from the ground
    col += max(0.,dot(vec3(0,-1,0), n))*.1*skyColor;
    // ambiant occusion
    col *= mix(ao,1.,.5);
    // fade in distance
    return mix( col, skyColor, smoothstep(30.,100., res.x) );
}


//---------------------------------------------------------------------
//   Calculate normal
// inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
//---------------------------------------------------------------------
vec3 normal(in vec3 pos, vec3 rd, float t ) {
    vec3 n = vec3(0);
    for( int i=ZERO; i<4; i++) {
        vec3 e = .5773*(2.*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.);
        n += e*map(pos+.002*e).x;
    }
	return normalize(n - max(0., dot(n,rd ))*rd);
}

//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

mat3 setCamera( in vec3 ro, in vec3 ta, in float r) {
	vec3 w = normalize(ta-ro),
         p = vec3(sin(r), cos(r),0.),
         u = normalize( cross(w,p) ),
         v = normalize( cross(u,w) );
    return mat3( u, v, w );
}

//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------
//#define iTime (iTime + 250.) // Direct to big Bone
void mainImage(out vec4 fragColor, in vec2 fragCoord) {

    // Teapot body profil (8 quadratic curves) 
	A[0]=vec2(0,0);A[1]=vec2(.64,0);A[2]=vec2(.64,.03);A[3]=vec2(.8,.12);A[4]=vec2(.8,.3);A[5]=vec2(.8,.48);A[6]=vec2(.64,.9);A[7]=vec2(.6,.93);
    A[8]=vec2(.56,.9);A[9]=vec2(.56,.96);A[10]=vec2(.12,1.02);A[11]=vec2(0,1.05);A[12]=vec2(.16,1.14);A[13]=vec2(.2,1.2);A[14]=vec2(0,1.2);
	// Teapot spout (2 quadratic curves)
	T1[0]=vec2(1.16, .96);T1[1]=vec2(1.04, .9);T1[2]=vec2(1,.72);T1[3]=vec2(.92, .48);T1[4]=vec2(.72, .42);
	// Teapot handle (2 quadratic curves)
	T2[0]=vec2(-.6, .78);T2[1]=vec2(-1.16, .84);T2[2]=vec2(-1.16,.63);T2[3]=vec2(-1.2, .42);;T2[4]=vec2(-.72, .24);

    gTime = iTime*6.;
   
    // Animation
    int it = int(floor(gTime));
    float kt = fract(gTime), dz = 1.;
   
    head = getPos(HEAD, it, kt, dz);

    shoulder1 = getPos(SHOULDER, it, kt, -dz);
    elbow1 = getPos(ELBOW, it, kt, -dz);
    wrist1 = getPos(WRIST, it, kt, -dz);
    
    foot1 = getPos(FOOT, it, kt, dz);
    ankle1 = getPos(ANKLE, it, kt, dz);
    knee1 = getPos(KNEE, it, kt, dz);
    hip1 = getPos(HIP, it, kt, dz);
    
    shoulder2 = getPos(SHOULDER, it+4, kt, dz);
    elbow2 = getPos(ELBOW, it+4, kt, dz);
    wrist2 = getPos(WRIST, it+4, kt, dz);

    foot2 = getPos(FOOT, it+4, kt, -dz);
    ankle2 = getPos(ANKLE, it+4, kt, -dz);
    knee2 = getPos(KNEE, it+4, kt, -dz);
    hip2 = getPos(HIP, it+4, kt, -dz);
    
    float dx = it%8 < 4 ? -85.*.02 : 85.*.02; 
    foot2.x += dx;
    ankle2.x += dx;
    knee2.x += dx;
    hip2.x += dx;

    shoulder2.x += dx;
    elbow2.x += dx;
    wrist2.x += dx;
    
    vec3 v1 = normalize(wrist1-elbow1),
    v0 = normalize(wrist1-shoulder1),
    v3 = normalize(cross(v1,v0)),
    v2 = cross(v1,v3);
    baseArm1 = mat3(v0,v2,-v3);
    
    v1 = normalize(wrist2-elbow2),
    v0 = normalize(wrist2-shoulder2),
    v3 = normalize(cross(v1,v0)),
    v2 = cross(v1,v3);
    baseArm2 = mat3(v0,v2,v3);
    
    v1 = normalize(shoulder1-shoulder2);
    v0 = normalize(mix(hip1,hip2,.5)-mix(shoulder1,shoulder2,.5));
    v2 = normalize(cross(v1,v0));
    v3 = normalize(cross(v1,v2));
    baseBag = mat3(-v2,v3,v1);
    
    v2 = normalize(knee1 - ankle1);
    v1 = normalize(ankle1 - foot1-v2*.1);
    v3 = cross(v1,v2);
    baseFoot1 = mat3(v1,v3,-cross(v1,v3));
    
    v2 = normalize(knee2 - ankle2);
    v1 = normalize(ankle2 - foot2-v2*.1);
    v3 = cross(v1,v2);
    baseFoot2 = mat3(v1,v3,-cross(v1,v3));
    
    float a = -1.5708*.4;
    rot = mat2(cos(a), sin(a), -sin(a), cos(a));
     
    a = .2*cos(.4*iTime) + .3*cos(.05*iTime);
    rot1 = mat2(cos(a), sin(a), -sin(a), cos(a));
     
    a = .5*cos(.5*3.141592*gTime);
    a = a*a;
    rot2 = mat2(cos(a), sin(a), -sin(a), cos(a));
    
// ------------------------------------
 
    // Screen 
    vec2 q = fragCoord.xy/iResolution.xy, 
         m = iMouse.xy/iResolution.y - .5,
         p = -1.+2.*q;
    p.x *= iResolution.x/iResolution.y;        
      
    // Camera	
    vec3 ro = vec3(  hip1.x+12.*cos(PI*(.05*iTime+m.x)),
                4.5+2.*(sin(.1*iTime))+4.*(m.y+.3),
                hip1.z+12.*sin(PI*(.05*iTime+m.x)));
    vec3 ta = hip1;
    ta.x +=1.2;
    ta.y = 1.2;
    mat3 ca = setCamera(ro, ta, 0.);
    vec3 rd = ca * normalize(vec3(p.xy,4.5) );
  
    // Ray intersection with scene
    vec2 res = Trace(ro, rd);
    if (rd.y >= 0.) {
       res = min2(res, vec2(999.,100.));
    } else {        
       res = min2(res, vec2(-ro.y / rd.y,GROUND));
    }
    
    // Rendering
    vec3 pos = ro + rd*res.x;
    vec3 n = pos.y<.02 ? vec3(0,1,0) : normal(pos, rd, res.x);
    vec3 col = doColor(pos, rd, n, res);
    col = pow( col, vec3(.4545) );                 // Gamma    
    col *= pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y), .1); // Vigneting
     
	fragColor = vec4(col,1);
}
