
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define R iResolution

#define DTR 0.01745329f
#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))


__DEVICE__ float3 cp,cn,cr,ro,rd,ss,oc,cc,gl,vb;

__DEVICE__ float tt,cd,sd,io,oa;


__DEVICE__ float bx(float3 p,float3 s)  {float3 q=abs_f3(p)-s; return _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f)+length(_fmaxf(q,to_float3_s(0.0f)));}
__DEVICE__ float cy(float3 p, float2 s) {p.y+=s.x/2.0f;p.y-=clamp(p.y,0.0f,s.x);return length(p)-s.y;}


__DEVICE__ float3 lattice(float3 p, int iter, float an)
{
    for(int i = 0; i < iter; i++)
    {
      swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(an*DTR)));
      swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot(an*DTR)));
      p=abs_f3(p)-1.0f;
      swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(-an*DTR)));
    }
    return p;
}


__DEVICE__ float mp(float3 p, int *ec, float4 iMouse, float2 R)
{
 

//now with mouse control
    if(iMouse.z>0.0f){
      swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(2.0f*(iMouse.y/iResolution.y-0.5f))));
      swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x) , rot(-7.0f*(iMouse.x/iResolution.x-0.5f))));
    }
    float3 pp=p;
    
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(tt*0.1f)));
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot(tt*0.1f)));
  
    float3 pl1 = lattice(p, 4, 25.0f);
    float3 pl2 = lattice(p, 4, 45.0f);
    float3 pl3 = lattice(p, 4, 70.0f);
  
    float b1=bx(pl1, to_float3(3.0f,0.15f,0.15f));
    float b2=cy(pl2, to_float2(5.0f,0.5f));
    float b3=bx(pl3, to_float3(3.0f,0.15f,0.15f));
    
    float an = _powf(sin(tt*3.0f)*0.5f+0.5f,10.0f);
    float an2 = 1.0f-_powf(sin(tt*3.0f+3.14f)*0.5f+0.5f,10.0f);
    sd = _mix(b1,b3,_cosf(tt)*0.5f+0.5f);
    sd = _mix(b2,sd, an+an2);
  
    if(sd>0.01f) gl += _expf(-sd*0.01f) * normalize(p*p) * 0.015f;
  
    sd=_fabs(sd)-0.001f;

    if(sd<0.001f)
    {
      oc=to_float3_s(0.0f);
      io=1.05f;
      oa=0.0f;
      ss=to_float3_s(0);
      vb=to_float3(0.0f,8,2.5f);
      *ec=2;  
    }
    return sd;
}

__DEVICE__ void tr(int *ec, float4 iMouse, float2 R){vb.x=0.0f;cd=0.0f;for(float i=0.0f;i<512.0f;i++){mp(ro+rd*cd,ec, iMouse, R);cd+=sd; if(sd<0.0001||cd>128.0f)break;}}
__DEVICE__ void nm(int *ec, float4 iMouse, float2 R)
{
  mat3 k= to_mat3_f3(cp-to_float3(0.001f,0,0),cp-to_float3(0,0.001f,0),cp-to_float3(0,0,0.001f));
  cn=normalize(mp(cp,ec, iMouse, R)-to_float3(mp(k.r0,ec, iMouse, R), mp(k.r1,ec, iMouse, R), mp(k.r2,ec, iMouse, R)));
}

__DEVICE__ void px(int *ec, float4 iMouse, float2 R)
{
  cc = to_float3(0.0f,0.0f,0.0f)+length(pow_f3(rd+to_float3(0,0.0f,0),to_float3_s(3)))*0.1f+gl;
  float3 l = to_float3(0.9f,0.7f,0.5f);
  if(cd>128.0f){ oa = 1.0f; return;}
  float df=clamp(length(cn*l),0.0f,1.0f);
  float3 fr=_powf(1.0f-df,3.0f)*_mix(cc,to_float3_s(0.4f),0.5f);
  float sp=(1.0f-length(cross(cr,cn*l)))*0.2f;
  float ao=_fminf(mp(cp+cn*0.3f,ec, iMouse, R)-0.3f,0.3f)*0.5f;
  cc=_mix((oc*(df+fr+ss)+fr+sp+ao+gl),oc,vb.x);
}

__DEVICE__ void render(float2 frag, float2 res, float time, out float4 *col, float4 iMouse, float2 R)
{
  float4 fc = to_float4_s(0.0f);
  int es=0,ec;
  
  //float3 cp,cn,cr,ro,rd,ss,oc,cc,gl,vb;

  //float tt,cd,io,oa;

  tt=mod_f(time, 260.0f);
  float2 uv=to_float2(frag.x/res.x,frag.y/res.y);
  uv-=0.5f;uv/=to_float2(res.y/res.x,1);
  ro=to_float3(0,0,-15);rd=normalize(to_float3_aw(uv,1));
  
  for(int i=0;i<20;i++)
  {
    tr(&ec, iMouse, R);cp=ro+rd*cd;
    nm(&ec, iMouse, R);ro=cp-cn*0.01f;
    cr=refract_f3(rd,cn,i%2==0?1.0f/io:io);
    if(length(cr)==0.&&es<=0){cr=reflect(rd,cn);es=ec;}
    
    if(max(es,0)%3==0 && cd<128.0f) rd=cr;es--;
    
    if(vb.x>0.0f&&i%2==1) oa = _powf(clamp(cd/vb.y,0.0f,1.0f),vb.z);
    px(&ec, iMouse, R);
    fc=fc+to_float4_aw(cc*oa,oa)*(1.0f-fc.w);  
    if((fc.w>=1.0f||cd>128.0f))break;
  }
  *col = fc/fc.w;
}

__KERNEL__ void NeonCrystalJipi477Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float2 iResolution, float4 iMouse)
{

    render(fragCoord,iResolution,iTime,&fragColor, iMouse, R);


  SetFragmentShaderComputedColor(fragColor);
}