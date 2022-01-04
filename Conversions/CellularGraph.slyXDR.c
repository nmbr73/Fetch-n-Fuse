// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

//__DEVICE__ float4 sin_f4(float4 i) {float4 r; r.x = _sinf(i.x); r.y = _sinf(i.y); r.z = _sinf(i.z); r.w = _sinf(i.w); return r;}

//#define swizw(V) to_float2((V).z,(V).w)

//#define swizwxy(V) to_float4((V).z,(V).w,(V).x,(V).y)

// To Test int the Inkubator
//#define iChannel1 iChannel0
//#define iChannel2 iChannel0
//#define iChannel3 iChannel0



#define R iResolution
#define A(U) _tex2DVecN(iChannel0,(U.x)/R.x,(U.y)/R.y,15)
#define B(U) _tex2DVecN(iChannel1,(U.x)/R.x,(U.y)/R.y,15)  //Channel1
#define C(U) _tex2DVecN(iChannel2,(U.x)/R.x,(U.y)/R.y,15)  //Channel2
#define D(U) _tex2DVecN(iChannel3,(U.x)/R.x,(U.y)/R.y,15)  //Channel3


//#define Main void mainImage(out float4 Q, float2 U)
__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b) {
   if (length(a-b)<1e-3||length(a)<1.||length(b)<1.0f) return 1e9;
    float2 m = 0.5f*(a+b); // midpoint
    return _fabs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
}
__DEVICE__ bool cmp (float2 p, float2 a, float2 b, float2 c, float2 d) {
    float l1 = ln(p,a,b),
          l2 = ln(p,c,d);
    if (l1<0.5f&&l2<0.5f) return length(a-b)<length(b-c);
    else return l1<l2;

}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

// Sort Particles 🕷
__KERNEL__ void CellularGraphFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    
    U += 0.5f;
    
    Q = A(U);
    for (float _x=-2.0f;_x<=2.0f;_x+=1.0f)
    for (float _y=-2.0f;_y<=2.0f;_y+=1.0f)
    {
        float2 u = to_float2(_x,_y);
        float4 q = A((U+u));
        if (length(U-swi2(q,x,y))<length(U-swi2(Q,x,y)))
          Q = q;
    }

  SetFragmentShaderComputedColor(Q);
}


// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------

//Sort Graph
__KERNEL__ void CellularGraphFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    

    U += 0.5f;
    float iFrame = iTime*30.0f;

    Q = B(U);
    Q.x = A(U).x;
    Q.y = A(U).y;
    
    for (float _x=-2.0f;_x<=2.0f;_x+=1.0f)
    for (float _y=-2.0f;_y<=2.0f;_y+=1.0f)
    {
        float2 u = to_float2(_x,_y);
        float4
            aa= A((U+u)),
            b = B((U+u)),
            a = to_float4(A(swi2(b,x,y)).x,A(swi2(b,x,y)).y,A(swi2(b,z,w)).x,A(swi2(b,z,w)).y),
            q = to_float4(A(swi2(Q,x,y)).x,A(swi2(Q,x,y)).y,A(swi2(Q,z,w)).x,A(swi2(Q,z,w)).y);
        if (cmp(U,swi2(q,x,y),swi2(aa,x,y),swi2(q,x,y),swi2(q,z,w)))
        {
            //swizw(Q) = swizw(q) = swixy(aa);
            Q.z = q.z = aa.x;
            Q.w = q.w = aa.y;
            
        } else
        if (cmp(U,swi2(a,x,y),swi2(a,z,w),swi2(q,x,y),swi2(q,z,w)))
        {
            q = a;
            Q = b;
        } else
        if (cmp(U,swi2(q,x,y),swi2(a,x,y),swi2(q,x,y),swi2(q,z,w)))
        {
            //swizw(q) = swixy(a);
            q.z = a.x;
            q.w = a.y;
            //swizw(Q) = swizw(a);
            Q.z = a.z;
            Q.w = a.w;
        }
    }
    //swixy(Q) = A(swixy(Q)).xy;
    Q.x = A(swi2(Q,x,y)).x;
    Q.y = A(swi2(Q,x,y)).y;
    
    //swizw(Q) = A(swizw(Q)).xy;
    Q.z = A(swi2(Q,z,w)).x;
    Q.w = A(swi2(Q,z,w)).y;
    
    if (length(U-swi2(Q,z,w))<length(U-swi2(Q,x,y)))
        Q = swi4(Q,z,w,x,y);
    if (iFrame<1) {
        Q = to_float4(U.x,U.y,U.x+1.0f,U.y+1.0f);
    }

  SetFragmentShaderComputedColor(Q);
}


// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------

// Color
__KERNEL__ void CellularGraphFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    
    U += 0.5f;

    float iFrame = iTime*30.0f;

    float4 d = D(U);
    Q = C(swi2(d,x,y));
    
    if (iFrame < 1)
      Q = (0.5f+0.5f*sin_f4(0.1f*U.y+to_float4(1,2,3,4)));

  SetFragmentShaderComputedColor(Q);
}


// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------

// Apply Forces
__KERNEL__ void CellularGraphFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    

    U += 0.5f;
    float iFrame = iTime*30.0f;

    Q = A(U);
    float4 cQ = C(swi2(Q,x,y)); 
    float2 f = to_float2_s(-1e-3);
    for (float _x=-3.0f;_x<=3.0f;_x+=1.0f)
    for (float _y=-3.0f;_y<=3.0f;_y+=1.0f)
    {
        float2 u = swi2(Q,x,y)+to_float2(_x,_y);
        float4 b = B(u),
             c = C(swi2(b,z,w)),
             a = A(swi2(b,z,w));
        float2 r = swi2(Q,x,y)-swi2(a,x,y);
        float l = length(r); 
        float s = step(ln(u,swi2(b,x,y),swi2(b,z,w)),1.0f);
        if (l>0.0f&&b.z>1.0f&&b.w>1.0f)
        {
            float q = -0.5f*dot(cQ,c);
            f += s*r/l*_expf(-l/2.0f);
            f += 0.5f*s*q*r/l/l;
        }
    }
    //swizw(Q) +=   f;
    Q.z +=   f.x;
    Q.w +=   f.y;
    
    //swixy(Q) +=  f + swizw(Q);
    Q.x +=   f.x + Q.z;
    Q.y +=   f.y + Q.w;
    
    if (length(swi2(Q,z,w))>1.0f)  { float2 Qzw=normalize(swi2(Q,z,w)); Q.z=Qzw.x;Q.w=Qzw.y;}
    if (iMouse.z>0.0f&&length(U-swi2(iMouse,x,y))<5.0f) {
        Q = to_float4(U.x,U.y,0,0);
    }
    if (Q.x<1.0f)       {Q.x=1.0f;Q.z*=-1.0f;}
    if (Q.y<1.0f)       {Q.y=1.0f;Q.w*=-1.0f;}
    if (R.x-Q.x<1.0f)   {Q.x=R.x-1.0f;Q.z*=-1.0f;}
    if (R.y-Q.y<1.0f)   {Q.y=R.y-1.0f;Q.w*=-1.0f;}
    if (iFrame<1) {
        Q = to_float4(_round(U.x/9.0f)*9.0f,_round(U.y/9.0f)*9.0f,0,0);
    }

  SetFragmentShaderComputedColor(Q);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
__KERNEL__ void CellularGraphFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    

    float4 a = A(U);
    float4 b = B(U); 
    float4 c = C(U);
    float r = length(swi2(b,x,y)-swi2(b,z,w));
    float l = ln(U,swi2(b,x,y),swi2(b,z,w));
    float p = length(U-swi2(a,x,y));
    Q = 1.4f*c
        *_expf(-0.003f*r*r)
        *smoothstep(1.0f,0.0f,l);
    //Q -= smoothstep(3.0f,2.0f,p);
    //Q = smoothstep(1.0f,0.0f,_fabs(p-2.0f))*c;
       
//Q = to_float4(1.0f,0.5f,0.3f,1.0f);//B(U);        //TEst
    
  SetFragmentShaderComputedColor(Q);
}


