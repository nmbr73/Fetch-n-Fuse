
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
//#define B(U) texelFetch(iChannel1,to_int2(U),0)
//#define C(U) texelFetch(iChannel2,to_int2(U),0)
//#define D(U) texelFetch(iChannel3,to_int2(U),0)
#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define B(U) texture(iChannel1,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define C(U) texture(iChannel2,(make_float2(to_int2_cfloat(U))+0.5f)/R)
#define D(U) texture(iChannel3,(make_float2(to_int2_cfloat(U))+0.5f)/R)


#define Init if (iFrame < 1) 
#define Border if (U.x<1.0f||R.x-U.x<1.0f||U.y<1.0f||R.y-U.y<1.0f)
#define O 6.0f
#define shape(U) (round((U)/O)*O)
#define N 12
#define Z to_float2(u)

__DEVICE__ float angle (float2 a, float2 b) {
  return _atan2f(a.x*b.y-a.y*b.x,dot(swi2(a,x,y),swi2(b,x,y)));
}
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
    if (length(a-b)<1e-4||length(a)<1e-4||length(b)<1e-4) return 1e9;
    
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
    float l = (length(p-a-(b-a)*i));
    return l;
}

__DEVICE__ float pie (float2 p, float2 a, float2 b) {
  float2 m = 0.5f*(a+b); // midpoint
  if (length(a-b)<1e-3) return 1e3; // ignore self
  return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
__DEVICE__ float line (float2 p, float2 a, float2 b) {
  if (length(a-b)<1.0f||length(a)<1.0f||length(b)<1.0f) return 1e9;
    
  float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  float l = (length(p-a-(b-a)*i));
  l=(pie(p,a,b));
  return l;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float2 force (float2 U, float4 a, float2 bb, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3) {
    float4 b = A(bb);
    if (b.x<5.0f && b.y<5.0f) return to_float2_s(0);
    
    float2 r = swi2(b,x,y)-swi2(a,x,y), v = swi2(b,z,w)-swi2(a,z,w),
         q = swi2(D(U),x,y)-swi2(D(bb),x,y);
    float l = length(r), j = length(q);
    if (l < 1e-9) return to_float2_s(0);
    float2 f = to_float2_s(0);
    if (l!=0.0f && j!=0.0f){
float zzzzzzzzzzzzzzzz;      
    f += -10.0f*(U.x/R.x-0.5f)*(bb.x/R.x-0.5f)*(2.0f+dot(swi2(a,z,w),swi2(b,z,w)))*r/_fmaxf(1.0f,l*l*l);
    f += 10.0f*r/_fmaxf(10.0f,l*l*l*l);
    f -= (100.0f)*r/_fmaxf(10.0f,l*l*l*l*l);
    }
    if (length(f)>1.0f) return normalize(f);
    return f;
}

__KERNEL__ void GraphNetworkFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 

    U+=0.5f;

    float2 u = shape(U);
    if (u.x+O>=R.x||u.y+O>=R.y||u.x<O||u.y<O||length(U-u)>1.0f) {  SetFragmentShaderComputedColor(Q); return; } //discard;}
    Q = A(u);
    float2 f = to_float2_s(0);
    for (int x=-2;x<2; x++) {
        for (int y=-2;y<2; y++) {
            float4 b = B(u+to_float2(x,y));
            f += (
                force(U,Q,swi2(b,x,y),R,iChannel0,iChannel3)+
                force(U,Q,swi2(b,z,w),R,iChannel0,iChannel3));
        }
    }
    f /= 16.0f;
    swi2S(Q,z,w, swi2(Q,z,w) + f-0.001f*swi2(Q,z,w));
    Q.w -= 0.002f;
    swi2S(Q,x,y, swi2(Q,x,y) + f+swi2(Q,z,w)*_sqrtf(1.0f/(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w)))));
    if (length(swi2(Q,z,w))>5.0f)    swi2S(Q,z,w, 5.0f*normalize(swi2(Q,z,w)));
    if (iMouse.z>0.0f)               swi2S(Q,z,w, swi2(Q,z,w) + 10.0f*(swi2(Q,x,y)-swi2(iMouse,x,y))/dot(swi2(Q,x,y)-swi2(iMouse,x,y),swi2(Q,x,y)-swi2(iMouse,x,y)));
    if (Q.x<5.0f)                   {Q.x=5.0f;Q.z = +_fabs(Q.z);}
    if (R.x-Q.x<5.0f)               {Q.x=R.x-5.0f;Q.z =-_fabs(Q.z);}
    if (Q.y<5.0f)                   {Q.y=5.0f;Q.w = +_fabs(Q.w);}
    //if (R.y-Q.y<5.0f) {Q.y=R.y-5.0f;Q.w =-_fabs(Q.w);}
    if (iFrame < 50) {
      Q = to_float4(u.x+0.5f*O,u.y+0.5f*O,0,0);
    }
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


__DEVICE__ void X (inout float4 *Q, inout float2 *r, float4 a, float2 U, float2 u, float4 n, float2 R, __TEXTURE2D__ iChannel0) {
    float l = line(swi2(a,x,y)-U+u,swi2(a,x,y),swi2(A(swi2(n,x,y)),x,y)),
         ll = line(swi2(a,x,y)-U+u,swi2(a,x,y),swi2(A(swi2(n,z,w)),x,y));
    if (l<(*r).x){
        *r = to_float2(l,(*r).x);
        *Q = to_float4(n.x,n.y,(*Q).x,(*Q).y);
    } else if (l<(*r).y) {
      (*r).y = l;
      //swi2(*Q,z,w) = swi2(n,x,y);
      (*Q).z = n.x;
      (*Q).w = n.y;
float ttttttttttttttttttttttttt;    
    }
    if (ll<(*r).x){
      *r = to_float2(ll,(*r).x);
      *Q = to_float4(n.z, n.w, (*Q).x, (*Q).y);
    } else if (ll<(*r).y) {
      (*r).y = ll;
      //swi2(*Q,z,w) = swi2(n,z,w);
      (*Q).z = n.z;
      (*Q).w = n.w;
    }
}


