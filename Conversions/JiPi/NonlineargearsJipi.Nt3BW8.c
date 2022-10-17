
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float2 hash23(float3 p3)
{//https://www.shadertoy.com/view/4djSRW
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}
//float2 ur, U;

#define R iResolution

__DEVICE__ float4 t (float2 v, int a, int b, float2 R, __TEXTURE2D__ iCh) {return texture(iCh,((v+to_float2(a,b))/R));}
__DEVICE__ float4 t (float2 v, float2 R, __TEXTURE2D__ iCh) {return texture(iCh,(v/R));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
__KERNEL__ void NonlineargearsJipiFuse__Buffer_A(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel2)
{


    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1) {
        Co = to_float4_s(0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,R, iChannel2),x,y);
            v -= tmp;
        }
        float4 me = t(v,R, iChannel2);
        for (int i = 0; i < 6; i++) {
            float2 tmp = swi2(t(v,R,iChannel2),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2(t(v, R,iChannel2),z,w));
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R, iChannel2),x,y);
            B -= swi2(t(B,R, iChannel2),x,y);
            C -= swi2(t(C,R, iChannel2),x,y);
            D -= swi2(t(D,R, iChannel2),x,y);
        }
        float4 n = t(v,0,1,R, iChannel2),
            e = t(v,1,0,R, iChannel2),
            s = t(v,0,-1,R, iChannel2),
            w = t(v,-1,0,R, iChannel2);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        if (U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f)      {me.x=0.1f;me.z=-1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f) {me.x=0.1f;me.z=1.0f;}
        if (U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f)      {me.x=-0.1f;me.z=1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f) {me.x=-0.1f;me.z=-1.0f;}
        else if (U.x<1.0f||ur.x-U.x<1.0f||ur.y-U.y<1.0f||U.y<1.0f) me.x*=0.0f,me.y*=0.0f;//swi2(me,x,y)*=0.0f;
                
        float o = 0.0f, m=10.0f;
        float2 _y = U/iResolution*m;
        _y = fract_f2(_y)*2.0f-1.0f+hash23(to_float3_aw(_floor(_y),iFrame))*2.0f-1.0f;
        me.w = me.w*0.99f + 2.0f*(1.0f+clamp(-0.2f*me.z*(me.z)*me.z,0.0f,2.0f))*smoothstep(0.004f,0.0f,length(_y)/m);
                
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0

