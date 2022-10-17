
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//amount of particles
__DEVICE__ const int MAX_PARTICLES = 5000; 
__DEVICE__ const float PARTICLE_REPEL_SIZE = 0.01f;

__DEVICE__ const float ANIMATE_FRAMES = 3600.0f;

#define PI 3.141592653598793f


union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };

union A2I
 {
   int4  I;     //32bit Integer
   int   A[4];  //32bit integer Array
 };

union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };

//hashing noise by IQ
__DEVICE__ float hash( int k ) {
  
  Zahl z;
  
  uint n = (uint)(k);
  n = (n << 13U) ^ n;
  n = n * (n * n * 15731U + 789221U) + 1376312589U;
  
  z._Uint = (n>>9U) | 0x3f800000U;
  
  //return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0f;
  return z._Float - 1.0f;
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

__DEVICE__ float linePointDist2(in float2 newPos, in float2 oldPos, in float2 fragCoord, in float2 resolution, out float2 *closest)
{
    float2 pDelta = (fragCoord - oldPos);
    float2 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    //*closest;  //????????????????????????????????
    if (deltaLen2 > 0.0000001f)
    {
        float deltaInvSqrt = 1.0f/_sqrtf(deltaLen2); // inversesqrt(deltaLen2);
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
    return dot(closestDelta, closestDelta);
}

__DEVICE__ float sdBox(in float2 p, in float2 boxCenter, in float2 boxSize)
{
    p -= boxCenter;
    float2 d = abs_f2(p)-boxSize;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float sdHorseshoe( in float2 p, in float2 c, in float r, in float2 w )
{
    p.x = _fabs(p.x);
    float l = length(p);
    p = mul_mat2_f2(to_mat2(-c.x, c.y, c.y, c.x),p);
    p = to_float2((p.y>0.0f || p.x>0.0f)?p.x:l*sign_f(-c.x), (p.x>0.0f)?p.y:l );
    p = to_float2(p.x,_fabs(p.y-r))-w;
    
    return length(_fmaxf(p, to_float2_s(0.0f) )) + _fminf(0.0f,_fmaxf(p.x,p.y));
}



__DEVICE__ float distanceFromWalls(float2 point, float2 resolution, float time)
{
  
  const float h1ang = 1.8f;
  const mat2 h1rot  = to_mat2(_cosf(h1ang), -_sinf(h1ang), _sinf(h1ang), _cosf(h1ang));
  const float h2ang = PI - h1ang;
  const mat2 h2rot  = to_mat2(_cosf(h2ang), -_sinf(h2ang), _sinf(h2ang), _cosf(h2ang));
  const float lang  = 1.7f;
  const mat2 lrot   = to_mat2(_cosf(lang), -_sinf(lang), _sinf(lang), _cosf(lang));
  
  
    time = mod_f(time, ANIMATE_FRAMES);
    float rightTime = _fminf(ANIMATE_FRAMES * 0.45f, time);
    float leftTime = _fminf(ANIMATE_FRAMES * 0.45f, time - ANIMATE_FRAMES * 0.5f);
    if (leftTime > 0.0f) rightTime = 0.0f;
    const float COMPRESS_RATE = 1.0f / (0.55f * ANIMATE_FRAMES);
    float minDist = 1e30;
    point.y *= resolution.y / resolution.x;
    point *= 2.0f;
    minDist = _fminf(minDist, point.x + 1.9f);
    minDist = _fminf(minDist, 1.9f - point.x);
    minDist = _fminf(minDist, point.y + 1.0f);
    minDist = _fminf(minDist, 1.0f - point.y);
    minDist = _fminf(minDist, sdBox(point, to_float2(0.0f, 0.0f), to_float2(1.35f, 5.00f)));
    minDist = _fminf(minDist, sdBox(point, to_float2(0.8f, 2.0f - rightTime * COMPRESS_RATE), to_float2(2.0f, 1.0f)));
    minDist = _fminf(minDist, sdBox(point, to_float2(0.8f, -2.0f + rightTime * COMPRESS_RATE), to_float2(2.0f, 1.0f)));
    minDist = _fminf(minDist, sdBox(point, to_float2(-0.8f, 2.0f - leftTime * COMPRESS_RATE), to_float2(2.0f, 1.0f)));
    minDist = _fminf(minDist, sdBox(point, to_float2(-0.8f, -2.0f + leftTime * COMPRESS_RATE), to_float2(2.0f, 1.0f)));
    minDist = _fmaxf(minDist, -sdHorseshoe(mul_mat2_f2(h1rot,(point - to_float2(-0.67f, 0.16f))),  to_float2(0.13f, 1.0f), 0.1f, to_float2(0.8f, 0.04f)));
    minDist = _fmaxf(minDist, -sdHorseshoe(mul_mat2_f2(h2rot,(point - to_float2(-0.07f, -0.14f))), to_float2(0.13f, 1.0f), 0.1f, to_float2(0.8f, 0.04f)));
    minDist = _fmaxf(minDist, -sdHorseshoe(mul_mat2_f2(h1rot,(point - to_float2(0.53f, 0.16f))),   to_float2(0.13f, 1.0f), 0.1f, to_float2(0.8f, 0.04f)));
    minDist = _fmaxf(minDist, -sdHorseshoe(mul_mat2_f2(h2rot,(point - to_float2(1.13f, -0.14f))),  to_float2(0.13f, 1.0f), 0.1f, to_float2(0.8f, 0.04f)));
    minDist = _fmaxf(minDist, -sdBox(mul_mat2_f2(lrot,(point - to_float2(1.3f, 0.04f))), to_float2(0.0f, 0.0f), to_float2(0.04f, 0.4f)));
    minDist = _fmaxf(minDist, -sdBox(mul_mat2_f2(lrot,(point - to_float2(1.4f, 0.02f))), to_float2(0.0f, 0.0f), to_float2(0.04f, 0.3f)));
    return minDist;
}

__DEVICE__ float2 getNormalFromWalls( float2 point, float2 resolution, float time )
{
  float2 tinyChangeX = to_float2( 0.001f, 0.0f );
  float2 tinyChangeY = to_float2( 0.0f , 0.001f );
    
  float upTinyChangeInX   = distanceFromWalls( point + tinyChangeX, resolution, time ); 
  float downTinyChangeInX = distanceFromWalls( point - tinyChangeX, resolution, time ); 
    
  float tinyChangeInX = upTinyChangeInX - downTinyChangeInX;
   
  float upTinyChangeInY   = distanceFromWalls( point + tinyChangeY, resolution, time ); 
  float downTinyChangeInY = distanceFromWalls( point - tinyChangeY, resolution, time ); 
    
  float tinyChangeInY = upTinyChangeInY - downTinyChangeInY;
    
  float2 normal = to_float2( tinyChangeInX, tinyChangeInY );
    
  return normalize(normal);
}

//returns the ids of the four closest particles from the input
__DEVICE__ int4 fxGetClosestInternal(__TEXTURE2D__ sampler, int2 xy, float2 iResolution)
{
    //return to_int4(texelFetch(sampler, xy, 0));
    return to_int4_cfloat(texture(sampler, (make_float2(xy)+0.5f)/iResolution));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X, iResolution)

#define UL_NEIGHBORS 0
#define UR_NEIGHBORS 1
#define LL_NEIGHBORS 2
#define LR_NEIGHBORS 3
#define POS_VEL 4
#define NUM_PARTICLE_DATA_TYPES 5

//returns the location of the particle within the particle buffer corresponding with the input id 
__DEVICE__ int2 fxLocFromIDInternal(int width, int id, int dataType)
{
  
  int index = id * NUM_PARTICLE_DATA_TYPES + dataType;
  return to_int2( index % width, index / width);
}

#define fxLocFromID(X, Y) fxLocFromIDInternal((int)(iResolution.x), X, Y)

struct fxParticle
{
    float2 pos;
    float2 vel;
    
    int4 neighbors[4];
};

//get the particle corresponding to the input id
__DEVICE__ fxParticle fxGetParticleInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id, float2 iResolution)
{
#ifdef XXX  
    float4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS), 0);
    float4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS), 0);
    float4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS), 0);
    float4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS), 0);
    float4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
