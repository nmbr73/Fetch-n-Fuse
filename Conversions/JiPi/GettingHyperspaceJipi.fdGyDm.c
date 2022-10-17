
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define NUM_STARS (1 << 11)

__DEVICE__ float rand(float x) {
    return fract(_sinf(x) * 123.456f);
}

__DEVICE__ float cross2(float2 a, float2 b) {
    return a.x * b.y - a.y * b.x;
}

__DEVICE__ float getDistanceLP(float2 s, float2 t, float2 p) {
    return _fabs(cross2(t - s, p - s) / distance_f2(t, s));
}

__DEVICE__ float getDistanceSP(float2 s, float2 t, float2 p) {
    if (dot(t - s, p - s) < 0.0f) {
        return distance_f2(p, s);
    }
    if (dot(s - t, p - t) < 0.0f) {
        return distance_f2(p, t);
    }
    return getDistanceLP(s, t, p);
}

__KERNEL__ void GettingHyperspaceJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;
float IIIIIIIIIIIIIIIII;    
    float3 col = to_float3_s(0);
    
    // Switching forward/backward transition
    const float cycle = 7.0f;
    float t = mod_f(iTime, cycle);
    if (t > cycle * 0.5f) {
       t = cycle - t;
    }
    // 1.5f sec delay timer
    float td = _fmaxf(t - 1.5f, 0.0f);
    
    
    for (int i = 0; i < NUM_STARS; i++) {
        // Create random star
        float x = rand((float)(i*3)*12.34f) * 2.0f - 1.0f;
        float y = rand((float)(i*3+1)*23.45f) * 2.0f - 1.0f;
        float2 c = to_float2(x, y);
        float r = rand((float)(i*3+2)) * 0.0018f;
        
        // Create line segment of star and get distance from uv coordinates
        float2 n = c * (_expf(t*4.0f)-1.0f) * 0.002f;
        float d = (getDistanceSP(c, c + n, swi2(uv,x,y))-r);
        
        // Line segment color
        col += to_float3_s(_expf(-800.0f*d)) * to_float3(0.7f, 0.8f, 1.0f);
        
        // Environmental lighting
        col += to_float3(0.7f, 0.8f, 1.0f) * ((_expf(t*0.00008f)-1.0f) + (_expf(td*td*0.01f)-1.0f) * (0.04f/length(swi2(uv,x,y))));
    }
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}