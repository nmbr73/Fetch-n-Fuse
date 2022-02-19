
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/29de534ed5e4a6a224d2dfffab240f2e19a9d95f5e39de8898e850efdb2a99de.mp3' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define time iTime

__DEVICE__ float noise3D(float3 p)
{
  return fract(_sinf(dot(p ,to_float3(12.9898f,78.233f,12.7378f))) * 43758.5453f)*2.0f-1.0f;
}

__DEVICE__ float3 mixc(float3 col1, float3 col2, float v)
{
    v = clamp(v,0.0f,1.0f);
    return col1+v*(col2-col1);
}

__KERNEL__ void BasicAudioVisualizerFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord / iResolution;
    float2 p = uv*2.0f-1.0f;
    p.x*=iResolution.x/iResolution.y;
    p.y+=0.5f;
    
    float3 col = to_float3_s(0.0f);
    float3 ref = to_float3_s(0.0f);
   
    float nBands = 64.0f;
    float i = _floor(uv.x*nBands);
    float f = fract(uv.x*nBands);
    float band = i/nBands;
    band *= band*band;
    band = band*0.995f;
    band += 0.005f;
    float s = texture( iChannel0, to_float2(band,0.25f) ).x;
    
    /* Gradient colors and amount here */
    const int nColors = 4;
    float3 colors[nColors];  
    colors[0] = to_float3(0.0f,0.0f,1.0f);
    colors[1] = to_float3(0.0f,1.0f,1.0f);
    colors[2] = to_float3(1.0f,1.0f,0.0f);
    colors[3] = to_float3(1.0f,0.0f,0.0f);
    
    float3 gradCol = colors[0];
    float n = (float)(nColors)-1.0f;
    for(int i = 1; i < nColors; i++)
    {
      gradCol = mixc(gradCol,colors[i],(s-(float)(i-1)/n)*n);
    }
      
    col += to_float3_s(1.0f-smoothstep(0.0f,0.01f,p.y-s*1.5f));
    col *= gradCol;

    ref += to_float3_s(1.0f-smoothstep(0.0f,-0.01f,p.y+s*1.5f));
    ref*= gradCol*smoothstep(-0.5f,0.5f,p.y);
    
    col = _mix(ref,col,smoothstep(-0.01f,0.01f,p.y));

    col *= smoothstep(0.125f,0.375f,f);
    col *= smoothstep(0.875f,0.625f,f);

    col = clamp(col, 0.0f, 1.0f);

    float dither = noise3D(to_float3_aw(p,time))*2.0f/256.0f;
    col += dither;
    
  fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}