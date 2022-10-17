
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define R iResolution
#define I iFrame
#define A(U) _tex2DVecN(iChannel0, (U).x/R.x,(U).y/R.y,15)
#define B(U) _tex2DVecN(iChannel1, (U).x/R.x,(U).y/R.y,15)
#define C(U) _tex2DVecN(iChannel2, (U).x/R.x,(U).y/R.y,15)
#define D(U) _tex2DVecN(iChannel3, (U).x/R.x,(U).y/R.y,15)
#define E(U) _tex2DVecN(iChannel4, (U).x/R.x,(U).y/R.y,15)

//#define Main void mainImage(out float4 Q, in float2 U) { R = iResolution; I = iFrame;
__DEVICE__ float G2 (float w, float s) {
    return 0.15915494309f*_expf(-0.5f*w*w/s/s)/(s*s);
}
__DEVICE__ float G1 (float w, float s) {
    return 0.3989422804f*_expf(-0.5f*w*w/s/s)/(s);
}
__DEVICE__ float heart (float2 u,float2 R) {
    u -= to_float2(0.5f,0.4f)*R;
    u.y -= 10.0f*_sqrtf(_fabs(u.x));
    u.y *= 1.0f;
    u.x *= 0.8f;
    if (length(u)<0.35f*R.y) return 1.0f;
    else                     return 0.0f;
}

__DEVICE__ float _12(float2 U, float2 R) {

    return clamp(_floor(U.x)+_floor(U.y)*R.x,0.0f,R.x*R.y);

}

__DEVICE__ float2 _21(float i, float2 R) {

    return clamp(to_float2(mod_f(i,R.x),_floor(i/R.x))+0.5f,to_float2_s(0),R);

}

__DEVICE__ float sg (float2 p, float2 a, float2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  float l = (length(p-a-(b-a)*i));
    return l;
}

__DEVICE__ float hash (float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float noise(float2 p)
{
    float4 w = to_float4_f2f2(
        _floor(p),
        ceil_f2(p)  );
    float 
        _00 = hash(swi2(w,x,y)),
        _01 = hash(swi2(w,x,w)),
        _10 = hash(swi2(w,z,y)),
        _11 = hash(swi2(w,z,w)),
         _0 = _mix(_00,_01,fract(p.y)),
         _1 = _mix(_10,_11,fract(p.y));
    return _mix(_0,_1,fract(p.x));
}
__DEVICE__ float fbm (float2 p) {
    float o = 0.0f;
    for (float i = 0.0f; i < 3.0f; i++) {
        o += noise(0.1f*p)/3.0f;
        o += 0.2f*_expf(-2.0f*_fabs(_sinf(0.02f*p.x+0.01f*p.y)))/3.0f;
        p *= 2.0f;
    }
    return o;
}
__DEVICE__ float2 grad (float2 p) {
    float 
    n = fbm(p+to_float2(0,1)),
    e = fbm(p+to_float2(1,0)),
    s = fbm(p-to_float2(0,1)),
    w = fbm(p-to_float2(1,0));
    return to_float2(e-w,n-s);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3


//#define keyClick(a)   ( texelFetch(iChannel2,to_int2(a,0),0).x > 0.0f)

#define  k ( 0.02f * R.x*R.y )

__KERNEL__ void ValentwineFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    U+=0.5f;
    
    float i = _12(U,R);
    Q = A(U);
    
    float2 f = to_float2_s(0);
    
    if ( i < k ) {
    for (float j = -20.0f; j <= 20.0f; j+=1.0f) 
        if (j!=0.0f) {//  && j+i>=0.0f && j+i<R.x*R.y) {
        float4 a = A(_21(mod_f(i+j,k),R));
        //if (j!=0.0f && j+i>=0.0f && j+i<R.x*R.y) {
        //vec4 a = A(_21(i+j,R));
        float2 r = swi2(a,x,y)-swi2(Q,x,y);
        float l = length(r);
        f += 50.0f*r/_sqrtf(l)*(l-_fabs(j))*(G1(j,10.0f)+2.0f*G1(j,5.0f));
    }
    for (float _x = -2.0f; _x <= 2.0f; _x+=1.0f)
    for (float _y = -2.0f; _y <= 2.0f; _y+=1.0f) {
        float2 u = to_float2(_x,_y);
        float4 d = D(swi2(Q,x,y)+u);
        f -= 100.0f*d.w*u;
    }
    if (length(f)>0.1f) f = 0.1f*normalize(f);
    //swi2(Q,z,w) += f-0.03f*swi2(Q,z,w);
    Q.z += f.x-0.03f*Q.z;
    Q.w += f.y-0.03f*Q.w;
    
    
    swi2S(Q,x,y, swi2(Q,x,y) + f + 1.5f*swi2(Q,z,w)*(1.0f/_sqrtf(1.0f+dot(swi2(Q,z,w),swi2(Q,z,w)))));
    
    float4 m = 0.5f*( A(_21(i-1.0f,R)) + A(_21(i+1.0f,R)) );
    swi2S(Q,z,w, _mix(swi2(Q,z,w),swi2(m,z,w),0.1f));
    swi2S(Q,x,y, _mix(swi2(Q,x,y),swi2(m,x,y),0.01f));
    if (Q.x>R.x)   Q.y=0.5f*R.y,Q.z=-10.0f, Q.x = 0.5f;
    if (Q.x<0.0f)  Q.y=0.5f*R.y,Q.z=10.0f, Q.x = 0.5f;;
    }
     if (iFrame < 1 || Reset) {
        if ( i > k ) 
          Q = to_float4(R.x+i,R.y+i,0,0); 
        else
          Q = to_float4(0.5f*R.x + 0.25f*R.y* _cosf( 6.28f*i/k + 0),0.5f*R.y + 0.25f*R.y* _cosf( 6.28f*i/k + 1.57f), 0,0 );
    }

  SetFragmentShaderComputedColor(Q);  
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1

__DEVICE__ void XY (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.x,R)),x,y))<length(U-swi2(A(_21((*Q).x,R)),x,y))) (*Q).x = q.x;
}
__DEVICE__ void ZW (float2 U, inout float4 *Q, float4 q, float2 R, __TEXTURE2D__ iChannel0) {
    if (length(U-swi2(A(_21(q.y,R)),x,y))<length(U-swi2(A(_21((*Q).y,R)),x,y))) (*Q).y = q.y;
}


