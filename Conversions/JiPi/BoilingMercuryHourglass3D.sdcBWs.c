
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


const float PARTICLE_DEBUG_RENDER_SIZE = 0.03f;
const float PARTICLE_VORONOI_CULL_DIST = 0.3f;
const float PARTICLE_SDF_CULL_DIST = 0.06f;
const int MAX_PARTICLES = 1000;
const float3 GRAVITY = to_float3(-0.00f, -0.15f, -0.1f);
const float DENSITY_SMOOTH_SIZE = 0.5f;
const float PARTICLE_REPEL_SIZE = 0.1f;
const float PARTICLE_COLLISION_SIZE = 0.2f;
const float PARTICLE_MASS = 1.0f;
const float PARTICLE_STIFFNESS = 1.0f;
const float IDEAL_DENSITY = 30.0f;
const float PARTICLE_VISCOSITY = 3.5f;
const float PARTICLE_REPEL = 0.8f;

// ===============================
// Generic Helpers/Constants
// ===============================

#define PI 3.141592653589793
#define TWOPI 6.283185307179586
#define HALFPI 1.570796326794896
#define SQRT2INV 0.7071067811865475

#define POLAR(theta) to_float3(_cosf(theta), 0.0f, _sinf(theta))
#define SPHERICAL(theta, phi) (_sinf(phi)*POLAR(theta) + to_float3_aw(0.0f, _cosf(phi), 0.0f))

// Same as built-in 'refract' (cf. link) but replaces the case which would
// normally result in 0 with a reflection (for total internal reflection)
// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/refract.xhtml
__DEVICE__ float3 refractFix(float3 I, float3 N, float eta) {
    float k = 1.0f - eta * eta * (1.0f - dot(N, I) * dot(N, I));
    return k < 0.0
        ? reflect(I, N) // <- 'refract' returns 0 here
      : eta * I - (eta * dot(N, I) + _sqrtf(k)) * N;
}

__DEVICE__ float4 blendOnto(float4 cFront, float4 cBehind) {
    return cFront + (1.0f - cFront.w)*cBehind;
}

__DEVICE__ float4 blendOnto(float4 cFront, float3 cBehind) {
    return cFront + (1.0f - cFront.w)*to_float4_aw(cBehind, 1.0f);
}

__DEVICE__ float length2(float2 v)
{
    return dot(v, v);
}

__DEVICE__ float length2(float3 v)
{
    return dot(v, v);
}

// ===============================
// Quaternion helpers
// (Unit quaternions: w+xi+yj+zk)
// ===============================

#define QID to_float4(0.0f, 0.0f, 0.0f, 1.0f)

__DEVICE__ float4 slerp(float4 a, float4 b, float t) {
    float d = dot(a, b);
    float4 a2 = a;

    if (d < 0.0f) {
        d = -d;
        a2 = -a;
    }
    if (d > 0.999f) {
        return normalize(_mix(a2, b, t));
    }

    float theta = _acosf(d);
    return (_sinf((1.0f-t)*theta)*a2 + _sinf(t*theta)*b) / _sinf(theta);
}

__DEVICE__ float4 qMul(float4 a, float4 b) {
    return to_float4(
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    );
}

__DEVICE__ float4 qConj(float4 q) {
    return to_float4(-swi3(q,x,y,z), q.w);
}

__DEVICE__ float4 qRot(float3 nvAxis, float angle) {
    return to_float4_aw(nvAxis*_sinf(angle*0.5f), _cosf(angle*0.5f));
}

__DEVICE__ mat3 qToMat(float4 q) {
    float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
    float xx = q.x*q.x, xy = q.x*q.y, xz = q.x*q.z;
    float yy = q.y*q.y, yz = q.y*q.z, zz = q.z*q.z;
    return mat3(
        1.0f - 2.0f*(yy + zz),
             2.0f*(xy + wz),
             2.0f*(xz - wy),

             2.0f*(xy - wz),
        1.0f - 2.0f*(xx + zz),
             2.0f*(yz + wx),

             2.0f*(xz + wy),
             2.0f*(yz - wx),
        1.0f - 2.0f*(xx + yy)
    );
}

// ===============================
// Reading/writing state
// ===============================

struct state {
    float3 p; // Pendulum pivot
    float3 q; // Accelerate p toward this point
    float3 v; // Pendulum "bob" (relative to pivot)
    float3 L; // Angular momentum
    float4 pr; // Object rotation (unit quaternion)
};

