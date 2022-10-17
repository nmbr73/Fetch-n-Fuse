
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



#define _saturatef(x) clamp(x,0.0f,1.0f)
#define lerp _mix
#define CurrentTime (iTime)
#define sincos(x,s,c) s = _sinf(x), c = _cosf(x)

#define PId2   1.57079632f
#define PI     3.141592653f
#define PI2    6.2831853f
#define MaxDist       100.0f
#define SurfaceDist   0.0001f

#define RayMarching(origin, dir, distFunc, iter, res, more) {\
    float hitDist = 0.0f;\
    float mat = 0.0f;\
    for (int i = 0;i < iter; ++i){\
        float3 ray = origin + hitDist * dir;\
        float2 curr = distFunc(ray,iTime);\
        if (hitDist > MaxDist || curr.x < SurfaceDist)\
            break;\
        {more;}\
        mat = curr.y;\
        hitDist += curr.x;}\
        res = to_float2(hitDist, mat);}

#define GetNormal(ray, distFunc, res) {\
    float2 k = to_float2(1.0f, -1.0f);\
    res = normalize(swi3(k,x,y,y) * distFunc(ray + swi3(k,x,y,y) * SurfaceDist,iTime).x +\
                    swi3(k,y,y,x) * distFunc(ray + swi3(k,y,y,x) * SurfaceDist,iTime).x +\
                    swi3(k,y,x,y) * distFunc(ray + swi3(k,y,x,y) * SurfaceDist,iTime).x +\
                    swi3(k,x,x,x) * distFunc(ray + swi3(k,x,x,x) * SurfaceDist,iTime).x);}

__DEVICE__ mat3 viewMatrix(float3 look)
{
    float3 right = normalize(cross(to_float3(0.0f, 1.0f, 0.0f), look));
    float3 up = cross(look, right);
    return to_mat3_f3(right, up, look);
}

__DEVICE__ float2 minX(float2 a, float2 b)
{
    return a.x < b.x ? a : b;
}

__DEVICE__ float min3(float a, float b, float c)
{
    return _fminf(a, _fminf(b, c));
}

__DEVICE__ float min4(float a, float b, float c, float d)
{
    return _fminf(min3(a, b, c), d);
}

__DEVICE__ float2 hash(float2 p)
{
    p = to_float2(dot(p, to_float2(127.1f, 311.7f)), dot(p, to_float2(269.5f, 183.3f)));
    return 2.0f * fract_f2(sin_f2(p) * 43758.5453123f) - 1.0f;
}

__DEVICE__ float3 hash(float3 p)
{
    p = to_float3(dot(p, to_float3(127.1f, 311.7f, 74.7f)), dot(p, to_float3(269.5f, 183.3f, 246.1f)), dot(p, to_float3(113.5f, 271.9f, 124.6f)));
    return 2.0f * fract_f3(sin_f3(p) * 43758.5453123f) - 1.0f;
}

__DEVICE__ float sdf3dSphere(float3 _point, float4 sphere)
{
    return length(_point - swi3(sphere,x,y,z)) - sphere.w;
}

__DEVICE__ float sdf3dInfPlane(float3 _point, float3 plane, float3 planeNormal)
{
    return dot((_point - plane), planeNormal);
}

__DEVICE__ float sdf3dInfCylinder(float3 _point, float4 cylinder, float3 cylinderDirection)
{
    _point -= swi3(cylinder,x,y,z);
    return length(_point - dot(_point, cylinderDirection) * cylinderDirection) - cylinder.w;
}

