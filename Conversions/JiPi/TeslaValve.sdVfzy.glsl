

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

float computeSpecular(
    in float specularCoefficient,
    in float specularExponent,
    in vec3 nvNormal,
    in vec3 nvFragToLight,
    in vec3 nvFragToCam)
{
    vec3 blinnH = normalize(nvFragToLight + nvFragToCam);
    float valSpecular = pow(max(0.0, dot(nvNormal, blinnH)), specularExponent);
    valSpecular *= specularCoefficient;

    return valSpecular;
}

vec2 texGrad(sampler2D sampler, vec2 fragCoord)
{
    float dx = texture(sampler, (fragCoord + vec2(1.0, 0.0))/iResolution.xy).x
             - texture(sampler, (fragCoord - vec2(1.0, 0.0))/iResolution.xy).x;
    float dy = texture(sampler, (fragCoord + vec2(0.0, 1.0))/iResolution.xy).x
             - texture(sampler, (fragCoord - vec2(0.0, 1.0))/iResolution.xy).x;
    return vec2(dx, dy);
}

void mainImage( out vec4 fragColor, vec2 fragCoord ){
    
    fragColor = vec4(0.0);
  	vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.xy;
   	ivec2 iFragCoord = ivec2(fragCoord);

    vec4 state = texture(iChannel3, vec2(0));
    float frames = state.x;

    float wallSd = distanceFromWalls(p, iResolution, frames);
    
    vec4 background = textureLod(iChannel2, fragCoord/iResolution.xy, 5.0);
    
    float lDensity = texture(iChannel0, fragCoord/iResolution.xy).x;
    vec2 lNorm2 = texGrad(iChannel0, fragCoord);
    vec3 lNorm = vec3(-lNorm2.x, 1.0, -lNorm2.y);
    lNorm = normalize(lNorm);

    vec4 water = texture(iChannel2, fragCoord/iResolution.xy + lNorm2 * 0.2) * vec4(1.0, 0.0, 0.0, 0.0);
    float waterSpec = computeSpecular(0.8, 20.0, lNorm, normalize(vec3(1.0, -1.0, 1.0)), vec3(0.0, 1.7+p.x * 0.3, 0.5 - p.y * 0.3));
    water.xyz += vec3(waterSpec) * 2.0;

    vec4 scene = mix(background, water, smoothstep(0.5, 1.0, lDensity));

    // Handle animated steel UVs for moving (particle compressing) walls
    float sideShift = 0.0;
    if (fragCoord.x > iResolution.x * 0.875)
    {
        if (frames > ANIMATE_FRAMES * 0.475)
        {
            frames = ANIMATE_FRAMES * 0.475;
        }
        sideShift = frames * 0.25;
        if (fragCoord.y < iResolution.y * 0.6)
        {
            sideShift = -sideShift;
        }
    }
    else if (fragCoord.x < iResolution.x * 0.125)
    {
        if (frames > ANIMATE_FRAMES * 0.975)
        {
            frames = ANIMATE_FRAMES * 0.975;
        }
        sideShift = frames * 0.25;
        if (fragCoord.y < iResolution.y * 0.5)
        {
            sideShift = -sideShift;
        }
    }

    vec2 steelCoord = vec2(fragCoord.x * 2.0, (fragCoord.y + sideShift) * 0.004);
    vec3 steelNorm = texture(iChannel1, steelCoord/iResolution.xy).xyz;
    steelNorm.xz += getNormalFromWalls(p, iResolution, frames) * (1.0 - smoothstep(0.02, 0.05, -wallSd));

    steelNorm = normalize(steelNorm);
    float steelSpec = computeSpecular(0.8, 15.0, steelNorm, normalize(vec3(1.0, -1.0, 1.0)), vec3(0.0, 1.75-p.x, 0.5 - p.y));
    vec4 steel = vec4(vec3(steelSpec), 1.0) * 0.5 + 0.3;

    fragColor = mix(scene, steel, smoothstep(0.02, 0.03, -wallSd));

    fragColor.w = 1.0;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//amount of particles
const int MAX_PARTICLES = 5000; 
const float PARTICLE_REPEL_SIZE = 0.01;

const float ANIMATE_FRAMES = 3600.0;

const float PI = 3.141592653598793;

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

float sdBox(in vec2 p, in vec2 boxCenter, in vec2 boxSize)
{
    p -= boxCenter;
    vec2 d = abs(p)-boxSize;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdHorseshoe( in vec2 p, in vec2 c, in float r, in vec2 w )
{
    p.x = abs(p.x);
    float l = length(p);
    p = mat2(-c.x, c.y, c.y, c.x)*p;
    p = vec2((p.y>0.0 || p.x>0.0)?p.x:l*sign(-c.x),
             (p.x>0.0)?p.y:l );
    p = vec2(p.x,abs(p.y-r))-w;
    return length(max(p,0.0)) + min(0.0,max(p.x,p.y));
}

const float h1ang = 1.8;
const mat2 h1rot = mat2(cos(h1ang), -sin(h1ang), sin(h1ang), cos(h1ang));
const float h2ang = PI - h1ang;
const mat2 h2rot = mat2(cos(h2ang), -sin(h2ang), sin(h2ang), cos(h2ang));
const float lang = 1.7;
const mat2 lrot = mat2(cos(lang), -sin(lang), sin(lang), cos(lang));

float distanceFromWalls(vec2 point, vec3 resolution, float time)
{
    time = mod(time, ANIMATE_FRAMES);
    float rightTime = min(ANIMATE_FRAMES * 0.45, time);
    float leftTime = min(ANIMATE_FRAMES * 0.45, time - ANIMATE_FRAMES * 0.5);
    if (leftTime > 0.0) rightTime = 0.0;
    const float COMPRESS_RATE = 1.0 / (0.55 * ANIMATE_FRAMES);
    float minDist = 1e30;
    point.y *= resolution.y / resolution.x;
    point *= 2.0;
    minDist = min(minDist, point.x + 1.9);
    minDist = min(minDist, 1.9 - point.x);
    minDist = min(minDist, point.y + 1.0);
    minDist = min(minDist, 1.0 - point.y);
    minDist = min(minDist, sdBox(point, vec2(0.0, 0.0), vec2(1.35, 5.00)));
    minDist = min(minDist, sdBox(point, vec2(0.8, 2.0 - rightTime * COMPRESS_RATE), vec2(2.0, 1.0)));
    minDist = min(minDist, sdBox(point, vec2(0.8, -2.0 + rightTime * COMPRESS_RATE), vec2(2.0, 1.0)));
    minDist = min(minDist, sdBox(point, vec2(-0.8, 2.0 - leftTime * COMPRESS_RATE), vec2(2.0, 1.0)));
    minDist = min(minDist, sdBox(point, vec2(-0.8, -2.0 + leftTime * COMPRESS_RATE), vec2(2.0, 1.0)));
    minDist = max(minDist, -sdHorseshoe(h1rot*(point - vec2(-0.67, 0.16)), vec2(0.13, 1.0), 0.1, vec2(0.8, 0.04)));
    minDist = max(minDist, -sdHorseshoe(h2rot*(point - vec2(-0.07, -0.14)), vec2(0.13, 1.0), 0.1, vec2(0.8, 0.04)));
    minDist = max(minDist, -sdHorseshoe(h1rot*(point - vec2(0.53, 0.16)), vec2(0.13, 1.0), 0.1, vec2(0.8, 0.04)));
    minDist = max(minDist, -sdHorseshoe(h2rot*(point - vec2(1.13, -0.14)), vec2(0.13, 1.0), 0.1, vec2(0.8, 0.04)));
    minDist = max(minDist, -sdBox(lrot*(point - vec2(1.3, 0.04)), vec2(0.0, 0.0), vec2(0.04, 0.4)));
    minDist = max(minDist, -sdBox(lrot*(point - vec2(1.4, 0.02)), vec2(0.0, 0.0), vec2(0.04, 0.3)));
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
// Only MAX_PARTICLES * NUM_PARTICLE_DATA_TYPES pixels are actually used.

const float PARTICLE_REPEL = 0.00005;
const float IDEAL_DENSITY = 20.0;

void sort0(inout ivec4 bestIds, inout vec4 bestDists, int id, int searchId, int dataType, in vec2 myPos);

void mainImage( out vec4 fragColor, vec2 fragCoord ){
    ivec2 iFragCoord = ivec2(fragCoord);
    
    int maxParticles = min(int(iResolution.x * iResolution.y) / NUM_PARTICLE_DATA_TYPES, MAX_PARTICLES);
    int index = iFragCoord.x + iFragCoord.y*int(iResolution.x);
    int id = index / NUM_PARTICLE_DATA_TYPES;
    int dataType = index - id * NUM_PARTICLE_DATA_TYPES;
    if(id>=maxParticles) return;
    
    vec4 state = texture(iChannel3, vec2(0));
    float frames = state.x;

    fxParticle data = fxGetParticle(id);
    
    if (dataType == POS_VEL)
    {
        if (iFrame == 0 || frames == 0.0)
        {
            //pick a "random" starting position
            float particlesPerRow = sqrt(float(maxParticles));
            float i = float(id % int(particlesPerRow));
            float j = float(id / int(particlesPerRow)) + float(id & 1) * 0.5;
            
            data.pos = vec2(i / particlesPerRow, j / particlesPerRow) * vec2(0.1, 1.8) + vec2(0.8, -0.9);
            data.vel = vec2(0);
        }
        else if (iFrame == 0 || frames == ANIMATE_FRAMES * 0.5)
        {
            //pick a "random" starting position
            float particlesPerRow = sqrt(float(maxParticles));
            float i = float(id % int(particlesPerRow));
            float j = float(id / int(particlesPerRow)) + float(id & 1) * 0.5;
            
            data.pos = vec2(i / particlesPerRow, j / particlesPerRow) * vec2(0.1, 1.8) + vec2(-0.9, -0.9);
            data.vel = vec2(0);
        }
        else
        {
            vec2 force = vec2(0);
            
            // Debug forces
            vec2 disturbPos = vec2(0.0, 0.0);
            vec2 disturbDelta = vec2(0.0, 0.0);
            if (iMouse.z > 0.0 && iMouse.w < 0.0)
            {
                disturbPos = ((2.0 * iMouse.xy / iResolution.xy) - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
                disturbDelta = (iMouse.xy - vec2(iMouse.z, -iMouse.w));
                disturbDelta = clamp(disturbDelta, -100.0, 100.0);
            }
            
            const float MOUSE_FIELD_SIZE = 0.3;
            float MOUSE_FIELD_STRENGTH = 0.3 / sqrt(iFrameRate);
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

                    particles[numSolverParticles].pos = otherPosVel.xy;
                    particles[numSolverParticles].vel = otherPosVel.zw;
                    ++numSolverParticles;
                }
            }       

            particles[numSolverParticles].pos = data.pos;
            particles[numSolverParticles].vel = data.vel;
            ++numSolverParticles;

            // Solve local neighborhood
            float pressure = 0.0;
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
                    // Integrate pos
                    particles[i].pos += particles[i].vel / float(NUM_ITERATIONS);
                }
            }
            
            // Combine solver results into force
            force += particles[numSolverParticles - 1].vel - data.vel;
                   
            // Apply force
            data.vel = data.vel + force;
            
            // Boundary
            float distToScene = distanceFromWalls(data.pos, iResolution, frames);
            float distToSceneOld = distanceFromWalls(data.pos, iResolution, frames - 1.0);
            vec2 distNormal = getNormalFromWalls(data.pos, iResolution, frames);

            if (distToScene < PARTICLE_REPEL_SIZE)
            {
                data.pos -= 1.0 * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
                data.vel -= 1.0 * distNormal * (distToScene - PARTICLE_REPEL_SIZE);
            }
            
            // Integrate position
            data.pos = data.pos + data.vel;
            data.pos = clamp(data.pos, -1.0, 1.0);
        }
    }
    else
    {
        // Nearest particle sort inspired by michael0884's Super SPH: https://www.shadertoy.com/view/tdXBRf
        //sort neighbors and neighbor neighbors
        vec4 nb0 = fxGetParticleData(id, dataType);
        ivec4 closest = fxGetClosest(ivec2(world2screen(data.pos)));
        ivec4 bestIds = ivec4(nb0);
        vec4 bestDists = vec4(length2(fxGetParticleData(bestIds[0], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[1], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[2], POS_VEL).xy - data.pos),
            length2(fxGetParticleData(bestIds[3], POS_VEL).xy - data.pos));
        
        for (int i = 0; i < 4; ++i)
        {
            sort0(bestIds, bestDists, id, int(nb0[i]), dataType, data.pos);  //sort this
            sort0(bestIds, bestDists, id, int(closest[i]), dataType, data.pos);  //sort this
        }
        
        int searchIterations = 1;
        if (iFrame < 5)
        {
            searchIterations = 10;
        }
        for(int k = 0; k < searchIterations; k++)
        {
            //random hash. We should make sure that two pixels in the same frame never make the same hash!
            float h = hash(
                iFragCoord.x + 
                iFragCoord.y*int(iResolution.x) + 
                iFrame*int(iResolution.x*iResolution.y) +
                k
            );
            //pick random id of particle
            int p = int(h*float(MAX_PARTICLES));
            vec2 randXY = fxGetParticleData(p, POS_VEL).xy;
            insertion_sort(bestIds, bestDists, p, length2(randXY - data.pos));
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
    
    //if (t > PARTICLE_REPEL_SIZE * 20.0) return;
   
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
    ivec4 old = fxGetClosest(ivec2(fragCoord));
    ivec4 new = ivec4(-1);
    
    //in this vector the distance to these particles will be stored 
    vec4 dis = vec4(1e6);

    insertion_sort(new, dis, old.x, distance2Particle(old.x, screen2world(fragCoord)));
    insertion_sort(new, dis, old.y, distance2Particle(old.y, screen2world(fragCoord)));
    insertion_sort(new, dis, old.z, distance2Particle(old.z, screen2world(fragCoord)));
    insertion_sort(new, dis, old.w, distance2Particle(old.w, screen2world(fragCoord)));
    
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
        //pick random id of particle
        int p = int(h*float(MAX_PARTICLES));
        insertion_sort(new, dis, p, distance2Particle(p, screen2world(fragCoord)));
    }
    
    fragColor = vec4(new); 
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Render particles and manage persistent state

const float PARTICLE_RENDER_SIZE = 0.04;

void renderParticle(in fxParticle p, in vec2 fragCoord, inout vec4 fragColor)
{   
    //if (p.density < 50.0) return;
    vec2 closest;
    float dist = linePointDist2(p.pos, p.pos - p.vel, fragCoord, iResolution, closest);

    //fragColor.w += max(0.0, PARTICLE_SDF_SIZE - sqrt(dist)) / PARTICLE_SDF_SIZE;
    //fragColor.xyz += 1000.2 * p.pressure * particleColor(p.uv) * max(0.0, PARTICLE_SIZE - sqrt(dist)) / PARTICLE_SIZE;
    //vec3 color = vec3(000.0*p.pressure, 30.0*length(p.vel), 0.012*p.density);
    vec3 color = vec3(2);//p.uv.y, sin(p.uv.y*23.0)*0.5 + 0.5, cos(p.uv.y* 10.0)*0.5 + 0.5);
    fragColor.xyz += color * max(0.0, PARTICLE_RENDER_SIZE - sqrt(dist)) / PARTICLE_RENDER_SIZE;
    
    // Render neighbor lines
    #if 0
    for(int i = 0; i < 4; i++){
        ivec4 neighbors = p.neighbors[i];
        for (int j = 0; j < 4; ++j)
        {
            int cid = neighbors[j];
            if(cid==-1 || cid >= MAX_PARTICLES || cid == 0) continue;

            vec2 otherPos = fxGetParticleData(cid, POS_VEL).xy;

            if (length(otherPos - p.pos) < 0.1)
            {
                float distToLin = linePointDist2(p.pos, p.pos + 0.5 * (otherPos - p.pos), fragCoord, iResolution, closest);
                fragColor.xyz += 0.3*color * max(0.0, PARTICLE_SIZE * 0.3 - sqrt(distToLin)) / (PARTICLE_SIZE);
            }
        }
    }
    #endif
}

void mainImage( out vec4 fragColor, vec2 fragCoord )
{
    if (ivec2(fragCoord) == ivec2(0))
    {
        vec4 state = texture(iChannel2, vec2(0));

        if (iFrame == 0 || keyDown(39) || iResolution.xy != state.yz)
        {
            state = vec4(0.0, iResolution.x, iResolution.y, 0.0);
        }
        else if (keyDown(37))
        {
            state = vec4(ANIMATE_FRAMES * 0.5, iResolution.x, iResolution.y, 0.0);
        }
        else
        {
            state.x += 1.0;
            if (state.x > ANIMATE_FRAMES || keyDown(32))
            {
                state.x = 0.0;
            }
        }
        fragColor = state;
        return;
    }
    
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
// gaussian blur from FabriceNeyret2's smart gaussian blur: https://www.shadertoy.com/view/WtKfD3

int           N = 7; // 7                              // target sampling rate
float         w = .1,                                   // filter width
              z;                                        // LOD MIPmap level to use for integration 
#define init  w = .01; \
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
    O = textureLod(iChannel0, u / iResolution.xy, 0.0);
    O.x = convol2D(U); return;
  //  O = convol1D(U,vec2(1,0));
}

