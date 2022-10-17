
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ mat2 rot(float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }
__DEVICE__ float3 lookAt (float3 from, float3 at, float2 uv, float fov)
{
  float3 z = normalize(at-from);
  float3 x = normalize(cross(z, to_float3(0,1,0)));
  float3 y = normalize(cross(x, z));
  return normalize(z * fov + uv.x * x + uv.y * y);
}

// Inigo Quilez
// https://iquilezles.org/articles/distfunctions/
__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}
__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
  return length(q)-t.y;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel1
// Connect Buffer A 'Texture: Blue Noise' to iChannel0



// fractal brownian motion https://thebookofshaders.com/13/
// with a "_fabs(_sinf(value))" twist 
__DEVICE__ float3 fbm (float3 p, __TEXTURE2D__ iChannel1)
{
    float3 result = to_float3_s(0.0f);
    float a = 0.5f;
    for (float i = 0.0f; i < 3.0f; ++i) {
        result += abs_f3(sin_f3(swi3(texture(iChannel1, p/a),x,y,z)*6.0f))*a;
        a /= 2.0f;
    }
    return result;
}

// signed distance function
__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel1)
{
    float dist = 100.0f;
    
    // timing
    float time = iTime;
    float anim = fract(time);
    float index = _floor(time);
    
    // noise animation
    float scale = 0.1f-anim*0.05f;
    float3 seed = p * scale + index * 0.12344f;
    float3 noise = fbm(seed,iChannel1);
    
    // shapes and distortions
    float size = 0.5f*_powf(anim,0.2f);
    float type = mod_f(index, 3.0f);
    dist = type > 1.5f ? sdTorus(p, to_float2(size, 0.1f)) :
           type > 0.5f ? sdBox(p,to_float3_s(size*0.7f)) :
           length(p)-size;
    dist -= anim * noise.x * 0.2f;
    dist += _powf(anim, 3.0f) * noise.y;
  
    // scale field when highly distorted to avoid artefacts
    return dist * (1.0f-anim*0.7f);
}

__KERNEL__ void TurboRainbowDissolverFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    float2 uv = (fragCoord-iResolution/2.0f)/iResolution.y;
  
    // background
    float3 color = to_float3_s(0.5f)*smoothstep(2.0f,0.5f,length(uv));
    
    // coordinates
    float3 pos = to_float3(0,0,-1.5f);
    swi2S(pos,x,z, mul_f2_mat2(swi2(pos,x,z) , rot(iTime*0.1f)));
    swi2S(pos,z,y, mul_f2_mat2(swi2(pos,z,y) , rot(_sinf(iTime*0.2f))));
    float3 ray = lookAt(pos, to_float3_s(0), uv, 1.5f);
    
    // noise
    float3 blue = swi3(texture(iChannel0, fragCoord/1024.0f),x,y,z);
    
    // raymarch
    const float count = 30.0f;
    float steps = 0.0f;
    float total = 0.0f;
    for (steps = count; steps > 0.0f; --steps) {
        float dist = map(pos, iTime,iChannel1);
        if (dist < total/iResolution.y || total > 3.0f) break;
        dist *= 0.9f+0.1f*blue.z;
        pos += ray * dist;
        total += dist;
    }
    
    // coloring
    float shade = steps/count;
    if (shade > 0.001f && total < 3.0f) {
    
        // NuSan https://www.shadertoy.com/view/3sBGzV
        float2 noff = to_float2(0.02f,0);
        float3 normal = normalize(map(pos,iTime,iChannel1)-to_float3(map(pos-swi3(noff,x,y,y),iTime,iChannel1), map(pos-swi3(noff,y,x,y),iTime,iChannel1), map(pos-swi3(noff,y,y,x),iTime,iChannel1)));
        
        color = to_float3_s(0.1f);
        float light = dot(reflect(ray, normal), to_float3(0,1,0))*0.5f+0.5f;
        float rainbow = dot(normal, -1.0f*normalize(pos))*0.5f+0.5f;
        color += to_float3_s(0.5f)*_powf(light, 4.5f);
        
        // Inigo Quilez color palette https://iquilezles.org/articles/palettes/
        color += (0.5f+0.5f*cos_f3(to_float3(0.0f,0.3f,0.6f)*6.0f+uv.y*3.0f+iTime))*_powf(rainbow,4.0f);
        color *= _powf(shade,0.5f);
    }
    
    fragColor = to_float4_aw(color, 1);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Temporal Anti Aliasing from:
// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/

// but only the color clamping...
// it's very subtle but I like it...

__KERNEL__ void TurboRainbowDissolverFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float3 color = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 temporal = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    float3 minColor = to_float3_s(9999.0f), maxColor = to_float3_s(-9999.0f);
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            float3 c = swi3(texture(iChannel0, uv + to_float2(x, y) / iResolution),x,y,z);
            minColor = _fminf(minColor, c);
            maxColor = _fmaxf(maxColor, c);
        }
    }
    temporal = clamp(temporal, minColor, maxColor);

    fragColor = to_float4_aw(_mix(color, temporal, 0.9f), 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0



// popping shapes in a turbo rainbow dissolver

// main code is in Buffer A
// Buffer B is a minimal temporal anti aliasing
__KERNEL__ void TurboRainbowDissolverFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}