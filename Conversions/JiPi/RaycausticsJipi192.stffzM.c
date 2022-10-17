
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define samples 64
//#define width 8.0f

///////////////// BORROWED FROM "JAYBIRD" /////////////////
// taken from Simple Water Caustic Pattern : https://www.shadertoy.com/view/3d3yRj
// 3D simplex noise adapted from https://www.shadertoy.com/view/Ws23RD 
__DEVICE__ float4 mod289(float4 _x)
{
    return _x - _floor(_x / 289.0f) * 289.0f;
}

__DEVICE__ float4 permute(float4 _x)
{
    return mod289((_x * 34.0f + 1.0f) * _x);
}

__DEVICE__ float4 snoise(float3 v)
{
    const float2 C = to_float2(1.0f / 6.0f, 1.0f / 3.0f);

    // First corner
    float3 i  = _floor(v + dot(v, to_float3_s(C.y)));
    float3 x0 = v   - i + dot(i, to_float3_s(C.x));

    // Other corners
    float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
    float3 l = 1.0f - g;
    float3 i1 = _fminf(swi3(g,x,y,z), swi3(l,z,x,y));
    float3 i2 = _fmaxf(swi3(g,x,y,z), swi3(l,z,x,y));

    float3 x1 = x0 - i1 + C.x;
    float3 x2 = x0 - i2 + C.y;
    float3 x3 = x0 - 0.5f;

    // Permutations
    float4 p =
      permute(permute(permute(i.z + to_float4(0.0f, i1.z, i2.z, 1.0f))
                            + i.y + to_float4(0.0f, i1.y, i2.y, 1.0f))
                            + i.x + to_float4(0.0f, i1.x, i2.x, 1.0f));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float4 j = p - 49.0f * _floor(p / 49.0f);  // mod_f(p,7*7)

    float4 x_ = _floor(j / 7.0f);
    float4 y_ = _floor(j - 7.0f * x_);

    float4 _x = (x_ * 2.0f + 0.5f) / 7.0f - 1.0f;
    float4 _y = (y_ * 2.0f + 0.5f) / 7.0f - 1.0f;

    float4 h = to_float4_s(1.0f) - abs_f4(_x) - abs_f4(_y);

    float4 b0 = to_float4_f2f2(swi2(_x,x,y), swi2(_y,x,y));
    float4 b1 = to_float4_f2f2(swi2(_x,z,w), swi2(_y,z,w));

    float4 s0 = _floor(b0) * 2.0f + 1.0f;
    float4 s1 = _floor(b1) * 2.0f + 1.0f;
    float4 sh = -1.0f*step(h, to_float4_s(0.0f));

    float4 a0 = swi4(b0,x,z,y,w) + swi4(s0,x,z,y,w) * swi4(sh,x,x,y,y);
    float4 a1 = swi4(b1,x,z,y,w) + swi4(s1,x,z,y,w) * swi4(sh,z,z,w,w);

    float3 g0 = to_float3_aw(swi2(a0,x,y), h.x);
    float3 g1 = to_float3_aw(swi2(a0,z,w), h.y);
    float3 g2 = to_float3_aw(swi2(a1,x,y), h.z);
    float3 g3 = to_float3_aw(swi2(a1,z,w), h.w);

    // Compute noise and gradient at P
    float4 m = _fmaxf(to_float4_s(0.6f) - to_float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), to_float4_s(0.0f));
    float4 m2 = m * m;
    float4 m3 = m2 * m;
    float4 m4 = m2 * m2;
    float3 grad =
      -6.0f * m3.x * x0 * dot(x0, g0) + m4.x * g0 +
      -6.0f * m3.y * x1 * dot(x1, g1) + m4.y * g1 +
      -6.0f * m3.z * x2 * dot(x2, g2) + m4.z * g2 +
      -6.0f * m3.w * x3 * dot(x3, g3) + m4.w * g3;
    float4 px = to_float4(dot(x0, g0), dot(x1, g1), dot(x2, g2), dot(x3, g3));
    return 42.0f * to_float4_aw(grad, dot(m4, px));
}
///////////////// END OF BORROWED CODE /////////////////

__DEVICE__ float gaussianf(float sigma, float dist)
{
    const float inv_sqrt_2pi = 0.3989422804014327f;
    float a = dist / sigma;
    return (inv_sqrt_2pi / sigma ) * _expf( -0.5f * a * a );
}

__DEVICE__ float ray_caustic(float2 uv, float2 res, float st, float width, float iTime)
{
    float2 pix_l = 1.0f/res;
    
    float sample_step = width*st/(float)(samples);
    
    int h_samples = samples / 2;
    
    float sum = 0.0f;
    for( int i = 0; i < samples; i++)
    {
        for(int y = 0; y < samples; y++)
        {
            float2 stepm = sample_step*to_float2(i-h_samples,y-h_samples);
            float2 coords = uv+pix_l*stepm;
            float2 dir = -st*swi2(snoise(to_float3_aw(coords,iTime*0.1f)),x,y);
            float dist = distance_f2(uv*res,uv*res+stepm+dir);
            sum += width*0.04f*gaussianf(4.0f,dist);
        }
    }
    return sum;
}

__KERNEL__ void RaycausticsJipi192Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER0(St, 0.0f, 100.0f, 50.0f);
    CONNECT_SLIDER1(width, 0.0f, 20.0f, 8.0f);
    

    float2 uv = (5.0f * fragCoord - iResolution) / iResolution.y;

    fragColor = to_float4_aw(to_float3_s(ray_caustic(uv,iResolution,St, width, iTime)),1.0f) + texture(iChannel0, fragCoord/iResolution);;


  SetFragmentShaderComputedColor(fragColor);
}