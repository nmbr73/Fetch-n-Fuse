

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Water material with depth peeling, light scattering and texture distortion
// Follow up to https://www.shadertoy.com/view/sdBGWh

// BufferA: Camera and resolution change tracking
// BufferB: R - Perlin noise FBM for wave height map
//          G - Worley noise for staggered mixing of textures

// Large bodies of water are blue mainly due to absorption of lower frequency light. 
// Scattering from particulates suspended in the water can add other tones. For simplicity, 
// we'll use a combined transmittance colour that attenuates with Beer's law. The environment
// and direct lights use the same depth along the refracted view ray. Light scattering uses a
// two-lobed Henyey-Greenstein phase function.

// Is there a simple phase function for water or some information on the rate of absorption?

// EDIT 1: Added separate light marching.

// EDIT 2: Handle total internal reflection

// Uncomment for separate light depth marching and better light contribution
//#define HQ_LIGHT

// Variable iterator initializer to stop loop unrolling
#define ZERO (min(iFrame,0))

// Comment out to remove environment map
#define CUBEMAP

// Index of refraction for water
#define IOR 1.333

// Ratios of air and water IOR for refraction
// Air to water
#define ETA 1.0/IOR
// Water to air
#define ETA_REVERSE IOR

const int MAX_STEPS = 50;
const float MIN_DIST = 0.01;
const float MAX_DIST = 5.0;
const float EPSILON = 1e-4;
const float DETAIL_EPSILON = 2e-3;
const float DETAIL_HEIGHT = 0.1;
const vec3 DETAIL_SCALE = vec3(1.0);
const vec3 BLENDING_SHARPNESS = vec3(4.0);

const vec3 sunLightColour = vec3(3.5);

vec3 waterColour = 0.85 * vec3(0.1, 0.75, 0.9);

// Amount of the background visible through the water
const float CLARITY = 0.75;

// Modifiers for light attenuation
const float DENSITY = 3.5;
const float DENSITY_POW = 1.0;

// In a circle of 2*PI
const float sunLocation = -2.0;
const float sunHeight = 0.9;

