

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
//  Buffer C renders the particles
//  Buffer D applies temporal blur
//
// ---------------------------------------------------------------------------------------


void mainImage( out vec4 fragColor, vec2 fragCoord ){
    
    fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//amount of particles
const int MAX_PARTICLES = 50000; 
const int PARTICLE_INIT_X = 10;
const float PARTICLE_SIZE = 0.003;
const float PARTICLE_REPEL_SIZE = 0.005;
const float MOVING_WALL_MAG = 0.0;
const float MOVING_WALL_TIME = 3.0;

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

float cross2(vec2 a, vec2 b)
{
    return a.x * b.y - a.y * b.x;
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


const int NUM_WING_POINTS = 25;
vec2 WING_POINTS[NUM_WING_POINTS] = vec2[](
    vec2(-0.9,  0.8),
    vec2(-0.65,  0.5),
    vec2(-0.85,  0.45),
    vec2(-0.55,  0.3),
    vec2(-0.75,  0.125),
    vec2(-0.45,  0.1),
    vec2(-0.63,  -0.2),
    vec2(-0.35, -0.1),
    vec2(-0.51,  -0.45),
    vec2(-0.25, -0.3),
    vec2(-0.2, -0.3),
    vec2(-0.1, -0.4),
    vec2(-0.075, -0.6),
    vec2(-0.175, -0.75),
    vec2(-0.05, -0.7),
    vec2(-0.15, -0.85),
    vec2(-0.045, -0.8),
    vec2(-0.05, -0.9),
    vec2(-0.0, -0.85),
    vec2(-0.0, 0.125),
    vec2(-0.045, 0.0),
    vec2(-0.125, -0.1),
    vec2(-0.2, -0.0),
    vec2(-0.3,  0.2),
    vec2(-0.6,  0.6)
);

float sdPolygon( in vec2[NUM_WING_POINTS] v, in vec2 p )
{
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for( int i=0, j=NUM_WING_POINTS-1; i<NUM_WING_POINTS; j=i, i++ )
    {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if( all(c) || all(not(c)) ) s*=-1.0;  
    }
    return s*sqrt(d);
}

const int NUM_HEAD_POINTS = 9;
vec2 HEAD_POINTS[NUM_HEAD_POINTS] = vec2[](
    vec2(-0.0, -0.2),
    vec2(0.0, 0.3),
    vec2(-0.1, 0.4),
    vec2(-0.2, 0.425),
    vec2(-0.145, 0.5),
    vec2(-0.05, 0.7),
    vec2(0.15, 0.75),
    vec2(0.25, 0.55),
    vec2(0.2, 0.35)
);

float sdPolygon( in vec2[NUM_HEAD_POINTS] v, in vec2 p )
{
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for( int i=0, j=NUM_HEAD_POINTS-1; i<NUM_HEAD_POINTS; j=i, i++ )
    {
        vec2 e = v[j] - v[i];
        vec2 w =    p - v[i];
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 c = bvec3(p.y>=v[i].y,p.y<v[j].y,e.x*w.y>e.y*w.x);
        if( all(c) || all(not(c)) ) s*=-1.0;  
    }
    return s*sqrt(d);
}


float sdPhoenix(in vec2 p)
{
#if 1
    float minDist = sdPolygon(WING_POINTS, p);
    minDist = min(minDist, sdPolygon(WING_POINTS, vec2(-p.x, p.y)));
    minDist = min(minDist, sdPolygon(HEAD_POINTS, p));
    return minDist;
#else
    float minDist = sdBox(p, vec2(0.05, 0.1));
    minDist = min(minDist, sdTriangle(p, vec2(-0.9, 0.9), vec2(-0.0, -0.0), vec2(-0.5, 0.9)));
    minDist = min(minDist, sdTriangle(p, vec2( 0.9, 0.9), vec2(-0.0, -0.0), vec2( 0.5, 0.9)));
    return minDist;
#endif
}

vec2 normPhoenix(in vec2 p)
{
    const float DELTA = 0.001;
    return vec2(sdPhoenix(p + vec2(DELTA, 0.0)) - sdPhoenix(p - vec2(DELTA, 0.0)),
                sdPhoenix(p + vec2(0.0, DELTA)) - sdPhoenix(p - vec2(0.0, DELTA)));
}

float distanceFromWalls(vec2 point, vec3 resolution, float time)
{
    float minDist = 1e30;
    minDist = min(minDist, point.x + resolution.x / resolution.y);
    minDist = min(minDist, resolution.x / resolution.y - point.x);
    minDist = min(minDist, point.y + 1.0);
    minDist = min(minDist, 1.0 - point.y);
    return minDist;
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

//returns the ids of the four closest particles from the input
ivec4 fxGetClosestInternal(sampler2D sampler, ivec2 xy)
{
    return ivec4(texelFetch(sampler, xy, 0));
}

#define fxGetClosest(X) fxGetClosestInternal(iChannel1, X)

#define UL_NEIGHBORS 0
#define UR_NEIGHBORS 1
#define LL_NEIGHBORS 2
#define LR_NEIGHBORS 3
#define POS_VEL 4
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
    vec2 uv;
};

//get the particle corresponding to the input id
fxParticle fxGetParticleInternal(sampler2D sampler, int resolutionWidth, int id)
{
    vec4 particleData0 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UL_NEIGHBORS), 0);
    vec4 particleData1 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, UR_NEIGHBORS), 0);
    vec4 particleData2 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LL_NEIGHBORS), 0);
    vec4 particleData3 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, LR_NEIGHBORS), 0);
    vec4 particleData4 = texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);

    fxParticle particle;
    particle.neighbors[0] = ivec4(particleData0);
    particle.neighbors[1] = ivec4(particleData1);
    particle.neighbors[2] = ivec4(particleData2);
    particle.neighbors[3] = ivec4(particleData3);
    particle.pos = particleData4.xy;
    particle.vel = particleData4.zw;
    
    return particle;
}


