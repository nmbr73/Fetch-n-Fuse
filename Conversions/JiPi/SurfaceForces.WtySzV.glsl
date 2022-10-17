

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Surface Forces" by wyatt. https://shadertoy.com/view/3tKXWm
// 2020-03-01 23:03:05

// Fork of "Last Fluid" by wyatt. https://shadertoy.com/view/3tcSDj
// 2020-02-28 04:10:38

float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
Main {
    vec4 b = B(U);
    Neighborhood;
    vec3 no = normalize(0.3*b.xyz+vec3(grdx+grdy+grdz,2)),
        re = reflect(no,vec3(0,0,1));
    Q.xyz = abs(0.5+0.8*b.xyz*mat3(.6,.7,.7,.3,.6,.1,-.4,.9,.1));
    float l = ln(vec3(0.5,0.5,6)*R.xyy,vec3(U,b.x),vec3(U,b.x)+re);
    Q *= 0.5*exp(-.001*l)+0.5*exp(-.01*l)+exp(-0.1*l);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Fluid step 1 : d/dt V = d/dx P

Main {
    
    vec3 f = A(U).xyz;
	Q = B(U-0.5*B(U-0.5*B(U).xy).xy);
    Neighborhood;
    Q.xy -= grd;  
    Neighborhood3;
    vec2 g = grdx*(3.*f.y-f.z)+grdy*(3.*f.z-f.x)+grdz*(3.*f.x-f.y);
    Q.xy += 0.05*g;
	Q.xy *= 0.99;
   Border Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Fluid step 2 : d/dt P = d/dx V

Main {
	Q = B(U-0.5*A(U-0.5*A(U).xy).xy);
    Neighborhood;
    Q.w  -= div;
    Neighborhood2;
    Q.xyz -= 0.25*(N*n.y-S*s.y+E*e.x-W*w.x).xyz;
    if (length(Q.xyz)>0.) 
        Q.xyz = mix(Q.xyz,normalize(Q.xyz),0.01);
    if (iFrame < 1) Q = vec4(0);
    Input
        Q.xyz = sin(vec3(1,2,3)*iTime);
    Border Q.x = -1.;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define Neighborhood2 vec4 N = B(U+vec2(0,1)), E = B(U+vec2(1,0)), S = B(U-vec2(0,1)), W = B(U-vec2(1,0)), M = 0.25*(n+e+s+w);
#define Neighborhood3 n = D(U+vec2(0,1)), e = D(U+vec2(1,0)), s = D(U-vec2(0,1)), w = D(U-vec2(1,0)), m = 0.25*(n+e+s+w);
#define grd 0.25*vec2(e.w-w.w,n.w-s.w)
#define grdx 0.25*vec2(e.x-w.x,n.x-s.x)
#define grdy 0.25*vec2(e.y-w.y,n.y-s.y)
#define grdz 0.25*vec2(e.z-w.z,n.z-s.z)
#define grd2 0.25*vec2(E.x-W.x,N.x-S.x)
#define div 0.25*(e.x-w.x+n.y-s.y)
#define Main void mainImage(out vec4 Q, vec2 U)
#define I 20.
#define loop for (float i = -I; i <= I; i++)
#define std vec4(16,8,4,1)
#define gau(x) 0.3989422804/std*exp(-x*x/std/std)
#define Input if ((iMouse.z>0.&&length(U-iMouse.xy)<30.)||(iFrame<1&&length(U-0.5*R)<26.))
#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// gaussian blur pass 1
Main {
	Q = vec4(0);
	loop Q += gau(i) * B(U+vec2(i,0));

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// gaussian blur pas2
Main {
	Q = 0.1*D(U);
	loop Q += gau(i) * C(U+vec2(0,i));
}