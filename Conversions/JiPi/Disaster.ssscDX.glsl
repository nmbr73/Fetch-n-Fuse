

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(out vec4 Q, vec2 U){

    R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
    
    vec4 b = B(U);
    vec4 a = A(b.xy);
    
    Q = vec4(0)+step(length(U-a.xy),.75);

}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; vec4 M; float T; int I;
#define Main void mainImage(out vec4 Q, vec2 U){UNIS
#define UNIS R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
#define A(U) texture(iChannel0,(U)/R)
#define B(U) texture(iChannel1,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel3,(U)/R)


float building(vec2 U) {
    if (U.y<10.) return 0.;
    if (length(U-vec2(.9)*R)<.02*R.x) return 1.;
    float r = U.x/R.x*30.-15.;
    float x = round(U.x/R.x*30.)-15.;
    
    float y = exp(-x*x/40.)*(1.5+sin(10.*x));
    if (abs(r-x)>.3+.5*sin(x)) return 0.;
    if (U.y/R.y < .4*y) return 1.;
    else return 0.;

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(out vec4 Q, vec2 U){

    R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
    
    if (building(U)==0.) {Q = vec4(0); return;}

    Q = A(U);

    vec2 f = vec2(0,-4e-4);
    
    for (float x = -5.; x <= 5.; x ++) 
    for (float y = -5.; y <= 5.; y ++) {
        vec4 b = B(Q.xy+vec2(x,y));
        vec4 a = A(b.xy);
        vec2 u = abs(Q.xy+vec2(x,y)-a.xy);
        if (u.x>.5||u.y>.5)continue;
        vec2 r = a.xy-Q.xy;
        float l = length(r);
        if (l<1.||l>6.) continue;
        float L = length(U-b.xy);
        if ((l-L)<.1*L||L>5.) 
            f += 3e-1*r*(l-L)/l/L/l;
    }
    
    if (length(f)>1.) f = normalize(f);
    
    Q.zw += f;
    Q.xy += .5*f+Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<30.) Q.zw *= .8;
    
    if (Q.y<10.) Q.y=10., Q.zw *= 0.;

    if (M.z>0.) Q.zw += 3e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1) {
    
        Q = vec4(U,0,0);
         if (length(U-vec2(.9)*R)<.02*R.x) Q.zw = vec2(-2.5,-1.5);
    }
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void XY (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.xy).xy)<length(U-A(Q.xy).xy)) Q.xy = q.xy;
}
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.zw).xy)<length(U-A(Q.zw).xy)) Q.zw = q.zw;
}

void mainImage(out vec4 Q, vec2 U){

    R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;

    Q = B(U);
    for (int x=-1;x<=1;x++)
    for (int y=-1;y<=1;y++) {
        XY(U,Q,B(U+vec2(x,y)));
        XY(U,Q,vec4(Q.xy+vec2(x,y),0,0));
    }
    
    if (I%12==0) 
        Q.zw = U;
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    XY(U,Q,Q.zwxy);
    
    if (I<1) Q = vec4(U,U);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage(out vec4 Q, vec2 U){

    R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
    
    if (building(U)==0.) {Q = vec4(0); return;}

    Q = A(U);

    vec2 f = vec2(0,-4e-4);
    
    for (float x = -5.; x <= 5.; x ++) 
    for (float y = -5.; y <= 5.; y ++) {
        vec4 b = B(Q.xy+vec2(x,y));
        vec4 a = A(b.xy);
        vec2 u = abs(Q.xy+vec2(x,y)-a.xy);
        if (u.x>.5||u.y>.5)continue;
        vec2 r = a.xy-Q.xy;
        float l = length(r);
        if (l<1.||l>6.) continue;
        float L = length(U-b.xy);
        if ((l-L)<.1*L||L>5.) 
            f += 3e-1*r*(l-L)/l/L/l;
    }
    
    if (length(f)>1.) f = normalize(f);
    
    Q.zw += f;
    Q.xy += .5*f+Q.zw*inversesqrt(1.+dot(Q.zw,Q.zw));
    
    if (Q.y<30.) Q.zw *= .8;
    
    if (Q.y<10.) Q.y=10., Q.zw *= 0.;

    if (M.z>0.) Q.zw += 3e-2*(M.xy-Q.xy)/(1.+length((M.xy-Q.xy)));

    if(I<1) {
    
        Q = vec4(U,0,0);
         if (length(U-vec2(.9)*R)<.02*R.x) Q.zw = vec2(-2.5,-1.5);
    }
    
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void XY (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.xy).xy)<length(U-A(Q.xy).xy)) Q.xy = q.xy;
}
void ZW (vec2 U, inout vec4 Q, vec4 q) {
    if (length(U-A(q.zw).xy)<length(U-A(Q.zw).xy)) Q.zw = q.zw;
}


void mainImage(out vec4 Q, vec2 U){

    R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
    Q = B(U);
    for (int x=-2;x<=2;x++)
    for (int y=-2;y<=2;y++) {
        XY(U,Q,B(U+vec2(x,y)));
        XY(U,Q,vec4(Q.xy+vec2(x,y),0,0));
    }
    
    if (I%12==0) 
        Q.zw = U;
    else
    {
        float k = exp2(float(11-(I%12)));
        ZW(U,Q,B(U+vec2(0,k)));
        ZW(U,Q,B(U+vec2(k,0)));
        ZW(U,Q,B(U-vec2(0,k)));
        ZW(U,Q,B(U-vec2(k,0)));
    }
    XY(U,Q,Q.zwxy);
    
    if (I<1) Q = vec4(U,U);
}