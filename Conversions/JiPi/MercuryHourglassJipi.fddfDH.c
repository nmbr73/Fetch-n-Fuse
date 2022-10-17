
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//amount of particles
const int PARTICLES = 1000; 
const int PARTICLE_INIT_X = 10;
const float PARTICLE_SIZE = 0.015f;
const float PARTICLE_REPEL_SIZE = 0.02f;

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

const float4 SCENE_LINES[5] = vec4[]( to_float4(-1.0f, -1.0f, 1.0f, -1.0f),
    to_float4(1.0f, 1.0f, -1.0f, 1.0f),
    to_float4(-1.0f, 1.0f, -1.0f, -1.0f),
    to_float4(1.0f, -1.0f, 1.0f, 1.0f),
    to_float4(-0.4f, 0.7f, -0.4f, -0.5f));
    
const int NUM_SCENE_LINES = 4;
const int NUM_SCENE_WALLS = 4;

__DEVICE__ float cross2(float2 a, float2 b)
{
    return a.x * b.y - a.y * b.x;
}

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
        *n = normalize(to_float2(-s.y, s.x));
        
        if (rxs < 0.0f) *n = - *n;
        
        return true;
    }
    
    return false;
} 

__DEVICE__ bool intersectScene(float2 from, float2 to, out float *t, out float2 *n)
{
    float intersectT;
    float2 intersectNormal;

    float minT = 1.0f;
    bool hit = false;
    for (int index = 0; index < NUM_SCENE_LINES; ++index)
    {
        float2 sceneFrom = swi2(SCENE_LINES[index],x,y);
        float2 sceneTo = swi2(SCENE_LINES[index],z,w);
        
        if(intersectPlane(from, to, sceneFrom, sceneTo, minT, &intersectT, &intersectNormal))
        {
            *t = minT = intersectT;
            *n = intersectNormal;
            hit = true;
        }
    }

    return hit;
}

__DEVICE__ float linePointDist2(in float2 newPos, in float2 oldPos, in float2 fragCoord, in float3 resolution, out float2 *closest)
{
    float2 pDelta = (fragCoord - oldPos);
    float2 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    *closest; //??????????????????????????????
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
    return dot(closestDelta, closestDelta);
}

__DEVICE__ float halfSpaceDist(float2 sceneFrom, float2 sceneTo, float2 point, float3 resolution)
{
    float2 dir = normalize(sceneTo - sceneFrom);
    float2 normal = to_float2(-dir.y, dir.x) * swi2(resolution,x,y) / resolution.y;
    return dot(point - sceneTo, normal);
}



#define PI  3.141592653589793f

__DEVICE__ float distanceFromWalls(float2 point, float3 resolution, float time)
{
    float theta = (_fmaxf(mod_f(time, 30.0f) - 20.0f, 0.0f) * 0.1f + 0.5f) * PI;
    point = point / 0.8f;
    point = to_float2(_sinf(theta) * point.x + _cosf(theta) * point.y, _cosf(theta) * point.x - _sinf(theta) * point.y);
    float dist0 = 0.45f - distance_f2(point, to_float2(0.0f, 0.45f));
    float dist1 = 0.45f - distance_f2(point, to_float2(0.0f, -0.45f));
    return _fmaxf(dist0, dist1);
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

__DEVICE__ float distanceFromScene(float2 point, float3 resolution, float time, out float2 *n)
{
	const int NUM_BEACH_POINTS = 0;
	
	const float3 BEACH_POINTS[5] = {
									to_float3(1.2f, -1.2f, 0.9f),
									to_float3(0.7f, -0.9f, 0.5f),
									to_float3(0.3f, -0.9f, 0.5f),
									to_float3(0.5f, -1.5f, 1.1f),
	                                to_float3(-0.6f, -2.0f, 1.1f)};
	
    float minDist = distanceFromWalls(point, resolution, time);
    n = getNormalFromWalls(point, resolution, time);
    for (int index = NUM_SCENE_WALLS; index < NUM_SCENE_LINES; ++index)
    {
        float2 sceneFrom = SCENE_LINES[index].xy;
        float2 sceneTo = SCENE_LINES[index].zw;
        
        float2 closest;
        float dist = linePointDist2(sceneFrom, sceneTo, point, resolution, closest);
        if (dist < minDist)
        {
            minDist = _sqrtf(dist);
            *n += normalize(point - closest);
        }
    }
    
    for (int index = 0; index < NUM_BEACH_POINTS; ++index)
    {
        float beachDist = distance_f2(point, swi2(BEACH_POINTS[index],x,y));
        minDist = _fminf((beachDist - BEACH_POINTS[index].z) * 32.2f, minDist);
    }

    *n = normalize(*n);
    return minDist;
}

//returns the ids of the four closest particles from the input
__DEVICE__ int4 fxGetClosestInternal(__TEXTURE2D__ sampler, int2 xy, float2 iResolution)
{
    //return to_int4(texelFetch(sampler, xy, 0));
	return to_int4(texture(sampler, (to_float2(xy.x,xy.y)+0.5f)/iResolution));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X, iResolution)

#define POS_VEL 0
#define UL_NEIGHBORS 1
#define UR_NEIGHBORS 2
#define LL_NEIGHBORS 3
#define LR_NEIGHBORS 4
#define NUM_PARTICLE_DATA_TYPES 5

//returns the location of the particle within the particle buffer corresponding with the input id 
__DEVICE__ int2 fxLocFromIDInternal(int width, int id, int dataType)
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
};

