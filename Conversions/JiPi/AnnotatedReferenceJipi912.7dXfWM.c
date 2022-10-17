
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/3c33c415862bb7964d256f4749408247da6596f2167dca2c86cc38f83c244aa6.mp3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define bars 32.0f                 // How many buckets to divide spectrum into
#define barSize 1.0f / bars        // Constant to avoid division in main loop
#define barGap 0.1f * barSize      // 0.1f represents gap on both sides, so a bar is
                                  // shown to be 80% as wide as the spectrum it samples
#define sampleSize 0.02f           // How accurately to sample spectrum, must be a factor of 1.0
                                  // Setting this too low may crash your browser!

// Helper for intensityToColour
__DEVICE__ float h2rgb(float h) {
  
  if(h < 0.0f) h += 1.0f;
  if(h < 0.166666f) return 0.1f + 4.8f * h;
  if(h < 0.5f)      return 0.9f;
  if(h < 0.666666f) return 0.1f + 4.8f * (0.666666f - h);
  return 0.1f;
}

// Map [0, 1] to rgb using hues from [240, 0] - ie blue to red
__DEVICE__ float3 intensityToColour(float i) {
  // Algorithm rearranged from http://www.w3.org/TR/css3-color/#hsl-color
  // with s = 0.8f, l = 0.5
  float h = 0.666666f - (i * 0.666666f);
  
  return to_float3(h2rgb(h + 0.333333f), h2rgb(h), h2rgb(h - 0.333333f));
}

__KERNEL__ void AnnotatedReferenceJipi912Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  // Map input coordinate to [0, 1)
  float2 uv = fragCoord / iResolution;
  
  // Get the starting x for this bar by rounding down
  float barStart = _floor(uv.x * bars) / bars;
 
  // Discard pixels in the 'gap' between bars
  if(uv.x - barStart < barGap || uv.x > barStart + barSize - barGap) {
    fragColor = to_float4_s(0.0f);
  }
    else
    {
    // Sample spectrum in bar area, keep cumulative total
    float intensity = 0.0f;
    for(float s = 0.0f; s < barSize; s += barSize * sampleSize) {
      // Shader toy shows loudness at a given frequency at (f, 0) with the same value in all channels
      intensity += texture(iChannel0, to_float2(barStart + s, 0.0f)).x;
    }
    intensity *= sampleSize; // Divide total by number of samples taken (which is 1 / sampleSize)
    intensity = clamp(intensity, 0.005f, 1.0f); // Show silent spectrum to be just poking out of the bottom
    
    // Only want to keep this pixel if it is lower (screenwise) than this bar is loud
    float i = (float)(intensity > uv.y); // Casting a comparison to float sets i to 0.0f or 1.0
    
    //fragColor = to_float4_aw(intensityToColour(uv.x), 1.0f);       // Demo of HSL function
    //fragColor = to_float4_aw(i);                                  // Black and white output
    fragColor = to_float4_aw(intensityToColour(intensity) * i, i);  // Final output
  }
  // Note that i2c output is multiplied by i even though i is sent in the alpha channel
  // This is because alpha is not 'pre-multiplied' for fragment shaders, you need to do it yourself


  SetFragmentShaderComputedColor(fragColor);
}