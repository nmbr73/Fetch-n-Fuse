
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float4 texelFetchC( __TEXTURE2D__ Channel, int2 pos, float2 iR)
{
    
    if ( (pos.x) >= 0 && (pos.x) < (int)(iR.x) && (pos.y) > 0 && (pos.y) < (int)(iR.y) )
    {
        return texture( Channel, (make_float2(pos)+0.5f)/iR );
    }
  else
    return to_float4_s(0);
}

__DEVICE__ float4 getX(__TEXTURE2D__ g, int2 p, float2 iR){
    //return texelFetch(g,p,0);
    return texelFetchC(g,p,iR);
    //return texture(g,(to_float2(p)+0.5f)/R);
}

//in which BufA is t-dt, BufB is t and BufA becomes t+dt
__DEVICE__ float4 getA(__TEXTURE2D__ iChannel1, int2 p, float2 R, float dx){
    float4 middle    = getX(iChannel1,p,R);     
    float4 up        = getX(iChannel1,p+to_int2( 0, 1),R);
    float4 down      = getX(iChannel1,p+to_int2( 0,-1),R);
    float4 right     = getX(iChannel1,p+to_int2( 1, 0),R);
    float4 left      = getX(iChannel1,p+to_int2(-1, 0),R);
    float4 upright   = getX(iChannel1,p+to_int2( 1, 1),R);
    float4 upleft    = getX(iChannel1,p+to_int2(-1, 1),R);
    float4 downright = getX(iChannel1,p+to_int2( 1,-1),R);
    float4 downleft  = getX(iChannel1,p+to_int2(-1,-1),R);
        
     return (-8.0f*middle + up + left + right + down + upright + upleft + downright + downleft)/(3.0f*dx*dx);  
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



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


#define R iResolution
__KERNEL__ void BasicwaveequationJipi124Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    const float dx = 0.2f;
    const float dt = 0.08f;

    int2 p = to_int2_cfloat(fragCoord);
    fragColor = 2.0f*getX(iChannel1,p,R)-getX(iChannel0,p,R)+getA(iChannel1,p,R,dx)*dt*dt;
    if(iMouse.z>0.0f && distance_f2(fragCoord,swi2(iMouse,x,y))<10.0f) fragColor = to_float4_s(1.0f);

    if (Blend1>0.0) fragColor = Blending(iChannel2, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);

    if (iFrame<1 || Reset) fragColor = to_float4_s(0.0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void BasicwaveequationJipi124Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    fragCoord+=0.5f;

    const float dx = 0.2f;
    const float dt = 0.08f;

    int2 p = to_int2_cfloat(fragCoord);
    fragColor = 2.0f*getX(iChannel1,p,R)-getX(iChannel0,p,R)+getA(iChannel1,p,R,dx)*dt*dt;
    if(iMouse.z>0.0f && distance_f2(fragCoord,swi2(iMouse,x,y))<10.0f) fragColor = to_float4_s(1.0f);

    if (iFrame<1 || Reset) fragColor = to_float4_s(0.0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BasicwaveequationJipi124Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  fragCoord+=0.5f;
  
  fragColor = texture(iChannel0,fragCoord / iResolution)*0.5f+0.5f;

  SetFragmentShaderComputedColor(fragColor);
}