vec3 rayDirection(float fieldOfView, vec2 fragCoord) {
    vec2 xy = fragCoord - iResolution.xy / 2.0;
    float z = (0.5 * iResolution.y) / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

// https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/
mat3 lookAt(vec3 camera, vec3 at, vec3 up){
  vec3 zaxis = normalize(at-camera);    
  vec3 xaxis = normalize(cross(zaxis, up));
  vec3 yaxis = cross(xaxis, zaxis);

  return mat3(xaxis, yaxis, -zaxis);
}


vec3 getSkyColour(vec3 rayDir){
    vec3 col;
#ifdef CUBEMAP
    col = inv_gamma(texture(iChannel2, rayDir).rgb);
    // Add some bloom to the environment
    col += 2.0 * pow(col, vec3(2));
#else 
    col = 0.5*(0.5+0.5*rayDir);
#endif
    return col;
}

float getGlow(float dist, float radius, float intensity){
    dist = max(dist, 1e-6);
	return pow(radius/dist, intensity);	
}

//-------------------------------- Rotations --------------------------------

vec3 rotate(vec3 p, vec4 q){
  return 2.0 * cross(q.xyz, p * q.w + cross(q.xyz, p)) + p;
}
vec3 rotateX(vec3 p, float angle){
    return rotate(p, vec4(sin(angle/2.0), 0.0, 0.0, cos(angle/2.0)));
}
vec3 rotateY(vec3 p, float angle){
	return rotate(p, vec4(0.0, sin(angle/2.0), 0.0, cos(angle/2.0)));
}
vec3 rotateZ(vec3 p, float angle){
	return rotate(p, vec4(0.0, 0.0, sin(angle), cos(angle)));
}


//---------------------------- Distance functions ----------------------------

// Distance functions and operators from:
// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

float displacement(vec3 p){
    return sin(p.x)*sin(p.y)*sin(p.z);
}

float opDisplace(vec3 p){
    vec3 offset = 0.4*iTime * normalize(vec3(1.0, -1.0, 0.1));
    return displacement(10.0*(p+offset));
}


float opSmoothSub( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float sphereSDF(vec3 p, float radius) {
    return length(p) - radius;
}

float sdRoundCone( vec3 p, float r1, float r2, float h ){
  vec2 q = vec2( length(p.xz), p.y );
    
  float b = (r1-r2)/h;
  float a = sqrt(1.0-b*b);
  float k = dot(q,vec2(-b,a));
    
  if( k < 0.0 ) return length(q) - r1;
  if( k > a*h ) return length(q-vec2(0.0,h)) - r2;
        
  return dot(q, vec2(a,b) ) - r1;
}

// https://www.iquilezles.org/www/articles/smin/smin.htm
float smoothMin(float a, float b, float k){
    float h = clamp(0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float getSDF(vec3 p, float sdfSign){

    p.y -= 0.4;
    float dist = 1e5;
    vec3 q = p;
    
    // Upper back
    dist = sphereSDF(q, 0.5);
    
   
    // Head
    q.y -= 0.25;
    q.x -= 0.45;
    q = rotateZ(q, 0.39);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.25, 0.25, 0.1), 0.25);
    
    
    // Upper body
    // Two round cones for chest and shoulders
    q = p;
    q.z = abs(q.z);
    q.y += 0.1;
    q.z -= 0.15;
    q = rotateX(q, -1.45);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.4, 0.35, 0.4), 0.25);   
    
    
    // Lower body
    q = p;
    q.y += 0.5;
    q.x += 0.15;
    q = rotateZ(q, 1.4);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.35, 0.25, 0.7), 0.5);

    
    // Base
    // A large round cone
    q = p;
    q.y += 1.4;
    q.x -= 0.1;
    q = rotateZ(q, -1.5);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.24, 2.0, 3.0), 0.25);


    // Subtract a sphere from the base to make it flared
    q = p;
    q.y += 4.75;
    
    dist = opSmoothSub(sphereSDF(q, 2.8), dist, 0.15);
     
    // Arms
    q = p;
    q.z = abs(q.z);
    q.z -= 0.8;
    q.y += 0.3;
    q = rotateZ(q, -1.7);
    q = rotateX(q, -0.2);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.22, 0.2, 0.3), 0.15);
    
    // Forearms
    q = p;
    q.z = abs(q.z);
    q.z -= 0.9;
    q.y += 0.8;
    q.x -= 0.15;
    q = rotateZ(q, -2.0);
    q = rotateX(q, 0.15);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.18, 0.18, 0.3), 0.1);
  
  
    // Fists
    q = p;
    q.z = abs(q.z);
    q.z -= 0.77;
    q.y += 0.95;
    q.x -= 0.55;
    q = rotateZ(q, PI*0.6);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.1, 0.1, 0.2), 0.15);
   
   
    float height = p.y+0.4;
    // Displace the surface for larger waves
    // Add more displacement lower down
    float strength = mix(0.02, 0.1, smoothstep(-0.6, -1.5, height));
    if(height < -1.5){
        // No displacement at the very bottom
        strength = mix(strength, 0.0, smoothstep(-1.5, -1.62, height));
    }
    dist += strength * opDisplace(p);
    
    return sdfSign * dist;
}

float distanceToScene(vec3 cameraPos, vec3 rayDir, float start, float end, float sdfSign){
	
    // Start at a predefined distance from the camera in the ray direction
    float depth = start;
    
    // Variable that tracks the distance to the scene at the current ray endpoint
    float dist;
    
    // For a set number of steps
    for (int i = ZERO; i < MAX_STEPS; i++) {
        
        // Get the sdf value at the ray endpoint, giving the maximum 
        // safe distance we can travel in any direction without hitting a surface
        dist = getSDF(cameraPos + depth * rayDir, sdfSign);
        
        // If it is small enough, we have hit a surface
        // Return the depth that the ray travelled through the scene
        if (dist < EPSILON){
            return depth;
        }
        
        // Else, march the ray by the sdf value
        depth += dist;
        
        // Test if we have left the scene
        if (depth >= end){
            return end;
        }
    }

    return depth;
}

//----------------------------- Texture distortion -----------------------------

// Find the local gradients in the X and Y directions which we use as the velocities 
// of the texure distortion
vec2 getGradient(vec2 uv){

    float delta = 1e-1;
    uv *= 0.3;
    
    float data = texture(iChannel1, uv).r;
    float gradX = data - texture(iChannel1, uv-vec2(delta, 0.0)).r;
    float gradY = data - texture(iChannel1, uv-vec2(0.0, delta)).r;
    
    return vec2(gradX, gradY);
}

