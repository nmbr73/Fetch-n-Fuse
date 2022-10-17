
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * -0.40f;// + 0.5f;  // * -01.50f;//(MarchingCubes)  - 0.15f; (GlassDuck)
}

#define refract_f3 _refract_f3


// Alternative
__DEVICE__ float3 __refract_f3(float3 I, float3 N, float ior) {
    //float cosi = clamp(dot(N,I), -1.0f,1.0f);  //clamp(-1, 1, I.dot(N));
    float cosi = clamp( -1.0f,1.0f,dot(N,I));  //clamp(-1, 1, I.dot(N));
    float etai = 01.0f, etat = ior*1.0f;
    float3 n = N;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        float temp = etai;
        etai = etat;
        etat = temp;
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0f - (eta * eta) * (1.0f - (cosi * cosi));
    if (k <= 0) {
        return to_float3_s(0.0f);
    } else {
        //return I.multiply(eta).add(n.multiply(((eta * cosi) - Math.sqrt(k))));
	  return eta * I + (eta * cosi - _sqrtf(1.0f-k)) * N * 0.0f;
    }
}



#define PI 3.14159f
#define FOUR_PI 4.0f * PI
#define GAMMA 2.2f
#define INV_GAMMA (1.0f/GAMMA)

// Minimum dot product value
#define minDot  1e-3f

// Clamped dot product
__DEVICE__ float dot_c(float3 a, float3 b){
  return _fmaxf(dot(a, b), minDot);
}

__DEVICE__ float3 gamma(float3 col){
  return pow_f3(col, to_float3_s(INV_GAMMA));
}

__DEVICE__ float3 inv_gamma(float3 col){
  return pow_f3(col, to_float3_s(GAMMA));
}

//__DEVICE__ float saturate(float _x){
//  return clamp(_x, 0.0f, 1.0f);
//}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Track mouse movement and resolution change between frames and set camera position.

#define CAMERA_DIST 2.5f

