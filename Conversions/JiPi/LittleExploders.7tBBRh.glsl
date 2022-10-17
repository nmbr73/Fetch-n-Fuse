

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float BG(vec2 u){u=sin(u+sin(u.yx+iTime*.1));return (u.x*u.y+u.x+u.y)*.025+0.05;}
vec4 cmap(float a){a=clamp(a,0.,1.6);return a*abs(vec4(sin(a),sin(a+.4),sin(a+1.7),1.));}
void mainImage(out vec4 O, in vec2 U){
  O=U.y>1.?cmap(BG(U*0.01)+BG(U.yx*0.03)*.2+texture(iChannel0,U/iResolution.xy).r):vec4(0);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define dot2(x) dot(x,x)
void mainImage(out vec4 O, in vec2 U){
  int X=int(floor(U.x));
  O=texture(iChannel0,U/iResolution.xy);
  bool bLogic=(U.y<1. && U.x<256.);
  if(iFrame<2)O=vec4(bLogic?vec2(cos(U.x),sin(U.x))*100.:vec2(0),0.,0.);
  U-=iResolution.xy*.5;
  float d=100.;
  for(int i=0;i<256;i++){
    vec4 v=texture(iChannel0,vec2(float(i)+.5,.5)/iResolution.xy);
    if(bLogic){
      if(i!=X){
        vec2 dlt=v.xy-O.xy,a=sign(dlt)/max(4.,dot2(dlt));
        float m=min(1.,.98+dot2(v.zw-O.zw)*0.0001+dot2(dlt)*0.0001);
        O.zw=O.zw*m+a;
      }
    }else d=min(d,length(U-v.xy));
  }
  if(bLogic){O.xy+=O.zw;O.zw-=sign(O.xy)*length(O.xy)*0.00001;}
  else O.r=O.r*.98+.2*smoothstep(10./iResolution.y,0.,d*.03);
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
int N[8]=int[8](-1,-5,0,0,0,-5,4,7);
float I(int n, float t, float d){
  int n2=(n+48)%12;float e=1.+sin(float(n))*.2-t*t*.5;
  if((n2==1)||(n2==4)||(n2==6)||(n2==8)||(n2==11))return 0.;//dorian
  return pow(fract(t*50.*pow(2.,float(n)/(12.-e*e*.05))),.5+t*t*d)*e;
}
vec2 Sound(float time){
  float tim=time*7.,b=floor(tim),b3=fract(tim/8.),b2=fract(tim/2.),b1=fract(tim);
  int n3=N[(int(b/128.)*2)%8]+N[int(b/8.)%8];
  int n2=n3+N[int(b/2.)%8],n=n2+N[int(b)%8];
  return vec2(I(n,b1,200.)*.65-I(n2+3,b2*1.005,30.)*.75,
   -I(n+5,b1*1.007,200.)*.5+I(n2,b2,20.)*.75-I(n3,b3*1.001,20.));
}
vec2 mainSound(int samp, float time){
  return Sound(time)*.1+Sound(time-.075).yx*.08-Sound(time-.15)*0.05;
}