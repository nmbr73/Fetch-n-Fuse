

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    fragColor = vec4(texture(iChannel0, uv).rgb, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Written by Chris Wallis (@chriskwallis)
#define PERFORMANCE_MODE 0
#define ULTRA_MODE 0 

#if PERFORMANCE_MODE
#define ALLOW_KEYBOARD_INPUT 0
#define SECONDARY_REFLECTION 0
#define ADD_WHITE_WATER 0
#define MAX_SDF_SPHERE_STEPS 12
#define SDF_START_STEP_SIZE 3.0
#define SDF_END_STEP_SIZE 15.0
#define MAX_VOLUME_MARCH_STEPS 10
#define BINARY_SEARCH_STEPS 5
#define MAX_OPAQUE_SHADOW_MARCH_STEPS 4
#define SHADOW_FACTOR_STEP_SIZE 10.0
#else
#if ULTRA_MODE
#define MAX_VOLUME_ENTRIES 2
#else
#define MAX_VOLUME_ENTRIES 1
#endif
#define MAX_VOLUME_ENTRIES 1
#define ALLOW_KEYBOARD_INPUT 1
#define SECONDARY_REFLECTION 1
#define ADD_WHITE_WATER 1
#define MAX_SDF_SPHERE_STEPS 20
#define SDF_START_STEP_SIZE 1.5
#define SDF_END_STEP_SIZE 8.0
#define MAX_VOLUME_MARCH_STEPS 20
#define BINARY_SEARCH_STEPS 6
#define MAX_OPAQUE_SHADOW_MARCH_STEPS 10
#define SHADOW_FACTOR_STEP_SIZE 7.0
#endif

#define GROUND_LEVEL 0.0
#define WATER_LEVEL 22.0
#define PI 3.14
#define LARGE_NUMBER 1e20
#define EPSILON 0.0001

#define NUM_SPHERES 5

#define SAND_FLOOR_OBJECT_ID 0
#define CORAL_ROCK_BASE_OBJECT_ID 1
#define NUM_OBJECTS (CORAL_ROCK_BASE_OBJECT_ID + NUM_SPHERES)

#define INVALID_OBJECT_ID int(-1)

#define AIR_IOR 1.0

#define SKY_AMBIENT_MULTIPLIER 0.1

#define MAX_WATER_DISPLACEMENT 15.0
#define MIN_REFLECTION_COEFFECIENT 0.0

#define SCENE_TYPE_OCEAN 1
#define SCENE_TYPE_SIMPLIFIED_OCEAN 2
#define SCENE_TYPE_OPAQUE 3

float WaterIor;
float WaterTurbulence;
float WaterAbsorption;
vec3 WaterColor;

// --------------------------------------------//
//               Noise Functions
// --------------------------------------------//
// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float hash1( float n )
{
    return fract( n*17.0*fract( n*0.3183099 ) );
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    
    float n = p.x + 317.0*p.y + 157.0*p.z;
    
    float a = hash1(n+0.0);
    float b = hash1(n+1.0);
    float c = hash1(n+317.0);
    float d = hash1(n+318.0);
    float e = hash1(n+157.0);
	float f = hash1(n+158.0);
    float g = hash1(n+474.0);
    float h = hash1(n+475.0);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}

const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm( in vec3 x, int iterations )
{
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for( int i=min(0, iFrame); i<iterations; i++ )
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
	return a;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm_4( in vec3 x )
{
    return fbm(x, 4);
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdPlane( vec3 p )
{
	return p.y;
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSmoothUnion( float d1, float d2, float k ) 
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); 
}

vec3 sdTranslate(vec3 pos, vec3 translate)
{
    return pos -= translate;
}

// Taken from https://iquilezles.org/articles/distfunctions
float sdSphere( vec3 p, vec3 origin, float s )
{
  p = sdTranslate(p, origin);
  return length(p)-s;
}


struct Sphere
{
    vec3 origin;
    float radius;
};
    
void GetSphere(int index, out vec3 origin, out float radius)
{
    Sphere spheres[NUM_SPHERES];
    spheres[0] = Sphere(vec3(38, GROUND_LEVEL, 32), 12.0);
    spheres[1] = Sphere(vec3(33, GROUND_LEVEL - 2.0, 20), 8.5);
    spheres[2] = Sphere(vec3(-25, GROUND_LEVEL - 32.0, 55), 40.0);
    spheres[3] = Sphere(vec3(-40, GROUND_LEVEL, 25), 12.0);
    spheres[4] = Sphere(vec3(45, GROUND_LEVEL, 10), 12.0);

    origin = spheres[index].origin;
    radius = spheres[index].radius;

}

float GetWaterWavesDisplacement(vec3 position, float time)
{
    return 7.0 * sin(position.x / 15.0 + time * 1.3) +
        6.0 * cos(position.z / 15.0 + time / 1.1);
}

float GetWaterNoise(vec3 position, float time)
{
    return WaterTurbulence * fbm_4(position / 15.0 + time / 3.0);
}

float QueryOceanDistanceField( in vec3 pos, float time)
{    
    return GetWaterWavesDisplacement(pos, time) + GetWaterNoise(pos, time) + sdPlane(pos - vec3(0, WATER_LEVEL, 0));
}

float QueryVolumetricDistanceField( in vec3 pos, float time)
{    
    float minDist = QueryOceanDistanceField(pos, time);
    minDist = sdSmoothSubtraction(sdSphere(pos, vec3(0.0, 20.0, 0), 35.0) +  5.0 * fbm_4(pos / vec3(12, 20, 12)  - time / 5.0), minDist, 12.0);   
    minDist = sdSmoothUnion(minDist, sdPlane(pos - vec3(0, GROUND_LEVEL - 1.0, 0)), 13.0);

    return minDist;
}

float IntersectVolumetric(in vec3 rayOrigin, in vec3 rayDirection, float maxT, float time, int sceneType, out bool intersectFound)
{
    float t = 0.0f;
    float sdfValue = 0.0;
    float stepSize = SDF_START_STEP_SIZE;
    float stepIncrement = float(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / float(MAX_SDF_SPHERE_STEPS);
    for(int i=0; i<MAX_SDF_SPHERE_STEPS; i++ )
    {
	    sdfValue = sceneType == SCENE_TYPE_OCEAN ?
            QueryVolumetricDistanceField(rayOrigin+rayDirection*t, time) :
        	QueryOceanDistanceField(rayOrigin+rayDirection*t, time);
        stepSize += stepIncrement;
        
        if( sdfValue < 0.0 || t>maxT ) break;
        t += max(sdfValue, stepSize);
    }
    
    if(sdfValue < 0.0f)
    {
        float start = 0.0;
        float end = stepSize;
        t -= stepSize;
        
        for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
        {
            float midPoint = (start + end) * 0.5;
            vec3 nextMarchPosition = rayOrigin + (t + midPoint) * rayDirection;
            float sdfValue = (sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(nextMarchPosition, time) :
                QueryOceanDistanceField(nextMarchPosition, time);
            
            // Use the SDF to nudget the mid point closer to the actual edge
            midPoint = clamp(midPoint + sdfValue, start, end);
            if(sdfValue < 0.0)
            {
                end = midPoint;
            }
            else
            {
                start = midPoint;
            }
        }
        t += end;
    }
    
    intersectFound = t<maxT && sdfValue < 0.0;
    return t;
}

// Taken from https://iquilezles.org/articles/normalsSDF
vec3 GetVolumeNormal( in vec3 pos, float time, int sceneType )
{
    vec3 n = vec3(0.0);
    for( int i=min(0, iFrame); i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*
            ((sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(pos+0.5*e, time) :
                QueryOceanDistanceField(pos+0.5*e, time));
    }
    return normalize(n);
}

struct CameraDescription
{
    vec3 Position;
    vec3 LookAt;    

    float LensHeight;
    float FocalDistance;
};
    
CameraDescription Camera = CameraDescription(
    vec3(0, 10, -20),
    vec3(0, 10, 0),
    2.0,
    1.6
);

struct Material
{
    vec3 albedo;
    float shininess;
};

Material GetMaterial(int objectID)
{
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
        return Material(0.9 * vec3(1.0, 1.0, 0.8), 50.0);
    }
    else // if(objectID >= CORAL_ROCK_BASE_OBJECT_ID) // it's coral
    {
        return Material(vec3(0.3, 0.4, 0.2), 3.0);
    }
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
float PlaneIntersection(vec3 rayOrigin, vec3 rayDirection, vec3 planeOrigin, vec3 planeNormal) 
{ 
    float t = -1.0f;
    float denom = dot(-planeNormal, rayDirection); 
    if (denom > EPSILON) { 
        vec3 rayToPlane = planeOrigin - rayOrigin; 
        return dot(rayToPlane, -planeNormal) / denom; 
    } 
 
    return t; 
} 
    
float SphereIntersection(
    in vec3 rayOrigin, 
    in vec3 rayDirection, 
    in vec3 sphereCenter, 
    in float sphereRadius)
{
      vec3 eMinusC = rayOrigin - sphereCenter;
      float dDotD = dot(rayDirection, rayDirection);

      float discriminant = dot(rayDirection, (eMinusC)) * dot(rayDirection, (eMinusC))
         - dDotD * (dot(eMinusC, eMinusC) - sphereRadius * sphereRadius);

      if (discriminant < 0.0) 
         return -1.0;

      float firstIntersect = (dot(-rayDirection, eMinusC) - sqrt(discriminant))
             / dDotD;
      
      float t = firstIntersect;
      return t;
}


void UpdateIfIntersected(
    inout float t,
    in float intersectionT, 
    in int intersectionObjectID,
    out int objectID)
{    
    if(intersectionT > EPSILON && intersectionT < t)
    {
        objectID = intersectionObjectID;
        t = intersectionT;
    }
}

float SandHeightMap(vec3 position)
{
    float sandGrainNoise = 0.1 * fbm(position * 10.0, 2);
    float sandDuneDisplacement = 0.7 * sin(10.0 * fbm_4(10.0 + position / 40.0));
	return sandGrainNoise  + sandDuneDisplacement;
}

float QueryOpaqueDistanceField(vec3 position, int objectID)
{
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
        return sdPlane(position) + SandHeightMap(position);
    }
    else
    {
        vec3 origin;
        float radius;
        GetSphere(objectID - CORAL_ROCK_BASE_OBJECT_ID, origin, radius);
        return sdSphere(position, origin, radius) + fbm_4(position);
    }
}

// Taken from https://iquilezles.org/articles/normalsSDF
vec3 GetOpaqueNormal( in vec3 pos, int objectID )
{
    vec3 n = vec3(0.0);
    for( int i=min(0, iFrame); i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*QueryOpaqueDistanceField(pos+0.5*e, objectID);
    }
    return normalize(n);
}

float IntersectOpaqueScene(in vec3 rayOrigin, in vec3 rayDirection, out int objectID)
{
    float intersectionT = LARGE_NUMBER;

    float t = LARGE_NUMBER;
    objectID = INVALID_OBJECT_ID;

    for(int i = min(0, iFrame); i < NUM_SPHERES; i++)
    {
        vec3 origin;
        float radius;
        GetSphere(i, origin, radius);
            UpdateIfIntersected(
            t,
            SphereIntersection(rayOrigin, rayDirection, origin, radius),
            CORAL_ROCK_BASE_OBJECT_ID + i,
            objectID);
    }
    
    UpdateIfIntersected(
        t,
        PlaneIntersection(rayOrigin, rayDirection, vec3(0, GROUND_LEVEL, 0), vec3(0, 1, 0)),
        SAND_FLOOR_OBJECT_ID,
        objectID);
    
    // Optimization for early-out on volume intersections
    UpdateIfIntersected(
        t,
        PlaneIntersection(rayOrigin, rayDirection, vec3(0, WATER_LEVEL + MAX_WATER_DISPLACEMENT, 0), vec3(0, 1, 0)),
        INVALID_OBJECT_ID,
        objectID);

    return t;
}

float Specular(in vec3 reflection, in vec3 lightDirection, float shininess)
{
    return 0.05 * pow(max(0.0, dot(reflection, lightDirection)), shininess);
}

vec3 Diffuse(in vec3 normal, in vec3 lightVec, in vec3 diffuse)
{
    float nDotL = dot(normal, lightVec);
    return clamp(nDotL * diffuse, 0.0, 1.0);
}

vec3 BeerLambert(vec3 absorption, float dist)
{
    return exp(-absorption * dist);
}

vec3 GetShadowFactor(in vec3 rayOrigin, in vec3 rayDirection, in int maxSteps, in float minMarchSize)
{
    float t = 0.0f;
    vec3 shadowFactor = vec3(1.0f);
    float signedDistance = 0.0;
    bool enteredVolume = false;
    for(int i = min(0, iFrame); i < maxSteps; i++)
    {         
        float marchSize = max(minMarchSize, abs(signedDistance));
        t += marchSize;

        vec3 position = rayOrigin + t*rayDirection;

        signedDistance = QueryVolumetricDistanceField(position, iTime);
        if(signedDistance < 0.0)
        {
            // Soften the shadows towards the edges to simulate an area light
            float softEdgeMultiplier = min(abs(signedDistance / 5.0), 1.0);
            shadowFactor *= BeerLambert(WaterAbsorption * softEdgeMultiplier / WaterColor, marchSize);
            enteredVolume = true;
        }
        else if(enteredVolume)
        {
            // Optimization taking advantage of the shape of the water. The water isn't
            // concave therefore if we've entered it once and are exiting, we're done
            break;
        }
    }
    return shadowFactor;
}

float GetApproximateIntersect(vec3 position, vec3 rayDirection)
{
    float distanceToPlane;
    
    // Special case for rays parallel to the ground plane to avoid divide
    // by zero issues
    if(abs(rayDirection.y) < 0.01)
    {
        distanceToPlane = LARGE_NUMBER;
    }
    else if(position.y < GROUND_LEVEL || position.y > WATER_LEVEL)
    {
        distanceToPlane = 0.0f;
    }
    else if(rayDirection.y > 0.0)
    {
		distanceToPlane = (WATER_LEVEL - position.y) / rayDirection.y;
        vec3 intersectPosition = position + distanceToPlane * rayDirection;
        
        distanceToPlane = max(0.0, distanceToPlane);
    }
    else
    {
        distanceToPlane = (position.y - GROUND_LEVEL) / abs(rayDirection.y);
    }
    return distanceToPlane;
}

vec3 GetApproximateShadowFactor(vec3 position, vec3 rayDirection)
{
    float distanceToPlane = GetApproximateIntersect(position, rayDirection);
	return BeerLambert(WaterAbsorption / WaterColor, distanceToPlane);
}

float seed = 0.;
float rand() { return fract(sin(seed++ + iTime)*43758.5453123); }

float smoothVoronoi( in vec2 x )
{
    ivec2 p = ivec2(floor( x ));
    vec2  f = fract( x );

    float res = 0.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        ivec2 b = ivec2( i, j );
        vec2  r = vec2( b ) - f + noise( vec3(p + b, 0.0) );
        float d = length( r );

        res += exp( -32.0*d );
    }
    return -(1.0/32.0)*log( res );
}

vec3 GetSunLightDirection()
{
    return normalize(vec3(0.3, 1.0, 1.65));
}

vec3 GetSunLightColor()
{
    return 0.9 * vec3(0.9, 0.75, 0.7);
}

vec3 GetBaseSkyColor(vec3 rayDirection)
{
	return mix(
        vec3(0.2, 0.5, 0.8),
        vec3(0.7, 0.75, 0.9),
         max(rayDirection.y, 0.0));
}

vec3 GetAmbientSkyColor()
{
    return SKY_AMBIENT_MULTIPLIER * GetBaseSkyColor(vec3(0, 1, 0));
}

vec3 GetAmbientShadowColor()
{
    return vec3(0, 0, 0.2);
}

float GetCloudDenity(vec3 position)
{
    float time = iTime * 0.25;
    vec3 noisePosition = position + vec3(0.0, 0.0, time);
    float noise = fbm_4(noisePosition);
    float noiseCutoff = -0.3;
    return max(0.0, 3.0f * (noise - noiseCutoff));
}

vec4 GetCloudColor(vec3 position)
{
    float cloudDensity = GetCloudDenity(position);
    vec3 cloudAlbedo = vec3(1, 1, 1);
    float cloudAbsorption = 0.6;
    float marchSize = 0.25;

    vec3 lightFactor = vec3(1, 1, 1);
    {
        vec3 marchPosition = position;
        int selfShadowSteps = 4;
        for(int i = 0; i < selfShadowSteps; i++)
        {
            marchPosition += GetSunLightDirection() * marchSize;
            float density = cloudAbsorption * GetCloudDenity(marchPosition);
            lightFactor *= BeerLambert(vec3(density, density, density), marchSize);
        }
    }

    return vec4(
        cloudAlbedo * 
        	(mix(GetAmbientShadowColor(), 1.3 * GetSunLightColor(), lightFactor) +
             GetAmbientSkyColor()), 
        min(cloudDensity, 1.0));
}

vec3 GetSkyColor(in vec3 rayDirection)
{
    vec3 skyColor = GetBaseSkyColor(rayDirection);
    vec4 cloudColor = GetCloudColor(rayDirection * 4.0);
    skyColor = mix(skyColor, cloudColor.rgb, cloudColor.a);

    return skyColor;
}

float FresnelFactor(
    float CurrentIOR,
    float NewIOR,
    vec3 Normal,
    vec3 RayDirection)
{
    float ReflectionCoefficient = 
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR)) *
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR));
    return 
        clamp(ReflectionCoefficient + (1.0 - ReflectionCoefficient) * pow(1.0 - dot(Normal, -RayDirection), 5.0), MIN_REFLECTION_COEFFECIENT, 1.0); 
}

