

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
    R = iResolution.xy;
    vec4 
        o = T(U),
        a = T(U+vec2(1,0)),
        b = T(U-vec2(1,0)),
        c = T(U+vec2(0,1)),
        d = T(U-vec2(0,1));
        
    vec4 g = vec4(a.zw-b.zw,c.zw-d.zw);
    vec2 dz = g.xz;
    vec2 dw = g.yw;
    vec3 n = normalize(vec3(dz,.05));
    vec4 tx = texture(iChannel2,reflect(vec3(0,0,1),n));
    C = abs(cos(o.w*vec4(140,162,175,20)))*(0.7+0.3*tx);
    vec2 u = U;
    for (int i = 0; i < 50; i++) {
        U -= T(U).xy;
    }
    U.x-= float(iFrame)*Mach_Number;
    U = sin(U*.2);
    vec2 w=fwidth(U)*1.2;
    C *= smoothstep(-w.x,w.x,abs(U.x))*smoothstep(-w.y,w.y,abs(U.y));
    U = u;
   float t = 0.5+0.3*sin(iTime);
   if (iMouse.z > 0.) t = 0.5+0.3*(iMouse.y/R.y*2.-1.);
   float si = sin(t), co = cos(t);
   mat2 ro = mat2(co,-si,si,co);
   U = (U-vec2(0.25,0.5)*R)*ro;
   U.x *= 0.1;
   U.y -= 20.*exp(-3e-2*U.x*U.x);
   if (length(U) < 6.) {
    	C = abs(sin(10.*o.w+o.z*vec4(1,2,5,4)));
   }
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Fluid
vec2 R;
vec4 T (vec2 U) { return texture(iChannel0,U/R);}
vec4 X (vec2 U, in vec4 C, vec2 r) {
	vec4 n = T(U+r); // neighbor
    vec2 rp = vec2(-r.y,r.x); // perpiduclar to r
    return vec4(
        	 r *(n.z-C.z) + // pressure term
             rp*(n.w*C.w) + // spin term 
        	 mix(C.xy,n.xy,0.3),//viscous term
        dot(r ,n.xy-C.xy)+n.z, // pressure calculation
    	dot(rp,n.xy-C.xy)-(n.w));// spin calculation
    
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = T(U);
   float r2 = sqrt(2.)*0.5; // without renormalization, the neighborhood needs to be equidistant to the cell
   // calculate the sum of all neighbor interactions
   C = X(U,C,vec2( 1, 0)) + 
       X(U,C,vec2( 0, 1))+
       X(U,C,vec2(-1, 0))+
       X(U,C,vec2( 0,-1))+
       X(U,C,vec2( r2, r2))+
       X(U,C,vec2(-r2, r2))+
       X(U,C,vec2(-r2,-r2))+
       X(U,C,vec2( r2,-r2));
   C /= 8.; // divide by the neighborhood size
  	
    
    
   if (iFrame < 1||U.x < 4.||R.x-U.x < 4.)
       C = vec4(Mach_Number,0,0,0);
   if (U.y < 4.||R.y-U.y < 4.) C.w = 0.;
   if (iFrame < 1) C.x = 0.;
   float t = 0.5+0.3*sin(iTime);
   if (iMouse.z > 0.) t = 0.5+0.3*(iMouse.y/R.y*2.-1.);
   float si = sin(t), co = cos(t);
   mat2 ro = mat2(co,-si,si,co);
   U = (U-vec2(0.25,0.5)*R)*ro;
   U.x *= 0.1;
   U.y -= 20.*exp(-3e-2*U.x*U.x);
   if (length(U) < 6.) C.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Advection Step
vec2 R;
vec4 T (vec2 U) {return texture(iChannel0,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   #define N 16.
   for (float i = 0.; i < N; i++)
       U -= T(U).xy/N;
   C = T(U);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Fluid
vec2 R;
vec4 T (vec2 U) { return texture(iChannel0,U/R);}
vec4 X (vec2 U, in vec4 C, vec2 r) {
	vec4 n = T(U+r); // neighbor
    vec2 rp = vec2(-r.y,r.x); // perpiduclar to r
    return vec4(
        	 r *(n.z-C.z) + // pressure term
             rp*(n.w*C.w) + // spin term 
        	 mix(C.xy,n.xy,0.3),//viscous term
        dot(r ,n.xy-C.xy)+n.z, // pressure calculation
    	dot(rp,n.xy-C.xy)-(n.w));// spin calculation
    
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = T(U);
   float r2 = sqrt(2.)*0.5; // without renormalization, the neighborhood needs to be equidistant to the cell
   // calculate the sum of all neighbor interactions
   C = X(U,C,vec2( 1, 0)) + 
       X(U,C,vec2( 0, 1))+
       X(U,C,vec2(-1, 0))+
       X(U,C,vec2( 0,-1))+
       X(U,C,vec2( r2, r2))+
       X(U,C,vec2(-r2, r2))+
       X(U,C,vec2(-r2,-r2))+
       X(U,C,vec2( r2,-r2));
   C /= 8.; // divide by the neighborhood size
  	
    
    
   if (iFrame < 1||U.x < 4.||R.x-U.x < 4.)
       C = vec4(Mach_Number,0,0,0);
   if (U.y < 4.||R.y-U.y < 4.) C.w = 0.;
   if (iFrame < 1) C.x = 0.;
   float t = 0.5+0.3*sin(iTime);
   if (iMouse.z > 0.) t = 0.5+0.3*(iMouse.y/R.y*2.-1.);
   float si = sin(t), co = cos(t);
   mat2 ro = mat2(co,-si,si,co);
   U = (U-vec2(0.25,0.5)*R)*ro;
   U.x *= 0.1;
   U.y -= 20.*exp(-3e-2*U.x*U.x);
   if (length(U) < 6.) C.xy *= 0.;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Advection Step
vec2 R;
vec4 T (vec2 U) {return texture(iChannel0,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   #define N 16.
   for (float i = 0.; i < N; i++)
       U -= T(U).xy/N;
   C = T(U);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define Mach_Number 1.
