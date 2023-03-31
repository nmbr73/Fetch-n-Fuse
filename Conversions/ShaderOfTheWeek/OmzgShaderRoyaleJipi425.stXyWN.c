
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Shader made Live during OMZG Shader Royale (12/02/2021) in about 80m
// 1st place
// https://www.twitch.tv/videos/911443995?t=01h12m13s

#define FEEDBACK   1
#define ALL_COLORS 1


__DEVICE__ mat2 rot(float a) {
  
  float ca=_cosf(a);
  float sa=_sinf(a);
  return to_mat2(ca,sa,-sa,ca);
}

__DEVICE__ float3 rnd(float3 p) {
  return fract_f3(sin_f3(p*524.574f+swi3(p,y,z,x)*874.512f)*352.341f);
}

__DEVICE__ float rnd(float t) {
  return fract(_sinf(t*472.547f)*537.884f);
}

__DEVICE__ float curve(float t, float d) {
  t/=d;
  return _mix(rnd(_floor(t)), rnd(_floor(t)+1.0f), _powf(smoothstep(0.0f,1.0f,fract(t)), 10.0f));
}

__DEVICE__ float3 curve(float3 t, float d) {
  t/=d;

  float3 tmp = to_float3(smoothstep(0.0f,1.0f,fract(t.x)),
                         smoothstep(0.0f,1.0f,fract(t.y)),
                         smoothstep(0.0f,1.0f,fract(t.z)));
  
  return mix_f3(rnd(_floor(t)), rnd(_floor(t)+1.0f), pow_f3(tmp, to_float3_s(10.0f)));
}

__DEVICE__ float box(float3 p, float3 s) {
  p=abs_f3(p)-s;
  return _fmaxf(p.x, _fmaxf(p.y,p.z));
}

__DEVICE__ float3 repeat(float3 p, float3 s) {
  return (fract_f3(p/s+0.5f)-0.5f)*s;  
}

__DEVICE__ float map(float3 p, float time, inout float3 *atm, inout float3 *id) {
  
  if(time>10.0f) p.y += curve(time*5.0f-length(swi2(p,x,z)), 1.3f)*0.6f;
  if(time>36.0f) swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(time*0.4f-length(swi2(p,x,z))*0.3f)));
    
  float3 p2=p+sin_f3(time*to_float3(1,1.2f,0.8f)*0.2f)*0.03f;
  float mm=10000.0f;
  *id=to_float3_s(0);
  // This is the main kifs, making lots of plane cuts in everything
  // we get back the distance to the nearest plane in "mm"
  // also each "block" between plane gets a different "id"
  for(float i=1.0f; i<4.0f; ++i) {
    float t=time*0.1f+curve(time+i*0.2f, 1.3f)*4.0f;
    swi2S(p2,x,z, mul_f2_mat2(swi2(p2,x,z) , rot(t)));
    t+=sign_f(p2.x); // this makes the cuts not totaly symmetric
    swi2S(p2,y,x, mul_f2_mat2(swi2(p2,y,x) , rot(t*0.7f)));
    
    *id += sign_f3(p2)*i*i; // change id depending on what plane's side we are
    p2=abs_f3(p2);
    mm=_fminf(mm, _fminf(p2.x,_fminf(p2.y,p2.z)));
    
    p2-=0.2f+_sinf(time*0.3f)*0.3f;
  }
  
  // now we translate each "block" randomly according to the id
  p += (curve(rnd(*id)+time*0.3f,0.7f)-0.5f)*0.8f;
  
  // to have the random breaks without killing the SDF completly, we will have to make the ray "slow down" near the planes
  // we will also use that as translucent glowy shape
  float d2=_fminf(mm,1.5f-length(p));
  
  // block shapes
  float d=_fabs(length(p)-1.0f-mm*1.4f)-0.1f;
  
  // add boxes or cylinder randomly on each block
  float3 r2=rnd(*id+0.1f);
  if(r2.x<0.3f) {
    d=_fminf(d, _fmaxf(box(repeat(p2, to_float3_s(0.25f)), to_float3_s(0.1f)), d-0.1f)); 
  } else if(r2.x<0.7f) {
    d=_fminf(d, _fmaxf(length(swi2(repeat(p2, to_float3_s(0.15f)),x,y)-0.05f), d-0.2f)); 
  }
  
  // cut everything by the planes
  d=_fmaxf(d,0.06f-mm);
  
  // translucent glowy shape, appearing sometimes
  *atm += to_float3(1,0.5f,0.3f)*r2*0.0013f/(0.01f+_fabs(d2))*_fmaxf(0.0f,curve(time+r2.y, 0.3f)-0.4f);
  // put the plane cuts in the SDF, but with _fmaxf(0.2f) so the ray will just slow down but then go through it
  d2=_fmaxf(d2,0.2f);  
  d=_fminf(d,d2);
  
  // floor plan
  float d3 = p.y+2.0f;
  
  // tried adding things to the terrain, but failed ^^
  float3 p3 = repeat(p,to_float3(10,10,10));
  //d3 = _fminf(d3, length(swi2(p3,x,z))-0.7f);
  //d=_fminf(d, _fmaxf(d3, 0.5f-_fabs(p3.y)));
  
  d=_fminf(d, _fmaxf(d3, 0.2f-mm));
  
  return d;
}

