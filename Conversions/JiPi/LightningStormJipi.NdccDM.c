
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//Track mouse movement and resolution change between frames and set camera position.

#define PI 3.14159f
#define EPS 1e-4
#define CAMERA_DIST 65.0f

__KERNEL__ void LightningStormJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;
  
    //Work with just the first four pixels.
    if((fragCoord.x == 0.5f) && (fragCoord.y < 4.0f)){
        
        //float4 oldMouse = texelFetch(iChannel0, ito_float2_s(0.5f), 0).xyzw;
        float4 oldMouse = texture(iChannel0, (make_float2((int)0.5f, (int)0.5f)+0.5f)/iResolution);
        float4 mouse = (iMouse / swi4(iResolution,x,y,x,y)); 
        float4 newMouse = to_float4_s(0);

        //float mouseDownLastFrame = texelFetch(iChannel0, to_int2(0.5f, 3.5f), 0).x;
        float mouseDownLastFrame = texture(iChannel0, (make_float2((int)0.5f,(int)3.5f)+0.5f)/iResolution).x;
        
        //If mouse button is down and was down last frame
        if(iMouse.z > 0.0f && mouseDownLastFrame > 0.0f){
            
            //Difference between mouse position last frame and now.
            float2 mouseMove = swi2(mouse,x,y)-swi2(oldMouse,z,w);
            newMouse = to_float4_f2f2(swi2(oldMouse,x,y) + to_float2(5.0f, 3.0f)*mouseMove, swi2(mouse,x,y));
        }else{
            newMouse = to_float4_f2f2(swi2(oldMouse,x,y), swi2(mouse,x,y));
        }
        newMouse.x = mod_f(newMouse.x, 2.0f*PI);
        newMouse.y = _fminf(0.99f, _fmaxf(-0.99f, newMouse.y));

        //Store mouse data in the first pixel of Buffer B.
        if(fragCoord.x == 0.5f && fragCoord.y == 0.5f){
            //Set value at first frames
            if(iFrame < 5){
                newMouse = to_float4(-0.0f, -0.05f, 0.0f, 0.0f);
            }
            fragColor = (newMouse);
        }

        //Store camera position in the second pixel of Buffer B.
        if(fragCoord.x == 0.5f && fragCoord.y == 1.5f){
            //Set camera position from mouse information.
            float3 cameraPos = CAMERA_DIST * to_float3(_sinf(newMouse.x), -_sinf(newMouse.y), -_cosf(newMouse.x));
            fragColor = to_float4_aw(cameraPos, 1.0f);
        }
        
        //Store resolution change data in the third pixel of Buffer B.
        if(fragCoord.x == 0.5f && fragCoord.y == 2.5f){
            float resolutionChangeFlag = 0.0f;
            //The resolution last frame.
            //float2 oldResolution = texelFetch(iChannel0, to_int2(0.5f, 2.5f), 0).yz;
            float2 oldResolution = swi2(texture(iChannel0, (make_float2((int)0.5f,(int)2.5f)+0.5f)/iResolution),y,z);
            
            
            if(iResolution.x != oldResolution.x || iResolution.y != oldResolution.y){
              resolutionChangeFlag = 1.0f;
            }
            
          fragColor = to_float4(resolutionChangeFlag, iResolution.x, iResolution.y, 1.0f);
        }
           
        //Store whether the mouse button is down in the fourth pixel of Buffer A
        if(fragCoord.x == 0.5f && fragCoord.y == 3.5f){
            if(iMouse.z > 0.0f){
              fragColor = to_float4_aw(to_float3_s(1.0f), 1.0f);
            }else{
              fragColor = to_float4_aw(to_float3_s(0.0f), 1.0f);
            }
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


//  Ray marched lightning with a volumetric cloud.
//
//  The bolts are capped cylinders which are offset by Perlin noise FBM and animated in time 
//  to look like electrical discharges. The glow is achieved by accumulating distance based 
//  glow along view rays. Internal flashes are additional ambient terms in the cloud lighting.
//
//  See https://www.shadertoy.com/view/3sffzj for cloud.

__DEVICE__ const float3 sunLightColour = {1.0f,1.0f,1.0f};//to_float3_s(1.0f);
__DEVICE__ const float3 skyColour = {0.0f,0.0f,0.0f};//to_float3_s(0);
__DEVICE__ const float3 horizonColour = {1.0f, 0.9f, 0.8f};//to_float3(1, 0.9f, 0.8f);

//---------------------- Lightning ----------------------
__DEVICE__ const int MAX_STEPS = 32;
__DEVICE__ const float MIN_DIST = 0.1f;
__DEVICE__ const float MAX_DIST = 1000.0f;
#define EPSILON  1e-4




//The speed and duration of the bolts. The speed and frequency are linked
__DEVICE__ float speed = 0.25f;






//------------------------ Cloud ------------------------
__DEVICE__ const float CLOUD_START = 20.0f;
__DEVICE__ const float CLOUD_HEIGHT = 20.0f;
__DEVICE__ const float CLOUD_END = 40.0f;//CLOUD_START + CLOUD_HEIGHT;
//For size of AABB
__DEVICE__ const float CLOUD_EXTENT = 20.0f;

__DEVICE__ const int STEPS_PRIMARY = 24;
__DEVICE__ const int STEPS_LIGHT = 6;

//Offset the sample point by blue noise every frame to get rid of banding
#define DITHERING
#define goldenRatio  1.61803398875f

__DEVICE__ const float3 minCorner = {-20.0f,20.0f,-20.0f};//{-CLOUD_EXTENT, CLOUD_START, -CLOUD_EXTENT};//to_float3(-CLOUD_EXTENT, CLOUD_START, -CLOUD_EXTENT);
__DEVICE__ const float3 maxCorner = {20.0f,40.0f,20.0f};//{CLOUD_EXTENT, CLOUD_END, CLOUD_EXTENT};//to_float3(CLOUD_EXTENT, CLOUD_END, CLOUD_EXTENT);

__DEVICE__ const float power = 6.0f;
__DEVICE__ const float densityMultiplier = 6.5f;

__DEVICE__ float3 rayDirection(float fieldOfView, float2 fragCoord, float2 iResolution) {
    float2 xy = fragCoord - iResolution / 2.0f;
    float z = (0.5f * iResolution.y) / _tanf(radians(fieldOfView) / 2.0f);
    return normalize(to_float3_aw(xy, -z));
}

//https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/
__DEVICE__ mat3 lookAt(float3 camera, float3 targetDir, float3 up){
    float3 zaxis = normalize(targetDir);    
    float3 xaxis = normalize(cross(zaxis, up));
    float3 yaxis = cross(xaxis, zaxis);
    return to_mat3_f3(xaxis, yaxis, -zaxis);
}

//https://www.shadertoy.com/view/3s3GDn
__DEVICE__ float getGlow(float dist, float radius, float intensity){
    dist = _fmaxf(dist, 1e-6);
    return _powf(radius/dist, intensity);  
}

//---------------------------- 1D Perlin noise ----------------------------
//Used to shape lightning bolts
//https://www.shadertoy.com/view/lt3BWM

#define HASHSCALE 0.1031f

__DEVICE__ float hash(float p){
    float3 p3  = fract_f3(to_float3_s(p) * HASHSCALE);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float fade(float t) { return t*t*t*(t*(6.0f*t-15.0f)+10.0f); }

__DEVICE__ float grad(float hash, float p){
    int i = (int)(1e4*hash);
    return (i & 1) == 0 ? p : -p;
}

__DEVICE__ float perlinNoise1D(float p){
    float pi = _floor(p), pf = p - pi, w = fade(pf);
    return _mix(grad(hash(pi), pf), grad(hash(pi + 1.0f), pf - 1.0f), w) * 2.0f;
}

//---------------------------- 3D Perlin noise ----------------------------
//Used to shape cloud

//https://www.shadertoy.com/view/4djSRW
__DEVICE__ float3 hash33(float3 p3){
    p3 = fract_f3(p3 * to_float3(0.1031f,0.11369f,0.13787f));
    p3 += dot(p3, swi3(p3,y,x,z)+19.19f);
    return -1.0f + 2.0f * fract_f3(to_float3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

__DEVICE__ float perlinNoise3D(float3 p){
    float3 pi = _floor(p);
    float3 pf = p - pi;

    float3 w = pf * pf * (3.0f - 2.0f * pf);

    return   _mix(
        _mix(
            _mix(dot(pf - to_float3(0, 0, 0), hash33(pi + to_float3(0, 0, 0))), 
                 dot(pf - to_float3(1, 0, 0), hash33(pi + to_float3(1, 0, 0))),
                 w.x),
            _mix(dot(pf - to_float3(0, 0, 1), hash33(pi + to_float3(0, 0, 1))), 
                 dot(pf - to_float3(1, 0, 1), hash33(pi + to_float3(1, 0, 1))),
                 w.x),
            w.z),
        _mix(
            _mix(dot(pf - to_float3(0, 1, 0), hash33(pi + to_float3(0, 1, 0))), 
                 dot(pf - to_float3(1, 1, 0), hash33(pi + to_float3(1, 1, 0))),
                 w.x),
            _mix(dot(pf - to_float3(0, 1, 1), hash33(pi + to_float3(0, 1, 1))), 
                 dot(pf - to_float3(1, 1, 1), hash33(pi + to_float3(1, 1, 1))),
                 w.x),
            w.z),
        w.y);
}

//---------------------------- Distance ---------------------------

__DEVICE__ bool intersectPlane(float3 n, float3 p, float3 org, float3 dir, out float *t){ 
    //Assuming vectors are all normalized
    float denom = dot(n, dir); 
    if(denom > 1e-6) { 
        *t = dot(p - org, n) / denom; 
        return (*t >= 0.0f); 
    } 
 
    return false; 
}

__DEVICE__ float sdCappedCylinder( float3 p, float h, float r ){
    float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - to_float2(h,r);
    return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}

__DEVICE__ float fbm(float pos, int octaves, float iTime){
    if(pos < 0.0f){
        return 0.0f;
    }
    float total = 0.0f;
    float frequency = 0.2f;
    float amplitude = 1.0f;
    for(int i = 0; i < octaves; i++){
        if(i > 2){
            pos += 0.5f*iTime * 25.0f;
        }
        total += perlinNoise1D(pos * frequency) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return total;
}

__DEVICE__ float getSDF(float3 p, float iTime, float strikeFrequency, float2 bolt[3], float speed) {

    float dist = 1e10;

    //Shift everything to start at the cloud
    p.y -= CLOUD_START;

    //The counter of a bolt in a series
    float t = 0.0f;
    //The offset of the series
    float shift = 0.0f;

    //Number of noise levels for FBM
    int octaves = 4;
    //Scale of the y coordinate as noise input. Controls the smoothness of the bolt
    float scale = 0.5f;
    //Offset to give simultaneous bolts different shapes
    float shapeOffset = 15.2f;
    //Fraction of the total bolt length 0->1
    float progress;

    //The fraction of the lifetime of the bolt it takes for it to descend.
    //The bolt persists in full form for 1.0f-descentDuration fraction of the total period.
    float descentDuration = 0.5f;

    //Spatial range of the bolt
    float range = CLOUD_EXTENT*0.4f;
    float boltLength = CLOUD_START*0.5f;
    //Bolt thickness
    float radius = 0.01f;
    //xz: the shape of the bolt
    //y:  progress used as bolt length and positioning
    float3 offset;
    float2 location;

    float time;

    for(int i = 0; i < 3; i++){

        shapeOffset *= 2.0f;
        shift = fract(shift + 0.25f);
        time = iTime * 25.0f * speed + shift;
        t = _floor(time)+1.0f;
        
        //Reset the position of the iteration bolt
        if(i == 0){
          bolt[0] = to_float2_s(1e10);
        }
        if(i == 1){
          bolt[1] = to_float2_s(1e10);
        }
        if(i == 2){
          bolt[2] = to_float2_s(1e10);
        }

        //Bolts strike randomly
        if(hash((float)(i)+t*0.026f) > strikeFrequency){
            continue;
        }
        location = 2.0f*to_float2(hash(t+(float)(i)+0.43f), hash(t+(float)(i)+0.3f))-1.0f;
        location *= range;
        progress = clamp(fract(time)/descentDuration, 0.0f, 1.0f);
        
        //Briefly increase the radius of the bolt the moment it makes contact
        if(progress > 0.95f && fract(time) - descentDuration < 0.1f){
            radius = 0.1f;
        }else{
            radius = 0.01f;
        }
        progress *= boltLength;
        offset = to_float3(location.x+fbm(shapeOffset+t*0.2f+(scale*p.y), octaves, iTime), 
                           progress, 
                           location.y+fbm(shapeOffset+t*0.12f-(scale*p.y), octaves, iTime));
        
        //Store the xz location of the iteration bolt
        //Raymarching translations are reversed so invert the sign
        if(i == 0){
          bolt[0] = -1.0f*swi2(location,x,y);
        }
        if(i == 1){
          bolt[1] = -1.0f*swi2(location,x,y);
        }
        if(i == 2){
          bolt[2] = -1.0f*swi2(location,x,y);
        }
        dist = _fminf(dist, sdCappedCylinder(p+offset, radius, progress));
    }

    return dist;
}

__DEVICE__ float distanceToScene(float3 cameraPos, float3 rayDir, float start, float end, out float3 *glow, float iTime, float3 boltColour, 
                                 float strikeFrequency, float2 bolt[3], float speed) {

    float depth = start;
    float dist;

    for (int i = 0; i < MAX_STEPS; i++) {

        float3 p = cameraPos + depth * rayDir;
        //Warping the cylinder breaks the shape. Reduce step size to avoid this.
        dist = 0.5f*getSDF(p, iTime, strikeFrequency, bolt, speed);
        //Accumulate the glow along the view ray.
        *glow += getGlow(dist, 0.01f, 0.8f) * boltColour;

        if (dist < EPSILON){
            return depth;
        }

        depth += dist;

        if (depth >= end){ 
            return end; 
        }
    }

    return end;
}

__DEVICE__ float3 getSkyColour(float3 rayDir){
    if(rayDir.y < 0.0f){
        return to_float3_s(0.025f);
    }

    return _mix(horizonColour, skyColour, _powf(rayDir.y, 0.03f));
}


//---------------------------- Cloud shape ----------------------------

//https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
//Compute the near and far intersections using the slab method.
//No intersection if tNear > tFar.
__DEVICE__ float2 intersectAABB(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax) {
    float3 tMin = (boxMin - rayOrigin) / rayDir;
    float3 tMax = (boxMax - rayOrigin) / rayDir;
    float3 t1 = _fminf(tMin, tMax);
    float3 t2 = _fmaxf(tMin, tMax);
    float tNear = _fmaxf(max(t1.x, t1.y), t1.z);
    float tFar = _fminf(min(t2.x, t2.y), t2.z);
    return to_float2(tNear, tFar);
}

__DEVICE__ bool insideAABB(float3 p){
    float eps = 1e-4;
    return  (p.x > minCorner.x-eps) && (p.y > minCorner.y-eps) && (p.z > minCorner.z-eps) && 
            (p.x < maxCorner.x+eps) && (p.y < maxCorner.y+eps) && (p.z < maxCorner.z+eps);
}

__DEVICE__ bool getCloudIntersection(float3 org, float3 dir, out float *distToStart, out float *totalDistance){
    float2 intersections = intersectAABB(org, dir, minCorner, maxCorner);

    if(insideAABB(org)){
        intersections.x = 1e-4;
    }

    *distToStart = intersections.x;
    *totalDistance = intersections.y - intersections.x;
    return intersections.x > 0.0f && (intersections.x < intersections.y);
}

__DEVICE__ float _saturatef(float x){
    return clamp(x, 0.0f, 1.0f);
}

__DEVICE__ float remap(float x, float low1, float high1, float low2, float high2){
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

__DEVICE__ float getNoise(float3 pos, float speed, float iTime){
    return 0.5f+0.5f*(perlinNoise3D(speed*iTime * 25.0f+pos));
}

__DEVICE__ float clouds(float3 p, out float *cloudHeight, float iTime){
      
  //Model an anvil cloud with two flipped hemispheres
    
    *cloudHeight = _saturatef((p.y - CLOUD_START)/(CLOUD_END-CLOUD_START));

    float bottom = 1.0f-_saturatef(length(swi2(p,x,z))/(1.25f*CLOUD_EXTENT));
    
    //Round top and bottom edges
    bottom *= _saturatef(remap(*cloudHeight, 0.25f*bottom, 1.0f, 1.0f, 0.0f)) 
          * _saturatef(remap(*cloudHeight, 0.0f, 0.175f, 0.45f, 1.0f));

    //Subtract coarse noise
    bottom = _saturatef(remap(bottom, 0.5f*getNoise(0.25f*p, 0.05f,iTime), 1.0f, 0.0f, 1.0f));
    //Subtract fine noise
    bottom = _saturatef(remap(bottom, 0.15f*getNoise(1.0f*p, 0.2f,iTime), 1.0f, 0.0f, 1.0f));

    float top = 1.0f-_saturatef(length(swi2(p,x,z))/(1.5f*CLOUD_EXTENT));
    
    //Round top and bottom edges
    top *= _saturatef(remap(1.0f- *cloudHeight, 0.25f*top, 1.0f, 1.0f, 0.0f)) 
        * _saturatef(remap(1.0f- *cloudHeight, 0.0f, 0.175f, 0.45f, 1.0f));
    
    //Subtract coarse noise
    top = _saturatef(remap(top, 0.5f*getNoise(0.25f*p, 0.05f,iTime), 1.0f, 0.0f, 1.0f));
    //Subtract fine noise
    top = _saturatef(remap(top, 0.15f*getNoise(1.0f*p, 0.2f,iTime), 1.0f, 0.0f, 1.0f));

    return (bottom+top)*densityMultiplier;
}

//---------------------------- Cloud lighting ----------------------------

__DEVICE__ float HenyeyGreenstein(float g, float costh){
    return (1.0f/(4.0f * 3.1415f))  * ((1.0f - g * g) / _powf(1.0f + g*g - 2.0f*g*costh, 1.5f));
}

//Get the amount of light that reaches a sample point.
__DEVICE__ float lightRay(float3 org, float3 p, float phaseFunction, float mu, float3 sunDirection, float iTime){
    float lightRayDistance = CLOUD_EXTENT*0.75f;
    float distToStart = 0.0f;
float lllllllllllllllllllll;
    getCloudIntersection(p, sunDirection, &distToStart, &lightRayDistance);

    float stepL = lightRayDistance/float(STEPS_LIGHT);

    float lightRayDensity = 0.0f;

    float cloudHeight = 0.0f;

    //Collect total density along light ray.
    for(int j = 0; j < STEPS_LIGHT; j++){
        //Reduce density of clouds when looking towards the sun for more luminous clouds.
        lightRayDensity += _mix(1.0f, 0.75f, mu) * 
            clouds(p + sunDirection * (float)(j) * stepL, &cloudHeight, iTime);
    }

    //Multiple scattering approximation from Nubis presentation credited to Wrenninge et al. 
    //Introduce another weaker Beer-Lambert function.
    float beersLaw = _fmaxf(_expf(-stepL * lightRayDensity), 
                            _expf(-stepL * lightRayDensity * 0.2f) * 0.75f);

    //Return product of Beer's law and powder effect depending on the 
    //view direction angle with the light direction.
    return _mix(beersLaw * 2.0f * (1.0f-(_expf(-stepL*lightRayDensity*2.0f))), beersLaw, mu);
}

__DEVICE__ float3 hash31(float p){
   float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
   p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
   return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x)); 
}

//Get the colour along the main view ray.
__DEVICE__ float3 mainRay(float3 org, float3 dir, float3 sunDirection, 
                          out float *totalTransmittance, float mu, float3 sunLightColour, float offset,
                          inout float *d, float iTime, float3 boltColour, float2 bolt[3]){

    const float internalFrequency = 0.5f;
    //Movement speed of lightning inside the cloud
    const float internalSpeed = 0.5f;

    //Variable to track transmittance along view ray. 
    //Assume clear sky and attenuate light when encountering clouds.
    *totalTransmittance = 1.0f;

    //Default to black.
    float3 colour = to_float3_s(0.0f);

    //The distance at which to start ray marching.
    float distToStart = 0.0f;

    //The length of the intersection.
    float totalDistance = 0.0f;

    //Determine if ray intersects bounding volume.
    //Set ray parameters in the cloud layer.
    bool renderClouds = getCloudIntersection(org, dir, &distToStart, &totalDistance);

    if(!renderClouds){
        return colour;
    }

    //Sampling step size.
    float stepS = totalDistance / (float)(STEPS_PRIMARY); 

    //Offset the starting point by blue noise.
    distToStart += stepS * offset;

    //Track distance to sample point.
    float dist = distToStart;

    //Initialise sampling point.
    float3 p = org + dist * dir;

    //Combine backward and forward scattering to have details in all directions.
    float phaseFunction = _mix(HenyeyGreenstein(-0.3f, mu), HenyeyGreenstein(0.3f, mu), 0.7f);

    float3 sunLight = sunLightColour * power;

    for(int i = 0; i < STEPS_PRIMARY; i++){

        //Normalised height for shaping and ambient lighting weighting.
        float cloudHeight;

        //Get density and cloud height at sample point
        float density = clouds(p, &cloudHeight, iTime);

        //Scattering and absorption coefficients.
        float sigmaS = 1.0f;
        float sigmaA = 0.0f;

        //Extinction coefficient.
        float sigmaE = sigmaS + sigmaA;

        float sampleSigmaS = sigmaS * density;
        float sampleSigmaE = sigmaE * density;

        //If there is a cloud at the sample point
        if(density > 0.0f ){
            //Store closest distance to the cloud
            *d = _fminf(*d, dist);
            
            //Internal lightning is additional ambient source that flickers and moves around
            //Get random position in the core of the cloud
            float3 source = to_float3(0, CLOUD_START + CLOUD_HEIGHT * 0.5f, 0) + 
                                     (2.0f*hash31(_floor(iTime * 25.0f*internalSpeed))-1.0f) * CLOUD_EXTENT * 0.25f;
            //Distance to the source position
            float prox = length(p - source);
            //Vary size for flicker
            float size = _sinf(45.0f*fract(iTime * 25.0f))+5.0f;
            //Get distance based glow
            float3 internal = getGlow(prox, size, 3.2f) * boltColour;
            //Internal lightning occurs randomly
            if(hash(_floor(iTime * 25.0f)) > internalFrequency){
              internal = to_float3_s(0);
            }
            
            //Add ambient source at bottom of cloud where lightning bolts exit
            size = 3.0f;
            float h = 0.9f*CLOUD_START;
            prox = length(p - to_float3(bolt[0].x, h, bolt[0].y));
            internal += getGlow(prox, size, 2.2f) * boltColour;
            
            prox = length(p - to_float3(bolt[1].x, h, bolt[1].y));
            internal += getGlow(prox, size, 2.2f) * boltColour;
            
            prox = length(p - to_float3(bolt[2].x, h, bolt[2].y));
            internal += getGlow(prox, size, 2.2f) * boltColour;
            
            //Combine lightning and height based ambient light
            float3 ambient = internal + sunLightColour * _mix((0.05f), (0.125f), cloudHeight);

            //Amount of sunlight that reaches the sample point through the cloud 
            //is the combination of ambient light and attenuated direct light.
            float3 luminance = ambient + sunLight * phaseFunction * 
                       lightRay(org, p, phaseFunction, mu, sunDirection, iTime);

            //Scale light contribution by density of the cloud.
            luminance *= sampleSigmaS;

            //Beer-Lambert.
            float transmittance = _expf(-sampleSigmaE * stepS);

            //Better energy conserving integration
            //"From Physically based sky, atmosphere and cloud rendering in Frostbite" 5.6
            //by Sebastian Hillaire.
            colour += *totalTransmittance * (luminance - luminance * transmittance) / sampleSigmaE; 

            //Attenuate the amount of light that reaches the camera.
            *totalTransmittance *= transmittance;  

            //If ray combined transmittance is close to 0, nothing beyond this sample 
            //point is visible, so break early.
            if(*totalTransmittance <= 0.01f){
                *totalTransmittance = 0.0f;
                break;
            }
        }

        dist += stepS;

        //Step along ray.
        p = org + dir * dist;
    }

    return colour;
}

//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
__DEVICE__ float3 ACESFilm(float3 x){
    return clamp((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f), 0.0f, 1.0f);
}

__KERNEL__ void LightningStormJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

float IIIIIIIIIIIIIIIIII;

    const float3 boltColour = to_float3(0.3f, 0.6f, 1.0f);

    const float strikeFrequency = 0.1;

    //Locations of the three bolts
    //float2 bolt0 = {1e10,1e10};//to_float2_s(1e10);
    //float2 bolt1 = {1e10,1e10};//to_float2_s(1e10);
    //float2 bolt2 = {1e10,1e10};//to_float2_s(1e10);

    float2 bolt[3] = {{1e10,1e10},{1e10,1e10},{1e10,1e10}};

    //The speed and duration of the bolts. The speed and frequency are linked
    float speed = 0.25f;

    //----------------- Define a camera -----------------

    float3 rayDir = rayDirection(40.0f, fragCoord, iResolution);

    //float3 cameraPos = texelFetch(iChannel0, to_int2(0.5f, 1.5f), 0).xyz;
    float3 cameraPos = swi3(texture(iChannel0, (make_float2((int)0.5f, (int)1.5f)+0.5f)/iResolution),x,y,z);

    float3 targetDir = to_float3(0,20,0) - cameraPos;

    float3 up = to_float3(0.0f, 1.0f, 0.0f);

    //Get the view matrix from the camera orientation
    mat3 viewMatrix = lookAt(cameraPos, targetDir, up);

    //Transform the ray to point in the correct direction
    rayDir = normalize(mul_mat3_f3(viewMatrix , rayDir));

    //---------------------------------------------------

    float offset = 0.0f;

    #ifdef DITHERING

        //From https://blog.demofox.org/2020/05/10/ray-marching-fog-with-blue-noise/
        //Get blue noise for the fragment.
        float blueNoise = texture(iChannel1, fragCoord / 1024.0f).x;
        offset = fract(blueNoise + (float)(iFrame%32) * goldenRatio);
    
    #endif
    
    //Lightning. Set bolt positions and accumulate glow.
    float3 glow = to_float3_s(0);
    float dist = distanceToScene(cameraPos, rayDir, MIN_DIST, MAX_DIST, &glow, iTime, boltColour, strikeFrequency, bolt, speed);
    
    //Cloud
    float totalTransmittance = 1.0f;
    float exposure = 0.5f;
    float3 sunDirection = normalize(to_float3_s(1));
    float mu = 0.5f+0.5f*dot(rayDir, sunDirection);
    //Distance to cloud.
    float d = 1e10;
    float3 colour = exposure * mainRay(cameraPos, rayDir, sunDirection, &totalTransmittance,
                                       mu, sunLightColour, offset, &d, iTime, boltColour, bolt); 

    float3 background = getSkyColour(rayDir);
    //Draw sun
    background += sunLightColour * 0.2f*getGlow(1.0f-mu, 0.001f, 0.55f);
    
    //Distance to plane at cloud bottom limit
    float t = 1e10;
    bool hitsPlane = intersectPlane(to_float3(0, -1, 0), to_float3(0, CLOUD_START, 0.0f), cameraPos,
                                    rayDir, &t);
    
    //t is distance to the plane below which lightning bolts occur, d is distance to the cloud
    //If lightning is behind cloud, add it to the background and draw the cloud in front.
    if(t >= d){
        background += glow;
    }

    colour += background * totalTransmittance;
    
    //If lightning is in front of the cloud, add it last. As the glow has no depth data, 
    //don't display it if above the bottom limit plane to avoid mixing error.
    if((t < d && hitsPlane) || cameraPos.y < CLOUD_START){
        colour += glow;
    }

    //Tonemapping
    colour = ACESFilm(colour);

    //Gamma correction 1.0f/2.2f = 0.4545...
    colour = pow_f3(colour, to_float3_s(0.4545f));
    
    // Output to screen
    fragColor = to_float4_aw(colour, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}