
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void VideoOnFFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 px = 3.0f/to_float2(640.0f,360.0f);
    float2 uv = fragCoord / iResolution;
    float4 tex = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float newG = _fminf(tex.y,_fmaxf(tex.x,tex.z));
    float d = _fabs(tex.y - newG);
    tex.y = newG * 0.9f;
    if (d > 0.0f)
    {
        //px*= _sinf(iTime+swi2(uv,y,x)*3.0f)*0.35f;
        uv -= 0.5f*px;
        float4 tex2 = _tex2DVecN(iChannel1,uv.x,uv.y,15);
        uv += px;
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        uv.x -= px.x -0.018f *_sinf(iTime*4.1f+tex2.x);
        uv.y += px.y +0.015f * _cosf(iTime*4.1f+tex2.y);
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        uv.y -= px.y;
        tex2 += _tex2DVecN(iChannel1,uv.x,uv.y,15);
        tex2 /= 4.013f;
        tex2 = clamp(tex2*1.02f-0.012f,0.0f,1.0f);
        tex = _fmaxf(clamp(tex*(1.0f-d),0.0f,1.0f),_mix(tex,tex2,smoothstep(-0.3f,0.23f,d)));
     }
        
  fragColor = tex;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define distance_f4(p1, p2) length(p1 - p2)

__KERNEL__ void VideoOnFFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 px = 1.5f / to_float2(640.0f,360.0f);
    float2 uv = fragCoord / iResolution;
    float4 tx = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    float dist = distance_f4(tx,texture(iChannel1,uv+px));
    px.y *= -1.0f;
    dist += distance_f4(tx,texture(iChannel1,uv+px));
    px.x *= -1.0f;
    dist += distance_f4(tx,texture(iChannel1,uv+px));
    px.y *= -1.0f;
    dist += distance_f4(tx,texture(iChannel1,uv+px));
    uv = mul_f2_mat2(uv,to_mat2(0.99f,0.01f,-0.001f,0.99f));
    fragColor = texture(iChannel0,uv+0.002f)*to_float4(0.91f,0.847f,0.0f,0.0f)+
                to_float4(smoothstep(0.3f,0.8f,dist),smoothstep(0.3f,1.4f,dist),0.0f,1.0f)*0.175f;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void VideoOnFFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  float2 uv = fragCoord / iResolution;
  fragColor = _fmaxf(_tex2DVecN(iChannel0,uv.x,uv.y,15),texture(iChannel1,uv+0.002f));


  SetFragmentShaderComputedColor(fragColor);
}