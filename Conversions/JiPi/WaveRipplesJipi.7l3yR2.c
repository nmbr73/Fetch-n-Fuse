
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0


__KERNEL__ void WaveRipplesJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_COLOR0(sunlightColor, 1.0f, 0.91f, 0.75f, 1.0f);
    CONNECT_SLIDER0(waveStrength, -1.0f, 1.0f, 0.02f);
    CONNECT_SLIDER1(waveSpeed, -1.0f, 10.0f, 5.0f);
    CONNECT_SLIDER2(frequency, -1.0f, 100.0f, 30.0f);
    CONNECT_SLIDER3(sunlightStrength, -1.0f, 10.0f, 5.0f);
    CONNECT_SLIDER4(Multiplier, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(Added, -10.0f, 10.0f, 1.0f);
    
    //Tweakable parameters
    //float waveStrength = 0.02f;
    //float frequency = 30.0f;
    //float waveSpeed = 5.0f;
    //float4 sunlightColor = to_float4(1.0f,0.91f,0.75f, 1.0f);
    //float sunlightStrength = 5.0f;
    //
    
    float2 tapPoint = to_float2(iMouse.x/iResolution.x,iMouse.y/iResolution.y);
    float2 uv = fragCoord / iResolution;
    float modifiedTime = iTime * waveSpeed;
    float aspectRatio = iResolution.x/iResolution.y;
    float2 distVec = uv - tapPoint;
    distVec.x *= aspectRatio;
    float distance = length(distVec);
    float2 newTexCoord = uv;
    
    //float multiplier = (distance < 1.0f) ? ((distance-1.0f)*(distance-1.0f)) : 0.0f;
    float multiplier = (distance < Multiplier) ? ((distance-Multiplier)*(distance-Multiplier)) : 0.0f;
    //float addend = (_sinf(frequency*distance-modifiedTime)+1.0f) * waveStrength * multiplier;
    float addend = (_sinf(frequency*distance-modifiedTime)+Added) * waveStrength * multiplier;
    newTexCoord += addend;    
    
    float4 colorToAdd = sunlightColor * sunlightStrength * addend;
    
  fragColor = _tex2DVecN(iChannel0,newTexCoord.x,newTexCoord.y,15) + colorToAdd;

  SetFragmentShaderComputedColor(fragColor);
}