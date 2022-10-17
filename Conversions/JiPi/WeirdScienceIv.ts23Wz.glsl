

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 F (vec2 U) {return texture(iChannel0,U/R);}
vec4 E (vec2 U) {return texture(iChannel2,U/R);}
vec4 I (vec2 U) {return texture(iChannel3,U/R);}
void mainImage( out vec4 C, in vec2 U )
{	R = iResolution.xy;
    vec4 i = I(U);
 	vec4 f = F(U);
 	vec4 e = E(U);
 	C = 0.5+0.5*sin(1.*length(e)+10.*vec4(1,1.3,1.5,4)*f.z);

 	vec3 n = normalize(vec3(f.xy,.1));
 	C *= 0.5+0.5*texture(iChannel1,n);
 	C *= 1.-i.xxxx+2.*length(i.zw);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Fluid
vec2 R;
vec4 I (vec2 U) {return texture(iChannel3,U/R);}
vec4 f (vec2 U) {return texture(iChannel0,U/R);}
vec4 F (vec2 U) {
    // advection step :
    //	count backwards through spacetime
    //  what was going on two half times ago?
    U-=0.5*f(U).xy;// where I am - half speed = where I was half a time ago
    U-=0.5*f(U).xy;// where I was half time ago - half speed = where I was last
    return f(U);   // now what ever was I doing one time ago?
}
vec4 E (vec2 U) {return texture(iChannel1,U/R);}
vec4 M (vec2 U) {return texture(iChannel2,U/R);}
vec4 X (vec2 U, in vec4 C, vec2 r) {
	vec4 n = F(U+r); // neighbor
    vec2 rp = vec2(-r.y,r.x); // perpiduclar to r
    return vec4(
        	 r *(n.z-C.z) //  pressure gradent
        +	 rp*(n.w*C.w)// + spin product
       	+	 C.xy, 		//  + advected velocity
 //====================//   = equals velocity dxy/dt
        dot(r ,n.xy-C.xy)+n.z,    // pressure = radial change in velocity + pressure between cells
    	dot(rp,n.xy-C.xy)-(n.w)); // spin     = circular change in velocity + spin between cells
    
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = F(U);
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
  
   C.xy += E(U).xy*(1.-I(U).x);
   if (iFrame < 1) C = vec4(0);
   if (U.x<1.||U.y<1.||R.x-U.x<1.||R.y-U.y<1.) C.xy *= 0.;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// ELECTRIC FIELD
vec2 R;
float ln (vec2 p, vec2 a, vec2 b) { // returns distance to line segment for mouse input
    return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));
}
vec2 v (vec2 U) {return texture(iChannel0, U/R).xy;}
vec3 E (vec3 U) {
    U.xy -= 0.5*v(U.xy);
    U.xy -= 0.5*v(U.xy);
	return texture(iChannel1,U.xy/R).xyz;
}
vec3 M (vec3 U) {
    U.xy -= 0.5*v(U.xy);
    U.xy -= 0.5*v(U.xy);
	return texture(iChannel2,U.xy/R).xyz;
}
vec4 I (vec2 U) {return texture(iChannel3,U/R);}
vec3 X (vec3 U, vec3 R) {
	return cross(R,M(U+R));
}

