
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/desmond-cheese/dubbin-out' to iChannel0


//Shader License: CC BY 3.0
//Author: Jan Mróz (jaszunio15)

#define LINE_WIDTH 1.6

//Precision of one band from 0 to 1
#define PRECISION 0.25

//Number of bands
#define BANDS_COUNT 64.0

//From 0 to 1
#define HIGH_FREQ_APPERANCE 0.7

#define AMPLITUDE 4.0

__DEVICE__ float hash(in float v)
{
   return fract(_sinf(v * 124.14518f) * 2123.14121f) - 0.5f;
}

__DEVICE__ float getBand(in float freq)
{
   return _powf(texture(iChannel0, to_float2(freq, 0.0f)).r, (2.0f - HIGH_FREQ_APPERANCE));   
}


__DEVICE__ float getSmoothBand(float band, float iterations, float bandStep)
{
   float sum = 0.0f;
    for(float i = 0.0f; i < iterations; i++)
    {
        sum += getBand(band + i * bandStep);
    }
    sum = smoothstep(0.2f, 1.0f, sum / iterations);
    return sum * sum;
}

__DEVICE__ float getOsc(float x)
{
    x *= 1000.0f;
   float osc = 0.0f;
    for (float i = 1.0f; i <= BANDS_COUNT; i++)
    {
       float freq = i / BANDS_COUNT;
        freq *= freq;
        float h = hash(i);
        osc += getSmoothBand(freq, (512.0f / BANDS_COUNT) * PRECISION, ((1.0f / PRECISION) / 512.0f)) 
              * _sinf( freq * (x + iTime * 500.0f * h));
    }
    osc /= float(BANDS_COUNT);
    
    return osc;
}

__KERNEL__ void SoundOscilloscopeJipi625Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 res = iResolution;
    float2 uv = (2.0f * fragCoord - res) / res.x;
    uv.x += iTime * 0.5f;// + 1.5f * hash(iTime);
    
    float ps = 1.0f / _fminf(res.x, res.y);
    
    
    float osc1 = getOsc(uv.x) * AMPLITUDE;
    
    float tgAlpha = clamp(fwidth(osc1) * res.x * 0.5f, 0.0f, 8.0f);
    float verticalThickness = _fabs(uv.y - osc1) / _sqrtf(tgAlpha * tgAlpha + 2.0f);
    
    float line = 1.0f - smoothstep(0.0f, ps * LINE_WIDTH, verticalThickness);
    line = smoothstep(0.0f, 0.5f, line);
    
    float blur = (1.0f - smoothstep(0.0f, ps * LINE_WIDTH * 32.0f, verticalThickness * 4.0f)) * 0.2f;
    
    fragColor = to_float4(line + blur);


  SetFragmentShaderComputedColor(fragColor);
}