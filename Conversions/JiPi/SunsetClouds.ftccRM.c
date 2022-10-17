
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

// Connect Buffer A 'Texture: Textur' to iChannel0

__DEVICE__ float hash(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,z,y,x) + 31.32f);
  return fract((p3.x + p3.y) * p3.z);
}
#define ei(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))

__DEVICE__ float noise ( float3 h ) {
    float3 f = _floor(h);
    float3 c = ceil_f3(h);
    float3 r = fract_f3(h);
    float _000 = hash(f),
          _001 = hash(to_float3_aw(swi2(f,x,y),c.z)),
          _010 = hash(to_float3(f.x,c.y,f.z)),
          _011 = hash(to_float3(f.x,c.y,c.z)),
          _100 = hash(to_float3(c.x,f.y,f.z)),
          _101 = hash(to_float3(c.x,f.y,c.z)),
          _110 = hash(to_float3_aw(swi2(c,x,y),f.z)),
          _111 = hash(c),
          _00 = _mix(_000,_001,r.z),
          _01 = _mix(_010,_011,r.z),
          _10 = _mix(_100,_101,r.z),
          _11 = _mix(_110,_111,r.z),
          _0 = _mix(_00,_01,r.y),
          _1 = _mix(_10,_11,r.y);
          return _mix(_0,_1,r.x);
}
__DEVICE__ float fbm (float3 p)
{
    float w = 0.0f;
    float N = 10.0f;
    for (float i = 1.0f; i < N; i+=1.0f)
    {
        swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) * 2.0f , ei(2.0f)));
        swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , ei(1.0f)));
        w += 4.0f*noise(p)/N*_powf(2.0f,-i);
    }
    return w;
}


__KERNEL__ void SunsetCloudsFuse__Buffer_A(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_CHECKBOX1(Textur, 0);

    U+=0.5f;

    float2 tuv = U/R;

    U = 3.0f*(U-0.5f*R)/R.y;
    //swi2(U,x,y) +=1.5f;
    U.x += 1.5f;
    U.y += 1.5f;
    
    float3 v = 2.0f*to_float3_aw(U,3.0f-0.2f*(float)(iFrame)/60.0f);
    float w = fbm(v)-0.02f;
    w *= 1.5f*_expf(0.05f*v.z);
    w *= 0.8f*smoothstep(-5.0f,1.0f,_expf(-v.x)-v.y+0.5f*v.z-2.0f);
    w = smoothstep(0.12f,0.21f,w);
    Q = to_float4_s(w*w);
    
    if(Textur) Q = texture(iChannel0, tuv);
    
    SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SunsetCloudsFuse__Buffer_B(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    U+=0.5f;
    
    Q = to_float4_s(1);
    for (float _x = 0.0f; _x < 40.0f; _x+=1.0f){
        float a = A(U+4.0f*to_float2(_x,0.3f*_x)).x;
        Q -= 0.01f*Q*a;
    }
    Q = to_float4(1,2,3,4)*0.2f+Q*sin_f4(0.5f+Q-(1.1f-_sqrtf((float)(iFrame)/60.0f)*0.2f)+to_float4(1,2,3,4));
    Q.w = A(U).x;
    
    SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void SunsetCloudsFuse__Buffer_C(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);

    U+=0.5f;
    
    Q = A(U);
    Q = _mix(C(U),Q*Q,Q.w);
    
    if (iFrame < 1 || Reset) {
        Q = 0.5f+0.5f*sin_f4(5.0f+U.y/R.y+to_float4(1,2,3,4));
    }
        
    SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void SunsetCloudsFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    U+=0.5f;
    
    Q = 0.9f*A(U);
    
    SetFragmentShaderComputedColor(Q);
}