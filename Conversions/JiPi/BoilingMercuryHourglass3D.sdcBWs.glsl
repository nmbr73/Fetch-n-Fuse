

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// ---------------------------------------------------------------------------------------
//	Created by fenix in 2022
//	License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
//  3D smoothed particle hydrodynamics, or at least a 
//
//  Particles are attenuated by 2D distance to line segment for motion blur.
// 
//  Buffer A computes the particle positions and a 3D voronoi
//  Buffer B performs a traditional 2D voronoi
//  Buffer C renders and maitains persistent state
//
// ---------------------------------------------------------------------------------------

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture(iChannel2, fragCoord/iResolution.xy);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const float PARTICLE_DEBUG_RENDER_SIZE = 0.03;
const float PARTICLE_VORONOI_CULL_DIST = 0.3;
const float PARTICLE_SDF_CULL_DIST = 0.06;
const int MAX_PARTICLES = 1000;
const vec3 GRAVITY = vec3(-0.00, -0.15, -0.1);
const float DENSITY_SMOOTH_SIZE = 0.5;
const float PARTICLE_REPEL_SIZE = 0.1;
const float PARTICLE_COLLISION_SIZE = 0.2;
const float PARTICLE_MASS = 1.0;
const float PARTICLE_STIFFNESS = 1.0;
const float IDEAL_DENSITY = 30.0;
const float PARTICLE_VISCOSITY = 3.5;
const float PARTICLE_REPEL = 0.8;

// ===============================
// Generic Helpers/Constants
// ===============================

#define PI 3.141592653589793
#define TWOPI 6.283185307179586
#define HALFPI 1.570796326794896
#define SQRT2INV 0.7071067811865475

#define POLAR(theta) vec3(cos(theta), 0.0, sin(theta))
#define SPHERICAL(theta, phi) (sin(phi)*POLAR(theta) + vec3(0.0, cos(phi), 0.0))

// Same as built-in 'refract' (cf. link) but replaces the case which would
// normally result in 0 with a reflection (for total internal reflection)
// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/refract.xhtml
vec3 refractFix(vec3 I, vec3 N, float eta) {
    float k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I));
    return k < 0.0
        ? reflect(I, N) // <- 'refract' returns 0 here
    	: eta * I - (eta * dot(N, I) + sqrt(k)) * N;
}

vec4 blendOnto(vec4 cFront, vec4 cBehind) {
    return cFront + (1.0 - cFront.a)*cBehind;
}

vec4 blendOnto(vec4 cFront, vec3 cBehind) {
    return cFront + (1.0 - cFront.a)*vec4(cBehind, 1.0);
}

float length2(vec2 v)
{
    return dot(v, v);
}

float length2(vec3 v)
{
    return dot(v, v);
}

// ===============================
// Quaternion helpers
// (Unit quaternions: w+xi+yj+zk)
// ===============================

#define QID vec4(0.0, 0.0, 0.0, 1.0)

vec4 slerp(vec4 a, vec4 b, float t) {
    float d = dot(a, b);
    vec4 a2 = a;

    if (d < 0.0) {
        d = -d;
        a2 = -a;
    }
    if (d > 0.999) {
        return normalize(mix(a2, b, t));
    }

    float theta = acos(d);
    return (sin((1.-t)*theta)*a2 + sin(t*theta)*b) / sin(theta);
}

vec4 qMul(vec4 a, vec4 b) {
    return vec4(
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    );
}

vec4 qConj(vec4 q) {
    return vec4(-q.xyz, q.w);
}

vec4 qRot(vec3 nvAxis, float angle) {
    return vec4(nvAxis*sin(angle*0.5), cos(angle*0.5));
}

mat3 qToMat(vec4 q) {
    float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
    float xx = q.x*q.x, xy = q.x*q.y, xz = q.x*q.z;
    float yy = q.y*q.y, yz = q.y*q.z, zz = q.z*q.z;
    return mat3(
        1. - 2.*(yy + zz),
             2.*(xy + wz),
             2.*(xz - wy),

             2.*(xy - wz),
        1. - 2.*(xx + zz),
             2.*(yz + wx),

             2.*(xz + wy),
             2.*(yz - wx),
        1. - 2.*(xx + yy)
    );
}

// ===============================
// Reading/writing state
// ===============================

struct state {
    vec3 p; // Pendulum pivot
    vec3 q; // Accelerate p toward this point
    vec3 v; // Pendulum "bob" (relative to pivot)
    vec3 L; // Angular momentum
    vec4 pr; // Object rotation (unit quaternion)
};

