
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define pi 3.141f
#define pitwo 6.28318f

#define Gravity 9.81f
// LatticeSize at which Simulation is performed
#define LatticeSize 5.5f
// Maximum Depth of Water in Meter
#define MaxShallowWaterDepth 5.0f
#define RelInitialWaterDepth 0.5f
// Guaranteed stable at 1.0f
#define ViscosityModifier 1.0f
#define BedFrictionCoefficient 0.0f
#define TerrainHeightScale 5.0f
#define WindAmplitude 0.0f

// User Input
#define MouseRadius 25.0f
#define MouseForce to_float2(10.0f, 0)
#define MouseHeightInjection 0.1f

// Keys
//const int kA=65,kB=66,kC=67,kD=68,kE=69,kF=70,kG=71,kH=72,kI=73,kJ=74,kK=75,kL=76,kM=77,kN=78,kO=79,kP=80,kQ=81,kR=82,kS=83,kT=84,kU=85,kV=86,kW=87,kX=88,kY=89,kZ=90;
//const int k0=48,k1=49,k2=50,k3=51,k4=52,k5=53,k6=54,k7=55,k8=56,k9=57;
//const int kSpace=32,kLeft=37,kUp=38,kRight=39,kDown=40;





// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float2 GetWind(float2 uv,float iTime)
{
  return _sinf(iTime) * WindAmplitude *  to_float2(-_cosf(uv.y * pi * 4.0f), 0);
}

__DEVICE__ float GetTerrainHeight(float2 uv)
{
  float h = (0.4f * _fmaxf(_cosf(uv.x * pitwo), _cosf(uv.y * pitwo)) + 0.6f);
  return h * TerrainHeightScale;
}

//Inigo Quilez; https://www.iquilezles.org/www/articles/smin/smin.htm
__DEVICE__ float smoothmin(float a, float b, float k)
{
  float h = _fmaxf(k - _fabs(a - b), 0.0f) / k;
  return _fminf(a, b) - h * h * k * (1.0f / 4.0f);
}

__DEVICE__ void EnforceStabilityConditions(inout float2 *u, inout float *h, float WaterDepthLimit, float VelocityMagnitudeLimit)
{
  /*
  * u = velocity
  * g = gravity
  * h = water depth
  * gh = squared wave speed
  * e = lattice speed (speed at which information travels) / MacroscopicParticleSpeed
  *
  * Stability Requirements:
  * |u| < _sqrtf(gh)
  *  gh < e^2
  * u.u < e^2 && |u| << e
  *  |u| DeltaX / Viscosity < 1
  *
  * => |u| < e/6
  *    |u| < _sqrtf(gh) < _sqrtf(e^2) (this is always larger than e/6)
  *     h  < e^2/g
  */
  // The factors 0.96f and 0.2f were empirically found.
  // Smoothmin prevents introducing sharp edges, which then produce ripples and waves on the surface.  
  *h = smoothmin(*h, 0.96f * WaterDepthLimit, 0.2f * WaterDepthLimit);
  // VelocityMagnitudeLimit should always be smaller than _sqrtf(Gravity * *h)
  float speedLimit = VelocityMagnitudeLimit;

  float magnitudeU = length(*u);
  if (magnitudeU > 0.0f)
  {
    *u /= magnitudeU;
    *u *= _fminf(magnitudeU, speedLimit);
  }
}


