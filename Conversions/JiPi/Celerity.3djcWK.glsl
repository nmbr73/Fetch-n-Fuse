

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    Neighborhood;
    vec2 g = vec2(e.w*e.z-w.z*w.w,n.z*n.w-s.z*s.w);
    vec3 no = normalize(vec3(g,.01));
    vec3 re = reflect(no,vec3(0,0,1));
	Q = 0.5+0.5*(1.-100.*length(g))*sin(abs(A(U).wwww)*3.1+3.1+vec4(1,2,3,4));
	Q *= 0.8+0.2*texture(iChannel3,re);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define Main void mainImage(out vec4 Q, vec2 U) 

#define Neighborhood vec4 n = A(U+vec2(0,1)), e = A(U+vec2(1,0)), s = A(U-vec2(0,1)), w = A(U-vec2(1,0));

#define Border if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy*=0.;
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
	Q = A(U);
    Neighborhood;
    Q.xy -= 0.25*vec2(e.z-w.z,n.z-s.z)*Q.w;
    
    if (iMouse.z>0.) {
    	if (length(U-iMouse.xy)<20.) Q.xyw=vec3(.2*cos(iTime),.2*sin(iTime),0.5+0.5*sin(iTime));
    }
    else if (length(U-vec2(.8,.2)*R)<10.)Q.xyw=vec3(-.3,.8,.6);
    
    Border;
    if(iFrame < 1) Q = vec4(0,0,0,max(U.x/R.x-0.2,0.));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
	Q = A(U);
    Neighborhood;
    Q.z -= 0.25*(e.x*e.w-w.x*w.w+n.y*n.w-s.y*s.w);
    Border;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    Neighborhood;
    U -= 0.25*(n.xy+e.xy+s.xy+w.xy);
	Q = A(U);
    Border;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
    Neighborhood;
	Q = A(U);
    Q.w -= 0.25*(n.w*n.y-s.w*s.y+e.x*e.w-w.x*w.w);
    Border;
}