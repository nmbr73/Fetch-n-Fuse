

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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
	probably in fullscreen on a lot of machines, but the detail was fiddly to code.	I 
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

#define FAR 20.

// Color scheme: 0, 1 or 2 -> Gold and aluminium, black and chrome, pink and chrome.
#define COLOR_SCHEME 0

// 2D rotation formula.
mat2 rot2(float a){ float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }

// I paid hommage to the original and kept the same rotation... OK, I'm lazy. :D
vec3 rotObj(vec3 p){
    
    p.yz *= rot2(iTime*.2/2.);
    p.xz *= rot2(iTime*.5/2.);
    return p;  
    
}


// More concise, self contained version of IQ's original 3D noise function.
float noise3D(in vec3 p){
    
    // Just some random figures, analogous to stride. You can change this, if you want.
	const vec3 s = vec3(113, 157, 1);
	
	vec3 ip = floor(p); // Unique unit cell ID.
    
    // Setting up the stride vector for randomization and interpolation, kind of. 
    // All kinds of shortcuts are taken here. Refer to IQ's original formula.
    vec4 h = vec4(0., s.yz, s.y + s.z) + dot(ip, s);
    
	p -= ip; // Cell's fractional component.
	
    // A bit of cubic smoothing, to give the noise that rounded look.
    p = p*p*(3. - 2.*p);
    
    // Standard 3D noise stuff. Retrieving 8 random scalar values for each cube corner,
    // then interpolating along X. There are countless ways to randomize, but this is
    // the way most are familar with: fract(sin(x)*largeNumber).
    h = mix(fract(sin(h)*43758.5453), fract(sin(h + s.x)*43758.5453), p.x);
	
    // Interpolating along Y.
    h.xy = mix(h.xz, h.yw, p.y);
    
    // Interpolating along Z, and returning the 3D noise value.
    float n = mix(h.x, h.y, p.z); // Range: [0, 1].
	
    return n;//abs(n - .5)*2.;
}