state readStateInternal(int iFrame, sampler2D sampler) {
    state s = state(
        to_float3_s(0.0f),
        to_float3_s(0.0f),
        to_float3_aw(0.0f, -_cosf(0.25f*PI), _sinf(0.25f*PI)),
        to_float3(0.0f, 0.5f, 0.0f),
        QID
    );
    if (iFrame > 0) {
        s.p = texelFetch(sampler, to_int2(0, 0), 0).xyz;
        s.q = texelFetch(sampler, to_int2(1, 0), 0).xyz;
        s.v = texelFetch(sampler, to_int2(2, 0), 0).xyz;
        s.L = texelFetch(sampler, to_int2(3, 0), 0).xyz;
        s.pr = texelFetch(sampler, to_int2(4, 0), 0);
    }
    return s;
}

#define readState() readStateInternal(iFrame, iChannel2)

__DEVICE__ void writeState(in state s, in float2 fragCoord, inout float4 fragColor) {
    if (_fabs(fragCoord.y - 0.0f-0.5f) < 0.5f) {
        if (_fabs(fragCoord.x - 0.0f-0.5f) < 0.5f) {
            fragColor = to_float4_aw(s.p, 1.0f);
        } else if (_fabs(fragCoord.x - 1.0f-0.5f) < 0.5f) {
            fragColor = to_float4_aw(s.q, 1.0f);
        } else if (_fabs(fragCoord.x - 2.0f-0.5f) < 0.5f) {
            fragColor = to_float4_aw(s.v, 1.0f);
        } else if (_fabs(fragCoord.x - 3.0f-0.5f) < 0.5f) {
            fragColor = to_float4_aw(s.L, 1.0f);
        } else if (_fabs(fragCoord.x - 4.0f-0.5f) < 0.5f) {
            fragColor = s.pr;
        }
    }
}

// ===============================
// Camera setup
// ===============================

#define RES iResolution
#define TAN_HALF_FOVY 0.5773502691896257

__DEVICE__ float3 nvCamDirFromClip(float3 iResolution, float3 nvFw, float2 clip) {
    float3 nvRt = normalize(cross(nvFw, to_float3(0.0f,1.0f,0.0f)));
    float3 nvUp = cross(nvRt, nvFw);
    return normalize(TAN_HALF_FOVY*(clip.x*(RES.x/RES.y)*nvRt + clip.y*nvUp) + nvFw);
}

__DEVICE__ void getCameraInternal(in state s, in float2 uv, in float iTime, in float3 iResolution, out float3 camPos, out float3 nvCamDir) {
    float2 mouseAng = to_float2(HALFPI*0.75f, PI*0.45f) + 0.2f*to_float2(_cosf(0.5f*iTime),_sinf(0.5f*iTime));
    camPos = to_float3(2.0f, 1.0f, 2.0f) + 5.0f * SPHERICAL(mouseAng.x, mouseAng.y);

    float3 lookTarget = _mix(to_float3_s(0.0f), s.p, 0.05f);
    float3 nvCamFw = normalize(lookTarget - camPos);

    nvCamDir = nvCamDirFromClip(iResolution, nvCamFw, uv*2.0f - 1.0f);
}

#define getCamera(X, Y, Z, W) getCameraInternal(X, Y, iTime, iResolution, Z, W)

// ===============================
// Physics, reading/writing state
// ===============================

__DEVICE__ void updateStateInternal(inout state s, in float4 iMouse, in int iFrame, in float iTime, in float3 iResolution) {

    // pr (object rotation unit quaternion) gets "slerped" towards qr
    float tmod = mod_f(float(iFrame) / 120.0f, 46.0f);
    
#if 0 // Stop auto-flip
    float4 qr = QID;
#else
    float4 qr = (
        tmod < 20.0f ? QID :
        tmod < 23.0f ? qRot(to_float3(-SQRT2INV, 0.0f, SQRT2INV), 0.5f*PI):
        tmod < 43.0f ? qRot(to_float3( 1.0f, 0.0f, 0.0f), PI):
        qRot(to_float3(-SQRT2INV, 0.0f, SQRT2INV), -0.5f*PI)
    );
#endif

    // p (object displacement) gets "lerped" towards q
    if (iMouse.z > 0.5f) {
        float2 uvMouse = swi2(iMouse,x,y) / iResolution;
        float3 camPos;
        float3 nvCamDir;
        getCamera(s, uvMouse, camPos, nvCamDir);

        float t = -camPos.z/nvCamDir.z;
        if (t > 0.0f && t < 50.0f) {
            float3 center = to_float3_s(0.0f);
            float3 hit = camPos + t*nvCamDir;
            float qToCenter = distance(center, s.q);
            float3 delta = hit - s.q;
            if (length2(delta) < 1.0f)
            {
                s.q = hit;
            }
            else
            {
                float angle = -_atan2f(delta.x, delta.y);
                float angle2 = length(swi2(delta,x,y));
                qr = qRot(to_float3(0.0f, 0.0f, 1.0f), angle);
                qr = qMul(qr, qRot(to_float3(1.0f, 0.0f, 0.0f), angle2));
            }
        }
    }

   // apply lerp p -> q and slerp pr -> qr
    float3 vel = 0.25f*(s.q - s.p);
    s.v = vel;
    s.p += vel;
    s.pr = normalize(slerp(s.pr, qr, 0.075f));

    // object acceleration
    float3 a = -0.25f*(s.q - s.p) + to_float3(0.0f, -1.0f, 0.0f);
    mat3 prMatInv = qToMat(qConj(s.pr));
    a = prMatInv*a;

    // hand-wavy torque and angular momentum
    float3 T = cross(s.v, a);
    s.L = 0.96f*s.L + 0.2f*T;
}

