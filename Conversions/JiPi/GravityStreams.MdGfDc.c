
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// #define USE_CINEMATIC_MODE      // Uncomment this line for a more cinematic view (camera will side-scroll)
// #define USE_BUMPY_STREAMS_MODE  // Uncomment this line to make streams bumpy (sausages-like)
// #define USE_GENERATION_SEED 123 // Uncomment this line to use a fixed generation seed (then reset the simulation to apply the changes)

#ifdef XXX    
const int nParticles = 20;
const float particlesSize = 8.0f;
const float collisionDamping = 0.5f;
const float streamsFadingExp = 0.001f;
const float gravityStrength = 1.6f / particlesSize;

const float3 ambientLightDir = normalize(to_float3(1.0f, 2.0f, 0.0f));
const float3 ambientLightCol = to_float3(1.1f, 1.0f, 0.9f);
const float3 backgroundColor = to_float3_s(0.65f);
const float streamsGlossExp = 120.0f;
const float spotlightsGlare = 0.0f;
#endif


#ifdef USE_BUMPY_STREAMS_MODE
#define particlesSize _mix(particlesSize, particlesSize * 0.5f, (1.0f + _sinf(1.85f + iTime * 11.93805208f)) * 0.5f)
#endif

#ifdef USE_GENERATION_SEED
#define generationSeed (float)(USE_GENERATION_SEED) // a fixed seed will generate the same output (in respect of the viewport size)
#else
#define generationSeed iTime //iDate.w // if no custom seed is provided, POSIX time is used instead (producing different results every time)
#endif

#ifdef XXX
const int2 cameraVelocity =
#ifdef USE_CINEMATIC_MODE
to_int2(1, 0);
#else
to_int2(0);
#endif
#endif
// Buf A: particles positions and inertia
// Buf B: scene albedo  (accumulated)
// Buf C: scene normals (accumulated)
// Image: final compositing

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Compute Physics (Verlet Integration)

__DEVICE__ float rand(in float2 co) {
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}

__DEVICE__ float2 randVec2(in float2 co, float iTime) {
    return to_float2(rand(swi2(co,x,y) + generationSeed * 0.0001f), rand(-1.0f*swi2(co,y,x) + generationSeed * 0.0001f));
}

__DEVICE__ float2 randNrm2(in float2 fragCoord, float iTime)
{
    float2 n = to_float2_s(-1.0f) + randVec2(fragCoord, iTime) * 2.0f;
    
    float l = length(n);   
    if(l <= 0.0000001f) n = to_float2(0.0f, (l = 1.0f));
    
    return (n / l);
}

__DEVICE__ void initParticle(in float2 fragCoord, inout float2 *particlePrevPosition, inout float2 *particleCurrPosition, float2 iResolution, float particlesSize, float iTime)
{
    *particleCurrPosition = randVec2(fragCoord, iTime) * iResolution;
    *particlePrevPosition = *particleCurrPosition - randNrm2(fragCoord, iTime) * particlesSize * 0.0625f;
}

__DEVICE__ float2 getParticlePosition(in int particleID, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    //int iChannel0_width = int(iChannelResolution[0].x);
    int iChannel0_width = (int)(iResolution.x);
    int2 particleCoord = to_int2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    //return texelFetch(iChannel0, particleCoord, 0).xy;
    return swi2(texture(iChannel0, (make_float2(particleCoord)+0.5f)/iResolution),x,y);
}

__DEVICE__ float2 computeGravitation(in int particleID, in float2 particlePosition, float2 iResolution, __TEXTURE2D__ iChannel0, float particlesSize, int nParticles, float gravityStrength)
{
    float2 acceleration = to_float2_s(0.0f);
        
    for(int i = 0; i < nParticles; ++i) if(i != particleID)
    {
        float2 v = (getParticlePosition(i, R, iChannel0) - particlePosition);
        float d = length(v);
float aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa;        
        if(d > 0.0000001f) acceleration += (v / d) / _powf(_fmaxf(d, particlesSize * 2.0f) * gravityStrength, 2.0f);
    }
    
    return acceleration;
}

