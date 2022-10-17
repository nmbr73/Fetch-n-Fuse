
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: London' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Andrin Rehmann
// 2020
// andrinrehmann.ch
// andrinrehmann@gmail.com

#define EXPANSION 2.5f

//note: uniformly distributed, normalized rand, [0;1[
__DEVICE__ float nrand( float2 n )
{
  return fract(_sinf(dot(swi2(n,x,y), to_float2(12.9898f, 78.233f)))* 43758.5453f);
}

__DEVICE__ float rand( float2 uv, float iTime )
{
  float t = fract( iTime );
  return nrand( uv + 0.07f*t );
}


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float iTime)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      float  f = rand(uv+to_float2_s(0.2f),iTime);
      float  _y = rand(uv, iTime);


      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(f+Par.x,0.0f,_y+Par.y,1.0f),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,0.0f,(tex.y+MulOff.y)*MulOff.x,1.0f),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,x,z, _mix(swi2(Q,x,z),  swi2(tex,x,y)*to_float2(f,_y), Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}

__KERNEL__ void LichenGrowthFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_CHECKBOX0(Reset, 0);  

  //Blending
  CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);  
  
  fragCoord+=0.5f;

  float du = 1.0f / iResolution.x;
  float dv = 1.0f / iResolution.y;
    
  float2 uv = fragCoord/iResolution;
    
  // Cell
  float _y = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
  // food
  float f = _tex2DVecN(iChannel0,uv.x,uv.y,15).z;
    
  // sum of surrounding _y's
  float s = 1.0f/16.0f * texture(iChannel0,uv + to_float2(-du,-dv)).x +
            3.0f/16.0f * texture(iChannel0,uv + to_float2(-du,0)).x + 
            1.0f/16.0f * texture(iChannel0,uv + to_float2(-du,dv)).x + 
            3.0f/16.0f * texture(iChannel0,uv + to_float2(0,-dv)).x + 
            3.0f/16.0f * texture(iChannel0,uv + to_float2(0,dv)).x +
            1.0f/16.0f * texture(iChannel0,uv + to_float2(du,-dv)).x +
            3.0f/16.0f * texture(iChannel0,uv + to_float2(du,0)).x + 
            1.0f/16.0f * texture(iChannel0,uv + to_float2(du,dv)).x;
    
    if (s > 0.5f && f > 0.5f){
        _y += 0.1f;
    }
    
    if (_y > 0.5f){
        f -= 0.01f;
    }
    
    if (f < 0.5f){
        _y -= _y * 0.1f;
    
    }
    
    f += 0.002f;
    
    
    if (distance_f2(swi2(iMouse,x,y), fragCoord) < 15.0f && iMouse.z > 0.0f){
        f = rand(uv+to_float2_s(0.2f),iTime);
        _y = rand(uv, iTime);
    }
    
    // Init
    if (iFrame < 1 || Reset){
        _y = rand(uv, iTime);
        f = rand(uv+to_float2_s(0.2f), iTime);
    }
    
    
    
    fragColor = to_float4(_y, 0, f, 1.0f);

    if (Blend1>0.0) fragColor = Blending(iChannel1, uv, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iTime);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void LichenGrowthFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    float3 buffer = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    fragColor = to_float4_aw(to_float3_s(buffer.x),1.0f);
    //fragColor = to_float4_aw(buffer,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}