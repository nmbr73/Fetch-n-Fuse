
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define SHADERTOY

#ifdef __cplusplus
  #define _in(T) const T &
  #define _inout(T) T &
  #define _out(T) T &
  #define _begin(type)  {
  #define _end }
  #define _mutable(T) T
  #define _constant(T) const T
  #define mul(a, b) (a) * (b)
  #define vec2 float2
  #define vec3 float3
  #define vec4 float4
#endif


#if defined(__cplusplus) || defined(SHADERTOY)
#define u_res iResolution
#define u_time iTime
#define u_mouse iMouse
#endif


#define PI 3.14159265359f

struct ray_t {
  float3 origin;
  float3 direction;
};
#define BIAS 1e-4 // small offset to avoid self-intersections

struct sphere_t {
  float3 origin;
  float radius;
  int material;
};

struct plane_t {
  float3 direction;
  float distance;
  int material;
};

struct hit_t {
  float t;
  int material_id;
  float3 normal;
  float3 origin;
};
#define max_dist 1e8
//_constant(hit_t) no_hit = _begin(hit_t)
//                              (float)(max_dist + 1e1), // 'infinite' distance
//                              -1, // material id
//                              {0.0f, 0.0f, 0.0f}, // normal
//                              {0.0f, 0.0f, 0.0f} // origin
//                          _end;

// ----------------------------------------------------------------------------
// Various 3D utilities functions
// ----------------------------------------------------------------------------

__DEVICE__ ray_t get_primary_ray(
  _in(vec3) cam_local_point,
  _inout(vec3) cam_origin,
  _inout(vec3) cam_look_at
){
  float3 fwd = normalize(cam_look_at - cam_origin);
  float3 up = to_float3(0, 1, 0);
  float3 right = cross(up, fwd);
  up = cross(fwd, right);

  ray_t r = _begin(ray_t)
    cam_origin,
    normalize(fwd + up * cam_local_point.y + right * cam_local_point.x)
  _end;
  return r;
}

//_constant(mat3) mat3_ident = to_mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);


__DEVICE__ mat2 rotate_2d(
  _in(float) angle_degrees
){
  float angle = radians(angle_degrees);
  float _sin = _sinf(angle);
  float _cos = _cosf(angle);
  return to_mat2(_cos, -_sin, _sin, _cos);
}

__DEVICE__ mat3 rotate_around_z(
  _in(float) angle_degrees
){
  float angle = radians(angle_degrees);
  float _sin = _sinf(angle);
  float _cos = _cosf(angle);
  return to_mat3(_cos, -_sin, 0, _sin, _cos, 0, 0, 0, 1);
}

__DEVICE__ mat3 rotate_around_y(
  _in(float) angle_degrees
){
  float angle = radians(angle_degrees);
  float _sin = _sinf(angle);
  float _cos = _cosf(angle);
  return to_mat3(_cos, 0, _sin, 0, 1, 0, -_sin, 0, _cos);
}

__DEVICE__ mat3 rotate_around_x(
  _in(float) angle_degrees
){
  float angle = radians(angle_degrees);
  float _sin = _sinf(angle);
  float _cos = _cosf(angle);
  return to_mat3(1, 0, 0, 0, _cos, -_sin, 0, _sin, _cos);
}

// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch24.html
__DEVICE__ float3 linear_to_srgb(
  _in(vec3) color
){
  const float p = 1.0f / 2.2f;
  return to_float3(_powf(color.x, p), _powf(color.y, p), _powf(color.z, p));
}
__DEVICE__ float3 srgb_to_linear(
  _in(vec3) color
){
  const float p = 2.2f;
  return to_float3(_powf(color.x, p), _powf(color.y, p), _powf(color.z, p));
}

#ifdef __cplusplus
__DEVICE__ float3 faceforward(
  _in(vec3) N,
  _in(vec3) I,
  _in(vec3) Nref
){
  
  return dot(Nref, I) < 0 ? N : -1.0f*N;
}
#endif

