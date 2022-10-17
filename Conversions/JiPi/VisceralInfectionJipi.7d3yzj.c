
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Wood' to iChannel1
// Connect Image 'Texture: Pebbles' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
    Based on:
  Desert Canyon by shane
  https://www.shadertoy.com/view/Xs33Df
    
    I removed shane's comments to avoid passing them as my own. (I have no idea what I'm doing). 
    Please refer to shane's shader for extensive comments.
*/


//#define FAR 125.0f

#define RM_STEPS 128



__DEVICE__ mat2 rot2( float th ){ float2 a = sin_f2(to_float2(1.5707963f, 0) + th); return to_mat2(a.x, a.y, -a.y, a.x); }

__DEVICE__ float hash(float n){ return fract(_cosf(n)*45758.5453f); }
__DEVICE__ float hash(float3 p){ return fract(_sinf(dot(p, to_float3(7, 157, 113)))*45758.5453f); }

__DEVICE__ float getGrey(float3 p){ return dot(p, to_float3(0.299f, 0.587f, 0.114f)); }

__DEVICE__ float sminP(float a, float b , float s){
    
    float h = clamp(0.5f + 0.5f*(b - a)/s, 0.0f, 1.0f);
    return _mix(b, a, h) - h*(1.0f-h)*s;
}

