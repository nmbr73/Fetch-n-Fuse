
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Video' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void InkGhostJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{

    CONNECT_CHECKBOX1(ManualKeying, 0);
    CONNECT_COLOR3(KeyingColor, 0.051f, 0.64f, 0.145f, 1.0f);
    CONNECT_COLOR4(OffsetColor, 0.8f, 0.8f, 0.8f, 1.0f);
    //CONNECT_SLIDER11(Offset, -1.0f, 1.0f, 0.8f);

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    
    float4 cam = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    
    float keying = smoothstep(0.0f,1.0f,distance_f3(swi3(cam,x,y,z), to_float3(13.0f/255.0f,163.0f/255.0f,37.0f/255.0f))*1.0f);
    if (ManualKeying)
      keying = smoothstep(0.0f,1.0f,distance_f3(swi3(cam,x,y,z), swi3(KeyingColor,x,y,z))*1.0f);
    
    
    fragColor = to_float4_aw(clamp(swi3(cam,x,y,z) + swi3(OffsetColor,x,y,z), to_float3_s(0.0f), to_float3_s(1.0f))*keying, keying);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel3




__KERNEL__ void InkGhostJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;

    CONNECT_SLIDER6(sampleDistance, -1.0f, 100.0f, 30.0f);
    CONNECT_SLIDER7(diffusion, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER8(turbulence, -1.0f, 1.0f, 0.2f);
    CONNECT_SLIDER9(fluidify, -1.0f, 1.0f, 0.1f);
    CONNECT_SLIDER10(attenuate, -1.0f, 1.0f, 0.005f);

    //float sampleDistance = 30.0f;
    //float diffusion = 1.0f;
    //float turbulence = 0.2f;
    //float fluidify = 0.1f;
    //float attenuate = 0.005f;

    float2 uv = (fragCoord/iResolution);
    
    float4 baseColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float2 sDist = sampleDistance/iResolution;
    
    float4 newColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    float2 turb = (swi2(_tex2DVecN(iChannel3,uv.x,uv.y,15),x,y)*2.0f-1.0f);

    float4 newColor1 = texture(iChannel1, uv + to_float2(1.0f,0.0f)*sDist);
    float4 newColor2 = texture(iChannel1, uv + to_float2(-1.0f,0.0f)*sDist);
    float4 newColor3 = texture(iChannel1, uv + to_float2(0.0f,1.0f)*sDist);
    float4 newColor4 = texture(iChannel1, uv + to_float2(0.0f,-1.0f)*sDist);
    
    float4 newColor5 = texture(iChannel1, uv + to_float2(1.0f,1.0f)*sDist);
    float4 newColor6 = texture(iChannel1, uv + to_float2(-1.0f,1.0f)*sDist);
    float4 newColor7 = texture(iChannel1, uv + to_float2(1.0f,-1.0f)*sDist);
    float4 newColor8 = texture(iChannel1, uv + to_float2(-1.0f,-1.0f)*sDist);
     
    float2 t = (newColor1.x+newColor1.y+newColor1.z)/3.0f * to_float2(1.0f,0.0f);
    t += (newColor2.x+newColor2.y+newColor2.z)/3.0f * to_float2(-1.0f,0.0f);
    t += (newColor3.x+newColor3.y+newColor3.z)/3.0f * to_float2(0.0f,1.0f);
    t += (newColor4.x+newColor4.y+newColor4.z)/3.0f * to_float2(0.0f,-1.0f);
    
    t += (newColor5.x+newColor5.y+newColor5.z)/3.0f * to_float2(1.0f,1.0f);
    t += (newColor6.x+newColor6.y+newColor6.z)/3.0f * to_float2(-1.0f,1.0f);
    t += (newColor7.x+newColor7.y+newColor7.z)/3.0f * to_float2(1.0f,-1.0f);
    t += (newColor8.x+newColor8.y+newColor8.z)/3.0f * to_float2(-1.0f,-1.0f);
    
    t /= 8.0f;
    float2 m = swi2(iMouse,x,y)/iResolution;
    float2 dir = (t+turb*turbulence)*iTimeDelta*diffusion*(m.x*2.0f-1.0f);
    
    float4 res = texture(iChannel1, uv + dir);
    
    if(iFrame < 10 || Reset ) //texture(iChannel2, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f)
    {
      fragColor =  baseColor;
    }
    else
    {
      fragColor = _mix(res, baseColor, clamp(baseColor.w*fluidify + attenuate,0.0f,1.0f));
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------




__DEVICE__ float hash(float n)
{
   return fract(_sinf(dot(to_float2(n,n) ,to_float2(12.9898f,78.233f))) * 43758.5453f);  
}  

__DEVICE__ float2 turbulence(float2 uv, float2 speed, float v, float dist, float random1, float random2, float iTime)
{
    float2 turb;
    turb.x = _sinf(uv.x);
    turb.y = _cosf(uv.y);
    
    for(int i = 0; i < 10; i++)
    {
        float ifloat = 1.0f + (float)(i);
        float ifloat1 = ifloat + random1;
        float ifloat2 = ifloat + random2; 
        
        float r1 = hash(ifloat1)*2.0f-1.0f;
        float r2 = hash(ifloat2)*2.0f-1.0f;
        
        float2 turb2;
        turb2.x = _sinf(uv.x*(1.0f + r1*v) + turb.y*dist*ifloat + iTime*speed.x*r2);
        turb2.y = _cosf(uv.y*(1.0f + r1*v) + turb.x*dist*ifloat + iTime*speed.y*r2);
        
        turb.x = _mix(turb.x, turb2.x, 0.5f);
        turb.y = _mix(turb.y, turb2.y, 0.5f);
    }
    
    return turb;
}

__KERNEL__ void InkGhostJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    fragCoord+=0.5f;

    CONNECT_POINT0(Speed, 0.0f, 0.0f);
    CONNECT_SLIDER2(v, -1.0f, 100.0f, 30.0f);
    CONNECT_SLIDER3(dist, -1.0f, 1.0f, 0.3f);
    CONNECT_SLIDER4(random1, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(random2, -1.0f, 1000.0f, 100.0f);


    float2 speed = to_float2(1.0f,2.0f) + Speed;
    //float v = 30.0f;
    //float dist = 0.3f;
    //float random1 = 1.0f;
    //float random2 = 100.0f;

    float ratio = iResolution.x/iResolution.y;
    float2 uv = fragCoord/iResolution;
    uv.x *= ratio;

    float2 turb = turbulence(uv, speed, v, dist, random1, random2, iTime)*0.5f+0.5f;
    
    fragColor = to_float4(turb.x, turb.y, 0.0f, 0.0f);
      
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Video' to iChannel1
// Connect Image 'Texture: Lichen' to iChannel2
// Connect Image 'Previsualization: Buffer B' to iChannel0

__KERNEL__ void InkGhostJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
  
    CONNECT_COLOR0(color1, 1.0f, 0.25f, 0.15f, 1.0f);
    CONNECT_COLOR1(color2, 0.5f, 0.1f, 0.1f, 1.0f);
    CONNECT_COLOR2(color3, 0.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_SLIDER0(multiplier, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(midPosition, -1.0f, 10.0f, 0.5f);
  
    //float4 color1 = to_float4(1.0f,0.25f,0.15f,1.0f);
    //float4 color2 = to_float4(0.5f,0.1f,0.1f,1.0f);
    //float4 color3 = to_float4(0.0f,0.0f,0.0f,1.0f);
    //float multiplier = 1.0f;
    //float midPosition = 0.5f;

    float2 uv = fragCoord / iResolution;
    float4 c = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    uv.x *= iResolution.x/iResolution.y;
    
    float4 tex = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    
    float l = clamp(((c.x+c.y+c.z)/3.0f)*multiplier,0.0f,1.0f);
    
    float4 res1 = _mix(color1, color2, smoothstep(0.0f,midPosition,l));
    float4 res2 = _mix(res1, color3, smoothstep(midPosition,1.0f,l));
    
    fragColor = res2*(0.9f + tex*0.1f);

  SetFragmentShaderComputedColor(fragColor);
}