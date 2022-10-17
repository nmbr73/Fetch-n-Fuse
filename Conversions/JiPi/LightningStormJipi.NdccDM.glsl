

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//	Ray marched lightning with a volumetric cloud.
//
//	The bolts are capped cylinders which are offset by Perlin noise FBM and animated in time 
//	to look like electrical discharges. The glow is achieved by accumulating distance based 
//	glow along view rays. Internal flashes are additional ambient terms in the cloud lighting.
//
//	See https://www.shadertoy.com/view/3sffzj for cloud.

const vec3 sunLightColour = vec3(1.0);
const vec3 skyColour = vec3(0);
const vec3 horizonColour = vec3(1, 0.9, 0.8);

//---------------------- Lightning ----------------------
const int MAX_STEPS = 32;
const float MIN_DIST = 0.1;
const float MAX_DIST = 1000.0;
const float EPSILON = 1e-4;

const vec3 boltColour = vec3(0.3, 0.6, 1.0);

const float strikeFrequency = 0.1;
//The speed and duration of the bolts. The speed and frequency are linked
float speed = 0.25;

const float internalFrequency = 0.5;
//Movement speed of lightning inside the cloud
const float internalSpeed = .5;

//Locations of the three bolts
vec2 bolt0 = vec2(1e10);
vec2 bolt1 = vec2(1e10);
vec2 bolt2 = vec2(1e10);

//------------------------ Cloud ------------------------
const float CLOUD_START = 20.0;
const float CLOUD_HEIGHT = 20.0;
const float CLOUD_END = CLOUD_START + CLOUD_HEIGHT;
//For size of AABB
const float CLOUD_EXTENT = 20.0;

const int STEPS_PRIMARY = 24;
const int STEPS_LIGHT = 6;

//Offset the sample point by blue noise every frame to get rid of banding
#define DITHERING
const float goldenRatio = 1.61803398875;

const vec3 minCorner = vec3(-CLOUD_EXTENT, CLOUD_START, -CLOUD_EXTENT);
const vec3 maxCorner = vec3(CLOUD_EXTENT, CLOUD_END, CLOUD_EXTENT);

const float power = 6.0;
const float densityMultiplier = 6.5;

vec3 rayDirection(float fieldOfView, vec2 fragCoord) {
    vec2 xy = fragCoord - iResolution.xy / 2.0;
    float z = (0.5 * iResolution.y) / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

//https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/
mat3 lookAt(vec3 camera, vec3 targetDir, vec3 up){
    vec3 zaxis = normalize(targetDir);    
    vec3 xaxis = normalize(cross(zaxis, up));
    vec3 yaxis = cross(xaxis, zaxis);

    return mat3(xaxis, yaxis, -zaxis);
}

//https://www.shadertoy.com/view/3s3GDn
float getGlow(float dist, float radius, float intensity){
    dist = max(dist, 1e-6);
    return pow(radius/dist, intensity);	
}

//---------------------------- 1D Perlin noise ----------------------------
//Used to shape lightning bolts
//https://www.shadertoy.com/view/lt3BWM

#define HASHSCALE 0.1031

float hash(float p){
    vec3 p3  = fract(vec3(p) * HASHSCALE);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float fade(float t) { return t*t*t*(t*(6.*t-15.)+10.); }

float grad(float hash, float p){
    int i = int(1e4*hash);
    return (i & 1) == 0 ? p : -p;
}

float perlinNoise1D(float p){
    float pi = floor(p), pf = p - pi, w = fade(pf);
    return mix(grad(hash(pi), pf), grad(hash(pi + 1.0), pf - 1.0), w) * 2.0;
}

//---------------------------- 3D Perlin noise ----------------------------
//Used to shape cloud

//https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3){
    p3 = fract(p3 * vec3(.1031,.11369,.13787));
    p3 += dot(p3, p3.yxz+19.19);
    return -1.0 + 2.0 * fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

float perlinNoise3D(vec3 p){
    vec3 pi = floor(p);
    vec3 pf = p - pi;

    vec3 w = pf * pf * (3.0 - 2.0 * pf);

    return 	mix(
        mix(
            mix(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))), 
                dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))),
                w.x),
            mix(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))), 
                dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))),
                w.x),
            w.z),
        mix(
            mix(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))), 
                dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))),
                w.x),
            mix(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))), 
                dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))),
                w.x),
            w.z),
        w.y);
}

//---------------------------- Distance ---------------------------

