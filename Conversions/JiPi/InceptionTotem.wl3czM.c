
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Processed by 'GLSL Shader Shrinker' (Shrunk by 125 characters)
// (https://github.com/deanthecoder/GLSLShaderShrinker)

// 'Inception Totem' dean_the_coder (Twitter: @deanthecoder)
// https://www.shadertoy.com/view/wl3czM
//
// Another quick and small demo, playing around with creating
// a better wood texture than I've made in the past.
// Still not happy with it - I think it needs some specular in
// the lighter areas, and probably worth of a new shader experiment.
//
// If the totem falls over, let me know...
//
// Thanks to Evvvvil, Flopine, Nusan, BigWings, Iq, Shane
// and a bunch of others for sharing their knowledge!

// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License


struct Hit {
  float d;
  int id;
  float3 uv;
};

// Thanks Shane - https://www.shadertoy.com/view/lstGRB
__DEVICE__ float n31(float3 p) {
  const float3 s = to_float3(7, 157, 113);
  float3 ip = _floor(p);
  p = fract_f3(p);
  p = p * p * (3.0f - 2.0f * p);
  float4 h = to_float4(0, s.y, s.z, s.y + s.z) + dot(ip, s);
float zzzzzzzzzzzzzzzzz;  
  h = _mix(fract_f4(sin_f4(h) * 43.5453f), fract_f4(sin_f4(h + s.x) * 43.5453f), p.x);
  swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y));
  return _mix(h.x, h.y, p.z);
}

__DEVICE__ float n21(float2 p) {
  const float3 s = to_float3(7, 157, 0);
  float2 h,
       ip = _floor(p);
  p = fract_f2(p);
  p = p * p * (3.0f - 2.0f * p);
  h = swi2(s,z,y) + dot(ip, swi2(s,x,y));
  h = _mix(fract_f2(sin_f2(h) * 43.5453f), fract_f2(sin_f2(h + s.x) * 43.5453f), p.x);
  return _mix(h.x, h.y, p.y);
}

__DEVICE__ float n11(float p) {
  float ip = _floor(p);
  p = fract(p);
  float2 h = fract_f2(sin_f2(to_float2(ip, ip + 1.0f) * 12.3456f) * 43.5453f);
  return _mix(h.x, h.y, p * p * (3.0f - 2.0f * p));
}

__DEVICE__ float smin(float a, float b, float k) {
  float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
  return _mix(b, a, h) - k * h * (1.0f - h);
}

__DEVICE__ Hit minH(Hit a, Hit b) {
  if (a.d < b.d) return a;
  return b;
}

__DEVICE__ mat2 rot(float a) {
  float c = _cosf(a),
        s = _sinf(a);
  return to_mat2(c, s, -s, c);
}

__DEVICE__ float sdCyl(float3 p, float2 hr) {
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)), p.y)) - hr;
  return _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f)));
}

__DEVICE__ float sdCapsule(float3 p, float h, float r) {
  p.y -= clamp(p.y, 0.0f, h);
  return length(p) - r;
}

__DEVICE__ float3 getRayDir(float3 ro, float2 uv) {
  float3 f = normalize(-ro),
       r = normalize(cross(to_float3(0, 1, 0), f));
  return normalize(f + r * uv.x + cross(f, r) * uv.y);
}

__DEVICE__ float wood(float2 p) {
  p.x *= 71.0f;
  p.y *= 1.9f;
  return n11(n21(p) * 30.0f);
}

__DEVICE__ Hit map(float3 p, float time) {
  float t,
        f = p.y;
  p.x += 0.2f + _cosf(time * 10.0f) * 0.05f;
  p.z += 3.5f + _sinf(time * 10.0f) * 0.05f;
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(time * 150.0f)));
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot(_mix(0.02f, 0.04f, _sinf(time * 0.001f) * 0.5f - 0.5f))));
  p.y -= 0.4f;
  t = 1.0f - _fabs(p.y / 0.4f + 0.07f);
float tttttttttttttt;  

  Hit par1 = {f, 1, p};
  Hit par2 = {smin(sdCyl(p, to_float2(smoothstep(0.0f, 1.0f, t * t * t) * 0.35f, 0.4f)), sdCapsule(p + to_float3(0, 0.35f, 0), 0.8f, 0.01f), _mix(0.03f, 0.3f, t * 0.7f)), 2, p};
   
  return minH ( par1, par2 );
}

__DEVICE__ float3 calcN(float3 p, float t, float time) {
  float h = 0.004f * t;
  float3 n = to_float3_s(0);
  //for (int i = _fminf(iFrame, 0); i < 4; i++) {
    for (int i =  0; i < 4; i++) {
    float3 e = 0.5773f * (2.0f * to_float3(((i + 3) >> 1) & 1, (i >> 1) & 1, i & 1) - 1.0f);
    n += e * map(p + e * h, time).d;
  }

  return normalize(n);
}

__DEVICE__ float calcShadow(float3 p, float3 ld, float time) {
  // Thanks iq.
  float s = 1.0f,
        t = 0.1f;
  for (float i = 0.0f; i < 20.0f; i++) {
    float h = map(p + ld * t, time).d;
    s = _fminf(s, 15.0f * h / t);
    t += h;
    if (s < 0.001f || t > 6.0f) break;
  }

  return clamp(s, 0.0f, 1.0f);
}

