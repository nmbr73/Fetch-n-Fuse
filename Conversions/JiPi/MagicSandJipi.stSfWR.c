
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Pebbles' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ mat2 rotate(float a) {
  return to_mat2(-_sinf(a), _cosf(a),
                  _cosf(a), _sinf(a));
}

__DEVICE__ float rand(float2 p, __TEXTURE2D__ iChannel1) {
  return _tex2DVecN(iChannel1,p.x,p.y,15).x;
}

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


__KERNEL__ void MagicSandJipiFuse__Buffer_A(float4 o, float2 q, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
      //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    q+=0.5f;

    q /= iResolution;   
 
    float2 sand = mul_f2_mat2(q, rotate(0.0f)) + 0.000001f;
    sand = mul_f2_mat2(sand, rotate(rand(q,iChannel1) * 0.02f));

    o = _tex2DVecN(iChannel0,sand.x,sand.y,15) * (0.99f - rand(q,iChannel1) * 0.1f);
    
    float2 p = q - 0.5f;
    p.x *= iResolution.x / iResolution.y;
    
    if (iMouse.z > 0.0f) {
        float2 m = swi2(iMouse,x,y);
        
        m /= iResolution;
        m -= 0.5f;
        m.x *= iResolution.x / iResolution.y;
        
      p -= m;
    }
    else {
        p += (to_float2(_cosf(iTime * 2.0f), _sinf(iTime * 1.15f)) + rand(p,iChannel1) * 0.2f) * 0.2f;
    }
    
    if (length(p + _sinf(_atan2f(p.x, p.y) * 3.0f) * 0.005f) - 0.05f - _fabs(_sinf(iTime * 2.0f)) * 0.025f < 0.0f) {
      o = to_float4(0.8f + _sinf(iTime * 5.0f) * 0.2f, 0.25f + _cosf(iTime * 5.0f), 1.0f - _tanf(iTime * 100.0f) * 0.025f, 1.0f);
    }

    if (Blend1>0.0) o = Blending(iChannel2, q, o, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, q, R);

//o=texture(iChannel2,q);

  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void MagicSandJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}