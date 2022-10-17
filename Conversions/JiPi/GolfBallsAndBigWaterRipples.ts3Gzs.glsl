

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Visualize Both Buffers

//inspired by these two excellent shaders
//https://www.shadertoy.com/view/wsfGzS
//https://www.shadertoy.com/view/tsBSWh

#define l normalize(vec2(1))

#define A(U) texelFetch(iChannel0,ivec2(U),0).x
#define B(U) texelFetch(iChannel1,ivec2(U),0).xy
#define dA(U,R) A(U+R)-A(U-R)
#define DB(U) length(B(U)-U)
#define dDB(U,R) DB(U+R)-DB(U-R)

void mainImage(out vec4 O,in vec2 U){
    O=vec4(
		vec3(.3,.3,1.)+
        dot(
            vec2(dA(U,vec2(1,0)),
                 dA(U,vec2(0,1))),
            l)+
        (.5*dot(
            normalize(
            	vec2(dDB(U,vec2(1,0)),
                 	 dDB(U,vec2(0,1)))),
        		l)
         +.5)
         *max(1.-.5*length(U-B(U)),0.),
      1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Water "Physics"

//Random hash from here:
//https://www.shadertoy.com/view/4djSRW
vec2 hash21(float p){
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//Comment this line out to try the simulation without automatic drops
#define AUTO

//Ripples code based on a few old ripples tutorials
#define T(a,b) texelFetch(iChannel0,ivec2(U)+ivec2(a,b),0)
void mainImage(out vec4 O,in vec2 U){
    float h=(T( 0, 1).x+
        	 T( 0,-1).x+
        	 T( 1, 0).x+
        	 T(-1, 0).x)/2.,
    	  t=.99*(h-T(0,0).y);
    
    if((iMouse.z>.5&&10.-length(iMouse.xy-U)>0.)
#ifdef AUTO       
       ||(iFrame%20==0&&10.-length(iResolution.xy*hash21(float(iFrame))-U)>0.)
#endif
      ){
    	t=1.;
    }
    vec4 B=texelFetch(iChannel1,ivec2(U),0);
    O=float(iFrame>10)*
        vec4(t-max(4.-length(B.xy-U),0.)*length(B.zw+vec2(.001))/200.
             ,T(0,0).x
             ,0
             ,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Particle storage and updating

#define Pspawn 20.
#define Pgrid 5.

//Similar particle implementation to:
//https://www.shadertoy.com/view/MlVfDR

#define A(U) texelFetch(iChannel0,ivec2(U),0).x
#define B(a,b) texelFetch(iChannel1,ivec2(U)+ivec2(a,b),0)
#define dA(U,R) A(U+R)-A(U-R)
#define DB(U) length(B(U).xy-U)

#define N(U,A,B) if(length(U-B.xy)<length(U-A.xy)){A = B;}

void mainImage(out vec4 O,in vec2 U){
    O  =  B( 0, 0) ;
    N(U,O,B( 0, 1));
    N(U,O,B( 1, 0));
    N(U,O,B( 0,-1));
    N(U,O,B(-1, 0));
    N(U,O,B( 2, 2));
    N(U,O,B( 2,-2));
    N(U,O,B(-2, 2));
    N(U,O,B(-2,-2));
    if(iFrame==0||(length(U-O.xy)>Pspawn)){
        O=vec4(round(U/Pgrid)*Pgrid,0.,0.);
    }
    O.zw-=vec2(dA(O.xy,vec2(1,0)),
               dA(O.xy,vec2(0,1)));
    O.wz*=.99;
    O.xy+=O.zw;
}