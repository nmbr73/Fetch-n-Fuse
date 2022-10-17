
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)

__KERNEL__ void ContinuumDetectorFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    U+=0.5f; 
    
    Q = A(U);
    float4 s = to_float4_s(0);
    float n = 1e-5, w = 0.0f;
    float2 g = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = _fmaxf(_expf(-200.0f*l*l)-0.2f,-1e-3);
        n+=weight;
        s += a*weight;
    }
    Q += (1.0f-w/8.0f)*(s/n-Q);
    Q = clamp(Q,0.0f,1.0f);
    if (iFrame < 20)   Q = B(U);
    if (iMouse.z>0.0f) Q = _mix(Q,Q,_expf(-50.0f*length(U-swi2(iMouse,x,y))/R.y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void ContinuumDetectorFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    U+=0.5f;
    
    
    Q = A(U);
    float4 s = to_float4_s(0);
    float n = 1e-5, w = 0.0f;
    float2 g = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = _fmaxf(_expf(-200.0f*l*l)-0.2f,-1e-3);
         n+=weight;
        s += a*weight;
    }
    Q += (1.0f-w/8.0f)*(s/n-Q);
    Q = clamp(Q,0.0f,1.0f);
    if (iFrame < 20)   Q = B(U);
    if (iMouse.z>0.0f) Q = _mix(Q,Q,_expf(-50.0f*length(U-swi2(iMouse,x,y))/R.y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void ContinuumDetectorFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    U+=0.5f;
    
    Q = A(U);
    float4 s = to_float4_s(0);
    float n = 1e-5, w = 0.0f;
    float2 g = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = _fmaxf(_expf(-200.0f*l*l)-0.2f,-1e-3);
         n+=weight;
        s += a*weight;
    }
    Q += (1.0f-w/8.0f)*(s/n-Q);
    Q = clamp(Q,0.0f,1.0f);
    if (iFrame < 20)   Q = B(U);
    if (iMouse.z>0.0f) Q = _mix(Q,Q,_expf(-50.0f*length(U-swi2(iMouse,x,y))/R.y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: London' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void ContinuumDetectorFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    U+=0.5f;
    
    Q = A(U);
    float4 s = to_float4_s(0);
    float n = 1e-5, w = 0.0f;
    float2 g = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 d = a - Q;
        float l = length(d);
        w += l;
        g += l * u;
        float weight = _fmaxf(_expf(-200.0f*l*l)-0.2f,-1e-3);
        n+=weight;
        s += a*weight;
    }
    Q += (1.0f-w/8.0f)*(s/n-Q);
    Q = clamp(Q,0.0f,1.0f);
    if (iFrame < 20)   Q = B(U);
    if (iMouse.z>0.0f) Q = _mix(Q,Q,_expf(-50.0f*length(U-swi2(iMouse,x,y))/R.y));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void ContinuumDetectorFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    U+=0.5f;
    
    Q = A(U);
    float4 d = to_float4_s(0);
    float w = 0.0f;
    float2 g = to_float2_s(0);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++) {
        float2 u = to_float2(_x,_y);
        float4 a = A(U+u);
        float4 d = a - Q;
        float l = length(d);
        d += a - Q;
        w += l;
        g += l * u;
    }
    float3 n = normalize(to_float3_aw(g,0.001f));
    Q *= 0.7f+0.5f*dot(n,normalize(to_float3(1,1,1)));

  SetFragmentShaderComputedColor(Q);
}