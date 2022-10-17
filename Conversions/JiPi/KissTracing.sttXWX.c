
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rock Tiles' to iChannel0
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Copyright (c) Timo Saarinen 2021
// You can use this Work in a Good and Cool Spirit.
//
// KISS Path Tracing 001: Spheres and a Plane
//------------------------------------------------------------------------
// There are two main approaches to rendering, rasterization 
// and ray tracing (RT). Like the name says, ray tracing is 
// just tracing through 3D space (usually), simple!
//
// Path Tracing is a type of ray tracing,
// and at its heart.. very simple!
//
// From wikipedia [https://en.wikipedia.org/wiki/Path_tracing]:
//   Path tracing is a computer graphics Monte Carlo method of rendering images of three-dimensional scenes such that the global illumination is faithful to reality. Fundamentally, the algorithm is integrating over all the illuminance arriving to a single point on the surface of an object. This illuminance is then reduced by a surface reflectance function (BRDF) to determine how much of it will go towards the viewpoint camera. This integration procedure is repeated for every pixel in the output image. When combined with physically accurate models of surfaces, accurate models of real light sources (light bulbs), and optically correct cameras, path tracing can produce still images that are indistinguishable from photographs.
//   Path tracing naturally simulates many effects that have to be specifically added to other methods (conventional ray tracing or scanline rendering), such as soft shadows, depth of field, motion blur, caustics, ambient occlusion, and indirect lighting. Implementation of a renderer including these effects is correspondingly simpler. An extended version of the algorithm is realized by volumetric path tracing, which considers the light scattering of a scene.
//   Due to its accuracy, unbiased nature, and algorithmic simplicity, path tracing is used to generate reference images when testing the quality of other rendering algorithms. However, the path tracing algorithm is relatively inefficient: a very large number of rays must be traced to get high-quality images free of noise artifacts. Several variants have been introduced which are more efficient than the original algorithm for many scenes, including bidirectional path tracing, volumetric path tracing, and Metropolis light transport. 
//
// Spheres and planes are the common first examples of ray tracing,
// with simple intersection code, so let's start with them..
//
// Unoptimized for clarity! Also disclaimer: WIP
#define PI  3.141592f        // close enough
#define EPSILON  0.0001f
#define NOHIT  999999999.0f  // keep positive intersection miss, so can easily _fminf() the closest one

#define ID_NONE        0
#define ID_SPHERE01    1
#define ID_SPHERE02    2
#define ID_SPHERE03    3
#define ID_PLANE       4



// Animate sphere positions
__DEVICE__ float3 sphere_center(int n, float sphere_radius, float iTime) {
    float3 p;
    // rotate around the world center, sphere bottom touching the floor
    p.x = _sinf(iTime + (float)(n)*2.0f*PI/3.0f)*(sphere_radius + _fabs(_sinf(iTime))*sphere_radius);
    p.z = _cosf(iTime + (float)(n)*2.0f*PI/3.0f)*(sphere_radius + _fabs(_sinf(iTime))*sphere_radius);
    p.y = sphere_radius;

    // and.. bounce!
    float ifreq = 2.0f*PI;
    float dur = 0.75f;
    float t = (mod_f(iTime, ifreq) - (ifreq-dur)) / dur; // [0,1] if bouncing
    if(t >= 0.0f) p.y += _fabs(_sinf(t*PI)*sphere_radius*2.0f); // jump!
    return p;
}

// Let's use full 4x4 transformation matrices here.
//
// One way is think them as 4 x vec4: 
//      to_float4(swi3(xaxis,x,y,z), translation.x)
//      to_float4(swi3(yaxis,x,y,z), translation.y)
//      to_float4(swi3(zaxis,x,y,z), translation.z)
//      to_float4(0, 0, 0,   1.0f)
//
// TODO: open more?
// TODO: looking "top-down" 4D vector .w component is 
//     0.0f for directions (X/Y/Z) and
//     1.0f for locations (T)

