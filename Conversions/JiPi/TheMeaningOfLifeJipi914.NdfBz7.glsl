

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by Sebastien DURAND - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//----------------------------------------------------------------
// Thanks to
// Iq: Deformed tubes, distance field, shadows, etc. 
// Shane: Texture 3D, render
//----------------------------------------------------------------

// The far plane. I'd like this to be larger, but the extra iterations required to render the 
// additional scenery starts to slow things down on my slower machine.
#define FAR 80.
#define PI 3.14159265
#define ZERO min(0,iFrame)

const float slab = 0.05;
const float ani0 = 2.,  // Start Grow
      ani1 = 6.,  
      ani2 = 10.,  // start graine
      ani3 = 20.,  // start move cam up
      ani4 = 30.,  // start move to frogs
	  ani5 = 46.,  // start turn arround frogs
      ani5b = 64., // To center of frogs
      ani5b2 = 83., // Eye bottom
      ani5c = 86., // Move hand 
      ani5d = 101., // Enter the ground
	  ani6 = 122., // Under the ground
 	  ani7 = 129.;

const mat2 rot = mat2(cos(-.3),sin(-.3),-sin(-.3),cos(-.3));

float dhaloLight, dhaloFrog;
float sanim01, sanim12, sanim23, sanim34, sanim45, sanim56, sanim5cd, sanim56r, sanim67;
float gPulse, gPulseGround;

// --------------------------------------------------------------

vec2 rotate( vec2 v, float a ) { return vec2( v.x*cos(a)+v.y*sin(a), -v.x*sin(a)+v.y*cos(a) ); }
vec2 sincos( float x ) { return vec2( sin(x), cos(x) ); }
vec3 opU( vec3 d1, vec3 d2 ){ return (d1.x<d2.x) ? d1 : d2;}


// --------------------------------------------------------------
// hash functions
// --------------------------------------------------------------
float hash( vec2 p ) { return fract(sin(1.0+dot(p,vec2(127.1,311.7)))*43758.545); }
float hash( float n ){ return fract(cos(n)*45758.5453); }
float hash( vec3 p ) { return fract(sin(dot(p, vec3(7, 157, 113)))*45758.5453); }
vec3 hash3( vec2 p ) {
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}
// --------------------------------------------------------------



// Grey scale.
float getGrey(vec3 p){ return dot(p, vec3(0.299, 0.587, 0.114)); }


// IQ's smooth minium function. 
float sminP(float a, float b , float s){
    float h = clamp(.5 + .5*(b-a)/s, 0. , 1.);
    return mix(b, a, h) - h*(1.-h)*s;
}

// Smooth maximum, based on the function above.
float smaxP(float a, float b, float s){
    float h = clamp( .5 + .5*(a-b)/s, 0., 1.);
    return mix(b, a, h) + h*(1.-h)*s;
}


// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
    n = max(n*n, 0.001);
    n /= (n.x + n.y + n.z );  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}


//--------------------------------------------------
// From Mercury
// Repeat around the origin by a fixed angle.
// For easier use, num of repetitions is use to specify the angle.
float pModPolar(inout vec2 p, float rep) {
	float angle = 2.*PI/rep,
         a = atan(p.y, p.x) + angle*.5,
         r = length(p),
         c = floor(a/angle);
	a = mod(a, angle) - angle*.5;
	p = vec2(cos(a), sin(a))*r;
	// For an odd number of repetitions, fix cell index of the cell in -x direction
	// (cell index would be e.g. -5 and 5 in the two halves of the cell):
	if (abs(c) >= rep*.5) c = abs(c);
	return c;
}
//-----------------------------------------------------


float sdSegment( vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1. );	
    return length(pa - ba*h) - r;
}

float sdEllipsoid( in vec3 p, in vec3 r) {
    float k0 = length(p/r), k1 = length(p/(r*r));
    return k0*(k0-1.0)/k1;
}

// capsule with bump in the middle -> use for arms and legs
float sdBumpCapsule( vec3 p, vec3 a, vec3 b, float r, float k) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. ),
    	 dd = k*cos(3.141592*h+1.57);  // Little adaptation
    return length(pa - ba*h) - r+dd; 
}

// ------------------------------------------------------------------