state readStateInternal(int iFrame, sampler2D sampler) {
    state s = state(
        vec3(0.0),
        vec3(0.0),
        vec3(0.0, -cos(0.25*PI), sin(0.25*PI)),
        vec3(0.0, 0.5, 0.0),
        QID
    );
    if (iFrame > 0) {
        s.p = texelFetch(sampler, ivec2(0, 0), 0).xyz;
        s.q = texelFetch(sampler, ivec2(1, 0), 0).xyz;
        s.v = texelFetch(sampler, ivec2(2, 0), 0).xyz;
        s.L = texelFetch(sampler, ivec2(3, 0), 0).xyz;
        s.pr = texelFetch(sampler, ivec2(4, 0), 0);
    }
    return s;
}

#define readState() readStateInternal(iFrame, iChannel2)

void writeState(in state s, in vec2 fragCoord, inout vec4 fragColor) {
    if (abs(fragCoord.y - 0.0-0.5) < 0.5) {
        if (abs(fragCoord.x - 0.0-0.5) < 0.5) {
            fragColor = vec4(s.p, 1.0);
        } else if (abs(fragCoord.x - 1.0-0.5) < 0.5) {
            fragColor = vec4(s.q, 1.0);
        } else if (abs(fragCoord.x - 2.0-0.5) < 0.5) {
            fragColor = vec4(s.v, 1.0);
        } else if (abs(fragCoord.x - 3.0-0.5) < 0.5) {
            fragColor = vec4(s.L, 1.0);
        } else if (abs(fragCoord.x - 4.0-0.5) < 0.5) {
            fragColor = s.pr;
        }
    }
}

// ===============================
// Camera setup
// ===============================

#define RES iResolution
#define TAN_HALF_FOVY 0.5773502691896257

vec3 nvCamDirFromClip(vec3 iResolution, vec3 nvFw, vec2 clip) {
    vec3 nvRt = normalize(cross(nvFw, vec3(0.,1.,0.)));
    vec3 nvUp = cross(nvRt, nvFw);
    return normalize(TAN_HALF_FOVY*(clip.x*(RES.x/RES.y)*nvRt + clip.y*nvUp) + nvFw);
}

void getCameraInternal(in state s, in vec2 uv, in float iTime, in vec3 iResolution, out vec3 camPos, out vec3 nvCamDir) {
    vec2 mouseAng = vec2(HALFPI*0.75, PI*0.45) + 0.2*vec2(cos(0.5*iTime),sin(0.5*iTime));
    camPos = vec3(2.0, 1.0, 2.0) + 5.0 * SPHERICAL(mouseAng.x, mouseAng.y);

    vec3 lookTarget = mix(vec3(0.0), s.p, 0.05);
    vec3 nvCamFw = normalize(lookTarget - camPos);

    nvCamDir = nvCamDirFromClip(iResolution, nvCamFw, uv*2. - 1.);
}

#define getCamera(X, Y, Z, W) getCameraInternal(X, Y, iTime, iResolution, Z, W)

// ===============================
// Physics, reading/writing state
// ===============================

void updateStateInternal(inout state s, in vec4 iMouse, in int iFrame, in float iTime, in vec3 iResolution) {

    // pr (object rotation unit quaternion) gets "slerped" towards qr
    float tmod = mod(float(iFrame) / 120.0, 46.0);
    
#if 0 // Stop auto-flip
    vec4 qr = QID;
#else
    vec4 qr = (
        tmod < 20.0 ? QID :
        tmod < 23.0 ? qRot(vec3(-SQRT2INV, 0.0, SQRT2INV), 0.5*PI):
        tmod < 43.0 ? qRot(vec3( 1.0, 0.0, 0.0), PI):
        qRot(vec3(-SQRT2INV, 0.0, SQRT2INV), -0.5*PI)
    );
#endif

    // p (object displacement) gets "lerped" towards q
    if (iMouse.z > 0.5) {
        vec2 uvMouse = iMouse.xy / iResolution.xy;
        vec3 camPos;
        vec3 nvCamDir;
        getCamera(s, uvMouse, camPos, nvCamDir);

        float t = -camPos.z/nvCamDir.z;
        if (t > 0.0 && t < 50.0) {
            vec3 center = vec3(0.0);
            vec3 hit = camPos + t*nvCamDir;
            float qToCenter = distance(center, s.q);
            vec3 delta = hit - s.q;
            if (length2(delta) < 1.0)
            {
                s.q = hit;
            }
            else
            {
                float angle = -atan(delta.x, delta.y);
                float angle2 = length(delta.xy);
                qr = qRot(vec3(0.0, 0.0, 1.0), angle);
                qr = qMul(qr, qRot(vec3(1.0, 0.0, 0.0), angle2));
            }
        }
    }

   // apply lerp p -> q and slerp pr -> qr
    vec3 vel = 0.25*(s.q - s.p);
    s.v = vel;
    s.p += vel;
    s.pr = normalize(slerp(s.pr, qr, 0.075));

    // object acceleration
    vec3 a = -0.25*(s.q - s.p) + vec3(0.0, -1.0, 0.0);
    mat3 prMatInv = qToMat(qConj(s.pr));
    a = prMatInv*a;

    // hand-wavy torque and angular momentum
    vec3 T = cross(s.v, a);
    s.L = 0.96*s.L + 0.2*T;
}

