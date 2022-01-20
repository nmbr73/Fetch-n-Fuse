

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define time iTime
#define rez iResolution
float wav(float t){return abs(fract(t)-.5);}
vec3 mcol=vec3(0.0);
float glw=0.;
float DE(vec3 p0){
  vec4 p=vec4(p0,1.1);
  vec3 c=mod(p0,10.)-4.;
  for(int n=0;n<2;n++){
    p.xyz=abs(mod(p.xyz,4.0)-2.0)-1.0;
    p*=2.0/clamp(dot(p.xyz,p.xyz),0.25,1.0);
    if(p.y>p.z)p.yz=p.zy;
    if(p.x>p.y)p.xy=p.yx;
    p.x+=1.0;
  }
  float d=(length(p.yz)-.1+0.1*wav(p.x*10.0))/p.w;
  glw+=max(wav(p.x+p0.y+p0.z+time)-.3,0.)/(1.+d*d);
  float g=abs(sin((c.x+c.z)*10.-time*10.));
  float d2=min(length(c.xy),length(c.yz+vec2(.5,0.)))-.125-g*.01;
  if(mcol.x>0.0){
    if(d<d2)mcol+=vec3(.4)+.1*abs(p.xyz);
    else mcol+=vec3(2.*g);
  }
  return min(d,d2);
}
vec3 normal(vec3 p, float d){//from dr2
  vec2 e=vec2(d,-d);vec4 v=vec4(DE(p+e.xxx),DE(p+e.xyy),DE(p+e.yxy),DE(p+e.yyx));
  return normalize(2.*v.yzw+vec3(v.x-v.y-v.z-v.w));
}
vec3 sky(vec3 rd, vec3 L){
  float d=2.*pow(max(0.,dot(rd,L)),20.);
  return vec3(d)+abs(rd)*.1;
}
float rnd;
void randomize(in vec2 p){rnd=fract(float(time)+sin(dot(p,vec2(13.3145,117.7391)))*42317.7654321);}