__DEVICE__ float simplexNoise(float3 p)
{
    float k1 = 0.333333f;
    float k2 = 0.166667f;
    
    float3 idx = _floor(p + (p.x + p.y + p.z) * k1);
    
    float3 a = p - ((idx) - (float)(idx.x + idx.y + idx.z) * k2);
    
    float3 tb1Arr[8] = {  to_float3(0, 0, 1), to_float3(0, 1, 0), to_float3( 0, 0, 0), to_float3(0, 1, 0), to_float3(0, 0, 1), to_float3( 0, 0, 0), to_float3(1, 0, 0), to_float3(1, 0, 0) };
    float3 tb2Arr[8] = {  to_float3(0, 1, 1), to_float3(0, 1, 1), to_float3( 0, 0, 0), to_float3(1, 1, 0), to_float3(1, 0, 1), to_float3( 0, 0, 0), to_float3(1, 0, 1), to_float3(1, 1, 0) };
    
    uint tbIdx = ((uint)(a.x > a.y) << 2) | ((uint)(a.x > a.z) << 1) | (uint)(a.y > a.z);
    
    float3 tb1 = tb1Arr[tbIdx], tb2 = tb2Arr[tbIdx];
    
    float3 b = a - tb1 + k2;
    float3 c = a - tb2 + k2 * 2.0f;
    float3 d = a - 1.0f + k2 * 3.0f;
    
    float4 kernel = _fmaxf(to_float4_s(0.5f) - to_float4(dot(a, a), dot(b, b), dot(c, c), dot(d, d)), to_float4_s(0.0f));
    kernel *= kernel;
    kernel *= kernel;
    float4 noise = kernel * to_float4(dot(a, hash(idx)), dot(b, hash(idx + tb1)), dot(c, hash(idx + tb2)), dot(d, hash(idx + 1.0f)));
    
    return dot(to_float4_s(52.0f), noise);
}

__DEVICE__ float simplexVoronoi(float3 p, float m)
{
    float k1 = 0.333333f;
    float k2 = 0.166667f;
    
    float3 idx = _floor(p + (p.x + p.y + p.z) * k1);
    
    float3 a = p - ((idx) - (float)(idx.x + idx.y + idx.z) * k2);
    
    float3 tb1Arr[8] = {  to_float3(0, 0, 1), to_float3(0, 1, 0), to_float3( 0, 0, 0), to_float3(0, 1, 0), to_float3(0, 0, 1), to_float3_s( 0), to_float3(1, 0, 0), to_float3(1, 0, 0) };
    float3 tb2Arr[8] = {  to_float3(0, 1, 1), to_float3(0, 1, 1), to_float3( 0, 0, 0), to_float3(1, 1, 0), to_float3(1, 0, 1), to_float3_s( 0), to_float3(1, 0, 1), to_float3(1, 1, 0) };
    
    uint tbIdx = ((uint)(a.x > a.y) << 2) | ((uint)(a.x > a.z) << 1) | (uint)(a.y > a.z);
    
    float3 tb1 = tb1Arr[tbIdx], tb2 = tb2Arr[tbIdx];
    
    float g = 0.1f;
    float3 offA = sin_f3(hash(idx) * 425.551f + m) * g;
    float3 offB = sin_f3(hash(idx + tb1) * 425.551f + m) * g + tb1 - k2;    
    float3 offC = sin_f3(hash(idx + tb2) * 425.551f + m) * g + tb2 - k2 * 2.0f;
    float3 offD = sin_f3(hash(idx + 1.0f) * 425.551f + m) * g + 1.0f - k2 * 3.0f;
    
    float dist = min4(sdf3dSphere(a, to_float4_aw(offA, 0.01f)), sdf3dSphere(a, to_float4_aw(offB, 0.01f)),
    sdf3dSphere(a, to_float4_aw(offC, 0.01f)), sdf3dSphere(a, to_float4_aw(offD, 0.01f)));
    
    return smoothstep(0.0f, 1.0f, dist);
}


__DEVICE__ float smin(float a, float b, float k)
{
    float h = _fmaxf(k - _fabs(a - b), 0.0f) / k;
    return _fminf(a, b) - h * h * h * k * (1.0f / 6.0f);
}

__DEVICE__ float segment(float3 _point, float4 A, float4 B)
{
    float3 ab = swi3(B,x,y,z) - swi3(A,x,y,z);
    float3 ap = _point - swi3(A,x,y,z);
    float t = _saturatef(dot(ap, ab) / dot(ab, ab));
    return length(ap - t * ab) - lerp(A.w, B.w, t);
}

