
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define R(p,a,t) _mix(a*dot(p,a),p,_cosf(t))+_sinf(t)*cross(p,a)
//#define H(h) (cos_f3((h)*6.3f+to_float3(0,23,21))*0.5f+0.5f)
#define H(h) (cos_f3((h)*6.3f+swi3(Color1,x,y,z))*0.5f+0.5f)

__KERNEL__ void Fractal108GazJipi323Fuse(float4 O, float2 C, float iTime, float2 iResolution)
{

CONNECT_COLOR0(Color1, 0.0f, 23.0f, 21.0f, 1.0f); 
CONNECT_COLOR1(Color2, 2.0f, 1.7f, 0.8f, 1.0f); 
CONNECT_SLIDER1(Increment, 0.0f, 100.0f, 20.0f);
CONNECT_SLIDER2(Par1, -1.0f, 1.0f, 0.01f);
CONNECT_SLIDER3(Par2, -1.0f, 1.0f, 0.2f);

    float2 r=iResolution;
    float3 p,c=to_float3_s(0),
    d=normalize(to_float3_aw((C-0.5f*r)/r.y,1));
    
    for(float i=0.0f,s,e,g=0.0f,t=iTime;i<90.0f;i+=1.0f){
        p=g*d;
        p.z-=0.5f;
        //p=R(p,H(t*0.01f),t*0.2f);
        p=R(p,H(t*Par1),t*Par2);
        s=1.0f;
        for(int j=0;j++<6;)
           p=p.x<p.y?swi3(p,z,x,y):swi3(p,z,y,x),
           s*=e=_fmaxf(1.0f/dot(p,p),1.5f),
           //p=abs_f3(p)*e-to_float3(2,1.7f,0.8f);
           p=abs_f3(p)*e-swi3(Color2,x,y,z);
        g+=e=_fabs(p.x)/s+1e-4;
      c+=_mix(to_float3_s(1),H(_logf(s)*0.4f),0.4f)*0.015f/_expf(0.1f*i*i*e);
  }
  c*=c*c;
  O=to_float4_aw(c,1);


  SetFragmentShaderComputedColor(O);
}