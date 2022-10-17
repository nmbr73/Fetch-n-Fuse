
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0


#define PI  3.14159265359f
#define time (-iTime*5.0f)



__DEVICE__ float genWave1(float len, float iTime)
{
  float wave = _sinf(8.0f * PI * len + time);
  wave = (wave + 1.0f) * 0.5f; // <0 ; 1>
  wave -= 0.3f;
  wave *= wave * wave;
  return wave;
}

__DEVICE__ float genWave2(float len, float iTime)
{
  float wave = _sinf(7.0f * PI * len + time);
  float wavePow = 1.0f - _powf(_fabs(wave*1.1f), 0.8f);
  wave = wavePow * wave; 
  return wave;
}

__DEVICE__ float scene(float len, float iTime)
{
  // you can select type of waves
  return genWave1(len, iTime);
}

__DEVICE__ float2 normal(float len, float iTime) 
{
  const float3 eps = to_float3(0.01f, 0.0f, 0.0f);
  
  float tg = (scene(len + eps.x, iTime) - scene(len, iTime)) / eps.x;
  return normalize(to_float2(-tg, 1.0f));
}

__KERNEL__ void WaterDropsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  float ratio = iResolution.x/iResolution.y;
  //uv.x*=ratio;
  float2 so = swi2(iMouse,x,y) / iResolution;
  float2 pos2 = (uv - so);     //wave origin
  float2 pos2n = normalize(pos2);

  float len = length(pos2);
  float wave = scene(len, iTime); 

  float2 uv2 = -pos2n * wave/(1.0f + 5.0f * len);

  fragColor = _tex2DVecN(iChannel0, uv.x + uv2.x, uv.y + uv2.y, 15);

  SetFragmentShaderComputedColor(fragColor);
}