#endif

    float4 particleData0 = texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData1 = texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData2 = texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData3 = texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData4 = texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, POS_VEL))+0.5f)/iResolution);


    fxParticle particle;
    particle.neighbors[0] = to_int4_cfloat(particleData0);
    particle.neighbors[1] = to_int4_cfloat(particleData1);
    particle.neighbors[2] = to_int4_cfloat(particleData2);
    particle.neighbors[3] = to_int4_cfloat(particleData3);
    particle.pos = swi2(particleData4,x,y);
    particle.vel = swi2(particleData4,z,w);
    
    return particle;
}


__DEVICE__ float4 fxSaveParticle(fxParticle p, int dataType)
{    
    
    //float4 pneighbors = to_float4_cint(p.neighbors);
    
    switch(dataType)
    {
    case UL_NEIGHBORS:
        return to_float4_cint(p.neighbors[0]);
    case UR_NEIGHBORS:
        return to_float4_cint(p.neighbors[1]);
    case LL_NEIGHBORS:
        return to_float4_cint(p.neighbors[2]);
    case LR_NEIGHBORS:
        return to_float4_cint(p.neighbors[3]);
    case POS_VEL:  
        return to_float4_f2f2(p.pos, p.vel);
    }
    return to_float4_s(0.0f); // Nothing
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, (int)(iResolution.x), X, iResolution)