#define updateState(X) updateStateInternal(X, iMouse, iFrame, iTime, iResolution)

// ===============================
// Geometry definitions
// ===============================

#define BOUNDING_SPHERE_RADIUS 4.0
#define GLASS_THICKNESS 0.1

float sdfPlane(vec3 planePoint, vec3 nvPlaneN, vec3 p) {
    return dot(p - planePoint, nvPlaneN);
}

float sdfInterval(float a, float b, float x) {
    return abs(x - 0.5*(a+b)) - 0.5*(b-a);
}

// From https://iquilezles.org/articles/distfunctions
float opSubtraction( float d1, float d2 ) { return max(-d1,d2); }
float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h);
}


float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }
    
float sdfContainer(vec3 p, state s) {
    vec3 d0 = abs(p - s.p - vec3(0.0, 2.09, 0.0));
    vec3 d1 = abs(p - s.p + vec3(0.0, 2.09, 0.0));
    float s0 = length(d0) - 1.7;
    float s1 = length(d1) - 1.7;
    return opSmoothUnion(s0, s1, 1.5);
}

float sdfGlass(vec3 p, state s) {
    float etchDepth = 0.0; // Can sample from e.g. cubemap here for some texture
    return sdfInterval(0.0, GLASS_THICKNESS - etchDepth, sdfContainer(p, s));
}

#define SDF_N_EPS 0.005
#define SDF_NORMAL(sdfFn, p, s) \
    normalize(vec3( \
        sdfFn( p+vec3(SDF_N_EPS,0.0,0.0), s ) - sdfFn( p-vec3(SDF_N_EPS,0.0,0.0), s ), \
        sdfFn( p+vec3(0.0,SDF_N_EPS,0.0), s ) - sdfFn( p-vec3(0.0,SDF_N_EPS,0.0), s ), \
        sdfFn( p+vec3(0.0,0.0,SDF_N_EPS), s ) - sdfFn( p-vec3(0.0,0.0,SDF_N_EPS), s )  \
    ))
    
//returns the ids of the four closest particles from the input
ivec4 fxGetClosestInternal(sampler2D sampler, ivec2 xy)
{
    return ivec4(texelFetch(sampler, xy, 0));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X)

#define FUL_NEIGHBORS 0
#define FUR_NEIGHBORS 1
#define FLL_NEIGHBORS 2
#define FLR_NEIGHBORS 3
#define BUL_NEIGHBORS 4
#define BUR_NEIGHBORS 5
#define BLL_NEIGHBORS 6
#define BLR_NEIGHBORS 7
#define POS 8
#define VEL 9
#define NUM_PARTICLE_DATA_TYPES 10

//returns the location of the particle within the particle buffer corresponding with the input id 
ivec2 fxLocFromIDInternal(int width, int id, int dataType)
{
    int index = id * NUM_PARTICLE_DATA_TYPES + dataType;
    return ivec2( index % width, index / width);
}

#define fxLocFromID(X, Y) fxLocFromIDInternal(int(iResolution.x), X, Y)

struct fxParticle
{
    vec3 pos;
    float density;
    vec3 vel;
    float pressure;
    
    ivec4 neighbors[8];
};

//get the particle corresponding to the input id
fxParticle fxGetParticleInternal(sampler2D sampler, int resolutionWidth, int id)
{
    vec4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FUL_NEIGHBORS), 0);
    vec4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FUR_NEIGHBORS), 0);
    vec4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FLL_NEIGHBORS), 0);
    vec4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FLR_NEIGHBORS), 0);
    vec4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BUL_NEIGHBORS), 0);
    vec4 particleData5 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BUR_NEIGHBORS), 0);
    vec4 particleData6 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BLL_NEIGHBORS), 0);
    vec4 particleData7 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BLR_NEIGHBORS), 0);
    vec4 particleData8 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS), 0);
    vec4 particleData9 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, VEL), 0);

    fxParticle particle;
    particle.pos = particleData8.xyz;
    particle.density = particleData8.w;
    particle.vel = particleData9.xyz;
    particle.pressure = particleData9.w;
    particle.neighbors[0] = ivec4(particleData0);
    particle.neighbors[1] = ivec4(particleData1);
    particle.neighbors[2] = ivec4(particleData2);
    particle.neighbors[3] = ivec4(particleData3);
    particle.neighbors[4] = ivec4(particleData4);
    particle.neighbors[5] = ivec4(particleData5);
    particle.neighbors[6] = ivec4(particleData6);
    particle.neighbors[7] = ivec4(particleData7);
    
    return particle;
}


