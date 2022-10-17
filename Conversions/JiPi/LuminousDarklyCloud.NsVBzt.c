
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash11(float p) {
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}
__DEVICE__ float3 hash33(float3 p3) {
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}
__DEVICE__ float hash12(float2 p) {
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

// Mercury
// https://mercury.sexy/hg_sdf/
__DEVICE__ float pModPolar(inout float2 *p, float repetitions) {
  float angle = 6.28f/repetitions;
  float a = _atan2f((*p).y, (*p).x) + angle/2.0f;
  float r = length(*p);
  float c = _floor(a/angle);
  a = mod_f(a,angle) - angle/2.0f;
  *p = to_float2(_cosf(a), _sinf(a))*r;
  // For an odd number of repetitions, fix cell index of the cell in -x direction
  // (cell index would be e.g. -5 and 5 in the two halves of the cell):
  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
  return c;
}

__DEVICE__ mat2 rot(float a) {
    float c = _cosf(a), s = _sinf(a);
    return to_mat2(c,-s,s,c);
}

__DEVICE__ float3 lookAt (float3 from, float3 at, float2 uv, float fov)
{
  float3 z = normalize(at-from);
  float3 x = normalize(cross(z, to_float3(0,1,0)));
  float3 y = normalize(cross(x, z));
  return normalize(z * fov + uv.x * x + uv.y * y);
}

// fbm gyroid noise
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 4; ++i) {
        result += _powf(_fabs(gyroid(seed/a)),3.0f)*a;
        a /= 2.0f;
    }
    return result;
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



// Luminous Darkly Cloud
//
// - inadequat volumetric rendering
// - incorrect ambien occlusion
// - overloading gyroid fbm noise
// - fake lightning
// - random point of view
//
// "but with the right numbers, it looks nice!"

// globals

__DEVICE__ float glow, cycle, noise, columns, index;
__DEVICE__ float3 target;
__DEVICE__ bool flash;

// signed distance function
__DEVICE__ float map(float3 p, float4 iMouse, float iTime)
{
    noise = fbm(p+to_float3(0,0,iTime*0.2f));
    
    // cloud
    float dist = length(p*to_float3(1,2,1))-1.0f;
    dist -= noise*0.5f;
    dist *= 0.3f;
    
    if (flash) {
        // mouse control
        if (iMouse.z > 0.5f) {
            swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(1.5f)));
            swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot(-target.x*2.0f)));
            swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(target.y*2.0f)));
        }

        // lightning
        float fade = smoothstep(3.0f,0.0f,p.y);
        p -= fbm(p+cycle)*0.3f*fade;
        
        float2 pxz = swi2(p,x,z);
        float c = pModPolar(&pxz, columns);
        p.x = pxz.x; p.z=pxz.y;
        
        p.x += 0.5f*_fminf(0.0f,_fmaxf(-p.y,0.0f)-2.0f);
        float shape = _fmaxf(p.y+1.0f, length(swi2(p,x,z))*2.0f);
        glow += 0.02f/shape;
        dist = _fminf(dist, shape);
    }
    
    return dist;
}

__KERNEL__ void LuminousDarklyCloudFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{


    CONNECT_COLOR0(cloudColor, 0.702f, 0.776f, 1.000f, 1.0f);
    CONNECT_COLOR1(lightColor, 1.000f, 0.812f, 0.400f, 1.0f);
    
    CONNECT_SLIDER0(Dense, -1.0f, 1.0f, 0.02f);
    CONNECT_SLIDER1(Dist, -1.0f, 1.0f, 0.02f);
    
    CONNECT_SLIDER2(DistMul, -1.0f, 1.0f, 0.3f);
    CONNECT_SLIDER3(DistOff, -1.0f, 2.0f, 0.7f);
    
    

    //const float3 cloudColor = to_float3(0.702f,0.776f,1.000f);
    //const float3 lightColor = to_float3(1.000f,0.812f,0.400f);

    // salt
    float3 rng = hash33(to_float3_aw(fragCoord, iFrame));
    
    // coordinates
    float2 uv = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 color = to_float3_s(0);
    float3 pos = to_float3(0,-1,5);
    float3 ray = lookAt(pos, to_float3_s(0), uv, 1.0f);
    
    // timeline
    float time = iTime*5.0f;
    float anim = fract(time);
    index = _floor(time);
    float alea = step(0.9f, hash11(index));
    cycle = index;
    columns = 1.0f+_floor(6.0f*hash11(index+186.0f));
    glow = 0.0f;
    flash = alea > 0.01f;
    
    // mouse interaction
    if (iMouse.z > 0.0f) {
        float2 mouse = (swi2(iMouse,x,y)-iResolution/2.0f)/iResolution.y;
        target = to_float3_aw(_floor(mouse*10.0f)/10.0f,0);
        cycle = hash12(swi2(iMouse,x,y));
        columns = _ceil(5.0f*hash12(swi2(iMouse,x,y)+76.0f));
        flash = true;
        anim = 0.0f;
    }
    
    // raymarch
    float maxDist = 10.0f;
    const float count = 30.0f;
    float steps = 0.0f;
    float total = 0.0f;
    float dense = 0.0f;
    for (steps = count; steps > 0.0f; --steps) {
        float dist = map(pos, iMouse, iTime);
        //dist *= 0.7f+0.3f*rng.x;
        dist *= DistOff+DistMul*rng.x;
        // sort of volumetric march
        if (dist < 0.1f) {
            dense += Dense;//0.02f;
            dist = Dist;//0.02f;
        }
        total += dist;
        if (dense >= 1.0f || total > maxDist) break;
        pos += ray * dist;
    }
    
    // cloud color
    color = swi3(cloudColor,x,y,z);
    #define getAO(dir,k) smoothstep(-k,k,map(pos+dir*k, iMouse, iTime)-map(pos-dir*k, iMouse, iTime))
    color *= 0.5f+0.5f*getAO(to_float3(0,1,0),0.5f);
    color *= 0.5f+0.5f*getAO(to_float3(0,1,0),2.0f);
    color *= dense;
    
    // lightning color
    color += swi3(lightColor,x,y,z) * _powf(glow, 2.0f) * (1.0f-anim);
    color = clamp(color, 0.0f, 1.0f);
    
    fragColor = to_float4_aw(color, 1);

  SetFragmentShaderComputedColor(fragColor);
}