__KERNEL__ void MacroscopiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel3)
{
  
  fragCoord += 0.5f;
  
  bool kViewMode = false;
  bool kMouseInputType = false;
  bool kResetSimulation = false;
  bool kToggleWater = false;
  
  // 5 6 7
  //  \|/     // D2Q9
  // 0-C-1    // Unweighted raw vectors
  //  /|\     // These need to be weighted with MacroscopicParticleSpeed
  // 2 3 4

  const float2 GridVectors[8] = 
    {
      to_float2(-1, 0), to_float2(1, 0),
      to_float2(-1, -1), to_float2(0, -1), to_float2(1, -1),
      to_float2(-1, 1), to_float2(0, 1), to_float2(1, 1)
    };
  const float2 ForceWeights = to_float2(1.0f / 3.0f, 1.0f / 12.0f);
  
  // ----------------------------------------------------------
  // Dynamic derived parameters
  // ----------------------------------------------------------
  float MinEddyViscosity = _sqrtf(Gravity * MaxShallowWaterDepth)  * MaxShallowWaterDepth / 6.0f;
  MinEddyViscosity *= ViscosityModifier;

  float MacroscopicParticleSpeed = (6.0f * MinEddyViscosity) / LatticeSize;
  float VelocityMagnitudeLimit = (1.0f / 6.0f) * MacroscopicParticleSpeed;

  float DeltaTime = LatticeSize / _sqrtf(Gravity * MaxShallowWaterDepth);
  
  float mpsPower = MacroscopicParticleSpeed * MacroscopicParticleSpeed;
  float WaterHeightLimit = mpsPower / Gravity;
  float4 ParticleSpeedDenominators;
  ParticleSpeedDenominators.x = 1.0f / (6.0f * mpsPower);
  ParticleSpeedDenominators.y = 1.0f / (3.0f * mpsPower);
  ParticleSpeedDenominators.z = 1.0f / (2.0f * mpsPower * mpsPower);
  ParticleSpeedDenominators.w = 1.0f / mpsPower;
  // ----------------------------------------------------------

  float2 inverseTextureSize = to_float2_s(1.0f) / iResolution;
  float2 uv = fragCoord * inverseTextureSize;
    
    // ----------------------------------------------------------
  // Early out for Reset/Init Simulation
  // ----------------------------------------------------------
  if(iFrame == 0)
  {
    float2 inverseTextureSize = to_float2_s(1.0f) / iResolution;
    float2 uv = fragCoord * inverseTextureSize;

    float2 initVelocity = to_float2(VelocityMagnitudeLimit, 0);
    float initHeight = RelInitialWaterDepth * MaxShallowWaterDepth;
    float terrainHeight = GetTerrainHeight(uv);

    fragColor = to_float4(initVelocity.x, initVelocity.y, _fmaxf(initHeight - terrainHeight, 0.0f), terrainHeight);
    
    SetFragmentShaderComputedColor(fragColor);
    return;
  }

  // ----------------------------------------------------------
  // Simulated Outputs
  // ----------------------------------------------------------
  float2 newVelocity = to_float2(0.0f, 0.0f);
  float newHeight = 0.0f;
  
  // ----------------------------------------------------------
  // Other & Intermediates
  // ----------------------------------------------------------
  float hCenterHalf;
  float terrainElevationCenter = GetTerrainHeight(uv);

  float2 externalForce = to_float2(0.0f, 0.0f);
  float heightInjection = 0.0f;
  
  externalForce += GetWind(uv,iTime);

  // User Force Input
  if (kMouseInputType)
  {
    if( iMouse.z>0.1f && distance_f2(swi2(iMouse,x,y),fragCoord) < MouseRadius)
    {
      externalForce += MouseForce;
    }
  }
  else 
  {
    if (iMouse.z>0.1f && distance_f2(swi2(iMouse,x,y),fragCoord) < MouseRadius)
    {
      heightInjection = MouseHeightInjection;
    }
  }
  
  // ----------------------------------------------------------
  // First Handle hCenter
  // ---------------------------------------------------------- 
  {
    float h = _tex2DVecN(iChannel0,uv.x,uv.y,15).z + heightInjection;
    float2 u = swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y);

    hCenterHalf = h * 0.5f;

    float localEquilibrium = -5.0f * Gravity * h * ParticleSpeedDenominators.x;
    localEquilibrium -= 2.0f * dot(u, u) * ParticleSpeedDenominators.y;
    localEquilibrium = localEquilibrium * h + h;

    newHeight = localEquilibrium;
  }
  
      
  // ----------------------------------------------------------
  // Neighbours
  // ----------------------------------------------------------
  for (int i = 0; i < 8; i++)
  {
    float2 uvNeighbour = uv - GridVectors[i] * inverseTextureSize;
    float terrainElevation = GetTerrainHeight(uvNeighbour);
    float h = _tex2DVecN(iChannel0,uvNeighbour.x,uvNeighbour.y,15).z + heightInjection;
    float2 u = swi2(_tex2DVecN(iChannel0,uvNeighbour.x,uvNeighbour.y,15),x,y);

    float2 particleVelocityVector = -1.0f*GridVectors[i] * MacroscopicParticleSpeed;

    float2 bedShearStress = BedFrictionCoefficient * u * length(u);
    externalForce -= bedShearStress;

    float doteu = dot(particleVelocityVector, u);
    float localEquilibrium = 0.0f;

    if (i == 0 || i == 1 || i == 3 || i == 6) // Direct Neighbours
    {
      localEquilibrium = ParticleSpeedDenominators.x * (Gravity * h - dot(u, u));
      localEquilibrium += doteu * (doteu * ParticleSpeedDenominators.z + ParticleSpeedDenominators.y);
      localEquilibrium *= h;
    } else // Diagonal Neighbours
    {
      localEquilibrium = ParticleSpeedDenominators.x * (Gravity * h - dot(u, u));
      localEquilibrium += doteu * (doteu * ParticleSpeedDenominators.z + ParticleSpeedDenominators.y);
      localEquilibrium *= 0.25f * h;
    }
    
    // Terrain Bed influence
    float actingForce = 0.0f;
    float hOverline = 0.5f * h + hCenterHalf;
    float terrainSlope = terrainElevation - terrainElevationCenter;

     // (Gravity * hOverline *  ParticleSpeedDenominators.w) is hOverline / ShallowWaterVolumeHeight and therefore the relative height 0...1
    float relativeShallowWaterHeight = (Gravity * hOverline * ParticleSpeedDenominators.w);
    actingForce += relativeShallowWaterHeight * terrainSlope;

    actingForce += (DeltaTime * ParticleSpeedDenominators.w) * dot(particleVelocityVector, externalForce);
    if (i == 0 || i == 1 || i == 3 || i == 6) // Direct Neighbours
    {
      actingForce *= ForceWeights.x;
    }
    else
    {
      actingForce *= ForceWeights.y;
    }

    float hi = localEquilibrium + actingForce;
    newHeight += hi;
    newVelocity += particleVelocityVector * hi;
  }

  if (newHeight > 0.0f)
  {
    newVelocity /= newHeight;
  }
  else
  {
    newVelocity = to_float2(0.0f, 0.0f);
    newHeight = 0.0f;
  }

  EnforceStabilityConditions(&newVelocity, &newHeight, WaterHeightLimit, VelocityMagnitudeLimit);
  

  fragColor = to_float4(newVelocity.x,newVelocity.y, newHeight, terrainElevationCenter);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Macroscopic LBM Shallow Water by Nico Ell
