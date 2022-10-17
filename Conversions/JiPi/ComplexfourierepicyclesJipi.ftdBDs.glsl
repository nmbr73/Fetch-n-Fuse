

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
I have always wanted to make this and I finally did after watching The Coding Train's
videos about DFT (Discrete Fourier Transform). This was also my first time actually
going under the hood of complex numbers.
*/

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    fragColor = texelFetch(iChannel1, ivec2(fragCoord), 0);

    float unit = 2.0 / iResolution.y;

    // Time (t) and time step (dt)
    float dt = TAU / float(path.length());
    float t = float(iFrame) * dt;

    // Draw each epicycle
    vec2 pos = vec2(0.0);
    for (int n=0; n < path.length(); n++) {
        vec2 prevPos = pos;
        vec3 epicycle = texelFetch(iChannel0, ivec2(n, 0), 0).xyz;
        fragColor.rgb = mix(fragColor.rgb, vec3(1.0, 1.0, 1.0), smoothstep(unit, 0.0, abs(length(uv - pos) - epicycle.x)));

        float a = t * epicycle.y + epicycle.z;
        pos += vec2(cos(a), sin(a)) * epicycle.x;
        fragColor.rgb = mix(fragColor.rgb, vec3(1.0, 1.0, 1.0), smoothstep(unit, 0.0, sdLine(uv, prevPos, pos)));
    }
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
DFT (Discrete Fourier Transform)
The path or "signal" gets broken down into a set of constant wave patterns.
The DFT output is calculated once and then maintained for the rest of the animation.
*/

