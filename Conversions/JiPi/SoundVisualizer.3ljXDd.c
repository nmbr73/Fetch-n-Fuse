
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/3c33c415862bb7964d256f4749408247da6596f2167dca2c86cc38f83c244aa6.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0f International License (CC BY 4.0f).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0f/>.
*/

__DEVICE__ float3 hue2rgb(in float h) {
    float3 k = mod_f3(to_float3(5.0f, 3.0f, 1.0f) + to_float3_s(h*360.0f/60.0f), (6.0f));
    return to_float3_s(1.0f) - clamp(_fminf(k, to_float3_s(4.0f) - k), to_float3_s(0.0f), to_float3_s(1.0f));
}

__KERNEL__ void SoundVisualizerFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;

    float aspect = iResolution.x / iResolution.y;
    float blurr = 0.3f;
    float sharpen = 1.7f;
    
    float2 maxWindow = to_float2(3.0f, 3.0f/aspect);
    uv = mix_f2(-maxWindow, maxWindow, uv);

    float r = dot(uv, uv);
    float theta = _atan2f(uv.y, uv.x) + 3.14f;
    
    float t = _fabs(2.0f*theta / (2.0f*3.14f) - 1.0f);

    float signal = 2.0f*texture(iChannel0,to_float2(t,0.75f)).x;
    float ampl = 2.0f*texture(iChannel0,to_float2(0.8f,0.25f)).x;
    
    float v = 1.0f - _powf(smoothstep(0.0f, blurr, _fabs(r - signal)), 0.01f);
    float hue = _powf(fract(_fabs(_sinf(theta/2.0f) * ampl)), sharpen);
      
    fragColor = v * to_float4_aw(hue2rgb(fract(hue + iTime/10.0f)), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0f International License (CC BY 4.0f).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0f/>.
*/

__KERNEL__ void SoundVisualizerFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;
    
    float2 uv = fragCoord/iResolution;

    float2 center = to_float2_s(0.5f + 0.15f*_sinf(iTime));
    float zoom = 1.02f;
    
    float4 prevParams = texture(iChannel0, (uv - center)/zoom + center);
    float4 bB = _tex2DVecN(iChannel1,uv.x,uv.y,15);

    fragColor = _mix(prevParams, bB, 0.1f) ;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0f International License (CC BY 4.0f).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0f/>.
*/

#define R iResolution

__KERNEL__ void SoundVisualizerFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    float3 bA = swi3(texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R),x,y,z);
    float3 bB = swi3(texture(iChannel1, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R),x,y,z);
   
    float3 col_rgb = bA + bB;

    col_rgb = col_rgb*_exp2f(3.5f);
    
    fragColor = to_float4_aw(pow_f3(col_rgb, to_float3_s(1.0f/2.2f)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}