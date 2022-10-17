

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4 bb = B(U), cc = C(U), dd = D(U),
         a = A(bb.xy), b = A(cc.xy);
    float l = length(U-a.xy),
        j = smoothstep(5.,4.,l);
   	Q = vec4(j)*max(0.6+0.4*sin(.3+(.1*cc.x/R.x+cc.y+1./(1.+10.*length(dd.zw))*vec4(1,2,3,4))),0.);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texelFetch(iChannel2,ivec2(U),0)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage (out vec4 Q, vec2 U)
#define Init if (iFrame < 1) 
#define init  (iFrame < 1) 
#define Border if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.)
float sg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return (length(p-a-(b-a)*i));
}
float mp (vec2 p, vec2 a, vec2 b) {
	vec2 m = 0.5*(a+b); // midpoint
    if (length(a-b)<1e-3) return 1e3; // ignore self
	return abs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Information Storage
Main {
	Q = A(U);
    if (iMouse.z>0.) {
        vec2 r = (iMouse.xy-Q.xy);
        Q.zw -= r/dot(r,r);
    }
   	if (iFrame > 10) 
    Q.zw -= D(Q.xy).zw;
    //Q.w -= 3e-3;
    if (length(Q.zw)>1.) Q.zw = normalize(Q.zw);
    Q.xy += Q.zw;
    

    if (Q.x<5.) {Q.x = 5.; Q.z *= -1.;}
    if (R.x-Q.x<5.) {Q.x = R.x-5.; Q.z *= -1.;}
    if (Q.y<5.) {Q.y = 5.; Q.w *= -1.;}
    if (R.y-Q.y<5.) {Q.y = R.y-5.; Q.w *= -1.;Q.z*=0.;}
    
    if (init)
    {
    	U = floor(U/15.+0.5)*15.;
        Q = vec4(clamp(U,vec2(3),R-3.),0,0);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Nearest Individual
void X (inout vec4 Q, inout vec4 a, vec2 U, vec2 r) {
    vec4 b = B(U+r);
    vec4 n = A(b.xy);
    float ln = length(n.xy-U), la = length(a.xy-U);
    if (ln<la) {
    	Q.xy = b.xy;
        a.xy = n.xy;
    }
}
void Xr (inout vec4 Q, inout vec4 a, vec2 U, float r) {
	 X(Q,a,U,vec2(r,0));
     X(Q,a,U,vec2(0,r));
     X(Q,a,U,vec2(0,-r));
     X(Q,a,U,vec2(-r,0));
}
Main {
	Q = B(U);
    vec4 a = A(Q.xy);
    Xr(Q,a,U,1.);
    Xr(Q,a,U,2.);
    Xr(Q,a,U,3.);
    Xr(Q,a,U,4.);
   	Init {
    	Q.xy = A(U).xy;
        Q.zw = vec2(0.);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Nearest Normalized Bisector
void X (inout vec4 Q, inout vec4 a, inout vec4 b, vec2 U, vec2 r) {
    vec4 Qn = C(U+r), qn = B(U+r), na = A(qn.xy), nb = A(Qn.xy);
   	float l = mp(U,a.xy,b.xy);
    
    if (mp(U,a.xy,na.xy)<l) {
    	Q = qn;
        b = na;
    }
    if (mp(U,a.xy,nb.xy)<l) {
        Q = Qn;
        b = nb;
    }
}
void Xr (inout vec4 Q, inout vec4 a, inout vec4 b, vec2 U, float r) {
	 X(Q,a,b,U,vec2(r,0));
     X(Q,a,b,U,vec2(0,r));
     X(Q,a,b,U,vec2(0,-r));
     X(Q,a,b,U,vec2(-r,0));
}
Main {
	Q = C(U);
    vec4 q = B(U),
         a = A(q.xy),
         b = A(Q.xy);
    Xr (Q,a,b,U,1.);
    Xr (Q,a,b,U,2.);
    Xr (Q,a,b,U,3.);
    Xr (Q,a,b,U,4.);
    
    vec2 r = b.xy - a.xy;
    if (length(r)>0.&&length(b.xy)>0.) 
        Q.zw = -r/dot(r,r)+10.*r/dot(r,r)/length(r) 
        - 0.1*abs(dot(b.zw-a.zw,r))*(b.zw-a.zw)*exp(-0.05*dot(r,r));
    
    Init {
        Q = A(U);
    	Q.zw = vec2(0);
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Sum the forces
Main {
	Q = vec4(0);
    for (int x = -3; x <= 3; x++)
    for (int y = -3; y <= 3; y++) {
    	vec2 r = vec2(x,y);
        Q += C(U+r)*exp(-.5*dot(r,r));
    }
}