
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


// This SDF raymarching demo implements the closest representative point (CRP)
// method for approximating spherical area lights. It also features soft shadows,
// translucency, and various postprocessing effects like DOF, bloom, and FXAA.
//
// Author: Johan Nysjö

#define SETTINGS_KEY_LIGHT_INTENSITY 0.8f
#define SETTINGS_SKY_LIGHT_INTENSITY 1.2f
#define SETTINGS_EXPOSURE            2.0f
#define SETTINGS_TRANSLUCENCY_ENABLED 1
#define SETTINGS_DISPLAY_STEP_COUNT   0

#define MAX_NUM_PRIMARY_RAY_STEPS   100

#define PI 3.141592653f

struct Ray {
    float3 origin;
    float3 direction;
};

struct PerspectiveCamera {
    float3 eye;
    float3 up;
    float3 center;
    float fovy;
    float aspect;
};

struct SphereAreaLight {
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct HemisphereLight {
    float3 up;
    float3 sky_color;
    float3 ground_color;
    float intensity;
};

struct Material {
    float3 base_color;
    float metalness;
    float gloss;
    float wrap;
};

__DEVICE__ float3 srgb2lin(float3 color)
{
  return pow_f3(color, to_float3_s(2.2f));
}

__DEVICE__ Ray get_camera_ray(PerspectiveCamera camera, float2 uv)
{
    float3 f = normalize(camera.center - camera.eye);
    float3 s = normalize(cross(f, normalize(camera.up)));
    float3 u = normalize(cross(s, f));

    float half_height = _tanf(0.5f * camera.fovy);
    float half_width = camera.aspect * half_height;
    float x = 2.0f * uv.x - 1.0f;
    float y = 2.0f * uv.y - 1.0f;

    Ray ray;
    ray.origin = camera.eye;
    ray.direction = normalize(f + x * half_width * s + y * half_height * u);
    
    return ray;
}

// Polynomial smooth min operation for SDFs
// Reference: http://www.iquilezles.org/www/articles/smin/smin.htm
__DEVICE__ float smin_op(float a, float b)
{
    float k = 0.1f;
    float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);

    return _mix(b, a, h) - k * h * (1.0f - h);
}

__DEVICE__ float sdf_sphere(float3 p, float r)
{
    return length(p) - r;
}

__DEVICE__ float sdf_sphere_noisy(float3 p, float r)
{
    float dist = length(p) - r;
    float freq = 35.0f;
    float magnitude = 0.015f;
    float displacement = magnitude * _sinf(freq * p.x) * _sinf(freq * p.y) * _sinf(freq * p.z);

    return dist + displacement;
}

__DEVICE__ float sdf(float3 pos, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    float time = iTime + 1.3f;

    // Generate three larger noisy spheres
    float displacement = 0.15f * _sinf(0.5f * PI * time) + 1.1f;
    float dist = sdf_sphere_noisy(pos - to_float3(0.0f, 0.0f, 0.0f), 0.4f);
    dist = smin_op(dist, sdf_sphere_noisy(pos - to_float3(-0.75f * displacement, 0.0f, 0.0f), 0.3f));
    dist = smin_op(dist, sdf_sphere_noisy(pos - to_float3(0.75f * displacement, 0.0f, 0.0f), 0.3f));

    // Generate a number of smaller spheres that orbits around the larger spheres
    const int num_particles = 10;
    const int max_num_particles = 64; // same as the noise texture width
    for(int i = 0; i < num_particles; ++i) {
        float3 rand_pos = 2.0f * swi3(texture(iChannel0, to_float2(0.0f, (float)(i) / (float)(max_num_particles))),x,y,z) - 1.0f;
        float speed = 0.5f * texture(iChannel0, to_float2(0.5f, (float)(i) / (float)(max_num_particles))).x;
        rand_pos.x *= _sinf(speed * PI * time);
        rand_pos.z *= -_cosf(speed * PI * time);
        dist = smin_op(dist, sdf_sphere(pos - to_float3(1.7f, 0.9f, 1.0f) * rand_pos, 0.15f));
    }

    return dist;
}

struct FirstHitInfo {
    float3 pos;
    float depth;
};

