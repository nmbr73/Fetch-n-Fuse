
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------



//#define Main void mainImage(out float4 Q, in float2 U) { R = iResolution; T = iTime; I = iFrame;
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
#define A(U) _tex2DVecN(iChannel0,((float)((int)(U).y)+0.5f)/R.x,((float)((int)(U).x)+0.5f)/R.y,15)

#define B(U) _tex2DVecN(iChannel1,(U).x/R.x,(U).y/R.y,15)
//#define C(U) texelFetch(iChannel2,to_int2(U),0)
#define C(U) _tex2DVecN(iChannel2,((float)((int)(U).y)+0.5f)/R.x,((float)((int)(U).x)+0.5f)/R.y,15)

//#define D(U) texelFetch(iChannel3,to_int2(U),0)
#define D(U) _tex2DVecN(iChannel3,((float)((int)(U).y)+0.5f)/R.x,((float)((int)(U).x)+0.5f)/R.y,15)

#define N 5
__DEVICE__ float sg (float2 p, float2 a, float2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f);
  float l = (length(p-a-(b-a)*i));
    return l;
}
__DEVICE__ float size(float2 U, float2 R) {
    float y = round(U.y/20.0f)*20.0f/R.y;
    if (U.y>0.5f*R.y) y = round(U.y/5.0f)*5.0f/R.y;
    return 7.0f*(5.3f-5.0f*_powf(y,0.2f));
}
__DEVICE__ float pie (float2 p, float2 a, float2 b, float sa, float sb) {
    if (length(a-b)==0.0f) return 1e9;
    return _fabs(dot(b-a,p-a)/dot(b-a,b-a)-(sa)/(sa+sb));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel3

#define swi2S(a,b,c,d) (a).b = (d).x; (a).c = (d).y


// Save bubble position and calc force 
__KERNEL__ void BubbleBatFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    
    U += 0.5f;
    float2 R = iResolution; float T = iTime; int I = iFrame;
    Q = A(U);
    float sz = size(U,R);
    float2 f = to_float2(0,-1e-3);

    for (float _x = -2.0f; _x <= 2.0f;_x+=1.0f)
    for (float _y = -2.0f; _y <= 2.0f;_y+=1.0f)
    {
        float4 d = D(swi2(Q,x,y)+to_float2(_x,_y));
        float4 a = A(swi2(d,x,y));
        float s = size(swi2(d,x,y),R);
        float2 r = swi2(a,x,y)-swi2(Q,x,y);
        float l = length(r);
        float w = sg(swi2(Q,x,y)+to_float2(_x,_y),swi2(Q,x,y),swi2(a,x,y));
        if (l>0.0f && l<sz+s && w<1.0f) {
            f += 0.01f*r/l*(l-sz-s);
            f += 0.03f*r/l*_expf(-l/s/sz);
        }
    }
    f *= 20.0f/sz;
    if (iMouse.z>0.0f) {
        float2 m = (swi2(Q,x,y)-swi2(iMouse,x,y));
        //swi2(Q,z,w) += 1e-2*_expf(-1e-2*length(m))*m;
        swi2S(Q,z,w, swi2(Q,z,w) + 1e-2*_expf(-1e-2*length(m))*m);
    }
    
    //swi2(Q,z,w) += f;
    swi2S(Q,z,w,swi2(Q,z,w) + f);
    
    //swi2(Q,x,y) += f + swi2(Q,z,w)*10.0f/sz;
    swi2S(Q,x,y,swi2(Q,x,y) + f + swi2(Q,z,w)*10.0f/sz);
    
    if (length(swi2(Q,z,w))*10.0f/sz>1.0f) swi2S(Q,z,w, normalize(swi2(Q,z,w))/10.0f*sz);
    
    if(Q.x<sz)     Q.x=sz,    swi2S(Q,z,w,swi2(Q,z,w)*0.0f);
    if(Q.y<sz)     Q.y=sz,    swi2S(Q,z,w,swi2(Q,z,w)*0.0f);
    if(Q.x>R.x-sz) Q.x=R.x-sz,swi2S(Q,z,w,swi2(Q,z,w)*0.0f);
    if(Q.y>R.y-sz) Q.y=R.y-sz,swi2S(Q,z,w,swi2(Q,z,w)*0.0f);
    
    if (I<1) {
        float2 v;
        v.y = round(U.y/20.0f)*20.0f;
        float s = 2.0f*size(to_float2(0,v.y),R);
        v.x = round(U.x/s)*s;
        Q = to_float4(v.x,v.y,0,0);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Forest_0' to iChannel1
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Display Bubbles 
__KERNEL__ void BubbleBatFuse__Buffer_B(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel1)
{
    U += 0.5f;
    float2 R = iResolution; float T = iTime; int I = iFrame;
    Q = to_float4_s(0);
    float4 c = C(U);
    float4 d = D(U);
    float4 a = A(swi2(c,x,y));
    float4 b = A(swi2(d,x,y));
    float sa = size(swi2(c,x,y),R);
    float sb = size(swi2(d,x,y),R);
        
    float2 v = U-swi2(a,x,y);
    if (length(U-swi2(a,x,y))<sa) {
        float z = _sqrtf(sa*sa-dot(v,v));
        float3 no = normalize(to_float3_aw(U,z)-to_float3_aw(swi2(a,x,y),0));
        Q = texture(iChannel1,reflect(no,-1.0f*normalize(to_float3_aw(swi2(U,x,y),0)-swi3(R,x,y,x))));
        float l = _fabs(length(v)-sa);
        float w = length(swi2(a,x,y)-swi2(b,x,y))*pie(U,swi2(a,x,y),swi2(b,x,y),sa,sb);
        l = _fminf(l,w);
        Q *= _expf(-0.01f*l*l);
    } 
    Q.w = sa;


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3


// track nearest bubble
__DEVICE__ void X (inout float4 *Q, float2 U, float4 a, float2 R, __TEXTURE2D__ iChannel0) {
    float4 q = A(swi2(*Q,x,y));
    float4 b = A(swi2(a,x,y));
    if((length(U-swi2(b,x,y))-size(swi2(a,x,y),R))<(length(U-swi2(q,x,y))-size(swi2(*Q,x,y),R)))
        *Q = a;
}
__KERNEL__ void BubbleBatFuse__Buffer_C(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame)
{
    U += 0.5f;
    float2 R = iResolution; float T = iTime; int I = iFrame;
    Q = C(U);
    X(&Q,U,C(U+to_float2(0,1)),R,iChannel0);
    X(&Q,U,C(U+to_float2(1,0)),R,iChannel0);
    X(&Q,U,C(U-to_float2(0,1)),R,iChannel0);
    X(&Q,U,C(U-to_float2(1,0)),R,iChannel0);
    X(&Q,U,C(U+to_float2(0,3)),R,iChannel0);
    X(&Q,U,C(U+to_float2(3,0)),R,iChannel0);
    X(&Q,U,C(U-to_float2(0,3)),R,iChannel0);
    X(&Q,U,C(U-to_float2(3,0)),R,iChannel0);
    
    X(&Q,U,D(U+to_float2(0,1)),R,iChannel0);
    X(&Q,U,D(U+to_float2(1,0)),R,iChannel0);
    X(&Q,U,D(U-to_float2(0,1)),R,iChannel0);
    X(&Q,U,D(U-to_float2(1,0)),R,iChannel0);
    
    if(iFrame < 1) {
        Q = to_float4(U.x,U.y,0,0);
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer B' to iChannel1
// Connect Buffer D 'Previsualization: Buffer C' to iChannel2
// Connect Buffer D 'Previsualization: Buffer D' to iChannel3


// Track neighborhood network
__DEVICE__ void Y (inout float4 *Q, float2 U, float4 c ,float4 a, float2 R, __TEXTURE2D__ iChannel0) {
    float4 q = A(swi2(*Q,x,y));
    float4 b = A(swi2(a,x,y));
    float sa = size(swi2(*Q,x,y),R);
    float sb = size(swi2(a,x,y),R);
    if (pie(U,swi2(c,x,y),swi2(b,x,y),sa,sb)<pie(U,swi2(c,x,y),swi2(q,x,y),sa,sb))
        *Q = a;
}
__KERNEL__ void BubbleBatFuse__Buffer_D(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame)
{
    U += 0.5f;
    float2 R = iResolution; float T = iTime; int I = iFrame;
    Q = D(U);
    float4 c = A(swi2(C(U),x,y));
    Y(&Q,U,c,D(U+to_float2(0,1)),R,iChannel0);
    Y(&Q,U,c,D(U+to_float2(1,0)),R,iChannel0);
    Y(&Q,U,c,D(U-to_float2(0,1)),R,iChannel0);
    Y(&Q,U,c,D(U-to_float2(1,0)),R,iChannel0);
    Y(&Q,U,c,D(U+to_float2(1,1)),R,iChannel0);
    Y(&Q,U,c,D(U+to_float2(1,-1)),R,iChannel0);
    Y(&Q,U,c,D(U-to_float2(1,1)),R,iChannel0);
    Y(&Q,U,c,D(U-to_float2(1,-1)),R,iChannel0);
    
    Y(&Q,U,c,C(U+to_float2(0,1)),R,iChannel0);
    Y(&Q,U,c,C(U+to_float2(1,0)),R,iChannel0);
    Y(&Q,U,c,C(U-to_float2(0,1)),R,iChannel0);
    Y(&Q,U,c,C(U-to_float2(1,0)),R,iChannel0);


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2
// Connect Image 'Previsualization: Buffer D' to iChannel3


// Sparkle
__KERNEL__ void BubbleBatFuse(float4 Q, float2 U, float iTime, float2 iResolution, int iFrame)
{
 
    float2 R = iResolution; float T = iTime; int I = iFrame;
    Q = 0.5f*B(U)+0.7f*sin_f4(4.5f+0.7f*U.y/R.y+to_float4(1,2,3,4));
    for (float i = -20.0f; i <= 20.0f; i+=1.0f) {
        float4 a = B(U+to_float2(i,0.5f*i));
        float4 b = B(U+to_float2(-0.3f*i,i));
        Q += 0.6f*to_float4(2,1,1,1)*_expf(-0.004f*i*i)*(0.5f+0.5f*sin_f4(0.4f*i+to_float4(1,2,3,4)))* 
            (pow_f4(a,to_float4_s(5))/_sqrtf(a.w)+
             pow_f4(b,to_float4_s(5))/_sqrtf(b.w));
    }


  SetFragmentShaderComputedColor(Q);
}