

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec2 aspect = vec2(iResolution.x/iResolution.y, 1.);
    vec2 uvQuad = uv*aspect;
    
    vec4 bufA = texture(iChannel0, uv);
    
    vec4 col = bufA;
    col.xy *= (uv.xy+(1.-uv.xy));
    
    if(iFrame == 0) {
        col = vec4(.0,.0,.0,1.);
    }
    
    fragColor = vec4(col.xy, .0, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
    vec2 uv = fragCoord/iResolution.xy;
    vec2 aspect = vec2(iResolution.x/iResolution.y, 1.);
    vec2 uvQuad = uv*aspect;
    
    vec2 mP = uvQuad - iMouse.xy/iResolution.xy*aspect;

    float d = 1.-length(mP);    
    
    vec4 bufA = vec4(texture(iChannel0, uv).x*0.999,texture(iChannel0, uv).y*0.997,texture(iChannel0, uv).z*0.998,texture(iChannel0, uv).w);
    vec2 mPN = bufA.zw;
    vec2 vel = min(max(abs(mPN - mP), vec2(0.001)), vec2(0.05));
    
    d = smoothstep(0.85,1.3,d+length(vel))/0.4;
    vec2 dot = vec2(d);

    dot = max(dot, bufA.xy);

    
    vec4 col = vec4(dot.x, dot.y, mP.x, mP.y);
    
    if(iFrame == 0) {
        col = vec4(.0,.0, mP.x, mP.y);
    }
    
    fragColor = vec4(col);
}