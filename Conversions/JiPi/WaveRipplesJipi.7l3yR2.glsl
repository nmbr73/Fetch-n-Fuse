

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    //Tweakable parameters
    float waveStrength = 0.02;
    float frequency = 30.0;
    float waveSpeed = 5.0;
    vec4 sunlightColor = vec4(1.0,0.91,0.75, 1.0);
    float sunlightStrength = 5.0;
    //
    
    vec2 tapPoint = vec2(iMouse.x/iResolution.x,iMouse.y/iResolution.y);
	vec2 uv = fragCoord.xy / iResolution.xy;
    float modifiedTime = iTime * waveSpeed;
    float aspectRatio = iResolution.x/iResolution.y;
    vec2 distVec = uv - tapPoint;
    distVec.x *= aspectRatio;
    float distance = length(distVec);
    vec2 newTexCoord = uv;
    
    float multiplier = (distance < 1.0) ? ((distance-1.0)*(distance-1.0)) : 0.0;
    float addend = (sin(frequency*distance-modifiedTime)+1.0) * waveStrength * multiplier;
    newTexCoord += addend;    
    
    vec4 colorToAdd = sunlightColor * sunlightStrength * addend;
    
	fragColor = texture(iChannel0, newTexCoord) + colorToAdd;
}