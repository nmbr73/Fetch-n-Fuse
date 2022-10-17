
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


/*

  Polyhedral Gears
  ----------------

  Interlocked animated gears mapped onto the surface of a sphere via a polyhedral 
  Goldberg arrangement. I've seen a few of these in static and gif form, and have
  always wanted to produce one in a pixel shader.

    The GPU compiler genie would disagree, but I was pleasantly surprized by how easily 
  this came together. I was expecting it to be difficult, considering that everything 
  would have to be tiled in a kind of geodesic-related fashion using cheap -- but more 
  than likely, confusing -- folded coordinates.

    I had a couple of images with no code nor explanation, so had to wing it. It was
  only after producing a working example that I combed through the polyhedral 
    classifications on Wikipedia and found that the cogs are rendered in an icosahedral 
  (Goldberg) arrangement. A Goldberg polyhedron is a dual of a geodesic sphere, which 
  is why the pattern looks similar. In particular, this Goldberg polyhedron has a 
  GP(3, 0) classification, which means there are 92 faces, 180 vertices and 270 edges. 
  That seems about right, considering there are 12 pentagons, 20 large hexagons and 60 
  smaller hexagons spread over the surface.

    Not being aware of the specifics meant that I simply rendered some cogs at the 
  appropriate positions on an icosahedron and dodecahedron, interspersed a couple of 
  smaller cogs within the confines of the icosahedral triangle face, then crossed my 
  fingers and hoped things magically lined up, which they did... I guess it was my 
  lucky day. :D

  I also noticed after combing the net for a while that a lot of the imagery originates 
  from Paul Nylander's site, "Bugman123.com." For anyone not familiar with it, it's 
  well worth the visit. When I get time, I'd like to produce a few other things from 
  there.

  Apologies in advance for the lower frame rate on slower machines in general, and 
  probably in fullscreen on a lot of machines, but the detail was fiddly to code.  I 
  won't bore you with the details, suffice to say that folded polyhedral cells and 
    detailed moving cogs don't enjoy one another's company. Apologies for the compile 
  time too... Having said that, even though the distance function could do with some 
  streamlining, I don't feel that it's complicated enough to warrant a ten year compile 
  time, so I feel whoever updated WebGL should be doing most of the apologizing. :)

  By the way, I'll put together a cleaner, more simplistic example later for anyone 
  who'd like to make one of these, but doesn't wish to comb through a bunch of 
  esoteric code.

  
  // Based on:
  
    // Not the easiest of geometry to wrap one's head around at the best of times, and 
  // from what I understand, DjinnKahn (Tom Sirgedas) was learning about shaders and 
  // SDF at the same time. Quite amazing.
    Icosahedron Weave - DjinnKahn
  https://www.shadertoy.com/view/Xty3Dy

  Other examples:

  // Knighty is more comfortable folding space than most. I fold space about 
    // as well as I fold laundry. :)
  Polyhedron again - knighty
  https://www.shadertoy.com/view/XlX3zB

    // Tdhooper has some awesome icosahedral examples.
    Icosahedron twist - tdhooper
  https://www.shadertoy.com/view/Mtc3RX

*/

#define FAR 20.0f

// Color scheme: 0, 1 or 2 -> Gold and aluminium, black and chrome, pink and chrome.
#define COLOR_SCHEME 0

// 2D rotation formula.
__DEVICE__ mat2 rot2(float a){ float c = _cosf(a), s = _sinf(a); return to_mat2(c, s, -s, c); }

// I paid hommage to the original and kept the same rotation... OK, I'm lazy. :D
__DEVICE__ float3 rotObj(float3 p, float iTime){
    
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot2(iTime*0.2f/2.0f));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot2(iTime*0.5f/2.0f));
    return p;  
    
}


// More concise, self contained version of IQ's original 3D noise function.
__DEVICE__ float noise3D(in float3 p){
    
    // Just some random figures, analogous to stride. You can change this, if you want.
  const float3 s = to_float3(113, 157, 1);
  
  float3 ip = _floor(p); // Unique unit cell ID.
    
  // Setting up the stride vector for randomization and interpolation, kind of. 
  // All kinds of shortcuts are taken here. Refer to IQ's original formula.
  float4 h = to_float4(0.0f, s.y, s.z, s.y + s.z) + dot(ip, s);
    
  p -= ip; // Cell's fractional component.
  
  // A bit of cubic smoothing, to give the noise that rounded look.
  p = p*p*(3.0f - 2.0f*p);
  
  // Standard 3D noise stuff. Retrieving 8 random scalar values for each cube corner,
  // then interpolating along X. There are countless ways to randomize, but this is
  // the way most are familar with: fract(_sinf(x)*largeNumber).
  h = _mix(fract(sin_f4(h)*43758.5453f), fract_f4(_sinf(h + s.x)*43758.5453f), p.x);

  // Interpolating along Y.
  swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y);
  
  // Interpolating along Z, and returning the 3D noise value.
  float n = _mix(h.x, h.y, p.z); // Range: [0, 1].

  return n;//_fabs(n - 0.5f)*2.0f;
}

