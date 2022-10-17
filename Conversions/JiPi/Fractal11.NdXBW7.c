
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define R(p,a,t) _mix(a*dot(p,a),p,_cosf(t))+_sinf(t)*cross(p,a)
#define H(h) (cos_f3((h)*6.3f+to_float3(0,23,21))*0.5f+0.5f)

__KERNEL__ void Fractal11Fuse(float4 O, float2 C, float iTime, float2 iResolution)
{


    float2 r=iResolution;
    float3 p,c=to_float3_s(0),
    d=normalize(to_float3_aw((C-0.5f*swi2(r,x,y))/r.y,1));
    
    for(float i=0.0f,s,e,g=0.0f,t=iTime;i++<90.0f;){
        p=g*d;
        p.z-=0.4f;
        p=R(p,H(t*0.01f),t*0.2f);
        p=abs_f3(p);
        s=3.0f;
        for(int j=0;j++<6;)
           s*=e=_fmaxf(1.0f/dot(p,p),3.0f),
           p=p.x<p.y?swi3(p,z,x,y):swi3(p,z,y,x),
           p=abs_f3(p*e-to_float3(5,1,5)),
           p=R(p,normalize(to_float3(2,2,1)),2.1f);

           g+=e=length(swi2(p,x,z))/s+1e-4;
           c+=_mix(to_float3_s(1),H(_logf(s)*0.3f),0.4f)*0.019f/_expf(0.2f*i*i*e);
    }
    c*=c*c;
    O=to_float4_aw(c,1);

  SetFragmentShaderComputedColor(O);
}