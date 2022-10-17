
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//amount of particles
__DEVICE__ const int MAX_PARTICLES = 5000; 
__DEVICE__ const int PARTICLE_INIT_X = 10;
__DEVICE__ const float PARTICLE_SIZE = 0.003f;
__DEVICE__ const float PARTICLE_REPEL_SIZE = 0.01f;
__DEVICE__ const float MOVING_WALL_MAG = 0.0f;
__DEVICE__ const float MOVING_WALL_TIME = 3.0f;

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
    //closest; ?????????
    if (deltaLen2 > 0.0000001f)
    {
        float deltaInvSqrt = 1.0f/_sqrtf(deltaLen2);
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

__DEVICE__ float distanceFromWalls(float2 point, float2 resolution, float time)
{
    float minDist = 1e30;
    minDist = _fminf(minDist, (point.x + 1.0f - MOVING_WALL_MAG - MOVING_WALL_MAG*_sinf(time / MOVING_WALL_TIME)) * resolution.x / resolution.y);
    minDist = _fminf(minDist, (1.0f - point.x) * resolution.x / resolution.y);
    minDist = _fminf(minDist, point.y + 1.0f);
    minDist = _fminf(minDist, 1.0f - point.y);
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
    
    
    float2 normal = to_float2(
                              tinyChangeInX,
                              tinyChangeInY
                             );
    
  return normalize(normal);
}

//returns the ids of the four closest particles from the input
__DEVICE__ int4 fxGetClosestInternal(__TEXTURE2D__  sampler, int2 xy, float2 iResolution)
{
    //return to_int4_cfloat(texelFetch(sampler, xy, 0));
    return to_int4_cfloat(texture(sampler, (make_float2(xy)+0.5f)/iResolution));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X, iResolution)

#define UL_NEIGHBORS 0
#define UR_NEIGHBORS 1
#define LL_NEIGHBORS 2
#define LR_NEIGHBORS 3
#define POS_VEL      4
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
    float2 uv;
};

//get the particle corresponding to the input id
__DEVICE__ fxParticle fxGetParticleInternal(__TEXTURE2D__  sampler, int resolutionWidth, int id, float2 iResolution)
{
  #ifdef ORG 
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
float zzzzzzzzzzzzzzzzzz;
    }
  return to_float4_s(0.0f); // Nothing
    
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, (int)(iResolution.x), X, iResolution)

__DEVICE__ float4 fxGetParticleDataInternal(__TEXTURE2D__  sampler, int resolutionWidth, int id, int dataType, float2 iResolution)
{
    //return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
    return texture(sampler, (make_float2(fxLocFromIDInternal(resolutionWidth, id, POS_VEL))+0.5f)/iResolution);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y, iResolution)

#define PI  3.141592653589793f

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

__DEVICE__ float square(float x)
{
    return x * x;
}

//#define keyClick(ascii)   ( texelFetch(iChannel3,to_int2(ascii,1),0).x > 0.0f)
//#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)
#ifdef ORG
__DEVICE__ void insertion_sort(inout int4 i, inout float4 d, int i_, float d_){  
    if(any(equal(ito_float4_aw(i_),i))) return;
    if     (d_ < d[0])             
        i = to_int4(i_,swi3(i,x,y,z)),    d = to_float4(d_,swi3(d,x,y,z));
    else if(d_ < d[1])             
        i = to_int4(i.x,i_,swi2(i,y,z)),  d = to_float4(d.x,d_,swi2(d,y,z));
    else if(d_ < d[2])            
        i = to_int4(swi2(i,x,y),i_,i.z),  d = to_float4(swi2(d,x,y),d_,d.z);
    else if(d_ < d[3])           
        i = to_int4(swi3(i,x,y,z),i_),    d = to_float4(swi3(d,x,y,z),d_);
}
#endif
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
// Connect Buffer A 'Texture: Start' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel2


// Particle Buffer
// in this buffer every pixel represents a particle
// the particles positions is stored in .xy
//           its velocity  is stored in .zw
// Only the first PARTICLES amount of pixels are actually used.



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
    
    if (t > PARTICLE_REPEL_SIZE * 20.0f) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);

}

