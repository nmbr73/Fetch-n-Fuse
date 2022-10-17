

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
    vec3 buffer = texture(iChannel0,uv).xyz;
    fragColor = vec4(vec3(buffer.x),1.);
    //fragColor = vec4(buffer,1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Andrin Rehmann
// 2020
// andrinrehmann.ch
// andrinrehmann@gmail.com

#define EXPANSION 2.5

//note: uniformly distributed, normalized rand, [0;1[
float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

float rand( vec2 uv )
{
	float t = fract( iTime );
	return nrand( uv + 0.07*t );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float du = 1. / iResolution.x;
    float dv = 1. / iResolution.y;
    
	vec2 uv = fragCoord/iResolution.xy;
    
    // Cell
	float y = texture(iChannel0,uv).x;
    // food
    float f = texture(iChannel0,uv).z;
    
    // sum of surrounding y's
    float s = 1./16. * texture(iChannel0,uv + vec2(-du,-dv)).x +
              3./16.  * texture(iChannel0,uv + vec2(-du,0)).x + 
              1./16. * texture(iChannel0,uv + vec2(-du,dv)).x + 
              3./16.  * texture(iChannel0,uv + vec2(0,-dv)).x + 
              3./16.  * texture(iChannel0,uv + vec2(0,dv)).x +
              1./16. * texture(iChannel0,uv + vec2(du,-dv)).x +
              3./16.  * texture(iChannel0,uv + vec2(du,0)).x + 
              1./16. * texture(iChannel0,uv + vec2(du,dv)).x;

    
    if (s > 0.5 && f > 0.5){
        y += 0.1;
    }
    
    if (y > 0.5){
        f -= 0.01;
    }
    
    if (f < 0.5){
        y -= y * 0.1;
    
    }
    
    f += 0.002;
    
    
    if (distance(iMouse.xy, fragCoord) < 15. && iMouse.z > 0.){
        f = rand(uv+vec2(0.2));
        y = rand(uv);
    }
    
    // Init
    if (iFrame < 1){
        y = rand(uv);
        f = rand(uv+vec2(0.2));
    }
    
    fragColor = vec4(y, 0, f, 1.);
}