// Simple fBm to produce some clouds.
__DEVICE__ float fbm(in float3 p){
    
    // Four layers of 3D noise.
    return 0.5333f*noise3D( p ) + 0.2667f*noise3D( p*2.02f ) + 0.1333f*noise3D( p*4.03f ) + 0.0667f*noise3D( p*8.03f );

}

// There's a neat way to construct an icosohedron using three mutually perpendicular rectangular 
// planes. If you reference something along the lines of icosahedron golden rectangle, you'll 
// get a pretty good idea. There's a standard image here:
// https://math.stackexchange.com/questions/2538184/proof-of-golden-rectangle-inside-an-icosahedron
//
// Anyway, even a cursory glance will give you a fair idea where the figures below originate. In 
// a vertex\indice list environment, you could produce an icosahedron without too much trouble at 
// all. However, in a realtime raymarching situation, you need to get to the triangle face 
// information in as few operations as possible. That's achieved via a bit of space folding using 
// the same information in a different way.
//
// If weaving wasn't necessary, you could use the concise "opIcosahedron" function and be done
// with it. Unfortunately, the "abs" calls throw out the triangular polarity information, which
// you need to distinguish one side of the triangle from the other -- I wasted a lot of time not
// realizing this until Djinn Kahn posted his example. He rewrote the folding function with an
// additional variable to track polarity (signs) during each iteration.
//
// With this function, you can obtain the triangle face information and use it to render in the
// three regions of symmetry -- each with a left and right X axis. From there, you can do whatever 
// you wish. 

// Vertices: to_float3(0, A, B), to_float3(B, 0, A), to_float3(-B, 0, A).
// Face center: (to_float3(0, A, B) + to_float3(0, 0, A)*2.0f)/3..
// Edges: (to_float3(0, A, B) + to_float3(B, 0, A))/2.0f,  etc.


// The following have come from DjinnKahn's "Icosahedron Weave" example, here:
// https://www.shadertoy.com/view/Xty3Dy
//
// It works fine, just the way it is, so I only made trivial changes. I'd like to cut down the
// number of operations in the "opIcosahedronWithPolarity" function, but so far, I can't see
// a way to do that.

const float PHI = (1.0f + _sqrtf(5.0f))/2.0f;
const float A = PHI/_sqrtf(1.0f + PHI*PHI);
const float B = 1.0f/_sqrtf( 1.0f + PHI*PHI);
const float J = (PHI - 1.0f)/2.0f; // 0.309016994375f;
const float K = PHI/2.0f;        //J + 0.5f;

const mat3 R0 = mat3(0.5f,  -K,   J   ,K ,  J, -0.5f   ,J , 0.5f,  K);
const mat3 R1 = mat3( K,   J, -0.5f   ,J , 0.5f,   K   ,0.5f ,-K,  J);
const mat3 R2 = mat3(-J, -0.5f,   K  ,0.5f , -K,  -J   ,K ,  J, 0.5f);
// A handy matrix that rotates icosahedral vertices into the dual dodecahedron postions. 
const mat3 R4 = mat3(0.587785252292f, -K, 0, -0.425325404176f, -J, 0.850650808352f, 0.688190960236f, 0.5f, 0.525731112119f);


// I wanted all vertices hardcoded.
const float3 v0 = to_float3(0, A, B);
const float3 v1 = to_float3(B, 0, A);
const float3 v2 = to_float3(-B, 0, A);
const float3 mid = (to_float3(0,A,B) + to_float3(0,0,A)*2.0f)/3.0f; // (v0 + v1 + v2)/3.0f*size.


/*
// I could make these constant and hardcode them, but it's so messy... I guess I could get my calculator
// out, but life's too short and this is just a shader. :)
mat3 basisHex, basisHexSm1, basisHexSm2;

// A cheap orthonormal basis vector function - Taken from Nimitz's "Cheap Orthonormal Basis" example, then 
// modified slightly.
//
//Cheap orthonormal basis by nimitz
//http://orbit.dtu.dk/fedora/objects/orbit:113874/datastreams/file_75b66578-222e-4c7d-abdf-f7e255100209/content
//via: http://psgraphics.blogspot.pt/2014/11/making-orthonormal-basis-from-unit.html
__DEVICE__ mat3 basis(in float3 n){
    
    float a = 1.0f/(1.0f + n.z);
    float b = -n.x*n.y*a;
    return mat3(1.0f - n.x*n.x*a, b, n.x, b, 1.0f - n.y*n.y*a, n.y, -n.x, -n.y, n.z);
                
}

// Precalculating some face normals for various vertex positions, then producing the basis
// matrix. Precalculation saves performing several loop calculations over and over. However,
// this is at the expense of inline readability and variables. GPUs don't like globals... 
// Damned if you do, damned if you don't. :) This example needs more speed, so I might 
// hardcode the following into constants -- or something -- at a later stage.
__DEVICE__ void initBases(){
    
  const float3 hexN = normalize(cross((mid - v0), (mid - v1)));

    basisHex = basis(hexN);
    basisHexSm1 = basis(normalize(_mix(v0, v2, 0.333f)));
    basisHexSm2 = basis(normalize(_mix(v0, v1, 0.333f)));
    
}
*/