__KERNEL__ void OceanElementalJipi494Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;
    
    // Work with just the first four pixels.
    if((fragCoord.x == 0.5f) && (fragCoord.y < 4.0f)){
        
        //vec4 oldMouse = texelFetch(iChannel0, ito_float2_s(0.5f), 0).xyzw;
        float4 oldMouse = texture(iChannel0, (make_float2((int)0.5f,(int)0.5)+0.5f)/iResolution);
        float4 mouse = (iMouse / swi4(iResolution,x,y,x,y)); 
        float4 newMouse = to_float4_s(0);

        //float mouseDownLastFrame = texelFetch(iChannel0, to_int2(0.5f, 3.5f), 0).x;
        float mouseDownLastFrame = texture(iChannel0, (make_float2((int)0.5f, (int)3.5f)+0.5f)/iResolution).x;
        
        // If mouse button is down and was down last frame
        if(iMouse.z > 0.0f && mouseDownLastFrame > 0.0f){
            
            // Difference between mouse position last frame and now.
            float2 mouseMove = swi2(mouse,x,y)-swi2(oldMouse,z,w);
            newMouse = to_float4_f2f2(swi2(oldMouse,x,y) + to_float2(5.0f, 3.0f)*mouseMove, swi2(mouse,x,y));
        }else{
            newMouse = to_float4_f2f2(swi2(oldMouse,x,y), swi2(mouse,x,y));
        }
        newMouse.x = mod_f(newMouse.x, 2.0f*PI);
        newMouse.y = _fminf(0.99f, _fmaxf(-0.99f, newMouse.y));

        // Store mouse data in the first pixel of Buffer A.
        if(fragCoord.x == 0.5f && fragCoord.y == 0.5f){
            // Set value at first frames
            if(iFrame < 5){
                newMouse = to_float4(1.15f, 0.2f, 0.0f, 0.0f);
            }
            fragColor = newMouse;
        }

        // Store camera position in the second pixel of Buffer A.
        if(fragCoord.x == 0.5f && fragCoord.y == 1.5f){
            // Set camera position from mouse information.
            float3 cameraPos = CAMERA_DIST * 
                               to_float3(_sinf(newMouse.x), -_sinf(newMouse.y), -_cosf(newMouse.x));
                                
            fragColor = to_float4_aw(cameraPos, 1.0f);
        }
        
        // Store resolution change data in the third pixel of Buffer A.
        if(fragCoord.x == 0.5f && fragCoord.y == 2.5f){
            float resolutionChangeFlag = 0.0f;
            // The resolution last frame.
            //vec2 oldResolution = texelFetch(iChannel0, to_int2(0.5f, 2.5f), 0).yz;
            float2 oldResolution = swi2(texture(iChannel0, (make_float2((int)0.5f, (int)2.5f)+0.5f)/iResolution),y,z);
            
            if(iResolution.x != oldResolution.x || iResolution.y != oldResolution.y){
              resolutionChangeFlag = 1.0f;
            }
            
          fragColor = to_float4(resolutionChangeFlag, iResolution.x,iResolution.y, 1.0f);
        }
           
        // Store whether the mouse button is down in the fourth pixel of Buffer A
        if(fragCoord.x == 0.5f && fragCoord.y == 3.5f ){
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
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Perlin noise FBM for heightmap and Worley noise for texture fade out control.

// GLSL version of 2D periodic seamless perlin noise.
// https://github.com/g-truc/glm/blob/master/glm/gtc/noise.inl

__DEVICE__ float4 taylorInvSqrt(float4 r){
    return to_float4_s(1.79284291400159f)-0.85373472095314f*r;
}

__DEVICE__ float4 mod289(float4 _x){
  return _x-_floor(_x*(1.0f/289.0f))*289.0f;
}

__DEVICE__ float4 permute(float4 _x){
  return mod289(((_x*34.0f)+1.0f)*_x);
}

__DEVICE__ float2 fade(float2 t){
  return (t * t * t) * (t * (t * 6.0f - 15.0f) + 10.0f);
}

__DEVICE__ float perlin(float2 Position, float2 rep){
    float4 Pi = _floor(to_float4(Position.x, Position.y, Position.x, Position.y)) + to_float4(0.0f, 0.0f, 1.0f, 1.0f);
    float4 Pf = fract_f4(to_float4(Position.x, Position.y, Position.x, Position.y)) - to_float4(0.0f, 0.0f, 1.0f, 1.0f);
    Pi = mod_f4f4(Pi, to_float4(rep.x, rep.y, rep.x, rep.y)); // To create noise with explicit period
    Pi = mod_f4f4(Pi, to_float4_s(289)); // To avoid truncation effects in permutation
    float4 ix = to_float4(Pi.x, Pi.z, Pi.x, Pi.z);
    float4 iy = to_float4(Pi.y, Pi.y, Pi.w, Pi.w);
    float4 fx = to_float4(Pf.x, Pf.z, Pf.x, Pf.z);
    float4 fy = to_float4(Pf.y, Pf.y, Pf.w, Pf.w);

    float4 i = permute(permute(ix) + iy);

    float4 gx = (float)(2) * fract_f4(i / (float)(41)) - (float)(1);
    float4 gy = abs_f4(gx) - (float)(0.5f);
    float4 tx = _floor(gx + (float)(0.5f));
    gx = gx - tx;

    float2 g00 = to_float2(gx.x, gy.x);
    float2 g10 = to_float2(gx.y, gy.y);
    float2 g01 = to_float2(gx.z, gy.z);
    float2 g11 = to_float2(gx.w, gy.w);

    float4 norm = taylorInvSqrt(to_float4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;

    float n00 = dot(g00, to_float2(fx.x, fy.x));
    float n10 = dot(g10, to_float2(fx.y, fy.y));
    float n01 = dot(g01, to_float2(fx.z, fy.z));
    float n11 = dot(g11, to_float2(fx.w, fy.w));

    float2 fade_xy = fade(to_float2(Pf.x, Pf.y));
    float2 n_x = _mix(to_float2(n00, n01), to_float2(n10, n11), fade_xy.x);
    float n_xy = _mix(n_x.x, n_x.y, fade_xy.y);
    return (float)(2.3f) * n_xy;
}

__DEVICE__ float hash(float n){
  return fract(_sinf(n) * 43758.5453f);
}

// From Shadertoy somewhere but not sure where originally.
__DEVICE__ float noise(in float3 x){
  float3 p = _floor(x);
  float3 f = fract_f3(x);

  f = f*f*(3.0f - 2.0f * f);
  float n = p.x + p.y*57.0f + 113.0f*p.z;
  return _mix(
         _mix(  _mix(hash(n + 0.0f), hash(n + 1.0f), f.x),
                _mix(hash(n + 57.0f), hash(n + 58.0f), f.x),
              f.y),
         _mix(
         _mix(hash(n + 113.0f), hash(n + 114.0f), f.x),
         _mix(hash(n + 170.0f), hash(n + 171.0f), f.x),
         f.y),
         f.z);
}

//float TILES = 1.0f;
#define TILES  1.0f

__DEVICE__ float worley(float3 pos, float numCells){
  float3 p = pos * numCells;
  float d = 1.0e10;
  for (int _x = -1; _x <= 1; _x++){
    for (int _y = -1; _y <= 1; _y++){
      for (int _z = -1; _z <= 1; _z++){
                float3 tp = _floor(p) + to_float3(_x, _y, _z);
                tp = p - tp - noise(mod_f3(tp, numCells / TILES));
                d = _fminf(d, dot(tp, tp));
            }
        }
    }
  return 1.0f - clamp(d, 0.0f, 1.0f);
}

__DEVICE__ float fbm(float2 pos, float2 scale){
    float res = 0.0f;
    float freq = 1.0f;
    float amp = 1.0f;
    float sum = 0.0f;
    
    int limit = 5;
    
    for(int i = 0; i < limit; i++){ 
        float offset = (float)(limit-i);
        float d = mod_f((float)(i), 2.0f) > 0.0f ? 1.0f : -1.0f;
        res += d * (perlin(freq*(pos+offset), freq*scale)) * amp;

        freq *= 2.0f;
        amp *= 0.5f;
    }
    return res/(float)(limit);
}


__KERNEL__ void OceanElementalJipi494Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;
   
    float2 uv = fragCoord/iResolution;
    float3 col = to_float3_s(0);
    
    //bool resolutionChanged = texelFetch(iChannel0, to_int2(0.5f, 2.5f), 0).r > 0.0f;
    bool resolutionChanged = texture(iChannel0, (make_float2((int)0.5f, (int)2.5f)+0.5f)/iResolution).x > 0.0f;
    
    if(iFrame == 0 || resolutionChanged){

        // Use diffrent scales for X and Y to get long wave fronts
        float2 scale = to_float2(8.0f, 15.0f);

        // For seamless texture, UV scale has to match rep
        float noise = perlin(scale*uv, scale);
        noise = 0.5f+0.5f*(fbm(scale*uv, scale));

        float s = 2.0f;
        float _worley = worley(s * to_float3_aw(uv, 0.0f), s);
        
        col = to_float3(noise, _worley, 0.0f);
        
    }else{
        //col = texelFetch(iChannel1, to_int2(fragCoord), 0).rgb;
        col = swi3(texture(iChannel1, (make_float2((int)fragCoord.x,(int)fragCoord.y)+0.5f)/iResolution),x,y,z);
    }
    
    // Output to screen
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


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
#define ZERO 0 //(_fminf(iFrame,0))

// Comment out to remove environment map
#define CUBEMAP

// Index of refraction for water
#define IOR 1.333f

// Ratios of air and water IOR for refraction
// Air to water
#define ETA 1.0f/IOR
// Water to air
#define ETA_REVERSE IOR

//const int MAX_STEPS = 50;
#define MAX_STEPS  50
//const float MIN_DIST = 0.01f;
#define MIN_DIST  0.01f
//const float MAX_DIST = 5.0f;
#define MAX_DIST  5.0f
#define EPSILON 1e-4f
#define DETAIL_EPSILON  2e-3f

//const float DETAIL_HEIGHT = 0.1f;
#define DETAIL_HEIGHT  0.1f
//const float3 DETAIL_SCALE = to_float3_s(1.0f);
#define DETAIL_SCALE  to_float3_s(1.0f)
//const float3 BLENDING_SHARPNESS = to_float3_s(4.0f);
#define BLENDING_SHARPNESS to_float3_s(4.0f)

/*
const float3 sunLightColour = to_float3_s(3.5f);

float3 waterColour = 0.85f * to_float3(0.1f, 0.75f, 0.9f);

// Amount of the background visible through the water
const float CLARITY = 0.75f;

// Modifiers for light attenuation
const float DENSITY = 3.5f;
const float DENSITY_POW = 1.0f;

// In a circle of 2*PI
const float sunLocation = -2.0f;
const float sunHeight = 0.9f;
*/

__DEVICE__ float3 rayDirection(float fieldOfView, float2 fragCoord, float2 iResolution) {
    float2 xy = fragCoord - iResolution / 2.0f;
    float z = (0.5f * iResolution.y) / _tanf(radians(fieldOfView) / 2.0f);
    return normalize(to_float3_aw(xy, -z));
}

// https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/
__DEVICE__ mat3 lookAt(float3 camera, float3 at, float3 up){
  float3 zaxis = normalize(at-camera);    
  float3 xaxis = normalize(cross(zaxis, up));
  float3 yaxis = cross(xaxis, zaxis);

  return to_mat3_f3(xaxis, yaxis, -zaxis);
}


__DEVICE__ float3 getSkyColour(float3 rayDir, __TEXTURE2D__ iChannel2){
    float3 col;
#ifdef CUBEMAP
    col = inv_gamma(swi3(decube_f3(iChannel2,rayDir),x,y,z));
    // Add some bloom to the environment
    col += 2.0f * pow_f3(col, to_float3_s(2));
#else 
    col = 0.5f*(0.5f+0.5f*rayDir);
#endif
    return col;
}

__DEVICE__ float getGlow(float dist, float radius, float intensity){
    dist = _fmaxf(dist, 1e-6f);
  return _powf(radius/dist, intensity);  
}

//-------------------------------- Rotations --------------------------------

__DEVICE__ float3 _rotate(float3 p, float4 q){
  return 2.0f * cross(swi3(q,x,y,z), p * q.w + cross(swi3(q,x,y,z), p)) + p;
}
__DEVICE__ float3 rotateX(float3 p, float angle){
    return _rotate(p, to_float4(_sinf(angle/2.0f), 0.0f, 0.0f, _cosf(angle/2.0f)));
}
__DEVICE__ float3 rotateY(float3 p, float angle){
  return _rotate(p, to_float4(0.0f, _sinf(angle/2.0f), 0.0f, _cosf(angle/2.0f)));
}
__DEVICE__ float3 rotateZ(float3 p, float angle){
  return _rotate(p, to_float4(0.0f, 0.0f, _sinf(angle), _cosf(angle)));
}


//---------------------------- Distance functions ----------------------------

// Distance functions and operators from:
// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

__DEVICE__ float displacement(float3 p){
    return _sinf(p.x)*_sinf(p.y)*_sinf(p.z);
}

__DEVICE__ float opDisplace(float3 p, float iTime){
    float3 offset = 0.4f*iTime * normalize(to_float3(1.0f, -1.0f, 0.1f));
    return displacement(10.0f*(p+offset));
}


__DEVICE__ float opSmoothSub( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); }

__DEVICE__ float sphereSDF(float3 p, float radius) {
    return length(p) - radius;
}

__DEVICE__ float sdRoundCone( float3 p, float r1, float r2, float h ){
  float2 q = to_float2( length(swi2(p,x,z)), p.y );
    
  float b = (r1-r2)/h;
  float a = _sqrtf(1.0f-b*b);
  float k = dot(q,to_float2(-b,a));
    
  if( k < 0.0f ) return length(q) - r1;
  if( k > a*h ) return length(q-to_float2(0.0f,h)) - r2;
        
  return dot(q, to_float2(a,b) ) - r1;
}

// https://www.iquilezles.org/www/articles/smin/smin.htm
__DEVICE__ float smoothMin(float a, float b, float k){
    float h = clamp(0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float getSDF(float3 p, float sdfSign, float iTime){

    p.y -= 0.4f;
    float dist = 1e5;
    float3 q = p;
    
    // Upper back
    dist = sphereSDF(q, 0.5f);
    
   
    // Head
    q.y -= 0.25f;
    q.x -= 0.45f;
    q = rotateZ(q, 0.39f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.25f, 0.25f, 0.1f), 0.25f);
    
    
    // Upper body
    // Two round cones for chest and shoulders
    q = p;
    q.z = _fabs(q.z);
    q.y += 0.1f;
    q.z -= 0.15f;
    q = rotateX(q, -1.45f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.4f, 0.35f, 0.4f), 0.25f);   
    
    
    // Lower body
    q = p;
    q.y += 0.5f;
    q.x += 0.15f;
    q = rotateZ(q, 1.4f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.35f, 0.25f, 0.7f), 0.5f);

    
    // Base
    // A large round cone
    q = p;
    q.y += 1.4f;
    q.x -= 0.1f;
    q = rotateZ(q, -1.5f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.24f, 2.0f, 3.0f), 0.25f);


    // Subtract a sphere from the base to make it flared
    q = p;
    q.y += 4.75f;
    
    dist = opSmoothSub(sphereSDF(q, 2.8f), dist, 0.15f);
     
    // Arms
    q = p;
    q.z = _fabs(q.z);
    q.z -= 0.8f;
    q.y += 0.3f;
    q = rotateZ(q, -1.7f);
    q = rotateX(q, -0.2f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.22f, 0.2f, 0.3f), 0.15f);
    
    // Forearms
    q = p;
    q.z = _fabs(q.z);
    q.z -= 0.9f;
    q.y += 0.8f;
    q.x -= 0.15f;
    q = rotateZ(q, -2.0f);
    q = rotateX(q, 0.15f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.18f, 0.18f, 0.3f), 0.1f);
  
  
    // Fists
    q = p;
    q.z = _fabs(q.z);
    q.z -= 0.77f;
    q.y += 0.95f;
    q.x -= 0.55f;
    q = rotateZ(q, PI*0.6f);
    
    dist = smoothMin(dist, sdRoundCone(q, 0.1f, 0.1f, 0.2f), 0.15f);
   
   
    float height = p.y+0.4f;
    // Displace the surface for larger waves
    // Add more displacement lower down
    float strength = _mix(0.02f, 0.1f, smoothstep(-0.6f, -1.5f, height));
    if(height < -1.5f){
        // No displacement at the very bottom
        strength = _mix(strength, 0.0f, smoothstep(-1.5f, -1.62f, height));
    }
    dist += strength * opDisplace(p,iTime);
    
    return sdfSign * dist;
}

