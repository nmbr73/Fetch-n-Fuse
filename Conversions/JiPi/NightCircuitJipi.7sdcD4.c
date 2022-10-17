
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define TAU _atan2f(1.0f,1.0f)*8.0f

__DEVICE__ void lookAt(inout float3 *rd,float3 ro,float3 ta,float3 up){
  float3 w=normalize(ta-ro),u=normalize(cross(w,up));
  *rd = (*rd).x*u + (*rd).y*cross(u,w) + (*rd).z*w;
}

__DEVICE__ void pointAt(inout float3 *p, float3 dir, float3 up){
  float3 u=normalize(cross(dir,up));
  *p = to_float3(dot(*p,u),dot(*p,cross(u,dir)),dot(*p,dir));
}

__DEVICE__ void rot(inout float3 *p,float3 a,float t){
  a=normalize(a);
  float3 u=cross(a,*p),v=cross(a,u);
  *p=u*_sinf(t)+v*_cosf(t)+a*dot(a,*p);
}


// https://www.shadertoy.com/view/WdfcWr
__DEVICE__ float2 pSFold(float2 p,float n){
    float h=_floor(_log2f(n)),a=TAU*_exp2f(h)/n;
    for(float i=0.0f;i<h+2.0f;i+=1.0f) {
      float2 v=to_float2(-_cosf(a),_sinf(a));
      float g=dot(p,v);
      p-=(g-_sqrtf(g*g+2e-3f))*v;
      a*=0.5f;
    }
    
    return p;  
}

#define seed 2576.0f
#define hash_f(p) fract(_sinf(p*12345.5f))
#define hash_f2(p) fract_f2(sin_f2(p*12345.5f))
#define hash_f3(p) fract_f3(sin_f3(p*12345.5f))

__DEVICE__ float3 randVec(float s)
{
  float2 n=hash_f2(to_float2(s,s+215.3f));
  return to_float3(_cosf(n.y)*_cosf(n.x),_sinf(n.y),_cosf(n.y)*_sinf(n.x));
}

__DEVICE__ float3 randCurve(float t,float n)
{
  float3 p = to_float3_s(0);
  for (int i=0; i<3; i++){
    float t2 = t*1.3f;
    p+=randVec(n+=365.0f)*_sinf((t2)+_sinf(t*0.6f)*0.5f);
  }
  return p;
}

__DEVICE__ float3 orbit(float t,float n, float iTime)
{
    float3 p = randCurve(-t*1.5f+iTime,seed)*5.0f;
    float3 off = randVec(n)*(t+0.05f)*0.6f;
    float time=iTime+hash_f(n)*5.0f;
    return p+off*_sinf(time+0.5f*_sinf(0.5f*time));
}



// rewrote 20/12/01
__DEVICE__ float2 sFold45(float2 p)
{
  float2 v=normalize(to_float2(1,-1));
  float g=dot(p,v);
  p-=(g-_sqrtf(g*g+5e-5f))*v;
  
  return p;
}

__DEVICE__ float stella(float3 p, float s)
{
  p = sqrt_f3(p*p+5e-5f); // https://iquilezles.org/articles/functions
  swi2S(p,x,z, sFold45(swi2(p,x,z)));
  swi2S(p,y,z, sFold45(swi2(p,y,z)));
  return dot(p,normalize(to_float3(1,1,-1)))-s;
}

/*
__DEVICE__ float stella(float3 p, float s)
{
    p=_fabs(p);
    if(p.x<p.z)p.xz=swi2(p,z,x);
    if(p.y<p.z)p.yz=swi2(p,z,y);
    return dot(p,normalize(to_float3(1,1,-1)))-s;
}
*/

__DEVICE__ float stellas(float3 p, float iTime)
{
    p.y-=-iTime;
    float c=2.0f;
    float3 e=_floor(p/c);
    e = sin_f3(11.0f*(2.5f*e+3.0f*swi3(e,y,z,x)+1.345f)); 
    p-=e*0.5f;
    p=mod_f(p,c)-c*0.5f;
    rot(&p,hash_f3(e+166.887f)-0.5f,iTime*1.5f); 
    return _fminf(0.7f,stella(p,0.08f));
}

__DEVICE__ float structure(float3 p, float iTime, inout float *g1, inout float *g2)
{
    float d=1e3,d0;
    for(int i=0;i<12;i++){
      float3 q=p,w=normalize(to_float3(_sqrtf(5.0f)*0.5f+0.5f,1,0)); 
        swi2S(w,x,y, swi2(w,x,y) * to_float2(i>>1&1,i&1)*2.0f-1.0f);
        
        //w=vec3[](w,swi3(w,y,z,x),swi3(w,z,x,y))[i%3];
        
        float3 _w[] = {w,swi3(w,y,z,x),swi3(w,z,x,y)};
        w = _w[i%3];
        
        pointAt(&q,w,-sign_f(w.x+w.y+w.z)*sign_f3(w)*swi3(w,z,x,y));
        
        d0=length(q-to_float3(0,0,clamp(q.z,2.0f,8.0f)))-0.4f+q.z*0.05f;
        d=_fminf(d,d0);
        *g2+=0.1f/(0.1f+d0*d0); // Distance glow by balkhan
        
        float c=0.8f;
        float e=_floor(q.z/c-c*0.5f);
        q.z-=c*clamp(round(q.z/c),3.0f,9.0f);
        
        q.z-=clamp(q.z,-0.05f,0.05f);
        swi2S(q,x,y, pSFold(swi2(q,x,y),5.0f));
        q.y-=1.4f-e*0.2f+_sinf(iTime*10.0f+e+(float)(i))*0.05f;
        q.x-=clamp(q.x,-2.0f,2.0f);
        q.y-=clamp(q.y,0.0f,0.2f);
        
        d0=length(q)*0.7f-0.05f;
        d=_fminf(d,d0);
        if(e==2.0f+_floor(mod_f(iTime*5.0f,7.0f)))
          *g1+=0.1f/(0.1f+d0*d0);
    }
    return d;
}

