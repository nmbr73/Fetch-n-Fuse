

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define int2 vec2
#define float2 vec2
#define int3 vec3
#define float3 vec3
#define int4 vec4
#define float4 vec4
#define frac fract
#define float2x2 mat2
#define float3x3 mat3
#define saturate(x) clamp(x,0.,1.)
#define lerp mix
#define CurrentTime (iTime)
#define sincos(x,s,c) s = sin(x),c = cos(x)
#define mul(x,y) (x*y)
#define atan2 atan
#define fmod mod

const float PId2 = 1.57079632f;
const float PI = 3.141592653f;
const float PI2 = 6.2831853f;
const float MaxDist = 100.f;
const float SurfaceDist = 0.0001f;

#define RayMarching(origin, dir, distFunc, iter, res, more) {\
    float hitDist = 0.f;\
    float mat = 0.f;\
    for (int i = 0;i < iter; ++i){\
        float3 ray = origin + hitDist * dir;\
        float2 curr = distFunc(ray);\
        if (hitDist > MaxDist || curr.x < SurfaceDist)\
            break;\
        {more;}\
        mat = curr.y;\
        hitDist += curr.x;}\
    res = float2(hitDist, mat);}

#define GetNormal(ray, distFunc, res) {\
    float2 k = float2(1.f, -1.f);\
    res = normalize(k.xyy * distFunc(ray + k.xyy * SurfaceDist).x +\
                     k.yyx * distFunc(ray + k.yyx * SurfaceDist).x +\
                     k.yxy * distFunc(ray + k.yxy * SurfaceDist).x +\
                     k.xxx * distFunc(ray + k.xxx * SurfaceDist).x);}

float3x3 viewMatrix(float3 look)
{
    float3 right = normalize(cross(float3(0.f, 1.f, 0.f), look));
    float3 up = cross(look, right);
    return float3x3(right, up, look);
}

float2 minX(float2 a, float2 b)
{
    return a.x < b.x ? a : b;
}

float min3(float a, float b, float c)
{
    return min(a, min(b, c));
}

float min4(float a, float b, float c, float d)
{
    return min(min3(a, b, c), d);
}

float2 hash(float2 p)
{
    p = float2(dot(p, float2(127.1f, 311.7f)), dot(p, float2(269.5f, 183.3f)));
    return 2.f * frac(sin(p) * 43758.5453123f) - 1.f;
}

float3 hash(float3 p)
{
    p = float3(dot(p, float3(127.1f, 311.7f, 74.7f)), dot(p, float3(269.5f, 183.3f, 246.1f)), dot(p, float3(113.5f, 271.9f, 124.6f)));
    return 2.f * frac(sin(p) * 43758.5453123) - 1.f;
}

float sdf3dSphere(float3 _point, float4 sphere)
{
    return length(_point - sphere.xyz) - sphere.w;
}

float sdf3dInfPlane(float3 _point, float3 plane, float3 planeNormal)
{
    return dot((_point - plane), planeNormal);
}

float sdf3dInfCylinder(float3 _point, float4 cylinder, float3 cylinderDirection)
{
    _point -= cylinder.xyz;
    return length(_point - dot(_point, cylinderDirection) * cylinderDirection) - cylinder.w;
}

float simplexNoise(float3 p)
{
    float k1 = 0.333333f;
    float k2 = 0.166667f;
    
    int3 idx = floor(p + (p.x + p.y + p.z) * k1);
    float3 a = p - (float3(idx) - float(idx.x + idx.y + idx.z) * k2);
    
    int3 tb1Arr[8] = vec3[8]
    ( int3(0, 0, 1), int3(0, 1, 0), int3( 0), int3(0, 1, 0), int3(0, 0, 1), int3( 0), int3(1, 0, 0), int3(1, 0, 0) );
    int3 tb2Arr[8] = vec3[8]
    ( int3(0, 1, 1), int3(0, 1, 1), int3( 0), int3(1, 1, 0), int3(1, 0, 1), int3( 0), int3(1, 0, 1), int3(1, 1, 0) );
    
    uint tbIdx = (uint(a.x > a.y) << 2) | (uint(a.x > a.z) << 1) | uint(a.y > a.z);
    
    int3 tb1 = tb1Arr[tbIdx], tb2 = tb2Arr[tbIdx];
    
    float3 b = a - tb1 + k2;
    float3 c = a - tb2 + k2 * 2.f;
    float3 d = a - 1.f + k2 * 3.f;
    
    float4 kernel = max(0.5f - float4(dot(a, a), dot(b, b), dot(c, c), dot(d, d)), 0.f);
    kernel *= kernel;
    kernel *= kernel;
    float4 noise = kernel * float4(dot(a, hash(idx)), dot(b, hash(idx + tb1)), dot(c, hash(idx + tb2)), dot(d, hash(idx + 1.f)));
    
    return dot(vec4(52.f), noise);
}