#define updateState(X) updateStateInternal(X, iMouse, iFrame, iTime, iResolution)

// ===============================
// Geometry definitions
// ===============================

#define BOUNDING_SPHERE_RADIUS 4.0
#define GLASS_THICKNESS 0.1

__DEVICE__ float sdfPlane(float3 planePoint, float3 nvPlaneN, float3 p) {
    return dot(p - planePoint, nvPlaneN);
}

__DEVICE__ float sdfInterval(float a, float b, float x) {
    return _fabs(x - 0.5f*(a+b)) - 0.5f*(b-a);
}

// From https://iquilezles.org/articles/distfunctions
__DEVICE__ float opSubtraction( float d1, float d2 ) { return _fmaxf(-d1,d2); }
__DEVICE__ float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h);
}


__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); }
    
__DEVICE__ float sdfContainer(float3 p, state s) {
    float3 d0 = _fabs(p - s.p - to_float3(0.0f, 2.09f, 0.0f));
    float3 d1 = _fabs(p - s.p + to_float3(0.0f, 2.09f, 0.0f));
    float s0 = length(d0) - 1.7f;
    float s1 = length(d1) - 1.7f;
    return opSmoothUnion(s0, s1, 1.5f);
}

__DEVICE__ float sdfGlass(float3 p, state s) {
    float etchDepth = 0.0f; // Can sample from e.g. cubemap here for some texture
    return sdfInterval(0.0f, GLASS_THICKNESS - etchDepth, sdfContainer(p, s));
}

#define SDF_N_EPS 0.005
#define SDF_NORMAL(sdfFn, p, s) \
    normalize(to_float3( \
        sdfFn( p+to_float3(SDF_N_EPS,0.0f,0.0f), s ) - sdfFn( p-to_float3(SDF_N_EPS,0.0f,0.0f), s ), \
        sdfFn( p+to_float3(0.0f,SDF_N_EPS,0.0f), s ) - sdfFn( p-to_float3(0.0f,SDF_N_EPS,0.0f), s ), \
        sdfFn( p+to_float3(0.0f,0.0f,SDF_N_EPS), s ) - sdfFn( p-to_float3(0.0f,0.0f,SDF_N_EPS), s )  \
    ))
    
//returns the ids of the four closest particles from the input
int4 fxGetClosestInternal(sampler2D sampler, int2 xy)
{
    return to_int4(texelFetch(sampler, xy, 0));
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
int2 fxLocFromIDInternal(int width, int id, int dataType)
{
    int index = id * NUM_PARTICLE_DATA_TYPES + dataType;
    return to_int2( index % width, index / width);
}

#define fxLocFromID(X, Y) fxLocFromIDInternal(int(iResolution.x), X, Y)

struct fxParticle
{
    float3 pos;
    float density;
    float3 vel;
    float pressure;
    
    int4 neighbors[8];
};

//get the particle corresponding to the input id
fxParticle fxGetParticleInternal(sampler2D sampler, int resolutionWidth, int id)
{
    float4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FUL_NEIGHBORS), 0);
    float4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FUR_NEIGHBORS), 0);
    float4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FLL_NEIGHBORS), 0);
    float4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FLR_NEIGHBORS), 0);
    float4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BUL_NEIGHBORS), 0);
    float4 particleData5 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BUR_NEIGHBORS), 0);
    float4 particleData6 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BLL_NEIGHBORS), 0);
    float4 particleData7 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, BLR_NEIGHBORS), 0);
    float4 particleData8 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS), 0);
    float4 particleData9 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, VEL), 0);

    fxParticle particle;
    particle.pos = swi3(particleData8,x,y,z);
    particle.density = particleData8.w;
    particle.vel = swi3(particleData9,x,y,z);
    particle.pressure = particleData9.w;
    particle.neighbors[0] = ito_float4_aw(particleData0);
    particle.neighbors[1] = to_int4(particleData1);
    particle.neighbors[2] = to_int4(particleData2);
    particle.neighbors[3] = to_int4(particleData3);
    particle.neighbors[4] = to_int4(particleData4);
    particle.neighbors[5] = to_int4(particleData5);
    particle.neighbors[6] = to_int4(particleData6);
    particle.neighbors[7] = to_int4(particleData7);
    
    return particle;
}