// Cyberjax suggested the following to replace the above with constants, which, 
// in theory, should run faster:
#define a(n) (1.0f/(1.0f + n.z))
#define b(n) (-n.x*n.y*a(n))
#define basis(n) mat3(1.0f - n.x*n.x*a(n), b(n), n.x, b(n), 1.0f - n.y*n.y*a(n), n.y, -n.x,-n.y,n.z)           

const float3 hexN = normalize(cross((mid - v0), (mid - v1)));
const float3 n1 = normalize(_mix(v0, v2, 0.333f));
const float3 n2 = normalize(_mix(v0, v1, 0.333f));

const mat3 basisHex = basis(hexN);
const mat3 basisHexSm1 = basis(n1);
const mat3 basisHexSm2 = basis(n2);

// Same as opIcosahedron, except without mirroring symmetry, so X-coordinate may be negative.
// (note: when this is used as a distance function, it's possible that the nearest object is
// on the opposite polarity, potentially causing a glitch).
__DEVICE__ float3 opIcosahedronWithPolarity(in float3 p){
   
  float3 pol = sign_f3(p);
    p = R0*abs_3(p);
  pol *= sign_f3(p);
    p = R1*abs_f3(p);
  pol *= sign_f3(p);
    p = R2*abs_f3(p);
  pol *= sign_f3(p);
    float3 ret = abs_f3(p);
    return ret * to_float3(pol.x*pol.y*pol.z, 1, 1);
}   

/*
// The original function -- sans polarity information -- is neat and concise.
__DEVICE__ float3 opIcosahedron(float3 p){ 
  
    p = R0*_fabs(p);
    p = R1*_fabs(p);
    p = R2*_fabs(p);
    return _fabs(p);  
} 
*/


// An object ID container.    
float4 objID;
// Other global IDs. These are slower as globals, but easier to pass around.
float spokes, sph;


// Three distance fields for each of the three different kinds of gears. I could role them into
// one larger, and much messier function, but I prefer it this way. I'll comment them more 
// thoroughly later, but in short, it's just a bunch of fiddly calls to create the gear's objects. 
// There's some cyclinder-like calls, and some repeat radial boxish calls for the spokes, etc. 
// Standard stuff. As always, a lot of these functions are hacky bounds to save some operations, 
// but they produce very similar results.
//
// The math and physics of the situation is less complicated than you'd think. The radii of the 
// gears have a ratio of 18:15:12. Since the radius and circumference is directly proportional, 
// this means the cog teeth ratio would be the same. The scalar rotational speed would be inversely 
// proportional -- Smaller wheels need to move faster to keep up with the larger wheel arcs. This
// means time needs to be in a ratio of: 1.0f/18: 1.0f/15.0f : 1.0f/12.0f You can see that implemented
// below.
//
// The only annoyance would be that when animating across the boundaries of the underlying 
// icosahedral structure -- used to position the gears, it's triangle based structure needs to be 
// considered. This means working with the number 3 and wrapping things... I hacked the wrapping 
// (the "mod_f(iTime/1.5" lines) in a hurry to get things working, but I'll take a closer look later 
// to see if there's a more elegant solution.

