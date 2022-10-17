

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Triangulated Actors*" by wyatt. https://shadertoy.com/view/ttdXDB
// 2020-02-22 04:42:08


Main {
    vec4 b = B(U),
         a = A(b.xy), aa = A(b.zw);
    
    	vec2 bb = floor(b.xy/10.+0.5)*10.;
        bb = clamp(bb,vec2(30),R-30.)/R;
    float o = length(U-a.xy),
          w = sg(U,a.xy,aa.xy),
          v = pie(U,a.xy,aa.xy);
    Q = .7-vec4(exp(-w-0.1*(v+1.)*length(aa.xy-a.xy)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage (out vec4 Q, vec2 U)
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
Main {
	Q = A(U);;
    for (int x = -3; x <= 3; x++)
    for (int y = -3; y <= 3; y++)
    Q.zw += C(Q.xy+vec2(x,y)).xy;
    vec2 v = Q.zw;
    float V = length(v);
    v = v*sqrt(V*V/(1.+V*V));
    //if (length(v)>0.)v = normalize(v);
    Q.xy += v;
    
    if (Q.x<2.)Q.x=2.;
    if (Q.y<2.)Q.y=2.;
    if (R.x-Q.x<2.)Q.x=R.x-2.;
    if (R.y-Q.y<2.)Q.y=R.y-2.;

    
    if (init)
    {
    	U = floor(U/5.+0.5)*5.;
        U = clamp(U,vec2(40),R-40.);
        Q = vec4(U,0,0);
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
Main {
	Q = B(U);
    vec4 a = A(Q.xy), aa= A(Q.zw);
    Xr(Q,a,aa,U,1.);
    Xr(Q,a,aa,U,2.);
    Xr(Q,a,aa,U,3.);
    Xr(Q,a,aa,U,4.);
    Xr(Q,a,aa,U,5.);
    Xr(Q,a,aa,U,6.);
   	Init {
        Q.xy = U;
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// COMPUTE FORCE FIELD
Main {
	vec4 b = B(U);
	vec4 a = A(b.xy), aa = A(b.zw);
	vec2 r = a.xy-aa.xy;
	float l = length(r);
   	vec2 v = vec2(0);
	float f = 0.;
    if (length(a.xy-aa.xy)>0.&&length(aa.zw-a.zw)>0.) f = abs(dot(normalize(a.xy-aa.xy),normalize((aa-a).zw)));
    if (l>0.&&length(a.xy-aa.xy)>4.42) v = 
        -1.*r/l/l+
       	50.*f*(aa-a).zw/max(.1,l*l*l*l);
       ;
    if (a.x<20.) v.x += .02;
    if (R.x-a.x<20.) v.x -= .02;
    if (a.y<20.) v.y += .02;
    if (R.y-a.y<20.) v.y -= .02;
    Q.xy = 3e-2*v;
    if (iMouse.z>0.&&length(a.xy-iMouse.xy)>0.) Q.xy += clamp(0.03*(a.xy-iMouse.xy)/dot((a.xy-iMouse.xy),(a.xy-iMouse.xy)),-2e-4,2e-4);
	if (iFrame < 10) Q.xy += 3e-2*(U-0.5*R)/dot(U-0.5*R,U-0.5*R);
}