__DEVICE__ float smaxP(float a, float b, float s){
  
    float h = clamp(0.5f + 0.5f*(a - b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f - h)*s;
}

__DEVICE__ float2 path(in float z, float ampA, float ampB, float freqA, float freqB){ 

    return to_float2(ampA*_sinf(z * freqA), ampB*_cosf(z * freqB) + 5.0f*(_sinf(z*0.025f)  - 5.0f)); 
}

__DEVICE__ float map(in float3 p, float iTime, __TEXTURE2D__ iChannel0, float ampA, float ampB, float freqA, float freqB, float ratio){
    
    p.x/=ratio;
    
    float tx = (-texture(iChannel0, swi2(p,x,z)/22.0f + swi2(p,x,y)/80.0f).x-0.4f) * 0.95f;
    
    float3 q = sin_f3(swi3(p,z,y,x)*0.2f - (0.5f+tx)*0.5f + iTime*0.13f)*sminP(5.0f, _powf((0.5f-tx)*5.5f, 0.22f), 3.0f);
    float h = q.x*q.y*q.z;
  
    swi2S(p,x,y, swi2(p,x,y) - path(p.z,ampA,ampB,freqA,freqB));
    float tnl = 2.0f - length(swi2(p,x,y)*to_float2(0.33f, 0.66f))*0.5f + h * 0.4f + (1.0f - tx)*0.25f;
    
    return tnl - tx*0.5f + tnl*0.3f; 
}

__DEVICE__ float getprot(in float3 p, float iTime, __TEXTURE2D__ iChannel0, float ratio){
    
    p.x/=ratio;
    
    float tx = (-texture(iChannel0, swi2(p,x,z)/22.0f + swi2(p,x,y)/80.0f).x-0.4f) * 0.95f;
    float3 q = sin_f3(swi3(p,z,y,x)*0.2f - (0.5f+tx)*0.5f + iTime*0.13f)*sminP(5.0f, _powf((0.5f-tx)*5.5f, 0.22f), 3.0f);
    
    return q.x*q.y*q.z;// + (1.0f - tx)*0.25f- tx*0.5f; 
}

// Log-Bisection Tracing
// https://www.shadertoy.com/view/4sSXzD
//
// Log-Bisection Tracing by nimitz (twitter: @stormoid)
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// Contact: nmz@Stormoid.com

__DEVICE__ float logBisectTrace(in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iChannel0, float ampA, float ampB, float freqA, float freqB, float FAR, float ratio){

    float t = 0.0f, told = 0.0f, mid, dn;
    float d = map(rd*t + ro, iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
    float sgn = sign_f(d);

    for (int i=0; i<RM_STEPS; i++){

        if (sign_f(d) != sgn || d < 0.001f || t > FAR) break;
 
        told = t;
           
        t += step(d, 1.0f)*(_logf(_fabs(d) + 1.1f) - d) + d;
        
        d = map(rd*t + ro,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
    }
    
    if (sign_f(d) != sgn){

        dn = sign_f(map(rd*told + ro,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio));
        
        float2 iv = to_float2(told, t); 

        for (int ii=0; ii<8; ii++){ 
            mid = dot(iv, to_float2_s(0.5f));
            float d = map(rd*mid + ro,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
            if (_fabs(d) < 0.001f)break;

            iv = _mix(to_float2(iv.x, mid), to_float2(mid, iv.y), step(0.0f, d*dn));
        }

        t = mid; 
    }
    
    return _fminf(t, FAR);
}

__DEVICE__ float3 normal(in float3 p, float iTime, __TEXTURE2D__ iChannel0, float ampA, float ampB, float freqA, float freqB, float ratio)
{  
  float2 e = to_float2(-1, 1)*0.001f;   
  return normalize(swi3(e,y,x,x)*map(p + swi3(e,y,x,x),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio) + swi3(e,x,x,y)*map(p + swi3(e,x,x,y),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio) + 
                   swi3(e,x,y,x)*map(p + swi3(e,x,y,x),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio) + swi3(e,y,y,y)*map(p + swi3(e,y,y,y),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio) );   
}

__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){
   
    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= (n.x + n.y + n.z );  
        
  return swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
}

__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 nor, float bumpfactor){
   
    const float eps = 0.001f;
       
    
    float3 grad = to_float3( getGrey(tex3D(tex, to_float3(p.x - eps, p.y, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y - eps, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y, p.z - eps), nor)));
    
    grad = (grad - getGrey(tex3D(tex, p, nor)))/eps; 
            
    grad -= nor*dot(nor, grad);          
                      
    return normalize(nor + grad*bumpfactor);
}

__DEVICE__ float calculateAO( in float3 p, in float3 n, float maxDist, float iTime, __TEXTURE2D__ iChannel0, float ampA, float ampB, float freqA, float freqB, float ratio)
{
  float ao = 0.0f, l;
  const float nbIte = 6.0f;

    for(float i=1.0f; i< nbIte+0.5f; i++){
    
        l = (i + hash(i))*0.5f/nbIte*maxDist;
        
        ao += (l - map( p + n*l,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio))/(1.0f + l);
    }
  
    return clamp(1.0f - ao/nbIte, 0.0f, 1.0f);
}

__DEVICE__ float curve(in float3 p, float iTime, __TEXTURE2D__ iChannel0, float ampA, float ampB, float freqA, float freqB, float ratio){

    const float eps = 0.05f, amp = 4.0f, ampInit = 0.5f;

    float2 e = to_float2(-1, 1)*eps; 
    
    float t1 = map(p + swi3(e,y,x,x),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio), t2 = map(p + swi3(e,x,x,y),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
    float t3 = map(p + swi3(e,x,y,x),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio), t4 = map(p + swi3(e,y,y,y),iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
    
    return clamp((t1 + t2 + t3 + t4 - 4.0f*map(p,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio))*amp + ampInit, 0.0f, 1.0f);
}


__KERNEL__ void VisceralInfectionJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_COLOR0(FogColor, 1.0f, 0.9f, 0.7f, 1.0f);
  CONNECT_COLOR1(Color1, 0.5f, 0.4f, 0.3f, 1.0f);
  CONNECT_COLOR2(Color2, 0.25f, 0.45f, 0.125f, 1.0f);
  CONNECT_COLOR3(Color3, 0.25f, 0.45f, 0.125f, 1.0f);
  CONNECT_COLOR4(Color4, 0.9f, 0.6f, 0.4f, 1.0f);
  
  
  CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
  CONNECT_SLIDER0(ViewZ, -50.0f, 50.0f, 0.0f);
  
  CONNECT_SLIDER1(freqA, -50.0f, 50.0f, 0.04f);
  CONNECT_SLIDER2(freqB, -50.0f, 50.0f, 0.0909f);
  CONNECT_SLIDER3(ampA, -50.0f, 50.0f, 20.0f);
  CONNECT_SLIDER4(ampB, -50.0f, 50.0f, 2.0f);
  
  //CONNECT_SLIDER5(tSize1, -50.0f, 50.0f, 0.16667f);
  CONNECT_SLIDER6(shd, -50.0f, 50.0f, 0.9f);
  CONNECT_SLIDER7(FAR, -50.0f, 500.0f, 125.0f);

  //const float freqA = 0.15f/3.75f;
  //const float freqB = 0.25f/2.75f;
  //const float ampA = 20.0f;
  //const float ampB = 2.0f;

  float2 u = (fragCoord - iResolution*0.5f)/iResolution.y;
  float ratio = iResolution.x/iResolution.y;
   
  float3 lookAt = to_float3(0, 0, 18.0f+iTime*1.9f);
  float3 ro = lookAt + to_float3(0, 0.0f, -0.25f) + to_float3_aw(ViewXY,ViewZ);

  swi2S(lookAt,x,y, swi2(lookAt,x,y) + path(lookAt.z,ampA,ampB,freqA,freqB));
  swi2S(ro,x,y, swi2(ro,x,y) + path(ro.z,ampA,ampB,freqA,freqB));

  float FOV = 3.14159f/1.5f; 
  float3 forward = normalize(lookAt - ro);
  float3 right = normalize(to_float3(forward.z, 0, -forward.x )); 
  float3 up = cross(forward, right);

  float3 rd = normalize(forward + FOV*u.x*right + FOV*u.y*up);
  
  swi2S(rd,x,y, mul_mat2_f2(rot2( path(lookAt.z,ampA,ampB,freqA,freqB).x/64.0f ) , swi2(rd,x,y)));
    
  float3 lp = to_float3(FAR*0.5f, FAR, FAR) + to_float3(0, 0, ro.z);

  float t = logBisectTrace(ro, rd,iTime,iChannel0,ampA,ampB,freqA,freqB, FAR,ratio);
    
  float3 fog = swi3(FogColor,x,y,z);//to_float3(1, 0.9f, 0.7f);
  //vec3 fog = to_float3(1, 0.3f, 0.3f);

  float3 col = fog;
  
  if (t < FAR){
      float3 sp = ro+t*rd;
      float3 sn = normal(sp,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);

      float3 ld = lp-sp;
      ld /= _fmaxf(length(ld), 0.001f); 
  
      const float tSize1 = 1.0f/6.0f;

      sn = doBumpMap(iChannel1, sp*tSize1, sn, 0.007f/(1.0f + t/FAR));
      
      //float shd = 0.9f;
      float curv = curve(sp,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio)*0.7f +0.1f;
      float ao = calculateAO(sp, sn, 10.0f,iTime,iChannel0,ampA,ampB,freqA,freqB,ratio);
      
      float dif = _fmaxf( dot( ld, sn ), 0.0f); 
      float spe = _powf(_fmaxf( dot( reflect(-ld, sn), -rd ), 0.0f ), 5.0f)*2.0f; 
      float fre = clamp(1.0f + dot(rd, sn), 0.0f, 1.0f);

      float Schlick = _powf( 1.0f - _fmaxf(dot(rd, normalize(rd + ld)), 0.0f), 1.5f);
      float fre2 = _mix(0.2f, 1.0f, Schlick);
       
      float amb = fre*fre2 + 0.76f*ao;
      
      //col = clamp(_mix(to_float3(0.5f, 0.4f, 0.3f), to_float3(0.25f, 0.45f, 0.125f),(sp.y+1.0f)*0.15f), to_float3(0.25f, 0.45f, 0.125f), to_float3_s(1));
      col = clamp(_mix(swi3(Color1,x,y,z), swi3(Color2,x,y,z),(sp.y+1.0f)*0.15f), swi3(Color3,x,y,z), to_float3_s(1));
      
      curv = smoothstep(0.0f, 0.7f, curv);
      col *= to_float3(curv*0.9f, curv*0.45f, curv*0.25f)*1.0f;
      
      float prot = smoothstep(-0.0f, -3.0f, getprot(sp,iTime,iChannel0,ratio));
      
      col = _mix(col, swi3(Color1,x,y,z), prot);//to_float3(0.9f, 0.6f, 0.4f), prot);
      col = (col*(dif + 0.1f) + fre2*spe)*shd*ao + amb*col; 
    }
    
    col = _mix(col, fog, _sqrtf(smoothstep(FAR - 85.0f, FAR, t)));
    col = pow_f3(_fmaxf(col, to_float3_s(0.0f)), to_float3_s(0.85f));
    u = fragCoord/iResolution;
    col *= _powf(16.0f*u.x*u.y*(1.0f - u.x)*(1.0f - u.y) , 0.0625f);

    fragColor = to_float4_aw(clamp(col, 0.0f, 1.0f), 1);

  SetFragmentShaderComputedColor(fragColor);
}