__DEVICE__ float rabbit(float3 p, float iTime, inout float *g3)
{
  float rrrrrrrrrrrrrrrrrrrrr;
    p-=randCurve(iTime,seed)*5.0f;
    rot(&p,to_float3_s(1),iTime);
    float d=stella(p,0.2f);
    *g3+=0.1f/(0.1f+d*d);
    return d;
}

__DEVICE__ float map(float3 p, float iTime, inout float *g1, inout float *g2, inout float *g3){
    return _fminf(min(stellas(p,iTime),structure(p, iTime, g1, g2)),rabbit(p, iTime, g3));
}

__DEVICE__ float3 calcNormal(float3 p, float iTime, inout float *g1, inout float *g2, inout float *g3)
{
  float3 n=to_float3_s(0);
  for(int i=0; i<4; i++){
    float3 e=0.001f*(to_float3(9>>i&1, i>>1&1, i&1)*2.0f-1.0f);
    n+=e*map(p+e, iTime,g1,g2,g3);
  }
  return normalize(n);
}

__DEVICE__ float3 doColor(float3 p, float iTime)
{
  if(stellas(p, iTime)<0.001f)return to_float3(0.7f,0.7f,1);
  return to_float3_s(1);
}

__DEVICE__ float3 hue(float h)
{
  return cos_f3((to_float3(0,2,-2)/3.0f+h)*TAU)*0.5f+0.5f;
}

__DEVICE__ float3 cLine(float3 ro, float3 rd, float3 a, float3 b)
{
  float3 ab=normalize(b-a),ao = a-ro;
  float d0=dot(rd,ab),d1=dot(rd,ao),d2=dot(ab,ao);
  float t = (d0*d1-d2)/(1.0f-d0*d0)/length(b-a);
  t= clamp(t,0.0f,1.0f);
  float3 p = a+(b-a)*t-ro;
  return to_float3(length(cross(p,rd)),dot(p,rd),t);
}


__DEVICE__ mat3 rotx(float a) { mat3 rot; rot.r0 = to_float3(1.0f, 0.0f, 0.0f);          rot.r1 = to_float3(0.0f, _cosf(a), -_sinf(a)); rot.r2 = to_float3(0.0f, _sinf(a), _cosf(a));  return rot; }
__DEVICE__ mat3 roty(float a) { mat3 rot; rot.r0 = to_float3(_cosf(a), 0.0f, _sinf(a));  rot.r1 = to_float3(0.0f, 1.0f, 0.0f);          rot.r2 = to_float3(-_sinf(a), 0.0f, _cosf(a)); return rot; }
__DEVICE__ mat3 rotz(float a) { mat3 rot; rot.r0 = to_float3(_cosf(a), -_sinf(a), 0.0f); rot.r1 = to_float3(_sinf(a), _cosf(a), 0.0f);  rot.r2 = to_float3(0.0f, 0.0f, 1.0f);          return rot; }


__DEVICE__ mat3 lookat2(float3 from, float3 to)
{
    float3 f = normalize(to - from);
    float3 _tmpr = normalize(cross(f, to_float3(0.0f, 1.0f, 0.0f)));
    float3 u = normalize(cross(_tmpr, f));
    float3 r = normalize(cross(u, f));
    return to_mat3_f3(r, u, f);
}