// https://catlikecoding.com/unity/tutorials/flow/texture-distortion/
float getDistortedTexture(vec2 uv){

    float strength = 0.5;
    
    // The texture is distorted in time and we switch between two texture states.
    // The transition is based on Worley noise which will shift the change of differet parts
    // for a more organic result
    float time = 0.5 * iTime + texture(iChannel1, 0.25*uv).g;
    
    float f = fract(time);
    
    // Get the velocity at the current location
    vec2 grad = getGradient(uv);
    uv *= 1.0;
    vec2 distortion = strength*vec2(grad.x, grad.y) + vec2(0, -0.3);

    // Get two shifted states of the texture distorted in time by the local velocity.
    // Loop the distortion from 0 -> 1 using fract(time)
    float distort1 = texture(iChannel1, uv + f * distortion).r;
    float distort2 = texture(iChannel1, uv + fract(time + 0.5) * distortion).r;

    // Mix between the two texture states to hide the sudden jump from 1 -> 0.
    // Modulate the value returned by the velocity.
    return (1.0-length(grad)) * (mix(distort1, distort2, abs(1.0 - 2.0 * f)));
}

//----------------------------- Normal mapping -----------------------------

// https://tinyurl.com/y5ebd7w7
vec3 getTriplanar(vec3 position, vec3 normal){

    // A hack to get the flow direction on the arms to be consistent
    vec2 xpos = position.zx;
    if(abs(position.z) > 0.65){
        // If position is below 0.0, flip the uv direction for upwards flow
        xpos = mix(xpos, vec2(position.z, -position.x), smoothstep(-0.0, -0.2, position.y));
    }

    vec3 xaxis = vec3(getDistortedTexture(DETAIL_SCALE.x*(position.zy)));
    vec3 yaxis = vec3(getDistortedTexture(DETAIL_SCALE.y*(xpos)));
    vec3 zaxis = vec3(getDistortedTexture(DETAIL_SCALE.z*(position.xy)));
   
    vec3 blending = abs(normal);
	blending = normalize(max(blending, 0.00001));
    blending = pow(blending, BLENDING_SHARPNESS);
	float b = (blending.x + blending.y + blending.z);
	blending /= b;

    return	xaxis * blending.x + 
       		yaxis * blending.y + 
        	zaxis * blending.z;
}

// Return the position of p extruded in the normal direction by a normal map
vec3 getDetailExtrusion(vec3 p, vec3 normal){

    float detail = DETAIL_HEIGHT * length(getTriplanar(p, normal));
    
    // Increase the normal extrusion height on the upper body
    float d = 1.0 + smoothstep(0.0, -0.5, p.y);
    return p + d * detail * normal;
}

// Tetrahedral normal technique with a loop to avoid inlining getSDF()
// This should improve compilation times
// https://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 getNormal(vec3 p, float sdfSign){
    vec3 n = vec3(0.0);
    int id;
    for(int i = ZERO; i < 4; i++){
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*getSDF(p+e*EPSILON, sdfSign);
    }
    return normalize(n);
}

// Get orthonormal basis from surface normal
// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void pixarONB(vec3 n, out vec3 b1, out vec3 b2){
	float sign_ = sign(n.z);
	float a = -1.0 / (sign_ + n.z);
	float b = n.x * n.y * a;
	b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
	b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

// Return the normal after applying a normal map
vec3 getDetailNormal(vec3 p, vec3 normal){

    vec3 tangent;
    vec3 bitangent;
    
    // Construct orthogonal directions tangent and bitangent to sample detail gradient in
    pixarONB(normal, tangent, bitangent);
    
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);

    vec3 delTangent = vec3(0);
    vec3 delBitangent = vec3(0);
    
    for(int i = ZERO; i < 2; i++){
        
        //i to  s
        //0 ->  1
        //1 -> -1
        float s = 1.0 - 2.0 * float(i&1);
    
        delTangent += s * getDetailExtrusion(p + s * tangent * DETAIL_EPSILON, normal);
        delBitangent += s * getDetailExtrusion(p + s * bitangent * DETAIL_EPSILON, normal);

    }
    
    return normalize(cross(delTangent, delBitangent));
}