//get the particle corresponding to the input id
__DEVICE__ struct fxParticle fxGetParticleInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id, float2 iResolution)
{
    float4 particleData0 = texture(sampler, (to_float2(fxLocFromIDInternal(resolutionWidth, id, POS_VEL))+0.5f)/iResolution);
    float4 particleData1 = texture(sampler, (to_float2(fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData2 = texture(sampler, (to_float2(fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData3 = texture(sampler, (to_float2(fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS))+0.5f)/iResolution);
    float4 particleData4 = texture(sampler, (to_float2(fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS))+0.5f)/iResolution);

    struct fxParticle particle;
    particle.pos = swi2(particleData0,x,y);
    particle.vel = swi2(particleData0,z,w);
    particle.neighbors[0] = to_int4(particleData1);
    particle.neighbors[1] = to_int4(particleData2);
    particle.neighbors[2] = to_int4(particleData3);
    particle.neighbors[3] = to_int4(particleData4);
    
    return particle;
}


__DEVICE__ float4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
		case POS_VEL:  
			return to_float4_f2f2(p.pos, p.vel);
		case UL_NEIGHBORS:
		case UR_NEIGHBORS:
		case LL_NEIGHBORS:
		case LR_NEIGHBORS:
			return to_float4(p.neighbors[dataType - 1]);
    }
}
#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

__DEVICE__ float4 fxGetParticleDataInternal(__TEXTURE2D__ sampler, int resolutionWidth, int id, int dataType, float2 iResolution)
{
    return texture(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL))+0.5f)/iResolution);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, (int)(iResolution.x), X, Y)


__DEVICE__ float SPHkernel (float x)
{
    return _cosf(x) + _cosf(x + x) - 1.0f / (x * x + 2.0f);
}

#define keyClick(ascii)   ( texture(iChannel3,to_int2(ascii,1),0).x > 0.0f)
#define keyDown(ascii)    ( texture(iChannel3,to_int2(ascii,0),0).x > 0.0f)