vec4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
    case FUL_NEIGHBORS:
        return vec4(p.neighbors[0]);
    case FUR_NEIGHBORS:
        return vec4(p.neighbors[1]);
    case FLL_NEIGHBORS:
        return vec4(p.neighbors[2]);
    case FLR_NEIGHBORS:
        return vec4(p.neighbors[3]);
    case BUL_NEIGHBORS:
        return vec4(p.neighbors[4]);
    case BUR_NEIGHBORS:
        return vec4(p.neighbors[5]);
    case BLL_NEIGHBORS:
        return vec4(p.neighbors[6]);
    case BLR_NEIGHBORS:
        return vec4(p.neighbors[7]);
    case POS:  
        return vec4(p.pos, p.density);
    case VEL:
        return vec4(p.vel, p.pressure);
    }
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

vec4 fxGetParticleDataInternal(sampler2D sampler, int resolutionWidth, int id, int dataType)
{
    return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, dataType), 0);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y)

float distanceSquared(vec3 a, vec3 b)
{
    vec3 delta = a - b;
    return dot(delta, delta);
}

float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash( int k ) {
    uint n = uint(k);
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0;
}

uvec4 hash(uvec4 x){
    x = ((x >> 16u) ^ x.yzwx) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x.yzwx) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x.yzwx) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x.yzwx) * 0x45d9f3bu;
    //x = (x >> 16u) ^ x;
    return x;
}
uvec4 hash(uvec3 x0){
    uvec4 x = x0.xyzz;
    x = ((x >> 16u) ^ x.yzxy) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x.yzxz) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x.yzxx) * 0x45d9f3bu;
    //x = (x >> 16u) ^ x;
    return x;
}

vec4 noise(ivec4 p){
    const float scale = pow(2., -32.);
    uvec4 h = hash(uvec4(p));
    return vec4(h)*scale;
}

vec4 noise(ivec3 p){
    const float scale = 1.0/float(0xffffffffU);
    uvec4 h = hash(uvec3(p));
    return vec4(h)*scale;
}

vec4 noise(ivec2 p){
    return noise(ivec3(p, 0));
}

float SPHKernel (float x)
{
    x *= (1.0 / DENSITY_SMOOTH_SIZE);
    if (x < 1.0)
        return 4.0 * cos(x*PI) + cos((x + x) * PI) + 3.0;
    else
        return 0.0;
}


float SPHKernel (vec3 deltaPos)
{
    float x = length(deltaPos);
    return SPHKernel(x);
}

float SPHgradKernel (float x)
{
    x *= (1.0 / PARTICLE_REPEL_SIZE);

    if (x < 4.0)
    {
        float xx = x*x;
        float xxx = xx*x;
        float xxxx = xxx*x;
        return PARTICLE_REPEL * (0.000 + 3.333 * x + -3.167 * xx + 0.917 * xxx + -0.083 * xxxx);
    }
    else
        return 0.0;
}

bool intersectSphere(in vec3 ro, in vec3 rd, in vec3 center, in float radius2, out float t) 
{ 
    float t0, t1;  //solutions for t if the ray intersects 
    // geometric solution
    vec3 L = center - ro; 
    float tca = dot(L, rd); 
    // if (tca < 0) return false;
    float d2 = dot(L, L) - tca * tca; 
    if (d2 > radius2) return false; 
    float thc = sqrt(radius2 - d2); 
    t0 = tca - thc; 
    t1 = tca + thc; 

    if (t0 > t1)
    {
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }
    if (t0 < 0.0) { 
        t0 = t1;  //if t0 is negative, let's use t1 instead 
        if (t0 < 0.0) return false;  //both t0 and t1 are negative 
    } 

    t = t0; 

    return true; 
} 

float linePointDist2(in vec3 newPos, in vec3 oldPos, in vec3 point, out vec3 closest)
{
    vec3 pDelta = (point - oldPos);
    vec3 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    if (deltaLen2 > 0.0000001)
    {
        float deltaInvSqrt = inversesqrt(deltaLen2);
        vec3 deltaNorm = delta * deltaInvSqrt;
        closest = oldPos + deltaNorm * max(0.0, min(1.0 / deltaInvSqrt, dot(deltaNorm, pDelta)));
    }
    else
    {
        closest = oldPos;
    }

    // Distance to closest point on line segment
    vec3 closestDelta = closest - point;
    return dot(closestDelta, closestDelta);
}

#define keyClick(ascii)   ( texelFetch(iChannel3,ivec2(ascii,1),0).x > 0.)
#define keyDown(ascii)    ( texelFetch(iChannel3,ivec2(ascii,0),0).x > 0.)

