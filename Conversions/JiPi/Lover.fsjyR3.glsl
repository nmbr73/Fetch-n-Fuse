

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main 
    vec4 b = B(U);
    Q = max(sin(.5+b.x/R.x+vec4(1,2,3,4)),0.);
    vec4 C = vec4(.8)+.3*grad(U).x;
    float q = 0.;
    vec2 v = 4.*(U-.5*R)/R.y+vec2(0,2);
    q += B(U+v).z;
    q += B(U+2.*v).z;
    q += B(U+3.*v).z;
    q += B(U+4.*v).z;
    q += B(U+5.*v).z;
    Q *= 1.+.1*dFdy(q);
    Q = mix(C,Q,min(q,1.));
    float w = 1e9;
    for (float i = 1.; i < 30.; i++) {
        float s = B(U+i*vec2(0,3)).z;
        if (s>0.) {
            w = i;
            break;
        }
    }
    Q -= exp(-.3*(w))*(1.-min(q,1.));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; int I;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U) { R = iResolution.xy; I = iFrame;
float G2 (float w, float s) {
    return 0.15915494309*exp(-.5*w*w/s/s)/(s*s);
}
float G1 (float w, float s) {
    return 0.3989422804*exp(-.5*w*w/s/s)/(s);
}
float heart (vec2 u) {
    u -= vec2(.5,.4)*R;
    u.y -= 10.*sqrt(abs(u.x));
    u.y *= 1.;
    u.x *= .8;
    if (length(u)<.35*R.y) return 1.;
    else return 0.;
}

float _12(vec2 U) {

    return clamp(floor(U.x)+floor(U.y)*R.x,0.,R.x*R.y);

}

vec2 _21(float i) {

    return clamp(vec2(mod(i,R.x),floor(i/R.x))+.5,vec2(0),R);

}

float sg (vec2 p, vec2 a, vec2 b) {
    float i = clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.);
	float l = (length(p-a-(b-a)*i));
    return l;
}

float hash (vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
float noise(vec2 p)
{
    vec4 w = vec4(
        floor(p),
        ceil (p)  );
    float 
        _00 = hash(w.xy),
        _01 = hash(w.xw),
        _10 = hash(w.zy),
        _11 = hash(w.zw),
    _0 = mix(_00,_01,fract(p.y)),
    _1 = mix(_10,_11,fract(p.y));
    return mix(_0,_1,fract(p.x));
}
float fbm (vec2 p) {
    float o = 0.;
    for (float i = 0.; i < 3.; i++) {
        o += noise(.1*p)/3.;
        o += .2*exp(-2.*abs(sin(.02*p.x+.01*p.y)))/3.;
        p *= 2.;
    }
    return o;
}
vec2 grad (vec2 p) {
    float 
    n = fbm(p+vec2(0,1)),
    e = fbm(p+vec2(1,0)),
    s = fbm(p-vec2(0,1)),
    w = fbm(p-vec2(1,0));
    return vec2(e-w,n-s);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main 
    float i = _12(U);
    Q = A(U);
    
    vec2 f = vec2(0);
    
    for (float j = -20.; j <= 20.; j++) 
    if (j!=0. && j+i>=0. && j+i<R.x*R.y) {
        vec4 a = A(_21(i+j));
        vec2 r = a.xy-Q.xy;
        float l = length(r);
        f += 50.*r/sqrt(l)*(l-abs(j))*(G1(j,10.)+2.*G1(j,5.));
    }
    for (float x = -2.; x <= 2.; x++)
    for (float y = -2.; y <= 2.; y++) {
        vec2 u = vec2(x,y);
        vec4 d = D(Q.xy+u);
        f -= 100.*d.w*u;
    }
    if (length(f)>.1) f = .1*normalize(f);
    Q.zw += f-.03*Q.zw;
    Q.xy += f+1.5*Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    vec4 m = .5*( A(_21(i-1.)) + A(_21(i+1.)) );
    Q.zw = mix(Q.zw,m.zw,0.1);
    Q.xy = mix(Q.xy,m.xy,0.01);
    if (Q.x>R.x)Q.y=.5*R.y,Q.z=-10.;
    if (Q.x<0.)Q.y=.5*R.y,Q.z=10.;
    
    if (iFrame < 1) {
        Q = vec4(0,.5*R.y,0,0);
        
        Q.x = i-.5*R.x*R.y;
    }

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void XY (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(_21(q.x)).xy)<length(U-A(_21(Q.x)).xy)) Q.x = q.x;
}
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(_21(q.y)).xy)<length(U-A(_21(Q.y)).xy)) Q.y = q.y;
}
Main
    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,Q,B(U+vec2(x,y)));
    }
    XY(U,Q,vec4(Q.x-3.));
    XY(U,Q,vec4(Q.x+3.));
    XY(U,Q,vec4(Q.x-7.));
    XY(U,Q,vec4(Q.x+7.));
    if (I%12==0) 
        Q.y = _12(U);
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    XY(U,Q,Q.yxzw);
    if (I<1) Q = vec4(_12(U));
    
    vec4 a1 = A(_21(Q.x));
    vec4 a2 = A(_21(Q.x+1.));
    vec4 a3 = A(_21(Q.x-1.));
    float l1 = sg(U,a1.xy,a2.xy);
    float l2 = sg(U,a1.xy,a3.xy);
    float l = min(l1,l2);
    Q.z = Q.w = smoothstep(2.,1.,l);
    Q.w -= .2*heart(U);
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main 
    Q = vec4(0);
    for (float x = -30.; x <= 30.; x++)
        Q += G1(x,10.)*B(U+vec2(x,0)).w;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main 
    Q = vec4(0);
    for (float y = -30.; y <= 30.; y++)
        Q += G1(y,10.)*C(U+vec2(0,y)).w;
        
    Q = mix(Q,D(U),.5);
}