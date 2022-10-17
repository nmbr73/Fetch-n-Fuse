
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define hash(x) fract(_sinf(x)*1e3)
#define hash_f2(x) fract_f2(sin_f2(x)*1e3)


//float2 l=to_float2(1,0);

__DEVICE__ float3 rot3d(float3 v,float a,float3 ax){
  ax=normalize(ax);
  return _cosf(a)*v+(1.0f-_cosf(a))*dot(ax,v)*ax-_sinf(a)*cross(ax,v);
}

__DEVICE__ float hash3(float3 p){
  float s=dot(p,to_float3(1.2134f,1.1623f,1.7232f));
  return hash(s);
}

__DEVICE__ float2 hash22(float2 p){
  float2 s=to_float2(dot(p,to_float2(1.6823f,1.2362f)),dot(p,to_float2(1.1631f,1.7223f)));
  return hash_f2(s)*2.0f-1.0f;
}

__DEVICE__ float sphere(float3 i,float3 f,float3 c){
  float r=hash3(i+c);
  
  if(r>0.95f)r=hash(r)*0.5f;
  else r=-0.5f;
  
  return length(f-c)-r;
}

// Reference:
// https://www.iquilezles.org/www/articles/fbmsdf/fbmsdf.htm
__DEVICE__ float sphereL(float3 p, float iTime){
  
  float2 l=to_float2(1,0);
  p.y-=iTime*0.5f;
  float d=1e5;
  float3 i=_floor(p);
  float3 f=fract(p);
  
  d=_fminf(d,sphere(i,f,swi3(l,y,y,y)));
  d=_fminf(d,sphere(i,f,swi3(l,y,y,x)));
  d=_fminf(d,sphere(i,f,swi3(l,y,x,y)));
  d=_fminf(d,sphere(i,f,swi3(l,y,x,x)));
  
  d=_fminf(d,sphere(i,f,swi3(l,x,y,y)));
  d=_fminf(d,sphere(i,f,swi3(l,x,y,x)));
  d=_fminf(d,sphere(i,f,swi3(l,x,x,y)));
  d=_fminf(d,sphere(i,f,swi3(l,x,x,x)));
  
  return d;
}

__DEVICE__ float perlin2d(float2 p){
  
  float2 l=to_float2(1,0);
  float2 i=_floor(p);
  float2 f=fract_f2(p);
  float2 u=f*f*f*(6.0f*f*f-15.0f*f+10.0f);
  
  return _mix(_mix(dot(f-swi2(l,y,y),hash22(i+swi2(l,y,y))),dot(f-swi2(l,x,y),hash22(i+swi2(l,x,y))),u.x),
              _mix(dot(f-swi2(l,y,x),hash22(i+swi2(l,y,x))),dot(f-swi2(l,x,x),hash22(i+swi2(l,x,x))),u.x),
              u.y);
}

__DEVICE__ float smin(float a,float b,float k){
  float h=_fmaxf(k-_fabs(a-b),0.0f);
  return _fminf(a,b)-h*h*0.25f/k;
}

__DEVICE__ float map(float3 p,float iTime){
  float d;
  d=sphereL(p,iTime);
  d=smin(d,p.y-perlin2d(swi2(p,z,x))*0.5f,0.4f);
  return d;
}

__DEVICE__ float3 calcN(float3 p,float iTime){
  float2 e=to_float2(1e-3,0);
  return normalize(to_float3(map(p+swi3(e,x,y,y),iTime)-map(p-swi3(e,x,y,y),iTime),
                             map(p+swi3(e,y,x,y),iTime)-map(p-swi3(e,y,x,y),iTime),
                             map(p+swi3(e,y,y,x),iTime)-map(p-swi3(e,y,y,x),iTime)));
}

__DEVICE__ float fog(float d,float den){
  float s=d*den;
  return _expf(-s*s);
}

__DEVICE__ float3 hsv(float h,float s,float v){
  return ((clamp(abs_f3(fract_f3(h+to_float3(0,2,1)/3.0f)*6.0f-3.0f)-1.0f,0.0f,1.0f)-1.0f)*s+1.0f)*v;
}

__DEVICE__ float3 getC(float3 p, float HSV[3]){
  float3 col;
  //col=hsv(perlin2d(swi2(p,z,x)*0.2f),0.8f,1.0f);
  col=hsv(perlin2d(swi2(p,z,x)*HSV[0]),HSV[1],HSV[2]);
  return col;
}

__KERNEL__ void FloatingCFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  CONNECT_POINT0(Look, 0.0f, 0.0f);
  CONNECT_SLIDER0(LookZ, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER1(Fog, 0.0f, 3.0f, 0.03f);
    
  CONNECT_SLIDER2(H, -10.0f, 30.0f, 0.2f);
  CONNECT_SLIDER3(S, -10.0f, 30.0f, 0.8f);
  CONNECT_SLIDER4(V, -10.0f, 30.0f, 1.0f);
  
  CONNECT_POINT1(Turn, 0.0f, 0.0f);
  CONNECT_SLIDER5(TurnZ, -10.0f, 30.0f, 0.0f);
  
  CONNECT_SLIDER6(TurnAX, -10.0f, 30.0f, 1.0f);
  CONNECT_SLIDER7(TurnAY, -10.0f, 30.0f, 7.0f);
  CONNECT_SLIDER8(TurnAZ, -10.0f, 30.0f, 2.0f);
  
  float hsv[3] = {H,S,V};
 
  float3 ld=normalize(to_float3(-1,2,5));

  float2 uv = to_float2(fragCoord.x / iResolution.x, fragCoord.y / iResolution.y);
  uv -= 0.5f;
  uv /= to_float2(iResolution.y / iResolution.x, 1)*0.5f;
  float3 col=to_float3_s(0);
  
  float3 cp=to_float3(0.5f,5,-iTime);
  
  cp += to_float3(Look.x,Look.y,LookZ);
  
  
  float3 cd=to_float3(0,0,-1);
  cd += to_float3(Turn.x,Turn.y,0.0f);
  
  float3 cs=normalize(cross(cd,to_float3(0,1,0)));
  float3 cu=cross(cs,cd);
  
  float3 rd=normalize(uv.x*cs+uv.y*cu+cd*2.0f);
  rd=rot3d(rd,iTime*0.1f+TurnZ,to_float3(TurnAX,TurnAY,TurnAZ));
  float3 rp=cp;
  
  float d;
  for(int i=0;i<100;i++){
    d=map(rp,iTime);
    if(_fabs(d)<1e-4){
      break;
    }
    rp+=rd*d;
  }
  
  float3 n=calcN(rp,iTime);
  float3 al=getC(rp,hsv);
  float diff=_fmaxf(dot(ld,n),0.0f);
  float spec=_powf(_fmaxf(dot(reflect(ld,n),rd),0.0f),20.0f);
  col+=al*diff+spec;
  float f=fog(length(rp-cp),Fog);//0.03f);
  col=_mix(to_float3_s(1),col,f);
  
  col=pow_f3(col,to_float3_s(1.0f/2.2f));
  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}