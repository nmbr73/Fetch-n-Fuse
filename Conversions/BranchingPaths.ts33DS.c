
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer D' to iChannel0


#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U).x/R.x,(U).y/R.y,15)

__DEVICE__ float4 X (inout float2 *c,inout float *m, float2 u, float2 r,float2 iResolution, __TEXTURE2D__ iChannel0 ) {
  float4 n = A(u+r);
  *m += n.z;
  if (length(u+r-swi2(n,x,y))<length(u-*c)) *c = swi2(n,x,y);
    return n;
} 
__KERNEL__ void BranchingPathsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    Q = A(U);
    float2 c = swi2(Q,x,y);
    float m=0.0f;
    float4 
        n = X(&c,&m,U,to_float2(0,1),iResolution,iChannel0),
        e = X(&c,&m,U,to_float2(1,0),iResolution,iChannel0),
        s = X(&c,&m,U,to_float2(0,-1),iResolution,iChannel0),
        w = X(&c,&m,U,to_float2(-1,0),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,-1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,-1),iResolution,iChannel0);
    float2 g = to_float2(e.z-w.z,n.z-s.z);

    //swi2(Q,x,y) = c;
    Q.x=c.x;
    Q.y=c.y;

    Q.z += (to_float4_s(m/8.0f)-Q).z+0.05f*Q.w - 0.0001f*Q.z;
    Q.w -= 0.001f*Q.w;

    float2 Qzw = _fmaxf(swi2(Q,z,w),to_float2(2,1)*smoothstep(4.0f,0.0f,length(U-c)));
    Q.z=Qzw.x;Q.w=Qzw.y;
    //swi2(Q,x,y) -= 0.25f*g;
    Q.x -= 0.25f*g.x;
    Q.y -= 0.25f*g.y;

    if (length(U-swi2(iMouse,x,y))<0.3f*R.y&&iMouse.z>0.0f) Q = to_float4(-R.x,-R.y,Q.z,0);
    if (iFrame < 1) Q = to_float4_f2f2(clamp(_floor(U/2.0f)*2.0f,0.5f*R-2.0f,0.5f*R+2.0f),to_float2_s(0.0f));


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void BranchingPathsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    Q = A(U);
    float2 c = swi2(Q,x,y);
    float m=0.0f;
    float4 
        n = X(&c,&m,U,to_float2(0,1),iResolution,iChannel0),
        e = X(&c,&m,U,to_float2(1,0),iResolution,iChannel0),
        s = X(&c,&m,U,to_float2(0,-1),iResolution,iChannel0),
        w = X(&c,&m,U,to_float2(-1,0),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,-1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,-1),iResolution,iChannel0);
    float2 g = to_float2(e.z-w.z,n.z-s.z);

    //swi2(Q,x,y) = c;
    Q.x=c.x;
    Q.y=c.y;

    Q.z += (to_float4_s(m/8.0f)-Q).z+0.05f*Q.w - 0.0001f*Q.z;
    Q.w -= 0.001f*Q.w;

    float2 Qzw = _fmaxf(swi2(Q,z,w),to_float2(2,1)*smoothstep(4.0f,0.0f,length(U-c)));
    Q.z=Qzw.x;Q.w=Qzw.y;
    //swi2(Q,x,y) -= 0.25f*g;
    Q.x -= 0.25f*g.x;
    Q.y -= 0.25f*g.y;

    if (length(U-swi2(iMouse,x,y))<0.3f*R.y&&iMouse.z>0.0f) Q = to_float4(-R.x,-R.y,Q.z,0);
    if (iFrame < 1) Q = to_float4_f2f2(clamp(_floor(U/2.0f)*2.0f,0.5f*R-2.0f,0.5f*R+2.0f),to_float2_s(0.0f));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer B' to iChannel0

