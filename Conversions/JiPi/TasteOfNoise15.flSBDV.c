
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1



// Taste of Noise 15 by Leon Denise 2022/05/17
// Using code from Inigo Quilez, Antoine Zanuttini and many more

// Wanted to play again with FBM noise after understanding
// I could sample the 3d texture to generate cheap value noise
// (well I think it is cheaper?)

// This is definitively one of my favorite noise pattern
// A FBM noise with cyclic absolute value, making it looks
// like a Gogotte stone or an abstract drawing from Moebius.


// rotation matrix
__DEVICE__ mat2 rot (float a) { return mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }

// transform linear value into cyclic absolute value
__DEVICE__ float3 bend(float3 v)
{
    return _fabs(_sinf(v*2.0f*6.283f+iTime*6.283f*0.1f));
}

// fractal brownian motion (layers of multi scale noise)
__DEVICE__ float3 fbm(float3 p)
{
    float3 result = to_float3_aw(0);
    float falloff = 0.5f;
    for (float index = 0.0f; index < 3.0f; ++index)
    {
        result += bend(texture(iChannel0, p/falloff).xyz) * falloff;
        falloff /= 2.0f;
    }
    return result;
}

// signed distance function
__DEVICE__ float map(float3 p)
{
    float dist = 100.0f;
    
    // animated fbm noise
    float3 seed = p * 0.08f;
    seed.z -= iTime*0.01f;
    float3 spicy = fbm(seed) * 2.0f - 1.0f;
    
    // sphere with distorted surface
    dist = length(p)-1.0f - spicy.x*0.2f;
    
    // scale down distance because domain is highly distorted
    return dist * 0.15f;
}

// Antoine Zanuttini
// https://www.shadertoy.com/view/3sBGzV
__DEVICE__ float3 getNormal (float3 pos)
{
    float2 noff = to_float2(0.01f,0);
    return normalize(map(pos)-to_float3(map(pos-swi3(noff,x,y,y)), map(pos-swi3(noff,y,x,y)), map(pos-swi3(noff,y,y,x))));
}

__KERNEL__ void TasteOfNoise15Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    // coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    float3 noise = texture(iChannel1, fragCoord/1024.0f).rgb;
    float3 ray = normalize(to_float3_aw(uv, 3.0f));
    float3 pos = to_float3(0,0,-3);
    
    // init variables
    float3 color, normal, tint;
    float index, shade, light;
    const float count = 50.0f;

    // ray marching
    for (index = count; index > 0.0f; --index)
    {
        float dist = map(pos);
        if (dist < 0.001f) break;
        dist *= 0.8f+0.1f*noise.z;
        pos += ray*dist;
    }
    
    // coloring
    shade = index/count;
    normal = getNormal(pos);
    light = _powf(dot(reflect(ray, normal), to_float3(0,1,0))*0.5f+0.5f, 2.0f);
    light += _powf(dot(normal, ray)*0.5f+0.5f, 0.5f);
    float dt = dot(reflect(normal, ray), -normalize(pos));
    color = 0.5f+0.5f*_cosf(to_float3(1,2,3)+pos.y*3.0f+dt*3.0f+0.5f);
    color = clamp((color + light * 0.5f) * shade, 0.0f, 1.0f);
    fragColor = to_float4_aw(color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}