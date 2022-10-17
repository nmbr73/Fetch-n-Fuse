
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Winning shader made at Revision 2022 Shader Showdown final

// This shader was coded live on stage in 25 minutes. Designed beforehand in several hours.

//float t,tt,b,s,gg,g; float3 pp,tp;

__DEVICE__ float smin(float a,float b,float k){float h=_fmaxf(0.0f,k-_fabs(a-b));return _fminf(a,b)-h*h*0.25f/k;}
__DEVICE__ float smax(float a,float b,float k){float h=_fmaxf(0.0f,k-_fabs(-a-b));return _fmaxf(-a,b)+h*h*0.25f/k;}
__DEVICE__ float cy(float3 p,float3 r){return _fmaxf(_fabs(length(swi2(p,x,z))-r.x)-r.y,_fabs(p.y)-r.z);}
__DEVICE__ mat2 r2(float r){return to_mat2(_cosf(r),_sinf(r),-_sinf(r),_cosf(r));}
__DEVICE__ float2 smin( float2 a, float2 b,float k ){ float h=clamp(0.5f+0.5f*(b.x-a.x)/k,0.0f,1.0f);return _mix(b,a,h)-k*h*(1.0f-h);}

__DEVICE__ float2 fb( float3 p,float ga, float tt, inout float *b, inout float *s, inout float *gg, inout float *g, inout float3 *pp, inout float3 *tp){
  *b=_sinf(p.y*15.0f)*0.03f; //frill
  float2 h,d,t=to_float2(length(p)-5.0f,0);   //blue base
  t.x=smax(length(swi2(p,x,z))-3.0f,t.x,0.5f);  //BASE CUT
  *pp=p; 
  swi2S(*pp,x,z, mul_f2_mat2(swi2(*pp,x,z),r2(_sinf(p.y*0.4f-tt)*0.4f))); //overall sway
  //(*pp).xz=_fabs(swi2(*pp,x,z))-3.0f;              //move out by 3
  (*pp).x=_fabs((*pp).x)-3.0f;              //move out by 3
  (*pp).z=_fabs((*pp).z)-3.0f;              //move out by 3
  
float fffffffffffffffffffffffffff;  
  h=to_float2(cy(*pp,to_float3(0.5f,0.2f,7.0f)),1);  //yellow tube
  h.x=smin(h.x,cy(abs_f3(*pp)-0.75f,to_float3(0.2f-*b,0.05f,6.0f)),0.5f); //yellow 4 outter tubes
  *tp=*pp;                //tp pp
  swi2S(*tp,x,z, mul_f2_mat2(swi2(*tp,x,z),r2(0.785f)));      //rot tp
  *tp=abs_f3(*tp);           //clone tp
  swi2S(*tp,x,z, mul_f2_mat2(swi2(*tp,x,z),r2(0.785f)));      //rot tp
  (*tp).y=mod_f((*tp).y,2.0f)-1.0f; //mod tp y
  d=to_float2(cy(swi3(*tp,y,x,z),to_float3(0.1f,0.01f,2.5f)),0.0f); //D cyl
  swi2S(*tp,x,z, mul_f2_mat2(swi2(*tp,x,z),r2(0.785f)));  //tp rot again
  *tp=abs_f3(*tp)-1.8f;  //tp move out 1.8
  d.x=smin(d.x,cy(*tp,to_float3(0.1f- *b *0.5f,0.05f,3.0f)),0.3f); //d side cyl
  d.x=_fmaxf(d.x,_fabs(p.y)-6.0f); //Cut whole of D
  (*pp).y=mod_f((*pp).y-tt*2.0f,3.0f)-1.5f;              //spore pp mod
  swi2S(*pp,x,z, mul_f2_mat2(swi2(*pp,x,z),r2(-tt)));                           //spore rot
  //(*pp).xz=_fabs(swi2(*pp,x,z))-_fmaxf(0.0f,_fabs(p.y*0.15f)-1.5f);//spore spread
  (*pp).x=_fabs((*pp).x)-_fmaxf(0.0f,_fabs(p.y*0.15f)-1.5f);//spore spread
  (*pp).z=_fabs((*pp).z)-_fmaxf(0.0f,_fabs(p.y*0.15f)-1.5f);//spore spread
  
  *s=length(*pp)-0.05f;                         //spores
  *g+=0.5f/(0.1f+ *s * *s *20.0f)*ga;                  //glo spores
  h.x=smin(h.x,*s,1.0f);                       //add spore to H
  h.x=smin(h.x,length(swi2(*pp,x,z))-0.02f,0.2f);       //spores lines
  h.x=smin(h.x,_fmaxf(length(swi2(*pp,x,z))-20.0f,2.5f),p.y*0.5f-5.0f); //spore tubes
  t=smin(t,h,1.5f);  //Add T and H
  t=smin(t,d,0.5f); //Add T and D
  (*tp).y=mod_f(p.y-tt*4.0f,4.0f)-2.0f; //PART tp mod p
  h=to_float2(length(*tp),1); //PART TO H
  *gg+=0.2f/(0.1f+h.x*h.x*100.0f)*ga; //GLOW PART H
  t=smin(t,h,0.5f+p.y*0.05f); //Add T and H with p.y
  t.x*=0.7f; //AVOID ARTIFACT
return t;
}

