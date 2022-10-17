
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Video' to iChannel2
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel0
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void LiquidWomanJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  float2 uv = fragCoord / iResolution;
    
  float4 girl = texture(iChannel2, to_float2(uv.x,1.0f-uv.y));

  float3 ray = to_float3(uv.x - 0.5f + 0.7f * girl.x + 0.1f * girl.z,uv.y - 0.5f + 0.7f * girl.y + 0.1f * girl.z,1.0f);

  float4 cube0 = decube_f3(iChannel0,ray);
  float4 cube1 = decube_f3(iChannel1,ray);
  
  float4 result = cube0 + cube1 + 0.1f * girl;
  
  fragColor = result;


  SetFragmentShaderComputedColor(fragColor);
}