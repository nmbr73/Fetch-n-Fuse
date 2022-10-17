

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Caustic Drawing and red and blue dot drawing
#define N 2
vec2 R;
float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 D (vec2 U) {return texture(iChannel3,U/R);}
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
float dI (vec2 U, vec3 me, vec3 light, float mu) {
    vec3 r = vec3(U,100);
    vec3 n = normalize(vec3(D(r.xy).zw,mu));
    vec3 li = reflect((r-light),n);
    float len = ln(me,r,li);
    return 5.e-1*exp(-len);
}
float I (vec2 U, vec3 me, vec3 light, float mu) {
    float intensity = 0.;
    for (int x = -N; x <= N; x++)
        for (int y = -N; y <= N; y++){
            float i = dI(U+vec2(x,y),me,light,10.*mu);
            intensity += i*i;
        }
        return intensity;
}
void mainImage( out vec4 Q, in vec2 U)
{
    R = iResolution.xy;
    vec3 light = vec3(0.5*R,1e5);
    vec3 me    = vec3(U,0);
	vec4 a = A(U);
    vec4 c = C(U);
    float l = I(U,me,light,1.);
    float r = smoothstep(2.+0.05*abs(a.w),.5,length(U-a.xy));
    Q = l+r*vec4(abs(sign(a.w)),-sign(a.w),-sign(a.w),1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Particle tracking
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void X (vec2 U, inout vec4 Q, vec2 u) {
    vec4 p = A(U+u);
    if (length(p.xy - U) < length(Q.xy-U)) Q = p;
    
}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
 	Q = A(U);
 	// measure neighborhood
 	for (int x = -1; x <= 1; x++)
     for (int y = -1; y <= 1; y++)
      X(U,Q,vec2(x,y));
 	vec2 u = Q.xy;
 	vec4 
        n = C(u+vec2(0,1)),
        e = C(u+vec2(1,0)),
        s = C(u+vec2(0,-1)),
        w = C(u+vec2(-1,0));
        
 	vec3 dx = e.xyz-w.xyz;
 	vec3 dy = n.xyz-s.xyz;
	// THE FORCE HERE IS COMPUTED WITH THE GRADIENT
 	// I DONT NEED ANY INFORMATION ABOUT NEIGHBORING PARTICLES
 	vec2 v = -Q.w*vec2(dx.x,dy.x)-vec2(dx.y,dy.y);
 	Q.xy += clamp(SPEED*v/(1.+0.2*abs(Q.w)),vec2(-1),vec2(1));
    // init
    if (iFrame < 1) {
        vec2 u = U;
        U = floor((u)/SEPARATION)*SEPARATION;
        if (U.x<0.55*R.x&&U.x>0.45*R.x) {
            U=floor(u/SEPARATION*4.)*SEPARATION/4.;
            Q = vec4(U,1,-0.5);
        } else Q = vec4(U,1,sign(U.x-0.5*R.x)*(U.y>0.5*R.y?-9.:2.));
    	
	}
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Blur pass 1
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
	Q = vec4(0);
 	for (float i = -BLUR_DEPTH ; i <= BLUR_DEPTH ; i++) {
 		vec4 a = A(U+vec2(i,0));
        vec4 c = vec4(a.w,1.,0,0)*smoothstep(1.+0.05*abs(a.w),1.,length(U+vec2(i,0)-a.xy));
        Q += c*sqrt(FORCE_RANGE)/FORCE_RANGE*exp(-i*i*0.5/FORCE_RANGE);
 	}
 		
 	
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Blur pass 2
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
	Q = vec4(0);
 	for (float i = -BLUR_DEPTH ; i <= BLUR_DEPTH ; i++) {
 		vec4 c = B(U+vec2(0,i));
        Q += c*sqrt(FORCE_RANGE)/FORCE_RANGE*exp(-i*i*0.5/FORCE_RANGE);
 	}
 vec4 
        n = C(U+vec2(0,1)),
        e = C(U+vec2(1,0)),
        s = C(U+vec2(0,-1)),
        w = C(U+vec2(-1,0));
 	Q = C(U) + 0.5*(Q-C(U));
 	if (iMouse.z > 0.) Q.xy += vec2(10.)*exp(-vec2(.01,.05)*length(U-iMouse.xy));
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// charge force range, collision force range ,0,0 does nothing
#define FORCE_RANGE vec4(   25, 2.5,     0,0)

// how many blur iterations
#define BLUR_DEPTH 40.
// multiplies the force per frame
#define SPEED 2.
// restart after changing. Smaller number -> more particles
#define SEPARATION 13.
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
//Gradient calculation for caustic
vec2 R;
vec4 A (vec2 U) {return texture(iChannel0,U/R);}
vec4 B (vec2 U) {return texture(iChannel1,U/R);}
vec4 C (vec2 U) {return texture(iChannel2,U/R);}
vec4 D (vec2 U) {return texture(iChannel3,U/R);}
void mainImage( out vec4 Q, in vec2 U )
{	R = iResolution.xy;
 	vec4 a = A(U);
 	float r=smoothstep(4.,1.,length(U-a.xy));
	Q = r*vec4(a.w,abs(a.w),-a.w,1);
 	Q = max(Q,D(U));
 	float 
        n = C(U+vec2(0,1)).x,
        e = C(U+vec2(1,0)).x,
        s = C(U+vec2(0,-1)).x,
        w = C(U+vec2(-1,0)).x;
 	Q.zw = vec2(e-w,n-s);
}