__DEVICE__ float3 cam(float3 p, float time) {

  float t=time*0.2f;//+curve(time, 1.3f)*7.0f;
  swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , rot(_sinf(t*1.3f)*0.5f-0.7f)));
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot(t)));
  
  return p;
}

__DEVICE__ float gao(float3 p, float3 r, float d, float time, inout float3 *atm, inout float3 *id) {
  return clamp(map(p+r*d, time, atm, id)/d,0.0f,1.0f)*0.5f+0.5f;  
}

__DEVICE__ float rnd(float2 uv) {
  return fract(dot(sin_f2(uv*521.744f+swi2(uv,y,x)*352.512f),to_float2_s(471.52f)));
}


__KERNEL__ void OmzgShaderRoyaleJipi425Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  fragCoord+=0.5f;

  float3 atm=to_float3_s(0);
  float3 id=to_float3_s(0); 

  float time=mod_f(iTime*0.3f, 300.0f);

  float2 uv = to_float2(fragCoord.x / iResolution.x, fragCoord.y / iResolution.y);
  uv -= 0.5f;
  uv /= to_float2(iResolution.y / iResolution.x, 1);
  
  //uv *= 1.0f+curve(time*3.0f-length(uv),0.7f)*0.2f; 

  float3 s=to_float3(curve(time, 0.7f)-0.5f,curve(time+7.2f, 0.8f)-0.5f,-8);
  float3 r=normalize(to_float3_aw(uv, 0.8f + curve(time, 1.7f)*1.4f));
  
  s=cam(s,time);
  r=cam(r,time);
  
  bool hit=false;
  
  float3 p=s;
  // raymarching loop
  for(int i=0; i<100; ++i) {
    
    float d=map(p,time, &atm, &id);
    if(d<0.001f) {
      hit=true;
      break;
    }
    if(d>100.0f) break;
    
    p+=r*d*0.8f;
    
  }
  
  float3 col=to_float3_s(0);
  if(hit) {
    float3 id2=id;
    float2 off=to_float2(0.01f,0);
    float3 n=normalize(map(p,time, &atm, &id)-to_float3(map(p+swi3(off,x,y,y),time, &atm, &id), map(p+swi3(off,y,x,y),time, &atm, &id), map(p+swi3(off,y,y,x),time, &atm, &id)));
    float3 l=normalize(to_float3(1,-3,2));
    if(dot(l,n)<0.0f) l=-l; // trick to have two opposite lights
    float3 h=normalize(l+r);
    float spec=_fmaxf(0.0f,dot(n,h));
    
    float fog=1.0f-clamp(length(p-s),0.0f,1.0f);
        
    // base shading
    float ao=gao(p,n,0.1f,time, &atm, &id)*gao(p,n,0.2f,time, &atm, &id)*gao(p,n,0.4f,time, &atm, &id)*gao(p,n,0.8f,time, &atm, &id);
    col += _fmaxf(0.0f,dot(n,l)) * (0.3f + _powf(spec,10.0f) + _powf(spec,50.0f)) * ao * 3.0f;
    
    // subsurface effect, didn't managed to make it very good
    for(float i=1.0f; i<15.0f; ++i) {
      float dist=i*0.07f;
      col += _fmaxf(0.0f,map(p+r*dist,time, &atm, &id)) * to_float3(0.5f+dist,0.5f,0.5f) * 0.8f * ao;
    }
    
    off.x=0.04f;
    float3 n2=normalize(map(p,time, &atm, &id)-to_float3(map(p+swi3(off,x,y,y),time, &atm, &id), map(p+swi3(off,y,x,y),time, &atm, &id), map(p+swi3(off,y,y,x),time, &atm, &id)));
    // outline effect (thanks FMS_Cat for the very great trick of difference between two normals with different offset size)
    col += to_float3(id2.x,id2.y*0.5f+0.4f,0.7f)*_powf(curve(time-id2.z, 0.7f),4.0f)*0.1f*length(n-n2);
    
    //col+=map(p-r*0.2f,time, &atm, &id)*1.0f; // quickest shading there is
  }
  
  // add glow
  col += pow_f3(atm*3.0f,to_float3_s(2.0f));
  
  // saturated colors becomes white
  col += _fmaxf(swi3(col,y,z,x)-1.0f,to_float3_s(0.0f));
  col += _fmaxf(swi3(col,z,x,y)-1.0f,to_float3_s(0.0f));
  
  // vignet
  col *= 1.2f-length(uv);
  
  #if ALL_COLORS
  if(time>18.0f) {
      float t4 = time*0.3f+uv.y*0.6f;
      if(time>30.0f) t4+=_floor(_fabs(uv.x+col.x*0.1f)*3.0f)*17.0f;
      swi2S(col,x,z, mul_f2_mat2(swi2(col,x,z),rot(t4)));
      swi2S(col,y,z, mul_f2_mat2(swi2(col,y,z),rot(t4*1.3f)));
      col=abs_f3(col);
  }
  #endif
  
  // "tonemapping"
  
  col=smoothstep(to_float3_s(0.0f),to_float3_s(1.0f),col);
  col=pow_f3(col, to_float3_s(0.4545f));
  
  #if FEEDBACK
  if(time>24.0f) {
      float2 uv2=fragCoord / iResolution;
      uv2-=0.5f;
      uv2*=0.92f+rnd(uv2)*0.03f;
      uv2+=0.5f;
      float3 c2=swi3(_tex2DVecN(iChannel0,uv2.x,uv2.y,15),x,y,z);
      float t3=0.0f;
      swi2S(c2,x,z, mul_f2_mat2(swi2(c2,x,z) , rot(0.05f+t3)));
      swi2S(c2,x,y, mul_f2_mat2(swi2(c2,x,y) , rot(0.02f+t3)));
      c2=abs_f3(c2);
      float fac=clamp(1.5f-length(uv)*1.3f,0.0f,1.0f);
      fac=_fminf(fac, _fmaxf(0.0f,_powf(fract(time*0.5f),2.0f)));
      col *= 0.3f+fac*0.7f;
      col += c2*0.9f*(1.0f-fac);
  }
  #endif
  
    fragColor = to_float4_aw(col, 1);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Shader made Live during OMZG Shader Royale (12/02/2021) in about 80m
// 1st place
// https://www.twitch.tv/videos/911443995?t=01h12m13s
// Code is in "Buffer A" so I can use "feedback" effects

__KERNEL__ void OmzgShaderRoyaleJipi425Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  float2 uv=fragCoord / iResolution;
  fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}