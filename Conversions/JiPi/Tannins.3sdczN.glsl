

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    vec3 norm = 
        normalize(vec3(e.w-w.w,n.w-s.w,3)),
        ref = reflect(vec3(0,0,-1),norm);
   
	vec4 b = B(U);
    Q = b*b.w;
    vec4 t = texture(iChannel3,ref);
    Q *= 0.8+30.*t*t*t*t;
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
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
    Q = vec4(0);
	for (int x = -1; x <= 1; x++)
	for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 a = A(U+u);
        #define q 1.1
		vec2 w1 = clamp(U+u+a.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+a.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q.xyz += m*a.w*a.xyz;
        Q.w += m*a.w;
    }
    if (Q.w>0.)
    	Q.xyz/=Q.w;
    Q.xy = clamp(Q.xy,vec2(-1),vec2(1));
    if (iFrame < 1) 
    {
        Q = vec4(0,0,.1,0);
    }
    vec4 d = D(U);
    if ((iFrame < 10||iMouse.z>0.)&&ln(U,d.xy,d.zw)<2.)
        Q = vec4(clamp(1e-2*(d.xy-d.zw),.5,.5),.5,.5);
    if (U.x<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
	Q = A(U);vec4 q = Q;
    for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 a = A(U+u);
        u = (u)/dot(u,u);
        Q.xy -= q.w*0.125*a.w*(.6*a.w*a.z+a.w-(1.-.6*a.w))*u;
        Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
    }
    if (Q.w < 1e-3) Q.z *= 0.;
    Q.y -= 1e-2*Q.w;
    Q.xy *= .5+.5*pow(Q.w,.1);
    
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// keep track of mouse
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 p = texture(iChannel0,fragCoord/iResolution.xy);
    if (iMouse.z>0.) {
        if (p.z>0.) fragColor =  vec4(iMouse.xy,p.xy);
    	else fragColor =  vec4(iMouse.xy,iMouse.xy);
    }
    else fragColor = vec4(-iResolution.xy,-iResolution.xy);
	if (iFrame < 10) fragColor = vec4(.4,.5+.1*sin(float(iFrame)),.6,.5)*R.xyxy;;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    Q = vec4(0);
	for (int x = -1; x <= 1; x++)
	for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 a = A(U+u), b = B(U+u);
        #define q 1.1
		vec2 w1 = clamp(U+u+a.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+a.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q.xyz += m*a.w*b.xyz;
        Q.w += m*a.w;
    }
    if (Q.w>0.)
    	Q.xyz/=Q.w;
    if (iFrame < 1) 
    {
        Q = vec4(0,0,0,0);
    }
    vec4 d = D(U);
    if ((iFrame < 10||iMouse.z>0.)&&ln(U,d.xy,d.zw)<2.)
        Q = vec4(1.+0.5*sin(iTime+iTime*vec3(1,2,3)),.5);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;

}