float simplexVoronoi(float3 p, float m)
{
    float k1 = 0.333333f;
    float k2 = 0.166667f;
    
    int3 idx = floor(p + (p.x + p.y + p.z) * k1);
    float3 a = p - (float3(idx) - float(idx.x + idx.y + idx.z) * k2);
    
    int3 tb1Arr[8] = vec3[8]
    ( int3(0, 0, 1), int3(0, 1, 0), int3( 0), int3(0, 1, 0), int3(0, 0, 1), int3( 0), int3(1, 0, 0), int3(1, 0, 0) );
    int3 tb2Arr[8] = vec3[8]
    ( int3(0, 1, 1), int3(0, 1, 1), int3( 0), int3(1, 1, 0), int3(1, 0, 1), int3( 0), int3(1, 0, 1), int3(1, 1, 0) );
    
    uint tbIdx = (uint(a.x > a.y) << 2) | (uint(a.x > a.z) << 1) | uint(a.y > a.z);
    
    int3 tb1 = tb1Arr[tbIdx], tb2 = tb2Arr[tbIdx];
    
    float g = 0.1f;
    float3 offA = sin(hash(idx) * 425.551f + m) * g;
    float3 offB = sin(hash(idx + tb1) * 425.551f + m) * g + tb1 - k2;    
    float3 offC = sin(hash(idx + tb2) * 425.551f + m) * g + tb2 - k2 * 2.f;
    float3 offD = sin(hash(idx + 1.f) * 425.551f + m) * g + 1.f - k2 * 3.f;
    
    float dist = min4(sdf3dSphere(a, float4(offA, 0.01f)), sdf3dSphere(a, float4(offB, 0.01f)),
    sdf3dSphere(a, float4(offC, 0.01f)), sdf3dSphere(a, float4(offD, 0.01f)));
    
    return smoothstep(0.f, 1.f, dist);
}


float smin(float a, float b, float k)
{
    float h = max(k - abs(a - b), 0.f) / k;
    return min(a, b) - h * h * h * k * (1.f / 6.f);
}

float segment(float3 _point, float4 A, float4 B)
{
    float3 ab = B.xyz - A.xyz;
    float3 ap = _point - A.xyz;
    float t = saturate(dot(ap, ab) / dot(ab, ab));
    return length(ap - t * ab) - lerp(A.w, B.w, t);
}

