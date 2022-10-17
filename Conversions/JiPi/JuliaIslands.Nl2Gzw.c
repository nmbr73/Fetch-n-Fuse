
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// water simulation
// waterlevel at 0.0
// xyzw: h1, h2, landscape height, 0

//float waterlevel = 0.5f;
#define pi 3.14159265359f


__DEVICE__ float hit(in float2 fragCoord, float4 iMouse) {
    float h = smoothstep(2.0f, 1.0f, distance_f2(fragCoord, swi2(iMouse,x,y)));
    return h;
}
__DEVICE__ float rain(in float2 fragCoord, float2 iResolution, float iTime) {
    float2 dropPos = to_float2((_sinf(iTime*pi*161.8f)*0.5f+0.5f)*iResolution.x, (_cosf(iTime*pi*161.0f)*0.5f + 0.5f)*iResolution.y);
    float h = smoothstep(2.0f, 1.0f, distance_f2(fragCoord, dropPos));
    return h;
    //return smoothstep(0.99f, 1.0f, _sinf(fragCoord.x*iTime)*_cosf(fragCoord.y*iTime))*1.0f;
}

__DEVICE__ float2 simStep(in float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0) {
   if(texture(iChannel0, fragCoord/iResolution).z < 0.0f) {
       return to_float2(((texture(iChannel0, (fragCoord + to_float2(-1,0))/iResolution).x +
                          texture(iChannel0, (fragCoord + to_float2( 1,0))/iResolution).x +
                          texture(iChannel0, (fragCoord + to_float2( 0,-1))/iResolution).x +
                          texture(iChannel0, (fragCoord + to_float2( 0,1))/iResolution).x) * 0.5f -
                          texture(iChannel0, fragCoord/iResolution).y) * 0.99f,
                          texture(iChannel0, fragCoord/iResolution).x);
   } else {
       return to_float2(0.0f, texture(iChannel0, fragCoord/iResolution).x);
   }
}

__DEVICE__ float landscapeSin(in float2 fragCoord) {
    return _sinf(fragCoord.x*10.0f)*_cosf(fragCoord.y*10.0f)-0.5f;
}


 
__DEVICE__ float2 cSqr(float2 c){
    return to_float2(c.x*c.x - c.y*c.y, 2.0f*c.x*c.y);
}

__DEVICE__ float landscapeJulia(in float2 fragCoord )
{
  
    // Juliaset
    const int maxIt = 16;
    
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = (fragCoord)*2.0f;
    int it = 0;
    float2 z = uv;
    float2 c = to_float2(-0.7f, 0.6f);
    float minD = length(z);
    for(int i=0; i< maxIt;i++){
        z = cSqr(z) + c;
        if(length(z) > 50.0f) break;
        it++;
        minD = _fminf(length(z), minD);
    }
    // Time varying pixel color
    
    return -1.0f + 1.5f*(float)(it)/(float)(maxIt);
}

__KERNEL__ void JuliaIslandsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Julia, 0);

    CONNECT_SLIDER0(LevelRain, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER1(LevelMouse, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(LevelSim, -1.0f, 10.0f, 1.0f);

    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 pointer = swi2(iMouse,x,y)/iResolution;
    
    // initialize landscape
    if(iFrame == 0 || iMouse.w > 0.5f || Reset)
    { 
       if(Julia)
          fragColor.z = landscapeJulia(uv);
        else 
          fragColor.z = -0.625f;
        
    } else {
        fragColor.z = texture(iChannel0, fragCoord/iResolution).z;
    }
    
    // Time varying pixel color
    float2 height = simStep(fragCoord,iResolution,iChannel0) * LevelSim  + hit(fragCoord, iMouse) * LevelMouse + rain(fragCoord,iResolution,iTime) * LevelRain;
    //float height = _sinf(fragCoord.x*0.1f);

    if(iFrame == 0 || Reset) height = to_float2_s(0.0f); 

    // Output to buffer
    //swi2(fragColor,x,y) = to_float2(height.x, height.y);
    fragColor.x = height.x;
    fragColor.y = height.y;

    

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0




__DEVICE__ float3 norm(in float2 fragCoord,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float bumpiness = 1.0f;
  
    float dX = texture(iChannel0, (fragCoord + to_float2(1,0))/iResolution).x - texture(iChannel0, (fragCoord + to_float2(-1,0))/iResolution).x;
    float dY = texture(iChannel0, (fragCoord + to_float2(0,1))/iResolution).x - texture(iChannel0, (fragCoord + to_float2(0,-1))/iResolution).x;

    return normalize( to_float3(-dX * bumpiness, -dY * bumpiness, 1.0f) );
}

__DEVICE__ float3 landColor(float h, float3 LandColor)
{
    float hn = h*0.5f + 0.5f;
    //return to_float3(0.1f, 0.3f, 0.4f) + to_float3(hn, 0.7f*hn, 0.4f*hn);
    return LandColor + to_float3(hn, 0.7f*hn, 0.4f*hn);
}

__KERNEL__ void JuliaIslandsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(LandColor, 0.1f, 0.3f, 0.4f, 1.0f);
    //CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);
  
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    // Output to screen
    float3 n = norm(fragCoord,iResolution,iChannel0);
    float3 c = to_float3_s(1.0f) * dot(n, normalize(to_float3(-1.0f, 1.0f, 0.0f)));
    float h = _tex2DVecN(iChannel0,uv.x,uv.y,15).z;
    if(h < 0.0f) {
        float rH = texture(iChannel0, uv+swi2(n,x,y)*0.5f*h).z;
        fragColor = to_float4_aw(swi3(c,x,y,z), LandColor.w) + to_float4_aw(landColor(rH,swi3(LandColor,x,y,z)),LandColor.w);
    } else {
        fragColor = to_float4_aw(landColor(h,swi3(LandColor,x,y,z)),LandColor.w);
    }

  SetFragmentShaderComputedColor(fragColor);
}