__DEVICE__ bool raymarch(Ray ray, out FirstHitInfo *hit_info, out int *num_steps, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    float tol = 0.0015f; // surface intersection tolerance
    float max_depth = 50.0f;

    // Raymarch through the signed distance field until a surface is hit or the
    // ray is terminated
    float3 pos = ray.origin;
    float depth = 0.0f;
    *num_steps = 0;
    bool hit = false;
    for(int i = 0; i < MAX_NUM_PRIMARY_RAY_STEPS; ++i) {
        *num_steps = i;    
        float dist = sdf(pos, iTime, R, iChannel0);
        if(_fabs(dist) < tol) {
            (*hit_info).pos = pos;
            (*hit_info).depth = depth;
            hit = true;
            break;
        }

        pos += ray.direction * dist;
        depth += dist;

        if(depth > max_depth) {
           break;   
        }
    }
    
    return hit;
}

__DEVICE__ float3 get_gradient(float3 pos, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    // Estimate SDF gradient with central differences
    float delta = 0.005f;
    float3 gradient = to_float3(
        sdf(pos + delta * to_float3(1.0f, 0.0f, 0.0f), iTime, R, iChannel0) -
        sdf(pos + delta * to_float3(-1.0f, 0.0f, 0.0f), iTime, R, iChannel0),
        sdf(pos + delta * to_float3(0.0f, 1.0f, 0.0f), iTime, R, iChannel0) -
        sdf(pos + delta * to_float3(0.0f, -1.0f, 0.0f), iTime, R, iChannel0),
        sdf(pos + delta * to_float3(0.0f, 0.0f, 1.0f), iTime, R, iChannel0) -
        sdf(pos + delta * to_float3(0.0f, 0.0f, -1.0f), iTime, R, iChannel0));

    return gradient;
}

__DEVICE__ float cast_shadow_ray(float3 pos, float3 L, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    const float tol = 0.0015f; // surface intersection tolerance
    const int max_num_steps = 25;
    float radius = 0.03f;
    float start_offset = 0.06f;
    
    Ray ray;
    ray.origin = pos;
    ray.direction = L;

    float t = start_offset;
    float visibility = 1.0f;
    for (int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = _fmaxf(0.0f, sdf(ray.origin + t * ray.direction, iTime,R,iChannel0));
        if (occluder_dist < radius) {
          visibility = _fminf(visibility, clamp(occluder_dist / radius, 0.0f, 1.0f));
        }

        if (occluder_dist < tol) {
           visibility = 0.0f;
           break;
        }
        
    t += occluder_dist;
    }

    return visibility;
}

__DEVICE__ float compute_ao(float3 pos, float3 N, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    const int max_num_steps = 5;
    float base_step_size = 0.025f;
    
    Ray ray;
    ray.origin = pos;
    ray.direction = N;

    float t = base_step_size;
    float occlusion = 1.0f;
    for (int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = _fmaxf(0.0f, sdf(ray.origin + t * ray.direction,iTime,R, iChannel0));
        float occlusion_i = clamp(occluder_dist / t, 0.0f, 1.0f);
        occlusion = _mix(occlusion_i, occlusion, 0.8f);
        t *= 2.0f;
    }

    return occlusion;
}

// Estimates local surface thickness by inverting the SDF and calculating the
// ambient occlusion along the negative normal direction. The returned thickness
// value will be in range [0, 1], where higher values indicate higher thickness.
__DEVICE__ float compute_local_thickness(float3 pos, float3 N, float iTime, float2 R, __TEXTURE2D__ iChannel0)
{
    const int max_num_steps = 4;
  float base_step_size = 0.03f;

    Ray ray;
    ray.origin = pos;
    ray.direction = -N;

    float t = base_step_size;
    float occlusion = 1.0f;
    for(int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = _fmaxf(0.0f, -sdf(ray.origin + t * ray.direction,iTime,R,iChannel0));        
        float occlusion_i = clamp(occluder_dist / t, 0.0f, 1.0f);
        occlusion = _mix(occlusion_i, occlusion, 0.8f);
        t *= 2.0f;
    }    
    float thickness = occlusion;

    return thickness;
}

__DEVICE__ float diffuse_wrap(float3 N, float3 L, float wrap)
{
    return _fmaxf(0.0f, (dot(L, N) + wrap) / ((1.0f + wrap) * (1.0f + wrap)));
}

