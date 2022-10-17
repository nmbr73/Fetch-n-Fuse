

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// ---------------------------------------------------------------------------------------
//	Created by fenix in 2022
//	License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// I just watched this video and was inspired to create this shader.
//
//     https://www.youtube.com/watch?v=3H79ZcBuw4M
//
// I have no idea how this works, but in my defence neither does the guy who made the video
// (or so he says). Neural Cellular Atomaton is a pretty fancy name for essentially a 
// continuous (non-binary) CA. You can experiment with this automata (and others) at:
//
//     https://neuralpatterns.io/
//
// Other than porting to shadertoy, all I did was fancy up the rendering a little bit.
//
// Buffer A computes the neural cellular atomaton
// Buffer B performs temporal blur of buffer A
// Image computes gradient, applies lighting and color
//
// ---------------------------------------------------------------------------------------

vec2 grad(vec2 fragCoord, float d)
{
    vec2 delta = vec2(d, 0);
    return vec2(texture(iChannel0, fragCoord/iResolution.xy + delta.xy).x -
        texture(iChannel0, fragCoord/iResolution.xy - delta.xy).x,
        texture(iChannel0, fragCoord/iResolution.xy + delta.yx).x -
        texture(iChannel0, fragCoord/iResolution.xy - delta.yx).x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 g = grad(fragCoord, 1./iResolution.y);
    vec3 norm = normalize(vec3(g, 1.));
    float value = texelFetch(iChannel0, ivec2(fragCoord), 0).r;
    vec3 color = mix(vec3(0.3,0.3,.5), vec3(1, .2, .2), smoothstep(0., 0.1, value));
    fragColor = vec4(color * dot(norm, normalize(vec3(1,-1,1))), 1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec3 kernel[] = vec3[]( vec3( 0.68, -0.9,   0.68),
                        vec3(-0.9,  -0.66, -0.9),
                        vec3( 0.68, -0.9,   0.68) );
                        
float convolve(vec2 fragCoord)
{
    float a = 0.;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            a += texelFetch(iChannel0, ivec2(fragCoord) + ivec2(i - 1, j - 1), 0).x * kernel[i][j];
        }
    }
    return a;
}

// inverse gaussian
float activation(float x)
{
    return -1. / pow(2., (0.6 * pow(x, 2.))) + 1.;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 state = texelFetch(iChannel0, ivec2(0), 0);
    
    if (ivec2(fragCoord) == ivec2(0))
    {
        if (iFrame == 0 || abs(state.x) != iResolution.x * iResolution.y || keyDown(KEY_SPACE))
        {
            state.x = -iResolution.x * iResolution.y;
        }
        else
        {
            state.x = abs(state.x);
        }
        
        fragColor = state;
        return;
    }
    
    if (iFrame == 0 || state.x < 0. ||
        (iMouse.z > 0. && distance(iMouse.xy, fragCoord) < 0.1*iResolution.y))
    {
        fragColor = vec4(hash(int(fragCoord.x * fragCoord.y + iDate.x) + iFrame));
        return;
    }
        
    fragColor = vec4(activation(convolve(fragCoord)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//hashing noise by IQ
float hash( int k )
{
    uint n = uint(k);
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0;
}

#define keyDown(ascii)    ( texelFetch(iChannel3,ivec2(ascii,0),0).x > 0.)

#define KEY_SPACE 32

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0) * 0.1 + texelFetch(iChannel1, ivec2(fragCoord), 0) * 0.9;
    
    if (iFrame == 0) fragColor = vec4(0);
}