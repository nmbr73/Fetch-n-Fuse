
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

//#define dt 0.25f





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
  return fract(to_float2(a.x * a.y, a.y * a.z));
}

__DEVICE__ bool reset(__TEXTURE2D__ sam) {
    return texture(sam, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
}

// particle
// x,y => pos [0 > iResolution]
// z   => angle [0 > 2pi]
// _fabs(w) => sensor angle [0 > pi]
// if (w > 0.0f) => activation

__DEVICE__ float4 getParticle(float2 p, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  return texture(iChannel0, (make_float2(to_int2_cfloat(loop(p, iResolution)))+0.5)/R);
}

__DEVICE__ float4 getPheromone(float2 p, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  return texture(iChannel1, (make_float2(to_int2_cfloat(loop(p, iResolution)))+0.5)/R);
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
          Q = _mix(Q, to_float4(U.x,U.y,(tex.x+MulOff.y)*MulOff.x,(tex.x+MulOff.y)*MulOff.x), Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          swi2S(Q,z,w, _mix(swi2(Q,z,w), (swi2(tex,x,y)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q, to_float4(U.x,U.y, Par.x, Par.y), Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q, to_float4(U.x,U.y, (tex.x+MulOff.y)*MulOff.x, (tex.x+MulOff.y)*MulOff.x),Blend);
    }
  
  return Q;
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// Created by Stephane Cuillerdier - Aiekick/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.



__DEVICE__ void SelectIfNearestNeighbor(inout float4 *pnb, float2 p, float2 dx, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float4 p_nb = getParticle(p + dx, iResolution, iChannel0);
    
  if(length(loop_d(swi2(p_nb,x,y) - p, iResolution)) < length(loop_d(swi2(*pnb,x,y) - p, iResolution)))
    {
        *pnb = p_nb;
    }
}

__DEVICE__ void SearchForNearestNeighbor(inout float4 *pnb, float2 p, float ring, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  // sides
    SelectIfNearestNeighbor(pnb, p, to_float2(-ring,0),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2(ring,0),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2(0,-ring),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2(0,ring),R,iChannel0);
  
  // corners
    SelectIfNearestNeighbor(pnb, p, to_float2_s(-ring),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2(ring,-ring),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2(-ring,ring),R,iChannel0);
    SelectIfNearestNeighbor(pnb, p, to_float2_s(ring),R,iChannel0);
}

__DEVICE__ float4 EmitParticle(float2 g, float4 p, float sensor_angle_rad_inf, float sensor_angle_rad_sup)
{
  float rand = Random(g + swi2(p,x,y)).x;
  
  //swi2(p,x,y) = g; // pos
  p.x = g.x; // pos
  p.y = g.y; // pos
  
  p.z = rand * 6.28318f; // angle

  // sensor angle and activation
  p.w = _mix(sensor_angle_rad_inf, sensor_angle_rad_sup, rand);
  
  return p;
}

__DEVICE__ float4 MoveParticle(float4 p, float particle_speed,float sensor_strenght, float sensor_distance, float dt, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  // left sensor
  float an = p.z + p.w;
  float2 sleft = swi2(p,x,y) + sensor_distance * to_float2(_cosf(an), _sinf(an));

  // right sensor
  an = p.z - p.w;
  float2 sright = swi2(p,x,y) + sensor_distance * to_float2(_cosf(an), _sinf(an));
    
  float diff_angle = 
                    getPheromone(sleft,R,iChannel1).x - 
                    getPheromone(sright,R,iChannel1).x;

  p.z += dt * sensor_strenght * _tanhf(0.3f * diff_angle);
  swi2S(p,x,y, swi2(p,x,y) + dt * particle_speed * to_float2(_cosf(p.z), _sinf(p.z)));
    
  swi2S(p,x,y, loop(swi2(p,x,y), iResolution));
  
  return p;
}

__DEVICE__ float4 PaintByMouse(float2 g, float4 p, float4 iMouse, float uMouseRadius, float sensor_angle_rad_inf, float sensor_angle_rad_sup)
{
  if (iMouse.z > 0.0f)
  {
    if (length(g - swi2(iMouse,x,y)) < uMouseRadius)
    {
      p = EmitParticle(g, p,sensor_angle_rad_inf,sensor_angle_rad_sup);
    }
  }
  
  return p;
}

__KERNEL__ void VoronoiTrackingExperiment3Fuse__Buffer_A(float4 fragParticles, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_SCREW0(uMouseRadius, 0.0f, 10.0f, 1.0f);
  //CONNECT_SCREW1(particle_speed, 0.0f, 10.0f, 5.0f);
  CONNECT_INTSLIDER0(Particle_Speed, 0, 100, 50);

  float particle_speed = (float)Particle_Speed / 10.0f;

  // sensor
  CONNECT_SCREW1(sensor_strenght, 0.0f, 50.0f, 20.0f);
  CONNECT_SCREW2(sensor_distance, 0.0f, 50.0f, 20.0f);
  CONNECT_SCREW3(sensor_angle_rad_inf, -1.0f, 5.0f, 1.0f);
  CONNECT_SCREW4(sensor_angle_rad_sup, -1.0f, 5.0f, 1.8f);
  
      //Blending
    CONNECT_SLIDER5(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER6(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
  
  CONNECT_SLIDER8(dt, -0.2f, 3.0f, 0.25f);
  
  
  fragCoord+=0.5f;

  fragParticles = getParticle(fragCoord,R,iChannel0);
  
  SearchForNearestNeighbor(&fragParticles, fragCoord, 1.0f,R,iChannel0);
  SearchForNearestNeighbor(&fragParticles, fragCoord, 2.0f,R,iChannel0);
  SearchForNearestNeighbor(&fragParticles, fragCoord, 3.0f,R,iChannel0);
  
  fragParticles = MoveParticle(fragParticles, particle_speed, sensor_strenght, sensor_distance, dt, R, iChannel1);
  fragParticles = PaintByMouse(fragCoord, fragParticles, iMouse, uMouseRadius,sensor_angle_rad_inf,sensor_angle_rad_sup);

  if (Blend1>0.0) fragParticles = Blending(iChannel2, fragCoord/R, fragParticles, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);


  if (iFrame < 1 || Reset) // reset 
  {
    fragParticles = to_float4_s(0);
        
    // start shape
    float2 uv = (fragCoord * 2.0f - iResolution) / iResolution.y;
    uv.y += _sinf(uv.x * 5.0f) * 0.3f;
    uv.x = mod_f(uv.x, 0.1f);
    float st = 5.0f / iResolution.y;
    if (length(uv) < st)
        fragParticles = EmitParticle(fragCoord, fragParticles,sensor_angle_rad_inf,sensor_angle_rad_sup);
  }
    
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

__DEVICE__ float4 DiffusePheromones(float2 g, float4 fragPheromone, float dt, float2 iResolution, __TEXTURE2D__ iChannel1)
{
    // laplacian
  float v = 0.0f;
  v += getPheromone(g + to_float2(-1, 0), R, iChannel1).x; // l
  v += getPheromone(g + to_float2( 0, 1), R, iChannel1).x; // t
  v += getPheromone(g + to_float2( 1, 0), R, iChannel1).x; // r
  v += getPheromone(g + to_float2( 0,-1), R, iChannel1).x; // b
  v -= 4.0f * fragPheromone.x;

  fragPheromone += dt * v;
  
  return fragPheromone;
}

__KERNEL__ void VoronoiTrackingExperiment3Fuse__Buffer_B(float4 fragPheromone, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
  CONNECT_CHECKBOX0(Reset, 0);
// pheromones
  CONNECT_SLIDER3(gauss_coef, -10.0f, 10.0f, 1.4f);
  CONNECT_SLIDER4(decay, -1.0f, 1.0f, 0.15f);
  
  CONNECT_SLIDER8(dt, -1.0f, 3.0f, 0.25f);
  
  fragCoord+=0.5f;


  fragPheromone = getPheromone(fragCoord, R, iChannel1);
  fragPheromone = DiffusePheromones(fragCoord, fragPheromone, dt, R, iChannel1);
  
  // write pheromones for each particles
  float4 p = getParticle(fragCoord, R, iChannel0);
  if (p.w > 0.0f)
  {
    float gauss = _expf(-_powf(length(fragCoord - swi2(p,x,y))/gauss_coef,2.0f));
    fragPheromone += dt * gauss;
  }
  
  // dissipation  
  fragPheromone -= dt * decay * fragPheromone;
    
  if (iFrame < 1 || Reset) // reset 
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

#define COUNT_STEPS 50
#define SCALE_STEP 0.005f

#ifdef XXX    
__DEVICE__ float4 getPheromone(float2 p)
{
  return texelFetch(iChannel1, to_int2(loop(p, iResolution)), 0);
}

__DEVICE__ float4 getPheromoneInv(float2 p)
{
  return getPheromone(p);
}
#endif

#define getPheromoneInv getPheromone

__DEVICE__ float4 layer(float2 p, float scale, float3 light, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  p -= iResolution * 0.5f;
  p *= scale;
  p += iResolution * 0.5f;
    
  float4 color;
  color.w = step(0.95f, clamp(getPheromone(p, iResolution, iChannel1).x*100.0f,0.0f,1.0f));
  swi3S(color,x,y,z, light * color.w);
  return color;
}

__KERNEL__ void VoronoiTrackingExperiment3Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
  CONNECT_COLOR0(Diffuse, 0.191553f, 0.267195f, 0.373984f, 1.0f);
  CONNECT_COLOR1(Specular, 0.243903f, 1.0f, 0.0f, 1.0f);
  CONNECT_COLOR2(LightDirection, 0.08232f, -0.24085f, -0.58841f, 1.0f);
  CONNECT_SLIDER1(specularPower, -10.0f, 100.0f, 20.0f);
  CONNECT_SLIDER2(Fog, -10.0f, 10.0f, 1.0f);

  // shading
  const float3 lightDiffuse = swi3(Diffuse,x,y,z);//to_float3(0.191553f,0.267195f,0.373984f);
  const float3 lightSpecular = swi3(Specular,x,y,z);//to_float3(0.243903f,1,0);
  const float3 lightDirection = swi3(LightDirection,x,y,z);//to_float3(0.08232f,-0.24085f,-0.58841f);
  //const float specularPower = 20.0f;

  //Background
  fragColor.w = Specular.w;

  // lighting
  float e = 100.0f / _fminf(iResolution.x, iResolution.y);
  float f = getPheromoneInv(fragCoord, R, iChannel1).x;
  float fx = (f-getPheromoneInv(fragCoord + to_float2(1,0), R, iChannel1).x)/e;
  float fy = (f-getPheromoneInv(fragCoord + to_float2(0,1), R, iChannel1).x)/e;
  float3 n = normalize(to_float3(0,0,1) - to_float3(fx,fy,0.0f));
  float diff = _fmaxf(dot(to_float3(0,0,1), n), 0.0f);
  float spec = _powf(_fmaxf(dot(normalize(lightDirection), reflect(to_float3(0,0,1),n)), 0.0f), specularPower);  
  float3 color = (clamp((lightDiffuse * diff + lightSpecular * spec) * 1.5f, 0.0f, 1.0f));
  
  // layering of same texture
  float fcount = float(_fmaxf(COUNT_STEPS, 1));
  float scale = 1.0f + fcount * SCALE_STEP;
  float fog = 0.0f;
  float fogStep = 1.0f / fcount;
  for (int i = 0 ; i < COUNT_STEPS ; ++i)
  {
    scale -= SCALE_STEP;
    fog += fogStep;
    float4 c = layer(fragCoord, scale, color, iResolution, iChannel1);
    if (c.w > 0.5f) // smart merge for avoid overwrite
      //swi3(fragColor,x,y,z) = swi3(c,x,y,z) * fog;
      fragColor = to_float4_aw(swi3(c,x,y,z) * fog * Fog , Diffuse.w);
  }


  SetFragmentShaderComputedColor(fragColor);
}