__DEVICE__ float2 Dist3(float3 p, float iTime)
{
    float time = CurrentTime;
    float rad = 6.0f;
    float3 tunnel = p;
    tunnel.z += time * 2.5f + 10.0f;
    tunnel.x -= _cosf(tunnel.z * 0.2f);
    tunnel.y -= _sinf(tunnel.z * 0.5f);

    float nos = simplexNoise(tunnel * 0.6f + simplexNoise(tunnel)) * 0.3f + 0.3f;
    float2 d = to_float2(-sdf3dInfCylinder(tunnel, to_float4(0.0f, 0.0f, 0.0f, rad), to_float3(0.0f, 0.0f, 1.0f)) - nos, 0.0f);
    {
        float3 hand = tunnel;
        float hAta = _atan2f(hand.y, hand.x) + PI2;
        float2 hHash = hash(to_float2_s(_floor((hAta * 12.0f) / PI2)));
        hand.z = _fmod(hand.z + hHash.y + hHash.x, PI2);
        float hTheta = _fmod(hAta, PI2 / 12.0f);
        float hLen = length(swi2(hand,x,y));
        sincos(hTheta, hand.y, hand.x);
        //swi2(hand,x,y) *= hLen;
        hand.x *= hLen;
        hand.y *= hLen;

        hand.x += _sinf(time * 2.5f + hHash.x * 32.125f) * 1.5f;
        hand.y -= 1.0f;
        float handDist = segment(hand, to_float4(rad + 0.5f, 0.65f, PI2, 0.3f), to_float4(rad - 0.4f, 0.5f, PI2, 0.1f));
        handDist = smin(handDist, sdf3dSphere(hand, to_float4(rad - 0.6f, 0.5f, PI2, 0.2f)), 0.8f);
        float fingerEnd = PI2 - _sinf(time * 6.0f - hand.y * hand.x * 0.5f + hHash.y * 34.123f) * 0.25f - 0.25f;
        handDist = smin(handDist, segment(hand, to_float4(rad - 0.9f, 0.5f, PI2, 0.09f), to_float4(rad - 1.25f, 0.5f, fingerEnd, 0.07f)), 0.2f);
        handDist = smin(handDist, segment(hand, to_float4(rad - 0.8f, 0.3f, PI2, 0.07f), to_float4(rad - 1.2f, 0.25f, fingerEnd, 0.07f)), 0.2f);
        handDist = smin(handDist, segment(hand, to_float4(rad - 0.5f, 0.25f, PI2, 0.09f), to_float4(rad - 0.85f, 0.1f, fingerEnd, 0.075f)), 0.1f);
        handDist = smin(handDist, segment(hand, to_float4(rad - 0.8f, 0.65f, PI2, 0.08f), to_float4(rad - 1.1f, 0.7f, fingerEnd, 0.06f)), 0.1f);
        handDist = smin(handDist, segment(hand, to_float4(rad - 0.6f, 0.75f, PI2, 0.07f), to_float4(rad - 0.9f, 0.85f, fingerEnd, 0.045f)), 0.1f);
        d.x = smin(d.x, handDist, 0.8f);
    }
    
    {
        float3 eye = tunnel;
        float eAta = _atan2f(eye.y, eye.x) + PI2;
        float2 eHash = hash(to_float2_s(_floor((eAta * 6.0f) / PI2))) * 0.5f + 0.5f;
        eye.z = _fmod(eye.z + eHash.y + eHash.x, PI2);
        float eTheta = _fmod(eAta, PI2 / 6.0f);
        float eLen = length(swi2(eye,x,y));
        sincos(eTheta, eye.y, eye.x);
        //swi2(eye,x,y) *= eLen;
        eye.x *= eLen;
        eye.y *= eLen;
        
        float4 ball = to_float4(rad - _fmaxf(1.0f - eHash.x, 0.0f), 1.5f, PI + eHash.y, eHash.x + 0.3f);
        float4 pupil = to_float4(rad - _fmaxf(1.0f - eHash.x, 0.0f) - 0.37f, 1.5f, PI + eHash.y, eHash.x);
        float3 eyeDir = swi3(pupil,x,y,z) - swi3(ball,x,y,z);
        float eyeLen = length(eyeDir);
        float eyeTime = _fmod(time, 10.0f);
        eyeDir.y -= smoothstep(0.0f, 0.2f, eyeTime) * (1.0f - smoothstep(5.5f, 5.7f, eyeTime)) * 0.3f;
        eyeDir.z -= smoothstep(3.0f, 3.2f, eyeTime) * (1.0f - smoothstep(8.5f, 8.7f, eyeTime)) * 0.4f;
        eyeDir = normalize(eyeDir);
        swi3S(pupil,x,y,z, swi3(ball,x,y,z) + eyeDir * eyeLen);
      
        float eyeDist = sdf3dSphere(eye, ball);
        eyeDist = smin(eyeDist, sdf3dSphere(eye, pupil), 0.01f);
        d.x = smin(d.x, eyeDist, 0.4f);
    }
  
    return d * 0.3f;
}