void mainImage( out vec4 c, in vec2 u )
{	R = iResolution.xy;
 
 	vec3 U = vec3(u,0);
 	# define l inversesqrt (2.)
 	vec3 mu = 0.25*(E(U+vec3(1,0,0))+E(U-vec3(1,0,0))+E(U+vec3(0,1,0))+E(U-vec3(0,1,0)));
    vec3 C = mix(E(U),mu,0.1)
        + (- M(U) + 
      ( X(U, vec3( 1, 0,0)) + 
        X(U, vec3(-1, 0,0)) + 
        X(U, vec3( 0, 1,0)) + 
        X(U, vec3( 0,-1,0)) + 
        X(U, vec3( l, l,0)) + 
        X(U, vec3( l,-l,0)) +
        X(U, vec3(-l,-l,0)) +
        X(U, vec3(-l, l,0)) ) / 8.);
 	
 	C.xy += I(U.xy).zw;
 
 	if (iFrame < 1) C = vec3(0);
	c = vec4(C,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// MAGNETIC FIELD
vec2 R;
vec4 I (vec2 U) {return texture(iChannel3,U/R);}
vec2 v (vec2 U) {return texture(iChannel0,U/R).xy;}
vec3 E (vec3 U) {
	return texture(iChannel1,U.xy/R).xyz;
}
vec3 M (vec3 U) {
    U.xy -= 0.5*v(U.xy);
    U.xy -= 0.5*v(U.xy);
	return texture(iChannel2,U.xy/R).xyz;
}
vec3 X (vec3 U, vec3 R) {
	return cross(R,E(U+R));
}
void mainImage( out vec4 c, in vec2 u )
{	R = iResolution.xy;
 
 	vec3 U = vec3(u,0);
 	# define l inversesqrt (2.)
 	vec3 mu = 0.25*(M(U+vec3(1,0,0))+M(U-vec3(1,0,0))+M(U+vec3(0,1,0))+M(U-vec3(0,1,0)));
    vec3 C = mix(M(U),mu,0.1) +( E(U) -
      ( X(U, vec3( 1, 0,0)) + 
        X(U, vec3(-1, 0,0)) + 
        X(U, vec3( 0, 1,0)) + 
        X(U, vec3( 0,-1,0)) + 
        X(U, vec3( l, l,0)) + 
        X(U, vec3( l,-l,0)) +
        X(U, vec3(-l,-l,0)) +
        X(U, vec3(-l, l,0)) ) / 8.);
 	if (iFrame < 1) C = vec3(0);
 	c = vec4(C,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// ion particles
vec2 R;
vec2 v (vec2 U) {return texture(iChannel0,U/R).xy;}
vec4 i (vec2 U) {return texture(iChannel3,U/R);}
vec4 I (vec2 U) {
    U-=0.5*v(U).xy;
    U-=0.5*v(U).xy;
    return i(U);
}
vec4 E (vec2 U) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
   C = I(U);
   float n = I(U+vec2(0,1)).x,
         e = I(U+vec2(1,0)).x,
       	 s = I(U-vec2(0,1)).x,
         w = I(U-vec2(1,0)).x,
         mu = 0.25*(n+e+s+w);
   
   if (iMouse.z>0.)C.x = mix(C.x,1.,smoothstep(10.,0.,length(U-iMouse.xy)));
   	  if (C.x < 0.5)  C.x -= 0.01*C.x*C.x;
 else if (C.x > 0.6)  C.x += 0.01*(1.-C.x)*(1.-C.x);
    
   C.zw = -vec2(n-s,-e+w)-vec2(e-w,n-s);
    
   if (iFrame < 1) {
       C = vec4(0);
       C.x = smoothstep(0.15*R.y,0.15*R.y-3.,length(U-0.5*R));
   }
}
// >>> ___ GLSL:[Sound] ____________________________________________________________________ <<<
#define pi2 6.2831
#define m_c 261.625565
#define pw 1.05946309436
vec3 hash31(float p) // Dave Hoskins
{
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+19.19);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}
float nl (float time, float nonlinear) {
    time *= .25;
	return 
        nonlinear*(
            4.*cos(time)+5.*cos(2.*time)+8.*cos(4.*time)+.5*cos(8.*time)
        );
}
float X (
    	 float time,
    	 float phase,
    	 float octave, 
         float note, 
         float nonlinear,
         float attack,
         float sustain,
         float release
        ) {
    float phi = phase*pi2 + nl(time,nonlinear);
    float w = m_c*pi2*exp2(octave)*pow(pw,note);
    float envelope = smoothstep(0.,attack,time-phase)*smoothstep(release,0.,time-phase-attack-sustain);
    return (cos(w*time-phi)+cos(0.5*w*time-phi))*envelope;
}

vec2 mainSound( in int samp, float t )
{
    float a = 0.;
    float time = 0.;
    float note = 0.;
    for (float i = 0.; i < 400.; i++) {
        vec3 h  = hash31(i);
        float duration = 8.*exp2(-floor(h.x*6.));
        float octave   = mod(20.*h.y*h.y+floor(i/100.),5.)-3.;
        note = 4.*mod(floor(h.z*4000.),3.);
        time += duration+.15;
        
        a += X (t,time,octave,note,15.*sin(0.3*i+3.*t*h.z),.01,duration*0.8,0.1);
        
        if (mod(i,100.)<1.) time = 0.;
        if (note > 12.||note<-12.) note = 0.;
        
    }
    
    return vec2(atan(0.2*a)/2.);
}