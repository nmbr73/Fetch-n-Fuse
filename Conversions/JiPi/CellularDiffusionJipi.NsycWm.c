
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float rand(float co)  { return fract(_sinf(co*(91.3458f)) * 47453.5453f); }
__DEVICE__ float rand(float2 co) { return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f); }

__DEVICE__ float randf(float2 uv, float t) {
  return rand(uv*(t+1.0f));
}

__DEVICE__ int colourr(float3 col) {
  //return int(dot(col, to_float3(0,1,2)));
  if (col.x > 0.5f) {
    return 0;
  } else if (col.y > 0.5f) {
    return 1;
  }
  return 2; 
}

__DEVICE__ float3 get_col(int k) {
  //k = k%3;
  if (k == 0) {
      return to_float3(1.0f,0.0f,0.0f);
  } else if (k == 1) {
      return to_float3(0.0f,1.0f,0.0f);
  } else {
      return to_float3(0.0f,0.0f,1.0f);
  }
  
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1




__KERNEL__ void CellularDiffusionJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord += 0.5f;
    
    float time = iTime;
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    float3 oldcol = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    int uvcol = colourr(oldcol);
    // slower to get all but whatever
    // get neighbouring colours
    int Ncol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(0,1))/iResolution),x,y,z));
    int Scol = colourr(swi3(texture(iChannel0,(fragCoord-to_float2(0,1))/iResolution),x,y,z));
    int Ecol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(1,0))/iResolution),x,y,z));
    int Wcol = colourr(swi3(texture(iChannel0,(fragCoord-to_float2(1,0))/iResolution),x,y,z));
    
    
    int NEcol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(1,1))/iResolution),x,y,z));
    int SEcol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(1,-1))/iResolution),x,y,z));
    int NWcol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(-1,1))/iResolution),x,y,z));
    int SWcol = colourr(swi3(texture(iChannel0,(fragCoord+to_float2(-1,-1))/iResolution),x,y,z));
    
    
    int prey = (uvcol+1)%3;
    int pred = (uvcol+2)%3;
    

    // sum neighbours
    int preyN = (int)(Ncol==prey) + (int)(Scol==prey) + (int)(Ecol==prey) + (int)(Wcol==prey);
    int predN = (int)(Ncol==pred) + (int)(Scol==pred) + (int)(Ecol==pred) + (int)(Wcol==pred);
    
    int predN2 = (int)(NEcol==pred) + (int)(SEcol==pred) + (int)(NWcol==pred) + (int)(SWcol==pred);
        
    float3 newcol;
    
    /*
    // another method, stable without rotating colours
    // but doesnt look good.
    // need to turn off rgb rotation in Image main
    float thresh = 2.5f;
    float kernal_diag = 1.0f;
    
    //if (time > 2.0f) {thresh = 3.0f;}
    if (float(predN) + kernal_diag*float(predN2) > thresh + randf(uv,time)*2.0f) {
      newcol = get_col(pred);
      //newcol = get_col(uvcol);
    } else {

      newcol = oldcol;
    }
    */
    
    
    
    // this code block almost behaves how i want, but it flashes
    // this is fixed by switching rgb order every other frame when printing
    // also has a weird miniture pattern that gives it a crt tv look.
    int thresh = 4;
    if ((predN2 + predN) < thresh) {
      newcol = get_col(pred);
    } else {
      newcol = oldcol;
    }
        
    // initialise to randomness
    if((int)(texture(iChannel1,to_float2(0,0)).y)==0) {
      float x = randf(uv,time)*3.0f;
      newcol = get_col((int)(x));
      //newcol = get_col(int(uv*3.0f));
    }

    // Output to screen
    fragColor = to_float4_aw(newcol,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// store an iteration number for switching frames
// store init flag
__KERNEL__ void CellularDiffusionJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
    fragCoord += 0.5f;  

    float2 uv = fragCoord / iResolution;
    float4 draw = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    if (fragCoord.x < 1.0f && fragCoord.y < 1.0f){
      draw.x = (float)((int)(draw.x + 1.0f) % 3);
      draw.y = 1.0f;
    }
    
    fragColor = draw;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void CellularDiffusionJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord += 0.5f;  

    float2 uv = fragCoord / iResolution;
    float4 draw = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    // normal draw
    fragColor = draw;
        
    // for the flashing image, rotate colour by frame num
    int col = colourr(swi3(draw,x,y,z));
    int rot = (int)(texture(iChannel1,to_float2(0,0)).x);
    fragColor = to_float4_aw(get_col((col-rot+2) % 3), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}