

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Hash without Sine
// https://www.shadertoy.com/view/4djSRW
float hashwithoutsine11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// Integer Hash - II
// - Inigo Quilez, Integer Hash - II, 2017
//   https://www.shadertoy.com/view/XlXcW4
uvec3 iqint2(uvec3 x)
{
    const uint k = 1103515245u;

    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;

    return x;
}

// http://www.jcgt.org/published/0009/03/02/
uvec3 pcg3d(uvec3 v) {

    v = v * 1664525u + 1013904223u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    v ^= v >> 16u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    return v;
}

// SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
uint superfast(uvec3 data)
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
uvec3 Hash(uvec3 v)
{
    //return uvec3(superfast(v));
    //return iqint2(v);
    return pcg3d(v);
}

// Turns a hashed uvec3 into a noise value
float HashToNoise(uvec3 hash)
{
    return (float(hash.y % 65536u) / 65536.0);
}

float HashToNormalizedNoise(uvec3 hash, vec3 offset)
{
    // Normalize hash
    vec3 hashF = vec3(
        float(hash.x % 65536u) / 32768.0 - 1.0,
        float(hash.y % 65536u) / 32768.0 - 1.0,
        float(hash.z % 65536u) / 32768.0 - 1.0
    );
    hashF /= length(hashF);
    
    return dot(hashF, offset);
}

float SCurve(float v)
{
    return v*v*(3.0-2.0*v);
}

vec3 SCurve(vec3 v)
{
    return v*v*(3.0-2.0*v);
}

float Perlin3D(vec3 uv)
{
    uvec3 u = uvec3(uv);
    
    float v;
    float scale = 1.0;
    uint pitch = 64u;
    for (int i = 0; i < 7; ++i)
    {
        uvec3 u2 = u - (u % uvec3(pitch));
        vec3 lerp = SCurve((uv - vec3(u2)) / vec3(pitch));
        
        float s000 = HashToNoise(Hash(u2));
        float s100 = HashToNoise(Hash(u2 + uvec3(pitch, 0u, 0u)));
        float s010 = HashToNoise(Hash(u2 + uvec3(0u, pitch, 0u)));
        float s110 = HashToNoise(Hash(u2 + uvec3(pitch, pitch, 0u)));
        float s001 = HashToNoise(Hash(u2 + uvec3(0u, 0u, pitch)));
        float s101 = HashToNoise(Hash(u2 + uvec3(pitch, 0u, pitch)));
        float s011 = HashToNoise(Hash(u2 + uvec3(0u, pitch, pitch)));
        float s111 = HashToNoise(Hash(u2 + uvec3(pitch, pitch, pitch)));
        
        v += scale * mix(
            mix(mix(s000, s100, lerp.x), mix(s010, s110, lerp.x), lerp.y),
            mix(mix(s001, s101, lerp.x), mix(s011, s111, lerp.x), lerp.y), lerp.z);
        scale /= 2.0;
        pitch /= 2u;
    }
    
    return v;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    float scale = 100.0;
    uv = (uv + iTime * 0.05) * scale;
    
    float n0 = Perlin3D(vec3(uv.xy, iTime * 5.0));
    float n1 = Perlin3D(vec3(uv.yx + iTime * 11.8 + vec2(1083.1389, 829.3289), iTime * 5.0));
    //float n2 = Perlin3D(vec3(uv * vec2(5.1389, 6.3289), iTime * 5.0));
    
    float cracks = pow(min(1.0, (1.0 - abs(n0 - n1)) * 1.115 - 0.1), 7.0);
    float clouds = n0 * n0 * 0.125 * (1.0 - cracks);
    float v = cracks + clouds;
    
    v += max(0.0, min(1.0, (n1 - n0) * 15.0)) * 0.5; // good way to split into regions
    
    //v = n0;
    //v = cracks;
    //v = clouds;

    // Apply blue tint
    vec3 col = vec3(pow(v, 2.0) * 0.9, pow(v, 1.5), v);
    fragColor = vec4(col,1.0);
}