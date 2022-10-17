
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//#define LENS

const float K_OVER_M = 700.0f;

const float ATTENUATION = 0.999f;

const float AMPLITUDE = 5.0f;

const float PULSATION = 40.0f;

const float DT = 0.012f;

const float MARGIN = 50.0f;

//--------------------------------------------
//--------------------------------------------
__DEVICE__ float elasticity(int2 xy, float2 rxy)
{
#ifdef LENS
    float2 delta = to_float2(xy) - to_float2(0.5f,0.5f)*rxy;
    float r = rxy.x * 0.2f;
    return dot(delta,delta) > r*r ? 2.0f : 1.0f;
#else
    return xy.x > int(rxy.x*0.4f) ? 3.0f : 1.0f;
#endif
}

//--------------------------------------------
//--------------------------------------------
__DEVICE__ float height(sampler2D HV, int2 sxy, int2 xy, float t)
{
    float s = 1.0f - smoothstep(0.0f, 12.0f, length(to_float2(sxy - xy)));
    
    float h = texelFetch(HV, xy, 0).x;
    h += s * AMPLITUDE * _sinf(PULSATION * t);
    return h;
}

//--------------------------------------------
//--------------------------------------------
__DEVICE__ float margin(float2 xy, float2 wh)
{
    float a0 = smoothstep(0.0f, MARGIN, xy.x);
    float a1 = smoothstep(0.0f, MARGIN, xy.y);  a0 = _fminf(a0, a1);
    a1 = smoothstep(0.0f, MARGIN, wh.x - xy.x); a0 = _fminf(a0, a1);
    a1 = smoothstep(0.0f, MARGIN, wh.y - xy.y); a0 = _fminf(a0, a1);
    return _powf(a0, 0.1f);
}

//--------------------------------------------
//--------------------------------------------
__DEVICE__ float acceleration(sampler2D HV, int2 sxy, int2 xy, float t, float2 rxy)
{
  // get height and velocity from buffer B
    float h = height(HV, sxy, xy, t);
    
    // get heights of neighbours
    float h0 = height(HV, sxy, xy - to_int2(1,0), t);
    float h1 = height(HV, sxy, xy + to_int2(1,0), t);
    float h2 = height(HV, sxy, xy - to_int2(0,1), t);
    float h3 = height(HV, sxy, xy + to_int2(0,1), t);
    
    // sum of (hi - h) is proportional to the elastic force
    float delta_h = h0 + h1 + h2 + h3 - 4.0f * h;
    
    // acceleration = (k / mass) * delta_h    
    float a = K_OVER_M * delta_h;
    
    return elasticity(xy, rxy) * a;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void InteractiveWaterSurfaceFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    int2 xy = to_int2(fragCoord);
    int2 sxy = iMouse.z > 0.0f ? to_int2(swi2(iMouse,x,y)) : (iTime<5.0f ? to_int2(to_float2(0.66f,0.5f)*iResolution) : to_int2(100000));
    
    // get velocity from buffer
    float2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution);
    
    fragColor = to_float4(hv, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void InteractiveWaterSurfaceFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    int2 xy = to_int2(fragCoord);
    int2 sxy = iMouse.z > 0.0f ? to_int2(swi2(iMouse,x,y)) : (iTime<5.0f ? to_int2(to_float2(0.66f,0.5f)*iResolution) : to_int2(100000));
    
    // get velocity from buffer
    float2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution);
    
    fragColor = to_float4(hv, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void InteractiveWaterSurfaceFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    int2 xy = to_int2(fragCoord);
    int2 sxy = iMouse.z > 0.0f ? to_int2(swi2(iMouse,x,y)) : (iTime<5.0f ? to_int2(to_float2(0.66f,0.5f)*iResolution) : to_int2(100000));
    
    // get velocity from buffer
    float2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution);
    
    fragColor = to_float4(hv, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void InteractiveWaterSurfaceFuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    int2 xy = to_int2(fragCoord);
    int2 sxy = iMouse.z > 0.0f ? to_int2(swi2(iMouse,x,y)) : (iTime<5.0f ? to_int2(to_float2(0.66f,0.5f)*iResolution) : to_int2(100000));
    
    // get velocity from buffer
    float2 hv = texelFetch(iChannel0, xy, 0).xy;
    
    // compute acceleration from buffer
    float a = acceleration(iChannel0, sxy, xy, float(iFrame) * DT, iResolution);

    hv.y += a * DT;
    
    // attenuation
    hv.y *= ATTENUATION;
    
    // position1 = position0 + velocity * delta
    hv.x += hv.y * DT;
    
    // margin attenuation
    hv.y *= margin(fragCoord, iResolution);
    
    fragColor = to_float4(hv, 0.0f, 0.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void InteractiveWaterSurfaceFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float4 c = texelFetch(iChannel0, to_int2(fragCoord), 0);
    
    // normal
    float dhx = dFdx(c.x);
    float dhy = dFdy(c.x);
    float3  n   = to_float3(dhx, dhy, 1.0f);
    n=normalize(n);
    
    float3  light   = normalize(to_float3(3,-4,5));
    float diffuse = dot(light, n);
    float spec    = _powf(_fmaxf(0.0f,-reflect(light, n).z),100.0f);
    
    float3 l = to_float3_aw(_fmaxf(diffuse,0.0f) + spec);
    float3 hv = to_float3((swi2(c,x,y) + 1.0f) / 2.0f, 0.5f);
    hv = clamp(hv, 0.0f, 1.0f);
    l = (_fabs(fragCoord.x - (iResolution.x*0.4f+0.5f)) < 1.0f)  ? to_float3_s(0.0f) : l;
    fragColor = to_float4(_mix(hv, l, 0.9f), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}