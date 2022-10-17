// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0
// Connect Image 'Texture: Organic 4' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
  Fast Thin-Film Interference

  This is a performance-optimized version of my previous 
  thin-film interference shader here: https://www.shadertoy.com/view/XddXRj
  This version also fixes a platform-specific bug and has
  a few other tweaks as well.

  Thin-film interference and chromatic dispersion are simulated at
  six different wavelengths and then downsampled to RGB.
*/

// To see just the reflection (no refraction/transmission) uncomment this next line:
//#define REFLECTANCE_ONLY

// performance and raymarching options
#define INTERSECTION_PRECISION 0.01f  // raymarcher intersection precision
#define ITERATIONS 20         // max number of iterations
#define AA_SAMPLES 1         // anti aliasing samples
#define BOUND 6.0f           // cube bounds check
#define DIST_SCALE 0.9f          // scaling factor for raymarching position update

// optical properties
#define DISPERSION 0.05f         // dispersion amount
#define IOR 0.9f              // base IOR value specified as a ratio
#define THICKNESS_SCALE 32.0f     // film thickness scaling factor
#define THICKNESS_CUBEMAP_SCALE 0.1f  // film thickness cubemap scaling factor
#define REFLECTANCE_SCALE 3.0f        // reflectance scaling factor
#define REFLECTANCE_GAMMA_SCALE 2.0f  // reflectance gamma scaling factor
#define FRESNEL_RATIO 0.7f       // fresnel weight for reflectance
#define SIGMOID_CONTRAST 8.0f         // contrast enhancement

#define TWO_PI 6.28318530718f
#define WAVELENGTHS 6         // number of wavelengths, not a free parameter

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

