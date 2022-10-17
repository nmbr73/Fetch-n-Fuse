

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Final Compositing (Deferred Lighting + Bloom)

vec2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
	ivec2 particleCoord = ivec2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}

vec3 computeLighting( in vec3 surfaceAlbedo,
                      in vec3 surfaceNormal,
                      in float surfaceGloss,
                      in vec3 lightCol,
                      in vec3 lightDir,
                      in float lightSpec,
                      in float lightAmb )
{
    float dot_n  = clamp(dot(surfaceNormal, lightDir), 0.0, 1.0);
    
    vec3 diffuse  = lightCol * surfaceAlbedo * clamp(dot_n, lightAmb, 1.0);
    vec3 specular = lightCol * float(dot_n > 0.0) * pow(clamp(dot(reflect(-lightDir, surfaceNormal), vec3(0.0, 0.0, 1.0)), 0.0, 1.0), surfaceGloss);
    
    return diffuse + specular * lightSpec;
}

vec3 computeSpotLight( in vec3 surfaceAlbedo,
                       in vec3 surfaceNormal,
                       in float surfaceGloss,
                       in vec3 surfacePos,  
                       in vec3 lightCol,
                       in vec3 lightPos,
                       in float lightRadius )
{
    vec3 lightVec = lightPos - surfacePos;
    float contribution = 1.0 / max(dot(lightVec, lightVec) * 0.08 / (lightRadius * lightRadius), 1.0);
    
    return computeLighting(surfaceAlbedo, surfaceNormal, surfaceGloss, lightCol, normalize(lightVec), 0.066667 * surfaceGloss, 0.0) * contribution;
}

vec3 computeLightGlow(in vec3 position, in vec3 lightCol, in vec3 lightPos, in float lightRadius)
{
    vec3 glare = spotlightsGlare * lightCol * smoothstep(lightRadius * 10.0, 0.0, length((lightPos.xy - position.xy) * vec2(1.0, 16.0)));
    vec3 innerGlow = vec3(0.8) * smoothstep(lightRadius, lightRadius * 0.5, distance(lightPos.xy, position.xy));
    vec3 outerGlow = 0.25 * lightCol * smoothstep(lightRadius * 2.5, 0.0, distance(lightPos.xy, position.xy));
  
    return innerGlow + outerGlow + glare;
}

vec3 computeVignetting(in vec2 fragCoord, in vec3 src) // https://www.shadertoy.com/view/4lSXDm
{
	vec2 coord = ((fragCoord.xy / iResolution.xy) - 0.5) * (iResolution.x / iResolution.y) * 2.0;
    float rf = sqrt(dot(coord, coord)) * 0.25;
    float rf2_1 = rf * rf + 1.0;
    
	return src * pow((1.0 / (rf2_1 * rf2_1)), 2.24);
}    

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec4 albedo = texelFetch(iChannel1, ivec2(fragCoord), 0);
	vec3 normal = normalize(texelFetch(iChannel2, ivec2(fragCoord), 0).xyz);
    vec3 position = vec3(fragCoord, -(1.0 - albedo.a) * 384.0 / particlesSize); // fake Z-depth from fade level
        
    fragColor = vec4(vec3(0.0), albedo.a); 
    fragColor.rgb += computeLighting(albedo.rgb, normal, streamsGlossExp, ambientLightCol, ambientLightDir, 0.5, 0.175);
    
    for(int i = 0; i < nParticles; ++i)
    {
        vec3 particlePos = vec3(getParticlePosition(i), 0.0);
        vec3 particleCol = texelFetch(iChannel1, ivec2(particlePos.xy), 0).rgb;
            
        fragColor.rgb += computeSpotLight(albedo.rgb, normal, streamsGlossExp, position, particleCol, particlePos, particlesSize);
    }
    
    fragColor.rgb = 1.25 * fragColor.rgb - vec3(0.075);
    fragColor.rgb = mix(backgroundColor, fragColor.rgb, min(fragColor.a * 1.125, 1.0));
    fragColor.rgb = computeVignetting(fragCoord, fragColor.rgb);
    
    for(int i = 0; i < nParticles; ++i)
    {
        vec3 particlePos = vec3(getParticlePosition(i), 0.0);
        vec3 particleCol = texelFetch(iChannel1, ivec2(particlePos.xy), 0).rgb;
        
        fragColor.rgb += computeLightGlow(position, particleCol, particlePos, particlesSize);
    }
    
    fragColor = vec4(pow(fragColor.rgb, vec3(1.0 / 2.24)), 1.0); // gamma correction
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Compute Physics (Verlet Integration)

float rand(in vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 randVec2(in vec2 co) {
	return vec2(rand(co.xy + generationSeed * 0.0001), rand(-co.yx + generationSeed * 0.0001));
}

vec2 randNrm2(in vec2 fragCoord)
{
	vec2 n = vec2(-1.0) + randVec2(fragCoord) * 2.0;
    
    float l = length(n);   
    if(l <= 0.0000001) n = vec2(0.0, (l = 1.0));
    
    return (n / l);
}

void initParticle(in vec2 fragCoord, inout vec2 particlePrevPosition, inout vec2 particleCurrPosition)
{
	particleCurrPosition = randVec2(fragCoord) * iResolution.xy;
    particlePrevPosition = particleCurrPosition - randNrm2(fragCoord) * particlesSize * 0.0625;
}

vec2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
	ivec2 particleCoord = ivec2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}