__DEVICE__ float specular_D_blinn_phong(float3 N, float3 H, float specular_power)
{
    return _powf(_fmaxf(0.0f, dot(N, H)), specular_power) * (specular_power + 8.0f) / 8.0f;
}

__DEVICE__ float specular_power_from_gloss(float gloss)
{
    return _powf(2.0f, 10.0f * gloss + 1.0f);
}

// Computes the closest representative point (CRP) on a spherical area light source.
// Reference: Brian Karnis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013
__DEVICE__ float3 sphere_area_light_crp(SphereAreaLight light, float3 N, float3 R, float3 eye)
{
    float3 L = (light.position - eye);
    float3 center_to_ray = dot(L, R) * R - L;
    float3 closest_point = L + center_to_ray * clamp(light.radius / length(center_to_ray), 0.0f, 1.0f);

    return closest_point;
}

// Computes a normalization factor for a spherical area light source. This factor
// should be multiplied with the Blinn-Phong specular D value so that the intensity
// of the specular highlight decreases when the sphere radius increases.
__DEVICE__ float sphere_area_light_normalization(SphereAreaLight light, float3 closest_point, float gloss)
{
  // Use the solid angle to the sphere light source to estimate a new gloss
    // value that widens the highlight. Based on the specular D modification
    // proposed by Brian Karnis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013.
    float d = length(closest_point);
    float roughness = 1.0f - gloss;
    float alpha = roughness * roughness;
    float alpha_new = clamp(alpha + 0.5f * light.radius / d, 0.0f, 1.0f);
    float gloss_new = 1.0f - _sqrtf(alpha_new);
    
    // Compute sphere normalization factor
    float specular_power = specular_power_from_gloss(gloss);
    float specular_power_new = specular_power_from_gloss(gloss_new);
    float normalization_factor = (8.0f + specular_power_new) / (8.0f + specular_power);

    return normalization_factor;
}

__DEVICE__ float3 fresnel_schlick(float3 R_F0, float3 E, float3 H)
{
    return R_F0 + (1.0f - R_F0) * _powf(1.0f - _fmaxf(0.0f, dot(E, H)), 5.0f);
}

__DEVICE__ float3 fresnel_schlick_gloss(float3 R_F0, float3 E, float3 N, float gloss)
{
    return R_F0 + (_fmaxf(to_float3_s(gloss), R_F0) - R_F0) * _powf(1.0f - _fmaxf(0.0f, dot(E, N)), 5.0f);
}

__DEVICE__ float3 hemisphere_diffuse(HemisphereLight light, float3 N)
{
    return _mix(light.ground_color, light.sky_color, 0.5f * dot(N, light.up) + 0.5f) * light.intensity;
}

__DEVICE__ float3 hemisphere_specular(HemisphereLight light, float3 R, float gloss)
{
    float g = _fminf(0.975f, gloss);
    float alpha = clamp(dot(R, light.up) / (1.0f - g * g), 0.0f, 1.0f);
    return _mix(light.ground_color, light.sky_color, alpha) * light.intensity;
}

__DEVICE__ float3 compute_shading(float3 pos, float3 N, float3 V, Material material, SphereAreaLight key_light,
                     HemisphereLight sky_light, float iTime, float2 iR, __TEXTURE2D__ iChannel0)
{
  float aaaaaaaaaaaaaaaaaaaaaaa;
    float3 L = normalize(key_light.position - pos);
    float3 R = normalize(reflect(-V, N));    

    float visibility = cast_shadow_ray(pos, L,iTime,iR,iChannel0);    
    float occlusion = compute_ao(pos, N,iTime,iR,iChannel0);

    float3 closest_point = sphere_area_light_crp(key_light, N, R, pos);
    float sphere_normalization =
        sphere_area_light_normalization(key_light, closest_point, material.gloss);
    float3 L_crp = normalize(closest_point);
    float3 H = normalize(V + L_crp);

    float3 specular_color = _mix(to_float3_s(0.04f), material.base_color, material.metalness);
    float3 diffuse_color = _mix(material.base_color, to_float3_s(0.0f), material.metalness);
    float specular_power = specular_power_from_gloss(material.gloss);
    float3 F0 = specular_color;
    float3 F = fresnel_schlick_gloss(specular_color, N, V, material.gloss);
    
    float3 key_light_color = key_light.color * key_light.intensity;

    // Diffuse lighting
    float3 output_color = to_float3_s(0.0f);
    output_color += visibility * (1.0f - F0) * diffuse_color *
                    diffuse_wrap(N, L_crp, material.wrap) * key_light_color;
    output_color += occlusion * (1.0f - F) * diffuse_color * hemisphere_diffuse(sky_light, N);

    // Specular lighting
    output_color += visibility * fresnel_schlick(specular_color, L_crp, H) *
                    specular_D_blinn_phong(N, H, specular_power) * sphere_normalization *
                    _fmaxf(0.0f, dot(N, L_crp)) * key_light_color;    
    output_color += occlusion * F * hemisphere_specular(sky_light, R, material.gloss);
    
    // Translucency
    if (SETTINGS_TRANSLUCENCY_ENABLED > 0) {
        float thickness = compute_local_thickness(pos, N,iTime,iR,iChannel0);
        output_color += diffuse_color * (1.0f - thickness) * _fmaxf(0.0f, dot(-L, V)) * key_light_color;
    }
    
    return output_color;
}


