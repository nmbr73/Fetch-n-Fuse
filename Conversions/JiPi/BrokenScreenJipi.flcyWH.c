
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Textur' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

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
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__KERNEL__ void BrokenScreenJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);

    CONNECT_SLIDER4(NV, -1.0f, 30.0f, 6.0f);
    CONNECT_SLIDER5(LengthOld, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER6(LightCone, -1.0f, 30.0f, 5.0f);

    fragCoord+=0.5f;
  
    float2 dvec;
    //if (iMouse.x<20.0f)
    {
      float r =  0.2f + 0.6f * _sinf(iTime* 0.62f);
      dvec = to_float2( (iResolution.x/2.0f + r *iResolution.y/2.0f * _sinf(iTime*2.2f)), ( iResolution.y/2.0f + r*iResolution.y/2.0f * _cosf( iTime*2.2f)));
    }
    //else
    if(iMouse.z>0.0f)
    {
      dvec = swi2(iMouse,x,y);
    }
       
    float2 uv = fragCoord/iResolution;
    float2 tc = fragCoord - swi2(dvec,x,y);
    tc/=iResolution.x;
    float o= length(tc);
    
    //float b = _powf(_fmaxf(1.0f-o*5.0f,0.0f),16.0f);
    float b = _powf(_fmaxf(1.0f-o*LightCone,0.0f),16.0f);
    //float3 nv = to_float3(tc.x * b * 6.0f,tc.y * b * 6.0f,b);
    float3 nv = to_float3(tc.x * b * NV,tc.y * b * NV,b);
    
    float3 oldervec = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 oldvec = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float2 old2d = swi2(oldvec,x,y);
    if (length(old2d) > LengthOld) //1.0f)
    {
        old2d = normalize(old2d);
        oldvec = to_float3_aw(old2d, oldvec.z);       
    }
    float3 oldervecnev = swi3(texture(iChannel0, uv - swi2(oldvec,x,y) * 0.5f),x,y,z);
    
    
    fragColor = to_float4_aw(nv + to_float3_aw(swi2(oldvec,x,y) *  0.999f, oldervecnev.z * 0.9f), 1.0f );

    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BrokenScreenJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(Color, 0.02f, 0.1f, 0.4f, 1.0f);
  CONNECT_SLIDER3(OKZ, -1.0f, 30.0f, 10.0f);
  
  float2 uv = fragCoord / iResolution;
  float4 ok = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  //fragColor = to_float4_aw(to_float3(0.02f,0.1f,0.4f) * (ok.z * 10.0f) + to_float3_s(ok.z), 3.0f);
  fragColor = to_float4_aw(swi3(Color,x,y,z) * (ok.z * OKZ) + to_float3_s(ok.z), Color.w);

  SetFragmentShaderComputedColor(fragColor);
}