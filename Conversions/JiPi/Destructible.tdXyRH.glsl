

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Destructible " by wyatt. https://shadertoy.com/view/3lKSDV
// 2020-03-13 18:10:34

// Fork of "Foam Simulation" by wyatt. https://shadertoy.com/view/tlyXDG
// 2020-03-08 02:50:08

// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54
vec4 color (float i) {
	return vec4(shape(co(i))/O/W/2.,0,1);
}
Main 
    vec4 b = B(U),
         a = A(b.x), aa = A(b.y), aaa = A(b.z),
    
    	 col1 = color(b.x), col2 = color(b.y), col3 = color(b.z);
    vec3 by = vec3(
         sg(U,aa.xy,aaa.xy)/sg(a.xy,aa.xy,aaa.xy),
         sg(U,a.xy,aaa.xy)/sg(aa.xy,a.xy,aaa.xy),
         sg(U,a.xy,aa.xy)/sg(aaa.xy,a.xy,aa.xy)
        );
    float o = length(U-a.xy),
        s = min(sg(U,a.xy,aa.xy),min(sg(U,a.xy,aaa.xy),sg(U,aa.xy,aaa.xy)));
	vec2 w = co(b.x);
    col1 = by.x*col1+by.y*col2+by.z*col3;
	Q = max(0.3*smoothstep(O/(1.+.1*o),0.,s),
        smoothstep(1.1*O,.9*O,o))*
        texture(iChannel2,1.6*col1.xy);
	Q = max(Q,0.1*abs(sin(vec4(1+2+3+4)+.1*tri(U,a.xy,aa.xy,aaa.xy))));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define co(i) vec2(mod(i,R.x),i/R.x)
#define id(U) (floor((U).x)+floor((U).y)*R.x)
#define A(i) texelFetch(iChannel0,ivec2(co(i)),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage (out vec4 Q, vec2 U) { U = gl_FragCoord.xy;
#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.)

#define O 6.
#define W round(0.75*R.y/O)
#define shape(u) clamp(round((u)/O)*O,O*2.,2.*O+O*W)

float pie (vec2 p, vec2 a, vec2 b) {
	vec2 m = 0.5*(a+b); // midpoint
    if (length(a-b)<1e-3) return 1e3; // ignore self
	return abs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
float sg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return (length(p-a-(b-a)*i));
}
vec2 cc (vec2 a, vec2 b, vec2 c) {
    vec2 ab = 0.5*(a+b), ac = 0.5*(a+c);
	float m1 = (a.x-b.x)/(b.y-a.y), m2 = (a.x-c.x)/(c.y-a.y);
    float b1 = ab.y-m1*ab.x, b2 = ac.y-m2*ac.x;
    float x = (b1-b2)/(m2-m1);
    return vec2(x,m1*x+b1);
}

