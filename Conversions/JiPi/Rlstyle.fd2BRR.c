
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__KERNEL__ void RlstyleFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Wave, 0);
  
    if (Wave == 1)  Wave = iResolution.y-1;
  
    fragCoord+=0.5f; 
   
    //same as usual uv but we offset by one so we grab the previous frame/texture from Buf A one frame higher;
    float2 ouv = to_float2(fragCoord.x, fragCoord.y-1.0f) / iResolution;
    //not offset texture for grabbing "max" values
    float2 uv = to_float2(fragCoord.x, fragCoord.y) / iResolution;
    
    //conversion factor for our texture to sound texture
    int tx = (int)(fragCoord.x);
    
    //grab previous frame but offset by one pixel
    fragColor = _tex2DVecN(iChannel0,ouv.x,ouv.y,15);
    //old values for grabbing "max" values
    float4 fragColorOld = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    //get frequency data
    //float freq = texture( iChannel1, (make_float2(to_int2(tx,0))+0.5f)/iResolution).x;
    float freq = texture( iChannel1, (make_float2(to_int2(tx,Wave))+0.5f)/iResolution).x;
    //float freq = texture( iChannel1, fragCoord/iResolution).x;
   
    //only overwrite pixel if its the bottom one!
    //fragColor = _mix(fragColor, to_float4_aw(to_float3(freq), 1.0f), clamp(1.0f-fragCoord.y,0.0f,1.0f));
    
    //simpler code for overwriting third to bottom pixel
    if ((int)(fragCoord.y) == 2) {
        fragColor = to_float4_aw(to_float3_s(freq),1.0f);
    }
    //write max in second to bottom pixel
    if ((int)(fragCoord.y) == 1) {
        if (freq > fragColorOld.x) {
          fragColor = to_float4(freq, 0.0f, 0.0f,1.0f);
        } else {
            //reduce max over time
          fragColor = to_float4(fragColorOld.x-0.005f, 0.0f, 0.0f,1.0f);
        }
    }

//fragColor = to_float4(freq,0.0f,0.0f,1.0f);
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void RlstyleFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(base, 7.0f, 38.0f, 70.0f, 255.0f);
    CONNECT_COLOR1(baseBKG, 7.0f, 38.0f, 70.0f, 255.0f);
    
    baseBKG /=255.0f;
    base /=255.0f;
    
  
    fragCoord+=0.5f;
   
    //basic background
    //float4 base = to_float4(7.0f/255.0f, 38.0f/255.0f, 70.0f/255.0f, 1.0f);
    fragColor = baseBKG;
    
    //proper ratios
    float2 uv = fragCoord / iResolution;
    uv.y = uv.y*1.1f;
    uv.x = uv.x*2.0f - 0.45f;
    
    
    //lookup conversion (512 frequences returned by input)
    int tx = int(uv.x*512.0f);
    
    //bucketed values of current and max frequencies
    int starter = (int)(_floor((float)(tx)/57.0f))*57;
    int diff = tx-starter;
    float sum = 0.0f;
    float maxSum = 0.0f;
    for (int i = 0; i<9;i++) {

      sum    = sum + texture( iChannel0, (make_float2(to_int2(starter+i,2))+0.5f)/iResolution).x;
      maxSum = maxSum + texture( iChannel0, (make_float2(to_int2(starter+i,1))+0.5f)/iResolution).x;
    }
    
    //normalize values
    sum = (sum/9.0f);
    maxSum = (maxSum/9.0f);
    
    //Draw bars
    float height = sum;
    float col = ((sum)-0.2f)*1.25f;
    if (sum > uv.y && diff>20) {
        fragColor = to_float4(uv.y + base.x, uv.y+base.y, uv.y+base.z, 1.0f);
    }
    
    //draw "max" lines
    float mDiff = _fabs((uv.y+0.01f)-maxSum);
    float mVal = 1.0f-(mDiff*50.0f);
    if (mDiff<0.02f && diff>20 && maxSum > 0.001f) {
        fragColor = to_float4(_mix(fragColor.x,1.0f, mVal),_mix(fragColor.y, 1.0f, mVal),_mix(fragColor.z,1.0f,  mVal), 1.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}