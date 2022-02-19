

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define time iTime
#define size iResolution

float pixelSize,focalDistance,aperture,fudgeFactor=0.6;//,shadowCone=0.5;
mat2 rmat(float a){float sa=sin(a),ca=cos(a);return mat2(ca,sa,-sa,ca);}
vec3 mcol=vec3(0);
const float mr=0.16, mxr=1.0; 
const vec4 scale=vec4(-2.0,-2.0,-2.0,2.0); 
vec4 p0=vec4(3.,0.76,1.12,0.2);//0.32,.76
float lightPos; 
vec2 DE(in vec3 z0){//amazing surface by kali/tglad with mods
 p0.x=cos((iTime+z0.y)*0.05)*3.5;
 z0.xz=z0.xz*rmat(z0.y*0.07);
 z0.y=abs(mod(z0.y,4.0)-2.0);
 vec4 z = vec4(z0,1.0); float dL=100.;
 for (int n = 0; n < 4; n++) { 
  if(z.x<z.z)z.xz=z.zx; 
  z.xy=clamp(z.xy, -1.0, 1.0) *2.0-z.xy; 
  z*=scale/clamp(max(dot(z.xy,z.xy),dot(z.xz,z.xz)),mr,mxr); 
  z+=p0; 
  if(n==1)dL=length(z.xyz+vec3(0.5,lightPos,0.5))/z.w;
 } 
 if(mcol.x>0.)mcol+=vec3(0.6)+sin(z.xyz*0.1)*0.4; 
 z.xyz=abs(z.xyz)-vec3(1.4,32.8,0.7); 
 return vec2(max(z.x,max(z.y,z.z))/z.w,dL); 
} 