// The pentagonal gear\cog..
__DEVICE__ float dist(float3 p, float r1, float r2, float iTime){
    
    // Using inverse time values, and wrapping the polar angles to cooincide with
    // the underlying icosaheral segments.
    //swi2(p,x,y) *= rot2(mod_f(iTime/1.5f, 3.0f*6.2831f/15.0f) + 6.2831f/2.0f);
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot2(mod_f(iTime/1.5f, 6.2831f/5.0f) + 3.14159265f));
    
    float a = _atan2f(p.y, _fabs(p.x)); // Note the "abs" call.
    
    // The outer gear radius needs to taper as it approaches the center to avoid
    // overlap.
    r1 *= (0.9f + p.z);
    
    // The outer cog teeth. Basically, we're applying the repeat polar space thing.
    float3 q = p; 
    float ia = _floor(a/6.2831f*15.0f) + 0.5f;
    swi2S(q,x,y, mul_mat2_f2(rot2(ia*6.2831f/15.0f) , swi2(q,x,y)); // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - 0.03f/3.0f; // Move the radial coordinate out to the edge of the rim.
    q = abs_f3(q);
    float spike = _mix(_fmaxf(q.x - 0.05f, q.y - 0.02f), length(swi2(q,x,y)*to_float2(0.7f, 1)) - 0.025f, 0.5f);
    float d2 = _fmaxf(spike, q.z - r2);
   
    // The inner spokes.
    q = p; 
    ia = _floor(a/6.2831f*5.0f) + 0.5f;
    swi2S(q,x,y, mul_mat2_f2(rot2(ia*6.2831f/5.0f) , swi2(q,x,y));
    sph = _fmaxf(max(_fabs(q.x), _fabs(q.y)) - 0.0225f, _fabs(q.z - 0.07f) - 0.06f);
    //sph = _fmaxf(length(swi2(q,x,y)) - 0.0225f, _fabs(q.z - 0.07f) - 0.06f);
    q = _fabs(q + to_float3(r1/2.0f, 0, -0.07f));
    //spokes = _fmaxf(max(q.x - r1/2.0f + 0.02f, q.y - 0.03f), q.z - 0.01f);
    spokes = _fmaxf(q.x - r1/2.0f + 0.02f, _fmaxf(max(q.y - 0.05f, q.z - 0.015f), (q.y + q.z)*0.7071f - 0.025f));
    
    // The inner cylindrical cog bit.
    p = abs_f3(p);
    float d = length(swi2(p,x,y));
    float di = _fabs(d  -  r1 + 0.1f/2.0f) - 0.05f/2.0f;
    d = _fabs(d  -  r1 + 0.075f/2.0f) - 0.075f/2.0f;
    d = _fminf(_fmaxf(d , p.z - r2), _fmaxf(di , p.z - r2 - 0.01f));
    
    // Return the minimum of the inner cog and teeth. The inner spokes have been kept global
    // for ID purposes and isn't include here... Yeah, messy, but this is a fiddly example
    // to work with. :)
    return _fminf(d, d2);
    
}

// The larger hexagonal gear\cog.
__DEVICE__ float dist2(float3 p, float r1, float r2, float iTime){
    // See the comments in the "dist" function.
    
    ///swi2(p,x,y) *= rot2(mod_f(iTime/1.8f, 3.0f*6.2831f/18.0f) + 3.0f*6.2831f/18.0f);
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot2(mod_f(iTime/1.8f, 6.2831f/6.0f) + 6.2831f/6.0f)));
    
    float a = _atan2f(p.y, _fabs(p.x)); // Note the "abs" call.
    
    p.z = -p.z;
    
    r1 *= (0.9f + p.z);
    
    float3 q = p; 
    
    float ia = _floor(a/6.2831f*18.0f) + 0.5f;
    swi2S(q,x,y, mul_mat2_f2( rot2(ia*6.2831f/18.0f) , swi2(q,x,y)); // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - 0.03f/3.0f; // Move the radial coordinate out to the edge of the rim.
    q = abs_f3(q);
    float spike = _mix(_fmaxf(q.x - 0.05f, q.y - 0.02f), length(swi2(q,x,y)*to_float2(0.7f, 1)) - 0.025f, 0.5f);
    float d2 = _fmaxf(spike, q.z - r2);
    
    q = p; 
    ia = _floor(a/6.2831f*6.0f) + 0.5f;
    swi2S(q,x,y, mul_f2_mat2(rot2(ia*6.2831f/6.0f) , swi2(q,x,y));
    sph = _fmaxf(max(_fabs(q.x), _fabs(q.y)) - 0.0275f, _fabs(q.z - 0.07f) - 0.06f);
    //sph = _fmaxf(length(swi2(q,x,y)) - 0.0275f, _fabs(q.z - 0.07f) - 0.06f);
    q = _fabs(q + to_float3(r1/2.0f, 0, -0.07f));
    //spokes = _fmaxf(max(q.x - r1/2.0f + 0.02f, q.y - 0.03f), q.z - 0.01f);
    spokes = _fmaxf(q.x - r1/2.0f + 0.02f, _fmaxf(max(q.y - 0.05f, q.z - 0.015f), (q.y + q.z)*0.7071f - 0.03f));
    
    p = abs_f3(p);
    float d = length(swi2(p,x,y));
    float di = _fabs(d  -  r1 + 0.1f/2.0f) - 0.05f/2.0f;
    d = _fabs(d  -  r1 + 0.075f/2.0f) - 0.075f/2.0f;
    d = _fminf(max(d , p.z - r2), _fmaxf(di , p.z - r2 - 0.0115f));
    
    return _fminf(d, d2);
}

// The smaller hexagonal gear\cog.
__DEVICE__ float dist3(float3 p, float r1, float r2, float iTime){
    
    // See the comments in the "dist" function.
    
    // This is the same as the two functions above, but the smaller cogs cross a boundary line
    // and directional polarity has to be considered.
    float dir = p.x < 0.? -1.0f : 1.0f;
    
    p.x = _fabs(p.x);
    float3 q2 = p;
    swi2S(p,x,y, mul_mat2_f2(swi2(p,x,y) , rot2(mod_f(iTime/1.2f*dir + 3.14159f/12.0f, 6.2831f/12.0f) + 5.0f*6.2831f/12.0f))); // + 5.0f*3.14159f/12.
    
    float a = _atan2f(p.y, _fabs(p.x)); // Note the "abs" call.
    
    r1 *= (0.9f + p.z);
    
    float3 q = p;
   
    float ia = _floor(a/6.2831f*12.0f) + 0.5f;
    swi2S(q,x,y, mul_mat2_f2( rot2(ia*6.2831f/12.0f) , swi2(q,x,y))); // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - 0.03f/3.0f; // Move the radial coordinate out to the edge of the rim.
    swi2S(q,x,y, _fabs(swi2(q,x,y)));
    float spike = _mix(_fmaxf(q.x - 0.05f, q.y - 0.02f), length(swi2(q,x,y)*to_float2(0.7f, 1)) - 0.025f, 0.5f);
    float d2 = _fmaxf(spike, _fabs(q.z) - r2);

    
    q = q2; 
    swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , rot2(mod_f(iTime/1.2f*dir, 6.2831f/6.0f) + 2.0f*6.2831f/6.0f));
    a = _atan2f(q.y, _fabs(q.x));
    ia = _floor(a/6.2831f*6.0f) + 0.5f;
    swi2S(q,x,y, mul_mat2_f2(rot2(ia*6.2831f/6.0f) , swi2(q,x,y)));
    sph = _fmaxf(max(_fabs(q.x), _fabs(q.y)) - 0.02f, _fabs(q.z - 0.07f) - 0.06f);
    //sph = _fmaxf(length(swi2(q,x,y)) - 0.02f, _fabs(q.z - 0.07f) - 0.06f);
    q = abs_f3(q + to_float3(r1/2.0f, 0, -0.07f));
    spokes = _fmaxf(q.x - r1/2.0f + 0.02f, _fmaxf(max(q.y - 0.04f, q.z - 0.015f), (q.y + q.z)*0.7071f - 0.02f));
    
    
    p = abs_f3(p);
    float d = length(swi2(p,x,y));
    float di = _fabs(d  -  r1 + 0.1f/2.0f) - 0.05f/2.0f;
    d = _fabs(d  -  r1 + 0.075f/2.0f) - 0.075f/2.0f;
    d = _fminf(max(d , p.z - r2), _fmaxf(di , p.z - r2 - 0.007f));
    
    return _fminf(d, d2);
    
}

