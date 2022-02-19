

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

float samplePiecewiseSmooth(in float x, in float res) {
    float xTimesRes = x * res;

    // Left sample point:
    float x1 = floor(xTimesRes) / res;
    float y1 = texture(iChannel0, vec2(x1, VIEW_MODE)).x;

    // Right sample point:
    float x2 = ceil(xTimesRes) / res;
    float y2 = texture(iChannel0, vec2(x2, VIEW_MODE)).x;

    // Prevent small breaks in the line:
    x2 += 0.001;    

    // Fit half of a sine wave between sample points:
    float sine = sin(((x - x1) / (x2 - x1) * 2.0 - 1.0) * 1.5707963267);
    return y1 + (0.5 + 0.5 * sine) * (y2 - y1);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    float curSample = samplePiecewiseSmooth(uv.x, 20.0);
    // Difference between the pixel position and the sample:
    float smoothError = smoothstep(0.03, 0.0, abs(uv.y - curSample));
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    // If the pixel is close to the line (I know, the naming isn't very intuitive):
    if (smoothError > 0.0) {
        // Mix red and yellow based on closeness:
        fragColor = vec4(mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), smoothError), 1.0);
    }
}