float ShadAO(in vec3 ro, in vec3 rd){
 float t=0.01*rnd,s=1.0,d,mn=0.01;
 for(int i=0;i<12;i++){
  d=max(DE(ro+rd*t)*1.5,mn);
  s=min(s,d/t+t*0.5);
  t+=d;
 }
 return s;
}
vec4 sphere(vec3 ro, vec3 rd){
  vec4 s=vec4(100);
  vec3 p=vec3(iTime*.5+6.,-4.,iTime*.5+8.);
  p=ro-p;
  float b=dot(-p,rd); 
  if(b>0.){
    float inner=b*b-dot(p,p)+.7;
    if(inner>0.){
      float t=b-sqrt(inner);
      if(t>0.)s=vec4(normalize(p+rd*t),t);
    }
  }
  return s;
}
vec3 scene(vec3 ro, vec3 rd){
  float t=DE(ro)*rnd,d,px=4.0/rez.x;
  vec4 s=sphere(ro,rd);
  for(int i=0;i<99;i++){
    t+=d=DE(ro+rd*t);
    if(t>20.0 || d<px*t)break;
    if(t>s.w){px*=10.;ro+=rd*s.w;rd=reflect(rd,s.xyz);t=0.01;}
  }
  vec3 L=normalize(vec3(0.4,0.025,0.5));
  vec3 bcol=sky(rd,L),col=bcol;
  float g=glw;
  if(d<px*t*5.0){
    mcol=vec3(0.001);
    vec3 so=ro+rd*t;
    vec3 N=normal(so,d);
    vec3 scol=mcol*0.25;
    float dif=0.5+0.5*dot(N,L);
    float vis=clamp(dot(N,-rd),0.05,1.0);
    float fr=pow(1.-vis,5.0);
    float shad=ShadAO(so,L);
    col=(scol*dif+.5*fr*sky(reflect(rd,N),L))*shad;
  }
  return mix(col,bcol,clamp(t*t/400.,0.,1.))+vec3(1.,.3,.1)*exp(-t)*clamp(g*g,0.,1.);
}
mat3 lookat(vec3 fw){vec3 up=vec3(0.0,0.8,0.1),rt=-normalize(cross(fw,up));return mat3(rt,normalize(cross(rt,fw)),fw);}
vec3 path(float t){t*=.5;t+=sin(t*.1)*7.;
  return vec3(t+sin(t*1.1),sin(t*.3)*.5-5.2,t+cos(t)*.7); 
}
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
 randomize(fragCoord);
 vec3 ro=path(iTime),fw=normalize(path(iTime+0.5)-ro);
 vec3 rd=lookat(fw)*normalize(vec3((iResolution.xy-2.0*fragCoord)/iResolution.y,1.0));
 fragColor=vec4(scene(ro,rd),1.0);
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
#define bps 5.0 
float rnd(float t){return fract(sin(mod(t,32.123)*32.123)*41.123);} 
vec2 nofs(float n){//the song's "random" ring 
  float r=0.5+0.5*rnd(floor(n));//random volume as well 
  n=mod(n,6.0); 
  if(n<1.0)n= 1.0; 
  else if(n<2.0)n= 3.0; 
  else if(n<3.0)n= 2.0; 
  else if(n<4.0)n= 0.0; 
  else if(n<5.0)n= 3.0; 
  else if(n<6.0)n= 0.0;
  else if(n<7.0)n= 1.0; 
  else n= -10.0;
  return vec2(n,r); 
} 
float scale(float note){//throws out dissonant tones 
 float n2=mod(note,12.); 
 if((n2==1.)||(n2==4.)||(n2==6.)||(n2==9.)||(n2==11.))note=-100.;//minor 
 return note; 
} 
// note number to frequency  from https://www.shadertoy.com/view/ldfSW2 
float ntof(float n){return (n>0.0)?440.0 * pow(2.0, (n - 67.0) / 12.0):0.0;} 
const float PI=3.14159; 
float Cos(float a){return cos(mod(a,PI*2.));} 
float Sin(float a){return Cos(a+PI/2.);} 
struct instr{float att,fo,vibe,vphas,phas,dtun;}; 
vec2 I(float n,float bt,instr i){//note,time,bt 0-8,instrument 
 float t=bt/bps,f=ntof(scale(n));if(f<12.)return vec2(0.0);f-=bt*i.dtun;f*=t*PI*2.; 
 f=exp(-bt*i.fo)*(1.0-exp(-bt*i.att))*Sin(f+Cos(bt*i.vibe*PI/8.+i.vphas*PI/2.)*Sin(f*i.phas))*(1.0-bt*0.125); 
 n+=t;return vec2(f*Sin(n),f*Cos(n));
}
vec2 mainSound(int samps, float time){//att,fo,vibe,vphs,phs,dtun
 instr epiano=instr(20.0,0.05,1.5,0.1,0.505,0.001);//silly fm synth instruments 
 instr bell=instr(400.0,0.15,3.,0.5,.66,0.01);
 instr sitar=instr(30.0,0.1,4.0,0.0,0.335,0.0025); 
 instr bassdrum=instr(500.0,1.0,4.0,0.76,1.0,0.5); 
 instr stick=instr(50.0,20.0,10.5,0.0,12.3131,100.0); 
 //instr pluckbass=instr(500.0,2.0,1.5,0.0,0.125,0.005);
 instr bass=instr(20.0,0.2,2.0,0.0,0.5,0.005); 
 float tim=time*bps,b0,b1,b2,t0,t1,t2; 
 vec2 a=vec2(0.0);//accumulator 
 for(float i=0.;i<8.;i+=1.){//go back 8 beats and add note tails 
   b0=floor(tim);b1=floor(tim*0.5);b2=floor(tim*0.25); 
   vec2 n2=nofs(b2*0.125)+nofs(b2*0.5)+nofs(b2);//build notes on top of notes like fbm 
   vec2 n1=n2+nofs(b1),n0=n1+nofs(b0); 
   t0=fract(tim)+i;
   a+=I(n0.x+91.0,t0,bell)*n0.y*.025;
   a+=I(n0.x+89.0,t0,bell)*n0.y*.05;
   a+=I(n0.x+87.0,t0,bell)*n0.y*.1;
   a+=I(n0.x+72.0,t0,sitar)*n0.y*.25;
   if(mod(i,1.)<1.)a+=I(n0.x+93.0,t0+rnd(t0)*0.002,stick)*n0.y*0.25;  
   if(mod(i,2.)<1.){//notes that play every 2 beats 
     t1=fract(tim*0.5)*2.0+i;
     a+=I(n1.x+67.0,t1+rnd(t1)*0.002,stick)*n1.y*.5;
     a+=I(n1.x+64.0,t1,epiano)*n1.y*.4; 
     a+=I(n1.x+60.0,t1,epiano)*n1.y*.5; 
     //a+=I(n1.x+36.0,time,t1,organ)*n1.y*.6;
     if(mod(i,4.)<1.){//every 4 
       t2=fract(tim*0.25)*4.0+i;
       a+=I(n2.x+48.0,t2,bass)*n2.y; 
       //a+=I(n2.x+36.0,t2+rnd(t2)*0.002,stick)*2.0;
       //a+=I(n2.x+12.0,t2+sin(t2),bassdrum)*2.0;
       a+=I(n2.x+13.0,t2+sin(t2),bassdrum)*2.0;
     } 
   } 
   tim-=1.;//go back in time to find old notes still decaying 
 } 
 return clamp(a/48.0,-1.,1.); 
}