
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/e81e818ac76a8983d746784b423178ee9f6cdcdf7f8e8d719341a6fe2d2ab303.webm' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'https://soundcloud.com/capsadmin/bill' to iChannel1


#define FFT(f) texture(iChannel1, to_float2(f, 0.0f)).x
#define PIXEL(x, y) texture(iChannel0, uv + to_float2(x, y) / iResolution).r

__KERNEL__ void RythmicFluidJipiFuse__Buffer_A(float4 out_color, float2 coordinates, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    
    float2 uv = swi2(coordinates,x,y) / iResolution;
    
    float v = PIXEL(0.0f, 0.0f);
    v = PIXEL(
        _sinf(PIXEL(v, 0.0f)  - PIXEL(-v, 0.0f) + 3.1415f) * v * 0.4f, 
        _cosf(PIXEL(0.0f, -v) - PIXEL(0.0f , v) - 1.57f) * v * 0.4
    );
    v += _powf(FFT(_powf(v*0.1f, 1.5f) * 0.25f) * 1.5f, 3.0f);
    v -= _powf(length(_tex2DVecN(iChannel2,uv.x,uv.y,15)) + 0.05f, 3.0f) * 0.08f;
    v *= 0.925f + FFT(v)*0.1f;
    
    out_color.x = v;


  SetFragmentShaderComputedColor(out_color);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void RythmicFluidJipiFuse(float4 out_color, float2 coordinates, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = coordinates.xy/iResolution;
    float v = _tex2DVecN(iChannel0,uv.x,uv.y,15).r * 1.5f;
        
    float3 color = _powf(to_float3_aw(_cosf(v), _tanf(v), _sinf(v)) * 0.5f + 0.5f, to_float3_s(0.5f));
    float3 e = to_float3_aw(to_float2_s(1.0f) / iResolution, 0.0f);
    float3 grad = normalize(to_float3(
        texture(iChannel0, uv + swi2(e,x,z)).x - texture(iChannel0, uv - swi2(e,x,z)).x, 
        texture(iChannel0, uv + swi2(e,z,y)).x - texture(iChannel0, uv - swi2(e,z,y)).x, 1.0f));
    float3 light = to_float3(0.26f, -0.32f, 0.91f);
    float diffuse = dot(grad, light);
    float spec = _powf(_fmaxf(0.0f, -reflect(light, grad).z), 32.0f);
    
    swi3(out_color,x,y,z) = (color * diffuse) + spec;
    out_color.w = 1.0f;


  SetFragmentShaderComputedColor(out_color);
}