__DEVICE__ float checkboard_pattern(
  _in(vec2) pos,
  _in(float) scale
){
  float2 pattern = _floor(pos * scale);
  return mod_f(pattern.x + pattern.y, 2.0f);
}

__DEVICE__ float band (
  _in(float) start,
  _in(float) peak,
  _in(float) end,
  _in(float) t
){
  return
  smoothstep (start, peak, t) *
  (1.0f - smoothstep (peak, end, t));
}

// from https://www.shadertoy.com/view/4sSSW3
// original http://orbit.dtu.dk/fedora/objects/orbit:113874/datastreams/file_75b66578-222e-4c7d-abdf-f7e255100209/content
__DEVICE__ void fast_orthonormal_basis(
  _in(vec3) n,
  out float3 *f,
  out float3 *r
){
  float a = 1.0f / (1.0f + n.z);
  float b = -n.x*n.y*a;
  *f = to_float3(1.0f - n.x*n.x*a, b, -n.x);
  *r = to_float3(b, 1.0f - n.y*n.y*a, -n.y);
}

// ----------------------------------------------------------------------------
// Analytical surface-ray intersection routines
// ----------------------------------------------------------------------------

// geometrical solution
// info: http://www.scratchapixel.com/old/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-sphere-intersection/
__DEVICE__ void intersect_sphere(
  _in(ray_t) ray,
  _in(sphere_t) sphere,
  inout hit_t *hit
){
  float3 rc = sphere.origin - ray.origin;
  float radius2 = sphere.radius * sphere.radius;
  float tca = dot(rc, ray.direction);
  if (tca < 0.0f) return;

  float d2 = dot(rc, rc) - tca * tca;
  if (d2 > radius2) return;

  float thc = _sqrtf(radius2 - d2);
  float t0 = tca - thc;
  float t1 = tca + thc;

  if (t0 < 0.0f) t0 = t1;
  if (t0 > (*hit).t) return;

  float3 impact = ray.origin + ray.direction * t0;

  (*hit).t = t0;
  (*hit).material_id = sphere.material;
  (*hit).origin = impact;
  (*hit).normal = (impact - sphere.origin) / sphere.radius;
}


// ----------------------------------------------------------------------------
// Volumetric utilities
// ----------------------------------------------------------------------------

struct volume_sampler_t {
  float3 origin; // start of ray
  float3 pos; // current pos of acccumulation ray
  float height;

  float coeff_absorb;
  float T; // transmitance

  float3 C; // color
  float alpha;
};

__DEVICE__ volume_sampler_t begin_volume(
  _in(vec3) origin,
  _in(float) coeff_absorb
){
  volume_sampler_t v = _begin(volume_sampler_t)
    origin, origin, 0.0f,
    coeff_absorb, 1.0f,
    to_float3(0.0f, 0.0f, 0.0f), 0.
  _end;
  return v;
}

__DEVICE__ float illuminate_volume(
  inout volume_sampler_t *vol,
  _in(vec3) V,
  _in(vec3) L
);

__DEVICE__ void integrate_volume(
  inout volume_sampler_t *vol,
  _in(vec3) V,
  _in(vec3) L,
  _in(float) density,
  _in(float) dt
){
  // change in transmittance (follows Beer-Lambert law)
  float T_i = _expf(-(*vol).coeff_absorb * density * dt);
  // Update accumulated transmittance
  (*vol).T *= T_i;
  // integrate output radiance (here essentially color)
  (*vol).C += (*vol).T * illuminate_volume(vol, V, L) * density * dt;
  // accumulate opacity
  (*vol).alpha += (1.0f - T_i) * (1.0f - (*vol).alpha);
}


// ----------------------------------------------------------------------------
// Noise function by iq from https://www.shadertoy.com/view/4sfGzS
// ----------------------------------------------------------------------------