__DEVICE__ void insertion_sort(inout int4 *i, inout float4 *d, int i_, float d_){  
    //if(any(equal(ito_float4_aw(i_),i))) return;
	if((*x).x  == *i && (*x).y  == *i && (*x).z  == *i && (*x).w  == *i ) return;
    if     (d_ < d[0])             
        *i = to_int4(i_,swi3(*i,x,y,z)),    *d = to_float4(d_,swi3(*d,x,y,z));
    else if(d_ < d[1])             
        *i = to_int4((*i).x,i_,swi2(i,y,z)), *d = to_float4((*d).x,d_,swi2(*d,y,z));
    else if(d_ < d[2])            
        *i = to_int4(swi2(*i,x,y),i_,i.z), *d = to_float4(swi2(*d,x,y),d_,(*d).z);
    else if(d_ < d[3])           
        *i = to_int4(swi3(*i,x,y,z),i_),    *d = to_float4(swi3(*d,x,y,z),d_);
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



__DEVICE__ bool iscoincidence(in int4 bestIds, int currentId, int id)
{
    return (id < 0) || 
          (id == currentId) ||
           any(equal(bestIds,to_int4(id)));
}

__DEVICE__ void sort0(inout int4 *bestIds, inout float4 *bestDists, int currentId, int searchId, int dataType, in float2 myPos)
{
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    float2 nbX = fxGetParticleData(searchId, dataType).xy; 

    float2 dx = nbX - myPos;
    int dir = int(2.0f*(_atan2f(dx.y, dx.x)+PI)/PI); 
    
    if(dir != (dataType - 1)) return; //not in this sector
    
    float t = length(dx);
   
    insertion_sort(bestIds, bestDists, searchId, t);
 
}
__KERNEL__ void MercuryHourglassJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel2)
{

	fragCoord+=0.5f;

	const float2 GRAVITY = to_float2(0.0000f, -0.00004f);
	const float MAX_SPEED = 0.008f;
	const float DAMPING = 1.0f;
	const float PARTICLE_REPEL = 0.0001f;
	const float WALL_REPEL = 0.0f;

    int2 iFragCoord = to_int2_cfloat(fragCoord);
    
    //we only simulate PARTICLES amount of particles
    int index = iFragCoord.x + iFragCoord.y*int(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=PARTICLES) return;
    
    fxParticle oldData = fxGetParticle(id);
    fxParticle newData = oldData;
    
    if (dataType == POS_VEL)
    {
        if(iFrame==0 || oldData.pos == to_float2(0.0f, 0.0f)){
            //pick a "random" starting position
            float h1 = hash(id);
            float h2 = hash(int(h1*41343.0f));
            newData.pos = to_float2(h1,h2) * to_float2(0.5f, 1.0f) - to_float2(0.25f, -0.0f);
            newData.vel = to_float2(0);
        }
        else
        {
            float3 densityVel = texture(iChannel2, oldData.pos * 0.5f + 0.5f).xyz;
            float damping = 0.8f - smoothstep(1.5f, 9.0f, densityVel.x) * 0.5f;
            
            float2 force = GRAVITY;

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
                //disturbPos = to_float2(_sinf(iTime * 0.5f), _sinf(iTime * 1.0f))* to_float2(1.2f, 0.2f) + to_float2(0.0f, -0.2f);
                //disturbDelta = 75.0f * to_float2(_cosf(iTime * 0.5f), _cosf(iTime * 1.0f));
            }
            const float MOUSE_FIELD_SIZE = 0.3f;
            const float MOUSE_FIELD_STRENGTH = 0.01f;
            float dist = distance(newData.pos * iResolution / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution;
            }

            float minDist = 1e6;
            for(int i = 0; i < 4; i++){
                int4 neighbors = oldData.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid > PARTICLES) continue;

                    fxParticle otherData = fxGetParticle(cid);
                    float2 otherPos = otherData.pos + otherData.vel * 0.5f;
                    float2 deltaPos = otherPos - newData.pos;
                    float2 deltaVel = otherData.vel - newData.vel;
                    float approach = dot(deltaPos, deltaVel);
                    float dist = length(deltaPos);
                    minDist = _fminf(minDist, dist);
                }
            }
            
            for(int i = 0; i < 4; i++){
                int4 neighbors = oldData.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid > PARTICLES) continue;

                    fxParticle otherData = fxGetParticle(cid);
                    float2 otherPos = otherData.pos + otherData.vel * 0.5f;
                    float2 deltaPos = otherPos - newData.pos;
                    float2 deltaVel = otherData.vel - newData.vel;
                    float approach = dot(deltaPos, deltaVel);
                    float dist = length(deltaPos);
                    
                    if (dist < (PARTICLE_REPEL_SIZE + PARTICLE_REPEL_SIZE))
                    {
                        force -= ( PARTICLE_REPEL * SPHkernel(dist / PARTICLE_REPEL_SIZE)) * normalize(deltaPos);
                    }
                    
                    if (approach < PARTICLE_REPEL_SIZE)
                    {
                        force += deltaVel * 0.15f * PARTICLE_SIZE / dist;
                    }
                    
                    if (minDist < PARTICLE_REPEL_SIZE * 4.0f)
                    {
                        force -= 0.001f / dist * (PARTICLE_REPEL_SIZE - minDist)* normalize(deltaPos);
                    }
                }
            }

            float2 distNormal;
            float distToScene = distanceFromScene(newData.pos, iResolution, iTime, distNormal);

            if (distToScene < PARTICLE_SIZE * 2.0f)
            {
                newData.pos -= 0.1f*distNormal * (distToScene);

                float dp = dot(newData.vel, distNormal);
                if (dp < 0.0f)
                {
                    force -= 0.1f*distNormal * dp;
                }
            }
            
            newData.vel = oldData.vel + force;
            
            newData.vel -= newData.vel*0.02f;//(0.5f*_tanhf(1.0f*(length(newData.vel)-1.5f))+0.5f);

            float velLength2 = dot(newData.vel, newData.vel);
            if (velLength2 > MAX_SPEED * MAX_SPEED)
            {
                newData.vel *= inversesqrt(velLength2) * MAX_SPEED;
            }
            if (distToScene < PARTICLE_SIZE * 2.0f && dot(newData.vel, distNormal) > 0.0f)
            {
                newData.vel -= 0.2f*reflect(newData.vel, distNormal);
            }
            newData.pos = newData.pos + newData.vel;
            newData.pos = clamp(newData.pos, -1.0f, 1.0f);
            
            
            if (newData.pos.x < -0.95f)
            {
            //    newData.pos = to_float2(0.8f, 0.4f) + mod_f(newData.pos, to_float2_s(0.2f));
            }

        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        float4 nb0 = fxGetParticleData(id, dataType);
        int4 closest = fxGetClosest(to_int2(world2screen(oldData.pos)));
        int4 bestIds = to_int4(-1);
        float4 bestDists = to_float4(1e6);
        
        //random sorts
        for (int i = 0; i < 8; ++i)
        {
            int searchId = int(float(iResolution.x*iResolution.y)*hash13(to_float3(iFrame, id, i)));
            sort0(bestIds, bestDists, id, searchId, dataType, newData.pos);
        }
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, int(nb0[i]), dataType, newData.pos);  //sort this
            sort0(bestIds, bestDists, id, int(closest[i]), dataType, newData.pos);  //sort this
            
            //use a sudorandom direction of the neighbor
            float4 nb1 = fxGetParticleData(int(nb0[i]), 1 + (iFrame+id)%4);
            sort0(bestIds, bestDists, id, int(nb1[0]), dataType, newData.pos);  
            sort0(bestIds, bestDists, id, int(nb1[1]), dataType, newData.pos);  
        }
        
        switch(dataType)
        {
        case UL_NEIGHBORS:
            newData.neighbors[0] = bestIds;
            break;
        case UR_NEIGHBORS:
            newData.neighbors[1] = bestIds;
            break;
        case LL_NEIGHBORS:
            newData.neighbors[2] = bestIds;
            break;
        case LR_NEIGHBORS:
            newData.neighbors[3] = bestIds;
            break;
        }
    }
    
    fragColor = fxSaveParticle(newData, dataType);
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
    float2 delta = swi2(fxGetParticleData(id, POS_VEL),x,y)-fragCoord;
    return dot(delta, delta);
}

