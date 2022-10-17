
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Cubemap' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel2
// Connect Buffer A 'Texture: Blue Noise' to iChannel1


// Taste of Noise 18 by Leon Denise 2022-05-17
// variation of https://www.shadertoy.com/view/fljBWK

// A very distorted volume
// Playing with a 3D FBM noise


// rotation matrix
__DEVICE__ mat2 rot (float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }

// shortcut for lighting
#define dt(rn,v,p) _powf(dot(rn,normalize(v))*0.5f+0.5f,p)

#define ss(a,b,t) smoothstep(a,b,t)

// https://iquilezles.org/articles/distfunctions/
__DEVICE__ float smin(float d1, float d2, float k) { float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f ); return _mix( d2, d1, h ) - k*h*(1.0f-h); }

// transform linear value into cyclic absolute value
__DEVICE__ float3 bend(float3 v, float iTime)
{
    return abs_f3(sin_f3(v*6.28f*4.0f-iTime * 0.2f));
}

// fractal brownian motion (layers of multi scale noise)
__DEVICE__ float3 fbm(float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float3 result = to_float3_s(0);
    float falloff = 0.5f;
    for (float index = 0.0f; index < 3.0f; ++index)
    {
        result += bend(swi3(decube_f3(iChannel0, p/falloff),x,y,z), iTime) * falloff;
        falloff /= 2.0f;
    }
    return result;
}

// signed distance function
__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0, inout float *details)
{
    float d = 0.0f;
    d = length(p)-1.0f;
    
    // FBM animated noise
    float3 ps = p * 0.04f;
    ps.z += iTime*0.0005f+0.2f;
    float3 spicy = fbm(ps,iTime, iChannel0);
    *details = spicy.x;
    spicy = spicy * 2.0f - 1.0f;
    
    // displace volume
    d += spicy.x * 0.9f;
    
    // volume to surface
    d = _fabs(d);
    
    return d * 0.025f;
}

// Antoine Zanuttini
// https://www.shadertoy.com/view/3sBGzV
__DEVICE__ float3 getNormal (float3 pos, float iTime, __TEXTURE2D__ iChannel0, inout float *details)
{
    float2 noff = to_float2(0.01f,0);
    return normalize(map(pos,iTime,iChannel0,details)-to_float3(map(pos-swi3(noff,x,y,y),iTime,iChannel0,details), map(pos-swi3(noff,y,x,y),iTime,iChannel0,details), map(pos-swi3(noff,y,y,x),iTime,iChannel0,details)));
}


__KERNEL__ void TasteOfNoise18Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    float details;

    // coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    float3 noise = swi3(_tex2DVecN(iChannel1, fragCoord.x/1024.0f+iTime,fragCoord.y/1024.0f+iTime,15),x,y,z);
    float3 ray = normalize(to_float3_aw(uv, 0.5f));
    float3 pos = to_float3(0,0,0);
    
    // init variables
    float3 color, normal, tint, dir, refl;
    float index, shade, light;
    const float count = 70.0f;

    // ray marching
    for (index = count; index > 0.0f; --index)
    {
        float dist = map(pos,iTime,iChannel0,&details);
        if (dist < 0.001f) break;
        dist *= 0.9f+0.1f*noise.z;
        pos += ray*dist;
    }
    
    // lighting
    shade = index/count;
    normal = getNormal(pos,iTime,iChannel0,&details);
    refl = reflect(ray, normal);
    tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)+details*5.0f + 0.5f);
    color = tint * dt(refl, to_float3(0,1,-1), 0.3f);
    color += to_float3(0.459f,0.102f,0.173f)*dt(refl, to_float3(0,-1,0.5f), 0.5f);
    color = _mix(color, to_float3_s(1), ss(0.4f,1.0f,dot(normal, -ray)));
    color = clamp(color * shade * 1.5f, 0.0f, 1.0f);
    
    // temporal buffer
    uv = fragCoord / iResolution;
    float3 frame = swi3(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y,z);
    color = _mix(color, frame, 0.9f);
    
    fragColor = to_float4_aw(color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Taste of Noise 18 by Leon Denise 2022-05-17
// variation of https://www.shadertoy.com/view/fljBWK

// A very distorted volume
// Playing with a 3D FBM noise

__KERNEL__ void TasteOfNoise18Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Render result of Buffer A
    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);


  SetFragmentShaderComputedColor(fragColor);
}