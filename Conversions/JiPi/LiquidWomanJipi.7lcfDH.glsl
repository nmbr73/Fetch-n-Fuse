

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    
	vec4 girl = texture(iChannel2, vec2(uv.x,1.0-uv.y));

	vec3 ray = vec3(uv.x - 0.5 + 0.7 * girl.x + 0.1 * girl.z,uv.y - 0.5 + 0.7 * girl.y + 0.1 * girl.z,1.0);

	vec4 cube0 = texture(iChannel0, ray);
	vec4 cube1 = texture(iChannel1, ray);
	
	vec4 result = cube0 + cube1 + 0.1 * girl;
	
	fragColor = result;
}
