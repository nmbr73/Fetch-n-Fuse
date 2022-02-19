
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define time iTime
#define size iResolution

//float pixelSize,focalDistance,aperture,fudgeFactor=0.6f;//,shadowCone=0.5f;

__DEVICE__ mat2 rmat(float a){float sa=_sinf(a),ca=_cosf(a);return to_mat2(ca,sa,-sa,ca);}

//float3 mcol=to_float3_s(0);
//const float mr=0.16f, mxr=1.0f; 
//const float4 scale=to_float4(-2.0f,-2.0f,-2.0f,2.0f); 
//float4 p0=to_float4(3.0f,0.76f,1.12f,0.2f);//0.32f,.76
//float lightPos; 

__DEVICE__ float2 DE(in float3 z0, float iTime, inout float3 *mcol, float mr, float mxr, float4 scale, inout float4 *p0, float lightPos){//amazing surface by kali/tglad with mods
 (*p0).x=_cosf((iTime+z0.y)*0.05f)*3.5f;
 swi2S(z0,x,z, mul_f2_mat2(swi2(z0,x,z),rmat(z0.y*0.07f)));
 z0.y=_fabs(mod_f(z0.y,4.0f)-2.0f);
 float4 _z = to_float4_aw(z0,1.0f); float dL=100.0f;
 for (int n = 0; n < 4; n++) { 
  if(_z.x<_z.z)  swi2S(_z,x,z,swi2(_z,z,x)); 
  //z.xy=clamp(swi2(z,x,y), -1.0f, 1.0f) *2.0f-swi2(z,x,y); 
  _z.x=clamp(_z.x, -1.0f, 1.0f) *2.0f-_z.x; 
  _z.y=clamp(_z.y, -1.0f, 1.0f) *2.0f-_z.y; 
  _z*=scale/clamp(_fmaxf(dot(swi2(_z,x,y),swi2(_z,x,y)),dot(swi2(_z,x,z),swi2(_z,x,z))),mr,mxr); 
  _z+=*p0; 
  if(n==1)dL=length(swi3(_z,x,y,z)+to_float3(0.5f,lightPos,0.5f))/_z.w;
 } 
 if((*mcol).x>0.0f) *mcol+=to_float3_s(0.6f)+sin_f3(swi3(_z,x,y,z)*0.1f)*0.4f; 
 swi3S(_z,x,y,z, abs_f3(swi3(_z,x,y,z))-to_float3(1.4f,32.8f,0.7f)); 
 return to_float2(_fmaxf(_z.x,_fmaxf(_z.y,_z.z))/_z.w,dL); 
} 

__DEVICE__ float CircleOfConfusion(float t, float pixelSize, float focalDistance, float aperture){//calculates the radius of the circle of confusion at length t
 return _fmaxf(_fabs(focalDistance-t)*aperture,pixelSize*(1.0f+t));
}
__DEVICE__ mat3 lookat(float3 fw,float3 up){
 fw=normalize(fw);float3 rt=normalize(cross(fw,normalize(up)));return to_mat3_f3(rt,cross(rt,fw),fw);
}
__DEVICE__ float linstep(float a, float b, float t){return clamp((t-a)/(b-a),0.0f,1.0f);}// i got this from knighty and/or darkbeam
//random seed and generator