__DEVICE__ float4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
    case FUL_NEIGHBORS:
        return to_float4_aw(p.neighbors[0]);
    case FUR_NEIGHBORS:
        return to_float4(p.neighbors[1]);
    case FLL_NEIGHBORS:
        return to_float4(p.neighbors[2]);
    case FLR_NEIGHBORS:
        return to_float4(p.neighbors[3]);
    case BUL_NEIGHBORS:
        return to_float4(p.neighbors[4]);
    case BUR_NEIGHBORS:
        return to_float4(p.neighbors[5]);
    case BLL_NEIGHBORS:
        return to_float4(p.neighbors[6]);
    case BLR_NEIGHBORS:
        return to_float4(p.neighbors[7]);
    case POS:  
        return to_float4(p.pos, p.density);
    case VEL:
        return to_float4_aw(p.vel, p.pressure);
    }
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

__DEVICE__ float4 fxGetParticleDataInternal(sampler2D sampler, int resolutionWidth, int id, int dataType)
{
    return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, dataType), 0);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y)

__DEVICE__ float distanceSquared(float3 a, float3 b)
{
    float3 delta = a - b;
    return dot(delta, delta);
}

__DEVICE__ float hash13(float3 p3)
{
  p3  = fract(p3 * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float hash( int k ) {
    uint n = uint(k);
  n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0f;
}

uvec4 hash(uvec4 x){
    x = ((x >> 16u) ^ swi4(x,y,z,w,x)) * 0x45d9f3bu;
    x = ((x >> 16u) ^ swi4(x,y,z,w,x)) * 0x45d9f3bu;
    x = ((x >> 16u) ^ swi4(x,y,z,w,x)) * 0x45d9f3bu;
    x = ((x >> 16u) ^ swi4(x,y,z,w,x)) * 0x45d9f3bu;
    //x = (x >> 16u) ^ x;
    return x;
}
uvec4 hash(uvec3 x0){
    uvec4 x = swi4(x0,x,y,z,z);
    x = ((x >> 16u) ^ swi4(x,y,z,x,y)) * 0x45d9f3bu;
    x = ((x >> 16u) ^ swi4(x,y,z,x,z)) * 0x45d9f3bu;
    x = ((x >> 16u) ^ swi4(x,y,z,x,x)) * 0x45d9f3bu;
    //x = (x >> 16u) ^ x;
    return x;
}

__DEVICE__ float4 noise(int4 p){
    const float scale = _powf(2.0f, -32.0f);
    uvec4 h = hash(uto_float4_aw(p));
    return to_float4(h)*scale;
}

__DEVICE__ float4 noise(int3 p){
    const float scale = 1.0f/float(0xffffffffU);
    uvec4 h = hash(uto_float3(p));
    return to_float4(h)*scale;
}

__DEVICE__ float4 noise(int2 p){
    return noise(to_int3(p, 0));
}

__DEVICE__ float SPHKernel (float x)
{
    x *= (1.0f / DENSITY_SMOOTH_SIZE);
    if (x < 1.0f)
        return 4.0f * _cosf(x*PI) + _cosf((x + x) * PI) + 3.0f;
    else
        return 0.0f;
}


__DEVICE__ float SPHKernel (float3 deltaPos)
{
    float x = length(deltaPos);
    return SPHKernel(x);
}

__DEVICE__ float SPHgradKernel (float x)
{
    x *= (1.0f / PARTICLE_REPEL_SIZE);

    if (x < 4.0f)
    {
        float xx = x*x;
        float xxx = xx*x;
        float xxxx = xxx*x;
        return PARTICLE_REPEL * (0.000f + 3.333f * x + -3.167f * xx + 0.917f * xxx + -0.083f * xxxx);
    }
    else
        return 0.0f;
}

__DEVICE__ bool intersectSphere(in float3 ro, in float3 rd, in float3 center, in float radius2, out float t) 
{ 
    float t0, t1;  //solutions for t if the ray intersects 
    // geometric solution
    float3 L = center - ro; 
    float tca = dot(L, rd); 
    // if (tca < 0) return false;
    float d2 = dot(L, L) - tca * tca; 
    if (d2 > radius2) return false; 
    float thc = _sqrtf(radius2 - d2); 
    t0 = tca - thc; 
    t1 = tca + thc; 

    if (t0 > t1)
    {
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }
    if (t0 < 0.0f) { 
        t0 = t1;  //if t0 is negative, let's use t1 instead 
        if (t0 < 0.0f) return false;  //both t0 and t1 are negative 
    } 

    t = t0; 

    return true; 
} 

__DEVICE__ float linePointDist2(in float3 newPos, in float3 oldPos, in float3 point, out float3 closest)
{
    float3 pDelta = (point - oldPos);
    float3 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    if (deltaLen2 > 0.0000001f)
    {
        float deltaInvSqrt = inversesqrt(deltaLen2);
        float3 deltaNorm = delta * deltaInvSqrt;
        closest = oldPos + deltaNorm * _fmaxf(0.0f, _fminf(1.0f / deltaInvSqrt, dot(deltaNorm, pDelta)));
    }
    else
    {
        closest = oldPos;
    }

    // Distance to closest point on line segment
    float3 closestDelta = closest - point;
    return dot(closestDelta, closestDelta);
}

#define keyClick(ascii)   ( texelFetch(iChannel3,to_int2(ascii,1),0).x > 0.0f)
#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)

__DEVICE__ void insertion_sort(inout int4 i, inout float4 d, int i_, float d_){  
    if(any(equal(ito_float4_aw(i_),i))) return;
    if     (d_ < d[0])             
        i = to_int4(i_,swi3(i,x,y,z)),    d = to_float4(d_,swi3(d,x,y,z));
    else if(d_ < d[1])             
        i = to_int4(i.x,i_,swi2(i,y,z)), d = to_float4(d.x,d_,swi2(d,y,z));
    else if(d_ < d[2])            
        i = to_int4(swi2(i,x,y),i_,i.z), d = to_float4(swi2(d,x,y),d_,d.z);
    else if(d_ < d[3])           
        i = to_int4(swi3(i,x,y,z),i_),    d = to_float4(swi3(d,x,y,z),d_);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// ---------------------------------------------------------------------------------------
// Computes the position of each particle, one per texture fragment.
// ---------------------------------------------------------------------------------------

void sort0(inout int4 bestIds, inout float4 bestDists, int id, int searchId, int dataType, in float3 myPos);
__KERNEL__ void BoilingMercuryHourglass3DFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{

    int2 iFragCoord = to_int2(fragCoord);
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
            const float3 INIT_CENTER = to_float3(0.0f, 2.0f, 0.0f);
            const float3 INIT_EXTENTS = to_float3(2.0f, 2.0f, 2.0f);
            float3 initPos = (noise(to_int4(int(fragCoord.x) / 2, fragCoord.y, int(fragCoord.x) / 2 + 2, fragCoord.y + 2.0f)).xyz - 0.5f) * INIT_EXTENTS + INIT_CENTER;

            data.pos = prMat*initPos + s.p;
            data.vel = to_float3(0.0f, 0.0f, 0.0f);
        }
        else
        {
            float myDensity = PARTICLE_MASS * SPHKernel(to_float3_aw(0));
            float newDensity = SPHKernel(to_float3(0));
            float3 force = GRAVITY;
            float3 densityGrad = to_float3(0);
            
            float3 avgVel = data.vel * myDensity;

            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 8; i++){
                int4 neighbors = data.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= MAX_PARTICLES) continue;
                    
                    float4 otherPosDensity = fxGetParticleData(cid, POS);
                    float4 otherVelPressure = fxGetParticleData(cid, VEL);
                    float3 deltaPos = data.pos - swi3(otherPosDensity,x,y,z);
                    float dist = length(deltaPos) + 0.001f;
                    float3 dir = deltaPos / dist;
                                            
                    float neighborDensity = PARTICLE_MASS * SPHKernel(deltaPos);
                    newDensity += neighborDensity;
                    avgVel += swi3(otherVelPressure,x,y,z) * neighborDensity;
                    densityGrad += neighborDensity * deltaPos;
                    
                    force -= dir * SPHgradKernel(dist);
                }
            }       
            
            force += 0.1f * avgVel / newDensity - data.vel * 0.3f;
            float overDensity = _fmaxf(0.0f, newDensity - IDEAL_DENSITY);
            force += overDensity * densityGrad * 0.0001f;
            
            // Record misc solver results
            data.density = newDensity;
            data.pressure = PARTICLE_STIFFNESS * (newDensity - IDEAL_DENSITY);            

            // Boundary
            float3 objectPoint = prMatInv * (data.pos - s.p) + s.p;
            float sd = sdfContainer(objectPoint, s);

            if (sd > -PARTICLE_REPEL_SIZE)
            {
                data.density += SPHKernel(sd) * 0.2f;
            }
            if (sd > 0.0f)
            {
                float3 normal = SDF_NORMAL(sdfContainer, objectPoint, s);
                normal = prMat * normal;
                data.pos = data.pos - (sd) * normal;
                data.vel += s.v;
                if(dot(data.vel, normal) > 0.0f)
                {
                    data.vel = reflect(data.vel, normal);
                    //data.vel -= normal * dot(data.vel, normal);
                    data.vel += -0.3f * data.vel;
                }
            }

            // Apply force
            data.vel = data.vel + force;

            // Damping
            data.vel -= data.vel * 0.1f;
            data.vel -= data.vel * length2(data.vel) * 0.1f;
            data.vel -= data.vel * 0.4f * smoothstep(10.0f, 40.0f, data.density);

            // Clamping
            float maxSpeed = 20000.0f / (iResolution.x + iResolution.y); // Dictated by voronoi update speed
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
        int4 nb0 = ito_float4_aw(fxGetParticleData(id, dataType));
        int4 bestIds = to_int4(-1);
        float4 bestDists = to_float4(1e6);
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, nb0[i], dataType, data.pos);
        }
        
        //random sorts
        for (int i = 0; i < 16; ++i)
        {
            int searchId = int(float(MAX_PARTICLES) * hash13(to_float3(iFrame, id, i)));
            sort0(bestIds, bestDists, id, searchId, dataType, data.pos);
        }
        
        fragColor = to_float4_aw(bestIds);
        
        return;
    }
    
    fragColor = fxSaveParticle(data, dataType);
}

