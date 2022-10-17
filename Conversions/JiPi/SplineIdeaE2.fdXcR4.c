
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define tanh_f2(i) to_float2(_tanhf((i).x), _tanhf((i).y))

// Collection of functions I use a lot:

#define pi 3.14159f

__DEVICE__ float thc(float a, float b) {
    return _tanhf(a * _cosf(b)) / _tanhf(a);
}

__DEVICE__ float ths(float a, float b) {
    return _tanhf(a * _sinf(b)) / _tanhf(a);
}

__DEVICE__ float2 thc(float a, float2 b) {
    return tanh_f2(a * cos_f2(b)) / _tanhf(a);
}

__DEVICE__ float2 ths(float a, float2 b) {
    return tanh_f2(a * sin_f2(b)) / _tanhf(a);
}

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float mlength(float2 uv) {
  
    return _fmaxf(_fabs(uv.x), _fabs(uv.y));
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float2 P(float time) {
    
    // Offset y values from x values (so it doesnt just move diagonally)
    float o = 0.01f;
    // Scale point locations to fit to screen
    float sc = 0.8f;
    
    // Next 4 points in sequence
    // (bad approach - has a bottom-left bias) (does it tho?)
    float2 p0 = -sc * 0.5f + sc * to_float2( h21(to_float2_s(_floor(time))),        h21(to_float2_s(o + _floor(time))) );
    float2 p1 = -sc * 0.5f + sc * to_float2( h21(to_float2_s(_floor(time + 1.0f))), h21(to_float2_s(o + _floor(time + 1.0f))) );
    float2 p2 = -sc * 0.5f + sc * to_float2( h21(to_float2_s(_floor(time + 2.0f))), h21(to_float2_s(o + _floor(time + 2.0f))) );
    float2 p3 = -sc * 0.5f + sc * to_float2( h21(to_float2_s(_floor(time + 3.0f))), h21(to_float2_s(o + _floor(time + 3.0f))) );

    float f = fract(time);

    float t = 0.8f;

    mat4 M = to_mat4(   0,       1,            0,   0,
                       -t,       0,            t,   0,
                   2.0f*t,  t-3.0f,  3.0f-2.0f*t,  -t,
                       -t,  2.0f-t,       t-2.0f,   t);
                       
    float4 U = to_float4(1.0f, f, f*f, f*f*f);
    float4 Px = to_float4(p0.x, p1.x, p2.x, p3.x);
    float4 Py = to_float4(p0.y, p1.y, p2.y, p3.y);
    //return to_float2(dot(Px, mul_mat4_f4(M , U)), dot(Py, mul_mat4_f4(M , U)));
    return to_float2(dot(Px, mul_f4_mat4(U,M)), dot(Py, mul_f4_mat4(U,M)));
}


__KERNEL__ void SplineIdeaE2Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    fragCoord+=0.5f;

    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;    
    
        
    float2 uv2 = uv;
    float time = 0.005f * h21(uv) + 1.8f * _cosf(3.5f * length(uv) + 0.5f * iTime) + 0.25f * iTime;
    float2 p = P(time);
   // float2 p2 = P(time - 0.5f * 0.1f);
    uv2 += p;
 

   // float th = pi/4.0f + _atan2f(p.y - p2.y, p.x - p2.x);
    float th = 1.0f * iTime;
    mat2 R = to_mat2(_cosf(th), _sinf(th), -_sinf(th), _cosf(th));
    uv2 = mul_f2_mat2(uv2,R);
    
    //uv2 = mul_mat2_f2(R,uv2);
    
    uv2 = abs_f2(uv2);

    float2 pt = 0.15f * (0.5f + 0.5f * ths(2.0f, iTime)) / _sqrtf(2.0f) * to_float2_s(1);
    //pt += 0.05f * to_float2(_cosf(24.0f * uv.x + 4.0f * iTime), _sinf(24.0f * uv.y + 4.0f * iTime));
    float d = length(uv2 - pt);
    
    float k = 2.0f / iResolution.y;
    float s = smoothstep(-k, k, -d + 0.025f);
    float _x = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;
    
    //_x = texture(iChannel0, fragCoord/iResolution).x;
    
    _x = 0.985f * clamp(_x, 0.0f, 1.0f);
    s = _fmaxf(_x, s);
    /*
    float d = length(uv - p);
    float k = 1.0f / iResolution.y;
    float s = smoothstep(-k, k, -d + 0.025f);
    float x = texelFetch( iChannel0, to_int2(fragCoord), 0 ).x;
    x = 0.98f * clamp(x, 0.0f, 1.0f);
    s = _fmaxf(x, s);
    */
    
    //s = _fmaxf(s, f * smoothstep(-k, k, -length(uv - p2) + 0.025f));

    fragColor = to_float4_s(s);

    if (iFrame < 0 || Reset) fragColor = to_float4_s(0);

  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SplineIdeaE2Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;
float IIIIIIIIIIIIIIIIIII;    
    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;

    //float x = texelFetch( iChannel0, to_int2(fragCoord), 0 ).x;
    float x = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution ).x;
    x = 4.0f * x * (1.0f-x);
    x = x * x;
    float3 e = to_float3_s(1.0f);
    float3 col2 = 2.0f * x * pal(_mix(0.3f, 0.35f, 0.5f + 0.5f * thc(2.0f, iTime)), e, e, e, to_float3(0.0f, 0.33f, 0.66f));
    fragColor = to_float4_aw(0.06f + col2, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}