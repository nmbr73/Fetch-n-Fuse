
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Star map shader...procedural space background

#define deg (3.1415927f / 180.0f)

// See derivation of noise functions by Morgan McGuire at https://www.shadertoy.com/view/4dS3Wd
//const int NUM_OCTAVES = 4;
#define NUM_OCTAVES 4

__DEVICE__ float hash(float n) { return fract(_sinf(n) * 1e4); }
__DEVICE__ float hash_f2(float2 p) { return fract(1e4 * _sinf(17.0f * p.x + p.y * 0.1f) * (0.1f + _fabs(_sinf(p.y * 13.0f + p.x)))); }
// 1 octave value noise
__DEVICE__ float noise(float x) { float i = _floor(x); float f = fract(x); float u = f * f * (3.0f - 2.0f * f); return _mix(hash(i), hash(i + 1.0f), u); }
__DEVICE__ float noise_f2(float2 x) { float2 i = _floor(x); float2 f = fract(x);  float a = hash_f2(i); float b = hash_f2(i + to_float2(1.0f, 0.0f)); float c = hash_f2(i + to_float2(0.0f, 1.0f)); float d = hash_f2(i + to_float2(1.0f, 1.0f)); float2 u = f * f * (3.0f - 2.0f * f); return _mix(a, b, u.x) + (c - a) * u.y * (1.0f - u.x) + (d - b) * u.x * u.y; }
__DEVICE__ float noise_f3(float3 x) { const float3 step = to_float3(110, 241, 171); float3 i = _floor(x); float3 f = fract(x); float n = dot(i, step); float3 u = f * f * (3.0f - 2.0f * f); return _mix(_mix(_mix( hash(n + dot(step, to_float3(0, 0, 0))), hash(n + dot(step, to_float3(1, 0, 0))), u.x), _mix( hash(n + dot(step, to_float3(0, 1, 0))), hash(n + dot(step, to_float3(1, 1, 0))), u.x), u.y), _mix(_mix( hash(n + dot(step, to_float3(0, 0, 1))), hash(n + dot(step, to_float3(1, 0, 1))), u.x), _mix( hash(n + dot(step, to_float3(0, 1, 1))), hash(n + dot(step, to_float3(1, 1, 1))), u.x), u.y), u.z); }
// Multi-octave value noise
__DEVICE__ float NOISE(float x) { float v = 0.0f; float a = 0.5f; float shift = (float)(100); for (int i = 0; i < NUM_OCTAVES; ++i) { v += a * noise(x); x = x * 2.0f + shift; a *= 0.5f; } return v; }
__DEVICE__ float NOISE_f2(float2 x) { float v = 0.0f; float a = 0.5f; float2 shift = to_float2_s(100.0f); mat2 rot = to_mat2(_cosf(0.5f), _sinf(0.5f), -_sinf(0.5f), _cosf(0.50f)); for (int i = 0; i < NUM_OCTAVES; ++i) { v += a * noise_f2(x); x = mul_mat2_f2(rot , x * 2.0f) + shift; a *= 0.5f; } return v; }
// Fast hash2 from https://www.shadertoy.com/view/lsfGWH
__DEVICE__ float hash2(float2 co) { return fract(_sinf(dot(swi2(co,x,y), to_float2(12.9898f,78.233f))) * 43758.5453f); }
__DEVICE__ float maxComponent(float2 v) { return _fmaxf(v.x, v.y); }
__DEVICE__ float maxComponent_f3(float3 v) { return _fmaxf(max(v.x, v.y), v.z); }
__DEVICE__ float minComponent(float2 v) { return _fminf(v.x, v.y); }
__DEVICE__ mat3 rotation(float yaw, float pitch) { return mul_mat3_mat3(to_mat3(_cosf(yaw), 0, -_sinf(yaw), 0, 1, 0, _sinf(yaw), 0, _cosf(yaw)) , to_mat3(1, 0, 0, 0, _cosf(pitch), _sinf(pitch), 0, -_sinf(pitch), _cosf(pitch))); }
__DEVICE__ float square(float x) { return x * x; }

///////////////////////////////////////////////////////////////////////

// Only globals needed for the actual spheremap