//--------------------------------- PBR ---------------------------------

// Trowbridge-Reitz
float distribution(vec3 n, vec3 h, float roughness){
    float a_2 = roughness*roughness;
	return a_2/(PI*pow(pow(dot_c(n, h),2.0) * (a_2 - 1.0) + 1.0, 2.0));
}

// GGX and Schlick-Beckmann
float geometry(float cosTheta, float k){
	return (cosTheta)/(cosTheta*(1.0-k)+k);
}

float smiths(vec3 n, vec3 viewDir, vec3 lightDir, float roughness){
    float k = pow(roughness + 1.0, 2.0)/8.0; 
	return geometry(dot_c(n, lightDir), k) * geometry(dot_c(n, viewDir), k);
}

// Fresnel-Schlick
vec3 fresnel(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

// Specular part of Cook-Torrance BRDF
vec3 BRDF(vec3 p, vec3 n, vec3 viewDir, vec3 lightDir, vec3 F0, float roughness){
    vec3 h = normalize(viewDir + lightDir);
    float cosTheta = dot_c(h, viewDir);
    float D = distribution(n, h, roughness);
    vec3 F = fresnel(cosTheta, F0);
    float G = smiths(n, viewDir, lightDir, roughness);
    
    vec3 specular =  D * F * G / max(0.0001, (4.0 * dot_c(lightDir, n) * dot_c(viewDir, n)));
    
    return specular;
}

// From the closest intersection with the scene, raymarch the negative SDF field to 
// find the far instersection. The distance inside the water is used to determine 
// transmittance and the attenuation of the environment.
vec3 getEnvironment(vec3 org, vec3 rayDir, out vec3 transmittance, out vec3 halfwayPoint){
        float sdfSign = -1.0;
        
        float distFar = distanceToScene(org, rayDir, MIN_DIST, MAX_DIST, sdfSign);
        
        vec3 positionFar = org + rayDir * distFar;
        halfwayPoint = org + rayDir * distFar * 0.5;
        vec3 geoNormalFar = getNormal(positionFar, sdfSign);

        // Avoid artefacts when trying to sample detail normals across Z-plane. Shape 
        // deformation increases the region where visible errors occur.
        if(abs(geoNormalFar.z) < 1e-5){
            geoNormalFar.z = 1e-5;
        }

        //Use the geometry normal on the far side to reduce noise
        vec3 refractedDir = normalize(refract(rayDir, geoNormalFar, ETA_REVERSE));

        // When total internal reflection occurs, reflect the ray off the far side
        // Critical angle for 1.333 -> 1.0 is 0.8483
        // cos(0.8483) = 0.66125
        if(dot(-rayDir, geoNormalFar) <= 0.66125){
            refractedDir = normalize(reflect(rayDir, geoNormalFar));
        }

        vec3 transmitted = getSkyColour(refractedDir);
        
        // View depth
        float d = DENSITY*length(org-positionFar);
        
        if(DENSITY_POW != 1.0){
            d = pow(d, DENSITY_POW);
        }
        
        
        // Beer's law depending on the water colour
        transmittance = exp( -d * (1.0 - waterColour));
        
        vec3 result = transmitted * transmittance;
        return result;
}

float getLightDepth(vec3 org, vec3 rayDir){
    float sdfSign = -1.0;
        
    return distanceToScene(org, rayDir, MIN_DIST, MAX_DIST, sdfSign);
}


//------------------------------- Shading -------------------------------

float HenyeyGreenstein(float g, float costh){
	return (1.0/(FOUR_PI))  * ((1.0 - g * g) / pow(1.0 + g*g - 2.0*g*costh, 1.5));
}

vec3 shadingPBR(vec3 cameraPos, vec3 lightPos, vec3 p, vec3 n, vec3 rayDir, vec3 geoNormal){
    vec3 I = vec3(0);

    vec3 F0 = vec3(0.02);
    float roughness = 0.1;

    vec3 vectorToLight = lightPos - p;
   	vec3 lightDir = normalize(vectorToLight);
    I +=  BRDF(p, n, -rayDir, lightDir, F0, roughness) 
        * sunLightColour 
        * dot_c(n, lightDir);
        

    vec3 transmittance;
    vec3 halfwayPoint;
    
    float f = smoothstep(0.0, -0.5, p.y);
    
    vec3 result = vec3(0);
    
    result += (1.0-f) * CLARITY * getEnvironment(p+rayDir*2.0*EPSILON,
                                  refract(rayDir, n, ETA), 
                                  transmittance, halfwayPoint);
    
   
    float mu = dot(refract(rayDir, n, ETA), lightDir);
    float phase = mix(HenyeyGreenstein(-0.3, mu), HenyeyGreenstein(0.85, mu), 0.5);
    
    #ifdef HQ_LIGHT
    
    float lightDepth = getLightDepth(p+rayDir*2.0*EPSILON, normalize(lightPos-halfwayPoint));
    lightDepth = max(1.0, lightDepth);

    vec3 lightTransmittance = exp(-DENSITY*lightDepth * (1.0 - waterColour));
    
    result += CLARITY * sunLightColour * lightTransmittance * phase;
    #else
    
    result += CLARITY * sunLightColour * transmittance * phase;
    
    #endif
    
    // Reflection of the environment.
    vec3 reflectedDir = normalize(reflect(rayDir, n));
    vec3 reflectedCol = getSkyColour(reflectedDir);
    
    float cosTheta = dot_c(n, -rayDir);
    vec3 F = fresnel(cosTheta, F0);
    
    result = mix(result, reflectedCol, F);
    
    // Foam based on wave height
    float waveHeight = length(getTriplanar(p, n));

    // Sharper crests higher up
    float e = mix(2.0, 16.0, 1.0-smoothstep(0.2, -1.3, p.y));
    
    result += f * pow(waveHeight, e);

    return result + I;
}

//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x){
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
        
    vec3 col = vec3(0);
    
	//----------------- Define a camera -----------------
    
    vec3 rayDir = rayDirection(60.0, fragCoord);

    //vec3 cameraPos = texelFetch(iChannel0, ivec2(0.5, 1.5), 0).xyz;
    vec3 cameraPos = texture(iChannel0, (vec2(ivec2(0.5, 1.5))+0.5)/iResolution.xy).xyz;

    vec3 targetDir = -cameraPos;

    vec3 up = vec3(0.0, 1.0, 0.0);

    // Get the view matrix from the camera orientation.
    mat3 viewMatrix = lookAt(cameraPos, targetDir, up);

    // Transform the ray to point in the correct direction.
    rayDir = normalize(viewMatrix * rayDir);

    //---------------------------------------------------
	
    vec3 lightPos = 100.0 * normalize(vec3(cos(sunLocation), sunHeight, sin(sunLocation)));
    vec3 lightDirection = normalize(lightPos);
    
    float sdfSign = 1.0;
    
    // Find the distance to where the ray stops.
    float dist = distanceToScene(cameraPos, rayDir, MIN_DIST, MAX_DIST, sdfSign);
    
    if(dist < MAX_DIST){
    
    
    vec3 position = cameraPos + rayDir * dist;
        
        vec3 geoNormal = getNormal(position, sdfSign);

        // Avoid artefacts when trying to sample detail normals across Z-plane. Shape 
        // deformation increases the region where visible errors occur.
        if(abs(geoNormal.z) < 1e-5){
            geoNormal.z = 1e-5;
        }

        vec3 detailNormal = normalize(getDetailNormal(position, geoNormal));
        
        col += shadingPBR(cameraPos, lightPos, position, detailNormal, rayDir, geoNormal);

        
    }else{
    
        col += getSkyColour(rayDir);
        float mu = dot(rayDir, lightDirection);
        col += sunLightColour * getGlow(1.0-mu, 0.0005, 1.0);
        
    }
    
    //col = vec3(getDistortedTexture(fragCoord.xy/iResolution.xy));
    //col = vec3(texture(iChannel1, fragCoord.xy/iResolution.xy, 0.0).r);
    
    //Tonemapping.
    col = ACESFilm(col);

    //Gamma correction 1.0/2.2 = 0.4545...
    col = pow(col, vec3(0.4545));

    //Output to screen.
    fragColor = vec4(col, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Track mouse movement and resolution change between frames and set camera position.

#define CAMERA_DIST 2.5

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    
    // Work with just the first four pixels.
    if((fragCoord.x == 0.5) && (fragCoord.y < 4.0)){
        
        //vec4 oldMouse = texelFetch(iChannel0, ivec2(0.5), 0).xyzw;
        vec4 oldMouse = texture(iChannel0, (vec2(ivec2(0.5))+0.5)/iResolution.xy);
        vec4 mouse = (iMouse / iResolution.xyxy); 
        vec4 newMouse = vec4(0);

        //float mouseDownLastFrame = texelFetch(iChannel0, ivec2(0.5, 3.5), 0).x;
        float mouseDownLastFrame = texture(iChannel0, (vec2(ivec2(0.5, 3.5))+0.5)/iResolution.xy).x;
        
        // If mouse button is down and was down last frame
        if(iMouse.z > 0.0 && mouseDownLastFrame > 0.0){
            
            // Difference between mouse position last frame and now.
            vec2 mouseMove = mouse.xy-oldMouse.zw;
            newMouse = vec4(oldMouse.xy + vec2(5.0, 3.0)*mouseMove, mouse.xy);
        }else{
            newMouse = vec4(oldMouse.xy, mouse.xy);
        }
        newMouse.x = mod(newMouse.x, 2.0*PI);
        newMouse.y = min(0.99, max(-0.99, newMouse.y));

        // Store mouse data in the first pixel of Buffer A.
        if(fragCoord == vec2(0.5, 0.5)){
            // Set value at first frames
            if(iFrame < 5){
                newMouse = vec4(1.15, 0.2, 0.0, 0.0);
            }
            fragColor = vec4(newMouse);
        }

        // Store camera position in the second pixel of Buffer A.
        if(fragCoord == vec2(0.5, 1.5)){
            // Set camera position from mouse information.
            vec3 cameraPos = CAMERA_DIST * 
                                vec3(sin(newMouse.x), -sin(newMouse.y), -cos(newMouse.x));
                                
            fragColor = vec4(cameraPos, 1.0);
        }
        
        // Store resolution change data in the third pixel of Buffer A.
        if(fragCoord == vec2(0.5, 2.5)){
            float resolutionChangeFlag = 0.0;
            // The resolution last frame.
            //vec2 oldResolution = texelFetch(iChannel0, ivec2(0.5, 2.5), 0).yz;
            vec2 oldResolution = texture(iChannel0, (vec2(ivec2(0.5, 2.5))+0.5)/iResolution.xy).yz;
            
            if(iResolution.xy != oldResolution){
            	resolutionChangeFlag = 1.0;
            }
            
        	fragColor = vec4(resolutionChangeFlag, iResolution.xy, 1.0);
        }
           
        // Store whether the mouse button is down in the fourth pixel of Buffer A
        if(fragCoord == vec2(0.5, 3.5)){
            if(iMouse.z > 0.0){
            	fragColor = vec4(vec3(1.0), 1.0);
            }else{
            	fragColor = vec4(vec3(0.0), 1.0);
            }
        }
        
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Perlin noise FBM for heightmap and Worley noise for texture fade out control.

// GLSL version of 2D periodic seamless perlin noise.
// https://github.com/g-truc/glm/blob/master/glm/gtc/noise.inl

vec4 taylorInvSqrt(vec4 r){
    return 1.79284291400159-0.85373472095314*r;
}

vec4 mod289(vec4 x){
  return x-floor(x*(1.0/289.0))*289.0;
}

vec4 permute(vec4 x){
  return mod289(((x*34.0)+1.0)*x);
}

vec2 fade(vec2 t){
  return (t * t * t) * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlin(vec2 Position, vec2 rep){
    vec4 Pi = floor(vec4(Position.x, Position.y, Position.x, Position.y)) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(vec4(Position.x, Position.y, Position.x, Position.y)) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod(Pi, vec4(rep.x, rep.y, rep.x, rep.y)); // To create noise with explicit period
    Pi = mod(Pi, vec4(289)); // To avoid truncation effects in permutation
    vec4 ix = vec4(Pi.x, Pi.z, Pi.x, Pi.z);
    vec4 iy = vec4(Pi.y, Pi.y, Pi.w, Pi.w);
    vec4 fx = vec4(Pf.x, Pf.z, Pf.x, Pf.z);
    vec4 fy = vec4(Pf.y, Pf.y, Pf.w, Pf.w);

    vec4 i = permute(permute(ix) + iy);

    vec4 gx = float(2) * fract(i / float(41)) - float(1);
    vec4 gy = abs(gx) - float(0.5);
    vec4 tx = floor(gx + float(0.5));
    gx = gx - tx;

    vec2 g00 = vec2(gx.x, gy.x);
    vec2 g10 = vec2(gx.y, gy.y);
    vec2 g01 = vec2(gx.z, gy.z);
    vec2 g11 = vec2(gx.w, gy.w);

    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;

    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));

    vec2 fade_xy = fade(vec2(Pf.x, Pf.y));
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return float(2.3) * n_xy;
}

float hash(float n){
	return fract(sin(n) * 43758.5453);
}

// From Shadertoy somewhere but not sure where originally.
float noise(in vec3 x){
	vec3 p = floor(x);
	vec3 f = fract(x);

	f = f*f*(3.0 - 2.0 * f);
	float n = p.x + p.y*57.0 + 113.0*p.z;
	return mix(
	mix(
       	mix(hash(n + 0.0), hash(n + 1.0), f.x),
		mix(hash(n + 57.0), hash(n + 58.0), f.x),
		f.y),
	mix(
		mix(hash(n + 113.0), hash(n + 114.0), f.x),
		mix(hash(n + 170.0), hash(n + 171.0), f.x),
		f.y),
	f.z);
}

float TILES = 1.0;

float worley(vec3 pos, float numCells){
	vec3 p = pos * numCells;
	float d = 1.0e10;
	for (int x = -1; x <= 1; x++){
		for (int y = -1; y <= 1; y++){
			for (int z = -1; z <= 1; z++){
                vec3 tp = floor(p) + vec3(x, y, z);
                tp = p - tp - noise(mod(tp, numCells / TILES));
                d = min(d, dot(tp, tp));
            }
        }
    }
	return 1.0 - clamp(d, 0.0, 1.0);
}

float fbm(vec2 pos, vec2 scale){
    float res = 0.0;
    float freq = 1.0;
    float amp = 1.0;
    float sum = 0.0;
    
    int limit = 5;
    
    for(int i = 0; i < limit; i++){ 
        float offset = float(limit-i);
        float d = mod(float(i), 2.0) > 0.0 ? 1.0 : -1.0;
        res += d * (perlin(freq*(pos+offset), freq*scale)) * amp;

        freq *= 2.0;
        amp *= 0.5;
    }
    return res/float(limit);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ){
   
    vec2 uv = fragCoord/iResolution.xy;
    vec3 col = vec3(0);
    
    //bool resolutionChanged = texelFetch(iChannel0, ivec2(0.5, 2.5), 0).r > 0.0;
    bool resolutionChanged = texture(iChannel0, (vec2(ivec2(0.5, 2.5))+0.5)/iResolution.xy).x > 0.0;
    
    if(iFrame == 0 || resolutionChanged){

        // Use diffrent scales for X and Y to get long wave fronts
        vec2 scale = vec2(8.0, 15.0);

        // For seamless texture, UV scale has to match rep
        float noise = perlin(scale*uv, vec2(scale));
        noise = 0.5+0.5*(fbm(scale*uv, vec2(scale)));

        float s = 2.0;
        float worley = worley(s * vec3(uv, 0.0), s);
        
        col = vec3(noise, worley, 0.0);
        
    }else{
        //col = texelFetch(iChannel1, ivec2(fragCoord.xy), 0).rgb;
        col = texture(iChannel1, (vec2(ivec2(fragCoord.xy))+0.5)/iResolution.xy).xyz;
    }
    
    // Output to screen
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.14159
#define FOUR_PI 4.0 * PI
#define GAMMA 2.2
#define INV_GAMMA (1.0/GAMMA)

// Minimum dot product value
const float minDot = 1e-3;

// Clamped dot product
float dot_c(vec3 a, vec3 b){
	return max(dot(a, b), minDot);
}

vec3 gamma(vec3 col){
	return pow(col, vec3(INV_GAMMA));
}

vec3 inv_gamma(vec3 col){
	return pow(col, vec3(GAMMA));
}

float saturate(float x){
	return clamp(x, 0.0, 1.0);
}