bool intersectPlane(vec3 n, vec3 p, vec3 org, vec3 dir, out float t){ 
    //Assuming vectors are all normalized
    float denom = dot(n, dir); 
    if(denom > 1e-6) { 
        t = dot(p - org, n) / denom; 
        return (t >= 0.0); 
    } 
 
    return false; 
}

float sdCappedCylinder( vec3 p, float h, float r ){
    vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(h,r);
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float fbm(float pos, int octaves){
    if(pos < 0.0){
        return 0.0;
    }
    float total = 0.0;
    float frequency = 0.2;
    float amplitude = 1.0;
    for(int i = 0; i < octaves; i++){
        if(i > 2){
            pos += 0.5*iTime * 25.0;
        }
        total += perlinNoise1D(pos * frequency) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return total;
}

float getSDF(vec3 p) {

    float dist = 1e10;

    //Shift everything to start at the cloud
    p.y -= CLOUD_START;

    //The counter of a bolt in a series
    float t = 0.0;
    //The offset of the series
    float shift = 0.0;

    //Number of noise levels for FBM
    int octaves = 4;
    //Scale of the y coordinate as noise input. Controls the smoothness of the bolt
    float scale = 0.5;
    //Offset to give simultaneous bolts different shapes
    float shapeOffset = 15.2;
    //Fraction of the total bolt length 0->1
    float progress;

    //The fraction of the lifetime of the bolt it takes for it to descend.
    //The bolt persists in full form for 1.0-descentDuration fraction of the total period.
    float descentDuration = 0.5;

    //Spatial range of the bolt
    float range = CLOUD_EXTENT*0.4;
    float boltLength = CLOUD_START*0.5;
    //Bolt thickness
    float radius = 0.01;
    //xz: the shape of the bolt
    //y:  progress used as bolt length and positioning
    vec3 offset;
    vec2 location;

    float time;

    for(int i = 0; i < 3; i++){

        shapeOffset *= 2.0;
        shift = fract(shift + 0.25);
        time = iTime * 25.0 * speed + shift;
        t = floor(time)+1.0;
        
        //Reset the position of the iteration bolt
        if(i == 0){
        	bolt0 = vec2(1e10);
        }
        if(i == 1){
        	bolt1 = vec2(1e10);
        }
        if(i == 2){
        	bolt2 = vec2(1e10);
        }

        //Bolts strike randomly
        if(hash(float(i)+t*0.026) > strikeFrequency){
            continue;
        }
        location = 2.0*vec2(hash(t+float(i)+0.43), hash(t+float(i)+0.3))-1.0;
        location *= range;
        progress = clamp(fract(time)/descentDuration, 0.0, 1.0);
        
        //Briefly increase the radius of the bolt the moment it makes contact
        if(progress > 0.95 && fract(time) - descentDuration < 0.1){
            radius = 0.1;
        }else{
            radius = 0.01;
        }
        progress *= boltLength;
        offset = vec3(location.x+fbm(shapeOffset+t*0.2+(scale*p.y), octaves), 
                      progress, 
                      location.y+fbm(shapeOffset+t*0.12-(scale*p.y), octaves));
        
        //Store the xz location of the iteration bolt
        //Raymarching translations are reversed so invert the sign
        if(i == 0){
        	bolt0 = -location.xy;
        }
        if(i == 1){
        	bolt1 = -location.xy;
        }
        if(i == 2){
        	bolt2 = -location.xy;
        }
        dist = min(dist, sdCappedCylinder(p+offset, radius, progress));
    }

    return dist;
}

float distanceToScene(vec3 cameraPos, vec3 rayDir, float start, float end, out vec3 glow) {

    float depth = start;
    float dist;
    
    for (int i = 0; i < MAX_STEPS; i++) {

        vec3 p = cameraPos + depth * rayDir;
        //Warping the cylinder breaks the shape. Reduce step size to avoid this.
        dist = 0.5*getSDF(p);
        //Accumulate the glow along the view ray.
        glow += getGlow(dist, 0.01, 0.8) * boltColour;

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

vec3 getSkyColour(vec3 rayDir){
    if(rayDir.y < 0.0){
        return vec3(0.025);
    }

    return mix(horizonColour, skyColour, pow(rayDir.y, 0.03));
}


//---------------------------- Cloud shape ----------------------------

//https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
//Compute the near and far intersections using the slab method.
//No intersection if tNear > tFar.
vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

bool insideAABB(vec3 p){
    float eps = 1e-4;
    return  (p.x > minCorner.x-eps) && (p.y > minCorner.y-eps) && (p.z > minCorner.z-eps) && 
        (p.x < maxCorner.x+eps) && (p.y < maxCorner.y+eps) && (p.z < maxCorner.z+eps);
}

bool getCloudIntersection(vec3 org, vec3 dir, out float distToStart, out float totalDistance){
    vec2 intersections = intersectAABB(org, dir, minCorner, maxCorner);

    if(insideAABB(org)){
        intersections.x = 1e-4;
    }

    distToStart = intersections.x;
    totalDistance = intersections.y - intersections.x;
    return intersections.x > 0.0 && (intersections.x < intersections.y);
}

float saturate(float x){
    return clamp(x, 0.0, 1.0);
}

float remap(float x, float low1, float high1, float low2, float high2){
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

float getNoise(vec3 pos, float speed){
    return 0.5+0.5*(perlinNoise3D(speed*iTime * 25.0+pos));
}

float clouds(vec3 p, out float cloudHeight){
	    
	//Model an anvil cloud with two flipped hemispheres
    
    cloudHeight = saturate((p.y - CLOUD_START)/(CLOUD_END-CLOUD_START));

    float bottom = 1.0-saturate(length(p.xz)/(1.25*CLOUD_EXTENT));
    
    //Round top and bottom edges
    bottom *= saturate(remap(cloudHeight, 0.25*bottom, 1.0, 1.0, 0.0)) 
        	* saturate(remap(cloudHeight, 0.0, 0.175, 0.45, 1.0));

    //Subtract coarse noise
    bottom = saturate(remap(bottom, 0.5*getNoise(0.25*p, 0.05), 1.0, 0.0, 1.0));
    //Subtract fine noise
    bottom = saturate(remap(bottom, 0.15*getNoise(1.0*p, 0.2), 1.0, 0.0, 1.0));

    float top = 1.0-saturate(length(p.xz)/(1.5*CLOUD_EXTENT));
    
    //Round top and bottom edges
    top *= saturate(remap(1.0-cloudHeight, 0.25*top, 1.0, 1.0, 0.0)) 
        * saturate(remap(1.0-cloudHeight, 0.0, 0.175, 0.45, 1.0));
    
    //Subtract coarse noise
    top = saturate(remap(top, 0.5*getNoise(0.25*p, 0.05), 1.0, 0.0, 1.0));
    //Subtract fine noise
    top = saturate(remap(top, 0.15*getNoise(1.0*p, 0.2), 1.0, 0.0, 1.0));

    return (bottom+top)*densityMultiplier;
}

//---------------------------- Cloud lighting ----------------------------

float HenyeyGreenstein(float g, float costh){
    return (1.0/(4.0 * 3.1415))  * ((1.0 - g * g) / pow(1.0 + g*g - 2.0*g*costh, 1.5));
}

//Get the amount of light that reaches a sample point.
float lightRay(vec3 org, vec3 p, float phaseFunction, float mu, vec3 sunDirection){
    float lightRayDistance = CLOUD_EXTENT*0.75;
    float distToStart = 0.0;

    getCloudIntersection(p, sunDirection, distToStart, lightRayDistance);

    float stepL = lightRayDistance/float(STEPS_LIGHT);

    float lightRayDensity = 0.0;

    float cloudHeight = 0.0;

    //Collect total density along light ray.
    for(int j = 0; j < STEPS_LIGHT; j++){
        //Reduce density of clouds when looking towards the sun for more luminous clouds.
        lightRayDensity += mix(1.0, 0.75, mu) * 
            clouds(p + sunDirection * float(j) * stepL, cloudHeight);
    }

    //Multiple scattering approximation from Nubis presentation credited to Wrenninge et al. 
    //Introduce another weaker Beer-Lambert function.
    float beersLaw = max(exp(-stepL * lightRayDensity), 
                         exp(-stepL * lightRayDensity * 0.2) * 0.75);

    //Return product of Beer's law and powder effect depending on the 
    //view direction angle with the light direction.
    return mix(beersLaw * 2.0 * (1.0-(exp(-stepL*lightRayDensity*2.0))), beersLaw, mu);
}

vec3 hash31(float p){
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}

//Get the colour along the main view ray.
vec3 mainRay(vec3 org, vec3 dir, vec3 sunDirection, 
             out float totalTransmittance, float mu, vec3 sunLightColour, float offset,
             inout float d){

    //Variable to track transmittance along view ray. 
    //Assume clear sky and attenuate light when encountering clouds.
    totalTransmittance = 1.0;

    //Default to black.
    vec3 colour = vec3(0.0);

    //The distance at which to start ray marching.
    float distToStart = 0.0;

    //The length of the intersection.
    float totalDistance = 0.0;

    //Determine if ray intersects bounding volume.
    //Set ray parameters in the cloud layer.
    bool renderClouds = getCloudIntersection(org, dir, distToStart, totalDistance);

    if(!renderClouds){
        return colour;
    }

    //Sampling step size.
    float stepS = totalDistance / float(STEPS_PRIMARY); 

    //Offset the starting point by blue noise.
    distToStart += stepS * offset;

    //Track distance to sample point.
    float dist = distToStart;

    //Initialise sampling point.
    vec3 p = org + dist * dir;

    //Combine backward and forward scattering to have details in all directions.
    float phaseFunction = mix(HenyeyGreenstein(-0.3, mu), HenyeyGreenstein(0.3, mu), 0.7);

    vec3 sunLight = sunLightColour * power;

    for(int i = 0; i < STEPS_PRIMARY; i++){

        //Normalised height for shaping and ambient lighting weighting.
        float cloudHeight;

        //Get density and cloud height at sample point
        float density = clouds(p, cloudHeight);

        //Scattering and absorption coefficients.
        float sigmaS = 1.0;
        float sigmaA = 0.0;

        //Extinction coefficient.
        float sigmaE = sigmaS + sigmaA;

        float sampleSigmaS = sigmaS * density;
        float sampleSigmaE = sigmaE * density;

        //If there is a cloud at the sample point
        if(density > 0.0 ){
            //Store closest distance to the cloud
            d = min(d, dist);
            
            //Internal lightning is additional ambient source that flickers and moves around
            //Get random position in the core of the cloud
            vec3 source = vec3(0, CLOUD_START + CLOUD_HEIGHT * 0.5, 0) + 
                		 (2.0*hash31(floor(iTime * 25.0*internalSpeed))-1.0) * CLOUD_EXTENT * 0.25;
            //Distance to the source position
            float prox = length(p - source);
            //Vary size for flicker
            float size = sin(45.0*fract(iTime * 25.0))+5.0;
            //Get distance based glow
            vec3 internal = getGlow(prox, size, 3.2) * boltColour;
            //Internal lightning occurs randomly
            if(hash(floor(iTime * 25.0)) > internalFrequency){
            	internal = vec3(0);
            }
            
            //Add ambient source at bottom of cloud where lightning bolts exit
            size = 3.0;
            float h = 0.9*CLOUD_START;
            prox = length(p - vec3(bolt0.x, h, bolt0.y));
            internal += getGlow(prox, size, 2.2) * boltColour;
            
            prox = length(p - vec3(bolt1.x, h, bolt1.y));
            internal += getGlow(prox, size, 2.2) * boltColour;
            
            prox = length(p - vec3(bolt2.x, h, bolt2.y));
            internal += getGlow(prox, size, 2.2) * boltColour;
            
            //Combine lightning and height based ambient light
            vec3 ambient = internal + sunLightColour * mix((0.05), (0.125), cloudHeight);

            //Amount of sunlight that reaches the sample point through the cloud 
            //is the combination of ambient light and attenuated direct light.
            vec3 luminance = ambient + sunLight * phaseFunction * 
                			 lightRay(org, p, phaseFunction, mu, sunDirection);

            //Scale light contribution by density of the cloud.
            luminance *= sampleSigmaS;

            //Beer-Lambert.
            float transmittance = exp(-sampleSigmaE * stepS);

            //Better energy conserving integration
            //"From Physically based sky, atmosphere and cloud rendering in Frostbite" 5.6
            //by Sebastian Hillaire.
            colour += 
                totalTransmittance * (luminance - luminance * transmittance) / sampleSigmaE; 

            //Attenuate the amount of light that reaches the camera.
            totalTransmittance *= transmittance;  

            //If ray combined transmittance is close to 0, nothing beyond this sample 
            //point is visible, so break early.
            if(totalTransmittance <= 0.01){
                totalTransmittance = 0.0;
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
vec3 ACESFilm(vec3 x){
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){


    //----------------- Define a camera -----------------

    vec3 rayDir = rayDirection(40.0, fragCoord);

    vec3 cameraPos = texelFetch(iChannel0, ivec2(0.5, 1.5), 0).xyz;

    vec3 targetDir = vec3(0,20,0) - cameraPos;

    vec3 up = vec3(0.0, 1.0, 0.0);

    //Get the view matrix from the camera orientation
    mat3 viewMatrix = lookAt(cameraPos, targetDir, up);

    //Transform the ray to point in the correct direction
    rayDir = normalize(viewMatrix * rayDir);

    //---------------------------------------------------

    float offset = 0.0;

    #ifdef DITHERING

        //From https://blog.demofox.org/2020/05/10/ray-marching-fog-with-blue-noise/
        //Get blue noise for the fragment.
        float blueNoise = texture(iChannel1, fragCoord / 1024.0).r;
        offset = fract(blueNoise + float(iFrame%32) * goldenRatio);
    
    #endif
    
    //Lightning. Set bolt positions and accumulate glow.
    vec3 glow = vec3(0);
    float dist = distanceToScene(cameraPos, rayDir, MIN_DIST, MAX_DIST, glow);
    
    //Cloud
    float totalTransmittance = 1.0;
    float exposure = 0.5;
    vec3 sunDirection = normalize(vec3(1));
    float mu = 0.5+0.5*dot(rayDir, sunDirection);
    //Distance to cloud.
    float d = 1e10;
    vec3 colour = exposure * mainRay(cameraPos, rayDir, sunDirection, totalTransmittance,
                                     mu, sunLightColour, offset, d); 

    vec3 background = getSkyColour(rayDir);
	//Draw sun
    background += sunLightColour * 0.2*getGlow(1.0-mu, 0.001, 0.55);
    
    //Distance to plane at cloud bottom limit
	float t = 1e10;
    bool hitsPlane = intersectPlane(vec3(0, -1, 0), vec3(0, CLOUD_START, 0.0), cameraPos,
                                    rayDir, t);
    
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

    //Gamma correction 1.0/2.2 = 0.4545...
    colour = pow(colour, vec3(0.4545));
    
    // Output to screen
    fragColor = vec4(colour, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Track mouse movement and resolution change between frames and set camera position.

#define PI 3.14159
#define EPS 1e-4
#define CAMERA_DIST 65.0

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    
    //Work with just the first four pixels.
    if((fragCoord.x == 0.5) && (fragCoord.y < 4.0)){
        
        vec4 oldMouse = texelFetch(iChannel0, ivec2(0.5), 0).xyzw;
        vec4 mouse = (iMouse / iResolution.xyxy); 
        vec4 newMouse = vec4(0);

        float mouseDownLastFrame = texelFetch(iChannel0, ivec2(0.5, 3.5), 0).x;
        
        //If mouse button is down and was down last frame
        if(iMouse.z > 0.0 && mouseDownLastFrame > 0.0){
            
            //Difference between mouse position last frame and now.
            vec2 mouseMove = mouse.xy-oldMouse.zw;
            newMouse = vec4(oldMouse.xy + vec2(5.0, 3.0)*mouseMove, mouse.xy);
        }else{
            newMouse = vec4(oldMouse.xy, mouse.xy);
        }
        newMouse.x = mod(newMouse.x, 2.0*PI);
        newMouse.y = min(0.99, max(-0.99, newMouse.y));

        //Store mouse data in the first pixel of Buffer B.
        if(fragCoord == vec2(0.5, 0.5)){
            //Set value at first frames
            if(iFrame < 5){
                newMouse = vec4(-0.0, -0.05, 0.0, 0.0);
            }
            fragColor = vec4(newMouse);
        }

        //Store camera position in the second pixel of Buffer B.
        if(fragCoord == vec2(0.5, 1.5)){
            //Set camera position from mouse information.
            vec3 cameraPos = CAMERA_DIST * vec3(sin(newMouse.x), -sin(newMouse.y), -cos(newMouse.x));
            fragColor = vec4(cameraPos, 1.0);
        }
        
        //Store resolution change data in the third pixel of Buffer B.
        if(fragCoord == vec2(0.5, 2.5)){
            float resolutionChangeFlag = 0.0;
            //The resolution last frame.
            vec2 oldResolution = texelFetch(iChannel0, ivec2(0.5, 2.5), 0).yz;
            
            if(iResolution.xy != oldResolution){
            	resolutionChangeFlag = 1.0;
            }
            
        	fragColor = vec4(resolutionChangeFlag, iResolution.xy, 1.0);
        }
           
        //Store whether the mouse button is down in the fourth pixel of Buffer A
        if(fragCoord == vec2(0.5, 3.5)){
            if(iMouse.z > 0.0){
            	fragColor = vec4(vec3(1.0), 1.0);
            }else{
            	fragColor = vec4(vec3(0.0), 1.0);
            }
        }
        
    }
}