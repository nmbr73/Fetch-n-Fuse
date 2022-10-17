
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__KERNEL__ void LiquidCassiniOvalsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Variante, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 

    float2 uv = fragCoord/iResolution;
    float t  = 0.5f * (1.0f + _sinf(iTime * 0.53f));
    float t2 = 0.5f * (1.0f + _sinf(iTime * 0.31f));
    float t3 = 0.5f * (1.0f + _sinf(iTime * 0.37f));
    float t4 = 0.5f * (1.0f + _sinf(iTime * 0.29f));
    float2 p = fragCoord;
    float maxDist = iResolution.y;
    float minX = iResolution.x / 6.0f;
    float xRange = iResolution.x - minX * 2.0f;
    float minY = iResolution.y / 6.0f;
    float yRange = iResolution.y - minY * 2.0f;
    float2 a = to_float2(minX + xRange * (1.0f - t2), iResolution.y / 3.0f);
    float2 b = to_float2(minX + xRange * t2, 2.0f * iResolution.y / 3.0f);
    float2 c = to_float2(iResolution.x / 2.0f, minY + yRange * t3);
    float2 e = to_float2(iResolution.x / 2.0f,  minY + yRange * (1.0f - t4));
    float r1 = length(p - a);
    float r2 = length(p - b);
    float r3 = length(p - c);
    float r4 = length(p - e);
    float d = iResolution.x / 10.0f + iResolution.x / 10.0f * _fabs(_sinf(iTime * 0.5f));
    float mx = iMouse.x / iResolution.x;
    float my = iMouse.y / iResolution.y;

    
    float distF = (1.0f - r1 * r2 * r3 * r4 / (d * d * d * d));
    float f = _fabs(0.5f - distF) / 0.5f;
    f = _powf(f, 0.3f + 2.0f * mx);
    float len = length(uv - to_float2(0.5f, 0.5f));
    
    float3 col = to_float3(0.7f - f * 0.7f, 1.0f - f, _fmaxf(0.0f, 0.3f + 0.7f - f * 0.7f)); //  //, 0.0f, f);

    if(Variante) col = to_float3_s(f < 0.5f + 0.5f * mx && f > 0.5f * my ? 1.0f : 0.0f);

    fragColor = to_float4_aw(col+(swi3(Color,x,y,z)-0.5f)*col.z,Color.w);

    
  SetFragmentShaderComputedColor(fragColor);
}