__DEVICE__ float3 Dist3Color(float3 p, float3 rayOrigin, float3 rayDir, float iTime)
{
    float time = CurrentTime;
    float rad = 6.0f;
    float3 tunnel = p;
    tunnel.z += time * 2.5f + 10.0f;
    tunnel.x -= _cosf(tunnel.z * 0.2f);
    tunnel.y -= _sinf(tunnel.z * 0.5f);

    float nos = simplexNoise(tunnel * 0.6f + simplexNoise(tunnel)) * 0.3f + 0.3f;
    float2 d = to_float2(-sdf3dInfCylinder(tunnel, to_float4(0.0f, 0.0f, 0.0f, rad), to_float3(0.0f, 0.0f, 1.0f)) - nos, 0.0f);    
    
    float3 hand = tunnel;
    float hAta = _atan2f(hand.y, hand.x) + PI2;
    float2 hHash = hash(to_float2_s(_floor((hAta * 12.0f) / PI2)));
    hand.z = _fmod(hand.z + hHash.y + hHash.x, PI2);
    float hTheta = _fmod(hAta, PI2 / 12.0f);
    float hLen = length(swi2(hand,x,y));
    sincos(hTheta, hand.y, hand.x);
    //swi2(hand,x,y) *= hLen;
    hand.x *= hLen;
    hand.y *= hLen;

    hand.x += _sinf(time * 2.5f + hHash.x * 32.125f) * 1.5f;
    hand.y -= 1.0f;
    float handDist = segment(hand, to_float4(rad + 0.5f, 0.65f, PI2, 0.3f), to_float4(rad - 0.4f, 0.5f, PI2, 0.1f));
    handDist = smin(handDist, sdf3dSphere(hand, to_float4(rad - 0.6f, 0.5f, PI2, 0.2f)), 0.8f);
    float fingerEnd = PI2 - _sinf(time * 6.0f - hand.y * hand.x * 0.5f + hHash.y * 34.123f) * 0.25f - 0.25f;
    handDist = smin(handDist, segment(hand, to_float4(rad - 0.9f, 0.5f, PI2, 0.09f), to_float4(rad - 1.25f, 0.5f, fingerEnd, 0.07f)), 0.2f);
    handDist = smin(handDist, segment(hand, to_float4(rad - 0.8f, 0.3f, PI2, 0.07f), to_float4(rad - 1.2f, 0.25f, fingerEnd, 0.07f)), 0.2f);
    handDist = smin(handDist, segment(hand, to_float4(rad - 0.5f, 0.25f, PI2, 0.09f), to_float4(rad - 0.85f, 0.1f, fingerEnd, 0.075f)), 0.1f);
    handDist = smin(handDist, segment(hand, to_float4(rad - 0.8f, 0.65f, PI2, 0.08f), to_float4(rad - 1.1f, 0.7f, fingerEnd, 0.06f)), 0.1f);
    handDist = smin(handDist, segment(hand, to_float4(rad - 0.6f, 0.75f, PI2, 0.07f), to_float4(rad - 0.9f, 0.85f, fingerEnd, 0.045f)), 0.1f);
    d = minX(d, to_float2(handDist, 1.0f));
    
    
    float3 eye = tunnel;
    float eAta =_atan2f(eye.y, eye.x) + PI2;
    float2 eHash = hash(to_float2_s(_floor((eAta * 6.0f) / PI2))) * 0.5f + 0.5f;
    eye.z = _fmod(eye.z + eHash.y + eHash.x, PI2);
    float eTheta = _fmod(eAta, PI2 / 6.0f);
    float eLen = length(swi2(eye,x,y));
    sincos(eTheta, eye.y, eye.x);
    //swi2(eye,x,y) *= eLen;
    eye.x *= eLen;
    eye.y *= eLen;
        
    float4 ball = to_float4(rad - _fmaxf(1.0f - eHash.x, 0.0f), 1.5f, PI + eHash.y, eHash.x + 0.3f);
    float4 pupil = to_float4(rad - _fmaxf(1.0f - eHash.x, 0.0f) - 0.37f, 1.5f, PI + eHash.y, eHash.x);
    float3 eyeDir = swi3(pupil,x,y,z) - swi3(ball,x,y,z);
    float eyeLen = length(eyeDir);
    float eyeTime = _fmod(time, 10.0f);
    eyeDir.y -= smoothstep(0.0f, 0.2f, eyeTime) * (1.0f - smoothstep(5.5f, 5.7f, eyeTime)) * 0.3f;
    eyeDir.z -= smoothstep(3.0f, 3.2f, eyeTime) * (1.0f - smoothstep(8.5f, 8.7f, eyeTime)) * 0.4f;
    eyeDir = normalize(eyeDir);
    swi3S(pupil,x,y,z, swi3(ball,x,y,z) + eyeDir * eyeLen);
        
    d = minX(d, to_float2(sdf3dSphere(eye, ball), 2.0f));
    d = minX(d, to_float2(sdf3dSphere(eye, pupil), 3.0f));
       
    float3 normal;
    GetNormal(p, Dist3, normal);
    float3 toLight = rayOrigin - p;
    float lightLen = length(toLight);
    toLight /= lightLen;
    float3 spotDir = normalize(rayOrigin - to_float3(_cosf(time * 0.6f) * 3.0f, _sinf(time * 0.5f) * 2.0f, 0.0f));
    float spotPow = _powf(_fmaxf(dot(toLight, spotDir), 0.0f), 4.0f);
    float intensity = spotPow * _fmaxf(dot(toLight, normal), 0.0f) * (1.0f - lightLen / 35.0f);

    float3 color = to_float3_s(0.0f);
    
    if (intensity > 0.0f)
    {
        intensity = _powf(intensity + 0.4f, 6.0f);
        float3 halfVec = (normalize(rayOrigin) + toLight) * 0.5f;
        float roughReflect = _fmaxf(dot(normal, halfVec), 0.0f) * spotPow;
        float fresnel = _fmaxf(dot(-rayDir, normal), 0.0f);
        
        if (d.y == 0.0f)
        {
            color = to_float3(1.0f, nos, 0.3f)*2.0f;
            color += roughReflect * to_float3(1.0f, 0.8f, 0.8f);
            color += roughReflect > 0.9f ? 1.0f : 0.0f;
            color += _powf(1.0f - fresnel + 0.3f, 16.0f) * (1.0f - to_float3(1.0f, 0.8f, 0.8f));
        }
        else if (d.y == 1.0f)
        {
            color = lerp(to_float3(0.98431f, 0.80784f, 0.69412f) * _powf(1.0f - simplexVoronoi(hand * to_float3(5.0f, 3.0f, 1.0f), 0.0f), 4.0f),
                         to_float3(1.0f, nos, 0.3f), _powf(hand.x * 0.15f, 4.0f));
        }
        else if (d.y == 2.0f)
        {
            float x = _powf(eye.x * 0.18f, 8.0f);
            color = lerp(to_float3_s(2.0f), to_float3(2.0f, nos , 0.2f), x);
            color.x += simplexVoronoi(eye * to_float3(8.0f, 8.0f, 1.0f), 0.0f) > 0.55f ? 1.0f : 0.0f;
            color += _powf(1.0f - fresnel + 0.35f, 16.0f) * to_float3(1.0f, nos + 0.3f, 0.6f);
            color += roughReflect;
        }
        else if (d.y == 3.0f)
        {
            float pd = _fmaxf(dot(eyeDir, normalize(eye - swi3(pupil,x,y,z))), 0.0f);
            float pupilIntensity = 1.0f - _powf(pd, 16.0f);
            color = to_float3(nos, 0.2f, 1.5f) * pupilIntensity * (simplexVoronoi(eye * nos*6.0f, 0.0f) + 0.2f) * 2.0f;
            color += roughReflect * pupilIntensity * to_float3(nos+0.3f, 0.2f, 1.0f);
            color += roughReflect > 0.965f ? roughReflect : 0.0f;
            color.x += _saturatef(smoothstep(0.0f, 0.4f, pupilIntensity) - pupilIntensity);
            color *= _powf(pd, 4.0f);
            color *= 2.0f;
        }
        float shd = 1.0f;
        float distSum = 0.05f;
        float3 ro = p + 0.15f * normal;
        for (int i = 0; i < 10; i++)
        {
            float curr = Dist3(ro + toLight * distSum, iTime).x;
            shd = _fminf(6.0f * curr / distSum, shd);
            distSum += curr;
        }
        intensity *= _fmaxf(shd, 0.4f);
    }
       
    return color * intensity;
}

