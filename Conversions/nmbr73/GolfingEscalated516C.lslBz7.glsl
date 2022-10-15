

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// 516 chars - Xor does the IMPOSSIBLE by removing 13 chars!!

#define V vec3//
#define S .1*sin(q.x+sin(q.z))//
mat2 m = mat2(8,6,-6,8);//
float h,s,t,x,p,d,v;//
V q,z,w,U;//
#define g(p)(q=p,h=abs(S),q.xz*=m*.1,h+S)//
float n(V p){//
    for(p*=.1,s=.08,t=.9; (s/=.4)<4e2; p+=t) t-=g(p)/s, p.xz*=m*.21;//
    return 3.-exp(t);}//
void mainImage(out vec4 O, vec2 u) {
    U = V(u,0)/iResolution-1.;
    for(d=p=x=0.; d++<2e2 && p/5e3<=x;)
        z=V(0,-8.*g(V(0,0,v=iTime*.2)),v/.1)+p*normalize(U-V(sin(v),U.y*.7-.1,-3)),
        p += x = z.y+n(z);
    O.rgb = d/5e2+.1+.1*log(p)
                -dot(V(w.z=.01),
                     normalize(V(n(z-w.zyx),
                                 x = n(z),
                                 n(z-w)-n(z.zyx*11.)/5e2)
                               -x))
           *n(z.zyx*6.)*V(5,10,15); }/*


// 529 chars - Applying #define trick:

#define V vec3//
#define S.1*sin(q.x+sin(q.z))//
mat2 m = mat2(8,6,-6,8);//
float h,s,t,x,p,o=.1,d,v;//
V q,z,w;//
#define g(p)(q=p,h=abs(S),q.xz*=m*.1,h+S)//
float n(V p){//
    for(p*=.1,s=.08,t=.9; (s/=.4)<4e2; p+=t) t-=g(p)/s, p.xz=p.xz*m*.21;//
    return 3.-exp(t);}//
#define mainImage(O,u)                                                                  \
    vec2 U = u/iResolution.xy;                                                          \
    U--;                                                                                \
    for(d=p=x=0.; d++<2e2 && p/5e3<=x; o+=.002 )                                        \
        z=V(0,-8.*g(w=V(0,0,v=iTime*.2)),v/.1)+p*normalize(V(U.x-sin(v),U.y*.3+.1,2)),  \
        p += x = z.y+n(z);                                                              \
    O.rgb = o+.1*log(p)                                                                 \
                -dot(V(w.z=.01),                                                        \
                     normalize(V(n(z-w.zyx),                                            \
                                 x = n(z),                                              \
                                 n(z-w)-n(z.zyx*11.)/5e2)                               \
                               -x))                                                     \
                *n(z.zyx*6.)*V(5,10,15)                                            


        
// 540 chars - Golfing master coyote, does it again ...

#define V vec3
#define S.1*sin(q.x+sin(q.z))

mat2 m = mat2(8,6,-6,8);
float h,s,t,x,p,o=.1,d,v;
V q,z,w;

#define g(p)(q=p,h=abs(S),q.xz*=m*.1,h+S)

float n(V p){
    for(p*=.1,s=.08,t=.9; (s/=.4)<4e2; p+=t) t-=g(p)/s, p.xz=p.xz*m*.21;
    return 3.-exp(t);}

void mainImage(out vec4 O,vec2 U)
{
    U/=iResolution.xy;
    U--;
    for(d=p=x=0.; d++<2e2 && p/5e3<=x; o+=.002 )
        z=V(0,-8.*g(w=V(0,0,v=iTime*.2)),v/.1)+p*normalize(V(U.x-sin(v),U.y*.3+.1,2)),
        p += x = z.y+n(z);
    O.xyz = o+.1*log(p)
                -dot(V(w.z=.01),
                     normalize(V(n(z-w.zyx),
                                 x = n(z),
                                 n(z-w)-n(z.zyx*11.)/5e2)
                               -x))
                *n(z.zyx*6.)*V(5,10,15);
}



// 555 chars - stduhpf uses several optimizations to bring this shader DOWN!

#define V vec3
#define S sin(p.x+sin(p.z))

mat2 m = .1*mat2(8,6,-6,8);
float h,s,t,i;

float g(V p){
    h=abs(S); p.xz*=m;
    return h+S;}

float n(V p){
    for(p*=.1,s=5.,i=t=.9; i++<9.;) t-=s*.1*g(p), s*=.4, p.xz*=m*2.1, p+=t;
    return 3.-exp(t);}

void mainImage(out vec4 O,vec2 U)
{
    float x=.0,p=x,o=1.,d=x;
    V z, w = V(0,0,iTime*.2);
    U/=iResolution.xy;
    for(U--; d++<2e2 && p/5e3<=x;o+=.02 )
        z=V(0,-.8*g(w),w.z/.1)+p*normalize(V(U.x-sin(w.z),U.y*.3+.1,2)),
        p += x = z.y+n(z);
    w.z=.01;
    O.xyz = .1*(o+log(p)
                -dot(V(.5),
                     normalize(V(n(z-w.zyx),
                                 x = n(z),
                                 n(z-w)-n(z.zyx*11.)/5e2)
                               -x))
                *n(z.zyx*6.)*V(1,2,3));
}



// 598 chars - by Greg Rostami

#define V vec3
#define W vec2
#define S sin(p.x+sin(p.y))

mat2 m = .1*mat2(8,6,-6,8);
float h,s,t;

float g(W p){
    h=abs(S); p*=m;
	return h+S;}

float n(W p){
    p*=.1;
    s=5.,t=.9;
	for(int i=0;i++<9;) t-=s*.1*g(p), s*=.4, p=p*m*2.1+t;
    return 3.-exp(t);}

void mainImage(out vec4 O,W U)
{
    float e,v=iTime*.2,u=sin(v),x=.0,p=x,o=x;
	V r=V(U/iResolution.xy-1.,0),z,y;
	for(int d=0;d++<200;)        
        if (p/5e3<=x)
			z=V(0,-.8*g(W(0,v)),v/.1)+p*normalize(V(r.x-u,r.y*.3+.1,2)),
            x=z.y+n(z.xz),p+=x,o++;
    x=n(z.xz);
    O.xyz = .1*(dot(V(-.5),normalize(V(n(z.xz-W(.01,0))-x,0,n(z.xz-W(0,.01))-x-n(z.zx*11.)/5e2)))*
        n(z.zx*6.)*V(1,2,3)+1.+o/50.+log(p));

}



// 664 chars: drift's original shader:

mat2 m=mat2(.8,-.6,.6,.8);

float g(vec2 p){
    float e=abs(sin(p.x+sin(p.y)));p=m*p;
	return .1*(e+sin(p.x+sin(p.y)));
}

float n(vec2 p){
    p*=.1;
    float s=5.,t=.9;
	for(int i=0;i<9;i++)
        t-=s*g(p),s*=.4,p=m*2.1*p+t;
    return 3.-exp(t);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    float v=iTime*2.,u=sin(v*.1),x=.0,p=.0,o=.0;
	vec3 r=vec3(fragCoord/iResolution.xy-1.,0),z,y;
	for(int d=0;d<288;d++)        
        if (p*.0002<=x)
			z=vec3(0,-8.*g(vec2(0,v)*.1),v)+p*normalize(vec3(r.x-u,r.y*.3+.1,2)),x=z.y+n(z.xz),p+=x,o++;
    x=n(z.xz);
    y=normalize(vec3(n(z.xz-vec2(.01,0))-x,0,n(z.xz-vec2(0,.01))-x-n(z.zx*11.)*.002));
    fragColor.xyz=dot(vec3(-.5),y)*n(z.zx*6.)*vec3(.1,.2,.3)+.1+o*.002+log(p)*.1;
}
*/