float2 Dist3(float3 p)
{
    float time = CurrentTime;
    float rad = 6.f;
    float3 tunnel = p;
    tunnel.z += time * 2.5f + 10.f;
    tunnel.x -= cos(tunnel.z * 0.2f);
    tunnel.y -= sin(tunnel.z * 0.5f);

    float nos = simplexNoise(tunnel * 0.6f + simplexNoise(tunnel)) * 0.3f + 0.3f;
    float2 d = float2(-sdf3dInfCylinder(tunnel, float4(0.f, 0.f, 0.f, rad), float3(0.f, 0.f, 1.f)) - nos, 0.f);
    {
        float3 hand = tunnel;
        float hAta = atan2(hand.y, hand.x) + PI2;
        float2 hHash = hash(vec2(floor((hAta * 12.f) / PI2)));
        hand.z = fmod(hand.z + hHash.y + hHash.x, PI2);
        float hTheta = fmod(hAta, PI2 / 12.f);
        float hLen = length(hand.xy);
        sincos(hTheta, hand.y, hand.x);
        hand.xy *= hLen;

        hand.x += sin(time * 2.5f + hHash.x * 32.125f) * 1.5f;
        hand.y -= 1.f;
        float handDist = segment(hand, float4(rad + 0.5f, 0.65f, PI2, 0.3f), float4(rad - 0.4f, 0.5f, PI2, 0.1f));
        handDist = smin(handDist, sdf3dSphere(hand, float4(rad - 0.6f, 0.5f, PI2, 0.2f)), 0.8f);
        float fingerEnd = PI2 - sin(time * 6.f - hand.y * hand.x * 0.5f + hHash.y * 34.123f) * 0.25f - 0.25f;
        handDist = smin(handDist, segment(hand, float4(rad - 0.9f, 0.5f, PI2, 0.09f), float4(rad - 1.25f, 0.5f, fingerEnd, 0.07f)), 0.2f);
        handDist = smin(handDist, segment(hand, float4(rad - 0.8f, 0.3f, PI2, 0.07f), float4(rad - 1.2f, 0.25f, fingerEnd, 0.07f)), 0.2f);
        handDist = smin(handDist, segment(hand, float4(rad - 0.5f, 0.25f, PI2, 0.09f), float4(rad - 0.85f, 0.1f, fingerEnd, 0.075f)), 0.1f);
        handDist = smin(handDist, segment(hand, float4(rad - 0.8f, 0.65f, PI2, 0.08f), float4(rad - 1.1f, 0.7f, fingerEnd, 0.06f)), 0.1f);
        handDist = smin(handDist, segment(hand, float4(rad - 0.6f, 0.75f, PI2, 0.07f), float4(rad - 0.9f, 0.85f, fingerEnd, 0.045f)), 0.1f);
        d.x = smin(d.x, handDist, 0.8f);
    }
    
    {
        float3 eye = tunnel;
        float eAta = atan2(eye.y, eye.x) + PI2;
        float2 eHash = hash(vec2(floor((eAta * 6.f) / PI2))) * 0.5f + 0.5f;
        eye.z = fmod(eye.z + eHash.y + eHash.x, PI2);
        float eTheta = fmod(eAta, PI2 / 6.f);
        float eLen = length(eye.xy);
        sincos(eTheta, eye.y, eye.x);
        eye.xy *= eLen;
        
        float4 ball = float4(rad - max(1.f - eHash.x, 0.f), 1.5f, PI + eHash.y, eHash.x + 0.3f);
        float4 pupil = float4(rad - max(1.f - eHash.x, 0.f) - 0.37f, 1.5f, PI + eHash.y, eHash.x);
        float3 eyeDir = pupil.xyz - ball.xyz;
        float eyeLen = length(eyeDir);
        float eyeTime = fmod(time, 10.f);
        eyeDir.y -= smoothstep(0.f, 0.2f, eyeTime) * (1.f - smoothstep(5.5f, 5.7f, eyeTime)) * 0.3f;
        eyeDir.z -= smoothstep(3.f, 3.2f, eyeTime) * (1.f - smoothstep(8.5f, 8.7f, eyeTime)) * 0.4f;
        eyeDir = normalize(eyeDir);
        pupil.xyz = ball.xyz + eyeDir * eyeLen;
        
        float eyeDist = sdf3dSphere(eye, ball);
        eyeDist = smin(eyeDist, sdf3dSphere(eye, pupil), 0.01f);
        d.x = smin(d.x, eyeDist, 0.4f);
    }
  
    return d * 0.3f;
}