// starplane was derived from https://www.shadertoy.com/view/lsfGWH
__DEVICE__ float starplane(float3 dir, float screenscale, float iTime) {
    //float screenscale = 1.0f / iResolution.x;

    // Project to a cube-map plane and scale with the resolution of the display
    float2 basePos = swi2(dir,x,y) * (0.5f / screenscale) / _fmaxf(1e-3f, _fabs(dir.z));

  const float largeStarSizePixels = 20.0f;

    // Probability that a pixel is NOT on a large star. Must change with largeStarSizePixels
  const float prob = 0.97f;

  float color = 0.0f;
  float2 pos = _floor(basePos / largeStarSizePixels);
  float starValue = hash2(pos);

    // Big stars
  if (starValue > prob) {

        // Sphere blobs
    float2 delta = basePos - largeStarSizePixels * (pos + to_float2_s(0.5f));
    color = _fmaxf(1.0f - length(delta) / (0.5f * largeStarSizePixels), 0.0f);

        // Star shapes
        color *= 1.0f / _fmaxf(1e-3f, _fabs(delta.x) * _fabs(delta.y));

        // Avoid triplanar seams where star distort and clump
        color *= _powf(_fabs(dir.z), 12.0f);
    }

    // Small stars

    // Stabilize stars under motion by locking to a grid
    basePos = _floor(basePos);

    if (hash2(swi2(basePos,x,y) * screenscale) > 0.997f) {
        float r = hash2(swi2(basePos,x,y) * 0.5f);
        color += r * (0.3f * _sinf(iTime * (r * 5.0f) + r) + 0.7f) * 1.5f;
    }

    // Weight by the z-plane
    return color * _fabs(dir.z);
}


__DEVICE__ float starbox(float3 dir, float screenscale, float iTime) {
  return starplane(swi3(dir,x,y,z),screenscale,iTime) + starplane(swi3(dir,y,z,x),screenscale,iTime) + starplane(swi3(dir,z,x,y),screenscale,iTime);
}


__DEVICE__ float starfield(float3 dir, float screenscale, float iTime) {
    return starbox(dir,screenscale,iTime) + starbox(mul_mat3_f3(rotation(45.0f * deg, 45.0f * deg) , dir), screenscale, iTime);
}


__DEVICE__ float3 nebula(float3 dir) {
    float purple = _fabs(dir.x);
    float yellow = noise(dir.y);
    float3 streakyHue = to_float3(purple + yellow, yellow * 0.7f, purple);
    float3 puffyHue = to_float3(0.8f, 0.1f, 1.0f);

    float streaky = _fminf(1.0f, 8.0f * _powf(NOISE_f2(swi2(dir,y,z) * square(dir.x) * 13.0f + swi2(dir,x,y) * square(dir.z) * 7.0f + to_float2(150.0f, 2.0f)), 10.0f));
    float puffy = square(NOISE_f2(swi2(dir,x,z) * 4.0f + to_float2(30, 10)) * dir.y);

    return clamp(puffyHue * puffy * (1.0f - streaky) + streaky * streakyHue, 0.0f, 1.0f);
}


__DEVICE__ float3 sun(float3 d, float iTime) {
    float angle = _atan2f(d.x, d.y);
    float falloff = _powf(_fmaxf(d.z, 0.0f), 10.0f);
  float3 core = to_float3(2.8f, 1.5f + 0.5f * noise_f2(iTime * 0.25f + swi2(d,x,y) * 5.0f), 1.5f) * falloff;
    float corona = NOISE_f2(to_float2(d.z * 250.0f + iTime, iTime * 0.2f + angle * 50.0f)) * smoothstep(0.95f, 0.92f, d.z) * falloff * square(d.z);

    return core * (1.0f - corona);
}


__DEVICE__ float4 planet(float3 view, float iTime) {
    const float PLANET_RADIUS = 0.65f;
    if (view.y > -PLANET_RADIUS) {
        return to_float4_s(0.0f);
    }

    // Compute the point on the planet sphere
    // float angle  = _atan2f(view.x, view.z); unused
    float radius = _sqrtf((1.0f + view.y) / (1.0f - PLANET_RADIUS));

    float3 s = to_float3_aw(radius * normalize(swi2(view,x,z)), _sqrtf(1.0f - square(radius)));


    float3 dir = s;
    dir = mul_mat3_f3(rotation(0.0f, iTime * 0.01f) , dir);
    float latLongLine = 0.0f;// (1.0f - _powf(smoothstep(0.0f, 0.04f, _fminf(_fabs(fract(_atan2f(dir.y, length(swi2(dir,x,z))) / (15.0f * deg)) - 0.5f), _fabs(fract(_atan2f(dir.x, dir.z) / (15.0f * deg)) - 0.5f)) * 2.0f), 10.0f));

    // Antialias the edge of the planet
    float4 surface = to_float4_aw(1.2f * to_float3(1.0f, 0.3f, 0.4f) *
        (noise_f3(dir * 39.0f + 3.5f) * 0.5f + noise_f3(dir * 26.0f) + 2.0f * noise_f3(dir * 13.0f + 1.0f)) *
         //to_float3_aw(swi2(s,y,x) * 0.5f + 0.5f, 0.0f).rbg, smoothstep(0.992f, 0.988f, radius));
         to_float3(s.y*0.5f+0.5,s.x*0.5+0.5f,0.0f), smoothstep(0.992f, 0.988f, radius));

    // Keep the clouds above the planet
    float4 cloud = to_float4_aw(to_float3_s(1.5f),
                      smoothstep(1.0f, 0.995f, radius) *
                      square(NOISE_f2(to_float2(iTime * 0.1f, 0.0f) + swi2(dir,x,z) * 11.0f * square(dir.y) + swi2(dir,y,x) * 3.0f + swi2(dir,z,y) * 1.2f)));

    return to_float4_aw(
        _mix(swi3(surface,x,y,z), swi3(cloud,x,y,z), cloud.w) * (_fmaxf(0.1f, s.y) * to_float3_s(1.0f - latLongLine)),
        _fmaxf(surface.w, cloud.w));
}


