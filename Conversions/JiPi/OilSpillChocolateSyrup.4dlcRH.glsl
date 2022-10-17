

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//light from https://www.shadertoy.com/view/MsGSRd

float colormap_red(float x) {
    return 1.61361058036781E+00 * x - 1.55391688559828E+02;
}

float colormap_green(float x) {
    return 9.99817607003891E-01 * x + 1.01544260700389E+00;
}

float colormap_blue(float x) {
    return 3.44167852062589E+00 * x - 6.19885917496444E+02;
}

vec4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}

float getVal(vec2 uv)
{
    return length(texture(iChannel0,uv).xyz);
}
    
vec2 getGrad(vec2 uv,float delta)
{
    vec2 d=vec2(delta,0);
    return vec2(
        getVal(uv+d.xy)-getVal(uv-d.xy),
        getVal(uv+d.yx)-getVal(uv-d.yx)
    )/delta;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 res = iResolution.xy;
    vec2 uv = fragCoord.xy / res;
    
    vec3 n = vec3(getGrad(uv,1.0/iResolution.y),350.0);
    n=normalize(n);
    vec3 light = normalize(vec3(0.01,0.75,2.25));
    
    //interactive light
    //vec3 light = normalize(vec3(iMouse.xy / iResolution.y,1.25) - vec3(fragCoord.xy / iResolution.y,0.0));

    float diff=clamp(dot(n,light),0.5,1.0);
    float spec=clamp(dot(reflect(light,n),vec3(0,0,-1)),0.0,1.0);
    spec=pow(spec,64.0)*1.5;
    
    float osc = sin(iTime*0.25)*0.5 + 0.5;
    vec4 fb = texture(iChannel0, uv);
    float avg = dot(fb.rgb, vec3(1.0))*0.333333;
    
    fb = colormap(fb.r);
    fb = fb.grba;
    fb.rgb += vec3(0.05,-0.05,0.0);
    //fb*=0.95;
	
    fragColor = fb * diff + spec;

}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float brushSize = 25.0;

float diff(vec2 t, vec2 b){
 	vec3 t1 = texture(iChannel0, t).xyz;
    vec3 t2 = texture(iChannel0, b).xyz;
    return dot(t1, vec3(1.0)) * dot(t2, vec3(1.0));
}

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 res = iResolution.xy;
    vec2 uv = fragCoord.xy / res;
    
    vec2 offs = vec2(0.4 / res);
        
    vec2 top = uv + vec2(0.0,-offs.y);
    vec2 bottom = uv + vec2(0.0,offs.y);
    vec2 left = uv + vec2(-offs.x, 0.0);
    vec2 right = uv + vec2(offs.x, 0.0);
    
    float gradient1 = diff(top, bottom);
    float gradient2 = diff(left, right);
    
    vec4 fc = vec4( ((length(gradient1) + length(gradient2))*0.07));
    
    if(iFrame < 15 || reset() ){
        fragColor = texture(iChannel1, uv);
    } else {
        vec4 old = texture(iChannel0, uv);
        //fc.a *= 0.38;
        fc.rgba = (fc.rgba * fc.a) + (old*(1.0 - fc.a));
        //fc += 0.05;
    	fragColor = clamp(fc, vec4(0.0), vec4(1.0));
        
    }
    
    if(iMouse.z >= 0.0){        
        fragColor += vec4(1.0 - clamp(length(iMouse.xy - fragCoord) - brushSize, 0.0,1.0));
    }
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//blur 1
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 res = iResolution.xy;
    vec2 uv = fragCoord.xy / res;
    vec2 step = vec2(1.750 / res);
    vec4 u = texture(iChannel0, uv + vec2(0.0,-step.y));
    vec4 d = texture(iChannel0, uv + vec2(0.0, step.y));
    vec4 l = texture(iChannel0, uv + vec2(-step.x,0.0));
    vec4 r = texture(iChannel0, uv + vec2(step.x,0.0));
    vec4 c = texture(iChannel0, uv);
    
    vec4 o = (u+d+l+r+c)*0.2;
    fragColor = o;//
    fragColor.a = texture(iChannel0, uv).a;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//blur 2
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 res = iResolution.xy;
    vec2 uv = fragCoord.xy / res;
    vec2 step = vec2(1.00 / res);
    vec4 u = texture(iChannel0, uv + vec2(0.0,-step.y));
    vec4 d = texture(iChannel0, uv + vec2(0.0, step.y));
    vec4 l = texture(iChannel0, uv + vec2(-step.x,0.0));
    vec4 r = texture(iChannel0, uv + vec2(step.x,0.0));
    vec4 o = (u+d+l+r)*0.25;
    fragColor = o;//
    fragColor.a = texture(iChannel0, uv).a;
}