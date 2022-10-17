
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


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
	#define SDF_START_STEP_SIZE 1.5f
	#define SDF_END_STEP_SIZE 8.0f
	#define MAX_VOLUME_MARCH_STEPS 20
	#define BINARY_SEARCH_STEPS 6
	#define MAX_OPAQUE_SHADOW_MARCH_STEPS 10
	#define SHADOW_FACTOR_STEP_SIZE 7.0f
#endif

#define GROUND_LEVEL 0.0f
#define WATER_LEVEL 22.0f
#define PI 3.14f
#define LARGE_NUMBER 1e20f
#define EPSILON 0.0001f

#define NUM_SPHERES 5

#define SAND_FLOOR_OBJECT_ID 0
#define CORAL_ROCK_BASE_OBJECT_ID 1
#define NUM_OBJECTS (CORAL_ROCK_BASE_OBJECT_ID + NUM_SPHERES)

#define INVALID_OBJECT_ID (int)(-1)

#define AIR_IOR 1.0f

#define SKY_AMBIENT_MULTIPLIER 0.1f

#define MAX_WATER_DISPLACEMENT 15.0f
#define MIN_REFLECTION_COEFFECIENT 0.0f

#define SCENE_TYPE_OCEAN 1
#define SCENE_TYPE_SIMPLIFIED_OCEAN 2
#define SCENE_TYPE_OPAQUE 3



// --------------------------------------------//
//               Noise Functions
// --------------------------------------------//
// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
__DEVICE__ float hash1( float n )
{
    return fract( n*17.0f*fract( n*0.3183099f ) );
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
__DEVICE__ float noise( in float3 x )
{
    float3 p = _floor(x);
    float3 w = fract_f3(x);
    
    float3 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    
    float n = p.x + 317.0f*p.y + 157.0f*p.z;
    
    float a = hash1(n+0.0f);
    float b = hash1(n+1.0f);
    float c = hash1(n+317.0f);
    float d = hash1(n+318.0f);
    float e = hash1(n+157.0f);
    float f = hash1(n+158.0f);
    float g = hash1(n+474.0f);
    float h = hash1(n+475.0f);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0f+2.0f*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}



// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
__DEVICE__ float fbm( in float3 x, int iterations )
{
	const mat3 m3  = to_mat3( 0.00f,  0.80f,  0.60f,
                      -0.80f,  0.36f, -0.48f,
                      -0.60f, -0.48f,  0.64f );
	
    float f = 2.0f;
    float s = 0.5f;
    float a = 0.0f;
    float b = 0.5f;
    for( int i=0; i<iterations; i++ )
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*mul_mat3_f3(m3,x);
    }
  return a;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
__DEVICE__ float fbm_4( in float3 x )
{
    return fbm(x, 4);
}

// Taken from https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdPlane( float3 p )
{
  return p.y;
}

// Taken from https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdSmoothUnion( float d1, float d2, float k ) 
{
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); 
}

// Taken from https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); 
}

__DEVICE__ float3 sdTranslate(float3 pos, float3 translate)
{
  
    return pos - translate;
}

// Taken from https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdSphere( float3 p, float3 origin, float s )
{
  p = sdTranslate(p, origin);
  return length(p)-s;
}


struct Sphere
{
    float3 origin;
    float radius;
};
    
__DEVICE__ void GetSphere(int index, out float3 *origin, out float *radius)
{
    struct Sphere spheres[NUM_SPHERES];
    spheres[0].origin = to_float3(38, GROUND_LEVEL, 32);
	  spheres[0].radius =  12.0f;
	
    spheres[1].origin = to_float3(33, GROUND_LEVEL - 2.0f, 20);
	  spheres[1].radius =  8.5f;
	
    spheres[2].origin = to_float3(-25, GROUND_LEVEL - 32.0f, 55);
	  spheres[2].radius =  40.0f;
	
    spheres[3].origin = to_float3(-40, GROUND_LEVEL, 25);
	  spheres[3].radius =  12.0f;
	
    spheres[4].origin = to_float3(45, GROUND_LEVEL, 10);
	  spheres[4].radius =  12.0f;

    *origin = spheres[index].origin;
    *radius = spheres[index].radius;

}

__DEVICE__ float GetWaterWavesDisplacement(float3 position, float time)
{
    return 7.0f * _sinf(position.x / 15.0f + time * 1.3f) +
           6.0f * _cosf(position.z / 15.0f + time / 1.1f);
}

__DEVICE__ float GetWaterNoise(float3 position, float time, float WaterTurbulence)
{
    return WaterTurbulence * fbm_4(position / 15.0f + time / 3.0f);
}

__DEVICE__ float QueryOceanDistanceField( in float3 pos, float time, float WaterTurbulence)
{    
    return GetWaterWavesDisplacement(pos, time) + GetWaterNoise(pos, time, WaterTurbulence) + sdPlane(pos - to_float3(0, WATER_LEVEL, 0));
}

