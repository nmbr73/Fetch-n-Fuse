
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// fbm gyroid noise
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 5; ++i) {
        result += gyroid(seed/a+result/a)*a;
        a /= 2.0f;
    }
    return result;
}

__DEVICE__ mat2 rot(float a) {
    float c = _cosf(a), s = _sinf(a);
    return to_mat2(c,-s,s,c);
}

// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float3 hash33(float3 p3) {
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
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

// Connect Buffer A 'Texture: Blending' to iChannel0


__DEVICE__ float map(float3 p, inout float *details, float iTime)
{
    // spicy fbm cyclic gyroid noise
    *details = _sinf(iTime*0.2f-fbm(p)+length(p));
    return _fmaxf(_fabs(*details*0.05f), p.z+2.0f);
}

__KERNEL__ void InterMembraneSpaceFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame)
{

    //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);


    fragCoord+=0.5f;

    float details;

    // salt
    float3 rng = hash33(to_float3_aw(fragCoord, iFrame));

    // coordinates
    float2 uv = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 color = to_float3_s(0);
    float3 ray = normalize(to_float3_aw(uv, -1.0f));
    swi2S(ray,x,y, mul_f2_mat2(swi2(ray,x,y) , rot(-0.7f)));
    float3 pos = ray*(0.5f+0.5f*rng.z);
    
    // raymarch
    float maxDist = 5.0f;
    const float count = 100.0f;
    float steps = 0.0f;
    float total = 0.0f;
    for (steps = count; steps > 0.0f; --steps) {
        float dist = map(pos, &details, iTime);
        if (dist < total/iResolution.y || total > maxDist) break;
        dist *= 0.9f+0.1f*rng.x;
        pos += ray * dist;
        total += dist;
    }
    
    // lighting
    float shade = steps/count;
    if (shade > 0.001f && total < maxDist) {
        float2 noff = to_float2(0.001f,0); // NuSan https://www.shadertoy.com/view/3sBGzV
        float3 normal = normalize(map(pos, &details, iTime)-to_float3(map(pos-swi3(noff,x,y,y), &details, iTime), map(pos-swi3(noff,y,x,y), &details, iTime), map(pos-swi3(noff,y,y,x), &details, iTime)));
        float top = dot(reflect(ray, normal), to_float3(0,1,0))*0.5f+0.5f;
        float3 tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)+pos.y+details*6.0f);
        color = to_float3_s(0.2f) + to_float3_s(0.8f)*top;
        color += tint * 0.5f;
        color *= shade*shade;
    }
    
    fragColor = to_float4_aw(color, 1);

  if (Blend1>0.0) fragColor = Blending(iChannel0, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Temporal Anti Aliasing from:
// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/

// but only the color clamping...

__KERNEL__ void InterMembraneSpaceFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f; 

    float2 uv = fragCoord / iResolution;
    float3 color = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 temporal = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    float3 minColor = to_float3_s(9999.0f), maxColor = to_float3_s(-9999.0f);
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            float3 c = swi3(texture(iChannel0, uv + to_float2(x, y) / iResolution),x,y,z);
            minColor = _fminf(minColor, c);
            maxColor = _fmaxf(maxColor, c);
        }
    }
    temporal = clamp(temporal, minColor, maxColor);
    fragColor = to_float4_aw(_mix(color, temporal, 0.9f), 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0



// Inter Membrane Space

// Buffer A : raymarching and lighting
// Buffer B : temporal anti aliasing

__KERNEL__ void InterMembraneSpaceFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}