vec2 computeGravitation(in int particleID, in vec2 particlePosition)
{
    vec2 acceleration = vec2(0.0);
        
	for(int i = 0; i < nParticles; ++i) if(i != particleID)
    {
        vec2 v = (getParticlePosition(i) - particlePosition);
        float d = length(v);
        
        if(d > 0.0000001) acceleration += (v / d) / pow(max(d, particlesSize * 2.0) * gravityStrength, 2.0);
    }
    
    return acceleration;
}

void solveCollisions(inout vec2 particlePrevPosition, inout vec2 particleCurrPosition)
{
    vec2 particleInertia = (particleCurrPosition - particlePrevPosition);
    
	if(particleCurrPosition.x < particlesSize || particleCurrPosition.x > iResolution.x - particlesSize)
    {
    	particleCurrPosition.x = clamp(particleCurrPosition.x, particlesSize, iResolution.x - particlesSize);
        particlePrevPosition.x = particleCurrPosition.x + particleInertia.x * collisionDamping;
    }
    
    if(particleCurrPosition.y < particlesSize || particleCurrPosition.y > iResolution.y - particlesSize)
    {
    	particleCurrPosition.y = clamp(particleCurrPosition.y, particlesSize, iResolution.y - particlesSize);
        particlePrevPosition.y = particleCurrPosition.y + particleInertia.y * collisionDamping;
    }
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    int particleID = int(floor(fragCoord.x) + iResolution.x * floor(fragCoord.y));
    if(particleID >= nParticles) return;
    
    vec4 particleData = texelFetch(iChannel0, ivec2(fragCoord), 0);
    vec2 particlePrevPosition = particleData.zw;
    vec2 particleCurrPosition = particleData.xy;
     
    if(iFrame == 0) initParticle(fragCoord, particlePrevPosition, particleCurrPosition);
   
    vec2 particleAcceleration = computeGravitation(particleID, particleCurrPosition);
    vec2 particleInertia = particleCurrPosition - particlePrevPosition;
    vec2 particleVelocity = particleInertia + particleAcceleration;
    
    particlePrevPosition = particleCurrPosition;
    particleCurrPosition += particleVelocity;
    
    solveCollisions(particlePrevPosition, particleCurrPosition);
    
    fragColor = vec4(particleCurrPosition, particlePrevPosition);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Compute Scene Albedo

vec2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
	ivec2 particleCoord = ivec2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}

vec3 getParticleColor(in vec2 p) {
    return normalize(vec3(0.1) + texture(iChannel2, p * 0.0001 + iTime * 0.005).rgb);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{ 
    fragColor = texelFetch(iChannel1, ivec2(fragCoord) + cameraVelocity, 0);
    fragColor.a *= (1.0 - streamsFadingExp);
        
	for(int i = 0; i < nParticles; ++i)
    {
        vec2 particlePos = getParticlePosition(i);
        vec3 particleCol = getParticleColor(particlePos);
        
        float alpha = smoothstep(particlesSize, particlesSize * 0.5, distance(fragCoord, particlePos));
        fragColor = mix(fragColor, vec4(particleCol , 1.0), alpha);
    }
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Compute Scene Normals

vec2 getParticlePosition(in int particleID)
{
    int iChannel0_width = int(iChannelResolution[0].x);
	ivec2 particleCoord = ivec2(particleID % iChannel0_width, particleID / iChannel0_width);
    
    return texelFetch(iChannel0, particleCoord, 0).xy;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{ 
    fragColor = texelFetch(iChannel1, ivec2(fragCoord) + cameraVelocity, 0);
    
	for(int i = 0; i < nParticles; ++i)
    {
        vec2 v = fragCoord - getParticlePosition(i);
        
        float l = length(v);
        float alpha = smoothstep(particlesSize, particlesSize * 0.5, l);
        
        float z = sqrt(abs(particlesSize * particlesSize - l * l));
        vec3 n = normalize(vec3(v, z));

        fragColor = mix(fragColor, vec4(n, 1.0), alpha);
    }
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

// #define USE_CINEMATIC_MODE      // Uncomment this line for a more cinematic view (camera will side-scroll)
// #define USE_BUMPY_STREAMS_MODE  // Uncomment this line to make streams bumpy (sausages-like)
// #define USE_GENERATION_SEED 123 // Uncomment this line to use a fixed generation seed (then reset the simulation to apply the changes)
    
const int nParticles = 20;
const float particlesSize = 8.0;
const float collisionDamping = 0.5;
const float streamsFadingExp = 0.001;
const float gravityStrength = 1.6 / particlesSize;

const vec3 ambientLightDir = normalize(vec3(1.0, 2.0, 0.0));
const vec3 ambientLightCol = vec3(1.1, 1.0, 0.9);
const vec3 backgroundColor = vec3(0.65);
const float streamsGlossExp = 120.0;
const float spotlightsGlare = 0.0;

#ifdef USE_BUMPY_STREAMS_MODE
#define particlesSize mix(particlesSize, particlesSize * 0.5, (1.0 + sin(1.85 + iTime * 11.93805208)) * 0.5)
#endif

#ifdef USE_GENERATION_SEED
#define generationSeed float(USE_GENERATION_SEED) // a fixed seed will generate the same output (in respect of the viewport size)
#else
#define generationSeed iDate.w // if no custom seed is provided, POSIX time is used instead (producing different results every time)
#endif

const ivec2 cameraVelocity =
#ifdef USE_CINEMATIC_MODE
ivec2(1, 0);
#else
ivec2(0);
#endif

// Buf A: particles positions and inertia
// Buf B: scene albedo  (accumulated)
// Buf C: scene normals (accumulated)
// Image: final compositing