// Contact: nico@nicoell.dev


// ----------------------------------------------------------
// Triangulator by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/lllGRr
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
// Contact the author for other licensing options

#define ITR 50
#define FAR 1000.0f
#define BASECOLOR to_float3(0.05f,0.1f,0.85f)

//__DEVICE__ mat2 mm2(in float a){float c = _cosf(a), s = _sinf(a);return to_mat2(c,-s,s,c);}
//__DEVICE__ mat2 m2 = mat2(0.934f, 0.358f, -0.358f, 0.934f);
//__DEVICE__ float tri(in float x){return _fabs(fract(x)-0.5f);}

__DEVICE__ float heightmap(in float2 p, bool kToggleWater, __TEXTURE2D__ iChannel0)
{
    float2 uv = (swi2(p,x,y) * 0.005f);
    uv.y += 0.45f;
    uv.x -= 0.5f;
    float waterDepth = _tex2DVecN(iChannel0,uv.x,uv.y,15).z;
    float terrainHeight = _tex2DVecN(iChannel0,uv.x,uv.y,15).w;
    if (kToggleWater)
    {
        return terrainHeight;
    } else
    {
        return (terrainHeight + waterDepth);
    }
}

//from jessifin (https://www.shadertoy.com/view/lslXDf)
__DEVICE__ float3 bary(float2 a, float2 b, float2 c, float2 p) 
{
    float2 v0 = b - a, v1 = c - a, v2 = p - a;
    float inv_denom = 1.0f / (v0.x * v1.y - v1.x * v0.y)+1e-9;
    float v = (v2.x * v1.y - v1.x * v2.y) * inv_denom;
    float w = (v0.x * v2.y - v2.x * v0.y) * inv_denom;
    float u = 1.0f - v - w;
    return abs_f3(to_float3(u,v,w));
}

