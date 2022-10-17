

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    fragColor = mix(texture(iChannel0, fragCoord/iResolution.xy),
                    texture(iChannel1, fragCoord/iResolution.xy), smoothstep(31., 30., iTime) /* smoothstep(200., 199., iTime)*/);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//-----------------------------------------------------
// Created by sebastien durand - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------

#define WITH_AO
#define WITH_REFLECTION
#define WITH_FACE

// Isosurface Renderer
#define g_traceLimit 64
#define g_traceSize .002


#define MAX_DIST 192.
#define RUN_STEP 120.

#define rot(a) mat2(cos(a),sin(a),-sin(a),cos(a))

float gTime;
mat2 rotHead;

vec3 gLightPos = vec3(2, 1, 6);
const vec3 gStaticPos = vec3(-95.,8.83,-121.7); //vec3(-95.5,12.8,-123);


//	[Shane] Combustible Clouds
//	------------------

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
vec3 hash33(vec3 p){ 
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}

// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
float n3D( in vec3 p ){
    vec3 i = floor(p); p -= i; p *= p*(3. - 2.*p);
	p.xy = texture(iChannel0, (p.xy + i.xy + vec2(37, 17)*i.z + .5)/256., -100.).yx;
	return mix(p.x, p.y, p.z);
}

// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
float trigNoise3D(in vec3 p){ 
    float res = 0., sum = 0.;
    // IQ's cheap, texture-lookup noise function. Very efficient, but still 
    // a little too processor intensive for multiple layer usage in a largish 
    // "for loop" setup. Therefore, just one layer is being used here.
    float n = n3D(p*8.);// + iTime*2.);
    // Two sinusoidal layers. I'm pretty sure you could get rid of one of 
    // the swizzles (I have a feeling the GPU doesn't like them as much), 
    // which I'll try to do later.
    
    vec3 t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265+1.57/2.))*0.5 + 0.5;
    p = p*1.5 + (t - 1.5); //  + iTime*0.1
    res += (dot(t, vec3(0.333)));

    t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265+1.57/2.))*0.5 + 0.5;
    res += (dot(t, vec3(0.333)))*0.7071;    
	 
	return ((res/1.7071))*0.85 + n*0.15;
}

vec4 clouds(in vec3 ro, in vec3 rd, in float tend) {
	//rd = rd.zyx;
    // Ray origin. Moving along the Z-axis.
    //vec3 ro = vec3(0, 0, iTime*.02);

    // Placing a light in front of the viewer and up a little. You could just make the 
    // light directional and be done with it, but giving it some point-like qualities 
    // makes it a little more interesting. You could also rotate it in sync with the 
    // camera, like a light beam from a flying vehicle.
    vec3 lp = gLightPos;
    //lp.xz = lp.xz*rM;
    //lp += ro;
    // The ray is effectively marching through discontinuous slices of noise, so at certain
    // angles, you can see the separation. A bit of randomization can mask that, to a degree.
    // At the end of the day, it's not a perfect process. Note, the ray is deliberately left 
    // unnormalized... if that's a word.
    //
    // Randomizing the direction.
    //rd = (rd + (hash33(rd.zyx)*0.004-0.002)); 
    // Randomizing the length also. 
    //rd *= (1. + fract(sin(dot(vec3(7, 157, 113), rd.zyx))*43758.5453)*0.04-0.02);  
    
    //rd = rd*.5 + normalize(rd)*.5;    
    
    // Some more randomization, to be used for color based jittering inside the loop.
    vec3 rnd = hash33(rd + 311.);

    // Local density, total density, and weighting factor.
    float lDe = 0., td = 0., w = 0.;

    // Closest surface distance, and total ray distance travelled.
    float d = 1., t = dot(rnd, vec3(.08));

    // Distance threshold. Higher numbers give thicker clouds, but fill up the screen too much.    
    const float h = .5;


    // Initializing the scene color to black, and declaring the surface position vector.
    vec3 col = vec3(0), sp;


    // Particle surface normal.
    //
    // Here's my hacky reasoning. I'd imagine you're going to hit the particle front on, so the normal
    // would just be the opposite of the unit direction ray. However particles are particles, so there'd
    // be some randomness attached... Yeah, I'm not buying it either. :)
    vec3 sn = normalize(hash33(rd.yxz)*.03-rd);

    // Raymarching loop.
    for (int i=0; i<48; i++) {

        // Loop break conditions. Seems to work, but let me
        // know if I've overlooked something.
        if((td>1.) || d<.001*t || t>80. || t>tend) break;

        sp = ro + rd*t; // Current ray position.
        d = trigNoise3D(sp*.75); // Closest distance to the surface... particle.

        // If we get within a certain distance, "h," of the surface, accumulate some surface values.
        // The "step" function is a branchless way to do an if statement, in case you're wondering.
        //
        // Values further away have less influence on the total. When you accumulate layers, you'll
        // usually need some kind of weighting algorithm based on some identifying factor - in this
        // case, it's distance. This is one of many ways to do it. In fact, you'll see variations on 
        // the following lines all over the place.
        //
        lDe = (h - d) * step(d, h); 
        w = (1. - td) * lDe;   

        // Use the weighting factor to accumulate density. How you do this is up to you. 
        td += w*w*8. + 1./60.; //w*w*5. + 1./50.;
        //td += w*.4 + 1./45.; // Looks cleaner, but a little washed out.


        // Point light calculations.
        vec3 ld = lp-sp; // Direction vector from the surface to the light position.
        float lDist = max(length(ld), 0.001); // Distance from the surface to the light.
        ld/=lDist; // Normalizing the directional light vector.

        // Using the light distance to perform some falloff.
        float atten = 1./(1. + lDist*0.1 + lDist*lDist*.03);

        // Ok, these don't entirely correlate with tracing through transparent particles,
        // but they add a little anglular based highlighting in order to fake proper lighting...
        // if that makes any sense. I wouldn't be surprised if the specular term isn't needed,
        // or could be taken outside the loop.
        float diff = max(dot(sn, ld ), 0.);
        float spec = pow(max(dot( reflect(-ld, sn), -rd ), 0.), 4.);

        // Accumulating the color. Note that I'm only adding a scalar value, in this case,
        // but you can add color combinations.
        col += w*(1.+ diff*.5 + spec*.5)*atten;
        // Optional extra: Color-based jittering. Roughens up the grey clouds that hit the camera lens.
        col += (fract(rnd*289. + t*40001.) - .5)*.02;;

        // Try this instead, to see what it looks like without the fake contrasting. Obviously,
        // much faster.
        //col += w*atten*1.25;


        // Enforce minimum stepsize. This is probably the most important part of the procedure.
        // It reminds me a little of of the soft shadows routine.
        t +=  max(d*.5, .02); //
        // t += 0.2; // t += d*0.5;// These also work, but don't seem as efficient.

    }
    
    col = max(col, 0.);

    
    // Adding a bit of a firey tinge to the cloud value.
    col = mix(pow(vec3(1.3, 1, 1)*col, vec3(1, 2, 10)), col, dot(cos(rd*6. +sin(rd.yzx*6.)), vec3(.333))*.2+.8);
 
    // Using the light position to produce a blueish sky and sun. Pretty standard.
    vec3 sky = vec3(.6, .8, 1.)*min((1.5+rd.y*.5)/2., 1.); 	
    sky = mix(vec3(1, 1, .9), vec3(.31, .42, .53), rd.y*0.5 + 0.5);
    
    float sun = clamp(dot(normalize(lp-ro), rd), 0.0, 1.0);
   
    // Combining the clouds, sky and sun to produce the final color.
    sky += vec3(1, .3, .05)*pow(sun, 5.)*.25; 
    sky += vec3(1, .4, .05)*pow(sun, 16.)*.35; 	
    col = mix(col, sky, smoothstep(0., 25., t));
 	col += vec3(1, .6, .05)*pow(sun, 16.)*.25; 	
 
    // Done.
    return vec4(col, clamp(0.,1.,td));
    
}


// standard       - https://www.shadertoy.com/view/4sjGzc

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------

//const float hipz = 8.;
// WALK -----

//                       Contact           Down               Pass               Up      

vec3 shoulder1, elbow1, wrist1, head,
     shoulder2, elbow2, wrist2;
vec3 foot1, ankle1, knee1, hip1,
     foot2, ankle2, knee2, hip2;
vec3 v2Foot1, v2Foot12, v2Foot2, v2Foot22, v3Foot1, v3Foot2;
vec3 v1Hand1, v2Hand1, v3Hand1,
     v1Hand2, v2Hand2, v3Hand2;




//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

float hash( float n ) { return fract(sin(n)*43758.5453123); }

// mix noise for alive animation, full source
vec4 hash4( vec4 n ) { return fract(sin(n)*1399763.5453123); }
vec3 hash3( vec3 n ) { return fract(sin(n)*1399763.5453123); }



//---------------------------------------------------------------------
//    Geometry
//---------------------------------------------------------------------

// Distance from ray to point
float distanceRayPoint(vec3 ro, vec3 rd, vec3 p, out float h) {
    h = dot(p-ro,rd);
    return length(p-ro-rd*h);
}


//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] https://iquilezles.org/articles/distfunctions
//---------------------------------------------------------------------

bool cube(vec3 ro, vec3 rd, vec3 sz, out float tn, out float tf) { //, out vec3 n) {
	vec3 m = 1./rd,
         k = abs(m)*sz,
         a = -m*ro-k*.5, 
         b = a+k;
//	n = -sign(rd)*step(a.yzx,a)*step(b.zxy,b);
    tn = max(max(a.x,a.y),a.z);
    tf = min(min(b.x,b.y),b.z); 
	return /*tn>0. &&*/ tn<tf;
}


float sdCap(in vec3 p, in vec3 a, in vec3 b, in float r ) {
    vec3 pa = p - a, ba = b - a;
    return length(pa - ba*clamp( dot(pa,ba)/dot(ba,ba), 0., 1. ) ) - r;
}

float sdCapsule(in vec3 p, in vec3 a, in vec3 b, in float r0, in float r1 ) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0., 1.);
    return length( pa - ba*h ) - mix(r0,r1,h);
}

float sdCap2(in vec3 p, in vec3 a, in vec3 b, in float r1, in float r2) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. );
    return length(pa - ba*h) - mix(r1,r2,h*h*h);
}

float udRoundBox(in vec3 p, in vec3 b, in float r ) {
  return length(max(abs(p)-b,0.))-r;
}

//https://www.shadertoy.com/view/Xs3GRB
float fCylinder(in vec3 p, in float r, in float height) {
	return max(length(p.xz) - r, abs(p.y) - height);
}

float sdPlane(in vec3 p, in vec3 n) {  // n must be normalized
  return dot(p,n);
}

float smin(in float a, in float b, in float k ) {
    float h = clamp( .5+.5*(b-a)/k, 0., 1. );
    return mix( b, a, h ) - k*h*(1.-h);
}

// Smooth maximum, based on the function above.
float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5 + 0.5*(a-b)/s, 0., 1.);
    return mix(b, a, h) + h*(1.0-h)*s;
}

// hg_sdf : http://mercury.sexy/hg_sdf/
float fOpEngrave(float a, float b, float r) {
	return max(a, (a + r - abs(b))*0.7071); //sqrt(.5));
}

float fOpIntersectionRound(float a, float b, float r) {
	vec2 u = max(vec2(r + a,r + b), vec2(0));
	return min(-r, max (a, b)) + length(u);
}


float sdEllipsoid( in vec3 p, in vec3 r) {
    return (length(p/r ) - 1.) * min(min(r.x,r.y),r.z);
}


float fOpDifferenceRound (float a, float b, float r) {
	return fOpIntersectionRound(a, -b, r);
}


//---------------------------------------------------------------------
//    Man + Ground distance field 
//---------------------------------------------------------------------

