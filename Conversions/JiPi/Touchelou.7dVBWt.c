
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Stars' to iChannel0
// Connect Image '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel2
// Connect Image 'Texture: Pebbles' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void TouchelouFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_COLOR1(ColorTex, 0.2f, 0.6f, 2.0f, 1.0f); 
    CONNECT_COLOR2(ColorBeat, 0.5f, 0.5f, 0.5f, 0.5f); 
    CONNECT_INTSLIDER0(samples, 1, 20, 5);
    CONNECT_SLIDER0(Intense, -10.0f, 10.0f, 1.0f);
    
    CONNECT_CHECKBOX0(AnimTex, 1);

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    uv /= to_float2(iResolution.y / iResolution.x, 1);
    
    // bass detection
    // the sound texture is 512x2
    int tx = (int)(uv.x*512.0f);
    

  // crude beat detection
    float bass = 0.0f;
    //int samples = 5;
    for(int i = 0 ; i < samples ; ++i) {
        //bass += texelFetch( iChannel2, to_int2(i,0), 0 ).x;
        bass += texture( iChannel2, (make_float2(to_int2(i,0))+0.5f)/iResolution ).x;
    }
    bass /= (float)(samples);
    float beat = smoothstep(0.8f, 1.0f, bass);
    

    float2 uv2 = uv + to_float2(iTime / 50.0f, iTime / 100.0f);

    float ratio = iResolution.y/iResolution.x;

    if (!AnimTex) uv2 = fragCoord/iResolution, ratio = 1.0f;
    

    float intens = _tex2DVecN(iChannel1,uv2.x*ratio,uv2.y,15).x * Intense;
 
    float anim = iTime / 10.0f;
    
 
    float2 def = to_float2(_sinf(intens + anim), _cosf(intens + anim / 2.0f));
    def -= 0.2f;

    float4 tex = texture(iChannel0, uv + def);
     
    tex *= ColorTex; //to_float4(0.2f, 0.6f, 2.0f, 1.0f);
    tex *= to_float4(1.0f, 1.0f, beat + 0.5f, 1.0f) + ColorBeat-0.5f;

    fragColor = to_float4_aw(swi3(tex,x,y,z) + swi3(Color,x,y,z)-0.5f, Color.w);


  SetFragmentShaderComputedColor(fragColor);
}