

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Second postprocessing pass. Applies FXAA and dithering.

#define SETTINGS_AA_ENABLED 1
#define SETTINGS_DITHER_ENABLED 1

float rgb2luma(vec3 color)
{
	return dot(vec3(0.3, 0.6, 0.1), color);    
}

// Implementation of the fast approximative anti-aliasing (FXAA) algorithm by
// Timothy Lottes. This version uses only four neighbor samples for edge
// detection and does not perform a full end-of-edge search, which will cause
// some artifacts and over-blurring but still produce acceptable results.
vec3 fxaa(sampler2D tex, vec2 texcoord)
{
    float subpixel_shift = 0.25;
    vec2 resolution = vec2(textureSize(tex, 0));
    vec2 delta = 1.0 / resolution;    
    vec4 pos = vec4(texcoord, texcoord - (0.5 + subpixel_shift) * delta);

    // Sample neighborhood sRGB values and convert them to luma
    float luma_nw = rgb2luma(texture(tex, pos.zw).rgb);
    float luma_ne = rgb2luma(texture(tex, pos.zw + vec2(delta.x, 0.0)).rgb);
    float luma_sw = rgb2luma(texture(tex, pos.zw + vec2(0.0, delta.y)).rgb);
    float luma_se = rgb2luma(texture(tex, pos.zw + delta).rgb);
    float luma_m  = rgb2luma(texture(tex, pos.xy).rgb);
    float luma_min = min(luma_m, min(min(luma_nw, luma_ne), min(luma_sw, luma_se)));
    float luma_max = max(luma_m, max(max(luma_nw, luma_ne), max(luma_sw, luma_se)));

    // Find local edge direction
    vec2 dir = vec2(-((luma_nw + luma_ne) - (luma_sw + luma_se)),
        (luma_nw + luma_sw) - (luma_ne + luma_se));
    float reduce_min = 1.0 / 128.0;
    float reduce_mul = 1.0 / 8.0;
    float dir_reduce = max(reduce_min,
        (luma_nw + luma_ne + luma_sw + luma_se) * 0.25 * reduce_mul);
    float rcp_dir_min = 1.0 / (min(abs(dir.x), abs(dir.y)) + dir_reduce);
    vec2 span_max = vec2(8.0);
    dir = clamp(dir * rcp_dir_min, -span_max, span_max) / resolution;

    // Blur along the edge to reduce aliasing
    vec3 rgb_a = 0.5 * (texture(tex, pos.xy - dir / 6.0).xyz +
        texture(tex, pos.xy + dir / 6.0).xyz);
    vec3 rgb_b = 0.5 * rgb_a + 0.25 * (texture(tex, pos.xy - dir / 2.0).xyz +
        texture(tex, pos.xy + dir / 2.0).xyz);
    float luma_b = rgb2luma(rgb_b);
    vec3 output_color = (luma_b < luma_min) || (luma_b > luma_max) ? rgb_a : rgb_b;

    return output_color;
}

