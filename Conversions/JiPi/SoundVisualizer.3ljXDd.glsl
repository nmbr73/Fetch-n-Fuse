

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0 International License (CC BY 4.0).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0/>.
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 bA = texelFetch(iChannel0, ivec2(fragCoord), 0).rgb;
    vec3 bB = texelFetch(iChannel1, ivec2(fragCoord), 0).rgb;
   
    vec3 col_rgb = bA + bB;

    col_rgb = col_rgb*exp2(3.5);
    
    fragColor = vec4(pow(col_rgb, vec3(1./2.2)), 1.);
} 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0 International License (CC BY 4.0).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0/>.
*/

vec3 hue2rgb(in float h) {
    vec3 k = mod(vec3(5., 3., 1.) + vec3(h*360./60.), vec3(6.));
    return vec3(1.) - clamp(min(k, vec3(4.) - k), vec3(0.), vec3(1.));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    float aspect = iResolution.x / iResolution.y;
    float blurr = 0.3;
    float sharpen = 1.7;
    
    vec2 maxWindow = vec2(3., 3./aspect);
    uv = mix(-maxWindow, maxWindow, uv);

    float r = dot(uv, uv);
    float theta = atan(uv.y, uv.x) + 3.14;
    
    float t = abs(2.*theta / (2.*3.14) - 1.);

    float signal = 2.0*texture(iChannel0,vec2(t,.75)).x;
    float ampl = 2.0*texture(iChannel0,vec2(0.8,.25)).x;
    
    float v = 1. - pow(smoothstep(0., blurr, abs(r - signal)), 0.01);
    float hue = pow(fract(abs(sin(theta/2.) * ampl)), sharpen);
    	
    fragColor = v * vec4(hue2rgb(fract(hue + iTime/10.)), 1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/**
 Sound visualizer (c) by Alban Fichet

 Sound visualizer is licensed under a
 Creative Commons Attribution 4.0 International License (CC BY 4.0).

 You should have received a copy of the license along with this
 work. If not, see <https://creativecommons.org/licenses/by/4.0/>.
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    vec2 center = vec2(.5 + 0.15*sin(iTime));
    float zoom = 1.02;
    
    vec4 prevParams = texture(iChannel0, (uv - center)/zoom + center);
    vec4 bB = texture(iChannel1, uv);

    fragColor = mix(prevParams, bB, 0.1) ;
}