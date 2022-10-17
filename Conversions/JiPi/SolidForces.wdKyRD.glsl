

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Mykhailo/Michael's tutorial : 

//    https://t.co/G2aPHqVEo7?amp=1

// Solid Forces : 
// https://www.shadertoy.com/view/WtK3zK

// Delete line 48 in common to see the difference

Main {
    R = iResolution.xy;
	Q = texture(iChannel1,U/R);
    vec4 
        n = texture(iChannel1,(U+vec2(0,1))/R),
        e = texture(iChannel1,(U+vec2(1,0))/R),
        s = texture(iChannel1,(U-vec2(0,1))/R),
        w = texture(iChannel1,(U-vec2(1,0))/R);
    vec3 no = normalize(vec3(e.x-w.x,-n.x+s.x,.1));
    Q.x = atan(.8*log(1.+Q.x));
    Q = Q.x*(.8+0.6*abs(cos(.1+2.*Q.w+(1.+Q.y+5.*Q.z)*vec4(1,2,3,4))));
    Q *= 0.9+0.5*texture(iChannel2,no);
    Q = .9-1.2*Q;
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
Main {
	R = iResolution.xy;
    M = iMouse;
    I = iFrame;
    T = iTime;
   	vec4 a, b;
    
   	prog (U,a,b,iChannel0,iChannel1);
    
    Q = a;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
Main {
	R = iResolution.xy;
    M = iMouse;
    I = iFrame;
    T = iTime;
   	vec4 a, b;
    
   	prog (U,a,b,iChannel0,iChannel1);
    
    Q = b;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
Main {
	R = iResolution.xy;
    M = iMouse;
    I = iFrame;
    T = iTime;
   	vec4 a, b;
    
   	prog2 (U,a,b,iChannel0,iChannel1);
    
    Q = a;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
Main {
	R = iResolution.xy;
    M = iMouse;
    I = iFrame;
    T = iTime;
   	vec4 a, b;
    
   	prog2 (U,a,b,iChannel0,iChannel1);
    
    Q = b;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 R;
vec4 M;
float T;
int I;
#define A(U) texture(cha,(U)/R)
#define B(U) texture(chb,(U)/R)
#define C(U) texture(iChannel2,(U)/R)
#define D(U) texture(iChannel2,(U)/R)
#define Main void mainImage (out vec4 Q, in vec2 U)
float signe (float x) {return atan(100.*x);}
void prog (vec2 U, out vec4 a, out vec4 b, sampler2D cha, sampler2D chb) {
	
    a = vec4(0); b = vec4(0);
    float n = 0.;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 aa = A(U+u), bb = B(U+u);
        #define q 1.075
		vec4 w = clamp(vec4(aa.xy-0.5*q,aa.xy+0.5*q),U.xyxy - 0.5,U.xyxy + 0.5);
        float m = (w.w-w.y)*(w.z-w.x)/(q*q);
        aa.xy = 0.5*(w.xy+w.zw);
        a += aa*bb.x*m;
        b.x += bb.x*m;
        b.yzw += bb.yzw*bb.x*m;
        n += bb.x;
    }
    if (b.x>0.) {
        a/=b.x;
        b.yzw/=b.x;
        //b.yzw = B(a.xy-a.zw).yzw;
        //a.zw = mix(A(a.xy-a.zw).zw,a.zw,clamp(2.*n,0.,1.));
    }
}
void prog2 (vec2 U, out vec4 a, out vec4 b, sampler2D cha, sampler2D chb) {
	
    a = A(U); b = B(U);
    vec2 f = vec2(0); float m = 0., p = 0., z = 0.;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 aa = A(U+u), bb = B(U+u);
        float l = dot(u,u);
        if (l>0.) {
            f += .1*0.125*b.x*(bb.x*(.2+bb.x*bb.y)-b.x*(.2+b.x*b.y))*u/l;
            f += 0.125*b.x*(bb.z*bb.x-b.z*b.x)*vec2(-u.y,u.x)/l;
            p += b.x*bb.x*dot(u/l,aa.zw-a.zw);
            z += b.x*bb.x*(dot(vec2(-u.y,u.x)/l,aa.zw-a.zw));
            m += bb.x;
        }
    }
    if (m>0.) {
       a.zw += f/m;
       a.xy += f/m;
       a.xy += f/m;
       b.y += p/m;
       b.z += z/m;
    }
    a.xy += a.zw;
    
    
    // Boundaries:
   	a.w -= .003/R.y*signe(b.x);
    if (a.x<10.) {a.z -= -.1;b.y*=0.9;}if (R.x-a.x<10.) {a.z -= .1;b.y*=0.9;}if (a.y<10.) {a.w -= -.1;b.y*=0.9;}if (R.y-a.y<10.) {a.w -= .1;b.y*=0.9;}
    if (I<1||U.x<1.||R.x-U.x<1.||R.y-U.y<1.||R.x-U.x<1.) {
    	a = vec4(U,0,0);
        b = vec4(0);
        if (length(U-0.5*R) < 0.3*R.y&&length(U-0.5*R)>0.) {b.y = 0.;b.x = 1.;a.zw = -.01*normalize(U-0.5*R);}
    	b.w = 0.;
    }
    if (M.z>0.) {
        float l = length(U-M.xy);
        if (l<8.) {
            b.x = 2.;
            a.xy = U;
            a.zw = .25*vec2(cos(.4*T),sin(.4*T));
            b.w = .4+.4*sin(.1*T);
        }
    }
}