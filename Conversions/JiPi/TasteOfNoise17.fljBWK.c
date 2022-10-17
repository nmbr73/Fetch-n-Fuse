
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel2
// Connect Buffer A 'Texture: Blue Noise' to iChannel1



// Taste of Noise 17 by Leon Denise 2022-05-17

// A very distorted volume
// Playing with a 3D FBM noise

float details;

// rotation matrix
__DEVICE__ mat2 rot (float a) { return mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }

// shortcut for lighting
#define dt(rn,v,p) _powf(dot(rn,normalize(v))*0.5f+0.5f,p)

// https://iquilezles.org/articles/distfunctions/
__DEVICE__ float smin(float d1, float d2, float k) { float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f ); return _mix( d2, d1, h ) - k*h*(1.0f-h); }

// transform linear value into cyclic absolute value
__DEVICE__ float3 bend(float3 v)
{
    return _fabs(_sinf(v*6.28f*2.0f-iTime*0.5f));
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
    float d = 0.0f;
    
    // FBM animated noise
    float3 ps = p * 0.05f;
    ps.z += iTime*0.001f;
    float3 spicy = fbm(ps);
    details = spicy.x;
    spicy = spicy * 2.0f - 1.0f;
    
    // displace volume
    d += spicy.x * 0.2f;
    
    // volume to surface
    d = _fabs(d)-0.1f;

    // substract volume from origin
    float carve = -1.25f+0.25f*_sinf(iTime*0.1f+length(p));
    d = smin(d, -(length(p)-0.0f), carve);
    
    return d * 0.25f;
}

// Antoine Zanuttini
// https://www.shadertoy.com/view/3sBGzV
__DEVICE__ float3 getNormal (float3 pos)
{
    float2 noff = to_float2(0.005f,0);
    return normalize(map(pos)-to_float3(map(pos-swi3(noff,x,y,y)), map(pos-swi3(noff,y,x,y)), map(pos-swi3(noff,y,y,x))));
}

__KERNEL__ void TasteOfNoise17Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    // coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    float3 noise = texture(iChannel1, fragCoord/1024.0f+iTime).rgb;
    float3 ray = normalize(to_float3_aw(uv, 0.5f));
    float3 pos = to_float3(0,0,0);
    
    // init variables
    float3 color, normal, tint, dir, refl;
    float index, shade, light;
    const float count = 50.0f;

    // ray marching
    for (index = count; index > 0.0f; --index)
    {
        float dist = map(pos);
        if (dist < 0.001f) break;
        dist *= 0.9f+0.1f*noise.z;
        pos += ray*dist;
    }
    
    // lighting
    shade = index/count;
    normal = getNormal(pos);
    tint = 0.5f+0.5f*_cosf(to_float3(1,2,3)+details*20.0f);
    refl = reflect(ray, normal);
    color += tint;
    color += to_float3(1.000f,0.502f,0.792f)*dt(refl, to_float3(0,0,-1), 0.5f);
    color = clamp(color * shade, 0.0f, 1.0f);
    
    // temporal buffer
    uv = fragCoord / iResolution;
    float3 frame = _tex2DVecN(iChannel2,uv.x,uv.y,15).rgb;
    color = _mix(color, frame, 0.9f);
    
    fragColor = to_float4_aw(color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0



// Taste of Noise 17 by Leon Denise 2022-05-17

// A very distorted volume
// Playing with a 3D FBM noise

__KERNEL__ void TasteOfNoise17Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Render result of Buffer A
    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);


  SetFragmentShaderComputedColor(fragColor);
}