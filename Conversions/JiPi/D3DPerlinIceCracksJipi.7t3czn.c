
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Hash without Sine
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hashwithoutsine11(float p)
{
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return fract(p);
}

// Integer Hash - II
// - Inigo Quilez, Integer Hash - II, 2017
//   https://www.shadertoy.com/view/XlXcW4
__DEVICE__ uint3 iqint2(uint3 _x)
{
    const uint k = 1103515245u;

//    _x = ((_x>>8U)^swi3(_x,y,z,x))*k;
//    _x = ((_x>>8U)^swi3(_x,y,z,x))*k;
//    _x = ((_x>>8U)^swi3(_x,y,z,x))*k;

  _x = (make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x))*k; //(_x>>8U)^swi3(_x,y,z,x))*k;
  _x = (make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x))*k; //
  _x = (make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x))*k; //
  


    return _x;
}

// http://www.jcgt.org/published/0009/03/02/
__DEVICE__ uint3 pcg3d(uint3 v) {

    v = v * 1664525u + 1013904223u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    //v ^= make_uint3(v.x >> 16u,v.y >> 16u,v.z >> 16u);
    v = make_uint3(v.x ^ (v.x >> 16u),v.y ^ (v.y >> 16u), v.z ^ (v.z >> 16u));

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    return v;
}

// SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
__DEVICE__ uint superfast(uint3 data)
{
    uint hash = 8u, tmp;

    hash += data.x & 0xffffu;
    tmp = (((data.x >> 16) & 0xffffu) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    hash += hash >> 11;

    hash += data.y & 0xffffu;
    tmp = (((data.y >> 16) & 0xffffu) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    hash += hash >> 11;

    hash += data.z & 0xffffu;
    tmp = (((data.z >> 16) & 0xffffu) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    hash += hash >> 11;

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

// Used to switch hash methods easily for testing
// pcg3d seems to be the best
__DEVICE__ uint3 Hash(uint3 v)
{
    //return make_uint3(superfast(v));
    //return iqint2(v);
    return pcg3d(v);
}

// Turns a hashed uvec3 into a noise value
__DEVICE__ float HashToNoise(uint3 hash)
{
    return ((float)(hash.y % 65536u) / 65536.0f);
}

__DEVICE__ float HashToNormalizedNoise(uint3 hash, float3 offset)
{
    // Normalize hash
    float3 hashF = to_float3(
                              float(hash.x % 65536u) / 32768.0f - 1.0f,
                              float(hash.y % 65536u) / 32768.0f - 1.0f,
                              float(hash.z % 65536u) / 32768.0f - 1.0
                            );
    hashF /= length(hashF);
    
    return dot(hashF, offset);
}

__DEVICE__ float SCurve(float v)
{
    return v*v*(3.0f-2.0f*v);
}

__DEVICE__ float3 SCurve(float3 v)
{
    return v*v*(3.0f-2.0f*v);
}

__DEVICE__ float Perlin3D(float3 uv)
{
    uint3 u = make_uint3((uint)uv.x, (uint)uv.y, (uint)uv.z);
    
    float v;
    float scale = 1.0f;
    uint pitch = 64u;
    for (int i = 0; i < 7; ++i)
    {
        uint3 u2 = u - (make_uint3(u.x%pitch, u.y%pitch, u.z%pitch));//(u % make_uint3(pitch));
        float3 _lerp = SCurve((uv - to_float3((float)u2.x,(float)u2.y,(float)u2.z)) / to_float3_s((float)pitch));
        
        float s000 = HashToNoise(Hash(u2));
        float s100 = HashToNoise(Hash(u2 + make_uint3(pitch, 0u, 0u)));
        float s010 = HashToNoise(Hash(u2 + make_uint3(0u, pitch, 0u)));
        float s110 = HashToNoise(Hash(u2 + make_uint3(pitch, pitch, 0u)));
        float s001 = HashToNoise(Hash(u2 + make_uint3(0u, 0u, pitch)));
        float s101 = HashToNoise(Hash(u2 + make_uint3(pitch, 0u, pitch)));
        float s011 = HashToNoise(Hash(u2 + make_uint3(0u, pitch, pitch)));
        float s111 = HashToNoise(Hash(u2 + make_uint3(pitch, pitch, pitch)));
float zzzzzzzzzzzzzzzzzzzzzz;        
        v += scale * _mix(
                     _mix(_mix(s000, s100, _lerp.x), _mix(s010, s110, _lerp.x), _lerp.y),
                     _mix(_mix(s001, s101, _lerp.x), _mix(s011, s111, _lerp.x), _lerp.y), _lerp.z);
        scale /= 2.0f;
        pitch /= 2u;
    }
    
    return v;
}

__KERNEL__ void D3DPerlinIceCracksJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    float scale = 100.0f;
    uv = (uv + iTime * 0.05f) * scale;
float IIIIIIIIIIIIIII;    
    float n0 = Perlin3D(to_float3_aw(swi2(uv,x,y), iTime * 5.0f));
    float n1 = Perlin3D(to_float3_aw(swi2(uv,y,x) + iTime * 11.8f + to_float2(1083.1389f, 829.3289f), iTime * 5.0f));
    //float n2 = Perlin3D(to_float3_aw(uv * to_float2(5.1389f, 6.3289f), iTime * 5.0f));
    
    float cracks = _powf(_fminf(1.0f, (1.0f - _fabs(n0 - n1)) * 1.115f - 0.1f), 7.0f);
    float clouds = n0 * n0 * 0.125f * (1.0f - cracks);
    float v = cracks + clouds;
    
    v += _fmaxf(0.0f, _fminf(1.0f, (n1 - n0) * 15.0f)) * 0.5f; // good way to split into regions
    
    //v = n0;
    //v = cracks;
    //v = clouds;

    // Apply blue tint
    float3 col = to_float3(_powf(v, 2.0f) * 0.9f, _powf(v, 1.5f), v);
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}