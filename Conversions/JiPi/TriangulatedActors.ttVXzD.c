
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)
#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)

#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)


#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  return (length(p-a-(b-a)*i));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b); // midpoint
  if (length(a-b)<1e-3) return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


// Information Storage
__KERNEL__ void TriangulatedActorsFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  
  CONNECT_CHECKBOX0(Reset, 0); 
  CONNECT_CHECKBOX1(Textur, 0); 

  U+=0.5f;

  float2 tuv = U/R;

  Q = A(U);
    for (int _x = -3; _x <= 3; _x++)
    for (int _y = -3; _y <= 3; _y++)
      swi2S(Q,z,w, swi2(Q,z,w) + swi2(C(swi2(Q,x,y)+to_float2(_x,_y)),x,y));
    float2 v = swi2(Q,z,w);
    float V = length(v);
    v = v*_sqrtf(V*V/(1.0f+V*V));
    //if (length(v)>0.0f)v = normalize(v);
    //swi2(Q,x,y) += v;
    Q.x += v.x;
    Q.y += v.y;
    
    if (Q.x<2.0f)      Q.x=2.0f;
    if (Q.y<2.0f)      Q.y=2.0f;
    if (R.x-Q.x<2.0f)  Q.x=R.x-2.0f;
    if (R.y-Q.y<2.0f)  Q.y=R.y-2.0f;

    if (init || Reset)
    {
      U = _floor(U/5.0f+0.5f)*5.0f;
      U = clamp(U,to_float2_s(40),R-40.0f);
      if (Textur)
      {
         float4 tex = texture(iChannel4, tuv);
         Q = to_float4_s(0.0f);
         if (tex.w >= 0.0f) 
           Q = to_float4(U.x,U.y,0,0);
      }
      else
         Q = to_float4(U.x,U.y,0,0);
    }
    
  
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// SORT TRIANGULATION
__DEVICE__ void X (inout float4 *Q, inout float4 *a, inout float4 *aa, float2 U, float2 r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float4 b = B(U+r);
    float4 n = A(swi2(b,x,y));
    float ln = length(swi2(n,x,y)-U), la = length(swi2(*a,x,y)-U);
    if (ln<=la) {
        //swi2(Q,x,y) = swi2(b,x,y);
        (*Q).x = b.x;
        (*Q).y = b.y;
        //swi2(a,x,y) = swi2(n,x,y);
        (*a).x = n.x;
        (*a).y = n.y;
    }
  
    float pn = pie(U,swi2(*a,x,y),swi2(n,x,y)), 
          pa = pie(U,swi2(*a,x,y),swi2(*aa,x,y));
    if (pn<=pa){
        *aa = n;
        //swi2(Q,z,w) = swi2(b,x,y);
        (*Q).z = b.x;
        (*Q).w = b.y;
    }
    n = A(swi2(b,z,w));
    ln = length(swi2(n,x,y)-U);
    if (ln<la) {
        //swi2(Q,x,y) = swi2(b,z,w);
        (*Q).x = b.z;
        (*Q).y = b.w;
        //swi2(a,x,y) = swi2(n,x,y);
        (*a).x = n.x;
        (*a).y = n.y;
    }
    pn = pie(U,swi2(*a,x,y),swi2(n,x,y));
    if (pn<pa){
        *aa = n;
        //swi2(Q,z,w) = swi2(b,z,w);
        (*Q).z = b.z;
        (*Q).w = b.w;
    }
}

__DEVICE__ void Xr (inout float4 *Q, inout float4 *a, inout float4 *aa, float2 U, float r, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
     X(Q,a,aa,U,to_float2(r,0),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(0,r),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(0,-r),R,iChannel0,iChannel1);
     X(Q,a,aa,U,to_float2(-r,0),R,iChannel0,iChannel1);
}