__DEVICE__ float noise( in float3 _x ) {
    float3 p = _floor(_x);
    float3 f = fract_f3(_x);

    f = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;
    return _mix(_mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                     _mix( hash(n+ 57.0f), hash(n+ 58.0f),f.x),f.y),
                _mix(_mix( hash(n+113.0f), hash(n+114.0f),f.x),
                     _mix( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
}

__DEVICE__ float3 noise3(float3 _x) {
  return to_float3( noise(_x+to_float3(123.456f,0.567f,0.37f)),
                    noise(_x+to_float3(0.11f,47.43f,19.17f)),
                    noise(_x) );
}

// a sphere with a little bit of warp
__DEVICE__ float sdf( float3 p, float iTime ) {
  float3 n = to_float3(_sinf(iTime * 0.5f), _sinf(iTime * 0.3f), _cosf(iTime * 0.2f));
  float3 q = 0.1f * (noise3(p + n) - 0.5f);
  
  return length(q + p) - 3.5f;
}

__DEVICE__ float3 fresnel( float3 rd, float3 norm, float3 n2 ) {
   float3 r0 = pow_f3((1.0f-n2)/(1.0f+n2), to_float3_s(2));
   
  if (isnan(r0.x))  r0.x = 0.0001f;
  if (isnan(r0.y))  r0.y = 0.0001f;
  if (isnan(r0.z))  r0.z = 0.0001f;
   
   return r0 + (1.0f - r0)*_powf(clamp(1.0f + dot(rd, norm), 0.0f, 1.0f), 5.0f);
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime ) {
    const float eps = INTERSECTION_PRECISION;

    const float3 v1 = to_float3( 1.0f,-1.0f,-1.0f);
    const float3 v2 = to_float3(-1.0f,-1.0f, 1.0f);
    const float3 v3 = to_float3(-1.0f, 1.0f,-1.0f);
    const float3 v4 = to_float3( 1.0f, 1.0f, 1.0f);

  return normalize( v1*sdf( pos + v1*eps, iTime ) + 
                    v2*sdf( pos + v2*eps, iTime) + 
                    v3*sdf( pos + v3*eps, iTime) + 
                    v4*sdf( pos + v4*eps, iTime) );
}

#define GAMMA_CURVE 50.0f
#define GAMMA_SCALE 4.5f
__DEVICE__ float3 filmic_gamma(float3 _x) {
  return log_f3(GAMMA_CURVE * _x + 1.0f) / GAMMA_SCALE;    
}

__DEVICE__ float3 filmic_gamma_inverse(float3 _y) {
  return (1.0f / GAMMA_CURVE) * (exp_f3(GAMMA_SCALE * _y) - 1.0f); 
}

// sample weights for the cubemap given a wavelength i
// room for improvement in this function
#define GREEN_WEIGHT 2.8f
__DEVICE__ float3 texCubeSampleWeights(float i) {
  float3 w = to_float3((1.0f - i) * (1.0f - i), GREEN_WEIGHT * i * (1.0f - i), i * i);
  return w / dot(w, to_float3_s(1.0f));
}

__DEVICE__ float3 sampleCubeMap(float3 i, float3 rd, __TEXTURE2D__ iChannel0) {
  float3 col = swi3(decube_f3(iChannel0, rd * to_float3(1.0f,1.0f,1.0f)),x,y,z);  // to_float3(1.0f,-1.0f,1.0f)
  return to_float3(
                    dot(texCubeSampleWeights(i.x), col),
                    dot(texCubeSampleWeights(i.y), col),
                    dot(texCubeSampleWeights(i.z), col)
                  );
}

__DEVICE__ float3 sampleCubeMap(float3 i, float3 rd0, float3 rd1, float3 rd2, __TEXTURE2D__ iChannel0) {
  float3 col0 = swi3(decube_f3(iChannel0, rd0 * to_float3(1.0f,-1.0f,1.0f)),x,y,z);
  float3 col1 = swi3(decube_f3(iChannel0, rd1 * to_float3(1.0f,-1.0f,1.0f)),x,y,z); 
  float3 col2 = swi3(decube_f3(iChannel0, rd2 * to_float3(1.0f,-1.0f,1.0f)),x,y,z); 
  return to_float3(
                  dot(texCubeSampleWeights(i.x), col0),
                  dot(texCubeSampleWeights(i.y), col1),
                  dot(texCubeSampleWeights(i.z), col2)
                  );
}



__DEVICE__ float3 sampleWeights(float i) {
  return to_float3((1.0f - i) * (1.0f - i), GREEN_WEIGHT * i * (1.0f - i), i * i);
}

__DEVICE__ float3 resample(float3 wl0, float3 wl1, float3 i0, float3 i1) {
  float3 w0 = sampleWeights(wl0.x);
  float3 w1 = sampleWeights(wl0.y);
  float3 w2 = sampleWeights(wl0.z);
  float3 w3 = sampleWeights(wl1.x);
  float3 w4 = sampleWeights(wl1.y);
  float3 w5 = sampleWeights(wl1.z);
  
  return i0.x * w0 + i0.y * w1 + i0.z * w2
       + i1.x * w3 + i1.y * w4 + i1.z * w5;
}

// downsample to RGB
__DEVICE__ float3 resampleColor(float3 rds[WAVELENGTHS], float3 refl0, float3 refl1, float3 wl0, float3 wl1, __TEXTURE2D__ iChannel0) {
   
    #ifdef REFLECTANCE_ONLY
      float3 intensity0 = refl0;
      float3 intensity1 = refl1;
    #else
      float3 cube0 = sampleCubeMap(wl0, rds[0], rds[1], rds[2], iChannel0);
      float3 cube1 = sampleCubeMap(wl1, rds[3], rds[4], rds[5], iChannel0);
    
      float3 intensity0 = filmic_gamma_inverse(cube0) + refl0;
      float3 intensity1 = filmic_gamma_inverse(cube1) + refl1;
    #endif
    float3 col = resample(wl0, wl1, intensity0, intensity1);

    return 1.4f * filmic_gamma(col / (float)(WAVELENGTHS));
}

__DEVICE__ float3 resampleColorSimple(float3 rd, float3 wl0, float3 wl1, __TEXTURE2D__ iChannel0) {
    float3 cube0 = sampleCubeMap(wl0, rd, iChannel0);
    float3 cube1 = sampleCubeMap(wl1, rd, iChannel0);
    
    float3 intensity0 = filmic_gamma_inverse(cube0);
    float3 intensity1 = filmic_gamma_inverse(cube1);
    float3 col = resample(wl0, wl1, intensity0, intensity1);

    return 1.4f * filmic_gamma(col / (float)(WAVELENGTHS));
}

// compute the wavelength/IOR curve values.
__DEVICE__ float3 iorCurve(float3 _x) {
  return _x;
}

__DEVICE__ float3 attenuation(float filmThickness, float3 wavelengths, float3 normal, float3 rd) {
  
  return 0.5f + 0.5f * cos_f3(((THICKNESS_SCALE * filmThickness)/(wavelengths + 1.0f)) * dot(normal, rd));    
}

__DEVICE__ float3 contrast(float3 _x) {
  return 1.0f / (1.0f + exp_f3(-SIGMOID_CONTRAST * (_x - 0.5f)));    
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



__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}





__KERNEL__ void FastThinFilmInterferenceFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float4 iDate, sampler2D iChannel0, sampler2D iChannel1)
{
  
  
    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER3(FilmThickness, -1.0f, 10.0f, 1.0f);

    float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
    float4 m = to_float4_f2f2(swi2(iMouse,x,y)/iResolution, swi2(iMouse,z,w));

    // camera movement
    float3 ro, ta;
    doCamera( &ro, &ta, iTime, m );
    mat3 camMat = calcLookAtMatrix( ro, ta, 0.0f );

    float dh = (0.666f / iResolution.y);
    const float rads = TWO_PI / (float)(AA_SAMPLES);
    
    float3 col = to_float3_s(0.0f);
    
    float3 wavelengths0 = to_float3(1.0f, 0.8f, 0.6f);
    float3 wavelengths1 = to_float3(0.4f, 0.2f, 0.0f);
    float3 iors0 = IOR + iorCurve(wavelengths0) * DISPERSION;
    float3 iors1 = IOR + iorCurve(wavelengths1) * DISPERSION;
    
    float3 rds[WAVELENGTHS];
    
    float filmThickness;
    
    for (int samp = 0; samp < AA_SAMPLES; samp++) {
        float2 dxy = dh * to_float2(_cosf((float)(samp) * rads), _sinf((float)(samp) * rads));
        float3 rd = normalize(mul_mat3_f3(camMat , to_float3_aw(swi2(p,x,y) + dxy, 1.5f))); // 1.5f is the lens length
        float3 pos = ro;
        bool hit = false;
        for (int j = 0; j < ITERATIONS; j++) {
            float t = DIST_SCALE * sdf(pos, iTime);
            pos += t * rd;
            hit = t < INTERSECTION_PRECISION;
            //if ( clamp(pos, -BOUND, BOUND) != pos || hit ) {
              float3 clampPos = clamp(pos, -BOUND, BOUND);
              if ( clampPos.x != pos.x || clampPos.y != pos.y || clampPos.z != pos.z || hit ) {
                break;    
            }
        }
        
        if (hit) {
            float3 normal = calcNormal(pos, iTime);

            filmThickness = FilmThickness * fancyCube( iChannel1, normal, THICKNESS_CUBEMAP_SCALE, 0.0f ).x + 0.1f;

            float3 att0 = attenuation(filmThickness, wavelengths0, normal, rd);
            float3 att1 = attenuation(filmThickness, wavelengths1, normal, rd);

            float3 f0 = (1.0f - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0f / iors0);
            float3 f1 = (1.0f - FRESNEL_RATIO) + FRESNEL_RATIO * fresnel(rd, normal, 1.0f / iors1);

            float3 rrd = reflect(rd, normal);

            float3 cube0 = REFLECTANCE_GAMMA_SCALE * att0 * sampleCubeMap(wavelengths0, rrd, iChannel0);
            float3 cube1 = REFLECTANCE_GAMMA_SCALE * att1 * sampleCubeMap(wavelengths1, rrd, iChannel0);

            float3 refl0 = REFLECTANCE_SCALE * filmic_gamma_inverse(mix_f3(to_float3_s(0), cube0, f0));
            float3 refl1 = REFLECTANCE_SCALE * filmic_gamma_inverse(mix_f3(to_float3_s(0), cube1, f1));

            rds[0] = _refract_f3(rd, normal, iors0.x, refmul, refoff);
            rds[1] = _refract_f3(rd, normal, iors0.y, refmul, refoff);
            rds[2] = _refract_f3(rd, normal, iors0.z, refmul, refoff);
            rds[3] = _refract_f3(rd, normal, iors1.x, refmul, refoff);
            rds[4] = _refract_f3(rd, normal, iors1.y, refmul, refoff);
            rds[5] = _refract_f3(rd, normal, iors1.z, refmul, refoff);
float IIIIIIIIIIIIIIIIIIIIIII;    
            col += resampleColor(rds, refl0, refl1, wavelengths0, wavelengths1, iChannel0);
        } else {
          col += resampleColorSimple(rd, wavelengths0, wavelengths1, iChannel0);    
        }

    }
    
    col /= (float)(AA_SAMPLES);
     
    fragColor = to_float4_aw( contrast(col), 1.0f );
    
    //fragColor = to_float4(filmThickness,filmThickness,filmThickness, 1.0f );
    
  SetFragmentShaderComputedColor(fragColor);
}