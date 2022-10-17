
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Organic 1' to iChannel2
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel0
// Connect Buffer A 'Texture: Pebbles' to iChannel1

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//-----------------------------------------------------
// Created by sebastien durand - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------

#define WITH_AO
#define WITH_REFLECTION
#define WITH_FACE

// Isosurface Renderer
#define g_traceLimit 64
#define g_traceSize 0.002f


#define MAX_DIST 192.0f
#define RUN_STEP 120.0f

#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))

__DEVICE__ float gTime;
__DEVICE__ mat2 rotHead;

__DEVICE__ float3 gLightPos = to_float3(2, 1, 6);
__DEVICE__ const float3 gStaticPos = to_float3(-95.0f,8.83f,-121.7f); //to_float3(-95.5f,12.8f,-123);


//  [Shane] Combustible Clouds
//  ------------------

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
__DEVICE__ float3 hash33(float3 p){ 
    float n = _sinf(dot(p, to_float3(7, 157, 113)));    
    return fract_f3(to_float3(2097152, 262144, 32768)*n); 
}

// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
__DEVICE__ float n3D( in float3 p, __TEXTURE2D__ iChannel0 ){
    float3 i = _floor(p); p -= i; p *= p*(3.0f - 2.0f*p);
  swi2S(p,x,y, swi2(texture(iChannel0, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f, -100.0f),y,x));
  return _mix(p.x, p.y, p.z);
}

// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
__DEVICE__ float trigNoise3D(in float3 p, __TEXTURE2D__ iChannel0){ 
    float res = 0.0f, sum = 0.0f;
    // IQ's cheap, texture-lookup noise function. Very efficient, but still 
    // a little too processor intensive for multiple layer usage in a largish 
    // "for loop" setup. Therefore, just one layer is being used here.
    float n = n3D(p*8.0f,iChannel0);// + iTime*2.0f);
    // Two sinusoidal layers. I'm pretty sure you could get rid of one of 
    // the swizzles (I have a feeling the GPU doesn't like them as much), 
    // which I'll try to do later.
    
    float3 t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f+1.57f/2.0f))*0.5f + 0.5f;
    p = p*1.5f + (t - 1.5f); //  + iTime*0.1
    res += (dot(t, to_float3_s(0.333f)));

    t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f+1.57f/2.0f))*0.5f + 0.5f;
    res += (dot(t, to_float3_s(0.333f)))*0.7071f;    
   
  return ((res/1.7071f))*0.85f + n*0.15f;
}

__DEVICE__ float4 clouds(in float3 ro, in float3 rd, in float tend, __TEXTURE2D__ iChannel0) {
  //rd = swi3(rd,z,y,x);
    // Ray origin. Moving along the Z-axis.
    //vec3 ro = to_float3(0, 0, iTime*0.02f);

    // Placing a light in front of the viewer and up a little. You could just make the 
    // light directional and be done with it, but giving it some point-like qualities 
    // makes it a little more interesting. You could also rotate it in sync with the 
    // camera, like a light beam from a flying vehicle.
    float3 lp = gLightPos;
    //swi2(lp,x,z) = swi2(lp,x,z)*rM;
    //lp += ro;
    // The ray is effectively marching through discontinuous slices of noise, so at certain
    // angles, you can see the separation. A bit of randomization can mask that, to a degree.
    // At the end of the day, it's not a perfect process. Note, the ray is deliberately left 
    // unnormalized... if that's a word.
    //
    // Randomizing the direction.
    //rd = (rd + (hash33(swi3(rd,z,y,x))*0.004f-0.002f)); 
    // Randomizing the length also. 
    //rd *= (1.0f + fract(_sinf(dot(to_float3(7, 157, 113), swi3(rd,z,y,x)))*43758.5453f)*0.04f-0.02f);  
    
    //rd = rd*0.5f + normalize(rd)*0.5f;    
    
    // Some more randomization, to be used for color based jittering inside the loop.
    float3 rnd = hash33(rd + 311.0f);

    // Local density, total density, and weighting factor.
    float lDe = 0.0f, td = 0.0f, w = 0.0f;

    // Closest surface distance, and total ray distance travelled.
    float d = 1.0f, t = dot(rnd, to_float3_s(0.08f));

    // Distance threshold. Higher numbers give thicker clouds, but fill up the screen too much.    
    const float h = 0.5f;


    // Initializing the scene color to black, and declaring the surface position vector.
    float3 col = to_float3_s(0), sp;


    // Particle surface normal.
    //
    // Here's my hacky reasoning. I'd imagine you're going to hit the particle front on, so the normal
    // would just be the opposite of the unit direction ray. However particles are particles, so there'd
    // be some randomness attached... Yeah, I'm not buying it either. :)
    float3 sn = normalize(hash33(swi3(rd,y,x,z))*0.03f-rd);

    // Raymarching loop.
    for (int i=0; i<48; i++) {

        // Loop break conditions. Seems to work, but let me
        // know if I've overlooked something.
        if((td>1.0f) || d<0.001f*t || t>80.0f || t>tend) break;

        sp = ro + rd*t; // Current ray position.
        d = trigNoise3D(sp*0.75f,iChannel0); // Closest distance to the surface... particle.

        // If we get within a certain distance, "h," of the surface, accumulate some surface values.
        // The "step" function is a branchless way to do an if statement, in case you're wondering.
        //
        // Values further away have less influence on the total. When you accumulate layers, you'll
        // usually need some kind of weighting algorithm based on some identifying factor - in this
        // case, it's distance. This is one of many ways to do it. In fact, you'll see variations on 
        // the following lines all over the place.
        //
        lDe = (h - d) * step(d, h); 
        w = (1.0f - td) * lDe;   

        // Use the weighting factor to accumulate density. How you do this is up to you. 
        td += w*w*8.0f + 1.0f/60.0f; //w*w*5.0f + 1.0f/50.0f;
        //td += w*0.4f + 1.0f/45.0f; // Looks cleaner, but a little washed out.


        // Point light calculations.
        float3 ld = lp-sp; // Direction vector from the surface to the light position.
        float lDist = _fmaxf(length(ld), 0.001f); // Distance from the surface to the light.
        ld/=lDist; // Normalizing the directional light vector.

        // Using the light distance to perform some falloff.
        float atten = 1.0f/(1.0f + lDist*0.1f + lDist*lDist*0.03f);

        // Ok, these don't entirely correlate with tracing through transparent particles,
        // but they add a little anglular based highlighting in order to fake proper lighting...
        // if that makes any sense. I wouldn't be surprised if the specular term isn't needed,
        // or could be taken outside the loop.
        float diff = _fmaxf(dot(sn, ld ), 0.0f);
        float spec = _powf(_fmaxf(dot( reflect(-ld, sn), -rd ), 0.0f), 4.0f);

        // Accumulating the color. Note that I'm only adding a scalar value, in this case,
        // but you can add color combinations.
        col += w*(1.0f+ diff*0.5f + spec*0.5f)*atten;
        // Optional extra: Color-based jittering. Roughens up the grey clouds that hit the camera lens.
        col += (fract(rnd*289.0f + t*40001.0f) - 0.5f)*0.02f;;

        // Try this instead, to see what it looks like without the fake contrasting. Obviously,
        // much faster.
        //col += w*atten*1.25f;


        // Enforce minimum stepsize. This is probably the most important part of the procedure.
        // It reminds me a little of of the soft shadows routine.
        t +=  _fmaxf(d*0.5f, 0.02f); //
        // t += 0.2f; // t += d*0.5f;// These also work, but don't seem as efficient.

    }
    
    col = _fmaxf(col, to_float3_s(0.0f));
float zzzzzzzzzzzzzzzzzzzzz;
    
    // Adding a bit of a firey tinge to the cloud value.
    col = _mix(pow_f3(to_float3(1.3f, 1, 1)*col, to_float3(1, 2, 10)), col, dot(_cosf(rd*6.0f +sin_f3(swi3(rd,y,z,x)*6.0f)), to_float3_s(0.333f))*0.2f+0.8f);
 
    // Using the light position to produce a blueish sky and sun. Pretty standard.
    float3 sky = to_float3(0.6f, 0.8f, 1.0f)*_fminf((1.5f+rd.y*0.5f)/2.0f, 1.0f);   
    sky = _mix(to_float3(1, 1, 0.9f), to_float3(0.31f, 0.42f, 0.53f), rd.y*0.5f + 0.5f);
    
    float sun = clamp(dot(normalize(lp-ro), rd), 0.0f, 1.0f);
   
    // Combining the clouds, sky and sun to produce the final color.
    sky += to_float3(1, 0.3f, 0.05f)*_powf(sun, 5.0f)*0.25f; 
    sky += to_float3(1, 0.4f, 0.05f)*_powf(sun, 16.0f)*0.35f;   
    col = _mix(col, sky, smoothstep(0.0f, 25.0f, t));
    col += to_float3(1, 0.6f, 0.05f)*_powf(sun, 16.0f)*0.25f;   
 
    // Done.
    return to_float4_aw(col, clamp(0.0f,1.0f,td));
  
}


// standard       - https://www.shadertoy.com/view/4sjGzc

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------

//const float hipz = 8.0f;
// WALK -----

//                       Contact           Down               Pass               Up      

__DEVICE__ float3 shoulder1, elbow1, wrist1, head,
                  shoulder2, elbow2, wrist2;
__DEVICE__ float3 foot1, ankle1, knee1, hip1,
                  foot2, ankle2, knee2, hip2;
__DEVICE__ float3 v2Foot1, v2Foot12, v2Foot2, v2Foot22, v3Foot1, v3Foot2;
__DEVICE__ float3 v1Hand1, v2Hand1, v3Hand1,
                  v1Hand2, v2Hand2, v3Hand2;




//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

__DEVICE__ float hash( float n ) { return fract(_sinf(n)*43758.5453123f); }

// mix noise for alive animation, full source
__DEVICE__ float4 hash4( float4 n ) { return fract_f4(sin_f4(n)*1399763.5453123f); }
__DEVICE__ float3 hash3( float3 n ) { return fract_f3(sin_f3(n)*1399763.5453123f); }



//---------------------------------------------------------------------
//    Geometry
//---------------------------------------------------------------------

// Distance from ray to point
__DEVICE__ float distanceRayPoint(float3 ro, float3 rd, float3 p, out float h) {
    h = dot(p-ro,rd);
    return length(p-ro-rd*h);
}


//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] https://iquilezles.org/articles/distfunctions
//---------------------------------------------------------------------

__DEVICE__ bool cube(float3 ro, float3 rd, float3 sz, out float tn, out float tf) { //, out float3 n) {
  float3 m = 1.0f/rd,
         k = abs_f3(m)*sz,
         a = -m*ro-k*0.5f, 
         b = a+k;
//  n = -sign(rd)*step(swi3(a,y,z,x),a)*step(swi3(b,z,x,y),b);
    tn = _fmaxf(max(a.x,a.y),a.z);
    tf = _fminf(min(b.x,b.y),b.z); 
  return /*tn>0.0f &&*/ tn<tf;
}


__DEVICE__ float sdCap(in float3 p, in float3 a, in float3 b, in float r ) {
    float3 pa = p - a, ba = b - a;
    return length(pa - ba*clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f ) ) - r;
}

__DEVICE__ float sdCapsule(in float3 p, in float3 a, in float3 b, in float r0, in float r1 ) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f);
    return length( pa - ba*h ) - _mix(r0,r1,h);
}

__DEVICE__ float sdCap2(in float3 p, in float3 a, in float3 b, in float r1, in float r2) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length(pa - ba*h) - _mix(r1,r2,h*h*h);
}

__DEVICE__ float udRoundBox(in float3 p, in float3 b, in float r ) {
  return length(_fmaxf(abs_f3(p)-b, to_float3_s(0.0f)))-r;
}

//https://www.shadertoy.com/view/Xs3GRB
__DEVICE__ float fCylinder(in float3 p, in float r, in float height) {
  return _fmaxf(length(swi2(p,x,z)) - r, _fabs(p.y) - height);
}

__DEVICE__ float sdPlane(in float3 p, in float3 n) {  // n must be normalized
  return dot(p,n);
}

__DEVICE__ float smin(in float a, in float b, in float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

// Smooth maximum, based on the function above.
__DEVICE__ float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5f + 0.5f*(a-b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f-h)*s;
}

// hg_sdf : http://mercury.sexy/hg_sdf/
__DEVICE__ float fOpEngrave(float a, float b, float r) {
  return _fmaxf(a, (a + r - _fabs(b))*0.7071f); //_sqrtf(0.5f));
}

__DEVICE__ float fOpIntersectionRound(float a, float b, float r) {
  float2 u = _fmaxf(to_float2(r + a,r + b), to_float2_s(0));
  return _fminf(-r, _fmaxf (a, b)) + length(u);
}


__DEVICE__ float sdEllipsoid( in float3 p, in float3 r) {
    return (length(p/r ) - 1.0f) * _fminf(min(r.x,r.y),r.z);
}


__DEVICE__ float fOpDifferenceRound (float a, float b, float r) {
  return fOpIntersectionRound(a, -b, r);
}


//---------------------------------------------------------------------
//    Man + Ground distance field 
//---------------------------------------------------------------------

// The canyon, complete with hills, gorges and tunnels. I would have liked to provide a far
// more interesting scene, but had to keep things simple in order to accommodate slower machines.
__DEVICE__ float mapGround(in float3 p, __TEXTURE2D__ iChannel1){
    float tx = 0.2f*(_cosf(p.x*0.03f))*(texture(iChannel1, swi2(p,x,z)/16.0f + swi2(p,x,y)/80.0f).x);
    float3 q = p*0.25f;
#ifdef IS_RUNNING
    float h = tx + 0.5f*(dot(sin_f3(q)*cos_f3(swi3(q,y,z,x)), to_float3_s(0.222f))) + dot(sin_f3(q*1.3f)*cos_f3(swi3(q,y,z,x)*1.4f), to_float3_s(0.111f));
    float d = p.y + smin(0.0f,smoothstep(0.2f,3.0f, _fabs(p.z))*(h)*9.0f,0.2f);
#else
    float h = tx + 0.5f*(dot(sin_f3(q)*cos_f3(swi3(q,y,z,x)), to_float3_s(0.222f))) + dot(sin_f3(q*1.3f)*cos_f3(swi3(q,y,z,x)*1.4f), to_float3_s(0.111f)) - 0.5f*(texture(iChannel1, swi2(q,x,z)/80.0f + swi2(q,x,z)/102.0f).x)
        - 0.5f*smoothstep(18.0f, 0.0f, length(swi2(p,x,z)-swi2(gStaticPos,x,z)))
        + 0.5f*smoothstep(130.0f, 0.0f, length(swi2(p,x,z)- swi2(gStaticPos,x,z) - to_float2(120,50)));
  float d = p.y + smin(0.0f, h*9.0f, 0.2f);
#endif
    return d; 
}

