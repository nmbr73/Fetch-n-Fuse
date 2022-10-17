

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define NBR_WAVE 100.
#define HEIGHT_WAVE 100.
#define PI 3.1415

mat2 rotate2d(float angle)
{
 	return mat2(cos(angle),-sin(angle),sin(angle),cos(angle));   
}

vec2 toPolar(vec2 uv)
{
    float distance = length(uv);
    float angle = atan(uv.y,uv.x);
    return vec2(angle/PI*2.,distance);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    float ratio = iResolution.y/iResolution.x;
    
    uv.x = (uv.x-0.25) / ratio;
    //uv = toPolar(uv-0.5);

    float _iTime = -iTime;

    // Time varying pixel color
    uv.y += (sin((toPolar(uv-0.5)).y*NBR_WAVE+ _iTime*20.0)/HEIGHT_WAVE);
    uv.x += (cos((toPolar(uv-0.5)).y*NBR_WAVE+ _iTime*20.0)/HEIGHT_WAVE);
    uv.y+=0.01; //zoomer sur l'image afin de masquer la répétition
    uv.y*=0.97;
    
    //si on joue sur les valeur x et y d'uv on peu crée d'autre type de déformation soit vertical soit horizontal ou les deux
    
    vec3 col = texture(iChannel0,uv).xyz;

    // Output to screen
    fragColor = vec4(col,1.0);
}