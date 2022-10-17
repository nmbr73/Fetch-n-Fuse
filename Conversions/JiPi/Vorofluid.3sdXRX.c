
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define size iResolution
#define SAMPLE(a, p, s) texture((a), (p)/s)

__DEVICE__ float gauss(float2 x, float r)
{
    return _expf(-_powf(length(x)/r,2.0f));
}
#define SPEED
   
#define PI 3.14159265f

#ifdef SPEED
//high speed
    #define dt 6.5f
    #define P 0.03f
  #define divergence 0.3f
#else
//high precision
   #define dt 2.0f
    #define P 0.08f
  #define divergence 0.0002f
#endif

//how many particles per pixel, 1 is max
#define particle_density 1.0f

const float radius = 2.0f;

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1
// Connect Buffer A 'Texture: Blending' to iChannel2


//voronoi particle tracking 

__DEVICE__ void Check(inout float4 *U, float2 pos, float2 dx, float2 size, __TEXTURE2D__ iChannel0)
{
    float4 Unb = SAMPLE(iChannel0, pos+dx, size);
    float2 sizep = size - to_float2(1,1);
    float2 rpos1 = mod_f2f2(pos-swi2(Unb,x,y)+size*0.5f,size) - size*0.5f;
    float2 rpos2 = mod_f2f2(pos-swi2(*U,x,y)+size*0.5f,size) - size*0.5f;
    //check if the stored neighbouring particle is closer to this position 
    if(length(rpos1) < length(rpos2))
    {
        *U = Unb; //copy the particle info
    }
}

