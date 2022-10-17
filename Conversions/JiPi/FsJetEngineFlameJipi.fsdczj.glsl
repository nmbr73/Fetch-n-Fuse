

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//
// Based on "Flame in the Wind" by kuvkar (https://www.shadertoy.com/view/4tXXRn)
//

float GetNoise(vec2 uv) // -> (-0.375, 0.375)
{
    float n = (texture(iChannel0, uv).r - 0.5) * 0.5; // -0.25, 0.25
    n += (texture(iChannel0, uv * 2.0).r - 0.5) * 0.5 * 0.5; // -0.375, 0.375
    
    return n;
}

mat2 GetRotationMatrix(float angle)
{
    mat2 m;
    m[0][0] = cos(angle); m[0][1] = -sin(angle);
    m[1][0] = sin(angle); m[1][1] = cos(angle);

    return m;
}

#define flamePersonalitySeed .5

// -----------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    vec2 flameSpacePosition = fragCoord.xy / iResolution.xy - vec2(0.5, 0.0); // (x=[-0.5, 0.5], y=[0.0, 1.0])
    
    flameSpacePosition.x *= (iResolution.x / iResolution.y); // obey aspect ratio
        
    // Simulate quad
    flameSpacePosition.x *= 1.4;
    if (abs(flameSpacePosition.x) > 1.0)
    {
        fragColor = vec4(0.0);
        return;
    }
    
    float paramFlameProgress = iTime;
    
    #define FlameSpeed 0.8
    vec2 noiseOffset = vec2(flamePersonalitySeed, flamePersonalitySeed - paramFlameProgress * FlameSpeed);
    
    ////////////////////////////////////////
    
    vec2 uv = flameSpacePosition;        
    
    //
    // Get noise for this fragment and time
    //
    
    #define NoiseResolution 0.7
    // (-0.375, 0.375)
    float fragmentNoise = GetNoise(uv * NoiseResolution + noiseOffset);
    
    //
    // Rotate fragment based on noise
    //
    
    float angle = fragmentNoise;

    // Tune amount of chaos
    //angle *= 0.45;

    // Rotate (and add)
    uv += GetRotationMatrix(angle) * uv;

    //
    // Calculate flameness
    //
    
    float flameWidth = 0.1 + 0.4 * min(1.0, sqrt(flameSpacePosition.y / 0.4)); // Taper down 
    float flameness = 1.0 - abs(uv.x) / flameWidth;
    
    // Taper flame up depending on randomized height
    float variationH = (fragmentNoise + 0.5) * 1.4;
    flameness *= smoothstep(1.1, variationH * 0.5, flameSpacePosition.y);    
    
    //
    // Emit
    //
    
    vec3 col1 = mix(vec3(1.0, 1.0, 0.6), vec3(1.0, 1.0, 1.0), flameness);
    col1 = mix(vec3(227.0/255.0, 69.0/255.0, 11.0/255.0), col1, smoothstep(0.3, 0.8, flameness));    
    float alpha = smoothstep(0.0, 0.5, flameness);
    
    //---------------------------------------------
    // Blend with black background    
    fragColor = mix(vec4(1.0), vec4(col1, 1.0), alpha);
}