
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float hash(float3 v3) {
  return fract(_sinf(dot(v3, to_float3(12.3f, 45.6f, 78.9f))) * 987654.321f);
}

__DEVICE__ float noise(float3 v3) {
  float3 i = _floor(v3);
  float3 f = fract_f3(v3);
  float3 b = smoothstep(to_float3_s(0.0f), to_float3_s(1.0f), f);
  float2 bin = to_float2(0.0f, 1.0f);
  return 2.0f * _mix(
                _mix(
                _mix(hash(i + swi3(bin,x,x,x)), hash(i + swi3(bin,y,x,x)), b.x),
                _mix(hash(i + swi3(bin,x,y,x)), hash(i + swi3(bin,y,y,x)), b.x),
                b.y
                    ),
                _mix(
                _mix(hash(i + swi3(bin,x,x,y)), hash(i + swi3(bin,y,x,y)), b.x),
                _mix(hash(i + swi3(bin,x,y,y)), hash(i + swi3(bin,y,y,y)), b.x),
                b.y
                    ),
                b.z
                    ) - 1.0f;
}

__DEVICE__ float3 rotate_y(float3 p, float t) {
    float3 a = to_float3(0.0f, 1.0f, 0.0f);
  return _mix(dot(a, p) * a, p, _cosf(t)) + cross(a, p) * _sinf(t);
}




__DEVICE__ float fbm_core(float3 p, float amp, float freq, float mul_amp, float mul_freq) {
  float h = 0.0f;
  for (int i = 0; i < 6; ++i) {
    h += amp * noise(p * freq);
    amp *= mul_amp;
    freq *= mul_freq;
  }
  return h;
}

__DEVICE__ float height(float3 p, float iTime, float RC, float RS, float H) {
  if (length(p) > RC) return 0.0f;
  p = normalize(p) * RS;
  p = rotate_y(p, iTime * 0.1f);
  return fbm_core(p, 0.8f, 0.2f, 0.4f, 2.7f) * H;
}

__DEVICE__ float d_sea(float3 p, float RS) {
  return length(p) - RS;
}

__DEVICE__ float d_ground(float3 p, float iTime, float RC, float RS, float H) {
  return length(p) - (_fmaxf(height(p, iTime, RC, RS, H), 0.0f) + RS);
}

__DEVICE__ float d_sphere(float3 p, float radius) {
  return length(p) - radius;
}

__DEVICE__ float4 rt_sphere(float3 p, float3 rd, float radius) {
  float hit = 0.0f;
  for(int i = 0; i < 100; ++i) {
    float d = d_sphere(p, radius);
    p += d * rd;
    if (d < 0.01f) {
      hit = 1.0f;
      break;
    }
  }
  return to_float4_aw(p, hit);
}

__DEVICE__ float3 normal_ground(float3 p, float iTime, float RC, float RS, float H) {
  //mat3 k = to_mat3_f3(p, p, p) - to_mat3_f(0.001f);
  
  float3 k[3];
    k[0] = to_float3(p.x-0.001,p.y,p.z);
    k[1] = to_float3(p.x,p.y-0.001,p.z);
    k[2] = to_float3(p.x,p.y,p.z-0.001);
  
  return normalize(d_ground(p,iTime,RC,RS,H) - to_float3(d_ground(k[0],iTime,RC,RS,H), d_ground(k[1],iTime,RC,RS,H), d_ground(k[2],iTime,RC,RS,H)));
}

__DEVICE__ float3 light_dir(float iTime) {
    return normalize(to_float3(0.6f * _sinf(0.1f * iTime), 0.5f, 1.0f));
}

__DEVICE__ float3 shade_star(float3 p, float3 rd, float iTime, float RC, float RS, float H) {
  float h = height(p,iTime,RC,RS,H);
  float3 snow = to_float3_s(1.0f);
  float3 sand = to_float3(0.7f, 0.66f, 0.53f);
  float3 grass = to_float3(0.1f, 0.7f, 0.3f);
  float snow_r = _expf(-_fabs(h - 0.5f * H) * 20.0f);
  float sand_r = _expf(-_fabs(h - 0.25f * H) * 20.0f);
  float grass_r = _expf(-_fabs(h) * 20.0f);
  float sum_r = snow_r + sand_r + grass_r;
  float3 dif_mat = (snow * snow_r + sand * sand_r + grass * grass_r) / sum_r;
  float3 ng = normal_ground(p,iTime,RC,RS,H);
  float3 L = light_dir(iTime);
  float dif_pow = _fmaxf(0.0f, dot(L, ng));
  float3 ground = to_float3_s(0.1f) + dif_mat * dif_pow;

  float3 ns = normalize(p);
  float3 rs = reflect(rd, ns);

  float3 sea = to_float3(0.1f, 0.4f, 0.9f) * (0.1f * dif_pow - 2.0f * h / H + 0.9f * _expf(_fmaxf(dot(L, rs), 0.0f))) + 0.1f * _sinf(h/H);
  return h < 0.0f ? sea : ground;
}

