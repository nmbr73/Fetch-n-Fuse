
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A'Previsualization: Buffer A' to iChannel0
// Connect Buffer A'Previsualization: Buffer B' to iChannel1

#define swi2S(a,b,c,d) {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// (C) Copyright 2021 by Yury Ershov

#define WALLS_BOUNCE   1

#define CH iChannel0


#define m_epsilon 0.000001f


__DEVICE__ float2 vclamp(float2 v) {

    float l = length(v);
    return l < 10.0f ? v : v/l;
}

// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash12(float2 position, float iTime)
{
    float2 p = (position + mod_f(iTime, 200.0f) * 1500.0f + 50.0f);
    float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract_f((p3.x + p3.y) * p3.z);
}

__DEVICE__ float overlapping_area(float2 l1, float2 r1, float2 l2, float2 r2)
{
    float x_dist = _fminf(r1.x, r2.x) - _fmaxf(l1.x, l2.x);
    float y_dist = _fminf(r1.y, r2.y) - _fmaxf(l1.y, l2.y);
    return x_dist > 0.0f && y_dist > 0.0f ? x_dist * y_dist : 0.0f;
}

__DEVICE__ float2 randomizespeed(float2 v, float iTime) {
  
  const float speed_rnd = 0.08f;
    return to_float2(v.x + (hash12(fract_f2(v)*1573.32f,iTime)-0.5f)*speed_rnd, v.y + (hash12(fract_f2(v)*178362.78f,iTime)-0.5f)*speed_rnd);
}


// Gas dynamics layer
__KERNEL__ void StarSystemsEvolutionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  
  const float dt = 1.0f;
  const float G = 0.01f;

    if (fragCoord.x < 1.5f || fragCoord.y < 1.5f || fragCoord.x > iResolution.x-1.5f || fragCoord.y > iResolution.y-1.5f) { 
      fragColor=to_float4_s(0.0f);
      //fragColor = texture(iChannel0,fragCoord / iResolution); // Test
      SetFragmentShaderComputedColor(fragColor);
      return; 
      }
    
    float2 uv = fragCoord / iResolution;

    if(iFrame < 2 || iMouse.z > 0.0f) {
        float r = distance_f2(fragCoord, iResolution/2.0f);
        if (r < 50.0f) {
            fragColor = to_float4(0.0f,-(uv.y - 0.5f)*30.0f/(r+1.0f)*50.0f,(uv.x - 0.5f)*30.0f/(r+1.0f)*50.0f,hash12(fragCoord,iTime)*5.0f*(52.0f-r)/20.0f);
        } else {
//            fragColor = to_float4(0.0f,0.0f,0.0f,hash12(fragCoord,iTime)*100.0f*m_epsilon);
            fragColor = to_float4_s(0.0f);
        }
        SetFragmentShaderComputedColor(fragColor);
        return;
    }

    float2 s1 = to_float2(1.0f, 1.0f) / iResolution;
    float4 pt0 = _tex2DVecN(iChannel0, ((float)((int)fragCoord.x)+0.5f)/iResolution.x, ((float)((int)fragCoord.y)+0.5f)/iResolution.y ,15);
    float2 speed0 = swi2(pt0,y,z);
    float mass0 = pt0.w;
    float2 f = to_float2_s(0.0f);
    float2 p = to_float2_s(0.0f);
    float mass2 = 0.0f;

    float2 sh;
    for (sh.y = -10.0f; sh.y < 10.5f; sh.y+=1.0f) for (sh.x = -10.0f; sh.x < 10.5f; sh.x+=1.0f) {
        float l = length(sh);
        float2 coord1 = fragCoord + sh;
        float4 pt1 = _tex2DVecN(iChannel0, ((float)((int)coord1.x)+0.5f)/iResolution.x, ((float)((int)coord1.y)+0.5f)/iResolution.y ,15);
        float2 speed1 = swi2(pt1,y,z);
        float mass1 = pt1.w;

        if (l > 3.5f) {    // must be 0.5f but bigger number prevents from collapsing into 2x2 dots.
            // Gravity, acceleration
            f += sh/l * mass0*mass1/l/l;
        }

        // speed: mass transfer, impulse change:
        float2 coord_next = coord1 + speed1 * dt;
        float overlap = overlapping_area(fragCoord, fragCoord+to_float2(1.0f,1.0f), coord_next, coord_next+to_float2(1.0f,1.0f));
        float dm = mass1 * overlap;
        mass2 += dm;
        p += speed1 * dm;
    }
    
    // Slight gravity towards the center to compensate the inability to feel further than 10 pts.
    float2 to_c = iResolution/2.0f - fragCoord;
    to_c = length(to_c) < 5.0f ? to_float2_s(0.0f) : normalize(to_c)/2000.0f;

    fragColor.x = 0.0f;
    fragColor.w = mass2 > m_epsilon ? mass2 : 0.0f;
    swi2S(fragColor,y,z, 
        mass2 > m_epsilon ?
        vclamp(randomizespeed(p/mass2 + dt * G*f/(mass0 > m_epsilon ? mass0 : mass2) + dt*to_c,iTime)) : to_float2_s(0.0f))
#if WALLS_BOUNCE
    if (fragCoord.x <= 11.0f) {
        if (fragColor.y < 0.0f) fragColor.y = _fabs(fragColor.y)/2.0f;
    } else if (fragCoord.x >= iResolution.x - 12.0f) {
        if (fragColor.y > 0.0f) fragColor.y = -_fabs(fragColor.y)/2.0f;
    }
    if (fragCoord.y <= 11.0f) {
        if (fragColor.z < 0.0f) fragColor.z = _fabs(fragColor.z)/2.0f;
    } else if (fragCoord.y >= iResolution.y - 12.0f) {
        if (fragColor.z > 0.0f) fragColor.z = -_fabs(fragColor.z)/2.0f;
    }
#endif

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image'Previsualization: Buffer A' to iChannel0
// Connect Image'Previsualization: Buffer B' to iChannel1


// (C) Copyright 2021 by Yury Ershov

// https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics

#define PALETTE        0    // 0=fire, 1=blue, 2=green

__DEVICE__ float3 col(float x) {
  return to_float3(
#if PALETTE == 0
      clamp(x, 0.0f, 1.0f/3.0f),
      clamp(x-1.0f/3.0f, 0.0f, 1.0f/3.0f),
      clamp(x-2.0f/3.0f, 0.0f, 1.0f/3.0f)
#elif PALETTE == 1
      clamp(x-2.0f/3.0f, 0.0f, 1.0f/3.0f),
      clamp(x-1.0f/3.0f, 0.0f, 1.0f/3.0f),
      clamp(x, 0.0f, 1.0f/3.0f)
#elif PALETTE == 2
      clamp(x-2.0f/3.0f, 0.0f, 1.0f/3.0f),
      clamp(x, 0.0f, 1.0f/3.0f),
      clamp(x-1.0f/3.0f, 0.0f, 1.0f/3.0f)
#endif
   ) * 3.0f;
}

__KERNEL__ void StarSystemsEvolutionFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  
  fragColor = to_float4_aw(col(_logf(_tex2DVecN(iChannel0,uv.x,uv.y,15).w*10000.0f)/12.0f), 1.0f);

//fragColor = _tex2DVecN(iChannel0,uv.x,uv. y,15);

  SetFragmentShaderComputedColor(fragColor);
}