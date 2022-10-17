

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
//  Multipass particle physics simulation, attempting to approximate smoothed particle
//  hydrodynamics. 
// 
//  Buffer A computes the particle positions and neighbors
//  Buffer B does a traditional voronoi search to help out building neighborhoods
//  Buffer C renders the particles into a height map
//  Buffer D blurs the height map
//
// ---------------------------------------------------------------------------------------

mat2 rotate2d(float theta) {
  float s = sin(theta), c = cos(theta);
  return mat2(c, -s, s, c);
}

mat3 camera(vec3 cameraPos, vec3 lookAtPoint) {
	vec3 cd = normalize(lookAtPoint - cameraPos);
	vec3 cr = normalize(cross(vec3(0, 1, 0), cd));
	vec3 cu = normalize(cross(cd, cr));
	
	return mat3(-cr, cu, -cd);
}

vec2 getGradFromHeightMap( vec2 point )
{
	vec2 tinyChangeX = vec2( 0.002, 0.0 );
    vec2 tinyChangeY = vec2( 0.0 , 0.002 );
    
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
    
    
	vec2 normal = vec2(
         			tinyChangeInX,
        			tinyChangeInY
    	 		  );
    
	return normal * 0.5;
}

const float MAX_TEMP = 0.135;

vec3 BlackBody(float _t)
{
    vec3 temp = vec3(min(1.0, _t / MAX_TEMP), min(1.0, _t / (2.0 * MAX_TEMP)), min(1.0, _t / (3.0 * MAX_TEMP)));
    return temp * temp * temp * temp;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    vec2 cameraUV = vec2(0.5 + iTime * 0.01, 0.5);

    vec3 lp = vec3(0);
    vec3 ro = vec3(0, 0, 3);
    ro.yz *= rotate2d(mix(-PI/2., PI/2., cameraUV.y));
    ro.xz *= rotate2d(mix(-PI, PI, cameraUV.x));

    vec3 rd = camera(ro, lp) * normalize(vec3(uv, -1));
  
    vec3 col = texture(iChannel2, rd).rgb;

    fragColor = vec4(col, 1.0);
  
    vec2 grad = getGradFromHeightMap( fragCoord / iResolution.xy ) * rotate2d(cameraUV.y);
    vec3 mercuryNormal = -normalize(vec3(grad, -1.0));
    vec4 foreground = texture(iChannel2, mercuryNormal);
    vec2 particle = texture(iChannel0, fragCoord/iResolution.xy).xy;
    if (particle.x > 0.0)
    {
        fragColor = foreground;
    }
    
    if (keyDown(32))
    {
        fragColor.xyz += max(vec3(0.0), BlackBody(MAX_TEMP * particle.y));
    }
    //fragColor.xyz = -vec3(distanceFromWalls(fragCoord/iResolution.xy * 2.0 - 1.0, iResolution, iTime));
    //fragColor.yz = getNormalFromWalls(fragCoord/iResolution.xy * 2.0 - 1.0, iResolution, iTime) * 0.5 + 0.5;
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//amount of particles
const int PARTICLES = 1000; 
const int PARTICLE_INIT_X = 10;
const float PARTICLE_SIZE = 0.015;
const float PARTICLE_REPEL_SIZE = 0.02;

//hashing noise by IQ
float hash( int k ) {
    uint n = uint(k);
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0;
}

float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 world2screenInternal(in vec2 p, in vec2 resolution)
{
    return (p + 1.0) * 0.5 * resolution;
}

#define world2screen(X) world2screenInternal(X, iResolution.xy)

vec2 screen2worldInternal(in vec2 p, in vec2 resolution)
{
    return (p / resolution) * 2.0 - 1.0;
}

#define screen2world(X) screen2worldInternal(X, iResolution.xy)

const vec4 SCENE_LINES[5] = vec4[]( vec4(-1.0, -1.0, 1.0, -1.0),
    vec4(1.0, 1.0, -1.0, 1.0),
    vec4(-1.0, 1.0, -1.0, -1.0),
    vec4(1.0, -1.0, 1.0, 1.0),
    vec4(-0.4, 0.7, -0.4, -0.5));
    
const int NUM_SCENE_LINES = 4;
const int NUM_SCENE_WALLS = 4;

float cross2(vec2 a, vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

bool intersectPlane(vec2 p0, vec2 p1, vec2 p2, vec2 p3, in float minT, out float t, out vec2 n)
{ 
    vec2 CmP = p2 - p0;
    vec2 r = p1 - p0;
    vec2 s = p3 - p2;

    float CmPxr = cross2(CmP, r);
    float CmPxs = cross2(CmP, s);
    float rxs = cross2(r, s);

    if (CmPxr == 0.0)
    {
        // Lines are collinear, and so intersect if they have any overlap
        return false;
        //return ((C.X - A.X < 0f) != (C.X - B.X < 0f))
          //  || ((C.Y - A.Y < 0f) != (C.Y - B.Y < 0f));
    }

    if (rxs == 0.0)
        return false; // Lines are parallel.

    float rxsr = 1.0 / rxs;
    t = CmPxs * rxsr;
    float u = CmPxr * rxsr;

    if (t >= 0.0 && t <= minT && u >= 0.0 && u <= 1.0)
    {
        n = normalize(vec2(-s.y, s.x));
        
        if (rxs < 0.0) n = -n;
        
        return true;
    }
    
    return false;
} 

bool intersectScene(vec2 from, vec2 to, out float t, out vec2 n)
{
    float intersectT;
    vec2 intersectNormal;

    float minT = 1.0;
    bool hit = false;
    for (int index = 0; index < NUM_SCENE_LINES; ++index)
    {
        vec2 sceneFrom = SCENE_LINES[index].xy;
        vec2 sceneTo = SCENE_LINES[index].zw;
        
        if(intersectPlane(from, to, sceneFrom, sceneTo, minT, intersectT, intersectNormal))
        {
            t = minT = intersectT;
            n = intersectNormal;
            hit = true;
        }
    }

    return hit;
}

float linePointDist2(in vec2 newPos, in vec2 oldPos, in vec2 fragCoord, in vec3 resolution, out vec2 closest)
{
    vec2 pDelta = (fragCoord - oldPos);
    vec2 delta = newPos - oldPos;
    float deltaLen2 = dot(delta, delta);

    // Find the closest point on the line segment from old to new
    closest;
    if (deltaLen2 > 0.0000001)
    {
        float deltaInvSqrt = inversesqrt(deltaLen2);
        vec2 deltaNorm = delta * deltaInvSqrt;
        closest = oldPos + deltaNorm * max(0.0, min(1.0 / deltaInvSqrt, dot(deltaNorm, pDelta)));
    }
    else
    {
        closest = oldPos;
    }

    // Distance to closest point on line segment
    vec2 closestDelta = closest - fragCoord;
    closestDelta *= resolution.xy / resolution.y;
    return dot(closestDelta, closestDelta);
}

float halfSpaceDist(vec2 sceneFrom, vec2 sceneTo, vec2 point, vec3 resolution)
{
    vec2 dir = normalize(sceneTo - sceneFrom);
    vec2 normal = vec2(-dir.y, dir.x) * resolution.xy / resolution.y;
    return dot(point - sceneTo, normal);
}

const int NUM_BEACH_POINTS = 0;

const vec3 BEACH_POINTS[5] = vec3[](
vec3(1.2, -1.2, 0.9),
vec3(0.7, -0.9, 0.5),
vec3(0.3, -0.9, 0.5),
vec3(0.5, -1.5, 1.1),
vec3(-0.6, -2.0, 1.1));

const float PI = 3.141592653589793;

float distanceFromWalls(vec2 point, vec3 resolution, float time)
{
    float theta = (max(mod(time, 30.0) - 20.0, 0.0) * 0.1 + 0.5) * PI;
    point = point / 0.8;
    point = vec2(sin(theta) * point.x + cos(theta) * point.y, cos(theta) * point.x - sin(theta) * point.y);
    float dist0 = 0.45 - distance(point, vec2(0.0, 0.45));
    float dist1 = 0.45 - distance(point, vec2(0.0, -0.45));
    return max(dist0, dist1);
}

vec2 getNormalFromWalls( vec2 point, vec3 resolution, float time )
{
	vec2 tinyChangeX = vec2( 0.001, 0.0 );
    vec2 tinyChangeY = vec2( 0.0 , 0.001 );
    
   	float upTinyChangeInX   = distanceFromWalls( point + tinyChangeX, resolution, time ); 
    float downTinyChangeInX = distanceFromWalls( point - tinyChangeX, resolution, time ); 
    
    float tinyChangeInX = upTinyChangeInX - downTinyChangeInX;
    
    
    float upTinyChangeInY   = distanceFromWalls( point + tinyChangeY, resolution, time ); 
    float downTinyChangeInY = distanceFromWalls( point - tinyChangeY, resolution, time ); 
    
    float tinyChangeInY = upTinyChangeInY - downTinyChangeInY;
    
    
	vec2 normal = vec2(
         			tinyChangeInX,
        			tinyChangeInY
    	 		  );
    
	return normalize(normal);
}

float distanceFromScene(vec2 point, vec3 resolution, float time, out vec2 n)
{
    float minDist = distanceFromWalls(point, resolution, time);
    n = getNormalFromWalls(point, resolution, time);
    for (int index = NUM_SCENE_WALLS; index < NUM_SCENE_LINES; ++index)
    {
        vec2 sceneFrom = SCENE_LINES[index].xy;
        vec2 sceneTo = SCENE_LINES[index].zw;
        
        vec2 closest;
        float dist = linePointDist2(sceneFrom, sceneTo, point, resolution, closest);
        if (dist < minDist)
        {
            minDist = sqrt(dist);
            n += normalize(point - closest);
        }
    }
    
    for (int index = 0; index < NUM_BEACH_POINTS; ++index)
    {
        float beachDist = distance(point, BEACH_POINTS[index].xy);
        minDist = min((beachDist - BEACH_POINTS[index].z) * 32.2, minDist);
    }

    n = normalize(n);
    return minDist;
}

//returns the ids of the four closest particles from the input
ivec4 fxGetClosestInternal(sampler2D sampler, ivec2 xy)
{
    return ivec4(texelFetch(sampler, xy, 0));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X)

#define POS_VEL 0
#define UL_NEIGHBORS 1
#define UR_NEIGHBORS 2
#define LL_NEIGHBORS 3
#define LR_NEIGHBORS 4
#define NUM_PARTICLE_DATA_TYPES 5

//returns the location of the particle within the particle buffer corresponding with the input id 
ivec2 fxLocFromIDInternal(int width, int id, int dataType)
{
    int index = id * NUM_PARTICLE_DATA_TYPES + dataType;
    return ivec2( index % width, index / width);
}

#define fxLocFromID(X, Y) fxLocFromIDInternal(int(iResolution.x), X, Y)

struct fxParticle
{
    vec2 pos;
    vec2 vel;
    
    ivec4 neighbors[4];
};

//get the particle corresponding to the input id
fxParticle fxGetParticleInternal(sampler2D sampler, int resolutionWidth, int id)
{
    vec4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
    vec4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS), 0);
    vec4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS), 0);
    vec4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS), 0);
    vec4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS), 0);

    fxParticle particle;
    particle.pos = particleData0.xy;
    particle.vel = particleData0.zw;
    particle.neighbors[0] = ivec4(particleData1);
    particle.neighbors[1] = ivec4(particleData2);
    particle.neighbors[2] = ivec4(particleData3);
    particle.neighbors[3] = ivec4(particleData4);
    
    return particle;
}