__DEVICE__ float QueryVolumetricDistanceField( in float3 pos, float time, float WaterTurbulence)
{    
    float minDist = QueryOceanDistanceField(pos, time, WaterTurbulence);
    minDist = sdSmoothSubtraction(sdSphere(pos, to_float3(0.0f, 20.0f, 0), 35.0f) +  5.0f * fbm_4(pos / to_float3(12, 20, 12)  - time / 5.0f), minDist, 12.0f);   
    minDist = sdSmoothUnion(minDist, sdPlane(pos - to_float3(0, GROUND_LEVEL - 1.0f, 0)), 13.0f);

    return minDist;
}

__DEVICE__ float IntersectVolumetric(in float3 rayOrigin, in float3 rayDirection, float maxT, float time, int sceneType, out bool *intersectFound, float WaterTurbulence)
{
    float t = 0.0f;
    float sdfValue = 0.0f;
    float stepSize = SDF_START_STEP_SIZE;
    float stepIncrement = (float)(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / (float)(MAX_SDF_SPHERE_STEPS);
    for(int i=0; i<MAX_SDF_SPHERE_STEPS; i++ )
    {
      sdfValue = sceneType == SCENE_TYPE_OCEAN ?
            QueryVolumetricDistanceField(rayOrigin+rayDirection*t, time, WaterTurbulence) :
            QueryOceanDistanceField(rayOrigin+rayDirection*t, time, WaterTurbulence);
            stepSize += stepIncrement;
        
        if( sdfValue < 0.0f || t>maxT ) break;
        t += _fmaxf(sdfValue, stepSize);
    }
float xxxxxxxxxxxxxxxxxx;    
    if(sdfValue < 0.0f)
    {
        float start = 0.0f;
        float end = stepSize;
        t -= stepSize;
        
        for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
        {
            float midPoint = (start + end) * 0.5f;
            float3 nextMarchPosition = rayOrigin + (t + midPoint) * rayDirection;
            float sdfValue = (sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(nextMarchPosition, time, WaterTurbulence) :
                QueryOceanDistanceField(nextMarchPosition, time, WaterTurbulence);
            
            // Use the SDF to nudget the mid point closer to the actual edge
            midPoint = clamp(midPoint + sdfValue, start, end);
            if(sdfValue < 0.0f)
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
    
    *intersectFound = t<maxT && sdfValue < 0.0f;
    return t;
}

// Taken from https://iquilezles.org/articles/normalsSDF
__DEVICE__ float3 GetVolumeNormal( in float3 pos, float time, int sceneType, float WaterTurbulence )
{
    float3 n = to_float3_s(0.0f);
    //for( int i=_fminf(0, iFrame); i<4; i++ )
		for( int i=0; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*
            ((sceneType == SCENE_TYPE_OCEAN) ?
                QueryVolumetricDistanceField(pos+0.5f*e, time, WaterTurbulence) :
                QueryOceanDistanceField(pos+0.5f*e, time, WaterTurbulence));
    }
    return normalize(n);
}

struct CameraDescription
{
    float3 Position;
    float3 LookAt;    

    float LensHeight;
    float FocalDistance;
};
    


struct Material
{
    float3 albedo;
    float shininess;
};

__DEVICE__ struct Material GetMaterial(int objectID)
{
	//separate Returnvariable  bauen ?
	
	struct Material ret;
	
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
		ret.albedo = 0.9f * to_float3(1.0f, 1.0f, 0.8f);
		ret.shininess = 50.0f;
        //return Material(0.9f * to_float3(1.0f, 1.0f, 0.8f), 50.0f);
		return ret;
    }
    else // if(objectID >= CORAL_ROCK_BASE_OBJECT_ID) // it's coral
    {
		ret.albedo = 0.9f * to_float3(0.3f, 0.4f, 0.2f);
		ret.shininess = 3.0f;
        //return Material(to_float3(0.3f, 0.4f, 0.2f), 3.0f);
		return ret;
    }
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
__DEVICE__ float PlaneIntersection(float3 rayOrigin, float3 rayDirection, float3 planeOrigin, float3 planeNormal) 
{ 
    float t = -1.0f;
    float denom = dot(-planeNormal, rayDirection); 
    if (denom > EPSILON) { 
        float3 rayToPlane = planeOrigin - rayOrigin; 
        return dot(rayToPlane, -planeNormal) / denom; 
    } 
 
    return t; 
} 
    
__DEVICE__ float SphereIntersection(
    in float3 rayOrigin, 
    in float3 rayDirection, 
    in float3 sphereCenter, 
    in float sphereRadius)
{
      float3 eMinusC = rayOrigin - sphereCenter;
      float dDotD = dot(rayDirection, rayDirection);

      float discriminant = dot(rayDirection, (eMinusC)) * dot(rayDirection, (eMinusC))
         - dDotD * (dot(eMinusC, eMinusC) - sphereRadius * sphereRadius);

      if (discriminant < 0.0f) 
         return -1.0f;

      float firstIntersect = (dot(-rayDirection, eMinusC) - _sqrtf(discriminant))
             / dDotD;
      
      float t = firstIntersect;
      return t;
}


__DEVICE__ void UpdateIfIntersected(
    inout float *t,
    in float intersectionT, 
    in int intersectionObjectID,
    out int *objectID)
{    
    if(intersectionT > EPSILON && intersectionT < *t)
    {
        *objectID = intersectionObjectID;
        *t = intersectionT;
    }
}

__DEVICE__ float SandHeightMap(float3 position)
{
  float sandGrainNoise = 0.1f * fbm(position * 10.0f, 2);
  float sandDuneDisplacement = 0.7f * _sinf(10.0f * fbm_4(10.0f + position / 40.0f));
  return sandGrainNoise  + sandDuneDisplacement;
}

__DEVICE__ float QueryOpaqueDistanceField(float3 position, int objectID)
{
    if(objectID == SAND_FLOOR_OBJECT_ID)
    {
        return sdPlane(position) + SandHeightMap(position);
    }
    else
    {
        float3 origin;
        float radius;
        GetSphere(objectID - CORAL_ROCK_BASE_OBJECT_ID, &origin, &radius);
        return sdSphere(position, origin, radius) + fbm_4(position);
    }
}

// Taken from https://iquilezles.org/articles/normalsSDF
__DEVICE__ float3 GetOpaqueNormal( in float3 pos, int objectID )
{
    float3 n = to_float3_s(0.0f);
    for( int i=0; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*QueryOpaqueDistanceField(pos+0.5f*e, objectID);
    }
    return normalize(n);
}

__DEVICE__ float IntersectOpaqueScene(in float3 rayOrigin, in float3 rayDirection, out int *objectID)
{
    float intersectionT = LARGE_NUMBER;

    float t = LARGE_NUMBER;
    *objectID = INVALID_OBJECT_ID;

    for(int i = 0; i < NUM_SPHERES; i++)
    {
        float3 origin;
        float radius;
        GetSphere(i, &origin, &radius);
            UpdateIfIntersected(
            &t,
            SphereIntersection(rayOrigin, rayDirection, origin, radius),
            CORAL_ROCK_BASE_OBJECT_ID + i,
            objectID);
    }
    
    UpdateIfIntersected(
        &t,
        PlaneIntersection(rayOrigin, rayDirection, to_float3(0, GROUND_LEVEL, 0), to_float3(0, 1, 0)),
        SAND_FLOOR_OBJECT_ID,
        objectID);
    
    // Optimization for early-out on volume intersections
    UpdateIfIntersected(
        &t,
        PlaneIntersection(rayOrigin, rayDirection, to_float3(0, WATER_LEVEL + MAX_WATER_DISPLACEMENT, 0), to_float3(0, 1, 0)),
        INVALID_OBJECT_ID,
        objectID);

    return t;
}

__DEVICE__ float Specular(in float3 reflection, in float3 lightDirection, float shininess)
{
    return 0.05f * _powf(_fmaxf(0.0f, dot(reflection, lightDirection)), shininess);
}

__DEVICE__ float3 Diffuse(in float3 normal, in float3 lightVec, in float3 diffuse)
{
    float nDotL = dot(normal, lightVec);
    return clamp(nDotL * diffuse, 0.0f, 1.0f);
}

__DEVICE__ float3 BeerLambert(float3 absorption, float dist)
{
    return exp_f3(-absorption * dist);
}

__DEVICE__ float3 GetShadowFactor(in float3 rayOrigin, in float3 rayDirection, in int maxSteps, in float minMarchSize, float WaterTurbulence, float WaterAbsorption, float3 WaterColor, float iTime)
{
    float t = 0.0f;
    float3 shadowFactor = to_float3_s(1.0f);
    float signedDistance = 0.0f;
    bool enteredVolume = false;
    for(int i = 0; i < maxSteps; i++)
    {         
        float marchSize = _fmaxf(minMarchSize, _fabs(signedDistance));
        t += marchSize;

        float3 position = rayOrigin + t*rayDirection;

        signedDistance = QueryVolumetricDistanceField(position, iTime, WaterTurbulence);
        if(signedDistance < 0.0f)
        {
            // Soften the shadows towards the edges to simulate an area light
            float softEdgeMultiplier = _fminf(_fabs(signedDistance / 5.0f), 1.0f);
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

__DEVICE__ float GetApproximateIntersect(float3 position, float3 rayDirection)
{
    float distanceToPlane;
    
    // Special case for rays parallel to the ground plane to avoid divide
    // by zero issues
    if(_fabs(rayDirection.y) < 0.01f)
    {
        distanceToPlane = LARGE_NUMBER;
    }
    else if(position.y < GROUND_LEVEL || position.y > WATER_LEVEL)
    {
        distanceToPlane = 0.0f;
    }
    else if(rayDirection.y > 0.0f)
    {
    distanceToPlane = (WATER_LEVEL - position.y) / rayDirection.y;
        float3 intersectPosition = position + distanceToPlane * rayDirection;
        
        distanceToPlane = _fmaxf(0.0f, distanceToPlane);
    }
    else
    {
        distanceToPlane = (position.y - GROUND_LEVEL) / _fabs(rayDirection.y);
    }
    return distanceToPlane;
}

__DEVICE__ float3 GetApproximateShadowFactor(float3 position, float3 rayDirection, float WaterAbsorption, float3 WaterColor)
{
    float distanceToPlane = GetApproximateIntersect(position, rayDirection);
  return BeerLambert(WaterAbsorption / WaterColor, distanceToPlane);
}


__DEVICE__ float rand(float seed, float iTime) { return fract(_sinf(seed++ + iTime)*43758.5453123f); }

__DEVICE__ float smoothVoronoi( in float2 x )
{
    int2 p = to_int2_cfloat(_floor( x ));
    float2  f = fract_f2( x );

    float res = 0.0f;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        int2 b = to_int2( i, j );
		
        float2  r = to_float2( (float)b.x, (float)b.y ) - f + noise( to_float3((float)(p.x + b.x), (float)(p.y + b.y), 0.0f) );
        float d = length( r );

        res += _expf( -32.0f*d );
    }
    return -(1.0f/32.0f)*_logf( res );
}

__DEVICE__ float3 GetSunLightDirection()
{
    return normalize(to_float3(0.3f, 1.0f, 1.65f));
}

__DEVICE__ float3 GetSunLightColor()
{
    return 0.9f * to_float3(0.9f, 0.75f, 0.7f);
}

__DEVICE__ float3 GetBaseSkyColor(float3 rayDirection)
{
  return _mix(
        to_float3(0.2f, 0.5f, 0.8f),
        to_float3(0.7f, 0.75f, 0.9f),
         _fmaxf(rayDirection.y, 0.0f));
}

__DEVICE__ float3 GetAmbientSkyColor()
{
    return SKY_AMBIENT_MULTIPLIER * GetBaseSkyColor(to_float3(0, 1, 0));
}

__DEVICE__ float3 GetAmbientShadowColor()
{
    return to_float3(0, 0, 0.2f);
}

__DEVICE__ float GetCloudDenity(float3 position, float iTime)
{
    float time = iTime * 0.25f;
    float3 noisePosition = position + to_float3(0.0f, 0.0f, time);
    float noise = fbm_4(noisePosition);
    float noiseCutoff = -0.3f;
    return _fmaxf(0.0f, 3.0f * (noise - noiseCutoff));
}

__DEVICE__ float4 GetCloudColor(float3 position, float iTime)
{
    float cloudDensity = GetCloudDenity(position, iTime);
    float3 cloudAlbedo = to_float3(1, 1, 1);
    float cloudAbsorption = 0.6f;
    float marchSize = 0.25f;

    float3 lightFactor = to_float3(1, 1, 1);
    {
        float3 marchPosition = position;
        int selfShadowSteps = 4;
        for(int i = 0; i < selfShadowSteps; i++)
        {
            marchPosition += GetSunLightDirection() * marchSize;
            float density = cloudAbsorption * GetCloudDenity(marchPosition,iTime);
            lightFactor *= BeerLambert(to_float3(density, density, density), marchSize);
        }
    }

    return to_float4_aw(
        cloudAlbedo * 
          ( mix_f3(GetAmbientShadowColor(), 1.3f * GetSunLightColor(), lightFactor) +
             GetAmbientSkyColor()), 
            _fminf(cloudDensity, 1.0f));
}

__DEVICE__ float3 GetSkyColor(in float3 rayDirection, float iTime)
{
    float3 skyColor = GetBaseSkyColor(rayDirection);
    float4 cloudColor = GetCloudColor(rayDirection * 4.0f,iTime);
    skyColor = _mix(skyColor, swi3(cloudColor,x,y,z), cloudColor.w);

    return skyColor;
}

__DEVICE__ float FresnelFactor(
    float CurrentIOR,
    float NewIOR,
    float3 Normal,
    float3 RayDirection)
{
    float ReflectionCoefficient = 
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR)) *
        ((CurrentIOR - NewIOR) / (CurrentIOR + NewIOR));
    return 
        clamp(ReflectionCoefficient + (1.0f - ReflectionCoefficient) * _powf(1.0f - dot(Normal, -RayDirection), 5.0f), MIN_REFLECTION_COEFFECIENT, 1.0f); 
}

__DEVICE__ float3 SandParallaxOcclusionMapping(float3 position, float3 view)
{
    int pomCount = 6;
    float marchSize = 0.3f;
    for(int i = 0; i < pomCount; i++)
    {
        if(position.y < GROUND_LEVEL -  SandHeightMap(position)) break;
        position += view * marchSize;
    }
    return position;
}

__DEVICE__ void CalculateLighting(float3 position, float3 view, int objectID, inout float3 *color, bool useFastLighting, float iTime, float WaterTurbulence, float WaterAbsorption, float3 WaterColor, float WaterIor)
{   
    struct Material material = GetMaterial(objectID);
    float sdfValue = QueryVolumetricDistanceField(position, iTime, WaterTurbulence);
    bool bUnderWater = sdfValue < 0.0f;

    float wetnessFactor = 0.0f;
    if(objectID == SAND_FLOOR_OBJECT_ID && !useFastLighting)
    {
        float wetSandDistance = 0.7f;
        if(sdfValue <= wetSandDistance)
        {
            // Darken the sand albedo to make it look wet
            float fadeEdge = 0.2f;
            wetnessFactor = 1.0f - _fmaxf(0.0f, (sdfValue - (wetSandDistance - fadeEdge)) / fadeEdge);
            material.albedo *= material.albedo * _mix(1.0f, 0.5f, wetnessFactor);
        }
        
        position = SandParallaxOcclusionMapping(position, view);
    }

    float3 normal = GetOpaqueNormal(position, objectID);
    float3 reflectionDirection = reflect(view, normal);
   
    int shadowObjectID = INVALID_OBJECT_ID;
    if(!useFastLighting)
    {
        IntersectOpaqueScene(position, GetSunLightDirection(), &shadowObjectID);
    }
    
    float3 shadowFactor = to_float3(0.0f, 0.0f, 0.0f);
    if(shadowObjectID == INVALID_OBJECT_ID)
    {
        float t;
        shadowFactor = useFastLighting ? 
                GetApproximateShadowFactor(position, GetSunLightDirection(), WaterAbsorption, WaterColor) :
                GetShadowFactor(position, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE, WaterTurbulence, WaterAbsorption, WaterColor, iTime);
        
        // Small back splash of the sky ambient color to fake a bit of gi
        *color += shadowFactor * material.albedo * _mix(0.4f * GetAmbientShadowColor(), GetSunLightColor(), _fmaxf(0.0f, dot(normal, GetSunLightDirection())));
        
        *color += shadowFactor * GetSunLightColor() * Specular(reflectionDirection, GetSunLightDirection(), material.shininess);
       
        if(!useFastLighting)
        {
            // Fake caustics
            float waterNoise = fract(GetWaterNoise(position, iTime, WaterTurbulence));
            float causticMultiplier = bUnderWater ? 7.0f : (1.0f - shadowFactor.x);
            *color += material.albedo * causticMultiplier * 0.027f *  _powf(
							smoothVoronoi(swi2(position,x,z) / 4.0f + 
							to_float2(iTime, iTime + 3.0f) + 
							3.0f * to_float2(_cosf(waterNoise), _sinf(waterNoise))), 5.0f);
        }
        
    }
    
    // Add a bit of reflection to wet sand to make it look like 
    // there's some water left over
    if(!useFastLighting && wetnessFactor > 0.0f)
    {
        // Water fills in the holes in the sand and generally
        // makes the surface planar so we can assume the normal is
        // pointing straight up
        float3 wetNormal = to_float3(0, 1, 0);
        float3 reflectionDirection = reflect(view, wetNormal);
        float fresnel = FresnelFactor(AIR_IOR, WaterIor, wetNormal, view);
        *color += shadowFactor * wetnessFactor * fresnel * GetSkyColor(reflectionDirection, iTime);
    }
    
    *color += GetAmbientSkyColor() * material.albedo;
}

__DEVICE__ float3 Render( in float3 rayOrigin, in float3 rayDirection, float WaterTurbulence, float WaterAbsorption, float3 WaterColor, float *WaterIor, float iTime)
{
    float3 accumulatedColor = to_float3_s(0.0f);
    float3 accumulatedColorMultiplier = to_float3_s(1.0f);
    
    int materialID = INVALID_OBJECT_ID;
    float t = IntersectOpaqueScene(rayOrigin, rayDirection, &materialID);
    float3 opaquePosition = rayOrigin + t*rayDirection;
    
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
                &intersectFound, WaterTurbulence);
        
        if(!intersectFound) break;
    else
        {
            outsideVolume = false;
            rayOrigin = rayOrigin + rayDirection * volumeStart;
            float3 volumeNormal = GetVolumeNormal(rayOrigin, iTime, SCENE_TYPE_OCEAN, WaterTurbulence);
            float3 reflection = reflect( rayDirection, volumeNormal);
            float fresnelFactor = FresnelFactor(AIR_IOR, *WaterIor, volumeNormal, rayDirection);
            float waterShininess = 100.0f;

            float whiteWaterFactor = 0.0f;
            float whiteWaterMaxHeight = 5.0f;
            float groundBlendFactor = _fminf(1.0f, (rayOrigin.y - GROUND_LEVEL) * 0.75f);
            #if ADD_WHITE_WATER
            if(firstEntry && rayOrigin.y <= whiteWaterMaxHeight)
            {
                *WaterIor = _mix(1.0f, *WaterIor, groundBlendFactor);
                
                float3 voronoisePosition = rayOrigin / 1.5f + to_float3(0, -iTime * 2.0f, _sinf(iTime));
                float noiseValue = _fabs(fbm(voronoisePosition, 2));
                voronoisePosition += 1.0f * to_float3(_cosf(noiseValue), 0.0f, _sinf(noiseValue));
                
                float heightLerp =  (whiteWaterMaxHeight - rayOrigin.y) / whiteWaterMaxHeight;
                whiteWaterFactor = _fabs(smoothVoronoi(swi2(voronoisePosition,x,z))) * heightLerp;
                whiteWaterFactor = clamp(whiteWaterFactor, 0.0f, 1.0f);
                whiteWaterFactor = _powf(whiteWaterFactor, 0.2f) * heightLerp;
                whiteWaterFactor *= _mix(_fabs(fbm(rayOrigin + to_float3(0, -iTime * 5.0f, 0), 2)), 1.0f, heightLerp);
                whiteWaterFactor *= groundBlendFactor;
                
                float3 shadowFactor =  GetShadowFactor(rayOrigin, GetSunLightDirection(), MAX_OPAQUE_SHADOW_MARCH_STEPS, SHADOW_FACTOR_STEP_SIZE, WaterTurbulence, WaterAbsorption, WaterColor, iTime);
                float3 diffuse = 0.5f * shadowFactor * GetSunLightColor() + 
                    0.7f * shadowFactor * _mix(GetAmbientShadowColor(), GetSunLightColor(), _fmaxf(0.0f, dot(volumeNormal, GetSunLightDirection())));
                accumulatedColor += to_float3_s(whiteWaterFactor) * (
                    diffuse +
                    shadowFactor * Specular(reflection, GetSunLightDirection(), 30.0f) * GetSunLightColor() +
                GetAmbientSkyColor());
            }
            #endif
            accumulatedColorMultiplier *= (1.0f - whiteWaterFactor);
            rayDirection = refract_f3(rayDirection, volumeNormal, AIR_IOR / *WaterIor);
            
            accumulatedColor += accumulatedColorMultiplier * Specular(reflection, GetSunLightDirection(), waterShininess) * GetSunLightColor();
            accumulatedColor += accumulatedColorMultiplier * fresnelFactor * GetSkyColor(reflection, iTime);
            accumulatedColorMultiplier *= (1.0f - fresnelFactor);
            
            // recalculate opaque depth now that the ray has been refracted
            t = IntersectOpaqueScene(rayOrigin, rayDirection, &materialID);
            if( materialID != INVALID_OBJECT_ID )
            {
                opaquePosition = rayOrigin + t*rayDirection;
            }

            float volumeDepth = 0.0f;
            float signedDistance = 0.0f;
            int i = 0;
            float3 marchPosition = to_float3_s(0);
            float minStepSize = SDF_START_STEP_SIZE;
            float minStepIncrement = (float)(SDF_END_STEP_SIZE - SDF_START_STEP_SIZE) / (float)(MAX_VOLUME_MARCH_STEPS);
            for(; i < MAX_VOLUME_MARCH_STEPS; i++)
            {
                float marchSize = _fmaxf(minStepSize, signedDistance);
                minStepSize += minStepIncrement;
                
                float3 nextMarchPosition = rayOrigin + (volumeDepth + marchSize) * rayDirection;
                signedDistance = QueryOceanDistanceField(nextMarchPosition, iTime, WaterTurbulence);
                if(signedDistance > 0.0f)
                {
                    float start = 0.0f;
                    float end = marchSize;

                    for(int j = 0; j < BINARY_SEARCH_STEPS; j++)
                    {
                        float midPoint = (start + end) * 0.5f;
                        float3 nextMarchPosition = rayOrigin + (volumeDepth + midPoint) * rayDirection;
                        float sdfValue = QueryVolumetricDistanceField(nextMarchPosition, iTime, WaterTurbulence);

                        // Use the SDF to nudget the mid point closer to the actual edge
                        midPoint = clamp(midPoint - sdfValue, start, end);
                        if(sdfValue > 0.0f)
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
                    volumeDepth = _fminf(volumeDepth, t);
                    break;
                }

                float3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(to_float3_s(WaterAbsorption) / WaterColor, marchSize);
                float3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;

                accumulatedColor += accumulatedColorMultiplier * WaterColor * absorptionFromMarch * 
                    GetSunLightColor() * GetApproximateShadowFactor(marchPosition, GetSunLightDirection(), WaterAbsorption, WaterColor);
                accumulatedColor += accumulatedColorMultiplier * absorptionFromMarch * GetAmbientSkyColor();

                if(signedDistance > 0.0f)
                {
                    intersectFound = true;
                    outsideVolume = true;
                    break;
                }
            }

            if(intersectFound && outsideVolume)
            {
                // Flip the normal since we're coming from inside the volume
                float3 exitNormal = -1.0f*GetVolumeNormal(marchPosition, iTime, SCENE_TYPE_SIMPLIFIED_OCEAN, WaterTurbulence);                    

                #if SECONDARY_REFLECTION
                float fresnelFactor = _fmaxf(0.2f, FresnelFactor(*WaterIor, AIR_IOR, exitNormal, rayDirection));
                float3 reflection = reflect(rayDirection, exitNormal);
                int reflectedMaterialID;
                float reflectionT = IntersectOpaqueScene(marchPosition, reflection, &reflectedMaterialID);
                if( reflectedMaterialID != INVALID_OBJECT_ID )
                {
                    float3 pos = marchPosition + reflection*reflectionT;
                    struct Material material = GetMaterial(reflectedMaterialID);
                    float3 color = to_float3_s(0);
					
					
                    CalculateLighting(pos,reflection, reflectedMaterialID, &color, true, iTime, WaterTurbulence, WaterAbsorption, WaterColor, *WaterIor);
                    accumulatedColor += accumulatedColorMultiplier * fresnelFactor * color;
                }
                else
                {
                    accumulatedColor += fresnelFactor * accumulatedColorMultiplier * GetSkyColor(rayDirection, iTime);

                }
                accumulatedColorMultiplier *= (1.0f - fresnelFactor);
                #endif
                
                rayDirection = refract_f3(rayDirection, exitNormal, *WaterIor / AIR_IOR);
                rayOrigin = marchPosition;
                t = IntersectOpaqueScene(marchPosition, rayDirection, &materialID);
                if( materialID != INVALID_OBJECT_ID )
                {
                    opaquePosition = rayOrigin + t*rayDirection;
                }
                outsideVolume = true;
            }

            if(!intersectFound)
            {
                float t = GetApproximateIntersect(marchPosition, rayDirection);
                float halfT = t / 2.0f;
                float3 halfwayPosition = marchPosition + rayDirection * halfT;
                float3 shadowFactor = GetApproximateShadowFactor(halfwayPosition, GetSunLightDirection(), WaterAbsorption, WaterColor);

                float3 previousLightFactor = accumulatedColorMultiplier;
                accumulatedColorMultiplier *= BeerLambert(WaterAbsorption / WaterColor, t);
                float3 absorptionFromMarch = previousLightFactor - accumulatedColorMultiplier;
                accumulatedColor += accumulatedColorMultiplier * WaterColor * shadowFactor * absorptionFromMarch * GetSunLightColor();
                accumulatedColor += accumulatedColorMultiplier * WaterColor * GetAmbientSkyColor() * absorptionFromMarch;

                volumeDepth += t;
                rayOrigin = rayOrigin + volumeDepth*rayDirection;
            }
        }
    }
    
    float3 opaqueColor = to_float3_s(0.0f);
    if(materialID != INVALID_OBJECT_ID)
    {
        CalculateLighting(opaquePosition,
                          rayDirection,
                          materialID, &opaqueColor,
                          false, iTime, WaterTurbulence, WaterAbsorption, WaterColor, *WaterIor);
    }
    else
    {
        opaqueColor = GetSkyColor(rayDirection, iTime);
    }
    
    return accumulatedColor + accumulatedColorMultiplier * opaqueColor;
}

__DEVICE__ mat3 GetViewMatrix(float xRotationFactor)
{ 
   float xRotation = ((1.0f - xRotationFactor) - 0.5f) * PI * 0.2f;
   return to_mat3( _cosf(xRotation), 0.0f, _sinf(xRotation),
                0.0f,           1.0f, 0.0f,    
                -_sinf(xRotation),0.0f, _cosf(xRotation));
}

__DEVICE__ float GetRotationFactor(float4 iMouse, float2 iResolution)
{
    return iMouse.x / iResolution.x;
}

__DEVICE__ bool IsInputThread(in float2 fragCoord)
{
    return ALLOW_KEYBOARD_INPUT != 0 && (int)(fragCoord.x) == 0 && (int)(fragCoord.y) == 0;
}
   
#ifdef  PROCESS
__DEVICE__ bool KeyDown(int char)
{
    return int(texelFetch(iChannel1, to_int2(char, 0), 0).x) > 0;
}

__DEVICE__ void ProcessInput(float *WaterIor, float *WaterTurbulence, float *WaterAbsorption, float3 *WaterColor)
{
    const float WaterIorChangeRate = 0.35f;
  if(KeyDown(87)) WaterIor += WaterIorChangeRate * iTimeDelta;
    if(KeyDown(83)) WaterIor -= WaterIorChangeRate * iTimeDelta;
    WaterIor = clamp(WaterIor, 1.0f, 1.8f);
    
    const float WaterTurbulanceChangeRate = 7.0f;
  if(KeyDown(69)) WaterTurbulence += WaterTurbulanceChangeRate * iTimeDelta;
    if(KeyDown(68)) WaterTurbulence -= WaterTurbulanceChangeRate * iTimeDelta;
    WaterTurbulence = clamp(WaterTurbulence, 0.0f, 50.0f);
       
    const float WaterAbsorptionChangeRate = 0.03f;
  if(KeyDown(81)) WaterAbsorption += WaterAbsorptionChangeRate * iTimeDelta;
    if(KeyDown(65)) WaterAbsorption -= WaterAbsorptionChangeRate * iTimeDelta;
    WaterAbsorption = clamp(WaterAbsorption, 0.0f, 1.0f);
    
    const float ColorChangeRate = 0.5f;
  if(KeyDown(89)) WaterColor.x += ColorChangeRate * iTimeDelta;
    if(KeyDown(72)) WaterColor.x -= ColorChangeRate * iTimeDelta;
    
    if(KeyDown(85)) WaterColor.y += ColorChangeRate * iTimeDelta;
    if(KeyDown(74)) WaterColor.y -= ColorChangeRate * iTimeDelta;
    
    if(KeyDown(73)) WaterColor.z += ColorChangeRate * iTimeDelta;
    if(KeyDown(75)) WaterColor.z -= ColorChangeRate * iTimeDelta;
    
    WaterColor = clamp(WaterColor, 0.05f, 0.99f);
}
#endif

__DEVICE__ float EncodeWaterColor(float3 WaterColor)
{
  float zzzzzzzzzzzzzzzzzz;
  
    return (float)(
         (int)(WaterColor.x * 64.0f) + 
        ((int)(WaterColor.y * 64.0f) << 6) +
        ((int)(WaterColor.z * 64.0f) << 12)); 
}

__DEVICE__ float3 DecodeWaterColor(float data, float3 WaterColor)
{
  WaterColor.x = (float)( (int)(data) & 63) / 64.0f;
  WaterColor.y = (float)(((int)(data) >> 6) & 63) / 64.0f;
  WaterColor.z = (float)(((int)(data) >> 12) & 63) / 64.0f;
  return WaterColor;
}

__DEVICE__ void LoadConstants(float *WaterIor, float *WaterTurbulence, float *WaterAbsorption, float3 *WaterColor, __TEXTURE2D__ iChannel0, int iFrame)
{
    if(iFrame == 0)// || KeyDown(82) || ALLOW_KEYBOARD_INPUT == 0)
    {
        *WaterColor = to_float3(0.1f, 0.82f, 1.0f);
        *WaterIor = 1.33f; // Actual IOR of water
        *WaterTurbulence = 2.5f;
        *WaterAbsorption = 0.028f;
    }
    else
    {
        //float4 data = texelFetch(iChannel0, to_int2(0, 0), 0);
		//xxxx=1;	
		float4 data = texture(iChannel0, to_float2(0, 0)+0.5);
        *WaterIor = data.x;
        *WaterTurbulence = data.y;
        *WaterAbsorption = data.z;
        *WaterColor = DecodeWaterColor(data.w, *WaterColor);
    }
}
     
__DEVICE__ float3 GammaCorrect(float3 color) 
{
    return pow_f3(color, to_float3_s(1.0f/2.2f));
}

__KERNEL__ void MoanaWaterJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
	
	CONNECT_COLOR0(WaterCOLOR, 0.1f, 0.82f, 1.0f, 1.0f);
  CONNECT_SLIDER0(WaterIOR, -1.0f, 1.0f, 0.0f);
	CONNECT_SLIDER1(WaterTURBULANCE, -1.0f, 50.0f, 0.0f);
	CONNECT_SLIDER2(WaterABSORPTION, -1.0f, 1.0f, 0.0f);
	
float IIIIIIIIIIIIIIIIIIIIIIII;	
	fragCoord+=0.5f;

	float WaterIor = 1.33f; // Actual IOR of water;
	float WaterTurbulence = 2.5f;;
	float WaterAbsorption = 0.028f;
	float3 WaterColor = swi3(WaterCOLOR,x,y,z);//to_float3(0.1f, 0.82f, 1.0f);
	
	float seed = 0.0f;
	
	WaterIor = clamp(WaterIor+WaterIOR, 1.0f, 1.8f);
    WaterTurbulence = clamp(WaterTurbulence+WaterTURBULANCE, 0.0f, 50.0f);
    WaterAbsorption = clamp(WaterAbsorption+WaterABSORPTION, 0.0f, 1.0f);
 
    WaterColor = clamp(WaterColor, 0.05f, 0.99f);
	
	struct CameraDescription Camera = {
										to_float3(0.0f, 10.0f, -20.0f),
										to_float3(0.0f, 10.0f, 0.0f),
										2.0f,
										1.6f
									  };

       
    float2 uv = fragCoord / iResolution;
    
    float aspectRatio = iResolution.x /  iResolution.y; 
    float lensWidth = Camera.LensHeight * aspectRatio;
    
    float3 NonNormalizedCameraView = Camera.LookAt - Camera.Position;
    float ViewLength = length(NonNormalizedCameraView);
    float3 CameraView = NonNormalizedCameraView / ViewLength;

    float3 lensPoint = Camera.Position;
    
    // Pivot the camera around the look at point
    float rotationFactor = GetRotationFactor(iMouse,iResolution);
    mat3 viewMatrix = GetViewMatrix(rotationFactor);
    CameraView = mul_f3_mat3(CameraView , viewMatrix);
    lensPoint = Camera.LookAt - CameraView * ViewLength;
    
    float3 CameraRight = cross(CameraView, to_float3(0, 1, 0));    
    float3 CameraUp = cross(CameraRight, CameraView);

    float3 focalPoint = lensPoint - Camera.FocalDistance * CameraView;
    lensPoint += CameraRight * (uv.x * 2.0f - 1.0f) * lensWidth / 2.0f;
    lensPoint += CameraUp * (uv.y * 2.0f - 1.0f) * Camera.LensHeight / 2.0f;
    
    float3 rayOrigin = focalPoint;
    float3 rayDirection = normalize(lensPoint - focalPoint);
    
    float3 color = Render(rayOrigin, rayDirection, WaterTurbulence, WaterAbsorption, WaterColor, &WaterIor, iTime);
    fragColor=to_float4_aw(GammaCorrect(color), 1.0f );


  SetFragmentShaderComputedColor(fragColor);
}
