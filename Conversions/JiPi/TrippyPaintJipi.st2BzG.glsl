

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(
    out vec4 out_frag_color,
    in vec2 frag_coord)
{
    vec2 texel_size = (1.0 / iResolution.xy);
    
    vec4 selfState = texture(iChannel0, (frag_coord.xy * texel_size));
    
    out_frag_color = vec4(sqrt(selfState).rgb, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(
    out vec4 out_frag_color,
    in vec2 frag_coord)
{
    vec2 texel_size = (1.0 / iResolution.xy);
    
	vec4 prev_self_state = texture(iChannel0, (frag_coord.xy * texel_size));
    
    vec4 new_self_state = prev_self_state;
    
    // Bleed in from neighbors.
    {
        float bleed_angle = 0.0;
        bleed_angle += fract(0.08 * iTime);
        //bleed_angle += fract(0.002 * (frag_coord.x + frag_coord.y));
        bleed_angle += (2.0 * texture(iChannel2, (0.0002 * frag_coord.xy)).r);
        bleed_angle *= k_tau;
        
        vec2 neighbor_offset = vec2(cos(bleed_angle), sin(bleed_angle));
         
		vec4 neighbor_state = texture(iChannel0, ((frag_coord.xy + neighbor_offset) * texel_size));
        
        float blending_fract = 0.4;
        //blending_fract = mix(0.15, 0.8, smoothstep(-1.0, 1.0, cos(k_tau * 0.15 * iTime)));
            
		new_self_state.rgb = mix(new_self_state.rgb, neighbor_state.rgb, blending_fract);
    }
    
    // If any mouse button is pressed, add more paint.
    if (iMouse.z > 0.0)
    {
        vec2 fragToMouse = ((iMouse.xy + 0.5) - frag_coord.xy);
        
        float fragmentToCursorSquared = dot(fragToMouse, fragToMouse);
        
        if (fragmentToCursorSquared < (k_cursor_radius * k_cursor_radius))
        {
            // Intentionally use a curved falloff.
            float strengthFract = (1.0 - (fragmentToCursorSquared / (k_cursor_radius * k_cursor_radius)));
            
            // Soften the edges.
            strengthFract *= strengthFract;
            strengthFract *= strengthFract;
		
            new_self_state.rgb = mix(new_self_state.rgb, calc_cursor_color(iTime), strengthFract);
        }
    }
    
    /* KILLED: Because it caused secondary colors to promptly warp into bizarre results.
    // Re-saturate the color to keep everything from turning muddy-brown.
    {
        vec3 self_hsb = rgb_to_hsb(new_self_state.rgb);
        
        const float minimum_saturation = 0.9;
        
        self_hsb.y = 
            (self_hsb.y < minimum_saturation) ?
				mix(self_hsb.y, minimum_saturation, 0.01) :
        		self_hsb.y;
        
        //self_hsb.z = step(0.01, self_hsb.z);
        
        new_self_state.rgb = hsb_to_rgb(self_hsb);
    }
	*/
    
    // If this is the first frame or the spacebar's been pressed.
	if (iFrame == 0)
    {
        vec2 central_normalized_uv = ((frag_coord.xy - (0.5 * iResolution.xy)) / vec2(iResolution.y));
        float distance_to_center = length(central_normalized_uv);
        
        float brightness = 
            smoothstep(0.45, 0.35, distance_to_center) * 
            smoothstep(0.25, 0.35, distance_to_center);
            
        float hue = (atan(central_normalized_uv.y, central_normalized_uv.x) / k_tau);
        hue = mix(0.66, 0.98, smoothstep(-1.0, 1.0, sin(k_tau * hue)));
        
        new_self_state = vec4(hsb_to_rgb(vec3(hue, 1.0, brightness)), 1.0);
    }
    else if (texelFetch(iChannel1, ivec2(k_keycode_spacebar, 1), 0).x > 0.0)
    {
        new_self_state = vec4(vec3(0.0), 1.0);
    }
    
    out_frag_color = new_self_state;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const float k_tau = radians(360.0);

const float k_cursor_radius = 30.0;

const int k_keycode_spacebar = 32;

vec3 hsb_to_rgb(
    vec3 hsb_color)
{
    // From: https://www.shadertoy.com/view/MsS3Wc
    vec3 rgb = clamp(abs(mod((hsb_color.x * 6.0) + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    rgb = (rgb * rgb * (3.0 - (2.0 * rgb)));
    return (hsb_color.z * mix(vec3(1.0), rgb, hsb_color.y));
}

/* These generate a rather ugly hue-wheel, plus it appears to drasticaly alter colors when just converting back and forth.
vec3 hsb_to_rgb(
    vec3 hsb_color)
{
    // From: https://www.laurivan.com/rgb-to-hsv-to-rgb-for-shaders/
    
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(hsb_color.xxx + K.xyz) * 6.0 - K.www);
   
    return hsb_color.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsb_color.y);
}

vec3 rgb_to_hsb(
    vec3 rgb_color)
{
    // From: https://www.laurivan.com/rgb-to-hsv-to-rgb-for-shaders/
    
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(rgb_color.bg, K.wz), vec4(rgb_color.gb, K.xy), step(rgb_color.b, rgb_color.g));
    vec4 q = mix(vec4(p.xyw, rgb_color.r), vec4(rgb_color.r, p.yzx), step(p.x, rgb_color.r));
 
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
*/

vec3 calc_cursor_color(
	float timeSeconds)
{
    return hsb_to_rgb(vec3(0.5 * timeSeconds, 1.0, 1.0));
}