void insertion_sort(inout ivec4 i, inout vec4 d, int i_, float d_){	
    if(any(equal(ivec4(i_),i))) return;
    if     (d_ < d[0])             
        i = ivec4(i_,i.xyz),    d = vec4(d_,d.xyz);
    else if(d_ < d[1])             
        i = ivec4(i.x,i_,i.yz), d = vec4(d.x,d_,d.yz);
    else if(d_ < d[2])            
        i = ivec4(i.xy,i_,i.z), d = vec4(d.xy,d_,d.z);
    else if(d_ < d[3])           
        i = ivec4(i.xyz,i_),    d = vec4(d.xyz,d_);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// ---------------------------------------------------------------------------------------
// Computes the position of each particle, one per texture fragment.
// ---------------------------------------------------------------------------------------

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int id, int searchId, int dataType, in vec3 myPos);
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 iFragCoord = ivec2(fragCoord);
    state s = readState();
    mat3 prMatInv = qToMat(qConj(s.pr));
    mat3 prMat = inverse(prMatInv);

    //we only simulate PARTICLES amount of particles
    int index = iFragCoord.x + iFragCoord.y*int(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if (id >= MAX_PARTICLES) return;
    
    fxParticle data = fxGetParticle(id);
    
    if (dataType == POS || dataType == VEL)
    {
        if (iFrame == 0 || data.pos != data.pos || keyDown(32))
        {
            const vec3 INIT_CENTER = vec3(0.0, 2.0, 0.0);
            const vec3 INIT_EXTENTS = vec3(2.0, 2.0, 2.0);
            vec3 initPos = (noise(ivec4(int(fragCoord.x) / 2, fragCoord.y, int(fragCoord.x) / 2 + 2, fragCoord.y + 2.0)).xyz - 0.5) * INIT_EXTENTS + INIT_CENTER;

            data.pos = prMat*initPos + s.p;
            data.vel = vec3(0.0, 0.0, 0.0);
        }
        else
        {
            float myDensity = PARTICLE_MASS * SPHKernel(vec3(0));
            float newDensity = SPHKernel(vec3(0));
            vec3 force = GRAVITY;
            vec3 densityGrad = vec3(0);
            
            vec3 avgVel = data.vel * myDensity;

            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 8; i++){
                ivec4 neighbors = data.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= MAX_PARTICLES) continue;
                    
                    vec4 otherPosDensity = fxGetParticleData(cid, POS);
                    vec4 otherVelPressure = fxGetParticleData(cid, VEL);
                    vec3 deltaPos = data.pos - otherPosDensity.xyz;
                    float dist = length(deltaPos) + 0.001;
                    vec3 dir = deltaPos / dist;
                                            
                    float neighborDensity = PARTICLE_MASS * SPHKernel(deltaPos);
                    newDensity += neighborDensity;
                    avgVel += otherVelPressure.xyz * neighborDensity;
                    densityGrad += neighborDensity * deltaPos;
                    
                    force -= dir * SPHgradKernel(dist);
                }
            }       
            
            force += 0.1 * avgVel / newDensity - data.vel * 0.3;
            float overDensity = max(0.0, newDensity - IDEAL_DENSITY);
            force += overDensity * densityGrad * 0.0001;
            
            // Record misc solver results
            data.density = newDensity;
            data.pressure = PARTICLE_STIFFNESS * (newDensity - IDEAL_DENSITY);            

            // Boundary
            vec3 objectPoint = prMatInv * (data.pos - s.p) + s.p;
            float sd = sdfContainer(objectPoint, s);

            if (sd > -PARTICLE_REPEL_SIZE)
            {
                data.density += SPHKernel(sd) * 0.2;
            }
            if (sd > 0.0)
            {
                vec3 normal = SDF_NORMAL(sdfContainer, objectPoint, s);
                normal = prMat * normal;
                data.pos = data.pos - (sd) * normal;
                data.vel += s.v;
                if(dot(data.vel, normal) > 0.0)
                {
                    data.vel = reflect(data.vel, normal);
                    //data.vel -= normal * dot(data.vel, normal);
                    data.vel += -0.3 * data.vel;
                }
            }

            // Apply force
            data.vel = data.vel + force;

            // Damping
            data.vel -= data.vel * 0.1;
            data.vel -= data.vel * length2(data.vel) * 0.1;
            data.vel -= data.vel * 0.4 * smoothstep(10.0, 40.0, data.density);

            // Clamping
            float maxSpeed = 20000.0 / (iResolution.x + iResolution.y); // Dictated by voronoi update speed
            float velLength2 = length2(data.vel);
            if (velLength2 > maxSpeed * maxSpeed)
            {
                data.vel *= inversesqrt(velLength2) * maxSpeed;
            }

            // Integrate position
            data.pos = data.pos + data.vel / myDensity;
        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        ivec4 nb0 = ivec4(fxGetParticleData(id, dataType));
        ivec4 bestIds = ivec4(-1);
        vec4 bestDists = vec4(1e6);
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, nb0[i], dataType, data.pos);
        }
        
        //random sorts
        for (int i = 0; i < 16; ++i)
        {
            int searchId = int(float(MAX_PARTICLES) * hash13(vec3(iFrame, id, i)));
            sort0(bestIds, bestDists, id, searchId, dataType, data.pos);
        }
        
        fragColor = vec4(bestIds);
        
        return;
    }
    
    fragColor = fxSaveParticle(data, dataType);
}

