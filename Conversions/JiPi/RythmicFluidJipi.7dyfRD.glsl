

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 out_color, in vec2 coordinates )
{
    vec2 uv = coordinates.xy/iResolution.xy;
    float v = texture(iChannel0, uv).r * 1.5;
        
    vec3 color = pow(vec3(cos(v), tan(v), sin(v)) * 0.5 + 0.5, vec3(0.5));
    vec3 e = vec3(vec2(1.0) / iResolution.xy, 0.0);
    vec3 grad = normalize(vec3(
        texture(iChannel0, uv + e.xz).x - texture(iChannel0, uv - e.xz).x, 
        texture(iChannel0, uv + e.zy).x - texture(iChannel0, uv - e.zy).x, 1.0));
    vec3 light = vec3(0.26, -0.32, 0.91);
    float diffuse = dot(grad, light);
    float spec = pow(max(0.0, -reflect(light, grad).z), 32.0);
    
    out_color.rgb = (color * diffuse) + spec;
    out_color.a = 1.0;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define FFT(f) texture(iChannel1, vec2(f, 0.0)).x
#define PIXEL(x, y) texture(iChannel0, uv + vec2(x, y) / iResolution.xy).r

void mainImage(out vec4 out_color, in vec2 coordinates)
{    
    vec2 uv = coordinates.xy / iResolution.xy;
    
    float v = PIXEL(0.0, 0.0);
    v = PIXEL(
        sin(PIXEL(v, 0.0)  - PIXEL(-v, 0.0) + 3.1415) * v * 0.4, 
        cos(PIXEL(0.0, -v) - PIXEL(0.0 , v) - 1.57) * v * 0.4
    );
    v += pow(FFT(pow(v*0.1, 1.5) * 0.25) * 1.5, 3.0);
    v -= pow(length(texture(iChannel2, uv)) + 0.05, 3.0) * 0.08;
    v *= 0.925 + FFT(v)*0.1;
    
    out_color.r = v;
}