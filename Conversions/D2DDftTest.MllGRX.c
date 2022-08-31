
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/c3a071ecf273428bc72fc72b2dd972671de8da420a2d4f917b75d20e1c24b34c.ogv' to iChannel0


#define FFT_SIZE 48
#define PI 3.14159265359

#define avg(v) ((v.x+v.y+v.z)/3.0f)

__DEVICE__ float2 fft(float2 uv)
{
    float2 complex = to_float2(0,0);
    
    uv *= float(FFT_SIZE);
    
    float size = float(FFT_SIZE);
    
    for(int x = 0;x < FFT_SIZE;x++)
    {
      for(int y = 0;y < FFT_SIZE;y++)
      {
            float a = 2.0f * PI * (uv.x * (float(x)/size) + uv.y * (float(y)/size));
            float3 samplev = texture(iChannel0,mod_f(to_float2(x,y)/size,1.0f)).rgb;
            complex += avg(samplev)*to_float2(_cosf(a),_sinf(a));
        }
    }
    
    return complex;
}

__KERNEL__ void D2DDftTestFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 res = iResolution / iResolution.y;
  float2 uv = fragCoord / iResolution.y;
    uv.x += (1.0f-res.x)/2.0f;
    uv.y = 1.0f-uv.y;
    
    float3 color = to_float3_s(0.0f);
    
    color = _tex2DVecN(iChannel0,uv.x,uv.y,15).rgb;
    
    if(uv.x < 1.0f && uv.x > 0.0f)
    {
      color = to_float3_aw(length(fft(uv-0.5f))/float(FFT_SIZE));
    }
    
  fragColor = to_float4(color,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}