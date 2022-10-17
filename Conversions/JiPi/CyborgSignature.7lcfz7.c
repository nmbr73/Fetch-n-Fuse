
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



#define speed 0.1f
//#define cycle 2.0f
//#define threshold 0.95f

#define T(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x
#define inside(uv) (_fabs((uv).x-0.5f) < 0.5f && _fabs((uv).y-0.5f) < 0.5f)

// Inigo Quilez https://iquilezles.org/articles/distfunctions2d/
__DEVICE__ float sdSegment( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3)
{
    p3  = fract_f3(p3 * 0.1031f);
    p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
    return fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float2 hash21(float p)
{
  float3 p3 = fract_f3((p) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// move horizontally, with circles and random offset
__DEVICE__ float2 move(float t)
{
    float2 pos = to_float2_s(0);
    float angle = t*10.0f;
    float radius = 0.1f;
    float jitter = 0.1f;
    float time = t*5.0f;
    float index = _floor(time);
    float anim = fract(time);
    float scroll = fract(t*speed);
    float2 rng = _mix(hash21(index), hash21(index+1.0f), anim);
    pos += (rng*2.0f-1.0f)*jitter;
    pos.x += scroll*2.0f-1.0f;
    pos.y += _powf(_fabs(_sinf(time*0.2f)), 20.0f)*0.5f;
    pos.y -= _powf(_fabs(_sinf(time*0.1f)), 50.0f)*0.4f;
    pos += to_float2(_cosf(angle),_sinf(angle*1.5f))*radius;
    return pos;
}

__KERNEL__ void CyborgSignatureFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, sampler2D iChannel0)
{

    CONNECT_SLIDER0(TimeDelta, -1.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(cycle, -1.0f, 10.0f, 2.0f);
    CONNECT_SLIDER2(threshold, -1.0f, 3.0f, 0.95f);
    CONNECT_SLIDER3(Level0, -1.0f, 1.0f, 0.0f);

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float2 pos = 1.5f*(fragCoord-iResolution/2.0f)/iResolution.y;
    
    // shape
    float thin = 0.02f+0.01f*_sinf(iTime*10.0f);
    float time = iTime;
    float dist = sdSegment(pos, move(time-iTimeDelta), move(time));
    float mask = smoothstep(thin,0.0f,dist);
    
    // frame buffer
    float4 frame = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    if (frame.y > 0.0f) dist = _fminf(dist, frame.y);
    float timestamp = _mix(frame.z, iTime, step(0.0001f,mask));
    mask = _fmaxf(mask*0.1f,frame.x);
    float material = step(threshold,fract(timestamp*cycle));
    
    // pack
    fragColor = to_float4(mask, dist, timestamp, material);
    
    // wipe
    float timeline = fract(iTime*speed);
    fragColor *= step(0.01f, timeline);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Texture: Blue Noise' to iChannel2



__KERNEL__ void CyborgSignatureFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float timeline = fract(iTime*speed);
    
    // compute normal
    float4 color = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 unit = to_float3_aw(1.0f/iResolution, 0);
    float3 normal = normalize(to_float3(
                                        T(uv+swi2(unit,x,z))-T(uv-swi2(unit,x,z)),
                                        T(uv-swi2(unit,z,y))-T(uv+swi2(unit,z,y)),
                                        color.x));
    
    // glow diffusion
    float glow = 0.0f;
    //float4 blue = texture(iChannel2, fragCoord/1024.0f+iTime)*2.0f-1.0f;
    float4 blue = _tex2DVecN(iChannel2, fragCoord.x/1024.0f+iTime,fragCoord.y/1024.0f+iTime,15)*2.0f-1.0f;
    
    uv += 5.0f*swi2(blue,x,y)/iResolution;
    float gold = _tex2DVecN(iChannel1,uv.x,uv.y,15).w;
    glow = _fmaxf(gold, color.w*0.35f);
    glow *= step(0.01f, timeline);
    
    fragColor = to_float4_aw(normal, glow);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1



// Cyborg Signature,
// when you have to sign that check for your ai bot therapist

__KERNEL__ void CyborgSignatureFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;
    
    float4 color = to_float4(0, 0, 0, 1);
    float2 uv = fragCoord/iResolution;
    float timeline = fract(iTime*speed);
    
    // data readability unpacking
    float4 data     = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float mask      = data.x;
    float timestamp = data.z;
    float dist      = data.y;
    float material  = data.w;
    float glow      = _tex2DVecN(iChannel1,uv.x,uv.y,15).w;
    float3 normal   = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    
    // background
    swi3S(color,x,y,z, to_float3_s(1) * smoothstep(2.0f, -2.0f, length(uv-0.5f)));
    
    // ambient occlusion
    if (0.01f < timeline) 
        color *= smoothstep(-0.5f,0.2f,dist);
    
    if (mask > 0.001f)
    {
        // lighting
        float3 light = normalize(to_float3(0,1,1));
        float shade = dot(normal, light)*0.5f+0.5f;
        color *= material;
        color += glow;
        color += _powf(shade, 10.0f);

    // debug g-buffer
    } else if (false) {
    
        uv *= 4.0f;
        if (inside(uv))
        {
            // data pack
            float4 d = _tex2DVecN(iChannel0,uv.x,uv.y,15);
            color = fract_f4(swi4(d,y,x,z,w)*3.0f); //.grba
        }
        uv.x -= 1.0f;
        if (inside(uv))
        {
            // normal and glow
            float4 d = _tex2DVecN(iChannel1,uv.x,uv.y,15);
            if (d.x > 0.001f)
                color += d;
            color += to_float4_s(d.w);
        }
    }
    
    // shine
    float3 tint = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*5.0f+uv.x*6.0f);
    swi3S(color,x,y,z, swi3(color,x,y,z) + tint*glow);
    
    fragColor = color;
    
    //fragColor = to_float4_s(glow);

  SetFragmentShaderComputedColor(fragColor);
}