float sdFrog(vec3 p) {

    float id = pModPolar(p.xz, 12.);
    p.x -= 12.;
    p.y += .1;
    
    float scale = .4 + .2*hash(id);
    p /= scale;
    p.xz += 2.*fract(11.*scale);
    
    float dFrog = length(p-vec3(.31,1.6,0));
    if (dFrog> 3.3+1./scale) return dFrog;
    
    float kRot = sanim5cd*.2*cos(id + 5.*iTime); 
    p.xz = rotate(p.xz, .5*kRot);
    vec3 pr = p;
    pr.xy *= rot;
    
    float sgn = sign(p.z);
    p.z = abs(p.z);
    pr.z = abs(pr.z);
    
    float dEye = length(pr-vec3(-2.,2.,.7)) - .7;
  
    float dLeg = sminP(sdBumpCapsule(p,vec3(2.1,.7,.3),vec3(.0,2.5, 2.7),.2,.3),
                     sminP(sdBumpCapsule(p,vec3(0.,2.4,2.8),vec3(1.8,.6,1.3),.2,.2),
                           sdSegment(p,vec3(1.8,.6,1.3),vec3(1.2,.3,1.6),.2),.05) ,.05);
    float dFeet =  min(sdSegment(p,vec3(1.1,.25,1.55),vec3(.2,.3,1.5),.08),
                       min(sdSegment(p,vec3(1.2,.25,1.6),vec3(-.0,.3,2.),.08),
    					   sdSegment(p,vec3(1.1,.25,1.75),vec3(.3,.3,2.3),.08)));
    float dFinger = min(min(length(p-vec3(.3,.3,1.5)),
                        length(p-vec3(.1,.3,2.))),
                        length(p-vec3(.4,.3,2.3))) - .12;
  
    vec3 pFinger = p;
    pFinger.z += 4.*sgn*kRot;
    
    float dLeg2 = sminP(sdBumpCapsule(p,vec3(-1.,1.6,1.6),vec3(-.2,1.2, 2.2-sgn*kRot),.2,.1),
                     sdBumpCapsule(p,vec3(-.2,1.2,2.3-sgn*kRot),vec3(-1.,.3,2.-4.*sgn*kRot),.2,.1),.1);
    
    float dFeet2 =  min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-1.8,.3,1.5),.1),
                       min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-2.,.3,2.),.1),
    				       min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-1.3,.3,1.1),.1),
                               sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-.85,.3,1.2),.1)
                              )));
    float dFinger2 = min(min(length(pFinger-vec3(-1.8,.3,1.5)),
                             length(pFinger-vec3(-2.,.3,2.))),
                         min(length(pFinger-vec3(-1.3,.3,1.1)),
                             length(pFinger-vec3(-.85,.3,1.2))) ) - .12;
    
    dFeet2 = sminP(dFinger2, dFeet2,.1); 
    dFeet = sminP(dFinger, dFeet,.1); 
    dLeg = sminP(dLeg, dFeet,.2);
    dLeg2 = sminP(dLeg2, dFeet2,.2);
    
    float dd = max(max(.5-p.y, dot(pr-vec3(3.,1.3,0), normalize(vec3(1,2,1)))),
                   max(dot(pr-vec3(-3.4,1.8,-.1), normalize(vec3(-1.9,1.,2))),
                       min(.8-pr.y, dot(pr-vec3(-.5,.3,2.), normalize(vec3(-1.5,-2.2,1))))));
	
    float dBody = sdEllipsoid(pr-vec3(-.5,1.2,0), vec3(2.7,1.3,2));
    dBody = smaxP(dd, dBody, .1);
    dBody = sminP(dBody, dEye, .2);
    
    float d = dBody;
    
    d = smaxP(dd, d, .1);
    d = sminP(d, dEye, .2);
    d = smaxP(d, -min(dLeg,dLeg2), .3);
    d = sminP(d, dLeg, .2);
    d = sminP(d, dLeg2, .15);
    d = smaxP(d, -(length(p - vec3(-1.5,2.4,.8)) - .5), .4);
    d = smaxP(d, -(length(pr - vec3(-3.05,1.55,.18))), .1);
    
    float kFrog = .55*smoothstep(.9,1.,cos(iTime+102.*id));
    d = sminP(d, sdEllipsoid(pr-vec3(-2.,.65-.2*kFrog,0.), mix(vec3(.5,.15,.9), vec3(1.,1.,1.8), kFrog)), .2);
    dEye = length(p-vec3(-1.5,2.4,.8)) - .5;
    
    return scale*min(d,dEye);
}


float mapTube( vec3 p ) {
    vec2 id = floor( (p.xz+5.0)/10. );
    
    float k = hash(id.x+101.*id.y);
    if (k>.3 || k<.1) return 999.;
    
    float tt = mod(iTime,1.5)/1.5;
    float ss = pow(tt,.2)*0.5 + 0.5;
    float pulseY = sanim01*(.25+ss*0.5*sin(tt*6.2831*3.+p.y)*exp(-tt*4.0));
    ss = pulseY*.25;

    p.xz = mod(p.xz+5.0, 10.) - 5.;
    p.xz += .5*sin( 2.0 + p.y*vec2(.53,.32) - vec2(1.57,.0) );

    return min( min(length(p.xz+.15*sincos(p.y)), 
                    length(p.xz+.15*sincos(p.y+4.))) - .15*(.8+.2*sin(2.*p.y)) - ss, 
                min(length(p.xz+.15*sincos(p.y+2.)) - .15*(.8+.2*sin(2.*p.y + 2.*(p.y-iTime)))-ss-.01, 
                    length(p.xz+.15*sincos(p.y+5.)) - .08*(.8+.2*sin(2.*p.y + ss + 8.*(p.y-iTime)))-.02-.3*ss));
}