__DEVICE__ float4 fxGetParticleDataInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id, int dataType, float2 iResolution)
{
  
    //return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
    return texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, POS_VEL))+0.5f)/iResolution);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, (int)(iResolution.x), X, Y, iResolution)

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

__DEVICE__ float length2(float2 v)
{
  
    return dot(v, v);
}



__DEVICE__ void insertion_sort(inout int4 *i, inout float4 *d, int i_, float d_){  
    //if(any(equal(ito_float4_s((float)i_),*i))) return;
    if(i_ == (*i).x || i_ == (*i).y || i_ == (*i).z || i_ == (*i).w ) return;
    if     (d_ < (*d).x)             
        //*i = to_int4(i_,swi3(*i,x,y,z)),    *d = to_float4(d_,swi3(d,x,y,z));
        *i = to_int4(i_, (*i).x,(*i).y,(*i).z),   *d = to_float4(d_,(*d).x,(*d).y,(*d).z);
    else if(d_ < (*d).y)             
        //*i = to_int4((*i).x,i_,swi2(*i,y,z)),  *d = to_float4((*d).x,d_,swi2(*d,y,z));
        *i = to_int4((*i).x,i_,(*i).y, (*i).z),  *d = to_float4((*d).x,d_,(*d).y, (*d).z);
    else if(d_ < (*d).z)            
        //*i = to_int4(swi2(*i,x,y),i_,(*i).z),  *d = to_float4(swi2(*d,x,y),d_,(*d).z);
        *i = to_int4((*i).x, (*i).y, i_, (*i).z), *d = to_float4((*d).x,(*d).y,d_,(*d).z);
    else if(d_ < (*d).w)           
        //*i = to_int4(swi3(*i,x,y,z),i_),    *d = to_float4(swi3(*d,x,y,z),d_);
        *i = to_int4((*i).x, (*i).y, (*i).z, i_), *d = to_float4_aw(swi3(*d,x,y,z),d_);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel3


// Particle Buffer
// in this buffer every pixel represents a particle
// the particles positions is stored in .xy
//           its velocity  is stored in .zw
// Only MAX_PARTICLES * NUM_PARTICLE_DATA_TYPES pixels are actually used.

__DEVICE__ const float PARTICLE_REPEL = 0.00005f;
__DEVICE__ const float IDEAL_DENSITY = 20.0f;

__DEVICE__ bool iscoincidence(in int4 bestIds, int currentId, int id)
{
  
  return id <= 0 ||
         id == currentId ||
        (bestIds.x == id || bestIds.y == id || bestIds.z == id || bestIds.w == id );
        
        //any(equal(bestIds,to_int4(id)));
}

__DEVICE__ void sort0(inout int4 *bestIds, inout float4 *bestDists, int currentId, int searchId, int dataType, in float2 myPos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    if(iscoincidence(*bestIds, currentId, searchId)) return; //particle already sorted
    
    float2 nbX = swi2(fxGetParticleData(searchId, POS_VEL),x,y);

    float2 dx = nbX - myPos;
    int dir = (int)(2.0f*(_atan2f(dx.y, dx.x)+PI)/PI); 

    if(dir != dataType) return; //not in this sector
    
    float t = length2(dx);
    
    //if (t > PARTICLE_REPEL_SIZE * 20.0f) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);
}

struct solverParticle
{
    float2 pos;
    float2 vel;
};


__KERNEL__ void TeslaValveFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;
    
    int2 iFragCoord = to_int2_cfloat(fragCoord);

    int maxParticles = _fminf((int)(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
    int index = iFragCoord.x + iFragCoord.y*(int)(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=maxParticles)
    {
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    float4 state = texture(iChannel3, to_float2_s(0));
    float frames = state.x;
 
    fxParticle data = fxGetParticle(id);
    
    if (dataType == POS_VEL)
    {
        if (iFrame == 0 || frames == 0.0f)
        {
            //pick a "random" starting position
            float particlesPerRow = _sqrtf((float)(maxParticles));
            float i = (float)(id % (int)(particlesPerRow));
            float j = (float)(id / (int)(particlesPerRow)) + (float)(id & 1) * 0.5f;
            
            data.pos = to_float2(i / particlesPerRow, j / particlesPerRow) * to_float2(0.1f, 1.8f) + to_float2(0.8f, -0.9f);
            data.vel = to_float2_s(0);
        }
        else if (iFrame == 0 || frames == ANIMATE_FRAMES * 0.5f)
        {
            //pick a "random" starting position
            float particlesPerRow = _sqrtf((float)(maxParticles));
            float i = (float)(id % (int)(particlesPerRow));
            float j = (float)(id / (int)(particlesPerRow)) + (float)(id & 1) * 0.5f;
            
            data.pos = to_float2(i / particlesPerRow, j / particlesPerRow) * to_float2(0.1f, 1.8f) + to_float2(-0.9f, -0.9f);
            data.vel = to_float2_s(0);
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
            
            const float MOUSE_FIELD_SIZE = 0.3f;
            float MOUSE_FIELD_STRENGTH = 1.0f;//0.3f / _sqrtf(iFrameRate); //Not defined
            float dist = distance_f2(data.pos * iResolution / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution;
            }


            
            solverParticle particles[17];
            int numSolverParticles = 0;
            float totalDensity = SPHKernel(0.0f);
           
            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 4; i++){
                //int4 neighbors = data.neighbors[i];
                A2I neighbors;
                neighbors.I = data.neighbors[i];
                
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors.A[j]; //neighbors[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= maxParticles) continue;
                    
                    float4 otherPosVel = fxGetParticleData(cid, POS_VEL);
                    
                    float2 deltaPos = swi2(otherPosVel,x,y) - data.pos;
                    float dist = length(deltaPos) + 0.0001f;
                    float nbDensity = SPHKernel(dist);
                    totalDensity += nbDensity;

                    particles[numSolverParticles].pos = swi2(otherPosVel,x,y);
                    particles[numSolverParticles].vel = swi2(otherPosVel,z,w);
                    ++numSolverParticles;
                }
            }       

            particles[numSolverParticles].pos = data.pos;
            particles[numSolverParticles].vel = data.vel;
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
                    // Integrate pos
                    particles[i].pos += particles[i].vel / (float)(NUM_ITERATIONS);
                }
            }
            
            // Combine solver results into force
            force += particles[numSolverParticles - 1].vel - data.vel;
                   
            // Apply force
            data.vel = data.vel + force;
            
            // Boundary
            float distToScene = distanceFromWalls(data.pos, iResolution, frames);
            float distToSceneOld = distanceFromWalls(data.pos, iResolution, frames - 1.0f);
            float2 distNormal = getNormalFromWalls(data.pos, iResolution, frames);

            if (distToScene < PARTICLE_REPEL_SIZE)
            {
                data.pos -= 1.0f * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                data.vel -= 1.0f * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
            }
            
            // Integrate position
            data.pos = data.pos + data.vel;
            data.pos = clamp(data.pos, -1.0f, 1.0f);
        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        //float4 nb0 = fxGetParticleData(id, dataType);
        A2F nb0;
        nb0.F = fxGetParticleData(id, dataType);
        
        //int4 closest = fxGetClosest(to_int2_cfloat(world2screen(data.pos)), iResolution);
        A2I closest;
        closest.I = fxGetClosest(to_int2_cfloat(world2screen(data.pos)));
        
        int4 bestIds = to_int4_cfloat(nb0.F);
        float4 bestDists = to_float4(length2(swi2(fxGetParticleData(bestIds.x, POS_VEL),x,y) - data.pos),
                                     length2(swi2(fxGetParticleData(bestIds.y, POS_VEL),x,y) - data.pos),
                                     length2(swi2(fxGetParticleData(bestIds.z, POS_VEL),x,y) - data.pos),
                                     length2(swi2(fxGetParticleData(bestIds.w, POS_VEL),x,y) - data.pos));
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(&bestIds, &bestDists, id, (int)(nb0.A[i]), dataType, data.pos, iResolution, iChannel0);  //sort this
            sort0(&bestIds, &bestDists, id, (int)(closest.A[i]), dataType, data.pos, iResolution, iChannel0);  //sort this
        }
        
        int searchIterations = 1;
        if (iFrame < 5 || Reset)
        {
            searchIterations = 10;
        }
        
        for(int k = 0; k < searchIterations; k++)
        {
            //random hash. We should make sure that two pixels in the same frame never make the same hash!
            float h = hash(
                            iFragCoord.x + 
                            iFragCoord.y*(int)(iResolution.x) + 
                            iFrame*(int)(iResolution.x*iResolution.y) +
                            k
                          );
            //pick random id of particle
            int p = (int)(h*(float)(MAX_PARTICLES));
            float2 randXY = swi2(fxGetParticleData(p, POS_VEL),x,y);
            insertion_sort(&bestIds, &bestDists, p, length2(randXY - data.pos));
        }

        fragColor = to_float4_cint(bestIds);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    fragColor = fxSaveParticle(data, dataType);

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

__DEVICE__ float distance2Particle(int id, float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0){
    if(id==-1) return 1e20;
    float2 delta = swi2(fxGetParticleData(id, POS_VEL),x,y)-fragCoord;
    return dot(delta, delta);
}


__KERNEL__ void TeslaValveFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f;

    int2 iFragCoord = to_int2_cfloat(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    int4 old = fxGetClosest(to_int2_cfloat(fragCoord));
    int4 _new = to_int4(-1,-1,-1,-1);
    
    //in this vector the distance to these particles will be stored 
    float4 dis = to_float4_s(1e6);

    insertion_sort(&_new, &dis, old.x, distance2Particle(old.x, screen2world(fragCoord), iResolution,iChannel0));
    insertion_sort(&_new, &dis, old.y, distance2Particle(old.y, screen2world(fragCoord), iResolution,iChannel0));
    insertion_sort(&_new, &dis, old.z, distance2Particle(old.z, screen2world(fragCoord), iResolution,iChannel0));
    insertion_sort(&_new, &dis, old.w, distance2Particle(old.w, screen2world(fragCoord), iResolution,iChannel0));
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            A2I old;
            old.I   = fxGetClosest( iFragCoord + to_int2( x, y));      

            for(int j=0; j<4; j++){
                int id = old.A[j];
                float dis2 = distance2Particle(id, screen2world(fragCoord), iResolution,iChannel0);
                insertion_sort( &_new, &dis, id, dis2 );
            }
        }
    }
    
    int searchIterations = 1;
    if (iFrame < 5 || Reset)
    {
        searchIterations = 10;
    }
    for(int k = 0; k < searchIterations; k++){
        //random hash. We should make sure that two pixels in the same frame never make the same hash!
        float h = hash(
                          iFragCoord.x + 
                          iFragCoord.y*(int)(iResolution.x) + 
                          iFrame*(int)(iResolution.x*iResolution.y) +
                          k
                      );
        //pick random id of particle
        int p = (int)(h*(float)(MAX_PARTICLES));
        insertion_sort(&_new, &dis, p, distance2Particle(p, screen2world(fragCoord), iResolution,iChannel0));
    }
    
    fragColor = to_float4_cint(_new); 
  
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Preset: Keyboard' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Render particles and manage persistent state

