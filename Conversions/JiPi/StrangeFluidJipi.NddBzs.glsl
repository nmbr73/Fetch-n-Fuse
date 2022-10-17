

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float val = texture(iChannel0, fragCoord/iResolution.xy).r;
 
   	//{fragColor = vec4(val,val,val,1.0); return;}
    fragColor = vec4(val, val, val, 1.0);
        
    vec4 color = pow(vec4(cos(val), tan(val), sin(val), 1.0) * 0.5 + 0.5, vec4(0.5));
    
    // code below taken from
    //https://www.shadertoy.com/view/Xsd3DB
    
    vec2 q = fragCoord.xy/iResolution.xy;
    
    vec3 e = vec3(vec2(1.0)/iResolution.xy,0.);
    float p10 = texture(iChannel0, q-e.zy).x;
    float p01 = texture(iChannel0, q-e.xz).x;
    float p21 = texture(iChannel0, q+e.xz).x;
    float p12 = texture(iChannel0, q+e.zy).x;
        
    vec3 grad = normalize(vec3(p21 - p01, p12 - p10, 1.));
    vec3 light = normalize(vec3(.2,-.25,.7));
    float diffuse = dot(grad,light);
    float spec = pow(max(0.,-reflect(light,grad).z),32.0);
    
    fragColor = (color * diffuse) + spec;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float pi = 3.1415;
const float pi2 = pi/2.0;

float random()
{
	return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898,78.233))) * 43758.5453);  
}

vec4 get_pixel(float x_offset, float y_offset)
{
	return texture(iChannel0, (gl_FragCoord.xy / iResolution.xy) + (vec2(x_offset, y_offset) / iResolution.xy));
}

float step_simulation()
{
	float val = get_pixel(0.0, 0.0).r;
    
    val += random()*val*0.15; // errosion
    
  	val = get_pixel(
    	sin(get_pixel(val, 0.0).r  - get_pixel(-val, 0.0) + pi).r  * val * 0.4, 
        cos(get_pixel(0.0, -val).r - get_pixel(0.0 , val) - pi2).r * val * 0.4
   	).r;
    
    val *= 1.0001;
    
    return val;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    float val = step_simulation();
 
    if(iFrame == 0)
        val = 
        	random()*length(iResolution.xy)/100.0 + 
        	smoothstep(length(iResolution.xy)/2.0, 0.5, length(iResolution.xy * 0.5 - fragCoord.xy))*25.0;
    
    if (iMouse.z > 0.0) 
        val += smoothstep(length(iResolution.xy)/10.0, 0.5, length(iMouse.xy - fragCoord.xy));
        
    fragColor.r = val;
}