//Raytraced sphere hit variable. 
//float balHit; 

__DEVICE__ float map(in float3 p){
   
    // Back plane. Six units behind the center of the weaved object.
    float pln = -p.z + 6.0f;
    
/*  
    // Raytraced sphere hit variable. It saves extra calculation, but
    // complicates things. Plus, it can interfere with shadows, add to 
    // the compile time... It saves a lot of pixel calculations though, 
    // especially in fullscreen... I'll have a think about it. :)
    if(balHit < 0.0f){
        
        objID = to_float4(1e5, 1e5, 1e5, pln);
        return pln;
        
    }
*/   
    
    // Rotate the object.
    p = rotObj(p);
    //swi2(p,y,z) *= rot2(-3.14159f/6.0f);
    
    float3 oP = p;
    
    float d = 1e5, d2 = 1e5, d3 = 1e5;
    
   
    // DjinnKahn's icosahedral distance function that produces a triangular face
    // and allows you to determine between the negative and positive X axis.
    
    // Large hexagonal face positions.
    float3 hexFace = opIcosahedronWithPolarity(p); 
    
    // Rotating the points above to the dual pentagonal face positions. This saves
    // a lot of extra operations. We're able to do this because of the icosahedron
    // and dodecahedron duality.
    float3 pentFace = R4*hexFace; 
    
    // Pentagon.
    float3 p1 = (pentFace - to_float3(0, 0, 1));
    d3 = _fminf(d3, dist( p1, 0.185f, 0.1f) );
    d = _fminf(d, spokes);
    d3 = _fminf(d3, sph);
    
    // Hexagon. 
    p1 = basisHex*(hexFace - mid*1.2425f);
    d3 = _fminf( d3, dist2( p1, 0.25f, 0.1f) );
    d = _fminf(d, spokes);
    d3 = _fminf(d3, sph);
    
    // Small cogs.
    p1 = basisHexSm1*(hexFace - _mix(v0, v2, 0.333f)*1.1547f);
    d2 = _fminf( d2, dist3( p1, 0.16f, 0.1f) );
    d = _fminf(d, spokes);
    d3 = _fminf(d3, sph);
    p1 = basisHexSm2*(hexFace - _mix(v0, v1, 0.333f)*1.1547f);
    d2 = _fminf( d2, dist3( p1, 0.16f, 0.1f) );
    d = _fminf(d, spokes);
    d3 = _fminf(d3, sph);
   
    // Capping off the edges of the gears with the outer sphere itself.
    float mainSph = length(oP);
    d = _fmaxf(d, mainSph - 1.0825f);
    d2 = _fmaxf(d2, mainSph - 1.116f);
    d3 = _fmaxf(d3, mainSph - 1.118f);
     
    // Store the individual object values for sorting later. Sorting multiple objects
    // inside a raymarching loop probably isn't the best idea. :)
    objID = to_float4(d, d2, d3, pln);
    
    return _fminf(min(d, d2), _fminf(d3, pln));
}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ. In instances where there's no descernible 
// aesthetic difference between it and the six tap version, it's worth using.
__DEVICE__ float3 calcNormal(in float3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    float2 e = to_float2(0.0025f, -0.0025f); 
    return normalize(swi3(e,x,y,y) * map(p + swi3(e,x,y,y)) + swi3(e,y,y,x) * map(p + swi3(e,y,y,x)) + swi3(e,y,x,y) * map(p + swi3(e,y,x,y)) + swi3(e,x,x,x) * map(p + swi3(e,x,x,x)));
}
*/

