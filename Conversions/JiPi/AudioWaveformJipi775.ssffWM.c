
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/894a09f482fb9b2822c093630fc37f0ce6cfec02b652e4e341323e4b6e4a4543.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
Version three of my Audio Waveform Visualizer.
I combined the sampling technique from version 2 (https://www.shadertoy.com/view/Wd3BRl)
with the curve fitting idea from my shader "Sine Wave Curve Fitting". The coloring was
partially inspired by Dave_Hoskins shader "Curve fitting". The curve fitting idea didn't
turn out to be too great for modelling characters but it was fun to experiment with and
maybe I'll use it again sometime. However, I decided it would work nicely in my Audio
Waveform Visualizer so this is what I did with it.
*/

// 0 for frequency mode, 1 for amplitude mode:
#define VIEW_MODE 0

__DEVICE__ float samplePiecewiseSmooth(in float x, in float res, __TEXTURE2D__ iChannel0, bool ViewMode) {
    float xTimesRes = x * res;

    // Left sample point:
    float x1 = _floor(xTimesRes) / res;
    float y1 = texture(iChannel0, to_float2(x1, (float)ViewMode)).x;

    // Right sample point:
    float x2 = _ceil(xTimesRes) / res;
    float y2 = texture(iChannel0, to_float2(x2, (float)ViewMode)).x;

    // Prevent small breaks in the line:
    x2 += 0.001f;    

    // Fit half of a sine wave between sample points:
    float sine = _sinf(((x - x1) / (x2 - x1) * 2.0f - 1.0f) * 1.5707963267f);
    return y1 + (0.5f + 0.5f * sine) * (y2 - y1);
}

__KERNEL__ void AudioWaveformJipi775Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(ViewMode, 0);

    float2 uv = fragCoord / iResolution;
    float zzzzzzzzzzzzzzzzzzzzz;
    float curSample = samplePiecewiseSmooth(uv.x, 20.0f,iChannel0, ViewMode);
    // Difference between the pixel position and the sample:
    float smoothError = smoothstep(0.03f, 0.0f, _fabs(uv.y - curSample));
    fragColor = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
    // If the pixel is close to the line (I know, the naming isn't very intuitive):
    if (smoothError > 0.0f) {
        // Mix red and yellow based on closeness:
        fragColor = to_float4_aw(_mix(to_float3(1.0f, 0.0f, 0.0f), to_float3(1.0f, 1.0f, 0.0f), smoothError), 1.0f);
    }


  SetFragmentShaderComputedColor(fragColor);
}