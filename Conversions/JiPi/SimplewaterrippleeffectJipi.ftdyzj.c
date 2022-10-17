
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Make this a smaller number for a smaller timestep.
// Don't make it bigger than 1.4f or the universe will explode.


__KERNEL__ void SimplewaterrippleeffectJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0); 
    
    fragCoord+=0.5f;

    const float delta = 1.0f;

    if (iFrame == 0) {
       fragColor = to_float4_s(0); 
       SetFragmentShaderComputedColor(fragColor);
       return;
    }
    
    float pressure = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution).x;
    float pVel     = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution).y;

    float p_right = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(1, 0))+0.5f)/iResolution).x;
    float p_left  = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(-1, 0))+0.5f)/iResolution).x;
    float p_up    = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(0, 1))+0.5f)/iResolution).x;
    float p_down  = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(0, -1))+0.5f)/iResolution).x;
    
    // Change values so the screen boundaries aren't fixed.
    if (fragCoord.x == 0.5f) p_left = p_right;
    if (fragCoord.x == iResolution.x - 0.5f) p_right = p_left;
    if (fragCoord.y == 0.5f) p_down = p_up;
    if (fragCoord.y == iResolution.y - 0.5f) p_up = p_down;

    // Apply horizontal wave function
    pVel += delta * (-2.0f * pressure + p_right + p_left) / 4.0f;
    // Apply vertical wave function (these could just as easily have been one line)
    pVel += delta * (-2.0f * pressure + p_up + p_down) / 4.0f;
    
    // Change pressure by pressure velocity
    pressure += delta * pVel;
    
    // "Spring" motion. This makes the waves look more like water waves and less like sound waves.
    pVel -= 0.005f * delta * pressure;
    
    // Velocity damping so things eventually calm down
    pVel *= 1.0f - 0.002f * delta;
    
    // Pressure damping to prevent it from building up forever.
    pressure *= 0.999f;
    
    //x = pressure. y = pressure velocity. Z and W = X and Y gradient
    fragColor = to_float4(pressure, pVel, (p_right - p_left) / 2.0f, (p_up - p_down) / 2.0f);
    
    
    if (iMouse.z > 1.0f) {
        float dist = distance_f2(fragCoord, swi2(iMouse,x,y));
        if (dist <= 20.0f) {
            fragColor.x += 1.0f - dist / 20.0f;
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SimplewaterrippleeffectJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    float4 data = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    // Brightness = water height
    //swi3(fragColor,x,y,z) = to_float3(data.x + 1.0f) / 2.0f;
    
    // Color = texture
    fragColor = texture(iChannel1, uv + 0.2f * swi2(data,z,w));
    
    // Sunlight glint
    float3 normal = normalize(to_float3(-data.z, 0.2f, -data.w));
    fragColor += to_float4_s(1) * _powf(_fmaxf(0.0f, dot(normal, normalize(to_float3(-3, 10, 3)))), 60.0f);


  SetFragmentShaderComputedColor(fragColor);
}