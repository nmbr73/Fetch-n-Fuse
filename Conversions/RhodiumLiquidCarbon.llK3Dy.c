
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// ***********************************************************
// Alcatraz / Rhodium 4k Intro liquid carbon
// by Jochen "Virgill" Feldkötter
//
// 4kb executable: http://www.pouet.net/prod.php?which=68239
// Youtube: https://www.youtube.com/watch?v=YK7fbtQw3ZU
// ***********************************************************

#define time iTime
#define res iResolution

//float bounce;

// signed box
__DEVICE__ float sdBox(float3 p,float3 b)
{
  float3 d=abs_f3(p)-b;
  return _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f)+length(_fmaxf(d,to_float3_s(0.0f)));
}

// rotation
__DEVICE__ float2 pR(float2 p, float a) 
{
  p=_cosf(a)*p+_sinf(a)*to_float2(p.y,-p.x);
  return p;
}

// 3D noise function (IQ)
__DEVICE__ float noise(float3 p)
{
    float3 ip=_floor(p);
    p-=ip; 
    float3 s=to_float3(7,157,113);
    float4 h=to_float4(0.0f,s.y,s.z,s.y+s.z)+dot(ip,s);
    p=p*p*(3.0f-2.0f*p); 
    h=_mix(fract_f4(sin_f4(h)*43758.5f),fract_f4(sin_f4(h+s.x)*43758.5f),p.x);
    swi2S(h,x,y, _mix(swi2(h,x,z),swi2(h,y,w),p.y));
    return _mix(h.x,h.y,p.z); 
}

__DEVICE__ float map(float3 p, float bounce, float iTime)
{  
    p.z-=1.0f;
    p*=0.9f;
    float2 _p = pR(swi2(p,y,z),bounce*1.0f+0.4f*p.x);
    p.y=_p.x; p.z=_p.y;
    return sdBox(p+to_float3(0,_sinf(1.6f*time),0),to_float3(20.0f, 0.05f, 1.2f))-0.4f*noise(8.0f*p+3.0f*bounce);
}

//  normal calculation
__DEVICE__ float3 calcNormal(float3 pos, float bounce, float iTime)
{
  float eps=0.0001f;
  float d=map(pos,bounce,iTime);
  return normalize(to_float3(map(pos+to_float3(eps,0,0),bounce,iTime)-d,map(pos+to_float3(0,eps,0),bounce,iTime)-d,map(pos+to_float3(0,0,eps),bounce,iTime)-d));
}


//   standard sphere tracing inside and outside
__DEVICE__ float castRayx(float3 ro,float3 rd, float bounce, float iTime) 
{
  float function_sign = (map(ro,bounce,iTime)<0.0f)?-1.0f:1.0f;
  float precis=0.0001f;
  float h=precis*2.0f;
  float t=0.0f;
  for(int i=0;i<120;i++) 
  {
    if(_fabs(h)<precis||t>12.0f)break;
    h=function_sign*map(ro+rd*t,bounce,iTime);
    t+=h;
  }
    return t;
}

//   refraction
__DEVICE__ float refr(float3 pos,float3 lig,float3 dir,float3 nor,float angle,out float *t2, out float3 *nor2, float bounce, float iTime)
{
  float h=0.0f;
  *t2=2.0f;
  float3 dir2=refract_f3(dir,nor,angle);  
  for(int i=0;i<50;i++) 
  {
    if(_fabs(h)>3.0f) break;
    h=map(pos+dir2* *t2,bounce,iTime);
    *t2-=h;
  }
  *nor2=calcNormal(pos+dir2* *t2,bounce,iTime);
  return(0.5f*clamp(dot(-lig,*nor2),0.0f,1.0f)+_powf(_fmaxf(dot(reflect(dir2,*nor2),lig),0.0f),8.0f));
}

//  softshadow 
__DEVICE__ float softshadow(float3 ro,float3 rd, float bounce, float iTime) 
{
  float sh=1.0f;
  float t=0.02f;
  float h=0.0f;
  for(int i=0;i<22;i++)  
  {
        if(t>20.0f)continue;
        h=map(ro+rd*t,bounce,iTime);
        sh=_fminf(sh,4.0f*h/t);
        t+=h;
    }
    return sh;
}

//  main function
__KERNEL__ void RhodiumLiquidCarbonFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(BaseColor, 1.0f, 0.25f, 0.0625f, 1.0f);
  //CONNECT_COLOR0(BaseColor, (1.0f/1.0f) ,  (1.0f/4.0f) , (1.0f/16.0f), 1.0f);
  
  CONNECT_SLIDER0(MixTex, 0.0f, 1.0f, 1.0f);
  CONNECT_SLIDER1(Mix2, 0.0f, 1.0f, 1.0f);
  CONNECT_SLIDER2(Threshold, -1.0f, 30.0f, 12.0f);
    
  float bounce=_fabs(fract(0.05f*time)-0.5f)*20.0f; // triangle function
    
  float2 uv=fragCoord/iResolution; 
  float2 p=uv*2.0f-1.0f;
   