__DEVICE__ float2 spotShaftDist1(float3 p, float iTime)
{
    float time = CurrentTime;

    float3 B = to_float3(_cosf(time * 0.6f) * 3.0f, _sinf(time * 0.5f) * 2.0f, 0.0f);
    float3 A = to_float3(0.0f, 0.0f, -9.0f); 
    
    time = time * 2.5f + 10.0f;
    A.x -= _cosf(time * 0.2f);
    A.y -= _sinf(time * 0.5f);
    A.y -= 1.0f;

    float3 ab = swi3(B,x,y,z) - swi3(A,x,y,z);
    float3 ap = p - swi3(A,x,y,z);
    float t = _saturatef(dot(ap, ab) / dot(ab, ab));
    
    return to_float2(length(ap - t * ab) + 0.06f, _sqrtf(1.0f - t) * _powf(_fmaxf(dot(normalize(ab), normalize(ap)), 0.0f), 32.0f));
}

__DEVICE__ void spotShaft(inout float4 *sum, float2 d)
{
    float4 col = to_float4_s(0.0f);
    col = lerp(to_float4_s(0.0f), to_float4_s(2.0f), d.y);
       
    //swi3(col,x,y,z) *= col.w;
    col.x *= col.w;
    col.y *= col.w;
    col.z *= col.w;
    
    *sum = *sum + (1.0f - (*sum).w) * col;
}