__KERNEL__ void PsychadelicRapberriesJipi226Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float AAAAAAAAAAAAAAAAAAAAAAAAAA;
    float2 uv = fragCoord / iResolution;

    // Set up the camera
    PerspectiveCamera camera;
    camera.eye = to_float3(0.0f, 0.0f, 2.0f);
    camera.up = to_float3(0.0f, 1.0f, 0.0f);
    camera.center = to_float3(0.0f, 0.0f, 0.0f);
    camera.fovy = radians(55.0f);
    camera.aspect = iResolution.x / iResolution.y;

    // Set up light sources    
    SphereAreaLight key_light;
    key_light.position = to_float3(4.0f, 4.0f, -3.0f);
    key_light.radius = 1.5f;
    key_light.color = to_float3(1.0f, 1.0f, 1.0f);
    key_light.intensity = SETTINGS_KEY_LIGHT_INTENSITY;
    
    HemisphereLight sky_light;
    sky_light.up = to_float3(0.0f, 1.0f, 0.0f);
    sky_light.sky_color = srgb2lin(to_float3(0.65f, 0.9f, 1.0f));
    sky_light.ground_color = srgb2lin(to_float3(0.25f, 0.25f, 0.25f));    
    sky_light.intensity = SETTINGS_SKY_LIGHT_INTENSITY;
    
    // Generate camera ray
    Ray ray = get_camera_ray(camera, uv);

    // Find the closest SDF surface intersection by raymarching
    FirstHitInfo hit_info;
    int num_steps = 0;    
    bool hit = raymarch(ray, &hit_info, &num_steps, iTime,R, iChannel0);

    // Apply shading
    float3 output_color = to_float3_s(0.0f);
    float output_depth = 0.0f;
    if (hit) {
        Material material;
        material.base_color = srgb2lin(to_float3(0.8f, 0.35f, uv.x));
        material.metalness = 0.0f;
        material.gloss = 0.65f;
        material.wrap = 0.6f;

    float3 N = normalize(get_gradient(hit_info.pos,iTime,R,iChannel0));
        float3 V = normalize(-ray.direction);

        output_color = compute_shading(hit_info.pos, N, V, material, key_light, sky_light,iTime,R,iChannel0);        
        output_depth = hit_info.depth;
    }
    else {
        output_color += hemisphere_diffuse(sky_light, ray.direction);
        output_depth = 999.0f;
    }
    
    output_color *= SETTINGS_EXPOSURE;
    
    if (SETTINGS_DISPLAY_STEP_COUNT > 0) {
        // Display a cost map of the number of raymarching steps (primary rays only).
        // NOTE: Remember to disable postprocessing effects (i.e., depth of field and
        // bloom) when enabling this setting.
        output_color = to_float3_s((float)(num_steps) / (float)(MAX_NUM_PRIMARY_RAY_STEPS));
    }
    
  fragColor = to_float4_aw(output_color, output_depth);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// First postprocessing pass. Applies DOF, bloom, vignette, and tone mapping.

#define SETTINGS_DEPTH_OF_FIELD_ENABLED 1
#define SETTINGS_BLOOM_ENABLED          1
#define SETTINGS_VIGNETTE_ENABLED       1