bool iscoincidence(in ivec4 bestIds, int currentId, int id)
{
    return id <= 0 ||
      	id == currentId ||
        any(equal(bestIds,ivec4(id)));
}

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int currentId, int searchId, int dataType, in vec3 myPos)
{
    if (searchId >= MAX_PARTICLES) return; // particle recycled
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    vec3 nbX = fxGetParticleData(searchId, POS).xyz; 

    vec3 dx = nbX - myPos;
    int dir = int((sign(dx.x) * 0.5 + 1.0) + 2.0 * (sign(dx.y) * 0.5 + 1.0) + 4.0 * (sign(dx.z) * 0.5 + 1.0));

    if(dir != dataType) return; //not in this sector
    
    float t = length2(dx);
    
    //if (t > PARTICLE_REPEL_SIZE * 20.0) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Gijs's Basic : Voronoi Tracking: https://www.shadertoy.com/view/WltSz7

// Voronoi Buffer
// every pixel stores the 4 closest particles to it
// every frame this data is shared between neighbours

const float PARTICLE_CULL_DIST_2 = PARTICLE_VORONOI_CULL_DIST * PARTICLE_VORONOI_CULL_DIST;

float particleDistance(in int id, in vec3 ro, in vec3 rd)
{
    if(id==-1) return 1e6;

    vec3 pos = fxGetParticleData(id, POS).xyz;
    
    float t;
    if (intersectSphere(ro, rd, pos, PARTICLE_CULL_DIST_2, t))
    {
        return t;
    }
    else
    {
        return 1e6;
    }
}

void mainImage( out vec4 fragColor, vec2 fragCoord )
{
   	ivec2 iFragCoord = ivec2(fragCoord);
    state s = readState();

    vec3 camPos;
    vec3 nvCamDir;
    vec2 uv = fragCoord / RES.xy;
    getCamera(s, uv, camPos, nvCamDir);
    
    //in this vector the four new closest particles' ids will be stored
    ivec4 new = ivec4(-1);
    vec4 dis = vec4(1e6);
        
    ivec4 old = fxGetClosest(ivec2(fragCoord));
    for (int i = 0; i < 4; ++i)
    {
        float dis2 = particleDistance(old[i], camPos, nvCamDir);
        insertion_sort(new, dis, old[i], dis2);
    }
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            ivec4 old   = fxGetClosest( iFragCoord + ivec2( x, y) );      

            for(int j=0; j<4; j++){
                int id = old[j];
                float dis2 = particleDistance(id, camPos, nvCamDir);
                insertion_sort( new, dis, id, dis2 );
            }
        }
    }
    
    int searchIterations = 3;
    if (iFrame < 5)
    {
        searchIterations = 100;
    }
    for(int k = 0; k < searchIterations; k++){
        //random hash. We should make sure that two pixels in the same frame never make the same hash!
        float h = hash(
            iFragCoord.x + 
            iFragCoord.y*int(iResolution.x) + 
            iFrame*int(iResolution.x*iResolution.y) +
            k
        );
        //pick random id of particle
        int p = int(h * float(MAX_PARTICLES));//int(h*float(MAX_PARTICLES));
        insertion_sort(new, dis, p, particleDistance(p, camPos, nvCamDir));
    }
    
    fragColor = vec4(new); 
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Ray marching inspired by Liquid in Glass by tmst https://www.shadertoy.com/view/3tfcRS

// ===============================
// Marching, lighting/materials
// ===============================

#define SDF_EPS 0.01
#define DSTEP_ADJUST_EPS 0.02
#define STEPS 80

#define LIGHT_COLOR vec3(1.0)

#define GLASS_COLOR vec3(0.0, 0.0, 0.0)
#define GLASS_OPACITY 0.6

#define IR_AIR 1.0
#define IR_GLASS 1.5

// Increase this few fewer artifacts to give your gfx card a workout
const int MAX_CACHED_PARTICLES = 12;

// Enums
#define SUBSTANCE_AIR 0
#define SUBSTANCE_GLASS 1

vec4 computeSpecular(
    in float specularCoefficient,
    in float specularExponent,
    in vec3 nvNormal,
    in vec3 nvFragToLight,
    in vec3 nvFragToCam)
{
    vec3 blinnH = normalize(nvFragToLight + nvFragToCam);
    float valSpecular = pow(max(0.0, dot(nvNormal, blinnH)), specularExponent);
    valSpecular *= specularCoefficient;

    return valSpecular*vec4(LIGHT_COLOR, 1.0);
}

