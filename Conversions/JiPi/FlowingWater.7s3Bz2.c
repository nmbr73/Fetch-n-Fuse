
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//amount of particles
const int MAX_PARTICLES = 6000; 
const int PARTICLE_INIT_X = 10;
const float PARTICLE_SIZE = 0.03f;
const float PARTICLE_REPEL_SIZE = 0.010f;
const float MOVING_WALL_MAG = 0.0f;
const float MOVING_WALL_TIME = 3.0f;

//hashing noise by IQ
__DEVICE__ float hash( int k ) {
    uint n = uint(k);
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0f;
}

__DEVICE__ float hash13(float3 p3)
{
    p3  = fract_f3(p3 * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float2 world2screenInternal(in float2 p, in float2 resolution)
{
    return (p + 1.0f) * 0.5f * resolution;
}

#define world2screen(X) world2screenInternal(X, iResolution)

__DEVICE__ float2 screen2worldInternal(in float2 p, in float2 resolution)
{
    return (p / resolution) * 2.0f - 1.0f;
}

#define screen2world(X) screen2worldInternal(X, iResolution)

__DEVICE__ float cross2(float2 a, float2 b)
{
    return a.x * b.y - a.y * b.x;
}

__DEVICE__ float length2(float2 v)
{
    return dot(v, v);
}

__DEVICE__ float linePointDist2(in float2 newPos, in float2 oldPos, in float2 fragCoord, in float3 resolution, out float2 *closest)
{
    float2 pDelta = (fragCoord - oldPos);
    float2 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    if (deltaLen2 > 0.0000001f)
    {
        float deltaInvSqrt = inversesqrt(deltaLen2);
        float2 deltaNorm = delta * deltaInvSqrt;
        *closest = oldPos + deltaNorm * _fmaxf(0.0f, _fminf(1.0f / deltaInvSqrt, dot(deltaNorm, pDelta)));
    }
    else
    {
        *closest = oldPos;
    }

    // Distance to closest point on line segment
    float2 closestDelta = *closest - fragCoord;
    closestDelta *= swi2(resolution,x,y) / resolution.y;
    return length2(closestDelta);
}

const float4 SCENE_LINES[13] = {to_float4(-1.0f, -1.0f, 1.0f, -1.0f),
    to_float4(-1.0f, 1.0f, 1.0f, 1.0f),
    to_float4(-1.0f, -1.0f, -1.0f, 1.0f),
    to_float4(1.0f, -1.0f, 1.0f, 1.0f),
    to_float4(-0.7f, 0.55f, 1.0f, 0.55f),
    to_float4(-0.7f, 0.45f, 1.0f, 0.45f),
    to_float4(-0.7f, 0.55f, -0.7f, 0.45f),
    to_float4(0.7f, 0.05f, -1.0f, 0.05f),
    to_float4(0.7f, -0.05f, -1.0f, -0.05f),
    to_float4(0.7f, 0.05f, 0.7f, -0.05f),
    to_float4(-0.7f, -0.55f, 1.0f, -0.55f),
    to_float4(-0.7f, -0.45f, 1.0f, -0.45f),
to_float4(-0.7f, -0.55f, -0.7f, -0.45f)};
    
const int NUM_SCENE_LINES = 13;

__DEVICE__ bool intersectPlane(float2 p0, float2 p1, float2 p2, float2 p3, in float minT, out float *t, out float2 *n)
{ 
    float2 CmP = p2 - p0;
    float2 r = p1 - p0;
    float2 s = p3 - p2;

    float CmPxr = cross2(CmP, r);
    float CmPxs = cross2(CmP, s);
    float rxs = cross2(r, s);

    if (CmPxr == 0.0f)
    {
        // Lines are collinear, and so intersect if they have any overlap
        return false;
        //return ((C.X - A.X < 0f) != (C.X - B.X < 0f))
          //  || ((C.Y - A.Y < 0f) != (C.Y - B.Y < 0f));
    }

    if (rxs == 0.0f)
        return false; // Lines are parallel.

    float rxsr = 1.0f / rxs;
    *t = CmPxs * rxsr;
    float u = CmPxr * rxsr;

    if (*t >= 0.0f && t <= minT && u >= 0.0f && u <= 1.0f)
    {
        n = normalize(to_float2(-s.y, s.x));
        
        if (rxs < 0.0f) *n = - *n;
        
        return true;
    }
    
    return false;
} 

__DEVICE__ bool intersectScene(float animate, float2 from, float2 to, out float *t, out float2 *n)
{
    float intersectT;
    float2 intersectNormal;

    float minT = 1.0f;
    bool hit = false;
    for (int index = 0; index < NUM_SCENE_LINES; ++index)
    {
        float2 sceneFrom = SCENE_LINES[index].xy;
        float2 sceneTo = SCENE_LINES[index].zw;
        
        if(intersectPlane(from, to, sceneFrom, sceneTo, minT, intersectT, intersectNormal))
        {
            *t = minT = intersectT;
            *n = intersectNormal;
            hit = true;
        }
    }

    return hit;
}

__DEVICE__ float distanceFromWalls(float2 point, float3 resolution, float time)
{
    float minDist = 1e30;
#if 1
    minDist = _fminf(minDist, (point.x + 1.0f - MOVING_WALL_MAG - MOVING_WALL_MAG*_sinf(time / MOVING_WALL_TIME)) * resolution.x / resolution.y);
    minDist = _fminf(minDist, (1.0f - point.x) * resolution.x / resolution.y);
    minDist = _fminf(minDist, point.y + 1.0f);
    minDist = _fminf(minDist, 1.0f - point.y);
#else
    for (int index = 0; index < NUM_SCENE_LINES; ++index)
    {
        float2 sceneFrom = SCENE_LINES[index].xy;
        float2 sceneTo = SCENE_LINES[index].zw;
        
        float2 closest;
        float dist = linePointDist2(sceneFrom, sceneTo, point, resolution, &closest);
        
        if (dist < minDist)
        {
            minDist = dist;
        }
    }
#endif
    return minDist;
}

__DEVICE__ float2 getNormalFromWalls( float2 point, float3 resolution, float time )
{
    float2 tinyChangeX = to_float2( 0.001f, 0.0f );
    float2 tinyChangeY = to_float2( 0.0f , 0.001f );
    
    float upTinyChangeInX   = distanceFromWalls( point + tinyChangeX, resolution, time ); 
    float downTinyChangeInX = distanceFromWalls( point - tinyChangeX, resolution, time ); 
    
    float tinyChangeInX = upTinyChangeInX - downTinyChangeInX;
    
    
    float upTinyChangeInY   = distanceFromWalls( point + tinyChangeY, resolution, time ); 
    float downTinyChangeInY = distanceFromWalls( point - tinyChangeY, resolution, time ); 
    
    float tinyChangeInY = upTinyChangeInY - downTinyChangeInY;
    
    
    float2 normal = to_float2(
                        tinyChangeInX,
                        tinyChangeInY
                             );
    
  return normalize(normal);
}

//returns the ids of the four closest particles from the input
int4 fxGetClosestInternal(__TEXTURE2D__ sampler, int2 xy, float2 iResolution)
{
    return to_int4(texelFetch(sampler, xy, 0));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X)

#define UL_NEIGHBORS 0
#define UR_NEIGHBORS 1
#define LL_NEIGHBORS 2
#define LR_NEIGHBORS 3
#define POS_VEL 4
#define FLUID 5
#define NUM_PARTICLE_DATA_TYPES 6

//returns the location of the particle within the particle buffer corresponding with the input id 
int2 fxLocFromIDInternal(int width, int id, int dataType)
{
    int index = id * NUM_PARTICLE_DATA_TYPES + dataType;
    return to_int2( index % width, index / width);
}

#define fxLocFromID(X, Y) fxLocFromIDInternal(int(iResolution.x), X, Y)

struct fxParticle
{
    float2 pos;
    float2 vel;
    
    int4 neighbors[4];
    float2 uv;
    float density;
    float pressure;
};

//get the particle corresponding to the input id
fxParticle fxGetParticleInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id)
{
    float4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS), 0);
    float4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS), 0);
    float4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS), 0);
    float4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS), 0);
    float4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
    float4 particleData5 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, FLUID), 0);

    fxParticle particle;
    particle.neighbors[0] = to_int4(particleData0);
    particle.neighbors[1] = to_int4(particleData1);
    particle.neighbors[2] = to_int4(particleData2);
    particle.neighbors[3] = to_int4(particleData3);
    particle.pos = swi2(particleData4,x,y);
    particle.vel = swi2(particleData4,z,w);
    particle.uv = swi2(particleData5,x,y);
    particle.density = particleData5.z;
    particle.pressure = particleData5.w;
    
    return particle;
}