// Generates well-distributed points on a unit disk using Vogel's method.
// Reference: Spreading points on a disc and on a sphere, http://blog.marmakoide.org/?p=1
__DEVICE__ float2 vogel_disk(int i, int num_samples)
{
  float radius = _sqrtf((float)(i) / (float)(num_samples));
    float golden_angle = 2.4f;    
    float phi = (float)(i) * golden_angle;
    float x = radius * _cosf(phi);
    float y = radius * _sinf(phi);

    return to_float2(x, y);
}

__DEVICE__ float3 depth_of_field(__TEXTURE2D__ tex, float2 texcoord, float depth, float coc_radius,
                    float focus_dist, float focus_width, float2 R)
{
    const int num_samples = 18;
    float2 resolution = R; //to_float2(textureSize(tex, 0));
    float aspect = resolution.x / resolution.y;

    // Calculate the circle of confusion (CoC) at the current depth 
    float coc = coc_radius * clamp(_fabs(focus_dist - depth) / focus_width, 0.0f, 1.0f);

    // Sample the color texture at well-distributed points in the CoC and
    // average the result
    float3 output_color = to_float3_s(0.0f);
    for (int i = 0; i < num_samples; ++i) {
        float2 offset = vogel_disk(i, num_samples);
        offset.y *= aspect;
        output_color += swi3(texture(tex, texcoord + coc * offset),x,y,z);
    }
    output_color /= (float)(num_samples);
float bbbbbbbbbbbbbbbbbbb;
    return output_color;
}

__DEVICE__ float gaussian_approx(float x)
{
  return x * x * (3.0f - 2.0f * x);    
}

__DEVICE__ float3 bloom(__TEXTURE2D__ tex, float2 uv, float intensity, float radius, float2 R)
{
    const int num_samples = 16;
    float aspect = iResolution.x / iResolution.y;
    
    // Blur the HDR input texture. Here we just generate well-distributed points
    // in a Vogel disk and weight each sample with a Gaussian weight.
    float3 output_color = to_float3_s(0.0f);
    float weight_sum = 0.0f;
    for (int i = 0; i < num_samples; ++i) {
        float2 offset = vogel_disk(i, num_samples);
        float weight = gaussian_approx(1.0f - length(offset));
        offset.y *= aspect;
        output_color += weight * swi3(texture(tex, uv + radius * offset),x,y,z);
        weight_sum += weight;
    }
    output_color /= weight_sum;

    // Multiply the filtered HDR value with the bloom strength/intensity
    output_color *= intensity;

    return output_color;
}

__DEVICE__ float vignette(in float2 uv)
{
    float radius = 2.5f;
    float dist = length(2.0f * uv - 1.0f);
    return 1.0f - _fminf(1.0f, dist / radius);
}

