

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
	fragColor = max(texture(iChannel0,uv),texture(iChannel1,uv+0.002));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 px = 4.0/vec2(640.0,360.0);
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 tex = pow(texture(iChannel0,uv)*1.3,vec4(1.8));
    float d = 1.0-smoothstep(0.0,0.08,length(tex));
    //float d = abs(tex.g - newG);
    //tex.g = newG * 0.9;
    if (d > 0.0)
    {
        //px*= sin(iTime+uv.yx*3.0)*.35;
        uv -= 0.5*px;
        vec4 tex2 = texture(iChannel1,uv);
        uv += px;
        tex2 += texture(iChannel1,uv);
        uv.x -= px.x -.018 *sin(iTime*4.1+tex2.r);
        uv.y += px.y +.015 * cos(iTime*4.1+tex2.g);
        tex2 += texture(iChannel1,uv);
        uv.y -= px.y;
        tex2 += texture(iChannel1,uv);
        tex2 /= 4.013;
        tex2 = clamp(tex2*1.02-0.012,0.0,1.0);
        tex = max(clamp(tex*(1.0-d),0.0,1.0),mix(tex,tex2,smoothstep(-1.3,0.23,d)));
     }
        
	fragColor = tex;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 px = 2.5 / vec2(640.0,360.0);
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 tx = texture(iChannel1,uv);
    float dist = distance(tx,texture(iChannel1,uv+px));
    px.y *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    px.x *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    px.y *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    uv *= mat2(0.999,0.001,-0.001,0.999);
	fragColor = texture(iChannel0,uv*0.995+0.0025)*vec4(0.93,0.91,0.0,0.0)+
        vec4(smoothstep(0.05,1.3,dist),smoothstep(0.1,2.8,dist),0.0,1.0)*.245;
}