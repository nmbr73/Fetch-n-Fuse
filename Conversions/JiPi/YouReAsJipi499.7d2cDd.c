
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Gray Noise Medium' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//Set initial condition / perturbation / advection / dissipation

//Select scheme to do backward advection
//#define EULER
#define RUNGE

// comment all to remove driving force.
//#define VORTICES
//#define LATTICE
#define NOISE




//macro
//#define GetVelocity(I,J) texelFetch( iChannel3, ijCoord+to_int2(I,J), 0 ).xy
//#define GetDensity(I,J) texelFetch( iChannel1, ijCoord+to_int2(I,J), 0 ).y

#define GetVelocityUV(XY) swi2(texture( iChannel3, (XY)),x,y)
#define GetDensityUV(XY) texture( iChannel1, (XY)).y



//http://mathworld.wolfram.com/HeartCurve.html
__DEVICE__ float2 getHeartPosition(float t){
    return to_float2( 16.0 * _sinf(t) * _sinf(t) * _sinf(t),
                     (13.0 * _cosf(t) - 5.0 * _cosf(2.0*t)
                      -2.0 * _cosf(3.0*t) - _cosf(4.0*t)));
}

__DEVICE__ float2 Euler(float2 posUV, float2 iResolution, float dt, __TEXTURE2D__ iChannel3){
    float2 AspectRatio = iResolution/iResolution.y;
    return dt*GetVelocityUV(posUV)/AspectRatio;
}

__DEVICE__ float2 Runge(float2 posUV, float2 iResolution, float dt, __TEXTURE2D__ iChannel3){
    float2 AspectRatio = iResolution/iResolution.y;
    float2 k1 = GetVelocityUV(posUV)/AspectRatio;
    float2 k2 = GetVelocityUV(posUV-0.5f*k1*dt)/AspectRatio;
    float2 k3 = GetVelocityUV(posUV-0.5f*k2*dt)/AspectRatio;
    float2 k4 = GetVelocityUV(posUV-k3*dt)/AspectRatio;
    return dt/6.0f*(k1+2.0f*k2+2.0f*k3+k4);
}

