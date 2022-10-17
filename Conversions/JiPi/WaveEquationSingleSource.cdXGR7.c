
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void WaveEquationSingleSourceFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse, float iTimeDelta, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0);

    CONNECT_SLIDER1(MouseS, -1.0f, 1.0f, 0.4f);
    CONNECT_SLIDER2(MouseWaveSrc, -1.0f, 1.0f, 0.3f);
    
    CONNECT_SLIDER3(waveSrcSize, -1.0f, 1.0f, 0.005f);
    CONNECT_SLIDER4(waveSrcFreq, -1.0f, 10.0f, 5.1f);
    CONNECT_SLIDER5(waveSpeed, -1.0f, 10.0f, 3.3f);
    CONNECT_SLIDER6(dampRate, -1.0f, 1.0f, 0.3f);
    
    CONNECT_SLIDER7(HMul, -1.0f, 100.0f, 25.0f);


    fragCoord+=0.5f;

    fragColor = to_float4(0.0f,0.0f,0.0f,1.0f);
    float2 waveSrc = swi2(iMouse,x,y) / to_float2_s(iResolution.y);
    if (iMouse.x == 0.0f && iMouse.y == 0.0f)
    {
        float s = MouseS;//0.4f;
        waveSrc = MouseWaveSrc * to_float2(_cosf(iTime*s),_sinf(2.0f*iTime*s))  + iResolution/iResolution.y*0.5f;
    }
    //float waveSrcSize = 0.005f;
    //float waveSrcFreq = 5.1f;
      //float waveLength = 2.0f;
    //float waveSpeed = 3.3f;
    //float dampRate = 0.3f;
    
    float2 uv = fragCoord / iResolution.y;
    
    float d2 = dot(uv - waveSrc,uv-waveSrc);
    swi2S(fragColor,x,y, _expf(-d2/(waveSrcSize*waveSrcSize)) * to_float2(_sinf(waveSrcFreq*iTime),waveSrcFreq*_cosf(waveSrcFreq*iTime)));
    
    //float4 prevPixel = texelFetch(iChannel0,to_int2(fragCoord),0);
    float4 prevPixel = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
    float h = prevPixel.x;
    float dh_dt = prevPixel.y;
    
    float L = 0.0f;
    for ( int i=0;i<5;++i)
    {
        for ( int j=0;j<5;++j)
        {
            //L -= texelFetch(iChannel0,to_int2(int(fragCoord.x)+i-2,int(fragCoord.y)+j-2),0).x;
            L -= texture(iChannel0, (make_float2(to_int2((int)(fragCoord.x)+i-2,(int)(fragCoord.y)+j-2))+0.5f)/iResolution).x;
        }
    }
    L += HMul*h;//25.0f*h;
    
    float d2h_dt2 = -L*(waveSpeed*waveSpeed);
    dh_dt += d2h_dt2 * iTimeDelta;
    h += dh_dt * iTimeDelta;
    
    h *= _expf(-iTimeDelta*dampRate);
    
    fragColor.x += h;
    fragColor.y += dh_dt;

  if(iFrame < 1 || Reset)
    fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void WaveEquationSingleSourceFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color1, 0.5f, 0.5f, 0.5f, 0.5f);
    CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.1f);
  
    fragCoord+=0.5f;

    //fragColor = texelFetch(iChannel0,to_int2(fragCoord),0)*0.1f+to_float4_s(0.5f);
    //fragColor = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution)*0.1f+to_float4_s(0.5f);
    fragColor = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution)*0.1f + Color1;

  SetFragmentShaderComputedColor(fragColor);
}
