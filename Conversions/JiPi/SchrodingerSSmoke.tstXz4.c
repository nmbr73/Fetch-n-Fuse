
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)

#define dt 0.66f
#define W dt*(m-Q+Q*d.x)

#define D(a,b) _atan2f(a.x*b.y-a.y*b.x,a.x*b.x+a.y*b.y)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void SchrodingerSSmokeFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;

    Q = A(U);
    float4 
        d = B(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        a = A(U+to_float2(1,1)),
        b = A(U+to_float2(1,-1)),
        c = A(U-to_float2(1,1)),
        f = A(U-to_float2(1,-1)),
        m = 1.0f/6.0f*(n+e+s+w+0.5f*(a+b+c+f));
    swi2S(Q,x,z, swi2(Q,x,z) + swi2((W),y,w));
    
    float2 p = mul_f2_mat2(0.6f*U, to_mat2(_cosf(iTime),-_sinf(iTime),_sinf(iTime),_cosf(iTime)));
    if (iFrame < 1) Q = to_float4(0,0.5f,0,0.5f);
    if ((iFrame<1 && length(U-0.5f*R)<40.0f)||(iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<30.0f)) 
        Q=to_float4(_sinf(p.x),_cosf(p.x),_cosf(p.x),-_sinf(p.x));
  
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void SchrodingerSSmokeFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;

    Q = A(U);
    float4 
        d = B(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0)),
        a = A(U+to_float2(1,1)),
        b = A(U+to_float2(1,-1)),
        c = A(U-to_float2(1,1)),
        f = A(U-to_float2(1,-1)),
        m = 1.0f/6.0f*(n+e+s+w+0.5f*(a+b+c+f));
    swi2S(Q,y,w, swi2(Q,y,w) - swi2((W),x,z));
    float2 p = mul_f2_mat2(0.6f*U , to_mat2(_cosf(iTime),-_sinf(iTime),_sinf(iTime),_cosf(iTime)));
    if (length(Q)<1e-4||iFrame < 1)  Q = to_float4(0,0.5f,0,0.5f);
    if ((iFrame<1 && length(U-0.5f*R)<40.0f)||(iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<30.0f)) 
        Q=to_float4(_sinf(p.x),_cosf(p.x),_cosf(p.x),-_sinf(p.x));
  


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void SchrodingerSSmokeFuse__Buffer_C(float4 Q, float2 U, float2 iResolution,)
{
    U+=0.5f;
    
    float4 
        a = A(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    float xy = dot(swi2(a,x,y),swi2(a,x,y)), zw = dot(swi2(a,z,w),swi2(a,z,w));
    Q = 0.25f*to_float4 (
        D (swi2(e,x,y),swi2(w,x,y)),
        D (swi2(n,x,y),swi2(s,x,y)),
        D (swi2(e,z,w),swi2(w,z,w)),
        D (swi2(n,z,w),swi2(s,z,w))
    );
    swi2S(Q,x,y, (swi2(Q,z,w)*zw+swi2(Q,x,y)*xy)/(xy+zw));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float4 b (float2 U, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {return B(U-swi2(A(U),x,y));}

__KERNEL__ void SchrodingerSSmokeFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    U+=0.5f;
    
    Q.x = 0.25f*(b(U+to_float2(0,1),R,iChannel0,iChannel1)+b(U+to_float2(1,0),R,iChannel0,iChannel1)+b(U-to_float2(0,1),R,iChannel0,iChannel1)+b(U-to_float2(1,0),R,iChannel0,iChannel1)).x;
    float4 
        a = A(U),
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    Q.x += 0.25f*(n.y-s.y+e.x-w.x);
    
    //swi3(Q,y,z,w) = b(U,R,iChannel0,iChannel1).yzw;
    Q.y = b(U,R,iChannel0,iChannel1).y;
    Q.z = b(U,R,iChannel0,iChannel1).z;
    Q.w = b(U,R,iChannel0,iChannel1).w;
    
    float2 p = mul_f2_mat2(0.05f*U , to_mat2(_cosf(iTime),-_sinf(iTime),_sinf(iTime),_cosf(iTime)));
    
    if (iFrame < 1)  Q.y=0.0f,Q.z=0.0f,Q.w=0.0f;//swi3(Q,y,z,w) = to_float3(0);
    if ((iFrame<1 && length(U-0.5f*R)<40.0f)||(iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<30.0f)) swi3S(Q,y,z,w, abs_f3(to_float3(_sinf(p.x),_cosf(0.5f*p.x),_cosf(p.x))));
float DDDDDDDDDDDDDD;  
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void SchrodingerSSmokeFuse(float4 Q, float2 U, float2 iResolution,)
{

  Q = swi4(A(U),y,z,w,x);

  SetFragmentShaderComputedColor(Q);
}