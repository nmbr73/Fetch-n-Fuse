

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
    vec3 col_add=vec3(0.0, 0.0, 0.0), col_scale=vec3(2.0,2.0,1.0);
    
	fragColor = vec4(abs(texture(iChannel0, uv).xzy)*col_scale+col_add, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float diffuse=5.5;// 3.5 also works well. 11.5 makes crazy colour swirls.

const float accel=0.1;
const float max_speed=0.3;

const float dissipate=0.001;
const float springiness=0.01;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // this makes makes it a bit more varied.
    //diffuse=sin(iTime*0.1)*12.0;
    //dissipate=sin(iTime)*0.01;
    
    vec2 uv = (fragCoord.xy) / iResolution.xy;
    vec2 delta=vec2(diffuse)/iResolution.xy;
    
    vec4 a_=texture(iChannel0, uv-delta);
    vec4 b_=texture(iChannel0, uv+vec2(delta.x, -delta.y));
    vec4 c_=texture(iChannel0, uv+vec2(-delta.x, delta.y));
    vec4 d_=texture(iChannel0, uv+delta);
    
    vec4 v=0.25*(a_+b_+c_+d_);
    uv-=delta*clamp(v.xy, vec2(-max_speed), vec2(max_speed));
    
    // propagate (backwards of what I actually need)
    v=texture(iChannel0, uv);
    
    vec4 a=texture(iChannel0, uv-delta);
    vec4 b=texture(iChannel0, uv+vec2(delta.x, -delta.y));
    vec4 c=texture(iChannel0, uv+vec2(-delta.x, delta.y));
    vec4 d=texture(iChannel0, uv+delta);
    vec4 avg=0.25*(a+b+c+d);
    v=mix(v,avg,dissipate);
    //v.w=avg.w;
    
    vec4 ddx=(b+d)-(a+c);
    vec4 ddy=(c+d)-(a+b);
    
        
    // x,y : velocity , z: 'pressure' (but not quite), w: buoyancy
    
    float divergence=ddx.x+ddy.y;
    
    v.xy-=vec2(ddx.z, ddy.z)*accel;
    v.z-=divergence*springiness;
    
    v.xy+=(v.w)*vec2(0.0, 1.0);
    
    float t=iTime*0.2+3.0;
    
    vec2 mousePos=iMouse.xy;
    if(mousePos.x<3.0)mousePos.xy=vec2(iResolution.xy*0.31);
    
    
    float mouse=length(mousePos.xy-fragCoord.xy);
    v+=vec4(0,0, 0, 0.001)*max(1.0-0.03*mouse, 0.0)*dot(mousePos.xy-fragCoord.xy, vec2(sin(t),cos(t)));   
    
    v.w*=0.99;

    if(texelFetch( iChannel1, ivec2(32,0), 0 ).x>0.5){
        fragColor=vec4(0.0);
    }else{    
   		fragColor=clamp(v*0.998, vec4(-1), vec4(1));        
        
    }    
}