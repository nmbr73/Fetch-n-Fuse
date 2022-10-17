

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Physarum testy test" by wyatt. https://shadertoy.com/view/wsXyzB
// 2020-03-24 05:18:34

Main {
    vec4 b = B(U), a = A(U);
	Q = b.zzzz;
    //Q = vec4(1)*smoothstep(1.,0.,length(U-a.xy));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(mod((U),R)),0)
#define B(U) texture(iChannel1,U/R)
#define C(U) texture(iChannel2,U/R)
#define D(U) texture(iChannel3,U/R)
#define Main void mainImage (out vec4 Q, vec2 U) 
#define Init if (iFrame < 1) 
#define Border if (U.x<1.||R.x-U.x<1.||U.y<1.||R.y-U.y<1.)
#define T(U) A((U)-dt*A(U).xy)
#define NeighborhoodT vec4 n = T(U+vec2(0,1)), e = T(U+vec2(1,0)), s = T(U-vec2(0,1)), w = T(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define grd 0.25*vec2(e.z-w.z,n.z-s.z)
#define grdw 0.25*vec2(e.w-w.w,n.w-s.w)
#define div 0.25*(e.x-w.x+n.y-s.y)
#define ro(a) mat2(cos(a),sin(-a),sin(a),cos(a))
float sg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	return (length(p-a-(b-a)*i));
}
//Dave H
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy)*2.-1.;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec2 r) {
    vec4 n = A(U+r);
	if (sg(U,n.xy,n.zw)<sg(U,Q.xy,Q.zw)) Q=n;
}
void X (inout vec4 Q, vec2 U, float r) {
	 X(Q,U,vec2(r,0));
     X(Q,U,vec2(0,r));
     X(Q,U,vec2(0,-r));
     X(Q,U,vec2(-r,0));
}
Main {
	Q = A(U);
    X(Q,U,1.);
    X(Q,U,2.);
    X(Q,U,3.);
    float b = B(Q.xy).z;
    vec2 v = (Q.xy-Q.zw)+C(Q.xy).xy+1.*hash23(vec3(Q.xy,iFrame));
    for (int x = -3; x<=3;x++) {
    	for (int y = -3; y<=3; y++) {
    		vec4 n = A(Q.xy+vec2(x,y));
            vec2 r = n.xy-Q.xy;
            float l = length(r);
            if (l>0.) v -= .1*r/l/l;
    	}	
    }
	
	//if (Q.x<3.){v.x=abs(v.x);}
    if (Q.y<3.){v.y=abs(v.y);}
    if (R.x-Q.x<3.){v.x=-abs(v.x);}
    if (R.y-Q.y<3.){v.y=-abs(v.y);}
    if (iMouse.z>0.)v -= normalize(Q.xy-iMouse.xy);
    v.x += 0.01;
	if (length(v)>0.) v = 2.*normalize(v);
    Q = vec4(
        Q.xy+v,Q.xy
    );
    if (abs(U.y-0.5*R.y)<10.&&R.x-U.x<1.) {
		Q.xy = vec2(R.x,U.y);
        Q.zw = Q.xy+vec2(1,0);
	}
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    Q = A(U)*0.99;
    vec4 a = B(U);
    Neighborhood;
    Q.w = m.w;
    Q = mix(Q,vec4(a.xy-a.zw,Q.z+1e-1,Q.w+1e-1),exp(-1.*sg(U,a.xy,a.zw)));

    Init Q = vec4(0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    Neighborhood;
    vec4 b = B(U);
    Q.xy = 3.*m.xy+2.*grd;
    Q.z = div;
    Init Q = vec4(0);
}