
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define A 0.0f // Amplitude
#define V 8.0f // Velocity
#define W 9.0f // Wavelength
#define T 0.07f // Thickness
#define S 2.0f // Sharpness
#define GAP to_float2(0.006f, 0.008f) //gap for edge

__DEVICE__ float sine(float2 p){
    return _powf(T / _fabs((p.y + 0.0f)), S);
}

__DEVICE__ bool detectEdge(float2 fCoord, float2 gap) {
    float2 uv = swi2(fCoord,x,y) / iResolution;
    float2 edgeDistance = 0.5f -  _fabs(uv - 0.5f);
    
    bvec2 edgeCompare = lessThan(edgeDistance, gap);
    bool isEdge = edgeCompare.x || edgeCompare.y;
    
    return isEdge;
}

__DEVICE__ float blurEdge(float2 fCoord, float2 gap) {
    float2 uv = swi2(fCoord,x,y) / iResolution;
    
    if(uv.y > (1.0f - gap.y)) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.y - (1.0f - gap.y)), S);
        return blurValue;
    } 
    
    if(uv.y < gap.y) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.y - gap.y), S);
        return blurValue;
    }
    
    if(uv.x > (1.0f - gap.x)) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.x - (1.0f - gap.x)), S);
        return blurValue;
    } 
    
    if(uv.x < gap.x) {
      float blurValue = 1.0f - _powf(T / _fabs(uv.x - gap.x), S);
        return blurValue;
    } 
    
    return 1.0f;
}

__KERNEL__ void MationiRandomColorBorderFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    if(detectEdge(fragCoord, GAP)) {
        
        // Normalized pixel coordinates (from 0 to 1)
        float2 uv = fragCoord/iResolution;

        // Time varying pixel color
        float3 col = 0.5f + 0.5f*_cosf(iTime+swi3(uv,x,y,x)+to_float3(0,2,4));

        fragColor = to_float4_aw(col,1.0f);
    } else {
      fragColor = to_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }


  SetFragmentShaderComputedColor(fragColor);
}