float map( vec3 pos, bool light) {
    vec3 p0 = pos;

    float h = mix(1.,.1, smoothstep(20., 14., length(pos.xz))) + gPulseGround;
    h *= texture(iChannel2, -p0.xz*.02).x;
    pos.y -= h;
    
    float kEat = sanim56*(1.+.1*gPulse); 
    vec3 pFrog = p0-vec3(0, - 1.5*kEat,0);
    
    float dFrog = iTime < ani4 ? 999. : sdFrog(pFrog);

    float dTube = mapTube(pos);

    vec2 id = floor( (pos.xz-1.)/2.);
    
    pos.xz = mod(pos.xz+1., 2.) - 1.;
    pos.xz += .2*sin(dot(120.*id,vec2(1213.15,1317.34)));
    pos.xz += vec2(1,-1)*.2*sin(3.5*iTime +3.*cos(pos.y))*sin(.5*pos.y);

    vec3 posButton = pos;
    
    float len = max(sanim01,.1)*(.5+.4*smoothstep(.4,.5,cos(.3*iTime+id.x))+0.3*sin(dot(110.*id,vec2(1213.15,1317.34))));
    float thi = sanim01*(.8+.4*cos(.4*iTime)) * slab * (0.5+0.3*sin(-3.151592*posButton.y/len));
   
    float d = 999.;
     
    if (hash(id.x+11.*id.y) > .6) {
        d = sdSegment( posButton, vec3(0.,-len*.25,0.), vec3(0,len,0), 4.*thi);
        float dlight = length(pos-vec3(0,fract(1.+cos(id.x+3.1*id.y)+iTime*.1)*15.,0))-.05*sanim23;
        if (light) dhaloLight = min(dhaloLight, pos.y > len ? dlight-.02*sanim23 : 9999.);
        if (sanim23 > 0.) {
            d = sminP(d, dlight, .3);
        }
    }
    if (light && sanim56 > 0.) {
        dhaloFrog = min(dhaloFrog, dFrog -.02*sanim56); 
    }

    // Bump arround frog ---------------------------------

    float idFrog = pModPolar(pFrog.xz, 12.);
    pFrog.x -= 12.;
    float scale = .4 + .2*hash(idFrog);
    pFrog /= scale;
    pFrog.xz += 2.*fract(11.*scale);
 
    float dBumpFrog = scale*(length(pFrog-vec3(-1.,1.7-2.1*kEat*kEat,0))-4.*kEat);
    // ---------------------------------------------------

    d = sminP(d, pos.y, .3);
    d = sminP(d, dBumpFrog, .6);
    d = smaxP(d,-dFrog, .3);

    
//    return min(dFrog, min(dTube, smaxP(-min(dTube-.3,length(p0.xz)-4.2), d, 1.)));
    return min(dFrog, min(dTube, smaxP(-dTube+.3, d, 1.)));
}