#ifdef XXXX
//Particle
__DEVICE__ particle Blending( __TEXTURE2D__ channel, float2 uv, particle P, float Blend, float2 Par, float2 MulOff, int Modus, float2 fragCoord, float2 R, float MAX_SPEED)
{
 
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2) //Startbedingung
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = _mix(P.X, fragCoord, Blend);
          P.V = _mix(P.V, MAX_SPEED * to_float2(_cosf(q), _sinf(q)), Blend);
          P.M = _mix(P.M, 0.45f - _fabs(fragCoord.x/iResolution.x - 0.5f), Blend);
        }      
        if ((int)Modus&4) // Geschwindigkeit
        {
          P.V = Par.x*_mix(P.V, to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend);
        }
        
        if ((int)Modus&8) // Masse
          P.M = _mix(P.M, (tex.x+MulOff.y)*MulOff.x, Blend);

      }
      else
      {
        if ((int)Modus&16) 
          P.M = _mix(P.M, (MulOff.y+0.45) - _fabs(fragCoord.x/iResolution.x - 0.5f), Blend);
      
        if ((int)Modus&32) //Special
        {
          float q = 2.0f*PI * hash12(1.0f + fragCoord);
          P.X = fragCoord;
          P.V = MAX_SPEED * to_float2(_cosf(q), _sinf(q));
          P.M = 0.45f - _fabs(fragCoord.x/iResolution.x - 0.5f);
        }  
      }
    }
  
  return P;
}
#endif



