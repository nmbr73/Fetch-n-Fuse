
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void MultiSubstanceFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
  
  CONNECT_CHECKBOX0(Reset, 0);
  U+=0.5f;
float AAAAAAAAAAAAAAAAAA;  
  Q = to_float4_s(0);
  for (int x = -1; x <= 1; x++)
  for (int y = -1; y <= 1; y++)
    {
      float2 u = to_float2(x,y);
      float4 a = A(U+u);
      #define _q 1.125f
      float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*_q,U - 0.5f,U + 0.5f),
             w2 = clamp(U+u+swi2(a,x,y)+0.5f*_q,U - 0.5f,U + 0.5f);
      float m = (w2.x-w1.x)*(w2.y-w1.y)/(_q*_q);
      swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
      Q.w += m*a.w;
    }
    if (Q.w>0.0f)
       //Q.xyz/=Q.w;
       Q.x/=Q.w,
       Q.y/=Q.w,
       Q.z/=Q.w;
       
    if (iFrame < 1 || Reset) 
    {
        Q = to_float4(0,0,0.1f,0);
        if (length(U/R-0.3f)<0.2f)  Q.w = 1.0f;
    }
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f) Q.x=0.5f, Q.w=0.5f; // swi2(Q,x,w) = to_float2(0.5f,0.5f);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)  Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    
  SetFragmentShaderComputedColor(Q); 
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void MultiSubstanceFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
  U+=0.5f;
float BBBBBBBBBBBBBBBBBB;  
  Q = to_float4_s(0);
  for (int x = -1; x <= 1; x++)
  for (int y = -1; y <= 1; y++)
    {
      float2 u = to_float2(x,y);
      float4 a = A(U+u);
      #define _q 1.125f
      float2 w1 = clamp(U+u+swi2(a,x,y)-0.5f*_q,U - 0.5f,U + 0.5f),
             w2 = clamp(U+u+swi2(a,x,y)+0.5f*_q,U - 0.5f,U + 0.5f);
      float m = (w2.x-w1.x)*(w2.y-w1.y)/(_q*_q);
      swi3S(Q,x,y,z, swi3(Q,x,y,z) + m*a.w*swi3(a,x,y,z));
      Q.w += m*a.w;
    }
    if (Q.w>0.0f)
      //Q.xyz/=Q.w;
       Q.x/=Q.w,
       Q.y/=Q.w,
       Q.z/=Q.w;
    
    
    if (iFrame < 1) 
    {
      Q = to_float4(0,0,0.1f,0);
      if (length(U/R-0.7f)<0.2f)Q.w = 1.0f;
    }
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<10.0f)  Q.x=0.5f, Q.w=0.5f; //swi2(Q,x,w) = to_float2(0.5f,0.5f);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)   Q.x*=0.0f, Q.y*=0.0f; //swi2(Q,x,y) *= 0.0f;
        
  SetFragmentShaderComputedColor(Q); 
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void MultiSubstanceFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
  U+=0.5f;
float CCCCCCCCCCCCCCCCCC;  
  Q = A(U); 
  float4 q = Q;
  for (int x = -1; x<=1; x++)
  for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u), b = B(U+u);
        u = (u)/dot(u,u);
        swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*a.w*(a.w*a.z-1.0f)*u);
        swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*b.w*(b.w*b.z+1.0f)*u);
        Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
    }
    if (Q.w < 1e-3) Q.z *= 0.0f;
    Q.y -= 5e-4*Q.w;
    //swi2(Q,x,y) *= 0.999f;
    Q.x *= 0.999f;
    Q.y *= 0.999f;
  
  SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel1
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void MultiSubstanceFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame)
{
  U+=0.5f;
float DDDDDDDDDDDDDDD;  
  Q = A(U);float4 q = Q;
  for (int x = -1; x<=1; x++)
  for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u), b = B(U+u);
        u = (u)/dot(u,u);
        swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*a.w*(a.w*a.z-1.0f)*u);
        swi2S(Q,x,y, swi2(Q,x,y) - q.w*0.125f*b.w*(b.w*b.z+1.0f)*u);
        Q.z -= q.w*0.125f*a.w*(dot(u,swi2(a,x,y)-swi2(q,x,y)));
    }
    if (Q.w < 1e-3) Q.z *= 0.0f;
    Q.y -= 5e-4*Q.w;
    //swi2(Q,x,y) *= 0.999f;
    Q.x *= 0.999f;
    Q.y *= 0.999f;
  
  SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel2
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void MultiSubstanceFuse(float4 Q, float2 U, float2 iResolution, int iFrame)
{
  U+=0.5f;
    float4
        n = A(U+to_float2(0,1))+B(U+to_float2(0,1)),
        e = A(U+to_float2(1,0))+B(U+to_float2(1,0)),
        s = A(U-to_float2(0,1))+B(U-to_float2(0,1)),
        w = A(U-to_float2(1,0))+B(U-to_float2(1,0));
    float3 norm = 
        normalize(to_float3(e.z*e.w-w.z*w.w,n.z*n.w-s.z*s.w,3)),
        ref = reflect(to_float3(0,0,-1),norm);
   
  float4 a = A(U), b = B(U);
  Q = 1.2f*(a.w+b.w)*sin_f4(-2.1f+3.0f*a.w*a.z+(b.w*b.z+0.4f)*to_float4(1,2,3,4));
  Q += 0.6f*(Q+0.4f)*C(U+40.0f*(a.w+b.w)*swi2(norm,x,y));
  Q *= 0.8f+decube_f3(iChannel3,ref);
  
  SetFragmentShaderComputedColor(Q);   
}