__DEVICE__ float rand2(inout float2 *randv2){// implementation derived from one found at: lumina.sourceforge.net/Tutorials/Noise.html

 *randv2+=to_float2(1.0f,1.0f);
 return fract(_sinf(dot(*randv2 ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}
__DEVICE__ float3 bg(float3 rd){
  float d=_fmaxf(0.0f,rd.x+rd.y+rd.z);
  return to_float3_s(d*d*0.25f)+rd*0.05f;
}

__KERNEL__ void MachineDnaFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
 float fudgeFactor=0.6f;
 float3 mcol=to_float3_s(0);
 const float mr=0.16f, mxr=1.0f; 
 const float4 scale=to_float4(-2.0f,-2.0f,-2.0f,2.0f); 
 float4 p0=to_float4(3.0f,0.76f,1.12f,0.2f);//0.32f,.76

 float2 randv2=fract_f2(cos_f2((fragCoord+fragCoord*to_float2(100.0f,100.0f))+to_float2_s(time)*10.0f)*1000.0f);
 float pixelSize=1.0f/size.y;
 float tim=time*0.1f;//camera, lighting and object setup
 float lightPos=_sinf(tim*20.0f)*5.0f; 
 float3 ro=to_float3(_cosf(tim),tim*2.0f,_sinf(tim))*5.0f; 
 float3 rd=mul_mat3_f3(lookat(to_float3(-ro.x,5.0f,-ro.z),to_float3(0.0f,1.0f,1.0f)),normalize(to_float3_aw((2.0f*fragCoord-swi2(size,x,y))/size.y,2.0f))); 
 float focalDistance=_fminf(length(ro)+0.001f,1.0f);
 float aperture=0.007f*focalDistance;
 float3 rt=normalize(cross(to_float3(0,1,0),rd)),up=cross(rd,rt);//just need to be perpendicular
 float3 lightColor=to_float3(1.0f,0.5f,0.25f)*2.0f;
 float4 col=to_float4_s(0.0f);float3 blm=to_float3_s(0);//color accumulator, .w=alpha, bloom accum
 float2 D;//for surface and light dist
 float t=0.0f,mld=100.0f,od,d=1.0f,old,ld=100.0f,dt=0.0f,ot;//distance traveled, minimum light distance
 for(int i=1;i<72;i++){//march loop
  if(col.w>0.9f || t>15.0f)break;//bail if we hit a surface or go out of bounds
  float rCoC=CircleOfConfusion(t,pixelSize,focalDistance,aperture);//calc the radius of CoC
  od=D.x;old=D.y,dt=t-ot;ot=t;//save old distances for normal, light direction calc
  D=DE(ro+rd*t,iTime, &mcol,mr,mxr,scale,&p0,lightPos);
  d=D.x+0.33f*rCoC;
  ld=D.y;//the distance estimate to light
  mld=_fminf(mld,ld);//the minimum light distance along the march
  if(d<rCoC){//if we are inside the sphere of confusion add its contribution
   float3 p=ro+rd*(t-dt);//back up to previos checkpoint
   float3 mcol=to_float3_s(0.01f);//collect color samples with normal deltas
   float2 Drt=DE(p+rt*dt,iTime, &mcol,mr,mxr,scale,&p0,lightPos),Dup=DE(p+up*dt,iTime,&mcol,mr,mxr,scale,&p0,lightPos);
   float3 N=normalize(rd*(D.x-od)+rt*(Drt.x-od)+up*(Dup.x-od));
   
   //if(N!=N) N=-rd;//if no gradient assume facing us ?????????
    
   float3 L=-1.0f*normalize(rd*(D.y-old)+rt*(Drt.y-old)+up*(Dup.y-old));
   float lightStrength=1.0f/(1.0f+ld*ld*20.0f);
   float3 scol=mcol*(0.4f*(1.0f+dot(N,L)+0.2f))*lightStrength;//average material color * diffuse lighting * attenuation
   scol+=_powf(_fmaxf(0.0f,dot(reflect(rd,N),L)),8.0f)*lightColor;//specular lighting
   mcol=to_float3_s(0);//clear the color accumulator before shadows
   //scol*=FuzzyShadow(p,L,ld,shadowCone,rCoC);//now stop the shadow march at light distance
   blm+=lightColor*_expf(-mld*t*10.0f)*(1.0f-col.w);//add a bloom around the light
   mld=100.0f;//clear the minimum light distance for the march
   float alpha=fudgeFactor*(1.0f-col.w)*linstep(-rCoC,rCoC,-d);//calculate the mix like cloud density
   col=to_float4_aw(swi3(col,x,y,z)+scol*alpha,clamp(col.w+alpha,0.0f,1.0f));//blend in the new color 
  }//move the minimum of the object and light distance
  d=_fabs(fudgeFactor*_fminf(d,ld+0.33f*rCoC)*(0.8f+0.2f*rand2(&randv2)));//add in noise to reduce banding and create fuzz
  t+=d;
 }//mix in background color and remaining bloom
 t=_fminf(15.0f,t);
 blm+=lightColor*_expf(-mld*t*10.0f)*(1.0f-col.w);///(1.0f+mld*mld*3000.0
 //swi3S(col,x,y,z, _mix(swi3(col,x,y,z),bg(rd),t/15.0f));
 fragColor = to_float4_aw(clamp(swi3(col,x,y,z)+blm,0.0f,1.0f),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