float ssg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return sign(dot(p-a,(b-a).yx*vec2(1,-1)))*length(p-a-(b-a)*i);
}
float tri (vec2 U, vec2 a, vec2 aa, vec2 aaa) {
    if (length(a-aaa)<1e-3||length(aa-aaa)<1e-3||length(a-aa)<1e-3) return 1e3;
    float ab = ssg(U,a,aa),
          bc = ssg(U,aa,aaa),
          ca = ssg(U,aaa,a),
          l = min(abs(ab),min(abs(bc),abs(ca))),
          s = (ab<0.&&bc<0.&&ca<0.)||(ab>0.&&bc>0.&&ca>0.)?-1.:1.;
	if (s>0.) return s*l;
    vec2 m = cc(a,aa,aaa);
    float v = length(a-m), w = length(U-m);
    return l*s/v;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Information Storage
Main 
	vec2 u = shape(U);
	float i = id(U);
    Q = A(i);
    vec2 f = vec2(0), f1 = vec2(0);
	float n = 0., n1 = 0.;
	for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++){
        vec2 v = shape(u+O*vec2(x,y));
        // triangulated force
    	f += C(Q.xy+vec2(x,y)).xy;
        // shape force
        vec4 t = A(id(v));
        vec2 r = t.xy-Q.xy;
        float l = length(r), ll = length(u-v), lll = length(u+O*vec2(x,y)-v);
        if (ll>1.&&ll<3.*O&&l>0.&&lll<1.) {
            n++;
            f += 500.*r/l/max(1.,l*l*ll)*sign(l-length(u-v));
        }
        // direct contact force
        t = A(B(Q.xy+vec2(x,y)).x);
        r = t.xy-Q.xy;
        l = length(r);
        if (l>0.) {
            n1++;
            f1 -= 50.*r/l/max(1.,l*l)*smoothstep(.9*O,.5*O,l);
        }
    }
	if (n>0.) f = f/n; else f = vec2(0);
	if (n1>0.) f1 = f1/n1; else f1 = vec2(0);
	Q.zw = Q.zw + f + f1-vec2(0,1e-3);
	Q.xy += f+Q.zw*(1./sqrt(1.+dot(Q.zw,Q.zw)));
    
    if (Q.x<15.){Q.z=abs(Q.z);}
    if (Q.y<15.){Q.w=abs(Q.w);}
    if (R.x-Q.x<15.){Q.z=-abs(Q.z);}
    if (R.y-Q.y<15.){Q.w=-abs(Q.w);}
	if (iFrame < 3)
    {
    	
        Q = vec4(u,0.4,0.5);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// SORT TRIANGULATION
void X (inout vec4 Q, inout vec4 a, inout vec4 aa, inout vec4 aaa, vec2 U, vec2 r) {
    vec4 b = B(U+r);
    // check b.x
    vec4 n = A(b.x);
    float ln = length(n.xy-U),
          la = length(a.xy-U);
    if (ln<la) {
    	Q.x = b.x;
        a = n;
    }
    float pn = pie(U,a.xy,n.xy),
          pa = pie(U,a.xy,aa.xy);
    if (pn<=pa){
        aa = n;
        Q.y = b.x;
    }
    float tn = tri(U,a.xy,aa.xy,n.xy),
          ta = tri(U,a.xy,aa.xy,aaa.xy);
    if (tn<ta) {
    	aaa = n;
        Q.z = b.x;
    }
    // check b.y
	n = A(b.y);
    ln = length(n.xy-U);
    la = length(a.xy-U);
    if (ln<la) {
    	Q.x = b.y;
        a = n;
    }
    pn = pie(U,a.xy,n.xy);
    pa = pie(U,a.xy,aa.xy);
    if (pn<pa){
        aa = n;
        Q.y = b.y;
    }
    tn = tri(U,a.xy,aa.xy,n.xy),
    ta = tri(U,a.xy,aa.xy,aaa.xy);
    if (tn<ta) {
    	aaa = n;
        Q.z = b.y;
    }
    // check b.z
	n = A(b.z);
    ln = length(n.xy-U);
    la = length(a.xy-U);
    if (ln<la) {
    	Q.x = b.z;
        a = n;
    }
    pn = pie(U,a.xy,n.xy);
    pa = pie(U,a.xy,aa.xy);
    if (pn<pa){
        aa = n;
        Q.y = b.z;
    }
    tn = tri(U,a.xy,aa.xy,n.xy),
    ta = tri(U,a.xy,aa.xy,aaa.xy);
    if (tn<ta) {
    	aaa = n;
        Q.z = b.z;
    }
}
void Xr (inout vec4 Q, inout vec4 a, inout vec4 aa, inout vec4 aaa, vec2 U, float r) {
	 X(Q,a,aa,aaa,U,vec2(r,0));
     X(Q,a,aa,aaa,U,vec2(0,r));
     X(Q,a,aa,aaa,U,vec2(0,-r));
     X(Q,a,aa,aaa,U,vec2(-r,0));
}
Main 
	Q = B(U);
    vec4 a = A(Q.x), aa= A(Q.y), aaa = A(Q.z);
    Xr(Q,a,aa,aaa,U,1.);
    Xr(Q,a,aa,aaa,U,2.);
    for (int i = 0; i < 10; i++) {
        float j = mod(float(iFrame + i),R.x*R.y);
        if (length(U-A(j).xy)<length(U-a.xy)) Q.x = float(j);
    }
    Init {
        Q.z = Q.y = Q.x = float(id(shape(U)));
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// COMPUTE FORCE FIELD
Main 
	vec4 b = B(U);
	vec4 a = A(b.x), aa = A(b.y), aaa = A(b.z);
	vec2 r = a.xy-aa.xy, r1 = a.xy-aaa.xy;
	float l = length(r), l1 = length(r1);
	if (l>0.&&l1>0.) 
      Q.xy = 
        50.*r/l/max(1.,l*l)*smoothstep(0.9*O,0.8*O,l)+
        50.*r1/l1/max(1.,l1*l1)*smoothstep(0.9*O,0.8*O,l1);
    if (length(Q.xy)>.1) Q.xy = 0.1*normalize(Q.xy);
	if (iMouse.z>0.&&length(a.xy-iMouse.xy)>0.)
        Q += vec4(120,120,0,0)*clamp(0.03*(a.xy-iMouse.xy)/dot((a.xy-iMouse.xy),(a.xy-iMouse.xy)),-2e-4,2e-4).xyxy;
	
}