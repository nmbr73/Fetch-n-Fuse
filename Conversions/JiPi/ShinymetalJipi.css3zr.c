
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Video' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// just storing the base texture in a buffer so it can be changed from one place

__KERNEL__ void ShinymetalJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float3 colour = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
    fragColor = to_float4_aw(colour, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// sobel filter



__DEVICE__ float LumaSobel(float2 uv, float2 scales, float2 iResolution, float4 iMouse,  __TEXTURE2D__ iChannel0)
{
  
    const float2 KERNEL[9] = 
                            {
                            to_float2(1.0f, -1.0f), to_float2(0.0f, -2.0f), to_float2(-1.0f, -1.0f),
                            to_float2(2.0f, 0.0f), to_float2(0.0f, 0.0f), to_float2(-2.0f, 0.0f),
                            to_float2(1.0f, 1.0f), to_float2(0.0f, 2.0f), to_float2(-1.0f, 1.0f)
                            };

    const float3 LUMA = to_float3(0.299f, 0.587f, 0.114f);
  
  
    float2 total = to_float2(0.0f, 0.0f);
    float scale;
    
    if (iMouse.z > 0.0f)
        scale = iMouse.y / iResolution.y;
    else
        scale = 0.5f;
    
    float2 pixelSize = scale * 4.0f / iResolution;
    
    for (int y = -1, z = 0; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++, z++)
        {
            float2 pixelUV = uv + pixelSize * to_float2(x, y);
            float3 pixel = swi3(texture(iChannel0, pixelUV),x,y,z);
            total += KERNEL[z] * dot(LUMA, pixel);
        }
    }

    return total.x * total.y * scales.x;
}

__KERNEL__ void ShinymetalJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 scales;
    
    if (iMouse.z > 0.0f)
        scales = swi2(iMouse,x,y) / iResolution;
    else
        scales = to_float2(_cosf(iTime), _sinf(iTime)) * to_float2(0.25f, 0.125f) + to_float2(0.75f, 0.375f);
    
    float sobel = LumaSobel(uv, scales,iResolution,iMouse,iChannel0);
    float3 colour = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z) * sobel;
    
    fragColor = to_float4_aw(colour, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


const float SHINE_BOOST = 50.0f;

__KERNEL__ void ShinymetalJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;
    float2 uv = fragCoord / iResolution;
    float3 colour = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 bloom = to_float3(0.0f, 0.0f, 0.0f);
    
    int lods = (int)(_log2f(iResolution.x));
    
    // fake mip bloom
    for (int i = 1; i < lods; i++)
        //bloom += swi3(texture(iChannel1, uv, float(i)),x,y,z);
        bloom += swi3(texture(iChannel1, uv),x,y,z);
    
    float bloomStrength;
    
    if (iMouse.z > 0.0f)
        bloomStrength = iMouse.x / iResolution.x;
    else
        bloomStrength = 0.5f;
    
    colour += bloom * bloomStrength * SHINE_BOOST;
    
    fragColor = to_float4_aw(colour / (float)(lods), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