__DEVICE__ void solveCollisions(inout float2 *particlePrevPosition, inout float2 *particleCurrPosition, float particlesSize, float collisionDamping, float2 R )
{
    float2 particleInertia = (*particleCurrPosition - *particlePrevPosition);
    
    if((*particleCurrPosition).x < particlesSize || (*particleCurrPosition).x > iResolution.x - particlesSize)
    {
        (*particleCurrPosition).x = clamp((*particleCurrPosition).x, particlesSize, iResolution.x - particlesSize);
        (*particlePrevPosition).x = (*particleCurrPosition).x + particleInertia.x * collisionDamping;
    }
    
    if((*particleCurrPosition).y < particlesSize || (*particleCurrPosition).y > iResolution.y - particlesSize)
    {
        (*particleCurrPosition).y = clamp((*particleCurrPosition).y, particlesSize, iResolution.y - particlesSize);
        (*particlePrevPosition).y = (*particleCurrPosition).y + particleInertia.y * collisionDamping;
    }
}

__KERNEL__ void GravityStreamsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, float3 iChannelResolution[], sampler2D iChannel0)
{
    fragCoord+=0.5f;

    const int nParticles = 20;
    const float particlesSize = 8.0f;
    const float collisionDamping = 0.5f;
    const float streamsFadingExp = 0.001f;
    const float gravityStrength = 1.6f / particlesSize;

    const float3 ambientLightDir = normalize(to_float3(1.0f, 2.0f, 0.0f));
    const float3 ambientLightCol = to_float3(1.1f, 1.0f, 0.9f);
    const float3 backgroundColor = to_float3_s(0.65f);
    const float streamsGlossExp = 120.0f;
    const float spotlightsGlare = 0.0f;


    int particleID = (int)(_floor(fragCoord.x) + iResolution.x * _floor(fragCoord.y));
    if(particleID >= nParticles) 
    {
      SetFragmentShaderComputedColor(fragColor);
      return;
    }  
float AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA;    
    //float4 particleData = texelFetch(iChannel0, to_int2(fragCoord), 0);
    float4 particleData = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R);
    
    float2 particlePrevPosition = swi2(particleData,z,w);
    float2 particleCurrPosition = swi2(particleData,x,y);
     
    if(iFrame == 0) initParticle(fragCoord, &particlePrevPosition, &particleCurrPosition, iResolution, particlesSize, iTime);
   
    float2 particleAcceleration = computeGravitation(particleID, particleCurrPosition, R, iChannel0, particlesSize, nParticles, gravityStrength);
    float2 particleInertia = particleCurrPosition - particlePrevPosition;
    float2 particleVelocity = particleInertia + particleAcceleration;
    
    particlePrevPosition = particleCurrPosition;
    particleCurrPosition += particleVelocity;
    
    solveCollisions(&particlePrevPosition, &particleCurrPosition, particlesSize, collisionDamping, R);
    
    fragColor = to_float4_f2f2(particleCurrPosition, particlePrevPosition);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: RGBA Noise Small' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Compute Scene Albedo
#ifdef XXXXXXXX
__DEVICE__ float2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
    int2 particleCoord = to_int2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}
#endif

__DEVICE__ float3 getParticleColor(in float2 p, float iTime, __TEXTURE2D__ iChannel2) {
    return normalize(to_float3_s(0.1f) + swi3(texture(iChannel2, p * 0.0001f + iTime * 0.005f),x,y,z));
}

__KERNEL__ void GravityStreamsFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;
 
    const int nParticles = 20;
    const float particlesSize = 8.0f;
    const float collisionDamping = 0.5f;
    const float streamsFadingExp = 0.001f;
    const float gravityStrength = 1.6f / particlesSize;

    const float3 ambientLightDir = normalize(to_float3(1.0f, 2.0f, 0.0f));
    const float3 ambientLightCol = to_float3(1.1f, 1.0f, 0.9f);
    const float3 backgroundColor = to_float3_s(0.65f);
    const float streamsGlossExp = 120.0f;
    const float spotlightsGlare = 0.0f;

    const int2 cameraVelocity =
    #ifdef USE_CINEMATIC_MODE
    to_int2(1, 0);
    #else
    to_int2(0, 0);
    #endif
     
 float BBBBBBBBBBBBBBBBBBBBBBBBBBB;
 
    //fragColor = texelFetch(iChannel1, to_int2(fragCoord) + cameraVelocity, 0);
    fragColor = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord)+cameraVelocity)+0.5f)/R );
    
    fragColor.w *= (1.0f - streamsFadingExp);
        
    for(int i = 0; i < nParticles; ++i)
    {
        float2 particlePos = getParticlePosition(i, R, iChannel0);
        float3 particleCol = getParticleColor(particlePos, iTime, iChannel2);
        
        float alpha = smoothstep(particlesSize, particlesSize * 0.5f, distance_f2(fragCoord, particlePos));
        fragColor = _mix(fragColor, to_float4_aw(particleCol , 1.0f), alpha);
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// Compute Scene Normals
#ifdef XXX
__DEVICE__ float2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
    int2 particleCoord = to_int2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}