__KERNEL__ void BranchingPathsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    Q = A(U);
    float2 c = swi2(Q,x,y);
    float m=0.0f;
    float4 
        n = X(&c,&m,U,to_float2(0,1),iResolution,iChannel0),
        e = X(&c,&m,U,to_float2(1,0),iResolution,iChannel0),
        s = X(&c,&m,U,to_float2(0,-1),iResolution,iChannel0),
        w = X(&c,&m,U,to_float2(-1,0),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,-1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,-1),iResolution,iChannel0);
    float2 g = to_float2(e.z-w.z,n.z-s.z);

    //swi2(Q,x,y) = c;
    Q.x=c.x;
    Q.y=c.y;

    Q.z += (to_float4_s(m/8.0f)-Q).z+0.05f*Q.w - 0.0001f*Q.z;
    Q.w -= 0.001f*Q.w;
    
    float2 Qzw = _fmaxf(swi2(Q,z,w),to_float2(2,1)*smoothstep(4.0f,0.0f,length(U-c)));
    Q.z=Qzw.x;Q.w=Qzw.y;
    //swi2(Q,x,y) -= 0.25f*g;
    Q.x -= 0.25f*g.x;
    Q.y -= 0.25f*g.y;
    
    if (length(U-swi2(iMouse,x,y))<0.3f*R.y&&iMouse.z>0.0f) Q = to_float4(-R.x,-R.y,Q.z,0);
    if (iFrame < 1) Q = to_float4_f2f2(clamp(_floor(U/2.0f)*2.0f,0.5f*R-2.0f,0.5f*R+2.0f),to_float2_s(0.0f));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer C' to iChannel0

__KERNEL__ void BranchingPathsFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{

    Q = A(U);
    float2 c = swi2(Q,x,y);
    float m=0.0f;
    float4 
        n = X(&c,&m,U,to_float2(0,1),iResolution,iChannel0),
        e = X(&c,&m,U,to_float2(1,0),iResolution,iChannel0),
        s = X(&c,&m,U,to_float2(0,-1),iResolution,iChannel0),
        w = X(&c,&m,U,to_float2(-1,0),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(1,-1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,1),iResolution,iChannel0);
    X(&c,&m,U,to_float2(-1,-1),iResolution,iChannel0);
    float2 g = to_float2(e.z-w.z,n.z-s.z);
    
    //swi2(Q,x,y) = c;
    Q.x=c.x;
    Q.y=c.y;
    
    Q.z += (to_float4_s(m/8.0f)-Q).z+0.05f*Q.w - 0.0001f*Q.z;
    Q.w -= 0.001f*Q.w;

    float2 Qzw = _fmaxf(swi2(Q,z,w),to_float2(2,1)*smoothstep(4.0f,0.0f,length(U-c)));
    Q.z=Qzw.x;Q.w=Qzw.y;
    //swi2(Q,x,y) -= 0.25f*g;
    Q.x -= 0.25f*g.x;
    Q.y -= 0.25f*g.y;    
    
    if (length(U-swi2(iMouse,x,y))<0.3f*R.y&&iMouse.z>0.0f) Q = to_float4(-R.x,-R.y,Q.z,0);
    if (iFrame < 1) Q = to_float4_f2f2(clamp(_floor(U/2.0f)*2.0f,0.5f*R-2.0f,0.5f*R+2.0f),to_float2_s(0.0f));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float4 X_Image(inout float2 *c, float2 u, float2 r,float2 iResolution, __TEXTURE2D__ iChannel0) {
  float4 n = A(u+r);
  if (length(u+r-swi2(n,x,y))<length(u-*c)) *c = swi2(n,x,y);
    return n;
} 
__KERNEL__ void BranchingPathsFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0)
{

    float4 a = A(U);
    float2 c = swi2(Q,x,y);
    float4 
        n = X_Image(&c,U,to_float2(0,1),iResolution,iChannel0),
        e = X_Image(&c,U,to_float2(1,0),iResolution,iChannel0),
        s = X_Image(&c,U,to_float2(0,-1),iResolution,iChannel0),
        w = X_Image(&c,U,to_float2(-1,0),iResolution,iChannel0),
        m = 0.25f*(n+e+s+w);
    float3 g = normalize(to_float3(e.z-w.z,n.z-s.z,0.3f));
    g = reflect(g,to_float3(0,0,1));
    float3 b = normalize(to_float3(e.w-w.w,n.w-s.w,1));
    float d = dot(g,normalize(to_float3(0,1,0.5f)));
    Q = (_expf(-4.0f*d*d))*m.w*abs_f4(sin_f4(2.0f+to_float4(1,2,3,4)*(1.0f+2.0f*m.z)));

    Q = to_float4_s(0.8f+0.2f*g.x)-0.8f*(1.0f+0.5f*(b.x+b.y)) * a.w * sin_f4(2.0f+0.5f*(g.z)*to_float4(1,2,3,4));

  SetFragmentShaderComputedColor(Q);
}