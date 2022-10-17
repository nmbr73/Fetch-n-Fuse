
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define M 0.01f*to_float4(1,2,4,8)
#define I 15.0f
#define O 0.5f*sqrt_f4(to_float4(1,2,4,8))
__DEVICE__ float4 hash (float p) // Dave (Hash)kins
{
  float4 p4 = fract_f4((p) * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
  return _floor(fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x))*10.0f)/10.0f-0.25f;
    
}
#define R iResolution
#define A(U) texture(iChannel0, (U)/R)
#define B(U) texture(iChannel1, (U)/R)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1


__DEVICE__ void swap (inout float4 *Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
  float4 n = A(U+r);
  if (_fabs(length(swi2(n,x,y)-U)-n.w)<_fabs(length(swi2(*Q,x,y)-U)-(*Q).w)) *Q = n;
}

__KERNEL__ void ColorDynamicsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
float AAAAAAAAAAAAAAAAAAA;     
    Q = A(U);
    swap(&Q,U, to_float2(0,1),R,iChannel0);
    swap(&Q,U, to_float2(1,0),R,iChannel0);
    swap(&Q,U, to_float2(0,-1),R,iChannel0);
    swap(&Q,U, to_float2(-1,0),R,iChannel0);
    swap(&Q,U, to_float2(1,1),R,iChannel0);
    swap(&Q,U, to_float2(1,-1),R,iChannel0);
    swap(&Q,U, to_float2(-1,-1),R,iChannel0);
    swap(&Q,U, to_float2(-1,1),R,iChannel0);
    swap(&Q,U, to_float2(4,4),R,iChannel0);
    swap(&Q,U, to_float2(-4,4),R,iChannel0);
    swap(&Q,U, to_float2(-4,-4),R,iChannel0);
    swap(&Q,U, to_float2(4,-4),R,iChannel0);
    float4 
        id = hash(Q.z);
    float4
        n = B(swi2(Q,x,y)+to_float2(0,1)),
        e = B(swi2(Q,x,y)+to_float2(1,0)),
        s = B(swi2(Q,x,y)-to_float2(0,1)),
        w = B(swi2(Q,x,y)-to_float2(1,0));
    float2 
        x = to_float2(e.x-w.x,n.x-s.x),
        y = to_float2(e.y-w.y,n.y-s.y),
        z = to_float2(e.z-w.z,n.z-s.z),
        a = to_float2(e.w-w.w,n.w-s.w);
    swi2S(Q,x,y, swi2(Q,x,y) - id.x*x+id.y*y+id.z*z+id.w*a);
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y)) < 10.0f) {
        float2 u = _floor(U/5.0f+0.5f)*5.0f;
        float4 n = to_float4(
                              u.x, u.y,
                              u.x+R.x*u.y,
                              1.0f+5.0f*hash(u.y+R.y*u.x).x
                            );
        if (_fabs(length(swi2(n,x,y)-U)-n.w)<_fabs(length(swi2(Q,x,y)-U)-Q.w)) Q = n;
    }
    if (iFrame < 1 || Reset) {
      float2 u = _floor(U/20.0f+0.5f)*20.0f;
      float h = hash(u.y+R.y*u.x).x;
      Q = to_float4(
                     u.x, u.y,
                     u.x+R.x*u.y,
                     1.0f+20.0f*h*h
                   );
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void ColorDynamicsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    
    Q = B(U)*0.5f;
    
    for (float i = -I; i < I; i++) {
        float2 x = U+2.0f*to_float2(i,0); 
        float4 a = A(x);
        float r = smoothstep(1.0f,0.5f,_fabs(length(swi2(a,x,y)-x)-a.w));
        Q += O*exp_f4(-M*i*i)*r*hash(a.z);
    }
    
    if (iFrame < 1 || Reset) {
      Q = to_float4_s(0);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void ColorDynamicsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame)
{
  CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    
    Q = B(U)*0.5f;
    
    for (float i = -I; i < I; i++) {
        float2 x = U+2.0f*to_float2(0,i); 
        float4 a = A(x);
        Q += exp_f4(-M*i*i)*a;
    }
    
    if (iFrame < 1 || Reset) {
      Q = to_float4_s(0);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void ColorDynamicsFuse(float4 Q, float2 U, float2 iResolution)
{
    U+=0.5f;
float IIIIIIIIIIIIIIIIIIIII;
    float4 a = A(U);
    
    
    Q = to_float4_s(0.7f) - 0.03f*B(U) + 3.0f*abs_f4(hash(a.z))*smoothstep(2.0f,0.0f,_fabs(length(U-swi2(a,x,y))-a.w));

  SetFragmentShaderComputedColor(Q);
}