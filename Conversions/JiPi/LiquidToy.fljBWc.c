
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



// shortcut to sample texture
#define TEX(uv) _tex2DVecN(iChannel0,uv.x,uv.y,15).r
#define TEX1(uv) _tex2DVecN(iChannel1,uv.x,uv.y,15).r
#define TEX2(uv) _tex2DVecN(iChannel2,uv.x,uv.y,15).r
#define TEX3(uv) _tex2DVecN(iChannel3,uv.x,uv.y,15).r

// shorcut for smoothstep uses
#define trace(edge, thin) smoothstep(thin,0.0f,edge)
#define ss(a,b,t) smoothstep(a,b,t)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1



// Liquid toy by Leon Denise 2022-05-18
// Playing with shading with a fake fluid heightmap

const float speed = 0.01f;
const float scale = 0.1f;
const float falloff = 3.0f;
const float fade = 0.4f;
const float strength = 1.0f;
const float range = 5.0f;

// fractal brownian motion (layers of multi scale noise)
__DEVICE__ float3 fbm(float3 p)
{
    float3 result = to_float3_aw(0);
    float amplitude = 0.5f;
    for (float index = 0.0f; index < 3.0f; ++index)
    {
        result += texture(iChannel0, p/amplitude).xyz * amplitude;
        amplitude /= falloff;
    }
    return result;
}

__KERNEL__ void LiquidToyFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, sampler2D iChannel0, sampler2D iChannel1)
{


    // coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    
    // noise
    float3 spice = fbm(to_float3_aw(uv*scale,iTime*speed));
    
    // draw circle at mouse or in motion
    float t = iTime*2.0f;
    float2 mouse = (swi2(iMouse,x,y) - iResolution / 2.0f)/iResolution.y;
    if (iMouse.z > 0.5f) uv -= mouse;
    else uv -= to_float2(_cosf(t),_sinf(t))*0.3f;
    float paint = trace(length(uv),0.1f);
    
    // expansion
    float2 offset = to_float2(0);
    uv = fragCoord / iResolution;
    float4 data = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    float3 unit = to_float3_aw(range/iResolution,0);
    float3 normal = normalize(to_float3(
        TEX1(uv - swi2(unit,x,z))-TEX1(uv + swi2(unit,x,z)),
        TEX1(uv - swi2(unit,z,y))-TEX1(uv + swi2(unit,z,y)),
        data.x*data.x)+0.001f);
    offset -= swi2(normal,x,y);
    
    // turbulence
    spice.x *= 6.28f*2.0f;
    spice.x += iTime;
    offset += to_float2(_cosf(spice.x),_sinf(spice.x));
    
    // sample buffer
    float4 frame = texture(iChannel1, uv + strength * offset / iResolution);
    
    // temporal fading buffer
    paint = _fmaxf(paint, frame.x - iTimeDelta * fade);
    
    // print result
    fragColor = to_float4(clamp(paint, 0.0f, 1.0f));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1



// Liquid toy by Leon Denise 2022-05-18
// Playing with shading with a fake fluid heightmap

__KERNEL__ void LiquidToyFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    
    // coordinates
    float2 uv = fragCoord / iResolution;
    float3 dither = texture(iChannel1, fragCoord / 1024.0f).rgb;
    
    // value from buffer A
    float4 data =  _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float gray = data.x;
    
    // gradient normal from gray value
    float range = 3.0f;
    float3 unit = to_float3_aw(range/iResolution,0);
    float3 normal = normalize(to_float3(
        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
        gray*gray*gray));
        
    // backlight
    float3 color = to_float3_s(0.3f)*(1.0f-_fabs(dot(normal, to_float3(0,0,1))));
    
    // specular light
    float3 dir = normalize(to_float3(0,1,2));
    float specular = _powf(dot(normal, dir)*0.5f+0.5f,20.0f);
    color += to_float3_s(0.5f)*ss(0.2f,1.0f,specular);
    
    // rainbow
    float3 tint = 0.5f+0.5f*_cosf(to_float3(1,2,3)*1.0f+dot(normal, dir)*4.0f-uv.y*3.0f-3.0f);
    color += tint * smoothstep(0.15f,0.0f,gray);

    // dither
    color -= dither.x*0.1f;
    
    // background blend
    float3 background = to_float3(1);
    background *= smoothstep(1.5f,-0.5f,length(uv-0.5f));
    color = _mix(background, clamp(color, 0.0f, 1.0f), ss(0.01f,0.1f,gray));
    
    // display layers when clic
    if (iMouse.z > 0.5f && iMouse.x/iResolution.x < 0.1f)
    {
        if (uv.x < 0.33f) color = to_float3_aw(gray);
        else if (uv.x < 0.66f) color = normal*0.5f+0.5f;
        else color = to_float3(tint);
    }

    fragColor = to_float4(color, 1);


  SetFragmentShaderComputedColor(fragColor);
}