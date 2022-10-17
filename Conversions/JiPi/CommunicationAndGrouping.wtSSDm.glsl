

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*min(dot(p-a,b-a),0.)/dot(b-a,b-a));}
void mainImage( out vec4 Q, in vec2 U )
{
 	vec4 
        n = D(U+vec2(0,1)),
        e = D(U+vec2(1,0)),
        s = D(U-vec2(0,1)),
        w = D(U-vec2(1,0));
    vec4 a = A(U);
    Q = C(U);
    Q = vec4(.7,.8,.9,1);
    vec3 no = normalize(vec3(e.w-w.w,n.w-s.w,2));
    vec3 re = reflect(normalize(vec3((U-0.5*R)/R.y,1)),no);
    float light = ln(vec3(2,2,2),vec3(U/R.y,0),vec3(U/R.y,0)+re);
    Q *= (exp(-light)+.4*exp(-.3*light))*(0.7+0.5*dot(re,normalize(vec3(U/R.y,0)-vec3(2,2,2))));

    
	
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void swap (inout vec4 Q, vec2 U, vec2 r) {
	vec4 n = A(U+r);
    if (length(U-n.xy)<length(U-Q.xy)) Q = n;
} 
void mainImage( out vec4 Q, in vec2 U )
{
   Q = A(U);
   swap(Q,U, vec2(0,1));
   swap(Q,U, vec2(1,0));
   swap(Q,U,-vec2(0,1));
   swap(Q,U,-vec2(1,0));
   swap(Q,U, vec2(1,1));
   swap(Q,U, vec2(1,-1));
   swap(Q,U,-vec2(1,1));
   swap(Q,U,-vec2(1,-1));
   swap(Q,U, vec2(0,2));
   swap(Q,U, vec2(2,0));
   swap(Q,U,-vec2(0,2));
   swap(Q,U,-vec2(2,0));
    
    vec2 u = mix(Q.xy,U,0.);
    vec4
        n = D(u+vec2(0,1)),
        e = D(u+vec2(1,0)),
        s = D(u-vec2(0,1)),
        w = D(u-vec2(1,0));
    Q.xy -= .5*Q.zw;
    vec2
        g = vec2(e.w-w.w,n.w-s.w);
    Q.zw = -g;
    if (length(Q.zw)>1.) Q.zw = normalize(Q.zw);
  	
    if (iFrame < 1||(iMouse.z>0.&&length(U-iMouse.xy)<10.)){
        vec2 u =floor(U/10.+0.5)*10.;
        Q = vec4(u,0,0);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

void mainImage( out vec4 Q, in vec2 U )
{
   Q = vec4(0);
    
    for (int i = -I; i <= I; i++) {
        vec2 u = U+vec2(i,0);
        vec4 a = A(u);
    	Q += (exp(-O*float(i*i)))*smoothstep(1.5,1.,length(u-a.xy));
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<

void mainImage( out vec4 Q, in vec2 U )
{
   Q = vec4(0);
    
    for (int i = -I; i <= I; i++) {
    	Q += (exp(-O*float(i*i)))*A(U+vec2(0,i));
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

#define O vec4(.01,.2,.5,.01)
#define I 20
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<

void mainImage( out vec4 Q, in vec2 U )
{
   Q = C(U);
   
   Q.w = .1*Q.x - Q.y;
   
    if (iMouse.z>0.) Q.w -= 100.*exp(-.05*length(iMouse.xy-U));
   Q.w = mix(Q.w,D(U).w,.75);
}