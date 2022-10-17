
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/3405e48f74815c7baa49133bdc835142948381fbe003ad2f12f5087715731153.ogv' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void LustreCremeFromJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_SLIDER0(SINx, -1.0f, 1.0f, 0.018f);
    CONNECT_SLIDER1(COSx, -1.0f, 1.0f, 0.015f);
    CONNECT_SLIDER2(Tex2, -10.0f, 10.0f, 4.013f);
    

    fragCoord+=0.5f;
float AAAAAAAAAAA;
    float2 px = 4.0f/to_float2(640.0f,360.0f);
    float2 uv = fragCoord / iResolution;
    float4 tex = pow_f4(_tex2DVecN(iChannel0,uv.x,uv.y,15)*1.3f,to_float4_s(1.8f));
    float d = 1.0f-smoothstep(0.0f,0.08f,length(tex));
    //float d = _fabs(tex.y - newG);
    //tex.y = newG * 0.9f;
    if (d > 0.0f)
    {
        //px*= _sinf(iTime+swi2(uv,y,x)*3.0f)*0.35f;
        uv -= 0.5f*px;
        float4 tex2 = _tex2DVecN(iChannel1,uv.x,uv.y,15);
        uv += px;
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        //uv.x -= px.x -0.018f *_sinf(iTime*4.1f+tex2.x);
        //uv.y += px.y +0.015f * _cosf(iTime*4.1f+tex2.y);
        uv.x -= px.x -SINx *_sinf(iTime*4.1f+tex2.x);
        uv.y += px.y +COSx * _cosf(iTime*4.1f+tex2.y);
        
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        uv.y -= px.y;
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        tex2 /= Tex2;//4.013f;
        tex2 = clamp(tex2*1.02f-0.012f,0.0f,1.0f);
        tex = _fmaxf(clamp(tex*(1.0f-d),0.0f,1.0f), _mix(tex,tex2,smoothstep(-1.3f,0.23f,d)));
     }
        
  fragColor = tex;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void LustreCremeFromJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color1, 0.93f, 0.91f, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    float2 px = 2.5f / to_float2(640.0f,360.0f);
    float2 uv = fragCoord / iResolution;
    float4 tx = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    float dist = distance_f4(tx,_tex2DVecN(iChannel1,uv.x+px.x,uv.y+px.y,15));
    px.y *= -1.0f;
    dist += distance_f4(tx,_tex2DVecN(iChannel1,uv.x+px.x,uv.y+px.y,15));
    px.x *= -1.0f;
    dist += distance_f4(tx,_tex2DVecN(iChannel1,uv.x+px.x,uv.y+px.y,15));
    px.y *= -1.0f;
    dist += distance_f4(tx,_tex2DVecN(iChannel1,uv.x+px.x,uv.y+px.y,15));
    uv = mul_f2_mat2(uv,to_mat2(0.999f,0.001f,-0.001f,0.999f));
    fragColor = _tex2DVecN(iChannel0, uv.x*0.995f+0.0025f, uv.y*0.995f+0.0025f, 15)*Color1+
                to_float4(smoothstep(0.05f,1.3f,dist),smoothstep(0.1f,2.8f,dist),0.0f,1.0f)*0.245f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void LustreCremeFromJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  float2 uv = fragCoord / iResolution;
  fragColor = _fmaxf(_tex2DVecN(iChannel0,uv.x,uv.y,15), _tex2DVecN(iChannel1, uv.x+0.002f, uv.y+0.002f, 15));

  SetFragmentShaderComputedColor(fragColor);
}