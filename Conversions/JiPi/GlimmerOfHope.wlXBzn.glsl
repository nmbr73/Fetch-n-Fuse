

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main 
{
    vec4 b = B(U);
    Q = A(U);
    for (float j = 0.; j < 6.2; j += 6.2/3.)
        for (float i = 0.; i < 7.; i++) {
            vec4 a = A(U+vec2(i,0)*ei(j+.1*iTime));
            Q += .5*a*(-1.+exp(8.*a))*gauss(i,3.);
        }
    Q = mix(vec4(.7,.7,.7,1)-b.zzzz+.1*Q,Q,2.*b.z);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)
vec4 pw (vec4 t, float p) {
	return vec4(pow(t.x,p),pow(t.y,p),pow(t.z,p),1);
}
#define ei(a) mat2(cos(a),-sin(a),sin(a),cos(a))
#define gauss( i, std) 0.3989422804/(std)*exp(-.5*(i)*(i)/(std)/(std))

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main 
{
    Q = vec4(0);
	for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    {
        vec2 u = vec2(x,y);
        vec4 aa = A(U+u);
        
    	#define q 1.5
		vec2 w1 = clamp(U+u+aa.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+aa.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q.xy += m*aa.z*aa.xy;
        Q.z += m*aa.z;
    }
    if (Q.z>0.)
    	Q.xy/=Q.z;
    if (iFrame < 1) 
    {
        Q = vec4(0,0,0,0);
        if (length(U/R-0.5)<0.4)Q.z = .8;
    }
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.) Q.xz = vec2(.5,.5);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main 
{
    Q = A(U);
    vec4 c = C(U), d = D(c.xy);
    float f = -.5*(d.x*2.-1.)-.5;
	for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    if (x != 0||y!=0)
    {
        vec2 u = vec2(x,y);
        vec4 aa = A(U+u), cc = C(U+u), dd = D(cc.xy);
        float ff = dd.x*3.-dd.z*2.;
        Q.xy -= 0.125*aa.z*normalize(u)*aa.z*(ff-.8+aa.z);
    }
    Q.xy*=1.-.005*(d.z+d.x);
    Q.y += 5e-4*Q.z*(d.y*2.-1.);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;

    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main 
{
    Q = vec4(0);
    float w = 0.;
	for (int x = -1; x<=1; x++)
	for (int y = -1; y<=1; y++)
    {
        vec2 u = vec2(x,y);
        vec4 aa = A(U+u), cc = C(U+u), dd = D(cc.xy);
        
    	#define q 1.
		vec2 w1 = clamp(U+u+aa.xy-0.5*q,U - 0.5,U + 0.5),
             w2 = clamp(U+u+aa.xy+0.5*q,U - 0.5,U + 0.5);
        float m = (w2.x-w1.x)*(w2.y-w1.y)/(q*q);
        Q += m*aa.z*cc;
        w += m*aa.z;
    }
    if (w>0.)
    	Q/=w;
    Q.xy = mix(Q.xy,U,1e-5);
    if (iFrame < 1) Q = vec4(U,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
    vec4 a = A(U), c = C(U), d = D(c.xy);
	float std = d.x+d.y-2.*d.z;
    
  	vec4
        n = A(U+vec2(0,1)),
        e = A(U+vec2(1,0)),
        s = A(U-vec2(0,1)),
        w = A(U-vec2(1,0));
    vec3 norm = 
        normalize(vec3(e.z-w.z,n.z-s.z,-.01*(std*std)));
    vec3 ref = reflect(vec3(0,0,1), norm);
    Q = min(a.z,1.)*d*sqrt(d);
    vec4 tx = texture(iChannel1,ref);
    Q *= 0.5+0.5*tx+.1*norm.x;
    Q *= exp(tx);
    Q = atan(Q);
}