

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    Q = C(U);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define Main void mainImage(out vec4 Q, in vec2 U)
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void X (inout vec4 Q, vec2 U, vec4 q) {
    if (length(U-q.xy)<length(U-Q.xy))Q=q;
}

Main {
    Q = A(U);
    X(Q,U,A(U+vec2(0,1)));
    X(Q,U,A(U+vec2(1,0)));
    X(Q,U,A(U-vec2(0,1)));
    X(Q,U,A(U-vec2(1,0)));
    vec2 v = mix(Q.xy,U,.000001);
    vec4 n = B(v+vec2(0,1));
    vec4 e = B(v+vec2(1,0));
    vec4 s = B(v-vec2(0,1));
    vec4 w = B(v-vec2(1,0));
    Q.xy -= .5*vec2(e.w-w.w,n.w-s.w);
    if (iFrame < 1) {
        U = ceil(U/200.)*200.;
        Q = vec4(U,U);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
    vec4 a = A(U);
    vec4 b = B(U);
    vec4 m = vec4(0);
    for (float x = -2.; x <= 2.; x++)
    for (float y = -2.; y <= 2.; y++)
    {
        m += 1./25.*B(U+vec2(x,y));
    }
    Q.x = smoothstep(2.,1.,length(U-a.xy));
    Q.y = max(Q.x,.93*b.y);
    Q.z = 0.;
    Q.w = m.w*.99+Q.y;
    
    if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.)Q *= 0.;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    vec4 b = B(U);
    vec4 a = A(U);
    vec4 c = C(U);
    Q = b.x*(.5+.5*sin(a.z+a.w*vec4(1,2,3,4)));
    Q = max(c*.99,Q);
}