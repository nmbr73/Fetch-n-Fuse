

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
This attempts to replicate the transition seen in Oddworld - Abe's Oddysee here:
https://www.youtube.com/watch?v=SYL6nxUkuOo&feature=youtu.be&t=68
*/

// Tweakable parameters
// I'm not sure they are well named for what they do.
// I'd like it if period was calculated from the others such that the effect always loops cleanly.
float freq = 8.0;
float period = 8.0;
float speed = 2.0;
float fade = 4.0;
float displacement = 0.2;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 R = iResolution.xy,
         U = ((2. * fragCoord.xy) - R) / min(R.x, R.y),
         T = fragCoord / R.y;
    float D = length(U);

    float frame_time = mod(iTime * speed, period);
    float pixel_time = max(0.0, frame_time - D);

    float wave_height = (cos(pixel_time * freq) + 1.0) / 2.0;
    float wave_scale = (1.0 - min(1.0, pixel_time / fade));
    float frac = wave_height * wave_scale;
    if (mod(iTime * speed, period * 2.0) > period)
    {
        frac = 1. - frac;
    }

    vec2 tc = T + ((U / D) * -((sin(pixel_time * freq) / fade) * wave_scale) * displacement);
    
    fragColor = mix(
        texture(iChannel0, tc),
        texture(iChannel1, tc),
        frac);
}