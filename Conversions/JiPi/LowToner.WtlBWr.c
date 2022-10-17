
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define E(U) texture(iChannel4,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)

#define r 1.3f

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void LowTonerFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    if (iFrame%2<1) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*r,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*r,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
            Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w;//Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = to_float4(0,0,0.1f,0);
            if (length(U-to_float2_s(0.5f)*R)<0.3f*R.y)Q.w = 0.8f;
        }
        if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<20.0f) Q.w = 0.3f;
        if (U.x<2.0f||U.y<2.0f||R.x-U.x<2.0f||R.y-U.y<2.0f) Q *= 0.0f;
    } else {
    Q = A(U);float4 q = Q;
    for (int x = -1; x<=1; x++)
    for (int y = -1; y<=1; y++)
    if (x!=0||y!=0)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u), b = B(U+u), c = C(U+u), d = E(U+u);//d = D(U+u);
        u = (u)/dot(u,u);
        swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*(
            a.w*(a.w*a.z-0.8f-0.1f*b.w-0.1f*c.w)+
            -d.x
            )*u);
      Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
    }
    if (Q.w < 1e-3) Q.z *= 0.0f;
    //swi2(Q,x,y) *= 0.999f;
    Q.x *= 0.999f;
    Q.y *= 0.999f;
    }
    
  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: London' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2

__KERNEL__ void LowTonerFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    if (iFrame%2<1) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*r,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*r,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
            Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w;//Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = to_float4(0,0,0.1f,0);
            if (length(U-to_float2_s(0.5f)*R)<0.3f*R.y)     Q.w = 0.8f;
        }
        if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<20.0f)  Q.w = 0.3f;
        if (U.x<2.0f||U.y<2.0f||R.x-U.x<2.0f||R.y-U.y<2.0f) Q *= 0.0f;
    } else {
        Q = A(U);float4 q = Q;
        for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++)
        if (x!=0||y!=0)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u), b = B(U+u), c = C(U+u), d = E(U+u);// d = D(U+u);
            u = (u)/dot(u,u);
            swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*(
                  a.w*(a.w*a.z-0.8f-0.1f*b.w-0.1f*c.w)+
                  -d.y
                  )*u);
          Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
        }
        if (Q.w < 1e-3) Q.z *= 0.0f;
        //swi2(Q,x,y) *= 0.999f;
        Q.x *= 0.999f;
        Q.y *= 0.999f;
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: London' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel2
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void LowTonerFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    if (iFrame%2<1) {
        Q = to_float4_s(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u);
            float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*r,U - 0.5f,U + 0.5f),
                 w2 = clamp(U+u+swi2(a,x,y)+0.5f*r,U - 0.5f,U + 0.5f);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(r*r);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
            Q.w += m*a.w;
        }
        if (Q.w>0.0f)
            Q.x/=Q.w,Q.y/=Q.w,Q.z/=Q.w;//Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = to_float4(0,0,0.1f,0);
            if (length(U-to_float2_s(0.5f)*R)<0.3f*R.y)Q.w = 0.8f;
        }
        if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<20.0f) Q.w = 0.3f;
        if (U.x<2.0f||U.y<2.0f||R.x-U.x<2.0f||R.y-U.y<2.0f) Q *= 0.0f;
    } else {
        Q = A(U);float4 q = Q;
        for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++)
        if (x!=0||y!=0)
        {
            float2 u = to_float2(x,y);
            float4 a = A(U+u), b = B(U+u), c = C(U+u), d = E(U+u);//d = D(U+u);
            u = (u)/dot(u,u);
float zzzzzzzzzzzzzzzzzzzzzz;            
            swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*(
                  a.w*(a.w*a.z-0.8f-0.1f*b.w-0.1f*c.w)+
                  -d.z
                  )*u);
          Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
        }
        if (Q.w < 1e-3) Q.z *= 0.0f;
        //swi2(Q,x,y) *= 0.999f;
        Q.x *= 0.999f;
        Q.y *= 0.999f;
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Cubemap: Forest_0' to iChannel3
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2


// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

__KERNEL__ void LowTonerFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;

    float4
        n = A(U+to_float2(0,1))+B(U+to_float2(0,1))+C(U+to_float2(0,1)),
        e = A(U+to_float2(1,0))+B(U+to_float2(1,0))+C(U+to_float2(1,0)),
        s = A(U-to_float2(0,1))+B(U-to_float2(0,1))+C(U-to_float2(0,1)),
        w = A(U-to_float2(1,0))+B(U-to_float2(1,0))+C(U-to_float2(1,0));
    float3 norm = 
        normalize(to_float3(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w,10)),
        ref = reflect(to_float3(0,0,-1),norm);
   
    float4 a = A(U), b = B(U), c = C(U);
    Q = to_float4(a.w,b.w,c.w,1);
    Q.w = length(swi3(decube_f3(iChannel3,ref),x,y,z));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "4-Substance" by wyatt. https://shadertoy.com/view/3lffzM
// 2020-08-03 18:26:39

// Fork of "Multi-Substance" by wyatt. https://shadertoy.com/view/WtffRM
// 2020-08-01 02:57:11

__KERNEL__ void LowTonerFuse(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;

    Q = 1.2f*D(U);
    
  SetFragmentShaderComputedColor(Q);    
}