__DEVICE__ float hash(
  _in(float) n
){
  return fract(_sinf(n)*753.5453123f);
}

__DEVICE__ float noise_iq(
  _in(vec3) x
){
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f*f*(3.0f - 2.0f*f);

#if 1
    float n = p.x + p.y*157.0f + 113.0f*p.z;
    return _mix(_mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                     _mix( hash(n+157.0f), hash(n+158.0f),f.x),f.y),
                _mix(_mix( hash(n+113.0f), hash(n+114.0f),f.x),
                     _mix( hash(n+270.0f), hash(n+271.0f),f.x),f.y),f.z);
#else
  float2 uv = (swi2(p,x,y) + to_float2(37.0f, 17.0f)*p.z) + swi2(f,x,y);
  float2 rg = swi2(texture( iChannel0, (uv+0.5f)/256.0f),y,x);
  return _mix(rg.x, rg.y, f.z);
#endif
}

#define noise(x) noise_iq(x)

// ----------------------------------------------------------------------------
// Fractional Brownian Motion
// depends on custom basis function
// ----------------------------------------------------------------------------

#define DECL_FBM_FUNC(_name, _octaves, _basis) __DEVICE__ float _name(_in(vec3) pos, _in(float) lacunarity, _in(float) init_gain, _in(float) gain) { float3 p = pos; float H = init_gain; float t = 0.0f; for (int i = 0; i < _octaves; i++) { t += _basis * H; p *= lacunarity; H *= gain; } return t; }

DECL_FBM_FUNC(fbm, 4, noise(p))

// ----------------------------------------------------------------------------
// Planet
// ----------------------------------------------------------------------------
//_constant(sphere_t) planet = _begin(sphere_t)
//                                {0, 0, 0}, 1.0f, 0
//                             _end;

#define max_height 0.4f
#define max_ray_dist (max_height * 4.0f)

__DEVICE__ float3 background(
  _in(ray_t) eye
){
#if 0
  return to_float3(0.15f, 0.3f, 0.4f);
#else
  _constant(vec3) sun_color = to_float3(1.0f, 0.9f, 0.55f);
  float sun_amount = dot(eye.direction, to_float3(0, 0, 1));

  float3 sky = _mix(
                    to_float3(0.0f, 0.05f, 0.2f),
                    to_float3(0.15f, 0.3f, 0.4f),
                    1.0f - eye.direction.y);
  sky += sun_color * _fminf(_powf(sun_amount, 30.0f) * 5.0f, 1.0f);
  sky += sun_color * _fminf(_powf(sun_amount, 10.0f) * 0.6f, 1.0f);

  return sky;
#endif
}

__DEVICE__ void setup_scene()
{
}

__DEVICE__ void setup_camera(
  inout float3 *eye,
  inout float3 *look_at
){
#if 0
  *eye = to_float3(0.0f, 0, -1.93f);
  *look_at = to_float3(-0.1f, 0.9f, 2);
#else
  *eye = to_float3(0, 0, -2.5f);
  *look_at = to_float3(0, 0, 2);
#endif
}

// ----------------------------------------------------------------------------
// Clouds
// ----------------------------------------------------------------------------
#define CLOUDS

#define anoise (_fabs(noise(p) * 2.0f - 1.0f))
DECL_FBM_FUNC(fbm_clouds, 4, anoise)

#define vol_coeff_absorb 30.034f
//__DEVICE__  _mutable(volume_sampler_t) cloud;

__DEVICE__ float illuminate_volume(
  inout volume_sampler_t *cloud,
  _in(vec3) V,
  _in(vec3) L
){
  return _expf((*cloud).height) / 0.055f;
}

