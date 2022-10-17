
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel1
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
  Thin-Film Interference Bubble

  This shader simulates thin-film interference patterns with a reasonable degree of accuracy.
  Chromatic dispersion is also simulated, and both effects use more than 3 wavelengths of light
  to increase accuracy. An arbitrary number of wavelengths can be simulated, which are then
    downsampled to RGB, in a similar fashion to the human eye.

*/

/* 
  BUG NOTICE!
  On some platforms (lower-end Nvidia graphics seem to be the common factor)
  the reflectance is much brighter than it's supposed to be. If the bubble
  looks mostly white on your platform, uncomment this next line:
*/
#define ITS_TOO_BRIGHT

// To see just the reflection (no refraction/transmission) uncomment this next line:
//#define REFLECTANCE_ONLY

// performance and raymarching options
#define WAVELENGTHS 5         // number of rays of different wavelengths to simulate, should be >= 3
#define INTERSECTION_PRECISION 0.01f  // raymarcher intersection precision
#define MIN_INCREMENT 0.02f       // distance stepped when entering the surface of the distance field
#define ITERATIONS 50         // max number of iterations
#define MAX_BOUNCES 2         // max number of reflection/refraction bounces
#define AA_SAMPLES 1         // anti aliasing samples
#define BOUND 6.0f           // cube bounds check
#define DIST_SCALE 0.9f          // scaling factor for raymarching position update

// optical properties
#define DISPERSION 0.05f         // dispersion amount
#define IOR 0.9f              // base IOR value specified as a ratio
#define THICKNESS_SCALE 64.0f     // film thickness scaling factor
#define THICKNESS_CUBEMAP_SCALE 0.07f // film thickness cubemap scaling factor
#define REFLECTANCE_SCALE 5.0f        // reflectance scaling factor

#define TWO_PI 6.28318530718f
#define PI 3.14159265359f

// visualize the average number of bounces for each of the rays
//#define VISUALIZE_BOUNCES

// iq's cubemap function
__DEVICE__ float3 fancyCube( __TEXTURE2D__ sam, in float3 d, in float s, in float b )
{
    float3 colx = swi3(texture( sam, 0.5f + s*swi2(d,y,z)/d.x),x,y,z);
    float3 coly = swi3(texture( sam, 0.5f + s*swi2(d,z,x)/d.y),x,y,z);
    float3 colz = swi3(texture( sam, 0.5f + s*swi2(d,x,y)/d.z),x,y,z);
    
    float3 n = d*d;
    
    return (colx*n.x + coly*n.y + colz*n.z)/(n.x+n.y+n.z);
}

// iq's 3D noise function
__DEVICE__ float hash( float n ){
    return fract(_sinf(n)*43758.5453f);
}

