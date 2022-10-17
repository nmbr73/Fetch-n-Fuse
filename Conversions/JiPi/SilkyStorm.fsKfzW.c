
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define repeat(p,r) (mod_f(p,r)-r/2.0f)
__DEVICE__ mat2 rot(float a) { return to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }
__DEVICE__ float3 lookAt (float3 from, float3 at, float2 uv, float fov)
{
  float3 z = normalize(at-from);
  float3 x = normalize(cross(z, to_float3(0,1,0)));
  float3 y = normalize(cross(x, z));
  return normalize(z * fov + uv.x * x + uv.y * y);
}
__DEVICE__ float gyroid (float3 s)
{
  return dot(sin_f3(s),cos_f3(swi3(s,y,z,x)));
}

// Dave Hoskins
// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float3 hash33(float3 p3) {
  p3 = fract(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel1
// Connect Buffer A 'Texture: Blue Noise' to iChannel0



// fractal brownian motion
// https://thebookofshaders.com/13/
__DEVICE__ float fbm (float3 p, float iTime)
{
    float result = 0.0f;
    float a = 0.5f;
    for (float i = 0.0f; i < 4.0f; ++i) {
        result += _sinf(gyroid(p/a)*3.14f+0.1f*iTime/a)*a;
        a /= 2.0f;
    }
    return result;
}

// signed distance function
__DEVICE__ float map(float3 p, float iTime)
{
    // tunnel
    float dist = _fmaxf(0.0f, -length(swi2(p,x,y))+0.5f);
    
    // displace with gyroid noise
    float t = iTime * 0.1f;
    float3 s = p * 1.0f;
    s.z -= t;
    float noise = fbm(s, iTime);
    dist -= 0.1f*noise;
    
    // filaments
    dist = _fminf(dist, _fabs(noise)+_fmaxf(0.0f,-p.z)*0.003f);
    
    return dist;
}

__DEVICE__ void coloring (inout float3 *color, in float3 pos, in float3 normal, in float3 ray, in float2 uv, in float shade, float iTime)
{
    // Inigo Quilez color palette
    // https://iquilezles.org/www/articles/palettes/palettes.htm
    float3 tint = 0.5f+0.5f*cos_f3(to_float3(0,0.3f,0.6f)*6.283f+iTime*0.2f+uv.y*2.0f);

    // lighting
    float3 rf = reflect(ray, normal);
    float top = dot(rf, to_float3(0,1,0))*0.5f+0.5f;
    float glow = dot(normal, ray)*0.5f+0.5f;
    *color = to_float3_s(0.5f) * _powf(dot(normal, -1.0f*normalize(pos))*0.5f+0.5f, 0.5f);
    *color += to_float3_s(0.2f)*clamp(top,0.0f,1.0f);
    *color += tint*glow;
    *color *= _powf(shade,0.5f);
}

__KERNEL__ void SilkyStormFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float3 color = to_float3_s(0);
    
    // coordinates
    float2 uv = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 pos = to_float3(0,0,1);
    float3 at = to_float3_s(0);
    swi2S(pos,x,z, mul_f2_mat2(swi2(pos,x,z) , rot(_cosf(iTime*0.1f)*0.2f)));
    swi2S(pos,z,y, mul_f2_mat2(swi2(pos,z,y) , rot(_sinf(iTime*0.2f)*0.1f)));
    float3 ray = lookAt(pos, at, uv, 1.0f);
    
    // noise
    float3 blue = swi3(texture(iChannel0, fragCoord/1024.0f),x,y,z);
    float3 white = hash33(to_float3_aw(fragCoord, iFrame));
    
    // start ahead
    pos += ray * white.z * 0.2f;
    
    // blur edges
    float dof = 0.2f*smoothstep(0.5f, 2.0f, length(uv));
    swi2S(ray,x,y, swi2(ray,x,y) + to_float2(_cosf(blue.x*6.28f),_sinf(blue.x*6.28f))*blue.z*dof);
    
    // raymarch
    float maxDist = 8.0f;
    const float count = 50.0f;
    float steps = 0.0f;
    float total = 0.0f;
    for (steps = count; steps > 0.0f; --steps) {
        float dist = map(pos, iTime);
        if (dist < total/iResolution.y || total > maxDist) break;
        dist *= 0.9f+0.1f*blue.z;
        ray += white * total*0.002f;
        pos += ray * dist;
        total += dist;
    }
    
    // coloring
    float shade = steps/count;
    if (shade > 0.001f && total < maxDist) {
        // NuSan
        // https://www.shadertoy.com/view/3sBGzV
        float2 noff = to_float2(0.01f,0);
        float3 normal = normalize(map(pos, iTime)-to_float3(map(pos-swi3(noff,x,y,y), iTime), map(pos-swi3(noff,y,x,y), iTime), map(pos-swi3(noff,y,y,x), iTime)));
        coloring(&color, pos, normal, ray, uv, shade, iTime);
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

__KERNEL__ void SilkyStormFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
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



// Silky Storm
// gyroidisticly tunneled

// main code is in Buffer A
// Buffer B is a minimal temporal anti aliasing
__KERNEL__ void SilkyStormFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}