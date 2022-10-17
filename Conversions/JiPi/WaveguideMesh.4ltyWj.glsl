

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float v = texture(iChannel0, uv).x;
    fragColor = vec4(v);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 bufB( in vec2 coord )
{
    vec2 uv = coord/iResolution.xy;
    return texture(iChannel1, uv)*2.0-1.0;
}
void mainImage( out vec4 fragColor, in vec2 c )
{
    vec2 uv = c/iResolution.xy;
    vec2 e = vec2(1, 0);
    float v = 0.5*(bufB(c-e.xy).x + bufB(c-e.yx).y + bufB(c+e.xy).z + bufB(c+e.yx).w);
    fragColor = vec4(v);
    if (iMouse.z > 0.0)
    	fragColor += vec4(sin(max(0.0,25.0-0.5*distance(c, iMouse.xy))));
    fragColor = fragColor*0.5+0.5;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
const float DECAY = 0.999;

float bufA( in vec2 coord )
{
    vec2 uv = coord/iResolution.xy;
    return texture(iChannel0, uv).a*2.0-1.0;
}
vec4 bufB( in vec2 coord )
{
    vec2 uv = coord/iResolution.xy;
    return texture(iChannel1, uv)*2.0-1.0;
}
void mainImage( out vec4 fragColor, in vec2 c )
{
    vec2 uv = c/iResolution.xy;
    vec2 e = vec2(1, 0);
    float p = bufA(c);
    fragColor = vec4(
    	p-bufB(c+e.xy).z,
        p-bufB(c+e.yx).w,
        p-bufB(c-e.xy).x,
        p-bufB(c-e.yx).y
    )*DECAY*0.5+0.5;
}