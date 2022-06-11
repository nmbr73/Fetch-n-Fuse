
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/desmond-cheese/dubbin-out' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//Shader License: CC BY 3.0
//Author: Jan Mróz (jaszunio15)

//#define LINE_WIDTH 1.6f

//Precision of one band from 0 to 1
#define PRECISION 0.25f

//Number of bands
#define BANDS_COUNT 64.0f

//From 0 to 1
#define HIGH_FREQ_APPERANCE 0.7f

//#define AMPLITUDE 4.0f

__DEVICE__ float hash(in float v)
{
   return fract(_sinf(v * 124.14518f) * 2123.14121f) - 0.5f;
}

__DEVICE__ float getBand(in float freq, __TEXTURE2D__ iChannel0)
{
   return _powf(texture(iChannel0, to_float2(freq, 0.0f)).x, (2.0f - HIGH_FREQ_APPERANCE));   
}


__DEVICE__ float getSmoothBand(float band, float iterations, float bandStep, __TEXTURE2D__ iChannel0)
{
   float sum = 0.0f;
    for(float i = 0.0f; i < iterations; i+=1.0f)
    {
        sum += getBand(band + i * bandStep,iChannel0);
    }
    sum = smoothstep(0.2f, 1.0f, sum / iterations);
    return sum * sum;
}

__DEVICE__ float getOsc(float x,float iTime, __TEXTURE2D__ iChannel0)
{
    x *= 1000.0f;
    float osc = 0.0f;
    for (float i = 1.0f; i <= BANDS_COUNT; i+=1.0f)
    {
       float freq = i / BANDS_COUNT;
        freq *= freq;
        float h = hash(i);
        osc += getSmoothBand(freq, (512.0f / BANDS_COUNT) * PRECISION, ((1.0f / PRECISION) / 512.0f),iChannel0) 
              * _sinf( freq * (x + iTime * 500.0f * h));
    }
    osc /= (float)(BANDS_COUNT);
    
    return osc;
}


__DEVICE__ float _fwidth(float inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}

__KERNEL__ void SoundOscilloscopeJipi625Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER0(Par, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(AMPLITUDE, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(LINE_WIDTH, 0.0f, 10.0f, 1.0f);

    float2 res = iResolution;
    float2 uv = (2.0f * fragCoord - res) / res.x;
    uv.x += iTime * 0.5f;// + 1.5f * hash(iTime);
    
    float ps = 1.0f / _fminf(res.x, res.y);
    
    
    float osc1 = getOsc(uv.x,iTime,iChannel0) * AMPLITUDE;
    
    float tgAlpha = clamp(_fwidth(osc1,iResolution,Par) * res.x * 0.5f, 0.0f, 8.0f);
    float verticalThickness = _fabs(uv.y - osc1) / _sqrtf(tgAlpha * tgAlpha + 2.0f);
    
    float line = 1.0f - smoothstep(0.0f, ps * LINE_WIDTH, verticalThickness);
    line = smoothstep(0.0f, 0.5f, line);
    
    float blur = (1.0f - smoothstep(0.0f, ps * LINE_WIDTH * 32.0f, verticalThickness * 4.0f)) * 0.2f;
    
    fragColor = to_float4_s(line + blur);
      
    Color+=0.5f;
    fragColor = to_float4_aw(swi3(Color,x,y,z)*(line + blur),Color.w-0.5f);  
      

  SetFragmentShaderComputedColor(fragColor);
}