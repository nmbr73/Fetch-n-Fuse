

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    Q = B(U);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define B(U) texelFetch(iChannel1,ivec2(U),0)
#define C(U) texelFetch(iChannel2,ivec2(U),0)
#define D(U) texelFetch(iChannel3,ivec2(U),0)
#define Main void mainImage(out vec4 Q, in vec2 U)
bool cell (vec2 u) {
    return u.x>=0.&&u.y>=0.&&u.x<1.&&u.y<1.;
}
#define pack(u) (floor(u.x)+R.x*floor(u.y))
#define unpack(i) vec2(mod(floor(i),R.x),floor(floor(i)/R.x))

#define touch(u) ((length(u)-2.)/(length(u)+2.))
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {

    Q = A(U);
    vec4 q = B(U);
    vec4 c = C(Q.xy);
    float n = 0.;
    vec2 f = vec2(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
        vec4 c = C(Q.xy+u);
        for (int k = 0; k < 4; k++){
            float index = c[k];
            if (index > 0.) {
                vec4 a = A(unpack(index));
                vec2 u = a.xy-Q.xy;
                if (length(u)>0.) {
                    float m = touch(u);
                    u = normalize(u);
                    f += .25*m*u;
                    n ++;
                }
            }
        }
    }
    Q.w -= .3/R.y*sign(U.y-0.5*R.y);
    Q.z -= .3/R.x*sign(U.x-0.5*R.x);
    if (n>0.) Q.zw += .5*f;
    Q.xy += Q.zw+f;
    if (length(Q.zw)>1.) Q.zw = normalize(Q.zw);
    
    if (Q.x < 2.) {Q.x = 2.; Q.z*=-1.;}
    if (R.x-Q.x < 2.) {Q.x = R.x-2.; Q.z*=-1.;}
    if (Q.y < 2.) {Q.y = 2.; Q.w*=-1.;}
    if (R.y-Q.y < 2.) {Q.y = R.y-2.; Q.w*=-1.;}
    
    
    if (iFrame < 1) {
        Q = vec4(U,0,0);
    }
    
    if (iMouse.z>0.&&length(Q.xy-iMouse.xy)<20.)
        Q.z = 1.;
    
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {

    vec4 c = C(U);
    vec2 v = unpack(c.x);
    Q = (c.x>0.?1.:0.)*(.5+.5*sin(2.+2.*sign(v.x-.5*R.x)+2.*sign(v.y-.5*R.y)*vec4(1,2,3,4)));
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
    Q = vec4(0);
    int i = 0;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
        vec4 c = C(U+u);
        for (int k = 0; k < 4; k++){
            float index = c[k];
            if (index>0.&&i<4) {
                vec4 a = A(unpack(index));
                if (cell(a.xy-floor(U)))
                    Q[i++] = index;
            }
        }
    }
    if (i==0) {
        vec4 d = D(U);
        vec4 a = A(d.xy);
        if (cell(a.xy-floor(U)))
            Q.x = pack(d.xy);
    }
    if (iFrame < 1) Q = vec4(pack(U),0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define N 12
void X (inout vec4 Q, inout float r, vec2 U, vec4 b) {
	
    vec4 a = A(b.xy);
    
    if (length(a.xy-U)<r) {
    	r = length(a.xy-U);
        Q.xy = b.xy;
    }

}

void Z (inout vec4 Q, inout float r, vec2 U, vec4 b) {
	
    vec4 a = A(b.zw);
    
    if (length(a.xy-U)<r) {
    	r = length(a.xy-U);
        Q.zw = b.zw;
    }

}

Main {
    
    Q = B(U);
    float r = length(A(Q.xy).xy-U);
    X(Q,r,U,B(U+vec2(0,1)));
    X(Q,r,U,B(U+vec2(1,0)));
    X(Q,r,U,B(U-vec2(0,1)));
    X(Q,r,U,B(U-vec2(1,0)));
    X(Q,r,U,B(U+vec2(0,2)));
    X(Q,r,U,B(U+vec2(2,0)));
    X(Q,r,U,B(U-vec2(0,2)));
    X(Q,r,U,B(U-vec2(2,0)));
    X(Q,r,U,Q.zwzw);
    r = length(A(Q.zw).xy-U);
    if (iFrame%N==0) Q.zw = U;
    else {
        float k = exp2(float(N-1-(iFrame%N)));
    	Z(Q,r,U,B(U+vec2(0,k)));
    	Z(Q,r,U,B(U+vec2(k,0)));
    	Z(Q,r,U,B(U-vec2(0,k)));
    	Z(Q,r,U,B(U-vec2(k,0)));
    }
    
	
    if (iFrame < 1) Q = vec4(U,U);
    
}