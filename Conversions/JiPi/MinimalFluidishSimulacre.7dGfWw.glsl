

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Minimal Fluidish Simulacre

// found by accident that shifting pixels
// on a height map with the slope direction
// calculated by sampling neighbors at random long range
// produces somehow turbulent fluid movement

// i'm amazed that it produces such organic patterns
// when there is no perlin noise, no gyroid, no force fields
// just white grainy noise and slope movement

// this accident is dedicated to Cornus Ammonis
// which works inspired me in so many ways
// and because this shader looks like a drunken version of his work

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

#define T(uv) texture(iChannel0,uv).a

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // the salt of life
    float noise = hash13(vec3(fragCoord, iFrame));
    
    // coordinates
    vec2 uv = fragCoord/iResolution.xy;

    // random spawn
    float height = clamp(.001/noise,0.,1.);
    
    // mouse interaction
    if (iMouse.z > 0.)
        height += clamp(.02/length(uv-iMouse.xy/iResolution.xy), 0., 1.);
    
    // move uv toward slope direction
    vec2 e = vec2(.2*noise,0);
    vec2 normal = normalize(vec2(T(uv+e.xy)-T(uv-e.xy),T(uv+e.yx)-T(uv-e.yx)));
    uv += 5. * normal * noise / iResolution.xy;

    // accumulate and fade away
    height = max(height, texture(iChannel0, uv).a - .005*noise);
    
    // lighting
    e = vec2(2./iResolution.y, 0);
    normal = normalize(vec2(T(uv+e.xy)-T(uv-e.xy),T(uv+e.yx)-T(uv-e.yx)));
    float light = dot(normal, vec2(0,-1))*.5+.5;
    
    fragColor = vec4(vec3(light*height), height);
}