
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

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
          Q = _mix(Q,to_float4_f2f2(U, (swi2(tex,x,y)+MulOff.y)*MulOff.x), Blend);   //Falsch rum - aber trotzdem toller Effekt
          

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), U , Blend));
          Q = _mix(Q,to_float4_f2f2((swi2(tex,x,y)+MulOff.y)*MulOff.x, U), Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend));
          swi2S(Q,z,w, _mix( swi2(Q,z,w), U, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,U.x,U.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,to_float4_f2f2(U, (swi2(tex,x,y)+MulOff.y)*MulOff.x), Blend);
    }
  
  return Q;
}


/*
  lots o' particles (Buf A)
  2016 stb

  This shader updates the particles.

  No attempt is made to preserve particles upon contact, so only a few will remain after a while :(
*/



// hash without sine
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 p) {
    float3 MOD3 = to_float3(443.8975f, 397.2973f, 491.1871f);
    float3 p3 = fract_f3((swi3(p,x,y,x)) * MOD3);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

__KERNEL__ void LotsOfParticlesJipi507Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_SLIDER0(ParticleDensity, -1.0f, 3.0f, 1.0f);
    
          //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  Position, Direction, Invers, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
    fragCoord+=0.5f;

    //const float ParticleDensity = 1.0f; // 0.0f-1.0

    float2 res = iResolution;
    float2 px = 1.0f / res;
    float2 uv = fragCoord / res;
    
    float4 buf[9];
    buf[0] = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    buf[1] = texture(iChannel0, fract(uv-to_float2(px.x, 0.0f)));
    buf[2] = texture(iChannel0, fract(uv-to_float2(-px.x, 0.0f)));
    buf[3] = texture(iChannel0, fract(uv-to_float2(0.0f, px.y)));
    buf[4] = texture(iChannel0, fract(uv-to_float2(0.0f, -px.y)));
    buf[5] = texture(iChannel0, fract(uv-to_float2(px.x, px.y)));
    buf[6] = texture(iChannel0, fract(uv-to_float2(-px.x, px.y)));
    buf[7] = texture(iChannel0, fract(uv-to_float2(px.x, -px.y)));
    buf[8] = texture(iChannel0, fract(uv-to_float2(-px.x, -px.y)));
    
    // this cell's particle direction & position, if any
    float2 pDir = swi2(buf[0],x,y);
    float2 pPos = swi2(buf[0],z,w);
    
    // update this cell's particle position
    pPos = mod_f2f2(pPos+pDir, res);
    
    // clear the current cell if its particle leaves it
    if( _floor(pPos.x)!=_floor(fragCoord.x) || _floor(pPos.y)!=_floor(fragCoord.y)  ) {
        pDir = to_float2_s(0.0f);
        pPos = to_float2_s(0.0f);
    }
    
    // add up any incoming particles
    float ct = 0.0f;
    float2 pDirAdd = to_float2_s(0.0f);
    float2 pPosAdd = to_float2_s(0.0f);
    for(int i=1; i<9; i++) {
        float2 pPosI = swi2(buf[i],z,w);
        pPosI = mod_f2f2(pPosI+swi2(buf[i],x,y), res);
        if(_floor(pPosI.x)==_floor(fragCoord.x) && _floor(pPosI.y)==_floor(fragCoord.y)) {
            pDirAdd += swi2(buf[i],x,y);
            pPosAdd += pPosI;
            ct ++;
        }
    }
    
    // if particles were added up, average and transfer them to the current cell
    if(ct>0.0f) {
        pDir = normalize(pDirAdd / ct);
        pPos = pPosAdd / ct;
    }
    
    // first frame particle setup
    if(iFrame==0 || Reset)
        if(ParticleDensity>hash12(fragCoord/res)) {
            float2 randXY =
                to_float2(
                    hash12(mod_f2(uv+iTime/100.0f-4.0f, 100.0f)),
                    hash12(mod_f2(uv-iTime/100.0f-8.0f, 100.0f))
        );
float AAAAAAAAAAAAAAAAAAA;            
            pDir = normalize(randXY-0.5f);
            pPos = fragCoord;
        }
    
    fragColor = to_float4_f2f2(pDir, pPos);

    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


/*
  lots o' particles (Buf B)
  2016 stb

  This shader predraws the particles for the Image shader.
*/

__KERNEL__ void LotsOfParticlesJipi507Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_SLIDER1(FadeAmt, 0.0f, 1.0f, 0.1f);
  fragCoord+=0.5f;

    //const float FadeAmt = 0.1f; // 0.0f-1.0

    float2 uv = fragCoord / iResolution;
    
    float pDot, pTrail;
    float2 pDir = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y);
    
    pDot = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
    pTrail = _tex2DVecN(iChannel1,uv.x,uv.y,15).y;
   
    // make this cell white if it has a nonzero vector length
    if(length(pDir)>0.0f)
         pDot = 1.0f;
    
    // trail effect
    pTrail = _fmaxf(pDot, pTrail-FadeAmt);
    
  fragColor = to_float4(pDot, pTrail, 0.0f, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


/*
  lots o' particles (Image)
  2016 stb

  Drawing from Buf B...
*/

__KERNEL__ void LotsOfParticlesJipi507Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  CONNECT_COLOR0(Color, 0.5f, 0.75f, 1.0f, 1.0f); 
  
  
  fragCoord+=0.5f;

float IIIIIIIIIIIIII;    

  float2 uv = fragCoord / iResolution;
    
  float pDot, pTrail;
    
  pDot = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
  pTrail = _tex2DVecN(iChannel0,uv.x,uv.y,15).y;
    
  //fragColor = to_float4_aw(to_float3(pDot)+to_float3(0.5f, 0.75f, 1.0f)*to_float3_aw(pTrail), Color.w);
  fragColor = to_float4_aw(to_float3_s(pDot)+swi3(Color,x,y,z)*to_float3_s(pTrail), Color.w);


  SetFragmentShaderComputedColor(fragColor);
}