__DEVICE__ const float PARTICLE_RENDER_SIZE = 0.04f;

__DEVICE__ void renderParticle(in fxParticle p, in float2 fragCoord, inout float4 *fragColor, float2 iResolution, __TEXTURE2D__ iChannel0)
{   
    //if (p.density < 50.0f) return;
    float2 closest;
    float dist = linePointDist2(p.pos, p.pos - p.vel, fragCoord, iResolution, &closest);

    //(*fragColor).w += _fmaxf(0.0f, PARTICLE_SDF_SIZE - _sqrtf(dist)) / PARTICLE_SDF_SIZE;
    //swi3(*fragColor,x,y,z) += 1000.2f * p.pressure * particleColor(p.uv) * _fmaxf(0.0f, PARTICLE_SIZE - _sqrtf(dist)) / PARTICLE_SIZE;
    //vec3 color = to_float3_aw(000.0f*p.pressure, 30.0f*length(p.vel), 0.012f*p.density);
    float3 color = to_float3_s(2);//p.uv.y, _sinf(p.uv.y*23.0f)*0.5f + 0.5f, _cosf(p.uv.y* 10.0f)*0.5f + 0.5f);
    swi3S(*fragColor,x,y,z, swi3(*fragColor,x,y,z) + color * _fmaxf(0.0f, PARTICLE_RENDER_SIZE - _sqrtf(dist)) / PARTICLE_RENDER_SIZE);
   
    // Render neighbor lines
    #if 0
    for(int i = 0; i < 4; i++){
        int4 neighbors = p.neighbors[i];
        for (int j = 0; j < 4; ++j)
        {
            int cid = neighbors[j];
            if(cid==-1 || cid >= MAX_PARTICLES || cid == 0) continue;

            float2 otherPos = swi2(fxGetParticleData(cid, POS_VEL),x,y);

            if (length(otherPos - p.pos) < 0.1f)
            {
                float distToLin = linePointDist2(p.pos, p.pos + 0.5f * (otherPos - p.pos), fragCoord, iResolution, &closest);
                swi3(fragColor,x,y,z) += 0.3f*color * _fmaxf(0.0f, PARTICLE_SIZE * 0.3f - _sqrtf(distToLin)) / (PARTICLE_SIZE);
            }
        }
    }
    #endif
}

