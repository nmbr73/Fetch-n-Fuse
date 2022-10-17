

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
    //col = (col - 0.5)* vec3(texture(iChannel1, uv)) -0.8;
    col = col * vec3(0.5,0.3,0.3);
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float calc(float colorSum, int range, float preColor){
    float d = float (range*range - 1);
    float diff = colorSum/d - preColor;
    
    float result ;
    
    if(diff > 0.){
        if(colorSum/d > 4. ){
            result += -0.01; 
        }else{
            result = preColor + 0.01;
        }
        
    }else{
        result = 0.;
    }
    
    return result;
}

vec3 kernel(ivec2 fragCoord){
    vec3 colorSum = vec3(0.);
    vec3 preColor = texelFetch(iChannel1, fragCoord, 0).rgb;
    int range = 10;
    for(int i = -range; i < range+1 ; i++ ){
        for(int j = -range; j < range+1 ; j++ ){
            if(i != 0 || j != 0){
                //colorSum += texelFetch(iChannel1, fragCoord + ivec2(i,j), 0).rgb;
                colorSum += texture(iChannel1, (vec2(fragCoord + ivec2(i,j))+0.5)/iResolution.xy).rgb;
            }  
        }
    }
    
    vec3 c = vec3(calc(colorSum.r, range, preColor.r),
    calc(colorSum.g, range, preColor.g),
    calc(colorSum.b, range, preColor.b));
    
    return c;
}

vec3 getColor(ivec2 fragCoord){
    vec3 num = kernel(fragCoord);
    return num;
}


vec2 N22(vec2 uv){
    vec3 a = fract(uv.xyx * vec3(123.34,234.34,345.65));
    a += dot(a, a+34.45);
    return fract(vec2(a.x*a.y, a.y*a.z));
}

vec3 circle(vec2 uv, vec2 pos, float size) {
    uv.x = uv.x*iResolution.x/iResolution.y;
    uv = uv - pos;
    vec3 col = vec3(length(10./size*uv));
    vec3 result = vec3(smoothstep(.98,1.0,col.x)); 
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
  
    vec3 col = vec3(0.);
    vec2 pos = vec2(1.);
    

    /**/
    if(iFrame < 10){
        col = vec3(texture(iChannel0, uv));
        //col = circle(uv, pos, 0.1);
        for(int i = 0; i < 10; i++ ){
            pos = vec2(float(i)+1.);
            //col += 1.* (1. - circle(uv, N22(pos), 1.));
        }
    }else{
        col = getColor(ivec2(fragCoord));
    }
    /**/
    
    fragColor = vec4(col,1);
}