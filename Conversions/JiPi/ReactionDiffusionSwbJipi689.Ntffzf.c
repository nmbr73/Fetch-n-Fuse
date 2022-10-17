
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// 2D Random
__DEVICE__ float random (in float2 st) {
    return fract(_sinf(dot(swi2(st,x,y), to_float2(12.9898f,78.233f))) * 43758.5453123f);
}
__DEVICE__ float noise (in float2 st) {
    float2 i = _floor(st);
    float2 f = fract_f2(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + to_float2(1.0f, 0.0f));
    float c = random(i + to_float2(0.0f, 1.0f));
    float d = random(i + to_float2(1.0f, 1.0f));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    float2 u = f*f*(3.0f-2.0f*f);
    // u = smoothstep(0.0f,1.0f,f);

    // Mix 4 coorners percentages
    return _mix(a, b, u.x) +
            (c - a)* u.y * (1.0f - u.x) +
            (d - b) * u.x * u.y;
}

__DEVICE__ float quantize(in float color, float steps) {
    return (_floor(color * steps)+0.150f) /steps;
}
__DEVICE__ float idk (in float a, in float b) {
    return noise(to_float2(a, b/2.0f));
}
__DEVICE__ float getBrightness(in float3 color) {
    //return (color.x + color.y + color.z)/3.0f;
    return (0.299f*color.x + 0.587f*color.y + 0.114f*color.z);
}
__DEVICE__ float getSaturation(in float3 color) {
    float colorMax = _fmaxf(color.z, _fmaxf(color.x, color.y));
    float colorMin = _fminf(color.z, _fminf(color.x, color.y));
    float luminence = getBrightness(color);
    if (luminence == 1.0f) {
        return 0.0f;
    }
    return (colorMax - colorMin) / (1.0f - (2.0f * luminence - 1.0f));
    

}
__DEVICE__ float3 getNewFrameColor(in float3 lastFrameColor, float2 uv, __TEXTURE2D__ lastFrame) {
    float pi = 3.14159f;

    float speed = 0.0012f;

    float3 newFrameColor =      swi3(texture(lastFrame, to_float2(uv.x + (0.99f - getBrightness(lastFrameColor)) * speed, uv.y)),x,y,z);
    float3 maybeNewFrameColor = swi3(texture(lastFrame, to_float2(uv.x - (0.99f - getBrightness(lastFrameColor)) * speed, uv.y)),x,y,z);
    /*
    if (random(swi2(lastFrameColor,x,y)) > 0.5f) {
    return newFrameColor;
    } else {
    return maybeNewFrameColor;
    }
    */
    if (getSaturation(maybeNewFrameColor) > getSaturation(newFrameColor)){
        return maybeNewFrameColor;
    }
    
    //newFrameColor = newFrameColor/2.0f;
    return newFrameColor;
}
__DEVICE__ float diffuse( in float state[9], float rules[9]) {
    float retVal = 0.0f;
    for (int i = 0; i < 9; i++) {
        retVal = retVal + state[i] * rules[i];
    }
    return retVal;
}
__DEVICE__ float2 getUV(in float2 inFragCoord, float2 resolution){
  inFragCoord.x = mod_f(inFragCoord.x, resolution.x);
  inFragCoord.y = mod_f(inFragCoord.y, resolution.y);
  return inFragCoord/swi2(resolution,x,y);
}


