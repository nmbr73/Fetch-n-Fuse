
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float iTime)
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
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y*_cosf(iTime)),Blend);
        
  
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,0.0f,(tex.y+MulOff.y)*MulOff.x,1.0f),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,x,y, _mix(swi2(Q,x,y),  swi2(tex,x,y)*Par, Blend));
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__KERNEL__ void Fireflow2Jipi754Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);  
  
  //Blending
  CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);  
  
    fragCoord+=0.5f;

    const float diffuse=5.5f;// 3.5f also works well. 11.5f makes crazy colour swirls.

    const float accel=0.1f;
    const float max_speed=0.3f;

    const float dissipate=0.001f;
    const float springiness=0.01f;


    // this makes makes it a bit more varied.
    //diffuse=_sinf(iTime*0.1f)*12.0f;
    //dissipate=_sinf(iTime)*0.01f;
    
    float2 uv = (fragCoord) / iResolution;
    float2 delta=to_float2_s(diffuse)/iResolution;
    
    float4 a_=texture(iChannel0, uv-delta);
    float4 b_=texture(iChannel0, uv+to_float2(delta.x, -delta.y));
    float4 c_=texture(iChannel0, uv+to_float2(-delta.x, delta.y));
    float4 d_=texture(iChannel0, uv+delta);
    
    float4 v=0.25f*(a_+b_+c_+d_);
    uv-=delta*clamp(swi2(v,x,y), to_float2_s(-max_speed), to_float2_s(max_speed));
    
    // propagate (backwards of what I actually need)
    v=_tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float4 a=texture(iChannel0, uv-delta);
    float4 b=texture(iChannel0, uv+to_float2(delta.x, -delta.y));
    float4 c=texture(iChannel0, uv+to_float2(-delta.x, delta.y));
    float4 d=texture(iChannel0, uv+delta);
    float4 avg=0.25f*(a+b+c+d);
    v=_mix(v,avg,dissipate);
    //v.w=avg.w;
    
    float4 ddx=(b+d)-(a+c);
    float4 ddy=(c+d)-(a+b);
    
        
    // x,y : velocity , z: 'pressure' (but not quite), w: buoyancy

    float divergence=ddx.x+ddy.y;
    
    swi2S(v,x,y, swi2(v,x,y)-to_float2(ddx.z, ddy.z)*accel);
    v.z-=divergence*springiness;
    
    //swi2(v,x,y)+=(v.w)*to_float2(0.0f, 1.0f);
    v.x+=(v.w)*(0.0f);
    v.y+=(v.w)*(1.0f);    
    
    float t=iTime*0.2f+3.0f;
    
    float2 mousePos=swi2(iMouse,x,y);
    if(mousePos.x<3.0f)   mousePos.x=(iResolution.x*0.31f),mousePos.y=(iResolution.y*0.31f); //mousePos.xy=to_float2(iResolution*0.31f);
    
    
    float mouse=length(swi2(mousePos,x,y)-fragCoord);
    v+=to_float4(0,0, 0, 0.001f)*_fmaxf(1.0f-0.03f*mouse, 0.0f)*dot(swi2(mousePos,x,y)-fragCoord, to_float2(_sinf(t),_cosf(t)));   
    
    v.w*=0.99f;

    if(Reset){
        fragColor=to_float4_s(0.0f);
    }else{    
       fragColor=clamp(v*0.998f, to_float4_s(-1), to_float4_s(1));
    }    

    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iTime);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void Fireflow2Jipi754Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float3 col_add=to_float3(0.0f, 0.0f, 0.0f), col_scale=to_float3(2.0f,2.0f,1.0f);
    
  fragColor = to_float4_aw(abs_f3(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,z,y))*col_scale+col_add, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
