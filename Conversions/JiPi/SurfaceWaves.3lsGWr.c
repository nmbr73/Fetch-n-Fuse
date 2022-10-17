
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// fluid pressure and velocity
#define R iResolution
__DEVICE__ float4 t (float2 U,float2 R,__TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 A (float2 U,float2 R,__TEXTURE2D__ iChannel0) {return t(U-swi2(t(U,R,iChannel0),x,y),R,iChannel0);}
__DEVICE__ float4 B (float2 U,float2 R,__TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__DEVICE__ float4 C (float2 U,float2 R,__TEXTURE2D__ iChannel2) {return texture(iChannel2,U/R);}
__DEVICE__ float ln (float2 p, float2 a, float2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));}

__KERNEL__ void SurfaceWavesFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    U+=0.5f;
    float4 
        b = B(U,R,iChannel1),
        c = C(U,R,iChannel2),
        me = Q = A(U,R,iChannel0),
        n = A(U+to_float2(0,1),R,iChannel0),
        e = A(U+to_float2(1,0),R,iChannel0),
        s = A(U-to_float2(0,1),R,iChannel0),
        w = A(U-to_float2(1,0),R,iChannel0);
    // navier stokes
    Q.x -= 0.25f*(e.w-w.w+c.x);
    Q.y -= 0.25f*(n.w-s.w+c.y);
    Q.z += 0.125f*(me.w-b.x); // vertical velocity feeds back with the height
    Q.w = 0.25f*((n.w+e.w+s.w+w.w)-me.z-b.y-(n.y-s.y+e.x-w.x)); // pressure calculation accounting for the vertical force
    // this part is pivotal but I don't remember why I put it there...
    //swi3(Q,x,y,z) *= _fminf(1.0f,b.x);
    Q.x *= _fminf(1.0f,b.x);
    Q.y *= _fminf(1.0f,b.x);
    Q.z *= _fminf(1.0f,b.x);
    
    //boundaries
    if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)  Q.x *= 0.0f, Q.y *= 0.0f, Q.z *= 0.0f;//swi3(Q,x,y,z) *= 0.0f;
    if (iFrame < 1)                             {float tmp = (0.1f*smoothstep(50.0f,45.0f,length(U-0.5f*R))); Q = to_float4(tmp,tmp,0,1);}
    float4 mo = texture(iChannel3,U/R);
    float l = ln(U,swi2(mo,x,y),swi2(mo,z,w));
    if (length(swi2(mo,x,y)-swi2(mo,z,w))>0.0f) swi2S(Q,x,y, swi2(Q,x,y) + 2.0f*_expf(-0.05f*l*l)*normalize(swi2(mo,x,y)-swi2(mo,z,w)));
    if (length(swi2(Q,x,y))>0.0f)               swi2S(Q,x,y, normalize(swi2(Q,x,y))*_fminf(0.8f,length(swi2(Q,x,y))));

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// fluid height
#define R iResolution
__DEVICE__ float4 AB (float2 U,float2 R,__TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 BB (float2 U,float2 R,__TEXTURE2D__ iChannel0,__TEXTURE2D__ iChannel1) {return texture(iChannel1,(U-swi2(AB(U,R,iChannel0),x,y))/R);}

__KERNEL__ void SurfaceWavesFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  U+=0.5f;
  Q = BB(U,R,iChannel0,iChannel1);
    float4 a = AB(U,R,iChannel0),
        n = BB(U+to_float2(0,1),R,iChannel0,iChannel1),
        e = BB(U+to_float2(1,0),R,iChannel0,iChannel1),
        s = BB(U-to_float2(0,1),R,iChannel0,iChannel1),
        w = BB(U-to_float2(1,0),R,iChannel0,iChannel1),
        m = 0.25f*(n+e+s+w);
    // basically the schrodinger equation 
    Q.y = m.x-Q.x;
    Q.x += a.z;
    // gradient for making the caustic in Image
    //swi2(Q,z,w) = to_float2(e.x-w.x,n.x-s.x);
    Q.z = e.x-w.x;
    Q.w = n.x-s.x;
    
    // boundaries
    if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f) Q.x = 1.0f;
    if (iFrame < 1)                                     Q = to_float4(1,0,0,0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// fluid vorticity
#define R iResolution
//__DEVICE__ float4 t (float2 U) {return texture(iChannel0,U/R);} //BufferA
__DEVICE__ float4 c (float2 U,float2 R,__TEXTURE2D__ iChannel2) {return texture(iChannel2,U/R);}
__DEVICE__ float4 CC (float2 U,float2 R,__TEXTURE2D__ iChannel0,__TEXTURE2D__ iChannel2) {return c(U-swi2(t(U,R,iChannel0),x,y),R,iChannel2);}
//__DEVICE__ float ln (float2 p, float2 a, float2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));}
__KERNEL__ void SurfaceWavesFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    U+=0.5f;
    float4 
        c = CC(U,R,iChannel0,iChannel2),
        n = t(U+to_float2(0,1),R,iChannel0),
        e = t(U+to_float2(1,0),R,iChannel0),
        s = t(U-to_float2(0,1),R,iChannel0),
        w = t(U-to_float2(1,0),R,iChannel0);
    // curl
     Q.z = 0.25f*(n.x-s.x+e.y-w.y);
        n = CC(U+to_float2(0,1),R,iChannel0,iChannel2);
        e = CC(U+to_float2(1,0),R,iChannel0,iChannel2);
        s = CC(U-to_float2(0,1),R,iChannel0,iChannel2);
        w = CC(U-to_float2(1,0),R,iChannel0,iChannel2);
    // magnus force
    //swi2(Q,x,y) = c.z*to_float2(s.z-n.z,w.z-e.z);
    Q.x = c.z*(s.z-n.z);
    Q.y = c.z*(s.z-n.z);
    
    if (iFrame < 1) Q = to_float4_s(0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


//Mouse
__KERNEL__ void SurfaceWavesFuse__Buffer_D(float4 Col, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  U+=0.5f;
  float4 p = texture(iChannel0,U/iResolution);
  if (iMouse.z>0.0f) {
    if (p.z>0.0f) Col =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
    else Col =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
  }
    else Col = to_float4_f2f2(-iResolution,-iResolution);


  SetFragmentShaderComputedColor(Col);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//calculate 5x5 caustic
#define R iResolution
__DEVICE__ float4 AI (float2 U,float2 R,__TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 BI (float2 U,float2 R,__TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}

__KERNEL__ void SurfaceWavesFuse(float4 Q, float2 U, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    U+=0.5f;
    Q = to_float4_s(0);
    for (int _x = -2; _x <= 2; _x++)
    for (int _y = -2; _y <= 2; _y++) {
       float3 u = normalize(to_float3(_x,_y,1));
       float2 b = swi2(BI(U+swi2(u,x,y),R,iChannel1),z,w);
       Q.x += dot(u,normalize(to_float3_aw(b,0.005f)));
       Q.y += dot(u,normalize(to_float3_aw(b,0.010f)));
       Q.z += dot(u,normalize(to_float3_aw(b,0.015f)));
    }
    Q = Q/20.0f;
    Q.w=Alpha;

  SetFragmentShaderComputedColor(Q);
}