__DEVICE__ void clouds_map(
  inout volume_sampler_t *cloud,
  _in(float) t_step
){
  float dens = fbm_clouds(
    (*cloud).pos * 3.2343f + to_float3(0.35f, 13.35f, 2.67f),
    2.0276f, 0.5f, 0.5f);

  #define cld_coverage 0.29475675f // higher=less clouds
  #define cld_fuzzy 0.0335f // higher=fuzzy, lower=blockier
  dens *= smoothstep(cld_coverage, cld_coverage + cld_fuzzy, dens);

  dens *= band(0.2f, 0.35f, 0.65f, (*cloud).height);

  integrate_volume(cloud,
                  (*cloud).pos, (*cloud).pos, // unused dummies 
                  dens, t_step);
}

__DEVICE__ void clouds_march(
  _in(ray_t) eye,
  inout volume_sampler_t *cloud,
  _in(float) max_travel,
  _in(mat3) rot, 
  sphere_t planet
){
  const int steps = 75;
  const float t_step = max_ray_dist / (float)(steps);
  float t = 0.0f;

  for (int i = 0; i < steps; i++) {
    if (t > max_travel || (*cloud).alpha >= 1.0f) return;
      
    float3 o = (*cloud).origin + t * eye.direction;
    (*cloud).pos = mul_mat3_f3(rot, o - planet.origin);

    (*cloud).height = (length((*cloud).pos) - planet.radius) / max_height;
    t += t_step;
    clouds_map(cloud, t_step);
  }
}

__DEVICE__ void clouds_shadow_march(
  _in(vec3) dir,
  inout volume_sampler_t *cloud,
  _in(mat3) rot, 
  sphere_t planet
){
  const int steps = 5;
  const float t_step = max_height / (float)(steps);
  float t = 0.0f;

  for (int i = 0; i < steps; i++) {
    float3 o = (*cloud).origin + t * dir;
    (*cloud).pos = mul_mat3_f3(rot, o - planet.origin);

    (*cloud).height = (length((*cloud).pos) - planet.radius) / max_height;
    t += t_step;
    clouds_map(cloud, t_step);
  }
}

// ----------------------------------------------------------------------------
// Terrain
// ----------------------------------------------------------------------------
#define TERR_STEPS 120
#define TERR_EPS 0.005f
#define rnoise (1.0f - _fabs(noise(p) * 2.0f - 1.0f))

DECL_FBM_FUNC(fbm_terr, 3, noise(p))
DECL_FBM_FUNC(fbm_terr_r, 3, rnoise)

DECL_FBM_FUNC(fbm_terr_normals, 7, noise(p))
DECL_FBM_FUNC(fbm_terr_r_normals, 7, rnoise)

__DEVICE__ float2 sdf_terrain_map(_in(vec3) pos, sphere_t planet)
{
  float h0 = fbm_terr(pos * 2.0987f, 2.0244f, 0.454f, 0.454f);
  float n0 = smoothstep(0.35f, 1.0f, h0);

  float h1 = fbm_terr_r(pos * 1.50987f + to_float3(1.9489f, 2.435f, 0.5483f), 2.0244f, 0.454f, 0.454f);
  float n1 = smoothstep(0.6f, 1.0f, h1);
  
  float n = n0 + n1;
  
  return to_float2(length(pos) - planet.radius - n * max_height, n / max_height);
}

__DEVICE__ float2 sdf_terrain_map_detail(_in(vec3) pos, sphere_t planet)
{
  float h0 = fbm_terr_normals(pos * 2.0987f, 2.0244f, 0.454f, 0.454f);
  float n0 = smoothstep(0.35f, 1.0f, h0);

  float h1 = fbm_terr_r_normals(pos * 1.50987f + to_float3(1.9489f, 2.435f, 0.5483f), 2.0244f, 0.454f, 0.454f);
  float n1 = smoothstep(0.6f, 1.0f, h1);

  float n = n0 + n1;

  return to_float2(length(pos) - planet.radius - n * max_height, n / max_height);
}