__KERNEL__ void MercuryHourglassJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
	fragCoord+=0.5f;	

    int2 iFragCoord = to_int2_cfloat(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    int4 new = to_int4(-1,-1,-1,-1);
    //in this vector the distance to these particles will be stored 
    float4 dis = to_float4_s(1e20);
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            int4 old   = fxGetClosest( iFragCoord + to_int2( x, y) );      

            for(int j=0; j<4; j++){
                int id = old[j];
                float dis2 = distance2Particle(id, screen2world(fragCoord));
                insertion_sort( &new, &dis, id, dis2 );
                
                #if 0
                for (int k=0; k < 4; ++k)
                {
                    int4 neighbors = to_int4(fxGetParticleData(id, k + 1));
                    float dis2 = distance2Particle(neighbors[k], screen2world(fragCoord));
                    insertion_sort( &new, &dis2, neighbors[k], dis2 );
                }
                #endif
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
        //pick random id of particle
        int p = (int)(h*(float)(PARTICLES));
        insertion_sort(&new, &dis, p, distance2Particle(p, screen2world(fragCoord)));
    }
    
    fragColor = to_float4(new); 
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


// Render particle SDF

const float PARTICLE_SDF_SIZE = 0.04f;

__DEVICE__ void renderParticle(in float2 newPos, in float2 oldPos, float density, in float2 fragCoord, inout float4 *fragColor)
{    
    float2 closest;
    float dist = linePointDist2(newPos, oldPos, fragCoord, iResolution, closest);
    
    (*fragColor).x += _fmaxf(0.0f, PARTICLE_SDF_SIZE - _sqrtf(dist)) / PARTICLE_SDF_SIZE;
    (*fragColor).y += _fmaxf(0.0f, PARTICLE_SIZE - _sqrtf(dist)) / PARTICLE_SIZE;
}

__KERNEL__ void MercuryHourglassJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution)
{

	fragCoord+=0.5f;
    
    fragColor = to_float4_s(0.0f);
    float2 p = (2.0f*fragCoord-iResolution)/iResolution;

    //get the id's of the 4 particles that (should be) closest.
    //the 4 ids are stored in .x, .y, .z, .w
    int4 ids = fxGetClosest(to_int2_cfloat(fragCoord));
    
    //draw the particles
    for(int i = 0; i < 4; i++){
        //get the particles position
        int id = ids[i];
        fxParticle particle = fxGetParticle(id);
        float2 pos = particle.pos;
        float2 vel = particle.vel;

        renderParticle(pos, pos - vel, 1.0f, p, &fragColor);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// gaussian blur from FabriceNeyret2's smart gaussian blur: https://www.shadertoy.com/view/WtKfD3


#define init  w = 0.02f; 
              //z = _ceil(_fmaxf(0.0f,_log2f(w*R.y/(float)(N))));   // N/w = res/2^z
#define R     iResolution


__DEVICE__ float convol2D(float2 U, int N, float w, __TEXTURE2D__ iChannel0 ) {                                                     
    float  O = 0.0f;  
    float r = (float)(N-1)/2.0f, g, t=0.0f;                                       
    for( int k=0; k<N*N; k++ ) {                                            
        float2 P = to_float2(k%N,k/N) / r - 1.0f;                                    
        t += g = _expf(-2.0f*dot(P,P) );                                        
        O += g * texture(iChannel0, (U+w*P) *R.y/R ).x;                 
    }                                                                       
    return O/t;                                                             
}      

__KERNEL__ void MercuryHourglassJipiFuse__Buffer_D(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{
	fragCoord+=0.5f;

	int           N = 7; // 7                              // target sampling rate
	float         w = 0.1f;                                // filter width
				                                         // LOD MIPmap level to use for integration 
    init 
    float2 U = u / R.y;  
    O = texture(iChannel0, u / iResolution);
    O.x = convol2D(U,N,w,iChannel0); //return;
    //  O = convol1D(U,to_float2(1,0));

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel2
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer C' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// ---------------------------------------------------------------------------------------
//  Created by fenix in 2022
//  License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
//  Multipass particle physics simulation, attempting to approximate smoothed particle
//  hydrodynamics. 
// 
//  Buffer A computes the particle positions and neighbors
//  Buffer B does a traditional voronoi search to help out building neighborhoods
//  Buffer C renders the particles into a height map
//  Buffer D blurs the height map
//
// ---------------------------------------------------------------------------------------

__DEVICE__ mat2 rotate2d(float theta) {
  float s = _sinf(theta), c = _cosf(theta);
  return to_mat2(c, -s, s, c);
}

__DEVICE__ mat3 camera(float3 cameraPos, float3 lookAtPoint) {
  float3 cd = normalize(lookAtPoint - cameraPos);
  float3 cr = normalize(cross(to_float3(0, 1, 0), cd));
  float3 cu = normalize(cross(cd, cr));
  
  return to_mat3(-cr, cu, -cd);
}

__DEVICE__ float2 getGradFromHeightMap( float2 point, __TEXTURE2D__ iChannel1 )
{
    float2 tinyChangeX = to_float2( 0.002f, 0.0f );
    float2 tinyChangeY = to_float2( 0.0f , 0.002f );
    
    float upTinyChangeInX0   = texture(iChannel1, point + tinyChangeX).x; 
    float upTinyChangeInX1   = texture(iChannel1, point + tinyChangeX + tinyChangeX).x; 
    float downTinyChangeInX0 = texture(iChannel1, point - tinyChangeX).x; 
    float downTinyChangeInX1 = texture(iChannel1, point - tinyChangeX - tinyChangeX).x; 
    
    float tinyChangeInX = upTinyChangeInX0 + upTinyChangeInX1 - downTinyChangeInX0 - downTinyChangeInX1;
    
    
    float upTinyChangeInY0   = texture(iChannel1, point + tinyChangeY).x; 
    float upTinyChangeInY1   = texture(iChannel1, point + tinyChangeY + tinyChangeY).x; 
    float downTinyChangeInY0 = texture(iChannel1, point - tinyChangeY).x; 
    float downTinyChangeInY1 = texture(iChannel1, point - tinyChangeY - tinyChangeY).x; 
    
    float tinyChangeInY = upTinyChangeInY0 + upTinyChangeInY1 - downTinyChangeInY0 - downTinyChangeInY1;
    
    
  float2 normal = to_float2(
                tinyChangeInX,
                tinyChangeInY
                           );
    
  return normal * 0.5f;
}



__DEVICE__ float3 BlackBody(float _t, float MAX_TEMP)
{
    float3 temp = to_float3_aw(_fminf(1.0f, _t / MAX_TEMP), _fminf(1.0f, _t / (2.0f * MAX_TEMP)), _fminf(1.0f, _t / (3.0f * MAX_TEMP)));
    return temp * temp * temp * temp;
}

__KERNEL__ void MercuryHourglassJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
	CONNECT_CHECKBOX0(keyDown, 0);
	fragCoord+=0.5f;	
	
	const float MAX_TEMP = 0.135f;

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 cameraUV = to_float2(0.5f + iTime * 0.01f, 0.5f);

    float3 lp = to_float3(0);
    float3 ro = to_float3(0, 0, 3);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , rotate2d(_mix(-PI/2.0f, PI/2.0f, cameraUV.y))));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rotate2d(_mix(-PI, PI, cameraUV.x))));

    float3 rd = mul_mat3_f3(camera(ro, lp) , normalize(to_float3_aw(uv, -1)));
  
    float3 col = swi3(_tex2DVecN(iChannel2,rd.x,rd.y,15),x,y,z);

    fragColor = to_float4_aw(col, 1.0f);
  
    float2 grad = mul_f2_mat2(getGradFromHeightMap( fragCoord / iResolution, iChannel1 ) , rotate2d(cameraUV.y));
    float3 mercuryNormal = -normalize(to_float3_aw(grad, -1.0f));
    float4 foreground = _tex2DVecN(iChannel2,mercuryNormal.x,mercuryNormal.y,15);
    float2 particle = swi2(texture(iChannel0, fragCoord/iResolution),x,y);
    if (particle.x > 0.0f)
    {
        fragColor = foreground;
    }
    
    if (keyDown)
    {
        swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + _fmaxf(to_float3_s(0.0f), BlackBody(MAX_TEMP * particle.y, MAX_TEMP)));
    }
    //swi3(fragColor,x,y,z) = -to_float3(distanceFromWalls(fragCoord/iResolution * 2.0f - 1.0f, iResolution, iTime));
    //swi2(fragColor,y,z) = getNormalFromWalls(fragCoord/iResolution * 2.0f - 1.0f, iResolution, iTime) * 0.5f + 0.5f;


  SetFragmentShaderComputedColor(fragColor);
}