vec3 SandParallaxOcclusionMapping(vec3 position, vec3 view)
{
    int pomCount = 6;
    float marchSize = 0.3;
    for(int i = 0; i < pomCount; i++)
    {
        if(position.y < GROUND_LEVEL -  SandHeightMap(position)) break;
        position += view * marchSize;
    }
    return position;
}

void CalculateLighting(vec3 position, vec3 view, int objectID, inout vec3 color, bool useFastLighting)
{   
    Material material = GetMaterial(objectID);
    float sdfValue = QueryVolumetricDistanceField(position, iTime);
    bool bUnderWater = sdfValue < 0.0;

    float wetnessFactor = 0.0;
    if(objectID == SAND_FLOOR_OBJECT_ID && !useFastLighting)
    {
        float wetSandDistance = 0.7;
		if(sdfValue <= wetSandDistance)
        {
            // Darken the sand albedo to make it look wet
            float fadeEdge = 0.2;
            wetnessFactor = 1.0 - max(0.0, (sdfValue - (wetSandDistance - fadeEdge)) / fadeEdge);
			material.albedo *= material.albedo * mix(1.0, 0.5, wetnessFactor);
        }
        
        position = SandParallaxOcclusionMapping(position, view);
    }

    vec3 normal = GetOpaqueNormal(position, objectID);
    vec3 reflectionDirection = reflect(view, normal);
   
    int shadowObjectID = INVALID_OBJECT_ID;
    if(!useFastLighting)
    {
        IntersectOpaqueScene(position, GetSunLightDirection(), shadowObjectID);
    }
    
	vec3 shadowFactor = vec3(0.0, 0.0, 0.0);
    if(shadowObjectID == INVALID_OBJECT_ID)
    {
        float t;
        shadowFactor = useFastLighting ? 
                GetApproximateShadowFactor(position, GetSunLightDirection()) :
                GetShadowFactor(position, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE);
        
        // Small back splash of the sky ambient color to fake a bit of gi
        color += shadowFactor * material.albedo * mix(0.4 * GetAmbientShadowColor(), GetSunLightColor(), max(0.0, dot(normal, GetSunLightDirection())));
        
        color += shadowFactor * GetSunLightColor() * Specular(reflectionDirection, GetSunLightDirection(), material.shininess);
 	    
        if(!useFastLighting)
        {
            // Fake caustics
            float waterNoise = fract(GetWaterNoise(position, iTime));
            float causticMultiplier = bUnderWater ? 7.0 : (1.0 - shadowFactor.r);
            color += material.albedo * causticMultiplier * 0.027 *  pow(
                smoothVoronoi(position.xz / 4.0 + 
                          vec2(iTime, iTime + 3.0) + 
                          3.0 * vec2(cos(waterNoise), sin(waterNoise))), 5.0);
        }
        
    }
    
    // Add a bit of reflection to wet sand to make it look like 
    // there's some water left over
    if(!useFastLighting && wetnessFactor > 0.0)
    {
        // Water fills in the holes in the sand and generally
        // makes the surface planar so we can assume the normal is
        // pointing straight up
        vec3 wetNormal = vec3(0, 1, 0);
        vec3 reflectionDirection = reflect(view, wetNormal);
        float fresnel = FresnelFactor(AIR_IOR, WaterIor, wetNormal, view);
        color += shadowFactor * wetnessFactor * fresnel * GetSkyColor(reflectionDirection);
    }
    
    color += GetAmbientSkyColor() * material.albedo;
}