__DEVICE__ float2 mp( float3 p,float ga, float tt, inout float *b, inout float *s, inout float *gg, inout float *g, inout float3 *pp, inout float3 *tp){
  float2 h,t=fb(p,ga,tt,b,s,gg,g,pp,tp);
  h=fb(p*0.3f+to_float3(0,3,0),ga,tt,b,s,gg,g,pp,tp);
  h.x/=0.3f;
  t=smin(h,t,1.5f);
  return t;
}
__DEVICE__ float2 tr( float3 ro,float3 rd, float tt, inout float *b, inout float *s, inout float *gg, inout float *g, inout float3 *pp, inout float3 *tp){
  float2 h,t=to_float2_s(0.1f);
  for(int i=0;i<128;i++){
    h=mp(ro+rd*t.x,1.0f,tt,b,s,gg,g,pp,tp);
    if(h.x<.0001||t.x>60.0f) break;
    t.x+=h.x;t.y=h.y;
  }
  if(t.x>60.0f) t.y=-1.0f;
  return t;
}
#define a(d) clamp(mp(po+no*d,0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x/d,0.0f,1.0f)
#define s(d) smoothstep(0.0f,1.0f,mp(po+ld*d,0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x/d)
__KERNEL__ void ChampagneicecreamJipi671Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

  float2 e=to_float2(0.00035f,-0.00035f);
 
  float s,gg,g;
  float3 pp,tp;
float IIIIIIIIIIIIIIIIIIIIIIII;
  float2 uv=(fragCoord/iResolution-0.5f)/to_float2(iResolution.y/iResolution.x,1);   
  float tt=mod_f(iTime,125.6637f)*0.5f+3.94f; 
  float b=_ceil(_cosf(tt*0.4f));
  float3 ro=_mix(to_float3(1.0f,10.0f+_sinf(tt*0.4f)*11.0f,_cosf(tt*0.4f)*2.0f),
                 to_float3(_cosf(tt*0.4f)*8.0f,10.0f+_sinf(tt*0.4f)*12.0f,_sinf(tt*0.4f)*8.0f),b),
  cw=normalize(to_float3(0,12.0f+6.0f*b,0)-ro),
  cu=normalize(cross(cw,to_float3(0,1,0))),
  cv=normalize(cross(cu,cw)),
  rd=mul_mat3_f3(to_mat3_f3(cu,cv,cw),normalize(to_float3_aw(uv,0.5f))),co,fo;
  co=fo=to_float3(0.17f,0.13f,0.12f)-length(uv)*0.2f-rd.y*0.1f;
  float2 z=tr(ro,rd,tt,&b,&s,&gg,&g,&pp,&tp); float t=z.x;
  if(z.y>-1.0f){
    float3 po=ro+rd*t;
    float3 lp=ro;
    float3 ld=normalize(lp-po);
    float3 no=normalize(swi3(e,x,y,y)*mp(po+swi3(e,x,y,y),0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x
                       +swi3(e,y,y,x)*mp(po+swi3(e,y,y,x),0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x
                       +swi3(e,y,x,y)*mp(po+swi3(e,y,x,y),0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x
                       +swi3(e,x,x,x)*mp(po+swi3(e,x,x,x),0.0f,tt,&b,&s,&gg,&g,&pp,&tp).x);
    float3 al=_mix(to_float3(0.4f,0.6f,0.7f),to_float3(0.7f,0.5f,0.3f),z.y);
    if(z.y>1.0f) al=to_float3_s(1);
    float dif=_fmaxf(0.0f,dot(no,ld)),
    fr=_powf(1.0f+dot(no,rd),4.0f),
    sp=_powf(_fmaxf(dot(reflect(-ld,no),-rd),0.0f),40.0f);
    co=_mix(sp+al*(a(0.1f)*a(0.5f)+0.4f)*(dif+s(2.0f)),fo,_fminf(fr,0.5f));
    co=_mix(fo,co,_expf(-0.00004f*t*t*t));
  }
  fragColor = to_float4_aw(pow_f3(co+g*0.2f*to_float3(0.1f,0.2f,0.5f)+gg*0.2f*to_float3(0.7f,0.5f,0.3f),to_float3_s(0.55f)),1);

  SetFragmentShaderComputedColor(fragColor);
}