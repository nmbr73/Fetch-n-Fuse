
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
#define SPEED
#define BLASTER
   
#define PI 3.14159265f

#ifdef SPEED
//high speed
    #define dt 8.5f
    #define P 0.01f
#else
//high precision
   #define dt 2.0f
    #define P 0.05f
#endif

//how many particles per pixel, 1 is max
#define particle_density 1.0f
#define minimal_density 0.8f

const float radius = 2.0f;

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


//voronoi particle tracking 

__DEVICE__ void Check(inout float4 *U, float2 pos, float2 dx, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float4 Unb = SAMPLE(iChannel0, pos+dx, size);
    float2 rpos1 = mod_f2f2(pos-swi2(Unb,x,y)+size*0.5f,size) - size*0.5f;
    float2 rpos2 = mod_f2f2(pos-swi2(*U,x,y)+size*0.5f,size) - size*0.5f;
    //check if the stored neighbouring particle is closer to this position 
    if(length(rpos1) < length(rpos2))
    {
        *U = Unb; //copy the particle info
    }
}

__DEVICE__ float4 B(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel1)
{
   return 5.0f*SAMPLE(iChannel1, pos, size);
}

__KERNEL__ void LavaBlasterFuse__Buffer_A(float4 U, float2 pos, float2 iResolution, float iTime, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  
    pos+=0.5f;

    U = SAMPLE(iChannel0, pos, size);
float AAAAAAAAAAAAAAA;    
    //check neighbours 
    Check(&U, pos, to_float2(-1,0),iResolution,iChannel0);
    Check(&U, pos, to_float2(1,0),iResolution,iChannel0);
    Check(&U, pos, to_float2(0,-1),iResolution,iChannel0);
    Check(&U, pos, to_float2(0,1),iResolution,iChannel0);
    Check(&U, pos, to_float2(-1,-1),iResolution,iChannel0);
    Check(&U, pos, to_float2(1,1),iResolution,iChannel0);
    Check(&U, pos, to_float2(1,-1),iResolution,iChannel0);
    Check(&U, pos, to_float2(1,-1),iResolution,iChannel0);
    swi2S(U,x,y, mod_f2f2(swi2(U,x,y),size)); //limit the position to the texture
    
    //make new particles by diverging existing ones
   if(length(mod_f2f2(pos-swi2(U,x,y)+size*0.5f,size) - size*0.5f) > 1.0f/minimal_density)
    {
        //swi2(U,x,y) = pos;
        U.x = pos.x;
        U.y = pos.y;
    }

    float2 ppos = swi2(U,x,y);

    float2 pressure = to_float2( B(ppos+to_float2(1,0),iResolution,iChannel1).z - B(ppos+to_float2(-1,0),iResolution,iChannel1).z,
                                 B(ppos+to_float2(0,1),iResolution,iChannel1).z - B(ppos+to_float2(0,-1),iResolution,iChannel1).z);
    //mouse interaction
    if(iMouse.z>0.0f)
    {
        float k = gauss(ppos-swi2(iMouse,x,y), 25.0f);
        swi2S(U,z,w, swi2(U,z,w)*(1.0f-k) + k*0.2f*to_float2(_cosf(0.02f*iTime*dt), _sinf(0.02f*iTime*dt)));
    }
  
    #ifdef BLASTER
     swi2S(U,z,w, swi2(U,z,w) + 0.002f*to_float2(_cosf(0.01f*iTime*dt), _sinf(0.01f*iTime*dt))*gauss(ppos-size*to_float2(0.5f,0.5f),8.0f)*dt);
    #endif
    
    //update the particle
    swi2S(U,z,w, swi2(U,z,w)*0.9995f); // decrease velocity with time
    swi2S(U,z,w, swi2(U,z,w) + P*pressure*dt);
    //smooth velocity
    float2 velocity = 0.0f*swi2(B(ppos,iResolution,iChannel1),x,y) + swi2(U,z,w);
    swi2S(U,x,y, swi2(U,x,y) + dt*velocity);
    swi2S(U,x,y, mod_f(swi2(U,x,y),size)); //limit the position to the texture
    
    if(iFrame < 1)
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


#ifdef XXX
vec4 B(float2 pos)
{
   return SAMPLE(iChannel1, pos, size);
}
#endif

//density and velocity
__DEVICE__ float3 pdensity(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
   float4 particle_param = SAMPLE(iChannel0, pos, size);
   return to_float3_aw(swi2(particle_param,z,w),gauss(pos - swi2(particle_param,x,y), 0.7f*radius));
}



__KERNEL__ void LavaBlasterFuse__Buffer_B(float4 u, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    pos+=0.5f; 

    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);

    float4 prev_u = SAMPLE(iChannel1, pos, size);
   
    float3 density = pdensity(pos,iResolution,iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z,  0.5f*density);
    float div = B(pos+to_float2(1,0),iResolution,iChannel1).x-B(pos-to_float2(1,0),iResolution,iChannel1).x+B(pos+to_float2(0,1),iResolution,iChannel1).y-B(pos-to_float2(0,1),iResolution,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),iResolution,iChannel1)+B(pos+to_float2(1,0),iResolution,iChannel1)+B(pos-to_float2(0,1),iResolution,iChannel1)+B(pos-to_float2(1,0),iResolution,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1

#ifdef XXX
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



__KERNEL__ void LavaBlasterFuse__Buffer_C(float4 u, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    pos+=0.5f;

    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);

    float4 prev_u = SAMPLE(iChannel1, pos, size);
  
    float3 density = pdensity(pos,iResolution,iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z, 0.5f*density);
    float div = B(pos+to_float2(1,0),iResolution,iChannel1).x-B(pos-to_float2(1,0),iResolution,iChannel1).x+B(pos+to_float2(0,1),iResolution,iChannel1).y-B(pos-to_float2(0,1),iResolution,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),iResolution,iChannel1)+B(pos+to_float2(1,0),iResolution,iChannel1)+B(pos-to_float2(0,1),iResolution,iChannel1)+B(pos-to_float2(1,0),iResolution,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel1

#ifdef XXX
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


__KERNEL__ void LavaBlasterFuse__Buffer_D(float4 u, float2 pos, float2 iResolution,  sampler2D iChannel0, sampler2D iChannel1)
{

    pos+=0.5f;

    const float2 damp = to_float2(0.000f,0.01f);
    const float2 ampl = to_float2(0.1f,1.0f);

    float4 prev_u = SAMPLE(iChannel1, pos, size);
  
    float3 density = pdensity(pos, iResolution, iChannel0);
    //exponential rolling average
    swi3S(u,x,y,z, 0.5f*density);
    float div = B(pos+to_float2(1,0),iResolution,iChannel1).x-B(pos-to_float2(1,0),iResolution,iChannel1).x+B(pos+to_float2(0,1),iResolution,iChannel1).y-B(pos-to_float2(0,1),iResolution,iChannel1).y;
    swi2S(u,z,w, (1.0f-0.001f)*0.25f*swi2((B(pos+to_float2(0,1),iResolution,iChannel1)+B(pos+to_float2(1,0),iResolution,iChannel1)+B(pos-to_float2(0,1),iResolution,iChannel1)+B(pos-to_float2(1,0),iResolution,iChannel1)),z,w));
    swi2S(u,z,w, swi2(u,z,w) + ampl*to_float2(div,density.z));

  SetFragmentShaderComputedColor(u);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


// Fork of "Vorofluid" by michael0884. https://shadertoy.com/view/3sdXRX
// 2019-11-01 23:20:11

// Fork of "Voronoi vortex particle fluid" by michael0884. https://shadertoy.com/view/WdcXzS
// 2019-10-30 21:27:02
//const int KEY_UP = 38;
//const int KEY_DOWN  = 40;
#ifdef XXX
__DEVICE__ float4 B(float2 pos)
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

__KERNEL__ void LavaBlasterFuse(float4 fragColor, float2 pos, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
   CONNECT_CHECKBOX0(KEY_UP, 0);

   float3 density = pdensity(pos,iResolution,iChannel0);
   float4 blur = SAMPLE(iChannel1, pos, size);
    float vorticity = B(pos+to_float2(1,0),iResolution,iChannel1).y-B(pos-to_float2(1,0),iResolution,iChannel1).y-B(pos+to_float2(0,1),iResolution,iChannel1).x+B(pos-to_float2(0,1),iResolution,iChannel1).x;
   //fragColor = to_float4(SAMPLE(iChannel2, pos, size).xyz  + 0.8f*to_float3(0.4f,0.6f,0.9f)*vorticity,1.0f);
    if(KEY_UP)
    {
        fragColor = to_float4_aw(2.0f*density.z*(7.0f*abs_f3(swi3(density,x,y,y))+to_float3(0.2f, 0.1f, 0.1f)),1.0f);
        fragColor = to_float4_aw(10.0f*abs_f3(swi3(density,x,y,y)) + 30.0f*to_float3(0,0,_fabs(blur.z)),1.0f);
    }
    else
    {
       float l1 = 490.0f*_fabs(vorticity);
       float l2 = 1.0f-l1;
       fragColor = to_float4_aw(to_float3(1.0f,0.3f,0.1f)*l1 + 0.0f*to_float3(0.1f,0.1f,0.1f)*l2,1.0f);
    }  


  SetFragmentShaderComputedColor(fragColor);
}