__DEVICE__ float4 BA(float2 pos, float2 size, __TEXTURE2D__ iChannel1)
{
   return 5.0f*SAMPLE(iChannel1, pos, size);
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
          swi2S(Q,z,w,  _mix(swi2(Q,z,w), to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend));
        
        
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


__KERNEL__ void VorofluidFuse__Buffer_A(float4 U, float2 pos, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
    
  
    pos+=0.5f;

    U = SAMPLE(iChannel0, pos, size);
    
    //check neighbours 
    Check(&U, pos, to_float2(-1,0), size, iChannel0);
    Check(&U, pos, to_float2(1,0), size, iChannel0);
    Check(&U, pos, to_float2(0,-1), size, iChannel0);
    Check(&U, pos, to_float2(0,1), size, iChannel0);
    Check(&U, pos, to_float2(-1,-1), size, iChannel0);
    Check(&U, pos, to_float2(1,1), size, iChannel0);
    Check(&U, pos, to_float2(1,-1), size, iChannel0);
    Check(&U, pos, to_float2(1,-1), size, iChannel0);
    Check(&U, pos, to_float2(-2,0), size, iChannel0);
    Check(&U, pos, to_float2(2,0), size, iChannel0);
    Check(&U, pos, to_float2(0,-2), size, iChannel0);
    Check(&U, pos, to_float2(0,2), size, iChannel0);
    
    float2 ppos = swi2(U,x,y)*(1.0f - divergence) + divergence*pos;

    //dont make the particles be too close
    float2 repulsion = to_float2(BA(ppos+to_float2(1,0),size,iChannel1).w - BA(ppos+to_float2(-1,0),size,iChannel1).w, BA(ppos+to_float2(0,1),size,iChannel1).w - BA(ppos+to_float2(0,-1),size,iChannel1).w);
    float2 pressure  = to_float2(BA(ppos+to_float2(1,0),size,iChannel1).z - BA(ppos+to_float2(-1,0),size,iChannel1).z, BA(ppos+to_float2(0,1),size,iChannel1).z - BA(ppos+to_float2(0,-1),size,iChannel1).z);
    //mouse interaction
    if(iMouse.z>0.0f)
    {
        float k = gauss(ppos-swi2(iMouse,x,y), 5.0f);
        swi2S(U,z,w, swi2(U,z,w)*(1.0f-k) + k*0.2f*to_float2(_cosf(0.03f*iTime*dt), _sinf(0.03f*iTime*dt)));
    }

    swi2S(U,z,w, swi2(U,z,w) + 0.002f*to_float2(_cosf(0.03f*iTime*dt), _sinf(0.03f*iTime*dt))*gauss(ppos-size*to_float2(0.5f,0.5f),8.0f)*dt);
    //update the particle
    
    //swi2(U,z,w) = swi2(U,z,w);  //????????????????
    swi2S(U,z,w, swi2(U,z,w) + P*pressure*dt);
    //smooth velocity
    float2 velocity = 0.0f*swi2(BA(ppos,size,iChannel1),x,y) + swi2(U,z,w) - 0.000f*repulsion;
    swi2S(U,x,y, swi2(U,x,y) + dt*velocity);
    swi2S(U,x,y, mod_f2f2(swi2(U,x,y),size)); //limit the position to the texture
    
    
    if (Blend1>0.0) U = Blending(iChannel2, pos/size, U, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, pos, size);
    
    
    if(iFrame < 1 || Reset)
    {
        //if(mod_f(pos, to_float2(1.0f/particle_density)).x < 1.0f && mod_f(pos, to_float2(1.0f/particle_density)).y < 1.0f)
        if(mod_f(pos.x, (1.0f/particle_density)) < 1.0f && mod_f(pos.y, (1.0f/particle_density)) < 1.0f)
           U = to_float4(pos.x,pos.y,0.0f,0.0f);
    }

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float4 B(float2 pos, float2 size, __TEXTURE2D__ iChannel1)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
__DEVICE__ float3 pdensity(float2 pos, float2 size, __TEXTURE2D__ iChannel0)
{
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float3_aw(swi2(particle_param,z,w),gauss(pos - swi2(particle_param,x,y), 0.7f*radius));
}


__KERNEL__ void VorofluidFuse__Buffer_B(float4 u, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    pos+=0.5f;  

    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);

    float4 prev_u = SAMPLE(iChannel1, pos, size);
  
    float3 density = pdensity(pos,size,iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z,  0.5f*density);
    float div = B(pos+to_float2(1,0),size,iChannel1).x-B(pos-to_float2(1,0),size,iChannel1).x+B(pos+to_float2(0,1),size,iChannel1).y-B(pos-to_float2(0,1),size,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),size,iChannel1)+B(pos+to_float2(1,0),size,iChannel1)+B(pos-to_float2(0,1),size,iChannel1)+B(pos-to_float2(1,0),size,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1

#ifdef xxx
vec4 B(float2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
__DEVICE__ float3 pdensity(float2 pos)
{
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float3(swi2(particle_param,z,w),gauss(pos - swi2(particle_param,x,y), 0.7f*radius));
}
#endif

__KERNEL__ void VorofluidFuse__Buffer_C(float4 u, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    pos+=0.5f;

    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);

    float4 prev_u = SAMPLE(iChannel1, pos, size);
   
    float3 density = pdensity(pos,size, iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z, 0.5f*density);
    float div = B(pos+to_float2(1,0),size,iChannel1).x-B(pos-to_float2(1,0),size,iChannel1).x+B(pos+to_float2(0,1),size,iChannel1).y-B(pos-to_float2(0,1),size,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),size,iChannel1)+B(pos+to_float2(1,0),size,iChannel1)+B(pos-to_float2(0,1),size,iChannel1)+B(pos-to_float2(1,0),size,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1

#ifdef xxxx
vec4 B(float2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}

//density and velocity
__DEVICE__ float3 pdensity(float2 pos)
{
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float3(swi2(particle_param,z,w),gauss(pos - swi2(particle_param,x,y), 0.7f*radius));
}
#endif


__KERNEL__ void VorofluidFuse__Buffer_D(float4 u, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    pos+=0.5f;
    
    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);
    
    float4 prev_u = SAMPLE(iChannel1, pos, size);
  
    float3 density = pdensity(pos,size, iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z, 0.5f*density);
    float div = B(pos+to_float2(1,0),size,iChannel1).x-B(pos-to_float2(1,0),size,iChannel1).x+B(pos+to_float2(0,1),size,iChannel1).y-B(pos-to_float2(0,1),size,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),size,iChannel1)+B(pos+to_float2(1,0),size,iChannel1)+B(pos-to_float2(0,1),size,iChannel1)+B(pos-to_float2(1,0),size,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Voronoi vortex particle fluid" by michael0884. https://shadertoy.com/view/WdcXzS
// 2019-10-30 21:27:02
//const int KEY_UP = 38;
//const int KEY_DOWN  = 40;

#ifdef XXX
__DEVICE__ float4 BI(float2 pos, float2 size, __TEXTURE2D__ iChannel1)
{
   return SAMPLE(iChannel1, pos, size);
}


//density and velocity
__DEVICE__ float3 pdensity(float2 pos, float2 size, __TEXTURE2D__ iChannel0)
{
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float3_aw(swi2(particle_param,z,w),gauss(pos - swi2(particle_param,x,y), 0.7f*radius));
}
#endif

__KERNEL__ void VorofluidFuse(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX1(KEY_UP, 0);
    CONNECT_CHECKBOX2(KEY_DOWN, 0);
    
    pos+=0.5f;

    float3 density = pdensity(pos,size,iChannel0);
    float4 blur = SAMPLE(iChannel1, pos, size);
    float vorticity = B(pos+to_float2(1,0),size,iChannel1).y-B(pos-to_float2(1,0),size,iChannel1).y-B(pos+to_float2(0,1),size,iChannel1).x+B(pos-to_float2(0,1),size,iChannel1).x;
    //fragColor = to_float4(SAMPLE(iChannel2, pos, size).xyz  + 0.8f*to_float3(0.4f,0.6f,0.9f)*vorticity,1.0f);
    if(KEY_UP > 0.5f)
    {
        fragColor = to_float4_aw(2.0f*density.z*(7.0f*abs_f3(swi3(density,x,y,y))+to_float3(0.2f, 0.1f, 0.1f)),1.0f);
      
    }
    else
    {
     
         fragColor = to_float4_aw(200.0f*to_float3(vorticity, _fabs(vorticity)*0.3f, - vorticity),1.0f);
         fragColor = to_float4_aw(10.0f*abs_f3(swi3(density,x,y,y)) + 30.0f*to_float3(0,0,_fabs(blur.z)),1.0f);
    }
    
  SetFragmentShaderComputedColor(fragColor);
}