//   bouncy cam every 10 seconds
  float wobble=(fract(0.1f*(time-1.0f))>=0.9f)?fract(-time)*0.1f*_sinf(30.0f*time):0.;
    
//  camera    
  float3 dir = normalize(to_float3_aw(2.0f*fragCoord-swi2(res,x,y), res.y));
  float3 org = to_float3(0,2.0f*wobble,-3.0f);  
    

//   standard sphere tracing:
  float3 color = to_float3_s(0.0f);
  float3 color2 =to_float3_s(0.0f);
  float t = castRayx(org,dir,bounce,iTime);
  float3 pos=org+dir*t;
  float3 nor=calcNormal(pos,bounce,iTime);

//   lighting:
  float3 lig=normalize(to_float3(0.2f,6.0f,0.5f));
//  scene depth    
  float depth=clamp((1.0f-0.09f*t),0.0f,1.0f);
    

  float4 tex = texture(iChannel0, uv);
  float4 tex2 = texture(iChannel1, uv);
   
    
  //float3 pos2 = swi3(tex,x,y,z);//to_float3_s(0.0f);
  float3 nor2 = to_float3_s(0.0f);
  //if(t<12.0f)
  if(t<Threshold)
  {
      color2 = to_float3_s(_fmaxf(dot(lig,nor),0.0f)  +  _powf(_fmaxf(dot(reflect(dir,nor),lig),0.0f),16.0f));
      color2 *=clamp(softshadow(pos,lig,bounce,iTime),0.0f,1.0f);  // shadow              
      float t2;
      swi3S(color2,x,y,z, swi3(color2,x,y,z) + refr(pos,lig,dir,nor,0.9f, &t2, &nor2,bounce,iTime)*depth);
      color2 -= clamp(0.1f*t2,0.0f,1.0f);        // inner intensity loss
  } 
else color2 = to_float3_s(1.0f);
  

  float tmp = 0.0f;
  float T = 1.0f;

//  animation of glow intensity    
  float intensity = 0.1f*-_sinf(0.209f*time+1.0f)+0.05f; 
  for(int i=0; i<128; i++)
  {
      float density = 0.0f; float nebula = noise(org+bounce);
      density=intensity-map(org+0.5f*nor2,bounce,iTime)*nebula;
      if(density>0.0f)
      {
        tmp = density / 128.0f;
        T *= 1.0f -tmp * 100.0f;
        if( T <= 0.0f) break;
      }
      org += dir*0.078f;
  }    
  
  
  
  //float3 basecol=to_float3(1.0f/1.0f ,  1.0f/4.0f , 1.0f/16.0f);
  float3 basecol = swi3(BaseColor,x,y,z);
     
  basecol = _mix(swi3(tex,x,y,z), basecol, MixTex);
  
  T=clamp(T,0.0f,1.5f); 
  color += basecol* _expf(4.0f*(0.5f-T) - 0.8f);
  
  color2 = _mix(color2, swi3(tex,x,y,z), Mix2);
  
  color2*=depth;
  color2+= (1.0f-depth)*noise(6.0f*dir+0.3f*time)*0.1f;  // subtle mist


//  scene depth included in alpha channel
  fragColor = to_float4_aw((1.0f*color+0.8f*color2)*1.3f,_fabs(0.67f-depth)*2.0f+4.0f*wobble);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'https://soundcloud.com/virgill/4klang-rhodium' to iChannel1


// ***********************************************************
// Alcatraz / Rhodium 4k Intro liquid carbon
// by Jochen "Virgill" Feldkötter
//
// 4kb executable: http://www.pouet.net/prod.php?which=68239
// Youtube: https://www.youtube.com/watch?v=YK7fbtQw3ZU
// ***********************************************************

//#define time iTime
//#define res iResolution



//   simplyfied version of Dave Hoskins blur
__DEVICE__ float3 dof(__TEXTURE2D__ tex,float2 uv,float rad, float2 iResolution)
{
  const float GA =2.399f; 
  const mat2 rot = to_mat2(_cosf(GA),_sinf(GA),-_sinf(GA),_cosf(GA));

  float3 acc=to_float3_s(0);
  float2 pixel=to_float2(0.002f*res.y/res.x,0.002f),angle=to_float2(0,rad);;
  rad=1.0f;
  for (int j=0;j<80;j++)
  {  
    rad += 1.0f/rad;
    angle = mul_f2_mat2(angle,rot);
    float4 col=texture(tex,uv+pixel*(rad-1.0f)*angle);
    acc+=swi3(col,x,y,z);
  }
  return acc/80.0f;
}

//-------------------------------------------------------------------------------------------
__KERNEL__ void RhodiumLiquidCarbonFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  float2 uv = fragCoord / iResolution;
  fragColor=to_float4_aw(dof(iChannel0,uv,_tex2DVecN(iChannel0,uv.x,uv.y,15).w, iResolution),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}