__DEVICE__ float3 sdf_terrain_normal(_in(vec3) p, sphere_t planet)
{
#define F(t) sdf_terrain_map_detail(t, planet).x
  float3 dt = to_float3(0.001f, 0, 0);

  return normalize(to_float3(
                              F(p + swi3(dt,x,z,z)) - F(p - swi3(dt,x,z,z)),
                              F(p + swi3(dt,z,x,z)) - F(p - swi3(dt,z,x,z)),
                              F(p + swi3(dt,z,z,x)) - F(p - swi3(dt,z,z,x))
                            ));
#undef F
}

// ----------------------------------------------------------------------------
// Lighting
// ----------------------------------------------------------------------------
__DEVICE__ float3 setup_lights(
  float3 L,
  _in(vec3) normal
){
  float3 diffuse = to_float3(0, 0, 0);

  // key light
  float3 c_L = to_float3(7, 5, 3);
  diffuse += _fmaxf(0.0f, dot(L, normal)) * c_L;

  // fill light 1 - faked hemisphere
  float hemi = clamp(0.25f + 0.5f * normal.y, 0.0f, 1.0f);
  diffuse += hemi * to_float3(0.4f, 0.6f, 0.8f) * 0.2f;

  // fill light 2 - ambient (reversed key)
  float amb = clamp(0.12f + 0.8f * _fmaxf(0.0f, dot(-L, normal)), 0.0f, 1.0f);
  diffuse += amb * to_float3(0.4f, 0.5f, 0.6f);

  return diffuse;
}

__DEVICE__ float3 illuminate(
  _in(vec3) pos,
  _in(vec3) eye,
  _in(mat3) local_xform,
  _in(vec2) df,
  sphere_t planet
){
  // current terrain height at position
  float h = df.y;
  //return to_float3_aw (h);
float tttttttttttttttttttttt;
  float3 w_normal = normalize(pos);
#define LIGHT
#ifdef LIGHT
  float3 normal = sdf_terrain_normal(pos, planet);
  float N = dot(normal, w_normal);
#else
  float N = w_normal.y;
#endif

  // materials
  #define c_water to_float3(0.015f, 0.110f, 0.455f)
  #define c_grass to_float3(0.086f, 0.132f, 0.018f)
  #define c_beach to_float3(0.153f, 0.172f, 0.121f)
  #define c_rock  to_float3(0.080f, 0.050f, 0.030f)
  #define c_snow  to_float3(0.600f, 0.600f, 0.600f)

  // limits
  #define l_water 0.05f
  #define l_shore 0.17f
  #define l_grass 0.211f
  #define l_rock 0.351f

  float s = smoothstep(0.4f, 1.0f, h);
  float3 rock = _mix(
    c_rock, c_snow,
    smoothstep(1.0f - 0.3f*s, 1.0f - 0.2f*s, N));

  float3 grass = _mix(
    c_grass, rock,
    smoothstep(l_grass, l_rock, h));
    
  float3 shoreline = _mix(
    c_beach, grass,
    smoothstep(l_shore, l_grass, h));

  float3 water = _mix(
    c_water / 2.0f, c_water,
    smoothstep(0.0f, l_water, h));

#ifdef LIGHT
  float3 L = mul_mat3_f3(local_xform, normalize(to_float3(1, 1, 0)));
  shoreline *= setup_lights(L, normal);
  float3 ocean = setup_lights(L, w_normal) * water;
#else
  float3 ocean = water;
#endif
  
  return _mix(
              ocean, shoreline,
              smoothstep(l_water, l_shore, h));
}

// ----------------------------------------------------------------------------
// Rendering
// ----------------------------------------------------------------------------
__DEVICE__ mat3 transpose(mat3 m)
{
    return(to_mat3(m.r0.x,m.r1.x,m.r2.x, m.r0.y,m.r1.y,m.r2.y, m.r0.z,m.r1.z,m.r2.z)); 	
}


