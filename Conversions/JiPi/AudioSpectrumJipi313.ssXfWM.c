
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/48e2d9ef22ca6673330b8c38a260c87694d2bbc94c19fec9dfa4a1222c364a99.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float3 B2_spline(float3 _x) { // returns 3 B-spline functions of degree 2
    float3 t = 3.0f * _x;
    float3 b0 = step(to_float3_s(0.0f), t)      * step(to_float3_s(0.0f), 1.0f-t);
    float3 b1 = step(to_float3_s(0.0f), t-1.0f) * step(to_float3_s(0.0f), 2.0f-t);
    float3 b2 = step(to_float3_s(0.0f), t-2.0f) * step(to_float3_s(0.0f), 3.0f-t);
    return 0.5f * (
      b0 * pow_f3(t, to_float3_s(2.0f)) +
      b1 * (-2.0f*pow_f3(t, to_float3_s(2.0f)) + 6.0f*t - 3.0f) + 
      b2 * pow_f3(3.0f-t,to_float3_s(2.0f))
    );
}

__KERNEL__ void AudioSpectrumJipi313Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    // create pixel coordinates
    float2 uv = fragCoord / iResolution;

    float fVBars = 100.0f;
    float fHSpacing = 1.00f;
    
    
    float x = _floor(uv.x * fVBars)/fVBars;
    float fSample = texture( iChannel0, to_float2(_fabs(2.0f * x - 1.0f), 0.25f)).x;
  
    float squarewave = sign_f(mod_f(uv.x, 1.0f/fVBars)-0.004f);
    float fft = squarewave * fSample* 0.5f;
    
    float fHBars = 100.0f;
    float fVSpacing = 0.180f;
    float fVFreq = (uv.y * 3.14f);
    fVFreq = sign_f(_sinf(fVFreq * fHBars)+1.0f-fVSpacing);

    float2 centered = to_float2_s(1.0f) * uv - to_float2_s(1.0f);
    float t = iTime / 100.0f;
    float polychrome = 1.0f;
    float3 spline_args = fract_f3(to_float3_s(polychrome*uv.x-t) + to_float3(0.0f, -1.0f/3.0f, -2.0f/3.0f));
    float3 spline = B2_spline(spline_args);
    
    float f = _fabs(centered.y);
    float3 base_color  = to_float3(1.0f, 1.0f, 1.0f) - f*spline;
    float3 flame_color = pow_f3(base_color, to_float3_s(3.0f));
    
    float tt = 0.3f - uv.y;
    float df = sign_f(tt);
    df = (df + 1.0f)/0.5f;
    float3 col = flame_color * to_float3_s(1.0f - step(fft, _fabs(0.3f-uv.y))) * to_float3_s(fVFreq);
    col -= col * df * 0.180f;
    
    // output final color
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}