vec4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
    case POS_VEL:  
        return vec4(p.pos, p.vel);
    case UL_NEIGHBORS:
    case UR_NEIGHBORS:
    case LL_NEIGHBORS:
    case LR_NEIGHBORS:
        return vec4(p.neighbors[dataType - 1]);
    }
}
#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

vec4 fxGetParticleDataInternal(sampler2D sampler, int resolutionWidth, int id, int dataType)
{
    return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y)


float SPHkernel (float x)
{
    return cos(x) + cos(x + x) - 1.0 / (x * x + 2.0);
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
// Particle Buffer
// in this buffer every pixel represents a particle
// the particles positions is stored in .xy
//           its velocity  is stored in .zw
// Only the first PARTICLES amount of pixels are actually used.

const vec2 GRAVITY = vec2(0.0000, -0.00004);
const float MAX_SPEED = 0.008;
const float DAMPING = 1.0;
const float PARTICLE_REPEL = 0.0001;
const float WALL_REPEL = 0.0;

vec4 saveParticle(fxParticle particle, int dataType);
void sort0(inout ivec4 bestIds, inout vec4 bestDists, int id, int searchId, int dataType, in vec2 myPos);

void mainImage( out vec4 fragColor, vec2 fragCoord ){
    ivec2 iFragCoord = ivec2(fragCoord);
    
    //we only simulate PARTICLES amount of particles
    int index = iFragCoord.x + iFragCoord.y*int(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=PARTICLES) return;
    
    fxParticle oldData = fxGetParticle(id);
    fxParticle newData = oldData;
    
    if (dataType == POS_VEL)
    {
        if(iFrame==0 || oldData.pos == vec2(0.0, 0.0)){
            //pick a "random" starting position
            float h1 = hash(id);
            float h2 = hash(int(h1*41343.));
            newData.pos = vec2(h1,h2) * vec2(0.5, 1.0) - vec2(0.25, -0.0);
            newData.vel = vec2(0);
        }
        else
        {
            vec3 densityVel = texture(iChannel2, oldData.pos * 0.5 + 0.5).xyz;
            float damping = 0.8 - smoothstep(1.5, 9.0, densityVel.x) * 0.5;
            
            vec2 force = GRAVITY;

            vec2 disturbPos = vec2(0.0, 0.0);
            vec2 disturbDelta = vec2(0.0, 0.0);
            if (iMouse.z > 0.0 && iMouse.w < 0.0)
            {
                disturbPos = ((2.0 * iMouse.xy / iResolution.xy) - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
                disturbDelta = (iMouse.xy - vec2(iMouse.z, -iMouse.w));
                disturbDelta = clamp(disturbDelta, -100.0, 100.0);
            }
            else
            {
                // auto disturb
                //disturbPos = vec2(sin(iTime * 0.5), sin(iTime * 1.0))* vec2(1.2, 0.2) + vec2(0.0, -0.2);
                //disturbDelta = 75.0 * vec2(cos(iTime * 0.5), cos(iTime * 1.0));
            }
            const float MOUSE_FIELD_SIZE = 0.3;
            const float MOUSE_FIELD_STRENGTH = 0.01;
            float dist = distance(newData.pos * iResolution.xy / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution.xy;
            }

            float minDist = 1e6;
            for(int i = 0; i < 4; i++){
                ivec4 neighbors = oldData.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid > PARTICLES) continue;

                    fxParticle otherData = fxGetParticle(cid);
                    vec2 otherPos = otherData.pos + otherData.vel * 0.5;
                    vec2 deltaPos = otherPos - newData.pos;
                    vec2 deltaVel = otherData.vel - newData.vel;
                    float approach = dot(deltaPos, deltaVel);
                    float dist = length(deltaPos);
                    minDist = min(minDist, dist);
                }
            }
            
            for(int i = 0; i < 4; i++){
                ivec4 neighbors = oldData.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid > PARTICLES) continue;

                    fxParticle otherData = fxGetParticle(cid);
                    vec2 otherPos = otherData.pos + otherData.vel * 0.5;
                    vec2 deltaPos = otherPos - newData.pos;
                    vec2 deltaVel = otherData.vel - newData.vel;
                    float approach = dot(deltaPos, deltaVel);
                    float dist = length(deltaPos);
                    
                    if (dist < (PARTICLE_REPEL_SIZE + PARTICLE_REPEL_SIZE))
                    {
                        force -= ( PARTICLE_REPEL * SPHkernel(dist / PARTICLE_REPEL_SIZE)) * normalize(deltaPos);
                    }
                    
                    if (approach < PARTICLE_REPEL_SIZE)
                    {
                        force += deltaVel * 0.15 * PARTICLE_SIZE / dist;
                    }
                    
                    if (minDist < PARTICLE_REPEL_SIZE * 4.0)
                    {
                        force -= 0.001 / dist * (PARTICLE_REPEL_SIZE - minDist)* normalize(deltaPos);
                    }
                }
            }

            vec2 distNormal;
            float distToScene = distanceFromScene(newData.pos, iResolution, iTime, distNormal);

            if (distToScene < PARTICLE_SIZE * 2.0)
            {
                newData.pos -= 0.1*distNormal * (distToScene);

                float dp = dot(newData.vel, distNormal);
                if (dp < 0.0)
                {
                    force -= 0.1*distNormal * dp;
                }
            }
            
            newData.vel = oldData.vel + force;
            
            newData.vel -= newData.vel*0.02;//(0.5*tanh(1.*(length(newData.vel)-1.5))+0.5);

            float velLength2 = dot(newData.vel, newData.vel);
            if (velLength2 > MAX_SPEED * MAX_SPEED)
            {
                newData.vel *= inversesqrt(velLength2) * MAX_SPEED;
            }
            if (distToScene < PARTICLE_SIZE * 2.0 && dot(newData.vel, distNormal) > 0.0)
            {
                newData.vel -= 0.2*reflect(newData.vel, distNormal);
            }
            newData.pos = newData.pos + newData.vel;
            newData.pos = clamp(newData.pos, -1.0, 1.0);
            
            
            if (newData.pos.x < -0.95)
            {
            //    newData.pos = vec2(0.8, 0.4) + mod(newData.pos, vec2(0.2));
            }

        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        vec4 nb0 = fxGetParticleData(id, dataType);
        ivec4 closest = fxGetClosest(ivec2(world2screen(oldData.pos)));
        ivec4 bestIds = ivec4(-1);
        vec4 bestDists = vec4(1e6);
        
        //random sorts
        for (int i = 0; i < 8; ++i)
        {
            int searchId = int(float(iResolution.x*iResolution.y)*hash13(vec3(iFrame, id, i)));
            sort0(bestIds, bestDists, id, searchId, dataType, newData.pos);
        }
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, int(nb0[i]), dataType, newData.pos);  //sort this
            sort0(bestIds, bestDists, id, int(closest[i]), dataType, newData.pos);  //sort this
            
            //use a sudorandom direction of the neighbor
            vec4 nb1 = fxGetParticleData(int(nb0[i]), 1 + (iFrame+id)%4);
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
}