#ifdef XXX
float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float2 hash23(float3 p3)
{//https://www.shadertoy.com/view/4djSRW
  p3 = fract(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}


//float2 ur, U;
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif

__KERNEL__ void NonlineargearsJipiFuse__Buffer_B(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    float2 U = uu+0.5f;;
    float2 ur = iResolution;
    if (iFrame < 1) {
        Co = to_float4_s(0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,R,iChannel0);
        for (int i = 0; i < 6; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2(t(v,R,iChannel0),z,w));
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R,iChannel0),x,y);
            B -= swi2(t(B,R,iChannel0),x,y);
            C -= swi2(t(C,R,iChannel0),x,y);
            D -= swi2(t(D,R,iChannel0),x,y);
        }
        float4 n = t(v,0,1,R,iChannel0),
               e = t(v,1,0,R,iChannel0),
               s = t(v,0,-1,R,iChannel0),
               w = t(v,-1,0,R,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
                       
        if (U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f)      {me.x=0.1f;me.z=-1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f) {me.x=0.1f;me.z=1.0f;}
        if (U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f)      {me.x=-0.1f;me.z=1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f) {me.x=-0.1f;me.z=-1.0f;}
        else if (U.x<1.0f||ur.x-U.x<1.0f||ur.y-U.y<1.0f||U.y<1.0f) me.x*=0.0f,me.y*=0.0f;//swi2(me,x,y)*=0.0f;
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1
// Connect Buffer C 'Previsualization: Buffer D' to iChannel3

#ifdef XXX
float ln (float2 p, float2 a, float2 b) {
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
}
__DEVICE__ float2 hash23(float3 p3)
{//https://www.shadertoy.com/view/4djSRW
  p3 = fract(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
    return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}
float2 ur, U;
__DEVICE__ float4 t (float2 v, int a, int b) {return texture(iChannel0,((v+to_float2(a,b))/ur));}
__DEVICE__ float4 t (float2 v) {return texture(iChannel0,(v/ur));}
__DEVICE__ float area (float2 a, float2 b, float2 c) { // area formula of a triangle from edge lengths
    float A = length(b-c), B = length(c-a), C = length(a-b), s = 0.5f*(A+B+C);
    return _sqrtf(s*(s-A)*(s-B)*(s-C));
}
#endif


__KERNEL__ void NonlineargearsJipiFuse__Buffer_C(float4 Co, float2 uu, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 U = uu+0.5f;
    float2 ur = iResolution;
    if (iFrame < 1) {
        Co = to_float4_s(0);
    } else {
        float2 v = U,
             A = v + to_float2( 1, 1),
             B = v + to_float2( 1,-1),
             C = v + to_float2(-1, 1),
             D = v + to_float2(-1,-1);
        for (int i = 0; i < 3; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
            v -= tmp;
        }
        float4 me = t(v,R,iChannel0);
        for (int i = 0; i < 6; i++) {
            float2 tmp = swi2(t(v,R,iChannel0),x,y);
            v -= tmp;
        }
        swi2S(me,z,w, swi2(t(v,R,iChannel0),z,w));
        for (int i = 0; i < 6; i++) {
            A -= swi2(t(A,R,iChannel0),x,y);
            B -= swi2(t(B,R,iChannel0),x,y);
            C -= swi2(t(C,R,iChannel0),x,y);
            D -= swi2(t(D,R,iChannel0),x,y);
        }
        float4 n = t(v,0,1,R,iChannel0),
               e = t(v,1,0,R,iChannel0),
               s = t(v,0,-1,R,iChannel0),
               w = t(v,-1,0,R,iChannel0);
        float4 ne = 0.25f*(n+e+s+w);
        me = _mix(me,ne,to_float4(0.06f,0.06f,1,0.0f));
        me.z  = me.z - (area(A,B,C)+area(B,C,D)-4.0f);
        float4 pr = to_float4(e.z,w.z,n.z,s.z);
        swi2S(me,x,y, swi2(me,x,y) + to_float2(pr.x-pr.y, pr.z-pr.w)/ur);
        
        
        float4 mouse = texture(iChannel1,to_float2_s(0.5f));
        float q = ln(U,swi2(mouse,x,y),swi2(mouse,z,w));
        float2 mo = swi2(mouse,x,y)-swi2(mouse,z,w);
        float l = length(mo);
        if (l>0.0f) {
          mo = normalize(mo)*_fminf(l,0.1f);
          me += _expf(-q*q)*to_float4_f2f2(normalize(swi2(mo,x,y)),to_float2_s(0.0f));
        }
        
        
        if (U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f)       {me.x=0.1f;me.z=-1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.7f*ur.y)<2.0f)  {me.x=0.1f;me.z=1.0f;}
        if (U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f)       {me.x=-0.1f;me.z=1.0f;}
        if (ur.x-U.x<2.0f&&_fabs(U.y-0.3f*ur.y)<2.0f)  {me.x=-0.1f;me.z=-1.0f;}
        else if (U.x<1.0f||ur.x-U.x<1.0f||ur.y-U.y<1.0f||U.y<1.0f) me.x*=0.0f,me.y*=0.0f;//swi2(me,x,y)*=0.0f;
        
        Co = me;
        swi3S(Co,x,y,z, clamp(swi3(Co,x,y,z), -1.0f*to_float3(0.5f,0.5f,40.0f), to_float3(0.5f,0.5f,40.0f)));
    }

  SetFragmentShaderComputedColor(Co);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer D' to iChannel0


// MOUSE
__KERNEL__ void NonlineargearsJipiFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    fragCoord+=0.5f; 
    
    float4 p = texture(iChannel0,fragCoord/iResolution);
    if (iMouse.z>0.0f) {
        if (p.z>0.0f) fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(p,x,y));
      else fragColor =  to_float4_f2f2(swi2(iMouse,x,y),swi2(iMouse,x,y));
    }
    else fragColor = to_float4_f2f2(-iResolution,-iResolution);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float2 hash22(float2 p)
{//https://www.shadertoy.com/view/4djSRW
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y))*2.0f-1.0f;
}

__DEVICE__ float2 vf (float2 v, __TEXTURE2D__ iChannel0) {
  v = swi2(_tex2DVecN(iChannel0,v.x,v.y,15),x,y);
  return v;
}
//__DEVICE__ float ln (float2 p, float2 a, float2 b) {
//    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));
//}
__DEVICE__ float ff (float2 U, float2 o, float2 R, __TEXTURE2D__ iChannel0) {
    float q = 0.3f*iResolution.x;
    float2 V = _floor(U*q+0.5f + o)/q;
    V += 0.5f*hash22(_floor(V*iResolution))/q;
    
    float2 v;
    v = vf(V,iChannel0);
    float a = 1e3;
    
    for (int i = 0; i < 4; i++) {
        v = 0.5f*vf(V, iChannel0);
        a = _fminf(a,1.2f*ln(U, V, V+v));
        V += v;
    }
    
    return _fmaxf(1.0f-iResolution.x*0.4f*a,0.0f);
}


__KERNEL__ void NonlineargearsJipiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{

    U+=0.5f; 
    
    float c = 0.0f;
    for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
        c += 0.33f*ff(U/iResolution,to_float2(x,y),iResolution, iChannel0);
    }
    }
    
    float4 me = texture(iChannel0,U/iResolution);
    swi3S(C,x,y,z, to_float3_s(c)*(0.3f+5.0f*me.w));
    C.y *= length(swi2(me,x,y))*10.0f;
    C.x *= _fmaxf(0.0f,0.2f+me.z);
    C.z *= _fmaxf(0.0f,0.2f-me.z);
    C*=to_float4(1,1.5f,2,1)*(1.0f+0.5f*C*C);

  SetFragmentShaderComputedColor(C);
}