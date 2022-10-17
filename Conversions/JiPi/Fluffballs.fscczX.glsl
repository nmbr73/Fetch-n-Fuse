

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R iResolution.xy

const float PI = 3.1415926;
mat2 rot(float c){float s=sin(c);return mat2(c=cos(c),s,-s,c);}

#define normal(FUNC,P,E)\
(\
    vec3(\
        FUNC(P+vec3(E,0,0))-FUNC(P-vec3(E,0,0)),\
        FUNC(P+vec3(0,E,0))-FUNC(P-vec3(0,E,0)),\
        FUNC(P+vec3(0,0,E))-FUNC(P-vec3(0,0,E))\
     )/(E*2.)\
)

float map(vec3 p){
    float d = 1e9;
    
    p -= (texture(iChannel0,p).xyz-.5)*.1;
    
    d = min(d,length(fract(p)-.5)+1.);
    
    p.xy = (p.xy+p.yx*vec2(-1,1))/sqrt(2.);
    
    p.xz = (p.xz+p.zx*vec2(-1,1))/sqrt(2.);
    p*=.4;
    p-=iTime*.3;
    d = min(d,(length(fract(p)-.5))/.4);
        
    //d = min(d,(sdBox(fract(p-.5)-.5,vec3(.4,.5,.6))+.7)/.4);
    
    return d;
}

float trace(vec3 ro,vec3 rd){
    vec3 p = ro;
    float t = 0.;
    float h = -.4;
    for(int i=0;i<40;i++){
        t += (map(p)+t*h)/(1.-h);
        p = ro+rd*t;
    }
    return t;
}

void mainImage(out vec4 O,vec2 U){
    vec2 uv = (2.*U-R)/R.y*.8;
    vec2 m = (2.*iMouse.xy-R)/R.y*2.;

    vec3 ro = vec3(sin(iTime*.2)*4.,sin(.1*iTime*1.23)*4.,-0)+iTime;
    vec3 rd = normalize(vec3(uv,1));
    
    if(iMouse.z>0.){
        rd.yz*=rot(-m.y);
        rd.xz*=rot(-m.x);
    }
    
    rd.yz*=rot(iTime*.37);
    rd.xy*=rot(iTime*.4);
     
    
    O.xyz = (rd*.5+.5)*2.;
    
    float t = trace(ro,rd);
    
    vec3 p = ro+rd*t;
    vec3 n = normal(map,p,.15);
    
    O.xyz += vec3(1,2,3)*max(dot(n,normalize(vec3(0,1,0)))*.5+.5,0.)*.2;//n*.5+.5;
    O.xyz += vec3(4,2,1)*max(dot(n,normalize(vec3(3,1,0))),0.);//n*.5+.5;


    O.xyz += vec3(.1,.2,.3)*exp(t*.4);
    O.xyz *= .6;O.xyz-=.4;
    
    O.xyz = 1. - exp(-O.xyz);
    O.xyz = pow(O.xyz,vec3(1./2.2));
}