__DEVICE__ float4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
    case UL_NEIGHBORS:
        return to_float4_aw(p.neighbors[0]);
    case UR_NEIGHBORS:
        return to_float4(p.neighbors[1]);
    case LL_NEIGHBORS:
        return to_float4(p.neighbors[2]);
    case LR_NEIGHBORS:
        return to_float4(p.neighbors[3]);
    case POS_VEL:  
        return to_float4(p.pos, p.vel);
    case FLUID:
        return to_float4(p.uv, p.density, p.pressure);
    }
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

__DEVICE__ float4 fxGetParticleDataInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id, int dataType)
{
    //return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
	return texture(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL));
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y)

const float PI = 3.141592653589793f;

__DEVICE__ float SPHKernel (float x)
{
    if (x < 1.0f)
        return 4.0f * _cosf(x*PI) + _cosf((x + x) * PI) + 3.0f;
    else
        return 0.0f;
}

__DEVICE__ float SPHgradKernel (float x)
{
    if (x < 4.0f)
    {
        float xx = x*x;
        float xxx = xx*x;
        float xxxx = xxx*x;
        return 0.000f + 3.333f * x + -3.167f * xx + 0.917f * xxx + -0.083f * xxxx;
    }
    else
        return 0.0f;
}

#define keyClick(ascii)   ( texelFetch(iChannel3,to_int2(ascii,1),0).x > 0.0f)
#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)

