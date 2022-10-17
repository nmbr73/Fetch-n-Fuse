

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec4 color1 = vec4(1.0,0.25,0.15,1.0);
vec4 color2 = vec4(0.5,0.1,0.1,1.0);
vec4 color3 = vec4(0.0,0.0,0.0,1.0);
float multiplier = 1.0;
float midPosition = 0.5;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 c = texture(iChannel0, uv);
    uv.x *= iResolution.x/iResolution.y;
    
    vec4 tex = texture(iChannel1, uv);
    
    float l = clamp(((c.x+c.y+c.z)/3.0)*multiplier,0.0,1.0);
    
    vec4 res1 = mix(color1, color2, smoothstep(0.0,midPosition,l));
    vec4 res2 = mix(res1, color3, smoothstep(midPosition,1.0,l));
    
    fragColor = res2*(0.9 + tex*0.1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   	vec2 uv = fragCoord.xy / iResolution.xy;
    
    vec4 cam = texture(iChannel1, uv);
    float keying = smoothstep(0.0,1.0,distance(cam.xyz, vec3(13.0/255.0,163.0/255.0,37.0/255.0))*1.0);
    
    fragColor = vec4(clamp(cam.xyz + vec3(0.8), vec3(0.0), vec3(1.0))*keying, keying);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
float sampleDistance = 30.0;
float diffusion = 1.0;
float turbulence = 0.2;
float fluidify = 0.1;
float attenuate = 0.005;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy/iResolution.xy);
    
    vec4 baseColor = texture(iChannel0, uv);
    
    vec2 sDist = sampleDistance/iResolution.xy;
    
    vec4 newColor = texture(iChannel1, uv);
    vec2 turb = (texture(iChannel3, uv).xy*2.0-1.0);

    vec4 newColor1 = texture(iChannel1, uv + vec2(1.0,0.0)*sDist);
    vec4 newColor2 = texture(iChannel1, uv + vec2(-1.0,0.0)*sDist);
    vec4 newColor3 = texture(iChannel1, uv + vec2(0.0,1.0)*sDist);
    vec4 newColor4 = texture(iChannel1, uv + vec2(0.0,-1.0)*sDist);
    
    vec4 newColor5 = texture(iChannel1, uv + vec2(1.0,1.0)*sDist);
    vec4 newColor6 = texture(iChannel1, uv + vec2(-1.0,1.0)*sDist);
    vec4 newColor7 = texture(iChannel1, uv + vec2(1.0,-1.0)*sDist);
    vec4 newColor8 = texture(iChannel1, uv + vec2(-1.0,-1.0)*sDist);
     
    vec2 t = (newColor1.x+newColor1.y+newColor1.z)/3.0 * vec2(1.0,0.0);
    t += (newColor2.x+newColor2.y+newColor2.z)/3.0 * vec2(-1.0,0.0);
    t += (newColor3.x+newColor3.y+newColor3.z)/3.0 * vec2(0.0,1.0);
    t += (newColor4.x+newColor4.y+newColor4.z)/3.0 * vec2(0.0,-1.0);
    
    t += (newColor5.x+newColor5.y+newColor5.z)/3.0 * vec2(1.0,1.0);
    t += (newColor6.x+newColor6.y+newColor6.z)/3.0 * vec2(-1.0,1.0);
    t += (newColor7.x+newColor7.y+newColor7.z)/3.0 * vec2(1.0,-1.0);
    t += (newColor8.x+newColor8.y+newColor8.z)/3.0 * vec2(-1.0,-1.0);
    
    t /= 8.0;
	vec2 m = iMouse.xy/iResolution.xy;
    vec2 dir = vec2(t+turb*turbulence)*iTimeDelta*diffusion*(m.x*2.0-1.0);
    
    vec4 res = texture(iChannel1, uv + dir);
    
    if(iFrame < 10 || texture(iChannel2, vec2(32.5/256.0, 0.5) ).x > 0.5)
    {
    	fragColor =  baseColor;
    }
    else
    {
    	fragColor = mix(res, baseColor, clamp(baseColor.a*fluidify + attenuate,0.0,1.0));
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 speed = vec2(1.0,2.0);
float v = 30.0;
float dist = 0.3;
float random1 = 1.0;
float random2 = 100.0;

float hash(float n)
{
   return fract(sin(dot(vec2(n,n) ,vec2(12.9898,78.233))) * 43758.5453);  
}  

vec2 turbulence(vec2 uv)
{
    vec2 turb;
    turb.x = sin(uv.x);
    turb.y = cos(uv.y);
    
    for(int i = 0; i < 10; i++)
    {
        float ifloat = 1.0 + float(i);
        float ifloat1 = ifloat + random1;
        float ifloat2 = ifloat + random2; 
        
        float r1 = hash(ifloat1)*2.0-1.0;
        float r2 = hash(ifloat2)*2.0-1.0;
        
        vec2 turb2;
        turb2.x = sin(uv.x*(1.0 + r1*v) + turb.y*dist*ifloat + iTime*speed.x*r2);
        turb2.y = cos(uv.y*(1.0 + r1*v) + turb.x*dist*ifloat + iTime*speed.y*r2);
        
        turb.x = mix(turb.x, turb2.x, 0.5);
        turb.y = mix(turb.y, turb2.y, 0.5);
    }
    
    return turb;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ratio = iResolution.x/iResolution.y;
    vec2 uv = fragCoord.xy/iResolution.xy;
    uv.x *= ratio;

    vec2 turb = turbulence(uv)*0.5+0.5;
    
    fragColor = vec4(turb.x, turb.y, 0.0, 0.0);
      
}