/*
// Standard normal function. 6 taps.
__DEVICE__ float3 calcNormal(in float3 p) {
  const float2 e = to_float2(0.002f, 0);
  return normalize(to_float3(map(p + swi3(e,x,y,y)) - map(p - swi3(e,x,y,y)), map(p + swi3(e,y,x,y)) - map(p - swi3(e,y,x,y)),  map(p + swi3(e,y,y,x)) - map(p - swi3(e,y,y,x))));
}
*/

/*
// Normal calculation, with some edging and curvature bundled in.
__DEVICE__ float3 calcNormal(float3 p, inout float edge, inout float crv, float t) { 
  
    // It's worth looking into using a fixed epsilon versus using an epsilon value that
    // varies with resolution. Each affects the look in different ways. Here, I'm using
    // a mixture. I want the lines to be thicker at larger resolutions, but not too thick.
    // As for accounting for PPI; There's not a lot I can do about that.
    float2 e = to_float2(3.0f/_mix(450.0f, _fminf(850.0f, iResolution.y), 0.35f), 0);//*(1.0f + t*t*0.7f);

  float d1 = map(p + swi3(e,x,y,y)), d2 = map(p - swi3(e,x,y,y));
  float d3 = map(p + swi3(e,y,x,y)), d4 = map(p - swi3(e,y,x,y));
  float d5 = map(p + swi3(e,y,y,x)), d6 = map(p - swi3(e,y,y,x));
  float d = map(p)*2.0f;

    edge = _fabs(d1 + d2 - d) + _fabs(d3 + d4 - d) + _fabs(d5 + d6 - d);
    //edge = _fabs(d1 + d2 + d3 + d4 + d5 + d6 - d*3.0f);
    edge = smoothstep(0.0f, 1.0f, _sqrtf(edge/e.x*2.0f));
     
    // Wider sample spread for the curvature.
    //e = to_float2(12.0f/450.0f, 0);
  //d1 = map(p + swi3(e,x,y,y)), d2 = map(p - swi3(e,x,y,y));
  //d3 = map(p + swi3(e,y,x,y)), d4 = map(p - swi3(e,y,x,y));
  //d5 = map(p + swi3(e,y,y,x)), d6 = map(p - swi3(e,y,y,x));
    //crv = clamp((d1 + d2 + d3 + d4 + d5 + d6 - d*3.0f)*32.0f + 0.5f, 0.0f, 1.0f);
 
    
    e = to_float2(0.001f, 0); //iResolution.y - Depending how you want different resolutions to look.
  d1 = map(p + swi3(e,x,y,y)), d2 = map(p - swi3(e,x,y,y));
  d3 = map(p + swi3(e,y,x,y)), d4 = map(p - swi3(e,y,x,y));
  d5 = map(p + swi3(e,y,y,x)), d6 = map(p - swi3(e,y,y,x));
  
    return normalize(to_float3(d1 - d2, d3 - d4, d5 - d6));
}
*/

// IQ rewrote the function above, and for reasons that defy my own sense 
// of logic, has cut the compile time down by at least half. For whatever
// reason, restricting an unroll in the larger iteration "trace" function
// has virtually no effect on compile time, yet doing the same with the
// fewer "map" calls in the following function does... I'll leave the
// explanation for that conundrum to the GPU instruction experts. :)
//
__DEVICE__ float3 calcNormal(float3 p, inout float edge, inout float crv, float t) { 
    
    float eps = 3.0f/_mix(450.0f, _fminf(850.0f, iResolution.y), 0.35f);

    float d = map(p);
    
    float3 e = to_float3(eps, 0, 0);
    
    float3 da = to_float3_s(-2.0f*d);
    //for( int i = _fminf(iFrame,0); i<3; i++ )
      for( int i = 0; i<3; i++ )
    {
        //for( int j=_fminf(iFrame,0); j<2; j++ )
          for( int j= 0; j<2; j++ )
            da[i] += map(p + e*(float)(1-2*j));
        e = swi3(e,z,x,y);
    }
    da = abs_f3(da);
    
    edge = da.x + da.y + da.z;
    edge = smoothstep(0.0f, 1.0f, _sqrtf(edge/e.x*2.0f));
    
    float3 n = to_float3_s(0.0f);
    //for( int i=_fminf(iFrame, 0); i<4; i++ )
      for( int i= 0; i<4; i++ )
    {
        float3 e = 0.57735f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.0f);
        n += e*map(p + 0.001f*e);
    }
    
    return normalize(n);
}

// Raymarching: The distance function is a little on the intensive side, so I'm 
// using as fewer iterations as necessary. Even though there's a breat, the compiler
// still has to unroll everything, and larger numbers make a difference.
__DEVICE__ float trace(in float3 ro, in float3 rd){
    
    float t = 0.0f, d;
    
    for(int i = 0; i<64; i++){
    
        d = map(ro + rd*t);
        if(_fabs(d)<0.001f*(1.0f + t*0.05f) || t > FAR) break;
        t += d;
    }
    
    return _fminf(t, FAR);
}