__KERNEL__ void NightCircuitJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
 
    CONNECT_COLOR0(Color1, 0.8f, 0.1f, 0.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.1f, 0.2f, 0.5f, 1.0f);
    CONNECT_COLOR2(ColorG1, 0.9f, 0.1f, 0.0f, 1.0f);
    CONNECT_COLOR3(ColorG2, 0.0f, 0.3f, 0.7f, 1.0f);
    CONNECT_COLOR4(ColorG3, 0.5f, 0.3f, 0.1f, 1.0f);
    
    CONNECT_SLIDER1(G1_fkt, 0.0f, 1.0f, 0.05f);
    CONNECT_SLIDER2(G2_fkt, 0.0f, 1.0f, 0.08f);
    CONNECT_SLIDER3(G3_fkt, 0.0f, 1.0f, 0.15f);
    
    //CONNECT_SLIDER0(ITR2, 0.0f, 100.0f, 40.0f);
 
    float g1=0.0f,g2=0.0f,g3=0.0f;

    float2 p = (2.0f * fragCoord - iResolution) / iResolution.y;
    float3 col=to_float3(0.0f,0.0f,0.05f);
   
    //float3 ro = to_float3(1, 0, int[](7,10,12,15)[int(_fabs(4.0f*_sinf(iTime*0.3f+3.0f*_sinf(iTime*0.2f))))&3]);

    int array[] = {7,10,12,15};
    float3 ro = to_float3(1, 0, array[(int)(_fabs(4.0f*_sinf(iTime*0.3f+3.0f*_sinf(iTime*0.2f))))&3]);
    
    rot(&ro,to_float3_s(1),iTime*0.2f);
    float3 ta = to_float3(2,1,2);
    float3 rd = normalize(to_float3_aw(p,2));
    
    
    float2 im = 2.0f * ((swi2(iMouse,x,y) / iResolution) - to_float2_s(0.5f));
    im.y *= 0.7f;
    
    //float3 rd = normalize(to_float3_aw(uv, 0.4f));
    
    float3 rp = to_float3(0.0f, 0.0f, -0.7f);//to_float3(0.0f, 0.7f, -0.7f);
    if(iMouse.z > 0.0f)
    {
      float3 _rp = rp;
      rp = mul_mat3_f3(roty(im.x) , rp);
      rp.y = (mul_mat3_f3(rotx(im.y) ,_rp)).y;
      rd = mul_mat3_f3(lookat2(rp, to_float3_s(0.0f)) , rd);
    }
    
    
    lookAt(&rd,ro,ta,to_float3(0,1,0));       
    float z=0.0f,d,i,ITR=50.0f;
    for(i=0.0f; i<ITR; i+=1.0f){
      z+=d=map(ro+rd*z, iTime, &g1,&g2,&g3);
      if(d<0.001||z>30.0f)break;
    }
    if(d<0.001f)
    {
      float3 p=ro+rd*z;
      float3 nor=calcNormal(p, iTime, &g1,&g2,&g3);
      float3 li=normalize(to_float3(1,1,-1));
      col=doColor(p, iTime);
      col*=_powf(1.0f-i/ITR,2.0f); 
      col*=clamp(dot(nor,li),0.3f,1.0f);
      col*=_fmaxf(0.5f+0.5f*nor.y,0.2f);
      //col+=to_float3(0.8f,0.1f,0.0f)*_powf(clamp(dot(reflect(normalize(p-ro),nor),normalize(to_float3(-1,-1,-1))),0.0f,1.0f),30.0f);
      //col+=to_float3(0.1f,0.2f,0.5f)*_powf(clamp(dot(reflect(normalize(p-ro),nor),normalize(to_float3(1,1,-1))),0.0f,1.0f),30.0f);
      col+=swi3(Color1,x,y,z)*_powf(clamp(dot(reflect(normalize(p-ro),nor),normalize(to_float3(-1,-1,-1))),0.0f,1.0f),30.0f);
      col+=swi3(Color2,x,y,z)*_powf(clamp(dot(reflect(normalize(p-ro),nor),normalize(to_float3(1,1,-1))),0.0f,1.0f),30.0f);
      col=_mix(to_float3_s(0),col,_expf(-z*z*0.00001f));
    }
    col+= swi3(ColorG1,x,y,z) * g1 * G1_fkt;// to_float3(0.9f,0.1f,0.0f)*g1*0.05f;
    col+= swi3(ColorG2,x,y,z) * g2 * G2_fkt;//to_float3(0.0f,0.3f,0.7f)*g2*0.08f;
    col+= swi3(ColorG3,x,y,z) * g3 * G3_fkt;//to_float3(0.5f,0.3f,0.1f)*g3*0.15f;
 
    // https://www.shadertoy.com/view/wtXSzX
    float3 de;
    ITR=40.0f;
    for(float i=0.0f; i<1.0f;i+=1.0f/7.0f)
    {
      de = to_float3_s(1e9);
      float off=hash_f(i*234.6f+256.0f);
      for(float j=0.0f;j<1.0f;j+=1.0f/ITR)
      {
        float t=j+off*0.5f;
        float3 c=cLine(ro,rd,orbit(t,off, iTime),orbit(t+1.0f/ITR,off, iTime));
        if (de.x*de.x*de.y>c.x*c.x*c.y)
        {
          de=c;
          de.z=j+c.z/ITR;
        }
      }
      float s = _powf(_fmaxf(0.0f,0.6f-de.z),2.0f)*0.1f;
      if(de.y>0.0f && z>de.y)
        col+=_mix(to_float3_s(1),hue(i),0.8f)*(1.0f-de.z*0.9f)*smoothstep(s+0.17f,s,de.x)*0.7f;
    }
    col = pow_f3(col,to_float3_s(0.8f+0.3f*_sinf(iTime*0.5f+3.0f*_sinf(iTime*0.3f))));
    //swi3(fragColor,x,y,z) = col;
    fragColor = to_float4_aw(col, Color1.w);


  SetFragmentShaderComputedColor(fragColor);
}