// The canyon, complete with hills, gorges and tunnels. I would have liked to provide a far
// more interesting scene, but had to keep things simple in order to accommodate slower machines.
float mapGround(in vec3 p){
    float tx = .2*(cos(p.x*.03))*(textureLod(iChannel1, p.xz/16. + p.xy/80., 0.0).x);
    vec3 q = p*0.25;
#ifdef IS_RUNNING
    float h = tx + .5*(dot(sin(q)*cos(q.yzx), vec3(.222))) + dot(sin(q*1.3)*cos(q.yzx*1.4), vec3(.111));
    float d = p.y + smin(0.,smoothstep(.2,3., abs(p.z))*(h)*9.,.2);
#else
    float h = tx + .5*(dot(sin(q)*cos(q.yzx), vec3(.222))) + dot(sin(q*1.3)*cos(q.yzx*1.4), vec3(.111)) - .5*(textureLod(iChannel1, q.xz/80. + q.xz/102., 0.0).x)
        - .5*smoothstep(18., 0., length(p.xz-gStaticPos.xz))
        + .5*smoothstep(130., 0., length(p.xz- gStaticPos.xz - vec2(120,50)));
	float d = p.y + smin(0., h*9., .2);
#endif
    return d; 
}

// capsule with bump in the middle -> use for neck
float sdCapsule2(in vec3 p,in vec3 a,in vec3 b, in float r0,in float r1,in float bump) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. );
    float dd = bump*sin(3.14*h);  // Little adaptation
    return length(pa - ba*h) - mix(r0,r1,h)*(1.+dd); 
}


const vec3 g_nozePos = vec3(0,-.28+.04,.47+.08);
const vec3 g_eyePos = vec3(.14,-.14,.29);
const float g_eyeSize = .09;
mat2   g_eyeRot;
const mat2 ma = rot(-.5);
const mat2 mb = rot(-.15);
const mat2 mc = rot(-.6);

float smax(in float a, in float b, in float k) {
    return log(exp(a/k)+exp(b/k))*k;
}


float mapHead(in vec3 p0) {
    vec3 p = p0;
    float d = MAX_DIST;
   
// Skull modeling -------------------------
    d = sdEllipsoid(p-vec3(0,.05,.0), vec3(.39,.48,.46));				  
    d = smin(d, sdEllipsoid(p-vec3(0.,.1,-.15), vec3(.42,.4,.4)),.1);     
    d = smin(d, udRoundBox(p-vec3(0,-.28,.2), vec3(.07,.05,.05),.05),.4); // Basic jaw 

// Neck -----------------------------------
 //   d = smin(d, dNeck, .05);

// Symetrie -------------------------------
    p.x = abs(p.x);

// Eye hole 
    d = smax(d, -sdEllipsoid(p-vec3(.12,-.16,.48), vec3(.09,.06,.09)), .07);

// Noze ------------------------------------
    const float animNoze = 0.;
    d = smin(d, max(-(length(p-vec3(.032,-.325,.45))-.028),   // Noze hole
                    smin(length(p-vec3(.043,-.29+.015*animNoze,.434))-.01,  // Nostrils
                    sdCapsule(p, vec3(0,-.13,.39), vec3(0,-.28+.008*animNoze,.47), .01,.04), .05)) // Bridge of the nose
            ,.065); 
   
// Mouth -----------------------------------    
    d = smin(d, length(p- vec3(.22,-.34,.08)), .17); // Jaw
    d = smin(d, sdCapsule(p, vec3(.16,-.35,.2), vec3(-.16,-.35,.2), .06,.06), .15); // Cheeks
   
    d = smin(d, max(-length(p.xz-vec2(0,.427))+.015,  	// Line under the noze
        		max(-p.y-.41+.008*animNoze,   						// Upper lip
                    sdEllipsoid(p- vec3(0,-.34,.37), vec3(.08,.15,.05)))), // Mouth bump
             .032);

// Chin -----------------------------------  
    d = smin(d, length(p- vec3(0,-.5,.26)), .2);   // Chin
    d = smin(d, length(p- vec3(0,-.44,.15)), .25); // Under chin 
  
// Eyelid ---------------------------------
	vec3 p_eye1 = p - g_eyePos;
    p_eye1.xz *= mb;
    
    vec3 p_eye2 = p_eye1;
    float d_eye = length(p_eye1) - g_eyeSize*1.;
          
	p_eye1.yz *= g_eyeRot;
	p_eye2.zy *= mc;
    
    float d1 = min(max(-p_eye1.y,d_eye - .01),
                   max(p_eye2.y,d_eye - .005));

    d = smin(d,d1,.01);
	return d; 
}

float mapLegs(const in vec3 pos){    
    // Leg 1
    float d = max(min(sdCap2(pos, foot1, ankle1, .1,.15),
                      sdCap2(pos, ankle1, knee1, .165,.105)),
				  -sdPlane(pos-ankle1+v2Foot1*.1, v2Foot12));
    // Leg 2
	d = min(d,max(min(sdCap2(pos, foot2, ankle2, .1,.15),
                      sdCap2(pos, ankle2, knee2, .165,.105)),
				  -sdPlane(pos-ankle2+v2Foot2*.1, v2Foot22)));
 
    d = fOpEngrave(d, min(min(sdCap(pos-ankle1, -.1*v3Foot1, .1*v3Foot1, .12), 
                              sdCap(pos-ankle2, -.1*v3Foot2, .1*v3Foot2, .12)), 
                          min(length(pos - knee1),length(pos - knee2))-.11), .015);
    
    d = min(d, sdCap2(pos,  hip1, knee1, .12, .075));
    return min(d, sdCap2(pos, hip2, knee2, .12, .075));
}

