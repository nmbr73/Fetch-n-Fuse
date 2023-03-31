
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define time iTime
#define resolution iResolution
#define so fract(_sinf(time)*123.456f)

__DEVICE__ mat3 lookat(float3 dir, float3 up) {
  float3 rt=normalize(cross(dir,up));
  return to_mat3_f3(rt,cross(rt,dir),dir);
}
__DEVICE__ float3 path(float t) {
  return to_float3(_sinf(t+_cosf(t)*0.5f)*0.5f,_cosf(t*0.5f),t);
}
__DEVICE__ mat2 rot(float a) {
  float s=_sinf(a);
  float c=_cosf(a);
  return to_mat2(c,s,-s,c);
}
__DEVICE__ float3 fractal(float2 p, float iTime) {
  p=fract_f2(p*0.1f);
  float m=1000.0f;
  for (int i=0; i<7; i++) {
    p=abs_f2(p)/clamp(_fabs(p.x*p.y),0.25f,2.0f)-1.2f;
    m=_fminf(m,_fabs(p.y)+fract(p.x*0.3f+time*0.5f+(float)(i)*0.25f));
  }
  m=_expf(-6.0f*m);
  return m*to_float3(_fabs(p.x),m,_fabs(p.y));
}

__DEVICE__ float coso(float3 pp, float iTime, float *br) {
  pp*=0.7f;
  swi2S(pp,x,y, mul_f2_mat2(swi2(pp,x,y),rot(pp.z*2.0f)));
  swi2S(pp,x,z, mul_f2_mat2(swi2(pp,x,z),rot(time*2.0f)));
  swi2S(pp,y,z, mul_f2_mat2(swi2(pp,y,z),rot(time)));
  float sph=length(pp)-0.04f;
  sph-=length(sin_f3(pp*40.0f))*0.05f;
  sph=_fmaxf(sph,-length(pp)+0.11f);
  float br2=length(pp)-0.03f;
  br2=_fminf(br2,length(swi2(pp,x,y))+0.005f);
  br2=_fminf(br2,length(swi2(pp,x,z))+0.005f);
  br2=_fminf(br2,length(swi2(pp,y,z))+0.005f);
  br2=_fmaxf(br2,length(pp)-1.0f);
  *br=_fminf(br2,*br);
  float d=_fminf(*br,sph);
  return d;
}


__DEVICE__ float de(float3 p, float iTime, float *br, float *hit, float3 sphpos, float3 *pos, float *tub) {
  *hit=0.0f;
  *br=1000.0f;
  float3 pp=p-sphpos;
  swi2S(p,x,y, swi2(p,x,y) - swi2(path(p.z),x,y));
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),rot(p.z+time*0.5f)));
  float s=_sinf(p.z*0.5f+time*0.5f);
  swi2S(p,x,y, swi2(p,x,y) * (1.3f-s*s*0.7f));
  
  for(int i=0; i<6; i++) {
    p=abs_f3(p)-0.4f;
  }
  *pos=p;
  *tub=-length(swi2(p,x,y))+0.45f+_sinf(p.z*10.0f)*0.1f*smoothstep(0.4f,0.5f,_fabs(0.5f-fract(p.z*0.05f))*2.0f);
  float co=coso(pp,iTime,br);
  co=_fminf(co,coso(pp+0.7f,iTime,br));
  co=_fminf(co,coso(pp-0.7f,iTime,br));
  float d=_fminf(*tub,co);
  if (d==*tub) *hit=step(fract(0.1f*length(sin_f3(p*10.0f))),0.05f);
  return d*0.3f;
}

