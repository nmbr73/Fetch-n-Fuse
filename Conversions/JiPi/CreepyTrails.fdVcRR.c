
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi 3.14159f

__DEVICE__ float4 cell(in int2 p, float2 iResolution, __TEXTURE2D__ iChannel0) {
    int2 r = to_int2_cfloat(iResolution);//to_int2(textureSize(iChannel0, 0));
    p = to_int2((p.x+r.x) % r.x,(p.y+r.y) % r.y);
    //return texelFetch(iChannel0, p, 0 );
    return texture(iChannel0, (make_float2(p.x,p.y)+0.5f)/iResolution );
} 

// From iq
__DEVICE__ float smin( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); 
}

__KERNEL__ void CreepyTrailsFuse__Buffer_A(float4 col, float2 f, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
    f+=0.5f;

    int2 px = to_int2_cfloat(f);
    float4 test = cell(px,iResolution,iChannel0);
    //px = to_int2(1.2f * (f-res));
    
    // Centre coords
    float2 res = _floor(0.5f * iResolution);
    f -= res;
    
    px = to_int2_cfloat(1.5f * f + 1.0f * iTime * to_float2(1,0));
    
    // Speed of time
    float spd = 0.125f;

    // Number of blobs
    float n = 40.0f;
    
    // Distance from blobs
    float d = 1e5;
    
    for (float i = 0.0f; i < n; i+=1.0f) {
        // Offset each blob
        float io = 2.0f * pi * i / n;
        
        // Time
        float t = spd * iTime + 2.0f * pi * _cosf(0.5f * spd * iTime + io);
        
        // Motion of blobs (idk how this works)
        float c = 1.0f + 0.5f * _cosf(4.0f * t + io);
        d = smin(d, c * length(f - 120.0f * (c-0.5f) * to_float2(_cosf(t+io), _sinf(t+io))), 2.0f);  
     }
     
     // Harsh shape
     float r = step(d, 5.0f);
     
     // Soft shape (going inwards)
     float s = smoothstep(0.0f, 5.0f, -d + 5.0f);
     
     float4 cl = cell(px,iResolution,iChannel0);
     cl = _mix(cl, cell(to_int2_cfloat(f),iResolution,iChannel0), 0.95f);
     col = to_float4_s(s);
     
     col = _mix(col + 0.99f * cl, 0.999f * abs_f4(col-cl), 1.0f);//cl * 0.999f;

     col = clamp(col, 0.0f, 1.0f);
     //col *= 0.5f;

  SetFragmentShaderComputedColor(col);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

#ifdef XXX
vec4 cell(in int2 p) {
    int2 r = to_int2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    return texelFetch(iChannel0, p, 0 );
} 
#endif

__DEVICE__ float3 pal(float t) {
    float3 d = 1.0f * to_float3(0,1,2)/3.0f;
    return 1.0f + 2.0f * cos_f3(6.28319f * (0.5f * t + d));
}

#define thc(a,b) _tanhf(a*_cosf(b))/_tanhf(a)

__KERNEL__ void CreepyTrailsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;
    
    // Zoom + distort
    float mx = 0.5f + 0.5f * thc(2.5f, length(uv) * 0.5f - 0.4f * iTime);
    float zm = _mix(0.44f, 0.6f, mx);
    zm += 1.5f - 1.5f * _tanhf(0.04f * iTime * iTime);
    
    //zm -= 0.3f * _tanhf(24.0f - length(uv) * 50.0f);
    
    // Pixel + cells etc.
    float2 res = 0.5f * _floor(iResolution);    
    int2 px = to_int2_cfloat(zm * fragCoord + (1.0f-zm) * res);
    
    float4 c = cell(px,iResolution,iChannel0);
    float4 b = cell(px - to_int2(0,1),iResolution,iChannel0);
    float4 t = cell(px + to_int2(0,1),iResolution,iChannel0);  
    float4 l = cell(px - to_int2(1,0),iResolution,iChannel0);
    float4 r = cell(px + to_int2(1,0),iResolution,iChannel0);    
    float4 sum = b + t + l + r;
      
    // Lighten right side
    float cn = 0.06f * smoothstep(-0.2f, 0.2f, uv.x); 
    
    // Cut colors into lines
    float fl = clamp(uv.y, -0.125f, 0.125f);
    
    // Background
    float3 col = pal(0.05f * (uv.x + 5.0f * uv.y) + 0.2f * cn - 0.7f);
    
    // Exterior outline
    // if (sum.x > 1.0f)
    col = c.y * pal(0.2f + uv.x * 0.2f + 0.3f * _floor(c.y * 40.0f) /40.0f);  
    
    //col -= swi3(c,x,y,z);
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}