// Look from origin "o" to target point "p" with up vector "up"
__DEVICE__ mat4 lookat(in float3 o, in float3 p, in float3 up)
{
    float3 delta = p - o;
    float3 z = normalize(delta); // the direction to look at (Z-axis)
    float3 x = normalize(cross(z, up)); // -> to-right direction (X-axis)
    float3 y = normalize(cross(x, z)); // -> to-up direction (Y-axis)

    // let's do it "unwrapped" for now..
    mat4 translation = to_mat4_f4(
        to_float4(1, 0, 0, -o.x),
        to_float4(0, 1, 0, -o.y),
        to_float4(0, 0, 1, -o.z),
        to_float4(0, 0, 0, 1));
        
    mat4 rotation = to_mat4_f4(
        to_float4_aw(swi3(x,x,y,z), 0),
        to_float4_aw(swi3(y,x,y,z), 0),
        to_float4_aw(swi3(z,x,y,z), 0),
        to_float4(0,0,0, 1));

    return mul_mat4_mat4(rotation , translation);
}

// Transform a 3D direction vector by a 4x4 matrix
__DEVICE__ float3 transform_dir(float3 dir, mat4 m) {
    return swi3((mul_mat4_f4(m , to_float4_aw(dir, 0.0f))),x,y,z);
}

// Find an intersection between ray ro+t*rd, where t=[0, <NOHIT]
// and a sphere located at "p" with radius "r".
//
// If hits, returns "t", otherwise NOHIT.
__DEVICE__ float isect_ray_sphere(in float3 ro, in float3 rd, in float3 p, in float r) {
    float3 oc = ro - p;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - r*r;
    float t = b*b - c;
    float t2 = (t > 0.0f) ? -b - _sqrtf(t) : NOHIT;
    return (t2 > 0.0f) ? t2 : NOHIT;
}

// Find an intersection between ray ro+t*rd, where t=[0, <NOHIT]
// and a plane going through point "p" with normal "n".
//
// If hits, returns t >= 0.0f, otherwise NOHIT.
__DEVICE__ float isect_ray_plane(in float3 ro, in float3 rd, in float3 p, in float3 n) {
    
    float denom = dot(rd, -n);
    return (denom > 0.0f) ? -dot(p - ro, n) / denom : NOHIT;
}

// Sample background from cubemap
__DEVICE__ float3 background(float3 dir, __TEXTURE2D__ iChannel1) {
    return swi3(decube_f3(iChannel1, dir),x,y,z); 
}

// Returns "t" if hits something, otherwise NOHIT
__DEVICE__ float hit(in float3 ro, in float3 rd, out float3 *hitp, out float3 *hitn, out int *id, float sphere_radius, float3 planen, float iTime) {
    // scene: 3 spheres + plane
    float3 scenter1 = sphere_center(0,sphere_radius,iTime);
    float3 scenter2 = sphere_center(1,sphere_radius,iTime);
    float3 scenter3 = sphere_center(2,sphere_radius,iTime);

    float sphe1_t = isect_ray_sphere(ro, rd, scenter1, sphere_radius);
    float sphe2_t = isect_ray_sphere(ro, rd, scenter2, sphere_radius);
    float sphe3_t = isect_ray_sphere(ro, rd, scenter3, sphere_radius);
    float plane_t = isect_ray_plane(ro, rd, to_float3(0,0,0), planen);
    
    float t = _fminf(sphe1_t, _fminf(sphe2_t, _fminf(sphe3_t, plane_t))); // closest hit or NOHIT
    *hitp = ro + t*rd; // world hit point

    // object id + world hit normal
    if( t == NOHIT   ) { *id = ID_NONE;     *hitn = -rd; } else
    if( t == sphe1_t ) { *id = ID_SPHERE01; *hitn = normalize(*hitp - scenter1); } else
    if( t == sphe2_t ) { *id = ID_SPHERE02; *hitn = normalize(*hitp - scenter2); } else
    if( t == sphe3_t ) { *id = ID_SPHERE03; *hitn = normalize(*hitp - scenter3); } else
    if( t == plane_t ) { *id = ID_PLANE;    *hitn = planen; } 

    // add some epsilon to position to compensate floating point inaccuracies
    *hitp += EPSILON* *hitn;
    return t;
}

