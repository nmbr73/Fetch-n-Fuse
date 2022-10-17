
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------




__DEVICE__ float rand(float2 n) {
  return fract(_cosf(dot(n, to_float2(12.9898f, 4.1414f))) * 43758.5453f);
}

__DEVICE__ float noise(float2 n) {
  const float2 d = to_float2(0.0f, 1.0f);
  float2 b = _floor(n), f = smoothstep(to_float2_s(0.0f), to_float2_s(1.0f), fract_f2(n));
  return _mix(_mix(rand(b), rand(b + swi2(d,y,x)), f.x), _mix(rand(b + swi2(d,x,y)), rand(b + swi2(d,y,y)), f.x), f.y);
}

__DEVICE__ float fbm(float2 n) {
  float total = 0.0f, amplitude = 1.0f;
  for (int i = 0; i < 4; i++) {
    total += noise(n) * amplitude;
    n += n;
    amplitude *= 0.5f;
  }
  return total;
}


__KERNEL__ void VioletFogJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  CONNECT_COLOR0(c1, 0.48627f, 0.0f, 0.38039f, 1.0f);
  CONNECT_COLOR1(c2, 0.67843f, 0.0f, 0.63294f, 1.0f);
  CONNECT_COLOR2(c3, 0.2f,     0.0f, 0.0f, 1.0f);
  CONNECT_COLOR3(c4, 0.64314f, 0.00392f, 0.84078f, 1.0f);
  CONNECT_COLOR4(c5, 0.1f,     0.1f, 0.1f, 1.0f);
  CONNECT_COLOR5(c6, 0.9f,     0.9f, 0.9f, 1.0f);
  //CONNECT_COLOR6(Color7 0.5f, 0.6f, 0.6f, 1.0f);
  
  CONNECT_POINT0(speed, 0.1f, 0.4f);
  CONNECT_SLIDER0(shift, -1.0f, 3.0f, 1.6f);

  //const float3 c1 = to_float3(124.0f/255.0f, 0.0f/255.0f, 97.0f/255.0f);
  //const float3 c2 = to_float3(173.0f/255.0f, 0.0f/255.0f, 161.4f/255.0f);
  //const float3 c3 = to_float3(0.2f, 0.0f, 0.0f);
  //const float3 c4 = to_float3(164.0f/255.0f, 1.0f/255.0f, 214.4f/255.0f);
  //const float3 c5 = to_float3_s(0.1f);
  //const float3 c6 = to_float3_s(0.9f);

  //float2 speed = to_float2(0.1f, 0.4f);
  //float shift = 1.6f;
  float2 p = fragCoord * 8.0f / swi2(iResolution,x,x);
  float q = fbm(p - iTime * 0.1f);
  float2 r = to_float2(fbm(p + q + iTime * speed.x - p.x - p.y), fbm(p + q - iTime * speed.y));
  float4 c = _mix(c1, c2, fbm(p + r)) + _mix(c3, c4, r.x) - _mix(c5, c6, r.y);
  float grad = fragCoord.y / iResolution.y;
  fragColor = (c * _cosf(shift * fragCoord.y / iResolution.y));
  fragColor *= (1.0f-grad);
  fragColor.w = c1.w;

  SetFragmentShaderComputedColor(fragColor);
}