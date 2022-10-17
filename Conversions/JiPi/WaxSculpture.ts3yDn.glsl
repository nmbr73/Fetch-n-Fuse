

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Tannins" by wyatt. https://shadertoy.com/view/3sdczN
// 2020-09-21 22:04:57

Main {
    vec4
        n = C(U+vec2(0,1))+B(U+vec2(0,1)),
        e = C(U+vec2(1,0))+B(U+vec2(1,0)),
        s = C(U-vec2(0,1))+B(U-vec2(0,1)),
        w = C(U-vec2(1,0))+B(U-vec2(1,0));
    vec3 norm = 
        normalize(vec3(e.w-w.w,n.w-s.w,.1)),
        ref = reflect(vec3(0,0,-1),norm);
   	vec4 b = B(U), c = C(U);
	Q = c+b*b.w;
    vec3 l = R.xyy;
    float li = ln(vec3(U,0),vec3(U,0)+ref,l);
    
    Q *= .2+0.5*exp(-li)+.5*exp(-2.*li);
    
    Q = atan(2.*Q);
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

#define rate 0.005
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
    if (iFrame%2==0) {
        Q = vec4(0);
        for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            vec2 u = vec2(x,y);
            vec4 a = A(U+u);
            #define o 1.3
            vec2 w1 = clamp(U+u+a.xy-0.5*o,U - 0.5,U + 0.5),
                 w2 = clamp(U+u+a.xy+0.5*o,U - 0.5,U + 0.5);
            float m = (w2.x-w1.x)*(w2.y-w1.y)/(o*o);
            Q.xyz += m*a.w*a.xyz;
            Q.w += m*a.w;
        }
        if (Q.w>0.)
            Q.xyz/=Q.w;
        if (iFrame < 1) 
        {
            Q = vec4(0,0,.1,0);
        }
   } else {
    	Q = A(U);vec4 q = Q;
        for (int x = -1; x<=1; x++)
        for (int y = -1; y<=1; y++)
        if (x != 0||y!=0)
        {
            vec2 u = vec2(x,y);
            vec4 a = A(U+u), c = C(U+u);
            u = (u)/dot(u,u);
            Q.xy -= q.w*0.125*.5*(c.w+a.w*(a.w*a.z-2.5))*u;
            Q.z -= q.w*0.125*a.w*(dot(u,a.xy-q.xy));
        }
    }
    Q.xy *= .2+.77*min(pow(Q.w,.01),1.);
    // Solidify
    Q.w -= Q.w*rate/(1.+10.*length(Q.xy));
    vec4 d = D(U);
        if ((iMouse.z>0.)&&ln(U,d.xy,d.zw)<8.)
            Q = vec4(1e-1*clamp(d.xy-d.zw,-.8,.8),1,1);
        if (U.y<1.||U.x<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xy *= 0.;
    
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
	vec4 a = A(U), b = B(U);
    Q = C(U);
    float f = 1./(1.+10.*length(Q.xy));
    Q.w += 6.*rate*a.w*f;
    Q.xyz = mix(Q.xyz,5.*b.xyz,rate*b.w*f);
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
    if ((iMouse.z>0.)&&ln(U,d.xy,d.zw)<8.)
        Q = vec4(1.+0.5*sin(iTime*.1+iTime*vec3(1,2,3)),1);

    Q -= Q*rate;
    if (iFrame < 1) Q = C(U);
}