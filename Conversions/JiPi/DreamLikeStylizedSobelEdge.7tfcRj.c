// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel0


__KERNEL__ void DreamLikeStylizedSobelEdgeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float kernelSize = 6.0f;
    float3 col = to_float3_s(0.0f);
    for(float i = 0.0f; i < kernelSize; i+=1.0f){
        float pointer = i - _floor(kernelSize * 0.5f);
        col += swi3(texture(iChannel0, uv + to_float2(i,0.0f)/iResolution ),x,y,z);
    }
    col/= kernelSize;
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void DreamLikeStylizedSobelEdgeFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
  
    float2 uv = fragCoord/iResolution;
    float kernelSize = 6.0f;
    float3 col = to_float3_s(0.0f);
    for(float i = 0.0f; i < kernelSize; i+=1.0f){
        float pointer = i - _floor(kernelSize * 0.5f);
        col += swi3(texture(iChannel0, uv + to_float2(0.0f,i)/iResolution ),x,y,z);
    }
    col/= kernelSize;
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: London' to iChannel0


__KERNEL__ void DreamLikeStylizedSobelEdgeFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float kernelSize = 20.0f;
    float3 col = to_float3_s(0.0f);
    for(float i = 0.0f; i < kernelSize; i+=1.0f){
        float pointer = i - _floor(kernelSize * 0.5f);
        col += swi3(texture(iChannel0, uv + to_float2(i,0.0f)/iResolution),x,y,z);
    }
    col/= kernelSize;
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void DreamLikeStylizedSobelEdgeFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
  
    float2 uv = fragCoord/iResolution;
    float kernelSize = 20.0f;
    float3 col = to_float3_s(0.0f);
    for(float i = 0.0f; i < kernelSize; i+=1.0f){
        float pointer = i - _floor(kernelSize * 0.5f);
        col += swi3(texture(iChannel0, uv + to_float2(0.0f,i)/iResolution),x,y,z);
    }
    col/= kernelSize;
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel2


__DEVICE__ float greyScale(float3 color){
    return (color.x + color.y + color.z) / 3.0f;
}
__DEVICE__ float3 post(float3 color){
    return _floor(color*15.0f+0.5f)/15.0f;
}


__KERNEL__ void DreamLikeStylizedSobelEdgeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_COLOR0(Color1, 0.4f, 0.2f, 0.4f, 1.0f); 
    CONNECT_COLOR1(Color2, 0.5f, 0.7f, 0.8f, 1.0f); 
    CONNECT_SLIDER0(OG, 0.0f, 5.0f, 2.2f);
    CONNECT_SLIDER1(threshold, 0.0f, 2.0f, 0.14f);
    CONNECT_CHECKBOX0(Special, 0);
    CONNECT_SLIDER2(Orientation, -10.0f, 30.0f, 1.0f);
  
  
    fragCoord+=0.5f;
    
    float Xsobel_kernel[9] =
           {-1.0f, 0.0f, 1.0f,
            -2.0f, 0.0f, 2.0f,
            -1.0f, 0.0f, 1.0f};
            
    float Ysobel_kernel[9] =
          {-1.0f, -2.0f, -1.0f,
            0.0f, 0.0f, 0.0f,
            1.0f, 2.0f, 1.0f};
            
    float2 uv = fragCoord/iResolution;
    float3 og = swi3(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y,z);
    og = pow_f3(og,to_float3_s(1.0f/2.2f));
    og = post(og);
    //float3 pat = _mix(to_float3(0.4f,0.2f,0.4f), to_float3(0.5f,0.7f,0.8f), uv.x+uv.y);
    float3 pat = _mix(swi3(Color1,x,y,z), swi3(Color2,x,y,z), uv.x+uv.y);
    og = _mix(pat, og, greyScale(og)*OG);//*2.2f);
    
    //float threshold = 0.14f;
    float Xedge = 0.0f;
    float Yedge = 0.0f;
    float kernelPointer = 0.0f;
    for(float x = 0.0f; x < 3.0f; x+=1.0f){
        for(float y = 0.0f; y < 3.0f; y+=1.0f){
            float result = greyScale(swi3(texture(iChannel0, uv+to_float2(x-1.0f,y-1.0f)/iResolution),x,y,z));
            Xedge += result * Xsobel_kernel[(int)(kernelPointer)];
            kernelPointer++;
        }
    }
    kernelPointer = 0.0f;
    for(float x = 0.0f; x < 3.0f; x+=1.0f){
        for(float y = 0.0f; y < 3.0f; y+=1.0f){
            float result = greyScale(swi3(texture(iChannel0, uv+to_float2(x-1.0f,y-1.0f)/iResolution),x,y,z));
            Yedge += result  * Ysobel_kernel[(int)(kernelPointer)];
            kernelPointer++;
        }
    }
   
   //remapping values
   //Xedge += 0.5f;
   //Yedge += 0.5f;
   float finalG = _sqrtf(Xedge*Xedge + Yedge*Yedge);
   if(finalG < threshold){
       finalG = 0.0f; 
   }
    
  
   float edgeOrientation = _atan2f(Yedge/Xedge, Orientation);// * Orientation;//1.0f);
   float g = edgeOrientation;
   float3 col = to_float3_s(finalG);
   
   if(Special)
     col = to_float3_s(g);
   
   fragColor = to_float4_aw(og-col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}