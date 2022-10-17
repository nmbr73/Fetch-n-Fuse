

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4 a = A(U), b = B(U);
    Q = a+b;
    float n = hash(U+vec2(0,1));
    float e = hash(U+vec2(1,0));
    float s = hash(U-vec2(0,1));
    float w = hash(U-vec2(1,0));
    vec3 no = normalize(vec3(e-s,n-s,1));
    
    Q = .9+.05*no.y-sqrt(Q);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
float ln (vec3 p, vec3 a, vec3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
float hash (vec2 p) // Dave H
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
    Q = A(U+vec2(0,1));
    Q = A(U+vec2(0,.05*length(Q.xyz)*Q.w));
    vec4 q = vec4(0);
    for (int x = -1; x<= 1; x++)
    for (int y = -1; y<= 1; y++)
    if (x!=0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        float h = hash(U+0.5*u);
        float m = (length(a.xyz));
        m = min(m,1.);
        vec4 w = vec4(atan(Q.w*a.www),a.w);
        q += m*pow(h,6.)*(a-Q)/dot(u,u);
    }
    Q += 0.125*q;
    
    vec4 d = D(U);
    if (iMouse.z>0.&&ln(U,d.xy,d.zw)<.025*R.y){
        Q = 0.5+0.5*sin(iTime+vec4(1,2,3,4));
        Q.w = 1.;
    }
    //if (iFrame < 1) Q = 1.-B(U);
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {

    Q = B(U);
    Q += 5e-4*A(U);
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    Q = A(U)*(1.-5e-4);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//Mouse
void mainImage( out vec4 C, in vec2 U )
{
    vec4 p = texture(iChannel0,U/iResolution.xy);
   	if (iMouse.z>0.) {
      if (p.z>0.) C =  vec4(iMouse.xy,p.xy);
    else C =  vec4(iMouse.xy,iMouse.xy);
   }
    else C = vec4(-iResolution.xy,-iResolution.xy);
}