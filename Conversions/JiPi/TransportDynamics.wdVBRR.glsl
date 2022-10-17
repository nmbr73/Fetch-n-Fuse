

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    vec4 a = A(U);
	Q = 1.1-atan(1.5*a.w)*(0.8+0.5*sin(.6+a.z+vec4(1,2,3,4)));
    //Q = a.wwww;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texture(iChannel0,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U)
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
float ln (vec3 p, vec3 a, vec3 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Apply forces
vec2 F (vec2 u) {
    vec4 a = A(gl_FragCoord.xy + u);
	return .125*max(a.w-.25,0.)*(a.w-.5)*u/dot(u,u);
}
Main {
	Q = A(U);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    if (x!=0||y!=0)
       Q.xy -= F(vec2(x,y));
   
    Q.y -= .2/R.y*(1.+Q.z);
	//if (length(Q.xy)>2.) Q.xy = 2.*normalize(Q.xy);
    if (iFrame < 1) {
    	Q = vec4(0,0,0,0);
        if (U.y<.5*R.y) Q.w = 1.;
        if (length(U-0.5*R)<0.25*R.y) Q.zw = vec2(4.,1);
    }
    if (iMouse.z>0.&&length(U-iMouse.xy)<20.)
        Q = vec4(1,0,6,1);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)Q.xy*=0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Conservative advect y-direction
Main {
    Q = vec4(0);
    if (iFrame%2==0)
	for (float i = -1.; i <= 1.; i++)
    {
    	vec2 u = vec2(0,i);
        vec4 a = .5*A(U+u-vec2(0,0.5)),
             b = A(U+u),
             c = .5*A(U+u+vec2(0,0.5));
        float w1 = 1.+c.y-a.y;
        if (w1>0.) {
            float w = clamp(u.y+.5+c.y,-.5,.5)-
                      clamp(u.y-.5+a.y,-.5,.5);
            Q.xyz += b.w*w/w1*b.xyz;
            Q.w += b.w*w/w1;
        }
    } else 
	for (float i = -1.; i <= 1.; i++)
    {
    	vec2 u = vec2(i,0);
        vec4 a = .5*A(U+u-vec2(0.5,0)),
             b = A(U+u),
             c = .5*A(U+u+vec2(0.5,0));
        float w1 = 1.+c.x-a.x;
        if (w1 > 0.) {
            float w = clamp(u.x+.5+c.x,-.5,.5)-
                      clamp(u.x-.5+a.x,-.5,.5);
            Q.xyz += b.w*w/w1*b.xyz;
            Q.w += b.w*w/w1;
        }
    }
    if (Q.w > 0.) Q.xyz /= Q.w;
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)Q.xy*=0.;

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// density-dependent diffusion
vec4 X (vec4 Q, vec2 u) {
	vec4 a = A(gl_FragCoord.xy + u);
    float f = Q.w-a.w;
    return mix(Q,a,clamp(1.*f*f,0.,1.));
}
Main {
    Q = A(U);
    Q = (
        0.125*X(Q,vec2(1,0))+
        0.125*X(Q,vec2(0,1))+
        0.125*X(Q,vec2(-1,0))+
        0.125*X(Q,vec2(0,-1))+
        0.125*X(Q,vec2(1,1))+
        0.125*X(Q,vec2(1,-1))+
        0.125*X(Q,vec2(-1,-1))+
        0.125*X(Q,vec2(-1,1)));
    if (iMouse.z>0.&&length(U-iMouse.xy)<10.)
        Q = vec4(.5,0,6,1);
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)Q.xy*=0.;

}