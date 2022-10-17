// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/enigmacurry/forest-chant' to iChannel0


// Fluidic Space - EnigmaCurry
// Adapted from Simplicity by JoshP
// https://www.shadertoy.com/view/lslGWr
// http://www.fractalforums.com/new-theories-and-research/very-simple-formula-for-fractal-patterns/
__DEVICE__ float field(in float3 p, float iTime) {
  float strength = 4.0f + 0.03f * _logf(1.e-6 + fract(_sinf(iTime) * 4373.11f));
  float accum = 0.0f;
  float prev = 0.0f;
  float tw = 1.11f;
  for (int i = 0; i < 32; ++i) {
    float mag = dot(p/1.3f, p/1.3f);
    p = abs_f3(p) / mag + to_float3(-0.5f, -0.4f, -1.5f);
    float w = _expf(-(float)(i) / 777.0f);
    accum += w * _expf(-strength * _powf(_fabs(mag - prev), 1.9f));
    tw += w;
    prev = mag;
  }

  return _fmaxf(0.0f, 4.0f * accum / tw - 0.5f);
}

__DEVICE__ float4 simplicity(float2 fragCoord, float fft, float iTime, float2 R) {
  float2 uv = 2.0f * fragCoord / iResolution - 1.0f;
  float2 uvs = uv * iResolution / _fmaxf(iResolution.x, iResolution.y);
  float3 p = to_float3_aw(uvs / 3.0f, 0) + to_float3(1.0f, 0.01f, 0.0f);
  p += 2.0f * to_float3(_sinf(iTime / 39.0f), _cosf(iTime / 2100.0f)-2.0f,  _sinf(iTime / 18.0f)-8.0f);
  float t = field(p,iTime);
  float v = (1.0f - _expf((_fabs(uv.x) - 1.0f) * 6.0f)) * (1.0f - _expf((_fabs(uv.y) - 1.0f) * 6.0f));
  return _mix(0.4f, 1.0f, v) * to_float4(1.8f * t * t * t, 1.4f * t * t, t, 1.0f) * fft;
}

__DEVICE__ float4 simplicity2(float2 fragCoord, float fft, float iTime, float2 R) {
  float fmod = _tanf(fft/21222.0f);
  float2 uv = 2.0f * fragCoord / iResolution - 1.0f;
  float2 uvs = uv * iResolution / _fmaxf(iResolution.x, iResolution.y);
  float3 p = to_float3_aw(uvs / 333.0f, 0) + to_float3(1.0f, 0.1f, 0.0f);
  p += _tanf(fmod) * to_float3(_cosf(iTime / 39.0f), _tanf(iTime / 2100.0f)-2.0f,  _sinf(iTime / 18.0f)-8.0f);
  float t = field(p,iTime);
  float v = (1.0f - _expf((_fabs(uv.x) - 1.0f) * 6.0f)) * (1.0f - _expf((_fabs(uv.y) - 1.0f) * 6.0f));
  return _mix(0.4f, 1.0f, v) * to_float4(1.8f * t * t * t, 1.4f * t * t, t, 1.0f) * fft;
}

__DEVICE__ float4 simplicity3(float2 fragCoord, float fft, float iTime, float2 R) {
  float fmod = _cosf(fft*13.0f);
  float2 uv = 2.0f * fragCoord / iResolution - 1.0f;
  float2 uvs = uv * iResolution / _fmaxf(iResolution.x, iResolution.y);
  float3 p = to_float3_aw(uvs / 1.0f, 0) + to_float3(1.0f, 0.01f, 0.0f);
  p += 2.19f * to_float3(_cosf(iTime / 3900.0f), _tanf(iTime / 2100.0f)-2.0f,  _sinf(iTime / 18.0f)-8.0f);
  float t = field(p,iTime);
  float v = (1.0f - _expf((_fabs(uv.x) - 1.0f) * 6.0f)) * (1.0f - _expf((_fabs(uv.y) - 1.0f) * 6.0f));
  return _mix(_sinf(fmod)+8.8f, 1.0f, v) * to_float4(0.8f * t * p.x * t, 0.9f * t, t, 1.0f) * fft;
}


__KERNEL__ void FluidicSpaceJipi699Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(FFT, -10.0f, 10.0f, 0.0f);

    float fft = clamp(texture( iChannel0, to_float2(0.1f,0.1f) ).x * 12.0f, 0.2f, 99999.0f);
    fft+=FFT;
    fragColor += sqrt_f4(simplicity(fragCoord, fft, iTime,R));
    fragColor += sqrt_f4(simplicity2(fragCoord, fft, iTime,R));
    fragColor += sqrt_f4(simplicity3(fragCoord, fft, iTime,R));

   if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
   if (ApplyColor)
   {
     fragColor = fragColor + (Color-0.5f);
     fragColor.w = Color.w;
   }

  SetFragmentShaderComputedColor(fragColor);
}