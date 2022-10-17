
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



#define PI (3.14159265359f)
#define HALF_PI (PI * 0.5f)
#define DOUBLE_PI (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)

struct Camera
{
    float3 position;
    float3 forward;
    float3 up;
};

__DEVICE__ mat3 LookAt(float3 forward, float3 up)
{
    mat3 m;
    m.r2 = forward;
    m.r0 = normalize(cross(up, m.r2));
    m.r1 = cross(m.r2, m.r0);
    return m;
}

__DEVICE__ float3 MakeViewRay(float2 uv, float aspect, float fov)
{
    float3 ray = to_float3_aw(uv * 2.0f - 1.0f, 1.0f);
    ray.x *= aspect;
    swi2S(ray,x,y, swi2(ray,x,y) * _tanf(fov * 0.5f * DEG2RAD));
    return normalize(ray);
}

__DEVICE__ float3 MakeWorldRay(float2 uv, float aspect, float fov, Camera cam)
{
    float3 ray = MakeViewRay(uv, aspect, fov);
    return mul_mat3_f3(LookAt(cam.forward, cam.up) , ray);
}

__DEVICE__ float3 MouseRotation(float2 screenPos, float2 screenSize)
{
    float2 muv = screenPos / screenSize;
    muv = muv * 2.0f - 1.0f;
    muv.x *= screenSize.x / screenSize.y;
    muv *= to_float2(PI, HALF_PI);
    muv.y = clamp(muv.y, -0.99f * HALF_PI, 0.99f * HALF_PI);
    float4 rot = to_float4(_cosf(muv.x), _sinf(muv.x), _cosf(muv.y), _sinf(muv.y));
    return normalize(to_float3(rot.y * rot.z, rot.w, rot.x * rot.z));
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0
// Connect Image 'Cubemap: Forest_0' to iChannel1



#define FOV (60.0f)
#define LIGHT_DIR to_float3(-1.0f, 1.0f, -1.0f)
#define STEPS (64)
#define GRID_SIZE (2.0f)
#define RAY_LEN (float(STEPS) * GRID_SIZE * 0.25f)

__DEVICE__ float pow2(float x) {return x * x;}
__DEVICE__ float pow4(float x) {return pow2(x) * pow2(x);}
__DEVICE__ float pow5(float x) {return pow4(x) * x;}

__DEVICE__ Camera GetCamera(float2 iResolution, float4 iMouse, float iTime)
{
    float3 forward;
    if (iMouse.z > 0.5f)
    {
        forward = MouseRotation(swi2(iMouse,x,y), iResolution);
    }
    else
    {
        const float speed = 0.125f;
        forward.y = _sinf(0.618f * speed * iTime) * 0.35f;
        swi2S(forward,x,z, to_float2(-_sinf(iTime * speed), _cosf(iTime * speed)));
        forward = normalize(forward);
    }
    Camera cam;
    cam.forward = forward;
    cam.up = to_float3(0.0f, 1.0f, 0.0f);
    cam.position = -(GRID_SIZE * 2.0f) * cam.forward;
    return cam;
}

__DEVICE__ float3 GetFogColor(float3 rd)
{
    return _mix(to_float3_s(0.0f), to_float3_s(1.0f), rd.y * 0.5f + 0.5f);
}

__DEVICE__ float GetFogIntensity(float t, float start, float decay)
{
    return 1.0f - _powf(1.0f - pow2(_fmaxf((t - start) / (1.0f - start), 0.0f)), decay);
}

__DEVICE__ float4 SampleVolleyball(float3 dirVec, float iTime, __TEXTURE2D__ iChannel0)
{
  
    float2 uv0 = swi2(dirVec,x,y);
    float2 uv1 = swi2(dirVec,y,z);
    float2 uv2 = swi2(dirVec,z,x);
    uv0.x = (uv0.x * 0.3f + 0.5f) * sign_f(dirVec.z) + iTime * -0.125f;
    uv1.x = (uv1.x * 0.3f + 0.5f) * sign_f(dirVec.x) + iTime * -0.125f;
    uv2.x = (uv2.x * 0.3f + 0.5f) * sign_f(dirVec.y) + iTime * -0.125f;
    uv0.y = (uv0.y * 0.8f + 0.5f);
    uv1.y = (uv1.y * 0.8f + 0.5f);
    uv2.y = (uv2.y * 0.8f + 0.5f);
    float4 col0 = _tex2DVecN(iChannel0,uv0.x,uv0.y,15);
    float4 col1 = _tex2DVecN(iChannel0,uv1.x,uv1.y,15);
    float4 col2 = _tex2DVecN(iChannel0,uv2.x,uv2.y,15);
    swi3S(col0,x,y,z, pow_f3(swi3(col0,x,y,z), to_float3_s(2.2f)));
    swi3S(col1,x,y,z, pow_f3(swi3(col1,x,y,z), to_float3_s(2.2f)));
    swi3S(col2,x,y,z, pow_f3(swi3(col2,x,y,z), to_float3_s(2.2f)));
    //col0 *= pow2(dirVec.z);
    //col1 *= pow2(dirVec.x);
    //col2 *= pow2(dirVec.y);
    col0 *= smoothstep(0.577350f, 0.6f, _fabs(dirVec.z)) * smoothstep(0.577350f, 0.55f, _fabs(dirVec.y));
    col1 *= smoothstep(0.577350f, 0.6f, _fabs(dirVec.x)) * smoothstep(0.577350f, 0.55f, _fabs(dirVec.z));
    col2 *= smoothstep(0.577350f, 0.6f, _fabs(dirVec.y)) * smoothstep(0.577350f, 0.55f, _fabs(dirVec.x));
    return col0 + col1 + col2;
}

__DEVICE__ void SampleMaterial(float3 N, float3 C, out float3 *D, out float3 *S, out float *R, float iTime, __TEXTURE2D__ iChannel0)
{
    float4 mainTex = SampleVolleyball(N, iTime, iChannel0);
    float3 albedo = swi3(mainTex,x,y,z) * C;
    float metallic = smoothstep(0.0f, 1.0f, 1.0f - mainTex.z);
    *D = albedo * (1.0f - metallic);
    *S = _mix(to_float3_s(0.04f), albedo, metallic);
    *R = 1.0f - mainTex.x;
    *R = _fmaxf(*R, 0.04f);
}

__DEVICE__ float Geometry(float NoV, float NoL, float k)
{
    float gv = 0.5f / (NoV * (1.0f - k) + k);
    float gl = 0.5f / (NoL * (1.0f - k) + k);
    return gv * gl;
}

__DEVICE__ float Distribution(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NoH * NoH * (a2 - 1.0f) + 1.0f;
    return a2 / (denom * denom);
}

__DEVICE__ float3 Fresnel(float NoV, float3 F0)
{
    return F0 + (1.0f - F0) * pow5(1.0f - NoV);
}

__DEVICE__ float3 DirectSpecular(float NoV, float NoL, float NoH, float HoV, float3 F0, float R)
{
    float k = pow2(R + 1.0f) / 8.0f;
    float G = Geometry(NoV, NoL, k);
    float D = Distribution(NoH, R * R);
    float3 F = Fresnel(HoV, F0);
    return G * D * F;
}

__DEVICE__ float3 DirectRadiance(float3 N, float3 V, float3 L, float3 D, float3 S, float R)
{
    float3 H = normalize(V + L);
    float NoV = _fmaxf(dot(N, V), 0.0f);
    float NoL = _fmaxf(dot(N, L), 0.0f);
    float NoH = _fmaxf(dot(N, H), 0.0f);
    float HoV = _fmaxf(dot(H, V), 0.0f);
    float3 radiance = D;
    radiance += DirectSpecular(NoV, NoL, NoH, HoV, S, R);
    radiance *= NoL;
    return radiance;
}

__DEVICE__ float3 IndirectRadiance(float3 N, float3 V, float3 D, float3 S, float R, __TEXTURE2D__ iChannel1)
{
    float3 reflVec = reflect(-V, N);
    float NoV = _fmaxf(dot(N, V), 0.0f);
    float3 F = Fresnel(NoV, S);
    //float lod = _sqrtf(R);
    //lod = 1.0f - (1.0f - lod) * _sqrtf(NoV);
    float3 radiance = to_float3_s(0.0f);
    radiance += F * pow_f3(swi3(decube_f3(iChannel1, reflVec),x,y,z), to_float3_s(2.2f));
    radiance += D * pow_f3(swi3(decube_f3(iChannel1, N),x,y,z), to_float3_s(2.2f));
    return radiance;
}

__DEVICE__ float RayCastSphere(float3 ro, float3 rd, float4 sphere, out float3 *N)
{
    float3 p = swi3(sphere,x,y,z) - ro;
    float a = dot(rd, rd);
    float b = dot(rd, p);
    float c = dot(p, p) - pow2(sphere.w);
    float delta = b * b - a * c;
    if (delta < 0.0f) return -1.0f;
    float t = (b - _sqrtf(delta)) / a;
    float3 P = ro + rd * t;
    *N = normalize(P - swi3(sphere,x,y,z));
    return t;
}




// wave distort
__DEVICE__ float genWave(float len, float iTime)
{
	float wave = _sinf(7.0f * PI * len - iTime*5.0);
	float wavePow = 1.0 - _powf(_fabs(wave*1.1), 0.8);
	wave = wavePow * wave; 
	return wave;
}


__KERNEL__ void CatvolleyballJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(WaveDistort, 0);

    if(WaveDistort)
    {
      float2 uv = fragCoord / iResolution;
      float2 so = swi2(iMouse,x,y) / iResolution;
      float2 pos2 = uv - so; 
      float2 pos2n = normalize(pos2);
      float dist = distance_f2(uv, so);

      float len = length(pos2);
      float wave = genWave(len,iTime); 

      float2 uv2 = -pos2n * wave/(1.0 + 5.0 * len);

      fragColor = _tex2DVecN(iChannel0, uv.x + uv2.x, uv.y + uv2.y, 15);

    }
    else
    {
      float2 uv = fragCoord / iResolution;
      float aspect = iResolution.x / iResolution.y;
      Camera cam = GetCamera(iResolution, iMouse, iTime);
      
      float3 ro = cam.position;
      float3 rd = MakeWorldRay(uv, aspect, FOV, cam);
      
      float4 sphere = to_float4(0.0f, 0.0f, 0.0f, 0.5f);
      
      float3 col = to_float3_s(0.0f);
      
      float hitDist = -1.0f;
      float3 hitNormal = to_float3_s(0.0f);
      for (int i = 0; i < STEPS; ++i)
      {
          float t = ((float)(i) + 0.5f) / (float)(STEPS);
          float3 pos = ro + rd * (t * RAY_LEN);

          float3 gpos = fract_f3(pos / GRID_SIZE + 0.5f);
          gpos -= 0.5f;
          gpos *= GRID_SIZE;

          hitDist = RayCastSphere(gpos, rd, sphere, &hitNormal);
          if (hitDist > 0.0f)
          {
              hitDist += (t * RAY_LEN);
              hitDist = clamp(hitDist, 0.0f, RAY_LEN);
              break;
          }
      }
      
      if (hitDist > 0.0f)
      {
          float3 lightDir = normalize(LIGHT_DIR);
          float3 D, S;
          float R;
          SampleMaterial(hitNormal, to_float3_s(1.0f), &D, &S, &R,iTime, iChannel0);
          float3 sphCol = to_float3_s(0.0f);
          sphCol += DirectRadiance(hitNormal, -rd, lightDir, D, S, R);
          sphCol += IndirectRadiance(hitNormal, -rd, D, S, R, iChannel1);
          
          float t = hitDist / RAY_LEN;
          col = _mix(sphCol, GetFogColor(rd), GetFogIntensity(t, 0.1f, 0.5f));
      }
      else
      {
          col = GetFogColor(rd);
      }

      col = pow_f3(col, to_float3_s(1.0f / 2.2f));
      fragColor = to_float4_aw(col, 1.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}