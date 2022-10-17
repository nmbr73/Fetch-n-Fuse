

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Volumetric screamer - Result of an improvised live coding session on Twitch
// LIVE SHADER CODING, SHADER SHOWDOWN STYLE, EVERY TUESDAYS 20:00 Uk time: 
// https://www.twitch.tv/evvvvil_

vec2 z;float tt,b,g=0.,gg=0.,cr;vec3 faceP,cp;vec4 su=vec4(0);
float smin( float d1,float d2,float k){ float h=max(k-abs(d1-d2),0.);return min(d1,d2)-h*h*.25/k;}
float smax( float d1,float d2,float k){ float h=max(k-abs(-d1-d2),0.);return max(-d1,d2)+h*h*.25/k;}
mat2 r2(float r){ return mat2(cos(r),sin(r),-sin(r),cos(r));} 
float noi(vec3 p){
    vec3 f=floor(p),s=vec3(7,157,113);
    p-=f;
    vec4 h=vec4(0,s.yz,s.y+s.z)+dot(f,s);
    p=p*p*(3.-2.*p);
    h=mix(fract(sin(h)*43758.5),fract(sin(h+s.x)*43758.5),p.x);
    h.xy=mix(h.xz,h.yw,p.y);
    return mix(h.x,h.y,p.z);  
}  
float ferlin(vec3 p){ 
    float f=0.,A=.5,I;
    p.zy+=tt*2.; 
    for(int i=0;i<3;i++) I=float(i),f+=A/(I+1.)*noi(p+I),p=(2.1+.1*I)*p;
    return f;
} 
float face(vec3 p){
    p-=vec3(0,-12.+b*20.,0)+sin(p.y*2.)*.1;
    p.yz*=r2(1.65*(1.-b));  
    faceP=p*vec3(1,.7,1);   
    float t=length(faceP)-4.-sin(p.y)*.66;
    t=smin(t,length(abs(faceP+vec3(0,-2.5,-1))-vec3(2,0,0))-4.,1.);
    vec3 spikeP=p+vec3(0,-3.9,2);
    spikeP.x=abs(spikeP.x)-2.;
    spikeP.xy*=r2(-.785);
    spikeP.yz*=r2(-.1785);
    t=smin(t,length(spikeP.xz)-2.+abs(p.x)*.2,1.5);
    vec3 eyeP=abs(p-vec3(0,2,0));
    eyeP.xy*=r2(.6);
    float eyes=max(eyeP.y,(length(abs(faceP+vec3(0,-1.5,3.))-vec3(1.,0,0))-3.));   
    t=smax(eyes,t,1.);   
    t=min(t,max(eyeP.y+4.,eyes));
    t=smax(length(faceP+vec3(0,2,-2.+5.*b))-2.5,t,.5); 
    spikeP.xy*=r2(-.1485);
    spikeP-=vec3(8.*b,-3,-1);
    t=smin(t,length(spikeP.xz)-1.+abs(spikeP.y+3.)*.25,1.5);
    return t;
}
float terrain(vec3 p){
    float t=p.y+5.+cos(length(p*(.5))-b*15.-tt*4.)*b+noi(p*.07+1.)*5.; //WOBBLE: cos(length(p*(.5))-b*15.-tt*4.)
    t=smax(length(p.xz)-2.-b*6.,t,3.);
    t=smin(t,length(p.xz)-1.+(p.y+15.-b*17.)*.5,1.5);
    return t;
} 
float cmp( vec3 p) 
{  
    float t=face(p);  
    t=smin(t,terrain(p),2.5);
    vec3 boltP=p;
    boltP=abs(boltP-vec3(0,0,2))-11.+sin(p.y*5.*p.x*.1+tt*25.5)*.05+4.*sin(p.y*.3-3.)+p.y*.2;//ORIGINAL SHADER IN BONZOMATIC HAD NOISE TEXTURE CALL FOR BETTER LIGHTNING BOLT EFFECT BUT, THIS SHADER BEING GREEDY ENOUGH, I THOUGHT BEST REPLACE WITH BUNCH OF SINS ON SHADERTOY
    float bolt=length(boltP.xz)-.1; //~Above line on bonzo end should be: abs(boltP-vec3(0,0,2))-11.+texture(texNoise,p.xy*.1+tt*.5).r*2.+4.*sin(p.y*.3-3)+p.y*.2;      
    bolt=max(bolt,p.y+10.-b*25.);
    float mouthFlash=max(p.z,length(faceP.xy-vec2(0,-2))+2.+p.z*.2*b);
    g+=0.1/(0.1+bolt*bolt*(1.02-b)*(40.-39.5*sin(p.y*.2-b*8.)));
    gg+=0.1/(0.1+mouthFlash*mouthFlash*(1.05-b)*(40.-39.5*sin(p.z*.3+tt*5.)));
    return t;
}  
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv=(fragCoord.xy/iResolution.xy-0.5)/vec2(iResolution.y/iResolution.x,1);
    tt=mod(iTime,62.82);
    b=smoothstep(0.,1.,sin(tt)*.5+.5);
    vec3 ro=vec3(sin(tt*.5)*10.,mix(15.,-3.,b),-20.+sin(tt*.5)*5.)*mix(vec3(1),vec3(2,1,cos(tt*.5)*1.5),cos(-tt*.5+.5)*.5+.5),
         cw=normalize(vec3(0,b*10.,0)-ro), cu=normalize(cross(cw,vec3(0,1,0))),
         cv=normalize(cross(cu,cw)), rd=mat3(cu,cv,cw)*normalize(vec3(uv,.5)),co,fo;
    co=fo=vec3(.1,.12,0.17)-length(uv)*.1-rd.y*.2;    
    cr=cmp(ro-3.)+fract(dot(sin(uv*476.567+uv.yx*785.951+tt),vec2(984.156)));
    for(int i=0;i<128;i++){
        cp=ro+rd*(cr+=1./2.5);
        if(su.a>.99) break; //NOTE TO SELF: cr>t NOT NEEDED AS ONLY VOLUMETRIC GEOM ARE PRESENT
        float de=clamp((-cmp(cp)*9.+8.*ferlin(cp))/8.,0.,1.);
        su+=vec4(vec3(mix(1.,0.,de)*de),mix(.07,de,exp(-.00001*cr*cr*cr)))*(1.-su.a);//FOG ON CLOUDS! mix(.07,de,exp(-.00001*cr*cr*cr))
    }
    co=mix(co,su.xyz,su.a);  
    fragColor = vec4(pow(co+g*.4*vec3(.5,.2,.1)+gg*.4*vec3(.1,.2,.5),vec3(.55)),1);
}