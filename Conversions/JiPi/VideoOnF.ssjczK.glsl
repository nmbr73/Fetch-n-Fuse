

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
    vec2 px = 3.0/vec2(640.0,360.0);
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 tex = texture(iChannel0,uv);
    float newG = min(tex.g,max(tex.r,tex.b));
    float d = abs(tex.g - newG);
    tex.g = newG * 0.9;
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
        tex = max(clamp(tex*(1.0-d),0.0,1.0),mix(tex,tex2,smoothstep(-0.3,0.23,d)));
     }
        
	fragColor = tex;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 px = 1.5 / vec2(640.0,360.0);
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 tx = texture(iChannel1,uv);
    float dist = distance(tx,texture(iChannel1,uv+px));
    px.y *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    px.x *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    px.y *= -1.0;
    dist += distance(tx,texture(iChannel1,uv+px));
    uv *= mat2(0.99,0.01,-0.001,0.99);
	fragColor = texture(iChannel0,uv+0.002)*vec4(0.91,0.847,0.0,0.0)+
        vec4(smoothstep(0.3,0.8,dist),smoothstep(0.3,1.4,dist),0.0,1.0)*.175;
}