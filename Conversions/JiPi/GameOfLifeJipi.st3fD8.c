
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture:Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float3 hash3( float2 p )
{
    float3 q = to_float3( dot(p,to_float2(127.1f,311.7f)), 
           dot(p,to_float2(269.5f,183.3f)), 
           dot(p,to_float2(419.2f,371.9f)) );
  return fract_f3(sin_f3(q)*43758.5453f);
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
          //Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));
          Q.x = _mix(Q.x,step(0.5f,hash3(U).x), Blend);

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
        
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




__KERNEL__ void GameOfLifeJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 px = 1.0f / iResolution;
    fragColor.x = texture(iChannel0, fragCoord/iResolution).x; 
    fragColor.z = texture(iChannel0, fragCoord/iResolution).z*0.9f;
    
    if(iFrame<2 || Reset ){
        if( _fabs(uv.y-0.5f)<0.2f){
          fragColor=to_float4_aw(to_float3_s(hash3(fragCoord+iTime).x)*2.0f, 1.0f);
        }else{
            fragColor=to_float4_s(0.0f);
        }
    }else{
        int count = -(int)( _tex2DVecN(iChannel0,uv.x,uv.y,15).x+0.1f);
        for(int x = -1; x <= 1; x++){
            for(int y = -1; y <= 1; y++){
                float2 checkCoords = mod_f(uv+to_float2(x,y)*px,1.0f);
                float4 checkPix = _tex2DVecN(iChannel0,checkCoords.x,checkCoords.y,15);
                count += (int)( checkPix.x+0.1f);
                fragColor.y += checkPix.y*0.11f;
                fragColor.z += checkPix.z*0.01f;
            }
        }
        
        if(count==3){     // conditions for new life
            fragColor.x = 1.0f;
            if(_tex2DVecN(iChannel0,uv.x,uv.y,15).x < 0.5f) // actually new, was previously dead
              fragColor.y=1.0f;
        }
        else if(count<2){ // lonely
            fragColor.x=0.0f;
            if(_tex2DVecN(iChannel0,uv.x,uv.y,15).x > 0.5f) // new death
              fragColor.z=1.0f;
        }
        else if(count>3){ // overcrowded
            fragColor.x=0.0f;
            if(_tex2DVecN(iChannel0,uv.x,uv.y,15).x > 0.5f) // new death
              fragColor.z=1.0f;
        }

    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);
        
        
    if(length(swi2(iMouse,x,y)-fragCoord)<15.0f && iMouse.z>0.0f)
        fragColor.x=step(0.5f,hash3(fragCoord).x);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float getv(float2 uv, __TEXTURE2D__ iChannel0){
   return _fmaxf(texture(iChannel0,uv).y, _tex2DVecN(iChannel0,uv.x,uv.y,15).z); // G channel for births, B is deaths
    // mixing in different ways provides different results
    // I like this one
}

__KERNEL__ void GameOfLifeJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_COLOR1(ColorLife, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(AlphaThres, -1.0f, 1.0f, 0.1f);
  
  float Alpha = 1.0f;
  
  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float2 px = 1.0f/iResolution;

  float3 norm = to_float3(getv(uv-to_float2(1,0)*px,iChannel0)-getv(uv-to_float2(-1,0)*px,iChannel0),
                          getv(uv-to_float2(0,1)*px,iChannel0)-getv(uv-to_float2(0,-1)*px,iChannel0),
                          0.2f);
  norm = normalize(norm);
  
  float3 light = normalize(to_float3(_sinf(iTime),_cosf(iTime),1.0f));
  
  fragColor=to_float4_aw(to_float3_s(dot(norm,light)),1.0f);
  fragColor = to_float4_s(1.0f) - (to_float4_s(1.0f)-fragColor)*(to_float4_s(1.0f)-swi4(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,x,x,w));//.rrra);
  //fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15); // uncoment for alt/direct rendering
  // I think it looks prettier but my work colleague pushed for 3Dish-based lighting
  // so I embossed the birth and death data, which is blurrified some

  float4 BufA = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  
  if ( BufA.x<=AlphaThres&&BufA.y<=AlphaThres&&BufA.z<=AlphaThres ) 
  {
    fragColor += Color-0.5f;
    Alpha = Color.w;
  }
  else
  {
    fragColor += ColorLife-0.5f;
  }

  fragColor.w = Alpha;


  SetFragmentShaderComputedColor(fragColor);
}