vec3 Render( in vec3 rayOrigin, in vec3 rayDirection)
{
    vec3 accumulatedColor = vec3(0.0f);
    vec3 accumulatedColorMultiplier = vec3(1.0);
    
    int materialID = INVALID_OBJECT_ID;
    float t = IntersectOpaqueScene(rayOrigin, rayDirection, materialID);
    vec3 opaquePosition = rayOrigin + t*rayDirection;
    
    bool outsideVolume = true;
    for(int entry = 0; entry < MAX_VOLUME_ENTRIES; entry++) 
    { 
        if(!outsideVolume) break;
        
        bool firstEntry = (entry == 0);
        bool intersectFound = false;
        float volumeStart = 
            IntersectVolumetric(
                rayOrigin,
                rayDirection, 
                t, 
                iTime,
                (firstEntry ? SCENE_TYPE_OCEAN : SCENE_TYPE_SIMPLIFIED_OCEAN),
                intersectFound);
        
        if(!intersectFound) break;
		else
        {
            outsideVolume = false;
            rayOrigin = rayOrigin + rayDirection * volumeStart;
            vec3 volumeNormal = GetVolumeNormal(rayOrigin, iTime, SCENE_TYPE_OCEAN);
            vec3 reflection = reflect( rayDirection, volumeNormal);
            float fresnelFactor = FresnelFactor(AIR_IOR, WaterIor, volumeNormal, rayDirection);
            float waterShininess = 100.0;

            float whiteWaterFactor = 0.0;
            float whiteWaterMaxHeight = 5.0;
            float groundBlendFactor = min(1.0, (rayOrigin.y - GROUND_LEVEL) * 0.75);
            #if ADD_WHITE_WATER
            if(firstEntry && rayOrigin.y <= whiteWaterMaxHeight)
            {
                WaterIor = mix(1.0, WaterIor, groundBlendFactor);
                
                vec3 voronoisePosition = rayOrigin / 1.5 + vec3(0, -iTime * 2.0, sin(iTime));
            	float noiseValue = abs(fbm(voronoisePosition, 2));
            	voronoisePosition += 1.0 * vec3(cos(noiseValue), 0.0, sin(noiseValue));
                
                float heightLerp =  (whiteWaterMaxHeight - rayOrigin.y) / whiteWaterMaxHeight;
                whiteWaterFactor = abs(smoothVoronoi(voronoisePosition.xz)) * heightLerp;
                whiteWaterFactor = clamp(whiteWaterFactor, 0.0, 1.0);
                whiteWaterFactor = pow(whiteWaterFactor, 0.2) * heightLerp;
                whiteWaterFactor *= mix(abs(fbm(rayOrigin + vec3(0, -iTime * 5.0, 0), 2)), 1.0, heightLerp);
                whiteWaterFactor *= groundBlendFactor;
                
                vec3 shadowFactor =  GetShadowFactor(rayOrigin, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE);
                vec3 diffuse = 0.5 * shadowFactor * GetSunLightColor() + 
                    0.7 * shadowFactor * mix(GetAmbientShadowColor(), GetSunLightColor(), max(0.0, dot(volumeNormal, GetSunLightDirection())));
                accumulatedColor += vec3(whiteWaterFactor) * (
                    diffuse +
                    shadowFactor * Specular(reflection, GetSunLightDirection(), 30.0) * GetSunLightColor() +
            		GetAmbientSkyColor());
            }
            #endif
            accumulatedColorMultiplier *= (1.0 - whiteWaterFactor);
            rayDirection = refract(rayDirection, volumeNormal, AIR_IOR / WaterIor);
            
            accumulatedColor += accumulatedColorMultiplier * Specular(reflection, GetSunLightDirection(), waterShininess) * GetSunLightColor();
            accumulatedColor += accumulatedColorMultiplier * fresnelFactor * GetSkyColor(reflection);
            accumulatedColorMultiplier *= (1.0 - fresnelFactor);
            
            // recalculate opaque depth now that the ray has been refracted
            t = IntersectOpaqueScene(rayOrigin, rayDirection, materialID);
            if( materialID != INVALID_OBJECT_ID )
            {
                opaquePosition = rayOrigin + t*rayDirection;
            }

            float volumeDepth = 0.0f;
            float signedDistance = 0.0;
            int i = 0;
            vec3 marchPosition = vec3(0);
            float minStepSize = SDF_START_STEP_SIZE;
            float minStepIncrement = float(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / float(MAX_VOLUME_MARCH_STEPS);
            for(; i < MAX_VOLUME_MARCH_STEPS; i++)
            {
                float marchSize = max(minStepSize, signedDistance);
				minStepSize += minStepIncrement;
                
                vec3 nextMarchPosition = rayOrigin + (volumeDepth + marchSize) * rayDirection;
                signedDistance = QueryOceanDistanceField(nextMarchPosition, iTime);
                if(signedDistance > 0.0f)
                {
                    float start = 0.0;
                    float end = marchSize;

                    for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
                    {
                        float midPoint = (start + end) * 0.5;
                        vec3 nextMarchPosition = rayOrigin + (volumeDepth + midPoint) * rayDirection;
                        float sdfValue = QueryVolumetricDistanceField(nextMarchPosition, iTime);

                        // Use the SDF to nudget the mid point closer to the actual edge
                        midPoint = clamp(midPoint - sdfValue, start, end);
                        if(sdfValue > 0.0)
                        {
                            end = midPoint;
                        }
                        else
                        {
                            start = midPoint;
                        }
                    }
                    marchSize = end;
                }

                volumeDepth += marchSize;
                marchPosition = rayOrigin + volumeDepth*rayDirection;

                if(volumeDepth > t)
                {
                    intersectFound = true;
                    volumeDepth = min(volumeDepth, t);
                    break;
                }

                vec3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(vec3(WaterAbsorption) / WaterColor, marchSize);
                vec3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;

                accumulatedColor += accumulatedColorMultiplier * WaterColor * absorptionFromMarch * 
                    GetSunLightColor() * GetApproximateShadowFactor(marchPosition, GetSunLightDirection());
                accumulatedColor += accumulatedColorMultiplier * absorptionFromMarch * GetAmbientSkyColor();

                if(signedDistance > 0.0)
                {
                    intersectFound = true;
                    outsideVolume = true;
                    break;
                }
            }

            if(intersectFound && outsideVolume)
            {
                // Flip the normal since we're coming from inside the volume
                vec3 exitNormal = -GetVolumeNormal(marchPosition, iTime, SCENE_TYPE_SIMPLIFIED_OCEAN);                    

                #if SECONDARY_REFLECTION
                float fresnelFactor = max(0.2, FresnelFactor(WaterIor, AIR_IOR, exitNormal, rayDirection));
                vec3 reflection = reflect(rayDirection, exitNormal);
                int reflectedMaterialID;
                float reflectionT = IntersectOpaqueScene(marchPosition, reflection, reflectedMaterialID);
                if( reflectedMaterialID != INVALID_OBJECT_ID )
                {
                    vec3 pos = marchPosition + reflection*reflectionT;
                    Material material = GetMaterial(reflectedMaterialID);
                    vec3 color = vec3(0);
                    CalculateLighting(pos,reflection, reflectedMaterialID, color, true);
                    accumulatedColor += accumulatedColorMultiplier * fresnelFactor * color;
                }
                else
                {
                    accumulatedColor += fresnelFactor * accumulatedColorMultiplier * GetSkyColor(rayDirection);

                }
                accumulatedColorMultiplier *= (1.0 - fresnelFactor);
                #endif
                
                rayDirection = refract(rayDirection, exitNormal, WaterIor / AIR_IOR);
                rayOrigin = marchPosition;
                t = IntersectOpaqueScene(marchPosition, rayDirection, materialID);
                if( materialID != INVALID_OBJECT_ID )
                {
                    opaquePosition = rayOrigin + t*rayDirection;
                }
                outsideVolume = true;
            }

            if(!intersectFound)
            {
                float t = GetApproximateIntersect(marchPosition, rayDirection);
                float halfT = t / 2.0;
                vec3 halfwayPosition = marchPosition + rayDirection * halfT;
                vec3 shadowFactor = GetApproximateShadowFactor(halfwayPosition, GetSunLightDirection());

                vec3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(WaterAbsorption / WaterColor, t);
                vec3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;
                accumulatedColor += accumulatedColorMultiplier * WaterColor * shadowFactor * absorptionFromMarch * GetSunLightColor();
                accumulatedColor += accumulatedColorMultiplier * WaterColor * GetAmbientSkyColor() * absorptionFromMarch;

                volumeDepth += t;
                rayOrigin = rayOrigin + volumeDepth*rayDirection;
            }
        }
    }
    
    vec3 opaqueColor = vec3(0.0f);
    if(materialID != INVALID_OBJECT_ID)
    {
        CalculateLighting(opaquePosition,
                          rayDirection,
                          materialID, opaqueColor,
                          false);
    }
    else
    {
        opaqueColor = GetSkyColor(rayDirection);
    }
    
    return accumulatedColor + accumulatedColorMultiplier * opaqueColor;
}

mat3 GetViewMatrix(float xRotationFactor)
{ 
   float xRotation = ((1.0 - xRotationFactor) - 0.5) * PI * 0.2;
   return mat3( cos(xRotation), 0.0, sin(xRotation),
                0.0,           1.0, 0.0,    
                -sin(xRotation),0.0, cos(xRotation));
}

float GetRotationFactor()
{
    return iMouse.x / iResolution.x;
}

bool IsInputThread(in vec2 fragCoord)
{
    return ALLOW_KEYBOARD_INPUT != 0 && int(fragCoord.x) == 0 && int(fragCoord.y) == 0;
}
   

bool KeyDown(int char)
{
    return int(texelFetch(iChannel1, ivec2(char, 0), 0).x) > 0;
}

void ProcessInput()
{
    const float WaterIorChangeRate = 0.35;
	if(KeyDown(87)) WaterIor += WaterIorChangeRate * iTimeDelta;
    if(KeyDown(83)) WaterIor -= WaterIorChangeRate * iTimeDelta;
    WaterIor = clamp(WaterIor, 1.0, 1.8);
    
    const float WaterTurbulanceChangeRate = 7.0;
	if(KeyDown(69)) WaterTurbulence += WaterTurbulanceChangeRate * iTimeDelta;
    if(KeyDown(68)) WaterTurbulence -= WaterTurbulanceChangeRate * iTimeDelta;
    WaterTurbulence = clamp(WaterTurbulence, 0.0, 50.0);
       
    const float WaterAbsorptionChangeRate = 0.03;
	if(KeyDown(81)) WaterAbsorption += WaterAbsorptionChangeRate * iTimeDelta;
    if(KeyDown(65)) WaterAbsorption -= WaterAbsorptionChangeRate * iTimeDelta;
    WaterAbsorption = clamp(WaterAbsorption, 0.0, 1.0);
    
    const float ColorChangeRate = 0.5;
	if(KeyDown(89)) WaterColor.r += ColorChangeRate * iTimeDelta;
    if(KeyDown(72)) WaterColor.r -= ColorChangeRate * iTimeDelta;
    
    if(KeyDown(85)) WaterColor.g += ColorChangeRate * iTimeDelta;
    if(KeyDown(74)) WaterColor.g -= ColorChangeRate * iTimeDelta;
    
    if(KeyDown(73)) WaterColor.b += ColorChangeRate * iTimeDelta;
    if(KeyDown(75)) WaterColor.b -= ColorChangeRate * iTimeDelta;
    
    WaterColor = clamp(WaterColor, 0.05, 0.99);
}

float EncodeWaterColor()
{
    return float(
        int(WaterColor.r * 64.0) + 
        (int(WaterColor.g * 64.0) << 6) +
        (int(WaterColor.b * 64.0) << 12)); 
}

void DecodeWaterColor(float data)
{
    WaterColor.r = float(int(data) & 63) / 64.0;
    WaterColor.g = float((int(data) >> 6) & 63) / 64.0;
	WaterColor.b = float((int(data) >> 12) & 63) / 64.0;
}

void LoadConstants()
{
    if(iFrame == 0 || KeyDown(82) || ALLOW_KEYBOARD_INPUT == 0)
    {
        WaterColor = vec3(0.1, 0.82, 1.0);
        WaterIor = 1.33; // Actual IOR of water
        WaterTurbulence = 2.5;
        WaterAbsorption = 0.028;
    }
    else
    {
        vec4 data = texelFetch(iChannel0, ivec2(0, 0), 0);
        WaterIor = data.r;
        WaterTurbulence = data.g;
        WaterAbsorption = data.b;
        DecodeWaterColor(data.a);
    }
}
     
vec3 GammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0/2.2));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    LoadConstants();
    
    // Sacrafice 1 pixel so that we can save up updates from input
    if(IsInputThread(fragCoord))
    {
        ProcessInput();
        fragColor = vec4(WaterIor, WaterTurbulence, WaterAbsorption, EncodeWaterColor());
        return;
    }
    
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    float aspectRatio = iResolution.x /  iResolution.y; 
    float lensWidth = Camera.LensHeight * aspectRatio;
    
    vec3 NonNormalizedCameraView = Camera.LookAt - Camera.Position;
    float ViewLength = length(NonNormalizedCameraView);
    vec3 CameraView = NonNormalizedCameraView / ViewLength;

    vec3 lensPoint = Camera.Position;
    
    // Pivot the camera around the look at point
    float rotationFactor = GetRotationFactor();
    mat3 viewMatrix = GetViewMatrix(rotationFactor);
    CameraView = CameraView * viewMatrix;
    lensPoint = Camera.LookAt - CameraView * ViewLength;
    
    vec3 CameraRight = cross(CameraView, vec3(0, 1, 0));    
    vec3 CameraUp = cross(CameraRight, CameraView);

    vec3 focalPoint = lensPoint - Camera.FocalDistance * CameraView;
    lensPoint += CameraRight * (uv.x * 2.0 - 1.0) * lensWidth / 2.0;
    lensPoint += CameraUp * (uv.y * 2.0 - 1.0) * Camera.LensHeight / 2.0;
    
    vec3 rayOrigin = focalPoint;
    vec3 rayDirection = normalize(lensPoint - focalPoint);
    
    vec3 color = Render(rayOrigin, rayDirection);
    fragColor=vec4(GammaCorrect(color), 1.0 );
}