vec4 cachedParticles[MAX_CACHED_PARTICLES];
int numCachedParticles = 0;

const float PARTICLE_CULL_DIST_2 = PARTICLE_SDF_CULL_DIST * PARTICLE_SDF_CULL_DIST;

float particleDistance(vec3 pos, in vec3 ro, in vec3 rd)
{
    float t;
    if (intersectSphere(ro, rd, pos, PARTICLE_CULL_DIST_2, t))
    {
        return t;
    }
    else
    {
        return 1e6;
    }
}

void cacheParticle(vec3 pos, vec3 camPos, vec3 nvCamDir)
{
    vec3 closest;
    float dist2 = particleDistance(pos, camPos, camPos + nvCamDir);

    for (int i = 0; i < numCachedParticles; ++i)
    {
        if (length2(cachedParticles[i].xyz - pos) < 0.01)
        {
            return;
        }
    }

    if (numCachedParticles < MAX_CACHED_PARTICLES)
    {
        cachedParticles[numCachedParticles++] = vec4(pos, dist2);
    }
    else
    {
        for (int i = 0; i < MAX_CACHED_PARTICLES; ++i)
        {
            if (cachedParticles[i].w > dist2)
            {
                cachedParticles[i] = vec4(pos, dist2);
                
                return;
            }
        }
    }
}

void cacheParticlesNearRay(vec2 fragCoord, vec3 camPos, vec3 nvRayDir)
{
    numCachedParticles = 0;
    
    // Disable fluid surface
//return;
    ivec4 nb0 = fxGetClosest(ivec2(fragCoord));
    
    for (int i = 0; i < 4; ++i)
    {
        int particleIndex = nb0[i];
        if (particleIndex == -1) continue;
        fxParticle p = fxGetParticle(particleIndex);

        cacheParticle(p.pos, camPos, nvRayDir);
        
        for (int n = 0; n < 8; ++n)
        {
            ivec4 neighborhood = p.neighbors[n];
            for (int j = 0; j < 2; ++j)
            {
                int particleIndex = neighborhood[j];
                if (particleIndex == -1) continue;
                vec3 nPos = fxGetParticleData(particleIndex, POS).xyz;

                cacheParticle(nPos, camPos, nvRayDir);
            }
        }
    }
}

// https://iquilezles.org/articles/smin/
float smin( float a, float b, float k )
{
    float res = exp2( -k*a ) + exp2( -k*b );
    return -log2( res )/k;
}

float sdfMercury(vec3 p, state s) {
    float dglass = sdfContainer(p, s);
    mat3 prMatInv = qToMat(qConj(s.pr));

    float minDist = 1e20;
    for (int i = 0; i < numCachedParticles; ++i)
    {
        minDist = smin(minDist, distance(p, prMatInv*(cachedParticles[i].xyz - s.p) + s.p) - 0.1, 8.0);
    }

    return opSubtraction(opSubtraction(minDist, dglass), dglass);
}