__DEVICE__ void insertion_sort(inout int4 *i, inout float4 *d, int i_, float d_){  
    //if(any(equal(ito_float4_s(i_),i))) return;
	if( i_ == (*i).x || i_ == (*i).y || i_ == (*i).z || i_ == (*i).w ||) return;
	
    if     (d_ < (*d).x)             
        *i = to_int4(i_,(*i).x,(*i).y,(*i).z),    *d = to_float4(d_,(*d).x,(*d).y,(*d).z);
    else if(d_ < (*d).y)             
        *i = to_int4((*i).x,i_,(*i).y,(*i).z),    *d = to_float4((*d).x,d_,(*d).y,(*d).z);
    else if(d_ < (*d).z)            
        *i = to_int4((*i).x,(*i).y,i_,(*i).z),    *d = to_float4((*d).x,(*d).y,d_,(*d).z);
    else if(d_ < (*d).w)           
        *i = to_int4((*i).x,(*i).y,(*i).z,i_),    *d = to_float4((*d).x,(*d).y,(*d).z,d_);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2


// Particle Buffer
// in this buffer every pixel represents a particle
// the particles positions is stored in .xy
//           its velocity  is stored in .zw
// Only the first PARTICLES amount of pixels are actually used.

const float2 GRAVITY = to_float2(0.0000f, -0.00012f);
const float DAMPING = 1.0f;
const float PARTICLE_REPEL = 0.0001f;
const float WALL_REPEL = 0.0f;
const float IDEAL_DENSITY = 106.0f;

void sort0(inout int4 bestIds, inout float4 bestDists, int id, int searchId, int dataType, in float2 myPos);

__KERNEL__ void FlowingWaterFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame)
{
    fragCoord +=0.5f;

    int2 iFragCoord = to_int2_cfloat(fragCoord);
    
    //we only simulate PARTICLES amount of particles
    int maxParticles = _fminf((int)(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
    int index = iFragCoord.x + iFragCoord.y*(int)(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=maxParticles) return;
    
    fxParticle data = fxGetParticle(id);
    
    if (dataType == POS_VEL || dataType == FLUID)
    {
        if (iFrame == 0 || keyDown(32))
        {
            //pick a "random" starting position
            float particlesPerRow = _sqrtf(float(maxParticles)) * 5.0f;
            float i = float(id % int(particlesPerRow));
            float j = float(id / int(particlesPerRow)) + float(id & 1) * 0.5f;
            float k = float(id % 4);
            
            data.pos = to_float2(i / particlesPerRow, j / particlesPerRow) * to_float2(1.8f, -0.2f) - to_float2(0.9f, -0.8f + 0.5f * k);
            data.vel = to_float2(0);
            data.uv = to_float2(data.pos.x * 0.5f + 0.5f, data.pos.y * 0.5f + 0.5f);
        }
        else
        {
            float2 force = to_float2_s(0);
            
            // Debug forces
            float2 disturbPos = to_float2(0.0f, 0.0f);
            float2 disturbDelta = to_float2(0.0f, 0.0f);
            if (iMouse.z > 0.0f && iMouse.w < 0.0f)
            {
                disturbPos = ((2.0f * swi2(iMouse,x,y) / iResolution) - 1.0f) * to_float2(iResolution.x / iResolution.y, 1.0f);
                disturbDelta = (swi2(iMouse,x,y) - to_float2(iMouse.z, -iMouse.w));
                disturbDelta = clamp(disturbDelta, -100.0f, 100.0f);
            }
            else
            {
                // auto disturb
                //disturbPos = to_float2(_sinf(iTime * 0.5f), _sinf(iTime * 1.0f))* to_float2(1.2f, 0.2f) + to_float2(0.0f, 0.2f);
                //disturbDelta = 80.0f * to_float2(_cosf(iTime * 0.5f), _cosf(iTime * 1.0f));
            }
            
            const float MOUSE_FIELD_SIZE = 0.3f;
            float MOUSE_FIELD_STRENGTH = 0.1f / _sqrtf(iFrameRate);
            float dist = distance(data.pos * iResolution / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution;
            }

            struct solverParticle
            {
                float2 pos;
                float2 vel;
                float density;
                int id;
            };
            
            struct solverParticle particles[17];
            int numSolverParticles = 0;
            float totalDensity = SPHKernel(0.0f);
            float2 densityGrad = to_float2(0);
            
            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 4; i++){
                int4 neighbors = data.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= maxParticles) continue;
                    
                    float4 otherPosVel = fxGetParticleData(cid, POS_VEL);
                    
                    // Don't affect particles on the other side of a wall
                    float2 normal;
                    float t;
                    if (intersectScene(iTime, data.pos, swi2(otherPosVel,x,y), t, normal)) continue;
                    
                    float4 otherFluid = fxGetParticleData(cid, FLUID);
                    
                    float2 deltaPos = swi2(otherPosVel,x,y) - data.pos;
                    float dist = length(deltaPos) + 0.0001f;
                    float nbDensity = SPHKernel(dist);
                    totalDensity += nbDensity;
                    densityGrad += nbDensity * deltaPos / dist;

                    particles[numSolverParticles].pos = swi2(otherPosVel,x,y);
                    particles[numSolverParticles].vel = swi2(otherPosVel,z,w);
                    particles[numSolverParticles].density = otherFluid.z;
                    particles[numSolverParticles].id = cid;
                    ++numSolverParticles;
                }
            }       

            particles[numSolverParticles].pos = data.pos;
            particles[numSolverParticles].vel = data.vel;
            particles[numSolverParticles].id = id;
            ++numSolverParticles;

            // Solve local neighborhood
            float pressure = 0.0f;
            const int NUM_ITERATIONS = 20;
            for(int iterations = 0; iterations < NUM_ITERATIONS; ++iterations)
            {
                for (int i = 0; i < numSolverParticles; ++i)
                {
                    float impulse;
                    for (int j = 0; j < numSolverParticles; ++j)
                    {
                        if (i != j)
                        {
                            float2 deltaPos = particles[i].pos - particles[j].pos;
                            float dist = length(deltaPos) + 0.001f;
                            float2 dir = deltaPos / dist; 

                            impulse = (PARTICLE_REPEL * SPHgradKernel(dist / PARTICLE_REPEL_SIZE));
                                                        
                            particles[i].vel += impulse * dir;
                            particles[j].vel -= impulse * dir;
                        }
                    }
                    
                    // Last particle is the one we're working on
                    pressure += impulse;
                }
                
                for (int i = 0; i < numSolverParticles; ++i)
                {
                    // Integrate vel
                    particles[i].vel -= particles[i].vel * 0.000004f - GRAVITY / float(NUM_ITERATIONS);
                    
                    // Integrate pos
                    particles[i].pos += particles[i].vel / float(NUM_ITERATIONS);
                }
            }
            
            // Combine solver results into force
            force += particles[numSolverParticles - 1].vel - data.vel;
            
            // Record misc solver results
            data.pressure = pressure;
            data.density = totalDensity;
                   
            // Apply force
            data.vel = data.vel + force;
            
            // Boundary
            float distToScene = distanceFromWalls(data.pos, iResolution, iTime);
            float distToSceneOld = distanceFromWalls(data.pos, iResolution, iTime - iTimeDelta);
            float2 distNormal = getNormalFromWalls(data.pos, iResolution, iTime);

            if (distToScene < PARTICLE_REPEL_SIZE)
            {
                data.pos -= 1.0f * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                data.vel -= 1.0f * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                float velToClip = dot(data.vel, distNormal);
                if (velToClip > 0.0f)
                {
                    data.vel -= distNormal * (distToSceneOld - distToScene);                    
                }
            }
            
            const int NUM_SCENE_ITERATIONS = 4;
            for (int i = 0; i < NUM_SCENE_ITERATIONS; ++i)
            {
                float t;
                float2 normal;
                float2 newPos = data.pos + data.vel;
                if (intersectScene(iTime, data.pos, newPos, t, normal))
                {    
                    float2 intersection = data.pos + t * data.vel;
                    float2 reflected = intersection + (1.0f - t) * reflect(data.vel, normal);
                    data.vel = reflect(data.vel, normal);
                }
            }

            // Damping
            data.vel -= data.vel * length2(data.vel) * 200.0f;
            data.vel -= data.vel * 0.9f * smoothstep(50.0f, 100.0f, data.density);

            // Clamping
            float maxSpeed = 30.0f / (iResolution.x + iResolution.y); // Dictated by voronoi update speed
            float velLength2 = length2(data.vel);
            if (velLength2 > maxSpeed * maxSpeed)
            {
                data.vel *= inversesqrt(velLength2) * maxSpeed;
            }

            // Integrate position
            data.pos = data.pos + data.vel;
            data.pos = clamp(data.pos, -1.0f, 1.0f);
            
            if (data.pos.x > 0.7f && data.pos.y < -0.99f)
            {
                data.pos.y += 1.98f;
            }
        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        float4 nb0 = fxGetParticleData(id, dataType);
        int4 closest = fxGetClosest(to_int2(world2screen(data.pos)));
        int4 bestIds = ito_float4_aw(nb0);
        float4 bestDists = to_float4(length2(fxGetParticleData(bestIds[0], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[1], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[2], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[3], POS_VEL).xy - data.pos));

        //random sorts
        for (int i = 0; i < 4; ++i)
        {
            int searchId = int(float(iResolution.x*iResolution.y)*hash13(to_float3(iFrame, id, i)));
            sort0(bestIds, bestDists, id, searchId, dataType, data.pos);
        }
        
        //see if the rendering buffer found anything better
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, int(closest[i]), dataType, data.pos);
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

__DEVICE__ void sort0(inout int4 bestIds, inout float4 bestDists, int currentId, int searchId, int dataType, in float2 myPos)
{
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    float2 nbX = fxGetParticleData(searchId, POS_VEL).xy; 

    float2 dx = nbX - myPos;
    int dir = int(2.0f*(_atan2f(dx.y, dx.x)+PI)/PI); 

    if(dir != dataType) return; //not in this sector
    
    float t = length2(dx);
    
    if (t > PARTICLE_REPEL_SIZE * 20.0f) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Gijs's Basic : Voronoi Tracking: https://www.shadertoy.com/view/WltSz7

// Voronoi Buffer
// every pixel stores the 4 closest particles to it
// every frame this data is shared between neighbours

__DEVICE__ float distance2Particle(int id, float2 fragCoord){
    if(id==-1) return 1e20;
    float2 delta = fxGetParticleData(id, POS_VEL).xy-fragCoord;
    return dot(delta, delta);
}

__KERNEL__ void FlowingWaterFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
    fragCoord +=0.5f;
	
    int2 iFragCoord = to_int2_cfloat(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    int4 new = fxGetClosest(to_int2_cfloat(fragCoord));
    //in this vector the distance to these particles will be stored 
    float4 dis = to_float4_aw(distance(fxGetParticleData(new[0], POS_VEL).xy, screen2world(fragCoord)),
        distance(swi2(fxGetParticleData(new[1], POS_VEL),x,y), screen2world(fragCoord)),
        distance(swi2(fxGetParticleData(new[2], POS_VEL),x,y), screen2world(fragCoord)),
        distance(swi2(fxGetParticleData(new[3], POS_VEL),x,y), screen2world(fragCoord)));
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            int4 old   = fxGetClosest( iFragCoord + to_int2( x, y) );      

            for(int j=0; j<4; j++){
                int id = old[j];
                float dis2 = distance2Particle(id, screen2world(fragCoord));
                insertion_sort( new, dis, id, dis2 );
            }
        }
    }
    
    int searchIterations = 1;
    if (iFrame < 5)
    {
        searchIterations = 10;
    }
    for(int k = 0; k < searchIterations; k++){
        //random hash. We should make sure that two pixels in the same frame never make the same hash!
        float h = hash(
            iFragCoord.x + 
            iFragCoord.y*int(iResolution.x) + 
            iFrame*int(iResolution.x*iResolution.y) +
            k
        );
        int maxParticles = _fminf(iFragCoord.x * iFragCoord.y / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
        //pick random id of particle
        int p = int(h*float(maxParticles));
        insertion_sort(new, dis, p, distance2Particle(p, screen2world(fragCoord)));
    }
    
    fragColor = to_float4(new); 
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__DEVICE__ float3 renderLine(in float2 from, in float2 to, in float3 color, in float size, in float2 fragCoord)
{
    float2 closest;
    float dist = linePointDist2(from, to, fragCoord, iResolution, &closest);
    return color * _fmaxf(0.0f, (size - _sqrtf(dist)) / (size));
}

__DEVICE__ float3 renderParticle(in fxParticle p, in float2 fragCoord)
{   
    //if (p.density < 50.0f) return;
    int maxParticles = _fminf(int(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);

    //swi3(fragColor,x,y,z) += 1000.2f * p.pressure * particleColor(p.uv) * _fmaxf(0.0f, PARTICLE_SIZE - _sqrtf(dist)) / PARTICLE_SIZE;
    float3 color = to_float3(1.0f, 1.0f, 1.0f);//to_float3(150.0f*p.pressure, 10000.0f*dot(p.vel, p.vel), 0.000012f*p.density*p.density*p.density);
    float3 fragColor = renderLine(p.pos, p.pos - p.vel, color, PARTICLE_SIZE, fragCoord);
    
    // Render neighbor lines
    #if 0
    for(int i = 0; i < 4; i++){
        int4 neighbors = p.neighbors[i];
        for (int j = 0; j < 4; ++j)
        {
            int cid = neighbors[j];
            if(cid==-1 || cid >= maxParticles || cid == 0) continue;

            float2 otherPos = fxGetParticleData(cid, POS_VEL).xy;

            if (length(otherPos - p.pos) < 0.1f)
            {
                //float distToLin = linePointDist2(p.pos, p.pos + 0.5f * (otherPos - p.pos), fragCoord, iResolution, closest);
                fragColor += renderLine(p.pos, p.pos + 0.5f * (otherPos - p.pos), color, PARTICLE_SIZE * 0.3f, fragCoord);//color * _fmaxf(0.0f, PARTICLE_SIZE * 0.3f - _sqrtf(distToLin)) / (PARTICLE_SIZE);
            }
        }
    }
    #endif
    
    return fragColor;
}

__KERNEL__ void FlowingWaterFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    fragCoord +=0.5f;
    
    fragColor = to_float4_s(0.0f);
    
    float2 p = (2.0f*fragCoord-iResolution)/iResolution;

#if 0
    for (int line = 0; line < NUM_SCENE_LINES; ++line)
    {
        swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + renderLine(swi2(SCENE_LINES[line],x,y), swi2(SCENE_LINES[line],z,w), to_float3(1.0f, 0.0f, 0.0f), PARTICLE_SIZE, p));
    }
#endif

    //get the id's of the 4 particles that (should be) closest.
    //the 4 ids are stored in .x, .y, .z, .w
    int4 ids = fxGetClosest(to_int2_cfloat(fragCoord));
    
    //draw the particles
    for(int i = 0; i < 4; i++){
        //get the particles position
        int id = ids[i];
        fxParticle particle = fxGetParticle(id);

        swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + renderParticle(particle, p));
    }
    
    //swi3(fragColor,x,y,z) = to_float3(distanceFromWalls(p, iResolution, iTime));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// gaussian blur from FabriceNeyret2's smart gaussian blur: https://www.shadertoy.com/view/WtKfD3

int           N = 11; // 7                              // target sampling rate
float         w = 0.1f,                                   // filter width
              z;                                        // LOD MIPmap level to use for integration 
#define init  w = 0.02f; \
              z = _ceil(_fmaxf(0.0f,_log2f(w*R.y/float(N))));   // N/w = res/2^z
#define R     iResolution


__DEVICE__ float convol2D(float2 U) {                                                     
    float  O = 0.0f;  
    float r = float(N-1)/2.0f, g, t=0.0f;                                       
    for( int k=0; k<N*N; k++ ) {                                            
        float2 P = to_float2(k%N,k/N) / r - 1.0f;                                    
        t += g = _expf(-2.0f*dot(P,P) );                                        
        O += g * textureLod(iChannel0, (U+w*P) *R.y/R, z ).x;                 
    }                                                                       
    return O/t;                                                             
}      

__KERNEL__ void FlowingWaterFuse__Buffer_D(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{
    u+=0.5f;
	
    init 
    float2 U = u / R.y;  
    O = texture(iChannel0, u / iResolution);
    O.x = convol2D(U); return;
    //  O = convol1D(U,to_float2(1,0));

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image 'Texture: Wood' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel0


// ---------------------------------------------------------------------------------------
//  Created by fenix in 2022
//  License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
//  Multipass particle physics simulation, attempting to approximate smoothed particle
//  hydrodynamics. 
// 
//  Buffer A computes the particle positions and neighbors
//  Buffer B does a traditional voronoi search to help out building neighborhoods
//  Buffer C renders the particles into a density texture
//  Buffer D blurs the density
//
// ---------------------------------------------------------------------------------------

__DEVICE__ float2 getGradFromHeightMap( float2 point )
{
    float2 tinyChangeX = to_float2( 0.002f, 0.0f );
    float2 tinyChangeY = to_float2( 0.0f , 0.002f );
    
    float upTinyChangeInX0   = texture(iChannel0, point + tinyChangeX).x; 
    float upTinyChangeInX1   = texture(iChannel0, point + tinyChangeX + tinyChangeX).x; 
    float downTinyChangeInX0 = texture(iChannel0, point - tinyChangeX).x; 
    float downTinyChangeInX1 = texture(iChannel0, point - tinyChangeX - tinyChangeX).x; 
    
    float tinyChangeInX = upTinyChangeInX0 + upTinyChangeInX1 - downTinyChangeInX0 - downTinyChangeInX1;
    
    
    float upTinyChangeInY0   = texture(iChannel0, point + tinyChangeY).x; 
    float upTinyChangeInY1   = texture(iChannel0, point + tinyChangeY + tinyChangeY).x; 
    float downTinyChangeInY0 = texture(iChannel0, point - tinyChangeY).x; 
    float downTinyChangeInY1 = texture(iChannel0, point - tinyChangeY - tinyChangeY).x; 
    
    float tinyChangeInY = upTinyChangeInY0 + upTinyChangeInY1 - downTinyChangeInY0 - downTinyChangeInY1;
    
    
    float2 normal = to_float2(
                   tinyChangeInX,
                   tinyChangeInY
                             );
    
  return normal * 0.5f;
}

__KERNEL__ void FlowingWaterFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    float2 p = fragCoord/iResolution;
    float density = textureLod(iChannel0, p, 1.0f).x;
    if (p.x > 0.15f && ((p.y > 0.725f && p.y < 0.775f) || (p.y > 0.225f && p.y < 0.275f))
        || p.x < 0.85f && (p.y > 0.475f && p.y < 0.525f))
    {
        fragColor = _tex2DVecN(iChannel2,p.x,p.y,15);
    }
    else
    {
        float4 background = texture(iChannel1, p);
        float2 grad = getGradFromHeightMap(p);
        if (p.y < 0.25f) grad.y -= p.y - 0.25f; // Hack to make bottom layer more visible
        float4 water = texture(iChannel1, p + grad * 0.7f); // Refract :)
        fragColor = _mix(background, water, smoothstep(0.5f, 1.0f, density)); // Blur water edges
    }
    //fragColor = _tex2DVecN(iChannel0,p.x,p.y,15);

  SetFragmentShaderComputedColor(fragColor);
}