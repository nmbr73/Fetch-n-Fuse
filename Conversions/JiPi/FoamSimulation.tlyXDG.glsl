

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Triangulated Actors****" by wyatt. https://shadertoy.com/view/ttGXDG
// 2020-03-06 04:21:54

Main 
    vec4 b = B(U),
         a = A(b.xy), aa = A(b.zw);
    
    float 	
          r = length(a.xy-aa.xy), 
          o = length(U-a.xy),
          w = sg(U,a.xy,aa.xy),
          v = pie(U,a.xy,aa.xy);
    
    Q = 0.5+0.5*smoothstep(12.,8.,min(r,10.*o))*
        smoothstep(3.,0.,w)*
        vec4(.9,0.8,1,1);
    Q = 0.5+0.5*sin(1.+3.1*Q+2.*abs(D(U).z));
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
	vec2 u = floor(U/8.+0.5)*8.;
    u = clamp(u,vec2(10),R-10.);
    Q = A(U);
    Q.zw = C(Q.xy).zw;
    for (int x = -2; x <= 2; x++)
    for (int y = -2; y <= 2; y++)
    Q.zw += C(Q.xy+vec2(x,y)).xy;
    Q.zw += 1e-6*sin(u);
    if (length(Q.zw)>.8) Q.zw = 0.8*normalize(Q.zw);
    Q.xy += Q.zw;
    
    if (Q.x<5.)Q.x=5.;
    if (Q.y<5.)Q.y=5.;
    if (R.x-Q.x<5.)Q.x=R.x-5.;
    if (R.y-Q.y<5.)Q.y=R.y-5.;

	if (Q.x<1.&&Q.y<1.) Q = 10.*R.xyxy;    
    if (init)
    {
    	
        Q = vec4(u,0,0);
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
    Xr(Q,a,aa,U,3.);
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
	if (l>2.3) 
      Q.xy = 
        0.1*(-r/l/l+20.*r/l/l/l/l);
    
    
    
    
    Q.zw = C(U-C(U).zw).zw;
    vec4 
    n = D(U+vec2(0,1)),
    e = D(U+vec2(1,0)),
    s = D(U-vec2(0,1)),
    w = D(U-vec2(1,0));
    Q.zw = mix(Q.zw,
           mix(aa.zw,a.zw,0.5+0.5*pie(U,a.xy,aa.xy)),
           smoothstep(2.5,1.5,sg(U,a.xy,aa.xy))*
           smoothstep(12.,8.,min(length(aa.xy-a.xy),10.*length(U-a.xy)))
    );
    Q.zw -= 0.25*vec2(e.z-w.z,n.z-s.z);
    if (length(Q.zw)>0.8) Q.zw = 0.8*normalize(Q.zw);
    if (iMouse.z>0.&&length(a.xy-iMouse.xy)>0.)
        Q -= vec4(1,1,-1,-1)*clamp(0.03*(a.xy-iMouse.xy)/dot((a.xy-iMouse.xy),(a.xy-iMouse.xy)),-2e-4,2e-4).xyxy;
	
    if (U.x < 2.||U.y < 2.||R.x-U.x<2.||R.y-U.y<2.) Q.zw *= 0.;
    
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main 
	Q = D(U-C(U).zw);
    vec4 
    n = C(U+vec2(0,1)),
    e = C(U+vec2(1,0)),
    s = C(U-vec2(0,1)),
    w = C(U-vec2(1,0));
    Q.zw = vec2(Q.z,0)-0.25*(e.z-w.z+n.w-s.w);
}