float3 Dist3Color(float3 p, float3 rayOrigin, float3 rayDir)
{
    float time = CurrentTime;
    float rad = 6.f;
    float3 tunnel = p;
    tunnel.z += time * 2.5f + 10.f;
    tunnel.x -= cos(tunnel.z * 0.2f);
    tunnel.y -= sin(tunnel.z * 0.5f);

    float nos = simplexNoise(tunnel * 0.6f + simplexNoise(tunnel)) * 0.3f + 0.3f;
    float2 d = float2(-sdf3dInfCylinder(tunnel, float4(0.f, 0.f, 0.f, rad), float3(0.f, 0.f, 1.f)) - nos, 0.f);    
    
    float3 hand = tunnel;
    float hAta = atan2(hand.y, hand.x) + PI2;
    float2 hHash = hash(vec2(floor((hAta * 12.f) / PI2)));
    hand.z = fmod(hand.z + hHash.y + hHash.x, PI2);
    float hTheta = fmod(hAta, PI2 / 12.f);
    float hLen = length(hand.xy);
    sincos(hTheta, hand.y, hand.x);
    hand.xy *= hLen;

    hand.x += sin(time * 2.5f + hHash.x * 32.125f) * 1.5f;
    hand.y -= 1.f;
    float handDist = segment(hand, float4(rad + 0.5f, 0.65f, PI2, 0.3f), float4(rad - 0.4f, 0.5f, PI2, 0.1f));
    handDist = smin(handDist, sdf3dSphere(hand, float4(rad - 0.6f, 0.5f, PI2, 0.2f)), 0.8f);
    float fingerEnd = PI2 - sin(time * 6.f - hand.y * hand.x * 0.5f + hHash.y * 34.123f) * 0.25f - 0.25f;
    handDist = smin(handDist, segment(hand, float4(rad - 0.9f, 0.5f, PI2, 0.09f), float4(rad - 1.25f, 0.5f, fingerEnd, 0.07f)), 0.2f);
    handDist = smin(handDist, segment(hand, float4(rad - 0.8f, 0.3f, PI2, 0.07f), float4(rad - 1.2f, 0.25f, fingerEnd, 0.07f)), 0.2f);
    handDist = smin(handDist, segment(hand, float4(rad - 0.5f, 0.25f, PI2, 0.09f), float4(rad - 0.85f, 0.1f, fingerEnd, 0.075f)), 0.1f);
    handDist = smin(handDist, segment(hand, float4(rad - 0.8f, 0.65f, PI2, 0.08f), float4(rad - 1.1f, 0.7f, fingerEnd, 0.06f)), 0.1f);
    handDist = smin(handDist, segment(hand, float4(rad - 0.6f, 0.75f, PI2, 0.07f), float4(rad - 0.9f, 0.85f, fingerEnd, 0.045f)), 0.1f);
    d = minX(d, float2(handDist, 1.f));
    
    
    float3 eye = tunnel;
    float eAta =atan2(eye.y, eye.x) + PI2;
    float2 eHash = hash(vec2(floor((eAta * 6.f) / PI2))) * 0.5f + 0.5f;
    eye.z = fmod(eye.z + eHash.y + eHash.x, PI2);
    float eTheta = fmod(eAta, PI2 / 6.f);
    float eLen = length(eye.xy);
    sincos(eTheta, eye.y, eye.x);
    eye.xy *= eLen;
        
    float4 ball = float4(rad - max(1.f - eHash.x, 0.f), 1.5f, PI + eHash.y, eHash.x + 0.3f);
    float4 pupil = float4(rad - max(1.f - eHash.x, 0.f) - 0.37f, 1.5f, PI + eHash.y, eHash.x);
    float3 eyeDir = pupil.xyz - ball.xyz;
    float eyeLen = length(eyeDir);
    float eyeTime = fmod(time, 10.f);
    eyeDir.y -= smoothstep(0.f, 0.2f, eyeTime) * (1.f - smoothstep(5.5f, 5.7f, eyeTime)) * 0.3f;
    eyeDir.z -= smoothstep(3.f, 3.2f, eyeTime) * (1.f - smoothstep(8.5f, 8.7f, eyeTime)) * 0.4f;
    eyeDir = normalize(eyeDir);
    pupil.xyz = ball.xyz + eyeDir * eyeLen;
        
    d = minX(d, float2(sdf3dSphere(eye, ball), 2.f));
    d = minX(d, float2(sdf3dSphere(eye, pupil), 3.f));
       
    float3 normal;
    GetNormal(p, Dist3, normal);
    float3 toLight = rayOrigin - p;
    float lightLen = length(toLight);
    toLight /= lightLen;
    float3 spotDir = normalize(rayOrigin - float3(cos(time * 0.6f) * 3.f, sin(time * 0.5f) * 2.f, 0.f));
    float spotPow = pow(max(dot(toLight, spotDir), 0.f), 4.f);
    float intensity = spotPow * max(dot(toLight, normal), 0.f) * (1.f - lightLen / 35.f);

    float3 color = vec3(0.f);
    
    if (intensity > 0.f)
    {
        intensity = pow(intensity + 0.4f, 6.f);
        float3 halfVec = (normalize(rayOrigin) + toLight) * 0.5f;
        float roughReflect = max(dot(normal, halfVec), 0.f) * spotPow;
        float fresnel = max(dot(-rayDir, normal), 0.f);
        
        if (d.y == 0.f)
        {
            color = float3(1.f, nos, 0.3f)*2.f;
            color += roughReflect * float3(1.f, 0.8f, 0.8f);
            color += roughReflect > 0.9f ? 1.f : 0.f;
            color += pow(1.f - fresnel + 0.3f, 16.f) * (1.f - float3(1.f, 0.8f, 0.8f));
        }
        else if (d.y == 1.f)
        {
            color = lerp(float3(0.98431f, 0.80784f, 0.69412f) * pow(1.f - simplexVoronoi(hand * float3(5.f, 3.f, 1.f), 0.f), 4.f),
                float3(1.f, nos, 0.3f), pow(hand.x * 0.15f, 4.f));
        }
        else if (d.y == 2.f)
        {
            float x = pow(eye.x * 0.18f, 8.f);
            color = lerp(vec3(2.f), float3(2.f, nos , 0.2f), x);
            color.r += simplexVoronoi(eye * float3(8.f, 8.f, 1.f), 0.f) > 0.55f ? 1.f : 0.f;
            color += pow(1.f - fresnel + 0.35f, 16.f) * float3(1.f, nos + 0.3f, 0.6f);
            color += roughReflect;
        }
        else if (d.y == 3.f)
        {
            float pd = max(dot(eyeDir, normalize(eye - pupil.xyz)), 0.f);
            float pupilIntensity = 1.f - pow(pd, 16.f);
            color = float3(nos, 0.2f, 1.5f) * pupilIntensity * (simplexVoronoi(eye * nos*6.f, 0.f) + 0.2f) * 2.f;
            color += roughReflect * pupilIntensity * float3(nos+0.3f, 0.2f, 1.f);
            color += roughReflect > 0.965f ? roughReflect : 0.f;
            color.r += saturate(smoothstep(0.f, 0.4f, pupilIntensity) - pupilIntensity);
            color *= pow(pd, 4.f);
            color *= 2.f;
        }
        float shd = 1.f;
        float distSum = 0.05f;
        float3 ro = p + 0.15f * normal;
        for (int i = 0; i < 10; i++)
        {
            float curr = Dist3(ro + toLight * distSum).x;
            shd = min(6.f * curr / distSum, shd);
            distSum += curr;
        }
        intensity *= max(shd, 0.4f);
    }
       
    return color * intensity;
}