__DEVICE__ float3 march(float3 from, float3 dir, float iTime, float3 sphpos, float3 Color2) {
  float det=0.001f, br=0.0f, tub=0.0f, hit=0.0f;
  float3 pos;

  float2 uv=to_float2(_atan2f(dir.x,dir.y)+time*0.5f,length(swi2(dir,x,y))+_sinf(time*0.2f));
  float3 col=fractal(uv,iTime);
  float d=0.0f,td=0.0f,g=0.0f, ref=0.0f, ltd=0.0f, li=0.0f;
  float3 p=from;
  for (int i=0; i<200; i++) {
    p+=dir*d;
    d=de(p, iTime, &br, &hit, sphpos, &pos, &tub);
    if (d<det && ref==0.0f && hit==1.0f) {
      float2 e=to_float2(0.0f,0.1f);
      float3 n=normalize(to_float3(de(p+swi3(e,y,x,x), iTime, &br, &hit, sphpos, &pos, &tub),de(p+swi3(e,x,y,x), iTime, &br, &hit, sphpos, &pos, &tub),de(p+swi3(e,x,x,y), iTime, &br, &hit, sphpos, &pos, &tub))-de(p, iTime, &br, &hit, sphpos, &pos, &tub));
      p-=dir*d*2.0f;
      dir=reflect(dir,n);
      ref=1.0f;
      td=0.0f;
      ltd=td;
      continue;
    }
    if (d<det || td>5.0f) break;
    td+=d;
    g+=0.1f/(0.1f+br*13.0f);
    li+=0.1f/(0.1f+tub*5.0f);
  }
  g=_fmaxf(g,li*0.15f);
  float f=1.0f-td/3.0f;
  if (ref==1.0f) f=1.0f-ltd/3.0f;
  if (d<0.01f) {
    col=to_float3_s(1.0f);
    float2 e=to_float2(0.0f,det);
    float3 n=normalize(to_float3(de(p+swi3(e,y,x,x), iTime, &br, &hit, sphpos, &pos, &tub),de(p+swi3(e,x,y,x), iTime, &br, &hit, sphpos, &pos, &tub),de(p+swi3(e,x,x,y), iTime, &br, &hit, sphpos, &pos, &tub))-de(p, iTime, &br, &hit, sphpos, &pos, &tub));
    col=to_float3_s(n.x)*0.7f;
    col+=fract(pos.z*5.0f)*to_float3(0.2f,0.1f,0.5f);
    col+=fractal(swi2(pos,x,z)*2.0f, iTime);
    if (tub>0.01f) col=to_float3_s(0.0f);
  }
  col*=f;
  float3 glo=g*0.1f*to_float3(2.0f,1.0f,2.0f)*(0.5f+so*1.5f)*0.5f;
  
  swi2S(glo,x,z, mul_f2_mat2(swi2(glo,x,z) , rot(dir.y*1.5f)));
    
  col+=glo;
  col*=Color2;//to_float3(0.8f,0.7f,0.7f);
  col=_mix(col,to_float3_s(1.0f),ref*0.3f);
  return col;
}


__KERNEL__ void EnergeticflybyJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  CONNECT_COLOR0(Color1, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_COLOR1(Color2, 0.8f, 0.7f, 0.7f, 1.0f);
  
  CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
  CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
  
  CONNECT_SLIDER1(Level0, 0.0f, 1.0f, 0.8f);
  
  float2 uv = to_float2(fragCoord.x / resolution.x, fragCoord.y / resolution.y);
  uv -= 0.5f;
  uv /= to_float2(resolution.y / resolution.x, 1);
  float t=time;
  float3 from = path(t) + to_float3_aw(ViewXY,ViewZ);
  if (mod_f(time,10.0f)>5.0f) from = path(_floor(t/4.0f+0.5f)*4.0f) + to_float3_aw(ViewXY,ViewZ);
  float3 sphpos=path(t+0.5f);
  from.x+=0.2f;
  float3 fw  = normalize(path(t+0.5f)-from);
  float3 dir = normalize(to_float3_aw(uv,0.5f));
  dir=mul_mat3_f3(lookat(fw,to_float3(fw.x*2.0f,1.0f,0.0f)),dir);
  swi2S(dir,x,z, swi2(dir,x,z) + _sinf(time)*0.3f);
  float3 col=march(from,dir, iTime, sphpos,swi3(Color2,x,y,z));
  col=_mix(to_float3_s(0.5f)*length(col),col,Level0);//0.8f);
  //fragColor = to_float4_aw(col,1.0f);

  fragColor = to_float4_aw(swi3(col,x,y,z) * (swi3(Color1,x,y,z) + 0.5f), Color1.w);

  SetFragmentShaderComputedColor(fragColor);
}