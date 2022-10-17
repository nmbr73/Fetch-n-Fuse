
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

#define dt 0.25


// borderless 
__DEVICE__ float2 loop_d(float2 p, float2 s){
  return mod_f2f2(p + s * 0.5f, s) - s * 0.5f;
}

__DEVICE__ float2 loop(float2 p, float2 s){
  return mod_f2f2(p, s);
}

__DEVICE__ float2 Random(float2 p){
  float3 a = fract_f3(swi3(p,x,y,x) * to_float3(123.34f,234.35f,345.65f));
  a += dot(a, a + 34.45f);
  return fract_f2(to_float2(a.x * a.y, a.y * a.z));
}


//__DEVICE__ bool reset(__TEXTURE2D__ sam) {
//    return texture(sam, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// particle
// x,y => pos [0 > iResolution]
// z   => angle [0 > 2pi]
// _fabs(w) => sensor angle [0 > pi]
// if (w > 0.0f) => activation

__DEVICE__ float4 getParticle(float2 p, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  return texture(iChannel0, (make_float2(to_int2_cfloat(loop(p, iResolution)))+0.5f)/iResolution);
}

__DEVICE__ float4 getPheromone(float2 p, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  return texture(iChannel1, (make_float2(to_int2_cfloat(loop(p, iResolution)))+0.5f)/iResolution);
}

__DEVICE__ void SelectIfNearestNeighbor(inout float4 *pnb, float2 p, float2 dx, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float4 p_nb = getParticle(p + dx,iResolution,iChannel0);
    
  if(length(loop_d(swi2(p_nb,x,y) - p, iResolution)) < length(loop_d(swi2(*pnb,x,y) - p, iResolution)))
    {
        *pnb = p_nb;
    }
}

__DEVICE__ void SearchForNearestNeighbor(inout float4 *pnb, float2 p, float ring, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  // sides
  SelectIfNearestNeighbor(pnb, p, to_float2(-ring,0),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2(ring,0),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2(0,-ring),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2(0,ring),iResolution,iChannel0);
  
  // corners
  SelectIfNearestNeighbor(pnb, p, to_float2_s(-ring),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2(ring,-ring),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2(-ring,ring),iResolution,iChannel0);
  SelectIfNearestNeighbor(pnb, p, to_float2_s(ring),iResolution,iChannel0);
}

__DEVICE__ void EmitParticle(float2 g, inout float4 *p)
{
  
  const float sensor_angle_rad_inf = 1.0f;
  const float sensor_angle_rad_sup = 1.8f;
  
  float rand = Random(g + swi2(*p,x,y)).x;
  
  //swi2(p,x,y) = g; // pos
  (*p).x=g.x;
  (*p).y=g.y;
  (*p).z = rand * 6.28318f; // angle

    // sensor angle and activation
  (*p).w = _mix(sensor_angle_rad_inf, sensor_angle_rad_sup, rand);
}

__DEVICE__ void MoveParticle(inout float4 *p, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  // particle
  const float particle_speed = 5.0f;
  // sensor
  const float sensor_strenght = 20.0f;
  const float sensor_distance = 20.0f;
    
  // left sensor
  float an = (*p).z + (*p).w;
  float2 sleft = swi2(*p,x,y) + sensor_distance * to_float2(_cosf(an), _sinf(an));
    
  // right sensor
  an = (*p).z - (*p).w;
  float2 sright = swi2(*p,x,y) + sensor_distance * to_float2(_cosf(an), _sinf(an));
    
  float diff_angle = 
        getPheromone(sleft,iResolution,iChannel1).x - 
        getPheromone(sright,iResolution,iChannel1).x;
  
  (*p).z += dt * sensor_strenght * _tanhf(0.3f * diff_angle);
  swi2S(*p,x,y, swi2(*p,x,y) + dt * particle_speed * to_float2(_cosf((*p).z), _sinf((*p).z)));
    
  swi2S(*p,x,y, loop(swi2(*p,x,y), iResolution));
}

__DEVICE__ void PaintByMouse(float2 g, inout float4 *p, float4 iMouse)
{
  // mouse
  const float uMouseRadius = 1.0f;
  
  if (iMouse.z > 0.0f)
  {
    if (length(g - swi2(iMouse,x,y)) < uMouseRadius)
    {
      EmitParticle(g, p);
    }
  }
}

