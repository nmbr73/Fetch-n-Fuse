
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Texture: Abstract 1' to iChannel0


/*
This attempts to replicate the transition seen in Oddworld - Abe's Oddysee here:
https://www.youtube.com/watch?v=SYL6nxUkuOo&feature=youtu.be&t=68
*/

// Tweakable parameters
// I'm not sure they are well named for what they do.
// I'd like it if period was calculated from the others such that the effect always loops cleanly.


__KERNEL__ void RippleTextureTransitionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{


    CONNECT_SLIDER0(Blend, 0.0f, 1.0f, 0.0f);

    CONNECT_SLIDER1(Freq, -1.0f, 20.0f, 8.0f);
    CONNECT_SLIDER2(Period, -1.0f, 20.0f, 8.0f);
    CONNECT_SLIDER3(Speed, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER4(Fade, -1.0f, 10.0f, 4.0f);
    CONNECT_SLIDER5(Displacement, -1.0f, 1.0f, 0.20f);
    
    CONNECT_SLIDER6(WaveHeight, -1.0f, 5.0f, 1.0f);

    

    //float freq = 8.0f;
    //float period = 8.0f;
    //float speed = 2.0f;
    //float fade = 4.0f;
    //float displacement = 0.2f;


    float2 R = iResolution,
         U = ((2.0f * fragCoord) - R) / _fminf(R.x, R.y);
    float2 T = fragCoord / R.y;
    float D = length(U);

    float ratio = iResolution.y/iResolution.x;

    float frame_time = mod_f(iTime * Speed, Period);
    float pixel_time = _fmaxf(0.0f, frame_time - D);

    float wave_height = (_cosf(pixel_time * Freq) + 1.0f) / 2.0f;
    float wave_scale = (1.0f - _fminf(1.0f, pixel_time / Fade));
    float frac = wave_height * wave_scale * WaveHeight;
    if (mod_f(iTime * Speed, Period * 2.0f) >= Period)
    {
        frac = 1.0f - frac;
    }

    float2 tc = T + ((U / D) * -((_sinf(pixel_time * Freq) / Fade) * wave_scale) * Displacement);
    
    fragColor = _mix(
        _tex2DVecN(iChannel0,tc.x*ratio,tc.y,15),
        _tex2DVecN(iChannel1,tc.x*ratio,tc.y,15),
        frac);//Blend);//frac);

  SetFragmentShaderComputedColor(fragColor);
}