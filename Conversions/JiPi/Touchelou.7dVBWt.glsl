

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
    vec2 uv = fragCoord/iResolution.xy;
    uv /= vec2(iResolution.y / iResolution.x, 1);
    
    // bass detection
    // the sound texture is 512x2
    int tx = int(uv.x*512.0);
    

	// crude beat detection
    float bass = 0.0;
    int samples = 5;
    for(int i = 0 ; i < samples ; ++i) {
        bass += texelFetch( iChannel2, ivec2(i,0), 0 ).x;
    }
    bass /= float(samples);
    float beat = smoothstep(0.8, 1.0, bass);
    

    vec2 uv2 = uv + vec2(iTime / 50.0, iTime / 100.0);

    float intens = texture(iChannel1, uv2).r;
 
    float anim = iTime / 10.0;
    
 
    vec2 def = vec2(sin(intens + anim), cos(intens + anim / 2.0));
    def -= 0.2;

    vec4 tex = texture(iChannel0, uv + def);
     

    tex *= vec4(0.2, 0.6, 2.0, 1.0);
    tex *= vec4(1.0, 1.0, beat + 0.5, 1.0);


    fragColor = vec4(tex.rgb, 1.0);
}