// capsule with bump in the middle -> use for neck
__DEVICE__ float sdCapsule2(in float3 p,in float3 a,in float3 b, in float r0,in float r1,in float bump) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    float dd = bump*_sinf(3.14f*h);  // Little adaptation
    return length(pa - ba*h) - _mix(r0,r1,h)*(1.0f+dd); 
}


__DEVICE__ const float3 g_nozePos = to_float3(0,-0.28f+0.04f,0.47f+0.08f);
__DEVICE__ const float3 g_eyePos = to_float3(0.14f,-0.14f,0.29f);
__DEVICE__ const float g_eyeSize = 0.09f;
__DEVICE__ mat2   g_eyeRot;
__DEVICE__ const mat2 ma = rot(-0.5f);
__DEVICE__ const mat2 mb = rot(-0.15f);
__DEVICE__ const mat2 mc = rot(-0.6f);

__DEVICE__ float smax(in float a, in float b, in float k) {
    return _logf(exp(a/k)+_expf(b/k))*k;
}


__DEVICE__ float mapHead(in float3 p0) {
    float3 p = p0;
    float d = MAX_DIST;
   
// Skull modeling -------------------------
    d = sdEllipsoid(p-to_float3(0,0.05f,0.0f), to_float3(0.39f,0.48f,0.46f));          
    d = smin(d, sdEllipsoid(p-to_float3(0.0f,0.1f,-0.15f), to_float3(0.42f,0.4f,0.4f)),0.1f);     
    d = smin(d, udRoundBox(p-to_float3(0,-0.28f,0.2f), to_float3(0.07f,0.05f,0.05f),0.05f),0.4f); // Basic jaw 

// Neck -----------------------------------
 //   d = smin(d, dNeck, 0.05f);

// Symetrie -------------------------------
    p.x = _fabs(p.x);

// Eye hole 
    d = smax(d, -sdEllipsoid(p-to_float3(0.12f,-0.16f,0.48f), to_float3(0.09f,0.06f,0.09f)), 0.07f);

// Noze ------------------------------------
    const float animNoze = 0.0f;
    d = smin(d, _fmaxf(-(length(p-to_float3(0.032f,-0.325f,0.45f))-0.028f),   // Noze hole
                    smin(length(p-to_float3(0.043f,-0.29f+0.015f*animNoze,0.434f))-0.01f,  // Nostrils
                    sdCapsule(p, to_float3(0,-0.13f,0.39f), to_float3(0,-0.28f+0.008f*animNoze,0.47f), 0.01f,0.04f), 0.05f)) // Bridge of the nose
                    ,0.065f); 
   
// Mouth -----------------------------------    
    d = smin(d, length(p- to_float3(0.22f,-0.34f,0.08f)), 0.17f); // Jaw
    d = smin(d, sdCapsule(p, to_float3(0.16f,-0.35f,0.2f), to_float3(-0.16f,-0.35f,0.2f), 0.06f,0.06f), 0.15f); // Cheeks
   
    d = smin(d, _fmaxf(-length(swi2(p,x,z)-to_float2(0,0.427f))+0.015f,    // Line under the noze
                _fmaxf(-p.y-0.41f+0.008f*animNoze,               // Upper lip
                sdEllipsoid(p- to_float3(0,-0.34f,0.37f), to_float3(0.08f,0.15f,0.05f)))), // Mouth bump
                0.032f);

// Chin -----------------------------------  
    d = smin(d, length(p- to_float3(0,-0.5f,0.26f)), 0.2f);   // Chin
    d = smin(d, length(p- to_float3(0,-0.44f,0.15f)), 0.25f); // Under chin 
  
// Eyelid ---------------------------------
    float3 p_eye1 = p - g_eyePos;
    swi2S(p_eye1,x,z, mul_f2_mat2(swi2(p_eye1,x,z) , mb));
    
    float3 p_eye2 = p_eye1;
    float d_eye = length(p_eye1) - g_eyeSize*1.0f;
          
    swi2S(p_eye1,y,z, mul_f2_mat2(swi2(p_eye1,y,z) , g_eyeRot));
    swi2S(p_eye2,z,y, mul_f2_mat2(swi2(p_eye2,z,y) , mc));
    
    float d1 = _fminf(max(-p_eye1.y,d_eye - 0.01f),
               _fmaxf(p_eye2.y,d_eye - 0.005f));

    d = smin(d,d1,0.01f);
  return d; 
}

__DEVICE__ float mapLegs(const in float3 pos){    
    // Leg 1
    float d = _fmaxf(_fminf(sdCap2(pos, foot1, ankle1, 0.1f,0.15f),
                      sdCap2(pos, ankle1, knee1, 0.165f,0.105f)),
                      -sdPlane(pos-ankle1+v2Foot1*0.1f, v2Foot12));
    // Leg 2
    d = _fminf(d,_fmaxf(_fminf(sdCap2(pos, foot2, ankle2, 0.1f,0.15f),
                      sdCap2(pos, ankle2, knee2, 0.165f,0.105f)),
                      -sdPlane(pos-ankle2+v2Foot2*0.1f, v2Foot22)));
 
    d = fOpEngrave(d, _fminf(_fminf(sdCap(pos-ankle1, -0.1f*v3Foot1, 0.1f*v3Foot1, 0.12f), 
                                    sdCap(pos-ankle2, -0.1f*v3Foot2, 0.1f*v3Foot2, 0.12f)), 
                             _fminf(length(pos - knee1),length(pos - knee2))-0.11f), 0.015f);
    
    d = _fminf(d, sdCap2(pos,  hip1, knee1, 0.12f, 0.075f));
    return _fminf(d, sdCap2(pos, hip2, knee2, 0.12f, 0.075f));
}

