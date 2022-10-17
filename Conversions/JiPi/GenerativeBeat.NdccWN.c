
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define fx iResolution.x/iResolution.y
#define PI 3.14159235859f
__DEVICE__ float rdm(float p){
    p*=1234.56f;
    p = fract(p * 0.1031f);
    p *= p + 33.33f;
    return fract(2.0f*p*p);
}
__DEVICE__ float sm(float m1,float m2, float e){
  return smoothstep(m1,m2,e);
}
__DEVICE__ mat2 rotate2d(float _angle){
    return to_mat2(_cosf(_angle),-_sinf(_angle),
                   _sinf(_angle),_cosf(_angle));
} 
__DEVICE__ mat2 scale(float2 _scale){
    return to_mat2(_scale.x,0.0f,
                   0.0f,_scale.y);
}
__DEVICE__ float mapr(float _value,float _low2,float _high2) {
  float val = _low2 + (_high2 - _low2) * (_value - 0.0f) / (1.0f - 0.0f);
    //float val = 0.1f;
  return val;
}
__DEVICE__ float3 l2(float2 uv,float h, float iTime, float2 iResolution){

  float red =mapr(rdm(h+201.0f),0.0f,1.0f);
  float g =mapr(rdm(h+431.0f),0.0f,1.0f);
  float b =mapr(rdm(h+3023.0f),0.0f,1.0f);

  float spr =1.0f;
  float2 sp = to_float2(mapr(rdm(h+21.0f),-spr,spr),
                        mapr(rdm(h+4031.0f),-spr,spr));
  float3 cf = to_float3(red,g,b);

   
  float fr = mapr(rdm(h+453.0f),5.0f,30.0f);
  uv.x*=fx;
  uv-=to_float2_s(0.5f);
  uv=mul_f2_mat2(uv,rotate2d(rdm(h+324.0f)*PI*2.0f));
  uv+=to_float2_s(0.5f);
  uv =fract(uv*fr+to_float2_s(iTime)*sp);
   
  float2 p =to_float2(0.5f*fx,0.5f)-uv;
  float r = length(p);

  float ridx = _floor(mapr(rdm(h+4685.0f),0.0f,3.0f));

  float e = 0.0f; 

  if(ridx == 0.0f){
    e = 1.0f-sm(0.1f,0.2f,uv.x);
  }else if(ridx == 1.0f ){
     e = 1.0f-sm(0.1f,0.2f,uv.x);
     e+= 1.0f-sm(0.1f,0.2f,uv.y);
  }else if(ridx == 2.0f){
    float2 p =to_float2(0.5f,0.5f)-uv;
    float r = length(p);
    e = 1.0f-sm(0.1f,0.2f,r);
  }
   
   float3 dib = cf +_sinf(e*10.0f+iTime);
   return dib;
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}




