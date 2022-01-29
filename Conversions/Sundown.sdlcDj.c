
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x, (U).y/R.y, 15)

#define H 50.0f*_expf(-10.0f*(U.x-0.5f*R.x)*(U.x-0.5f*R.x)/R.x/R.x)
#define ei(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
__DEVICE__ float3 hash (float2 p)
{
    float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}
__DEVICE__ float3 noise(float2 p){
    float4 w = to_float4_f2f2(_floor(p),ceil_f2(p));
    float3 _00 = hash(swi2(w,x,y)),
           _01 = hash(swi2(w,x,w)),
           _10 = hash(swi2(w,z,y)),
           _11 = hash(swi2(w,z,w)),
            _0 = _mix(_00,_01,fract(p.y)),
            _1 = _mix(_10,_11,fract(p.y));
     return _mix(_0,_1,fract(p.x));
}
__DEVICE__ float3 fbm (float2 p) {
    float3 w = to_float3_s(0);
    float N = 5.0f;
    for (float i = 1.0f; i < N; i+=1.0f) {
        p = mul_f2_mat2(p*1.7f,ei(0.5f));
        w += noise(p)/N/i;
    }
    return w;
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


#define T(U) (A(U-swi2(A(U),x,y)))

__KERNEL__ void SundownFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f; 
    Q = T(U);
    float4 n = T(U+to_float2(0,1)),
           e = T(U+to_float2(1,0)),
           s = T(U-to_float2(0,1)),
           w = T(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y)-0.25f*to_float2(e.z-w.z,n.z-s.z));
    swi2S(Q,x,y, swi2(Q,x,y)-0.05f*to_float2(s.w-n.w,e.w-w.w));
    Q.z = 0.25f*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.0f-2.0f*U.y/R.y);
    float h = H;
    Q.w = _mix(Q.w,1.0f,smoothstep(2.0f*h,h,U.y));
    if(U.x<1.0f||R.x-U.x<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    if(U.y<1.0f||R.y-U.y<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SundownFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = T(U);
    float4 n = T(U+to_float2(0,1)),
           e = T(U+to_float2(1,0)),
           s = T(U-to_float2(0,1)),
           w = T(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y)-0.25f*to_float2(e.z-w.z,n.z-s.z));
    swi2S(Q,x,y, swi2(Q,x,y)-0.05f*to_float2(s.w-n.w,e.w-w.w));
    Q.z = 0.25f*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.0f-2.0f*U.y/R.y);
    float h = H;
    Q.w = _mix(Q.w,1.0f,smoothstep(2.0f*h,h,U.y));
    if(U.x<1.0f||R.x-U.x<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    if(U.y<1.0f||R.y-U.y<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    
  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void SundownFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = T(U);
    float4 n = T(U+to_float2(0,1)),
           e = T(U+to_float2(1,0)),
           s = T(U-to_float2(0,1)),
           w = T(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y)-0.25f*to_float2(e.z-w.z,n.z-s.z));
    swi2S(Q,x,y, swi2(Q,x,y)-0.05f*to_float2(s.w-n.w,e.w-w.w));
    Q.z = 0.25f*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.0f-2.0f*U.y/R.y);
    float h = H;
    Q.w = _mix(Q.w,1.0f,smoothstep(2.0f*h,h,U.y));
    if(U.x<1.0f||R.x-U.x<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    if(U.y<1.0f||R.y-U.y<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    
  SetFragmentShaderComputedColor(Q);       
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void SundownFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;
    Q = T(U);
    float4 n = T(U+to_float2(0,1)),
           e = T(U+to_float2(1,0)),
           s = T(U-to_float2(0,1)),
           w = T(U-to_float2(1,0));
    swi2S(Q,x,y, swi2(Q,x,y)-0.25f*to_float2(e.z-w.z,n.z-s.z));
    swi2S(Q,x,y, swi2(Q,x,y)-0.05f*to_float2(s.w-n.w,e.w-w.w));
    Q.z = 0.25f*(n.z+e.z+s.z+w.z-n.y-e.x+s.y+w.x);
    Q.y += 1e-4*Q.w*(1.0f-2.0f*U.y/R.y);
    float h = H;
    Q.w = _mix(Q.w,1.0f,smoothstep(2.0f*h,h,U.y));
    if(U.x<1.0f||R.x-U.x<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    if(U.y<1.0f||R.y-U.y<1.0f) swi3S(Q,x,y,w,swi3(Q,x,y,w) * 0.0f);
    
    if(iFrame < 2) Q = to_float4_s(0.0f); // Sicherheitshalber
    
  SetFragmentShaderComputedColor(Q);       
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void SundownFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0)
{
   
    U -= 0.5f*R;
    U *= 0.6f/(0.8f+0.35f*U.y/R.y);
    U += 0.5f*R;
    U.y += 0.15f*R.y;
    float4 a = A(U),
           n = A(U+to_float2(0,1)),
           e = A(U+to_float2(1,0)),
           s = A(U-to_float2(0,1)),
           w = A(U-to_float2(1,0));
    float2 g = to_float2(e.w-w.w,n.w-s.w);
    float f = 0.0f;
    float2 u = U;
    for (float i = -10.0f; i < 10.0f; i+=1.0f) {
        float w = _powf(A(u).w,1.0f+0.01f*i);
        f += w/10.0f;
        u = to_float2(0.5f,0)*R+(u-to_float2(0.5f,0)*R)*(1.0f-0.005f);
    }
    Q = 0.4f+0.4f*sin_f4(1.0f-3.0f*U.y/R.y+5.0f*H/R.y+to_float4(1,2,3,4));
    Q += 2.0f*H/R.y;
    float2 d = normalize(to_float2(0.5f,0)*R-U);
    float2 p = U;
    float W = 0.0f;
    for (float i = 0.0f; i < 200.0f; i+=1.0f) {
        p += 6.0f*d;
        W += 0.01f*A(p).w;
    }
    Q -= 0.4f*W;
    float4 C = 0.8f+0.3f*sin_f4(W+1.0f/(1.0f+a.w)+2.0f-3.0f*U.y/R.y+3.0f*H/R.y+to_float4(1,2,3,4));
    Q = _mix(Q,C,f);
    //Q = to_float4_aw(0);
    { // stars
    for (float i = 1.0f;i < 5.0f; i+=1.0f)
    for (int k = 0; k < 9; k++) {
        float2 u = round(U)+to_float2(k%3,k/3)-1.0f;
        float3 h = hash(u)*2.0f-1.0f;
        float2 r = u-U+swi2(h,x,y);
        float l = 5.0f/i*length(r);
        Q += 8.0f/i/_sqrtf(i)*_expf(-1e1*f)*10.0f*(1.0f+0.5f*_sinf(iTime+6.2f*h.z))*_expf(-5e1*l)*_fmaxf(2.0f*U.y/R.y-1.0f,0.0f);
    }}
    
   SetFragmentShaderComputedColor(Q);  
}