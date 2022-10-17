
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0


__KERNEL__ void RefractiveJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  float2 uv = fragCoord / iResolution;
    float4 jc = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    uv -= to_float2(1.05f,0.15f);
    fragColor = _tex2DVecN(iChannel1, uv.x + jc.x, uv.y + jc.z, 15);


  SetFragmentShaderComputedColor(fragColor);
}