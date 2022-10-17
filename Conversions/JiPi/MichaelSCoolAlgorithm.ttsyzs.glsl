

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
Main {
    R = iResolution.xy;
	Q = texture(iChannel1,U/R);
    Q.x = .4*log(1.+Q.x);
    Q = Q.x*(0.6+0.4*cos(1.2*Q.w*vec4(1,2,3,4)));
    
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
#define Main void mainImage (out vec4 Q, in vec2 U)
float signe (float x) {return atan(100.*x);}
void prog (vec2 U, out vec4 a, out vec4 b, sampler2D cha, sampler2D chb) {
	
    a = vec4(0); b = vec4(0);
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 u = vec2(x,y);
    	vec4 aa = A(U+u), bb = B(U+u);
        aa.xy += aa.zw;
        #define q clamp(bb.x,1.,1.1)
		vec4 w = clamp(vec4(aa.xy-0.5*q,aa.xy+0.5*q),U.xyxy - 0.5,U.xyxy + 0.5);
        float m = (w.w-w.y)*(w.z-w.x)/(q*q);
        aa.xy = 0.5*(w.xy+w.zw);
        a += aa*bb.x*m;
        b.x += bb.x*m;
        b.yzw += bb.yzw*bb.x*m;
    }
    if (b.x>0.) {
        a/=b.x;
        b.yzw/=b.x;
    }
}
void prog2 (vec2 U, out vec4 a, out vec4 b, sampler2D cha, sampler2D chb) {
	
    a = A(U); b = B(U);
    vec2 f = vec2(0); float m = 0.;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    
    if (abs(x)!=abs(y))
    {
        vec2 u = vec2(x,y);
    	vec4 aa = A(U+u), bb = B(U+u);
        f += .01*(bb.x*(.05-.01*bb.x))*u;
        m += bb.x;
    }
    if (m>0.) a.zw += f;
    
    
    // Boundaries:
   	a.w -= .1/R.y*signe(b.x);
    if (a.x<10.) {a.z -= -.1;b.z*=0.9;}if (R.x-a.x<10.) {a.z -= .1;b.z*=0.9;}if (a.y<10.) {a.w -= -.05;b.z*=0.9;}if (R.y-a.y<10.) {a.w -= .1;b.z*=0.9;}
    if (I<1||U.x<1.||R.x-U.x<1.||R.y-U.y<1.||R.x-U.x<1.) {
    	a = vec4(U,0,0);
        b = vec4(0);
        if (length(U-0.5*R) < 0.4*R.y) b.x = 15.;
        if (U.x<0.5*R.x) b.w = 1.;
    }
    if (M.z>0. && length(U-M.xy) < 20.) {
        b.x = 2.;
        a.xy = U;
        a.zw = .6*vec2(cos(.1*T),sin(.1*T));
        b.w = 2.;
    }
}