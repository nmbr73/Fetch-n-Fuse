
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Gray Noise Medium' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'Texture: Textur' to iChannel2
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define SCANSIZE 2
//float DECAY = 0.9f;
// Adjust the decay if you adjust the scansize. Try 1 and .925. There are lots of wild effects with different
// values
//
// It's funny how at scansize 1, there is this stable pattern where a black pixel will be surrounded by lit pixels.
// Each frame, a given pixel adds the direction of nearby brigtness, so if it's surrounded, it will stay dark
// because I'm using the length of the scan vector to estimate the brightness, which is zero when it's surrounded.

__DEVICE__ float2 ScanNeighbors(int2 p, float2 iResolution, __TEXTURE2D__ iChannel1) {
    float2 dir = to_float2_s(0.0f);
    for (int y=-SCANSIZE; y<=SCANSIZE; y++) {
        for (int x=-SCANSIZE; x<=SCANSIZE; x++) {
            if(x==0 && y==0) continue;
            //dir += to_float2(x,y)*texelFetch(iChannel1, p+to_int2(x,y), 0).r;
            dir += to_float2(x,y)*texture(iChannel1, (make_float2(p+to_int2(x,y))+0.5f)/iResolution).x;
        }
    }
    // returns the average direction of the nearby brigtness
    return (dir);
}
__DEVICE__ float3 iterate(int2 ifrag, float DECAY, float2 iResolution, __TEXTURE2D__ iChannel1, float iTime) {
   //float DECAY = 0.89f+_sinf(iTime*0.5f)*0.02f; // animating decay

    float3 col = to_float3_s(0.0f);
   // if(ifrag.y == int(iResolution.y/2.0f+10.0f*_sinf(iTime*2.0f))){  // sweeping line
   //     col.x = 0.0f;
   // } else {
        //float3 tc = to_float3(texelFetch(iChannel1, ifrag, 0).rgb);
        float3 tc = swi3(texture(iChannel1, (make_float2(ifrag)+0.5f)/iResolution),x,y,z);
        
        
        float2 dir = ScanNeighbors(ifrag,iResolution,iChannel1);
        float2 lightDir = normalize( (to_float2(_sinf(iTime),_cosf(iTime))));
        swi2S(tc,z,y, to_float2_s(smoothstep(-3.0f,3.0f,dot(dir, lightDir))));
        tc.x += smoothstep(0.0f,7.0f*(float)(SCANSIZE*SCANSIZE),length(dir));
        tc.x *= DECAY;
        tc.y = smoothstep(0.8f,1.0f,DECAY);
        col = clamp(tc,0.0f,1.0f);
    //}
    return col;
}

__KERNEL__ void CreepingWavesFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_BUTTON0(Modus, 1, Icks, PunchIn, Zet, Weh, PunchOut);

    float DECAY = 0.9f;

    float2 uv = fragCoord/iResolution;
    int2 ifrag = to_int2_cfloat(fragCoord);
    float3 col = to_float3_s(0);
    
    if(iFrame <10) {
        col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    } else {
        col = iterate(ifrag,DECAY,iResolution,iChannel1,iTime);
    }
    

  if (Blend1>0.0f)
  {
    float4 tex = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    if (tex.w != 0.0f)    
    {
      //tex = tex*2.0 - 1.0f;
      if ((int)Modus & 2)  col.x = _mix(col.x,tex.x,Blend1);
      //if ((int)Modus & 4) col  = to_float4(0.0,0.0f,-1.0f,-1.0f);
      if ((int)Modus & 8)  col.y = _mix(col.y,tex.y,Blend1);
      if ((int)Modus & 16) col   = _mix(col, to_float3_s(0.0f),Blend1);
      if ((int)Modus & 32) col   = _mix(col, swi3(tex,x,y,z),Blend1);
    }
    else
    {
      if ((int)Modus & 4) col = to_float3(0.0f,0.0f,0.0f);
    }
  }
    
    
    fragColor = to_float4_aw(col,1.0f);
    

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CreepingWavesFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    // Time varying pixel color
    float3 col =  swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    
    // Output to screen
    col.x /= 2.0f; // value data stored in r, lighting in gb
    col.x += col.z;
    fragColor = to_float4_aw(swi3(col,z,x,x),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}