float textureFrog(vec3 p, out vec4 out_posIdFrog) {
    float id = pModPolar(p.xz, 12.);
    p.x -= 12.;
    p.y += .1;
    
    float scale = .4 + .2*hash(id);
    p /= scale;
    p.xz += 2.*fract(11.*scale);
    
    float dFrog = length(p-vec3(.31,1.6,0));
    if (dFrog> 3.3+1./scale) return dFrog;
        
   
    float kRot = sanim5cd*.2*cos(id + 5.*iTime); 
    p.xz = rotate(p.xz, .5*kRot);
    vec3 p0 = p;
    vec3 pr = p;
    pr.xy *= rot;
    
    float sgn = sign(p.z);
    p.z = abs(p.z);
    pr.z = abs(pr.z);
    
    float dEye = length(pr-vec3(-2.,2.,.7)) - .7;
  
    float dLeg = sminP(sdBumpCapsule(p,vec3(2.1,.7,.3),vec3(.0,2.5, 2.7),.2,.3),
                     sminP(sdBumpCapsule(p,vec3(0.,2.4,2.8),vec3(1.8,.6,1.3),.2,.2),
                          sdSegment(p,vec3(1.8,.6,1.3),vec3(1.2,.3,1.6),.2),.05) ,.05);
    float dFeet =  min(sdSegment(p,vec3(1.1,.25,1.55),vec3(.2,.3,1.5),.08),
                       min(sdSegment(p,vec3(1.2,.25,1.6),vec3(-.0,.3,2.),.08),
    				sdSegment(p,vec3(1.1,.25,1.75),vec3(.3,.3,2.3),.08)));
    float dFinger = min(min(length(p-vec3(.3,.3,1.5)),
                        length(p-vec3(.1,.3,2.))),
                        length(p-vec3(.4,.3,2.3))) - .12;
  
    vec3 pFinger = p;
    pFinger.z += 4.*sgn*kRot;
    float dLeg2 = sminP(sdBumpCapsule(p,vec3(-1.,1.6,1.6),vec3(-.2,1.2, 2.2-sgn*kRot),.2,.1),
                     sdBumpCapsule(p,vec3(-.2,1.2,2.3-sgn*kRot),vec3(-1.,.3,2.-4.*sgn*kRot),.2,.1),.1);
    
    float dFeet2 =  min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-1.8,.3,1.5),.1),
                       min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-2.,.3,2.),.1),
    				       min(sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-1.3,.3,1.1),.1),
                               sdSegment(pFinger,vec3(-1.,.2,2.),vec3(-.85,.3,1.2),.1)
                              )));
    float dFinger2 = min(min(length(pFinger-vec3(-1.8,.3,1.5)),
                             length(pFinger-vec3(-2.,.3,2.))),
                         min(length(pFinger-vec3(-1.3,.3,1.1)),
                             length(pFinger-vec3(-.85,.3,1.2))) ) - .12;
    
    dFeet2 = sminP(dFinger2, dFeet2,.1); 
    dFeet = sminP(dFinger, dFeet,.1); 
    dLeg = sminP(dLeg, dFeet,.2);
    dLeg2 = sminP(dLeg2, dFeet2,.2);
    
    float dd = max(max(.5-p.y, dot(pr-vec3(3.,1.3,0), normalize(vec3(1,2,1)))),
                   max(dot(pr-vec3(-3.4,1.8,-.1), normalize(vec3(-1.9,1.,2))),
                       min(.8-pr.y, dot(pr-vec3(-.5,.3,2.), normalize(vec3(-1.5,-2.2,1))))));
	
    float dBody = sdEllipsoid(pr-vec3(-.5,1.2,0), vec3(2.7,1.3,2));
    dBody = smaxP(dd, dBody, .1);
    dBody = sminP(dBody, dEye, .2);
    
    float d = dBody;
    
    d = smaxP(dd, d, .1);
    d = sminP(d, dEye, .2);
    d = smaxP(d, -min(dLeg,dLeg2), .3);
    d = sminP(d, dLeg, .2);
    d = sminP(d, dLeg2, .15);
    d = smaxP(d, -(length(p - vec3(-1.5,2.4,.8)) - .5), .4);
    d = smaxP(d, -(length(pr - vec3(-3.05,1.55,.18))), .1);
    
    float kFrog = .55*smoothstep(.9,1.,cos(iTime+102.*id));
    d = sminP(d, sdEllipsoid(pr-vec3(-2.,.65-.2*kFrog,0.), mix(vec3(.5,.15,.9), vec3(1.,1.,1.8), kFrog)), .2);
    vec3 pEye = p-vec3(-1.5,2.4,.8);
    dEye = length(pEye) - .5;
    
    out_posIdFrog = d<dEye ? vec4(p0, 20.+abs(id)) : vec4(pEye, 31.+abs(id));
    
    return scale*min(d,dEye);
}


float texturePtTube(vec3 p, out vec4 out_idPosTube) {

    vec2 id = floor( (p.xz+5.0)/10. );

    float k = hash(id.x+101.*id.y);
    if (k>.3 || k<.1) return 999.;
    
    float tt = mod(iTime,1.5)/1.5;
    float ss = pow(tt,.2)*0.5 + 0.5;
    float pulseY = sanim01*(.25+ss*0.5*sin(tt*6.2831*3.0+p.y)*exp(-tt*4.0));
    ss = pulseY*.25;

    p.xz = mod( p.xz+5.0, 10.0 ) - 5.0;
    p.xz += 0.5*sin( 2.0 + (p.y)*vec2(0.53,0.32) - vec2(1.57,0.0) );

    vec3 p1 = p; p1.xz += 0.15*sincos(p.y);
    vec3 p2 = p; p2.xz += 0.15*sincos(p.y+2.0);
    vec3 p3 = p; p3.xz += 0.15*sincos(p.y+4.0);
    vec3 p4 = p; p4.xz += 0.15*sincos(p.y+5.0);   
    
    float h1 = length(p1.xz),
         h2 = length(p2.xz),
     	 h3 = length(p3.xz),
     	 h4 = length(p4.xz);

    vec3 res = opU( opU(vec3(h1-0.15*(0.8+0.2*sin(2.*p.y))-ss, 10., p.y), 
                    	vec3(h2-0.15*(0.8+0.2*sin(2.*p.y+2.0*(p2.y-iTime)))-ss-.01, 11., p.y)), 
                    opU(vec3(h3-0.15*(0.8+0.2*sin(2.*p.y))-ss, 12., p.y),
        				vec3(h4-0.08*(0.8+0.2*sin(2.*p.y+ss+8.0*(p.y-iTime)))-.02-.3*ss, 13., p.y) ));

    out_idPosTube = vec4(res.y == 10. ? p1 : res.y == 11. ? p2 : res.y == 12. ? p3 : p4, res.y);
    return res.x;
}