__KERNEL__ void TriangulatedActorsFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(Reset, 0); 
  CONNECT_CHECKBOX1(Textur, 0); 

  U+=0.5f;
  Q = B(U);
    float4 a = A(swi2(Q,x,y)), aa= A(swi2(Q,z,w));
    Xr(&Q,&a,&aa,U,1.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,2.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,3.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,4.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,5.0f,R,iChannel0,iChannel1);
    Xr(&Q,&a,&aa,U,6.0f,R,iChannel0,iChannel1);

    if (init || Reset)
    {
      
      if (Textur)
      {
         float4 tex = texture(iChannel3, U/R);
         if (tex.w >= 0.0f) 
           Q.x=U.x, Q.y=U.y; //Q = to_float4(U.x,U.y, 0, 0);
      }
      else
        //swi2(Q,x,y) = U;
        Q.x=U.x, Q.y=U.y;
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// COMPUTE FORCE FIELD
__KERNEL__ void TriangulatedActorsFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  CONNECT_CHECKBOX0(Reset, 0); 
  CONNECT_CHECKBOX1(Textur, 0); 

  U+=0.5f;
  float4 b = B(U);
  float4 a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));
  float2 r = swi2(a,x,y)-swi2(aa,x,y);
  float l = length(r);
  float2 v = to_float2_s(0);
  float f = 0.0f;
    if (length(swi2(a,x,y)-swi2(aa,x,y))>0.&&length(swi2(aa,z,w)-swi2(a,z,w))>0.0f) f = _fabs(dot(normalize(swi2(a,x,y)-swi2(aa,x,y)),normalize(swi2((aa-a),z,w))));
    if (l>0.0f && length(swi2(a,x,y)-swi2(aa,x,y))>4.42f) 
      v = -1.0f*r/l/l+
          50.0f*f*swi2((aa-a),z,w)/_fmaxf(0.1f,l*l*l*l);

    if (a.x<20.0f)     v.x += 0.02f;
    if (R.x-a.x<20.0f) v.x -= 0.02f;
    if (a.y<20.0f)     v.y += 0.02f;
    if (R.y-a.y<20.0f) v.y -= 0.02f;
    //swi2(Q,x,y) = 3e-2*v;
    Q.x = 3e-2*v.x;
    Q.y = 3e-2*v.y;
    if (iMouse.z>0.0f && length(swi2(a,x,y)-swi2(iMouse,x,y))>0.0f) swi2S(Q,x,y, swi2(Q,x,y) + clamp(0.03f*(swi2(a,x,y)-swi2(iMouse,x,y))/dot((swi2(a,x,y)-swi2(iMouse,x,y)),(swi2(a,x,y)-swi2(iMouse,x,y))),-2e-4,2e-4));
    if (iFrame < 10 || Reset)          
    {
      if (Textur)
      {
         float4 tex = texture(iChannel3, U/R);
         if (tex.w >= 0.0f) 
           swi2S(Q,x,y, swi2(Q,x,y) + 3e-2*(U-0.5f*R)/dot(U-0.5f*R,U-0.5f*R)) //Q.x=U.x, Q.y=U.y; //Q = to_float4(U.x,U.y, 0, 0);
      }
      else
         swi2S(Q,x,y, swi2(Q,x,y) + 3e-2*(U-0.5f*R)/dot(U-0.5f*R,U-0.5f*R));
    }
  
  SetFragmentShaderComputedColor(Q);  
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// Fork of "Triangulated Actors*" by wyatt. https://shadertoy.com/view/ttdXDB
// 2020-02-22 04:42:08

__KERNEL__ void TriangulatedActorsFuse(float4 Q, float2 U, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    U+=0.5f;
    float4 b = B(U),
    a = A(swi2(b,x,y)), aa = A(swi2(b,z,w));

    float2 bb = _floor(swi2(b,x,y)/10.0f+0.5f)*10.0f;
    bb = clamp(bb,to_float2_s(30),R-30.0f)/R;
    float o = length(U-swi2(a,x,y)),
          w = sg(U,swi2(a,x,y),swi2(aa,x,y)),
          v = pie(U,swi2(a,x,y),swi2(aa,x,y));
    Q = to_float4_s(0.7f)-to_float4_s(_expf(-w-0.1f*(v+1.0f)*length(swi2(aa,x,y)-swi2(a,x,y))));
    
  SetFragmentShaderComputedColor(Q);    
}