void march(in state s, in vec2 fragCoord, in vec3 pRay, in vec3 nvRayIn, out vec4 color, out vec3 nvRayOut)
{
    // Light (in world coordinates)
    vec3 pLightO = pRay + vec3(0.0, 10.0, 0.0);

    // Light and camera (in object coordinates)
    mat3 prMatInv = qToMat(qConj(s.pr));
    vec3 pCam = prMatInv*(pRay - s.p) + s.p;
    vec3 pLight = prMatInv*(pLightO - s.p) + s.p;

    // Ray while marching (in object coordinates)
    vec3 pCur = pCam;
    vec3 nvRayCur = prMatInv*nvRayIn;

    cacheParticlesNearRay(fragCoord, pRay, nvRayIn);

    color = vec4(0.0);
    int curSubstance = SUBSTANCE_AIR;

    int i=0;
    for (; i<STEPS; i++) {

        // Quick exits
        // ----------------
        vec3 centerToCur = pCur - s.p;
        if (
            (length(centerToCur) > BOUNDING_SPHERE_RADIUS) &&
            (dot(nvRayCur, centerToCur) > 0.0)
        ) { break; }

        if (color.a > 0.95) { break; }
		// ----------------
        
        float sdGlass = sdfGlass(pCur, s);
        float sdMercury = sdfMercury(pCur, s);
        vec3 dpStep = abs(min(sdGlass, sdMercury))*nvRayCur;

        vec3 nvGlass = SDF_NORMAL(sdfGlass, pCur, s);
        vec3 nvMercury = SDF_NORMAL(sdfMercury, pCur, s);

        if (curSubstance == SUBSTANCE_AIR)
        {
            if (sdGlass < SDF_EPS && dot(nvGlass,nvRayCur) < 0.0)
            {
                curSubstance = SUBSTANCE_GLASS;

                vec4 sColor = computeSpecular(
                    0.8, 80.0, nvGlass, normalize(pLight-pCur), normalize(pCam-pCur)
                );
                color = blendOnto(color, sColor);

                // Schlick approximation
                float cosHitAngle = clamp(dot(nvGlass, -nvRayCur), 0.0, 1.0);
                float r0 = pow((IR_GLASS-IR_AIR)/(IR_GLASS+IR_AIR), 2.0);
                float valRefl = mix(r0, 1.0, pow(clamp(1.0 - cosHitAngle, 0.0, 1.0), 3.0)); // Modified exponent 5 -> 3

                vec3 nvRefl = reflect(nvRayCur, nvGlass);
                color = blendOnto(color, valRefl*vec4(texture(iChannel3, nvRefl).rgb, 1.0));

                dpStep = sdGlass*nvRayCur;
                dpStep += -DSTEP_ADJUST_EPS*nvGlass;
                nvRayCur = refractFix(nvRayCur, nvGlass, IR_AIR/IR_GLASS);
            }
            else if (sdMercury < SDF_EPS && dot(nvMercury, nvRayCur) < 0.0)
            {
                vec4 sColor = computeSpecular(
                    1.0, 40.0, nvMercury, normalize(pLight-pCur), normalize(pCam-pCur)
                );
                color = blendOnto(color, sColor);

                nvRayCur = reflect(nvRayCur, nvMercury);
            }
        }
        else if (curSubstance == SUBSTANCE_GLASS)
        {
            float sdGlassInv = -sdGlass;
            vec3 nvGlassInv = -nvGlass;

            dpStep = abs(sdGlassInv)*nvRayCur;

            color = blendOnto(color, clamp(GLASS_OPACITY*sdGlassInv,0.0,1.0)*vec4(GLASS_COLOR, 1.0));

            if (sdGlassInv < SDF_EPS && dot(nvGlassInv,nvRayCur) < 0.0)
            {
                curSubstance = SUBSTANCE_AIR;

                dpStep = sdGlassInv*nvRayCur;
                dpStep += -DSTEP_ADJUST_EPS*nvGlassInv;
                nvRayCur = refractFix(nvRayCur, nvGlassInv, IR_GLASS/IR_AIR);
            }

        } 
        pCur += dpStep;
    }

    // Convert ray direction from object to world coordinates
    nvRayOut = qToMat(s.pr)*nvRayCur;
}

void renderParticle(in vec3 pos, in vec3 camPos, in vec3 nvCamDir, in vec3 color, inout vec4 fragColor)
{    
    float t;
    if (intersectSphere(camPos, nvCamDir, pos, PARTICLE_DEBUG_RENDER_SIZE * PARTICLE_DEBUG_RENDER_SIZE, t))
    {
        fragColor.xyz = color;
    }
}

void renderAllParticles(vec2 fragCoord, vec3 camPos, vec3 nvCamDir, inout vec4 fragColor)
{
#if 1 // Render particles from voronoi buffer
    ivec4 nb0 = fxGetClosest(ivec2(fragCoord));
    
    for (int i = 0; i < 4; ++i)
    {
        int particleIndex = nb0[i];
        if (particleIndex == -1) continue;
        vec4 pos = fxGetParticleData(particleIndex, POS);
        
        vec3 color = vec3(pos.w * pos.w * 0.001, 0.8, 0.0);
        renderParticle(pos.xyz, camPos, nvCamDir, color, fragColor);
    }
#else // Render all particles (slow)
    for (int particleIndex = 0; particleIndex < MAX_PARTICLES; ++particleIndex)
    {
        vec4 pos = fxGetParticleData(particleIndex, POS);
        
        vec3 color = vec3(pos.w * pos.w * 0.001, 0.8, 0.0);
        renderParticle(pos.xyz, camPos, nvCamDir, color, fragColor);
    }
#endif
}

// ===============================
// Main render
// ===============================

vec4 mainColor(vec2 fragCoord, state s) {
    vec3 camPos;
    vec3 nvCamDir;
    vec2 uv = fragCoord / RES.xy;
    getCamera(s, uv, camPos, nvCamDir);

    vec4 color;
    vec3 nvRayOut;
    march(s, fragCoord, camPos, nvCamDir, color, nvRayOut);
    
    // Debug draw particles
    //renderAllParticles(fragCoord, camPos, nvCamDir, color);

    return blendOnto(color, texture(iChannel3, nvRayOut).rgb);

}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    state s = readState();

    fragColor = mainColor(fragCoord, s);

    updateState(s);
    writeState(s, fragCoord, fragColor);
}
