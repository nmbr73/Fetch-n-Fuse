

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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
const float PI = 3.141592; // close enough
const float EPSILON = 0.0001;
const float NOHIT = 999999999.0; // keep positive intersection miss, so can easily min() the closest one

const int ID_NONE       = 0;
const int ID_SPHERE01   = 1;
const int ID_SPHERE02   = 2;
const int ID_SPHERE03   = 3;
const int ID_PLANE      = 4;

const vec3 sun_dir = normalize(vec3(1,1,1));
const float camera_distance = 1.5;
const float sphere_radius = 0.5;
const vec3 planen = vec3(0,1,0);
const float plane_halfsize = 3.0; // floor "rectangle"

// Animate sphere positions
vec3 sphere_center(int n) {
    vec3 p;
    // rotate around the world center, sphere bottom touching the floor
    p.x = sin(iTime + float(n)*2.*PI/3.)*(sphere_radius + abs(sin(iTime))*sphere_radius);
    p.z = cos(iTime + float(n)*2.*PI/3.)*(sphere_radius + abs(sin(iTime))*sphere_radius);
    p.y = sphere_radius;

    // and.. bounce!
    float ifreq = 2.*PI;
    float dur = 0.75;
    float t = (mod(iTime, ifreq) - (ifreq-dur)) / dur; // [0,1] if bouncing
    if(t >= 0.0) p.y += abs(sin(t*PI)*sphere_radius*2.); // jump!
    return p;
}

// Let's use full 4x4 transformation matrices here.
//
// One way is think them as 4 x vec4: 
//      vec4(xaxis.xyz, translation.x)
//      vec4(yaxis.xyz, translation.y)
//      vec4(zaxis.xyz, translation.z)
//      vec4(0, 0, 0,   1.0)
//
// TODO: open more?
// TODO: looking "top-down" 4D vector .w component is 
//     0.0 for directions (X/Y/Z) and
//     1.0 for locations (T)

// Look from origin "o" to target point "p" with up vector "up"
mat4 lookat(in vec3 o, in vec3 p, in vec3 up)
{
    vec3 delta = p - o;
    vec3 z = normalize(delta); // the direction to look at (Z-axis)
    vec3 x = normalize(cross(z, up)); // -> to-right direction (X-axis)
    vec3 y = normalize(cross(x, z)); // -> to-up direction (Y-axis)

    // let's do it "unwrapped" for now..
    mat4 translation = mat4(
        vec4(1, 0, 0, -o.x),
        vec4(0, 1, 0, -o.y),
        vec4(0, 0, 1, -o.z),
        vec4(0, 0, 0, 1));
        
    mat4 rotation = mat4(
        vec4(x.xyz, 0),
        vec4(y.xyz, 0),
        vec4(z.xyz, 0),
        vec4(0,0,0, 1));

    return rotation * translation;
}

// Transform a 3D direction vector by a 4x4 matrix
vec3 transform_dir(vec3 dir, mat4 m) {
    return (m * vec4(dir, 0.0)).xyz;
}

// Find an intersection between ray ro+t*rd, where t=[0, <NOHIT]
// and a sphere located at "p" with radius "r".
//
// If hits, returns "t", otherwise NOHIT.
float isect_ray_sphere(in vec3 ro, in vec3 rd, in vec3 p, in float r) {
    vec3 oc = ro - p;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - r*r;
    float t = b*b - c;
    float t2 = (t > 0.0) ? -b - sqrt(t) : NOHIT;
    return (t2 > 0.0) ? t2 : NOHIT;
}

// Find an intersection between ray ro+t*rd, where t=[0, <NOHIT]
// and a plane going through point "p" with normal "n".
//
// If hits, returns t >= 0.0, otherwise NOHIT.
float isect_ray_plane(in vec3 ro, in vec3 rd, in vec3 p, in vec3 n) {
    
    float denom = dot(rd, -n);
    return (denom > 0.0) ? -dot(p - ro, n) / denom : NOHIT;
}

// Sample background from cubemap
vec3 background(vec3 dir) {
    return texture(iChannel1, dir.xyz).xyz; 
}

