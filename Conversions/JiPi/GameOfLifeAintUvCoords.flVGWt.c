
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel0
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void GameOfLifeAintUvCoordsFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;
    float2 uv = 2.0f * fragCoord/swi2(iResolution,x,x);
    if(iFrame < 10 || Reset){
      fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    }
    else
    {
     fragColor = texture(iChannel1,fragCoord/iResolution);
     
     float3 diff = to_float3(1,0,-1)/iResolution.x;
     
     float4 sum = texture(iChannel1,fragCoord/iResolution - swi2(diff,x,x));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,x,y));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,x,z));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,y,x));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,y,z));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,z,x));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,z,y));
     sum += texture(iChannel1,fragCoord/iResolution - swi2(diff,z,z));
     
     float4 result = texture(iChannel1,fragCoord/iResolution);
     float amount = 0.01f;
     float minl = 2.0f;
     float maxl = 3.0f;
     
     if(sum.z < minl || sum.y > maxl)
         fragColor.x =  _fmaxf(0.0f,result.x -amount);
     else 
         fragColor.x = _fminf(1.0f,result.x + amount);
         
      if(sum.x <minl || sum.z >maxl)
         fragColor.y =  _fmaxf(0.0f,result.y -amount);
     else 
         fragColor.y = _fminf(1.0f,result.y + amount);
         
     if(sum.y < minl || sum.x > maxl)
         fragColor.z =  _fmaxf(0.0f,result.z -amount);
     else 
         fragColor.z = _fminf(1.0f,result.z + amount);
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Small' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void GameOfLifeAintUvCoordsFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

      // Normalized pixel coordinates (from 0 to 1)
      float2 uv = fragCoord/iResolution;
    
      float3 gol1 = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
      float3 gol2 = swi3(texture(iChannel0,uv + 0.254f),x,y,z);
      float4 gol = to_float4_aw(gol1 + gol2,1.0f)*0.5f;
      //fragColor = gol;
      fragColor = texture(iChannel1,uv*0.1f + 0.25f* gol.z *swi2(gol,x,y));
  

  SetFragmentShaderComputedColor(fragColor);
}