__KERNEL__ void GraphNetworkFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float BBBBBBBBBBBBBBBBBBBB;    
    U+=0.5f;

    float2 u = shape(U);
    float4 a = A(u);
    Q = B(U);
    float2 r = to_float2(line(swi2(a,x,y)-U+u,swi2(a,x,y),swi2(A(swi2(Q,x,y)),x,y))
                        ,line(swi2(a,x,y)-U+u,swi2(a,x,y),swi2(A(swi2(Q,z,w)),x,y)));
    
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,z,w)+U-u),R,iChannel0);
    
    X(&Q,&r,a,U,u,B(U+to_float2(1,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U+to_float2(0,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U+to_float2(1,0)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U-to_float2(0,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U-to_float2(1,0)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U+to_float2(1,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U+to_float2(1,-1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U-to_float2(1,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(U-to_float2(1,-1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u+to_float2(0,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u+to_float2(1,0)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u-to_float2(0,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u-to_float2(1,0)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u+to_float2(1,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u+to_float2(1,-1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u-to_float2(1,1)),R,iChannel0);
    X(&Q,&r,a,U,u,B(swi2(Q,x,y)+U-u-to_float2(1,-1)),R,iChannel0);
    X(&Q,&r,a,U,u,to_float4_f2f2(U-O*to_float2(1,0),U+O*to_float2(1,0)),R,iChannel0);
    X(&Q,&r,a,U,u,to_float4_f2f2(U-O*to_float2(0,1),U+O*to_float2(0,1)),R,iChannel0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2



__DEVICE__ void Y (inout float4 *Q, float2 U, float4 n, float2 R, __TEXTURE2D__ iChannel0) {
  float YYYYYYYYYYYYYYYYYYYYYYYYYYY;
  float l = length(U-swi2(A(swi2(n,x,y)),x,y)),//sg(U,A(swi2(n,x,y)).xy,A(swi2(n,z,w)).xy),
       ll = length(U-swi2(A(swi2(*Q,x,y)),x,y));//sg(U,A(swi2(Q,x,y)).xy,A(swi2(Q,z,w)).xy);
    if (l<ll) (*Q).x = n.x, (*Q).y = n.y;//swi2(*Q,x,y) = swi2(n,x,y);
}
__DEVICE__ void XC (inout float4 *Q, float2 U, float4 n, float2 R, __TEXTURE2D__ iChannel0) {
  float l = length(U-swi2(A(swi2(n,z,w)),x,y)),//sg(U,A(swi2(n,x,y)).xy,A(swi2(n,z,w)).xy),
       ll = length(U-swi2(A(swi2(*Q,z,w)),x,y));//sg(U,A(swi2(Q,x,y)).xy,A(swi2(Q,z,w)).xy);
    if (l<ll) (*Q).z = n.z, (*Q).w = n.w;//swi2(Q,z,w) = swi2(n,z,w);
}

__KERNEL__ void GraphNetworkFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
float CCCCCCCCCCCCCCCCCCCCC;    
    U+=0.5f;

    Q = swi4(C(U),x,y,x,y);
    if (iFrame%N==0)  Q.x=U.x,Q.y=U.y;//swi2(Q,x,y) = U;
    else {
      float k = _exp2f((float)(N-1-(iFrame%N)));
      Y(&Q,U,C(U+to_float2(0,k)), R,iChannel0);
      Y(&Q,U,C(U+to_float2(k,0)), R,iChannel0);
      Y(&Q,U,C(U-to_float2(0,k)), R,iChannel0);
      Y(&Q,U,C(U-to_float2(k,0)), R,iChannel0);
    }

    XC(&Q,U,C(U+to_float2(0,1)), R,iChannel0);
    XC(&Q,U,C(U+to_float2(1,0)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(0,1)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(1,0)), R,iChannel0);
    
    XC(&Q,U,C(U+to_float2(1,1)), R,iChannel0);
    XC(&Q,U,C(U+to_float2(1,-1)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(1,1)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(1,-1)), R,iChannel0);
    
    XC(&Q,U,C(U+to_float2(0,4)), R,iChannel0);
    XC(&Q,U,C(U+to_float2(4,0)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(0,4)), R,iChannel0);
    XC(&Q,U,C(U-to_float2(4,0)), R,iChannel0);
    
    XC(&Q,U,C(U), R,iChannel0);
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void GraphNetworkFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;
    swi2S(Q,x,y, 0.3f*shape(U));
    
  SetFragmentShaderComputedColor(Q);    
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Graph Network" by wyatt. https://shadertoy.com/view/tssyDS
// 2020-04-09 20:04:37

__KERNEL__ void GraphNetworkFuse(float4 Q, float2 U, float2 iResolution, int iFrame, float4 iMouse)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    
    U+=0.5f;

    U -= 0.5f*R;
    //U*=0.1f;
    U += 0.5f*R;
    Q = to_float4_s(0);
    for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++) {
          float4 d = C(U+to_float2(x,y));
          float4 a = A(swi2(d,z,w)), b = A(swi2(d,z,w));
          Q += 0.2f/(1.0f+(0.7f-_fabs(d.z/R.x-0.5f))*dot(U-swi2(a,x,y),U-swi2(a,x,y)))*(0.6f+0.4f*sin_f4(5.0f*d.z/R.x+to_float4(1,2,3,4)));
        }
        
  SetFragmentShaderComputedColor(Q);        
}