#endif

__KERNEL__ void GravityStreamsFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f; 
 
    const int nParticles = 20;
    const float particlesSize = 8.0f;
    const float collisionDamping = 0.5f;
    const float streamsFadingExp = 0.001f;
    const float gravityStrength = 1.6f / particlesSize;

    const float3 ambientLightDir = normalize(to_float3(1.0f, 2.0f, 0.0f));
    const float3 ambientLightCol = to_float3(1.1f, 1.0f, 0.9f);
    const float3 backgroundColor = to_float3_s(0.65f);
    const float streamsGlossExp = 120.0f;
    const float spotlightsGlare = 0.0f;

    const int2 cameraVelocity =
    #ifdef USE_CINEMATIC_MODE
    to_int2(1, 0);
    #else
    to_int2(0, 0);
    #endif
 
 float CCCCCCCCCCCCCCCCCCCCCCCCCCCC;
    //fragColor = texelFetch(iChannel1, to_int2(fragCoord) + cameraVelocity, 0);
    fragColor = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord)+cameraVelocity)+0.5f)/R );
    
    for(int i = 0; i < nParticles; ++i)
    {
        float2 v = fragCoord - getParticlePosition(i,R,iChannel0);
        
        float l = length(v);
        float alpha = smoothstep(particlesSize, particlesSize * 0.5f, l);
        
        float z = _sqrtf(_fabs(particlesSize * particlesSize - l * l));
        float3 n = normalize(to_float3_aw(v, z));

        fragColor = _mix(fragColor, to_float4_aw(n, 1.0f), alpha);
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/48e2d9ef22ca6673330b8c38a260c87694d2bbc94c19fec9dfa4a1222c364a99.mp3' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// Final Compositing (Deferred Lighting + Bloom)
#ifdef XXX
__DEVICE__ float2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
    int2 particleCoord = to_int2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}
#endif

__DEVICE__ float3 computeLighting( in float3 surfaceAlbedo,
                      in float3 surfaceNormal,
                      in float surfaceGloss,
                      in float3 lightCol,
                      in float3 lightDir,
                      in float lightSpec,
                      in float lightAmb )
{
    float dot_n  = clamp(dot(surfaceNormal, lightDir), 0.0f, 1.0f);
    
    float3 diffuse  = lightCol * surfaceAlbedo * clamp(dot_n, lightAmb, 1.0f);
    float3 specular = lightCol * (float)(dot_n > 0.0f) * _powf(clamp(dot(reflect(-lightDir, surfaceNormal), to_float3(0.0f, 0.0f, 1.0f)), 0.0f, 1.0f), surfaceGloss);
    
    return diffuse + specular * lightSpec;
}

__DEVICE__ float3 computeSpotLight( in float3 surfaceAlbedo,
                       in float3 surfaceNormal,
                       in float surfaceGloss,
                       in float3 surfacePos,  
                       in float3 lightCol,
                       in float3 lightPos,
                       in float lightRadius )
{
    float3 lightVec = lightPos - surfacePos;
    float contribution = 1.0f / _fmaxf(dot(lightVec, lightVec) * 0.08f / (lightRadius * lightRadius), 1.0f);
    
    return computeLighting(surfaceAlbedo, surfaceNormal, surfaceGloss, lightCol, normalize(lightVec), 0.066667f * surfaceGloss, 0.0f) * contribution;
}