vec3 dither(vec2 screen_pos)
{
    float white_noise = fract(sin(dot(vec2(screen_pos), vec2(12.989, 78.233))) * 43758.545);
    return vec3((0.5 * white_noise - 0.5) / 255.0);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 output_color = texture(iChannel0, uv).rgb;    

    // Apply anti-aliasing
    if (SETTINGS_AA_ENABLED > 0) {
        output_color = fxaa(iChannel0, uv);
    }

    // Apply dithering, to reduce color banding
    if (SETTINGS_DITHER_ENABLED > 0) {
	    output_color += dither(fragCoord.xy);
        output_color = clamp(output_color, 0.0, 1.0);        
    }

    fragColor = vec4(output_color, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// This SDF raymarching demo implements the closest representative point (CRP)
// method for approximating spherical area lights. It also features soft shadows,
// translucency, and various postprocessing effects like DOF, bloom, and FXAA.
//
// Author: Johan Nysjö

#define SETTINGS_KEY_LIGHT_INTENSITY 0.8
#define SETTINGS_SKY_LIGHT_INTENSITY 1.2
#define SETTINGS_EXPOSURE 2.0
#define SETTINGS_TRANSLUCENCY_ENABLED 1
#define SETTINGS_DISPLAY_STEP_COUNT 0

#define MAX_NUM_PRIMARY_RAY_STEPS 100

#define PI 3.141592653

struct Ray {
	vec3 origin;
    vec3 direction;
};

struct PerspectiveCamera {
	vec3 eye;
    vec3 up;
    vec3 center;
    float fovy;
    float aspect;
};

struct SphereAreaLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

struct HemisphereLight {
    vec3 up;
    vec3 sky_color;
    vec3 ground_color;
    float intensity;
};

struct Material {
    vec3 base_color;
    float metalness;
    float gloss;
    float wrap;
};

vec3 srgb2lin(vec3 color)
{
	return pow(color, vec3(2.2));
}

Ray get_camera_ray(PerspectiveCamera camera, vec2 uv)
{
    vec3 f = normalize(camera.center - camera.eye);
    vec3 s = normalize(cross(f, normalize(camera.up)));
    vec3 u = normalize(cross(s, f));

    float half_height = tan(0.5 * camera.fovy);
    float half_width = camera.aspect * half_height;
    float x = 2.0 * uv.x - 1.0;
    float y = 2.0 * uv.y - 1.0;

    Ray ray;
    ray.origin = camera.eye;
    ray.direction = normalize(f + x * half_width * s + y * half_height * u);
    
    return ray;
}

// Polynomial smooth min operation for SDFs
// Reference: http://www.iquilezles.org/www/articles/smin/smin.htm
float smin_op(float a, float b)
{
    float k = 0.1;
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);

    return mix(b, a, h) - k * h * (1.0 - h);
}

float sdf_sphere(vec3 p, float r)
{
    return length(p) - r;
}

float sdf_sphere_noisy(vec3 p, float r)
{
    float dist = length(p) - r;
    float freq = 35.0;
    float magnitude = 0.015;
    float displacement = magnitude * sin(freq * p.x) * sin(freq * p.y) * sin(freq * p.z);

    return dist + displacement;
}

float sdf(vec3 pos)
{
    float time = iTime + 1.3;

    // Generate three larger noisy spheres
    float displacement = 0.15 * sin(0.5 * PI * time) + 1.1;
    float dist = sdf_sphere_noisy(pos - vec3(0.0, 0.0, 0.0), 0.4);
    dist = smin_op(dist, sdf_sphere_noisy(pos - vec3(-0.75 * displacement, 0.0, 0.0), 0.3));
    dist = smin_op(dist, sdf_sphere_noisy(pos - vec3(0.75 * displacement, 0.0, 0.0), 0.3));

    // Generate a number of smaller spheres that orbits around the larger spheres
    const int num_particles = 10;
    const int max_num_particles = 64; // same as the noise texture width
    for(int i = 0; i < num_particles; ++i) {
     	vec3 rand_pos = 2.0 * texture(iChannel0, vec2(0.0, float(i) / float(max_num_particles))).rgb - 1.0;
        float speed = 0.5 * texture(iChannel0, vec2(0.5, float(i) / float(max_num_particles))).r;
        rand_pos.x *= sin(speed * PI * time);
        rand_pos.z *= -cos(speed * PI * time);
        dist = smin_op(dist, sdf_sphere(pos - vec3(1.7, 0.9, 1.0) * rand_pos, 0.15));
    }

    return dist;
}

struct FirstHitInfo {
    vec3 pos;
    float depth;
};

bool raymarch(Ray ray, out FirstHitInfo hit_info, out int num_steps)
{
    float tol = 0.0015; // surface intersection tolerance
    float max_depth = 50.0;

    // Raymarch through the signed distance field until a surface is hit or the
    // ray is terminated
    vec3 pos = ray.origin;
    float depth = 0.0;
    num_steps = 0;
    bool hit = false;
    for(int i = 0; i < MAX_NUM_PRIMARY_RAY_STEPS; ++i) {
        num_steps = i;    
        float dist = sdf(pos);
        if(abs(dist) < tol) {
         	hit_info.pos = pos;
            hit_info.depth = depth;
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

vec3 get_gradient(vec3 pos)
{
    // Estimate SDF gradient with central differences
    float delta = 0.005;
    vec3 gradient = vec3(
        sdf(pos + delta * vec3(1.0, 0.0, 0.0)) -
        sdf(pos + delta * vec3(-1.0, 0.0, 0.0)),
        sdf(pos + delta * vec3(0.0, 1.0, 0.0)) -
        sdf(pos + delta * vec3(0.0, -1.0, 0.0)),
        sdf(pos + delta * vec3(0.0, 0.0, 1.0)) -
        sdf(pos + delta * vec3(0.0, 0.0, -1.0)));

    return gradient;
}

float cast_shadow_ray(vec3 pos, vec3 L)
{
    const float tol = 0.0015; // surface intersection tolerance
    const int max_num_steps = 25;
    float radius = 0.03;
    float start_offset = 0.06;
    
    Ray ray;
    ray.origin = pos;
    ray.direction = L;

    float t = start_offset;
    float visibility = 1.0;
    for (int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = max(0.0, sdf(ray.origin + t * ray.direction));
        if (occluder_dist < radius) {
        	visibility = min(visibility, clamp(occluder_dist / radius, 0.0, 1.0));
        }

        if (occluder_dist < tol) {
           visibility = 0.0;
           break;
        }
        
		t += occluder_dist;
    }

    return visibility;
}

float compute_ao(vec3 pos, vec3 N)
{
    const int max_num_steps = 5;
    float base_step_size = 0.025;
    
    Ray ray;
    ray.origin = pos;
    ray.direction = N;

    float t = base_step_size;
    float occlusion = 1.0;
    for (int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = max(0.0, sdf(ray.origin + t * ray.direction));
        float occlusion_i = clamp(occluder_dist / t, 0.0, 1.0);
	    occlusion = mix(occlusion_i, occlusion, 0.8);
        t *= 2.0;
    }

    return occlusion;
}

// Estimates local surface thickness by inverting the SDF and calculating the
// ambient occlusion along the negative normal direction. The returned thickness
// value will be in range [0, 1], where higher values indicate higher thickness.
float compute_local_thickness(vec3 pos, vec3 N)
{
    const int max_num_steps = 4;
	float base_step_size = 0.03;

    Ray ray;
    ray.origin = pos;
    ray.direction = -N;

    float t = base_step_size;
    float occlusion = 1.0;
    for(int i = 0; i < max_num_steps; ++i) {
        float occluder_dist = max(0.0, -sdf(ray.origin + t * ray.direction));        
        float occlusion_i = clamp(occluder_dist / t, 0.0, 1.0);
        occlusion = mix(occlusion_i, occlusion, 0.8);
        t *= 2.0;
    }    
    float thickness = occlusion;

    return thickness;
}

float diffuse_wrap(vec3 N, vec3 L, float wrap)
{
    return max(0.0, (dot(L, N) + wrap) / ((1.0 + wrap) * (1.0 + wrap)));
}

float specular_D_blinn_phong(vec3 N, vec3 H, float specular_power)
{
    return pow(max(0.0, dot(N, H)), specular_power) * (specular_power + 8.0) / 8.0;
}

float specular_power_from_gloss(float gloss)
{
    return pow(2.0, 10.0 * gloss + 1.0);
}

// Computes the closest representative point (CRP) on a spherical area light source.
// Reference: Brian Karnis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013
vec3 sphere_area_light_crp(SphereAreaLight light, vec3 N, vec3 R, vec3 eye)
{
    vec3 L = (light.position - eye);
    vec3 center_to_ray = dot(L, R) * R - L;
    vec3 closest_point = L + center_to_ray * clamp(light.radius / length(center_to_ray), 0.0, 1.0);

    return closest_point;
}

// Computes a normalization factor for a spherical area light source. This factor
// should be multiplied with the Blinn-Phong specular D value so that the intensity
// of the specular highlight decreases when the sphere radius increases.
float sphere_area_light_normalization(SphereAreaLight light, vec3 closest_point, float gloss)
{
	// Use the solid angle to the sphere light source to estimate a new gloss
    // value that widens the highlight. Based on the specular D modification
    // proposed by Brian Karnis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013.
    float d = length(closest_point);
    float roughness = 1.0 - gloss;
    float alpha = roughness * roughness;
    float alpha_new = clamp(alpha + 0.5 * light.radius / d, 0.0, 1.0);
    float gloss_new = 1.0 - sqrt(alpha_new);
    
    // Compute sphere normalization factor
    float specular_power = specular_power_from_gloss(gloss);
    float specular_power_new = specular_power_from_gloss(gloss_new);
    float normalization_factor = (8.0 + specular_power_new) / (8.0 + specular_power);

    return normalization_factor;
}

vec3 fresnel_schlick(vec3 R_F0, vec3 E, vec3 H)
{
    return R_F0 + (1.0 - R_F0) * pow(1.0 - max(0.0, dot(E, H)), 5.0);
}

vec3 fresnel_schlick_gloss(vec3 R_F0, vec3 E, vec3 N, float gloss)
{
    return R_F0 + (max(vec3(gloss), R_F0) - R_F0) * pow(1.0 - max(0.0, dot(E, N)), 5.0);
}

vec3 hemisphere_diffuse(HemisphereLight light, vec3 N)
{
    return mix(light.ground_color, light.sky_color, 0.5 * dot(N, light.up) + 0.5) * light.intensity;
}

vec3 hemisphere_specular(HemisphereLight light, vec3 R, float gloss)
{
    float g = min(0.975, gloss);
    float alpha = clamp(dot(R, light.up) / (1.0 - g * g), 0.0, 1.0);
    return mix(light.ground_color, light.sky_color, alpha) * light.intensity;
}

vec3 compute_shading(vec3 pos, vec3 N, vec3 V, Material material, SphereAreaLight key_light,
                     HemisphereLight sky_light)
{
    vec3 L = normalize(key_light.position - pos);
    vec3 R = normalize(reflect(-V, N));    

	float visibility = cast_shadow_ray(pos, L);    
    float occlusion = compute_ao(pos, N);

    vec3 closest_point = sphere_area_light_crp(key_light, N, R, pos);
    float sphere_normalization =
        sphere_area_light_normalization(key_light, closest_point, material.gloss);
    vec3 L_crp = normalize(closest_point);
    vec3 H = normalize(V + L_crp);

    vec3 specular_color = mix(vec3(0.04), material.base_color, material.metalness);
    vec3 diffuse_color = mix(material.base_color, vec3(0.0), material.metalness);
    float specular_power = specular_power_from_gloss(material.gloss);
    vec3 F0 = specular_color;
    vec3 F = fresnel_schlick_gloss(specular_color, N, V, material.gloss);
    
    vec3 key_light_color = key_light.color * key_light.intensity;

    // Diffuse lighting
    vec3 output_color = vec3(0.0);
    output_color += visibility * (1.0 - F0) * diffuse_color *
                    diffuse_wrap(N, L_crp, material.wrap) * key_light_color;
    output_color += occlusion * (1.0 - F) * diffuse_color * hemisphere_diffuse(sky_light, N);

    // Specular lighting
    output_color += visibility * fresnel_schlick(specular_color, L_crp, H) *
                    specular_D_blinn_phong(N, H, specular_power) * sphere_normalization *
                    max(0.0, dot(N, L_crp)) * key_light_color;    
    output_color += occlusion * F * hemisphere_specular(sky_light, R, material.gloss);
    
    // Translucency
    if (SETTINGS_TRANSLUCENCY_ENABLED > 0) {
        float thickness = compute_local_thickness(pos, N);
        output_color += diffuse_color * (1.0 - thickness) * max(0.0, dot(-L, V)) * key_light_color;
    }
    
    return output_color;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
	vec2 uv = fragCoord.xy / iResolution.xy;

    // Set up the camera
    PerspectiveCamera camera;
    camera.eye = vec3(0.0, 0.0, 2.0);
    camera.up = vec3(0.0, 1.0, 0.0);
    camera.center = vec3(0.0, 0.0, 0.0);
    camera.fovy = radians(55.0);
    camera.aspect = iResolution.x / iResolution.y;

    // Set up light sources    
    SphereAreaLight key_light;
    key_light.position = vec3(4.0, 4.0, -3.0);
    key_light.radius = 1.5;
    key_light.color = vec3(1.0, 1.0, 1.0);
    key_light.intensity = SETTINGS_KEY_LIGHT_INTENSITY;
    
    HemisphereLight sky_light;
    sky_light.up = vec3(0.0, 1.0, 0.0);
    sky_light.sky_color = srgb2lin(vec3(0.65, 0.9, 1.0));
    sky_light.ground_color = srgb2lin(vec3(0.25, 0.25, 0.25));    
    sky_light.intensity = SETTINGS_SKY_LIGHT_INTENSITY;
    
    // Generate camera ray
    Ray ray = get_camera_ray(camera, uv);

    // Find the closest SDF surface intersection by raymarching
    FirstHitInfo hit_info;
    int num_steps = 0;    
    bool hit = raymarch(ray, hit_info, num_steps);

    // Apply shading
    vec3 output_color = vec3(0.0);
    float output_depth = 0.0;
    if (hit) {
        Material material;
        material.base_color = srgb2lin(vec3(0.8, 0.35, uv.x));
        material.metalness = 0.0;
        material.gloss = 0.65;
        material.wrap = 0.6;

		vec3 N = normalize(get_gradient(hit_info.pos));
        vec3 V = normalize(-ray.direction);

        output_color = compute_shading(hit_info.pos, N, V, material, key_light, sky_light);        
        output_depth = hit_info.depth;
    }
    else {
        output_color.rgb += hemisphere_diffuse(sky_light, ray.direction);
        output_depth = 999.0;
    }
    
    output_color *= SETTINGS_EXPOSURE;
    
    if (SETTINGS_DISPLAY_STEP_COUNT > 0) {
        // Display a cost map of the number of raymarching steps (primary rays only).
        // NOTE: Remember to disable postprocessing effects (i.e., depth of field and
        // bloom) when enabling this setting.
        output_color.rgb = vec3(float(num_steps) / float(MAX_NUM_PRIMARY_RAY_STEPS));
    }
    
	fragColor = vec4(output_color, output_depth);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// First postprocessing pass. Applies DOF, bloom, vignette, and tone mapping.

#define SETTINGS_DEPTH_OF_FIELD_ENABLED 1
#define SETTINGS_BLOOM_ENABLED 1
#define SETTINGS_VIGNETTE_ENABLED 1

// Generates well-distributed points on a unit disk using Vogel's method.
// Reference: Spreading points on a disc and on a sphere, http://blog.marmakoide.org/?p=1
vec2 vogel_disk(int i, int num_samples)
{
	float radius = sqrt(float(i) / float(num_samples));
    float golden_angle = 2.4;    
    float phi = float(i) * golden_angle;
    float x = radius * cos(phi);
    float y = radius * sin(phi);

    return vec2(x, y);
}

vec3 depth_of_field(sampler2D tex, vec2 texcoord, float depth, float coc_radius,
                    float focus_dist, float focus_width)
{
    const int num_samples = 18;
    vec2 resolution = vec2(textureSize(tex, 0));
    float aspect = resolution.x / resolution.y;

    // Calculate the circle of confusion (CoC) at the current depth 
    float coc = coc_radius * clamp(abs(focus_dist - depth) / focus_width, 0.0, 1.0);

    // Sample the color texture at well-distributed points in the CoC and
    // average the result
    vec3 output_color = vec3(0.0);
    for (int i = 0; i < num_samples; ++i) {
        vec2 offset = vogel_disk(i, num_samples);
        offset.y *= aspect;
        output_color += texture(tex, texcoord + coc * offset).rgb;
    }
    output_color /= float(num_samples);

    return output_color;
}

float gaussian_approx(float x)
{
	return x * x * (3.0 - 2.0 * x);    
}

vec3 bloom(sampler2D tex, vec2 uv, float intensity, float radius)
{
    const int num_samples = 16;
    float aspect = iResolution.x / iResolution.y;
    
    // Blur the HDR input texture. Here we just generate well-distributed points
    // in a Vogel disk and weight each sample with a Gaussian weight.
    vec3 output_color = vec3(0.0);
    float weight_sum = 0.0;
    for (int i = 0; i < num_samples; ++i) {
        vec2 offset = vogel_disk(i, num_samples);
        float weight = gaussian_approx(1.0 - length(offset));
        offset.y *= aspect;
        output_color += weight * texture(tex, uv + radius * offset).rgb;
        weight_sum += weight;
    }
    output_color /= weight_sum;

    // Multiply the filtered HDR value with the bloom strength/intensity
    output_color *= intensity;

    return output_color;
}

float vignette(in vec2 uv)
{
	float radius = 2.5;
    float dist = length(2.0 * uv - 1.0);
    return 1.0 - min(1.0, dist / radius);
}

// Approximation of the ACES filmic tone mapping curve.
// Reference: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 aces(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 lin2srgb(vec3 color)
{
	return pow(color, vec3(0.454));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
	vec4 output_color = texture(iChannel0, uv);

    // Apply depth of field
    if (SETTINGS_DEPTH_OF_FIELD_ENABLED > 0) {
	    float depth = output_color.a;
    	float coc_radius = 0.004;
        float focus_dist = 1.8;
        float focus_width = 1.0;
    	output_color.rgb = depth_of_field(iChannel0, uv, depth, coc_radius, focus_dist, focus_width);
    }

    // Apply bloom
    if (SETTINGS_BLOOM_ENABLED > 0) {
	    float bloom_intensity = 0.07;
    	float bloom_radius = 0.02;
		output_color.rgb += bloom(iChannel0, uv, bloom_intensity, bloom_radius);
    }

    // Apply vignette
    if (SETTINGS_VIGNETTE_ENABLED > 0) {
	    output_color.rgb *= vec3(vignette(uv));
    }
    
    // Apply tone mapping and gamma correction
    output_color.rgb = aces(output_color.rgb);
    output_color.rgb = lin2srgb(output_color.rgb);

	fragColor = output_color;
}