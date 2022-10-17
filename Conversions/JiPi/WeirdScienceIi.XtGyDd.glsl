

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec4 t (vec2 v) {return texture(iChannel0,v/iResolution.xy);}
void mainImage( out vec4 C, in vec2 U )
{
    vec4 me = t(U);
    vec2 dw = vec2(
    	t(U+vec2(1,0)).w-t(U-vec2(1,0)).w,
    	t(U+vec2(0,1)).w-t(U-vec2(0,1)).w
    );
    vec3 n = normalize(vec3(dw,.05));
    C = sin(vec4(1.5,2.5,3,4)*me.w*2.);
    float l = dot(n,normalize(vec3(3,1,0)));
    C *= 0.7+0.4*(0.5-0.5*l)*texture(iChannel2, reflect(vec3(0,0,-1),n));
	C = (0.8+0.4*l)*sqrt(C)*sqrt(sqrt(C));
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define Y 6.5
#define S -0.7

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec2 st (float p, float n, float p1, float n1, vec2 v) {
	return (p*p1+n*n1-p*n1-n*p1)*normalize(v)/dot(v,v);
}
vec2 f (vec2 U) {
	vec4 me = T(U);
    vec2 sf = vec2(0);
    me.z+=1.;
    for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
       if (x==0&&y==0) continue;
       vec2 u = vec2(x,y)*Y;
       vec4 o = T(U+u);
       o.z+=1.;
       sf += st (me.w*me.z,(1.-me.w)*me.z,o.w*o.z,(1.-o.w)*o.z,u);
    }
    }
    return sf;
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u = T(U), a = T(A), b = T(B), c = T(C), d = T(D);
 	vec4 p;
 	vec2 g = vec2(0);
 	#define I 2
 	for (int i = 0; i < I; i++) {
        U -=u.xy; A -=a.xy; B -=b.xy; C -=c.xy; D -=d.xy; 
        p += vec4(length(U-A),length(U-B),length(U-C),length(U-D))-1.;
        g += vec2(a.z-c.z,b.z-d.z);
       	u = T(U);a = T(A); b = T(B); c = T(C); d = T(D);
 	}   
 	Q = u; 
 	vec4 N = 0.25*(a+b+c+d);
 	Q = mix(Q,N, vec4(0,0,1,0.01)); 
 	Q.xy -= g/10./float(I); 
 	Q.z += (p.x+p.y+p.z+p.w)/10.;
 	
 	Q.xy += S*f(U);
    
 	Q.z *= 0.999;
    
 	
 	if (iFrame < 1) Q = vec4(0);
 	if (length(U-vec2(0.1,.5)*R) < 2.) Q.xyw = vec3(0.2,0,.6);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xyw=vec3(0,0,0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define Y 4.5
#define S -.4

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec2 st (float p, float n, float p1, float n1, vec2 v) {
	return (p*p1+n*n1-p*n1-n*p1)*normalize(v)/dot(v,v);
}
vec2 f (vec2 U) {
	vec4 me = T(U);
    vec2 sf = vec2(0);
    me.z+=1.;
    for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
       if (x==0&&y==0) continue;
       vec2 u = vec2(x,y)*Y;
       vec4 o = T(U+u);
       o.z+=1.;
       sf += st (me.w*me.z,(1.-me.w)*me.z,o.w*o.z,(1.-o.w)*o.z,u);
    }
    }
    return sf;
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u = T(U), a = T(A), b = T(B), c = T(C), d = T(D);
 	vec4 p;
 	vec2 g = vec2(0);
 	#define I 2
 	for (int i = 0; i < I; i++) {
        U -=u.xy; A -=a.xy; B -=b.xy; C -=c.xy; D -=d.xy; 
        p += vec4(length(U-A),length(U-B),length(U-C),length(U-D))-1.;
        g += vec2(a.z-c.z,b.z-d.z);
       	u = T(U);a = T(A); b = T(B); c = T(C); d = T(D);
 	}   
 	Q = u; 
 	vec4 N = 0.25*(a+b+c+d);
 	Q = mix(Q,N, vec4(0,0,1,0.01)); 
 	Q.xy -= g/10./float(I); 
 	Q.z += (p.x+p.y+p.z+p.w)/10.;
 	
 	Q.xy += S*f(U);
    
 	Q.z *= 0.999;
    
 	
 	if (iFrame < 1) Q = vec4(0);
 	if (length(U-vec2(0.1,.5)*R) < 2.) Q.xyw = vec3(0.2,0,.6);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xyw=vec3(0,0,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define Y 2.
#define S 0.2

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec2 st (float p, float n, float p1, float n1, vec2 v) {
	return (p*p1+n*n1-p*n1-n*p1)*normalize(v)/dot(v,v);
}
vec2 f (vec2 U) {
	vec4 me = T(U);
    vec2 sf = vec2(0);
    for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
       if (x==0&&y==0) continue;
       vec2 u = vec2(x,y)*Y;
       vec4 o = T(U+u);
       sf += st (me.w,(1.-me.w),o.w,(1.-o.w),u);
    }
    }
    return sf;
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u = T(U), a = T(A), b = T(B), c = T(C), d = T(D);
 	vec4 p;
 	vec2 g = vec2(0);
 	#define I 2
 	for (int i = 0; i < I; i++) {
        U -=u.xy; A -=a.xy; B -=b.xy; C -=c.xy; D -=d.xy; 
        p += vec4(length(U-A),length(U-B),length(U-C),length(U-D))-1.;
        g += vec2(a.z-c.z,b.z-d.z);
       	u = T(U);a = T(A); b = T(B); c = T(C); d = T(D);
 	}   
 	Q = u; 
 	vec4 N = 0.25*(a+b+c+d);
 	Q = mix(Q,N, vec4(0,0,1,0.01)); 
 	Q.xy -= g/10./float(I); 
 	Q.z += (p.x+p.y+p.z+p.w)/10.;
 	
 	Q.xy += S*f(U);
    
 	Q.z *= 0.999;
    
 	
 	if (iFrame < 1) Q = vec4(0);
 	if (length(U-vec2(0.1,.5)*R) < 2.) Q.xyw = vec3(0.2,0,.6);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xyw=vec3(0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define Y 1.5
#define S 0.4

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec2 st (float p, float n, float p1, float n1, vec2 v) {
	return (p*p1+n*n1-p*n1-n*p1)*normalize(v)/dot(v,v);
}
vec2 f (vec2 U) {
	vec4 me = T(U);
    vec2 sf = vec2(0);
    for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
       if (x==0&&y==0) continue;
       vec2 u = vec2(x,y)*Y;
       vec4 o = T(U+u);
       sf += st (me.w,(1.-me.w),o.w,(1.-o.w),u);
    }
    }
    return sf;
}
void mainImage( out vec4 Q, in vec2 U )
{   R = iResolution.xy;
 	vec2 O = U,A = U+vec2(1,0),B = U+vec2(0,1),C = U+vec2(-1,0),D = U+vec2(0,-1);
 	vec4 u = T(U), a = T(A), b = T(B), c = T(C), d = T(D);
 	vec4 p;
 	vec2 g = vec2(0);
 	#define I 2
 	for (int i = 0; i < I; i++) {
        U -=u.xy; A -=a.xy; B -=b.xy; C -=c.xy; D -=d.xy; 
        p += vec4(length(U-A),length(U-B),length(U-C),length(U-D))-1.;
        g += vec2(a.z-c.z,b.z-d.z);
       	u = T(U);a = T(A); b = T(B); c = T(C); d = T(D);
 	}   
 	Q = u; 
 	vec4 N = 0.25*(a+b+c+d);
 	Q = mix(Q,N, vec4(0,0,1,0.01)); 
 	Q.xy -= g/10./float(I); 
 	Q.z += (p.x+p.y+p.z+p.w)/10.;
 	
 	Q.xy += S*f(U);
    
 	Q.z *= 0.999;
    
 	
 	if (iFrame < 1) Q = vec4(0);
 	if (length(U-vec2(0.1,.5)*R) < 2.) Q.xyw = vec3(0.2,0,.6);
 	if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) Q.xyw=vec3(0,0,0);
}