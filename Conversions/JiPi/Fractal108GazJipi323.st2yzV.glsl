

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define R(p,a,t) mix(a*dot(p,a),p,cos(t))+sin(t)*cross(p,a)
#define H(h) (cos((h)*6.3+vec3(0,23,21))*.5+.5)

void mainImage(out vec4 O, vec2 C)
{
    vec3 p,r=iResolution,c=vec3(0),
    d=normalize(vec3((C-.5*r.xy)/r.y,1));
 	for(float i=0.,s,e,g=0.,t=iTime;i++<90.;){
        p=g*d;
        p.z-=.5;
        p=R(p,H(t*.01),t*.2);
        s=1.;
        for(int j=0;j++<6;)
           p=p.x<p.y?p.zxy:p.zyx,
           s*=e=max(1./dot(p,p),1.5),
           p=abs(p)*e-vec3(2,1.7,.8);
        g+=e=abs(p.x)/s+1e-4;
	    c+=mix(vec3(1),H(log(s)*.4),.4)*.015/exp(.1*i*i*e);
	}
	c*=c*c;
    O=vec4(c,1);
}