/*
  Idea is quite simple, find which (triangular) side of a given tile we're in,
  then get 3 samples and compute height using barycentric coordinates.
*/
__DEVICE__ float map(float3 p, bool kToggleWater, __TEXTURE2D__ iChannel0)
{
    float3 q = fract_f3(p)-0.5f;
    float3 iq = _floor(p);
    float2 p1 = to_float2(iq.x-0.5f, iq.z+0.5f);
    float2 p2 = to_float2(iq.x+0.5f, iq.z-0.5f);
    
    float d1 = heightmap(p1,kToggleWater,iChannel0);
    float d2 = heightmap(p2,kToggleWater,iChannel0);
    
    float sw = sign_f(q.x+q.z); 
    float2 px = to_float2(iq.x+0.5f*sw, iq.z+0.5f*sw);
    float dx = heightmap(px,kToggleWater,iChannel0);
    float3 bar = bary(to_float2(0.5f*sw,0.5f*sw),to_float2(-0.5f,0.5f),to_float2(0.5f,-0.5f), swi2(q,x,z));
    return (bar.x*dx + bar.y*d1 + bar.z*d2 + p.y + 3.0f)*0.9f;
}

__DEVICE__ float march(in float3 ro, in float3 rd, bool kToggleWater, __TEXTURE2D__ iChannel0)
{
  float precis = 0.001f;
    float h=precis*2.0f;
    float d = 0.0f;
    for( int i=0; i<ITR; i++ )
    {
        if( _fabs(h)<precis || d>FAR ) break;
        d += h;
      float res = map(ro+rd*d,kToggleWater,iChannel0)*0.1f;
        h = res;
    }
  return d;
}

__DEVICE__ float3 normal(const in float3 p, bool kToggleWater, __TEXTURE2D__ iChannel0)
{  
    float2 e = to_float2(-1.0f, 1.0f)*0.01f;
  return normalize(swi3(e,y,x,x)*map(p + swi3(e,y,x,x),kToggleWater,iChannel0) + swi3(e,x,x,y)*map(p + swi3(e,x,x,y),kToggleWater,iChannel0) + 
                   swi3(e,x,y,x)*map(p + swi3(e,x,y,x),kToggleWater,iChannel0) + swi3(e,y,y,y)*map(p + swi3(e,y,y,y),kToggleWater,iChannel0) );   
}
// ----------------------------------------------------------

// From https://www.shadertoy.com/view/Xdy3zG
//fancy function to compute a color from the velocity
__DEVICE__ float4 computeColor(float normal_value)
{
    float3 color;
    if(normal_value<0.0f) normal_value = 0.0f;
    if(normal_value>1.0f) normal_value = 1.0f;
    float v1 = 0.01f/7.0f;
    float v2 = 2.0f/7.0f;
    float v3 = 3.0f/7.0f;
    float v4 = 4.0f/7.0f;
    float v5 = 5.0f/7.0f;
    float v6 = 6.0f/7.0f;
    //compute color
    if(normal_value<v1)
    {
      float c = normal_value/v1;
      color.x = 140.0f*(1.0f-c);
      color.y = 70.0f*(1.0f-c);
      color.z = 19.0f*(1.0f-c) + 91.0f*c;
    }
    else if(normal_value<v2)
    {
      float c = (normal_value-v1)/(v2-v1);
      color.x = 0.0f;
      color.y = 255.0f*c;
      color.z = 91.0f*(1.0f-c) + 255.0f*c;
    }
    else if(normal_value<v3)
    {
      float c = (normal_value-v2)/(v3-v2);
      color.x =  0.0f*c;
      color.y = 255.0f*(1.0f-c) + 128.0f*c;
      color.z = 255.0f*(1.0f-c) + 0.0f*c;
    }
    else if(normal_value<v4)
    {
      float c = (normal_value-v3)/(v4-v3);
      color.x = 255.0f*c;
      color.y = 128.0f*(1.0f-c) + 255.0f*c;
      color.z = 0.0f;
    }
    else if(normal_value<v5)
    {
      float c = (normal_value-v4)/(v5-v4);
      color.x = 255.0f*(1.0f-c) + 255.0f*c;
      color.y = 255.0f*(1.0f-c) + 96.0f*c;
      color.z = 0.0f;
    }
    else if(normal_value<v6)
    {
      float c = (normal_value-v5)/(v6-v5);
      color.x = 255.0f*(1.0f-c) + 107.0f*c;
      color.y = 96.0f*(1.0f-c);
      color.z = 0.0f;
    }
    else
    {
      float c = (normal_value-v6)/(1.0f-v6);
      color.x = 107.0f*(1.0f-c) + 223.0f*c;
      color.y = 77.0f*c;
      color.z = 77.0f*c;
    }
    return to_float4(color.x/255.0f,color.y/255.0f,color.z/255.0f,1.0f);
}

