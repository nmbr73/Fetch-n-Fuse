

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Graph Network" by wyatt. https://shadertoy.com/view/tssyDS
// 2020-04-09 20:04:37

Main {
    U -= 0.5*R;
    //U*=0.1;
    U += 0.5*R;
	Q = vec4(0);
    for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++) {
        	vec4 d = C(U+vec2(x,y));
            vec4 a = A(d.zw), b = A(d.zw);
            Q += .2/(1.+(.7-abs(d.z/R.x-0.5))*dot(U-a.xy,U-a.xy))*(.6+0.4*sin(5.*d.z/R.x+vec4(1,2,3,4)));
         }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texelFetch(iChannel2,ivec2(U),0)
#define D(U) texelFetch(iChannel3,ivec2(U),0)
#define Main void mainImage (out vec4 Q, vec2 U) 
#define Init if (iFrame < 1) 
#define Border if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.)
#define O 6.
#define shape(U) (round((U)/O)*O)
#define N 12
#define Z vec2(u)
float angle (vec2 a, vec2 b) {
	return atan(a.x*b.y-a.y*b.x,dot(a.xy,b.xy));
}
float sg (vec2 p, vec2 a, vec2 b) {
    if (length(a-b)<1e-4||length(a)<1e-4||length(b)<1e-4) return 1e9;
    
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	float l = (length(p-a-(b-a)*i));
    return l;
}