/*
__DEVICE__ float hash( float n ){ return fract(_cosf(n)*45758.5453f); }

// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function and have been looking for an excuse to use it. For a better version, and usage, 
// refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
__DEVICE__ float calculateAO( in float3 p, in float3 n, float maxDist )
{
  float ao = 0.0f, l;
  const float nbIte = 6.0f;
  //const float falloff = 0.9f;
    for( float i=1.0f; i< nbIte + 0.5f; i++ ){
    
        l = (i + hash(i))*0.5f/nbIte*maxDist;
        ao += (l - map( p + n*l ))/(1.0f+ l);// / _powf(1.0f+l, falloff);
    }
  
    return clamp( 1.0f-ao/nbIte, 0.0f, 1.0f);
}
*/

// Ambient occlusion, for that self shadowed look.
// Based on the original by IQ.
__DEVICE__ float calcAO(in float3 p, in float3 n)
{
    float sca = 4.0f, occ = 0.0f;
    for( int i=1; i<6; i++ ){
    
        float hr = (float)(i)*0.125f/5.0f;        
        float dd = map(p + hr*n);
        occ += (hr - dd)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - occ, 0.0f, 1.0f);   
    
}

// The iterations should be higher for proper accuracy.
__DEVICE__ float softShadow(in float3 ro, in float3 rd, float t, in float end, in float k){

    float shade = 1.0f;
    // Increase this and the shadows will be more accurate, but more iterations slow things down.
    const int maxIterationsShad = 24; 

    // The "start" value, or minimum, should be set to something more than the stop-threshold, so as to avoid a collision with 
    // the surface the ray is setting out from. It doesn't matter how many times I write shadow code, I always seem to forget this.
    // If adding shadows seems to make everything look dark, that tends to be the problem.
    float dist = 0.001f*(1.0f + t*0.05f);
    float stepDist = end/(float)(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i = 0; i<maxIterationsShad; i++){
        // End, or maximum, should be set to the distance from the light to surface point. If you go beyond that
        // you may hit a surface not between the surface and the light.
        float h = map(ro + rd*dist);
        shade = _fminf(shade, k*h/dist);
        //shade = _fminf(shade, smoothstep(0.0f, 1.0f, k*h/dist));
        
        // What h combination you add to the distance depends on speed, accuracy, etc. To be honest, I find it impossible to find 
        // the perfect balance. Faster GPUs give you more options, because more shadow iterations always produce better results.
        // Anyway, here's some posibilities. Which one you use, depends on the situation:
        // +=_fmaxf(h, 0.001f), +=clamp( h, 0.01f, 0.25f ), +=_fminf( h, 0.1f ), +=stepDist, +=_fminf(h, stepDist*2.0f), etc.
        
        dist += clamp(h, 0.01f, 0.25f);
        
        // There's some accuracy loss involved, but early exits from accumulative distance functions can help.
        if (h<0.0001f || dist > end) break; 
    }

    // Return the shadow value. Note that I'm not using clamp. The shadow value should be capped to zero
    // prior to adding a constant... Unless you're not adding a constant. :)
    return _fminf(max(shade, 0.0f) + 0.1f, 1.0f); 
}


// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it 
// does give the impression that the surface is reflecting the surrounds in some way.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
__DEVICE__ float3 envMap(float3 p){
    
    p *= 3.0f;
    
    float n3D2 = noise3D(p*3.0f);
   
    // A bit of fBm.
    float c = noise3D(p)*0.57f + noise3D(p*2.0f)*0.28f + noise3D(p*4.0f)*0.15f;
    c = smoothstep(0.25f, 1.0f, c); // Putting in some dark space.
    
    p = to_float3(c, c*c, c*c*c); // Bluish tinge.
    
    return _mix(p, swi3(p,z,y,x), n3D2*0.5f + 0.5f); // Mixing in a bit of purple.

}

/*
// Intersection of a sphere. IQ's formula - trimmed down a little.
__DEVICE__ float traceSphere( in float3 ro, in float3 rd, in float4 sph ){

  ro -= swi3(sph,x,y,z);
  float b = dot(ro, rd);
  float h = b*b - dot(ro, ro) + sph.w*sph.w;
    return h<0.0f ? -1.0f : -b - _sqrtf( h );
}
*/