// Returns "t" if hits something, otherwise NOHIT
float hit(in vec3 ro, in vec3 rd, out vec3 hitp, out vec3 hitn, out int id) {
    // scene: 3 spheres + plane
    vec3 scenter1 = sphere_center(0);
    vec3 scenter2 = sphere_center(1);
    vec3 scenter3 = sphere_center(2);

    float sphe1_t = isect_ray_sphere(ro, rd, scenter1, sphere_radius);
    float sphe2_t = isect_ray_sphere(ro, rd, scenter2, sphere_radius);
    float sphe3_t = isect_ray_sphere(ro, rd, scenter3, sphere_radius);
    float plane_t = isect_ray_plane(ro, rd, vec3(0,0,0), planen);
    
    float t = min(sphe1_t, min(sphe2_t, min(sphe3_t, plane_t))); // closest hit or NOHIT
    hitp = ro + t*rd; // world hit point

    // object id + world hit normal
    if( t == NOHIT   ) { id = ID_NONE;     hitn = -rd; } else
    if( t == sphe1_t ) { id = ID_SPHERE01; hitn = normalize(hitp - scenter1); } else
    if( t == sphe2_t ) { id = ID_SPHERE02; hitn = normalize(hitp - scenter2); } else
    if( t == sphe3_t ) { id = ID_SPHERE03; hitn = normalize(hitp - scenter3); } else
    if( t == plane_t ) { id = ID_PLANE;    hitn = planen; } 

    // add some epsilon to position to compensate floating point inaccuracies
    hitp += EPSILON*hitn;
    return t;
}

// path tracing
vec3 trace(in vec3 ro, in vec3 rd) {
    const int maxdepth = 3;
    for(int depth=0; depth < maxdepth; ++depth) {
        int id;
        vec3 hitp; // world position of hit point
        vec3 hitn; // world normal of hit point
        float t = hit(ro, rd, hitp, hitn, id); // sets "hitp", "hitn", "id"

        switch(id) {
            case ID_SPHERE01:
            case ID_SPHERE02:
            case ID_SPHERE03: {
                // hits a sphere - 100% reflective, so continue path to reflection direction
                vec3 reflection = normalize(reflect(rd, hitn)); // reflect the ray around sphere normal
                ro = hitp;
                rd = reflection;
                break;
            }
            default:
                // make up rectangular floor plane area
                if( id == ID_PLANE && abs(hitp.x) < plane_halfsize && abs(hitp.z) < plane_halfsize ) { 
                    vec3 unused_p; vec3 unused_n; int unused_id;
                    float shadowhitt = hit(hitp, sun_dir, unused_p, unused_n, unused_id); // shadow from sun, 1.0 if unblocked
                    vec3 shadowmul = mix(vec3(0.5), vec3(1), min(1.0, 0.25*shadowhitt)); // fake gradient by shadower distance
                    vec3 tex = texture(iChannel0, hitp.xz).xyz; // sample 2D floor texture
                    return shadowmul * tex;
                } else { 
                    // misses scene objects -> background, terminate path
                    return background(rd);
                }
        }
    }
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 rotation = iMouse.xy / iResolution.xy; rotation.y = 1.0 - rotation.y; // Mouse [0,1] + tweak
    vec2 p = (2.0*fragCoord.xy - iResolution.xy) / iResolution.y; // Pixel coordinates y=[-1,1], x=[-1*aspect,1*aspect] where aspect=width/height
    
    vec3 ro = vec3(sin(-rotation.x*PI)*camera_distance, 1.1 + cos(-rotation.y*PI)*1.0, cos(-rotation.x*PI)*camera_distance); // Mouse rotation around the sphere -> ray origin (camera position)
    mat4 m = lookat(ro, vec3(0,sphere_radius,0), vec3(0,1,0)); // Camera->World transformation matrix
    vec3 ldir = normalize(vec3(p, 1.0)); // Local ray direction (Camera Space)
    vec3 rd = transform_dir(ldir, m); // -> World Space

    vec3 c = trace(ro, rd);

    fragColor = vec4(c, 1.0);
}