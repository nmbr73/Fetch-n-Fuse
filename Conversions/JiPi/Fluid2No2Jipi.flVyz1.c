__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = _tex2DVecN(channel,uv.x,uv.y,15);

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


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__KERNEL__ void Fluid2No2JipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
      //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT3(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;
     
    float2 uv = fragCoord/iResolution;
    float4 col = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    fragColor = col;

    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}


// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Texture: Blending' to iChannel1
// Connect Buffer B 'Texture: Blending2' to iChannel2

__KERNEL__ void Fluid2No2JipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_POINT0(Paspartout, 0.0f, 0.0f );
    CONNECT_POINT1(Source1, 0.0f, 0.0f );
    CONNECT_POINT2(Source2, 0.0f, 0.0f );
    
    //Blending
    CONNECT_SLIDER10(Blend2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER11(Blend2Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER12(Blend2Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus2, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT4(Par2, 0.0f, 0.0f);

    CONNECT_SLIDER5(DP, -1.0f, 20.0f, 8.0f);
    CONNECT_SLIDER6(Density, -1.0f, 20.0f, 8.0f);
    CONNECT_SLIDER7(SourceSize, -1.0f, 10.0f, 1.0f);


    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 uvc = uv - to_float2_s(0.5f);
    float4 col = to_float4_s(0.0f);
    
    //int2 fragI = to_int2_cfloat(fragCoord);
    
    float2 ex = to_float2(1.0f, 0.0f);
    float2 ey = to_float2(0.0f, 1.0f);
    
    float2 coords = fragCoord;
    
    float2 cfx = coords + ex;
    float2 cbx = coords - ex;
    float2 cfy = coords + ey;
    float2 cby = coords - ey;
    
    if(iFrame == 0 || Reset)
    {
        col = to_float4_s(0.0f);
    }
    else if(distance_f2(uvc, to_float2_s(0.0f)) < 1.0f) {
        
        
        //advect to new spot
        float4 u = texture(iChannel0, (coords / iResolution));
        
        float4 ufx = texture(iChannel0, (cfx / iResolution));
        float4 ubx = texture(iChannel0, (cbx / iResolution));
        float4 ufy = texture(iChannel0, (cfy / iResolution));
        float4 uby = texture(iChannel0, (cby / iResolution));
                
        float dpx = ufx.z - ubx.z;
        float dpy = ufy.z - uby.z;
        
        cfx -= swi2(ufx,x,y);
        cbx -= swi2(ubx,x,y);
        cfy -= swi2(ufy,x,y);
        cby -= swi2(uby,x,y);
        
        coords -= swi2(u,x,y);
        
        float densX = distance_f2(coords, cfx) + distance_f2(coords, cbx) - 2.0f;
        float densY = distance_f2(coords, cfy) + distance_f2(coords, cby) - 2.0f;
        
        float density = densX + densY;
        
        //apply changes
        col = texture(iChannel0, (coords / iResolution));
        
        col.x -= dpx / DP;//8.0f;
        col.y -= dpy / DP;//8.0f;
        
        //get data at new spot
        ufx = texture(iChannel0, (cfx / iResolution));
        ubx = texture(iChannel0, (cbx / iResolution));
        ufy = texture(iChannel0, (cfy / iResolution));
        uby = texture(iChannel0, (cby / iResolution));
        
        float4 uAvg = (ufx + ubx + ufy + uby) / 4.0f;
        
        col.z = uAvg.z + (density / Density);
        
        
        //col.z = 0.1f * col.w;
        //swi2(col,x,y) += to_float2(0.0f, -0.02f) * iTimeDelta;
               
        if(iMouse.z > 0.0f){
          float2 muvc = (swi2(iMouse,x,y) / iResolution) - to_float2_s(0.5f);
          col += to_float4(-muvc.x,-muvc.y, 0.0f, 1.0f) * iTimeDelta * (0.1f / (0.001f + distance_f2(uvc, muvc))) * 0.05f;
        }
        
        if(_fabs(coords.x - iResolution.x * 0.5f) >= iResolution.x * (0.4f+Paspartout.x) || _fabs(coords.y - iResolution.y * 0.5f) >= iResolution.y * (0.4f+Paspartout.y)){
            //swi3(col,x,y,z) = to_float3_s(0.0f); 
            col.x = 0.0f; 
            col.y = 0.0f; 
            col.z = 0.0f; 
        }
        
        if(distance_f2(coords, iResolution * (to_float2(0.25f, 0.5f)+Source1)) < SourceSize){ //1.0f){
            col += to_float4(0.2f, 0.01f * _sinf(iTime), 0.0f, 0.1f);
        }
        
        if(distance_f2(coords, iResolution * (to_float2(0.75f, 0.5f)+Source2)) < SourceSize){ //1.0f){
            col += to_float4(-0.2f, -0.01f * _sinf(iTime), 0.0f, 0.1f);
        }
        
        //dye will dissipate over time
        //col.w *= 0.9999f;

        if (Blend2>0.0) col = Blending(iChannel2, fragCoord/iResolution, col, Blend2, Par2, to_float2(Blend2Mul,Blend2Off), Modus2, fragCoord, iResolution);
        
    }
    
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void Fluid2No2JipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    
    CONNECT_SLIDER8(Brightness, -1.0f, 10.0f, 3.0f);
    CONNECT_SLIDER9(Colormix, -1.0f, 1.0f, 0.7f);
    
    float2 uv = fragCoord/iResolution;
    float4 col = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float2 muv = swi2(iMouse,x,y) / iResolution;
    
    //float sound = texture(iChannel1, to_float2(0.75f, 0.25f)).x;
    
    //fragColor = 3.0f *  _mix(to_float4_s(col.w), (col), 0.7f);
    fragColor = Brightness *  _mix(to_float4_s(col.w), (col), Colormix);

  SetFragmentShaderComputedColor(fragColor);
}