float2 spotShaftDist1(float3 p)
{
    float time = CurrentTime;

    float3 B = float3(cos(time * 0.6f) * 3.f, sin(time * 0.5f) * 2.f, 0.f);
    float3 A = float3(0.f, 0.f, -9.f); 
    
    time = time * 2.5f + 10.f;
    A.x -= cos(time * 0.2f);
    A.y -= sin(time * 0.5f);
    A.y -= 1.f;

    float3 ab = B.xyz - A.xyz;
    float3 ap = p - A.xyz;
    float t = saturate(dot(ap, ab) / dot(ab, ab));
    
    return float2(length(ap - t * ab) + 0.06f, sqrt(1.f - t) * pow(max(dot(normalize(ab), normalize(ap)), 0.f), 32.f));
}

void spotShaft(inout float4 sum, float2 d)
{
    float4 col = vec4(0.f);
    col = lerp(vec4(0.f), vec4(2.f), d.y);
       
    col.rgb *= col.a;
    sum = sum + (1.f - sum.a) * col;
}

float4 Unknown(vec2 uv)
{
    float4 color = float4(0.f, 0.f, 0.f, 0.f);
 
    float3 rayTarget = float3(0.f, 0.f, 0.f);
    float3 rayOrigin = float3(0.f, 0.f, -9.f);
    float time = CurrentTime;
    time = time * 2.5f + 10.f;
    rayOrigin.x -= cos(time * 0.2f);
    rayOrigin.y -= sin(time * 0.5f);
    float3 rayDir = normalize(float3(uv, 1.f));
    rayDir = mul(rayDir, viewMatrix(normalize(rayTarget - rayOrigin)));
    
    float2 march;
    RayMarching(rayOrigin, rayDir, Dist3, 60, march, 0);
    float3 p = rayOrigin + rayDir * march.x;

    color.rgb = Dist3Color(p, rayOrigin, rayDir);
        
    float4 shaft = vec4(0.f);
    RayMarching(rayOrigin, rayDir, spotShaftDist1, 15, float2 d, spotShaft(shaft, curr));
    color.rgb += shaft.rgb * float3(1.f, 0.75f, 0.85f) * 0.5f;
    
    color.a = 1.f;
    return color;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 TexC = 2.f*(fragCoord.xy/iResolution.xy)-1.f;
    TexC.x *= iResolution.x/iResolution.y;

    fragColor = Unknown(TexC);
}