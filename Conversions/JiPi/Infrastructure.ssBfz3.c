
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0


#define MAX_STEPS 100.0f
#define MIN_DIST 0.001f
#define MAX_DIST 75.0f
#define pi 3.14159f
#define oz to_float2(0.01f,0)
#define rot(a) to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))
#define c1 _mix(2.0f*to_float4(0.3f, 0.25f, 0.2f, 0), to_float4_s(0.5f), 0.75f)
#define c2 _mix(2.0f*to_float4(0.2f, 0.25f, 0.3f, 1.0f), to_float4_s(0.5f), 0.75f)
#define sat(t) clamp(t, 0.0f, 1.0f)

// Ray marching result struct
struct RayMarch {
    float dist;     // Raymarch distance
    float steps;    // Raymarch steps
    float3 pos;       // Surface position
    float3 normal;    // Surface normal
    float4 col;       // Surface color
};

// Random [0,1]
__DEVICE__ float rand(float2 p) {
  return fract(_sinf(dot(p, to_float2(12.543f,514.123f)))*4732.12f);
}

// 3D Box SDF by iq
__DEVICE__ float box(float2 p, float2 r) {
    float2 d = abs_f2(p) - r;
    return length(_fmaxf(d, to_float2_s(0.0f))) + _fminf(max(d.x, d.y), 0.0f);
}

// Greeble surface detail
__DEVICE__ float greeble(float3 p) {
    float r = 0.0f;
    for (int i = 0; i < 4; i++) {
        p += 2.0f*rand(_floor(swi2(p,x,z)));
        r += sign_f(_sinf(p.x)*_sinf(p.y)*_sinf(p.z));
        p *= 2.0f;
    }
    return r;
}

// Scene distance function
__DEVICE__ float scene(float3 p) {
    float gr = 0.01f*greeble(0.5f*p);
    float3 pc = p;
    pc.z = mod_f(pc.z,40.0f)-20.0f;
    swi2S(p,x,y, abs_f2(swi2(p,x,y)));
    p.x = mod_f(p.x,2.0f)-1.0f;
    p.z = mod_f(p.z,2.0f)-1.0f;
    p.y = mod_f(p.y,2.0f)-0.0f;
    float r = 1.0f-p.y;
    r = _fmaxf(r, -box(swi2(pc,y,z), to_float2(2.5f,10.0f)));
    r = _fminf(r, box(swi2(p,x,z), to_float2_s(0.1f)));
    r = _fminf(r, box(swi2(p,x,y)-to_float2(0,1), to_float2_s(0.1f)));
    r = _fminf(r, box(swi2(p,z,y)-to_float2(0,1), to_float2_s(0.1f)));
    return 0.9f*r+gr;
}

// Texturing
__DEVICE__ float4 textureColor(float3 p, float3 n, __TEXTURE2D__ iChannel0) {
    int axis = (_fabs(n.y)>=0.9f ? 0 : (_fabs(n.z)>=0.9f ? 1 : 2));
    float2 tc = (axis==0 ? swi2(p,x,z) : (axis==1 ? swi2(p,x,y) : swi2(p,y,z)));
    tc *= 0.5f;
    tc += 0.1f*p.z;
    tc += 0.25f*p.y;
    float4 col = _tex2DVecN(iChannel0,tc.x,tc.y,15);
    col *= col*col;
    col = to_float4_s(3.0f*col.x);
    return sat(col);
}

// Ray marching
__DEVICE__ RayMarch march(float3 cam, float3 ray, __TEXTURE2D__ iChannel0) {
    float dist = 0.0f;
    float d = 0.0f;
    float steps = 0.0f;
    float3 p;
    while (steps < MAX_STEPS) {
        p = cam + ray * dist;
        d = scene(p);
        dist += d;
        if (d < MIN_DIST || dist > MAX_DIST) break;
        steps++;
    }
    float3 n = normalize(d-to_float3(scene(p-swi3(oz,x,y,y)), scene(p-swi3(oz,y,x,y)), scene(p-swi3(oz,y,y,x))));
    float4 col = textureColor(p,n,iChannel0);
    
    RayMarch ret = {dist, steps, p, n, col};
    
    return ret;

}

// Main
__KERNEL__ void InfrastructureFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 

    float2 uv = fragCoord/iResolution.y;
    uv -= iResolution/iResolution.y/2.0f;
    // Camera setup
    float3 cam = to_float3_s(0);
    cam.z += 0.5f*iTime;
    float3 ray = normalize(to_float3(uv.x, uv.y, 0.8f));
    float rx = (iMouse.z >= 0.5f ? iMouse.x/iResolution.x*2.0f*pi-pi
                                 : 0.25f*_sinf(0.2f*iTime));
    float ry = (iMouse.z >= 0.5f ? iMouse.y/iResolution.y*pi-pi/2.0
                                 : _sinf(0.3f*iTime)*smoothstep(-1.0f, 1.0f, _sinf(0.6f*iTime)));
    swi2S(ray,y,z, mul_f2_mat2(swi2(ray,y,z) , rot(ry)));
    swi2S(ray,x,z, mul_f2_mat2(swi2(ray,x,z) , rot(rx)));
    swi2S(ray,x,y, mul_f2_mat2(swi2(ray,x,y) , rot(pi/2.0f)));
    // Ray march
    RayMarch rm = march(cam, ray,iChannel0);
    // Lighting
    float l1 = _fmaxf(dot(rm.normal, normalize(to_float3(1, -1.0f, 0))), 0.0f);
    float l2 = _fmaxf(dot(rm.normal, normalize(to_float3(1, 1.0f, 0))), 0.0f);
    float l3 = _fmaxf(dot(rm.normal, normalize(to_float3(0, 0, -1))), 0.0f);
    float l4 = _fmaxf(dot(rm.normal, normalize(to_float3(-1, 0, 0))), 0.0f);
    float l5 = _fmaxf(dot(rm.normal, normalize(to_float3(0, 0, 1))), 0.0f);
    float4 l = rm.col*2.0f*c1*((l1+l2)+0.3f*(l3+l5)+0.2f*l4);
    float drk = sat(rm.pos.x/15.0f+1.0f);
    drk *= drk*drk;
    float lght = sat(rm.pos.x/20.0f-0.1f);
    float d = 1.0f-rm.dist/MAX_DIST;
    d *= d*d*d;
    // Final composition
    fragColor = drk*l*d+2.5f*c1*lght;

    fragColor = fragColor + (Color-0.5f);
    fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}