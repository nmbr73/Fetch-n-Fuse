

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Rigid Body Test 123" by wyatt. https://shadertoy.com/view/fsscRr
// 2022-01-10 00:45:24

void mainImage(out vec4 Q, in vec2 U) {

vec2 R=iResolution.xy; float T=iTime; int I=iFrame; vec4 M=iMouse;
//if (iFrame % 8 > 0) discard;
Q = sin(5.3+U.y/R.y+(1.+U.x/R.x)*vec4(1,2,3,4));
Q *= .5*(exp(-30.*U.x/R.x)+exp(-30.*(R.x-U.x)/R.x)+exp(-30.*U.y/R.y)+exp(-30.*(R.y-U.y)/R.y));
Q = 1.-Q;
vec2 m = vec2(0);
vec2 p = U;
for (float i = 0.; i < N; i++) {
    vec4 a = A(vec2(i,0)+.5);
    vec4 at= A(vec2(i,1)+.5);
    vec2 aw = size(i)/6.;
    float d = map(p,a,at,aw);
    if (abs(d)<2.) Q = vec4(0);
    else if (dot(p-a.xy,vec2(0,1)*ei(at.x))>0.&&
        d<0.);
    else if (d<0.) Q = vec4(1,.2,.5,0);
    else {
        m += mag(p,a,at);
    }
}
Q *= 1.-.8*(Q.y)*length(m)*(.5+.5*sin(atan(m.x,m.y)+vec4(1,2,3,4)));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R; float T; int I; vec4 M;
#define A(U) texture(iChannel0,(U)/R)
#define Main void mainImage(out vec4 Q, in vec2 U) {R=iResolution.xy;T=iTime;I=iFrame;M=iMouse;
#define ei(a) mat2(cos(a),-sin(a),sin(a),cos(a))

#define N min(5.+float(I/10),100.)


vec2 size (float i) {
    vec2 s = vec2(1,2.+3.5*i/100.)*.012*R.x;
    return s;
}
vec2 mag (vec2 p, vec4 a, vec4 t) {
    //https://en.wikipedia.org/wiki/Magnetic_moment#Magnetism
    vec2 r = (p.xy-a.xy)*ei(-t.x);
    vec2 m = vec2(0,1);
    float l = length(r);
    if (l >0.) return 1e2*(3.*r/l*dot(r/l,m)-m)/(l*l);
    else return vec2(0);
}
#define dt .3
vec2 vel (vec2 p, vec4 a, vec4 t) {
    vec2 w = p.xy-a.xy;
    if (length(w)>0.) w = vec2(-w.y,w.x);
    return a.zw + w*t.y;
}
vec2 MF (vec2 p, vec4 a, vec4 t, vec4 b, vec4 bt) {
    vec2 r = p.xy-a.xy;
    vec2 m1 = vec2(0,300./sqrt(1.+N))*ei(t.x);
    vec2 m2 = vec2(0,300./sqrt(1.+N))*ei(bt.x);
    float l = length(r);
    if (l>0.) {
          vec2 rh = r/l;
          return (m2*dot(m1,rh)+m1*dot(m2,rh)+rh*dot(m1,m2)-5.*rh*dot(m1,rh)*dot(m2,rh)) / ( l*l*l*l );
    } else return vec2(0);
}
float map ( vec2 p, vec4 a, vec4 t, vec2 b )
{ //iquilezles.org/www/articles/distfunctions/distfunctions.html
  p = (p-a.xy)*ei(-t.x);
  vec2 q = (abs(p) - b);
  return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

vec2 norm (vec2 p, vec4 a, vec4 t, vec2 b)
{   
    vec2 e = vec2(1,0);
    return 1./dt*normalize(vec2(
        map(p+e.xy,a,t,b)-map(p-e.xy,a,t,b),
        map(p+e.yx,a,t,b)-map(p-e.yx,a,t,b)
    ));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U) {

    vec2 R=iResolution.xy; float T=iTime; int I=iFrame; vec4 M=iMouse;

    if (U.y>4.)discard;
    Q = A(vec2(U.x,.5));
    vec4 t = A(vec2(U.x,1.5));
    vec4 oQ = A(vec2(U.x,2.5));
    vec4 ot = A(vec2(U.x,3.5));
    vec2 w = size(floor(U.x));
    
    if (U.y<2.) {
        vec2 f = vec2(0);
        float tor = 0.;
        float h = 0.;
        for(float j = 0.; j < N; j++) if (j!=floor(U.x)) {
            vec4 b = A(vec2(j,0)+.5);
            vec4 bt = A(vec2(j,1)+.5);
            vec2 bw = size(j);
            for (float x = 0.; x<=1.; x++)
            for (float y = 0.; y<=1.; y++)
            {
                { 
                    vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
                    // magnetic force
                    {
                        vec2 r = q-b.xy;
                        vec2 im = MF(q,Q,t,b,bt);
                        if (length(r)>0.) {
                            tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                            f += im*abs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,.5*bw);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
                { // their point in my box
                    vec2 q = bw*(vec2(x,y)-.5)*ei(bt.x)+b.xy;
                    float m = map(q,Q,t,.5*w);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
            }
        }
        for (float x = 0.; x<=1.; x++)
        for (float y = 0.; y<=1.; y++)
        {
            vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
            vec4 b = vec4(.5*R.xy,0,0);
            float m = -map(q,b,vec4(0),b.xy);
            if (m<0.) {
                vec2 r = q-Q.xy;
                vec2 v = vel(q,b,vec4(0))-vel(q,Q,t);
                float l = length(r);
                vec2 im = -norm(q,b,vec4(0),.5*R.xy)+v;
                tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                f += im*abs(dot(normalize(r),normalize(im)));
                h += 1.;
            }
        }
        if (h>0.) f/=h, tor/=h;
        Q.zw += dt*(f);
        Q.xy += dt*(.5*f+Q.zw);
        t.y += dt*(tor);
        t.x += dt*(.5*tor+t.y);
        if (M.z>0.) Q.zw += 1e-3*(Q.xy-M.xy)*exp(-.001*length(Q.xy-M.xy));
        if (length(Q.zw)>1./dt) Q.zw = 1./dt*normalize(Q.zw);
        if (abs(t.y)>dt) t.y = dt*sign(t.y);
    }
    
    if(iFrame < 1) {
        vec2 size = size(floor(U.x));
        Q = vec4(.5*R+.1*R.y*(1.+6.*U.x/N)*vec2(cos(36.*U.x/N),sin(36.*U.x/N)),0,0);
        t = vec4(10.*U.x/N,0,0,0);
    }
    if (mod(U.y,2.)>1.) Q = t;

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U) {

    vec2 R=iResolution.xy; float T=iTime; int I=iFrame; vec4 M=iMouse;
    if (U.y>4.)discard;
    Q = A(vec2(U.x,.5));
    vec4 t = A(vec2(U.x,1.5));
    vec4 oQ = A(vec2(U.x,2.5));
    vec4 ot = A(vec2(U.x,3.5));
    vec2 w = size(floor(U.x));
    
    if (U.y<2.) {
        vec2 f = vec2(0);
        float tor = 0.;
        float h = 0.;
        for(float j = 0.; j < N; j++) if (j!=floor(U.x)) {
            vec4 b = A(vec2(j,0)+.5);
            vec4 bt = A(vec2(j,1)+.5);
            vec2 bw = size(j);
            for (float x = 0.; x<=1.; x++)
            for (float y = 0.; y<=1.; y++)
            {
                { 
                    vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
                    // magnetic force
                    {
                        vec2 r = q-b.xy;
                        vec2 im = MF(q,Q,t,b,bt);
                        if (length(r)>0.) {
                            tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                            f += im*abs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,.5*bw);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
                { // their point in my box
                    vec2 q = bw*(vec2(x,y)-.5)*ei(bt.x)+b.xy;
                    float m = map(q,Q,t,.5*w);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
            }
        }
        for (float x = 0.; x<=1.; x++)
        for (float y = 0.; y<=1.; y++)
        {
            vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
            vec4 b = vec4(.5*R.xy,0,0);
            float m = -map(q,b,vec4(0),b.xy);
            if (m<0.) {
                vec2 r = q-Q.xy;
                vec2 v = vel(q,b,vec4(0))-vel(q,Q,t);
                float l = length(r);
                vec2 im = -norm(q,b,vec4(0),.5*R.xy)+v;
                tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                f += im*abs(dot(normalize(r),normalize(im)));
                h += 1.;
            }
        }
        if (h>0.) f/=h, tor/=h;
        Q.zw += dt*(f);
        Q.xy += dt*(.5*f+Q.zw);
        t.y += dt*(tor);
        t.x += dt*(.5*tor+t.y);
        if (M.z>0.) Q.zw += 1e-3*(Q.xy-M.xy)*exp(-.001*length(Q.xy-M.xy));
        if (length(Q.zw)>1./dt) Q.zw = 1./dt*normalize(Q.zw);
        if (abs(t.y)>dt) t.y = dt*sign(t.y);
    }
    
    if(iFrame < 1) {
        vec2 size = size(floor(U.x));
        Q = vec4(.5*R+.1*R.y*(1.+6.*U.x/N)*vec2(cos(36.*U.x/N),sin(36.*U.x/N)),0,0);
        t = vec4(10.*U.x/N,0,0,0);
    }
    if (mod(U.y,2.)>1.) Q = t;

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U) {

    vec2 R=iResolution.xy; float T=iTime; int I=iFrame; vec4 M=iMouse;
    if (U.y>4.)discard;
    Q = A(vec2(U.x,.5));
    vec4 t = A(vec2(U.x,1.5));
    vec4 oQ = A(vec2(U.x,2.5));
    vec4 ot = A(vec2(U.x,3.5));
    vec2 w = size(floor(U.x));
    
    if (U.y<2.) {
        vec2 f = vec2(0);
        float tor = 0.;
        float h = 0.;
        for(float j = 0.; j < N; j++) if (j!=floor(U.x)) {
            vec4 b = A(vec2(j,0)+.5);
            vec4 bt = A(vec2(j,1)+.5);
            vec2 bw = size(j);
            for (float x = 0.; x<=1.; x++)
            for (float y = 0.; y<=1.; y++)
            {
                { 
                    vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
                    // magnetic force
                    {
                        vec2 r = q-b.xy;
                        vec2 im = MF(q,Q,t,b,bt);
                        if (length(r)>0.) {
                            tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                            f += im*abs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,.5*bw);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
                { // their point in my box
                    vec2 q = bw*(vec2(x,y)-.5)*ei(bt.x)+b.xy;
                    float m = map(q,Q,t,.5*w);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
            }
        }
        for (float x = 0.; x<=1.; x++)
        for (float y = 0.; y<=1.; y++)
        {
            vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
            vec4 b = vec4(.5*R.xy,0,0);
            float m = -map(q,b,vec4(0),b.xy);
            if (m<0.) {
                vec2 r = q-Q.xy;
                vec2 v = vel(q,b,vec4(0))-vel(q,Q,t);
                float l = length(r);
                vec2 im = -norm(q,b,vec4(0),.5*R.xy)+v;
                tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                f += im*abs(dot(normalize(r),normalize(im)));
                h += 1.;
            }
        }
        if (h>0.) f/=h, tor/=h;
        Q.zw += dt*(f);
        Q.xy += dt*(.5*f+Q.zw);
        t.y += dt*(tor);
        t.x += dt*(.5*tor+t.y);
        if (M.z>0.) Q.zw += 1e-3*(Q.xy-M.xy)*exp(-.001*length(Q.xy-M.xy));
        if (length(Q.zw)>1./dt) Q.zw = 1./dt*normalize(Q.zw);
        if (abs(t.y)>dt) t.y = dt*sign(t.y);
    }
    
    if(iFrame < 1) {
        vec2 size = size(floor(U.x));
        Q = vec4(.5*R+.1*R.y*(1.+6.*U.x/N)*vec2(cos(36.*U.x/N),sin(36.*U.x/N)),0,0);
        t = vec4(10.*U.x/N,0,0,0);
    }
    if (mod(U.y,2.)>1.) Q = t;

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage(out vec4 Q, in vec2 U) {

    vec2 R=iResolution.xy; float T=iTime; int I=iFrame; vec4 M=iMouse;
    if (U.y>4.)discard;
    Q = A(vec2(U.x,.5));
    vec4 t = A(vec2(U.x,1.5));
    vec4 oQ = A(vec2(U.x,2.5));
    vec4 ot = A(vec2(U.x,3.5));
    vec2 w = size(floor(U.x));
    
    if (U.y<2.) {
        vec2 f = vec2(0);
        float tor = 0.;
        float h = 0.;
        for(float j = 0.; j < N; j++) if (j!=floor(U.x)) {
            vec4 b = A(vec2(j,0)+.5);
            vec4 bt = A(vec2(j,1)+.5);
            vec2 bw = size(j);
            for (float x = 0.; x<=1.; x++)
            for (float y = 0.; y<=1.; y++)
            {
                { 
                    vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
                    // magnetic force
                    {
                        vec2 r = q-b.xy;
                        vec2 im = MF(q,Q,t,b,bt);
                        if (length(r)>0.) {
                            tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                            f += im*abs(dot(normalize(r),normalize(im)));
                        }
                    }
                    // my point in their box
                    float m = map(q,b,bt,.5*bw);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
                { // their point in my box
                    vec2 q = bw*(vec2(x,y)-.5)*ei(bt.x)+b.xy;
                    float m = map(q,Q,t,.5*w);
                    if (m<0.) {
                        vec2 r = q-Q.xy;
                        vec2 v = vel(q,b,bt)-vel(q,Q,t);
                        float l = length(r);
                        vec2 im = norm(q,b,bt,.5*bw)+v;
                        tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                        f += im*abs(dot(normalize(r),normalize(im)));
                        h += 1.;
                    }
                }
            }
        }
        for (float x = 0.; x<=1.; x++)
        for (float y = 0.; y<=1.; y++)
        {
            vec2 q = w*(vec2(x,y)-.5)*ei(t.x)+Q.xy;
            vec4 b = vec4(.5*R.xy,0,0);
            float m = -map(q,b,vec4(0),b.xy);
            if (m<0.) {
                vec2 r = q-Q.xy;
                vec2 v = vel(q,b,vec4(0))-vel(q,Q,t);
                float l = length(r);
                vec2 im = -norm(q,b,vec4(0),.5*R.xy)+v;
                tor -= dot(r,vec2(-im.y,im.x))/(w.x*w.y);
                f += im*abs(dot(normalize(r),normalize(im)));
                h += 1.;
            }
        }
        if (h>0.) f/=h, tor/=h;
        Q.zw += dt*(f);
        Q.xy += dt*(.5*f+Q.zw);
        t.y += dt*(tor);
        t.x += dt*(.5*tor+t.y);
        if (M.z>0.) Q.zw += 1e-3*(Q.xy-M.xy)*exp(-.001*length(Q.xy-M.xy));
        if (length(Q.zw)>1./dt) Q.zw = 1./dt*normalize(Q.zw);
        if (abs(t.y)>dt) t.y = dt*sign(t.y);
    }
    
    if(iFrame < 1) {
        vec2 size = size(floor(U.x));
        Q = vec4(.5*R+.1*R.y*(1.+6.*U.x/N)*vec2(cos(36.*U.x/N),sin(36.*U.x/N)),0,0);
        t = vec4(10.*U.x/N,0,0,0);
    }
    if (mod(U.y,2.)>1.) Q = t;

}