__DEVICE__ float3 render(
  _in(ray_t) eye,
  _in(vec3) point_cam,
  float u_time,
  float4 u_mouse
){
  
  _mutable(volume_sampler_t) cloud;
  _constant(sphere_t) planet = _begin(sphere_t)
                                {0, 0, 0}, 1.0f, 0
                               _end;
  
  
  mat3 rot_y = rotate_around_y(27.0f);
  mat3 rot = mul_mat3_mat3(rotate_around_x(u_time * -12.0f), rot_y);
  mat3 rot_cloud = mul_mat3_mat3(rotate_around_x(u_time * 8.0f), rot_y);
    if (u_mouse.z > 0.0f) {
        rot = rotate_around_y(-u_mouse.x);
        rot_cloud = rotate_around_y(-u_mouse.x);
        rot = mul_mat3_mat3(rot, rotate_around_x(u_mouse.y));
        rot_cloud = mul_mat3_mat3(rot_cloud, rotate_around_x(u_mouse.y));
    }

  sphere_t atmosphere = planet;
  atmosphere.radius += max_height;

  _constant(hit_t) no_hit = _begin(hit_t)
                                (float)(max_dist + 1e1), // 'infinite' distance
                                -1, // material id
                                {0.0f, 0.0f, 0.0f}, // normal
                                {0.0f, 0.0f, 0.0f} // origin
                            _end;

  hit_t hit = no_hit;
  intersect_sphere(eye, atmosphere, &hit);
  if (hit.material_id < 0) {
    return background(eye);
  }

  float t = 0.0f;
  float2 df = to_float2(1, max_height);
  float3 pos;
  float max_cld_ray_dist = max_ray_dist;
  
  for (int i = 0; i < TERR_STEPS; i++) {
    if (t > max_ray_dist) break;
    
    float3 o = hit.origin + t * eye.direction;
    pos = mul_mat3_f3(rot, o - planet.origin);

    df = sdf_terrain_map(pos, planet);

    if (df.x < TERR_EPS) {
      max_cld_ray_dist = t;
      break;
    }

    t += df.x * 0.4567f;
  }

#ifdef CLOUDS
  cloud = begin_volume(hit.origin, vol_coeff_absorb);
  clouds_march(eye, &cloud, max_cld_ray_dist, rot_cloud, planet);
#endif
  
  if (df.x < TERR_EPS) {
    float3 c_terr = illuminate(pos, eye.direction, rot, df, planet);
    float3 c_cld = cloud.C;
    float alpha = cloud.alpha;
    float shadow = 1.0f;

#ifdef CLOUDS // clouds ground shadows
    pos = mul_mat3_f3(transpose(rot), pos);
    cloud = begin_volume(pos, vol_coeff_absorb);
    float3 local_up = normalize(pos);
    clouds_shadow_march(local_up, &cloud, rot_cloud, planet);
    shadow = _mix(0.7f, 1.0f, step(cloud.alpha, 0.33f));
#endif

    return _mix(c_terr * shadow, c_cld, alpha);
  } else {
    return _mix(background(eye), cloud.C, cloud.alpha);
  }
}

#define FOV _tanf(radians(30.0f))
// ----------------------------------------------------------------------------
// Main Rendering function
// depends on external defines: FOV
// ----------------------------------------------------------------------------

__KERNEL__ void TinyPlanetCloudsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0)
{

  // assuming screen width is larger than height 
  float2 aspect_ratio = to_float2(u_res.x / u_res.y, 1);

  float3 color = to_float3(0, 0, 0);

  float3 eye, look_at;
  setup_camera(&eye, &look_at);

  //setup_scene();

  float2 point_ndc = fragCoord / swi2(u_res,x,y);

  float3 point_cam = to_float3_aw( (2.0f * point_ndc - 1.0f) * aspect_ratio * FOV, -1.0f);

  ray_t ray = get_primary_ray(point_cam, eye, look_at);

  color += render(ray, point_cam, u_time, u_mouse);

  fragColor = to_float4_aw(linear_to_srgb(color), 1);
  
  SetFragmentShaderComputedColor(fragColor);
}