// path tracing
__DEVICE__ float3 trace(in float3 ro, in float3 rd, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float sphere_radius, float3 planen, float plane_halfsize, float3 sun_dir, float iTime, float ratio) {
    const int maxdepth = 3;
    for(int depth=0; depth < maxdepth; ++depth) {
        int id;
        float3 hitp; // world position of hit point
        float3 hitn; // world normal of hit point
        float t = hit(ro, rd, &hitp, &hitn, &id, sphere_radius,planen,iTime); // sets "hitp", "hitn", "id"

        switch(id) {
            case ID_SPHERE01:
            case ID_SPHERE02:
            case ID_SPHERE03: {
                // hits a sphere - 100% reflective, so continue path to reflection direction
                float3 reflection = normalize(reflect(rd, hitn)); // reflect the ray around sphere normal
                ro = hitp;
                rd = reflection;
                break;
            }
            default:
                // make up rectangular floor plane area
                if( id == ID_PLANE && _fabs(hitp.x) < plane_halfsize && _fabs(hitp.z) < plane_halfsize ) { 
                    float3 unused_p; float3 unused_n; int unused_id;
                    float shadowhitt = hit(hitp, sun_dir, &unused_p, &unused_n, &unused_id, sphere_radius,planen,iTime); // shadow from sun, 1.0f if unblocked
                    float3 shadowmul = _mix(to_float3_s(0.5f), to_float3_s(1), _fminf(1.0f, 0.25f*shadowhitt)); // fake gradient by shadower distance
                    float2 tuv = to_float2( hitp.x*ratio, hitp.z);
                    //float3 tex = swi3(texture(iChannel0, swi2(hitp,x,z)),x,y,z); // sample 2D floor texture
                    float3 tex = swi3(texture(iChannel0, tuv),x,y,z); // sample 2D floor texture
                    return shadowmul * tex;
                } else { 
                    // misses scene objects -> background, terminate path
                    return background(rd,iChannel1);
                }
        }
    }
    return to_float3_s(0.0f);
}

__KERNEL__ void KissTracingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
  
    const float3 sun_dir = normalize(to_float3(1,1,1));
    const float camera_distance = 1.5f;
    const float sphere_radius = 0.5f;
    const float3 planen = to_float3(0,1,0);
    const float plane_halfsize = 3.0f; // floor "rectangle"
    float ratio = iResolution.y/iResolution.x;

    float2 rotation = swi2(iMouse,x,y) / iResolution; rotation.y = 1.0f - rotation.y; // Mouse [0,1] + tweak
    float2 p = (2.0f*fragCoord - iResolution) / iResolution.y; // Pixel coordinates y=[-1,1], x=[-1*aspect,1*aspect] where aspect=width/height

    float3 ro = to_float3(_sinf(-rotation.x*PI)*camera_distance, 1.1f + _cosf(-rotation.y*PI)*1.0f, _cosf(-rotation.x*PI)*camera_distance); // Mouse rotation around the sphere -> ray origin (camera position)
    mat4 m = lookat(ro, to_float3(0,sphere_radius,0), to_float3(0,1,0)); // Camera->World transformation matrix
    float3 ldir = normalize(to_float3_aw(p, 1.0f)); // Local ray direction (Camera Space)
    float3 rd = transform_dir(ldir, m); // -> World Space

    float3 c = trace(ro, rd,iChannel0,iChannel1, sphere_radius, planen, plane_halfsize, sun_dir,iTime, ratio);

    fragColor = to_float4_aw(c, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}