// Complex multiplication operator
#define cmul(a, b) vec2(a.x * b.x - a.y * b.y, a.x * b.y + b.x * a.y)

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 iFragCoord = ivec2(fragCoord);
    if (iFrame == 0) {
        // Calculate the DFT of the path
        float N = float(path.length());
        float k = fragCoord.x + 0.5; // fragCoord strangely goes from 0.5 to resolution - 0.5 (according to the documentation)
        vec2 sum = vec2(0.0);
        for (float n=0.0; n < N; n++) {
            float phi = TAU / N * k * n;
            sum += cmul(path[int(n)], vec2(cos(phi), -sin(phi)));
        }

        sum /= N;

        float phase = atan(sum.y, sum.x);
        float amp = length(sum);
        float freq = k;

        fragColor.xyz = vec3(amp, freq, phase);
    }

    if (iFrame > 0) {
        fragColor = texelFetch(iChannel0, iFragCoord, 0); // Maintain buffer data
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//const vec2[] path = vec2[](vec2(-0.345, 0.16333333333333333), vec2(-0.345, 0.195), vec2(-0.345, 0.22666666666666666), vec2(-0.3616666666666667, 0.24333333333333335), vec2(-0.3883333333333333, 0.25), vec2(-0.4166666666666667, 0.255), vec2(-0.4483333333333333, 0.255), vec2(-0.48, 0.255), vec2(-0.5116666666666667, 0.255), vec2(-0.54, 0.25), vec2(-0.56, 0.235), vec2(-0.5833333333333334, 0.21166666666666667), vec2(-0.6066666666666667, 0.18666666666666668), vec2(-0.6116666666666667, 0.155), vec2(-0.6116666666666667, 0.12), vec2(-0.6033333333333334, 0.09166666666666666), vec2(-0.5866666666666667, 0.06), vec2(-0.5666666666666667, 0.04), vec2(-0.5516666666666666, 0.02), vec2(-0.5316666666666666, 0.0), vec2(-0.5033333333333333, -0.0033333333333333335), vec2(-0.4716666666666667, -0.0033333333333333335), vec2(-0.44, -0.0033333333333333335), vec2(-0.44, -0.0033333333333333335), vec2(-0.4766666666666667, -0.0033333333333333335), vec2(-0.5083333333333333, -0.015), vec2(-0.5316666666666666, -0.03166666666666667), vec2(-0.5516666666666666, -0.04666666666666667), vec2(-0.5666666666666667, -0.06333333333333334), vec2(-0.5833333333333334, -0.07833333333333334), vec2(-0.595, -0.09833333333333333), vec2(-0.595, -0.13333333333333333), vec2(-0.58, -0.16166666666666665), vec2(-0.56, -0.18166666666666667), vec2(-0.5316666666666666, -0.20166666666666666), vec2(-0.48333333333333334, -0.225), vec2(-0.4483333333333333, -0.24166666666666667), vec2(-0.4166666666666667, -0.245), vec2(-0.385, -0.245), vec2(-0.325, -0.23), vec2(-0.2816666666666667, -0.21333333333333335), vec2(-0.25333333333333335, -0.19), vec2(-0.23, -0.16166666666666665), vec2(-0.21, -0.13333333333333333), vec2(-0.195, -0.10166666666666667), vec2(-0.17, -0.075), vec2(-0.14666666666666667, -0.035), vec2(-0.14333333333333334, 0.0), vec2(-0.13166666666666665, 0.065), vec2(-0.13166666666666665, 0.09666666666666666), vec2(-0.13166666666666665, 0.12833333333333333), vec2(-0.13166666666666665, 0.16), vec2(-0.13166666666666665, 0.195), vec2(-0.13833333333333334, 0.22666666666666666), vec2(-0.155, 0.25), vec2(-0.17833333333333334, 0.2633333333333333), vec2(-0.20166666666666666, 0.24666666666666667), vec2(-0.21, 0.21166666666666667), vec2(-0.21, 0.175), vec2(-0.20166666666666666, 0.11666666666666667), vec2(-0.20166666666666666, 0.07166666666666667), vec2(-0.20166666666666666, 0.03666666666666667), vec2(-0.20166666666666666, 0.0), vec2(-0.19833333333333333, -0.035), vec2(-0.18666666666666668, -0.06333333333333334), vec2(-0.175, -0.09), vec2(-0.16333333333333333, -0.13), vec2(-0.155, -0.15833333333333333), vec2(-0.14333333333333334, -0.185), vec2(-0.11833333333333333, -0.205), vec2(-0.10333333333333333, -0.22166666666666668), vec2(-0.075, -0.21333333333333335), vec2(-0.051666666666666666, -0.18166666666666667), vec2(-0.035, -0.16166666666666665), vec2(-0.03166666666666667, -0.13333333333333333), vec2(-0.023333333333333334, -0.10166666666666667), vec2(-0.011666666666666667, -0.055), vec2(-0.011666666666666667, -0.023333333333333334), vec2(-0.011666666666666667, 0.013333333333333334), vec2(-0.011666666666666667, 0.045), vec2(-0.011666666666666667, 0.07666666666666666), vec2(-0.011666666666666667, 0.045), vec2(-0.011666666666666667, 0.008333333333333333), vec2(-0.011666666666666667, -0.02666666666666667), vec2(-0.008333333333333333, -0.06333333333333334), vec2(0.011666666666666667, -0.08666666666666667), vec2(0.035, -0.10166666666666667), vec2(0.06, -0.095), vec2(0.08666666666666667, -0.07), vec2(0.10666666666666667, -0.05), vec2(0.11166666666666666, -0.018333333333333333), vec2(0.115, 0.008333333333333333), vec2(0.11833333333333333, 0.03666666666666667), vec2(0.11833333333333333, 0.06833333333333333), vec2(0.135, 0.03666666666666667), vec2(0.12333333333333334, 0.008333333333333333), vec2(0.11166666666666666, -0.018333333333333333), vec2(0.11166666666666666, -0.05), vec2(0.11166666666666666, -0.08333333333333333), vec2(0.11166666666666666, -0.115), vec2(0.11166666666666666, -0.15333333333333332), vec2(0.11166666666666666, -0.19), vec2(0.11166666666666666, -0.23333333333333334), vec2(0.11166666666666666, -0.2683333333333333), vec2(0.10666666666666667, -0.3), vec2(0.095, -0.3333333333333333), vec2(0.08333333333333333, -0.365), vec2(0.055, -0.39166666666666666), vec2(0.023333333333333334, -0.4033333333333333), vec2(0.0033333333333333335, -0.37166666666666665), vec2(0.0033333333333333335, -0.34), vec2(0.016666666666666666, -0.31666666666666665), vec2(0.04, -0.285), vec2(0.055, -0.25666666666666665), vec2(0.075, -0.21666666666666667), vec2(0.095, -0.17), vec2(0.10666666666666667, -0.13833333333333334), vec2(0.12666666666666668, -0.09), vec2(0.14666666666666667, -0.058333333333333334), vec2(0.17, -0.03166666666666667), vec2(0.195, -0.006666666666666667), vec2(0.215, 0.016666666666666666), vec2(0.235, 0.03666666666666667), vec2(0.26166666666666666, 0.04833333333333333), vec2(0.29333333333333333, 0.04833333333333333), vec2(0.285, 0.051666666666666666), vec2(0.25, 0.045), vec2(0.235, 0.02), vec2(0.23833333333333334, -0.018333333333333333), vec2(0.24666666666666667, -0.058333333333333334), vec2(0.26666666666666666, -0.095), vec2(0.29833333333333334, -0.115), vec2(0.3333333333333333, -0.08333333333333333), vec2(0.35, -0.06333333333333334), vec2(0.3566666666666667, -0.02666666666666667), vec2(0.3566666666666667, 0.008333333333333333), vec2(0.345, 0.03666666666666667), vec2(0.325, 0.051666666666666666), vec2(0.3016666666666667, 0.065), vec2(0.3333333333333333, 0.06), vec2(0.3566666666666667, 0.045), vec2(0.36833333333333335, 0.013333333333333334), vec2(0.36833333333333335, -0.035), vec2(0.36833333333333335, -0.07), vec2(0.37666666666666665, -0.10166666666666667), vec2(0.405, -0.115), vec2(0.4483333333333333, -0.075), vec2(0.46, -0.03166666666666667), vec2(0.4683333333333333, 0.02), vec2(0.4716666666666667, 0.08), vec2(0.4716666666666667, 0.135), vec2(0.4716666666666667, 0.16666666666666666), vec2(0.465, 0.20333333333333334), vec2(0.465, 0.235), vec2(0.445, 0.2633333333333333), vec2(0.4166666666666667, 0.27), vec2(0.4166666666666667, 0.235), vec2(0.42, 0.2), vec2(0.42, 0.16666666666666666), vec2(0.42, 0.12833333333333333), vec2(0.42833333333333334, 0.09166666666666666), vec2(0.445, 0.03666666666666667), vec2(0.45666666666666667, -0.011666666666666667), vec2(0.46, -0.058333333333333334), vec2(0.46, -0.095), vec2(0.46, -0.13), vec2(0.46, -0.16666666666666666), vec2(0.465, -0.15833333333333333), vec2(0.465, -0.12666666666666668), vec2(0.465, -0.09), vec2(0.465, -0.058333333333333334), vec2(0.4683333333333333, -0.02666666666666667), vec2(0.48833333333333334, 0.005), vec2(0.5116666666666667, 0.013333333333333334), vec2(0.535, -0.0033333333333333335), vec2(0.535, -0.03833333333333333), vec2(0.535, -0.07), vec2(0.535, -0.10666666666666667), vec2(0.535, -0.13833333333333334), vec2(0.555, -0.16166666666666665), vec2(0.575, -0.17333333333333334), vec2(0.595, -0.15833333333333333), vec2(0.6116666666666667, -0.13), vec2(0.615, -0.08666666666666667), vec2(0.615, -0.04666666666666667), vec2(0.615, 0.005), vec2(0.615, 0.03666666666666667), vec2(0.615, 0.06833333333333333), vec2(0.615, 0.1), vec2(0.6116666666666667, 0.14), vec2(0.6033333333333334, 0.175), vec2(0.6, 0.20333333333333334), vec2(0.5833333333333334, 0.235), vec2(0.575, 0.25833333333333336), vec2(0.5516666666666666, 0.275), vec2(0.5316666666666666, 0.29), vec2(0.5116666666666667, 0.315), vec2(0.49166666666666664, 0.32666666666666666), vec2(0.4683333333333333, 0.3383333333333333), vec2(0.43666666666666665, 0.3383333333333333), vec2(0.405, 0.3383333333333333), vec2(0.37333333333333335, 0.3383333333333333), vec2(0.3416666666666667, 0.3383333333333333), vec2(0.31, 0.32166666666666666), vec2(0.2816666666666667, 0.30666666666666664), vec2(0.25333333333333335, 0.2833333333333333), vec2(0.235, 0.2633333333333333), vec2(0.21, 0.24333333333333335), vec2(0.19, 0.21833333333333332), vec2(0.16666666666666666, 0.20333333333333334), vec2(0.14666666666666667, 0.18333333333333332), vec2(0.13166666666666665, 0.16333333333333333), vec2(0.12333333333333334, 0.13166666666666665), vec2(0.12333333333333334, 0.13166666666666665), vec2(0.12333333333333334, 0.16333333333333333), vec2(0.11166666666666666, 0.195), vec2(0.08333333333333333, 0.22333333333333333), vec2(0.06333333333333334, 0.24666666666666667), vec2(0.035, 0.27), vec2(0.011666666666666667, 0.2633333333333333), vec2(0.008333333333333333, 0.22333333333333333), vec2(0.0033333333333333335, 0.19166666666666668), vec2(-0.008333333333333333, 0.16), vec2(-0.02, 0.14833333333333334), vec2(-0.02, 0.18), vec2(-0.02, 0.21166666666666667), vec2(-0.035, 0.24333333333333335), vec2(-0.051666666666666666, 0.26666666666666666), vec2(-0.07166666666666667, 0.2866666666666667), vec2(-0.08666666666666667, 0.3016666666666667), vec2(-0.10666666666666667, 0.32166666666666666), vec2(-0.13833333333333334, 0.32166666666666666), vec2(-0.175, 0.32166666666666666), vec2(-0.20666666666666667, 0.315), vec2(-0.23333333333333334, 0.29833333333333334), vec2(-0.26166666666666666, 0.2833333333333333), vec2(-0.29333333333333333, 0.25833333333333336), vec2(-0.325, 0.22666666666666666), vec2(-0.345, 0.19166666666666668), vec2(-0.3566666666666667, 0.155));
const vec2[] path = vec2[](vec2(0.85, -0.46), vec2(-0.55, 0.35), vec2(-0.345, 0.22666666666666666), vec2(-0.3616666666666667, 0.24333333333333335), vec2(-0.3883333333333333, 0.25), vec2(-0.4166666666666667, 0.255), vec2(-0.4483333333333333, 0.255), vec2(-0.48, 0.255), vec2(-0.5116666666666667, 0.255), vec2(-0.54, 0.25), vec2(-0.56, 0.235), vec2(-0.5833333333333334, 0.21166666666666667), vec2(-0.6066666666666667, 0.18666666666666668), vec2(-0.6116666666666667, 0.155), vec2(-0.6116666666666667, 0.12), vec2(-0.6033333333333334, 0.09166666666666666), vec2(-0.5866666666666667, 0.06), vec2(-0.5666666666666667, 0.04), vec2(-0.5516666666666666, 0.02), vec2(-0.5316666666666666, 0.0), vec2(-0.5033333333333333, -0.0033333333333333335), vec2(-0.4716666666666667, -0.0033333333333333335), vec2(-0.44, -0.0033333333333333335), vec2(-0.44, -0.0033333333333333335), vec2(-0.4766666666666667, -0.0033333333333333335), vec2(-0.5083333333333333, -0.015), vec2(-0.5316666666666666, -0.03166666666666667), vec2(-0.5516666666666666, -0.04666666666666667), vec2(-0.5666666666666667, -0.06333333333333334), vec2(-0.5833333333333334, -0.07833333333333334), vec2(-0.595, -0.09833333333333333), vec2(-0.595, -0.13333333333333333), vec2(-0.58, -0.16166666666666665), vec2(-0.56, -0.18166666666666667), vec2(-0.5316666666666666, -0.20166666666666666), vec2(-0.48333333333333334, -0.225), vec2(-0.4483333333333333, -0.24166666666666667), vec2(-0.4166666666666667, -0.245), vec2(-0.385, -0.245), vec2(-0.325, -0.23), vec2(-0.2816666666666667, -0.21333333333333335), vec2(-0.25333333333333335, -0.19), vec2(-0.23, -0.16166666666666665), vec2(-0.21, -0.13333333333333333), vec2(-0.195, -0.10166666666666667), vec2(-0.17, -0.075), vec2(-0.14666666666666667, -0.035), vec2(-0.14333333333333334, 0.0), vec2(-0.13166666666666665, 0.065), vec2(-0.13166666666666665, 0.09666666666666666), vec2(-0.13166666666666665, 0.12833333333333333), vec2(-0.13166666666666665, 0.16), vec2(-0.13166666666666665, 0.195), vec2(-0.13833333333333334, 0.22666666666666666), vec2(-0.155, 0.25), vec2(-0.17833333333333334, 0.2633333333333333), vec2(-0.20166666666666666, 0.24666666666666667), vec2(-0.21, 0.21166666666666667), vec2(-0.21, 0.175), vec2(-0.20166666666666666, 0.11666666666666667), vec2(-0.20166666666666666, 0.07166666666666667), vec2(-0.20166666666666666, 0.03666666666666667), vec2(-0.20166666666666666, 0.0), vec2(-0.19833333333333333, -0.035), vec2(-0.18666666666666668, -0.06333333333333334), vec2(-0.175, -0.09), vec2(-0.16333333333333333, -0.13), vec2(-0.155, -0.15833333333333333), vec2(-0.14333333333333334, -0.185), vec2(-0.11833333333333333, -0.205), vec2(-0.10333333333333333, -0.22166666666666668), vec2(-0.075, -0.21333333333333335), vec2(-0.051666666666666666, -0.18166666666666667), vec2(-0.035, -0.16166666666666665), vec2(-0.03166666666666667, -0.13333333333333333), vec2(-0.023333333333333334, -0.10166666666666667), vec2(-0.011666666666666667, -0.055), vec2(-0.011666666666666667, -0.023333333333333334), vec2(-0.011666666666666667, 0.013333333333333334), vec2(-0.011666666666666667, 0.045), vec2(-0.011666666666666667, 0.07666666666666666), vec2(-0.011666666666666667, 0.045), vec2(-0.011666666666666667, 0.008333333333333333), vec2(-0.011666666666666667, -0.02666666666666667), vec2(-0.008333333333333333, -0.06333333333333334), vec2(0.011666666666666667, -0.08666666666666667), vec2(0.035, -0.10166666666666667), vec2(0.06, -0.095), vec2(0.08666666666666667, -0.07), vec2(0.10666666666666667, -0.05), vec2(0.11166666666666666, -0.018333333333333333), vec2(0.115, 0.008333333333333333), vec2(0.11833333333333333, 0.03666666666666667), vec2(0.11833333333333333, 0.06833333333333333), vec2(0.135, 0.03666666666666667), vec2(0.12333333333333334, 0.008333333333333333), vec2(0.11166666666666666, -0.018333333333333333), vec2(0.11166666666666666, -0.05), vec2(0.11166666666666666, -0.08333333333333333), vec2(0.11166666666666666, -0.115), vec2(0.11166666666666666, -0.15333333333333332), vec2(0.11166666666666666, -0.19), vec2(0.11166666666666666, -0.23333333333333334), vec2(0.11166666666666666, -0.2683333333333333), vec2(0.10666666666666667, -0.3), vec2(0.095, -0.3333333333333333), vec2(0.08333333333333333, -0.365), vec2(0.055, -0.39166666666666666), vec2(0.023333333333333334, -0.4033333333333333), vec2(0.0033333333333333335, -0.37166666666666665), vec2(0.0033333333333333335, -0.34), vec2(0.016666666666666666, -0.31666666666666665), vec2(0.04, -0.285), vec2(0.055, -0.25666666666666665), vec2(0.075, -0.21666666666666667), vec2(0.095, -0.17), vec2(0.10666666666666667, -0.13833333333333334), vec2(0.12666666666666668, -0.09), vec2(0.14666666666666667, -0.058333333333333334), vec2(0.17, -0.03166666666666667), vec2(0.195, -0.006666666666666667), vec2(0.215, 0.016666666666666666), vec2(0.235, 0.03666666666666667), vec2(0.26166666666666666, 0.04833333333333333), vec2(0.29333333333333333, 0.04833333333333333), vec2(0.285, 0.051666666666666666), vec2(0.25, 0.045), vec2(0.235, 0.02), vec2(0.23833333333333334, -0.018333333333333333), vec2(0.24666666666666667, -0.058333333333333334), vec2(0.26666666666666666, -0.095), vec2(0.29833333333333334, -0.115), vec2(0.3333333333333333, -0.08333333333333333), vec2(0.35, -0.06333333333333334), vec2(0.3566666666666667, -0.02666666666666667), vec2(0.3566666666666667, 0.008333333333333333), vec2(0.345, 0.03666666666666667), vec2(0.325, 0.051666666666666666), vec2(0.3016666666666667, 0.065), vec2(0.3333333333333333, 0.06), vec2(0.3566666666666667, 0.045), vec2(0.36833333333333335, 0.013333333333333334), vec2(0.36833333333333335, -0.035), vec2(0.36833333333333335, -0.07), vec2(0.37666666666666665, -0.10166666666666667), vec2(0.405, -0.115), vec2(0.4483333333333333, -0.075), vec2(0.46, -0.03166666666666667), vec2(0.4683333333333333, 0.02), vec2(0.4716666666666667, 0.08), vec2(0.4716666666666667, 0.135), vec2(0.4716666666666667, 0.16666666666666666), vec2(0.465, 0.20333333333333334), vec2(0.465, 0.235), vec2(0.445, 0.2633333333333333), vec2(0.4166666666666667, 0.27), vec2(0.4166666666666667, 0.235), vec2(0.42, 0.2), vec2(0.42, 0.16666666666666666), vec2(0.42, 0.12833333333333333), vec2(0.42833333333333334, 0.09166666666666666), vec2(0.445, 0.03666666666666667), vec2(0.45666666666666667, -0.011666666666666667), vec2(0.46, -0.058333333333333334), vec2(0.46, -0.095), vec2(0.46, -0.13), vec2(0.46, -0.16666666666666666), vec2(0.465, -0.15833333333333333), vec2(0.465, -0.12666666666666668), vec2(0.465, -0.09), vec2(0.465, -0.058333333333333334), vec2(0.4683333333333333, -0.02666666666666667), vec2(0.48833333333333334, 0.005), vec2(0.5116666666666667, 0.013333333333333334), vec2(0.535, -0.0033333333333333335), vec2(0.535, -0.03833333333333333), vec2(0.535, -0.07), vec2(0.535, -0.10666666666666667), vec2(0.535, -0.13833333333333334), vec2(0.555, -0.16166666666666665), vec2(0.575, -0.17333333333333334), vec2(0.595, -0.15833333333333333), vec2(0.6116666666666667, -0.13), vec2(0.615, -0.08666666666666667), vec2(0.615, -0.04666666666666667), vec2(0.615, 0.005), vec2(0.615, 0.03666666666666667), vec2(0.615, 0.06833333333333333), vec2(0.615, 0.1), vec2(0.6116666666666667, 0.14), vec2(0.6033333333333334, 0.175), vec2(0.6, 0.20333333333333334), vec2(0.5833333333333334, 0.235), vec2(0.575, 0.25833333333333336), vec2(0.5516666666666666, 0.275), vec2(0.5316666666666666, 0.29), vec2(0.5116666666666667, 0.315), vec2(0.49166666666666664, 0.32666666666666666), vec2(0.4683333333333333, 0.3383333333333333), vec2(0.43666666666666665, 0.3383333333333333), vec2(0.405, 0.3383333333333333), vec2(0.37333333333333335, 0.3383333333333333), vec2(0.3416666666666667, 0.3383333333333333), vec2(0.31, 0.32166666666666666), vec2(0.2816666666666667, 0.30666666666666664), vec2(0.25333333333333335, 0.2833333333333333), vec2(0.235, 0.2633333333333333), vec2(0.21, 0.24333333333333335), vec2(0.19, 0.21833333333333332), vec2(0.16666666666666666, 0.20333333333333334), vec2(0.14666666666666667, 0.18333333333333332), vec2(0.13166666666666665, 0.16333333333333333), vec2(0.12333333333333334, 0.13166666666666665), vec2(0.12333333333333334, 0.13166666666666665), vec2(0.12333333333333334, 0.16333333333333333), vec2(0.11166666666666666, 0.195), vec2(0.08333333333333333, 0.22333333333333333), vec2(0.06333333333333334, 0.24666666666666667), vec2(0.035, 0.27), vec2(0.011666666666666667, 0.2633333333333333), vec2(0.008333333333333333, 0.22333333333333333), vec2(0.0033333333333333335, 0.19166666666666668), vec2(-0.008333333333333333, 0.16), vec2(-0.02, 0.14833333333333334), vec2(-0.02, 0.18), vec2(-0.02, 0.21166666666666667), vec2(-0.035, 0.24333333333333335), vec2(-0.051666666666666666, 0.26666666666666666), vec2(-0.07166666666666667, 0.2866666666666667), vec2(-0.08666666666666667, 0.3016666666666667), vec2(-0.10666666666666667, 0.32166666666666666), vec2(-0.13833333333333334, 0.32166666666666666), vec2(-0.175, 0.32166666666666666), vec2(-0.20666666666666667, 0.315), vec2(-0.23333333333333334, 0.29833333333333334), vec2(-0.26166666666666666, 0.2833333333333333), vec2(-0.29333333333333333, 0.25833333333333336), vec2(-0.325, 0.22666666666666666), vec2(-0.345, 0.19166666666666668), vec2(-0.3566666666666667, 0.155));
const float TAU = 6.28318530718;

float sdLine(in vec2 p, in vec2 a, in vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Visualization of the path as it is drawn
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    fragColor = texelFetch(iChannel1, ivec2(fragCoord), 0);

    float unit = 2.0 / iResolution.y;

    float dt = TAU / float(path.length());
    float t = float(iFrame) * dt;
    float tPrev = t - dt;

    // Calculate the previous and current positions
    vec2 prevPos = vec2(0.0);
    vec2 pos = vec2(0.0);
    for (int n=0; n < path.length(); n++) {
        vec3 epicycle = texelFetch(iChannel0, ivec2(n, 0), 0).xyz;

        float aPrev = tPrev * epicycle.y + epicycle.z;
        prevPos += vec2(cos(aPrev), sin(aPrev)) * epicycle.x;

        float a = t * epicycle.y + epicycle.z;
        pos += vec2(cos(a), sin(a)) * epicycle.x;
    }

    // Draw a new segment from the previous position to the current position
    fragColor.rgb = mix(fragColor.rgb, vec3(0.0, 1.0, 0.0), smoothstep(unit, 0.0, sdLine(uv, prevPos, pos)));

    // Clear the buffer after the path has been completed
    if (iFrame % path.length() == 0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}