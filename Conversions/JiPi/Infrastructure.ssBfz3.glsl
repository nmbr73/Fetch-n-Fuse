

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 100.0
#define MIN_DIST 0.001
#define MAX_DIST 75.0
#define pi 3.14159
#define oz vec2(0.01,0)
#define rot(a) mat2(cos(a), sin(a), -sin(a), cos(a))
#define c1 mix(2.0*vec4(0.3, 0.25, 0.2, 0), vec4(0.5), 0.75)
#define c2 mix(2.0*vec4(0.2, 0.25, 0.3, 1.0), vec4(0.5), 0.75)
#define sat(t) clamp(t, 0.0, 1.0)

// Ray marching result struct
struct RayMarch {
    float dist;     // Raymarch distance
    float steps;    // Raymarch steps
    vec3 pos;       // Surface position
    vec3 normal;    // Surface normal
    vec4 col;       // Surface color
};

// Random [0,1]
float rand(vec2 p) {
	return fract(sin(dot(p, vec2(12.543,514.123)))*4732.12);
}

// 3D Box SDF by iq
float box(vec2 p, vec2 r) {
    vec2 d = abs(p) - r;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// Greeble surface detail
float greeble(vec3 p) {
    float r = 0.0;
    for (int i = 0; i < 4; i++) {
        p += 2.*rand(floor(p.xz));
        r += sign(sin(p.x)*sin(p.y)*sin(p.z));
        p *= 2.0;
    }
    return r;
}

// Scene distance function
float scene(vec3 p) {
    float gr = 0.01*greeble(0.5*p);
    vec3 pc = p;
    pc.z = mod(pc.z,40.0)-20.0;
    p.xy = abs(p.xy);
    p.x = mod(p.x,2.0)-1.0;
    p.z = mod(p.z,2.0)-1.0;
    p.y = mod(p.y,2.0)-.0;
    float r = 1.0-p.y;
    r = max(r, -box(pc.yz, vec2(2.5,10.)));
    r = min(r, box(p.xz, vec2(0.1)));
    r = min(r, box(p.xy-vec2(0,1), vec2(0.1)));
    r = min(r, box(p.zy-vec2(0,1), vec2(0.1)));
    return 0.9*r+gr;
}

// Texturing
vec4 textureColor(vec3 p, vec3 n) {
    int axis = (abs(n.y)>=0.9 ? 0 : (abs(n.z)>=0.9 ? 1 : 2));
    vec2 tc = (axis==0 ? p.xz : (axis==1 ? p.xy : p.yz));
    tc *= 0.5;
    tc += 0.1*p.z;
    tc += 0.25*p.y;
    vec4 col = texture(iChannel0, tc);
    col *= col*col;
    col = vec4(3.0*col.r);
    return sat(col);
}

// Ray marching
RayMarch march(vec3 cam, vec3 ray) {
    float dist = 0.0;
    float d = 0.0;
    float steps = 0.0;
    vec3 p;
    while (steps < MAX_STEPS) {
        p = cam + ray * dist;
        d = scene(p);
        dist += d;
        if (d < MIN_DIST || dist > MAX_DIST) break;
        steps++;
    }
    vec3 n = normalize(d-vec3(scene(p-oz.xyy), scene(p-oz.yxy), scene(p-oz.yyx)));
    vec4 col = textureColor(p,n);
    return RayMarch(dist, steps, p, n, col);

}

// Main
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord/iResolution.y;
    uv -= iResolution.xy/iResolution.y/2.0;
    // Camera setup
    vec3 cam = vec3(0);
    cam.z += 0.5*iTime;
    vec3 ray = normalize(vec3(uv.x, uv.y, 0.8));
    float rx = (iMouse.z >= 0.5 ? iMouse.x/iResolution.x*2.0*pi-pi
                : 0.25*sin(0.2*iTime));
    float ry = (iMouse.z >= 0.5 ? iMouse.y/iResolution.y*pi-pi/2.0
                : sin(0.3*iTime)*smoothstep(-1.0, 1.0, sin(0.6*iTime)));
    ray.yz *= rot(ry);
    ray.xz *= rot(rx);
    ray.xy *= rot(pi/2.0);
    // Ray march
    RayMarch rm = march(cam, ray);
    // Lighting
    float l1 = max(dot(rm.normal, normalize(vec3(1, -1.0, 0))), 0.0);
    float l2 = max(dot(rm.normal, normalize(vec3(1, 1.0, 0))), 0.0);
    float l3 = max(dot(rm.normal, normalize(vec3(0, 0, -1))), 0.0);
    float l4 = max(dot(rm.normal, normalize(vec3(-1, 0, 0))), 0.0);
    float l5 = max(dot(rm.normal, normalize(vec3(0, 0, 1))), 0.0);
    vec4 l = rm.col*2.0*c1*((l1+l2)+0.3*(l3+l5)+0.2*l4);
    float drk = sat(rm.pos.x/15.0+1.0);
    drk *= drk*drk;
    float lght = sat(rm.pos.x/20.0-0.1);
    float d = 1.0-rm.dist/MAX_DIST;
    d *= d*d*d;
    // Final composition
    fragColor = drk*l*d+2.5*c1*lght;
}