// Approximation of the ACES filmic tone mapping curve.
// Reference: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
__DEVICE__ float3 aces(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

__DEVICE__ float3 lin2srgb(float3 color)
{
  return pow_f3(color, to_float3_s(0.454f));
}

__KERNEL__ void PsychadelicRapberriesJipi226Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBBBBB;
    float2 uv = fragCoord / iResolution;
    float4 output_color = _tex2DVecN(iChannel0,uv.x,uv.y,15);

    float3 color = swi3(output_color,x,y,z);

    // Apply depth of field
    if (SETTINGS_DEPTH_OF_FIELD_ENABLED > 0) {
      float depth = output_color.w;
      float coc_radius = 0.004f;
      float focus_dist = 1.8f;
      float focus_width = 1.0f;
      color = depth_of_field(iChannel0, uv, depth, coc_radius, focus_dist, focus_width,R);
    }

    // Apply bloom
    if (SETTINGS_BLOOM_ENABLED > 0) {
      float bloom_intensity = 0.07f;
      float bloom_radius = 0.02f;
      color += bloom(iChannel0, uv, bloom_intensity, bloom_radius,R);
    }

    // Apply vignette
    if (SETTINGS_VIGNETTE_ENABLED > 0) {
      color *= to_float3_s(vignette(uv));
    }
    
    // Apply tone mapping and gamma correction
    color = aces(color);
    color = lin2srgb(color);

    fragColor = to_float4_aw(color,output_color.w);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0


// Second postprocessing pass. Applies FXAA and dithering.

#define SETTINGS_AA_ENABLED 1
#define SETTINGS_DITHER_ENABLED 1

__DEVICE__ float rgb2luma(float3 color)
{
  return dot(to_float3(0.3f, 0.6f, 0.1f), color);    
}

// Implementation of the fast approximative anti-aliasing (FXAA) algorithm by
// Timothy Lottes. This version uses only four neighbor samples for edge
// detection and does not perform a full end-of-edge search, which will cause
// some artifacts and over-blurring but still produce acceptable results.
__DEVICE__ float3 fxaa(__TEXTURE2D__ tex, float2 texcoord, float2 R)
{
    float subpixel_shift = 0.25f;
    float2 resolution = R;//to_float2(textureSize(tex, 0));
    float2 delta = 1.0f / resolution;    
    float4 pos = to_float4_f2f2(texcoord, texcoord - (0.5f + subpixel_shift) * delta);
float ffffffffffffffffffff;
    // Sample neighborhood sRGB values and convert them to luma
    float luma_nw = rgb2luma(swi3(texture(tex, swi2(pos,z,w)),x,y,z));
    float luma_ne = rgb2luma(swi3(texture(tex, swi2(pos,z,w) + to_float2(delta.x, 0.0f)),x,y,z));
    float luma_sw = rgb2luma(swi3(texture(tex, swi2(pos,z,w) + to_float2(0.0f, delta.y)),x,y,z));
    float luma_se = rgb2luma(swi3(texture(tex, swi2(pos,z,w) + delta),x,y,z));
    float luma_m  = rgb2luma(swi3(texture(tex, swi2(pos,x,y)),x,y,z));
    float luma_min = _fminf(luma_m, _fminf(min(luma_nw, luma_ne), _fminf(luma_sw, luma_se)));
    float luma_max = _fmaxf(luma_m, _fmaxf(max(luma_nw, luma_ne), _fmaxf(luma_sw, luma_se)));

    // Find local edge direction
    float2 dir = to_float2(-((luma_nw + luma_ne) - (luma_sw + luma_se)),
        (luma_nw + luma_sw) - (luma_ne + luma_se));
    float reduce_min = 1.0f / 128.0f;
    float reduce_mul = 1.0f / 8.0f;
    float dir_reduce = _fmaxf(reduce_min,
        (luma_nw + luma_ne + luma_sw + luma_se) * 0.25f * reduce_mul);
    float rcp_dir_min = 1.0f / (_fminf(_fabs(dir.x), _fabs(dir.y)) + dir_reduce);
    float2 span_max = to_float2_s(8.0f);
    dir = clamp(dir * rcp_dir_min, -span_max, span_max) / resolution;

    // Blur along the edge to reduce aliasing
    float3 rgb_a = 0.5f * (swi3(texture(tex, swi2(pos,x,y) - dir / 6.0f),x,y,z) +
                           swi3(texture(tex, swi2(pos,x,y) + dir / 6.0f),x,y,z));
    float3 rgb_b = 0.5f * rgb_a + 0.25f * (swi3(texture(tex, swi2(pos,x,y) - dir / 2.0f),x,y,z) +
                                           swi3(texture(tex, swi2(pos,x,y) + dir / 2.0f),x,y,z));
    float luma_b = rgb2luma(rgb_b);
    float3 output_color = (luma_b < luma_min) || (luma_b > luma_max) ? rgb_a : rgb_b;

    return output_color;
}

__DEVICE__ float3 dither(float2 screen_pos)
{
    float white_noise = fract(_sinf(dot((screen_pos), to_float2(12.989f, 78.233f))) * 43758.545f);
    return to_float3_s((0.5f * white_noise - 0.5f) / 255.0f);
}

__KERNEL__ void PsychadelicRapberriesJipi226Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float3 output_color = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);

    // Apply anti-aliasing
    if (SETTINGS_AA_ENABLED > 0) {
        output_color = fxaa(iChannel0, uv,R);
    }

    // Apply dithering, to reduce color banding
    if (SETTINGS_DITHER_ENABLED > 0) {
      output_color += dither(fragCoord);
        output_color = clamp(output_color, 0.0f, 1.0f);        
    }

    fragColor = to_float4_aw(output_color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}