vec4 fxSaveParticle(fxParticle p, int dataType)
{    
    switch(dataType)
    {
    case UL_NEIGHBORS:
        return vec4(p.neighbors[0]);
    case UR_NEIGHBORS:
        return vec4(p.neighbors[1]);
    case LL_NEIGHBORS:
        return vec4(p.neighbors[2]);
    case LR_NEIGHBORS:
        return vec4(p.neighbors[3]);
    case POS_VEL:  
        return vec4(p.pos, p.vel);

    }
}

#define fxGetParticle(X) fxGetParticleInternal(iChannel0, int(iResolution.x), X)

vec4 fxGetParticleDataInternal(sampler2D sampler, int resolutionWidth, int id, int dataType)
{
    return texelFetch(sampler, fxLocFromIDInternal(resolutionWidth, id, POS_VEL), 0);
}

#define fxGetParticleData(X, Y) fxGetParticleDataInternal(iChannel0, int(iResolution.x), X, Y)

const float PI = 3.141592653589793;

float SPHKernel (float x)
{
    if (x < 1.0)
        return 4.0 * cos(x*PI) + cos((x + x) * PI) + 3.0;
    else
        return 0.0;
}

float SPHgradKernel (float x)
{
    if (x < 4.0)
    {
        float xx = x*x;
        float xxx = xx*x;
        float xxxx = xxx*x;
        return 0.000 + 3.333 * x + -3.167 * xx + 0.917 * xxx + -0.083 * xxxx;
    }
    else
        return 0.0;
}

float length2(vec2 v)
{
    return dot(v, v);
}

float square(float x)
{
    return x * x;
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

const vec2 GRAVITY = vec2(0.0000, -0.0000);
const float DAMPING = 1.0;
const float PARTICLE_REPEL = 0.0000005;
const float WALL_REPEL = 0.0;
const float IDEAL_DENSITY = 106.0;

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int id, int searchId, int dataType, in vec2 myPos);