__KERNEL__ void ReactionDiffusionSwbJipi689Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    fragCoord+=0.5f;
    // Normalized pixel coordinates (from 0 to 1)
    float pi = 3.14159f;
    bool zoomers = false;
    bool do_quantize = false;
    float2 uv = fragCoord/iResolution;
    float3 lastFrameColor = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    // float3 newFrameColor = getNewFrameColor(lastFrameColor, uv, iChannel0);
    fragColor = to_float4_aw(lastFrameColor, 1.0f);
    //  fragColor.x = random(uv);
    float webcamBrightness = getBrightness(swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z));
    //anotherNoiseVal = mod_f(iTime, 5.0f)/5.0f;
    if (iTime < 0.05f || Reset) {
        fragColor = texture(iChannel1, (swi2(uv,x,y)/to_float2(4.0f, 5.0f)));
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    if (do_quantize) {
        fragColor.x = quantize(fragColor.x, 20.0f);
        fragColor.y = quantize(fragColor.y, 10.0f);
        fragColor.z = quantize(fragColor.z, 5.0f);
    }
    
    float diagonalDiffuse = 0.05f;
    float adjacentDiffuse = 0.2f;
    float rules[9] = {
                     diagonalDiffuse, adjacentDiffuse, diagonalDiffuse,
                     adjacentDiffuse, -1.0f, adjacentDiffuse,
                     diagonalDiffuse, adjacentDiffuse, diagonalDiffuse};
    
    float fstate[9] = {
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y - 1.0f), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y - 1.0f), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y - 1.0f), iResolution)).x,
                      
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y), iResolution)).x,
                      
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y + 1.0f), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y + 1.0f), iResolution)).x,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y + 1.0f), iResolution)).x
                      };
    
    float bstate[9] = {
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y - 1.0f), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y - 1.0f), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y - 1.0f), iResolution)).y,
                      
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y), iResolution)).y,
                      
                      texture(iChannel0, getUV(to_float2(fragCoord.x - 1.0f, fragCoord.y + 1.0f), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x, fragCoord.y + 1.0f), iResolution)).y,
                      texture(iChannel0, getUV(to_float2(fragCoord.x + 1.0f, fragCoord.y + 1.0f), iResolution)).y
                      };
    
    
    
    float timestep = 1.0f;
    float bunnies = lastFrameColor.y;
    if (mod_f((float)(iFrame), 5.0f) != 0.0f) {
        // fragColor = to_float4(swi3(lastFrameColor,x,y,z), 1);
        // return;
    }
    // diffuse
    float dA = 1.0f;
    float dB = 1.0f - webcamBrightness;  // 0.5f;  
    dB = (_sinf(iTime/10.0f) + 1.0f)/3.0f;
    fragColor = to_float4(dA * diffuse(fstate, rules), dB * diffuse(bstate, rules), 0, 1);
    /* */

    float maxFood = 1.0f;
    // float feedRate = 0.15f;
    // feedRate = (webcamBrightness + 0.45f)/3.15f;
    float feedRate = 0.055f;

    float dieRate = 0.062f;

    // react

    float eatenFood = lastFrameColor.x * (lastFrameColor.y * lastFrameColor.y);
    fragColor.x = fragColor.x - eatenFood;
    fragColor.y = fragColor.y + eatenFood;
    // kill
    fragColor.y = clamp(lastFrameColor.y + timestep*(fragColor.y - ((dieRate + feedRate) * lastFrameColor.y)), 0.0001f, 1.0f);
    // feed
    float addedFood = feedRate * clamp(maxFood - lastFrameColor.x, 0.0f, 1.0f);
    fragColor.x = clamp(timestep*(fragColor.x + addedFood) + lastFrameColor.x, 0.0f, 1.0f);
    fragColor.z = eatenFood;
    
    if (distance_f2(fragCoord, swi2(iMouse,x,y)) < 40.0f && iMouse.z > 0.0f) {
        fragColor.x = 0.5f;
        if (distance_f2(fragCoord, swi2(iMouse,x,y)) < 20.0f) {
            fragColor.y = 0.1f;
        }
    }
    /**/
    /*
        // react
    float eatenFood = lastFrameColor.x * lastFrameColor.y * lastFrameColor.y;
    fragColor.x = fragColor.x - eatenFood;
    fragColor.y += eatenFood;
    // kill
    fragColor.y = fragColor.y - (dieRate * lastFrameColor.y);
    // feed
    float addedFood = feedRate * (maxFood - lastFrameColor.x);
    fragColor.x = fragColor.x + addedFood;
    fragColor.z = 0.0f;
    */
    // fragColor.x = noise(to_float2(idk(uv.x, iTime), idk(uv.y, iTime))*scale);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Small' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0



__DEVICE__ int getLowerRampIndex(in float key, float4 ramp[10]){
  
    // change ramp count if you add more points on the gradient
    int rampCount = 5;
    
    for (int i = 0; i < rampCount - 1; i++) {
        float lowerBound = ramp[i].w;
        float upperBound = ramp[i + 1].w;
        if (key <= ramp[i + 1].w) {
            return i;
        }
    }
    return rampCount - 1;
}

__DEVICE__ float easeInOutCubic(in float _x) {
    if (_x < 0.5f) {
        return 4.0f * _x * _x * _x;
    }    
    return 1.0f - _powf(-2.0f * _x + 2.0f, 3.0f) / 2.0f;
}

__DEVICE__ float getLum(in float3 color) {
    return 0.2126f*color.x + 0.7152f*color.y + 0.0722f*color.z;
}

