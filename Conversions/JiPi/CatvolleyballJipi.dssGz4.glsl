

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

#define FOV (60.0)
#define LIGHT_DIR vec3(-1.0, 1.0, -1.0)
#define STEPS (64)
#define GRID_SIZE (2.0)
#define RAY_LEN (float(STEPS) * GRID_SIZE * 0.25)

float pow2(float x) {return x * x;}
float pow4(float x) {return pow2(x) * pow2(x);}
float pow5(float x) {return pow4(x) * x;}

Camera GetCamera()
{
    vec3 forward;
    if (iMouse.z > 0.5)
    {
        forward = MouseRotation(iMouse.xy, iResolution.xy);
    }
    else
    {
        const float speed = 0.125;
        forward.y = sin(0.618 * speed * iTime) * 0.35;
        forward.xz = vec2(-sin(iTime * speed), cos(iTime * speed));
        forward = normalize(forward);
    }
    Camera cam;
    cam.forward = forward;
    cam.up = vec3(0.0, 1.0, 0.0);
    cam.position = -(GRID_SIZE * 2.0) * cam.forward;
    return cam;
}

vec3 GetFogColor(vec3 rd)
{
    return mix(vec3(0.0), vec3(1.0), rd.y * 0.5 + 0.5);
}

float GetFogIntensity(float t, float start, float decay)
{
    return 1.0 - pow(1.0 - pow2(max((t - start) / (1.0 - start), 0.0)), decay);
}

vec4 SampleVolleyball(vec3 dirVec)
{
    vec2 uv0 = dirVec.xy;
    vec2 uv1 = dirVec.yz;
    vec2 uv2 = dirVec.zx;
    uv0.x = (uv0.x * 0.3 + 0.5) * sign(dirVec.z) + iTime * -0.125;
    uv1.x = (uv1.x * 0.3 + 0.5) * sign(dirVec.x) + iTime * -0.125;
    uv2.x = (uv2.x * 0.3 + 0.5) * sign(dirVec.y) + iTime * -0.125;
    uv0.y = (uv0.y * 0.8 + 0.5);
    uv1.y = (uv1.y * 0.8 + 0.5);
    uv2.y = (uv2.y * 0.8 + 0.5);
    vec4 col0 = texture(iChannel0, uv0);
    vec4 col1 = texture(iChannel0, uv1);
    vec4 col2 = texture(iChannel0, uv2);
    col0.rgb = pow(col0.rgb, vec3(2.2));
    col1.rgb = pow(col1.rgb, vec3(2.2));
    col2.rgb = pow(col2.rgb, vec3(2.2));
    //col0 *= pow2(dirVec.z);
    //col1 *= pow2(dirVec.x);
    //col2 *= pow2(dirVec.y);
    col0 *= smoothstep(0.577350, 0.6, abs(dirVec.z)) * smoothstep(0.577350, 0.55, abs(dirVec.y));
    col1 *= smoothstep(0.577350, 0.6, abs(dirVec.x)) * smoothstep(0.577350, 0.55, abs(dirVec.z));
    col2 *= smoothstep(0.577350, 0.6, abs(dirVec.y)) * smoothstep(0.577350, 0.55, abs(dirVec.x));
    return col0 + col1 + col2;
}

void SampleMaterial(vec3 N, vec3 C, out vec3 D, out vec3 S, out float R)
{
    vec4 mainTex = SampleVolleyball(N);
    vec3 albedo = mainTex.rgb * C;
    float metallic = smoothstep(0.0, 1.0, 1.0 - mainTex.b);
    D = albedo * (1.0 - metallic);
    S = mix(vec3(0.04), albedo, metallic);
    R = 1.0 - mainTex.r;
    R = max(R, 0.04);
}

float Geometry(float NoV, float NoL, float k)
{
    float gv = 0.5 / (NoV * (1.0 - k) + k);
    float gl = 0.5 / (NoL * (1.0 - k) + k);
    return gv * gl;
}

float Distribution(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (denom * denom);
}

vec3 Fresnel(float NoV, vec3 F0)
{
    return F0 + (1.0 - F0) * pow5(1.0 - NoV);
}

vec3 DirectSpecular(float NoV, float NoL, float NoH, float HoV, vec3 F0, float R)
{
    float k = pow2(R + 1.0) / 8.0;
    float G = Geometry(NoV, NoL, k);
    float D = Distribution(NoH, R * R);
    vec3 F = Fresnel(HoV, F0);
    return G * D * F;
}

vec3 DirectRadiance(vec3 N, vec3 V, vec3 L, vec3 D, vec3 S, float R)
{
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float HoV = max(dot(H, V), 0.0);
    vec3 radiance = D;
    radiance += DirectSpecular(NoV, NoL, NoH, HoV, S, R);
    radiance *= NoL;
    return radiance;
}

