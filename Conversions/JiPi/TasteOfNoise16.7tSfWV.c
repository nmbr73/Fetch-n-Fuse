
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



// shortcut to sample texture
#define TEX(uv) _tex2DVecN(iChannel0,uv.x,uv.y,15).r

// polar domain repetition used for the triangle
__DEVICE__ float2 moda(float2 p, float repetitions)
{
  float angle = 2.0f*3.14f/repetitions;
  float a = _atan2f(p.y, p.x) + angle/2.0f;
  a = mod_f(a,angle) - angle/2.0f;
  return to_float2(_cosf(a), _sinf(a))*length(p);
}

// rotation matrix
__DEVICE__ mat2 rot (float a) { return mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)); }
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/aea6b99da1d53055107966b59ac5444fc8bc7b3ce2d0bbb6a4a3cbae1d97f3aa.bin' to iChannel0



// The noise animated pass with shape and glow layers

const float speed = 0.01f;
const float scale = 0.08f;
const float cycle = 1.5f;
const float falloff = 1.8f;

// transform linear value into cyclic absolute value
__DEVICE__ float3 bend(float3 v)
{
    return _fabs(_sinf(v*cycle*6.283f+iTime*6.283f*speed*10.0f));
}

// fractal brownian motion (layers of multi scale noise)
__DEVICE__ float3 fbm(float3 p)
{
    float3 result = to_float3_aw(0);
    float amplitude = 0.5f;
    for (float index = 0.0f; index < 4.0f; ++index)
    {
        result += bend(texture(iChannel0, p/amplitude).xyz) * amplitude;
        amplitude /= falloff;
    }
    return result;
}

__KERNEL__ void TasteOfNoise16Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    // noise from coordinates
    float2 uv = (fragCoord - iResolution / 2.0f)/iResolution.y;
    float3 noise = fbm(to_float3_aw(uv, iTime * speed) * scale);
    
    // fade noise with circle
    noise.x -= 0.5f*smoothstep(0.3f,0.0f,_fabs(length(uv)-0.6f));
    
    // keyhole shape
    float shape = 10.0f;
    shape = _fminf(shape, _fmaxf(0.0f,length(uv-to_float2(0,0.05f))-0.07f));
    uv.y += 0.1f;
    shape = _fminf(shape, _fmaxf(0.0f, moda(uv*rot(1.57f), 3.0f).r-0.05f));
    
    // add shape to soft white
    noise.x += 0.5f*smoothstep(0.1f,0.0f,_fabs(shape-0.03f));
    
    // add shape to glow
    noise.y *= smoothstep(0.0f,0.1f,_fabs(shape-0.01f));
    
    // remove shape
    float hole = smoothstep(0.04f,0.0f,shape+0.02f);
    noise.x -= hole*1.9f;
    noise.y += hole;
    
    fragColor = to_float4(clamp(noise, 0.0f, 1.0f), 1);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0



// Taste of Noise 16 by Leon Denise 2022-05-17

// An experiment of lighting with a 3D FBM noise.
// Trying to render organic volumes without raymarching.
// Clic to display the diffent layers, which are from left to right: 
// height, normal, noise, glow, lighting and shape

__KERNEL__ void TasteOfNoise16Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    // coordinates
    float2 uv = fragCoord / iResolution;
    
    // value from noise buffer A
    float3 noise = _tex2DVecN(iChannel0,uv.x,uv.y,15).rgb;
    float gray = noise.x;
    
    // gradient normal from gray value
    float3 unit = to_float3_aw(5.0f/iResolution,0);
    float3 normal = normalize(to_float3(
        TEX(uv + swi2(unit,x,z))-TEX(uv - swi2(unit,x,z)),
        TEX(uv - swi2(unit,z,y))-TEX(uv + swi2(unit,z,y)),
        gray*gray));
    
    float3 dir = normalize(to_float3(0,1,0.2f)); // light direction
    float angle = dot(normal, dir); // light and surface angle
    float3 color = to_float3_s(0.2f); // ambient
    float light = _powf(angle*0.5f+0.5f,10.0f);
    float soft = 0.5f*smoothstep(0.0f,0.2f,gray-0.75f);
    float glow = 0.1f/(noise.y*noise.y)*noise.z;
    float3 tint = 0.5f+0.5f*_cosf(to_float3(1,2,3)+length(uv)*3.0f+iTime+angle); // iq palette
    color += to_float3(1)*light; // specular light
    color += soft; // soft white
    color += tint*glow; // glow rainbow
    color *= gray; // shadows
    
    // display layers when clic
    if (iMouse.z > 0.5f)
    {
        if (uv.x < 0.16f) color = to_float3(gray);
        else if (uv.x < 0.33f) color = normal*0.5f+0.5f;
        else if (uv.x < 0.5f) color = to_float3(1.0f-noise);
        else if (uv.x < 0.66f) color = to_float3(glow);
        else if (uv.x < 0.86f) color = to_float3(0.2f+light)*gray;
        else color = to_float3(soft);
        if (uv.y < 0.02f) color = 0.5f+0.5f*_cosf(to_float3(1,2,3)+uv.x*3.0f+iTime);
    }

    fragColor = to_float4_aw(color, 1);


  SetFragmentShaderComputedColor(fragColor);
}