// Simple fBm to produce some clouds.
float fbm(in vec3 p){
    
    // Four layers of 3D noise.
    return 0.5333*noise3D( p ) + 0.2667*noise3D( p*2.02 ) + 0.1333*noise3D( p*4.03 ) + 0.0667*noise3D( p*8.03 );

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

// Vertices: vec3(0, A, B), vec3(B, 0, A), vec3(-B, 0, A).
// Face center: (vec3(0, A, B) + vec3(0, 0, A)*2.)/3..
// Edges: (vec3(0, A, B) + vec3(B, 0, A))/2.,  etc.


// The following have come from DjinnKahn's "Icosahedron Weave" example, here:
// https://www.shadertoy.com/view/Xty3Dy
//
// It works fine, just the way it is, so I only made trivial changes. I'd like to cut down the
// number of operations in the "opIcosahedronWithPolarity" function, but so far, I can't see
// a way to do that.

const float PHI = (1. + sqrt(5.))/2.;
const float A = PHI/sqrt(1. + PHI*PHI);
const float B = 1./sqrt( 1. + PHI*PHI);
const float J = (PHI - 1.)/2.; // .309016994375;
const float K = PHI/2.;        //J + .5;
const mat3 R0 = mat3(.5,  -K,   J   ,K ,  J, -.5   ,J , .5,  K);
const mat3 R1 = mat3( K,   J, -.5   ,J , .5,   K   ,.5 ,-K,  J);
const mat3 R2 = mat3(-J, -.5,   K  ,.5 , -K,  -J   ,K ,  J, .5);
// A handy matrix that rotates icosahedral vertices into the dual dodecahedron postions. 
const mat3 R4 = mat3(.587785252292, -K, 0, -.425325404176, -J, .850650808352, .688190960236, .5, .525731112119);


// I wanted all vertices hardcoded.
const vec3 v0 = vec3(0, A, B);
const vec3 v1 = vec3(B, 0, A);
const vec3 v2 = vec3(-B, 0, A);
const vec3 mid = (vec3(0,A,B) + vec3(0,0,A)*2.)/3.; // (v0 + v1 + v2)/3.*size.


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
mat3 basis(in vec3 n){
    
    float a = 1./(1. + n.z);
    float b = -n.x*n.y*a;
    return mat3(1. - n.x*n.x*a, b, n.x, b, 1. - n.y*n.y*a, n.y, -n.x, -n.y, n.z);
                
}

// Precalculating some face normals for various vertex positions, then producing the basis
// matrix. Precalculation saves performing several loop calculations over and over. However,
// this is at the expense of inline readability and variables. GPUs don't like globals... 
// Damned if you do, damned if you don't. :) This example needs more speed, so I might 
// hardcode the following into constants -- or something -- at a later stage.
void initBases(){
    
	const vec3 hexN = normalize(cross((mid - v0), (mid - v1)));

    basisHex = basis(hexN);
    basisHexSm1 = basis(normalize(mix(v0, v2, .333)));
    basisHexSm2 = basis(normalize(mix(v0, v1, .333)));
    
}
*/

// Cyberjax suggested the following to replace the above with constants, which, 
// in theory, should run faster:
#define a(n) (1./(1. + n.z))
#define b(n) (-n.x*n.y*a(n))
#define basis(n) mat3(1. - n.x*n.x*a(n), b(n), n.x, b(n), 1. - n.y*n.y*a(n), n.y, -n.x,-n.y,n.z)           

const vec3 hexN = normalize(cross((mid - v0), (mid - v1)));
const vec3 n1 = normalize(mix(v0, v2, .333));
const vec3 n2 = normalize(mix(v0, v1, .333));

const mat3 basisHex = basis(hexN);
const mat3 basisHexSm1 = basis(n1);
const mat3 basisHexSm2 = basis(n2);

// Same as opIcosahedron, except without mirroring symmetry, so X-coordinate may be negative.
// (note: when this is used as a distance function, it's possible that the nearest object is
// on the opposite polarity, potentially causing a glitch).
vec3 opIcosahedronWithPolarity(in vec3 p){
   
	vec3 pol = sign(p);
    p = R0*abs(p);
	pol *= sign(p);
    p = R1*abs(p);
	pol *= sign(p);
    p = R2*abs(p);
	pol *= sign(p);
    vec3 ret = abs(p);
    return ret * vec3(pol.x*pol.y*pol.z, 1, 1);
}   

/*
// The original function -- sans polarity information -- is neat and concise.
vec3 opIcosahedron(vec3 p){ 
  
    p = R0*abs(p);
    p = R1*abs(p);
    p = R2*abs(p);
    return abs(p);  
} 
*/


// An object ID container.    
vec4 objID;
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
// means time needs to be in a ratio of: 1./18: 1./15. : 1./12. You can see that implemented
// below.
//
// The only annoyance would be that when animating across the boundaries of the underlying 
// icosahedral structure -- used to position the gears, it's triangle based structure needs to be 
// considered. This means working with the number 3 and wrapping things... I hacked the wrapping 
// (the "mod(iTime/1.5" lines) in a hurry to get things working, but I'll take a closer look later 
// to see if there's a more elegant solution.

// The pentagonal gear\cog..
float dist(vec3 p, float r1, float r2){
    
    // Using inverse time values, and wrapping the polar angles to cooincide with
    // the underlying icosaheral segments.
    //p.xy *= rot2(mod(iTime/1.5, 3.*6.2831/15.) + 6.2831/2.);
    p.xy *= rot2(mod(iTime/1.5, 6.2831/5.) + 3.14159265);
    
    float a = atan(p.y, abs(p.x)); // Note the "abs" call.
    
    // The outer gear radius needs to taper as it approaches the center to avoid
    // overlap.
    r1 *= (.9 + p.z);
    
    // The outer cog teeth. Basically, we're applying the repeat polar space thing.
    vec3 q = p; 
    float ia = floor(a/6.2831*15.) + .5;
    q.xy = rot2(ia*6.2831/15.)*q.xy; // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - .03/3.; // Move the radial coordinate out to the edge of the rim.
    q = abs(q);
    float spike = mix(max(q.x - .05, q.y - .02), length(q.xy*vec2(.7, 1)) - .025, .5);
    float d2 = max(spike, q.z - r2);
   
    // The inner spokes.
    q = p; 
    ia = floor(a/6.2831*5.) + .5;
    q.xy = rot2(ia*6.2831/5.)*q.xy;
    sph = max(max(abs(q.x), abs(q.y)) - .0225, abs(q.z - .07) - .06);
    //sph = max(length(q.xy) - .0225, abs(q.z - .07) - .06);
    q = abs(q + vec3(r1/2., 0, -.07));
    //spokes = max(max(q.x - r1/2. + .02, q.y - .03), q.z - .01);
    spokes = max(q.x - r1/2. + .02, max(max(q.y - .05, q.z - .015), (q.y + q.z)*.7071 - .025));
    
    // The inner cylindrical cog bit.
    p = abs(p);
    float d = length(p.xy);
    float di = abs(d  -  r1 + .1/2.) - .05/2.;
    d = abs(d  -  r1 + .075/2.) - .075/2.;
    d = min(max(d , p.z - r2), max(di , p.z - r2 - .01));
    
    // Return the minimum of the inner cog and teeth. The inner spokes have been kept global
    // for ID purposes and isn't include here... Yeah, messy, but this is a fiddly example
    // to work with. :)
    return min(d, d2);
    
}

// The larger hexagonal gear\cog.
float dist2(vec3 p, float r1, float r2){
    
    
    // See the comments in the "dist" function.
    
    ///p.xy *= rot2(mod(iTime/1.8, 3.*6.2831/18.) + 3.*6.2831/18.);
    p.xy *= rot2(mod(iTime/1.8, 6.2831/6.) + 6.2831/6.);
    
    float a = atan(p.y, abs(p.x)); // Note the "abs" call.
    
    p.z = -p.z;
    
    r1 *= (.9 + p.z);
    
    vec3 q = p; 
    
    float ia = floor(a/6.2831*18.) + .5;
    q.xy = rot2(ia*6.2831/18.)*q.xy; // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - .03/3.; // Move the radial coordinate out to the edge of the rim.
    q = abs(q);
    float spike = mix(max(q.x - .05, q.y - .02), length(q.xy*vec2(.7, 1)) - .025, .5);
    float d2 = max(spike, q.z - r2);
    
    q = p; 
    ia = floor(a/6.2831*6.) + .5;
    q.xy = rot2(ia*6.2831/6.)*q.xy;
    sph = max(max(abs(q.x), abs(q.y)) - .0275, abs(q.z - .07) - .06);
    //sph = max(length(q.xy) - .0275, abs(q.z - .07) - .06);
    q = abs(q + vec3(r1/2., 0, -.07));
    //spokes = max(max(q.x - r1/2. + .02, q.y - .03), q.z - .01);
    spokes = max(q.x - r1/2. + .02, max(max(q.y - .05, q.z - .015), (q.y + q.z)*.7071 - .03));
    
    p = abs(p);
    float d = length(p.xy);
    float di = abs(d  -  r1 + .1/2.) - .05/2.;
    d = abs(d  -  r1 + .075/2.) - .075/2.;
    d = min(max(d , p.z - r2), max(di , p.z - r2 - .0115));
    
    return min(d, d2);
    
}

// The smaller hexagonal gear\cog.
float dist3(vec3 p, float r1, float r2){
    
    // See the comments in the "dist" function.
    
    // This is the same as the two functions above, but the smaller cogs cross a boundary line
    // and directional polarity has to be considered.
    float dir = p.x < 0.? -1. : 1.;
    
    p.x = abs(p.x);
    vec3 q2 = p;
    p.xy *= rot2(mod(iTime/1.2*dir + 3.14159/12., 6.2831/12.) + 5.*6.2831/12.); // + 5.*3.14159/12.
    
    
    float a = atan(p.y, abs(p.x)); // Note the "abs" call.
    
    r1 *= (.9 + p.z);
    
    vec3 q = p;
   
    float ia = floor(a/6.2831*12.) + .5;
    q.xy = rot2(ia*6.2831/12.)*q.xy; // Convert to polar coordinates. X -> radius, Y -> angle.
    q.x += r1;// - .03/3.; // Move the radial coordinate out to the edge of the rim.
    q.xy = abs(q.xy);
    float spike = mix(max(q.x - .05, q.y - .02), length(q.xy*vec2(.7, 1)) - .025, .5);
    float d2 = max(spike, abs(q.z) - r2);

    
    q = q2; 
    q.xy *= rot2(mod(iTime/1.2*dir, 6.2831/6.) + 2.*6.2831/6.);
    a = atan(q.y, abs(q.x));
    ia = floor(a/6.2831*6.) + .5;
    q.xy = rot2(ia*6.2831/6.)*q.xy;
    sph = max(max(abs(q.x), abs(q.y)) - .02, abs(q.z - .07) - .06);
    //sph = max(length(q.xy) - .02, abs(q.z - .07) - .06);
    q = abs(q + vec3(r1/2., 0, -.07));
    spokes = max(q.x - r1/2. + .02, max(max(q.y - .04, q.z - .015), (q.y + q.z)*.7071 - .02));
    
    
    p = abs(p);
    float d = length(p.xy);
    float di = abs(d  -  r1 + .1/2.) - .05/2.;
    d = abs(d  -  r1 + .075/2.) - .075/2.;
    d = min(max(d , p.z - r2), max(di , p.z - r2 - .007));
    
    return min(d, d2);
    
}

//Raytraced sphere hit variable. 
//float balHit; 

float map(in vec3 p){
   
    
    // Back plane. Six units behind the center of the weaved object.
    float pln = -p.z + 6.;
    
/*  
    // Raytraced sphere hit variable. It saves extra calculation, but
    // complicates things. Plus, it can interfere with shadows, add to 
    // the compile time... It saves a lot of pixel calculations though, 
    // especially in fullscreen... I'll have a think about it. :)
    if(balHit < 0.){
        
        objID = vec4(1e5, 1e5, 1e5, pln);
        return pln;
        
    }
*/   
    
    // Rotate the object.
    p = rotObj(p);
    //p.yz *= rot2(-3.14159/6.);
    
    vec3 oP = p;
    
    
    float d = 1e5, d2 = 1e5, d3 = 1e5;
    
   
    // DjinnKahn's icosahedral distance function that produces a triangular face
    // and allows you to determine between the negative and positive X axis.
    
    // Large hexagonal face positions.
    vec3 hexFace = opIcosahedronWithPolarity(p); 
    
    // Rotating the points above to the dual pentagonal face positions. This saves
    // a lot of extra operations. We're able to do this because of the icosahedron
    // and dodecahedron duality.
    vec3 pentFace = R4*hexFace; 
    
    // Pentagon.
    vec3 p1 = (pentFace - vec3(0, 0, 1));
    d3 = min(d3, dist( p1, .185, .1) );
    d = min(d, spokes);
    d3 = min(d3, sph);
    
    // Hexagon. 
    p1 = basisHex*(hexFace - mid*1.2425);
    d3 = min( d3, dist2( p1, .25, .1) );
    d = min(d, spokes);
    d3 = min(d3, sph);
    
    // Small cogs.
    p1 = basisHexSm1*(hexFace - mix(v0, v2, .333)*1.1547);
    d2 = min( d2, dist3( p1, .16, .1) );
    d = min(d, spokes);
    d3 = min(d3, sph);
    p1 = basisHexSm2*(hexFace - mix(v0, v1, .333)*1.1547);
    d2 = min( d2, dist3( p1, .16, .1) );
    d = min(d, spokes);
    d3 = min(d3, sph);
    
   
    // Capping off the edges of the gears with the outer sphere itself.
    float mainSph = length(oP);
    d = max(d, mainSph - 1.0825);
    d2 = max(d2, mainSph - 1.116);
    d3 = max(d3, mainSph - 1.118);
 
    

    
    // Store the individual object values for sorting later. Sorting multiple objects
    // inside a raymarching loop probably isn't the best idea. :)
    objID = vec4(d, d2, d3, pln);
    
    return min(min(d, d2), min(d3, pln));
}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ. In instances where there's no descernible 
// aesthetic difference between it and the six tap version, it's worth using.
vec3 calcNormal(in vec3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    vec2 e = vec2(0.0025, -0.0025); 
    return normalize(e.xyy * map(p + e.xyy) + e.yyx * map(p + e.yyx) + e.yxy * map(p + e.yxy) + e.xxx * map(p + e.xxx));
}
*/

/*
// Standard normal function. 6 taps.
vec3 calcNormal(in vec3 p) {
	const vec2 e = vec2(0.002, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}
*/

/*
// Normal calculation, with some edging and curvature bundled in.
vec3 calcNormal(vec3 p, inout float edge, inout float crv, float t) { 
	
    // It's worth looking into using a fixed epsilon versus using an epsilon value that
    // varies with resolution. Each affects the look in different ways. Here, I'm using
    // a mixture. I want the lines to be thicker at larger resolutions, but not too thick.
    // As for accounting for PPI; There's not a lot I can do about that.
    vec2 e = vec2(3./mix(450., min(850., iResolution.y), .35), 0);//*(1. + t*t*.7);

	float d1 = map(p + e.xyy), d2 = map(p - e.xyy);
	float d3 = map(p + e.yxy), d4 = map(p - e.yxy);
	float d5 = map(p + e.yyx), d6 = map(p - e.yyx);
	float d = map(p)*2.;

    edge = abs(d1 + d2 - d) + abs(d3 + d4 - d) + abs(d5 + d6 - d);
    //edge = abs(d1 + d2 + d3 + d4 + d5 + d6 - d*3.);
    edge = smoothstep(0., 1., sqrt(edge/e.x*2.));
     
    // Wider sample spread for the curvature.
    //e = vec2(12./450., 0);
	//d1 = map(p + e.xyy), d2 = map(p - e.xyy);
	//d3 = map(p + e.yxy), d4 = map(p - e.yxy);
	//d5 = map(p + e.yyx), d6 = map(p - e.yyx);
    //crv = clamp((d1 + d2 + d3 + d4 + d5 + d6 - d*3.)*32. + .5, 0., 1.);
 
    
    e = vec2(.001, 0); //iResolution.y - Depending how you want different resolutions to look.
	d1 = map(p + e.xyy), d2 = map(p - e.xyy);
	d3 = map(p + e.yxy), d4 = map(p - e.yxy);
	d5 = map(p + e.yyx), d6 = map(p - e.yyx);
	
    return normalize(vec3(d1 - d2, d3 - d4, d5 - d6));
}
*/

// IQ rewrote the function above, and for reasons that defy my own sense 
// of logic, has cut the compile time down by at least half. For whatever
// reason, restricting an unroll in the larger iteration "trace" function
// has virtually no effect on compile time, yet doing the same with the
// fewer "map" calls in the following function does... I'll leave the
// explanation for that conundrum to the GPU instruction experts. :)
//
vec3 calcNormal(vec3 p, inout float edge, inout float crv, float t) { 
    
    float eps = 3./mix(450., min(850., iResolution.y), .35);

    float d = map(p);
    
    vec3 e = vec3(eps, 0, 0);
    
    vec3 da = vec3(-2.*d);
    for( int i = min(iFrame,0); i<3; i++ )
    {
        for( int j=min(iFrame,0); j<2; j++ )
            da[i] += map(p + e*float(1-2*j));
        e = e.zxy;
    }
    da = abs(da);
    
    edge = da.x + da.y + da.z;
    edge = smoothstep(0., 1., sqrt(edge/e.x*2.));
    
    vec3 n = vec3(0.0);
    for( int i=min(iFrame, 0); i<4; i++ )
    {
        vec3 e = .57735*(2.*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.);
        n += e*map(p + .001*e);
    }
    
    return normalize(n);
}

// Raymarching: The distance function is a little on the intensive side, so I'm 
// using as fewer iterations as necessary. Even though there's a breat, the compiler
// still has to unroll everything, and larger numbers make a difference.
float trace(in vec3 ro, in vec3 rd){
    
    float t = 0., d;
    
    for(int i = 0; i<64; i++){
    
        d = map(ro + rd*t);
        if(abs(d)<.001*(1. + t*.05) || t > FAR) break;
        t += d;
    }
    
    return min(t, FAR);
}

/*
float hash( float n ){ return fract(cos(n)*45758.5453); }

// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function and have been looking for an excuse to use it. For a better version, and usage, 
// refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
float calculateAO( in vec3 p, in vec3 n, float maxDist )
{
	float ao = 0.0, l;
	const float nbIte = 6.0;
	//const float falloff = 0.9;
    for( float i=1.; i< nbIte + .5; i++ ){
    
        l = (i + hash(i))*.5/nbIte*maxDist;
        ao += (l - map( p + n*l ))/(1.+ l);// / pow(1.+l, falloff);
    }
	
    return clamp( 1.-ao/nbIte, 0., 1.);
}
*/

// Ambient occlusion, for that self shadowed look.
// Based on the original by IQ.
float calcAO(in vec3 p, in vec3 n)
{
	float sca = 4., occ = 0.0;
    for( int i=1; i<6; i++ ){
    
        float hr = float(i)*.125/5.;        
        float dd = map(p + hr*n);
        occ += (hr - dd)*sca;
        sca *= .75;
    }
    return clamp(1. - occ, 0., 1.);   
    
}

// The iterations should be higher for proper accuracy.
float softShadow(in vec3 ro, in vec3 rd, float t, in float end, in float k){

    float shade = 1.0;
    // Increase this and the shadows will be more accurate, but more iterations slow things down.
    const int maxIterationsShad = 24; 

    // The "start" value, or minimum, should be set to something more than the stop-threshold, so as to avoid a collision with 
    // the surface the ray is setting out from. It doesn't matter how many times I write shadow code, I always seem to forget this.
    // If adding shadows seems to make everything look dark, that tends to be the problem.
    float dist = .001*(1. + t*.05);
    float stepDist = end/float(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i = 0; i<maxIterationsShad; i++){
        // End, or maximum, should be set to the distance from the light to surface point. If you go beyond that
        // you may hit a surface not between the surface and the light.
        float h = map(ro + rd*dist);
        shade = min(shade, k*h/dist);
        //shade = min(shade, smoothstep(0.0, 1.0, k*h/dist));
        
        // What h combination you add to the distance depends on speed, accuracy, etc. To be honest, I find it impossible to find 
        // the perfect balance. Faster GPUs give you more options, because more shadow iterations always produce better results.
        // Anyway, here's some posibilities. Which one you use, depends on the situation:
        // +=max(h, 0.001), +=clamp( h, 0.01, 0.25 ), +=min( h, 0.1 ), +=stepDist, +=min(h, stepDist*2.), etc.
        
        dist += clamp(h, 0.01, 0.25);
        
        // There's some accuracy loss involved, but early exits from accumulative distance functions can help.
        if (h<0.0001 || dist > end) break; 
    }

    // Return the shadow value. Note that I'm not using clamp. The shadow value should be capped to zero
    // prior to adding a constant... Unless you're not adding a constant. :)
    return min(max(shade, 0.) + .1, 1.); 
}


// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it 
// does give the impression that the surface is reflecting the surrounds in some way.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
vec3 envMap(vec3 p){
    
    p *= 3.;
    
    float n3D2 = noise3D(p*3.);
   
    // A bit of fBm.
    float c = noise3D(p)*.57 + noise3D(p*2.)*.28 + noise3D(p*4.)*.15;
    c = smoothstep(.25, 1., c); // Putting in some dark space.
    
    p = vec3(c, c*c, c*c*c); // Bluish tinge.
    
    return mix(p, p.zyx, n3D2*.5 + .5); // Mixing in a bit of purple.

}

/*
// Intersection of a sphere. IQ's formula - trimmed down a little.
float traceSphere( in vec3 ro, in vec3 rd, in vec4 sph ){

	ro -= sph.xyz;
	float b = dot(ro, rd);
	float h = b*b - dot(ro, ro) + sph.w*sph.w;
    return h<0. ? -1. : -b - sqrt( h );
}
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Aspect correct screen coordinates. Restricting the size of the
    // fullscreen object to maintain a little detail on larger screens.
    // A consequence is a larger background. By the way, this was coded
    // for the 800 by 450 canvas or smaller, rather than fullscreen.
    vec2 p = (fragCoord - iResolution.xy*.5)/min(850., iResolution.y);
    
    // Unit direction ray.
    vec3 rd = normalize(vec3(p, 1));
    
    // Ray origin, doubling as the camera postion.
    vec3 ro = vec3(0, 0, -2.75);
    
    // Light position. Near the camera.
    vec3 lp = ro + vec3(.25, 2, -.1);
    
    // Basic camera rotation.
    rd.xy *= rot2(sin(iTime/8.)*.2);
    rd.xz *= rot2(sin(iTime/4.)*.1);
    
    // Precalculating some basis matrix values.
    // Replaced with Cyberjax's suggestion.
    //initBases();
    
    //balHit = traceSphere(ro, rd, vec4(vec3(0), 1.2)); // Main object pixel flag.
    
    // Ray march.
    float t = trace(ro, rd);
    
    // Object identification: Back plane: 3, Golden joins: 2., 
    // Ball joins: 1., Silver pipes:  0.
    float svObjID = objID.x<objID.y && objID.x<objID.z && objID.x<objID.w? 0.: 
    objID.y<objID.z && objID.y<objID.w ? 1. : objID.z<objID.w? 2. : 3.;

    
    // Initiate the scene color zero.
    vec3 sceneCol = vec3(0);
    
    // Surface hit. Color it up.
    if(t < FAR){
    
        // Position.
        vec3 pos = ro + rd*t;
        // Normal.
        //vec3 nor = calcNormal(pos);
        // Normal, plus edges and curvature. The latter isn't used.
        float edge = 0., crv = 1.;
        vec3 nor = calcNormal(pos, edge, crv, t);
        
        //vec3 rp = rotObj(pos);
        
        // Light direction vector.
        vec3 li = lp - pos;
        float lDist = max(length(li), .001);
        li /= lDist;
        
        // Light falloff - attenuation.
        float atten = 1./(1. + lDist*.05 + lDist*lDist*0.025);
        
        // Soft shadow and occlusion.
        float shd = softShadow(pos + nor*.0015, li, t, lDist, 8.); // Shadows.
        float ao = calcAO(pos, nor);
        
        
        float diff = max(dot(li, nor), .0); // Diffuse.
        float spec = pow(max(dot(reflect(-li, nor), -rd), 0.), 16.); // Specular.
        // Ramping up the diffuse. Sometimes, it can make things look more metallic.
        float od = diff;
        diff = pow(diff, 4.)*2.; 
        
        
        float Schlick = pow( 1. - max(dot(rd, normalize(rd + li)), 0.), 5.0);
		float fre2 = mix(.5, 1., Schlick);  //F0 = .5.
		
        // Spokes: ObjID == 0.;
        #if COLOR_SCHEME == 0
        vec3 col = vec3(.6); // Silver.
        #elif COLOR_SCHEME == 1
        vec3 col = vec3(.1); // Black.
        #else
        vec3 col = vec3(.9, .2, .4); // Pink.
        #endif

        
        if(svObjID == 1.) { // Smaller hexagon cogs.
            #if COLOR_SCHEME == 0
            col = vec3(1, .5, .2)*.7; // Gold.
            #else 
            col = vec3(.6); // Chrome.
            #endif
         }
        if(svObjID == 2.){ // Larger hexagon and pentagon cogs.
            
            #if COLOR_SCHEME == 0
            col = vec3(1, .65, .3)*.7; // Gold-Copper
            #else 
            col = vec3(.6); // Chrome.
            #endif
        }
        if(svObjID == 3.) { // Back plane.
            
            // Dark background color. Noise is applied afterward (See below).
            #if COLOR_SCHEME == 0
            col = vec3(1, .7, .4)*.045; // Brown.
            #elif COLOR_SCHEME == 1
            col = vec3(.7)*.045; // Charcoal.
            #else
            col = vec3(.6, .7, 1)*.045; // Blue.
            #endif
        }
        
        
        // Applying some subtle 3D fBm noise to the gear object and back plane to grunge
        // it up slightly. Technically, I should be keeping a copy of the individual gear
        // rotations and applying those, but the compiler is hating on this example enough
        // already, so I've lowered these noise intensity and added noisy environmental 
        // reflection so that the sliding UV effect is negigible.       
        float txSz = 1.;
        vec3 txPos = pos; 
        if(svObjID == 3.) txSz /= 4.;
        else txPos = rotObj(txPos);
        col *= fbm(txPos*64.*txSz)*.75 + .5;
        
    
        
        // Diffuse plus ambient term.
        sceneCol = col*(diff + .25); 
        
        // Specular term.
        if(svObjID == 3.) sceneCol += col*vec3(1, .6, .2).zyx*spec*.25; // Less specular on the back plane.
        else sceneCol += col*vec3(.5, .75, 1.)*spec*2.;
        
        // Fake environment mapping.
        float envF = 4.;
        if(svObjID == 0.) envF = 8.;
        sceneCol += sceneCol*envMap(reflect(rd, nor))*envF;
        
        // Edges.
        sceneCol *= 1. - edge*.8;
        //col = col*.7 + edge*.3;
        
        sceneCol *= atten*shd*ao; // Applying the light falloff, shadows and AO.
        
        
        
         
    }
    
    // Screen color. Rough gamma correction. No fog or postprocessing.
    fragColor = vec4(sqrt(clamp(sceneCol, 0., 1.)), 1);
}