__DEVICE__ float mapGirl(const in float3 pos){
    float3 
         ep0 = _mix(shoulder1,shoulder2,0.5f),
         ha0 = _mix(hip1,hip2,0.5f),
         h1 = head + to_float3(0.0f,-0.24f-0.05f,0),
         h2 = head + to_float3(0.0f,0.15f-0.05f,0),
         hn = normalize(h1-h2),
         a = _mix(ha0,ep0,0.15f), b = _mix(ha0,ep0,0.75f);
    
    float3 posRot = pos; // - head; 
    swi2S(posRot,x,z, (swi2(posRot,x,z) - swi2(head,x,z)) * rotHead + swi2(head,x,z));
   // swi2S(posRot,x,z, (swi2(posRot,x,z)) * rotHead);
    
    float d = mapLegs(pos);

    // Head
    float scaleHead = 1.75f;
    float3 pHead = (posRot - head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = _fminf(d, dHead);

  // Eye
    pHead.x = _fabs(pHead.x);
    float3 p_eye = pHead-g_eyePos;
    swi2S(p_eye,x,z, mul_f2_mat2(swi2(p_eye,x,z) , ma));  
    float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    d = _fminf(d, dEye);
        
  // Arms
    d = _fminf(d, sdCapsule2(pos, shoulder1, elbow1, 0.054f,0.051f,0.5f));
    d = _fminf(d, sdCapsule2(pos, shoulder2, elbow2, 0.054f,0.051f,0.5f));
    float dArm = sdCap2(pos, elbow1, wrist1-0.01f*v1Hand1, 0.09f, 0.055f);
    dArm = _fminf(dArm,  sdCap2(pos, elbow2, wrist2-0.01f*v1Hand2, 0.09f, 0.055f));
    dArm = fOpEngrave(dArm, 
                      _fminf(_fminf(length(pos - wrist2), length(pos - wrist1)),
                             _fminf(length(pos - elbow2), length(pos - elbow1))) - 0.1f,0.008f);

    
  // Neck and Shoulders
    d = smin(d, _fminf(sdCapsule2(pos+to_float3(0.03f,0,0), _mix(shoulder1, shoulder2,0.1f),_mix(shoulder1, shoulder2,0.9f),0.08f, 0.08f,0.6f),
                       sdCap(pos,ep0-to_float3(0.03f,0,0), head-to_float3(0.08f,0.1f,0), 0.09f)), 0.12f);
    
  // Torso
    d = smin(d, _fminf(sdCap2(pos, a+to_float3(0,0,0.03f), b-to_float3(0,0,0.04f), 0.19f,0.22f),sdCap2(pos, a-to_float3(0,0,0.03f), b+to_float3(0,0,0.04f), 0.19f,0.22f)),0.18f);
    
  // Fingers 1
    float3 c = wrist1-v3Hand1*0.03f;
    float d2 = sdCap(pos, c-v1Hand1*0.06f+v2Hand1*0.03f+v3Hand1*0.06f, wrist1+0.09f*(v2Hand1+v1Hand1+v3Hand1), 0.02f);
    d2 = _fminf(d2, sdCap(pos, c, wrist1+0.18f*(v1Hand1+v2Hand1*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.01f, wrist1+0.2f*(v1Hand1-v2Hand1*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.02f, wrist1+0.18f*(v1Hand1-v2Hand1*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.04f, wrist1+0.15f*(v1Hand1-v2Hand1*0.8f), 0.014f));
    
  // Fingers 2     
    c = wrist2-v3Hand2*0.03f;
    d2 = _fminf(d2, sdCap(pos, c-v1Hand2*0.06f+v2Hand2*0.03f+v3Hand2*0.06f, wrist2+0.09f*(v2Hand2+v1Hand2+v3Hand2), 0.02f));
    d2 = _fminf(d2, sdCap(pos, c, wrist2+0.18f*(v1Hand2+v2Hand2*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.01f, wrist2+0.2f*(v1Hand2-v2Hand2*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.02f, wrist2+0.18f*(v1Hand2-v2Hand2*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.04f, wrist2+0.15f*(v1Hand2-v2Hand2*0.8f), 0.014f));


    d = _fminf(d, smin(d2, dArm, 0.12f));
   
    // Short
    float dShort = _fminf(sdCap(pos, hip1+to_float3(-0.03f,0,0), _mix(hip1, knee1,0.25f), 0.13f), 
                          sdCap(pos, hip2+to_float3(-0.03f,0,0), _mix(hip2, knee2,0.25f), 0.13f));                    
    // TODO ca serait plus cool de bouger avec les hanches mais il faut recuperer une base fixee sur les hanches
    dShort = smin(dShort, _mix(d, sdCap(pos, a, ha0+to_float3(0,0.1f,0), 0.22f),0.5f),0.1f);
    d = _fminf(d, dShort);

     // Casque
    float dHelmet;
    posRot += to_float3(0.03f,0.02f,0);
    dHelmet = _fmaxf(sdPlane(posRot-h1+hn*0.07f, hn), _mix(dHead, sdCap2(posRot, h1-to_float3(0.23f,0,0), h2-to_float3(0,0.05f,0),0.28f,0.36f),0.5f));
    dHelmet = _fmaxf(-fCylinder(posRot-h1-to_float3(0.2f,0,0), 0.18f,0.3f), dHelmet); 

    dHelmet = fOpEngrave(dHelmet, sdCap(posRot-h2, -to_float3(0,0.1f,1), -to_float3(0,0.1f,-1), 0.1f),0.015f);
   
  d = _fminf(d, dHelmet);
//  d = _fminf(d, mapGround(pos,iChannel1));
            
    return _fminf(d2,d);
}


//---------------------------------------------------------------------
//    Girl colors 
//---------------------------------------------------------------------
#define min2(a, b) (a.x<b.x?a:b)
#define max2(a, b) (a.x>b.x?a:b)

#define ID_MAN 100.0f
#define ID_GROUND 90.0f
#define ID_GLOVE 106.0f 
#define ID_HELMET 107.0f
#define ID_FOOT 108.0f
#define ID_SHORT 110.0f
#define ID_LEG  201.0f
#define ID_SKIN 202.0f
#define ID_ARM  203.0f
#define ID_TORSO 204.0f
#define ID_EYE 205.0f

__DEVICE__ const float3 COLOR_SKIN = to_float3(0.6f,0.43f,0.3f);
__DEVICE__ const float3 COLOR_ARMOR = to_float3(0.14f,0.79f,0.7f);
__DEVICE__ const float3 COLOR_CLOTHES2 = to_float3(0.14f,0.79f,0.7f);
__DEVICE__ const float3 COLOR_CLOTHES = to_float3(0.66f,0.94f,0.91f);
 



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
#define sat(x) clamp(x, 0.0f, 1.0f)

__DEVICE__ float remap01(float a, float b, float t) {
  return sat((t-a)/(b-a));
}

__DEVICE__ float remap(float a, float b, float c, float d, float t) {
  return sat((t-a)/(b-a)) * (d-c) + c;
}

__DEVICE__ float2 within(float2 uv, float4 rect) {
  return (uv-swi2(rect,x,y))/(swi2(rect,z,w)-swi2(rect,x,y));
}

__DEVICE__ float4 Brow(float2 uv, float smile) {
    float offs = _mix(0.2f, 0.0f, smile);
    uv.y += offs;
    
    float y = uv.y;
    uv.y += uv.x*_mix(0.5f, 0.8f, smile)-_mix(0.1f, 0.3f, smile);
    uv.x -= _mix(0.0f, 0.1f, smile);
    uv -= 0.5f;
    
    float4 col = to_float4_s(0.0f);
    
    float blur = 0.1f;
    
     float d1 = length(uv);
    float s1 = S(0.45f, 0.45f-blur, d1);
    float d2 = length(uv-to_float2(0.1f, -0.2f)*0.7f);
    float s2 = S(0.5f, 0.5f-blur, d2);
    
    float browMask = sat(s1-s2);
    
    float colMask = remap01(0.7f, 0.8f, y)*0.75f;
    colMask *= S(0.6f, 0.9f, browMask);
    colMask *= smile;
    float4 browCol = to_float4(0.04f, 0.02f, 0.02f, 1.0f); //_mix(to_float4(0.4f, 0.2f, 0.2f, 1.0f), to_float4(1.0f, 0.75f, 0.5f, 1.0f), colMask); 
   
    uv.y += 0.15f-offs*0.5f;
    blur += _mix(0.0f, 0.1f, smile);
    d1 = length(uv);
    s1 = S(0.45f, 0.45f-blur, d1);
    d2 = length(uv-to_float2(0.1f, -0.2f)*0.7f);
    s2 = S(0.5f, 0.5f-blur, d2);
    float shadowMask = sat(s1-s2);
    
    col = _mix(col, to_float4(0.0f,0.0f,0.0f,1.0f), S(0.0f, 1.0f, shadowMask)*0.5f);
    col = _mix(col, browCol, S(0.2f, 0.4f, browMask));
    
    return col;
}

__DEVICE__ float4 Mouth(float2 uv, float smile) {
    uv -= 0.5f;
    float4 col = to_float4(0.5f, 0.18f, 0.05f, 1.0f);
    
    uv.y *= 1.5f;
    uv.y -= uv.x*uv.x*2.0f*smile;
    
    uv.x *= _mix(2.5f, 1.0f, smile);
float aaaaaaaaaaaaaaaaaaa;    
    float d = length(uv);
    col.w = S(0.5f, 0.48f, d);
    
    float2 tUv = uv;
    tUv.y += (_fabs(uv.x)*0.5f+0.1f)*(1.0f-smile);
    float td = length(tUv-to_float2(0.0f, 0.6f));
    
    float3 toothCol = to_float3_s(1.0f)*S(0.6f, 0.35f, d);
    swi3S(col,x,y,z, _mix(swi3(col,x,y,z), toothCol, S(0.4f, 0.37f, td)));
    
    td = length(uv+to_float2(0.0f, 0.5f));
    swi3S(col,x,y,z, _mix(swi3(col,x,y,z), to_float3(1.0f, 0.5f, 0.5f), S(0.5f, 0.2f, td)));
    return col;
}

__DEVICE__ float4 drawFace(float3 pos) {
  pos -= head;
  pos.y += 0.05f;
  swi2S(pos,x,z, mul_f2_mat2(swi2(pos,x,z) , rotHead));
  float4 col = to_float4_aw(COLOR_SKIN, ID_SKIN);
  if (pos.x < 0.0f) return col;
  float2 uv = swi2(pos,z,y)*2.8f;
  float side = sign_f(uv.x);
  uv.x = _fabs(uv.x);
  float d = length(uv-to_float2(0.28f, -0.42f));
  float cheek = S(0.2f,0.01f, d)*0.4f;
  cheek *= S(0.17f, 0.16f, d);
  swi3S(col,x,y,z, _mix(swi3(col,x,y,z), to_float3(1.0f, 0.1f, 0.1f), cheek));
  float4 mouth = Mouth(within(uv, to_float4(-0.27f, -0.72f, 0.27f, -0.60f)), 0.5f);
  col = _mix(col, mouth, mouth.w);
  float4 brow = Brow(within(uv, to_float4(0.06f, -0.14f, 0.51f, 0.06f)), 0.0f);
  col = _mix(col, brow, brow.w);
  return to_float4_aw(swi3(col,x,y,z), ID_SKIN);
}



__DEVICE__ float4 drawEye(in float3 p,float iTime) {
    float3 posRot = p; // - head; 
    swi2S(posRot,x,z, (swi2(posRot,x,z) - mul_f2_mat2(swi2(head,x,z)) , rotHead) + swi2(head,x,z));
    
    float scaleHead = 1.75f;
    float3 pHead = (posRot-head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;

  // Eye
    float3 p_eye = pHead-g_eyePos;
    float3 g_eyePosloc = g_eyePos;
    g_eyePosloc.x *= sign_f(pHead.x);
    float3 pe = pHead - g_eyePosloc;

   
    float a = 0.2f*_sinf(2.0f*iTime)*_cosf(0.01f*iTime);//clamp(_atan2f(-dir.x, dir.z), -0.6f,0.6f), 
    float ca = _cosf(a), sa = _sinf(a);
    swi2S(pe,x,z, mul_f2_mat2(swi2(pe,x,z) , to_mat2(ca, sa, -sa, ca)));

    float b = 0.2f;//0.1f+0.1f*_sinf(iTime*0.1f);//clamp(_atan2f(-dir.y, dir.z), -0.3f,0.3f), 
    float cb = _cosf(b), sb = _sinf(b);
    swi2S(pe,y,z, mul_f2_mat2(swi2(pe,y,z) , to_mat2(cb, sb, -sb, cb)));
    
    float d = length(swi2(pe,x,y));
    float3 col = _mix(to_float3_s(0), _mix(to_float3(0.88f,0.41f,0.0f), _mix(to_float3_s(0),to_float3_s(1.5f),
                   0.5f+0.5f*smoothstep(0.0405f,0.0415f,d)), smoothstep(0.04f,0.041f,d)), smoothstep(0.02f,0.025f,d));

    return to_float4_aw(col,ID_EYE);
}

__DEVICE__ float sdFish(float3 o) {
    float2 p = swi2((o - (shoulder1 + shoulder2)*0.5f),z,y) + to_float2(0.04f,0.17f);
    p *= 2.0f;
      
    float dsub = _fminf(length(p-to_float2(0.8f,0.0f)) - 0.45f, length(p-to_float2(-0.14f,0.05f)) - 0.11f);  
    p.y = _fabs(p.y);
    float d = length(p-to_float2(0.0f,-0.15f)) - 0.3f;
    d = _fminf(d, length(p-to_float2(0.56f,-0.15f)) - 0.3f);
    d = _fmaxf(d, -dsub);
    return (1.0f-smoothstep(0.05f,0.06f,d));
}

#endif    

// -----------------------------------------


__DEVICE__ float4 getColor(float id, float3 pos,float iTime) {
    //return id != ID_TORSO && id <= ID_SHORT ? to_float4(0.3f,0.3f,0.7f,id) : to_float4(0.5f,0.5f,1,id);
  return   0.3f+0.7f*(
      id == ID_LEG ? to_float4_aw(COLOR_CLOTHES, ID_LEG) :
#ifdef WITH_FACE        
      id == ID_EYE ? drawEye(pos,iTime) :
      id == ID_SKIN ?  drawFace(pos) : 
      id == ID_TORSO ? to_float4_aw(_mix(COLOR_CLOTHES,to_float3_s(0),sdFish(pos)), ID_TORSO) :
#else                  
           id == ID_EYE ?  to_float4(1,1,1, ID_EYE) :
           id == ID_SKIN ?  to_float4_aw(COLOR_SKIN, ID_SKIN) :
      id == ID_TORSO ? to_float4_aw(COLOR_CLOTHES, ID_TORSO) :
#endif
      id == ID_ARM ? to_float4_aw(COLOR_SKIN, ID_ARM) :
      id == ID_SHORT ? to_float4_aw(COLOR_CLOTHES2, ID_SHORT) :
//          id == ID_FOOT ? to_float4_aw(COLOR_ARMOR, id) :
//      id == ID_GLOVE ? to_float4_aw(COLOR_ARMOR, ID_GLOVE) :
//      id == ID_HELMET ? to_float4_aw(COLOR_ARMOR, ID_HELMET) :
//      id == ID_GROUND ? to_float4(textureLod(iChannel2, swi2(pos,x,z)*0.1f,1.0f).rgb, ID_GROUND) :
        to_float4_aw(COLOR_ARMOR, id));
}


__DEVICE__ float mapColor(in float3 pos) {
    float3 
         ep0 = _mix(shoulder1,shoulder2,0.5f),
         ha0 = _mix(hip1,hip2,0.5f),
         h1 = head + to_float3(0.0f,-0.29f,0),
         h2 = head + to_float3(0.0f,0.1f,0),
         hn = to_float3(0,-1,0),//normalize(h1-h2),
         a = _mix(ha0,ep0,0.15f), b = _mix(ha0,ep0,0.79f);
    float3 posRot = pos; 
    swi2S(posRot,x,z, (swi2(posRot,x,z) - mul_f2_mat2(swi2(h1,x,z)) , rotHead) + swi2(h1,x,z));
    
    // Leg 1
    float d = _fmaxf(min(sdCap2(pos, foot1, ankle1, 0.1f,0.15f),
                      sdCap2(pos, ankle1, knee1, 0.165f,0.105f)),
                      -sdPlane(pos-ankle1+v2Foot1*0.1f, v2Foot12));
  // Leg 2
    d = _fminf(d,_fmaxf(_fminf(sdCap2(pos, foot2, ankle2, 0.1f,0.15f),
                      sdCap2(pos, ankle2, knee2, 0.165f,0.105f)),
                     -sdPlane(pos-ankle2+v2Foot2*0.1f, v2Foot22)));                          
    
    float2 dd;
    dd = min2(to_float2(d, ID_FOOT), 
              to_float2(_fminf(sdCap2(pos, hip1, knee1, 0.12f, 0.075f),
                               sdCap2(pos, hip2,  knee2, 0.12f, 0.075f)), ID_LEG));

  // Head
    float scaleHead = 1.75f;
    float3 pHead = (posRot - head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
  // d = _fminf(d, dHead);

  // Eye
    pHead.x = _fabs(pHead.x);
    float3 p_eye = pHead-g_eyePos;
    swi2S(p_eye,x,z, mul_f2_mat2(swi2(p_eye,x,z) , ma));  
    
    float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    dd = min2(dd, to_float2(dEye,ID_EYE));

    dd = min2(dd, to_float2(_fminf(dHead,             
                             sdCap(pos, ep0-to_float3(0.03f,0,0), head-to_float3(0.08f,0.1f,0), 0.1f)), ID_SKIN));  // neck           
    
  // Arms
    dd = min2(dd, to_float2(_fminf(sdCap2(pos, shoulder1, elbow1, 0.054f,0.051f), sdCap2(pos, shoulder2, elbow2, 0.054f,0.051f)), ID_LEG));
    
  // Fingers 1
    float3 c = wrist1-v3Hand1*0.03f;
    float d2 = sdCap(pos, c-v1Hand1*0.06f+v2Hand1*0.03f+v3Hand1*0.06f, wrist1+0.09f*(v2Hand1+v1Hand1+v3Hand1), 0.02f);
    d2 = _fminf(d2, sdCap(pos, c, wrist1+0.18f*(v1Hand1+v2Hand1*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.01f, wrist1+0.2f*(v1Hand1-v2Hand1*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.02f, wrist1+0.18f*(v1Hand1-v2Hand1*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.04f, wrist1+0.15f*(v1Hand1-v2Hand1*0.8f), 0.014f));
    
  // Fingers 2     
    c = wrist2-v3Hand2*0.03f;
    d2 = _fminf(d2, sdCap(pos, c-v1Hand2*0.06f+v2Hand2*0.03f+v3Hand2*0.06f, wrist2+0.09f*(v2Hand2+v1Hand2+v3Hand2), 0.02f));
    d2 = _fminf(d2, sdCap(pos, c, wrist2+0.18f*(v1Hand2+v2Hand2*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.01f, wrist2+0.2f*(v1Hand2-v2Hand2*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.02f, wrist2+0.18f*(v1Hand2-v2Hand2*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.04f, wrist2+0.15f*(v1Hand2-v2Hand2*0.8f), 0.014f));

    d2 = _fminf(d2, _fminf(sdCap2(pos, elbow1, wrist1-0.01f*v1Hand1, 0.09f, 0.055f),  
                     sdCap2(pos, elbow2, wrist2-0.01f*v1Hand2, 0.09f, 0.055f)));
    dd = min2(dd, to_float2(d2, ID_GLOVE));
  
    
    
    
    
  // Torso
    dd = min2(dd, to_float2(smin(sdCapsule2(pos+to_float3(0.03f,0,0), _mix(shoulder1, shoulder2,0.1f),_mix(shoulder1, shoulder2,0.9f),0.08f, 0.08f,0.6f), 
                           _fminf(sdCap2(pos, a+to_float3(0,0,0.03f), b-to_float3(0,0,0.04f), 0.19f,0.22f),sdCap2(pos, a-to_float3(0,0,0.03f), b+to_float3(0,0,0.04f), 0.19f,0.22f)),0.18f), ID_TORSO));
  
  // Short
    float dShort = _fminf(sdCap(pos, hip1, _mix(hip1,knee1,0.3f), 0.12f), 
                          sdCap(pos, hip2, _mix(hip2,knee2,0.3f), 0.12f));                    
    dd = min2(dd, to_float2(_fminf(dShort, _mix(dd.x, sdCap(pos, a, ha0, 0.22f),0.75f)), ID_SHORT));

  // Casque
    float dHelmet;
    posRot += to_float3(0.03f,0.02f,0);
    dHelmet = _fmaxf(sdPlane(posRot-h1+hn*0.08f, hn), _mix(dHead, sdCap2(posRot, h1-to_float3(0.23f,0,0), head+to_float3(0,0.05f,0),0.28f,0.36f),0.5f));
    dHelmet = _fmaxf(-fCylinder(posRot-h1-to_float3(0.2f,0,0), 0.18f,0.3f), dHelmet); 
    dd = min2(dd, to_float2(dHelmet, ID_HELMET));

    return dd.y;
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------
              
__DEVICE__ float logBisectTrace(in float3 ro, in float3 rd, __TEXTURE2D__ iChannel1){

    float t = 0.0f, told = 0.0f, mid, dn;
    float d = mapGround(rd*t + ro, iChannel1);
    float sgn = sign_f(d);
#ifdef IS_RUNNING
    for (int i=0; i<64; i++){
#else        
    for (int i=0; i<92; i++){
#endif
        // If the threshold is crossed with no detection, use the bisection method.
        // Also, break for the usual reasons. Note that there's only one "break"
        // statement in the loop. I heard GPUs like that... but who knows?
        if (sign_f(d) != sgn || d < 0.001f || t > MAX_DIST) break;
        told = t;
        // Branchless version of the following:      
        t += step(d, 1.0f)*(_logf(_fabs(d) + 1.1f) - d) + d;
        //t += _logf(_fabs(d) + 1.1f);
        //t += d;//step(-1.0f, -d)*(d - d*0.5f) + d*0.5f;
        d = mapGround(rd*t + ro, iChannel1);
    }
    // If a threshold was crossed without a solution, use the bisection method.
    if (sign(d) != sgn){
        // Based on suggestions from CeeJayDK, with some minor changes.
        dn = sign(mapGround(rd*told + ro, iChannel1));
        float2 iv = to_float2(told, t); // Near, Far
        // 6 iterations seems to be more than enough, for most cases...
        // but there's an early exit, so I've added a couple more.
        for (int ii=0; ii<8; ii++){ 
            //Evaluate midpoint
            mid = dot(iv, to_float2_s(0.5f));
            float d = mapGround(rd*mid + ro,iChannel1);
            if (_fabs(d) < 0.001f)break;
            // Suggestion from movAX13h - Shadertoy is one of those rare
            // sites with helpful commenters. :)
            // Set mid to near or far, depending on which side we're on.
            iv = _mix(to_float2(iv.x, mid), to_float2(mid, iv.y), step(0.0f, d*dn));
        }

        t = mid; 
        
    }
    
    //if (_fabs(d) < PRECISION) t += d;

    return _fminf(t, MAX_DIST);
}

__DEVICE__ float2 Trace(in float3 pos, in float3 ray, in float start, in float end, __TEXTURE2D__ iChannel1 ) {
    // Trace if in bbox
    float t=start, h, tn=start, tf=end;
    float tGround = logBisectTrace(pos, ray, iChannel1);

   // start = _fmaxf(start, );
    end = _fminf(tGround, end);
    
    if (cube(pos-head-to_float3(0.1f,-1.0f,0), ray, to_float3(1.2f, 1.7f,0.7f)*2.0f,  tn, tf)) {
        end = _fminf(tf, end);
        t = _fmaxf(tn, start);// - 0.3f*hash33(pos+ray).x;
        for( int i=0; i < g_traceLimit; i++) {
          if (t > end) break;
            h = mapGirl( pos+t*ray );
            if (h < g_traceSize) {
                return to_float2(t+h, mapColor(pos+t*ray));
            }
            t += h;
        }
        if (t < end) return to_float2(t, mapColor(pos+t*ray)); 
    } 
    
    return tGround < MAX_DIST ?  to_float2(tGround, ID_GROUND) : to_float2(MAX_DIST, 0.0f);
}

//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){
    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= (n.x + n.y + n.z );  
  return swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
}

__DEVICE__ float getGrey(float3 p){ return dot(p, to_float3(0.299f, 0.587f, 0.114f)); }

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 nor, float bumpfactor){
    const float eps = 0.001f;
    float3 grad = to_float3( getGrey(tex3D(tex, to_float3(p.x-eps, p.y, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y-eps, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y, p.z-eps), nor)));
    grad = (grad - getGrey(tex3D(tex,  p , nor)))/eps; 
    grad -= nor*dot(nor, grad);                  
    return normalize( nor + grad*bumpfactor );
}


//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

#ifdef WITH_SHADOW
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax ) {
  float h, res = 1.0f, t = mint;
    for(int i=0; i<24; i++) {
    h = map( ro + rd*t );
        res = _fminf( res, 8.0f*h/t );
        t += clamp( h, 0.05f, 0.2f );
        if( h<0.01f || t>tmax ) break;
    }
    return clamp(res, 0.0f, 1.0f);
}
#endif

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
__DEVICE__ float calcAO( in float3 pos, in float3 nor ){
  float dd, hr, sca = 1.0f, totao = 0.0f;
    float3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = 0.01f + 0.05f*(float)(aoi);
        aopos =  nor * hr + pos;
        totao += -(mapGirl( aopos )-hr)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - 4.0f*totao, 0.0f, 1.0f);
}
__DEVICE__ float calcAOGround( in float3 pos, in float3 nor, __TEXTURE2D__ iChannel1 ){
  float dd, hr, sca = 1.0f, totao = 0.0f;
    float3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = 0.01f + 0.05f*(float)(aoi);
        aopos =  nor * hr + pos;
        totao += -(_fminf(mapGround(aopos, iChannel1),mapLegs(aopos))-hr)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - 4.0f*totao, 0.0f, 1.0f);
}
#endif


//---------------------------------------------------------------------
//   Shading
//   Adapted from Shane / Iq
//---------------------------------------------------------------------

__DEVICE__ float3 shading( in float3 sp, in float3 rd, in float3 sn, in float3 col, in float id, out float *reflexion, __TEXTURE2D__ iChannel1){
    
    float3 general = to_float3(240,198,157)/256.0f,
       back = to_float3(63,56,46)/256.0f;
    float3 ref = reflect( rd, sn );
float bbbbbbbbbbbbbbbbbbbbb;
    // lighitng   
#ifdef WITH_AO
    float occ = id == ID_GROUND ? calcAOGround( sp, sn. iChannel1 ) : calcAO( sp, sn );
#else
    float occ = 1.0f;
#endif
    float3  ld = normalize( gLightPos );
    float3  hal = normalize( rd - ld);
    float amb = 0.15f; //clamp( 0.5f+0.5f*sn.y, 0.0f, 1.0f );
    float dif = clamp( dot( sn, ld ), 0.0f, 1.0f );
    float bac = clamp( dot( sn, normalize(to_float3(-ld.x,0.0f,-ld.z))), 0.0f, 1.0f );//*clamp( 1.0f-sp.y,0.0f,1.0f);
    float dom = smoothstep( -0.1f, 0.1f, ref.y );
    float fre = _powf( clamp(1.0f+dot(sn,rd),0.0f,1.0f), 2.0f );

    *reflexion = fre*occ;
    
#ifdef WITH_SHADOW
    dif *= calcSoftshadow( sp, ld, 0.05f, 2.0f );
#endif
    
    float spe =  _powf( clamp( dot( sn, -hal ), 0.0f, 1.0f ), id >= ID_SHORT ? 10.0f : 164.0f) * dif * (0.04f + 0.96f*_powf( clamp(1.0f+dot(hal,rd),0.0f,1.0f), 50.0f ));

    float3 lin = to_float3_s(0.0f);
    lin += 0.80f*dif*general/*to_float3(1.00f,0.80f,0.55f)*/*(0.3f+0.7f*occ);
    lin += 0.40f*amb*occ*general;//to_float3(0.40f,0.60f,1.00f);
   // lin += 0.15f*dom*occ*general;//to_float3(0.40f,0.60f,1.00f)*occ;
    lin += 0.15f*bac*back/*to_float3(0.25f,0.25f,0.25f)*/*occ;
   // lin += 0.25f*fre*to_float3(1.00f,1.00f,1.00f)*occ;
   
    col = col*lin;
    col += (id == ID_EYE ? 10.0f : id >= ID_SHORT ? 0.3f : 1.0f)*spe*to_float3(1.00f,0.90f,0.70f);
    
    return col;
}



//---------------------------------------------------------------------
//   Calculate normal
//   From TekF 
//---------------------------------------------------------------------
__DEVICE__ float3 Normal(in float3 pos, in float3 ray, in float t, float2 R) {
  float pitch = 0.1f * t / iResolution.x;   
  pitch = _fmaxf( pitch, 0.002f );
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x), // tetrahedral offsets
         p1 = pos+swi3(d,x,y,y),
         p2 = pos+swi3(d,y,x,y),
         p3 = pos+swi3(d,y,y,x);

  float f0 = mapGirl(p0), f1 = mapGirl(p1), f2 = mapGirl(p2),  f3 = mapGirl(p3);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  //   return normalize(grad);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}

__DEVICE__ float3 NormalGround(in float3 pos, in float3 ray, in float t, __TEXTURE2D__ iChannel1) {
  float pitch = 0.2f * t / iResolution.x;   
  pitch = _fmaxf( pitch, 0.005f );
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x), // tetrahedral offsets
       p1 = pos+swi3(d,x,y,y),
       p2 = pos+swi3(d,y,x,y),
       p3 = pos+swi3(d,y,y,x);

  float f0 = mapGround(p0,iChannel1), f1 = mapGround(p1,iChannel1), f2 = mapGround(p2,iChannel1), f3 = mapGround(p3, iChannel1);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  //   return normalize(grad);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}



//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

__DEVICE__ mat3 setCamera(in float3 ro, in float3 ta, in float cr) {
  float3 cw = normalize(ta-ro),
     cp = to_float3_aw(_sinf(cr), _cosf(cr), 0.0f),
     cu = normalize( cross(cw,cp) ),
     cv = normalize( cross(cu,cw) );
    return to_mat3_f3( cu, cv, cw );
}


//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------


// Interpolate pos of articulations
__DEVICE__ float3 getPos(float3 p, int it, float kt, float z, float dx) {
  return 0.02f*to_float3(p.x + dx, 150.0f-p.y, p.z*z);
}
    
    
__DEVICE__ void initLookingPosition(int idPlanet, float iTime) {
  const int it = 6;
  const float kt = 0.0f;

  float3 delta = gStaticPos;

  float dx = 0.0f;
  float mv = 0.5f*_cosf(3.0f*iTime);
    
        
  head = delta + getPos(to_float3(85+94,20,0), it, kt, 1.0f, dx);

  shoulder1 = delta +getPos(to_float3(85+91,43,16), it, kt, -1.0f, dx) + 0.02f*to_float3(-5,0,0);
  elbow1 = delta +getPos(to_float3(85+91,73,25), it, kt, -1.0f, dx);
  wrist1 = delta +getPos(to_float3(85+88,103,25), it, kt, -1.0f, dx) + 0.02f*to_float3(18,22,7);

  foot1 = delta +getPos(to_float3(182,150,10), it, kt, 1.0f, dx)  + 0.02f*to_float3(54,54,4);
  ankle1 = delta +getPos(to_float3(164,146,5), it, kt, 1.0f, dx) + 0.02f*to_float3(56,42,8);
  knee1 = delta + getPos(to_float3(167,118,7), it, kt, 1.0f, dx) + 0.02f*to_float3(30,42,12);
  hip1 = delta +getPos(to_float3(168,91,8), it, kt, 1.0f, dx) + 0.02f;

  shoulder2 = delta +getPos(to_float3(85+91,43,16), it, kt, 1.0f, dx) + 0.02f*to_float3(-5,0,0);
  elbow2 = delta +getPos(to_float3(85+91,73,25), it, kt, 1.0f, dx) + 0.02f*to_float3(4,5,0);
  wrist2 = delta +getPos(to_float3(85+88,103,25), it, kt, 1.0f, dx) + 0.02f*to_float3(23,29,-20);

  foot2 = delta +getPos(to_float3(182,150,10), it, kt, -1.0f, dx) + 0.02f*to_float3(56,52,-8);
  ankle2 = delta +getPos(to_float3(164,146,5), it, kt, -1.0f, dx) + 0.02f*to_float3(64,38,-8);
  knee2 = delta +getPos(to_float3(167,118,7), it, kt, -1.0f, dx) + 0.02f*to_float3(32,28,-8);
  hip2 = delta +getPos(to_float3(168,91,8), it, kt, -1.0f, dx)  + 0.02f;
}

__DEVICE__ float4 render(in float3 ro, in float3 rd, out float3 *roRef, out float3 *rdRef,float2 R, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 ) {
  float t = MAX_DIST, traceStart = 0.0f, traceEnd = MAX_DIST;

    // Render ------------------------
    float3 col;
    float4 colClouds = to_float4_s(0);
    float2 tScene = Trace(ro, rd, traceStart, t, iChannel1);
    
    if (tScene.x > MAX_DIST-5.0f) {
    colClouds = clouds(to_float3_s(0), rd, t, iChannel0);
    }

    float reflection = 0.0f;
  
    *roRef = ro;
    *rdRef = rd;
    
  if (tScene.x < MAX_DIST) {
       
    float3 pos = ro + rd*tScene.x;
    float id = tScene.y;
    float3 sn, sceneColor, rnd = hash33(rd + 311.0f);

    if (id == ID_GROUND) {
      sn = NormalGround(pos, rd, tScene.x, iChannel1);
      sn = doBumpMap(iChannel2, pos/3.0f, sn, 0.035f/(1.0f + tScene.x/MAX_DIST)); 
      sceneColor = _mix(to_float3(0.5f,0.45f,0.4f), to_float3_s(0.12f), clamp(0.0f,1.0f,2.0f*pos.y))+(fract(rnd*289.0f + tScene.x*41.0f) - 0.5f)*0.03f;    
    } else {
      sn = Normal(pos, rd, tScene.x, R);
      if (id == ID_HELMET) {
        float3 posRot = pos; 
        swi2S(posRot,x,z, mul_f2_mat2((swi2(posRot,x,z)-swi2(head,x,z)) , rotHead);
        sn = doBumpMap(iChannel2, posRot/16.0f+0.5f, sn, 0.0004f/(1.0f + tScene.x/MAX_DIST)); 
      }
      float4 sceneColor4 = getColor(id, pos, iTime);
      id = sceneColor4.w;
      sceneColor = swi3(sceneColor4,x,y,z);
    }
    
    *rdRef = reflect(rd,sn);
    *roRef = pos + *rdRef*0.1f;
        
    float reflexion;
        
    // Shading
    col = shading(pos, rd, sn, sceneColor, id, &reflexion, iChannel1);
    reflection = ID_GROUND==id && pos.y<0.2f ? 0.9f : (reflexion)*0.2f;

    // Fog
    col = _mix(col, swi3(colClouds,x,y,z), smoothstep(MAX_DIST-5.0f, MAX_DIST, tScene.x));
    col += (fract(rnd*289.0f + tScene.x*40001.0f) - 0.5f)*0.05f;
    float f = MAX_DIST*0.3f;
    float3 fogColor = to_float3(0.87f,0.85f,1);
    col = _mix( 0.2f*fogColor, col, _exp2f(-tScene.x*fogColor/f) );
    
    } else {
    col = swi3(colClouds,x,y,z);
  }

  return to_float4_aw(col, reflection);
}



__KERNEL__ void MyDarkLandJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
    fragCoord+=0.5f;
  
    if (iTime<30.0f) SetFragmentShaderComputedColor(fragColor); return }; //discard;

   
    gTime = iTime*8.0f;
   
    rotHead = rot(0.6f*(_sinf(0.05f*gTime)*_cosf(0.001f*gTime+1.11f)));
        
    // Animation
    int it = (int)(_floor(gTime));
    float kt = fract(gTime);

    // - init man position -----------------------------------
    initLookingPosition(0, iTime);
    const float a_eyeClose = 0.55f, a_eyeOpen = -0.3f;
    const float t_openEye = 3.0f, t_rotDown = 10.0f, t_closeEye = 1.0f;
    
    // - Eye blink -------------------------------------------
    float time = iTime;
    float a_PaupieresCligne = _mix(a_eyeOpen,a_eyeClose, hash(_floor(time*10.0f))>.98?2.*_fabs(fract(20.0f*time)-0.5f):0.);    
    float a_Paupieres = _mix(a_eyeClose, 0.2f, smoothstep(t_openEye, t_openEye+3.0f, time));    
    a_Paupieres = _mix(a_Paupieres, a_PaupieresCligne, smoothstep(t_rotDown, t_rotDown+1.0f, time));

    g_eyeRot = rot(a_Paupieres);
    
    // Init base vectors --------------------------------------------

    // Foot1 flat part - vector base linked to leg 1
    v2Foot1 = normalize(knee1 - ankle1);
    float3 v1Foot1 = normalize(ankle1 - foot1-v2Foot1*0.1f);
    v3Foot1 = cross(v1Foot1,v2Foot1);
    v2Foot12 = -cross(v1Foot1, v3Foot1);

    v2Foot2 = normalize(knee2 - ankle2);
    float3 v1Foot2 = normalize(ankle2 - foot2-v2Foot2*0.1f);
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

    float2 m = iMouse.xy/iResolution.y - 0.5f;

    float traceStart = 0.2f;

    float3 ro, rd;
    float2 q;
    
// - Camera -----------------------------------------
    
    q = (fragCoord)/iResolution;
    float2 p = -1.0f + 2.0f*q;
    p.x *= iResolution.x/iResolution.y;


  
    time = iTime - 40.0f - 30.0f;
    float dist =  _mix(30.0f,2.0f,smoothstep(0.0f,20.0f,time)) + _mix(0.0f,60.0f, smoothstep(130.0f,240.0f,time));

    float3 ta = head + to_float3(dist*0.1f,-0.05f,0);
    ro = ta + dist*to_float3(_cosf(0.1f*time/* + 6.28f*m.x*/), _mix(0.02f,-0.13f,smoothstep(20.0f,60.0f,time)), _sinf(0.1f*time/* + 6.28f*m.x*/));
     

    // camera-to-world transformation
    mat3 ca = setCamera(ro, ta, 0.0f);

    // ray direction
    rd = ca * normalize( to_float3(swi2(p,x,y),_mix(3.5f,4.5f,smoothstep(0.0f,20.0f,time))));

// ---------------------------------------------------
    float3 roRef, rdRef;
    float4 col = render(ro, rd, &roRef, &rdRef,R,iTime,iChannel0,iChannel1);

#ifdef WITH_REFLECTION
    float3 roRef2, rdRef2;
    swi3S(col,x,y,z, swi3(col,x,y,z)*(1.0f- col.w) + col.w*swi3(clouds(to_float3_s(0), rdRef, MAX_DIST, iChannel0),x,y,z));
#endif

// Post processing stuff --------------------

    // Gamma
    swi3(col,x,y,z, pow_f3(swi3(col,x,y,z), to_float3_s(0.6545f) ));

    // Vigneting
    swi3S(col,x,y,z, swi3(col,x,y,z) * _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.15f)); 
    
  fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Organic 1' to iChannel2
// Connect Buffer B 'Texture: RGBA Noise Medium' to iChannel0
// Connect Buffer B 'Texture: Pebbles' to iChannel1


//-----------------------------------------------------
// Created by sebastien durand - 2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
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
#define g_traceSize 0.002f


#define MAX_DIST 64.0f
#define RUN_STEP 110.0f

//#define rot(a) mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))



__DEVICE__ const float3 gStaticPos = to_float3(-95.0f,8.83f,-121.7f); //to_float3(-95.5f,12.8f,-123);

__DEVICE__ const float3 g_nozePos = to_float3(0,-0.28f+0.04f,0.55f);
__DEVICE__ const float3 g_eyePos = to_float3(0.14f,-0.14f,0.29f);
__DEVICE__ const float g_eyeSize = 0.09f;
__DEVICE__ const mat2 ma = rot(-0.5f);
__DEVICE__ const mat2 mb = rot(-0.15f);
__DEVICE__ const mat2 mc = rot(-0.6f);




__DEVICE__ float gTime;
__DEVICE__ mat2 rotHead, g_eyeRot;
__DEVICE__ float3 gLightPos = to_float3(2, 1, 6);



//  [Shane] Combustible Clouds
//  ------------------

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
__DEVICE__ float3 hash33(float3 p){ 
    float n = _sinf(dot(p, to_float3(7, 157, 113)));    
    return fract_f3(to_float3(2097152, 262144, 32768)*n); 
}

// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
__DEVICE__ float n3D( in float3 p, __TEXTURE2D__ iChannel0 ){
  float3 i = _floor(p); p -= i; p *= p*(3.0f - 2.0f*p);
  swi2S(p,x,y, swi2(texture(iChannel0, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f, -100.0f),y,x));
  return _mix(p.x, p.y, p.z);
}

// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
__DEVICE__ float trigNoise3D(in float3 p, __TEXTURE2D__ iChannel0){ 
    float res = 0.0f, sum = 0.0f;
    float n = n3D(p*8.0f, iChannel0);
    float3 t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f+1.57f/2.0f))*0.5f + 0.5f;
    p = p*1.5f + (t - 1.5f); //  + iTime*0.1
    res += (dot(t, to_float3_s(0.333f)));
    t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f+1.57f/2.0f))*0.5f + 0.5f;
    res += (dot(t, to_float3_s(0.333f)))*0.7071f;    
  return ((res/1.7071f))*0.85f + n*0.15f;
}

__DEVICE__ float4 clouds(in float3 ro, in float3 rd, in float tend, __TEXTURE2D__ iChannel0) {
    float3 lp = gLightPos;
    float3 rnd = hash33(rd + 311.0f);
    float lDe = 0.0f, td = 0.0f, w = 0.0f;
    float d = 1.0f, t = dot(rnd, to_float3_s(0.08f));
    const float h = 0.5f;
    float3 col = to_float3_s(0), sp;
    float3 sn = normalize(hash33(swi3(rd,y,x,z))*0.03f-rd);

    // Raymarching loop.
    for (int i=0; i<48; i++) {
        if((td>1.0f) || d<0.001f*t || t>70.0f || t>tend) break;

        sp = ro + rd*t; // Current ray position.
        d = trigNoise3D(sp*0.75f, iChannel0); // Closest distance to the surface... particle.
        lDe = (h - d) * step(d, h); 
        w = (1.0f - td) * lDe;   
        td += w*w*8.0f + 1.0f/60.0f; //w*w*5.0f + 1.0f/50.0f;
        float3 ld = lp-sp; // Direction vector from the surface to the light position.
        float lDist = _fmaxf(length(ld), 0.001f); // Distance from the surface to the light.
        ld/=lDist; // Normalizing the directional light vector.
        float atten = 1.0f/(1.0f + lDist*0.1f + lDist*lDist*0.03f);
        float diff = _fmaxf(dot(sn, ld ), 0.0f);
        float spec = _powf(_fmaxf(dot( reflect(-ld, sn), -rd ), 0.0f), 4.0f);
        col += w*(1.0f+ diff*0.5f + spec*0.5f)*atten;
        col += (fract_f3(rnd*289.0f + t*41.0f) - 0.5f)*0.02f;;
        t +=  _fmaxf(d*0.5f, 0.02f); //
    }
    
    col = _fmaxf(col, to_float3_s(0.0f));
    col = _mix(_powf(to_float3(1.3f, 1, 1)*col, to_float3(1, 2, 10)), col, dot(_cosf(rd*6.0f +_sinf(swi3(rd,y,z,x)*6.0f)), to_float3_s(0.333f))*0.2f+0.8f);
    float3 sky = to_float3(0.6f, 0.8f, 1.0f)*_fminf((1.5f+rd.y*0.5f)/2.0f, 1.0f);   
    sky = _mix(to_float3(1, 1, 0.9f), to_float3(0.31f, 0.42f, 0.53f), rd.y*0.5f + 0.5f);
    
    float sun = clamp(dot(normalize(lp-ro), rd), 0.0f, 1.0f);
   
    // Combining the clouds, sky and sun to produce the final color.
    sky += to_float3(1, 0.3f, 0.05f)*_powf(sun, 5.0f)*0.25f; 
    sky += to_float3(1, 0.4f, 0.05f)*_powf(sun, 16.0f)*0.35f;   
    col = _mix(col, sky, smoothstep(0.0f, 25.0f, t));
   col += to_float3(1, 0.6f, 0.05f)*_powf(sun, 16.0f)*0.25f;   
 
    // Done.
    return to_float4_aw(col, clamp(0.0f,1.0f,td));    
}


// standard       - https://www.shadertoy.com/view/4sjGzc

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------
// RUN ------
//                       Contact           Down               Pass               Up      

__DEVICE__ float3[9] HEAD2 = { to_float3(67,17,0),    to_float3(184,23,0),     to_float3(279,18,0),     to_float3(375,14,0), to_float3(67,17,0),    to_float3(184,23,0),     to_float3(279,18,0),   to_float3(375,14,0),   to_float3(67,17,0)};
__DEVICE__ float3[9] SHOULDER2 = { to_float3(60,38,16), to_float3(178,46,16),    to_float3(273,41,16),    to_float3(369,38,16), to_float3(67,42,16),    to_float3(182,49,16), to_float3(273,41,16), to_float3(369,38,16), to_float3(60,38,16)};
__DEVICE__ float3[9] ELBOW2 = { to_float3(36,43,25),   to_float3(155,58,25), to_float3(262,64,25), to_float3(371,61,25), to_float3(75,62,25),   to_float3(186,72,25), to_float3(273,67,25), to_float3(355,55,25), to_float3(36,43,25)};
__DEVICE__ float3[9] WRIST2 = { to_float3(24,60,20),  to_float3(148,77,20), to_float3(271,84,25), to_float3(391,68,25), to_float3(93,54,10),  to_float3(206,67,20), to_float3(291,77,25), to_float3(360,80,20), to_float3(24,60,20)};
__DEVICE__ float3[9] HIP2 = { to_float3(55,76,8.0f),  to_float3(171,84,8.0f),   to_float3(264,79,8.0f),   to_float3(360,77,8.0f),  to_float3(50,78,8.0f), to_float3(171,84,8.0f),  to_float3(264,79,8.0f),  to_float3(360,77,8.0f), to_float3(55,76,8.0f)};
__DEVICE__ float3[9] KNEE2 = { to_float3(73,102,7), to_float3(188,111,8),  to_float3(267,112,10), to_float3(358,107,10),  to_float3(41,105,7), to_float3(169,115,7),  to_float3(279,108,7),  to_float3(386,99,7), to_float3(73,102,7)};
__DEVICE__ float3[9] ANKLE2={ to_float3(89,131,5),   to_float3(175,142,6),   to_float3(255,142,10),  to_float3(340,135,10),  to_float3(7,108,5), to_float3(138,114,5),  to_float3(250,115,5),  to_float3(372,126,5), to_float3(89,131,5)};
__DEVICE__ float3[9] FOOT2 = {  to_float3(104,127,10), to_float3(191,144,10),  to_float3(270,144,10),  to_float3(350,144,10),  to_float3(3,122,10),to_float3(131,126,10), to_float3(246,129,10), to_float3(382,136,10), to_float3(104,127,10)};

__DEVICE__ float3 shoulder1, elbow1, wrist1, head, shoulder2, elbow2, wrist2;
__DEVICE__ float3 foot1, ankle1, knee1, hip1, foot2, ankle2, knee2, hip2;
__DEVICE__ float3 v2Foot1, v2Foot12, v2Foot2, v2Foot22, v3Foot1, v3Foot2;
__DEVICE__ float3 v1Hand1, v2Hand1, v3Hand1, v1Hand2, v2Hand2, v3Hand2;


// Interpolate pos of articulations when running
// TODO: standardize arrays to fusion with getPos
__DEVICE__ float3 getPos2(float3 arr[9], int it0, float kt, float z) {
    int it = it0 % 8;
    
    float3 p0 = arr[it], p1 = arr[it+1];
    p0.x -= (float)(it%4)*80.0f - (it>3?RUN_STEP:0.0f);
    p1.x -= (float)((it+1)%4)*80.0f - (it>2?RUN_STEP:0.0f) - (it>6?RUN_STEP:0.0f);
    
    float3 p = _mix(p0, p1, /*smoothstep(0.0f,1.0f,*/kt/*)*/);
    if (z<0.0f) p.x += -RUN_STEP;
    
    p.x += float(it0/8)*RUN_STEP*2.0f;
    return 0.02f*to_float3(p.x, 144.0f-p.y, p.z*z);
}


//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

__DEVICE__ float hash( float n ) { return fract(_sinf(n)*43758.5453123f); }

// mix noise for alive animation, full source
__DEVICE__ float4 hash4( float4 n ) { return fract_f4(sin_f4(n)*1399763.5453123f); }
__DEVICE__ float3 hash3( float3 n ) { return fract_f3(sin_f3(n)*1399763.5453123f); }


//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] https://iquilezles.org/articles/distfunctions
//---------------------------------------------------------------------

__DEVICE__ bool cube(float3 ro, float3 rd, float3 sz, out float *tn, out float *tf) { 
  float3 m = 1.0f/rd,
         k = abs_f3(m)*sz,
         a = -m*ro-k*0.5f, 
         b = a+k;
    *tn = _fmaxf(max(a.x,a.y),a.z);
    *tf = _fminf(min(b.x,b.y),b.z); 
  return *tn>0.0f && *tn<*tf;
}


__DEVICE__ float sdCap(in float3 p, in float3 a, in float3 b, in float r ) {
    float3 pa = p - a, ba = b - a;
    return length(pa - ba*clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f ) ) - r;
}

__DEVICE__ float sdCapsule(in float3 p, in float3 a, in float3 b, in float r0, in float r1 ) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f);
    return length( pa - ba*h ) - _mix(r0,r1,h);
}

__DEVICE__ float sdCap2(in float3 p, in float3 a, in float3 b, in float r1, in float r2) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length(pa - ba*h) - _mix(r1,r2,h*h*h);
}

__DEVICE__ float udRoundBox(in float3 p, in float3 b, in float r ) {
  return length(_fmaxf(abs_f3(p)-b, to_float3_s(0.0f)))-r;
}

//https://www.shadertoy.com/view/Xs3GRB
__DEVICE__ float fCylinder(in float3 p, in float r, in float height) {
  return _fmaxf(length(swi2(p,x,z)) - r, _fabs(p.y) - height);
}

__DEVICE__ float sdPlane(in float3 p, in float3 n) {  // n must be normalized
  return dot(p,n);
}

__DEVICE__ float smin(in float a, in float b, in float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

// Smooth maximum, based on the function above.
__DEVICE__ float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5f + 0.5f*(a-b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f-h)*s;
}

// hg_sdf : http://mercury.sexy/hg_sdf/
__DEVICE__ float fOpEngrave(float a, float b, float r) {
  return _fmaxf(a, (a + r - _fabs(b))*0.7071f); //_sqrtf(0.5f));
}

__DEVICE__ float fOpIntersectionRound(float a, float b, float r) {
  float2 u = _fmaxf(to_float2(r + a,r + b), to_float2_s(0));
  return _fminf(-r, _fmaxf (a, b)) + length(u);
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r) {
    return (length(p/r ) - 1.0f) * _fminf(min(r.x,r.y),r.z);
}


__DEVICE__ float fOpDifferenceRound (float a, float b, float r) {
  return fOpIntersectionRound(a, -b, r);
}


//---------------------------------------------------------------------
//    Man + Ground distance field 
//---------------------------------------------------------------------

// The canyon, complete with hills, gorges and tunnels. I would have liked to provide a far
// more interesting scene, but had to keep things simple in order to accommodate slower machines.
__DEVICE__ float mapGround(in float3 p, __TEXTURE2D__ iChannel1){
    float tx = -0.2f*(texture(iChannel1, swi2(p,x,z)/16.0f + swi2(p,x,y)/80.0f).x);
    float3 q = p*0.25f;

    float h = tx + 0.2f+0.5f*(dot(sin_f3(q)*cos_f3(swi3(q,y,z,x)), to_float3_s(0.222f))) + dot(sin_f3(q*1.3f)*cos_f3(swi3(q,y,z,x)*1.4f), to_float3_s(0.111f));
    float d = p.y + smin(0.0f,smoothstep(0.2f,3.0f, _fabs(p.z))*(-h)*6.0f,0.2f);

    return d; 
}

// capsule with bump in the middle -> use for neck
__DEVICE__ float sdCapsule2(in float3 p,in float3 a,in float3 b, in float r0,in float r1,in float bump) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    float dd = bump*_sinf(3.14f*h);  // Little adaptation
    return length(pa - ba*h) - _mix(r0,r1,h)*(1.0f+dd); 
}


__DEVICE__ float smax(in float a, in float b, in float k) {
    return _logf(exp(a/k)+_expf(b/k))*k;
}


__DEVICE__ float mapHead(in float3 p) {
    float d;
float cccccccccccccccccccc;   
// Skull modeling -------------------------
    d = sdEllipsoid(p-to_float3(0,0.05f,0.0f), to_float3(0.39f,0.48f,0.46f));          
    d = smin(d, sdEllipsoid(p-to_float3(0.0f,0.1f,-0.15f), to_float3(0.42f,0.4f,0.4f)),0.1f);     
    d = smin(d, udRoundBox(p-to_float3(0,-0.28f,0.2f), to_float3(0.07f,0.05f,0.05f),0.05f),0.4f); // Basic jaw 

// Symetrie -------------------------------
    p.x = _fabs(p.x);

// Eye hole 
    d = smax(d, -sdEllipsoid(p-to_float3(0.12f,-0.16f,0.48f), to_float3(0.09f,0.06f,0.09f)), 0.07f);

// Noze ------------------------------------
    d = smin(d, _fmaxf(-(length(p-to_float3(0.032f,-0.325f,0.45f))-0.028f),   // Noze hole
                    smin(length(p-to_float3(0.043f,-0.29f,0.434f))-0.01f,  // Nostrils
                    sdCapsule(p, to_float3(0,-0.13f,0.39f), to_float3(0,-0.28f,0.47f), 0.01f,0.04f), 0.05f)) // Bridge of the nose
                    ,0.065f); 
   
// Mouth -----------------------------------    
    d = smin(d, length(p- to_float3(0.22f,-0.34f,0.08f)), 0.17f); // Jaw
    d = smin(d, sdCapsule(p, to_float3(0.16f,-0.35f,0.2f), to_float3(-0.16f,-0.35f,0.2f), 0.06f,0.06f), 0.15f); // Cheeks
   
    d = smin(d, _fmaxf(-length(swi2(p,x,z)-to_float2(0,0.427f))+0.015f,    // Line under the noze
                _fmaxf(-p.y-0.41f,               // Upper lip
                sdEllipsoid(p- to_float3(0,-0.34f,0.37f), to_float3(0.08f,0.15f,0.05f)))), // Mouth bump
                0.032f);

// Chin -----------------------------------  
    d = smin(d, length(p- to_float3(0,-0.5f,0.26f)), 0.2f);   // Chin
    d = smin(d, length(p- to_float3(0,-0.44f,0.15f)), 0.25f); // Under chin 
  
    //d = smin(d, sdCapsule(p, to_float3(0.24f,-0.1f,0.33f), to_float3(0.08f,-0.05f,0.46f), 0.0f,0.01f), 0.11f); // Eyebrow 
    
// Eyelid ---------------------------------
    float3 p_eye1 = p - g_eyePos;
    //swi2(p_eye1,x,z) *= mb;
    swi2S(p_eye1,x,z, mul_f2_mat2(swi2(p_eye1,x,z) , mb));
    swi2S(p_eye1,x,z, mul_f2_mat2(swi2(p_eye1,x,z) , mb));
    
    float3 p_eye2 = p_eye1;
    float d_eye = length(p_eye1) - g_eyeSize*1.0f;
          
    swi2S(p_eye1,y,z, mul_f2_mat2(swi2(p_eye1,y,z) , g_eyeRot));
    swi2S(p_eye2,z,y, mul_f2_mat2(swi2(p_eye2,z,y) , mc));
    
    float d1 = _fminf(_fmaxf(-p_eye1.y,d_eye - 0.01f),
                      _fmaxf(p_eye2.y,d_eye - 0.005f));

    d = smin(d,d1,0.01f);
  return d; 
}



__DEVICE__ float mapLegs(const in float3 pos){    
    // Leg 1
    float d = _fmaxf(_fminf(sdCap2(pos, foot1, ankle1, 0.1f,0.15f),
                            sdCap2(pos, ankle1, knee1, 0.165f,0.105f)),
                           -sdPlane(pos-ankle1+v2Foot1*0.1f, v2Foot12));
    // Leg 2
    d = _fminf(d,_fmaxf(_fminf(sdCap2(pos, foot2, ankle2, 0.1f,0.15f),
                               sdCap2(pos, ankle2, knee2, 0.165f,0.105f)),
                              -sdPlane(pos-ankle2+v2Foot2*0.1f, v2Foot22)));
 
    d = fOpEngrave(d, _fminf(_fminf(sdCap(pos-ankle1, -0.1f*v3Foot1, 0.1f*v3Foot1, 0.12f), 
                                    sdCap(pos-ankle2, -0.1f*v3Foot2, 0.1f*v3Foot2, 0.12f)), 
                             _fminf(length(pos - knee1),length(pos - knee2))-0.11f), 0.015f);
    
    d = _fminf(d, sdCap2(pos,  hip1, knee1, 0.12f, 0.075f));
    return _fminf(d, sdCap2(pos, hip2, knee2, 0.12f, 0.075f));
}

__DEVICE__ float mapGirl(const in float3 pos){
    float3 
         ep0 = _mix(shoulder1,shoulder2,0.5f),
         ha0 = _mix(hip1,hip2,0.5f),
         h1 = head + to_float3(0.0f,-0.24f-0.05f,0),
         h2 = head + to_float3(0.0f,0.15f-0.05f,0),
         hn = normalize(h1-h2),
         a = _mix(ha0,ep0,0.15f), b = _mix(ha0,ep0,0.75f);
    
    float3 posRot = pos; // - head; 
    swi2S(posRot,x,z, mul_f2_mat2((swi2(posRot,x,z) - swi2(head,x,z)) , rotHead) + swi2(head,x,z));
   // swi2(posRot,x,z) = (swi2(posRot,x,z)) * rotHead;
    
    float d = mapLegs(pos);

    // Head
    float scaleHead = 1.75f;
    float3 pHead = (posRot - head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = _fminf(d, dHead);

  // Eye
    pHead.x = _fabs(pHead.x);
    float3 p_eye = pHead-g_eyePos;
    swi2S(p_eye,x,z, mul_f2_mat2(swi2(p_eye,x,z) , ma));  
    float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    d = _fminf(d, dEye);
        
    // Arms
    d = _fminf(d, sdCapsule2(pos, shoulder1, elbow1, 0.054f,0.051f,0.5f));
    d = _fminf(d, sdCapsule2(pos, shoulder2, elbow2, 0.054f,0.051f,0.5f));
    float dArm = sdCap2(pos, elbow1, wrist1-0.01f*v1Hand1, 0.09f, 0.055f);
    dArm = _fminf(dArm,  sdCap2(pos, elbow2, wrist2-0.01f*v1Hand2, 0.09f, 0.055f));
    dArm = fOpEngrave(dArm, 
                       _fminf(_fminf(length(pos - wrist2), length(pos - wrist1)),
                              _fminf(length(pos - elbow2), length(pos - elbow1))) - 0.1f,0.008f);

    
    // Neck and Shoulders
    d = smin(d, _fminf(sdCapsule2(pos+to_float3(0.03f,0,0), _mix(shoulder1, shoulder2,0.1f),_mix(shoulder1, shoulder2,0.9f),0.08f, 0.08f,0.6f),
                       sdCap(pos,ep0-to_float3(0.03f,0,0), head-to_float3(0.08f,0.1f,0), 0.09f)), 0.06f);
    
    // Torso
    d = smin(d, _fminf(sdCap2(pos, a+to_float3(0,0,0.03f), b-to_float3(0,0,0.04f), 0.19f,0.22f),sdCap2(pos, a-to_float3(0,0,0.03f), b+to_float3(0,0,0.04f), 0.19f,0.22f)),0.18f);
    
  // Fingers 1
    float3 c = wrist1-v3Hand1*0.03f;
    float d2 = sdCap(pos, c-v1Hand1*0.06f+v2Hand1*0.03f+v3Hand1*0.06f, wrist1+0.09f*(v2Hand1+v1Hand1+v3Hand1), 0.02f);
    d2 = _fminf(d2, sdCap(pos, c, wrist1+0.18f*(v1Hand1+v2Hand1*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.01f, wrist1+0.2f*(v1Hand1-v2Hand1*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.02f, wrist1+0.18f*(v1Hand1-v2Hand1*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.04f, wrist1+0.15f*(v1Hand1-v2Hand1*0.8f), 0.014f));
    
  // Fingers 2     
    c = wrist2-v3Hand2*0.03f;
    d2 = _fminf(d2, sdCap(pos, c-v1Hand2*0.06f+v2Hand2*0.03f+v3Hand2*0.06f, wrist2+0.09f*(v2Hand2+v1Hand2+v3Hand2), 0.02f));
    d2 = _fminf(d2, sdCap(pos, c, wrist2+0.18f*(v1Hand2+v2Hand2*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.01f, wrist2+0.2f*(v1Hand2-v2Hand2*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.02f, wrist2+0.18f*(v1Hand2-v2Hand2*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.04f, wrist2+0.15f*(v1Hand2-v2Hand2*0.8f), 0.014f));


    d = _fminf(d, smin(d2, dArm, 0.05f));
   
    // Base corps
    // v2 = normalize(a - b);
   // float3 v3 = normalize(cross(to_float3(1,0,0),v2));
   // float3 v1 = cross(v3, v2);
    
    // Short
    float dShort = _fminf(sdCap(pos, hip1+to_float3(-0.03f,0,0), _mix(hip1, knee1,0.25f), 0.13f), 
                          sdCap(pos, hip2+to_float3(-0.03f,0,0), _mix(hip2, knee2,0.25f), 0.13f));                    
    // TODO ca serait plus cool de bouger avec les hanches mais il faut recuperer une base fixee sur les hanches
    dShort = smin(dShort, _mix(d, sdCap(pos, a, ha0+to_float3(0,0.1f,0), 0.22f),0.5f),0.1f);
    d = _fminf(d, dShort);

    // Casque
    float dHelmet;
    posRot += to_float3(0.03f,0.02f,0);
    dHelmet = _fmaxf(sdPlane(posRot-h1+hn*0.07f, hn), _mix(dHead, sdCap2(posRot, h1-to_float3(0.23f,0,0), h2-to_float3(0,0.05f,0),0.28f,0.36f),0.5f));
    dHelmet = _fmaxf(-fCylinder(posRot-h1-to_float3(0.2f,0,0), 0.18f,0.3f), dHelmet); 
    dHelmet = fOpEngrave(dHelmet, sdCap(posRot-h2, -to_float3(0,0.1f,1), -to_float3(0,0.1f,-1), 0.1f),0.015f);
   
    d = _fminf(d, dHelmet);
//  d = _fminf(d, mapGround(pos));
            
    return _fminf(d2,d);
}


//---------------------------------------------------------------------
//    Girl colors 
//---------------------------------------------------------------------
#define min2(a, b) (a.x<b.x?a:b)
#define max2(a, b) (a.x>b.x?a:b)

#define ID_MAN 100.
#define ID_GROUND 90.
#define ID_GLOVE 106.0f 
#define ID_HELMET 107.
#define ID_FOOT 108.
#define ID_SHORT 110.
#define ID_LEG  201.
#define ID_SKIN 202.
#define ID_ARM  203.
#define ID_TORSO 204.
#define ID_EYE 205.


__DEVICE__ const float3 COLOR_SKIN = to_float3(0.6f,0.43f,0.3f);
__DEVICE__ const float3 COLOR_ARMOR = to_float3(0.14f,0.79f,0.7f);
__DEVICE__ const float3 COLOR_CLOTHES2 = to_float3(0.14f,0.79f,0.7f);
__DEVICE__ const float3 COLOR_CLOTHES = to_float3(0.66f,0.94f,0.91f);



__DEVICE__ float4 drawEye(in float3 p) {
    float3 posRot = p; // - head; 
    swi2S(posRot,x,z, mul_f2_mat2((swi2(posRot,x,z) - swi2(head,x,z)) , rotHead) + swi2(head,x,z));
    
    float scaleHead = 1.75f;
    float3 pHead = (posRot-head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;

  // Eye
    float3 p_eye = pHead-g_eyePos;
    float3 g_eyePosloc = g_eyePos;
    g_eyePosloc.x *= sign(pHead.x);
    float3 pe = pHead - g_eyePosloc;

   
    float a = 0.2f*_sinf(2.0f*iTime)*_cosf(0.01f*iTime);//clamp(_atan2f(-dir.x, dir.z), -0.6f,0.6f), 
    float ca = _cosf(a), sa = _sinf(a);
    swi2S(pe,x,z, mul_f2_mat2(swi2(pe,x,z) , to_mat2(ca, sa, -sa, ca)));

    float b = 0.2f;//0.1f+0.1f*_sinf(iTime*0.1f);//clamp(_atan2f(-dir.y, dir.z), -0.3f,0.3f), 
    float cb = _cosf(b), sb = _sinf(b);
    swi2S(pe,y,z, mul_f2_mat2(swi2(pe,y,z) , to_mat2(cb, sb, -sb, cb)));
    
    float d = length(swi2(pe,x,y));
    float3 col = _mix(to_float3_s(0), _mix(to_float3(0.88f,0.41f,0.0f), _mix(to_float3_s(0),to_float3_s(1.5f),
                   0.5f+0.5f*smoothstep(0.0405f,0.0415f,d)), smoothstep(0.04f,0.041f,d)), smoothstep(0.02f,0.025f,d));
   // float d2 = smoothstep(0.03f,0.04f,length(swi2(pe,x,y)));
    return to_float4_aw(col,ID_EYE);
}

__DEVICE__ float sdFish(float3 o) {
    float2 p = (o - (shoulder1 + shoulder2)*0.5f).zy + to_float2(0.04f,0.17f);
    p *= 2.0f;
    float dsub = _fminf(length(p-to_float2(0.8f,0.0f)) - 0.45f, length(p-to_float2(-0.14f,0.05f)) - 0.11f);  
    p.y = _fabs(p.y);
    float d = length(p-to_float2(0.0f,-0.15f)) - 0.3f;
    d = _fminf(d, length(p-to_float2(0.56f,-0.15f)) - 0.3f);
    d = _fmaxf(d, -dsub);
    return (1.0f-smoothstep(0.05f,0.06f,d));
}

           
// -----------------------------------------


__DEVICE__ float4 getColor(float id, float3 pos) {
  return   0.3f+0.7f*(id == ID_FOOT ? to_float4_aw(COLOR_ARMOR, id) :
      id == ID_LEG ? to_float4_aw(COLOR_CLOTHES, ID_LEG) :
      id == ID_EYE ? drawEye(pos) :
      id == ID_SKIN ?  to_float4_aw(COLOR_SKIN, ID_SKIN) :
      id == ID_TORSO ? to_float4_aw(_mix(COLOR_CLOTHES,to_float3_s(0),sdFish(pos)), ID_TORSO) :
      id == ID_ARM ? to_float4_aw(COLOR_SKIN, ID_ARM) :
      id == ID_SHORT ? to_float4_aw(COLOR_CLOTHES2, ID_SHORT) :
      to_float4_aw(COLOR_ARMOR, id));
}


__DEVICE__ float mapColor(in float3 pos) {
    float3 
         ep0 = _mix(shoulder1,shoulder2,0.5f),
         ha0 = _mix(hip1,hip2,0.5f),
         h1 = head + to_float3(0.0f,-0.24f-0.05f,0),
         h2 = head + to_float3(0.0f,0.15f-0.05f,0),
         hn = normalize(h1-h2),
         a = _mix(ha0,ep0,0.15f), b = _mix(ha0,ep0,0.79f);
    float3 posRot = pos; 
    swi2S(posRot,x,z, mul_f2_mat2((swi2(posRot,x,z) - swi2(h1,x,z)) , rotHead) + swi2(h1,x,z));
    
    // Leg 1
    float d = _fmaxf(_fminf(sdCap2(pos, foot1, ankle1, 0.1f,0.15f),
                            sdCap2(pos, ankle1, knee1, 0.165f,0.105f)),
                           -sdPlane(pos-ankle1+v2Foot1*0.1f, v2Foot12));
    // Leg 2
  d = _fminf(d,_fmaxf(_fminf(sdCap2(pos, foot2, ankle2, 0.1f,0.15f),
                             sdCap2(pos, ankle2, knee2, 0.165f,0.105f)),
                            -sdPlane(pos-ankle2+v2Foot2*0.1f, v2Foot22)));                          
    
    float2 dd;
    dd = min2(to_float2(d, ID_FOOT), 
              to_float2(_fminf(sdCap(pos, knee1, hip1, 0.075f),
               sdCap(pos, knee2, hip2, 0.075f)), ID_LEG));

    // Head
    float scaleHead = 1.75f;
    float3 pHead = (posRot - head) + to_float3(0.02f, 0.04f, 0);
    pHead = swi3(pHead,z,y,x)*scaleHead;
    float dHead = mapHead(pHead)/scaleHead;
    d = _fminf(d, dHead);

  // Eye
    pHead.x = _fabs(pHead.x);
    float3 p_eye = pHead-g_eyePos;
    swi2(p_eye,x,z) *= ma;  

    
  float dEye = (length(p_eye) - g_eyeSize)/scaleHead;
    dd = min2(dd, to_float2(dEye,ID_EYE));

  //  d = _fminf(d, dHead);
    dd = min2(dd, to_float2(_fminf(dHead,               // chin
            sdCap(pos, ep0-to_float3(0.03f,0,0), head-to_float3(0.08f,0.1f,0), 0.1f)), ID_SKIN));  // neck           
    // Arms
    dd = min2(dd, to_float2(_fminf(sdCap(pos, shoulder2, elbow2, 0.05f), sdCap(pos, shoulder1, elbow1, 0.05f)), ID_LEG));
    
  // Fingers 1
    float3 c = wrist1-v3Hand1*0.03f;
    float d2 = sdCap(pos, c-v1Hand1*0.06f+v2Hand1*0.03f+v3Hand1*0.06f, wrist1+0.09f*(v2Hand1+v1Hand1+v3Hand1), 0.02f);
    d2 = _fminf(d2, sdCap(pos, c, wrist1+0.18f*(v1Hand1+v2Hand1*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.01f, wrist1+0.2f*(v1Hand1-v2Hand1*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.02f, wrist1+0.18f*(v1Hand1-v2Hand1*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand1*0.04f, wrist1+0.15f*(v1Hand1-v2Hand1*0.8f), 0.014f));
    
  // Fingers 2     
    c = wrist2-v3Hand2*0.03f;
    d2 = _fminf(d2, sdCap(pos, c-v1Hand2*0.06f+v2Hand2*0.03f+v3Hand2*0.06f, wrist2+0.09f*(v2Hand2+v1Hand2+v3Hand2), 0.02f));
    d2 = _fminf(d2, sdCap(pos, c, wrist2+0.18f*(v1Hand2+v2Hand2*0.2f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.01f, wrist2+0.2f*(v1Hand2-v2Hand2*0.2f), 0.017f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.02f, wrist2+0.18f*(v1Hand2-v2Hand2*0.5f), 0.016f));
    d2 = _fminf(d2, sdCap(pos, c-v2Hand2*0.04f, wrist2+0.15f*(v1Hand2-v2Hand2*0.8f), 0.014f));

    d2 = _fminf(d2, _fminf(sdCap2(pos, elbow1, wrist1-0.01f*v1Hand1, 0.09f, 0.055f),  
                     sdCap2(pos, elbow2, wrist2-0.01f*v1Hand2, 0.09f, 0.055f)));
    dd = min2(dd, to_float2(d2, ID_GLOVE));
           
    // Torso
    dd = min2(dd, to_float2(_fminf(sdCap(pos, shoulder1, shoulder2, 0.1f), 
                           sdCap2(pos, a, b, 0.19f,0.22f)), ID_TORSO));
  
    // Short
    float dShort = _fminf(sdCap(pos, hip1, _mix(hip1,knee1,0.3f), 0.12f), 
                       sdCap(pos, hip2, _mix(hip2,knee2,0.3f), 0.12f));                    
    dd = min2(dd, to_float2(_fminf(dShort, _mix(dd.x, sdCap(pos, a, ha0, 0.22f),0.75f)), ID_SHORT));

    // Casque
  float dHelmet;
    posRot += to_float3(0.03f,0.02f,0);
    dHelmet = _fmaxf(sdPlane(posRot-h1+hn*0.08f, hn), _mix(dHead, sdCap2(posRot, h1-to_float3(0.23f,0,0), h2-to_float3(0,0.05f,0),0.28f,0.36f),0.5f));
    dHelmet = _fmaxf(-fCylinder(posRot-h1-to_float3(0.2f,0,0), 0.18f,0.3f), dHelmet); 
    dd = min2(dd, to_float2(dHelmet, ID_HELMET));

    return dd.y;
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------
              
__DEVICE__ float logBisectTrace(in float3 ro, in float3 rd){

    float t = 0.0f, told = 0.0f, mid, dn;
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
        if (sign(d) != sgn || d < 0.001f || t > MAX_DIST) break;
        told = t;
        // Branchless version of the following:      
        t += step(d, 1.0f)*(_logf(_fabs(d) + 1.1f) - d) + d;
        //t += _logf(_fabs(d) + 1.1f);
        //t += d;//step(-1.0f, -d)*(d - d*0.5f) + d*0.5f;
        d = mapGround(rd*t + ro);
    }
    // If a threshold was crossed without a solution, use the bisection method.
    if (sign(d) != sgn){
        // Based on suggestions from CeeJayDK, with some minor changes.
        dn = sign(mapGround(rd*told + ro));
        float2 iv = to_float2(told, t); // Near, Far
        // 6 iterations seems to be more than enough, for most cases...
        // but there's an early exit, so I've added a couple more.
        for (int ii=0; ii<8; ii++){ 
            //Evaluate midpoint
            mid = dot(iv, to_float2_s(0.5f));
            float d = mapGround(rd*mid + ro);
            if (_fabs(d) < 0.001f)break;
            // Suggestion from movAX13h - Shadertoy is one of those rare
            // sites with helpful commenters. :)
            // Set mid to near or far, depending on which side we're on.
            iv = _mix(to_float2(iv.x, mid), to_float2(mid, iv.y), step(0.0f, d*dn));
        }

        t = mid; 
        
    }
    
    //if (_fabs(d) < PRECISION) t += d;

    return _fminf(t, MAX_DIST);
}

__DEVICE__ float2 Trace(in float3 pos, in float3 ray, in float start, in float end ) {
    // Trace if in bbox
    float t=start, h, tn=start, tf=end;
    float tGround = logBisectTrace(pos, ray);

   // start = _fmaxf(start, );
    end = _fminf(tGround, end);
    
    if (cube(pos-head-to_float3(-0.1f,-1.0f,0), ray, to_float3(1.2f, 1.7f,0.7f)*2.0f,  tn, tf)) {
        end = _fminf(tf, end);
        t = _fmaxf(tn, start);// - 0.3f*hash33(pos+ray).x;
        for( int i=0; i < g_traceLimit; i++) {
      if (t > end) break;
            h = mapGirl( pos+t*ray );
            if (h < g_traceSize) {
                return to_float2(t+h, mapColor(pos+t*ray));
            }
            t += h;
        }
        if (t < end) return to_float2(t, mapColor(pos+t*ray)); 
    } 
    
    return tGround < MAX_DIST ?  to_float2(tGround, ID_GROUND) : to_float2(MAX_DIST, 0.0f);
}

//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D( sampler2D tex, in float3 p, in float3 n ){
    n = _fmaxf(n*n, 0.001f);
    n /= (n.x + n.y + n.z );  
  return (texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z).xyz;
}

__DEVICE__ float getGrey(float3 p){ return dot(p, to_float3(0.299f, 0.587f, 0.114f)); }

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
__DEVICE__ float3 doBumpMap( sampler2D tex, in float3 p, in float3 nor, float bumpfactor){
    const float eps = 0.001f;
    float3 grad = to_float3( getGrey(tex3D(tex, to_float3(p.x-eps, p.y, p.z), nor)),
                      getGrey(tex3D(tex, to_float3(p.x, p.y-eps, p.z), nor)),
                      getGrey(tex3D(tex, to_float3(p.x, p.y, p.z-eps), nor)));
    grad = (grad - getGrey(tex3D(tex,  p , nor)))/eps; 
    grad -= nor*dot(nor, grad);                  
    return normalize( nor + grad*bumpfactor );
}


//---------------------------------------------------------------------
//   Soft shadows
//---------------------------------------------------------------------

#ifdef WITH_SHADOW
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax ) {
  float h, res = 1.0f, t = mint;
    for(int i=0; i<24; i++) {
    h = map( ro + rd*t );
        res = _fminf( res, 8.0f*h/t );
        t += clamp( h, 0.05f, 0.2f );
        if( h<0.01f || t>tmax ) break;
    }
    return clamp(res, 0.0f, 1.0f);
}
#endif

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
__DEVICE__ float calcAO( in float3 pos, in float3 nor ){
  float dd, hr, sca = 1.0f, totao = 0.0f;
    float3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = 0.01f + 0.05f*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(mapGirl( aopos )-hr)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - 4.0f*totao, 0.0f, 1.0f);
}
__DEVICE__ float calcAOGround( in float3 pos, in float3 nor ){
  float dd, hr, sca = 1.0f, totao = 0.0f;
    float3 aopos; 
    for( int aoi=0; aoi<5; aoi++ ) {
        hr = 0.01f + 0.05f*float(aoi);
        aopos =  nor * hr + pos;
        totao += -(_fminf(mapGround(aopos),mapLegs(aopos))-hr)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - 4.0f*totao, 0.0f, 1.0f);
}
#endif


//---------------------------------------------------------------------
//   Shading
//   Adapted from Shane / Iq
//---------------------------------------------------------------------

__DEVICE__ float3 shading( in float3 sp, in float3 rd, in float3 sn, in float3 col, in float id, out float reflexion){
    
    float3 general = to_float3(240,198,157)/256.0f,
       back = to_float3(63,56,46)/256.0f;
    float3 ref = reflect( rd, sn );

    // lighitng   
#ifdef WITH_AO
    float occ = id == ID_GROUND ? calcAOGround( sp, sn ) : calcAO( sp, sn );
#else
    float occ = 1.0f;
#endif
    float3  ld = normalize( gLightPos );
    float3  hal = normalize( rd - ld);
    float amb = 0.15f; //clamp( 0.5f+0.5f*sn.y, 0.0f, 1.0f );
    float dif = clamp( dot( sn, ld ), 0.0f, 1.0f );
    float bac = clamp( dot( sn, normalize(to_float3(-ld.x,0.0f,-ld.z))), 0.0f, 1.0f );//*clamp( 1.0f-sp.y,0.0f,1.0f);
    float dom = smoothstep( -0.1f, 0.1f, ref.y );
    float fre = _powf( clamp(1.0f+dot(sn,rd),0.0f,1.0f), 2.0f );

    reflexion = fre*occ;
    
#ifdef WITH_SHADOW
    dif *= calcSoftshadow( sp, ld, 0.05f, 2.0f );
#endif
    
    float spe =  _powf( clamp( dot( sn, -hal ), 0.0f, 1.0f ), id >= ID_SHORT ? 10.0f : 164.0f) * dif * (0.04f + 0.96f*_powf( clamp(1.0f+dot(hal,rd),0.0f,1.0f), 50.0f ));

    float3 lin = to_float3_s(0.0f);
    lin += 0.80f*dif*general/*to_float3(1.00f,0.80f,0.55f)*/*(0.3f+0.7f*occ);
    lin += 0.40f*amb*occ*general;//to_float3(0.40f,0.60f,1.00f);
   // lin += 0.15f*dom*occ*general;//to_float3(0.40f,0.60f,1.00f)*occ;
    lin += 0.15f*bac*back/*to_float3(0.25f,0.25f,0.25f)*/*occ;
   // lin += 0.25f*fre*to_float3(1.00f,1.00f,1.00f)*occ;
   
    col = col*lin;
    col += (id == ID_EYE ? 10.0f : id >= ID_SHORT ? 0.3f : 1.0f)*spe*to_float3(1.00f,0.90f,0.70f);
    
    return col;
}



//---------------------------------------------------------------------
//   Calculate normal
//   From TekF 
//---------------------------------------------------------------------
__DEVICE__ float3 Normal(in float3 pos, in float3 ray, in float t) {
  float pitch = 0.1f * t / iResolution.x;   
  pitch = _fmaxf( pitch, 0.002f );
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x), // tetrahedral offsets
       p1 = pos+swi3(d,x,y,y),
       p2 = pos+swi3(d,y,x,y),
       p3 = pos+swi3(d,y,y,x);

   float f0 = mapGirl(p0), f1 = mapGirl(p1), f2 = mapGirl(p2),  f3 = mapGirl(p3);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}

__DEVICE__ float3 NormalGround(in float3 pos, in float3 ray, in float t) {
  float pitch = 0.2f * t / iResolution.x;   
  pitch = _fmaxf( pitch, 0.005f );
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x), // tetrahedral offsets
       p1 = pos+swi3(d,x,y,y),
       p2 = pos+swi3(d,y,x,y),
       p3 = pos+swi3(d,y,y,x);

   float f0 = mapGround(p0), f1 = mapGround(p1), f2 = mapGround(p2), f3 = mapGround(p3);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
 //   return normalize(grad);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}



//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

__DEVICE__ mat3 setCamera(in float3 ro, in float3 ta, in float cr) {
  float3 cw = normalize(ta-ro),
     cp = to_float3_aw(_sinf(cr), _cosf(cr), 0.0f),
     cu = normalize( cross(cw,cp) ),
     cv = normalize( cross(cu,cw) );
    return to_mat3_f3( cu, cv, cw );
}


//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------


    
__DEVICE__ void initRunningPosition(int it, float kt) {
  head = getPos2(HEAD2, it, kt, 1.0f);

  shoulder1 = getPos2(SHOULDER2, it, kt, 1.0f);
  elbow1 = getPos2(ELBOW2, it, kt, 1.0f);
  wrist1 = getPos2(WRIST2, it, kt, 1.0f);

  foot1 = getPos2(FOOT2, it, kt, 1.0f);
  ankle1 = getPos2(ANKLE2, it, kt, 1.0f);
  knee1 = getPos2(KNEE2, it, kt, 1.0f);
  hip1 = getPos2(HIP2, it, kt, 1.0f);

  shoulder2 = getPos2(SHOULDER2, it+4, kt, -1.0f);
  elbow2 = getPos2(ELBOW2, it+4, kt, -1.0f);
  wrist2 = getPos2(WRIST2, it+4, kt, -1.0f);

  foot2 = getPos2(FOOT2, it+4, kt, -1.0f);
  ankle2 = getPos2(ANKLE2, it+4, kt, -1.0f);
  knee2 = getPos2(KNEE2, it+4, kt, -1.0f);
  hip2 = getPos2(HIP2, it+4, kt, -1.0f);
}



__DEVICE__ float4 render(in float3 ro, in float3 rd, out float3 roRef, out float3 rdRef) {
  float t = MAX_DIST, traceStart = 0.0f, traceEnd = MAX_DIST;

    // Render ------------------------
  float3 col;
    float4 colClouds = to_float4(0);
    float2 tScene = Trace(ro, rd, traceStart, t);
    
    if (tScene.x > MAX_DIST-5.0f) {
    colClouds = clouds(to_float3(0), rd, t);
    }

  float reflection = 0.0f;
  
    roRef = ro;
    rdRef = rd;
    
  if (tScene.x < MAX_DIST) {
       
    float3 pos = ro + rd*tScene.x;
        float id = tScene.y;
    float3 sn, sceneColor, rnd = hash33(rd + 311.0f);

        if (id == ID_GROUND) {
            sn = NormalGround(pos, rd, tScene.x);
      sn = doBumpMap(iChannel2, pos/4.0f, sn, 0.025f/(1.0f + tScene.x/MAX_DIST)); 
      sceneColor = _mix(1.2f*to_float3(0.5f,0.45f,0.4f), to_float3_s(0.12f), clamp(0.0f,1.0f,2.0f*pos.y))+(fract(rnd*289.0f + tScene.x*41.0f) - 0.5f)*0.03f;    
        } else {
      sn = Normal(pos, rd, tScene.x);
   #ifndef IS_RUNNING
            if (id == ID_HELMET) {
                float3 posRot = pos; 
          swi2(posRot,x,z) = (swi2(posRot,x,z)-swi2(head,x,z)) * rotHead;
              sn = doBumpMap(iChannel2, posRot/16.0f+0.5f, sn, 0.0004f/(1.0f + tScene.x/MAX_DIST)); 
            }
   #endif         
      float4 sceneColor4 = getColor(id, pos);
      id = sceneColor4.w;
      sceneColor = swi3(sceneColor4,x,y,z);
        }
    
        rdRef = reflect(rd,sn);
        roRef = pos + rdRef*0.1f;
        
        float reflexion;
        
    // Shading
    col = shading(pos, rd, sn, sceneColor, id, reflexion);
    reflection = ID_GROUND==id && pos.y<0.2f ? 0.9f : (reflexion)*0.2f;

    // Fog
    col = _mix(col, swi3(colClouds,x,y,z), smoothstep(MAX_DIST-5.0f, MAX_DIST, tScene.x));
        col += (fract(rnd*289.0f + tScene.x*4001.0f) - 0.5f)*0.05f;
        float f = MAX_DIST*0.3f;
        float3 fogColor = to_float3(0.87f,0.85f,1);
        col = _mix( 0.2f*fogColor, col, _exp2f(-tScene.x*fogColor/f) );
    
    } else {
    col = swi3(colClouds,x,y,z);
  }

  return to_float4_aw(col, reflection);
}



__DEVICE__ const float
    a_eyeClose = 0.55f, 
    a_eyeOpen = -0.3f;




__KERNEL__ void MyDarkLandJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;

    if (iTime>31.0f) discard;
    
    gTime = iTime*8.0f+0.1f;
   
    rotHead = rot(0.6f*(_sinf(0.05f*gTime)*_cosf(0.001f*gTime+1.11f)));
        
    // Animation
    int it = int(_floor(gTime));
    float kt = fract(gTime);

#ifdef IS_RUNNING
//    drawHand = false;
    const bool isRunning = true; //_cosf(iTime*0.2f)>0.0f;;
#else
//    drawHand = true;
    const bool isRunning = false;
#endif

    // - init man position -----------------------------------
    
#ifdef IS_RUNNING
  //  if (isRunning) {
    initRunningPosition(it, kt);
  //  } else {
//    initWalkingPosition(it, kt);
  //  }
#else
  initLookingPosition(0);
#endif

//    const float t_openEye = 3.0f, t_rotDown = 10.0f, t_closeEye = 1.0f;
    // - Eye blink -------------------------------------------
    float time = iTime;
/*    float a_PaupieresCligne = _mix(a_eyeOpen,a_eyeClose, hash(_floor(time*10.0f))>.98?2.*_fabs(fract(20.0f*time)-0.5f):0.);    
    float a_Paupieres = _mix(a_eyeClose, 0.2f, smoothstep(t_openEye, t_openEye+3.0f, time));    
    a_Paupieres = _mix(a_Paupieres, a_PaupieresCligne, smoothstep(t_rotDown, t_rotDown+1.0f, time));
   // a_Paupieres = _mix(a_Paupieres, a_eyeClose, smoothstep(t_closeEye, t_closeEye+3.0f, time));
*/
    g_eyeRot = rot(-0.3f);
    
// Init base vectors --------------------------------------------

  // Foot1 flat part - vector base linked to leg 1
    v2Foot1 = normalize(knee1 - ankle1);
  float3 v1Foot1 = normalize(ankle1 - foot1-v2Foot1*0.1f);
  v3Foot1 = cross(v1Foot1,v2Foot1);
  v2Foot12 = -cross(v1Foot1, v3Foot1);

    v2Foot2 = normalize(knee2 - ankle2);
    float3 v1Foot2 = normalize(ankle2 - foot2-v2Foot2*0.1f);
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

    float2 m = iMouse.xy/iResolution.y - 0.5f;

  float traceStart = 0.2f;

    float3 ro, rd;
    float2 q;
    
// - Camera -----------------------------------------
    
  q = (fragCoord)/iResolution;
  float2 p = -1.0f + 2.0f*q;
  p.x *= iResolution.x/iResolution.y;

    time = iTime - 35.0f;
    
    float3 ta = to_float3(_mix(hip1.x,gTime*0.58f,0.75f), 1.4f, 0.0f);
  ro = ta + 11.0f*to_float3(_cosf(-0.2f*time),0.07f,_sinf(-0.2f*time));
  ro.y = _fmaxf(0.01f, ro.y);

    // camera-to-world transformation
  mat3 ca = setCamera(ro, ta, 0.0f);

  // ray direction
  rd = ca * normalize( to_float3(swi2(p,x,y), 3.5f) );

// ---------------------------------------------------
    float3 roRef, rdRef;
  float4 col = render(ro, rd, roRef, rdRef);

// Post processing stuff --------------------

    // Teinte
  // swi3(col,x,y,z) = 0.2f*length(swi3(col,x,y,z))*to_float3(1,0.5f,0)+0.8f*swi3(col,x,y,z);

  // Gamma
     swi3(col,x,y,z) = _powf(swi3(col,x,y,z), to_float3_s(0.6545f) );

    // Vigneting
    swi3(col,x,y,z) *= _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.15f); 
    
  fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'https://soundcloud.com/tavi230/armand-amar-inanna' to iChannel3




__KERNEL__ void MyDarkLandJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;  

    fragColor = _mix(texture(iChannel0, fragCoord/iResolution),
                    texture(iChannel1, fragCoord/iResolution), smoothstep(31.0f, 30.0f, iTime) /* smoothstep(200.0f, 199.0f, iTime)*/);


  SetFragmentShaderComputedColor(fragColor);
}