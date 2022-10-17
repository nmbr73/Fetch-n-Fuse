

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Foam Simulation" by wyatt. https://shadertoy.com/view/tlyXDG
// 2020-03-08 02:50:08

// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54

Main 
    vec4 b = B(U),
         a = A(b.xy), aa = A(b.zw);
    
    float 	
          r = length(a.xy-aa.xy), 
          rr = length(shape(b.xy)-shape(b.zw)),
          o = length(U-a.xy),
          w = sg(U,a.xy,aa.xy),
          v = pie(U,a.xy,aa.xy);
    
    Q = smoothstep(1.5*O,O,min(rr,O*o))*
        smoothstep(2.,0.,w)*
        (0.6+0.4*sin(vec4(1,2,3,4)+6.2*length(round(10.+b.xy/W/O)*W*O/R)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage (out vec4 Q, vec2 U) { U = gl_FragCoord.xy;
#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.)

#define O 5.
#define W 5.
vec2 shape (vec2 u) {
    u = round(u/O)*O;
    vec2 q = W*O+min(floor(u/60.)*60.,O*W*8.);
	u = clamp(u,O*2.+q,2.*O+O*W+q);
    return u;
}
float sg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return (length(p-a-(b-a)*i));
}
float pie (vec2 p, vec2 a, vec2 b) {
	vec2 m = 0.5*(a+b); // midpoint
    if (length(a-b)<1e-3) return 1e3; // ignore self
	return abs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Information Storage
Main 
	vec2 u = shape(U);
    Q = A(U);
    vec2 f = vec2(0), f1 = vec2(0);
	float n = 0., n1 = 0.;
	for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++){
        vec2 v =shape(u+O*vec2(x,y));
    	Q.zw += C(Q.xy+vec2(x,y)).xy;
        vec4 t = A(v);
        vec2 r = t.xy-Q.xy;
        float l = length(r), ll = length(u-v), lll = length(u+O*vec2(x,y)-v);
        if (ll>1.&&ll<3.*O&&l>0.&&lll<1.) {
            n++;
            f += 100.*r/l/max(1.,l*l*ll)*sign(l-length(u-v));
        }
        t = A(B(Q.xy+vec2(x,y)).xy);
        r = t.xy-Q.xy;
        l = length(r);
        if (l>0.) {
            n1++;
            f1 -= 10.*r/l/max(1.,l*l)*smoothstep(.9*O,0.5*O,l);
        }
    }
	if (n>0.) f = f/n; else f = vec2(0);
	if (n1>0.) f1 = f1/n1; else f1 = vec2(0);
	Q.zw = Q.zw + f + f1;
	Q.xy += f+Q.zw*(1./sqrt(1.+dot(Q.zw,Q.zw)));
    
    if (Q.x<5.){Q.x=5.;Q.z*=-.9;}
    if (Q.y<5.){Q.y=5.;Q.w*=-.9;}
    if (R.x-Q.x<5.){Q.x=R.x-5.;Q.z*=-.9;}
    if (R.y-Q.y<5.){Q.y=R.y-5.;Q.w*=-.9;}

	if (iFrame < 3)
    {
    	
        Q = vec4(u,0.2,0.3);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// SORT TRIANGULATION
void X (inout vec4 Q, inout vec4 a, inout vec4 aa, vec2 U, vec2 r) {
    vec4 b = B(U+r);
    vec4 n = A(b.xy);
    float ln = length(n.xy-U), la = length(a.xy-U);
    if (ln<=la) {
    	Q.xy = b.xy;
        a.xy = n.xy;
    }
    float pn = pie(U,a.xy,n.xy), 
          pa = pie(U,a.xy,aa.xy);
    if (pn<=pa){
        aa = n;
        Q.zw = b.xy;
    }
	n = A(b.zw);
    ln = length(n.xy-U);
    if (ln<la) {
    	Q.xy = b.zw;
        a.xy = n.xy;
    }
    pn = pie(U,a.xy,n.xy);
    if (pn<pa){
        aa = n;
        Q.zw = b.zw;
    }
}
void Xr (inout vec4 Q, inout vec4 a, inout vec4 aa, vec2 U, float r) {
	 X(Q,a,aa,U,vec2(r,0));
     X(Q,a,aa,U,vec2(0,r));
     X(Q,a,aa,U,vec2(0,-r));
     X(Q,a,aa,U,vec2(-r,0));
}
Main 
	Q = B(U);
    vec4 a = A(Q.xy), aa= A(Q.zw);
    Xr(Q,a,aa,U,1.);
    Xr(Q,a,aa,U,2.);
    Xr(Q,a,aa,U,4.);
    Xr(Q,a,aa,U,8.);
    for (int i = 0; i < 10; i++) {
        vec2 u = vec2(
            (10*iFrame+i)%int(R.x),
            (10*iFrame+i)/int(R.x)%int(R.y));
        if (length(U-A(u).xy)<length(U-a.xy)) Q.xy = u;
    }
    Init {
        Q.xy = U;
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// COMPUTE FORCE FIELD
Main 
	vec4 b = B(U);
	vec4 a = A(b.xy), aa = A(b.zw);
	vec2 r = a.xy-aa.xy;
	float l = length(r);
   	vec2 v = vec2(0);
	if (l>0.) 
      Q.xy = 100.*r/l/max(1.,l*l)*smoothstep(0.9*O,0.5*O,l);
    if (length(Q.xy)>.1) Q.xy = 0.1*normalize(Q.xy);
	if (iMouse.z>0.&&length(a.xy-iMouse.xy)>0.)
        Q -= vec4(2,2,0,0)*clamp(0.03*(a.xy-iMouse.xy)/dot((a.xy-iMouse.xy),(a.xy-iMouse.xy)),-2e-4,2e-4).xyxy;
	
}