__KERNEL__ void ValentwineFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Mouse_Active, 0);
    CONNECT_CHECKBOX2(Heart_Active, 1);
    CONNECT_SLIDER0(MouseSize, -10.0f, 100.0f, 20.0f);
  
    U+=0.5f;
    Q = B(U);
    for (int _x=-1;_x<=1;_x++)
    for (int _y=-1;_y<=1;_y++) {
        XY(U,&Q,B(U+to_float2(_x,_y)),R,iChannel0);
    }
    
    XY(U,&Q,to_float4_s(Q.x-3.0f),R,iChannel0);
    XY(U,&Q,to_float4_s(Q.x+3.0f),R,iChannel0);
    XY(U,&Q,to_float4_s(Q.x-7.0f),R,iChannel0);
    XY(U,&Q,to_float4_s(Q.x+7.0f),R,iChannel0);
    if (I%12==0) 
        Q.y = _12(U,R);
    else
    {
        float _k = _exp2f((float)(11-(I%12)));
        ZW(U,&Q,B(U+to_float2(0,_k)),R,iChannel0);
        ZW(U,&Q,B(U+to_float2(_k,0)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(0,_k)),R,iChannel0);
        ZW(U,&Q,B(U-to_float2(_k,0)),R,iChannel0);
    }
    XY(U,&Q,swi4(Q,y,x,z,w),R,iChannel0);
    if (I<1) Q = to_float4_s(_12(U,R));
    
    float4 a1 = A(_21(Q.x,R));
    float4 a2 = A(_21(Q.x+1.0f,R));
    float4 a3 = A(_21(Q.x-1.0f,R));
    float l1 = sg(U,swi2(a1,x,y),swi2(a2,x,y));
    float l2 = sg(U,swi2(a1,x,y),swi2(a3,x,y));
    float l = _fminf(l1,l2);
    Q.z = Q.w = smoothstep(2.0f,1.0f,l);
    
    if(Heart_Active == 1)
      Q.w -= 0.2f*heart(U,R);
    
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<MouseSize)
        Q.w = 0.0f;
    
    
  SetFragmentShaderComputedColor(Q);    
    
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void ValentwineFuse__Buffer_C(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f; 
    Q = to_float4_s(0);
    for (float _x = -30.0f; _x <= 30.0f; _x+=1.0f)
        Q += G1(_x,10.0f)*B(U+to_float2(_x,0)).w;
      
  SetFragmentShaderComputedColor(Q);      
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3

__KERNEL__ void ValentwineFuse__Buffer_D(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f; 
    Q = to_float4_s(0);
    for (float _y = -30.0f; _y <= 30.0f; _y+=1.0f)
        Q += G1(_y,10.0f)*C(U+to_float2(0,_y)).w;
        
    Q = _mix(Q,D(U),0.5f);
    
   SetFragmentShaderComputedColor(Q);   
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Fork of "Lover 2" by FabriceNeyret2. https://shadertoy.com/view/ssjyWc
// 2022-02-14 12:52:00

// Fork of "Lover" by wyatt. https://shadertoy.com/view/fsjyR3
// 2022-02-07 18:41:05

__KERNEL__ void ValentwineFuse(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    U+=0.5f;  
    
    float4 bg = to_float4(1.000f,0.859f,0.878f,0.0f);
    Q = bg;
    float c = B(U).z;
    if (c > 0.0f) 
        Q = to_float4(c, 0.0f, 0.0f, 0.0f); 
      
    SetFragmentShaderComputedColor(Q);    
}