float pie (vec2 p, vec2 a, vec2 b) {
	vec2 m = 0.5*(a+b); // midpoint
    if (length(a-b)<1e-3) return 1e3; // ignore self
	return abs(dot(p-m,b-m)/dot(b-m,b-m)); // pojection
} 
float line (vec2 p, vec2 a, vec2 b) {
    if (length(a-b)<1.||length(a)<1.||length(b)<1.) return 1e9;
    
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	float l = (length(p-a-(b-a)*i));
    l=(pie(p,a,b));
    return l;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 force (vec2 U, vec4 a, vec2 bb) {
    vec4 b = A(bb);
    if (b.x<5.&&b.y<5.) return vec2(0);
    
    vec2 r = b.xy-a.xy, v = b.zw-a.zw,
         q = D(U).xy-D(bb).xy;
    float l = length(r), j = length(q);
    if (l < 1e-9) return vec2(0);
    vec2 f = vec2(0);
    if (l!=0.&&j!=0.){
    f += -10.*(U.x/R.x-0.5)*(bb.x/R.x-0.5)*(2.+dot(a.zw,b.zw))*r/max(1.,l*l*l);
    f += 10.*r/max(10.,l*l*l*l);
    f -= (100.)*r/max(10.,l*l*l*l*l);
    }
    if (length(f)>1.) return normalize(f);
    return f;
}
Main {
    vec2 u = shape(U);
    if (u.x+O>=R.x||u.y+O>=R.y||u.x<O||u.y<O||length(U-u)>1.) discard;
    Q = A(u);
    vec2 f = vec2(0);
    for (int x=-2;x<2; x++) {
        for (int y=-2;y<2; y++) {
            vec4 b = B(u+vec2(x,y));
            f += (
                force(U,Q,b.xy)+
                force(U,Q,b.zw));
        }
	}
    f /= 16.;
    Q.zw += f-0.001*Q.zw;
    Q.w -= 0.002;
    Q.xy += f+Q.zw*sqrt(1./(1.+dot(Q.zw,Q.zw)));
    if (length(Q.zw)>5.)Q.zw=5.*normalize(Q.zw);
    if (iMouse.z>0.) Q.zw += 10.*(Q.xy-iMouse.xy)/dot(Q.xy-iMouse.xy,Q.xy-iMouse.xy);
    if (Q.x<5.) {Q.x=5.;Q.z = +abs(Q.z);}
    if (R.x-Q.x<5.) {Q.x=R.x-5.;Q.z =-abs(Q.z);}
    if (Q.y<5.) {Q.y=5.;Q.w = +abs(Q.w);}
    //if (R.y-Q.y<5.) {Q.y=R.y-5.;Q.w =-abs(Q.w);}
    if (iFrame < 50) {
    	Q = vec4(u+0.5*O,0,0);
    }
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void X (inout vec4 Q, inout vec2 r, vec4 a, vec2 U, vec2 u, vec4 n) {
	float l = line(a.xy-U+u,a.xy,A(n.xy).xy),
          ll = line(a.xy-U+u,a.xy,A(n.zw).xy);
    if (l<r.x){
        r = vec2(l,r.x);
        Q=vec4(n.xy,Q.xy);
    } else if (l<r.y) {
    	r.y = l;
        Q.zw = n.xy;
    }
    if (ll<r.x){
    	r = vec2(ll,r.x);
        Q=vec4(n.zw,Q.xy);
    } else if (ll<r.y) {
    	r.y = ll;
        Q.zw = n.zw;
    }
}
Main {
    vec2 u = shape(U);
    vec4 a = A(u);
	Q = B(U);
    vec2 r = vec2(line(a.xy-U+u,a.xy,A(Q.xy).xy)
                 ,line(a.xy-U+u,a.xy,A(Q.zw).xy));
    
    X(Q,r,a,U,u,B(Q.xy+U-u));
    X(Q,r,a,U,u,B(Q.zw+U-u));
    
    X(Q,r,a,U,u,B(U+vec2(1,1)));
    X(Q,r,a,U,u,B(U+vec2(0,1)));
    X(Q,r,a,U,u,B(U+vec2(1,0)));
    X(Q,r,a,U,u,B(U-vec2(0,1)));
    X(Q,r,a,U,u,B(U-vec2(1,0)));
    X(Q,r,a,U,u,B(U+vec2(1,1)));
    X(Q,r,a,U,u,B(U+vec2(1,-1)));
    X(Q,r,a,U,u,B(U-vec2(1,1)));
    X(Q,r,a,U,u,B(U-vec2(1,-1)));
    X(Q,r,a,U,u,B(Q.xy+U-u+vec2(0,1)));
    X(Q,r,a,U,u,B(Q.xy+U-u+vec2(1,0)));
    X(Q,r,a,U,u,B(Q.xy+U-u-vec2(0,1)));
    X(Q,r,a,U,u,B(Q.xy+U-u-vec2(1,0)));
    X(Q,r,a,U,u,B(Q.xy+U-u+vec2(1,1)));
    X(Q,r,a,U,u,B(Q.xy+U-u+vec2(1,-1)));
    X(Q,r,a,U,u,B(Q.xy+U-u-vec2(1,1)));
    X(Q,r,a,U,u,B(Q.xy+U-u-vec2(1,-1)));
    X(Q,r,a,U,u,vec4(U-O*vec2(1,0),U+O*vec2(1,0)));
    X(Q,r,a,U,u,vec4(U-O*vec2(0,1),U+O*vec2(0,1)));

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<

void Y (inout vec4 Q, vec2 U, vec4 n) {
	float l = length(U-A(n.xy).xy),//sg(U,A(n.xy).xy,A(n.zw).xy),
          ll = length(U-A(Q.xy).xy);//sg(U,A(Q.xy).xy,A(Q.zw).xy);
    if (l<ll) Q.xy = n.xy;
}
void X (inout vec4 Q, vec2 U, vec4 n) {
	float l = length(U-A(n.zw).xy),//sg(U,A(n.xy).xy,A(n.zw).xy),
          ll = length(U-A(Q.zw).xy);//sg(U,A(Q.xy).xy,A(Q.zw).xy);
    if (l<ll) Q.zw = n.zw;
}
Main {
	Q = C(U).xyxy;
    if (iFrame%N==0) Q.xy = U;
    else {
        float k = exp2(float(N-1-(iFrame%N)));
    	Y(Q,U,C(U+vec2(0,k)));
    	Y(Q,U,C(U+vec2(k,0)));
    	Y(Q,U,C(U-vec2(0,k)));
    	Y(Q,U,C(U-vec2(k,0)));
    }

	X(Q,U,C(U+vec2(0,1)));
    X(Q,U,C(U+vec2(1,0)));
    X(Q,U,C(U-vec2(0,1)));
    X(Q,U,C(U-vec2(1,0)));
    
    X(Q,U,C(U+vec2(1,1)));
    X(Q,U,C(U+vec2(1,-1)));
    X(Q,U,C(U-vec2(1,1)));
    X(Q,U,C(U-vec2(1,-1)));
    
    X(Q,U,C(U+vec2(0,4)));
    X(Q,U,C(U+vec2(4,0)));
    X(Q,U,C(U-vec2(0,4)));
    X(Q,U,C(U-vec2(4,0)));
    
    X(Q,U,C(U));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
	Q.xy = 0.3*shape(U);
}