__KERNEL__ void TeslaValveFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(keyDown32, 0);
    CONNECT_CHECKBOX2(keyDown37, 0);
    CONNECT_CHECKBOX3(keyDown39, 0);

    fragCoord+=0.5f;

    //if (to_int2(fragCoord) == to_int2(0))
    if ((int)(fragCoord.x) == (int)(0) && (int)(fragCoord.y) == (int)(0))
    {
        float4 state = texture(iChannel2, to_float2_s(0));

        if (iFrame == 0 || Reset || keyDown39 ||  (iResolution.x != state.y || iResolution.x != state.z)) //(iResolution != swi2(state,y,z))
        {
            state = to_float4(0.0f, iResolution.x, iResolution.y, 0.0f);
        }
        else if (keyDown37)
        {
            state = to_float4(ANIMATE_FRAMES * 0.5f, iResolution.x, iResolution.y, 0.0f);
        }
        else
        {
            state.x += 1.0f;
            if (state.x > ANIMATE_FRAMES || keyDown32)
            {
                state.x = 0.0f;
            }
        }
        fragColor = state;
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    fragColor = to_float4_s(0.0f);
    float2 p = (2.0f*fragCoord-iResolution)/iResolution;

    //get the id's of the 4 particles that (should be) closest.
    //the 4 ids are stored in .x, .y, .z, .w
    A2I ids;
    ids.I = fxGetClosest(to_int2_cfloat(fragCoord));
    
    //draw the particles
    for(int i = 0; i < 4; i++){
        //get the particles position
        int id = ids.A[i];
        fxParticle particle = fxGetParticle(id);

        renderParticle(particle, p, &fragColor, iResolution, iChannel0);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// gaussian blur from FabriceNeyret2's smart gaussian blur: https://www.shadertoy.com/view/WtKfD3

                                       // LOD MIPmap level to use for integration 
#define init  w = 0.01f; 
//              z = _ceil(_fmaxf(0.0f,_log2f(w*R.y/float(N))));   // N/w = res/2^z
#define R     iResolution


__DEVICE__ float convol2D(__TEXTURE2D__ iChannel0, float2 U, float2 R, float w, int N) {
    
    float  O = 0.0f;  
    float r = (float)(N-1)/2.0f, g, t=0.0f;                                       
    for( int k=0; k<N*N; k++ ) {                                            
        float2 P = to_float2(k%N,k/N) / r - 1.0f;                                    
        t += g = _expf(-2.0f*dot(P,P) );                                        
        O += g * texture(iChannel0, (U+w*P) *R.y/R).x;
    }                                                                       
    return O/t;                                                             
}      

__KERNEL__ void TeslaValveFuse__Buffer_D(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{

    u+=0.5f;

    int           N = 7; // 7                              // target sampling rate
    float         w = 0.1f;                                // filter width
                  
    init 
    float2 U = u / R.y;  
    O = texture(iChannel0, u / iResolution);
    O.x = convol2D(iChannel0,U,R,w,N); //return;
  //  O = convol1D(U,to_float2(1,0));

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel3
// Connect Image 'Previsualization: Buffer D' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


// ---------------------------------------------------------------------------------------
//  Created by fenix in 2022
//  License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
//  Same basic engine as "Self supporting SPH" https://www.shadertoy.com/view/7ddfRB
//  Tesla valves were invented in 1920 by Nikola Tesla, but had little practical
//  use until researchers recently used it to create a microfluidic pump.
//
//      https://en.wikipedia.org/wiki/Tesla_valve
//
//  I'm not sure why (yet) but the fluid is a lot more active at lower resolutions.
//  The valve "works" though at multiple resolutions, letting relatively more fluid
//  through from right to left than left to right. The left/right cycle is 3600 frames
//  long and is controlled by ANIMATE_FRAMES.
// 
//  Buffer A computes the particle positions and neighbors
//  Buffer B does a traditional voronoi search to help out building neighborhoods
//  Buffer C renders the particles into a density texture (used for rendering only)
//  Buffer D blurs the density (used for rendering only)
// ---------------------------------------------------------------------------------------

__DEVICE__ float computeSpecular(
    in float specularCoefficient,
    in float specularExponent,
    in float3 nvNormal,
    in float3 nvFragToLight,
    in float3 nvFragToCam)
{
    float3 blinnH = normalize(nvFragToLight + nvFragToCam);
    float valSpecular = _powf(_fmaxf(0.0f, dot(nvNormal, blinnH)), specularExponent);
    valSpecular *= specularCoefficient;

    return valSpecular;
}

__DEVICE__ float2 texGrad(__TEXTURE2D__ sampler, float2 fragCoord, float2 iResolution)
{
  
    float dx = texture(sampler, (fragCoord + to_float2(1.0f, 0.0f))/iResolution).x
             - texture(sampler, (fragCoord - to_float2(1.0f, 0.0f))/iResolution).x;
    float dy = texture(sampler, (fragCoord + to_float2(0.0f, 1.0f))/iResolution).x
             - texture(sampler, (fragCoord - to_float2(0.0f, 1.0f))/iResolution).x;
    return to_float2(dx, dy);
}

__KERNEL__ void TeslaValveFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{


    
    fragColor = to_float4_s(0.0f);
    float2 p = (2.0f*fragCoord-iResolution)/iResolution;
    int2 iFragCoord = to_int2_cfloat(fragCoord);

    float4 state = texture(iChannel3, to_float2_s(0));
    float frames = state.x;

    float wallSd = distanceFromWalls(p, iResolution, frames);
    
    float4 background = texture(iChannel2, fragCoord/iResolution);
    
    float lDensity = texture(iChannel0, fragCoord/iResolution).x;
    float2 lNorm2 = texGrad(iChannel0, fragCoord, iResolution);
    float3 lNorm = to_float3(-lNorm2.x, 1.0f, -lNorm2.y);
    lNorm = normalize(lNorm);

    float4 water = texture(iChannel2, fragCoord/iResolution + lNorm2 * 0.2f) * to_float4(1.0f, 0.0f, 0.0f, 0.0f);
    float waterSpec = computeSpecular(0.8f, 20.0f, lNorm, normalize(to_float3(1.0f, -1.0f, 1.0f)), to_float3(0.0f, 1.7f+p.x * 0.3f, 0.5f - p.y * 0.3f));
    swi3S(water,x,y,z, swi3(water,x,y,z) + to_float3_s(waterSpec) * 2.0f);

    float4 scene = _mix(background, water, smoothstep(0.5f, 1.0f, lDensity));

    // Handle animated steel UVs for moving (particle compressing) walls
    float sideShift = 0.0f;
    if (fragCoord.x > iResolution.x * 0.875f)
    {
        if (frames > ANIMATE_FRAMES * 0.475f)
        {
            frames = ANIMATE_FRAMES * 0.475f;
        }
        sideShift = frames * 0.25f;
        if (fragCoord.y < iResolution.y * 0.6f)
        {
            sideShift = -sideShift;
        }
    }
    else if (fragCoord.x < iResolution.x * 0.125f)
    {
        if (frames > ANIMATE_FRAMES * 0.975f)
        {
            frames = ANIMATE_FRAMES * 0.975f;
        }
        sideShift = frames * 0.25f;
        if (fragCoord.y < iResolution.y * 0.5f)
        {
            sideShift = -sideShift;
        }
    }

    float2 steelCoord = to_float2(fragCoord.x * 2.0f, (fragCoord.y + sideShift) * 0.004f);
    float3 steelNorm = swi3(texture(iChannel1, steelCoord/iResolution),x,y,z);
    swi2S(steelNorm,x,z, swi2(steelNorm,x,z) + getNormalFromWalls(p, iResolution, frames) * (1.0f - smoothstep(0.02f, 0.05f, -wallSd)));

    steelNorm = normalize(steelNorm);
    float steelSpec = computeSpecular(0.8f, 15.0f, steelNorm, normalize(to_float3(1.0f, -1.0f, 1.0f)), to_float3(0.0f, 1.75f-p.x, 0.5f - p.y));
    float4 steel = to_float4_aw(to_float3_s(steelSpec), 1.0f) * 0.5f + 0.3f;

    fragColor = _mix(scene, steel, smoothstep(0.02f, 0.03f, -wallSd));

    fragColor.w = 1.0f;

  SetFragmentShaderComputedColor(fragColor);
}