__KERNEL__ void PolyhedralGearsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame)
{

    // Aspect correct screen coordinates. Restricting the size of the
    // fullscreen object to maintain a little detail on larger screens.
    // A consequence is a larger background. By the way, this was coded
    // for the 800 by 450 canvas or smaller, rather than fullscreen.
    float2 p = (fragCoord - iResolution*0.5f)/_fminf(850.0f, iResolution.y);
    
    // Unit direction ray.
    float3 rd = normalize(to_float3_aw(p, 1));
    
    // Ray origin, doubling as the camera postion.
    float3 ro = to_float3(0, 0, -2.75f);
    
    // Light position. Near the camera.
    float3 lp = ro + to_float3(0.25f, 2, -0.1f);
    
    // Basic camera rotation.
    swi2S(rd,x,y, mul_f2_mat2swi2(rd,x,y) , rot2(_sinf(iTime/8.0f)*0.2f)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z) , rot2(_sinf(iTime/4.0f)*0.1f)));
    
    // Precalculating some basis matrix values.
    // Replaced with Cyberjax's suggestion.
    //initBases();
    
    //balHit = traceSphere(ro, rd, to_float4_aw(to_float3(0), 1.2f)); // Main object pixel flag.
    
    // Ray march.
    float t = trace(ro, rd);
    
    // Object identification: Back plane: 3, Golden joins: 2.0f, 
    // Ball joins: 1.0f, Silver pipes:  0.
    float svObjID = objID.x<objID.y && objID.x<objID.z && objID.x<objID.w? 0.: 
    objID.y<objID.z && objID.y<objID.w ? 1.0f : objID.z<objID.w? 2.0f : 3.0f;

    
    // Initiate the scene color zero.
    float3 sceneCol = to_float3_s(0);
    
    // Surface hit. Color it up.
    if(t < FAR){
    
        // Position.
        float3 pos = ro + rd*t;
        // Normal.
        //vec3 nor = calcNormal(pos);
        // Normal, plus edges and curvature. The latter isn't used.
        float edge = 0.0f, crv = 1.0f;
        float3 nor = calcNormal(pos, edge, crv, t);
        
        //vec3 rp = rotObj(pos);
        
        // Light direction vector.
        float3 li = lp - pos;
        float lDist = _fmaxf(length(li), 0.001f);
        li /= lDist;
        
        // Light falloff - attenuation.
        float atten = 1.0f/(1.0f + lDist*0.05f + lDist*lDist*0.025f);
        
        // Soft shadow and occlusion.
        float shd = softShadow(pos + nor*0.0015f, li, t, lDist, 8.0f); // Shadows.
        float ao = calcAO(pos, nor);
        
        
        float diff = _fmaxf(dot(li, nor), 0.0f); // Diffuse.
        float spec = _powf(_fmaxf(dot(reflect(-li, nor), -rd), 0.0f), 16.0f); // Specular.
        // Ramping up the diffuse. Sometimes, it can make things look more metallic.
        float od = diff;
        diff = _powf(diff, 4.0f)*2.0f; 
        
        
        float Schlick = _powf( 1.0f - _fmaxf(dot(rd, normalize(rd + li)), 0.0f), 5.0f);
        float fre2 = _mix(0.5f, 1.0f, Schlick);  //F0 = .5.
    
        // Spokes: ObjID == 0.0f;
        #if COLOR_SCHEME == 0
        float3 col = to_float3_s(0.6f); // Silver.
        #elif COLOR_SCHEME == 1
        float3 col = to_float3_s(0.1f); // Black.
        #else
        float3 col = to_float3(0.9f, 0.2f, 0.4f); // Pink.
        #endif

        
        if(svObjID == 1.0f) { // Smaller hexagon cogs.
            #if COLOR_SCHEME == 0
            col = to_float3(1, 0.5f, 0.2f)*0.7f; // Gold.
            #else 
            col = to_float3_s(0.6f); // Chrome.
            #endif
         }
        if(svObjID == 2.0f){ // Larger hexagon and pentagon cogs.
            
            #if COLOR_SCHEME == 0
            col = to_float3(1, 0.65f, 0.3f)*0.7f; // Gold-Copper
            #else 
            col = to_float3_s(0.6f); // Chrome.
            #endif
        }
        if(svObjID == 3.0f) { // Back plane.
            
            // Dark background color. Noise is applied afterward (See below).
            #if COLOR_SCHEME == 0
            col = to_float3(1, 0.7f, 0.4f)*0.045f; // Brown.
            #elif COLOR_SCHEME == 1
            col = to_float3_s(0.7f)*0.045f; // Charcoal.
            #else
            col = to_float3(0.6f, 0.7f, 1)*0.045f; // Blue.
            #endif
        }
        
        
        // Applying some subtle 3D fBm noise to the gear object and back plane to grunge
        // it up slightly. Technically, I should be keeping a copy of the individual gear
        // rotations and applying those, but the compiler is hating on this example enough
        // already, so I've lowered these noise intensity and added noisy environmental 
        // reflection so that the sliding UV effect is negigible.       
        float txSz = 1.0f;
        float3 txPos = pos; 
        if(svObjID == 3.0f) txSz /= 4.0f;
        else txPos = rotObj(txPos);
        col *= fbm(txPos*64.0f*txSz)*0.75f + 0.5f;
    
        
        // Diffuse plus ambient term.
        sceneCol = col*(diff + 0.25f); 
        
        // Specular term.
        if(svObjID == 3.0f) sceneCol += col*to_float3(1, 0.6f, 0.2f).zyx*spec*0.25f; // Less specular on the back plane.
        else sceneCol += col*to_float3(0.5f, 0.75f, 1.0f)*spec*2.0f;
        
        // Fake environment mapping.
        float envF = 4.0f;
        if(svObjID == 0.0f) envF = 8.0f;
        sceneCol += sceneCol*envMap(reflect(rd, nor))*envF;
        
        // Edges.
        sceneCol *= 1.0f - edge*0.8f;
        //col = col*0.7f + edge*0.3f;
        
        sceneCol *= atten*shd*ao; // Applying the light falloff, shadows and AO.
    }
    
    // Screen color. Rough gamma correction. No fog or postprocessing.
    fragColor = to_float4_aw(sqrt_f3(clamp(sceneCol, 0.0f, 1.0f)), 1);

  SetFragmentShaderComputedColor(fragColor);
}
