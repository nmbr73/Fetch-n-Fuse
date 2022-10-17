
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



#define k_tau  radians(360.0f)

const float k_cursor_radius = 30.0f;

//const int k_keycode_spacebar = 32;

__DEVICE__ float3 hsb_to_rgb(
    float3 hsb_color)
{
    // From: https://www.shadertoy.com/view/MsS3Wc
    float3 rgb = clamp(abs_f3(mod_f((hsb_color.x * 6.0f) + to_float3(0.0f, 4.0f, 2.0f), 6.0f) - 3.0f) - 1.0f, 0.0f, 1.0f);
    rgb = (rgb * rgb * (3.0f - (2.0f * rgb)));
    return (hsb_color.z * _mix(to_float3_s(1.0f), rgb, hsb_color.y));
}

/* These generate a rather ugly hue-wheel, plus it appears to drasticaly alter colors when just converting back and forth.
__DEVICE__ float3 hsb_to_rgb(
    float3 hsb_color)
{
    // From: https://www.laurivan.com/rgb-to-hsv-to-rgb-for-shaders/
    
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = _fabs(fract(swi3(hsb_color,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
   
    return hsb_color.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), hsb_color.y);
}

__DEVICE__ float3 rgb_to_hsb(
    float3 rgb_color)
{
    // From: https://www.laurivan.com/rgb-to-hsv-to-rgb-for-shaders/
    
    float4 K = to_float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float4 p = _mix(to_float4(rgb_color.bg, swi2(K,w,z)), to_float4(rgb_color.gb, swi2(K,x,y)), step(rgb_color.z, rgb_color.y));
    float4 q = _mix(to_float4(swi3(p,x,y,w), rgb_color.x), to_float4(rgb_color.x, swi3(p,y,z,x)), step(p.x, rgb_color.x));
 
    float d = q.x - _fminf(q.w, q.y);
    float e = 1.0e-10;
    
    return to_float3_aw(_fabs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}
*/

__DEVICE__ float3 calc_cursor_color(float timeSeconds)
{
    return hsb_to_rgb(to_float3(0.5f * timeSeconds, 1.0f, 1.0f));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Gray Noise Medium' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void TrippyPaintJipiFuse__Buffer_A(float4 out_frag_color, float2 frag_coord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    frag_coord+=0.5f;

    float2 texel_size = (1.0f / iResolution);
    
    float4 prev_self_state = texture(iChannel0, (swi2(frag_coord,x,y) * texel_size));
    
    float4 new_self_state = prev_self_state;
    
    // Bleed in from neighbors.
    {
        float bleed_angle = 0.0f;
        bleed_angle += fract(0.08f * iTime);
        //bleed_angle += fract(0.002f * (frag_coord.x + frag_coord.y));
        bleed_angle += (2.0f * texture(iChannel1, (0.0002f * swi2(frag_coord,x,y))).x);
        bleed_angle *= k_tau;
        
        float2 neighbor_offset = to_float2(_cosf(bleed_angle), _sinf(bleed_angle));
         
        float4 neighbor_state = texture(iChannel0, ((swi2(frag_coord,x,y) + neighbor_offset) * texel_size));
        
        float blending_fract = 0.4f;
        //blending_fract = _mix(0.15f, 0.8f, smoothstep(-1.0f, 1.0f, _cosf(k_tau * 0.15f * iTime)));
            
        swi3S(new_self_state,x,y,z, _mix(swi3(new_self_state,x,y,z), swi3(neighbor_state,x,y,z), blending_fract));
    }
    
    // If any mouse button is pressed, add more paint.
    if (iMouse.z > 0.0f)
    {
        float2 fragToMouse = ((swi2(iMouse,x,y) + 0.5f) - swi2(frag_coord,x,y));
        float fragmentToCursorSquared = dot(fragToMouse, fragToMouse);
        if (fragmentToCursorSquared < (k_cursor_radius * k_cursor_radius))
        {
            // Intentionally use a curved falloff.
            float strengthFract = (1.0f - (fragmentToCursorSquared / (k_cursor_radius * k_cursor_radius)));
            
            // Soften the edges.
            strengthFract *= strengthFract;
            strengthFract *= strengthFract;
    
            swi3S(new_self_state,x,y,z, _mix(swi3(new_self_state,x,y,z), calc_cursor_color(iTime), strengthFract));
        }
    }
    
    /* KILLED: Because it caused secondary colors to promptly warp into bizarre results.
    // Re-saturate the color to keep everything from turning muddy-brown.
    {
        float3 self_hsb = rgb_to_hsb(swi3(new_self_state,x,y,z));
        
        const float minimum_saturation = 0.9f;
        
        self_hsb.y = 
            (self_hsb.y < minimum_saturation) ?
        _mix(self_hsb.y, minimum_saturation, 0.01f) :
            self_hsb.y;
        
        //self_hsb.z = step(0.01f, self_hsb.z);
        
        swi3(new_self_state,x,y,z) = hsb_to_rgb(self_hsb);
    }
  */
    
    // If this is the first frame or the spacebar's been pressed.
  if (iFrame == 0 || Reset)
    {
        float2 central_normalized_uv = ((swi2(frag_coord,x,y) - (0.5f * iResolution)) / to_float2_s(iResolution.y));
        float distance_to_center = length(central_normalized_uv);
        
        float brightness = 
            smoothstep(0.45f, 0.35f, distance_to_center) * 
            smoothstep(0.25f, 0.35f, distance_to_center);
            
        float hue = (_atan2f(central_normalized_uv.y, central_normalized_uv.x) / k_tau);
        hue = _mix(0.66f, 0.98f, smoothstep(-1.0f, 1.0f, _sinf(k_tau * hue)));
        
        new_self_state = to_float4_aw(hsb_to_rgb(to_float3(hue, 1.0f, brightness)), 1.0f);
    }
    else if (Reset)
    {
        new_self_state = to_float4_aw(to_float3_s(0.0f), 1.0f);
    }
    
    out_frag_color = new_self_state;

  SetFragmentShaderComputedColor(out_frag_color);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void TrippyPaintJipiFuse(float4 out_frag_color, float2 frag_coord, float2 iResolution, sampler2D iChannel0)
{

    float2 texel_size = (1.0f / iResolution);
    float4 selfState = texture(iChannel0, (swi2(frag_coord,x,y) * texel_size));
    
    out_frag_color = to_float4_aw(swi3(sqrt_f4(selfState),x,y,z), 1.0f);

  SetFragmentShaderComputedColor(out_frag_color);
}