__DEVICE__ float cloud(float3 p, float iTime) {
  float zzzzzzzzzzzzz;
  p = rotate_y(p, iTime * -0.1f);
  p += 0.1f * iTime;
  return _powf(clamp(_fabs(fbm_core(p, 1.1f, 0.3f, 0.5f, 2.2f)), 0.0f, 1.0f), 2.0f);
}

__DEVICE__ float cloud_shadow(float3 p, float iTime, float H) {
  p -= light_dir(iTime) * H;
  return 1.0f - cloud(p, iTime) * 0.1f;
}

__DEVICE__ float3 bg(float2 uv, float iTime) {
  float t = iTime * 0.0001f;
  float3 from = to_float3(t * 2.0f, t, -1.0f);
  float s = 0.1f, fade = 1.0f;
  float3 v = to_float3_s(0.0f);
  for (int r = 0; r < 20; ++r) {
    float2 rot_uv = mul_f2_mat2(uv , to_mat2(_cosf(s), _sinf(s), -_sinf(s), _cosf(s)));
    float3 p = to_float3_aw(rot_uv, -s) * 0.25f + from;
    float repeat = 2.0f;
    p = mod_f3(p + 0.5f * repeat, repeat) - 0.5f * repeat;
    p *= 10.0f;
    float pa = 0.0f, a = 0.0f;
    for (int i = 0; i < 17; ++i) {
      p = abs_f3(p) / dot(p, p) - 0.53f;
      a += _fabs(length(p) - pa);
      pa = length(p);
    }
    a *= a * a;
    v += to_float3(s, s*s, s*s*s*s) * a * 0.0015f * fade;
    fade *= 0.75f;
    s += 0.1f;
  }
  return v * 0.005f;
}

__DEVICE__ float3 render_base(float2 uv, float3 p, float3 rd, float iTime, float RC, float RS, float H, float C, float3 sky_base_color) {
  float4 rt = rt_sphere(p, rd, RS);
  if (rt.w > 0.0f) {
    return shade_star(swi3(rt,x,y,z), rd, iTime, RC,RS,H) * cloud_shadow(swi3(rt,x,y,z), iTime, H);
  } else {
    //float3 sky_base_color = to_float3(0.1f, 0.4f, 0.8f);
    float sky_mix = smoothstep(RS, RS + 10.0f * C, length(swi2(rd,x,y) * 10.0f));
    return _mix(sky_base_color, bg(uv, iTime), sky_mix);
  }
}

__DEVICE__ float3 render(float2 uv, float3 p, float3 rd, float iTime, float RC, float RS, float H, float C, float3 sky_base_color) {
  float4 rt = rt_sphere(p, rd, RC);
  float ccol = rt.w > 0.0f ? cloud(swi3(rt,x,y,z),iTime) : 0.0f;
  return _mix(render_base(uv, p, rd, iTime,RC,RS,H,C,sky_base_color), to_float3_s(1.0f), ccol);
}

__KERNEL__ void BasicPlanetFuse(float4 fragColor, float2 fragCoord, float iTime, float4 iMouse, float2 iResolution)
{

  CONNECT_COLOR0(SkyBaseColor, 0.1f, 0.4f, 0.8f, 1.0f);
  CONNECT_SLIDER0(Zoom, -20.0f, 20.0f, 0.0f);
  
  CONNECT_SLIDER1(RS, -1.0f, 20.0f, 5.0f);
  CONNECT_SLIDER2(C, -1.0f, 1.0f, 0.1f);
  CONNECT_SLIDER3(H, -1.0f, 10.0f, 1.0f);
  

  //const float RS = 5.0f;
  //const float C = 0.1f; // height of cloud
  const float RC = RS + C;
  //const float H = 1.0f;

  float3 muv = to_float3( iMouse.x/iResolution.x - 0.5f,iMouse.y/iResolution.y - 0.5f, Zoom);

  float2 uv = (2.0f * fragCoord - iResolution) / _fminf(iResolution.x, iResolution.y);
  float3 p = to_float3(0.0f, 0.0f, 10.0f);
  float3 rd = normalize(to_float3_aw(uv, -1.0f)+ muv);

  fragColor = to_float4_aw( render(uv, p, rd, iTime,RC,RS,H,C, swi3(SkyBaseColor,x,y,z)), 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}