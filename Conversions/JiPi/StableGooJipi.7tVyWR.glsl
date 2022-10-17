

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float v = texture(iChannel0, fragCoord/iResolution.xy).r;
    v *= 0.5;
    v = v * 0.5 + 0.5;
    v = clamp(v, 0.0, 1.0);
    
    
    fragColor.r = v*1.25;
    fragColor.g = sin(v*0.1)*5.0+v;
    fragColor.b = pow(v*5.0, 0.5)*0.26;
    fragColor.a = 1.0;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float TEMPERATURE = 2.0;
const float RADIUS = 1.33;


const float PI = 3.14159265358979323846264338327950288419716939937510582097494459230781640;

float random()
{
	return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898,78.233))) * 43758.5453);  
}

float get_average(vec2 uv, float size)
{
    const float points = 14.0;
    const float Start = 2.0 / points;
    vec2 scale = (RADIUS * 5.0 / iResolution.xy) + size;

    float res = texture(iChannel0, uv).r;

    for (float point = 0.0; point < points; point++)
    {
        float r = (PI * 2.0 * (1.0 / points)) * (point + Start);
        res += texture(iChannel0, uv + vec2(sin(r), cos(r)) * scale).r;
    }

    res /= points;

    return res;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    vec3 noise = texture(iChannel2, iTime*0.01 + uv*0.015).rgb;
    float height = (noise.b*2.0-1.0) * 0.25;
    
    {	
        vec2 muv = noise.xy;
        
        vec2 p = uv - muv;
        float r = length(p);
        float a = atan(p.y, p.x);
        r = pow(r*2.0, 1.0 + height * 0.05);
        p = r * vec2(cos(a)*0.5, sin(a)*0.5);

        uv = p + muv;
    }
    
    /*if (texelFetch(iChannel1, ivec2(82, 0), 0).x == 1.0) {
        float rot = radians(10.0);
        uv -= 0.5;
        uv *= mat2(cos(rot), -sin(rot), sin(rot), cos(rot)) * 1.0002;
        uv += 0.5;
    }*/ 
        
    //uv.y += 0.00025;

    if (iFrame == 0)
    {
        fragColor.r = random();
        return;
    }
    
    float val = texture(iChannel0, uv).r;
    float avg = get_average(uv, height*-0.005);
    fragColor.r = sin(avg * (2.3 + TEMPERATURE + height*2.0)) + sin(val);
   
    if (iMouse.z > 0.0) 
        fragColor.r += smoothstep(length(iResolution.xy)/5.0, 0.5, length(iMouse.xy - fragCoord.xy)) * sin(iTime*10.0);
}  