
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------



__DEVICE__ float2 hash( float2 p ) {
  p = to_float2( dot(p,to_float2(354.3f,542.8f)), dot(p,to_float2(185.4f,196.3f)) );

  return -1.0f + 2.0f*fract_f2(sin_f2(p) * 68556.4357786f);
}

__DEVICE__ float noise( in float2 p ) {
    float f1 = 0.366f;
    float f2 = 0.211324865f; 
    float2 k = _floor( p + (p.x+p.y) *f1);
    float2 a = p - k + (k.x+k.y) * f2;
    float2 s = step(swi2(a,y,x),swi2(a,x,y));
    float2 b = a - s + f2;
    float2 c = a - 1.0f + 2.0f*f2;

    float3 h = _fmaxf( 0.5f-to_float3(dot(a,b), dot(b,b), dot(c,c) ), to_float3_s(0.0f) );

    float3 n = h*h*h*h*to_float3( dot(a,hash(k+0.0f)), dot(b,hash(k+s)), dot(c,hash(k+1.0f)));

    return dot( n, to_float3_s(70.0f) );
}


__DEVICE__ float fbm ( in float2 p ) {
    float f = 0.0f;
    mat2 m = to_mat2( 1.6f,  1.2f, -1.2f,  1.6f );
    f  = 0.5000f*noise(p); p = mul_mat2_f2(m,p);
    f += 0.2500f*noise(p); p = mul_mat2_f2(m,p);
    f += 0.1250f*noise(p); p = mul_mat2_f2(m,p);
    f += 0.0625f*noise(p); p = mul_mat2_f2(m,p);
    f = 0.5f + 0.5f * f;
    return f;
}

__DEVICE__ float3 map(float2 uv) {
    float2 s = to_float2(1.0f/630.0f, 1.0f/354.0f);
    float p = fbm(uv);
    float h = fbm(uv + s * to_float2(1.0f, 0));
    float v = fbm(uv + s * to_float2(0, 1.0f));
    float2 xy = (p - to_float2(h, v))*40.0f;
    return to_float3_aw(xy + 0.2f, 9.0f);
}

__KERNEL__ void CrackedmirrorfireJipi154Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{


    float2 uv = fragCoord / iResolution;
    float2 fmove= to_float2(-0.02f, 0.0f);
    float3 m = map(uv * to_float2(1.0f, 0.3f) + fmove*iTime);
    float2 disp = clamp((swi2(m,x,y) - 0.5f) * 0.15f, -1.0f, 1.0f);
    uv += disp;
    float2 fmove1= to_float2(-0.02f, -0.3f);
    float2 uv1 = (uv * to_float2(1.0f, 0.5f)) + fmove1 * iTime;
    
    float n = fbm(3.2f * uv1);
    float col = _powf(1.0f - uv.y, 4.0f) * 4.0f;
    float colN = n * col; 

    float3 color = colN * to_float3(3.0f*n, 3.0f*n*n*n, n*n*n*n);
    float3 color2 = colN * to_float3(2.0f*n, 3.2f*n*n*n,2.0f*n*n*n*n);
    color = _mix(color, swi3(color2,x,y,z), _powf(1.0f - uv.y, 1.5f));
    
    fragColor = to_float4_aw(color , 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}