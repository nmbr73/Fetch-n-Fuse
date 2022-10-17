
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A texture(iChannel0,(u+i)/R)


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


__KERNEL__ void TinyPaintStreamsFuse__Buffer_A(float4 r, float2 u, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(StartTex, 0);
    
            //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON1(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    u+=0.5f;

    float2 x, i = u-u, f = i; r -= r;
    float G, m = texture(iChannel0,(u+i)/R).w;
    // iterate over neighbouring cells
    // swi2(A,x,y) = velocity
    // A.z = pigment
    // A.w = density
    for(int k = 81; k-->0;)
        i = to_float2(k%9,k/9)-4.0f,
        x = i + swi2(A,x,y), // move particle according to velocity

        // Gaussian diffusion, sigma=_sqrtf(0.5f)
        G = A.w / _expf(dot(x,x)) / 3.142f,

        // advection
        r += to_float4_aw(swi3(A,x,y,z), 1) * G,
    
        // pressure forces (smoothed particle hydrodynamics)
        f -= ( m*m-m       // pressure at current position
                           // density * (density - reference fluid density)
             + A.w*A.w-A.w // pressure at neighbour position
             ) * G * x;    // gradient of smoothing kernel

    if(r.w > 0.0f) // not vacuum
    {
        swi3S(r,x,y,z, swi3(r,x,y,z) / r.w);          // convert momentum to velocity, normalise pigment
        swi2S(r,x,y, swi2(r,x,y) + clamp(f / r.w, -0.1f, 0.1f)); // acceleration
    }
    // gravity
    r.y -= 0.005f;

    // boundary
    if(u.y < 9.0f) r.y += 1.0f;
    
    // streams
    r = length(u - R * to_float2(0.2f, 0.9f)) < 9.0f ?
            to_float4(_sinf(iTime) + 2.0f, -1, 0, 1) :
        length(u - R * to_float2(0.8f, 0.9f)) < 9.0f ?
            to_float4(_sinf(iTime) - 2.0f, -1, 1, 1) : r;
        
    if (iFrame < 2 || Reset)                  // init
    { 
      if (StartTex)
      {
         r = texture(iChannel1, u/R); 
      }
      else
        if(u.y > 0.8f*R.y) r = to_float4(0,0, u.x < R.x/2.0f, 1); 
    }  

    if (Blend1>0.0) r = Blending(iChannel1, u/R, r, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, u, R); 
    
       
  SetFragmentShaderComputedColor(r);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void TinyPaintStreamsFuse(float4 r, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  
  Color-=0.5f;
  
  u+=0.5f;
  float2 x, i = u-u, f = i; r -= r - 1.0f + A.w * (to_float4(0, A.z, 1.0f - A.z, 0) - Color); 
  
SetFragmentShaderComputedColor(r);    
}