float mapGirl(const in vec3 pos){
    vec3 
         ep0 = mix(shoulder1,shoulder2,.5),
         ha0 = mix(hip1,hip2,.5),
         h1 = head + vec3(.0,-.24-.05,0),
         h2 = head + vec3(.0,.15-.05,0),
         hn = normalize(h1-h2),
         a = mix(ha0,ep0,.15), b = mix(ha0,ep0,.75);
    
    vec3 posRot = pos; // - head; 
    posRot.xz = (posRot.xz - head.xz) * rotHead + head.xz;
   // posRot.xz = (posRot.xz) * rotHead;
    
    float d = mapLegs(pos);

    // Head
    float scaleHead = 1.75;
    vec3 pHead = (posRot - head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = min(d, dHead);

	// Eye
    pHead.x = abs(pHead.x);
    vec3 p_eye = pHead-g_eyePos;
    p_eye.xz *= ma;  
	float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    d = min(d, dEye);
        
    // Arms
    d = min(d, sdCapsule2(pos, shoulder1, elbow1, .054,.051,.5));
    d = min(d, sdCapsule2(pos, shoulder2, elbow2, .054,.051,.5));
    float dArm = sdCap2(pos, elbow1, wrist1-.01*v1Hand1, .09, .055);
    dArm = min(dArm,  sdCap2(pos, elbow2, wrist2-.01*v1Hand2, .09, .055));
    dArm = fOpEngrave(dArm, 
                      min(min(length(pos - wrist2), length(pos - wrist1)),
                          min(length(pos - elbow2), length(pos - elbow1))) - .1,.008);

    
    // Neck and Shoulders
    d = smin(d, min(sdCapsule2(pos+vec3(.03,0,0), mix(shoulder1, shoulder2,.1),mix(shoulder1, shoulder2,.9),.08, .08,.6),
                    sdCap(pos,ep0-vec3(.03,0,0), head-vec3(.08,.1,0), .09)), .12);
    
    // Torso
	d = smin(d, min(sdCap2(pos, a+vec3(0,0,.03), b-vec3(0,0,.04), .19,.22),sdCap2(pos, a-vec3(0,0,.03), b+vec3(0,0,.04), .19,.22)),.18);
    
	// Fingers 1
    vec3 c = wrist1-v3Hand1*.03;
    float d2 = sdCap(pos, c-v1Hand1*.06+v2Hand1*.03+v3Hand1*.06, wrist1+.09*(v2Hand1+v1Hand1+v3Hand1), .02);
    d2 = min(d2, sdCap(pos, c, wrist1+.18*(v1Hand1+v2Hand1*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.01, wrist1+.2*(v1Hand1-v2Hand1*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.02, wrist1+.18*(v1Hand1-v2Hand1*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.04, wrist1+.15*(v1Hand1-v2Hand1*.8), .014));
    
	// Fingers 2     
    c = wrist2-v3Hand2*.03;
    d2 = min(d2, sdCap(pos, c-v1Hand2*.06+v2Hand2*.03+v3Hand2*.06, wrist2+.09*(v2Hand2+v1Hand2+v3Hand2), .02));
    d2 = min(d2, sdCap(pos, c, wrist2+.18*(v1Hand2+v2Hand2*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.01, wrist2+.2*(v1Hand2-v2Hand2*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.02, wrist2+.18*(v1Hand2-v2Hand2*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.04, wrist2+.15*(v1Hand2-v2Hand2*.8), .014));


    d = min(d, smin(d2, dArm, .12));
   
    // Short
    float dShort = min(sdCap(pos, hip1+vec3(-.03,0,0), mix(hip1, knee1,.25), .13), 
                       sdCap(pos, hip2+vec3(-.03,0,0), mix(hip2, knee2,.25), .13));                    
    // TODO ca serait plus cool de bouger avec les hanches mais il faut recuperer une base fixee sur les hanches
    dShort = smin(dShort, mix(d, sdCap(pos, a, ha0+vec3(0,.1,0), .22),.5),.1);
    d = min(d, dShort);

   // Casque
	float dHelmet;
    posRot += vec3(.03,.02,0);
    dHelmet = max(sdPlane(posRot-h1+hn*.07, hn), mix(dHead, sdCap2(posRot, h1-vec3(.23,0,0), h2-vec3(0,.05,0),.28,.36),.5));
    dHelmet = max(-fCylinder(posRot-h1-vec3(.2,0,0), .18,.3), dHelmet); 

    dHelmet = fOpEngrave(dHelmet, sdCap(posRot-h2, -vec3(0,.1,1), -vec3(0,.1,-1), .1),.015);
   
	d = min(d, dHelmet);
//	d = min(d, mapGround(pos));
            
    return min(d2,d);
}


//---------------------------------------------------------------------
//    Girl colors 
//---------------------------------------------------------------------
#define min2(a, b) (a.x<b.x?a:b)
#define max2(a, b) (a.x>b.x?a:b)

#define ID_MAN 100.
#define ID_GROUND 90.
#define ID_GLOVE 106. 
#define ID_HELMET 107.
#define ID_FOOT 108.
#define ID_SHORT 110.
#define ID_LEG  201.
#define ID_SKIN 202.
#define ID_ARM  203.
#define ID_TORSO 204.
#define ID_EYE 205.

const vec3 COLOR_SKIN = vec3(.6,.43,.3);
const vec3 COLOR_ARMOR = vec3(.14,.79,.7);
const vec3 COLOR_CLOTHES2 = vec3(.14,.79,.7);
const vec3 COLOR_CLOTHES = vec3(.66,.94,.91);
 



#ifdef WITH_FACE

//---------------------------------------------------------------------
//   Draw face
//   "Smiley Tutorial" by Martijn Steinrucken aka BigWings - 2017
//---------------------------------------------------------------------
// This Smiley is part of my ShaderToy Tutorial series on YouTube:
// Part 1 - Creating the Smiley - https://www.youtube.com/watch?v=ZlNnrpM0TRg
// Part 2 - Animating the Smiley - https://www.youtube.com/watch?v=vlD_KOrzGDc&t=83s
//---------------------------------------------------------------------


#define S(a, b, t) smoothstep(a, b, t)
#define B(a, b, blur, t) S(a-blur, a+blur, t)*S(b+blur, b-blur, t)
#define sat(x) clamp(x, 0., 1.)

float remap01(float a, float b, float t) {
	return sat((t-a)/(b-a));
}

float remap(float a, float b, float c, float d, float t) {
	return sat((t-a)/(b-a)) * (d-c) + c;
}

vec2 within(vec2 uv, vec4 rect) {
	return (uv-rect.xy)/(rect.zw-rect.xy);
}

vec4 Brow(vec2 uv, float smile) {
    float offs = mix(.2, 0., smile);
    uv.y += offs;
    
    float y = uv.y;
    uv.y += uv.x*mix(.5, .8, smile)-mix(.1, .3, smile);
    uv.x -= mix(.0, .1, smile);
    uv -= .5;
    
    vec4 col = vec4(0.);
    
    float blur = .1;
    
   	float d1 = length(uv);
    float s1 = S(.45, .45-blur, d1);
    float d2 = length(uv-vec2(.1, -.2)*.7);
    float s2 = S(.5, .5-blur, d2);
    
    float browMask = sat(s1-s2);
    
    float colMask = remap01(.7, .8, y)*.75;
    colMask *= S(.6, .9, browMask);
    colMask *= smile;
    vec4 browCol = vec4(.04, .02, .02, 1.); //mix(vec4(.4, .2, .2, 1.), vec4(1., .75, .5, 1.), colMask); 
   
    uv.y += .15-offs*.5;
    blur += mix(.0, .1, smile);
    d1 = length(uv);
    s1 = S(.45, .45-blur, d1);
    d2 = length(uv-vec2(.1, -.2)*.7);
    s2 = S(.5, .5-blur, d2);
    float shadowMask = sat(s1-s2);
    
    col = mix(col, vec4(0.,0.,0.,1.), S(.0, 1., shadowMask)*.5);
    
    col = mix(col, browCol, S(.2, .4, browMask));
    
    return col;
}

vec4 Mouth(vec2 uv, float smile) {
    uv -= .5;
	vec4 col = vec4(.5, .18, .05, 1.);
    
    uv.y *= 1.5;
    uv.y -= uv.x*uv.x*2.*smile;
    
    uv.x *= mix(2.5, 1., smile);
    
    float d = length(uv);
    col.a = S(.5, .48, d);
    
    vec2 tUv = uv;
    tUv.y += (abs(uv.x)*.5+.1)*(1.-smile);
    float td = length(tUv-vec2(0., .6));
    
    vec3 toothCol = vec3(1.)*S(.6, .35, d);
    col.rgb = mix(col.rgb, toothCol, S(.4, .37, td));
    
    td = length(uv+vec2(0., .5));
    col.rgb = mix(col.rgb, vec3(1., .5, .5), S(.5, .2, td));
    return col;
}

vec4 drawFace(vec3 pos) {
    pos -= head;
    pos.y += .05;
    pos.xz *= rotHead;
	vec4 col = vec4(COLOR_SKIN, ID_SKIN);
	if (pos.x < 0.) return col;
    vec2 uv = pos.zy*2.8;
	float side = sign(uv.x);
	uv.x = abs(uv.x);
      float d = length(uv-vec2(.28, -.42));
    float cheek = S(.2,.01, d)*.4;
    cheek *= S(.17, .16, d);
    col.rgb = mix(col.rgb, vec3(1., .1, .1), cheek);
	vec4 mouth = Mouth(within(uv, vec4(-.27, -.72, .27, -.60)), .5);
	col = mix(col, mouth, mouth.a);
	vec4 brow = Brow(within(uv, vec4(.06, -.14, .51, .06)), 0.);
	col = mix(col, brow, brow.a);
    return vec4(col.rgb, ID_SKIN);
}



vec4 drawEye(in vec3 p) {
    vec3 posRot = p; // - head; 
    posRot.xz = (posRot.xz - head.xz) * rotHead + head.xz;
    
    float scaleHead = 1.75;
    vec3 pHead = (posRot-head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;

	// Eye
    vec3 p_eye = pHead-g_eyePos;
    vec3 g_eyePosloc = g_eyePos;
    g_eyePosloc.x *= sign(pHead.x);
   	vec3 pe = pHead - g_eyePosloc;

   
    float a = .2*sin(2.*iTime)*cos(.01*iTime);//clamp(atan(-dir.x, dir.z), -.6,.6), 
    float ca = cos(a), sa = sin(a);
    pe.xz *= mat2(ca, sa, -sa, ca);

    float b = .2;//.1+.1*sin(iTime*.1);//clamp(atan(-dir.y, dir.z), -.3,.3), 
    float cb = cos(b), sb = sin(b);
    pe.yz *= mat2(cb, sb, -sb, cb);
    
    float d = length(pe.xy);
    vec3 col = mix(vec3(0), mix(vec3(.88,.41,.0), mix(vec3(0),vec3(1.5),
                   .5+.5*smoothstep(.0405,.0415,d)), smoothstep(.04,.041,d)), smoothstep(.02,.025,d));

    return vec4(col,ID_EYE);
}

float sdFish(vec3 o) {
    vec2 p = (o - (shoulder1 + shoulder2)*.5).zy + vec2(.04,.17);
    p *= 2.;
      
    float dsub = min(length(p-vec2(.8,.0)) - .45, length(p-vec2(-.14,.05)) - .11);  
    p.y = abs(p.y);
    float d = length(p-vec2(.0,-.15)) - .3;
    d = min(d, length(p-vec2(.56,-.15)) - .3);
    d = max(d, -dsub);
    return (1.-smoothstep(.05,.06,d));
}

#endif    

// -----------------------------------------


vec4 getColor(float id, vec3 pos) {
    //return id != ID_TORSO && id <= ID_SHORT ? vec4(.3,.3,.7,id) : vec4(.5,.5,1,id);
	return 	.3+.7*(
			id == ID_LEG ? vec4(COLOR_CLOTHES, ID_LEG) :
#ifdef WITH_FACE        
			id == ID_EYE ? drawEye(pos) :
			id == ID_SKIN ?	drawFace(pos) : 
			id == ID_TORSO ? vec4(mix(COLOR_CLOTHES,vec3(0),sdFish(pos)), ID_TORSO) :
#else                  
         	id == ID_EYE ?	vec4(1,1,1, ID_EYE) :
         	id == ID_SKIN ?	vec4(COLOR_SKIN, ID_SKIN) :
			id == ID_TORSO ? vec4(COLOR_CLOTHES, ID_TORSO) :
#endif
			id == ID_ARM ? vec4(COLOR_SKIN, ID_ARM) :
			id == ID_SHORT ? vec4(COLOR_CLOTHES2, ID_SHORT) :
//        	id == ID_FOOT ? vec4(COLOR_ARMOR, id) :
//			id == ID_GLOVE ? vec4(COLOR_ARMOR, ID_GLOVE) :
//			id == ID_HELMET ? vec4(COLOR_ARMOR, ID_HELMET) :
//			id == ID_GROUND ? vec4(textureLod(iChannel2, pos.xz*.1,1.).rgb, ID_GROUND) :
    		vec4(COLOR_ARMOR, id));
}


float mapColor(in vec3 pos) {
    vec3 
         ep0 = mix(shoulder1,shoulder2,.5),
         ha0 = mix(hip1,hip2,.5),
         h1 = head + vec3(.0,-.29,0),
         h2 = head + vec3(.0,.1,0),
         hn = vec3(0,-1,0),//normalize(h1-h2),
         a = mix(ha0,ep0,.15), b = mix(ha0,ep0,.79);
    vec3 posRot = pos; 
    posRot.xz = (posRot.xz - h1.xz) * rotHead + h1.xz;
    
    // Leg 1
    float d = max(min(sdCap2(pos, foot1, ankle1, .1,.15),
                      sdCap2(pos, ankle1, knee1, .165,.105)),
				  -sdPlane(pos-ankle1+v2Foot1*.1, v2Foot12));
    // Leg 2
	d = min(d,max(min(sdCap2(pos, foot2, ankle2, .1,.15),
                      sdCap2(pos, ankle2, knee2, .165,.105)),
				  -sdPlane(pos-ankle2+v2Foot2*.1, v2Foot22)));                          
    
    vec2 dd;
    dd = min2(vec2(d, ID_FOOT), 
              vec2(min(sdCap2(pos, hip1, knee1, .12, .075),
    				   sdCap2(pos, hip2,  knee2, .12, .075)), ID_LEG));

    // Head
    float scaleHead = 1.75;
    vec3 pHead = (posRot - head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
   // d = min(d, dHead);

	// Eye
    pHead.x = abs(pHead.x);
    vec3 p_eye = pHead-g_eyePos;
    p_eye.xz *= ma;  
    
	float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    dd = min2(dd, vec2(dEye,ID_EYE));

    dd = min2(dd, vec2(min(dHead,             
						sdCap(pos, ep0-vec3(.03,0,0), head-vec3(.08,.1,0), .1)), ID_SKIN));  // neck           
    
    // Arms
    dd = min2(dd, vec2(min(sdCap2(pos, shoulder1, elbow1, .054,.051), sdCap2(pos, shoulder2, elbow2, .054,.051)), ID_LEG));
    
	// Fingers 1
    vec3 c = wrist1-v3Hand1*.03;
    float d2 = sdCap(pos, c-v1Hand1*.06+v2Hand1*.03+v3Hand1*.06, wrist1+.09*(v2Hand1+v1Hand1+v3Hand1), .02);
    d2 = min(d2, sdCap(pos, c, wrist1+.18*(v1Hand1+v2Hand1*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.01, wrist1+.2*(v1Hand1-v2Hand1*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.02, wrist1+.18*(v1Hand1-v2Hand1*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.04, wrist1+.15*(v1Hand1-v2Hand1*.8), .014));
    
	// Fingers 2     
    c = wrist2-v3Hand2*.03;
    d2 = min(d2, sdCap(pos, c-v1Hand2*.06+v2Hand2*.03+v3Hand2*.06, wrist2+.09*(v2Hand2+v1Hand2+v3Hand2), .02));
    d2 = min(d2, sdCap(pos, c, wrist2+.18*(v1Hand2+v2Hand2*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.01, wrist2+.2*(v1Hand2-v2Hand2*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.02, wrist2+.18*(v1Hand2-v2Hand2*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.04, wrist2+.15*(v1Hand2-v2Hand2*.8), .014));

    d2 = min(d2, min(sdCap2(pos, elbow1, wrist1-.01*v1Hand1, .09, .055),  
                     sdCap2(pos, elbow2, wrist2-.01*v1Hand2, .09, .055)));
    dd = min2(dd, vec2(d2, ID_GLOVE));
  
    
    
    
    
    // Torso
    dd = min2(dd, vec2(smin(sdCapsule2(pos+vec3(.03,0,0), mix(shoulder1, shoulder2,.1),mix(shoulder1, shoulder2,.9),.08, .08,.6), 
                           min(sdCap2(pos, a+vec3(0,0,.03), b-vec3(0,0,.04), .19,.22),sdCap2(pos, a-vec3(0,0,.03), b+vec3(0,0,.04), .19,.22)),.18), ID_TORSO));
  
    // Short
    float dShort = min(sdCap(pos, hip1, mix(hip1,knee1,.3), .12), 
                       sdCap(pos, hip2, mix(hip2,knee2,.3), .12));                    
    dd = min2(dd, vec2(min(dShort, mix(dd.x, sdCap(pos, a, ha0, .22),.75)), ID_SHORT));

    // Casque
	float dHelmet;
    posRot += vec3(.03,.02,0);
    dHelmet = max(sdPlane(posRot-h1+hn*.08, hn), mix(dHead, sdCap2(posRot, h1-vec3(.23,0,0), head+vec3(0,.05,0),.28,.36),.5));
    dHelmet = max(-fCylinder(posRot-h1-vec3(.2,0,0), .18,.3), dHelmet); 
    dd = min2(dd, vec2(dHelmet, ID_HELMET));

    return dd.y;
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------
              
float logBisectTrace(in vec3 ro, in vec3 rd){

    float t = 0., told = 0., mid, dn;
    float d = mapGround(rd*t + ro);
    float sgn = sign(d);
#ifdef IS_RUNNING
    for (int i=0; i<64; i++){
#else        
    for (int i=0; i<92; i++){
#endif
        // If the threshold is crossed with no detection, use the bisection method.
        // Also, break for the usual reasons. Note that there's only one "break"
        // statement in the loop. I heard GPUs like that... but who knows?
        if (sign(d) != sgn || d < 0.001 || t > MAX_DIST) break;
        told = t;
        // Branchless version of the following:      
        t += step(d, 1.)*(log(abs(d) + 1.1) - d) + d;
        //t += log(abs(d) + 1.1);
        //t += d;//step(-1., -d)*(d - d*.5) + d*.5;
        d = mapGround(rd*t + ro);
    }
    // If a threshold was crossed without a solution, use the bisection method.
    if (sign(d) != sgn){
        // Based on suggestions from CeeJayDK, with some minor changes.
        dn = sign(mapGround(rd*told + ro));
        vec2 iv = vec2(told, t); // Near, Far
        // 6 iterations seems to be more than enough, for most cases...
        // but there's an early exit, so I've added a couple more.
        for (int ii=0; ii<8; ii++){ 
            //Evaluate midpoint
            mid = dot(iv, vec2(.5));
            float d = mapGround(rd*mid + ro);
            if (abs(d) < 0.001)break;
            // Suggestion from movAX13h - Shadertoy is one of those rare
            // sites with helpful commenters. :)
            // Set mid to near or far, depending on which side we're on.
            iv = mix(vec2(iv.x, mid), vec2(mid, iv.y), step(0.0, d*dn));
        }

        t = mid; 
        
    }
    
    //if (abs(d) < PRECISION) t += d;

    return min(t, MAX_DIST);
}

vec2 Trace(in vec3 pos, in vec3 ray, in float start, in float end ) {
    // Trace if in bbox
    float t=start, h, tn=start, tf=end;
    float tGround = logBisectTrace(pos, ray);

   // start = max(start, );
    end = min(tGround, end);
    
    if (cube(pos-head-vec3(.1,-1.,0), ray, vec3(1.2, 1.7,.7)*2.,  tn, tf)) {
        end = min(tf, end);
        t = max(tn, start);// - .3*hash33(pos+ray).x;
        for( int i=0; i < g_traceLimit; i++) {
			if (t > end) break;
            h = mapGirl( pos+t*ray );
            if (h < g_traceSize) {
                return vec2(t+h, mapColor(pos+t*ray));
            }
            t += h;
        }
        if (t < end) return vec2(t, mapColor(pos+t*ray)); 
    } 
    
    return tGround < MAX_DIST ?  vec2(tGround, ID_GROUND) : vec2(MAX_DIST, 0.);
}

//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
    n = max(n*n, 0.001);
    n /= (n.x + n.y + n.z );  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

float getGrey(vec3 p){ return dot(p, vec3(0.299, 0.587, 0.114)); }

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


//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

#ifdef WITH_SHADOW
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax ) {
	float h, res = 1., t = mint;
    for(int i=0; i<24; i++) {
		h = map( ro + rd*t );
        res = min( res, 8.*h/t );
        t += clamp( h, .05, .2 );
        if( h<.01 || t>tmax ) break;
    }
    return clamp(res, 0., 1.);
}
#endif

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
float calcAO( in vec3 pos, in vec3 nor ){
	float dd, hr, sca = 1., totao = 0.;
    vec3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = .01 + .05*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(mapGirl( aopos )-hr)*sca;
        sca *= .75;
    }
    return clamp(1. - 4.*totao, 0., 1.);
}
float calcAOGround( in vec3 pos, in vec3 nor ){
	float dd, hr, sca = 1., totao = 0.;
    vec3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = .01 + .05*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(min(mapGround(aopos),mapLegs(aopos))-hr)*sca;
        sca *= .75;
    }
    return clamp(1. - 4.*totao, 0., 1.);
}
#endif


//---------------------------------------------------------------------
//   Shading
//   Adapted from Shane / Iq
//---------------------------------------------------------------------

vec3 shading( in vec3 sp, in vec3 rd, in vec3 sn, in vec3 col, in float id, out float reflexion){
    
    vec3 general = vec3(240,198,157)/256.,
    	 back = vec3(63,56,46)/256.;
    vec3 ref = reflect( rd, sn );

    // lighitng   
#ifdef WITH_AO
    float occ = id == ID_GROUND ? calcAOGround( sp, sn ) : calcAO( sp, sn );
#else
    float occ = 1.;
#endif
    vec3  ld = normalize( gLightPos );
    vec3  hal = normalize( rd - ld);
    float amb = .15; //clamp( .5+.5*sn.y, 0., 1. );
    float dif = clamp( dot( sn, ld ), 0., 1. );
    float bac = clamp( dot( sn, normalize(vec3(-ld.x,0.,-ld.z))), 0., 1. );//*clamp( 1.-sp.y,0.,1.);
    float dom = smoothstep( -.1, .1, ref.y );
    float fre = pow( clamp(1.+dot(sn,rd),0.,1.), 2. );

    reflexion = fre*occ;
    
#ifdef WITH_SHADOW
    dif *= calcSoftshadow( sp, ld, .05, 2. );
#endif
    
    float spe =  pow( clamp( dot( sn, -hal ), 0., 1. ), id >= ID_SHORT ? 10. : 164.) * dif * (.04 + .96*pow( clamp(1.+dot(hal,rd),0.,1.), 50. ));

    vec3 lin = vec3(0.0);
    lin += .80*dif*general/*vec3(1.00,0.80,0.55)*/*(.3+.7*occ);
    lin += .40*amb*occ*general;//vec3(0.40,0.60,1.00);
   // lin += .15*dom*occ*general;//vec3(0.40,0.60,1.00)*occ;
    lin += .15*bac*back/*vec3(0.25,0.25,0.25)*/*occ;
   // lin += .25*fre*vec3(1.00,1.00,1.00)*occ;
   
    col = col*lin;
    col += (id == ID_EYE ? 10. : id >= ID_SHORT ? .3 : 1.)*spe*vec3(1.00,0.90,0.70);
    
    return col;
}



//---------------------------------------------------------------------
//   Calculate normal
//   From TekF 
//---------------------------------------------------------------------
vec3 Normal(in vec3 pos, in vec3 ray, in float t) {
	float pitch = .1 * t / iResolution.x;   
	pitch = max( pitch, .002 );
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx, // tetrahedral offsets
	     p1 = pos+d.xyy,
	     p2 = pos+d.yxy,
	     p3 = pos+d.yyx;

 	float f0 = mapGirl(p0), f1 = mapGirl(p1), f2 = mapGirl(p2),	f3 = mapGirl(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}

vec3 NormalGround(in vec3 pos, in vec3 ray, in float t) {
	float pitch = .2 * t / iResolution.x;   
	pitch = max( pitch, .005 );
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx, // tetrahedral offsets
	     p1 = pos+d.xyy,
	     p2 = pos+d.yxy,
	     p3 = pos+d.yyx;

 	float f0 = mapGround(p0), f1 = mapGround(p1), f2 = mapGround(p2), f3 = mapGround(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}



//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

mat3 setCamera(in vec3 ro, in vec3 ta, in float cr) {
	vec3 cw = normalize(ta-ro),
		 cp = vec3(sin(cr), cos(cr), 0.),
		 cu = normalize( cross(cw,cp) ),
		 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}


//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------


// Interpolate pos of articulations
vec3 getPos(vec3 p, int it, float kt, float z, float dx) {
	return .02*vec3(p.x + dx, 150.-p.y, p.z*z);
}
    
    
void initLookingPosition(int idPlanet) {
	const int it = 6;
	const float kt = 0.;

	vec3 delta = gStaticPos;

	float dx = 0.;
	float mv = .5*cos(3.*iTime);
		
        
	head = delta + getPos(vec3(85+94,20,0), it, kt, 1., dx);

	shoulder1 = delta +getPos(vec3(85+91,43,16), it, kt, -1., dx) + .02*vec3(-5,0,0);
	elbow1 = delta +getPos(vec3(85+91,73,25), it, kt, -1., dx);
	wrist1 = delta +getPos(vec3(85+88,103,25), it, kt, -1., dx) + .02*vec3(18,22,7);

	foot1 = delta +getPos(vec3(182,150,10), it, kt, 1., dx)  + .02*vec3(54,54,4);
	ankle1 = delta +getPos(vec3(164,146,5), it, kt, 1., dx) + .02*vec3(56,42,8);
	knee1 = delta + getPos(vec3(167,118,7), it, kt, 1., dx) + .02*vec3(30,42,12);
	hip1 = delta +getPos(vec3(168,91,8), it, kt, 1., dx) + .02;

	shoulder2 = delta +getPos(vec3(85+91,43,16), it, kt, 1., dx) + .02*vec3(-5,0,0);
	elbow2 = delta +getPos(vec3(85+91,73,25), it, kt, 1., dx) + .02*vec3(4,5,0);
	wrist2 = delta +getPos(vec3(85+88,103,25), it, kt, 1., dx) + .02*vec3(23,29,-20);

	foot2 = delta +getPos(vec3(182,150,10), it, kt, -1., dx) + .02*vec3(56,52,-8);
	ankle2 = delta +getPos(vec3(164,146,5), it, kt, -1., dx) + .02*vec3(64,38,-8);
	knee2 = delta +getPos(vec3(167,118,7), it, kt, -1., dx) + .02*vec3(32,28,-8);
	hip2 = delta +getPos(vec3(168,91,8), it, kt, -1., dx)  + .02;
}

vec4 render(in vec3 ro, in vec3 rd, out vec3 roRef, out vec3 rdRef) {
	float t = MAX_DIST, traceStart = 0., traceEnd = MAX_DIST;

    // Render ------------------------
	vec3 col;
    vec4 colClouds = vec4(0);
    vec2 tScene = Trace(ro, rd, traceStart, t);
    
    if (tScene.x > MAX_DIST-5.) {
		colClouds = clouds(vec3(0), rd, t);
    }

	float reflection = 0.;
	
    roRef = ro;
    rdRef = rd;
    
	if (tScene.x < MAX_DIST) {
       
		vec3 pos = ro + rd*tScene.x;
        float id = tScene.y;
		vec3 sn, sceneColor, rnd = hash33(rd + 311.);

        if (id == ID_GROUND) {
            sn = NormalGround(pos, rd, tScene.x);
			sn = doBumpMap(iChannel2, pos/3., sn, .035/(1. + tScene.x/MAX_DIST)); 
			sceneColor = mix(vec3(.5,.45,.4), vec3(.12), clamp(0.,1.,2.*pos.y))+(fract(rnd*289. + tScene.x*41.) - .5)*.03;    
        } else {
			sn = Normal(pos, rd, tScene.x);
            if (id == ID_HELMET) {
                vec3 posRot = pos; 
    			posRot.xz = (posRot.xz-head.xz) * rotHead;
            	sn = doBumpMap(iChannel2, posRot/16.+.5, sn, .0004/(1. + tScene.x/MAX_DIST)); 
            }
			vec4 sceneColor4 = getColor(id, pos);
			id = sceneColor4.w;
			sceneColor = sceneColor4.rgb;
        }
		
        rdRef = reflect(rd,sn);
		roRef = pos + rdRef*.1;
        
        float reflexion;
        
		// Shading
		col = shading(pos, rd, sn, sceneColor, id, reflexion);
		reflection = ID_GROUND==id && pos.y<.2 ? .9 : (reflexion)*.2;

		// Fog
		col = mix(col, colClouds.rgb, smoothstep(MAX_DIST-5., MAX_DIST, tScene.x));
        col += (fract(rnd*289. + tScene.x*40001.) - .5)*.05;
        float f = MAX_DIST*.3;
        vec3 fogColor = vec3(.87,.85,1);
        col = mix( .2*fogColor, col, exp2(-tScene.x*fogColor/f) );
		
    } else {
		col = colClouds.rgb;
	}

	return vec4(col, reflection);
}








void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    if (iTime<30.) discard;

   
    gTime = iTime*8.;
   
    rotHead = rot(.6*(sin(.05*gTime)*cos(.001*gTime+1.11)));
        
    // Animation
    int it = int(floor(gTime));
    float kt = fract(gTime);

    // - init man position -----------------------------------
    initLookingPosition(0);
    const float a_eyeClose = .55, a_eyeOpen = -.3;
    const float t_openEye = 3., t_rotDown = 10., t_closeEye = 1.;
    
    // - Eye blink -------------------------------------------
    float time = iTime;
    float a_PaupieresCligne = mix(a_eyeOpen,a_eyeClose, hash(floor(time*10.))>.98?2.*abs(fract(20.*time)-.5):0.);    
    float a_Paupieres = mix(a_eyeClose, .2, smoothstep(t_openEye, t_openEye+3., time));    
    a_Paupieres = mix(a_Paupieres, a_PaupieresCligne, smoothstep(t_rotDown, t_rotDown+1., time));

    g_eyeRot = rot(a_Paupieres);
    
// Init base vectors --------------------------------------------

  // Foot1 flat part - vector base linked to leg 1
    v2Foot1 = normalize(knee1 - ankle1);
	vec3 v1Foot1 = normalize(ankle1 - foot1-v2Foot1*.1);
	v3Foot1 = cross(v1Foot1,v2Foot1);
	v2Foot12 = -cross(v1Foot1, v3Foot1);

    v2Foot2 = normalize(knee2 - ankle2);
    vec3 v1Foot2 = normalize(ankle2 - foot2-v2Foot2*.1);
	v3Foot2 = cross(v1Foot2,v2Foot2);
    v2Foot22 = -cross(v1Foot2, v3Foot2); 

	// Arm 1
    v1Hand1 = normalize(wrist1-elbow1);
    v3Hand1 = -normalize(cross(v1Hand1,normalize(wrist1-shoulder1)));
    v2Hand1 = -cross(v1Hand1,v3Hand1);
    v3Hand1 = v3Hand1;          

    v1Hand2 = normalize(wrist2-elbow2);
    v3Hand2 = -normalize(cross(v1Hand2,normalize(wrist2-shoulder2)));
    v2Hand2 = -cross(v1Hand2,v3Hand2);
    v3Hand2 = -v3Hand2; 

// --------------------------------------------------------------

    vec2 m = iMouse.xy/iResolution.y - .5;

	float traceStart = .2;

    vec3 ro, rd;
  	vec2 q;
    
// - Camera -----------------------------------------
    
	q = (fragCoord.xy)/iResolution.xy;
	vec2 p = -1. + 2.*q;
	p.x *= iResolution.x/iResolution.y;


	
    time = iTime - 40. - 30.;
    float dist =  mix(30.,2.,smoothstep(0.,20.,time)) + mix(0.,60., smoothstep(130.,240.,time));

    vec3 ta = head + vec3(dist*.1,-.05,0);
    ro = ta + dist*vec3(cos(.1*time/* + 6.28*m.x*/), mix(.02,-.13,smoothstep(20.,60.,time)), sin(.1*time/* + 6.28*m.x*/));
	   

	// camera-to-world transformation
	mat3 ca = setCamera(ro, ta, 0.0);

	// ray direction
	rd = ca * normalize( vec3(p.xy,mix(3.5,4.5,smoothstep(0.,20.,time))));

// ---------------------------------------------------
    vec3 roRef, rdRef;
	vec4 col = render(ro, rd, roRef, rdRef);

#ifdef WITH_REFLECTION
		vec3 roRef2, rdRef2;
		col.rgb = col.rgb*(1.- col.w) + col.w*clouds(vec3(0), rdRef, MAX_DIST).rgb;
#endif

// Post processing stuff --------------------

	// Gamma
     col.rgb = pow(col.rgb, vec3(0.6545) );

    // Vigneting
    col.rgb *= pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y), .15); 
    
	fragColor = col;
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//-----------------------------------------------------
// Created by sebastien durand - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------

#define IS_RUNNING

//#define WITH_SHADOW
#define WITH_AO


#ifndef IS_RUNNING
  #define WITH_REFLECTION
 
#endif
 #define WITH_FACE
// Isosurface Renderer
#define g_traceLimit 48
#define g_traceSize .002


#define MAX_DIST 64.
#define RUN_STEP 110.

#define rot(a) mat2(cos(a),sin(a),-sin(a),cos(a))



const vec3 gStaticPos = vec3(-95.,8.83,-121.7); //vec3(-95.5,12.8,-123);

const vec3 g_nozePos = vec3(0,-.28+.04,.55);
const vec3 g_eyePos = vec3(.14,-.14,.29);
const float g_eyeSize = .09;
const mat2 ma = rot(-.5);
const mat2 mb = rot(-.15);
const mat2 mc = rot(-.6);




float gTime;
mat2 rotHead, g_eyeRot;
vec3 gLightPos = vec3(2, 1, 6);



//	[Shane] Combustible Clouds
//	------------------

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
vec3 hash33(vec3 p){ 
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}

// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
float n3D( in vec3 p ){
    vec3 i = floor(p); p -= i; p *= p*(3. - 2.*p);
	p.xy = texture(iChannel0, (p.xy + i.xy + vec2(37, 17)*i.z + .5)/256., -100.).yx;
	return mix(p.x, p.y, p.z);
}

// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
float trigNoise3D(in vec3 p){ 
    float res = 0., sum = 0.;
    float n = n3D(p*8.);
    vec3 t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265+1.57/2.))*0.5 + 0.5;
    p = p*1.5 + (t - 1.5); //  + iTime*0.1
    res += (dot(t, vec3(0.333)));
    t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265+1.57/2.))*0.5 + 0.5;
    res += (dot(t, vec3(0.333)))*0.7071;    
	return ((res/1.7071))*0.85 + n*0.15;
}

vec4 clouds(in vec3 ro, in vec3 rd, in float tend) {
    vec3 lp = gLightPos;
    vec3 rnd = hash33(rd + 311.);
    float lDe = 0., td = 0., w = 0.;
    float d = 1., t = dot(rnd, vec3(.08));
    const float h = .5;
    vec3 col = vec3(0), sp;
    vec3 sn = normalize(hash33(rd.yxz)*.03-rd);

    // Raymarching loop.
    for (int i=0; i<48; i++) {
        if((td>1.) || d<.001*t || t>70. || t>tend) break;

        sp = ro + rd*t; // Current ray position.
        d = trigNoise3D(sp*.75); // Closest distance to the surface... particle.
        lDe = (h - d) * step(d, h); 
        w = (1. - td) * lDe;   
        td += w*w*8. + 1./60.; //w*w*5. + 1./50.;
        vec3 ld = lp-sp; // Direction vector from the surface to the light position.
        float lDist = max(length(ld), 0.001); // Distance from the surface to the light.
        ld/=lDist; // Normalizing the directional light vector.
        float atten = 1./(1. + lDist*0.1 + lDist*lDist*.03);
        float diff = max(dot(sn, ld ), 0.);
        float spec = pow(max(dot( reflect(-ld, sn), -rd ), 0.), 4.);
        col += w*(1.+ diff*.5 + spec*.5)*atten;
        col += (fract(rnd*289. + t*41.) - .5)*.02;;
        t +=  max(d*.5, .02); //
    }
    
    col = max(col, 0.);
    col = mix(pow(vec3(1.3, 1, 1)*col, vec3(1, 2, 10)), col, dot(cos(rd*6. +sin(rd.yzx*6.)), vec3(.333))*.2+.8);
    vec3 sky = vec3(.6, .8, 1.)*min((1.5+rd.y*.5)/2., 1.); 	
    sky = mix(vec3(1, 1, .9), vec3(.31, .42, .53), rd.y*0.5 + 0.5);
    
    float sun = clamp(dot(normalize(lp-ro), rd), 0.0, 1.0);
   
    // Combining the clouds, sky and sun to produce the final color.
    sky += vec3(1, .3, .05)*pow(sun, 5.)*.25; 
    sky += vec3(1, .4, .05)*pow(sun, 16.)*.35; 	
    col = mix(col, sky, smoothstep(0., 25., t));
 	col += vec3(1, .6, .05)*pow(sun, 16.)*.25; 	
 
    // Done.
    return vec4(col, clamp(0.,1.,td));    
}


// standard       - https://www.shadertoy.com/view/4sjGzc

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------
// RUN ------
//                       Contact           Down               Pass               Up      

vec3[9] HEAD2 = vec3[9]( vec3(67,17,0),    vec3(184,23,0),     vec3(279,18,0),     vec3(375,14,0), vec3(67,17,0),    vec3(184,23,0),     vec3(279,18,0),   vec3(375,14,0),   vec3(67,17,0));
vec3[9] SHOULDER2 = vec3[9](vec3(60,38,16), vec3(178,46,16),    vec3(273,41,16),    vec3(369,38,16), vec3(67,42,16),    vec3(182,49,16), vec3(273,41,16), vec3(369,38,16), vec3(60,38,16));
vec3[9] ELBOW2 = vec3[9]( vec3(36,43,25),   vec3(155,58,25), vec3(262,64,25), vec3(371,61,25), vec3(75,62,25),   vec3(186,72,25), vec3(273,67,25), vec3(355,55,25), vec3(36,43,25));
vec3[9] WRIST2 = vec3[9](vec3(24,60,20),  vec3(148,77,20), vec3(271,84,25), vec3(391,68,25), vec3(93,54,10),  vec3(206,67,20), vec3(291,77,25), vec3(360,80,20), vec3(24,60,20));
vec3[9] HIP2 = vec3[9](vec3(55,76,8.),  vec3(171,84,8.),   vec3(264,79,8.),   vec3(360,77,8.),  vec3(50,78,8.), vec3(171,84,8.),  vec3(264,79,8.),  vec3(360,77,8.), vec3(55,76,8.));
vec3[9] KNEE2 = vec3[9]( vec3(73,102,7), vec3(188,111,8),  vec3(267,112,10), vec3(358,107,10),  vec3(41,105,7), vec3(169,115,7),  vec3(279,108,7),  vec3(386,99,7), vec3(73,102,7));
vec3[9] ANKLE2=vec3[9](vec3(89,131,5),   vec3(175,142,6),   vec3(255,142,10),  vec3(340,135,10),  vec3(7,108,5), vec3(138,114,5),  vec3(250,115,5),  vec3(372,126,5), vec3(89,131,5));
vec3[9] FOOT2 = vec3[9](  vec3(104,127,10), vec3(191,144,10),  vec3(270,144,10),  vec3(350,144,10),  vec3(3,122,10),vec3(131,126,10), vec3(246,129,10), vec3(382,136,10), vec3(104,127,10));

vec3 shoulder1, elbow1, wrist1, head, shoulder2, elbow2, wrist2;
vec3 foot1, ankle1, knee1, hip1, foot2, ankle2, knee2, hip2;
vec3 v2Foot1, v2Foot12, v2Foot2, v2Foot22, v3Foot1, v3Foot2;
vec3 v1Hand1, v2Hand1, v3Hand1, v1Hand2, v2Hand2, v3Hand2;


// Interpolate pos of articulations when running
// TODO: standardize arrays to fusion with getPos
vec3 getPos2(vec3 arr[9], int it0, float kt, float z) {
    int it = it0 % 8;
    
    vec3 p0 = arr[it], p1 = arr[it+1];
    p0.x -= float(it%4)*80. - (it>3?RUN_STEP:0.);
    p1.x -= float((it+1)%4)*80. - (it>2?RUN_STEP:0.) - (it>6?RUN_STEP:0.);
    
    vec3 p = mix(p0, p1, /*smoothstep(0.,1.,*/kt/*)*/);
    if (z<0.) p.x += -RUN_STEP;
    
    p.x += float(it0/8)*RUN_STEP*2.;
    return .02*vec3(p.x, 144.-p.y, p.z*z);
}


//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

float hash( float n ) { return fract(sin(n)*43758.5453123); }

// mix noise for alive animation, full source
vec4 hash4( vec4 n ) { return fract(sin(n)*1399763.5453123); }
vec3 hash3( vec3 n ) { return fract(sin(n)*1399763.5453123); }


//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] https://iquilezles.org/articles/distfunctions
//---------------------------------------------------------------------

bool cube(vec3 ro, vec3 rd, vec3 sz, out float tn, out float tf) { 
	vec3 m = 1./rd,
         k = abs(m)*sz,
         a = -m*ro-k*.5, 
         b = a+k;
    tn = max(max(a.x,a.y),a.z);
    tf = min(min(b.x,b.y),b.z); 
	return tn>0. && tn<tf;
}


float sdCap(in vec3 p, in vec3 a, in vec3 b, in float r ) {
    vec3 pa = p - a, ba = b - a;
    return length(pa - ba*clamp( dot(pa,ba)/dot(ba,ba), 0., 1. ) ) - r;
}

float sdCapsule(in vec3 p, in vec3 a, in vec3 b, in float r0, in float r1 ) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0., 1.);
    return length( pa - ba*h ) - mix(r0,r1,h);
}

float sdCap2(in vec3 p, in vec3 a, in vec3 b, in float r1, in float r2) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. );
    return length(pa - ba*h) - mix(r1,r2,h*h*h);
}

float udRoundBox(in vec3 p, in vec3 b, in float r ) {
  return length(max(abs(p)-b,0.))-r;
}

//https://www.shadertoy.com/view/Xs3GRB
float fCylinder(in vec3 p, in float r, in float height) {
	return max(length(p.xz) - r, abs(p.y) - height);
}

float sdPlane(in vec3 p, in vec3 n) {  // n must be normalized
  return dot(p,n);
}

float smin(in float a, in float b, in float k ) {
    float h = clamp( .5+.5*(b-a)/k, 0., 1. );
    return mix( b, a, h ) - k*h*(1.-h);
}

// Smooth maximum, based on the function above.
float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5 + 0.5*(a-b)/s, 0., 1.);
    return mix(b, a, h) + h*(1.0-h)*s;
}

// hg_sdf : http://mercury.sexy/hg_sdf/
float fOpEngrave(float a, float b, float r) {
	return max(a, (a + r - abs(b))*0.7071); //sqrt(.5));
}

float fOpIntersectionRound(float a, float b, float r) {
	vec2 u = max(vec2(r + a,r + b), vec2(0));
	return min(-r, max (a, b)) + length(u);
}

float sdEllipsoid( in vec3 p, in vec3 r) {
    return (length(p/r ) - 1.) * min(min(r.x,r.y),r.z);
}


float fOpDifferenceRound (float a, float b, float r) {
	return fOpIntersectionRound(a, -b, r);
}


//---------------------------------------------------------------------
//    Man + Ground distance field 
//---------------------------------------------------------------------

// The canyon, complete with hills, gorges and tunnels. I would have liked to provide a far
// more interesting scene, but had to keep things simple in order to accommodate slower machines.
float mapGround(in vec3 p){
    float tx = -.2*(textureLod(iChannel1, p.xz/16. + p.xy/80., 0.0).x);
    vec3 q = p*0.25;

    float h = tx + .2+.5*(dot(sin(q)*cos(q.yzx), vec3(.222))) + dot(sin(q*1.3)*cos(q.yzx*1.4), vec3(.111));
    float d = p.y + smin(0.,smoothstep(.2,3., abs(p.z))*(-h)*6.,.2);

    return d; 
}

// capsule with bump in the middle -> use for neck
float sdCapsule2(in vec3 p,in vec3 a,in vec3 b, in float r0,in float r1,in float bump) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1. );
    float dd = bump*sin(3.14*h);  // Little adaptation
    return length(pa - ba*h) - mix(r0,r1,h)*(1.+dd); 
}


float smax(in float a, in float b, in float k) {
    return log(exp(a/k)+exp(b/k))*k;
}


float mapHead(in vec3 p) {
    float d;
   
// Skull modeling -------------------------
    d = sdEllipsoid(p-vec3(0,.05,.0), vec3(.39,.48,.46));				  
    d = smin(d, sdEllipsoid(p-vec3(0.,.1,-.15), vec3(.42,.4,.4)),.1);     
    d = smin(d, udRoundBox(p-vec3(0,-.28,.2), vec3(.07,.05,.05),.05),.4); // Basic jaw 

// Symetrie -------------------------------
    p.x = abs(p.x);

// Eye hole 
    d = smax(d, -sdEllipsoid(p-vec3(.12,-.16,.48), vec3(.09,.06,.09)), .07);

// Noze ------------------------------------
    d = smin(d, max(-(length(p-vec3(.032,-.325,.45))-.028),   // Noze hole
                    smin(length(p-vec3(.043,-.29,.434))-.01,  // Nostrils
                    sdCapsule(p, vec3(0,-.13,.39), vec3(0,-.28,.47), .01,.04), .05)) // Bridge of the nose
            ,.065); 
   
// Mouth -----------------------------------    
    d = smin(d, length(p- vec3(.22,-.34,.08)), .17); // Jaw
    d = smin(d, sdCapsule(p, vec3(.16,-.35,.2), vec3(-.16,-.35,.2), .06,.06), .15); // Cheeks
   
    d = smin(d, max(-length(p.xz-vec2(0,.427))+.015,  	// Line under the noze
        		max(-p.y-.41,   						// Upper lip
                    sdEllipsoid(p- vec3(0,-.34,.37), vec3(.08,.15,.05)))), // Mouth bump
             .032);

// Chin -----------------------------------  
    d = smin(d, length(p- vec3(0,-.5,.26)), .2);   // Chin
    d = smin(d, length(p- vec3(0,-.44,.15)), .25); // Under chin 
  
    //d = smin(d, sdCapsule(p, vec3(.24,-.1,.33), vec3(.08,-.05,.46), .0,.01), .11); // Eyebrow 
    
// Eyelid ---------------------------------
	vec3 p_eye1 = p - g_eyePos;
    p_eye1.xz *= mb;
    
    vec3 p_eye2 = p_eye1;
    float d_eye = length(p_eye1) - g_eyeSize*1.;
          
	p_eye1.yz *= g_eyeRot;
	p_eye2.zy *= mc;
    
    float d1 = min(max(-p_eye1.y,d_eye - .01),
                   max(p_eye2.y,d_eye - .005));

    d = smin(d,d1,.01);
	return d; 
}



float mapLegs(const in vec3 pos){    
    // Leg 1
    float d = max(min(sdCap2(pos, foot1, ankle1, .1,.15),
                      sdCap2(pos, ankle1, knee1, .165,.105)),
				  -sdPlane(pos-ankle1+v2Foot1*.1, v2Foot12));
    // Leg 2
	d = min(d,max(min(sdCap2(pos, foot2, ankle2, .1,.15),
                      sdCap2(pos, ankle2, knee2, .165,.105)),
				  -sdPlane(pos-ankle2+v2Foot2*.1, v2Foot22)));
 
    d = fOpEngrave(d, min(min(sdCap(pos-ankle1, -.1*v3Foot1, .1*v3Foot1, .12), 
                              sdCap(pos-ankle2, -.1*v3Foot2, .1*v3Foot2, .12)), 
                          min(length(pos - knee1),length(pos - knee2))-.11), .015);
    
    d = min(d, sdCap2(pos,  hip1, knee1, .12, .075));
    return min(d, sdCap2(pos, hip2, knee2, .12, .075));
}

float mapGirl(const in vec3 pos){
    vec3 
         ep0 = mix(shoulder1,shoulder2,.5),
         ha0 = mix(hip1,hip2,.5),
         h1 = head + vec3(.0,-.24-.05,0),
         h2 = head + vec3(.0,.15-.05,0),
         hn = normalize(h1-h2),
         a = mix(ha0,ep0,.15), b = mix(ha0,ep0,.75);
    
    vec3 posRot = pos; // - head; 
    posRot.xz = (posRot.xz - head.xz) * rotHead + head.xz;
   // posRot.xz = (posRot.xz) * rotHead;
    
    float d = mapLegs(pos);

    // Head
    float scaleHead = 1.75;
    vec3 pHead = (posRot - head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = min(d, dHead);

	// Eye
    pHead.x = abs(pHead.x);
    vec3 p_eye = pHead-g_eyePos;
    p_eye.xz *= ma;  
	float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    d = min(d, dEye);
        
    // Arms
    d = min(d, sdCapsule2(pos, shoulder1, elbow1, .054,.051,.5));
    d = min(d, sdCapsule2(pos, shoulder2, elbow2, .054,.051,.5));
    float dArm = sdCap2(pos, elbow1, wrist1-.01*v1Hand1, .09, .055);
    dArm = min(dArm,  sdCap2(pos, elbow2, wrist2-.01*v1Hand2, .09, .055));
    dArm = fOpEngrave(dArm, 
                      min(min(length(pos - wrist2), length(pos - wrist1)),
                          min(length(pos - elbow2), length(pos - elbow1))) - .1,.008);

    
    // Neck and Shoulders
    d = smin(d, min(sdCapsule2(pos+vec3(.03,0,0), mix(shoulder1, shoulder2,.1),mix(shoulder1, shoulder2,.9),.08, .08,.6),
                    sdCap(pos,ep0-vec3(.03,0,0), head-vec3(.08,.1,0), .09)), .06);
    
    // Torso
	d = smin(d, min(sdCap2(pos, a+vec3(0,0,.03), b-vec3(0,0,.04), .19,.22),sdCap2(pos, a-vec3(0,0,.03), b+vec3(0,0,.04), .19,.22)),.18);
    
	// Fingers 1
    vec3 c = wrist1-v3Hand1*.03;
    float d2 = sdCap(pos, c-v1Hand1*.06+v2Hand1*.03+v3Hand1*.06, wrist1+.09*(v2Hand1+v1Hand1+v3Hand1), .02);
    d2 = min(d2, sdCap(pos, c, wrist1+.18*(v1Hand1+v2Hand1*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.01, wrist1+.2*(v1Hand1-v2Hand1*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.02, wrist1+.18*(v1Hand1-v2Hand1*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.04, wrist1+.15*(v1Hand1-v2Hand1*.8), .014));
    
	// Fingers 2     
    c = wrist2-v3Hand2*.03;
    d2 = min(d2, sdCap(pos, c-v1Hand2*.06+v2Hand2*.03+v3Hand2*.06, wrist2+.09*(v2Hand2+v1Hand2+v3Hand2), .02));
    d2 = min(d2, sdCap(pos, c, wrist2+.18*(v1Hand2+v2Hand2*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.01, wrist2+.2*(v1Hand2-v2Hand2*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.02, wrist2+.18*(v1Hand2-v2Hand2*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.04, wrist2+.15*(v1Hand2-v2Hand2*.8), .014));


    d = min(d, smin(d2, dArm, .05));
   
    // Base corps
    // v2 = normalize(a - b);
   // vec3 v3 = normalize(cross(vec3(1,0,0),v2));
   // vec3 v1 = cross(v3, v2);
    
    // Short
    float dShort = min(sdCap(pos, hip1+vec3(-.03,0,0), mix(hip1, knee1,.25), .13), 
                       sdCap(pos, hip2+vec3(-.03,0,0), mix(hip2, knee2,.25), .13));                    
    // TODO ca serait plus cool de bouger avec les hanches mais il faut recuperer une base fixee sur les hanches
    dShort = smin(dShort, mix(d, sdCap(pos, a, ha0+vec3(0,.1,0), .22),.5),.1);
    d = min(d, dShort);

   // Casque
	float dHelmet;
    posRot += vec3(.03,.02,0);
    dHelmet = max(sdPlane(posRot-h1+hn*.07, hn), mix(dHead, sdCap2(posRot, h1-vec3(.23,0,0), h2-vec3(0,.05,0),.28,.36),.5));
    dHelmet = max(-fCylinder(posRot-h1-vec3(.2,0,0), .18,.3), dHelmet); 
    dHelmet = fOpEngrave(dHelmet, sdCap(posRot-h2, -vec3(0,.1,1), -vec3(0,.1,-1), .1),.015);
   
	d = min(d, dHelmet);
//	d = min(d, mapGround(pos));
            
    return min(d2,d);
}


//---------------------------------------------------------------------
//    Girl colors 
//---------------------------------------------------------------------
#define min2(a, b) (a.x<b.x?a:b)
#define max2(a, b) (a.x>b.x?a:b)

#define ID_MAN 100.
#define ID_GROUND 90.
#define ID_GLOVE 106. 
#define ID_HELMET 107.
#define ID_FOOT 108.
#define ID_SHORT 110.
#define ID_LEG  201.
#define ID_SKIN 202.
#define ID_ARM  203.
#define ID_TORSO 204.
#define ID_EYE 205.


const vec3 COLOR_SKIN = vec3(.6,.43,.3);
const vec3 COLOR_ARMOR = vec3(.14,.79,.7);
const vec3 COLOR_CLOTHES2 = vec3(.14,.79,.7);
const vec3 COLOR_CLOTHES = vec3(.66,.94,.91);



vec4 drawEye(in vec3 p) {
    vec3 posRot = p; // - head; 
    posRot.xz = (posRot.xz - head.xz) * rotHead + head.xz;
    
    float scaleHead = 1.75;
    vec3 pHead = (posRot-head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;

	// Eye
    vec3 p_eye = pHead-g_eyePos;
    vec3 g_eyePosloc = g_eyePos;
    g_eyePosloc.x *= sign(pHead.x);
   	vec3 pe = pHead - g_eyePosloc;

   
    float a = .2*sin(2.*iTime)*cos(.01*iTime);//clamp(atan(-dir.x, dir.z), -.6,.6), 
    float ca = cos(a), sa = sin(a);
    pe.xz *= mat2(ca, sa, -sa, ca);

    float b = .2;//.1+.1*sin(iTime*.1);//clamp(atan(-dir.y, dir.z), -.3,.3), 
    float cb = cos(b), sb = sin(b);
    pe.yz *= mat2(cb, sb, -sb, cb);
    
    float d = length(pe.xy);
    vec3 col = mix(vec3(0), mix(vec3(.88,.41,.0), mix(vec3(0),vec3(1.5),
                   .5+.5*smoothstep(.0405,.0415,d)), smoothstep(.04,.041,d)), smoothstep(.02,.025,d));
   // float d2 = smoothstep(.03,.04,length(pe.xy));
    return vec4(col,ID_EYE);
}

float sdFish(vec3 o) {
    vec2 p = (o - (shoulder1 + shoulder2)*.5).zy + vec2(.04,.17);
    p *= 2.;
    float dsub = min(length(p-vec2(.8,.0)) - .45, length(p-vec2(-.14,.05)) - .11);  
    p.y = abs(p.y);
    float d = length(p-vec2(.0,-.15)) - .3;
    d = min(d, length(p-vec2(.56,-.15)) - .3);
    d = max(d, -dsub);
    return (1.-smoothstep(.05,.06,d));
}

           
// -----------------------------------------


vec4 getColor(float id, vec3 pos) {
	return 	.3+.7*(id == ID_FOOT ? vec4(COLOR_ARMOR, id) :
			id == ID_LEG ? vec4(COLOR_CLOTHES, ID_LEG) :
			id == ID_EYE ? drawEye(pos) :
         	id == ID_SKIN ?	vec4(COLOR_SKIN, ID_SKIN) :
            id == ID_TORSO ? vec4(mix(COLOR_CLOTHES,vec3(0),sdFish(pos)), ID_TORSO) :
			id == ID_ARM ? vec4(COLOR_SKIN, ID_ARM) :
			id == ID_SHORT ? vec4(COLOR_CLOTHES2, ID_SHORT) :
    		vec4(COLOR_ARMOR, id));
}


float mapColor(in vec3 pos) {
    vec3 
         ep0 = mix(shoulder1,shoulder2,.5),
         ha0 = mix(hip1,hip2,.5),
         h1 = head + vec3(.0,-.24-.05,0),
         h2 = head + vec3(.0,.15-.05,0),
         hn = normalize(h1-h2),
         a = mix(ha0,ep0,.15), b = mix(ha0,ep0,.79);
    vec3 posRot = pos; 
    posRot.xz = (posRot.xz - h1.xz) * rotHead + h1.xz;
    
    // Leg 1
    float d = max(min(sdCap2(pos, foot1, ankle1, .1,.15),
                      sdCap2(pos, ankle1, knee1, .165,.105)),
				  -sdPlane(pos-ankle1+v2Foot1*.1, v2Foot12));
    // Leg 2
	d = min(d,max(min(sdCap2(pos, foot2, ankle2, .1,.15),
                      sdCap2(pos, ankle2, knee2, .165,.105)),
				  -sdPlane(pos-ankle2+v2Foot2*.1, v2Foot22)));                          
    
    vec2 dd;
    dd = min2(vec2(d, ID_FOOT), 
              vec2(min(sdCap(pos, knee1, hip1, .075),
    				   sdCap(pos, knee2, hip2, .075)), ID_LEG));

    // Head
    float scaleHead = 1.75;
    vec3 pHead = (posRot - head) + vec3(.02, .04, 0);
    pHead = pHead.zyx*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = min(d, dHead);

	// Eye
    pHead.x = abs(pHead.x);
    vec3 p_eye = pHead-g_eyePos;
    p_eye.xz *= ma;  

    
	float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    dd = min2(dd, vec2(dEye,ID_EYE));

  //  d = min(d, dHead);
    dd = min2(dd, vec2(min(dHead,               // chin
						sdCap(pos, ep0-vec3(.03,0,0), head-vec3(.08,.1,0), .1)), ID_SKIN));  // neck           
    // Arms
    dd = min2(dd, vec2(min(sdCap(pos, shoulder2, elbow2, .05), sdCap(pos, shoulder1, elbow1, .05)), ID_LEG));
    
	// Fingers 1
    vec3 c = wrist1-v3Hand1*.03;
    float d2 = sdCap(pos, c-v1Hand1*.06+v2Hand1*.03+v3Hand1*.06, wrist1+.09*(v2Hand1+v1Hand1+v3Hand1), .02);
    d2 = min(d2, sdCap(pos, c, wrist1+.18*(v1Hand1+v2Hand1*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.01, wrist1+.2*(v1Hand1-v2Hand1*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.02, wrist1+.18*(v1Hand1-v2Hand1*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand1*.04, wrist1+.15*(v1Hand1-v2Hand1*.8), .014));
    
	// Fingers 2     
    c = wrist2-v3Hand2*.03;
    d2 = min(d2, sdCap(pos, c-v1Hand2*.06+v2Hand2*.03+v3Hand2*.06, wrist2+.09*(v2Hand2+v1Hand2+v3Hand2), .02));
    d2 = min(d2, sdCap(pos, c, wrist2+.18*(v1Hand2+v2Hand2*.2), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.01, wrist2+.2*(v1Hand2-v2Hand2*.2), .017));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.02, wrist2+.18*(v1Hand2-v2Hand2*.5), .016));
    d2 = min(d2, sdCap(pos, c-v2Hand2*.04, wrist2+.15*(v1Hand2-v2Hand2*.8), .014));

    d2 = min(d2, min(sdCap2(pos, elbow1, wrist1-.01*v1Hand1, .09, .055),  
                     sdCap2(pos, elbow2, wrist2-.01*v1Hand2, .09, .055)));
    dd = min2(dd, vec2(d2, ID_GLOVE));
           
    // Torso
    dd = min2(dd, vec2(min(sdCap(pos, shoulder1, shoulder2, .1), 
                           sdCap2(pos, a, b, .19,.22)), ID_TORSO));
  
    // Short
    float dShort = min(sdCap(pos, hip1, mix(hip1,knee1,.3), .12), 
                       sdCap(pos, hip2, mix(hip2,knee2,.3), .12));                    
    dd = min2(dd, vec2(min(dShort, mix(dd.x, sdCap(pos, a, ha0, .22),.75)), ID_SHORT));

    // Casque
	float dHelmet;
    posRot += vec3(.03,.02,0);
    dHelmet = max(sdPlane(posRot-h1+hn*.08, hn), mix(dHead, sdCap2(posRot, h1-vec3(.23,0,0), h2-vec3(0,.05,0),.28,.36),.5));
    dHelmet = max(-fCylinder(posRot-h1-vec3(.2,0,0), .18,.3), dHelmet); 
    dd = min2(dd, vec2(dHelmet, ID_HELMET));

    return dd.y;
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------
              
float logBisectTrace(in vec3 ro, in vec3 rd){

    float t = 0., told = 0., mid, dn;
    float d = mapGround(rd*t + ro);
    float sgn = sign(d);
#ifdef IS_RUNNING
    for (int i=0; i<64; i++){
#else        
    for (int i=0; i<92; i++){
#endif
        // If the threshold is crossed with no detection, use the bisection method.
        // Also, break for the usual reasons. Note that there's only one "break"
        // statement in the loop. I heard GPUs like that... but who knows?
        if (sign(d) != sgn || d < 0.001 || t > MAX_DIST) break;
        told = t;
        // Branchless version of the following:      
        t += step(d, 1.)*(log(abs(d) + 1.1) - d) + d;
        //t += log(abs(d) + 1.1);
        //t += d;//step(-1., -d)*(d - d*.5) + d*.5;
        d = mapGround(rd*t + ro);
    }
    // If a threshold was crossed without a solution, use the bisection method.
    if (sign(d) != sgn){
        // Based on suggestions from CeeJayDK, with some minor changes.
        dn = sign(mapGround(rd*told + ro));
        vec2 iv = vec2(told, t); // Near, Far
        // 6 iterations seems to be more than enough, for most cases...
        // but there's an early exit, so I've added a couple more.
        for (int ii=0; ii<8; ii++){ 
            //Evaluate midpoint
            mid = dot(iv, vec2(.5));
            float d = mapGround(rd*mid + ro);
            if (abs(d) < 0.001)break;
            // Suggestion from movAX13h - Shadertoy is one of those rare
            // sites with helpful commenters. :)
            // Set mid to near or far, depending on which side we're on.
            iv = mix(vec2(iv.x, mid), vec2(mid, iv.y), step(0.0, d*dn));
        }

        t = mid; 
        
    }
    
    //if (abs(d) < PRECISION) t += d;

    return min(t, MAX_DIST);
}

vec2 Trace(in vec3 pos, in vec3 ray, in float start, in float end ) {
    // Trace if in bbox
    float t=start, h, tn=start, tf=end;
    float tGround = logBisectTrace(pos, ray);

   // start = max(start, );
    end = min(tGround, end);
    
    if (cube(pos-head-vec3(-.1,-1.,0), ray, vec3(1.2, 1.7,.7)*2.,  tn, tf)) {
        end = min(tf, end);
        t = max(tn, start);// - .3*hash33(pos+ray).x;
        for( int i=0; i < g_traceLimit; i++) {
			if (t > end) break;
            h = mapGirl( pos+t*ray );
            if (h < g_traceSize) {
                return vec2(t+h, mapColor(pos+t*ray));
            }
            t += h;
        }
        if (t < end) return vec2(t, mapColor(pos+t*ray)); 
    } 
    
    return tGround < MAX_DIST ?  vec2(tGround, ID_GROUND) : vec2(MAX_DIST, 0.);
}

//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
    n = max(n*n, 0.001);
    n /= (n.x + n.y + n.z );  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

float getGrey(vec3 p){ return dot(p, vec3(0.299, 0.587, 0.114)); }

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


//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

#ifdef WITH_SHADOW
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax ) {
	float h, res = 1., t = mint;
    for(int i=0; i<24; i++) {
		h = map( ro + rd*t );
        res = min( res, 8.*h/t );
        t += clamp( h, .05, .2 );
        if( h<.01 || t>tmax ) break;
    }
    return clamp(res, 0., 1.);
}
#endif

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
float calcAO( in vec3 pos, in vec3 nor ){
	float dd, hr, sca = 1., totao = 0.;
    vec3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = .01 + .05*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(mapGirl( aopos )-hr)*sca;
        sca *= .75;
    }
    return clamp(1. - 4.*totao, 0., 1.);
}
float calcAOGround( in vec3 pos, in vec3 nor ){
	float dd, hr, sca = 1., totao = 0.;
    vec3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = .01 + .05*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(min(mapGround(aopos),mapLegs(aopos))-hr)*sca;
        sca *= .75;
    }
    return clamp(1. - 4.*totao, 0., 1.);
}
#endif


//---------------------------------------------------------------------
//   Shading
//   Adapted from Shane / Iq
//---------------------------------------------------------------------

vec3 shading( in vec3 sp, in vec3 rd, in vec3 sn, in vec3 col, in float id, out float reflexion){
    
    vec3 general = vec3(240,198,157)/256.,
    	 back = vec3(63,56,46)/256.;
    vec3 ref = reflect( rd, sn );

    // lighitng   
#ifdef WITH_AO
    float occ = id == ID_GROUND ? calcAOGround( sp, sn ) : calcAO( sp, sn );
#else
    float occ = 1.;
#endif
    vec3  ld = normalize( gLightPos );
    vec3  hal = normalize( rd - ld);
    float amb = .15; //clamp( .5+.5*sn.y, 0., 1. );
    float dif = clamp( dot( sn, ld ), 0., 1. );
    float bac = clamp( dot( sn, normalize(vec3(-ld.x,0.,-ld.z))), 0., 1. );//*clamp( 1.-sp.y,0.,1.);
    float dom = smoothstep( -.1, .1, ref.y );
    float fre = pow( clamp(1.+dot(sn,rd),0.,1.), 2. );

    reflexion = fre*occ;
    
#ifdef WITH_SHADOW
    dif *= calcSoftshadow( sp, ld, .05, 2. );
#endif
    
    float spe =  pow( clamp( dot( sn, -hal ), 0., 1. ), id >= ID_SHORT ? 10. : 164.) * dif * (.04 + .96*pow( clamp(1.+dot(hal,rd),0.,1.), 50. ));

    vec3 lin = vec3(0.0);
    lin += .80*dif*general/*vec3(1.00,0.80,0.55)*/*(.3+.7*occ);
    lin += .40*amb*occ*general;//vec3(0.40,0.60,1.00);
   // lin += .15*dom*occ*general;//vec3(0.40,0.60,1.00)*occ;
    lin += .15*bac*back/*vec3(0.25,0.25,0.25)*/*occ;
   // lin += .25*fre*vec3(1.00,1.00,1.00)*occ;
   
    col = col*lin;
    col += (id == ID_EYE ? 10. : id >= ID_SHORT ? .3 : 1.)*spe*vec3(1.00,0.90,0.70);
    
    return col;
}



//---------------------------------------------------------------------
//   Calculate normal
//   From TekF 
//---------------------------------------------------------------------
vec3 Normal(in vec3 pos, in vec3 ray, in float t) {
	float pitch = .1 * t / iResolution.x;   
	pitch = max( pitch, .002 );
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx, // tetrahedral offsets
	     p1 = pos+d.xyy,
	     p2 = pos+d.yxy,
	     p3 = pos+d.yyx;

 	float f0 = mapGirl(p0), f1 = mapGirl(p1), f2 = mapGirl(p2),	f3 = mapGirl(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}

vec3 NormalGround(in vec3 pos, in vec3 ray, in float t) {
	float pitch = .2 * t / iResolution.x;   
	pitch = max( pitch, .005 );
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx, // tetrahedral offsets
	     p1 = pos+d.xyy,
	     p2 = pos+d.yxy,
	     p3 = pos+d.yyx;

 	float f0 = mapGround(p0), f1 = mapGround(p1), f2 = mapGround(p2), f3 = mapGround(p3);
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}



//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

mat3 setCamera(in vec3 ro, in vec3 ta, in float cr) {
	vec3 cw = normalize(ta-ro),
		 cp = vec3(sin(cr), cos(cr), 0.),
		 cu = normalize( cross(cw,cp) ),
		 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}


//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------


    
void initRunningPosition(int it, float kt) {
	head = getPos2(HEAD2, it, kt, 1.);

	shoulder1 = getPos2(SHOULDER2, it, kt, 1.);
	elbow1 = getPos2(ELBOW2, it, kt, 1.);
	wrist1 = getPos2(WRIST2, it, kt, 1.);

	foot1 = getPos2(FOOT2, it, kt, 1.);
	ankle1 = getPos2(ANKLE2, it, kt, 1.);
	knee1 = getPos2(KNEE2, it, kt, 1.);
	hip1 = getPos2(HIP2, it, kt, 1.);

	shoulder2 = getPos2(SHOULDER2, it+4, kt, -1.);
	elbow2 = getPos2(ELBOW2, it+4, kt, -1.);
	wrist2 = getPos2(WRIST2, it+4, kt, -1.);

	foot2 = getPos2(FOOT2, it+4, kt, -1.);
	ankle2 = getPos2(ANKLE2, it+4, kt, -1.);
	knee2 = getPos2(KNEE2, it+4, kt, -1.);
	hip2 = getPos2(HIP2, it+4, kt, -1.);
}



vec4 render(in vec3 ro, in vec3 rd, out vec3 roRef, out vec3 rdRef) {
	float t = MAX_DIST, traceStart = 0., traceEnd = MAX_DIST;

    // Render ------------------------
	vec3 col;
    vec4 colClouds = vec4(0);
    vec2 tScene = Trace(ro, rd, traceStart, t);
    
    if (tScene.x > MAX_DIST-5.) {
		colClouds = clouds(vec3(0), rd, t);
    }

	float reflection = 0.;
	
    roRef = ro;
    rdRef = rd;
    
	if (tScene.x < MAX_DIST) {
       
		vec3 pos = ro + rd*tScene.x;
        float id = tScene.y;
		vec3 sn, sceneColor, rnd = hash33(rd + 311.);

        if (id == ID_GROUND) {
            sn = NormalGround(pos, rd, tScene.x);
			sn = doBumpMap(iChannel2, pos/4., sn, .025/(1. + tScene.x/MAX_DIST)); 
			sceneColor = mix(1.2*vec3(.5,.45,.4), vec3(.12), clamp(0.,1.,2.*pos.y))+(fract(rnd*289. + tScene.x*41.) - .5)*.03;    
        } else {
			sn = Normal(pos, rd, tScene.x);
   #ifndef IS_RUNNING
            if (id == ID_HELMET) {
                vec3 posRot = pos; 
    			posRot.xz = (posRot.xz-head.xz) * rotHead;
            	sn = doBumpMap(iChannel2, posRot/16.+.5, sn, .0004/(1. + tScene.x/MAX_DIST)); 
            }
   #endif         
			vec4 sceneColor4 = getColor(id, pos);
			id = sceneColor4.w;
			sceneColor = sceneColor4.rgb;
        }
		
        rdRef = reflect(rd,sn);
		roRef = pos + rdRef*.1;
        
        float reflexion;
        
		// Shading
		col = shading(pos, rd, sn, sceneColor, id, reflexion);
		reflection = ID_GROUND==id && pos.y<.2 ? .9 : (reflexion)*.2;

		// Fog
		col = mix(col, colClouds.rgb, smoothstep(MAX_DIST-5., MAX_DIST, tScene.x));
        col += (fract(rnd*289. + tScene.x*4001.) - .5)*.05;
        float f = MAX_DIST*.3;
        vec3 fogColor = vec3(.87,.85,1);
        col = mix( .2*fogColor, col, exp2(-tScene.x*fogColor/f) );
		
    } else {
		col = colClouds.rgb;
	}

	return vec4(col, reflection);
}



const float
    a_eyeClose = .55, 
    a_eyeOpen = -.3;




void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

    if (iTime>31.) discard;
    
    gTime = iTime*8.+.1;
   
    rotHead = rot(.6*(sin(.05*gTime)*cos(.001*gTime+1.11)));
        
    // Animation
    int it = int(floor(gTime));
    float kt = fract(gTime);

#ifdef IS_RUNNING
//    drawHand = false;
    const bool isRunning = true; //cos(iTime*.2)>0.;;
#else
//    drawHand = true;
    const bool isRunning = false;
#endif

    // - init man position -----------------------------------
    
#ifdef IS_RUNNING
  //  if (isRunning) {
		initRunningPosition(it, kt);
  //  } else {
//		initWalkingPosition(it, kt);
  //  }
#else
	initLookingPosition(0);
#endif

//    const float t_openEye = 3., t_rotDown = 10., t_closeEye = 1.;
    // - Eye blink -------------------------------------------
    float time = iTime;
/*    float a_PaupieresCligne = mix(a_eyeOpen,a_eyeClose, hash(floor(time*10.))>.98?2.*abs(fract(20.*time)-.5):0.);    
    float a_Paupieres = mix(a_eyeClose, .2, smoothstep(t_openEye, t_openEye+3., time));    
    a_Paupieres = mix(a_Paupieres, a_PaupieresCligne, smoothstep(t_rotDown, t_rotDown+1., time));
   // a_Paupieres = mix(a_Paupieres, a_eyeClose, smoothstep(t_closeEye, t_closeEye+3., time));
*/
    g_eyeRot = rot(-.3);
    
// Init base vectors --------------------------------------------

  // Foot1 flat part - vector base linked to leg 1
    v2Foot1 = normalize(knee1 - ankle1);
	vec3 v1Foot1 = normalize(ankle1 - foot1-v2Foot1*.1);
	v3Foot1 = cross(v1Foot1,v2Foot1);
	v2Foot12 = -cross(v1Foot1, v3Foot1);

    v2Foot2 = normalize(knee2 - ankle2);
    vec3 v1Foot2 = normalize(ankle2 - foot2-v2Foot2*.1);
	v3Foot2 = cross(v1Foot2,v2Foot2);
    v2Foot22 = -cross(v1Foot2, v3Foot2); 

	// Arm 1
    v1Hand1 = normalize(wrist1-elbow1);
    v3Hand1 = -normalize(cross(v1Hand1,normalize(wrist1-shoulder1)));
    v2Hand1 = -cross(v1Hand1,v3Hand1);
    v3Hand1 = isRunning ? -v3Hand1 : v3Hand1;          
   // if (drawHand) {
    	
   // }
	// Arm 2
    v1Hand2 = normalize(wrist2-elbow2);
    v3Hand2 = -normalize(cross(v1Hand2,normalize(wrist2-shoulder2)));
    v2Hand2 = -cross(v1Hand2,v3Hand2);
    v3Hand2 = isRunning ? v3Hand2 : -v3Hand2; 
//vec3 tmp = v2Hand2; v2Hand2 = -v3Hand2; v3Hand2 = tmp;
// --------------------------------------------------------------

    vec2 m = iMouse.xy/iResolution.y - .5;

	float traceStart = .2;

    vec3 ro, rd;
  	vec2 q;
    
// - Camera -----------------------------------------
    
	q = (fragCoord.xy)/iResolution.xy;
	vec2 p = -1. + 2.*q;
	p.x *= iResolution.x/iResolution.y;

    time = iTime - 35.;
    
    vec3 ta = vec3(mix(hip1.x,gTime*.58,.75), 1.4, 0.);
	ro = ta + 11.*vec3(cos(-.2*time),.07,sin(-.2*time));
	ro.y = max(0.01, ro.y);

    // camera-to-world transformation
	mat3 ca = setCamera(ro, ta, 0.0);

	// ray direction
	rd = ca * normalize( vec3(p.xy, 3.5) );

// ---------------------------------------------------
    vec3 roRef, rdRef;
	vec4 col = render(ro, rd, roRef, rdRef);

// Post processing stuff --------------------

    // Teinte
  // col.rgb = .2*length(col.rgb)*vec3(1,.5,0)+.8*col.rgb;

	// Gamma
     col.rgb = pow(col.rgb, vec3(0.6545) );

    // Vigneting
    col.rgb *= pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y), .15); 
    
	fragColor = col;
}

