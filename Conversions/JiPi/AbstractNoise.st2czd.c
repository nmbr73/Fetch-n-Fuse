
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------




__DEVICE__ float Random1(float x, float y, float z) {
    return (fract(_sinf((x * 12.9898f) + (y * 78.233f) + (z * 195.1533f)) * 43758.5453123f) * 2.0f) - 1.0f;
}

__DEVICE__ float SmoothNoise1(float3 p) {
    float ix0 = _floor(p.x),
          iy0 = _floor(p.y),
          iz0 = _floor(p.z),
          ix1 = ix0 + 1.0f,
          iy1 = iy0 + 1.0f,
          iz1 = iz0 + 1.0f,
          fx = p.x - ix0,
          fy = p.y - iy0,
          fz = p.z - iz0;

    fx *= fx * (3.0f - (fx * 2.0f));
    fy *= fy * (3.0f - (fy * 2.0f));
    fz *= fz * (3.0f - (fz * 2.0f));

    return _mix(_mix(_mix(Random1(ix0, iy0, iz0), Random1(ix1, iy0, iz0), fx),
                     _mix(Random1(ix0, iy1, iz0), Random1(ix1, iy1, iz0), fx), fy),
                _mix(_mix(Random1(ix0, iy0, iz1), Random1(ix1, iy0, iz1), fx),
                     _mix(Random1(ix0, iy1, iz1), Random1(ix1, iy1, iz1), fx), fy), fz);
}

__DEVICE__ float FractalSmoothNoise1(float3 p) {
    float y = 0.0f;
    
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    float gain = 0.5f;
    float lacunarity = 2.0f;
    for(int i = 0; i < 8; i++) {
        y += SmoothNoise1(p * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    return y;
}

__DEVICE__ float Map(float3 p) {
    return ((FractalSmoothNoise1(p + to_float3_s(0.5f)) * 0.5f) + 0.5f) - 0.3f;
}

__DEVICE__ float March(float3 ro, float3 rd, int MAX_STEPS, float MIN_DIST, float MAX_DIST) {
    float d = 0.0f;
    for(int i = 0; i < MAX_STEPS; i++) {
        float sd = Map(ro + (rd * d));
        d += sd;
        if(d > MAX_DIST) return MAX_DIST;
        if(_fabs(sd) < MIN_DIST) break;
    }
    return d;
}

__KERNEL__ void AbstractNoiseFuse(float4 o, float2 i, float iTime, float2 iResolution)
{
  
    CONNECT_BUTTON0(Modus1, 1, Start,  End);
    CONNECT_BUTTON1(Modus2, 0, Eins,  Zwei, Drei);
    CONNECT_BUTTON2(Modus3, 0, Beginn,  Velo, Mass, InvMass, Special);
  
    CONNECT_INTSLIDER0(MAX_STEPS, 0, 300, 150);
    CONNECT_SLIDER0(MAX_DIST, 0.0f, 50.0f, 20.0f);
    CONNECT_SLIDER1(MIN_DIST, -1.0f, 1.0f, 0.01f);
  
    //const int MAX_STEPS = 150;
    //const float MAX_DIST = 20.0f;
    //const float MIN_DIST = 0.01f;

    float2 uv = (i - (0.5f *  iResolution)) / _fminf(iResolution.x, iResolution.y);

    float3 col = to_float3_s(0.0f);

    float3 ro = to_float3(0.0f, 0.0f, iTime * 3.0f);
    float3 rd = normalize(to_float3_aw(uv, 1.0f));

    float d = March(ro, rd, MAX_STEPS, MIN_DIST, MAX_DIST);
    float3 p = ro + (rd * d);

    col += _fabs(d) / MAX_DIST;

    o = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(o);
}