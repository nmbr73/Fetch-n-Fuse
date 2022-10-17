
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

__DEVICE__ float ln (float2 p, float2 a, float2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float ln (float3 p, float3 a, float3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Apply forces
__DEVICE__ float2 F (float2 u, float2 fragCoord, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3) {
    float4 a = A(fragCoord + u);
    float4 d = D(fragCoord + u);
  return 0.1f*(d.x+0.1f*a.w*(0.9f+0.1f*_fabs(a.w-1.0f)))*u/dot(u,u);
}

__KERNEL__ void SandWaterFuse__Buffer_A(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;

    Q = A(U);
    float4 d = D(U);
    for (int _x = -1; _x <= 1; _x++)
    for (int _y = -1; _y <= 1; _y++)
    if (_x!=0||_y!=0)
       swi2S(Q,x,y, swi2(Q,x,y) - Q.w*F(to_float2(_x,_y),U,R,iChannel0,iChannel1));
    Q.w += 1e-3;
    if (length(swi2(Q,x,y))>0.5f)  swi2S(Q,x,y, 0.5f*normalize(swi2(Q,x,y)));
    if (iFrame < 1 || Reset) {
        Q = to_float4(0,0,0,0);
        if (U.y<0.5f*R.y-0.5f*_fabs(U.x-0.5f*R.x))    Q.w = 1.0f;
        if (length(U-0.5f*R)<0.25f*R.y)               Q.z = 0.1f, Q.w = 1.0f; //swi2(Q,z,w) = to_float2(0.1f,1);
    }
    if (iMouse.z>0.&&length(U-swi2(iMouse,x,y))<5.0f)
        Q = to_float4(1,0,6,1);
    if (U.x<2.0f||U.y<2.0f||R.x-U.x<2.0f||R.y-U.y<2.0f){
        Q.w*=0.9f;
        swi2S(Q,x,y, normalize(U-0.5f*R));
    }
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Conservative advect alternating x and y direction
__KERNEL__ void SandWaterFuse__Buffer_B(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
 
    Q = to_float4_s(0);
    if (iFrame%2==0)
    for (float i = -1.0f; i <= 1.0f; i++)
    {
        float2 u = to_float2(0,i);
        float4 a = A(U+u-to_float2(0,0.5f)),
             b = A(U+u),
             c = A(U+u+to_float2(0,0.5f));
        float w1 = 1.0f+c.y-a.y;
        if (w1>0.0f) {
            float w = clamp(u.y+0.5f+c.y,-0.5f,0.5f)-
                      clamp(u.y-0.5f+a.y,-0.5f,0.5f);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + b.w*w/w1*swi3(b,x,y,z));
            Q.w += b.w*w/w1;
        }
    } else 
  for (float i = -1.0f; i <= 1.0f; i++)
    {
      float2 u = to_float2(i,0);
        float4 a = A(U+u-to_float2(0.5f,0)),
             b = A(U+u),
             c = A(U+u+to_float2(0.5f,0));
        float w1 = 1.0f+c.x-a.x;
        if (w1 > 0.0f) {
            float w = clamp(u.x+0.5f+c.x,-0.5f,0.5f)-
                      clamp(u.x-0.5f+a.x,-0.5f,0.5f);
            swi3S(Q,x,y,z, swi3(Q,x,y,z) + b.w*w/w1*swi3(b,x,y,z));
            Q.w += b.w*w/w1;
        }
    }
    if (Q.w > 0.0f)  Q.x/=Q.w, Q.y/=Q.w, Q.z/=Q.w;//swi3(Q,x,y,z) /= Q.w;
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// density-dependent diffusion
__DEVICE__ float4 X (float4 Q, float2 u, float2 fragCoord,float2 R, __TEXTURE2D__ iChannel0) {
    float4 a = A(fragCoord + u);
    float f = Q.w-a.w;
    return _mix(Q,a,clamp(10.0f*f*f,0.0f,1.0f));
}
__KERNEL__ void SandWaterFuse__Buffer_C(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
  
    Q = A(U);
    Q = 
        0.125f*X(Q,to_float2(1,0),U,R,iChannel0)+
        0.125f*X(Q,to_float2(0,1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,0),U,R,iChannel0)+
        0.125f*X(Q,to_float2(0,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(1,1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(1,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,-1),U,R,iChannel0)+
        0.125f*X(Q,to_float2(-1,1),U,R,iChannel0);
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Texture: Pebbles' to iChannel2
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void SandWaterFuse__Buffer_D(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;

    Q = D(U);
    float4 a = A(U);
    Q += 1e-5*(1.0f+0.1f*_expf(-length(U-0.5f*R)/R.y));
    Q -= 1e-3*a.w*Q.w*(length(swi2(a,x,y))-0.1f*a.w);
    if (iFrame < 30 || Reset)  Q = to_float4_s(1)+0.05f*C(U).x+_expf(-length(U-0.5f*R)/R.y);
    
  SetFragmentShaderComputedColor(Q);        
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void SandWaterFuse(float4 Q, float2 U, float iTime, int iFrame, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_COLOR0(Color1, 1.0f, 0.9f, 0.8f, 1.0f);
    CONNECT_COLOR1(Color2, 0.0f, 0.03f, 0.1f, 0.0f);
    CONNECT_SLIDER0(Brightness, 0.0f, 1.0f, 0.5f);
    CONNECT_SLIDER1(Alpha, 0.0f, 1.0f, 1.0f);
    U+=0.5f;

    float4 a = A(U), b = D(U);
    //Q = 0.5f*b*to_float4(1,0.9f,0.8f,1) + to_float4(0,0.03f*a.w,0.1f*a.w,0);
    Q = Brightness*b*Color1 + to_float4(Color2.x,Color2.y*a.w,Color2.z*a.w, Color2.w);
        
    Q.w = Alpha;    
    
  SetFragmentShaderComputedColor(Q);    
}