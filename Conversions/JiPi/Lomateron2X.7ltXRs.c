
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
//#define Main void mainImage(out float4 Q, in float2 U)


// oneshade:
//https://www.shadertoy.com/view/7sKSRh
//float std;
__DEVICE__ float erf(in float x, float std) {
    x *= std;
    //return sign(x) * _sqrtf(1.0f - _expf(-1.239192f * x * x));
    return sign_f(x) * _sqrtf(1.0f - _exp2f(-1.787776f * x * x)); // likely faster version by @spalmer
}
__DEVICE__ float erfstep (float a, float b, float x, float std) {
    float zzzzzzzzzzzzz;
    return 0.5f*(erf(b-x,std)-erf(a-x,std));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void Lomateron2XFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f;
float AAAAAAAAAAAAAAAA;
    float std = 2.0f;
    float4 dQ = Q = to_float4_s(0);
    for (float x = -4.0f; x<=4.0f;x+=1.0f)
    for (float y = -4.0f; y<=4.0f;y+=1.0f)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float2 v = u+swi2(a,x,y);
        float w = erfstep(-0.5f,0.5f,v.x,std)*
                  erfstep(-0.5f,0.5f,v.y,std);
        swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + w*a.w*swi3(a,x,y,z));
        dQ.w   += w*a.w;
    }
    if (dQ.w>0.0f)
    {
        //dQ.xyz/=dQ.w;
        dQ.x/=dQ.w;
        dQ.y/=dQ.w;
        dQ.z/=dQ.w;
        
        Q = dQ;
    }
    
    if (iFrame < 1) {Q = to_float4(0,0,0,0.1f);}
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void Lomateron2XFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    U+=0.5f;
float BBBBBBBBBBBBBBBBB;        
    Q = A(U);
    float4 dQ = to_float4_s(0);
    for (float x = -1.0f; x<=1.0f;x+=1.0f)
    for (float y = -1.0f; y<=1.0f;y+=1.0f)
    if(x!=0.0f||y!=0.0f)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float4 b = B(U+u);
        float f = 0.125f*(a.w+b.w);
        swi2S(dQ,x,y, swi2(dQ,x,y) - f*u/dot(u,u));
    }
    Q += dQ;
    Q.y -= 0.5f/R.y;
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)         M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)  Q = to_float4_f2f2(0.1f*normalize(M-0.5f*R), to_float2(-1,3.0f));
    if (iFrame < 1)            Q = to_float4(0,0,U.x/R.x,0.2f+0.1f*_cosf(U.x));
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;
    if (R.y-U.y<1.0f)          Q.w *= 0.0f;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void Lomateron2XFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
    U+=0.5f; 
      
    float std = 1.0f;
    float4 dQ = Q = to_float4_s(0);
    for (float x = -4.0f; x<=4.0f;x+=1.0f)
    for (float y = -4.0f; y<=4.0f;y+=1.0f)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float2 v = u+swi2(a,x,y);
        float w = erfstep(-0.5f,0.5f,v.x, std)*
                  erfstep(-0.5f,0.5f,v.y, std);
        swi3S(dQ,x,y,z, swi3(dQ,x,y,z) + w*a.w*swi3(a,x,y,z));
        dQ.w   += w*a.w;
    }
    if (dQ.w>0.0f)
    {
        //dQ.xyz/=dQ.w;
        dQ.x/=dQ.w;
        dQ.y/=dQ.w;
        dQ.z/=dQ.w;
        Q = dQ;
    }
       
    if (iFrame < 1) {Q = to_float4(0,0,0,0.1f);}
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void Lomateron2XFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    U+=0.5f; 
        
    Q = A(U);
    float4 dQ = to_float4_s(0);
    for (float x = -1.0f; x<=1.0f;x+=1.0f)
    for (float y = -1.0f; y<=1.0f;y+=1.0f)
    if(x!=0.||y!=0.0f)
    {
        float2 u = to_float2(x,y);
        float4 a = A(U+u);
        float4 b = B(U+u);
        float f = 0.1f*(
        a.w*(a.w-1.0f)+b.w);
        swi2S(dQ,x,y, swi2(dQ,x,y) - f*u/dot(u,u));
    }
    Q += dQ;
    Q.y -= 0.5f/R.y;
    float2 M = 1.5f*R;
    if (iMouse.z>0.0f)           M = swi2(iMouse,x,y);
    if(length(U-M)<0.02f*R.y)    Q = to_float4_f2f2(0.1f*normalize(M-0.5f*R),to_float2(-1,3.0f));
    if (iFrame < 1)              Q = to_float4(0,0,0,0.5f+0.1f*_sinf(U.x));
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f) Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y)*=0.0f;
    if (R.y-U.y<1.0f)            Q.w *= 0.0f;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void Lomateron2XFuse(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f; 
    
    float4 a = A(U), b = B(U); 
    Q = swi4(b,w,w,w,w);
    Q += a.w*_fmaxf(cos_f4(1.7f+5.0f*a.z+to_float4(1,2,3,4)), to_float4_s(0.0f));
    Q = to_float4_s(1.0f)-Q;
    
  SetFragmentShaderComputedColor(Q);    
}