__DEVICE__ float4 Unknown(float2 uv, float iTime)
{
    float4 color = to_float4(0.0f, 0.0f, 0.0f, 0.0f);
 
    float3 rayTarget = to_float3(0.0f, 0.0f, 0.0f);
    float3 rayOrigin = to_float3(0.0f, 0.0f, -9.0f);
    float time = CurrentTime;
    time = time * 2.5f + 10.0f;
    rayOrigin.x -= _cosf(time * 0.2f);
    rayOrigin.y -= _sinf(time * 0.5f);
    float3 rayDir = normalize(to_float3_aw(uv, 1.0f));
    rayDir = mul_f3_mat3(rayDir, viewMatrix(normalize(rayTarget - rayOrigin)));
    
    
    float2 march;
    RayMarching(rayOrigin, rayDir, Dist3, 60, march, 0);
    float3 p = rayOrigin + rayDir * march.x;

    color = to_float4_aw(Dist3Color(p, rayOrigin, rayDir, iTime), color.w);
        
    float4 shaft = to_float4_s(0.0f);
    float2 d;
    RayMarching(rayOrigin, rayDir, spotShaftDist1, 15, d, spotShaft(&shaft, curr));
    swi3S(color,x,y,z, swi3(color,x,y,z) + swi3(shaft,x,y,z) * to_float3(1.0f, 0.75f, 0.85f) * 0.5f);
    
    color.w = 1.0f;
    return color;
}


__KERNEL__ void Unknown1Jipi528Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 TexC = 2.0f*(fragCoord/iResolution)-1.0f;
    TexC.x *= iResolution.x/iResolution.y;

    fragColor = Unknown(TexC, iTime);

  SetFragmentShaderComputedColor(fragColor);
}