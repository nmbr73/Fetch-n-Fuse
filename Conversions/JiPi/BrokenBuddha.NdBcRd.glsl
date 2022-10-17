

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//broken buddha by eiffie 
//track the position of points near the border of the mandelbrot set as they orbit
#define time iTime
#define rez iResolution.xy

void mainImage(out vec4 O, in vec2 U){
  O=texture(iChannel0,U/rez.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define time iTime
#define rez iResolution.xy
#define pi 3.14159
vec4 load(float x, float y){return texture(iChannel0,vec2(x+.5,y+.5)/rez.xy);}
mat2 rmat(float a){return mat2(cos(a),sin(a),-sin(a),cos(a));}
vec2 cmul(vec2 a, vec2 b){return vec2(a.x*b.x-a.y*b.y,dot(a,b.yx));}
void mainImage(out vec4 O, in vec2 U){
  O=texture(iChannel0,U/rez.xy);
  U=floor(U);
  vec4 L=load(0.,0.);
  float f=float(iFrame);
  float a=floor(f)*0.001;
  
  if(U.x+U.y==0.){
    int iters=int(floor(a))+7;
    a*=pi;
    vec2 p=vec2(cos(a),sin(a/1.5)),rd=-p;p.x-=.5*cos(a*.007);
    for(int i=0;i<32;i++){
      float dr=1.,r=length(p);
      vec2 C=p,Z=p;
      for(int n=0;n<iters && r<2000.;n++){
        Z=cmul(Z,Z)+C;
        dr=dr*r*2.+1.;
        r=length(Z);
      }
      p+=rd*.5*log(r)*r/dr;
    }
    p+=rd*(1.+cos(a*.3))*.1;
    O=vec4(p.x,p.y,p.x,p.y); 
  }else{
    vec2 p0=L.xy;
    vec2 u=(2.*U-rez.xy)/rez.y;
    for(int j=0;j<5;j++){
      vec2 p=p0+vec2(0.001)*float(j);
      for(int n=0;n<20 && dot(p,p)<300000.;n++){
        p=cmul(p,p)+p0;
        float d=length(abs(u)-abs(p));d=smoothstep(2./rez.y,0.0,d)*.2;///(1.+40.*O.r*O.r);
        vec3 col=vec3(d*p*rmat(a*7.+float(j)*0.3),d);
        col.yz*=rmat(a*5.+float(j+n)*.03);
        O+=vec4(abs(col.grb),0.);
      }
    }
    O=O*.995;
  }
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
#define bps 2.5
#define pi 3.14159
int N[8]=int[8](0,4,2,4,5,7,9,5);
float scale(float note){//throws out dissonant tones 
 float n2=mod(note,12.); 
 if((n2==1.)||(n2==6.)||(n2==8.)||(n2==10.))note=-100.; 
 return note; 
}
// note number to frequency  from https://www.shadertoy.com/view/ldfSW2 
float ntof(int n0){
 float n=scale(float(n0));
 return (n>0.)?440.0 * pow(2.0, (float(n) - 67.0) / 12.0):0.0;
} 
vec2 rot(vec2 v, float a) {return cos(a)*v+sin(a)*vec2(v.y,-v.x);}
vec2 I(int n, float t, vec3 p, vec4 e, vec4 d){
  float bt=t,t2,t3,t4,f=ntof(n);
  if(f==0.)return vec2(0);
  t-=t*d.y*sin((d.w*pi+t)*pi*d.z);t3=t*d.x;
  t2=fract(t*p.y*f);t4=fract(t3*p.y*f);
  t=fract(t*f);t3=fract(t3*f);
  float amp=abs(1.+cos(bt*e.z*pi)*e.w)*exp(-bt*e.y)*(1.0-exp(-bt*e.x))*min(1.,100.-bt*100.);
  return amp*rot(vec2(sin(pow(t,p.x)*pi*2.+pi*2.*p.z*sin(pow(t2,p.x)*pi*2.)),
    sin(pow(t3,p.x)*pi*2.+pi*2.*p.z*sin(pow(t4,p.x)*pi*2.))),float(n)+bt*pi*2.);
}

vec2 mainSound(int samp, float time){
  float tim=time*bps;if(tim>80.)tim-=80.;if(tim>80.)tim+=48.+128.;
  float bt=floor(tim),t=fract(tim);
  int n1=N[int(bt/64.)%8];
  int n0=n1+N[int(bt/8.)%8],n=n0+N[int(bt)%8],n2=n0+N[int(bt/2.)%8],n4=n0+N[int(bt/4.)%8];
  const vec4 dtn=vec4(1.014,0.005,1.,.5);
  vec2 v=I(n+60,t,vec3(1.-t*.5,1.,t),vec4(.25,1.,18.,t*.6),dtn)*.25;
  
  t=fract(tim/2.); 
  float fo=pow(1.-t,20.0);
  v+=I(n2+60,t,vec3(1.-fo*.5,1.5,fo),vec4(1.,4.0,24.,t*.8),dtn);
  
  t=fract(tim/4.);
  fo=pow(1.-t,20.0);
  v+=I(n4+67,t,vec3(1.-fo*.5,1.5,fo),vec4(1.,4.0,36.,t*.8),dtn);
  
  t=fract(tim/8.);
  fo=pow(1.-t,20.0);
  v+=I(n0+60,t,vec3(1.-fo*.5,1.5,fo),vec4(1.,2.0,48.,t*.6),dtn);
  return v*.2;
}