

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define timescale .5
#define scaleX 1.5
#define scaleY 0.3

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 col = vec3(0,0,0);
    vec2 uv = fragCoord/iResolution.xy;
    
    float dist = texture(iChannel0,vec2(uv.x*scaleX - iTime*1.1*timescale,uv.y*scaleY - iTime*1.8*timescale)).r;
    
    float tex = texture(iChannel0,vec2(uv.x*scaleX + dist*0.2,uv.y*scaleY - iTime*1.5*timescale)).r;
    
    tex += uv.y*.5;
    float fire = pow(1.-tex,2.3);
    fire -= (1.-(abs(uv.x-.5)*2.))*.5;
    
    col += fire *5.* mix(vec3(.0,.2,1),vec3(1,.21,0),uv.x);

    fragColor = vec4(col,1.0);
}