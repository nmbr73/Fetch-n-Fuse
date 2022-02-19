

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

float hash(in float v)
{
 	return fract(sin(v * 124.14518) * 2123.14121) - 0.5;
}

float getBand(in float freq)
{
 	return pow(texture(iChannel0, vec2(freq, 0.0)).r, (2.0 - HIGH_FREQ_APPERANCE));   
}


float getSmoothBand(float band, float iterations, float bandStep)
{
 	float sum = 0.0;
    for(float i = 0.0; i < iterations; i++)
    {
        sum += getBand(band + i * bandStep);
    }
    sum = smoothstep(0.2, 1.0, sum / iterations);
    return sum * sum;
}

float getOsc(float x)
{
    x *= 1000.0;
 	float osc = 0.0;
    for (float i = 1.0; i <= BANDS_COUNT; i++)
    {
     	float freq = i / BANDS_COUNT;
        freq *= freq;
        float h = hash(i);
        osc += getSmoothBand(freq, (512.0 / BANDS_COUNT) * PRECISION, ((1.0 / PRECISION) / 512.0)) 
            	* sin( freq * (x + iTime * 500.0 * h));
    }
    osc /= float(BANDS_COUNT);
    
    return osc;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  	vec2 res = iResolution.xy;
    vec2 uv = (2.0 * fragCoord - res) / res.x;
    uv.x += iTime * 0.5;// + 1.5 * hash(iTime);
    
    float ps = 1.0 / min(res.x, res.y);
    
    
    float osc1 = getOsc(uv.x) * AMPLITUDE;
    
    float tgAlpha = clamp(fwidth(osc1) * res.x * 0.5, 0.0, 8.0);
    float verticalThickness = abs(uv.y - osc1) / sqrt(tgAlpha * tgAlpha + 2.0);
    
    float line = 1.0 - smoothstep(0.0, ps * LINE_WIDTH, verticalThickness);
    line = smoothstep(0.0, 0.5, line);
    
    float blur = (1.0 - smoothstep(0.0, ps * LINE_WIDTH * 32.0, verticalThickness * 4.0)) * 0.2;
    
    fragColor = vec4(line + blur);
}