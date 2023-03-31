
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float vmax(float2 v) {return _fmaxf(v.x, v.y);}
__DEVICE__ float fBox2(float2 p, float2 b) {return vmax(abs_f2(p)-b);}

__DEVICE__ mat2 rot(float a) {float s=_sinf(a), c=_cosf(a); return to_mat2(c,s,-s,c);}
__DEVICE__ float wf1(float2 p){return _sinf(p.x) + _cosf(p.y);}

__DEVICE__ float cappedCylinder(float3 p, float h, float r){
    float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - to_float2(r, h);
    return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}

__DEVICE__ float3 map(float3 p, inout float3 *gl, inout float *gl1, inout float3 *gl2, float iTime) {
    float3 r = to_float3_s(0.0f);
    float3 d = to_float3_s(0.0f);
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(iTime * 0.5f)));
    float3 m = p;
    
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(_sinf(-p.y * 0.5f) * 1.1f)));
    swi2S(p,x,z, abs_f2(swi2(p,z,x))-to_float2_s(0.8f));
    float i = _sinf(p.y * 3.0f + iTime * 10.0f) * 0.5f + 0.5f;
    float b = cappedCylinder(p,  5.5f , ((i - 0.5f) * 2.0f * 0.3f) * _cosf(p.y * 0.2f));
    *gl += (0.0004f/(0.01f+b*b)) * _mix(to_float3(1.0f,0.0f,1.0f), to_float3(1.0f,1.0f,0.0f), p.y);
    r.x = _fmaxf(cappedCylinder(p, 2.0f, 0.3f + 0.2f * i), -cappedCylinder(p, 3.0f, 0.2f + 0.25f * i));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(p.y * 3.0f + iTime * 2.0f)));
    float3 q = p;
    swi2S(q,x,z, mul_f2_mat2(swi2(q,x,z) , rot(3.14f/2.0f)));
    if (fBox2(swi2(p,x,y), to_float2(0.2f, 10.0f)) < 0.0f)      r.y=3.0f, r.z=0.0f;//swi2S(r,y,z, to_float2(3.0f,0.0f)); 
    else if (fBox2(swi2(q,x,y), to_float2(0.2f, 10.0f)) < 0.0f) r.y=4.0f, r.z=0.0f;//swi2S(r,y,z, to_float2(4.0f, 0.0f));
    else                                                        r.y=1.0f, r.z=1.0f;//swi2S(r,y,z, to_float2_s(1.0f));
    *gl1 += (0.000001f/(0.000001f+_powf(r.x+0.003f, 2.0f)));
    d.x = _fminf(r.x, cappedCylinder(p, 8.5f , (0.25f + (i - 0.5f) * 2.0f * 0.15f) * _cosf(p.y * 0.2f)));
    d.y = 2.0f;
    if (r.x > d.x) r = d;
    p = m;
    d.x = length(p) - 0.45f - 0.1f * (_sinf(iTime * 10.0f) * 0.5f + 0.5f);
    *gl2 += (0.0006f/(0.01f+d.x*d.x)) * _mix(to_float3(1.0f,0.0f,1.0f), to_float3(1.0f,1.0f,0.0f), m.y);
    if (r.x > d.x) r = d;
    p = m;
    if (p.y > 0.0f) swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(0.3f)));
    p = abs_f3(p);
    swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x) , rot(-3.14f/4.0f)));
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot(-3.14f/4.0f)));
    p.y -= 1.0f;
    q = p;
    swi2S(p,y,x, mul_f2_mat2(swi2(p,y,x) , rot( _sinf(p.y * 3.14f) * 0.3f )));
    d.x =  cappedCylinder(p, 1.0f , (0.06f + (i - 0.5f) * 2.0f * 0.04f));
    p = q; p.y -= 1.0f;
    d.x = _fminf(d.x, length(p) - 0.15f - 0.05f * (_sinf(iTime * 10.0f + 1.5f) * 0.5f + 0.5f));
    *gl2 += (0.0003f/(0.01f+d.x*d.x)) * _mix(to_float3(1.0f,0.0f,1.0f), to_float3(1.0f,1.0f,0.0f), -m.y);
    d.y = 2.0f;
    if (r.x > d.x) r = d;
    return r;
}


