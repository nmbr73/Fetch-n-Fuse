
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blue Noise' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// global


// spicy fbm gyroid noise
__DEVICE__ float gyroid (float3 seed) { return dot(sin_f3(seed),cos_f3(swi3(seed,y,z,x))); }
__DEVICE__ float fbm (float3 seed, float iTime) {
    float result = 0.0f;
    float a = 0.5f;
    for (int i = 0; i < 6; ++i) {
        seed.z -= iTime*0.1f+result*0.1f;
        result += _fabs(gyroid(seed/a))*a;
        a /= 2.0f;
    }
    return result;
}

// signed distance function
__DEVICE__ float map(float3 p, float iTime, inout float *noise)
{
    *noise = fbm(p*0.5f, iTime);
    float dist = -length(swi2(p,x,y))+2.0f - *noise * *noise;
    return dist*0.5f;
}

__KERNEL__ void CloudyCouldCallFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    
    fragCoord+=0.5f;
    
    float noise;
    
    float3 color = to_float3_s(0);
float AAAAAAAAAAAAAAAAAAA;    
    // coordinates
    float2 uv = (fragCoord-iResolution/2.0f)/iResolution.y;
    float3 pos = to_float3(0,0,5);
    float3 ray = normalize(to_float3_aw(uv,-1));
    
    // animated blue noise by Alan Wolfe
    // https://www.shadertoy.com/view/XsVBDR
    const float c_goldenRatioConjugate = 0.61803398875f;
    float4 rng = texture(iChannel0, fragCoord / to_float2_s(1024.0f));
    //rng = fract_f4(rng + (float)(iFrame%256) * c_goldenRatioConjugate);
    
    // raymarch
    float maxDist = 10.0f;
    const float count = 20.0f;
    float total = 0.0f;
    float dense = 0.0f;
    for (float steps = count; steps > 0.0f; --steps) {
        float dist = map(pos, iTime, &noise);
        dist *= 0.7f+0.3f*rng.x;
        // sort of volumetric march
        if (dist < 0.1f*rng.z) {
            dense += 0.2f;
            dist = 0.02f;
        }
        total += dist;
        if (dense >= 1.0f || total > maxDist) break;
        pos += ray * dist;
    }
    
    // cloud color
    float n = noise;
    #define getAO(dir,k) smoothstep(-k,k,map(pos+dir*k,iTime, &noise)-map(pos-dir*k,iTime, &noise))
    float ao = getAO(to_float3(0,0,1),(0.1f*rng.x+0.1f));
    color = 0.5f+0.5f*cos_f3(to_float3(1,2,3)*0.5f+ao+pos.z*0.3f-uv.y);
    color *= 0.2f+0.8f*n;
    
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

__KERNEL__ void CloudyCouldCallFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float3 color = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    float3 temporal = swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z);
    float3 minColor = to_float3_s(9999.0f), maxColor = to_float3_s(-9999.0f);
    for(int _x = -1; _x <= 1; ++_x){
        for(int _y = -1; _y <= 1; ++_y){
            float3 c = swi3(texture(iChannel0, uv + to_float2(_x, _y) / iResolution),x,y,z);
            minColor = _fminf(minColor, c);
            maxColor = _fmaxf(maxColor, c);
        }
    }
    temporal = clamp(temporal, minColor, maxColor);
    swi3S(fragColor,x,y,z, _mix(color, temporal, 0.9f));
    fragColor.w = 1.0f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0



// Cloudy Could Call
//
// - inadequat volumetric rendering
// - incorrect ambient occlusion
// - blue noise cache-misère
// - chiaroscuro simulacrum
// - nimitz protean clouds wannabe
//
// "Atmosphère ! Atmosphère ! Est-ce que j'ai une gueule d'atmosphère ?"
// - Arletty (1938)

// Buffer A : cloud rendering
// Buffer B : temporal anti aliasing

__KERNEL__ void CloudyCouldCallFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}