__KERNEL__ void GenerativeBeatFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  //Blending
  CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
  CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
  CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
  CONNECT_POINT2(Par1, 0.0f, 0.0f);


  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float2 puv = uv;
  
  
  float t  = _floor(iTime*1.0f)*0.01f;
  float sr = t;
  float3 d1 = l2(uv,sr, iTime, iResolution);
 

  const int cnt = 5;

  float3 d1aux = d1;
  float3 fin = d1;
  for(int i =0; i<cnt;i++){
    
    float3 d2 = l2(uv,sr+120.0f+(float)(i)*4.0f, iTime, iResolution);
    float idx = (float)(i)/(float)(cnt)*PI*2.0f;
    //MASK CIRCLE : 
    float2 uv_c = uv;
    uv_c.x*=fx;

    float fase = rdm(sr*465.0f+(float)(i))*PI*2.0f;
    
    float fase2 = rdm(sr*465.0f+(float)(i))*PI*2.0f;

    float ampx = _sinf(rdm(sr*4587.0f+(float)(i)*535.0f)+iTime*0.02f)*0.15f+0.05f;
    float ampy = _cosf(rdm(sr*4587.0f+(float)(i)*535.0f)+iTime*0.02f)*0.15f+0.05f;
    float s = mapr(rdm(sr+(float)(i)*325.0f),0.1f,0.2f);
    float2 p = to_float2(0.5f*fx,0.5f) - uv_c;
    float r = length(p);
    float a = _atan2f(p.x,p.y);


    float amp_mof = mapr(rdm(sr*6384.0f+(float)(i)*5341.0f+t*10.0f),0.01f,0.08f);
    float mof = _sinf(a*5.0f+iTime)*amp_mof;
    float e = 0.0f;
    
    float ridx = _floor(mapr(rdm(sr+4685.0f+(float)(i)*579.0f*10.0f),0.0f,4.0f));
    
    if(i == cnt){
      ridx = 0.0f;
    }
    
    e = 1.0f-sm(s,s+0.8f,_sinf(r*10.0f+idx*40.0f+_sinf(iTime+idx))-mof);

    if((fin.x != d1aux.x || fin.y != d1aux.y || fin.z != d1aux.z) && e > 0.001f){
      fin = _mix(fin,d2,e);
    }else{
      float3 d3 = l2(uv,sr+620.0f+(float)(i)*57.0f, iTime, iResolution);
      fin = _mix(fin,d3,e);
    }
  }

  float prom = length(fin);
    
  float4 fb2 = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  float fb2_prom = (fb2.x+fb2.y+fb2.z)/3.0f;
  
  puv-=to_float2_s(0.5f+fb2_prom*0.001f);
  puv= mul_f2_mat2(puv,scale(to_float2_s(0.995f+fb2_prom*0.0001f)));
  puv+=to_float2_s(0.5f-fb2_prom*0.001f);
  
  float4 fb = _tex2DVecN(iChannel0,puv.x,puv.y,15);
    
  if(prom > 0.995f){
    fin = to_float3_s(0.0f);
  }
  
  float prom2 = (fin.x+fin.y+fin.z)/3.0f;
  
  fin = smoothstep(to_float3_s(0.1f),to_float3_s(0.9f),fin);
  if( prom2 < 0.2f){
    fin = swi3(fb,x,y,z)*0.992f;
  }
  
  fragColor = to_float4_aw(fin, 1.0f);

  if (Blend1>0.0) fragColor = Blending(iChannel1, uv, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void GenerativeBeatFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  fragCoord+=0.5f;

  float2 uv = fragCoord / iResolution;
  float2 puv = uv;
  
  
  float t  = _floor(iTime*1.0f)*0.01f;
  float sr = t;
  float3 d1 = l2(uv,sr, iTime, iResolution);
 

   const int cnt = 5;

   float3 d1aux = d1;
   float3 fin = d1;
   for(int i =0; i<cnt;i++){
    
    float3 d2 = l2(uv,sr+120.0f+(float)(i)*4.0f, iTime, iResolution);
    float idx = (float)(i)/(float)(cnt)*PI*2.0f;
    //MASK CIRCLE : 
    float2 uv_c = uv;
    uv_c.x*=fx;

    float fase = rdm(sr*465.0f+(float)(i))*PI*2.0f;
    
    float fase2 = rdm(sr*465.0f+(float)(i))*PI*2.0f;

    float ampx = _sinf(rdm(sr*4587.0f+(float)(i)*535.0f)+iTime*0.02f)*0.15f+0.05f;
    float ampy = _cosf(rdm(sr*4587.0f+(float)(i)*535.0f)+iTime*0.02f)*0.15f+0.05f;
    float s = mapr(rdm(sr+float(i)*325.0f),0.1f,0.2f);
    float2 p = to_float2(0.5f*fx,0.5f) - uv_c;
    float r = length(p);
    float a = _atan2f(p.x,p.y);


    float amp_mof = mapr(rdm(sr*6384.0f+(float)(i)*5341.0f+t*10.0f),0.01f,0.08f);
    float mof = _sinf(a*5.0f+iTime)*amp_mof;
    float e = 0.0f;
    
    float ridx = _floor(mapr(rdm(sr+4685.0f+(float)(i)*579.0f*10.0f),0.0f,4.0f));
    
    if(i == cnt){
      ridx = 0.0f;
    }
    
    e = 1.0f-sm(s,s+0.8f,_sinf(r*10.0f+idx*40.0f+_sinf(iTime+idx))-mof);


    if((fin.x != d1aux.x || fin.y != d1aux.y || fin.z != d1aux.z) && e > 0.001f){
      fin = _mix(fin,d2,e);
    }else{
      float3 d3 = l2(uv,sr+620.0f+(float)(i)*57.0f, iTime, iResolution);
      fin = _mix(fin,d3,e);
    }
  }

  float prom = length(fin);
  
  
  float4 fb2 = _tex2DVecN(iChannel0,uv.x,uv.y,15);
  float fb2_prom = (fb2.x+fb2.y+fb2.z)/3.0f;
  
  puv-=to_float2_s(0.5f+fb2_prom*0.001f);
  puv= mul_f2_mat2(puv,scale(to_float2_s(0.995f+fb2_prom*0.0001f)));
  puv+=to_float2_s(0.5f-fb2_prom*0.001f);
  
  float4 fb = _tex2DVecN(iChannel0,puv.x,puv.y,15);
    
  if(prom > 0.995f){
    fin = to_float3_s(0.0f);
  }
  
  float prom2 = (fin.x+fin.y+fin.z)/3.0f;
  
  fin = smoothstep(to_float3_s(0.1f),to_float3_s(0.9f),fin);
  if( prom2 < 0.2f){
    fin = swi3(fb,x,y,z)*0.992f;
  }
  
  fragColor = to_float4_aw(fin, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}