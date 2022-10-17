
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float2 EncodeForce(float2 force)
{
    force = clamp(force, -1.0f, 1.0f);
    return force * 0.5f + 0.5f;
}

__DEVICE__ float2 DecodeForce(float2 force)
{
    force = force * 2.0f - 1.0f;
    return force;
}

#define pi   3.14159265359f
#define tau  6.28318530718f

__DEVICE__ mat2 rot(float a) 
{
    float2 s = sin_f2(to_float2(a, a + pi/2.0f));
    return to_mat2(s.y,s.x,-s.x,s.y);
}

__DEVICE__ float linearStep(float a, float b, float x)
{
    return clamp((x - a)/(b - a), 0.0f, 1.0f);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Noise' to iChannel2


//Init Fluid


__DEVICE__ float4 GetNoise(float2 uv, float ratio, float3 noiseSpeed1, float noiseSize1, float3 noiseSpeed2, float noiseSize2, float iTime, __TEXTURE2D__ iChannel2 )
{
    float3 noiseCoord1;
    //swi2(noiseCoord1,x,y) = uv;
    noiseCoord1.x = uv.x;
    noiseCoord1.y = uv.y;
    noiseCoord1.x *= ratio;
    noiseCoord1 += iTime * noiseSpeed1;
    noiseCoord1 *= noiseSize1;
    
    float3 noiseCoord2;
    //swi2(noiseCoord2,x,y) = uv;
    noiseCoord2.x = uv.x;
    noiseCoord2.y = uv.y;
    noiseCoord2.x *= ratio;
    noiseCoord2 += iTime * noiseSpeed2;
    noiseCoord2 *= noiseSize2;
    
    float4 noise1 = _tex2DVecN(iChannel2,noiseCoord1.x,noiseCoord1.y,15);
    float4 noise2 = _tex2DVecN(iChannel2,noiseCoord2.x,noiseCoord2.y,15);
    
    float4 noise = (noise1 + noise2) / 2.0f;
    
    return noise;
}

__KERNEL__ void FireFireJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel2)
{

    fragCoord+=0.5f;
    
    CONNECT_COLOR0(NoiseSpeed1, -0.05f, 0.0f, 0.2f, 1.0f);
    CONNECT_SLIDER0(noiseSize1, -1.0f, 10.0f, 3.3f);
    CONNECT_COLOR1(NoiseSpeed2, -0.05f, 0.0f, 0.2f, 1.0f);
    CONNECT_SLIDER1(noiseSize2, -1.0f, 10.0f, 0.8f);
    
    float3 noiseSpeed1 = swi3(NoiseSpeed1,x,y,z); //to_float3(-0.05f, 0.0f, 0.2f);
    //const float noiseSize1 = 3.3f;
    float3 noiseSpeed2 = swi3(NoiseSpeed2,x,y,z); //to_float3(-0.05f, 0.0f, 0.2f);
    //const float noiseSize2 = 0.8f;
    
    const float circleForceAmount = 15.0f;
    const float2 randomForceAmount = to_float2(0.5f, 0.75f);
    const float2 upForce = to_float2(0.0f, 0.8f);
    const float2 moveSpeed = to_float2(1.0f, 2.0f);

    float ratio = iResolution.x / iResolution.y;
    float2 uv = fragCoord / iResolution;
    
    float2 circleCoord = uv;   
    float2 mousePos = to_float2_s(0.0f);
    float2 circleVelocity = to_float2_s(0.0f);
    
    if(iMouse.z > 0.5f)
    {     
        circleCoord -= swi2(iMouse,x,y)/iResolution;
    }
    else
    {
        circleCoord -= 0.5f;
        swi2S(circleCoord,x,y, swi2(circleCoord,x,y) + sin_f2(iTime * moveSpeed) * to_float2(0.35f, 0.25f));
    }
    
    circleCoord.x *= ratio;
    
    float circle = length(circleCoord);
    float bottom = uv.y;
    
    float4 masksIN = to_float4(0.08f, 0.35f, 0.05f, 0.2f);
    float4 masksOUT = to_float4(0.06f, 0.0f, 0.0f, 0.0f);
    float4 masksValue = to_float4(circle, circle, bottom, bottom);
    float4 masks = smoothstep(masksIN, masksOUT, masksValue);

    float2 mask = swi2(masks,x,y) + swi2(masks,z,w);
    
    float4 noise = GetNoise(uv, ratio, noiseSpeed1, noiseSize1, noiseSpeed2, noiseSize2, iTime, iChannel2);
        
    float2 force = circleCoord * swi2(noise,x,y) * circleForceAmount * masks.x;
    force += (swi2(noise,x,y) - 0.5f) * (masks.x * randomForceAmount.x + masks.z * randomForceAmount.y);
    force.y += (0.25f + 0.75f * noise.z) * (masks.x * upForce.x + masks.z * upForce.y);
    force = EncodeForce(force);
    
    fragColor = to_float4(force.x, force.y, mask.x, mask.y);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Noise' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel1

//Move Fluid

__KERNEL__ void FireFireJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    const float flow1 = 0.5f;
    const float flow2 = 0.75f;
    const float speed = 0.02f;
    const float gravity = -0.15f;

    float ratio = iResolution.x / iResolution.y;
    float2 uv = fragCoord / iResolution;

    float4 source = _tex2DVecN(iChannel0,uv.x,uv.y,15);
 
    float2 force = swi2(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y);
    force = DecodeForce(force);
    force.y -= gravity;
    
    float2 s = to_float2_s(speed);
    s.x /= ratio;
    force *= s;
    
    source.z = smoothstep(flow1, flow2, source.z);
    
    float2 movedForce = swi2(texture(iChannel1, uv - force),x,y);
    movedForce = _mix(movedForce, swi2(source,x,y), source.z);
    
    fragColor = to_float4(movedForce.x, movedForce.y, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: Noise' to iChannel2
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


//Update Fluid


#ifdef XXXX

__DEVICE__ float4 GetNoise(float2 uv, float ratio, float3 noiseSpeed1, float noiseSize1, float3 noiseSpeed2, float noiseSize2)
{
    float3 noiseCoord1;
    swi2(noiseCoord1,x,y) = uv;
    noiseCoord1.x *= ratio;
    noiseCoord1 += iTime * noiseSpeed1;
    noiseCoord1 *= noiseSize1;
    
    float3 noiseCoord2;
    swi2(noiseCoord2,x,y) = uv;
    noiseCoord2.x *= ratio;
    noiseCoord2 += iTime * noiseSpeed2;
    noiseCoord2 *= noiseSize2;
    
    float4 noise1 = _tex2DVecN(iChannel2,noiseCoord1.x,noiseCoord1.y,15);
    float4 noise2 = _tex2DVecN(iChannel2,noiseCoord2.x,noiseCoord2.y,15);
    
    float4 noise = (noise1 + noise2) / 2.0f;
    
    return noise;
}

#endif

__KERNEL__ void FireFireJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;
    
    CONNECT_COLOR3(NoiseSpeed1C, 0.0f, 0.1f, 0.2f, 1.0f);
    CONNECT_SLIDER3(noiseSize1C, -1.0f, 10.0f, 2.7f);
    CONNECT_COLOR4(NoiseSpeed2C, 0.0f, -0.1f, -0.2f, 1.0f);
    CONNECT_SLIDER5(noiseSize2C, -1.0f, 10.0f, 0.8f);
    

    const int Xiterations = 2;
    const int Yiterations = 2;

    const float sampleDistance1 = 0.006f;
    const float sampleDistance2 = 0.0001f;

    const float forceDamping = 0.01f;

    float3 noiseSpeed1 = swi3(NoiseSpeed1C,x,y,z); //to_float3(0.0f, 0.1f, 0.2f);
    //const float noiseSize1 = 2.7f;
    float3 noiseSpeed2 = swi3(NoiseSpeed1C,x,y,z); //to_float3(0.0f, -0.1f, -0.2f);
    //const float noiseSize2 = 0.8f;

    const float turbulenceAmount = 2.0f;


    float ratio = iResolution.x / iResolution.y;
    float2 uv = fragCoord / iResolution;
    
    float4 source = _tex2DVecN(iChannel0,uv.x,uv.y,15);    
    
    float2 currentForce = DecodeForce(swi2(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y));
    float currentForceMagnitude = length(currentForce);
       
    float3 sampleDistance; 
    //swi2(sampleDistance,x,y) = to_float2(_mix(sampleDistance1, sampleDistance2, smoothstep(-0.25f, 0.65f, currentForceMagnitude)));
    sampleDistance.x = _mix(sampleDistance1, sampleDistance2, smoothstep(-0.25f, 0.65f, currentForceMagnitude));
    sampleDistance.y = _mix(sampleDistance1, sampleDistance2, smoothstep(-0.25f, 0.65f, currentForceMagnitude));
    
    
    sampleDistance.z = 0.0f;
    
    float2 totalForce = to_float2_s(0.0f);
    float iterations = 0.0f;
    
    for(int x = -Xiterations; x <= Xiterations; x++)
    {
        for(int y = -Yiterations; y <= Yiterations; y++)
        {
            float3 dir = to_float3((float)(x), (float)(y), 0.0f);
            float4 sampledValue = texture(iChannel1, uv + swi2(dir,x,y) * swi2(sampleDistance,x,y));
            
            float2 force = DecodeForce(swi2(sampledValue,x,y)); 
            float forceValue = length(force);
            totalForce += force * forceValue;
            iterations += forceValue;
        }
    }
    
    totalForce /= iterations;  
    totalForce -= totalForce * forceDamping;
    
    float turbulence = GetNoise(uv, ratio, noiseSpeed1, noiseSize1C, noiseSpeed2, noiseSize2C, iTime, iChannel2).z - 0.5f;
    turbulence *= _mix(0.0f, turbulenceAmount, smoothstep(0.0f, 1.0f, currentForceMagnitude));
    
    totalForce = mul_f2_mat2(totalForce, rot(turbulence));
    totalForce = EncodeForce(totalForce);
    
    fragColor = to_float4(totalForce.x, totalForce.y, 0.0f, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


__DEVICE__ float3 gradient(float value, float3 color1, float3 color2, float3 color3, float3 color4, float3 color5, float a, float b, float c)
{
    float4 start = to_float4(0.0f, a, b, c);
    float4 end = to_float4(a, b, c, 1.0f);
    float4 mixValue = smoothstep(start, end, to_float4_s(value));
    
    float3 color = _mix(color1, color2, mixValue.x);
    color = _mix(color, color3, mixValue.y);
    color = _mix(color, color4, mixValue.z);
    color = _mix(color, color5, mixValue.w);
    
    return color;
}

__KERNEL__ void FireFireJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 uv = fragCoord / iResolution;
float IIIIIIIIIIIIIIIII;    
    const float3 color1 = to_float3(0.0f, 0.05f, 0.2f);
    const float3 color2 = to_float3(0.1f, 0.0f, 0.1f);
    const float3 color3 = to_float3(0.5f, 0.15f, 0.25f);
    const float3 color4 = to_float3(2.0f, 1.25f, 0.7f);
    const float3 color5 = to_float3(2.0f, 2.0f, 2.0f);

    const float3 glowColor1 = to_float3(1.5f, 0.5f, 0.0f);
    const float3 glowColor2 = to_float3(1.5f, 1.5f, 0.5f);

    const float3 lightColor = to_float3(1.0f, 1.5f, 0.75f);
    const float3 lightDirection = normalize(to_float3(0.0f, -1.0f, 0.0f));

    const float a = 0.125f;
    const float b = 0.35f;
    const float c = 0.5f;
    
    
    float4 source = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float2 force = swi2(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y);
    force = DecodeForce(force);
    
    float value = length(force);
    
    float glow = source.w + source.z * 0.75f;
    glow /= 2.0f;
    
    float3 color = gradient(value, color1, color2, color3, color4, color5, a, b, c);
    color += _mix(glowColor1, glowColor2, glow) * glow;
    
    float3 normal = to_float3(force.x, force.y, 1.0f) * 0.5f;
    normal = normalize(normal);
    
    float NdotL = smoothstep(-0.5f, 0.5f, dot(normal, lightDirection));
    color += color * NdotL * lightColor;
    
    fragColor = to_float4_aw(color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}