__DEVICE__ float3 computeLightGlow(in float3 position, in float3 lightCol, in float3 lightPos, in float lightRadius, float spotlightsGlare)
{
    float3 glare = spotlightsGlare * lightCol * smoothstep(lightRadius * 10.0f, 0.0f, length((swi2(lightPos,x,y) - swi2(position,x,y)) * to_float2(1.0f, 16.0f)));
    float3 innerGlow = to_float3_s(0.8f) * smoothstep(lightRadius, lightRadius * 0.5f, distance_f2(swi2(lightPos,x,y), swi2(position,x,y)));
    float3 outerGlow = 0.25f * lightCol * smoothstep(lightRadius * 2.5f, 0.0f, distance_f2(swi2(lightPos,x,y), swi2(position,x,y)));
float iiiiiiiiiiiiiiiiiiiiiiii;  
    return innerGlow + outerGlow + glare;
}

__DEVICE__ float3 computeVignetting(in float2 fragCoord, in float3 src, float2 R) // https://www.shadertoy.com/view/4lSXDm
{
    float2 coord = ((fragCoord / iResolution) - 0.5f) * (iResolution.x / iResolution.y) * 2.0f;
    float rf = _sqrtf(dot(coord, coord)) * 0.25f;
    float rf2_1 = rf * rf + 1.0f;
    
    return src * _powf((1.0f / (rf2_1 * rf2_1)), 2.24f);
}    

__KERNEL__ void GravityStreamsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    const int nParticles = 20;
    const float particlesSize = 8.0f;
    const float collisionDamping = 0.5f;
    const float streamsFadingExp = 0.001f;
    const float gravityStrength = 1.6f / particlesSize;

    const float3 ambientLightDir = normalize(to_float3(1.0f, 2.0f, 0.0f));
    const float3 ambientLightCol = to_float3(1.1f, 1.0f, 0.9f);
    const float3 backgroundColor = to_float3_s(0.65f);
    const float streamsGlossExp = 120.0f;
    const float spotlightsGlare = 0.0f;



    //float4 albedo = texelFetch(iChannel1, to_int2(fragCoord), 0);
    float4 albedo = texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R);
    //float3 normal = normalize(texelFetch(iChannel2, to_int2(fragCoord), 0).xyz);
    float3 normal = normalize(swi3(texture(iChannel2, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R),x,y,z));
    
    float3 position = to_float3_aw(fragCoord, -(1.0f - albedo.w) * 384.0f / particlesSize); // fake Z-depth from fade level
        
    fragColor = to_float4_aw(to_float3_s(0.0f), albedo.w); 
    float3 fragColorxyz = swi3(fragColor,x,y,z) + computeLighting(swi3(albedo,x,y,z), normal, streamsGlossExp, ambientLightCol, ambientLightDir, 0.5f, 0.175f);
    
    
    
    for(int i = 0; i < nParticles; ++i)
    {
        float3 particlePos = to_float3_aw(getParticlePosition(i,R,iChannel0), 0.0f);
        //float3 particleCol = texelFetch(iChannel1, to_int2(swi2(particlePos,x,y)), 0).rgb;
        float3 particleCol = swi3(texture(iChannel1, (make_float2(to_int2_cfloat(swi2(particlePos,x,y)))+0.5f)/R),x,y,z);
            
        fragColorxyz += computeSpotLight(swi3(albedo,x,y,z), normal, streamsGlossExp, position, particleCol, particlePos, particlesSize);
    }
    
    fragColorxyz = 1.25f * fragColorxyz - to_float3_s(0.075f);
    fragColorxyz = _mix(backgroundColor, fragColorxyz, _fminf(fragColor.w * 1.125f, 1.0f));
    fragColorxyz = computeVignetting(fragCoord, fragColorxyz, R);
    
    for(int i = 0; i < nParticles; ++i)
    {
        float3 particlePos = to_float3_aw(getParticlePosition(i,R,iChannel0), 0.0f);
        //float3 particleCol = texelFetch(iChannel1, to_int2(swi2(particlePos,x,y)), 0).rgb;
        float3 particleCol = swi3(texture(iChannel1, (make_float2(to_int2_cfloat(swi2(particlePos,x,y)))+0.5f)/R),x,y,z);
float IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII;        
        fragColorxyz += computeLightGlow(position, particleCol, particlePos, particlesSize, spotlightsGlare);
    }
    
    fragColor = to_float4_aw(pow_f3(fragColorxyz, to_float3_s(1.0f / 2.24f)), 1.0f); // gamma correction

  SetFragmentShaderComputedColor(fragColor);
}