

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Winning shader made at Revision 2022 Shader Showdown final

// This shader was coded live on stage in 25 minutes. Designed beforehand in several hours.

vec2 z,v,e=vec2(.00035,-.00035);float t,tt,b,bb,s,gg,g; vec3 pp,tp,po,no,al,ld,lp,op;
float smin(float a,float b,float k){float h=max(0.,k-abs(a-b));return min(a,b)-h*h*.25/k;}
float smax(float a,float b,float k){float h=max(0.,k-abs(-a-b));return max(-a,b)+h*h*.25/k;}
float cy(vec3 p,vec3 r){return max(abs(length(p.xz)-r.x)-r.y,abs(p.y)-r.z);}
mat2 r2(float r){return mat2(cos(r),sin(r),-sin(r),cos(r));}
vec2 smin( vec2 a, vec2 b,float k ){ float h=clamp(.5+.5*(b.x-a.x)/k,.0,1.);return mix(b,a,h)-k*h*(1.0-h);}
vec2 fb( vec3 p,float ga){
  b=sin(p.y*15.)*.03; //frill
  vec2 h,d,t=vec2(length(p)-5.,0);   //blue base
  t.x=smax(length(p.xz)-3.,t.x,.5);  //BASE CUT
  pp=p; pp.xz*=r2(sin(p.y*.4-tt)*.4); //overall sway
  pp.xz=abs(pp.xz)-3.;              //move out by 3
  h=vec2(cy(pp,vec3(.5,.2,7.)),1);  //yellow tube
  h.x=smin(h.x,cy(abs(pp)-.75,vec3(.2-b,.05,6.)),.5); //yellow 4 outter tubes
  tp=pp;                //tp pp
  tp.xz*=r2(.785);      //rot tp
  tp=abs(tp);           //clone tp
  tp.xz*=r2(.785);      //rot tp
  tp.y=mod(tp.y,2.)-1.; //mod tp y
  d=vec2(cy(tp.yxz,vec3(.1,0.01,2.5)),0.); //D cyl
  tp.xz*=r2(.785);  //tp rot again
  tp=abs(tp)-1.8;  //tp move out 1.8
  d.x=smin(d.x,cy(tp,vec3(.1-b*.5,.05,3.)),.3); //d side cyl
  d.x=max(d.x,abs(p.y)-6.); //Cut whole of D
  pp.y=mod(pp.y-tt*2.,3.)-1.5;              //spore pp mod
  pp.xz*=r2(-tt);                           //spore rot
  pp.xz=abs(pp.xz)-max(0.,abs(p.y*.15)-1.5);//spore spread
  s=length(pp)-.05;                         //spores
  g+=0.5/(0.1+s*s*20.)*ga;                  //glo spores
  h.x=smin(h.x,s,1.);                       //add spore to H
  h.x=smin(h.x,length(pp.xz)-.02,.2);       //spores lines
  h.x=smin(h.x,max(length(pp.xz)-20.,2.5),p.y*.5-5.); //spore tubes
  t=smin(t,h,1.5);  //Add T and H
  t=smin(t,d,.5); //Add T and D
  tp.y=mod(p.y-tt*4.,4.)-2.; //PART tp mod p
  h=vec2(length(tp),1); //PART TO H
  gg+=0.2/(0.1+h.x*h.x*100.)*ga; //GLOW PART H
  t=smin(t,h,0.5+p.y*.05); //Add T and H with p.y
  t.x*=0.7; //AVOID ARTIFACT
return t;
}
vec2 mp( vec3 p,float ga){
  vec2 h,t=fb(p,ga);
  h=fb(p*.3+vec3(0,3,0),ga);
  h.x/=.3;
  t=smin(h,t,1.5);
  return t;
}
vec2 tr( vec3 ro,vec3 rd){
  vec2 h,t=vec2(.1);
  for(int i=0;i<128;i++){
    h=mp(ro+rd*t.x,1.);
    if(h.x<.0001||t.x>60.) break;
    t.x+=h.x;t.y=h.y;
  }
  if(t.x>60.) t.y=-1.;
	return t;
}
#define a(d) clamp(mp(po+no*d,0.).x/d,0.,1.)
#define s(d) smoothstep(0.,1.,mp(po+ld*d,0.).x/d)
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
  vec2 uv=(fragCoord.xy/iResolution.xy-0.5)/vec2(iResolution.y/iResolution.x,1);   
  tt=mod(iTime,125.6637)*.5+3.94; 
  b=ceil(cos(tt*.4));
  vec3 ro=mix(vec3(1.,10.+sin(tt*.4)*11.,cos(tt*.4)*2.),
  vec3(cos(tt*.4)*8.,10.+sin(tt*.4)*12.,sin(tt*.4)*8.),b),
  cw=normalize(vec3(0,12.+6.*b,0)-ro),
  cu=normalize(cross(cw,vec3(0,1,0))),
  cv=normalize(cross(cu,cw)),
  rd=mat3(cu,cv,cw)*normalize(vec3(uv,.5)),co,fo;
  co=fo=vec3(.17,.13,.12)-length(uv)*.2-rd.y*.1;
  z=tr(ro,rd);t=z.x;
  if(z.y>-1.){
    po=ro+rd*t;
    lp=ro;ld=normalize(lp-po);
    no=normalize(e.xyy*mp(po+e.xyy,0.).x+e.yyx*mp(po+e.yyx,0.).x+e.yxy*mp(po+e.yxy,0.).x+e.xxx*mp(po+e.xxx,0.).x);
    al=mix(vec3(.4,.6,.7),vec3(.7,.5,.3),z.y);
    if(z.y>1.) al=vec3(1);
    float dif=max(0.,dot(no,ld)),
    fr=pow(1.+dot(no,rd),4.),
    sp=pow(max(dot(reflect(-ld,no),-rd),0.),40.);
    co=mix(sp+al*(a(.1)*a(.5)+.4)*(dif+s(2.)),fo,min(fr,.5));
    co=mix(fo,co,exp(-.00004*t*t*t));
  }
  fragColor = vec4(pow(co+g*.2*vec3(.1,.2,.5)+gg*.2*vec3(.7,.5,.3),vec3(.55)),1);
}