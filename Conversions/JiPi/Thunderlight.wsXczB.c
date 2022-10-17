
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution

#define A(U) _tex2DVecN(iChannel0, ((float)((int)(U).x)+0.5f)/R.x,((float)((int)(U).y)+0.5f)/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3, (U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4, (U).x/R.x,(U).y/R.y,15)



#define Init if (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)
#define T(U) A((U)-dt*swi2(A(U),x,y))
#define NeighborhoodT float4 n = T(U+to_float2(0,1)), e = T(U+to_float2(1,0)), s = T(U-to_float2(0,1)), w = T(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define Neighborhood float4 n = A(U+to_float2(0,1)), e = A(U+to_float2(1,0)), s = A(U-to_float2(0,1)), w = A(U-to_float2(1,0)), m = 0.25f*(n+e+s+w);
#define grd 0.25f*to_float2(e.z-w.z,n.z-s.z)
#define grdw 0.25f*to_float2(e.w-w.w,n.w-s.w)
#define div 0.25f*(e.x-w.x+n.y-s.y)
#define ro(a) to_mat2(_cosf(a),_sinf(-a),_sinf(a),_cosf(a))
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return (length(p-a-(b-a)*i));
}
//Dave H
__DEVICE__ float2 hash23(float3 p3)
{
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y))*2.0f-1.0f;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


__DEVICE__ void _X (inout float4 *Q, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0) {
  float4 n = A(U+r);
  if (sg(U,swi2(n,x,y),swi2(n,z,w))<sg(U,swi2(*Q,x,y),swi2(*Q,z,w))) *Q=n;
}
__DEVICE__ void X (inout float4 *Q, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0) {
     _X(Q,U,to_float2(r,0),R,iChannel0);
     _X(Q,U,to_float2(0,r),R,iChannel0);
     _X(Q,U,to_float2(0,-r),R,iChannel0);
     _X(Q,U,to_float2(-r,0),R,iChannel0);
}

__KERNEL__ void ThunderlightFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;

    Q = A(U);
    X(&Q,U,1.0f,R,iChannel0);
    X(&Q,U,2.0f,R,iChannel0);
    X(&Q,U,3.0f,R,iChannel0);
    float b = B(swi2(Q,x,y)).z;
    float2 v = (swi2(Q,x,y)-swi2(Q,z,w))+swi2(C(swi2(Q,x,y)),x,y)+1.0f*hash23(to_float3_aw(swi2(Q,x,y),iFrame));
    for (int _x = -3; _x<=3;_x++) {
      for (int _y = -3; _y<=3; _y++) {
        float4 n = A(swi2(Q,x,y)+to_float2(_x,_y));
            float2 r = swi2(n,x,y)-swi2(Q,x,y);
            float l = length(r);
            if (l>0.0f) v -= 0.1f*r/l/l;
      }  
    }
  
  //if (Q.x<3.0f){v.x=_fabs(v.x);}
    if (Q.y<3.0f)     {v.y=_fabs(v.y);}
    if (R.x-Q.x<3.0f) {v.x=-_fabs(v.x);}
    if (R.y-Q.y<3.0f) {v.y=-_fabs(v.y);}
    if (iMouse.z>0.0f) v -= normalize(swi2(Q,x,y)-swi2(iMouse,x,y));
    v.x += 0.01f;
    if (length(v)>0.0f) v = 2.0f*normalize(v);
    Q = to_float4_f2f2(swi2(Q,x,y)+v,swi2(Q,x,y));
    
    if (_fabs(U.y-0.5f*R.y)<10.&&R.x-U.x<1.0f) {
      //swi2(Q,x,y) = to_float2(R.x,U.y);
      Q.x=R.x; Q.y=U.y;
      //swi2(Q,z,w) = swi2(Q,x,y)+to_float2(1,0);
      Q.z=Q.x+1.0f; Q.w=Q.y+0.0f;
      
    }

  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


__KERNEL__ void ThunderlightFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    Q = A(U)*0.99f;
    float4 a = B(U);
    Neighborhood;
    Q.w = m.w;
    float2 tmp = swi2(a,x,y)-swi2(a,z,w);
    Q = _mix(Q,to_float4(tmp.x,tmp.y,Q.z+1e-1,Q.w+1e-1),_expf(-1.0f*sg(U,swi2(a,x,y),swi2(a,z,w))));

    Init Q = to_float4_s(0);

  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0

__KERNEL__ void ThunderlightFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    
    Neighborhood;
    float4 b = B(U);
    swi2S(Q,x,y, 3.0f*swi2(m,x,y)+2.0f*grd);
    Q.z = div;
    Init Q = to_float4_s(0);
    
  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel2


// Fork of "Physarum testy test" by wyatt. https://shadertoy.com/view/wsXyzB
// 2020-03-24 05:18:34

__KERNEL__ void ThunderlightFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
  CONNECT_CHECKBOX0(Reset, 0);
  U+=0.5f;
    
  float4 b = B(U), a = A(U);
  Q = swi4(b,z,z,z,z);
  //Q = to_float4(1)*smoothstep(1.0f,0.0f,length(U-swi2(a,x,y)));
  
  SetFragmentShaderComputedColor(Q);    
}