__KERNEL__ void ExperimentMalzeFuse__Buffer_A(float4 fragParticles, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
 
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
 
  fragCoord += 0.5f;

  fragParticles = getParticle(fragCoord,iResolution,iChannel0);
  
  SearchForNearestNeighbor(&fragParticles, fragCoord, 1.0f,iResolution,iChannel0);
  SearchForNearestNeighbor(&fragParticles, fragCoord, 2.0f,iResolution,iChannel0);
  SearchForNearestNeighbor(&fragParticles, fragCoord, 3.0f,iResolution,iChannel0);
  
  MoveParticle(&fragParticles,iResolution,iChannel1);
  PaintByMouse(fragCoord, &fragParticles, iMouse);


  //Textureblending
  if (Blend1 > 0.0f)
  {
    float2 tuv = fragCoord/iResolution;
    float4 tex = texture(iChannel3, tuv);

    if (tex.w > 0.0f)
      EmitParticle(fragCoord, &fragParticles);
           
  }



  if (iFrame < 1) // reset 
  {
    fragParticles = to_float4_s(0);
        
    // start shape
    float2 uv = (fragCoord * 2.0f - iResolution) / iResolution.y;
    uv.y += _sinf(uv.x * 5.0f) * 0.3f;
    uv.x = mod_f(uv.x, 0.1f);
    float st = 5.0f / iResolution.y;
    if (length(uv) < st)
        EmitParticle(fragCoord, &fragParticles);
  }
    
  if (Reset)
    fragParticles = to_float4_s(0);


  SetFragmentShaderComputedColor(fragParticles);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// pheromone
// x   => pheromone quantity
/*
__DEVICE__ float4 getParticle(float2 p)
{
  return texelFetch(iChannel0, to_int2(loop(p, iResolution)), 0);
}

__DEVICE__ float4 getPheromone(float2 p)
{
  return texelFetch(iChannel1, to_int2(loop(p, iResolution)), 0);
}
*/

__DEVICE__ void DiffusePheromones(float2 g, inout float4 *fragPheromone,float2 iResolution,__TEXTURE2D__ iChannel1)
{
  // laplacian
  float v = 0.0f;
  v += getPheromone(g + to_float2(-1, 0),iResolution,iChannel1).x; // l
  v += getPheromone(g + to_float2( 0, 1),iResolution,iChannel1).x; // t
  v += getPheromone(g + to_float2( 1, 0),iResolution,iChannel1).x; // r
  v += getPheromone(g + to_float2( 0,-1),iResolution,iChannel1).x; // b
  v -= 4.0f * (*fragPheromone).x;

  *fragPheromone += dt * v;
}

__KERNEL__ void ExperimentMalzeFuse__Buffer_B(float4 fragPheromone, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(Reset, 0);
  
  fragCoord += 0.5f;

  // pheromones
  const float gauss_coef = 1.4f;
  const float decay = 0.15f;

  fragPheromone = getPheromone(fragCoord,iResolution,iChannel1);
    
  DiffusePheromones(fragCoord, &fragPheromone,iResolution,iChannel1);
  
  // write pheromones for each particles
  float4 p = getParticle(fragCoord,iResolution,iChannel0);
  if (p.w > 0.0f)
  {
    float gauss = _expf(-_powf(length(fragCoord - swi2(p,x,y))/gauss_coef,2.0f));
    fragPheromone += dt * gauss;
  }
  
  // dissipation  
  fragPheromone -= dt * decay * fragPheromone;
    
  if (iFrame < 1 || Reset) // || reset(iChannel3)) // reset 
    fragPheromone = to_float4_s(0);

  SetFragmentShaderComputedColor(fragPheromone);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Voronoi Tracking Experiment 1

// based on the shader https://www.shadertoy.com/view/tlKGDh of michael0884

// use mouse for add particles
// use spacebar for clear the screen
/*
__DEVICE__ float4 getPheromone(float2 p)
{
  return texelFetch(iChannel1, to_int2(loop(p, iResolution)), 0);
}
*/
__DEVICE__ float4 getPheromoneInv(float2 p,float2 iResolution,__TEXTURE2D__ iChannel1)
{
  return getPheromone(p,iResolution,iChannel1);
}

__KERNEL__ void ExperimentMalzeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{

  fragCoord += 0.5f;

  // shading
  const float3 lightDiffuse = to_float3(0.191553f,0.267195f,0.373984f);
  const float3 lightSpecular = to_float3(0.243903f,1,0);
  const float3 lightDirection = to_float3(0.08232f,-0.24085f,-0.58841f);
  const float specularPower = 20.0f;


  float e = 100.0f / _fminf(iResolution.x, iResolution.y);
  float f = getPheromoneInv(fragCoord,iResolution,iChannel1).x;
  float fx = (f-getPheromoneInv(fragCoord + to_float2(1,0),iResolution,iChannel1).x)/e;
  float fy = (f-getPheromoneInv(fragCoord + to_float2(0,1),iResolution,iChannel1).x)/e;
  float3 n = normalize(to_float3(0,0,1) - to_float3(fx,fy,0.0f));
  
  float diff = _fmaxf(dot(to_float3(0,0,1), n), 0.0f);
  float spec = _powf(_fmaxf(dot(normalize(lightDirection), reflect(to_float3(0,0,1),n)), 0.0f), specularPower);
    
  swi3S(fragColor,x,y,z, lightDiffuse * diff + lightSpecular * spec); 
  //swi3(fragColor,x,y,z) *= 1.5f;
  fragColor.x *= 1.5f; 
  fragColor.y *= 1.5f;
  fragColor.z *= 1.5f;
  
  //fragColor = getPheromone(fragCoord);
    
  fragColor.w = 1.0f;
  
  SetFragmentShaderComputedColor(fragColor);
}