__DEVICE__ float4 evalSpecials(in float4 color, in float3 tex_col, float2 Threshold) {
    //if (color.x > 68.5f && color.x < 69.5f) {
      if (color.x > Threshold.x && color.x < Threshold.y) {
        return to_float4_aw(tex_col, color.w);
    }
    else if (color.x > 1.0f && color.y > 1.0f && color.z > 1.0f) {
        return to_float4(color.x/255.0f, color.y/255.0f, color.z/255.0f, color.w);
    }
    return color;
    
}
__KERNEL__ void ReactionDiffusionSwbJipi689Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    CONNECT_SLIDER0(Band, -1.0f, 10.0f, 1.1f);
    CONNECT_SLIDER1(UpperThres, -1.0f, 200.0f, 68.5f);
    CONNECT_SLIDER2(LowerThres, -1.0f, 200.0f, 69.5f);
    CONNECT_SLIDER3(ShowGradient, -1.0f, 1.0f, 0.95f);
    CONNECT_COLOR0(Zurple,   0.65f, 0.65f, 0.85f, 1.0f); 
    CONNECT_COLOR1(Wow,      0.8f, 1.0f, 0.007f, 1.0f); 
    CONNECT_COLOR2(MaxedOut, 1.0f, 1.0f, 1.0f, 2.0f); 

    float2 Threshold= to_float2( UpperThres, LowerThres);

    // change the ramp values for colors
    // a is the position on the gradient. 
    // 69 in r = return the original color

    float3 zurple = to_float3(0.65f, 0.1921f, 0.4235f);

#ifdef ORG
    float4 ramp[10] = {
              to_float4_aw(to_float3(0.65f, 0.65f, 0.85f), 0.0f), // zurple
              to_float4_aw(to_float3(0.8f, 1.0f, 0.007f), 1.0f), // wow yellow
              to_float4_aw(to_float3(0.8f, 1.0f, 0.007f), 1.0f), // wow yellow
              to_float4_aw(to_float3(0.8f, 1.0f, 0.007f), 1.0f), // wow yellow
              to_float4_aw(to_float3(0.8f, 1.0f, 0.007f), 1.0f), // wow yellow
              to_float4_aw(to_float3(0.8f, 1.0f, 0.007f), 1.0f), // wow yellow
              to_float4_aw(to_float3(1.0f, 1.0f, 1.0f), 2.0f), // white, maxed out
              to_float4_aw(to_float3(1.0f, 1.0f, 1.0f), 2.0f), // white, maxed out
              to_float4_aw(to_float3(1.0f, 1.0f, 1.0f), 2.0f), // white, maxed out
              to_float4_aw(to_float3(1.0f, 1.0f, 1.0f), 2.0f) // white, maxed out
              };
#endif
    float4 ramp[10] = {
              Zurple, // zurple
              Wow, // wow yellow
              Wow, // wow yellow
              Wow, // wow yellow
              Wow, // wow yellow
              Wow, // wow yellow
              MaxedOut, // white, maxed out
              MaxedOut, // white, maxed out
              MaxedOut, // white, maxed out
              MaxedOut // white, maxed out
              };




ramp[0] = to_float4_aw(swi3(Zurple,x,y,z),0.0f);

    float4 min = ramp[0];
    float4 max = ramp[1];
    // ramp[1] = to_float4_aw(ramp[1].rgb, (_sinf(iTime * 0.5f)/4.0f) + 1.0f);
    // float4 color1 = to_float4(to_float3(0.0f, 1.0f, 0.0f), 1.0f);
    bool found = false;
    float2 uv = fragCoord/iResolution;
    
    
    float3 tex_col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    //float mixVal = getLum(tex_col);
    float mixVal = Band - tex_col.x;//1.1f - tex_col.x;
    
    // debug show gradient, try commenting this out
    //if (uv.x > 0.95f) { mixVal = uv.y; }
    if (uv.x > ShowGradient) { mixVal = uv.y; }
    // end show gradient
float IIIIIIIIIIIIIIIIIIIIIIII;  
    int lowerRampIndex = getLowerRampIndex(mixVal, ramp);
    min = ramp[lowerRampIndex];
    max = ramp[lowerRampIndex + 1];
    mixVal = ((mixVal-min.w)/(max.w - min.w));
    mixVal = easeInOutCubic(mixVal);
    // Normalized pixel coordinates (from 0 to 1)
    // r value of 69 = get the texture color
    min = evalSpecials(min, tex_col,Threshold);
    max = evalSpecials(max, tex_col,Threshold);
    float3 color = _mix(swi3(min,x,y,z), swi3(max,x,y,z), mixVal);

    // Time varying pixel color

    // Output to screen
    fragColor = to_float4_aw(color, Zurple.w);

  SetFragmentShaderComputedColor(fragColor);
}