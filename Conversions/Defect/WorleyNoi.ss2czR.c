
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0




__DEVICE__ float permuteX(float x)
{
  float modValue = 512.0f;
    float t = ((x * 67.0f) + 71.0f) * x;
  return mod_f(t, modValue);
}

__DEVICE__ float permuteY(float x)
{
  float modValue = 512.0f;
    float t = ((x * 73.0f) + 83.0f) * x;
  return mod_f(t, modValue);
}

__DEVICE__ float shiftX(float value)
{
    return fract(value * (1.0f / 73.0f));
}

__DEVICE__ float shiftY(float value)
{
    return fract(value * (1.0f / 69.0f));
}

__DEVICE__ float2 rand(float2 v)
{
    float modValue = 512.0f;
    v = mod_f(v, modValue);
    float rX = permuteX(permuteX(v.x) + v.y);
    float rY = permuteY(permuteY(v.x) + v.y);
    return to_float2(shiftX(rX), shiftY(rY));
}

__DEVICE__ float worleyNoise(float2 uv)
{
    float2 p = _floor(uv);
    float2 f = fract_f2(uv);
    float dis = 1e9f;
    int range = 1;
    
    float2 findPos, findJitPos;
    for(int i = -range; i <= range; i++)
    {
        for(int j = -range; j <= range; j++)
        {
            float2 b = to_float2(i, j);
            float2 jitPos = b - f + rand(p + b);
            float len = dot(jitPos, jitPos);
            if (dis > len)
            {
                dis = len;
                findPos = b;
                findJitPos = jitPos;
            }
        }
    }
    
    dis = 1e9f;
    range = 2;
    for(int i = -range; i <= range; i++)
    {
        for(int j = -range; j <= range; j++)
        {
            float2 b = findPos + to_float2(i, j);
            float2 jitPos = b - f + rand(p + b);
            float len = dot((findJitPos + jitPos) * 0.5f, normalize(jitPos - findJitPos));
            
            if (dis > len)
            {
                dis = len;
            }
        }
    }
    
     //return smoothstep( 0.0f, 0.5f, dis );
    return dis;
}

__DEVICE__ float2 noise(float2 uv)
{
    float2 p = _floor(uv);
    float2 f = fract_f2(uv);
    
    float2 v = f * f * (3.0f - 2.0f * f);
    
    float2 result = _mix(
                    _mix(rand(p + to_float2(0.0f, 0.0f)), rand(p + to_float2(1.0f, 0.0f)), v.x), 
                    _mix(rand(p + to_float2(0.0f, 1.0f)), rand(p + to_float2(1.0f, 1.0f)), v.x), 
                    v.y);
    
    return result - 0.5f;
}

__DEVICE__ float2 fbm(float2 uv)
{
    float2 res = to_float2_s(0.0f);
    float s = 0.25f;
    for(int i = 0; i < 5; i++)
    {
    uv *= 2.0f;
        res += s * noise(uv);
        s *= 0.5f;
    }
    return res;
}

__KERNEL__ void WorleyNoiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/swi2(iResolution,x,y);
    
    uv.x*=1.77f;
    
    uv *= 4.0f;
    uv += 0.0f;
    
    float2 I = _floor(uv/2.0f); 
    bool vert = mod_f(I.x+I.y,2.0f)==0.0f; 
    
    float result;
    for(float i = 0.0f; i < 3.0f; i += 01.0f)
    {
        uv *= 1.5f;
        float2 jituv = fbm(uv * 1.5f);
        float col = worleyNoise(uv + jituv);
        
        float ground = _fminf(1.0f, 1.8f * _powf(col, 0.22f));
        result += (1.0f - ground) / _exp2f(i);
    }

    if (vert) result = 1.0f-result; 
    
    float4 tex = _tex2DVecN(iChannel0, fragCoord.x/iResolution.x,fragCoord.y/iResolution.y,15);
    
    //fragColor = result + tex;
    

    fragColor = to_float4_aw(to_float3_s(result), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}