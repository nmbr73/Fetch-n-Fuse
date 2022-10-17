
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define size iResolution
#define SAMPLE(a, p, s) texture((a), (p)/s)

__DEVICE__ float gauss(float2 _x, float r)
{
    return _expf(-_powf(length(_x)/r,2.0f));
}
   
#define PI 3.14159265f
//#define dt 0.6f


__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
        {
          float4 target = to_float4(30.0f*_floor(U.x/30.0f)+60.0f*_sinf(U.x),30.0f*_floor(U.y/30.0f)-10.0f*_cosf(U.y+U.y),1.0f,1.0f);
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(U+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,target,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));
        }
        if ((int)Modus&4)
          swi2S(Q,x,y, _mix( swi2(Q,x,y), U+Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
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
// Connect Buffer A 'Texture: Gray Noise Medium' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


//voronoi particle tracking 

__DEVICE__ void Check(inout float4 *U, float2 pos, float2 dx, float2 size, __TEXTURE2D__ iChannel0)
{
    float4 Unb = SAMPLE(iChannel0, (pos+dx), size);
    //check if the stored neighbouring particle is closer to this position 
    if(length(swi2(Unb,x,y) - pos) < length(swi2(*U,x,y) - pos))
    {
        *U = Unb; //copy the particle info
    }
}

__DEVICE__ float4 B(float2 pos, float2 size, __TEXTURE2D__ iChannel1)
{
   return 5.0f*SAMPLE(iChannel1, pos, size);
}

__KERNEL__ void FireworkWavesFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(dt, 0.0f, 2.0f, 0.6f);
    CONNECT_SLIDER1(divergence, -0.1f, 0.10f, 0.001f);
    CONNECT_POINT0(ParticleXY, 0.0f, 0.45f);

    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  XY, ZW, XYPar, Special);
    CONNECT_POINT5(Par1, 0.0f, 0.0f); 


    pos+=0.5f;

    U = SAMPLE(iChannel0, pos, size);
    
    //check neighbours 
    Check(&U, pos, to_float2(-1,0), size,iChannel0);
    Check(&U, pos, to_float2(1,0), size,iChannel0);
    Check(&U, pos, to_float2(0,-1), size,iChannel0);
    Check(&U, pos, to_float2(0,1), size,iChannel0);
    
    //small divergence
    //float2 ppos = 0.999f*swi2(U,x,y)+0.001f*swi2(pos,x,y);
    float2 ppos = (1.0f-divergence)*swi2(U,x,y)+divergence*pos;
float AAAAAAAAAAAAAAAAAAA;    
    float4 Bdx = B(ppos+to_float2(1,0), size,iChannel1) - B(ppos-to_float2(1,0), size,iChannel1); 
    float4 Bdy = B(ppos+to_float2(0,1), size,iChannel1) - B(ppos-to_float2(0,1), size,iChannel1);
    
    //update the particle
    //swi2S(U,x,y, swi2(U,x,y) - dt*(to_float2(Bdx.x - Bdx.y, Bdy.x - Bdy.y) + to_float2(0.0f,0.45f)) );
    swi2S(U,x,y, swi2(U,x,y) - dt*(to_float2(Bdx.x - Bdx.y, Bdy.x - Bdy.y) + ParticleXY) );
    
    
    if (Blend1>0.0) U = Blending(iChannel2, pos/size, U, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, pos, size);
    
    if(iFrame < 1 || Reset)
    {
        U = to_float4(30.0f*_floor(pos.x/30.0f)+60.0f*_sinf(pos.x),30.0f*_floor(pos.y/30.0f)-10.0f*_cosf(pos.y+pos.y),1.0f,1.0f);
    }

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1




__DEVICE__ float4 pdensity(float2 pos, float2 size, __TEXTURE2D__ iChannel0)
{
   //solve the dampened poisson equation
   const float radius = 1.5f;
float BBBBBBBBBBBBBBBBBBBBB;   
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float4(particle_param.z,particle_param.w,1.0f,1.0f)*gauss(pos - swi2(particle_param,x,y), radius);
}

__DEVICE__ float4 BB(float2 pos, float2 size, __TEXTURE2D__ iChannel1)
{
   return SAMPLE(iChannel1, pos, size);
}


__KERNEL__ void FireworkWavesFuse__Buffer_B(float4 U, float2 pos, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_POINT1(dampXY, 0.005f, 0.05f);
    CONNECT_POINT2(dampZW, 0.001f, 0.001f);
    CONNECT_POINT3(amplXY, 0.05f, 0.05f);
    CONNECT_POINT4(amplZW, 0.001f, 0.001f);
    
    pos+=0.5f;
    
    //const float4 damp = to_float4(0.005f,0.05f,0.001f,0.001f);
    //const float4 ampl = to_float4(0.05f,0.05f,0.001f,0.001f);
    float4 damp = to_float4_f2f2(dampXY,dampZW);
    float4 ampl = to_float4_f2f2(amplXY,amplZW);
    
    float4 density = pdensity(pos,size,iChannel0);
    U = (to_float4_s(1.0f)-damp)*0.25f*(BB(pos+to_float2(0,1),size,iChannel1)
                                       +BB(pos+to_float2(1,0),size,iChannel1)
                                       +BB(pos-to_float2(0,1),size,iChannel1)
                                       +BB(pos-to_float2(1,0),size,iChannel1));
    U += density*ampl;

    if(iFrame < 1 || Reset)
    {
      U = to_float4_s(0.0f);
    }

  SetFragmentShaderComputedColor(U);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Fork of "Hellflame" by michael0884. https://shadertoy.com/view/3d3SRS
// 2019-10-27 20:38:34

__KERNEL__ void FireworkWavesFuse(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
  
   pos+=0.5f;
    
   float4 density = 1.3f*SAMPLE(iChannel1, pos, size);
   fragColor = to_float4_aw(sin_f3(2.0f*density.x*to_float3(0.4f,0.2f,1.1f)) + sin_f3(5.0f*to_float3(0.7f,0.7f,1.0f)*density.y),1.0f);

    if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
    if (ApplyColor)
    {
      fragColor = (fragColor + (Color-0.5f))*fragColor.w;
      fragColor.w = Color.w;
    }

  SetFragmentShaderComputedColor(fragColor);
}
