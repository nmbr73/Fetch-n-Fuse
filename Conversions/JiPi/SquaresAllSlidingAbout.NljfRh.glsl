

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    fragColor = texture(iChannel0, fragCoord/iResolution.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define SQUARES 5.0
#define WIGGLE 2.0

vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    vec2 a = hash22(floor(uv*SQUARES) + floor(WIGGLE*iTime))-.5;
    a *= 1.4;
    
    if(iTime < 2.)
        fragColor = texture(iChannel1, uv);
    else
    {
        fragColor = texture(iChannel0, uv+a/iResolution.xy);
    }
}