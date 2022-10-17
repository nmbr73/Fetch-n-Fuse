
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
__DEVICE__ float4 pw (float4 t, float p) {
  return to_float4(_powf(t.x,p), _powf(t.y,p), _powf(t.z,p), 1);
}
#define ei(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
#define gauss( i, std) 0.3989422804f/(std)*_expf(-0.5f*(i)*(i)/(std)/(std))

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void GlimmerOfHopeFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float AAAAAAAAAAAAAAAAAAAAA;
    U+=0.5f;
    Q = to_float4_s(0);
    for (int _x = -1; _x<=1; _x++)
    for (int _y = -1; _y<=1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 aa = A(U+u);
        
        #define q 1.5f
        float2 w1 = clamp(U+u+swi2(aa,x,y)-0.5f*q,U - 0.5f,U + 0.5f),
               w2 = clamp(U+u+swi2(aa,x,y)+0.5f*q,U - 0.5f,U + 0.5f);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        swi2S(Q,x,y, swi2(Q,x,y) + (m*aa.z*swi2(aa,x,y)));
        Q.z += m*aa.z;
    }
    if (Q.z>0.0f)
        Q.x/=Q.z, Q.y/=Q.z;//Q.xy/=Q.z;
    if (iFrame < 1 || Reset) 
    {
        Q = to_float4(0,0,0,0);
        if (length(U/R-0.5f)<0.4f)   Q.z = 0.8f;
    }
    if (iMouse.z>0.0f && length(U-swi2(iMouse,x,y))<10.0f)  Q.x=0.5f, Q.z=0.5f;// swi2(Q,x,z) = to_float2(0.5f,0.5f);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)     Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Cubemap: Forest_0' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Texture: Lichen' to iChannel3

__KERNEL__ void GlimmerOfHopeFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(ResetB, 0); 
    U+=0.5f;
float BBBBBBBBBBBBBBBBBBBBB;    
    Q = A(U);
    float4 c = C(U), d = D(swi2(c,x,y));
    float f = -0.5f*(d.x*2.0f-1.0f)-0.5f;
    for (int x = -1; x<=1; x++)
    for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
      {
          float2 u = to_float2(x,y);
          float4 aa = A(U+u), cc = C(U+u), dd = D(swi2(cc,x,y));
          float ff = dd.x*3.0f-dd.z*2.0f;
          //swi2S(Q,x,y, swi2(Q,x,y) - 0.125f*aa.z*normalize(u)*aa.z*(ff-0.8f+aa.z));
          swi2S(Q,x,y, swi2(Q,x,y) - (0.125f*aa.z*normalize(u)*aa.z*(ff-0.8f+aa.z))); 
          
          
      }
    //swi2S(Q,x,y, swi2(Q,x,y) * 1.0f-0.005f*(d.z+d.x));
    swi2S(Q,x,y, swi2(Q,x,y) * (1.0f-0.005f*(d.z+d.x))); //!!!!!!!!!!!!!!!!!!
    
    
    Q.y += 5e-4*Q.z*(d.y*2.0f-1.0f);
    if (U.x<1.0f||U.y<1.0f||R.x-U.x<1.0f||R.y-U.y<1.0f)     Q.x*=0.0f,Q.y*=0.0f;//swi2(Q,x,y) *= 0.0f;

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void GlimmerOfHopeFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(ResetC, 0); 
    U+=0.5f;
    
    Q = to_float4_s(0);
    float w = 0.0f;
    for (int _x = -1; _x<=1; _x++)
    for (int _y = -1; _y<=1; _y++)
    {
        float2 u = to_float2(_x,_y);
        float4 aa = A(U+u), cc = C(U+u);//, dd = D(swi2(cc,x,y));
        
        #define q 1.0f
        float2 w1 = clamp(U+u+swi2(aa,x,y)-0.5f*q,U - 0.5f,U + 0.5f),
               w2 = clamp(U+u+swi2(aa,x,y)+0.5f*q,U - 0.5f,U + 0.5f);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q += m*aa.z*cc;
        w += m*aa.z;
    }
    if (w>0.0f)
      Q/=w;
    swi2S(Q,x,y, _mix(swi2(Q,x,y),U,1e-5));
    if (iFrame < 1) Q = to_float4(U.x,U.y,0,0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Lichen' to iChannel3
// Connect Buffer D 'Cubemap: Forest_0' to iChannel1
// Connect Buffer D 'Previsualization: Buffer B' to iChannel0
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2

__KERNEL__ void GlimmerOfHopeFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(ResetD, 0); 
    U+=0.5f;
    
    float4 a = A(U), c = C(U), d = D(swi2(c,x,y));
    float std = d.x+d.y-2.0f*d.z;
    
    float4
        n = A(U+to_float2(0,1)),
        e = A(U+to_float2(1,0)),
        s = A(U-to_float2(0,1)),
        w = A(U-to_float2(1,0));
    float3 norm = 
        normalize(to_float3(e.z-w.z,n.z-s.z,-0.01f*(std*std)));
    float3 ref = reflect(to_float3(0,0,1), norm);
    Q = _fminf(a.z,1.0f)*d*sqrt_f4(d);
    float4 tx = decube_f3(iChannel1,ref);
    Q *= 0.5f+0.5f*tx+0.1f*norm.x;
    Q *= exp_f4(tx);
    Q = atan_f4(Q,to_float4_s(1.0f));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void GlimmerOfHopeFuse(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    U+=0.5f;
    
    float4 b = B(U);
    Q = A(U);
    for (float j = 0.0f; j < 6.2f; j += 6.2f/3.0f)
        for (float i = 0.0f; i < 7.0f; i+=1.0f) {
            float4 a = A(U+mul_f2_mat2(to_float2(i,0),ei(j+0.1f*iTime)));
            Q += 0.5f*a*(-1.0f+exp_f4(8.0f*a))*gauss(i,3.0f);
        }
    Q = _mix(to_float4(0.7f,0.7f,0.7f,1)-swi4(b,z,z,z,z)+0.1f*Q,Q,2.0f*b.z);
    
  SetFragmentShaderComputedColor(Q);    
}