__DEVICE__ float noise( in float3 x ) {
    float3 p = _floor(x);
    float3 f = fract_f3(x);

    f = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;
    return _mix(_mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                     _mix( hash(n+ 57.0f), hash(n+ 58.0f),f.x),f.y),
                _mix(_mix( hash(n+113.0f), hash(n+114.0f),f.x),
                     _mix( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
}

__DEVICE__ float3 noise3(float3 x) {
  return to_float3( noise(x+to_float3(123.456f,0.567f,0.37f)),
                    noise(x+to_float3(0.11f,47.43f,19.17f)),
                    noise(x) );
}


// a sphere with a little bit of warp
__DEVICE__ float sdf( float3 p, float iTime ) {
  float3 n = pow_f3(to_float3(_sinf(iTime * 0.5f), _sinf(iTime * 0.3f), _cosf(iTime * 0.2f)), to_float3_s(2.0f));
  
  if (isnan(n.x))  n.x = 0.0001f;
  if (isnan(n.y))  n.y = 0.0001f;
  if (isnan(n.z))  n.z = 0.0001f;
  
  float3 q = 0.1f * noise3(p + n);
  
  return length(q + p)-3.5f;
}

// Fresnel factor from TambakoJaguar Diamond Test shader here: https://www.shadertoy.com/view/XdtGDj
// see also: https://en.wikipedia.org/wiki/Schlick's_approximation
__DEVICE__ float fresnel( float3 ray, float3 norm, float n2 )
{
   float n1 = 1.0f;
   float angle = clamp(_acosf(-dot(ray, norm)), -3.14f/2.15f, 3.14f/2.15f);
   float r0 = _powf((n1-n2)/(n1+n2), 2.0f);
   float r = r0 + (1.0f - r0)*_powf(1.0f - _cosf(angle), 5.0f);
   return clamp(0.0f, 1.0f, r);
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime ) {
    const float eps = INTERSECTION_PRECISION;

    const float3 v1 = to_float3( 1.0f,-1.0f,-1.0f);
    const float3 v2 = to_float3(-1.0f,-1.0f, 1.0f);
    const float3 v3 = to_float3(-1.0f, 1.0f,-1.0f);
    const float3 v4 = to_float3( 1.0f, 1.0f, 1.0f);

  return normalize( v1*sdf( pos + v1*eps,iTime ) + 
                    v2*sdf( pos + v2*eps,iTime ) + 
                    v3*sdf( pos + v3*eps,iTime ) + 
                    v4*sdf( pos + v4*eps,iTime ) );
}

struct Bounce
{
    float3 position;
    float3 ray_direction;
    float attenuation;
    float reflectance;
    float ior;
    float bounces;
    float wavelength;
};
    
__DEVICE__ float sigmoid(float t, float t0, float k) {
    return 1.0f / (1.0f + _expf(-k*(t - t0)));  
}

#define GAMMA_CURVE 50.0f
#define GAMMA_SCALE 4.5f

__DEVICE__ float3 filmic_gamma(float3 x) {
  return log_f3(GAMMA_CURVE * x + 1.0f) / GAMMA_SCALE;    
}

__DEVICE__ float filmic_gamma_inverse(float y) {
  return (1.0f/GAMMA_CURVE) * (-1.0f + _expf(GAMMA_SCALE * y)); 
}

// sample weights for the cubemap given a wavelength i
// room for improvement in this function
__DEVICE__ float3 texCubeSampleWeights(float i) {
  float3 w = to_float3((1.0f - i) * (1.0f - i), 2.0f * i * (1.0f - i), i * i);
  return w / dot(w, to_float3_s(1.0f));
}

__DEVICE__ float sampleCubeMap(float i, float3 rd, __TEXTURE2D__ iChannel0) {
  float3 col = swi3(decube_f3(iChannel0, rd * to_float3(1.0f,1.0f,1.0f)),x,y,z);  // to_float3(1.0f,-1.0f,1.0f) führt zum horz. Spiegelbild 
  return dot(texCubeSampleWeights(i), col);
}

__DEVICE__ void doCamera( out float3 *camPos, out float3 *camTar, in float time, in float4 m ) {
    *camTar = to_float3(0.0f,0.0f,0.0f); 
    if (_fmaxf(m.z, m.w) <= 0.0f) {
      float an = 1.5f + _sinf(time * 0.05f) * 4.0f;
    *camPos = to_float3(6.5f*_sinf(an), 0.0f ,6.5f*_cosf(an));   
    } else {
      float an = 10.0f * m.x - 5.0f;
    *camPos = to_float3(6.5f*_sinf(an),10.0f * m.y - 5.0f,6.5f*_cosf(an)); 
    }
}

__DEVICE__ mat3 calcLookAtMatrix( in float3 ro, in float3 ta, in float roll )
{
    float3 ww = normalize( ta - ro );
    float3 uu = normalize( cross(ww,to_float3(_sinf(roll),_cosf(roll),0.0f) ) );
    float3 vv = normalize( cross(uu,ww));
    return to_mat3_f3( uu, vv, ww );
}

// MATLAB Jet color scheme
__DEVICE__ float3 jet(float x) {

   x = clamp(x, 0.0f, 1.0f);

   if (x < 0.25f) {
       return(to_float3(0.0f, 4.0f * x, 1.0f));
   } else if (x < 0.5f) {
       return(to_float3(0.0f, 1.0f, 1.0f + 4.0f * (0.25f - x)));
   } else if (x < 0.75f) {
       return(to_float3(4.0f * (x - 0.5f), 1.0f, 0.0f));
   } else {
       return(to_float3(1.0f, 1.0f + 4.0f * (0.75f - x), 0.0f));
   }
   
}

// 4PL curve fit to experimentally-determined values
__DEVICE__ float greenWeight() {
    float a = 4569547.0f;
    float b = 2.899324f;
    float c = 0.008024607f;
    float d = 0.07336188f;

    return d + (a - d) / (1.0f + _powf(log((float)(WAVELENGTHS))/c, b)) + 2.0f;    
}

// sample weights for downsampling to RGB. Ideally this would be close to the 
// RGB response curves for the human eye, instead I use a simple ad hoc solution here.
// Could definitely be improved upon.
__DEVICE__ float3 sampleWeights(float i) {
  return to_float3((1.0f - i) * (1.0f - i), greenWeight() * i * (1.0f - i), i * i);
}

// downsample to RGB
__DEVICE__ float3 resampleColor(Bounce bounces[WAVELENGTHS], __TEXTURE2D__ iChannel0) {
    float3 col = to_float3_s(0.0f);
    
    for (int i = 0; i < WAVELENGTHS; i++) {        
        float reflectance = bounces[i].reflectance;
        float index = (float)(i) / (float)(WAVELENGTHS - 1);
        float texCubeIntensity = filmic_gamma_inverse(
            clamp(bounces[i].attenuation * sampleCubeMap(index, bounces[i].ray_direction, iChannel0), 0.0f, 0.99f)
        );
      float intensity = texCubeIntensity + reflectance;
        col += sampleWeights(index) * intensity;
    }

    return 1.3f * filmic_gamma(2.0f * col / (float)(WAVELENGTHS));
}

// compute average number of bounces for the VISUALIZE_BOUNCES render mode
__DEVICE__ float avgBounces(Bounce bounces[WAVELENGTHS]) {
    float avg = 0.0f;
    
    for (int i = 0; i < WAVELENGTHS; i++) {        
         avg += bounces[i].bounces;;
    }

    return avg / (float)(WAVELENGTHS);
}

// compute the wavelength/IOR curve values.
__DEVICE__ float iorCurve(float x) {
  return x;
}

__DEVICE__ Bounce initialize(float3 ro, float3 rd, float i) {
    i = i / (float)(WAVELENGTHS - 1);
    float ior = IOR + iorCurve(1.0f - i) * DISPERSION;
    Bounce ret = {ro, rd, 1.0f, 0.0f, ior, 1.0f, i};
    return ret;//Bounce(ro, rd, 1.0f, 0.0f, ior, 1.0f, i); 
}

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}




__KERNEL__ void ThinFilmInterferenceBubbleFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float4 iDate, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);

    float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
    float4 m = to_float4_f2f2(swi2(iMouse,x,y)/iResolution, swi2(iMouse,z,w));



    // camera movement
    float3 ro, ta;
    doCamera( &ro, &ta, iTime, m );
    
    ta = to_float3_aw(ViewXY,ViewZ);
    
    mat3 camMat = calcLookAtMatrix( ro, ta, 0.0f );
    
    float dh = (0.5f / iResolution.y);
    const float rads = TWO_PI / (float)(AA_SAMPLES);
    
    Bounce bounces[WAVELENGTHS];
    
    float3 col = to_float3_s(0.0f);
    
    for (int samp = 0; samp < AA_SAMPLES; samp++) {
        float2 dxy = dh * to_float2(_cosf((float)(samp) * rads), _sinf((float)(samp) * rads));
        float3 rd = normalize(mul_mat3_f3(camMat , to_float3_aw(swi2(p,x,y) + dxy, 1.5f))); // 1.5f is the lens length

        for (int i = 0; i < WAVELENGTHS; i++) {
            bounces[i] = initialize(ro, rd, (float)(i));    
        }

        for (int i = 0; i < WAVELENGTHS; i++) {
            for (int j = 0; j < ITERATIONS; j++) {
                float td = DIST_SCALE * sdf(bounces[i].position, iTime);
                float t = _fabs(td);
                float sig = sign_f(td);
                
                float3 pos = bounces[i].position + t * bounces[i].ray_direction;
                if ( (sig > 0.0f && bounces[i].bounces > 1.0f) 
                    || int(bounces[i].bounces) >= MAX_BOUNCES 
                    || ( clamp(pos.x, -BOUND, BOUND) != pos.x || clamp(pos.y, -BOUND, BOUND) != pos.y || clamp(pos.z, -BOUND, BOUND) != pos.z) ) {
                  break;    
                } else if ( t < INTERSECTION_PRECISION ) {
                    float3 normal = calcNormal(pos,iTime);
float IIIIIIIIIIIIIIIIIII;
                    #ifdef REFLECTANCE_ONLY
                      bounces[i].attenuation = 0.0f;
                    #endif

                    float filmThickness = fancyCube( iChannel1, normal, THICKNESS_CUBEMAP_SCALE, 0.0f ).x + 0.1f;
                    
                    float attenuation = 0.5f + 0.5f * _cosf(((THICKNESS_SCALE * filmThickness)/(bounces[i].wavelength + 1.0f)) * dot(normal, bounces[i].ray_direction));
                    float ior = sig < 0.0f ? 1.0f / bounces[i].ior : bounces[i].ior;

                    // cubemap reflection
                    float f = fresnel(bounces[i].ray_direction, normal, 0.5f / ior);
                    float texCubeSample = attenuation * sampleCubeMap(bounces[i].wavelength, reflect(bounces[i].ray_direction, normal),iChannel0);
                    
                    #ifdef ITS_TOO_BRIGHT
                      bounces[i].reflectance += REFLECTANCE_SCALE * filmic_gamma_inverse(_mix(0.0f, 0.8f * texCubeSample - 0.1f, f));
                    #else
                      bounces[i].reflectance += REFLECTANCE_SCALE * filmic_gamma_inverse(_mix(0.0f, 4.0f * texCubeSample - 0.5f, f));
                    #endif

                    bounces[i].ray_direction = normalize(_refract_f3(bounces[i].ray_direction, normal, ior,refmul,refoff));
                    bounces[i].position = pos + MIN_INCREMENT * bounces[i].ray_direction;
                    bounces[i].bounces += 1.0f;
                } else {
                    bounces[i].position = pos;
                }
            }
        }

        #ifdef VISUALIZE_BOUNCES
          col += jet(avgBounces(bounces) / (float)(MAX_BOUNCES));
        #else
          col += resampleColor(bounces,iChannel0);
        #endif
    }
    
    col /= (float)(AA_SAMPLES);
     
    fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}