__DEVICE__ float distanceToScene(float3 cameraPos, float3 rayDir, float start, float end, float sdfSign, float iTime){
  
    // Start at a predefined distance from the camera in the ray direction
    float depth = start;
    
    // Variable that tracks the distance to the scene at the current ray endpoint
    float dist;
    
    // For a set number of steps
    for (int i = ZERO; i < MAX_STEPS; i++) {
        
        // Get the sdf value at the ray endpoint, giving the maximum 
        // safe distance we can travel in any direction without hitting a surface
        dist = getSDF(cameraPos + depth * rayDir, sdfSign,iTime);
        
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
__DEVICE__ float2 getGradient(float2 uv, __TEXTURE2D__ iChannel1){

    float delta = 1e-1;
    uv *= 0.3f;
    
    float data = _tex2DVecN(iChannel1,uv.x,uv.y,15).x;
    float gradX = data - texture(iChannel1, uv-to_float2(delta, 0.0f)).x;
    float gradY = data - texture(iChannel1, uv-to_float2(0.0f, delta)).x;
    
    return to_float2(gradX, gradY);
}

// https://catlikecoding.com/unity/tutorials/flow/texture-distortion/
__DEVICE__ float getDistortedTexture(float2 uv, __TEXTURE2D__ iChannel1, float iTime){

    float strength = 0.5f;
    
    // The texture is distorted in time and we switch between two texture states.
    // The transition is based on Worley noise which will shift the change of differet parts
    // for a more organic result
    float time = 0.5f * iTime + texture(iChannel1, 0.25f*uv).y;
    
    float f = fract(time);
    
    // Get the velocity at the current location
    float2 grad = getGradient(uv,iChannel1);
    uv *= 1.0f;
    float2 distortion = strength*to_float2(grad.x, grad.y) + to_float2(0, -0.3f);

    // Get two shifted states of the texture distorted in time by the local velocity.
    // Loop the distortion from 0 -> 1 using fract(time)
    float distort1 = texture(iChannel1, uv + f * distortion).x;
    float distort2 = texture(iChannel1, uv + fract(time + 0.5f) * distortion).x;

    // Mix between the two texture states to hide the sudden jump from 1 -> 0.
    // Modulate the value returned by the velocity.
    return (1.0f-length(grad)) * (_mix(distort1, distort2, _fabs(1.0f - 2.0f * f)));
}

//----------------------------- Normal mapping -----------------------------

// https://tinyurl.com/y5ebd7w7
__DEVICE__ float3 getTriplanar(float3 position, float3 normal, __TEXTURE2D__ iChannel1, float iTime){

    // A hack to get the flow direction on the arms to be consistent
    float2 xpos = swi2(position,z,x);
    if(_fabs(position.z) > 0.65f){
        // If position is below 0.0f, flip the uv direction for upwards flow
        xpos = _mix(xpos, to_float2(position.z, -position.x), smoothstep(-0.0f, -0.2f, position.y));
    }

    float3 xaxis = to_float3_s(getDistortedTexture(DETAIL_SCALE.x*(swi2(position,z,y)),iChannel1,iTime));
    float3 yaxis = to_float3_s(getDistortedTexture(DETAIL_SCALE.y*(xpos),iChannel1,iTime));
    float3 zaxis = to_float3_s(getDistortedTexture(DETAIL_SCALE.z*(swi2(position,x,y)),iChannel1,iTime));

    float3 blending = abs_f3(normal);
    blending = normalize(_fmaxf(blending, to_float3_s(0.00001f)));
    blending = pow_f3(blending, BLENDING_SHARPNESS);
    blending = pow_f3(blending, BLENDING_SHARPNESS);
    float b = (blending.x + blending.y + blending.z);
    blending /= b;

    return  xaxis * blending.x + 
            yaxis * blending.y + 
            zaxis * blending.z;
}

// Return the position of p extruded in the normal direction by a normal map
__DEVICE__ float3 getDetailExtrusion(float3 p, float3 normal, __TEXTURE2D__ iChannel1, float iTime){

    float detail = DETAIL_HEIGHT * length(getTriplanar(p, normal,iChannel1,iTime));
    
    // Increase the normal extrusion height on the upper body
    float d = 1.0f + smoothstep(0.0f, -0.5f, p.y);
    return p + d * detail * normal;
}

// Tetrahedral normal technique with a loop to avoid inlining getSDF()
// This should improve compilation times
// https://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
__DEVICE__ float3 getNormal(float3 p, float sdfSign, float iTime){
    float3 n = to_float3_s(0.0f);
    int id;
    for(int i = ZERO; i < 4; i++){
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*getSDF(p+e*EPSILON, sdfSign,iTime);
    }
    return normalize(n);
}

// Get orthonormal basis from surface normal
// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
__DEVICE__ void pixarONB(float3 n, out float3 *b1, out float3 *b2){
  float sign_ = sign_f(n.z);
  float a = -1.0f / (sign_ + n.z);
  float b = n.x * n.y * a;
  *b1 = to_float3(1.0f + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
  *b2 = to_float3(b, sign_ + n.y * n.y * a, -n.y);
}

// Return the normal after applying a normal map
__DEVICE__ float3 getDetailNormal(float3 p, float3 normal, __TEXTURE2D__ iChannel1, float iTime){

    float3 tangent;
    float3 bitangent;
    
    // Construct orthogonal directions tangent and bitangent to sample detail gradient in
    pixarONB(normal, &tangent, &bitangent);
    
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);

    float3 delTangent = to_float3_s(0);
    float3 delBitangent = to_float3_s(0);
    
    for(int i = ZERO; i < 2; i++){
        
        //i to  s
        //0 ->  1
        //1 -> -1
        float s = 1.0f - 2.0f * (float)(i&1);
    
        delTangent += s * getDetailExtrusion(p + s * tangent * DETAIL_EPSILON, normal,iChannel1,iTime);
        delBitangent += s * getDetailExtrusion(p + s * bitangent * DETAIL_EPSILON, normal,iChannel1,iTime);

    }
    
    return normalize(cross(delTangent, delBitangent));
}

//--------------------------------- PBR ---------------------------------

// Trowbridge-Reitz
__DEVICE__ float distribution(float3 n, float3 h, float roughness){
  float a_2 = roughness*roughness;
  return a_2/(PI*_powf(pow(dot_c(n, h),2.0f) * (a_2 - 1.0f) + 1.0f, 2.0f));
}

// GGX and Schlick-Beckmann
__DEVICE__ float geometry(float cosTheta, float k){
  return (cosTheta)/(cosTheta*(1.0f-k)+k);
}

__DEVICE__ float smiths(float3 n, float3 viewDir, float3 lightDir, float roughness){
    float k = _powf(roughness + 1.0f, 2.0f)/8.0f; 
  return geometry(dot_c(n, lightDir), k) * geometry(dot_c(n, viewDir), k);
}

// Fresnel-Schlick
__DEVICE__ float3 fresnel(float cosTheta, float3 F0){
    return F0 + (1.0f - F0) * _powf(1.0f - cosTheta, 5.0f);
} 

// Specular part of Cook-Torrance BRDF
__DEVICE__ float3 BRDF(float3 p, float3 n, float3 viewDir, float3 lightDir, float3 F0, float roughness){
    float3 h = normalize(viewDir + lightDir);
    float cosTheta = dot_c(h, viewDir);
    float D = distribution(n, h, roughness);
    float3 F = fresnel(cosTheta, F0);
    float G = smiths(n, viewDir, lightDir, roughness);
    
    float3 specular =  D * F * G / _fmaxf(0.0001f, (4.0f * dot_c(lightDir, n) * dot_c(viewDir, n)));
    
    return specular;
}

// From the closest intersection with the scene, raymarch the negative SDF field to 
// find the far instersection. The distance inside the water is used to determine 
// transmittance and the attenuation of the environment.
__DEVICE__ float3 getEnvironment(float3 org, float3 rayDir, out float3 *transmittance, out float3 *halfwayPoint, __TEXTURE2D__ iChannel2,
                                 float DENSITY, float DENSITY_POW, float3 waterColour, float iTime ){
        float sdfSign = -1.0f;
        
        float distFar = distanceToScene(org, rayDir, MIN_DIST, MAX_DIST, sdfSign,iTime);
        
        float3 positionFar = org + rayDir * distFar;
        *halfwayPoint = org + rayDir * distFar * 0.5f;
        float3 geoNormalFar = getNormal(positionFar, sdfSign,iTime);

        // Avoid artefacts when trying to sample detail normals across Z-plane. Shape 
        // deformation increases the region where visible errors occur.
        if(_fabs(geoNormalFar.z) < 1e-5){
            geoNormalFar.z = 1e-5;
        }

        //Use the geometry normal on the far side to reduce noise
        float3 refractedDir = normalize(refract_f3(rayDir, geoNormalFar, ETA_REVERSE));

        // When total internal reflection occurs, reflect the ray off the far side
        // Critical angle for 1.333f -> 1.0f is 0.8483
        // _cosf(0.8483f) = 0.66125
        if(dot(-rayDir, geoNormalFar) <= 0.66125f){
            refractedDir = normalize(reflect(rayDir, geoNormalFar));
        }

        float3 transmitted = getSkyColour(refractedDir, iChannel2);
        
        // View depth
        float d = DENSITY*length(org-positionFar);
        
        if(DENSITY_POW != 1.0f){
            d = _powf(d, DENSITY_POW);
        }
        
        
        // Beer's law depending on the water colour
        *transmittance = exp_f3( -d * (1.0f - waterColour));
        
        float3 result = transmitted * *transmittance;
        return result;
}

__DEVICE__ float getLightDepth(float3 org, float3 rayDir, float iTime){
    float sdfSign = -1.0f;
        
    return distanceToScene(org, rayDir, MIN_DIST, MAX_DIST, sdfSign,iTime);
}


//------------------------------- Shading -------------------------------

__DEVICE__ float HenyeyGreenstein(float g, float costh){
  return (1.0f/(FOUR_PI))  * ((1.0f - g * g) / _powf(1.0f + g*g - 2.0f*g*costh, 1.5f));
}

__DEVICE__ float3 shadingPBR(float3 cameraPos, float3 lightPos, float3 p, float3 n, float3 rayDir, float3 geoNormal, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2,
                             float3 sunLightColour, float CLARITY, float DENSITY, float DENSITY_POW, float3 waterColour, float iTime){
    float3 I = to_float3_s(0);

    float3 F0 = to_float3_s(0.02f);
    float roughness = 0.1f;

    float3 vectorToLight = lightPos - p;
     float3 lightDir = normalize(vectorToLight);
    I +=  BRDF(p, n, -rayDir, lightDir, F0, roughness) 
        * sunLightColour 
        * dot_c(n, lightDir);
        

    float3 transmittance;
    float3 halfwayPoint;
    
    float f = smoothstep(0.0f, -0.5f, p.y);
    
    float3 result = to_float3_s(0);
    
    result += (1.0f-f) * CLARITY * getEnvironment(p+rayDir*2.0f*EPSILON,
                                                  refract_f3(rayDir, n, ETA), 
                                                  &transmittance, &halfwayPoint,iChannel2,
                                                  DENSITY, DENSITY_POW, waterColour, iTime);
    
   
    float mu = dot(refract_f3(rayDir, n, ETA), lightDir);
    float phase = _mix(HenyeyGreenstein(-0.3f, mu), HenyeyGreenstein(0.85f, mu), 0.5f);
    
    #ifdef HQ_LIGHT
    
    float lightDepth = getLightDepth(p+rayDir*2.0f*EPSILON, normalize(lightPos-halfwayPoint,iTime));
    lightDepth = _fmaxf(1.0f, lightDepth);

    float3 lightTransmittance = _expf(-DENSITY*lightDepth * (1.0f - waterColour));
    
    result += CLARITY * sunLightColour * lightTransmittance * phase;
    #else
    
    result += CLARITY * sunLightColour * transmittance * phase;
    
    #endif
    
    // Reflection of the environment.
    float3 reflectedDir = normalize(reflect(rayDir, n));
    float3 reflectedCol = getSkyColour(reflectedDir,iChannel2);
    
    float cosTheta = dot_c(n, -rayDir);
    float3 F = fresnel(cosTheta, F0);
    
    result = mix_f3(result, reflectedCol, F);
    
    // Foam based on wave height
    float waveHeight = length(getTriplanar(p, n, iChannel1,iTime));

    // Sharper crests higher up
    float e = _mix(2.0f, 16.0f, 1.0f-smoothstep(0.2f, -1.3f, p.y));
    
    result += f * _powf(waveHeight, e);

    return result + I;
}

//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
__DEVICE__ float3 ACESFilm(float3 x){
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0f, 1.0f);
}

__KERNEL__ void OceanElementalJipi494Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f; 
        
    const float3 sunLightColour = to_float3_s(3.5f);

    float3 waterColour = 0.85f * to_float3(0.1f, 0.75f, 0.9f);

    // Amount of the background visible through the water
    const float CLARITY = 0.75f;

    // Modifiers for light attenuation
    const float DENSITY = 3.5f;
    const float DENSITY_POW = 1.0f;

    // In a circle of 2*PI
    const float sunLocation = -2.0f;
    const float sunHeight = 0.9f;    
        
    float3 col = to_float3_s(0);
    
  //----------------- Define a camera -----------------
    
    float3 rayDir = rayDirection(60.0f, fragCoord, iResolution);

    //vec3 cameraPos = texelFetch(iChannel0, to_int2(0.5f, 1.5f), 0).xyz;
    float3 cameraPos = swi3(texture(iChannel0, (make_float2((int)0.5f, (int)1.5f)+0.5f)/iResolution),x,y,z);

    float3 targetDir = -cameraPos;

    float3 up = to_float3(0.0f, 1.0f, 0.0f);

    // Get the view matrix from the camera orientation.
    mat3 viewMatrix = lookAt(cameraPos, targetDir, up);

    // Transform the ray to point in the correct direction.
    rayDir = normalize(mul_mat3_f3(viewMatrix , rayDir));

    //---------------------------------------------------
  
    float3 lightPos = 100.0f * normalize(to_float3(_cosf(sunLocation), sunHeight, _sinf(sunLocation)));
    float3 lightDirection = normalize(lightPos);
    
    float sdfSign = 1.0f;
    
    // Find the distance to where the ray stops.
    float dist = distanceToScene(cameraPos, rayDir, MIN_DIST, MAX_DIST, sdfSign,iTime);
    
    if(dist < MAX_DIST){
    
    
    float3 position = cameraPos + rayDir * dist;
        
        float3 geoNormal = getNormal(position, sdfSign,iTime);

        // Avoid artefacts when trying to sample detail normals across Z-plane. Shape 
        // deformation increases the region where visible errors occur.
        if(_fabs(geoNormal.z) < 1e-5){
            geoNormal.z = 1e-5;
        }

        float3 detailNormal = normalize(getDetailNormal(position, geoNormal,iChannel1,iTime));
        
        col += shadingPBR(cameraPos, lightPos, position, detailNormal, rayDir, geoNormal,iChannel1,iChannel2,
                          sunLightColour,CLARITY,DENSITY,DENSITY_POW,waterColour,iTime);

        
    }else{
    
        col += getSkyColour(rayDir,iChannel2);
        float mu = dot(rayDir, lightDirection);
        col += sunLightColour * getGlow(1.0f-mu, 0.0005f, 1.0f);
        
    }
    
    //col = to_float3(getDistortedTexture(fragCoord/iResolution,iChannel1,iTime));
    //col = to_float3(texture(iChannel1, fragCoord/iResolution, 0.0f).r);
    
    //Tonemapping.
    col = ACESFilm(col);

    //Gamma correction 1.0f/2.2f = 0.4545...
    col = pow_f3(col, to_float3_s(0.4545f));

    //Output to screen.
    fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}