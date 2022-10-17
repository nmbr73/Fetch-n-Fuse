
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//increase R as much as your computer can take. 4-5 is usually sufficient though.
//setting R to 10 is fun, albeit slow
const float R=4.0f;
const float R2=8.0f;

__DEVICE__ mat2 rot2(float a) {
  float c = _cosf(a);
  float s = _sinf(a);
  return to_mat2(c, s,-s, c);
}
__DEVICE__ float box(float2 p,float2 b)
{
    float2 d = abs_f2(p) - b;
    return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}
__DEVICE__ float rotatedBox(float2 p, float2 b, float time)
{
  float2 q=p;
  swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y),rot2(time)));
  return box(q,b);
}
__DEVICE__ float circle(float2 p,float r)
{
  return length(p)-r;
}
__DEVICE__ float edge(float2 p)
{   
  float d=0.5f-length(p+to_float2_s(-0.5f));
  d=_fmaxf(d,0.3f-length(p-to_float2_s(0.0f)));
  return d;
}
__DEVICE__ float grid(float2 p)
{
  float gS=130.0f;
  float2 q=gS*(fract_f2(p/gS)-0.5f);
  return length(q)-gS/5.0f;
}
__DEVICE__ float items(float2 p,float time)
{
  float d=box(p-to_float2(80.0f,60.0f),to_float2(40.0f,20.0f));
  d=_fminf(d,circle(p-to_float2(230.0f+40.0f*_sinf(time),100.0f+20.0f*_sinf(time)),30.0f));
  d=_fminf(d,circle(p-to_float2(400.0f,180.0f),80.0f));
  d=_fmaxf(d,-circle(p-to_float2(400.0f,180.0f),40.0f));
  d=_fmaxf(d,-rotatedBox(p-to_float2(400.0f,180.0f),to_float2(90.0f,20.0f),time));
  d=_fminf(d,rotatedBox(p-to_float2(135.0f,115.0f),to_float2(20.0f,80.0f),-0.7f));
  d=_fminf(d,circle(p-to_float2(800.0f,500.0f),100.0f));
  d=_fminf(d,circle(p-to_float2(300.0f,550.0f),30.0f));
  d=_fminf(d,box(p-to_float2(1000.0f,400.0f+300.0f*_sinf(time*0.1f)),to_float2(50.0f,20.0f)));
  return d;
}
__DEVICE__ float map(float2 p, float time)
{
  float d=items(p,time);
  return d;
}
__DEVICE__ float2 mapNormal(float2 p, float t)
{
  float2 eps = to_float2(0.0f, 0.001f);
  float2 normal = normalize(to_float2(
              map(p + swi2(eps,y,x),t) - map(p - swi2(eps,y,x),t),
              map(p + swi2(eps,x,y),t) - map(p - swi2(eps,x,y),t)));
    return normal;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//particle handling
__DEVICE__ float4 getParticle(float2 p, float R, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float4 mass=to_float4(0.0f,0.0f,0.0f,0.0f);
    float s=0.8f;//modify this value. 0.5f is default. 1.0f is big; above 1 is interesting ex. 2, 3
    float nh=0.0f;//modify this as well. 0.0f is default.
    float t=0.0f;
    for(float i=-R;i<=R;i++){
      for(float j=-R;j<=R;j++){
        float4 prt = texture(iChannel0,(p+to_float2(j,i))/iResolution);
        if(prt.x!=-1.0f || prt.y!=-1.0f || prt.z!=-1.0f || prt.w!=-1.0f){
          if( _fabs(prt.x+prt.z-j)<=s && _fabs(prt.y+prt.w-i)<=s ){
              mass+=to_float4(prt.x+prt.z-j,prt.y+prt.w-i,prt.z,prt.w);
              t++;
          }
        }
      }
    }
    if(t>nh){
      return mass/t;
    }
    return to_float4_s(-1.0f);
}

#define _reflect(I,N) (I-2.0f*dot(N,I)*N)

__KERNEL__ void FireWithUniversalGravityFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER0(R, 0.0f, 10.0f, 4.0f);
    CONNECT_SLIDER1(R2, 0.0f, 20.0f, 8.0f);
  
    fragCoord+=0.5f;

    //increase R as much as your computer can take. 4-5 is usually sufficient though.
    //setting R to 10 is fun, albeit slow
//    const float R=4.0f;
//    const float R2=8.0f;

    float2 uv = fragCoord/iResolution;
    if(iFrame<=5 || Reset){
      fragColor = to_float4_s(0.0f);
    }else{
        if(length(to_float2(fragCoord.x-iMouse.x,fragCoord.y-iMouse.y))<20.&&iMouse.z > 0.0f){
            float2 v=-R*normalize(to_float2(fragCoord.x-iMouse.x+3.0f*_sinf(iTime*100.0f),fragCoord.y-iMouse.y+3.0f*_sinf(iTime*251.0f)));
            fragColor = to_float4(0.0f,0.0f,v.x,v.y);
        }else{
          float4 p=getParticle(fragCoord,R, iResolution, iChannel0);
            if(map(fragCoord,iTime)>=-R&&p.x!=-1.0f){
                float2 rez=1.0f/iResolution;
                float2 n=mapNormal(fragCoord+swi2(p,x,y),iTime);
                //gravity
                float2 ac=n*0.1f/(1.0f+0.001f*map(swi2(p,x,y),iTime));
                float2 nv=to_float2(clamp(0.999f*p.z+ac.x,-R,R),clamp(0.999f*p.w+ac.y,-R,R));
                if(map(fragCoord+swi2(p,x,y),iTime)<=0.0f){
                    //the normal vector can be made more accurate here by getting the exact collision
                    swi2S(p,z,w, 1.1f*_reflect(to_float2(p.z,p.w),n));
                    nv=to_float2(clamp(p.z,-R,R),clamp(p.w,-R,R));
                }
                fragColor = to_float4(p.x,p.y,nv.x,nv.y);
            }else{
                fragColor = to_float4_s(-1.0f);
            }
        }
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1



__KERNEL__ void FireWithUniversalGravityFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX1(Textur, 0); 
    CONNECT_SLIDER0(R, 0.0f, 10.0f, 4.0f);
    CONNECT_SLIDER1(R2, 0.0f, 20.0f, 8.0f);
    CONNECT_SLIDER2(str, 0.0f, 2.0f, 0.8f);
float BBBBBBBBBBBBB;

    fragCoord+=0.5f;

    //increase R as much as your computer can take. 4-5 is usually sufficient though.
    //setting R to 10 is fun, albeit slow
    //const float R=4.0f;
    //const float R2=8.0f;

    //trails
    //const float str=0.8f; //good values are 0.8f,0.99f,-.1

    float2 uv = fragCoord/iResolution;
    fragColor = to_float4_aw(swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z)*str,1.0f);
    float4 tex = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    if(tex.x!=-1.0f || tex.y!=-1.0f || tex.z!=-1.0f || tex.w!=-1.0f){
      fragColor +=1.0f;
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


#define rMode 0
//#define rMode 1
//#define rMode 2
//#define rMode 3
//#define rMode 4

#define shadow false
//#define shadow true

//IMPORTANT. IF IT DOESNT WORK THEN GO TO BUFFER A AND SET THE BUFFER TO CLAMP AND THEN REPEAT
__DEVICE__ float numPart(float2 p, float R2, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float t=0.0f;
    float t1=0.0f;
    for(float i=-R2;i<=R2;i++){
      for(float j=-R2;j<=R2;j++){
        float w=_powf(3.0f,-length(to_float2(i,j)/R2));
        if(length(to_float2(i,j))>R2){
          continue;
        }
        if(texture(iChannel0,p+(to_float2(j,i)/iResolution)).x!=-1.0f){
        //if(texture(iChannel1,p+(to_float2(j,i)/iResolution)).x>0.1f){
          t+=w;
        }
        t1+=w;
      }
    }
    return t/t1;
}

__DEVICE__ bool hasPart(float2 p, float R2, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    for(float i=-R2;i<=R2;i++){
      for(float j=-R2;j<=R2;j++){
        if(length(to_float2(i,j))>R2){
          continue;
        }
        
        
        //if(texture(iChannel0,p+(to_float2(j,i)/iResolution))!=to_float4(-1.0f)){
        float4 tex = texture(iChannel0,p+(to_float2(j,i)/iResolution));
        if(tex.x!=-1.0f || tex.y!=-1.0f || tex.z!=-1.0f || tex.w!=-1.0f){
          return true;
        }
      }
    }
    return false;
}

//convert HSV to RGB
__DEVICE__ float3 hsv2rgb(float3 c){
  float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
  return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

__DEVICE__ float3 bg(float2 p)
{
  return to_float3_s(0.5f*mod_f(_ceil(p.x) + _ceil(p.y), 2.0f));
}

__DEVICE__ float3 render(float2 uv, float2 p, float R2, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1){
float zzzzzzzzzzzzzzz;    
    float m=map(p,iTime);
    if(m<0.0f){
      if(m>-10.0f){
        float2 tnorm = normalize(to_float2(0.5f,1.0f));
        return to_float3_s(0.5f+0.4f*dot(mapNormal(p,iTime),normalize(to_float2(0.5f,1.0f))));
        //(return to_float3(0.5f+0.4f*dot(mapNormal(p,iTime)),tnorm.x,tnorm.y);
      }else{
        return to_float3_s(0.5f);
      }
    }else{
      float mc=1.0f;
      if(shadow){
        //mc=1.0f-(1.0f/(0.05f*m+1.0f));
          mc=1.0f-(1.0f/(0.1f*m+1.0f));
      }
      if(rMode==3||rMode==4){
        float h=numPart(uv,R2,iResolution,iChannel0);
        if(rMode==3){
          return mc*_mix(to_float3_s(0.0f),hsv2rgb(to_float3(h*0.5f,1.0f,1.0f)),clamp(h,0.0f,1.0f));
        }
         //return mc*_mix(to_float3_s(0.0f),hsv2rgb(to_float3(h/20.0f,1.0f,1.0f)),clamp(h*10.0f,0.0f,1.0f));
        //return to_float3_aw(numPart(uv,R2,iResolution,iChannel0));
        //return mc*to_float3(step(0.2f,h));
        }else{
            float h=_tex2DVecN(iChannel1,uv.x,uv.y,15).x;
            float3 sh=to_float3_aw(1.0f/iResolution,0.0f);
            h+=texture(iChannel1,uv+swi2(sh,x,z)).x+texture(iChannel1,uv+swi2(sh,z,y)).x
             +texture(iChannel1,uv-swi2(sh,x,z)).x+texture(iChannel1,uv-swi2(sh,z,y)).x;
            h/=5.0f;
            float rs=10.0f; //good values are 10, 20, 30
            if(rMode==0){
                return mc*_mix(bg(p*4.0f),hsv2rgb(to_float3(h/20.0f,1.0f,1.0f)),h);
            }else if(rMode==1){
                return mc*_mix(to_float3_s(0.0f),hsv2rgb(to_float3(h/20.0f,1.0f,1.0f)),1.0f-h);
            }else if(rMode==2){
                return mc*_mix(to_float3_s(0.0f),hsv2rgb(to_float3((h/20.0f)+0.6f,1.0f,1.0f)),h);
            }
        }
    }
    
}

__KERNEL__ void FireWithUniversalGravityFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    //increase R as much as your computer can take. 4-5 is usually sufficient though.
    //setting R to 10 is fun, albeit slow
    const float R=4.0f;
    const float R2=8.0f;
float IIIIIIIIIIIIIIIII;
    float2 uv = fragCoord/iResolution;
    float3 c = (render(uv,fragCoord, R2, iTime, iResolution, iChannel0, iChannel1));
    fragColor = to_float4_aw(c,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}