// ----------------------------------------------------------
__KERNEL__ void MacroscopiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel3)
{

  bool kViewMode = false;
  bool kMouseInputType = false;
  bool kResetSimulation = false;
  bool kToggleWater = false;
   
  CONNECT_BUTTON0(Modi,0,ViewMode,InputType,ToggleWater) 
  Modi = Modi-1;
  
  if (Modi == 1) kViewMode = true;
  if (Modi == 2) kMouseInputType = true;
  if (Modi == 3) kToggleWater = true;
   
    float2 uv = fragCoord/iResolution;
    float4 velocityHeight = _tex2DVecN(iChannel0,uv.x,uv.y,15);

    float4 mixedColor;
    if (kViewMode == true)
    {
        if (kToggleWater == true)
        {
            mixedColor = to_float4_aw(swi3(computeColor((velocityHeight.w / MaxShallowWaterDepth)),x,y,z), 1.0f);
        }
        else
        {
            mixedColor = to_float4_aw(swi3(computeColor((velocityHeight.z / MaxShallowWaterDepth)),x,y,z), 1.0f);
        }
    }
    else
    {
      float zzzzzzzzzzzzzzzzzzzzzzzzzz;
        float2 eps = to_float2(0.1f, 0.0f);
    
        float2 st = fragCoord / iResolution;
        float finv = _tanf(40.0f * 0.5f * pi / 180.0f);
        float aspect = iResolution.x / iResolution.y;
        st.x = st.x * aspect;
        st = (st - to_float2(aspect * 0.5f, 0.95f)) * finv;

        float3 ro = to_float3(0.0f, 75.0f, 0.0f);
        float3 rd = normalize(to_float3_aw(st, 0.5f));

        float rz = march(ro,rd,kToggleWater,iChannel0);
        float3 col = to_float3_s(0.0f);

        if ( rz < FAR ) 
        {
            float3 pos = ro+rz*rd;
            float3 nor= normal(pos,kToggleWater,iChannel0);
            float3 ligt = normalize(to_float3(-0.2f, 0.05f, -0.2f));

            float dif = clamp(dot( nor, ligt ), 0.0f, 1.0f);
            float fre = _powf(clamp(1.0f+dot(nor,rd),0.0f,1.0f), 3.0f);
            float3 brdf = 2.0f*to_float3(0.10f,0.11f,0.1f);
            brdf += 1.9f*dif*to_float3(0.8f,1.0f,0.05f);
            col = BASECOLOR;
            col = col*brdf + fre*0.5f*to_float3(0.7f,0.8f,1.0f);
        }
        col = clamp(col,0.0f,1.0f);
        col = pow_f3(col,to_float3_s(0.9f));
        col *= _powf( 16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.1f);
        
        mixedColor = to_float4_aw(col, 1.0f);
    }


    fragColor = mixedColor;


  SetFragmentShaderComputedColor(fragColor);
}