__DEVICE__ float3 sphereColor(float3 dir, float screenscale, float iTime, bool SHOW_PLANET) {
    float3 n = nebula(dir);
    float4 p;
    if (SHOW_PLANET)
      p= planet(dir,iTime);
    else
      p= to_float4_s(0.0f);

    float3 color =
        sun(dir,iTime) +
        _mix(to_float3_s(starfield(dir,screenscale,iTime)) * (1.0f - maxComponent_f3(n)) +  // Nebula holds out star
          n, // nebula
            swi3(p,x,y,z), p.w); // planet

  return color;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Spheremap visualization code from https://www.shadertoy.com/view/4sSXzG

__KERNEL__ void StarMap1Fuse(float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  CONNECT_CHECKBOX0(SHOW_LARGE_GRID,false);
  CONNECT_CHECKBOX1(SHOW_SPHERE,true);
  CONNECT_CHECKBOX2(SHOW_SPHERE_GRID,true);
  CONNECT_CHECKBOX3(SHOW_PLANET,true);

  float3 fragColorRGB;

  float scale = 1.0f / _fminf(iResolution.x, iResolution.y);
  float screenscale = 1.0f / iResolution.x;


  // Of the background
  const float verticalFieldOfView = 60.0f * deg;
  const float insetSphereRadius = 0.22f;

  float yaw   = -((iMouse.x / iResolution.y) * 2.0f - 1.0f) * 3.0f;
  float pitch = ((iMouse.y / iResolution.y) * 2.0f - 1.0f) * 3.0f;

  float3 dir = mul_mat3_f3(rotation(yaw, pitch) , normalize(to_float3_aw(fragCoord - iResolution / 2.0f, iResolution.y / ( -2.0f * _tanf(verticalFieldOfView / 2.0f)))));

  fragColorRGB = sphereColor(dir,screenscale,iTime,SHOW_PLANET);
  if (SHOW_LARGE_GRID) {
    float latLongLine = (1.0f - _powf(smoothstep(0.0f, 0.04f, _fminf(_fabs(fract(_atan2f(dir.y, length(swi2(dir,x,z))) / (15.0f * deg)) - 0.5f), _fabs(fract(_atan2f(dir.x, dir.z) / (15.0f * deg)) - 0.5f)) * 2.0f), 10.0f));
      fragColorRGB += latLongLine * to_float3(0.0f, 0.7f, 1.5f);
  }


  if (SHOW_SPHERE) {
    // Inset sphere
    float2 spherePoint = (fragCoord * scale - insetSphereRadius * 1.1f) / insetSphereRadius;
    if (length(spherePoint) <= 1.0f) {

        // Antialias using many samples
        float3 c = to_float3_s(0.0f);
        for (int x = -3; x <= 3; ++x) {
          for (int y = -3; y <= 3; ++y) {
            float2 s = clamp(((fragCoord + to_float2((float)x, (float)y) / 7.0f) * scale - insetSphereRadius * 1.1f) / insetSphereRadius, to_float2_s(-1.0f), to_float2_s(1.0f));
            dir = mul_mat3_f3(rotation(iTime, -iTime * 0.17f) , to_float3_aw(swi2(s,x,y), _sqrtf(_fmaxf(0.0f, 1.0f - dot(swi2(s,x,y), swi2(s,x,y))))));
            c += sphereColor(dir,screenscale,iTime,SHOW_PLANET);

            if (SHOW_SPHERE_GRID) {
              float latLongLine = (1.0f - _powf(smoothstep(0.0f, 0.04f, _fminf(_fabs(fract(_atan2f(dir.y, length(swi2(dir,x,z))) / (15.0f * deg)) - 0.5f), _fabs(fract(_atan2f(dir.x, dir.z) / (15.0f * deg)) - 0.5f)) * 2.0f), 10.0f));
                c += latLongLine * to_float3(0.0f, 0.7f, 1.5f);
            }
          }
        }
        c /= 36.0f;

        // Fade the inset sphere to antialias its border transition
        fragColorRGB = _mix(sqrt_f3(fragColorRGB), c, clamp((1.0f - length(spherePoint)) * 100.0f, 0.0f, 1.0f));
    }
  }

  fragColorRGB = sqrt_f3(fragColorRGB);

  SetFragmentShaderComputedColor(to_float4_aw(fragColorRGB,1.0f));
}