float CircleOfConfusion(float t){//calculates the radius of the circle of confusion at length t
 return max(abs(focalDistance-t)*aperture,pixelSize*(1.0+t));
}
mat3 lookat(vec3 fw,vec3 up){
 fw=normalize(fw);vec3 rt=normalize(cross(fw,normalize(up)));return mat3(rt,cross(rt,fw),fw);
}
float linstep(float a, float b, float t){return clamp((t-a)/(b-a),0.,1.);}// i got this from knighty and/or darkbeam
//random seed and generator
vec2 randv2;
float rand2(){// implementation derived from one found at: lumina.sourceforge.net/Tutorials/Noise.html
 randv2+=vec2(1.0,1.0);
 return fract(sin(dot(randv2 ,vec2(12.9898,78.233))) * 43758.5453);
}
vec3 bg(vec3 rd){
  float d=max(0.,rd.x+rd.y+rd.z);
  return vec3(d*d*.25)+rd*.05;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
 randv2=fract(cos((fragCoord.xy+fragCoord.yx*vec2(100.0,100.0))+vec2(time)*10.0)*1000.0);
 pixelSize=1.0/size.y;
 float tim=time*0.1;//camera, lighting and object setup
 lightPos=sin(tim*20.0)*5.; 
 vec3 ro=vec3(cos(tim),tim*2.0,sin(tim))*5.0; 
 vec3 rd=lookat(vec3(-ro.x,5.0,-ro.z),vec3(0.0,1.0,1.0))*normalize(vec3((2.0*gl_FragCoord.xy-size.xy)/size.y,2.0)); 
 focalDistance=min(length(ro)+0.001,1.0);
 aperture=0.007*focalDistance;
 vec3 rt=normalize(cross(vec3(0,1,0),rd)),up=cross(rd,rt);//just need to be perpendicular
 vec3 lightColor=vec3(1.0,0.5,0.25)*2.0;
 vec4 col=vec4(0.0);vec3 blm=vec3(0);//color accumulator, .w=alpha, bloom accum
 vec2 D;//for surface and light dist
 float t=0.0,mld=100.0,od,d=1.,old,ld=100.,dt=0.,ot;//distance traveled, minimum light distance
 for(int i=1;i<72;i++){//march loop
  if(col.w>0.9 || t>15.0)break;//bail if we hit a surface or go out of bounds
  float rCoC=CircleOfConfusion(t);//calc the radius of CoC
  od=D.x;old=D.y,dt=t-ot;ot=t;//save old distances for normal, light direction calc
  D=DE(ro+rd*t);
  d=D.x+0.33*rCoC;
  ld=D.y;//the distance estimate to light
  mld=min(mld,ld);//the minimum light distance along the march
  if(d<rCoC){//if we are inside the sphere of confusion add its contribution
   vec3 p=ro+rd*(t-dt);//back up to previos checkpoint
   mcol=vec3(0.01);//collect color samples with normal deltas
   vec2 Drt=DE(p+rt*dt),Dup=DE(p+up*dt);
   vec3 N=normalize(rd*(D.x-od)+rt*(Drt.x-od)+up*(Dup.x-od));
   if(N!=N)N=-rd;//if no gradient assume facing us
   vec3 L=-normalize(rd*(D.y-old)+rt*(Drt.y-old)+up*(Dup.y-old));
   float lightStrength=1.0/(1.0+ld*ld*20.0);
   vec3 scol=mcol*(0.4*(1.0+dot(N,L)+.2))*lightStrength;//average material color * diffuse lighting * attenuation
   scol+=pow(max(0.0,dot(reflect(rd,N),L)),8.0)*lightColor;//specular lighting
   mcol=vec3(0);//clear the color accumulator before shadows
   //scol*=FuzzyShadow(p,L,ld,shadowCone,rCoC);//now stop the shadow march at light distance
   blm+=lightColor*exp(-mld*t*10.)*(1.0-col.w);//add a bloom around the light
   mld=100.0;//clear the minimum light distance for the march
   float alpha=fudgeFactor*(1.0-col.w)*linstep(-rCoC,rCoC,-d);//calculate the mix like cloud density
   col=vec4(col.rgb+scol*alpha,clamp(col.w+alpha,0.0,1.0));//blend in the new color 
  }//move the minimum of the object and light distance
  d=abs(fudgeFactor*min(d,ld+0.33*rCoC)*(0.8+0.2*rand2()));//add in noise to reduce banding and create fuzz
  t+=d;
 }//mix in background color and remaining bloom
 t=min(15.,t);
 blm+=lightColor*exp(-mld*t*10.)*(1.0-col.w);///(1.0+mld*mld*3000.0
 col.rgb=mix(col.rgb,bg(rd),t/15.);
 fragColor = vec4(clamp(col.rgb+blm,0.0,1.0),1.0);
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
#define bps 3.0 
float rnd(float t){return fract(sin(mod(t,32.123)*32.123)*41.123);} 
vec2 nofs(float n){//the song's "random" ring 
  float r=0.5+0.5*rnd(floor(n));//random volume as well 
  n=mod(n,8.0); 
  if(n<1.0)n= 0.0; 
  else if(n<2.0)n= 5.0; 
  else if(n<3.0)n= -4.0; 
  else if(n<4.0)n= 4.0; 
  else if(n<5.0)n= -5.0; 
  else if(n<6.0)n= 3.0; 
  else if(n<7.0)n= 2.0; 
  else n=0.0;
  return vec2(n,r); 
}
float scale(float note){//throws out dissonant tones 
 float n2=mod(note,12.); 
 if((n2==1.)||(n2==2.)||(n2==4.)||(n2==6.)||(n2==8.)||(n2==9.)||(n2==11.))note=-100.;//pentatonic minor
 return note; 
} 
// note number to frequency  from https://www.shadertoy.com/view/ldfSW2 
float ntof(float n){return (n>0.0)?440.0 * pow(2.0, (n - 67.0) / 12.0):0.0;} 
const float PI=3.14159; 
float Cos(float a){return cos(mod(a,PI*2.));} 
float Sin(float a){return Cos(a+PI/2.);} 
struct instr{float att,fo,vibe,vphas,phas,dtun;}; 
vec2 I(float n,float t,float bt,instr i){//note,time,bt 0-8,instrument 
 float f=ntof(scale(n));if(f<12.)return vec2(0.0);f-=bt*i.dtun;f*=t*PI*2.; 
 f=exp(-bt*i.fo)*(1.0-exp(-bt*i.att))*Sin(f+Cos(bt*i.vibe*PI/8.+i.vphas*PI/2.)*Sin(f*i.phas))*(1.0-bt*0.125); 
 n+=t;return vec2(f*Sin(n),f*Cos(n));
} 
vec2 mainSound(int samp,float time){//att,fo,vibe,vphs,phs,dtun
 instr epiano=instr(50.0,0.05,1.5,0.1,1.5,0.001);//silly fm synth instruments 
 instr sitar=instr(2.0,.2,8.0,0.0,0.5,0.0025); 
 instr bassdrum=instr(500.0,1.0,4.0,0.76,1.0,0.5); 
 instr stick=instr(500.0,1.0,10.5,0.0,2.3131,1000.0); 
 instr pluckbass=instr(500.0,2.0,1.5,0.0,0.125,0.005); 
 instr bass=instr(20.0,0.2,2.0,0.0,0.5,0.005); 
 float tim=time*bps,b0,b1,b2,t0,t1,t2; 
 vec2 a=vec2(0.0);//accumulator 
 for(float i=0.;i<8.;i+=1.){//go back 8 beats and add note tails 
   b0=floor(tim);b1=floor(tim*0.5);b2=floor(tim*0.25); 
   vec2 n2=nofs(b2*0.0625)+nofs(b2*0.25)+nofs(b2);//build notes on top of notes like fbm 
   vec2 n1=n2+nofs(b1),n0=n2+nofs(b0); 
   t0=fract(tim)+i; 
   a+=I(n0.x+60.0,time,t0,sitar)*n0.y/(1.+abs(n0.x)*.25);
   if(mod(i,1.)<1.){
     a+=I(n0.x+93.0,time+Sin(t0*372.0),t0,stick)*n0.y*.1;
     a+=I(n0.x+67.0,time,t0,sitar)*n0.y/(3.+abs(n0.x+7.)*.25);
     a+=I(n0.x+72.0,time,t0,sitar)*n0.y/(3.+abs(n0.x+7.)*.25);
   } 
   if(mod(i,2.)<1.){//notes that play every 2 beats 
     t1=fract(tim*0.5)*2.0+i;
     //a+=I(n1.x+67.0,time,t1,epiano)*n1.y;
     a+=I(n1.x+65.,time,t1,epiano)*n1.y*.125; 
     a+=I(n1.x+64.,time,t1,epiano)*n1.y*.125; 
     a+=I(n1.x+60.,time,t1,epiano)*n1.y*.125; 
     //a+=I(n1.x+36.0,time,t1,pluckbass)*n1.y*4.0;
     a+=I(n1.x+32.0,t1/bps+0.008*sin(t1*3.0),t1,bassdrum)*2.0;
     a+=I(n2.x+31.0,t1/bps+0.008*sin(t1*2.0),t1,bassdrum)*2.0;
     if(mod(i,4.)<1.){//every 4 
       t2=fract(tim*0.25)*4.0+i;
       a+=I(n2.x+48.0,time,t2,bass)*n2.y;
       a+=I(n2.x+52.0,time,t2,bass)*n2.y;
       a+=I(96.0,time,t2,stick)*n2.y*.25;
       
     } 
   } 
   tim-=1.;//go back in time to find old notes still decaying 
 } 
 return clamp(a/48.0,-1.,1.); 
}