vec3 IndirectRadiance(vec3 N, vec3 V, vec3 D, vec3 S, float R)
{
    vec3 reflVec = reflect(-V, N);
    float NoV = max(dot(N, V), 0.0);
    vec3 F = Fresnel(NoV, S);
    float lod = sqrt(R);
    lod = 1.0 - (1.0 - lod) * sqrt(NoV);
    vec3 radiance = vec3(0.0);
    radiance += F * pow(textureLod(iChannel1, reflVec, lod * 8.0).rgb, vec3(2.2));
    radiance += D * pow(textureLod(iChannel1, N, 8.0).rgb, vec3(2.2));
    return radiance;
}

float RayCastSphere(vec3 ro, vec3 rd, vec4 sphere, out vec3 N)
{
    vec3 p = sphere.xyz - ro;
    float a = dot(rd, rd);
    float b = dot(rd, p);
    float c = dot(p, p) - pow2(sphere.w);
    float delta = b * b - a * c;
    if (delta < 0.0) return -1.0;
    float t = (b - sqrt(delta)) / a;
    vec3 P = ro + rd * t;
    N = normalize(P - sphere.xyz);
    return t;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    float aspect = iResolution.x / iResolution.y;
    Camera cam = GetCamera();
    
    vec3 ro = cam.position;
    vec3 rd = MakeWorldRay(uv, aspect, FOV, cam);
    
    vec4 sphere = vec4(0.0, 0.0, 0.0, 0.5);
    
    vec3 col = vec3(0.0);
    
    float hitDist = -1.0;
    vec3 hitNormal = vec3(0.0);
    for (int i = 0; i < STEPS; ++i)
    {
        float t = (float(i) + 0.5) / float(STEPS);
        vec3 pos = ro + rd * (t * RAY_LEN);

        vec3 gpos = fract(pos / GRID_SIZE + 0.5);
        gpos -= 0.5;
        gpos *= GRID_SIZE;

        hitDist = RayCastSphere(gpos, rd, sphere, hitNormal);
        if (hitDist > 0.0)
        {
            hitDist += (t * RAY_LEN);
            hitDist = clamp(hitDist, 0.0, RAY_LEN);
            break;
        }
    }
    
    if (hitDist > 0.0)
    {
        vec3 lightDir = normalize(LIGHT_DIR);
        vec3 D, S;
        float R;
        SampleMaterial(hitNormal, vec3(1.0), D, S, R);
        vec3 sphCol = vec3(0.0);
        sphCol += DirectRadiance(hitNormal, -rd, lightDir, D, S, R);
        sphCol += IndirectRadiance(hitNormal, -rd, D, S, R);
        
        float t = hitDist / RAY_LEN;
        col = mix(sphCol, GetFogColor(rd), GetFogIntensity(t, 0.1, 0.5));
    }
    else
    {
        col = GetFogColor(rd);
    }

    col = pow(col, vec3(1.0 / 2.2));
    fragColor = vec4(col, 1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define PI (3.14159265359)
#define HALF_PI (PI * 0.5)
#define DOUBLE_PI (PI * 2.0)
#define DEG2RAD (PI / 180.0)

struct Camera
{
    vec3 position;
    vec3 forward;
    vec3 up;
};

mat3 LookAt(vec3 forward, vec3 up)
{
    mat3 m;
    m[2] = forward;
    m[0] = normalize(cross(up, m[2]));
    m[1] = cross(m[2], m[0]);
    return m;
}

vec3 MakeViewRay(vec2 uv, float aspect, float fov)
{
    vec3 ray = vec3(uv * 2.0 - 1.0, 1.0);
    ray.x *= aspect;
    ray.xy *= tan(fov * 0.5 * DEG2RAD);
    return normalize(ray);
}

vec3 MakeWorldRay(vec2 uv, float aspect, float fov, Camera cam)
{
    vec3 ray = MakeViewRay(uv, aspect, fov);
    return LookAt(cam.forward, cam.up) * ray;
}

vec3 MouseRotation(vec2 screenPos, vec2 screenSize)
{
    vec2 muv = screenPos / screenSize;
    muv = muv * 2.0 - 1.0;
    muv.x *= screenSize.x / screenSize.y;
    muv *= vec2(PI, HALF_PI);
    muv.y = clamp(muv.y, -0.99 * HALF_PI, 0.99 * HALF_PI);
    vec4 rot = vec4(cos(muv.x), sin(muv.x), cos(muv.y), sin(muv.y));
    return normalize(vec3(rot.y * rot.z, rot.w, rot.x * rot.z));
}