__DEVICE__ float3 norm(float3 po, inout float3 *gl, inout float *gl1, inout float3 *gl2, float iTime) {

  const float2 e = to_float2(0.00035f, -0.00035f);
  return normalize(swi3(e,y,y,x)*map(po+swi3(e,y,y,x),gl,gl1,gl2,iTime).x + swi3(e,y,x,y)*map(po+swi3(e,y,x,y),gl,gl1,gl2,iTime).x +
                   swi3(e,x,y,y)*map(po+swi3(e,x,y,y),gl,gl1,gl2,iTime).x + swi3(e,x,x,x)*map(po+swi3(e,x,x,x),gl,gl1,gl2,iTime).x);
}

__KERNEL__ void ColorProcessingJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float4 iMouse, float2 iResolution)
{
  
    CONNECT_COLOR0(Color, 0.1f, 0.3f, 0.2f, 1.0f);
    CONNECT_COLOR1(Color1, 0.1f, 0.3f, 0.2f, 1.0f);
    CONNECT_COLOR2(Color2, 0.7f, 0.7f, 0.3f, 1.0f);
    CONNECT_COLOR3(Color3, 0.5f, 0.9f, 0.5f, 1.0f);
    CONNECT_COLOR4(Color4, 0.5f, 0.5f, 0.9f, 1.0f);
  
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
  
    CONNECT_SLIDER1(Hx, 0.0f, 2.0f, 0.7f);

    float3 gl = to_float3_s(0.0f);
    float gl1 = 0.0f;
    float3 gl2 = to_float3_s(0.0f);

    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;
    float3 ro = to_float3(0.0f,3.0f,-6.0f);
           ro += to_float3_aw(ViewXY,ViewZ);  
    float3 rd = normalize(to_float3_aw(uv,1.0f)),
           p, h;
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z) , rot(-0.4f)));
    float t = 0.0f;
    for(int i = 0; i < 120; i++) {
         p = ro + rd * t;
         h = map(p,&gl,&gl1,&gl2,iTime);
         if (h.x<0.0001f||t>40.0f) {
             if (h.z == 1.0f) h.x = _fabs(h.x) + 0.0001f;
             else break;
         };
         t += h.x * Hx;//0.7f;
    }
    //float3 ld = swi3(LD,x,y,z);//to_float3(0.0f, 1.0f,0.0f);
    //float3 ld1 = swi3(LD1,x,y,z);//to_float3(3.0f, 3.0f, 0.0f);
    //swi2S(ld1,x,z, mul_f2_mat2(swi2(ld1,x,z) , rot(iTime * 0.3f))); 
    float3 col = to_float3_s(0.1f);
    
    float alpha = Color.w;
    
    if (h.x<0.0001f) {
        if (h.y == 1.0f) col = swi3(Color1,x,y,z);//to_float3(0.1f, 0.3f, 0.2f);
        if (h.y == 2.0f) col = swi3(Color2,x,y,z);//to_float3(0.7f, 0.7f, 0.3f);
        if (h.y == 3.0f) col = swi3(Color3,x,y,z);//to_float3(0.5f, 0.9f, 0.5f);
        if (h.y == 4.0f) col = swi3(Color4,x,y,z);//to_float3(0.5f, 0.5f, 0.9f);
        alpha = 1.0f;
    }
    //col = _mix(col, to_float3(0.1f, 0.3f, 0.2f), clamp(gl1,0.0f,1.0f));
    col = _mix(col, swi3(Color,x,y,z), clamp(gl1,0.0f,1.0f));
    col += gl;
    col += gl2;
    fragColor = to_float4_aw(col,alpha);

  SetFragmentShaderComputedColor(fragColor);
}