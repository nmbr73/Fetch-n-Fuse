
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define T(uv) _tex2DVecN(iChannel0,(uv).x,(uv).y,15).x
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 3; ++i) {
        result += gyroid(seed/a)*a;
        a /= 3.0f;
    }
    return result;
}

// Dave Hoskins https://www.shadertoy.com/view/4djSRW
__DEVICE__ float2 hash21(float p)
{
  float3 p3 = fract_f3((p) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

// Victor Shepardson + Inigo Quilez 
// https://www.shadertoy.com/view/XlXcW4

__DEVICE__ float3 hash( uint3 _x )
{
  const uint k = 1103515245U;  // GLIB C
float zzzzzzzzzzzzzzzzzzzz;  
    //x = ((x>>8U)^swi3(x,y,z,x))*k;
    //x = ((x>>8U)^swi3(x,y,z,x))*k;
    //x = ((x>>8U)^swi3(x,y,z,x))*k;
    
    _x = make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x) *k;
    _x = make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x) *k;
    _x = make_uint3((_x.x>>8U)^_x.y, (_x.y>>8U)^_x.z, (_x.z>>8U)^_x.x) *k;
    
    return make_float3(_x)*(1.0f/(float)(0xffffffffU));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0



// move brush
__DEVICE__ float2 move(float t)
{
    float2 pos = to_float2_s(0);
    
    // random targets
    float jitter = 0.5f;
    float time = t*3.0f;
    float index = _floor(time);
    float anim = fract(time);
    float2 rng = _mix(hash21(index), hash21(index+1.0f), anim);
    pos += (rng*2.0f-1.0f)*jitter;
    
    // translate to right
    pos.x += 0.5f;
    
    // twist it
    float angle = t;
    float radius = 0.1f;
    pos += to_float2(_cosf(angle),_sinf(angle))*radius;
    
    // fbm gyroid noise
    angle = fbm(to_float3_aw(pos,t))*6.28f;
    radius = 0.2f;
    pos += to_float2(_cosf(angle),_sinf(angle))*radius;
    return pos;
}

__KERNEL__ void SinusoidalPaintingFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    // coordinates
    float2 uv = fragCoord/R;
    float2 p = 1.5f*(fragCoord-R/2.0f)/R.y;
    
    // scroll
    uv.x += 1.0f/R.x;
    
    // framebuffer
    float4 frame = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float mask = frame.x;
    float sdf = frame.y;
    
    // interaction
    if (iMouse.z > 0.0f)
    {
        float2 mouse = swi2(iMouse,x,y);
        float4 prev = texture(iChannel0, to_float2_s(0));
float tttttttttttttttttt;
        float3 dither = hash(make_uint3(fragCoord.x, fragCoord.y, iFrame)); 
        mouse = prev.z > 0.0f ? _mix(mouse, swi2(prev,x,y), dither.x) : mouse;
        mouse = 1.5f*(mouse-R/2.0f)/R.y;
        float thin = 0.04f+0.03f*_sinf(iTime*20.0f);
        float dist = length(p-mouse);
        float msk = smoothstep(thin,0.0f,dist);
        if (msk > 0.001f) frame.z = iTime;
        sdf = sdf < 0.001f ? dist : _fminf(sdf, dist);
        mask += msk;
    }
    else
    {
        // accumulate noisy results
        for (float frames = 20.0f; frames > 0.0f; --frames)
        {
            // cursor timeline with noise offset
            float f = (float)(iFrame) + frames * 200.0f;
            float3 rng = hash(make_uint3(fragCoord.x, fragCoord.y, f));
            float cursor = rng.x*0.03f+iTime;

            // brush
            float thin = 0.04f+0.03f*_sinf(cursor*20.0f);
            float dist = length(p-move(cursor));
            float msk = smoothstep(thin,0.0f,dist);

            // timestamp
            if (msk > 0.001f) frame.z = iTime;

            // distance
            sdf = sdf < 0.001f ? dist : _fminf(sdf, dist);

            // accumulate
            mask += msk;
        }
    }

    // save data
    frame.x = mask;
    frame.y = sdf;
    fragColor = frame;
    
    // avoid glitch after disabling fullscreen
    if (fragCoord.x > R.x-1.0f) fragColor = to_float4(0,0,0,1);
    
    if (fragCoord.x < 1.0f && fragCoord.y < 1.0f)
    {
       fragColor = iMouse;
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void SinusoidalPaintingFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    
    // compute normal
    float4 color = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 unit = to_float3_aw(1.0f/iResolution, 0);
    float3 normal = normalize(to_float3(
                                        T(uv+swi2(unit,x,z))-T(uv-swi2(unit,x,z)),
                                        T(uv-swi2(unit,z,y))-T(uv+swi2(unit,z,y)),
                                        color.x));
                            
    fragColor = to_float4_aw(normal, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1



// Sinusoidal Painting
// when you let sine paint

__KERNEL__ void SinusoidalPaintingFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    float4 color = to_float4(0, 0, 0, 1);
    float2 uv = fragCoord/iResolution;
    float4 data = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float mask = data.x;
    
    if (mask > 0.001f)
    {
        // lighting
        float3 normal = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);//.rgb;
        float3 light = normalize(to_float3(0,1,1));
        float timestamp = data.z;
        float shade = dot(normal, light)*0.5f+0.5f;
        float3 palette = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*5.0f+timestamp*3.0f);
        swi3S(color,x,y,z, palette * shade);
        color += _powf(shade,  50.0f);
    }
    else
    {
        // background
        swi3(color,x,y,z) = to_float3_s(1) * smoothstep(2.0f, -2.0f, length(uv-0.5f));

        // shadow
        float sdf = data.y;
        color *= smoothstep(-0.3f, 0.2f,  sdf);
    }
    
    fragColor = color;

  SetFragmentShaderComputedColor(fragColor);
}