vec4 texturePt(vec3 pos) {
    vec3 p0 = pos;

    float h = mix(1.,.1, smoothstep(20., 14., length(pos.xz))) + gPulseGround;
    h *= texture(iChannel2, -p0.xz*.02).x;
    pos.y -= h;
    
   
    float kEat = sanim56*(1.+.1*gPulse); 
    vec3 pFrog = p0-vec3(0,-1.5*kEat,0);
    vec4 idPosFrog;
    float dFrog = textureFrog(pFrog, idPosFrog);

    vec4 idPosTube;
    float dTube = texturePtTube(pos, idPosTube);
         
    vec2 id = floor( (pos.xz-1.)/2.);    
    pos.xz -= .2*sin(dot(120.*id,vec2(1213.15,1317.34)));
    pos.xz += vec2(1,-1)*.2*sin(3.5*iTime +3.*cos(pos.y))*sin(.5*pos.y);

    return dTube < .01 ? idPosTube : dFrog < .01 ? idPosFrog : vec4(pos,1.);
}


#define EDGE_WIDTH .001

vec3 trace(in vec3 ro, in vec3 rd, in float maxd) {
	// edge detection
    dhaloLight = 9999.; // reset closest trap
    dhaloFrog = 9999.;
    float lastt,lastDistEval = 1e10;
	float edge = 0.0;
    float iter = 0.;

    float t = hash(rd);
    float d = 999.;//map(rd*t + ro);
    for (int i=ZERO; i<240; i++){
		d = .7*map(rd*t + ro, true);
        if ( abs(d) < 0.002 || t > maxd) break;
        t += min(.9,d);
    }
    return vec3(t);
}


// Tetrahedral normal, courtesy of IQ.
// -- Calculate normals -------------------------------------

vec3 calcNormal(in vec3 pos, in vec3 ray, in float t) {

	float pitch = .2 * t / iResolution.x;
	pitch = max( pitch, .002 );
	
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx, p1 = pos+d.xyy, p2 = pos+d.yxy, p3 = pos+d.yyx;
	float f0 = map(p0,false), f1 = map(p1,false), f2 = map(p2,false), f3 = map(p3,false);
	
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0, dot (grad,ray))*ray);
}




// The iterations should be higher for proper accuracy, but in this case, I wanted less accuracy, just to leave
// behind some subtle trails of light in the caves. They're fake, but they look a little like light streaming 
// through some cracks... kind of.
float softShadow(in vec3 ro, in vec3 rd, in float start, in float end, in float k){

    float shade = 1.0;
    const int maxIterationsShad = 32; 
    float dist = start;
    float stepDist = end/float(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i=ZERO; i<maxIterationsShad; i++){
        float h = map(ro + rd*dist,false);
        shade = min(shade, smoothstep(0.0, 1.0, k*h/dist));
        dist += clamp(h, 0.1, stepDist*2.);
        if (abs(h)<0.001 || dist > end) break; 
    }
    return min(max(shade, 0.) + 0.1, 1.0); 
}



// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function and have been looking for an excuse to use it. For a better version, and usage, 
// refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
float calculateAO(in vec3 pos, in vec3 nor) {
    float dd, hr=.01, totao=.0, sca=1.;
    for(int aoi=ZERO; aoi<4; aoi++ ) {
        dd = map(nor * hr + pos,false);
        totao += -(dd-hr)*sca;
        sca *= .8;
        hr += .06;
    }
    return clamp(1.-4.*totao, 0., 1.);
}



// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
vec3 doBumpMap( sampler2D tex, in vec3 p, in vec3 nor, float bumpfactor){
   
    const float eps = 0.001;
    vec3 grad = vec3( getGrey(tex3D(tex, vec3(p.x-eps, p.y, p.z), nor)),
                      getGrey(tex3D(tex, vec3(p.x, p.y-eps, p.z), nor)),
                      getGrey(tex3D(tex, vec3(p.x, p.y, p.z-eps), nor)));
    
    grad = (grad - getGrey(tex3D(tex,  p , nor)))/eps;             
    grad -= nor*dot(nor, grad);          
    return normalize( nor + grad*bumpfactor );
	
}

vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d ) {
    return a + b*cos( 6.28318*(c*t+d) );
}

vec3 palette(float id, float k) {
    return 2.*pal( k, vec3(0.5,0.8,0.8),vec3(0.6,0.3,0.5),vec3(1.0,.2,1.0), vec3((id-10.)*.01) );
}


// HSV to RGB conversion 
// [iq: https://www.shadertoy.com/view/MsS3Wc]
vec3 hsv2rgb_smooth(float x, float y, float z) {
    vec3 rgb = clamp( abs(mod(x*6.+vec3(0.,4.,2.),6.)-3.)-1., 0., 1.);
	rgb = rgb*rgb*(3.-2.*rgb); // cubic smoothing	
	return z * mix( vec3(1), rgb, y);
}


// -------------------------------------------------------------------
// pupils effect came from lexicobol shader:
// https://www.shadertoy.com/view/XsjXz1
// -------------------------------------------------------------------



float iqnoise( in vec2 x, float u, float v )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
	float k = 1.0+63.0*pow(1.0-v,4.0);
	float va = 0.0;
	float wt = 0.0;
    for( int j=-2+ZERO; j<=2; j++ )
    for( int i=-2+ZERO; i<=2; i++ ) {
        vec2 g = vec2(i,j);
		vec3 o = hash3( p + g )*vec3(u,u,1.0);
		vec2 r = g - f + o.xy;
		float d = dot(r,r);
		float ww = pow( 1.0-smoothstep(0.0,1.414,sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
    }
	
    return va/wt;
}

float noise ( vec2 x)
{
	return iqnoise(x, 0.0, 1.0);
}

mat2 m = mat2( 0.8, 0.6, -0.6, 0.8);

float fbm( vec2 p)
{
	float f = 0.0;
    f += 0.5000 * noise(p); p *= m* 2.02;
    f += 0.2500 * noise(p); p *= m* 2.03;
    f += 0.1250 * noise(p); p *= m* 2.01;
    f += 0.0625 * noise(p); p *= m* 2.04;
    f /= 0.9375;
    return f;
}

vec3 iris(vec2 p, float open)
{

    float r = sqrt( dot (p,p));
    float r_pupil = .15 + .15*smoothstep(.5,2.,open);
    
    float dPupil = length(vec2(abs(p.x)+.2, p.y)) - .35;// + .15*smoothstep(.5,2.,open);

    float a = atan(p.y, p.x); // + 0.01*iTime;
    vec3 col = vec3(1.0);
    
    float ss = 0.5;// + 0.5 * sin(iTime * 2.0);
    float anim = 1.0 + 0.05*ss* clamp(1.0-r, 0.0, 1.0);
    r *= anim;
        
    if( r< .8) {
		col = vec3(0.12, 0.60, 0.57);
        float f = fbm(5.0 * p);
        col = mix(col, 2.*vec3(1.,.8,0.12), f); 
        
        f = 1.0 - smoothstep( 0., .1, dPupil);
        col = mix(col, vec3(0.12,1., 0.30), f); 
        
        a += 0.05 * fbm(20.0*p);
        
        f = smoothstep(0.3, 1.0, fbm(vec2(5.0 * r, 20.0 * a))); // white highlight
        col = mix(col, vec3(1.0), f);
        
        f = smoothstep(0.3, 1.0, fbm(vec2(5.0 * r, 5.0 * a))); // yellow highlight
        col = mix(col, vec3(1.5,.8,0.12), f);
        
        f = smoothstep(0.5, 1.0, fbm(vec2(5.0 * r, 15.0 * a))); // dark highlight
        col *= 1.0 - f;
        
        f = smoothstep(0.55, 0.8, r); //dark at edge
        col *= 1.0 - 0.6*f;
        
        f = smoothstep( 0., .05, dPupil); //pupil
        col *= f; 
        
        f = smoothstep(0.75, 0.8, r);
        col = .5*mix(col, vec3(1.0), f);
    }
    
	return 3.*col;
}



#ifdef STEREOGRAPHIC
vec3 getStereoDir(vec2 fragCoord)
{
	vec2 p = fragCoord.xy / iResolution.xy;
    float t = 3.+iTime*.08, ct = cos(t), st = sin(t);
	float m = .5;
    p = (p * 2. * m - m)*.7;
    p.x *= iResolution.x/iResolution.y;
    p *= mat2(ct,st,-st,ct);

	return normalize(vec3(2.*p.x,dot(p,p)-1.,2.*p.y));
}  
#endif



void mainImage( out vec4 fragColor, in vec2 fragCoord ){	
	float tt = mod(iTime,1.5)/1.5;
    float ss = pow(tt,.2)*0.5 + 0.5;

    sanim01 = smoothstep(ani0,ani1,iTime);
    sanim12 = smoothstep(ani1,ani2,iTime);
    sanim23 = smoothstep(ani2,ani3,iTime);
    sanim34 = smoothstep(ani3,ani4,iTime);
    sanim45 = smoothstep(ani4,ani5,iTime);
    sanim56r = smoothstep(ani5b,ani5c,iTime); 
    float sanim5c = smoothstep(ani5,ani5c,iTime); 
    sanim5cd = smoothstep(ani5c,ani5d,iTime); 
    sanim56 = smoothstep(ani5d,ani6,iTime); 
    sanim67 = smoothstep(ani6,ani7,iTime);
    
    // Heart pulse
    gPulse = (.25+ss*0.5*sin(tt*6.2831*3.0)*exp(-tt*4.0));
    
    // Ground Pulse
    gPulseGround = .2+.8*mix(gPulse, 0., sanim45 + sanim67);
    
	// Screen coordinates.
	vec2 u = (fragCoord - iResolution.xy*0.5)/iResolution.y;
	    
    vec2 uv = gl_FragCoord.xy / iResolution.xy;

    // Camera Setup.
    float a = .1*iTime + 2.*3.141592*iMouse.x/iResolution.x;
    
	vec3 ro = vec3(-92.0,  4.5, -78.);
    ro = mix(ro, vec3(-106.0,  8.0, -84.), sanim34); 
    ro = mix(ro, vec3(cos(-a), .3, sin(-a))*26., sanim45);
    ro = mix(ro, vec3(1,3,0), sanim5c);
    ro = mix(ro, vec3(1,3,0) + vec3(1.,.2,.8)*(iTime - ani6), sanim67);
    
	vec3 lookAt = ro + vec3(.25, -.22, .5); // Camera position, doubling as the ray origin.
	lookAt = mix(lookAt, vec3(0, 0, 0), sanim34); // Camera position, doubling as the ray origin.
	lookAt = mix(lookAt, vec3(cos(a), .25, sin(a))*7., sanim5c); // Camera position, doubling as the ray origin.
 
    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159/6.; // FOV - Field of view.
    vec3 forward = normalize(lookAt-ro);
    vec3 right = normalize(vec3(forward.z, 0., -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*u.x*right + FOV*u.y*up);


    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
	//rd.xz = rot2( /*iMouse.x/iResolution.x +*/ path(lookAt.z).x/64. )*rd.xz;

    // Usually, you'd just make this a unit directional light, and be done with it, but I
    // like some of the angular subtleties of point lights, so this is a point light a
    // long distance away. Fake, and probably not advisable, but no one will notice.

	vec3 res = trace(ro, rd, FAR);
    float t = res.x;
    
    // Standard sky routine. Worth learning. For outdoor scenes, you render the sky, then the
    // terrain, then mix together with a fog falloff. Pretty straight forward.
    vec3 sky = .3*vec3(1.,1.3,1.3);//getSky(ro, rd, normalize(lp - ro));
    vec3 col = sky;
	
 //   vec3 lp = mix(ro+vec3(5.05,-1.5,-5.05), ro+vec3(.05,12.5,-5.05), sanim56r);
 //   lp = mix(lp, ro+vec3(5.05,-1.5,-5.05), sanim67);
    
     vec3 lp = (forward*.5+up-right)*FAR/*vec3(FAR*.5, FAR, FAR)*/ + vec3(0, 0, ro.z);

    if (t < FAR){
    	
        vec3 sp = ro+t*rd; // Surface point.
        vec4 spt = texturePt(sp); // Surface points on objects coords (to enable textures to follow object moves)
        vec3 sn = calcNormal( sp, rd, t ); // Surface normal.

		// Light direction
        vec3 ld = normalize(lp-sp);

        // Texture scale factor.        
        const float tSize1 = 1./3.;
        float k;
        vec3 colTxt;
        
        if (spt.w > 30.) {
            // Frog eyes
            vec3 pe = spt.xyz;
            float a = .2*cos(.1*iTime),
                  ca = cos(a), sa = sin(a);
            pe.xz *= mat2(ca, sa, -sa, ca);
            float b = mix(3.1-1.5*fract(iTime*.2+.17*spt.w), 4.2, step(ani5b2, iTime)),//sanim56r),
                  cb = cos(b), sb = sin(b);
            pe.xy *= mat2(cb, sb, -sb, cb);
            colTxt = iris((pe.zy), 20.5);

        } else if (spt.w > 19.) {
            // Frog Body
            vec3 hh = hash3(vec2(spt.w,spt.w));
            colTxt = hsv2rgb_smooth(spt.w*.2, .6, .7);
            colTxt = mix(colTxt, vec3(0.), .7*smoothstep(.6,.7, hh*.5+tex3D(iChannel2, spt.xyz*tSize1, sn).x));
            colTxt = mix(colTxt, vec3(0,1,1), .1*smoothstep(.0,1., -sn.y));
            colTxt = .7*sqrt(colTxt);
	        sn = doBumpMap(iChannel2, 2.*spt.xyz*tSize1, sn, .2/(1. + t/FAR));
            
        } else if (spt.w > 5.) {

	        float k = tex3D(iChannel0, spt.xyz*tSize1 + .1*spt.w, sn).x;
            colTxt = mix(vec3(1.2,.5,.4), palette(1., spt.w), .7+.3*cos(spt.w+4.*spt.y-5.*iTime));     
            colTxt = mix(colTxt, vec3(1,0,0), .2+.5*smoothstep(.2,.8,k));

    	    sn = doBumpMap(iChannel0, spt.xyz*tSize1, sn, .007/(1. + t/FAR));

        }  else {
            k = tex3D(iChannel0, spt.xyz*tSize1, sn).x;
           // colTxt = mix(vec3(1,.5,.3), 4.*(.6+.5*sin(.1*iTime+.01*length(spt.xz)))*vec3(0,1,1), .5+.5*smoothstep(.4,.7,k+.05*cos(2.*iTime)));
            colTxt = mix(.3*vec3(1,.5,.3), 1.3*vec3(0,1,1), .4+.6*smoothstep(.4,.7,k+.05*cos(2.*iTime)));
            colTxt = mix(colTxt, 1.7*vec3(1.8,1.8,.5), sanim01*smoothstep(.4,0.,abs(.8-spt.y)));
       		sn = doBumpMap(iChannel0, spt.xyz*tSize1, sn, .007/(1. + t/FAR));//max(1.-length(fwidth(sn)), .001)*hash(sp)/(1.+t/FAR)
        }
       
    	// prevent normals pointing away from camera (caused by precision errors)
		sn = normalize(sn - max(.0, dot(sn,rd))*rd);       
                
        float d2 = 1.;//RayMarchOut(sp+rd*(.05*4. + noise.x*0.05), ld);
        
        float shd = softShadow(sp, ld, 0.005, 4., 8.); // Shadows.
        float ao = calculateAO(sp, sn); // Ambient occlusion.
        float dif = max( dot( ld, sn ), 0.0); // Diffuse term.
        float spe = pow(max( dot( reflect(-ld, sn), -rd ), 0.0 ), 29.); // Specular term.
        float fre = clamp(1.0 + dot(rd, sn), 0.0, 1.0); // Fresnel reflection term.
       
        // Schlick approximation. I use it to tone down the specular term. It's pretty subtle,
        // so could almost be aproximated by a constant, but I prefer it. Here, it's being
        // used to give a hard clay consistency... It "kind of" works.
		float Schlick = pow( 1. - max(dot(rd, normalize(rd + ld)), 0.), 5.0);
		float fre2 = mix(.2, 1., Schlick);  //F0 = .2 - Hard clay... or close enough.
       
        // Overal global ambience. Without it, the cave sections would be pretty dark. It's made up,
        // but I figured a little reflectance would be in amongst it... Sounds good, anyway. :)
        float amb = fre*fre2 + .06*ao;
        
        // Coloring the soil - based on depth. Based on a line from Dave Hoskins's "Skin Peeler."
        col = colTxt;
        col = (col*(dif*d2 + .1) + fre2*spe*2.)*shd*ao + amb*col;
    } 
    
    col = .5*mix(col, sky, smoothstep(5., FAR, t));
   
    // Light
    if (dhaloLight < t) {
        float BloomFalloff = 50000.; 
 		col += mix(1.5*vec3(1.,1.,.4), sky, .5+.5*smoothstep(5., FAR, dhaloLight))/(1.+dhaloLight*dhaloLight*dhaloLight*BloomFalloff);
    }
    if (dhaloFrog < t) {
        float BloomFalloff = 50000.; 
 		col += mix(sanim56*vec3(1.,1.,.4), vec3(0), .5+.5*smoothstep(5., FAR, dhaloFrog))/(1.+dhaloFrog*dhaloFrog*dhaloFrog*BloomFalloff);
    }
   
    
    // gamma correction
    col = pow(max(col, 0.), vec3(.7));

    u = fragCoord/iResolution.xy;
    col *= pow( 16.0*u.x*u.y*(1.0-u.x)*(1.0-u.y) , .32);

    
	fragColor = vec4(clamp(col, 0., 1.), 1.0 );
}