__DEVICE__ bool iscoincidence(in int4 bestIds, int currentId, int id)
{
    return id <= 0 ||
        id == currentId ||
        any(equal(bestIds,to_int4(id)));
}

__DEVICE__ void sort0(inout int4 bestIds, inout float4 bestDists, int currentId, int searchId, int dataType, in float3 myPos)
{
    if (searchId >= MAX_PARTICLES) return; // particle recycled
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    float3 nbX = fxGetParticleData(searchId, POS).xyz; 

    float3 dx = nbX - myPos;
    int dir = int((sign(dx.x) * 0.5f + 1.0f) + 2.0f * (sign(dx.y) * 0.5f + 1.0f) + 4.0f * (sign(dx.z) * 0.5f + 1.0f));

    if(dir != dataType) return; //not in this sector
    
    float t = length2(dx);
    
    //if (t > PARTICLE_REPEL_SIZE * 20.0f) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// Gijs's Basic : Voronoi Tracking: https://www.shadertoy.com/view/WltSz7

// Voronoi Buffer
// every pixel stores the 4 closest particles to it
// every frame this data is shared between neighbours

const float PARTICLE_CULL_DIST_2 = PARTICLE_VORONOI_CULL_DIST * PARTICLE_VORONOI_CULL_DIST;

__DEVICE__ float particleDistance(in int id, in float3 ro, in float3 rd)
{
    if(id==-1) return 1e6;

    float3 pos = fxGetParticleData(id, POS).xyz;
    
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

__KERNEL__ void BoilingMercuryHourglass3DFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{

     int2 iFragCoord = to_int2(fragCoord);
    state s = readState();

    float3 camPos;
    float3 nvCamDir;
    float2 uv = fragCoord / swi2(RES,x,y);
    getCamera(s, uv, camPos, nvCamDir);
    
    //in this vector the four new closest particles' ids will be stored
    int4 new = to_int4(-1);
    float4 dis = to_float4(1e6);
        
    int4 old = fxGetClosest(to_int2(fragCoord));
    for (int i = 0; i < 4; ++i)
    {
        float dis2 = particleDistance(old[i], camPos, nvCamDir);
        insertion_sort(new, dis, old[i], dis2);
    }
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            int4 old   = fxGetClosest( iFragCoord + to_int2( x, y) );      

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
    
    fragColor = to_float4(new); 
    


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Cubemap: Forest_0' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Ray marching inspired by Liquid in Glass by tmst https://www.shadertoy.com/view/3tfcRS

// ===============================
// Marching, lighting/materials
// ===============================

#define SDF_EPS 0.01
#define DSTEP_ADJUST_EPS 0.02
#define STEPS 80

#define LIGHT_COLOR to_float3_s(1.0f)

#define GLASS_COLOR to_float3(0.0f, 0.0f, 0.0f)
#define GLASS_OPACITY 0.6

#define IR_AIR 1.0
#define IR_GLASS 1.5

// Increase this few fewer artifacts to give your gfx card a workout
const int MAX_CACHED_PARTICLES = 12;

// Enums
#define SUBSTANCE_AIR 0
#define SUBSTANCE_GLASS 1

__DEVICE__ float4 computeSpecular(
    in float specularCoefficient,
    in float specularExponent,
    in float3 nvNormal,
    in float3 nvFragToLight,
    in float3 nvFragToCam)
{
    float3 blinnH = normalize(nvFragToLight + nvFragToCam);
    float valSpecular = _powf(_fmaxf(0.0f, dot(nvNormal, blinnH)), specularExponent);
    valSpecular *= specularCoefficient;

    return valSpecular*to_float4_aw(LIGHT_COLOR, 1.0f);
}

float4 cachedParticles[MAX_CACHED_PARTICLES];
int numCachedParticles = 0;

const float PARTICLE_CULL_DIST_2 = PARTICLE_SDF_CULL_DIST * PARTICLE_SDF_CULL_DIST;

__DEVICE__ float particleDistance(float3 pos, in float3 ro, in float3 rd)
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

__DEVICE__ void cacheParticle(float3 pos, float3 camPos, float3 nvCamDir)
{
    float3 closest;
    float dist2 = particleDistance(pos, camPos, camPos + nvCamDir);

    for (int i = 0; i < numCachedParticles; ++i)
    {
        if (length2(cachedParticles[i].xyz - pos) < 0.01f)
        {
            return;
        }
    }

    if (numCachedParticles < MAX_CACHED_PARTICLES)
    {
        cachedParticles[numCachedParticles++] = to_float4_aw(pos, dist2);
    }
    else
    {
        for (int i = 0; i < MAX_CACHED_PARTICLES; ++i)
        {
            if (cachedParticles[i].w > dist2)
            {
                cachedParticles[i] = to_float4_aw(pos, dist2);
                
                return;
            }
        }
    }
}

__DEVICE__ void cacheParticlesNearRay(float2 fragCoord, float3 camPos, float3 nvRayDir)
{
    numCachedParticles = 0;
    
    // Disable fluid surface
//return;
    int4 nb0 = fxGetClosest(to_int2(fragCoord));
    
    for (int i = 0; i < 4; ++i)
    {
        int particleIndex = nb0[i];
        if (particleIndex == -1) continue;
        fxParticle p = fxGetParticle(particleIndex);

        cacheParticle(p.pos, camPos, nvRayDir);
        
        for (int n = 0; n < 8; ++n)
        {
            int4 neighborhood = p.neighbors[n];
            for (int j = 0; j < 2; ++j)
            {
                int particleIndex = neighborhood[j];
                if (particleIndex == -1) continue;
                float3 nPos = fxGetParticleData(particleIndex, POS).xyz;

                cacheParticle(nPos, camPos, nvRayDir);
            }
        }
    }
}

// https://iquilezles.org/articles/smin/
__DEVICE__ float smin( float a, float b, float k )
{
    float res = _exp2f( -k*a ) + _exp2f( -k*b );
    return -_log2f( res )/k;
}

__DEVICE__ float sdfMercury(float3 p, state s) {
    float dglass = sdfContainer(p, s);
    mat3 prMatInv = qToMat(qConj(s.pr));

    float minDist = 1e20;
    for (int i = 0; i < numCachedParticles; ++i)
    {
        minDist = smin(minDist, distance(p, prMatInv*(cachedParticles[i].xyz - s.p) + s.p) - 0.1f, 8.0f);
    }

    return opSubtraction(opSubtraction(minDist, dglass), dglass);
}

__DEVICE__ void march(in state s, in float2 fragCoord, in float3 pRay, in float3 nvRayIn, out float4 color, out float3 nvRayOut)
{
    // Light (in world coordinates)
    float3 pLightO = pRay + to_float3(0.0f, 10.0f, 0.0f);

    // Light and camera (in object coordinates)
    mat3 prMatInv = qToMat(qConj(s.pr));
    float3 pCam = prMatInv*(pRay - s.p) + s.p;
    float3 pLight = prMatInv*(pLightO - s.p) + s.p;

    // Ray while marching (in object coordinates)
    float3 pCur = pCam;
    float3 nvRayCur = prMatInv*nvRayIn;

    cacheParticlesNearRay(fragCoord, pRay, nvRayIn);

    color = to_float4_s(0.0f);
    int curSubstance = SUBSTANCE_AIR;

    int i=0;
    for (; i<STEPS; i++) {

        // Quick exits
        // ----------------
        float3 centerToCur = pCur - s.p;
        if (
            (length(centerToCur) > BOUNDING_SPHERE_RADIUS) &&
            (dot(nvRayCur, centerToCur) > 0.0f)
        ) { break; }

        if (color.w > 0.95f) { break; }
    // ----------------
        
        float sdGlass = sdfGlass(pCur, s);
        float sdMercury = sdfMercury(pCur, s);
        float3 dpStep = _fabs(_fminf(sdGlass, sdMercury))*nvRayCur;

        float3 nvGlass = SDF_NORMAL(sdfGlass, pCur, s);
        float3 nvMercury = SDF_NORMAL(sdfMercury, pCur, s);

        if (curSubstance == SUBSTANCE_AIR)
        {
            if (sdGlass < SDF_EPS && dot(nvGlass,nvRayCur) < 0.0f)
            {
                curSubstance = SUBSTANCE_GLASS;

                float4 sColor = computeSpecular(
                    0.8f, 80.0f, nvGlass, normalize(pLight-pCur), normalize(pCam-pCur)
                );
                color = blendOnto(color, sColor);

                // Schlick approximation
                float cosHitAngle = clamp(dot(nvGlass, -nvRayCur), 0.0f, 1.0f);
                float r0 = _powf((IR_GLASS-IR_AIR)/(IR_GLASS+IR_AIR), 2.0f);
                float valRefl = _mix(r0, 1.0f, _powf(clamp(1.0f - cosHitAngle, 0.0f, 1.0f), 3.0f)); // Modified exponent 5 -> 3

                float3 nvRefl = reflect(nvRayCur, nvGlass);
                color = blendOnto(color, valRefl*to_float4(_tex2DVecN(iChannel3,nvRefl.x,nvRefl.y,15).rgb, 1.0f));

                dpStep = sdGlass*nvRayCur;
                dpStep += -DSTEP_ADJUST_EPS*nvGlass;
                nvRayCur = refractFix(nvRayCur, nvGlass, IR_AIR/IR_GLASS);
            }
            else if (sdMercury < SDF_EPS && dot(nvMercury, nvRayCur) < 0.0f)
            {
                float4 sColor = computeSpecular(
                    1.0f, 40.0f, nvMercury, normalize(pLight-pCur), normalize(pCam-pCur)
                );
                color = blendOnto(color, sColor);

                nvRayCur = reflect(nvRayCur, nvMercury);
            }
        }
        else if (curSubstance == SUBSTANCE_GLASS)
        {
            float sdGlassInv = -sdGlass;
            float3 nvGlassInv = -nvGlass;

            dpStep = _fabs(sdGlassInv)*nvRayCur;

            color = blendOnto(color, clamp(GLASS_OPACITY*sdGlassInv,0.0f,1.0f)*to_float4_aw(GLASS_COLOR, 1.0f));

            if (sdGlassInv < SDF_EPS && dot(nvGlassInv,nvRayCur) < 0.0f)
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

__DEVICE__ void renderParticle(in float3 pos, in float3 camPos, in float3 nvCamDir, in float3 color, inout float4 fragColor)
{    
    float t;
    if (intersectSphere(camPos, nvCamDir, pos, PARTICLE_DEBUG_RENDER_SIZE * PARTICLE_DEBUG_RENDER_SIZE, t))
    {
        swi3(fragColor,x,y,z) = color;
    }
}

__DEVICE__ void renderAllParticles(float2 fragCoord, float3 camPos, float3 nvCamDir, inout float4 fragColor)
{
#if 1 // Render particles from voronoi buffer
    int4 nb0 = fxGetClosest(to_int2(fragCoord));
    
    for (int i = 0; i < 4; ++i)
    {
        int particleIndex = nb0[i];
        if (particleIndex == -1) continue;
        float4 pos = fxGetParticleData(particleIndex, POS);
        
        float3 color = to_float3(pos.w * pos.w * 0.001f, 0.8f, 0.0f);
        renderParticle(swi3(pos,x,y,z), camPos, nvCamDir, color, fragColor);
    }
#else // Render all particles (slow)
    for (int particleIndex = 0; particleIndex < MAX_PARTICLES; ++particleIndex)
    {
        float4 pos = fxGetParticleData(particleIndex, POS);
        
        float3 color = to_float3(pos.w * pos.w * 0.001f, 0.8f, 0.0f);
        renderParticle(swi3(pos,x,y,z), camPos, nvCamDir, color, fragColor);
    }
#endif
}

// ===============================
// Main render
// ===============================

__DEVICE__ float4 mainColor(float2 fragCoord, state s) {
    float3 camPos;
    float3 nvCamDir;
    float2 uv = fragCoord / swi2(RES,x,y);
    getCamera(s, uv, camPos, nvCamDir);

    float4 color;
    float3 nvRayOut;
    march(s, fragCoord, camPos, nvCamDir, color, nvRayOut);
    
    // Debug draw particles
    //renderAllParticles(fragCoord, camPos, nvCamDir, color);

    return blendOnto(color, _tex2DVecN(iChannel3,nvRayOut.x,nvRayOut.y,15).rgb);

}

__KERNEL__ void BoilingMercuryHourglass3DFuse__Buffer_C(float4 fragColor, float2 fragCoord, sampler2D iChannel3)
{

    state s = readState();

    fragColor = mainColor(fragCoord, s);

    updateState(s);
    writeState(s, fragCoord, fragColor);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// ---------------------------------------------------------------------------------------
//  Created by fenix in 2022
//  License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
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

__KERNEL__ void BoilingMercuryHourglass3DFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel2)
{

    fragColor = texture(iChannel2, fragCoord/iResolution);


  SetFragmentShaderComputedColor(fragColor);
}