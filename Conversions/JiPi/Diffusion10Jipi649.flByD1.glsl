

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define S(a,b,t) smoothstep(a,b,t)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    //uv.x *= iResolution.x/iResolution.y;
    vec3 col = vec3(texture(iChannel0, uv));
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec3 kernel(ivec2 fragCoord, vec2 R){
    vec3 num = vec3(0.);
    vec3 preNum = texture(iChannel1, (vec2(fragCoord)+0.5)/R).rgb;
    for(int i = -1; i < 2 ; i++ ){
        for(int j = -1; j < 2 ; j++ ){
            if(i != 0 || j != 0){
                num += texture(iChannel1, (vec2(fragCoord + ivec2(i,j))+0.5)/R).rgb;
            }  
        }
    }
    
    vec3 diff = num/8.0 - preNum;
    if(diff.r > 0.){
        preNum = preNum + diff*1.;
    }else{
        preNum = preNum ;
    }
    
    return preNum;
}

vec3 getColor(ivec2 fragCoord, vec2 R){
    vec3 num = kernel(fragCoord, R);
    return num;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
  
    vec3 col = vec3(texture(iChannel0, uv));
    
    /**/
    if(iFrame < 10){
        col = vec3(texture(iChannel0, uv));
    }else{
        col = getColor(ivec2(fragCoord),iResolution.xy);
    }
    /**/
    fragColor = vec4(col.r, col.r, col.r,1);
}