bool iscoincidence(in ivec4 bestIds, int currentId, int id)
{
    return (id < 0) || 
      		(id == currentId) ||
           any(equal(bestIds,ivec4(id)));
}

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int currentId, int searchId, int dataType, in vec2 myPos)
{
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    vec2 nbX = fxGetParticleData(searchId, dataType).xy; 

    vec2 dx = nbX - myPos;
    int dir = int(2.*(atan(dx.y, dx.x)+PI)/PI); 
    
    if(dir != (dataType - 1)) return; //not in this sector
    
    float t = length(dx);
   
    insertion_sort(bestIds, bestDists, searchId, t);
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Gijs's Basic : Voronoi Tracking: https://www.shadertoy.com/view/WltSz7

// Voronoi Buffer
// every pixel stores the 4 closest particles to it
// every frame this data is shared between neighbours

float distance2Particle(int id, vec2 fragCoord){
    if(id==-1) return 1e20;
    vec2 delta = fxGetParticleData(id, POS_VEL).xy-fragCoord;
    return dot(delta, delta);
}

void mainImage( out vec4 fragColor, vec2 fragCoord ){
   	ivec2 iFragCoord = ivec2(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    ivec4 new = ivec4(-1);
    //in this vector the distance to these particles will be stored 
    vec4 dis = vec4(1e20);
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            ivec4 old   = fxGetClosest( iFragCoord + ivec2( x, y) );      

            for(int j=0; j<4; j++){
                int id = old[j];
                float dis2 = distance2Particle(id, screen2world(fragCoord));
                insertion_sort( new, dis, id, dis2 );
                
                #if 0
                for (int k=0; k < 4; ++k)
                {
                    ivec4 neighbors = ivec4(fxGetParticleData(id, k + 1));
                    float dis2 = distance2Particle(neighbors[k], screen2world(fragCoord));
                    insertion_sort( new, dis2, neighbors[k], dis2 );
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
            iFragCoord.y*int(iResolution.x) + 
            iFrame*int(iResolution.x*iResolution.y) +
            k
        );
        //pick random id of particle
        int p = int(h*float(PARTICLES));
        insertion_sort(new, dis, p, distance2Particle(p, screen2world(fragCoord)));
    }
    
    fragColor = vec4(new); 
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Render particle SDF

const float PARTICLE_SDF_SIZE = 0.04;

void renderParticle(in vec2 newPos, in vec2 oldPos, float density, in vec2 fragCoord, inout vec4 fragColor)
{    
    vec2 closest;
    float dist = linePointDist2(newPos, oldPos, fragCoord, iResolution, closest);
    
    fragColor.x += max(0.0, PARTICLE_SDF_SIZE - sqrt(dist)) / PARTICLE_SDF_SIZE;
    fragColor.y += max(0.0, PARTICLE_SIZE - sqrt(dist)) / PARTICLE_SIZE;
}

void mainImage( out vec4 fragColor, vec2 fragCoord ){
    
    fragColor = vec4(0.0);
  	vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.xy;

    //get the id's of the 4 particles that (should be) closest.
    //the 4 ids are stored in .x, .y, .z, .w
    ivec4 ids = fxGetClosest(ivec2(fragCoord));
    
    //draw the particles
    for(int i = 0; i < 4; i++){
        //get the particles position
        int id = ids[i];
        fxParticle particle = fxGetParticle(id);
        vec2 pos = particle.pos;
        vec2 vel = particle.vel;

        renderParticle(pos, pos - vel, 1.0, p, fragColor);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// gaussian blur from FabriceNeyret2's smart gaussian blur: https://www.shadertoy.com/view/WtKfD3

int           N = 7; // 7                              // target sampling rate
float         w = .1,                                   // filter width
              z;                                        // LOD MIPmap level to use for integration 
#define init  w = .02; \
              z = ceil(max(0.,log2(w*R.y/float(N))));   // N/w = res/2^z
#define R     iResolution.xy


float convol2D(vec2 U) {                                                     
    float  O = 0.0;  
    float r = float(N-1)/2., g, t=0.;                                       
    for( int k=0; k<N*N; k++ ) {                                            
        vec2 P = vec2(k%N,k/N) / r - 1.;                                    
        t += g = exp(-2.*dot(P,P) );                                        
        O += g * textureLod(iChannel0, (U+w*P) *R.y/R, z ).x;                 
    }                                                                       
    return O/t;                                                             
}      

void mainImage( out vec4 O, vec2 u )
{
    init 
    vec2 U = u / R.y;  
    O = texture(iChannel0, u / iResolution.xy);
    O.x = convol2D(U); return;
  //  O = convol1D(U,vec2(1,0));
}

