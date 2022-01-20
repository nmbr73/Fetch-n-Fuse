
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define swi2S(a,b,c,d) {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 

//#define swi2S(a,b,c,d) a.b##c = d // if metal



__DEVICE__ float hash(float2 p)
{
  float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}


__DEVICE__ mat2 rot(float a)
{
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c,s,-s,c);
}

__DEVICE__ float3 path(float t)
    {
    float3 p=to_float3_aw(to_float2(_sinf(t*0.1f),_cosf(t*0.05f))*10.0f,t);
    p.x+=smoothstep(0.0f,0.5f,_fabs(0.5f-fract_f(t*0.02f)))*10.0f;
    return p;
}

__DEVICE__ float fractal(float2 p, float t)
{
    p=abs_f2(5.0f-mod_f2(p*0.2f,10.0f))-5.0f;
    float ot=1000.0f;
    for (int i=0; i<7; i++)
    {
        p=abs_f2(p)/clamp(p.x*p.y,0.25f,2.0f)-1.0f;
        if(i>0)ot=_fminf(ot,_fabs(p.x)+0.7f*fract_f(_fabs(p.y)*0.05f+t*0.05f+(float)(i)*0.3f));
        
    }
    ot=_expf(-10.0f*ot);
    return ot;
}

__DEVICE__ float box(float3 p, float3 l)
{
    float3 c=abs_f3(p)-l;
    return length(_fmaxf(to_float3_s(0.0f),c))+_fminf(0.0f,_fmaxf(c.x,_fmaxf(c.y,c.z)));
}

__DEVICE__ float de(float3 p, inout float *boxhit, float t, float3 adv, inout float3 *boxp)
{
    *boxhit=0.0f;
    float3 p2=p-adv;
    swi2S(p2,x,z,mul_f2_mat2(swi2(p2,x,z),rot(t*0.2f)));
    swi2S(p2,x,y,mul_f2_mat2(swi2(p2,x,y),rot(t*0.1f)));
    swi2S(p2,y,z,mul_f2_mat2(swi2(p2,y,z),rot(t*0.15f)));
    float b=box(p2,to_float3_s(1.0f));
    swi2S(p,x,y,swi2(p,x,y)-swi2(path(p.z),x,y));
    float s=sign_f(p.y);
    p.y=-_fabs(p.y)-3.0f;
    p.z=mod_f(p.z,20.0f)-10.0f;
    for (int i=0; i<5; i++)
    {
        p=abs_f3(p)-1.0f;
        swi2S(p,x,z,mul_f2_mat2(swi2(p,x,z),rot(radians(s*-45.0f))));
        swi2S(p,y,z,mul_f2_mat2(swi2(p,y,z),rot(radians(90.0f))));
    }
    float f=-box(p,to_float3(5.0f,5.0f,10.0f));
    float d=_fminf(f,b);
    if (d==b) *boxp=p2, *boxhit=1.0f;
    return d*0.7f;
}


__DEVICE__ float3 march(float3 from, float3 dir, float t, float3 adv, float2 gl_FragCoord)
{
    float det=0.001f, boxhit;
    float3 boxp;
  
    float3 p,n,g=to_float3_s(0.0f);
    float d, td=0.0f;
    for (int i=0; i<80; i++)
    {
        p=from+td*dir;
        d=de(p,&boxhit,t,adv, &boxp)*(1.0f-hash(gl_FragCoord+t)*0.3f);
        if (d<det && boxhit<0.5f) break;
        td+=_fmaxf(det,_fabs(d));
        float f=fractal(swi2(p,x,y),t)+fractal(swi2(p,x,z),t)+fractal(swi2(p,y,z),t);
        //boxp*=0.5f;
        float b=fractal(swi2(boxp,x,y),t)+fractal(swi2(boxp,x,z),t)+fractal(swi2(boxp,y,z),t);
        
        float3 colf=to_float3(f*f,f,f*f*f);
        float3 colb=to_float3(b+0.1f,b*b+0.05f,0.0f);
        g+=colf/(3.0f+d*d*2.0f)*_expf(-0.0015f*td*td)*step(5.0f,td)/2.0f*(1.0f-boxhit);
        g+=colb/(10.0f+d*d*20.0f)*boxhit*0.5f;
    }
    return g;
}

__DEVICE__ mat3 lookat(float3 dir, float3 up) 
{
  dir=normalize(dir);
  float3 rt=normalize(cross(dir,normalize(up)));
  return to_mat3_f3(rt,cross(rt,dir),dir);
}


__KERNEL__ void ExitTheMatrixFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    float2 uv = (fragCoord-iResolution*0.5f)/iResolution.y;
    float t=iTime*7.0f;
    float3 from=path(t);
    float3 adv=path(t+6.0f+_sinf(t*0.1f)*3.0f);
    float3 dir=normalize(to_float3_aw(uv,0.7f));
    dir=mul_mat3_f3(lookat(adv-from,to_float3(0.0f,1.0f,0.0f)),dir);
    float3 col=march(from, dir,t,adv, fragCoord);
    fragColor=to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}