__KERNEL__ void SpinningSparkCloudsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(keyDown, 0);
    CONNECT_CHECKBOX2(StartTex, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    CONNECT_SLIDER0(TimeDelta, -1.0f, 1.0f, 0.0f);
    
    
    CONNECT_SLIDER5(Start1, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER6(Start2, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER7(Start3, -2.0f, 2.0f, 1.0f);

    fragCoord+=0.5f;

    const float2 GRAVITY = to_float2(0.0000f, -0.0000f);
    const float DAMPING = 1.0f;
    const float PARTICLE_REPEL = 0.0001f;
    const float WALL_REPEL = 0.0f;
    const float IDEAL_DENSITY = 106.0f;

    int2 iFragCoord = to_int2_cfloat(fragCoord);
    
    //we only simulate PARTICLES amount of particles
    int maxParticles = _fminf((int)(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
    int index = iFragCoord.x + iFragCoord.y*(int)(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
   
 
    if(id>=maxParticles) 
    { 
      SetFragmentShaderComputedColor(fragColor);
      return;
    }         
    
    fxParticle data = fxGetParticle(id);
    
    
    //if (Blend1>0.0) P = Blending(iChannel1, fragCoord/R, P, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R, MAX_SPEED);  
    
    float4 tex = texture(iChannel3, fragCoord/iResolution);     //Funktioniert nicht !!
    if (dataType == POS_VEL)
    {
        if (iFrame == 0 || keyDown)
        {
          float ffffffffffff;
          if(StartTex)
          {
            tex = texture(iChannel3, fragCoord/iResolution);
            if (tex.w>0.0f)
            {
              
              int index = iFragCoord.x + iFragCoord.y*(int)(iResolution.x);
              int id = index / NUM_PARTICLE_DATA_TYPES;
              
              //pick a "random" starting position
              float particlesPerRow = _sqrtf((float)(maxParticles))*Start1;
              float i = (float)(id % (int)(particlesPerRow)) * Start2;
              float j = (float)(id / (int)(particlesPerRow)) + (float)(id & 1) * 0.5f * Start3;
              data.vel = to_float2_s(0);
              data.uv = to_float2(data.pos.x * 0.5f + 0.5f, data.pos.y * 0.5f + 0.5f);
            }
          }
          else
          {
            //pick a "random" starting position
            float particlesPerRow = _sqrtf((float)(maxParticles))*Start1;
            float i = (float)(id % (int)(particlesPerRow)) * Start2;
            float j = (float)(id / (int)(particlesPerRow)) + (float)(id & 1) * 0.5f * Start3;
            
            data.pos = to_float2(i / particlesPerRow, j / particlesPerRow) * 1.8f - 0.9f;
            data.vel = to_float2_s(0);
            data.uv = to_float2(data.pos.x * 0.5f + 0.5f, data.pos.y * 0.5f + 0.5f);
          }
        }
        else
        {
            float2 force = to_float2_s(0);
            
            // Debug forces
            float2 disturbPos = to_float2(0.0f, 0.0f);
            float2 disturbDelta = to_float2(0.0f, 0.0f);
            if (iMouse.z > 0.0f)// && iMouse.w < 0.0f)
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
            float MOUSE_FIELD_STRENGTH = 0.5f; //0.5f / _sqrtf(iFrameRate); // not defined
            float dist = distance_f2(data.pos * iResolution / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution;
            }

            struct solverParticle
            {
                float2 pos;
                float2 vel;
            };
            
            solverParticle particles[17];
            int numSolverParticles = 0;
            float totalDensity = SPHKernel(0.0f);
            float2 densityGrad = to_float2_s(0);
            
            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 4; i++){
                //int4 neighbors = data.neighbors[i];
                A2I neighbors;
                neighbors.I = data.neighbors[i];
                
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors.A[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= maxParticles) continue;
                    
                    float4 otherPosVel = fxGetParticleData(cid, POS_VEL);
                    
                    
                    float2 deltaPos = swi2(otherPosVel,x,y) - data.pos;
                    float dist = length(deltaPos) + 0.0001f;
                    float nbDensity = SPHKernel(dist);
                    totalDensity += nbDensity;
                    densityGrad += nbDensity * deltaPos / dist;

                    particles[numSolverParticles].pos = swi2(otherPosVel,x,y);
                    particles[numSolverParticles].vel = swi2(otherPosVel,z,w);
                    
                    ++numSolverParticles;
                    
                    // Apply crazy auto-stirring force
                    data.vel += 0.02f * to_float2(deltaPos.y, -deltaPos.x);
                }
            }       

            particles[numSolverParticles].pos = data.pos;
            particles[numSolverParticles].vel = data.vel;
            ++numSolverParticles;

            // Solve local neighborhood
            float pressure = 0.0f;
            const int NUM_ITERATIONS = 25;
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
                    particles[i].vel -= particles[i].vel * 0.0004f - GRAVITY / (float)(NUM_ITERATIONS);
                    
                    // Integrate pos
                    particles[i].pos += particles[i].vel / float(NUM_ITERATIONS);
                }
            }
            
            // Combine solver results into force
            force += particles[numSolverParticles - 1].vel - data.vel;
            
float AAAAAAAAAAAAAAAA;
                   
            // Apply force
            data.vel = data.vel + force;
            
            // Boundary
            float distToScene = distanceFromWalls(data.pos, iResolution, iTime);
            float distToSceneOld = distanceFromWalls(data.pos, iResolution, iTime - iTimeDelta + TimeDelta);
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
            
            // Damping
            data.vel -= data.vel * length2(data.vel) * 200.0f;


            // Clamping
            float maxSpeed = 0.006f; // Dictated by voronoi update speed
            float velLength2 = length2(data.vel);
            if (velLength2 > maxSpeed * maxSpeed)
            {
                //data.vel *= inversesqrt(velLength2) * maxSpeed;
                data.vel *= 1.0f/_sqrtf(velLength2) * maxSpeed;
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
        
        //int4 closest = fxGetClosest(to_int2(world2screen(data.pos)));
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

__KERNEL__ void SpinningSparkCloudsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBBBB;
    int2 iFragCoord = to_int2_cfloat(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    int4 _new = fxGetClosest(to_int2_cfloat(fragCoord));
    //in this vector the distance to these particles will be stored 
    float4 dis = to_float4(distance_f2(swi2(fxGetParticleData(_new.x, POS_VEL),x,y), screen2world(fragCoord)),
                           distance_f2(swi2(fxGetParticleData(_new.y, POS_VEL),x,y), screen2world(fragCoord)),
                           distance_f2(swi2(fxGetParticleData(_new.z, POS_VEL),x,y), screen2world(fragCoord)),
                           distance_f2(swi2(fxGetParticleData(_new.w, POS_VEL),x,y), screen2world(fragCoord)));
  
    for(int _x=-2; _x<=2; _x++){
        for(int _y=-2; _y<=2; _y++){
            //int4 old   = fxGetClosest( iFragCoord + to_int2( x, y) );      
            A2I old; 
            old.I = fxGetClosest( iFragCoord + to_int2( _x, _y) );
            
            for(int j=0; j<4; j++){
                int id = old.A[j];
                float dis2 = distance2Particle(id, screen2world(fragCoord), iResolution, iChannel0);
                insertion_sort( &_new, &dis, id, dis2 );
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
                        iFragCoord.y*(int)(iResolution.x) + 
                        iFrame*(int)(iResolution.x*iResolution.y) +
                        k
                      );
        int maxParticles = _fminf(iFragCoord.x * iFragCoord.y / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
        //pick random id of particle
        int p = (int)(h*(float)(maxParticles));
        insertion_sort(&_new, &dis, p, distance2Particle(p, screen2world(fragCoord), iResolution, iChannel0));
    }
    
    fragColor = to_float4_cint(_new); 

    if (Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// Particle render



__DEVICE__ float3 BlackBody(float _t)
{
  const float MAX_TEMP = 0.002f;
  
    float3 temp = to_float3(_fminf(1.0f, _t / MAX_TEMP), _fminf(1.0f, _t / (2.0f * MAX_TEMP)), _fminf(1.0f, _t / (3.0f * MAX_TEMP)));
    return temp * temp * temp * temp;
}

__DEVICE__ void renderParticle(in fxParticle p, in float2 fragCoord, inout float4 *fragColor, float2 iResolution, __TEXTURE2D__ iChannel0)
{   
    float2 closest;
    float dist = linePointDist2(p.pos, p.pos - 1.5f * p.vel, fragCoord, iResolution, &closest);
    int maxParticles = _fminf((int)(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
float cccccccccccccc;
    float3 color = BlackBody(length(p.vel));
    swi3S(*fragColor,x,y,z, swi3(*fragColor,x,y,z) + color * _fmaxf(0.0f, PARTICLE_SIZE - _sqrtf(dist)) / PARTICLE_SIZE);
}

__KERNEL__ void SpinningSparkCloudsFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;
    
    fragColor = to_float4_s(0.0f);
    float2 p = (2.0f*fragCoord-iResolution)/iResolution;

    //get the id's of the 4 particles that (should be) closest.
    //the 4 ids are stored in .x, .y, .z, .w
    //int4 ids = fxGetClosest(to_int2_cfloat(fragCoord));
    A2I ids;
    ids.I = fxGetClosest(to_int2_cfloat(fragCoord));
    
    //draw the particles
    for(int i = 0; i < 4; i++){
        //get the particles position
        int id = ids.A[i];
        fxParticle particle = fxGetParticle(id);

        renderParticle(particle, p, &fragColor, iResolution, iChannel0);
    }

  if (Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Temporal blur

__KERNEL__ void SpinningSparkCloudsFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
 
   
    CONNECT_CHECKBOX0(Reset, 0);
    fragCoord+=0.5f; 

    
    
    //float4 newColor = texelFetch(iChannel0, to_int2(fragCoord), 0);
    float4 newColor = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
    
    //float4 oldColor = texelFetch(iChannel1, to_int2(fragCoord), 0);
    float4 oldColor = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
    
    fragColor = oldColor * 0.95f + newColor;

    if (iFrame == 0 || Reset) fragColor = to_float4_s(0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
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
//  Buffer C renders the particles
//  Buffer D applies temporal blur
//
// ---------------------------------------------------------------------------------------


__KERNEL__ void SpinningSparkCloudsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    
    //fragColor = texelFetch(iChannel0, to_int2(fragCoord), 0);
    fragColor = texture(iChannel0, fragCoord/iResolution);

  SetFragmentShaderComputedColor(fragColor);
}