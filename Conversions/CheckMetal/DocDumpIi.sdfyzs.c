
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define time iTime
#define rez iResolution
__DEVICE__ float wav(float t){
  return _fabs(fract(t)-0.5f);
}

//float3 mcol=to_float3_s(0.0f);
//float glw=0.0f;

__DEVICE__ float DE(float3 p0, float iTime, inout float3 *mcol, inout float *glw){
  float4 p=to_float4_aw(p0,1.1f);
  float3 c=mod_f(p0,10.0f)-4.0f;
  
  for(int n=0;n<2;n++){
    swi3S(p,x,y,z,abs_f3(mod_f3(swi3(p,x,y,z),4.0f)-2.0f)-1.0f);
    p*=2.0f/clamp(dot(swi3(p,x,y,z),swi3(p,x,y,z)),0.25f,1.0f);
    if(p.y>p.z)  swi2S(p,y,z,swi2(p,z,y));
    if(p.x>p.y)  swi2S(p,x,y,swi2(p,y,x));
    p.x+=1.0f;
  }
  float d=(length(swi2(p,y,z))-0.1f+0.1f*wav(p.x*10.0f))/p.w;
  *glw+=_fmaxf(wav(p.x+p0.y+p0.z+time)-0.3f,0.0f)/(1.0f+d*d);
  float g=_fabs(_sinf((c.x+c.z)*10.0f-time*10.0f));
  float d2=_fminf(length(swi2(c,x,y)),length(swi2(c,y,z)+to_float2(0.5f,0.0f)))-0.125f-g*0.01f;
  if((*mcol).x>0.0f){
    if(d<d2) *mcol+=to_float3_s(0.4f)+0.1f*abs_f3(swi3(p,x,y,z));
    else     *mcol+=to_float3_s(2.0f*g);
  }
  return _fminf(d,d2);
}
__DEVICE__ float3 normal(float3 p, float d, float iTime, inout float3 *mcol, inout float *glw){//from dr2
  float2 e=to_float2(d,-d);
  float4 v=to_float4(DE(p+swi3(e,x,x,x),iTime,mcol,glw),DE(p+swi3(e,x,y,y),iTime,mcol,glw),DE(p+swi3(e,y,x,y),iTime,mcol,glw),DE(p+swi3(e,y,y,x),iTime,mcol,glw));
  return normalize(2.0f*swi3(v,y,z,w)+to_float3_s(v.x-v.y-v.z-v.w));
}
__DEVICE__ float3 sky(float3 rd, float3 L){
  float d=2.0f*_powf(_fmaxf(0.0f,dot(rd,L)),20.0f);
  return to_float3_s(d)+abs_f3(rd)*0.1f;
}

__DEVICE__ void randomize(in float2 p, float iTime, inout float *rnd){
  *rnd=fract((float)(time)+_sinf(dot(p,to_float2(13.3145f,117.7391f)))*42317.7654321f);
}

__DEVICE__ float ShadAO(in float3 ro, in float3 rd, float iTime, float rnd, inout float3 *mcol, inout float *glw){
 float t=0.01f*rnd,s=1.0f,d,mn=0.01f;
 for(int i=0;i<12;i++){
  d=_fmaxf(DE(ro+rd*t,iTime,mcol,glw)*1.5f,mn);
  s=_fminf(s,d/t+t*0.5f);
  t+=d;
 }
 return s;
}
__DEVICE__ float4 sphere(float3 ro, float3 rd, float iTime){
  float4 s=to_float4_s(100);
  float3 p=to_float3(iTime*0.5f+6.0f,-4.0f,iTime*0.5f+8.0f);
  p=ro-p;
  float b=dot(-p,rd); 
  if(b>0.0f){
    float inner=b*b-dot(p,p)+0.7f;
    if(inner>0.0f){
      float t=b-_sqrtf(inner);
      if(t>0.0f)s=to_float4_aw(normalize(p+rd*t),t);
    }
  }
  return s;
}
__DEVICE__ float3 scene(float3 ro, float3 rd, float iTime, float rnd, float2 iResolution){
  float3 mcol = to_float3_s(0.0f);
  float glw = 0.0f;
  float t=DE(ro,iTime,&mcol,&glw)*rnd,d,px=4.0f/rez.x;
  float4 s=sphere(ro,rd,iTime);
  for(int i=0;i<99;i++){
    t+=d=DE(ro+rd*t,iTime,&mcol,&glw);
    if(t>20.0f || d<px*t)break;
    if(t>s.w){px*=10.0f;ro+=rd*s.w;rd=reflect(rd,swi3(s,x,y,z));t=0.01f;}
  }
  float3 L=normalize(to_float3(0.4f,0.025f,0.5f));
  float3 bcol=sky(rd,L),col=bcol;
  float g=glw;
  if(d<px*t*5.0f){
    mcol=to_float3_s(0.001f);
    float3 so=ro+rd*t;
    float3 N=normal(so,d,iTime,&mcol,&glw);
    float3 scol=mcol*0.25f;
    float dif=0.5f+0.5f*dot(N,L);
    float vis=clamp(dot(N,-rd),0.05f,1.0f);
    float fr=_powf(1.0f-vis,5.0f);
    float shad=ShadAO(so,L,iTime, rnd,&mcol,&glw);
    col=(scol*dif+0.5f*fr*sky(reflect(rd,N),L))*shad;
  }
  return _mix(col,bcol,clamp(t*t/400.0f,0.0f,1.0f))+to_float3(1.0f,0.3f,0.1f)*_expf(-t)*clamp(g*g,0.0f,1.0f);
}
__DEVICE__ mat3 lookat(float3 fw){
  float3 up=to_float3(0.0f,0.8f,0.1f),rt=-1.0f*normalize(cross(fw,up));
  return to_mat3_f3(rt,normalize(cross(rt,fw)),fw);
}
__DEVICE__ float3 path(float t){
  t*=0.5f;t+=_sinf(t*0.1f)*7.0f;
  return to_float3(t+_sinf(t*1.1f),_sinf(t*0.3f)*0.5f-5.2f,t+_cosf(t)*0.7f); 
}
__KERNEL__ void DocDumpIiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
 float rnd;
 randomize(fragCoord,iTime,&rnd);
 float3 ro=path(iTime),fw=normalize(path(iTime+0.5f)-ro);
 float3 rd=mul_mat3_f3(lookat(fw),normalize(to_float3_aw((iResolution-2.0f*fragCoord)/iResolution.y,1.0f)));
 fragColor=to_float4_aw(scene(ro,rd,iTime,rnd, iResolution),1.0f);


 SetFragmentShaderComputedColor(fragColor);
}