// Quick ambient occlusion.
__DEVICE__ float ao(float3 p, float3 n, float h, float time) { return map(p + h * n, time).d / h; }

__DEVICE__ float3 vignette(float3 c, float2 fc, float2 iResolution) {
  float2 q = swi2(fc,x,y) / iResolution;
  c *= 0.5f + 0.5f * _powf(16.0f * q.x * q.y * (1.0f - q.x) * (1.0f - q.y), 0.4f);
  return c;
}

__DEVICE__ float3 lights(float3 p, float3 rd, float d, Hit h, float time) {
  float3 mat,
       ld = normalize(to_float3(6, 3, -10) - p),
       ld2 = ld * to_float3(-1, 1, 1),
       n = calcN(p, d, time);
  if (h.id == 1) {
    // Table.
    mat = _mix(_mix(to_float3(0.17f, 0.1f, 0.05f), to_float3(0.08f, 0.05f, 0.03f), wood(swi2(p,x,z))), to_float3(0.2f, 0.16f, 0.08f), 0.3f * wood(swi2(p,x,z) * 0.2f));
    n.x -= smoothstep(0.98f, 1.0f, _powf(_fabs(_sinf(p.x * 2.4f)), 90.0f)) * 0.3f;
    n = normalize(n);
  }
  else // Totem.
        mat = 0.03f * _mix(to_float3(0.4f, 0.3f, 0.2f), _mix(to_float3(0.6f, 0.3f, 0.2f), 2.0f * to_float3(0.7f, 0.6f, 0.5f), n31(h.uv * 1e2)), n31(h.uv * 36.5f));

  float _ao = dot(to_float3(ao(p, n, 0.2f,time), ao(p, n, 0.5f,time), ao(p, n, 2.0f,time)), to_float3(0.3f, 0.4f, 0.3f)),
        l1 = _fmaxf(0.0f, 0.1f + 0.9f * dot(ld, n)),
        spe = smoothstep(0.0f, 1.0f, _powf(_fmaxf(0.0f, dot(rd, reflect(ld, n))), 20.0f)) * 10.0f + smoothstep(0.0f, 1.0f, _powf(_fmaxf(0.0f, dot(rd, reflect(ld2, n))), 20.0f)) * 2.0f,
        fre = smoothstep(0.7f, 1.0f, 1.0f + dot(rd, n));

  l1 *= _mix(0.4f, 1.0f, _mix(calcShadow(p, ld, time), calcShadow(p, ld2, time), 0.3f));
  return _mix(mat * (l1 * _ao + spe) * to_float3(2, 1.6f, 1.4f), to_float3_s(0.005f), fre);
}

__DEVICE__ float3 march(float3 ro, float3 rd, float time) {
  // Raymarch.
  float3 p, c;
  float d = 0.01f;
  Hit h;
  for (float i = 0.0f; i < 90.0f; i+=1.0f) {
    p = ro + rd * d;
    h = map(p, time);
    if (_fabs(h.d) < 0.0015f) break;
    if (d > 48.0f) return to_float3_s(0); // Distance limit reached - Stop.
    d += h.d;
  }
float qqqqqqqqqqqqqqqqq;
  c = lights(p, rd, d, h, time) * _expf(-d * 0.14f);
    float f = smoothstep(-2.2f, -3.0f, p.z) * (h.id == 1 ? 0.4f : 1.0f);
  if (f > 0.0f) {
    // Show reflection on the totem.
    ro = p;
    rd = reflect(rd, calcN(p, d, time));
    d = 0.1f;
    for (float i = 0.0f; i < 90.0f; i++) {
      p = ro + rd * d;
      h = map(p, time);
      if (_fabs(h.d) < 0.002f || d > 1.0f) break;
      d += h.d;
    }

    c = _mix(c, d > 1.0f ? to_float3_s(0) : lights(p, rd, d, h, time), 0.2f * f);
  }

  return c;
}

__KERNEL__ void InceptionTotemFuse(float4 fragColor, float2 fc, float iTime, float2 iResolution, int iFrame)
{

  float time = mod_f(iTime * 0.2f, 30.0f);
float IIIIIIIIIIIIIIIIIII;    
  float3 ro = to_float3(0, 0, -5),
        col = to_float3_s(0);
  swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , rot(-0.13f - _sinf(time * 0.3f) * 0.02f)));
  swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rot(0.07f + _cosf(time) * 0.02f)));
    
  for (float dx = 0.0f; dx <= 1.0f; dx+=1.0f) {
    for (float dy = 0.0f; dy <= 1.0f; dy+=1.0f) {
      float2 uv = (fc + to_float2(dx, dy) * 0.5f - 0.5f * iResolution) / iResolution.y;
      col += march(ro, getRayDir(ro, uv), time);
    }
  }

  col /= 4.0f;

  fragColor = to_float4_aw(vignette(pow_f3(col * 3.0f, to_float3_s(0.45f)), fc, iResolution), 0);

  SetFragmentShaderComputedColor(fragColor);
}