//main
__KERNEL__ void YouReAsJipi499Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(Thickness, 0.001f, 10.0f, 0.01f);
    CONNECT_SLIDER1(Speed, 0.01f, 10.0f, 1.0f);
    
    fragCoord +=0.5f;
    
    // must be modified in all buffers and image
    // simulation parameters
    const float dt = 1.0f/400.0f;
    const float reynold = 200.0f;
    const float vorticesStrength = 5.0f; //strength of vortices
    const float sourceStrength = 40.0f; //stength of sources
    const float kappa = 0.0f; //substance diffusion constant
    const float alpha = 12.0f; //substance dissipation rate
    const float radius = 0.07f; //radius of sources
   
    
    // set grid
    float dx = 1.0f / iResolution.y;
    float dxPow = dx *dx ;
    float2 uvCoord = dx*fragCoord;
    int2 ijCoord = to_int2_cfloat(_floor(fragCoord));
    
    // advect via semi-lagrangian method
    float vorticity = vorticesStrength*(texture(iChannel0, mod_f(fragCoord/ iResolution.y-to_float2(0,0.04f*iTime), iResolution/ iResolution.y)).x-0.5f);
    
   
    float2 posUV =fragCoord/iResolution;
    #ifdef EULER
    float2 posAdvUV = posUV-Euler(posUV,iResolution,dt, iChannel3);
    #endif
    #ifdef RUNGE
    float2 posAdvUV = posUV-Runge(posUV,iResolution,dt, iChannel3);
    #endif
    float densityAdv = GetDensityUV(posAdvUV)/(1.0f+dt*alpha);


    // add ice with mouse
    float pert = length((fragCoord - swi2(iMouse,x,y)) / iResolution.y); 
    if(iMouse.z > 0.0f && pert < radius) {
        densityAdv += dt*sourceStrength;
    }
    
    //if (length(swi2(uvCoord,x,y)-dx*_floor(0.5f*iResolution) +to_float2(0.5f*_sinf(0.75f*iTime),0.3f*_cosf(1.0f*iTime)))<=radius)
    if (length(fragCoord/iResolution-(getHeartPosition(iTime*Speed)/40.0f+0.5f)) < Thickness)
    {
        densityAdv += dt*sourceStrength;
    }
    
    fragColor = to_float4(vorticity, densityAdv, 0, 0); 
    
    if(Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


// solve for diffusion



//macro
#define GetVorticity(I,J) texture( iChannel0, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).x
#define GetDensity(I,J) texture( iChannel0, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).y

__KERNEL__ void YouReAsJipi499Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
    fragCoord +=0.5f;

    // must be modified in all buffers and image
    // simulation parameters
    const float dt = 1.0f/400.0f;
    const float reynold = 200.0f;
    const float vorticesStrength = 5.0f; //strength of vortices
    const float sourceStrength = 40.0f; //stength of sources
    const float kappa = 0.00005f; //substance diffusion constant
    const float alpha = 12.0f; //substance dissipation rate
    const float radius = 0.07f; //radius of sources

    
    // set grid
    float dx = 1.0f / iResolution.y;
    float dxPow = dx *dx ;
    float2 uvCoord = dx*fragCoord;
    int2 ijCoord = to_int2_cfloat(_floor(fragCoord));

    
    //to compute finite difference approximaton
    float vortij = GetVorticity(0,0); 
    
    //to compute density finite difference approximaton
    float densij = GetDensity(0,0); 
    float densip1j = GetDensity(1,0); 
    float densim1j = GetDensity(-1,0); 
    float densijp1 = GetDensity(0,1); 
    float densijm1 = GetDensity(0,-1); 
    
    
    //should use more than 1 iteration...
    //solve with jacobi for new velocity with laplacian
    float coef = kappa*dt/(dxPow);
    densij = (densij+coef*(densip1j+densim1j+densijp1+densijm1))/(1.0f+4.0f*coef);
    
    fragColor = to_float4(vortij,densij,0,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// solve for stream



//macro
//#define GetVorticity(I,J) texture( iChannel0, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).x
#define GetStream(I,J) texture( iChannel2, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).x

__KERNEL__ void YouReAsJipi499Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord +=0.5f;
    
    // must be modified in all buffers and image
    // simulation parameters
    const float dt = 1.0f/400.0f;
    const float reynold = 200.0f;
    const float vorticesStrength = 5.0f; //strength of vortices
    const float sourceStrength = 40.0f; //stength of sources
    const float kappa = 0.0f; //substance diffusion constant
    const float alpha = 12.0f; //substance dissipation rate
    const float radius = 0.07f; //radius of sources

    //set grid
    float dx = 1.0f / iResolution.y;
    float dxPow = dx *dx ;
    float2 uvCoord = dx*fragCoord;
    int2 ijCoord = to_int2_cfloat(_floor(fragCoord));

        //to compute finite difference approximaton
        float vortij = GetVorticity(0,0); 

        //to compute finite difference approximaton
        float streamij = GetStream(0,0); 
        float streamip1j = GetStream(1,0); 
        float streamim1j = GetStream(-1,0); 
        float streamijp1 = GetStream(0,1);
        float streamijm1 = GetStream(0,-1);

        //Set boundary condition (image method) 
        // Left

        if (ijCoord.x == 0) 
        {
            streamim1j = GetStream(-ijCoord.x+(int)(iResolution.x)-1,0);
        }
        // Right
        if (ijCoord.x == int(iResolution.x)-1) 
        {
            streamip1j = GetStream(-ijCoord.x,0);
        }

        // Down
        if (ijCoord.y == 0) 
        {
            streamijm1 = GetStream(0,-ijCoord.y+(int)(iResolution.y)-1);
        }

        // Up
        if (ijCoord.y == (int)(iResolution.y)-1) 
        {
            streamijp1 = GetStream(0,-ijCoord.y);
        }


        // should use more than 1 iteration...
        // compute stream via jacobi iteration... 
        // sadly it take a while for the stream initial condition to be computed...
        streamij = (-vortij+dt/dxPow*(streamip1j+streamim1j+streamijp1+streamijm1))/(1.0f+4.0f*dt/dxPow);

        fragColor = to_float4(streamij,0,0,0);

  if(Reset) fragColor = to_float4_s(0.0f);  

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// compute velocity



//macro
//#define GetStream(I,J) texture( iChannel2, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).x
#define GetVelocity(I,J) swi2(texture( iChannel3, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution),x,y)

__KERNEL__ void YouReAsJipi499Fuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord += 0.5f;

    // must be modified in all buffers and image
    // simulation parameters
    const float dt = 1.0f/400.0f;
    const float reynold = 200.0f;
    const float vorticesStrength = 5.0f; //strength of vortices
    const float sourceStrength = 40.0f; //stength of sources
    const float kappa = 0.0f; //substance diffusion constant
    const float alpha = 12.0f; //substance dissipation rate
    const float radius = 0.07f; //radius of sources


    //set grid
    float dx = 1.0f / iResolution.y;
    float dxPow = dx *dx ;
    float2 uvCoord = dx*fragCoord;
    int2 ijCoord = to_int2_cfloat(_floor(fragCoord));
        //to compute finite difference approximaton
        float streamij = GetStream(0,0); 
        float streamip1j = GetStream(1,0); 
        float streamim1j = GetStream(-1,0); 
        float streamijp1 = GetStream(0,1);
        float streamijm1 = GetStream(0,-1);

        //Set boundary condition (image method) 
        // Left

        if (ijCoord.x == 0) 
        {
            streamim1j = GetStream(-ijCoord.x+(int)(iResolution.x)-1,0);
        }
        // Right
        if (ijCoord.x == int(iResolution.x)-1) 
        {
            streamip1j = GetStream(-ijCoord.x,0);
        }

        // Down
        if (ijCoord.y == 0) 
        {
            streamijm1 = GetStream(0,-ijCoord.y+(int)(iResolution.y)-1);
        }

        // Up
        if (ijCoord.y == (int)(iResolution.y)-1) 
        {
            streamijp1 = GetStream(0,-ijCoord.y);
        }

        //compute velocity from stream function
        float2 uij =  0.5f*to_float2(streamijp1-streamijm1, -(streamip1j-streamim1j))/dx+to_float2(0.0f,0.25f);

        fragColor = to_float4(uij.x,uij.y, 0, 0);

  //if(Reset) fragColor = to_float4_s(0.0f);  

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Variation of a precedent project
// https://www.shadertoy.com/view/Xt2cRV

// added substance advection/diffusion along the flow (like die coloring in fluid experiment)
// you can modify the parameter (but do it in all buffer)

// Made it look like ice accidently so decided to keep it



//macro
//#define GetVorticity(I,J) texelFetch( iChannel0, ijCoord+to_int2(I,J), 0 ).x
//#define GetStream(I,J) texelFetch( iChannel2, ijCoord+to_int2(I,J), 0 ).x
//#define GetVelocity(I,J) texelFetch( iChannel3, ijCoord+to_int2(I,J), 0 ).xy
#define GetDensity(I,J) texture( iChannel1, (make_float2(ijCoord+to_int2(I,J))+0.5f)/iResolution).y

// COLORMAP

__DEVICE__ float3 hot(float t)
{
    return to_float3(smoothstep(0.00f,0.33f,t),
                     smoothstep(0.33f,0.66f,t),
                     smoothstep(0.66f,1.00f,t));
}
// for testing purpose, https://www.shadertoy.com/view/4dlczB
__DEVICE__ float3 blackbody(float t)
{
  float Temp = t*7500.0f;
  float3 col = to_float3_s(255.0f);
  col.x = 56100000.0f * _powf(Temp,(-3.0f / 2.0f)) + 148.0f;
  col.y = 100.04f * _logf(Temp) - 623.6f;
  if (Temp > 6500.0f) col.y = 35200000.0f * _powf(Temp,(-3.0f / 2.0f)) + 184.0f;
  col.z = 194.18f * _logf(Temp) - 1448.6f;
  col = clamp(col, 0.0f, 255.0f)/255.0f;
  if (Temp < 1000.0f) col *= Temp/1000.0f;
  return col;
}



__KERNEL__ void YouReAsJipi499Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    fragCoord += 0.5f;

    // must be modified in all buffers and image
    // simulation parameters
    const float dt = 1.0f/400.0f;
    const float reynold = 200.0f;
    const float vorticesStrength = 5.0f; //strength of vortices
    const float sourceStrength = 40.0f; //stength of sources
    const float kappa = 0.0f; //substance diffusion constant
    const float alpha = 12.0f; //substance dissipation rate
    const float radius = 0.07f; //radius of sources


    //set grid
    float dx = 1.0f / iResolution.y;
    float dxPow = dx *dx ;
    float2 uvCoord = dx*fragCoord;
    int2 ijCoord = to_int2_cfloat(_floor(fragCoord));

    float3 col = blackbody(GetDensity(0,0));
    
    fragColor = to_float4_aw(col,1);


  SetFragmentShaderComputedColor(fragColor);
}