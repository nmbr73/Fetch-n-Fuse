
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define tanh_f4(i) to_float4(_tanhf((i).x), _tanhf((i).y), _tanhf((i).z), _tanhf((i).w))


//#define T(p) texture(iChannel0,(p)/iResolution)
#define T(p) _tex2DVecN(iChannel0, (p).x/iResolution.x, (p).y/iResolution.y, 15)

#define dt 0.1f

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


__KERNEL__ void WaveSimulationJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{  
    
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER1(PenSize2, -10.0f, 100.0f, 50.0f);
    
    CONNECT_SLIDER2(ParV, -10.0f, 10.0f, 0.5f);
    CONNECT_SLIDER3(ParA, -1.0f, 1.0f, 0.08f);
    CONNECT_SLIDER4(ParX, -1.0f, 1.0f, 0.001f);
    
    //Blending
    CONNECT_SLIDER5(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    float2 c   = to_float2(1,0.95f);
    
    float x  = fragCoord.x;
    float y  = fragCoord.y;
    
    float xp = mod_f(fragCoord.x + 1.0f, iResolution.x);
    float xm = mod_f(fragCoord.x - 1.0f, iResolution.x);
    float yp = mod_f(fragCoord.y + 1.0f, iResolution.y);
    float ym = mod_f(fragCoord.y - 1.0f, iResolution.y);
    
    float2 X   = to_float2(T(to_float2(x,y)).x, T(to_float2(x,y)).z);
    
    float2 Xp0 = to_float2(T(to_float2(xp,y)).x, T(to_float2(xp,y)).z);
    float2 Xm0 = to_float2(T(to_float2(xm,y)).x, T(to_float2(xm,y)).z);
    float2 X0p = to_float2(T(to_float2(x,yp)).x, T(to_float2(x,yp)).z);
    float2 X0m = to_float2(T(to_float2(x,ym)).x, T(to_float2(x,ym)).z);
    
    float2 V   = to_float2(T(to_float2(x,y)).y, T(to_float2(x,y)).w);
    
    float2 A   = c*c * (Xp0 + Xm0 + X0p + X0m - 4.0f*X);
    
    //X += V * dt + 0.5f * A * dt * dt;
    //X += A * dt * 0.08f;
    //X -= X * dt * 0.001f;
    X += V * dt + ParV * A * dt * dt;
    X += A * dt * ParA;
    X -= X * dt * ParX;

    
    V += A * dt;
    
    if(iMouse.z > 0.0f)
    {
        X += PenSize2*dt*_expf(-distance_f2(swi2(iMouse,x,y), fragCoord)*distance_f2(swi2(iMouse,x,y), fragCoord)/30.0f);
    }
    
    if (iTime > 0.1f && Reset == 0)
    {
        fragColor = to_float4(X.x,V.x,X.y,V.y);
        
        if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);
        
    }
    else
    {
        float r = length(fragCoord - iResolution/2.0f);
        float q = 10.0f*_expf(-r*r/300.0f);
        fragColor = to_float4(q,0,q,0);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void WaveSimulationJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_SLIDER0(PenSize, -10.0f, 50.0f, 10.0f);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Output to screen
    float4 col = (_tex2DVecN(iChannel0,uv.x,uv.y,15));
    col *= col;
    col.y += col.w;
    col.y *= 5.0f;
    
    if(_fabs(distance_f2(swi2(iMouse,x,y), fragCoord)-PenSize)<0.5f && iMouse.z > 0.0f) col = to_float4_s(100);
    
    fragColor = tanh_f4(col/3.0f);
    
    fragColor += (Color-0.5f)*col.w;
    
    fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}