void mainImage( out vec4 fragColor, vec2 fragCoord ){
    ivec2 iFragCoord = ivec2(fragCoord);
    
    //we only simulate PARTICLES amount of particles
    int maxParticles = min(int(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
    int index = iFragCoord.x + iFragCoord.y*int(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=maxParticles) return;
    
    fxParticle data = fxGetParticle(id);
    vec4 state = texelFetch(iChannel2, ivec2(0), 0);
    
    if (dataType == POS_VEL)
    {
        if (iFrame == 0 || keyDown(32) || state.z > 0.0)
        {
            //pick a "random" starting position
            float particlesPerRow = sqrt(float(maxParticles));
            float i = float(id % int(particlesPerRow));
            float j = float(id / int(particlesPerRow)) + float(id & 1) * 0.5;
            
            data.pos = (vec2(i / particlesPerRow, j / particlesPerRow) * 1.8 - 0.9) * vec2(iResolution.x / iResolution.y, 1.0);
            data.vel = vec2(0);
            data.uv = vec2(data.pos.x * 0.5 + 0.5, data.pos.y * 0.5 + 0.5);
        }
        else
        {
            vec2 force = vec2(0);
            
            // Debug forces
            vec2 disturbPos = vec2(0.0, 0.0);
            vec2 disturbDelta = vec2(0.0, 0.0);
            if (iMouse.z > 0.0 && iMouse.w < 0.0)
            {
                disturbPos = (vec2(4.0, 2.0) * (iMouse.xy - 0.5 * iResolution.xy) / iResolution.y);
                disturbDelta = (iMouse.xy - vec2(iMouse.z, -iMouse.w));
                disturbDelta = clamp(disturbDelta, -100.0, 100.0);
            }
            else
            {
                // auto disturb
                //disturbPos = vec2(sin(iTime * 0.5), sin(iTime * 1.0))* vec2(1.2, 0.2) + vec2(0.0, 0.2);
                //disturbDelta = 80.0 * vec2(cos(iTime * 0.5), cos(iTime * 1.0));
            }
            
            const float MOUSE_FIELD_SIZE = 0.3;
            float MOUSE_FIELD_STRENGTH = 0.1 / sqrt(iFrameRate);
            float dist = distance(data.pos * iResolution.xy / iResolution.y, disturbPos);
            if (dist < MOUSE_FIELD_SIZE)
            {
                force += (MOUSE_FIELD_SIZE - dist) * MOUSE_FIELD_STRENGTH * disturbDelta / iResolution.xy;
            }

            struct solverParticle
            {
                vec2 pos;
                vec2 vel;
            };
            
            solverParticle particles[17];
            int numSolverParticles = 0;
            float totalDensity = SPHKernel(0.0);
            vec2 densityGrad = vec2(0);
            
            // Compute neighborhood density and density gradient, and init solver particles
            for(int i = 0; i < 4; i++){
                ivec4 neighbors = data.neighbors[i];
                for (int j = 0; j < 4; ++j)
                {
                    int cid = neighbors[j];
                    if(cid==id || cid==-1 || cid == 0 || cid >= maxParticles) continue;
                    
                    vec4 otherPosVel = fxGetParticleData(cid, POS_VEL);
                    
                    
                    vec2 deltaPos = otherPosVel.xy - data.pos;
                    float dist = length(deltaPos) + 0.0001;
                    float nbDensity = SPHKernel(dist);
                    totalDensity += nbDensity;
                    densityGrad += nbDensity * deltaPos / dist;

                    particles[numSolverParticles].pos = otherPosVel.xy;
                    particles[numSolverParticles].vel = otherPosVel.zw;
                    
                    ++numSolverParticles;
                    
                    // Apply crazy auto-stirring force
                    data.vel += 0.004 * vec2(deltaPos.y, -deltaPos.x);
                }
            }       

            particles[numSolverParticles].pos = data.pos;
            particles[numSolverParticles].vel = data.vel;
            ++numSolverParticles;

            // Solve local neighborhood
            float pressure = 0.0;
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
                            vec2 deltaPos = particles[i].pos - particles[j].pos;
                            float dist = length(deltaPos) + 0.001;
                            vec2 dir = deltaPos / dist; 

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
                    particles[i].vel -= particles[i].vel * 0.0004 - GRAVITY / float(NUM_ITERATIONS);
                    
                    // Integrate pos
                    particles[i].pos += particles[i].vel / float(NUM_ITERATIONS);
                }
            }
            
            // Combine solver results into force
            force += particles[numSolverParticles - 1].vel - data.vel;
            
            vec2 pSpace = data.pos * vec2(iResolution.y / iResolution.x, 1.0) * 1.2;
            if (sdPhoenix(pSpace) >= 0.0)
            {
                force -= normPhoenix(pSpace) * 0.015;
            }
                   
            // Apply force
            data.vel = data.vel + force;
            
            // Boundary
            float distToScene = distanceFromWalls(data.pos, iResolution, iTime);
            float distToSceneOld = distanceFromWalls(data.pos, iResolution, iTime - iTimeDelta);
            vec2 distNormal = getNormalFromWalls(data.pos, iResolution, iTime);

            if (distToScene < PARTICLE_REPEL_SIZE)
            {
                data.pos -= 1.0 * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                data.vel -= 1.0 * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                float velToClip = dot(data.vel, distNormal);
                if (velToClip > 0.0)
                {
                    data.vel -= distNormal * (distToSceneOld - distToScene);                    
                }
            }
            
            // Damping
            data.vel -= data.vel * length2(data.vel) * 200.0;


            // Clamping
            float maxSpeed = 0.006; // Dictated by voronoi update speed
            float velLength2 = length2(data.vel);
            if (velLength2 > maxSpeed * maxSpeed)
            {
                data.vel *= inversesqrt(velLength2) * maxSpeed;
            }

            // Integrate position
            data.pos = data.pos + data.vel;
            data.pos = clamp(data.pos, vec2(-iResolution.x / iResolution.y, -1.0), vec2(iResolution.x / iResolution.y, 1.0));
        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        vec4 nb0 = fxGetParticleData(id, dataType);
        ivec4 closest = fxGetClosest(ivec2(world2screen(data.pos * vec2(iResolution.y / iResolution.x, 1.0))));
        ivec4 bestIds = ivec4(-1);
        vec4 bestDists = vec4(1e6);
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, int(nb0[i]), dataType, data.pos);  //sort this
            sort0(bestIds, bestDists, id, int(closest[i]), dataType, data.pos);  //sort this
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

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int currentId, int searchId, int dataType, in vec2 myPos)
{
    if(iscoincidence(bestIds, currentId, searchId)) return; //particle already sorted
    
    vec2 nbX = fxGetParticleData(searchId, POS_VEL).xy; 

    vec2 dx = nbX - myPos;
    int dir = int(2.*(atan(dx.y, dx.x)+PI)/PI); 

    if(dir != dataType) return; //not in this sector
    
    float t = length2(dx);
    
    if (t > PARTICLE_REPEL_SIZE * 20.0) return;
   
    insertion_sort(bestIds, bestDists, searchId, t);
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Gijs's Basic : Voronoi Tracking: https://www.shadertoy.com/view/WltSz7

// Voronoi Buffer
// every pixel stores the 4 closest particles to it
// every frame this data is shared between neighbours

float distance2Particle(int id, vec2 fragCoord){
    if(id==-1) return 1e20;
    vec2 delta = (fxGetParticleData(id, POS_VEL).xy * vec2(iResolution.y / iResolution.x, 1.0))-fragCoord;
    return dot(delta, delta);
}

void mainImage( out vec4 fragColor, vec2 fragCoord ){
   	ivec2 iFragCoord = ivec2(fragCoord);

    //in this vector the four new closest particles' ids will be stored
    ivec4 new = ivec4(-1);
    //in this vector the distance to these particles will be stored 
    vec4 dis = vec4(1e6);
    
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            ivec4 old   = fxGetClosest( iFragCoord + ivec2( x, y) );      

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
        //int maxParticles = min(iFragCoord.x * iFragCoord.y / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
        //pick random id of particle
        int p = int(h*float(MAX_PARTICLES));
        insertion_sort(new, dis, p, distance2Particle(p, screen2world(fragCoord)));
    }
    
    fragColor = vec4(new); 
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Particle render

const float MAX_TEMP = 0.002;

vec3 BlackBody(float _t)
{
    vec3 temp = vec3(min(1.0, _t / MAX_TEMP), min(1.0, _t / (2.0 * MAX_TEMP)), min(1.0, _t / (3.0 * MAX_TEMP)));
    return temp * temp * temp * temp;
}

void renderParticle(in fxParticle p, in vec2 fragCoord, inout vec4 fragColor)
{   
    vec2 closest;
    p.pos.x *= iResolution.y / iResolution.x;
    float dist = linePointDist2(p.pos, p.pos - 1.5 * p.vel, fragCoord, iResolution, closest);
    int maxParticles = min(int(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);

    vec3 color = BlackBody(length(p.vel));
    fragColor.xyz += color * max(0.0, PARTICLE_SIZE - sqrt(dist)) / PARTICLE_SIZE;
    
        // Render neighbor lines
    #if 0
    for(int i = 0; i < 4; i++){
        ivec4 neighbors = p.neighbors[i];
        for (int j = 0; j < 4; ++j)
        {
            int cid = neighbors[j];
            if(cid==-1 || cid >= maxParticles || cid == 0) continue;

            vec2 otherPos = fxGetParticleData(cid, POS_VEL).xy;
            otherPos.x *= iResolution.y / iResolution.x;

            if (length(otherPos - p.pos) < 0.1)
            {
                float distToLin = linePointDist2(p.pos, p.pos + 0.5 * (otherPos - p.pos), fragCoord, iResolution, closest);
                fragColor.xyz += 10.*color * max(0.0, PARTICLE_SIZE * 0.3 - sqrt(distToLin)) / (PARTICLE_SIZE);
            }
        }
    }
    #endif
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

        renderParticle(particle, p, fragColor);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Temporal blur

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    if (ivec2(fragCoord) == ivec2(0))
    {
        if (iFrame == 0)
        {
            fragColor = vec4(iResolution.xy, 0.0, 0.0);
            return;
        }
        
        vec4 state = texelFetch(iChannel1, ivec2(0), 0);
        if (iResolution.xy != state.xy)
        {
            state = vec4(iResolution.xy, 1.0, 0.0);
        }
        else
        {
            state.z = 0.0;
        }
        
        fragColor = state;
        return;
    }
    
    if (iFrame == 0) fragColor = vec4(0);
    
    vec4 newColor = texelFetch(iChannel0, ivec2(fragCoord), 0);
    vec4 oldColor = texelFetch(iChannel1, ivec2(fragCoord), 0);
    
    fragColor = oldColor * 0.95 + newColor;
}