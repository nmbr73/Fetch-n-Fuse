

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{	R = iResolution.xy;
 	 vec4 me = T(U),
        a = T(U+vec2(1,0)),
        b = T(U-vec2(1,0)),
        c = T(U+vec2(0,1)),
        d = T(U-vec2(0,1));
 	vec3 n = normalize( vec3(a.z-b.z, c.z-d.z, .1  ));
 	vec4 p = P(U);
 	p = p*p*p;
    C = 1.3*abs(sin(
        4.*me.zwzw + (
        p.w*vec4(1,2,3,4)+
        p.z*vec4(3,2,1,4)+
        p.y*vec4(2,3,1,4)+
        p.x*vec4(3,1,2,4))
    ));
 	C *= 0.5+0.5*texture(iChannel2,n);
	
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 R;
vec4 t (vec2 U) { // access the buffer
	return texture(iChannel0,U/R);
}
vec4 T (vec2 U) {
    // sample things where they were, not where they are
    // half step backwards through time twice
	U -= 0.5*t(U).xy;
	U -= 0.5*t(U).xy;
    return t(U);
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
    // me and my neighborhoood
    // anytime there is a "0.25*__"
    // this is because we are using a grid
    // if we had hexagonal tiling you would see "1/6*__"
   vec4 me = T(U),
        n = T(U+vec2(0,1)), // north  up
        e = T(U+vec2(1,0)), // east   left
        s = T(U-vec2(0,1)), // south  down
        w = T(U-vec2(1,0)), // west   right
       mu = 0.25*(n+e+s+w); // average
   C = me;
   C.x -= 0.25*(e.z-w.z); // change in pressure from left to right
   C.y -= 0.25*(n.z-s.z); // change in pressure from top to bottom
   // divergence plus pressure exchange :
   C.z = mu.z // average pressure of neighborhood
        +0.25*(s.y-n.y+w.x-e.x); // how much is the neighborhood pushing on me

/////////////////////
// adapted from : https://www.shadertoy.com/view/4tGfDW   	
#define SPIN_PERMITIVITY 0.1    
/**/C.w = 
    mix(mu.w,C.w,SPIN_PERMITIVITY) // neighbors trade spin
    +			 SPIN_PERMITIVITY*(// the curl puts spin into the cell
        (e.y - w.y - n.x + s.x) - C.w // difference between the curl and the spin
    );
/**/C.xy += 
    abs(C.w)* // the spin of the cell 
    0.25*vec2(
        n.w-s.w,  // the baseball shooter force from top to bottom - so it shoots from left to right
        w.w-e.w); // the baseball shooter force from the left to the right - so it shoots up or down
////////////////////// 
    
   
  // boundary conditions
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = mix(C,vec4(0,0,0,1),0.01);
   if (U.x < 1. || iFrame < 1) C = vec4(0.15,0,0,0);
   #else
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = mix(C,vec4(0.7,0,0,1),0.01);
   if (length(U-vec2(0.9,0.5)*R)<0.02*R.x) C = mix(C,vec4(-0.7,0,0,1),0.01);
   if (length(U-vec2(0.5,0.1)*R)<0.02*R.x) C = mix(C,vec4(0,0.7,0,1),0.01);
   if (length(U-vec2(0.5,0.9)*R)<0.02*R.x) C = mix(C,vec4(0,-0.7,0,1),0.01);
   if (iFrame < 1) C = vec4(0);
   #endif
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{   R = iResolution.xy;
 	U = U-0.5*T(U).xy;
 	U = U-0.5*T(U).xy;
 	C = P(U);
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = vec4(0,0,0,1.5);
   if (U.x < 1. || iFrame < 1) C = vec4(0);
   #else
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C.x = 1.;
   if (length(U-vec2(0.9,0.5)*R)<0.02*R.x) C.y = 1.;
   if (length(U-vec2(0.5,0.1)*R)<0.02*R.x) C.z = 1.;
   if (length(U-vec2(0.5,0.9)*R)<0.02*R.x) C.w = 1.;
   if (iFrame < 1) C = vec4(0);
   #endif
 	if (iFrame < 1) C = vec4(0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 R;
vec4 t (vec2 U) { // access the buffer
	return texture(iChannel0,U/R);
}
vec4 T (vec2 U) {
    // sample things where they were, not where they are
    // half step backwards through time twice
	U -= 0.5*t(U).xy;
	U -= 0.5*t(U).xy;
    return t(U);
}
void mainImage( out vec4 C, in vec2 U )
{
   R = iResolution.xy;
    // me and my neighborhoood
    // anytime there is a "0.25*__"
    // this is because we are using a grid
    // if we had hexagonal tiling you would see "1/6*__"
   vec4 me = T(U),
        n = T(U+vec2(0,1)), // north  up
        e = T(U+vec2(1,0)), // east   left
        s = T(U-vec2(0,1)), // south  down
        w = T(U-vec2(1,0)), // west   right
       mu = 0.25*(n+e+s+w); // average
   C = me;
   C.x -= 0.25*(e.z-w.z); // change in pressure from left to right
   C.y -= 0.25*(n.z-s.z); // change in pressure from top to bottom
   // divergence plus pressure exchange :
   C.z = mu.z // average pressure of neighborhood
        +0.25*(s.y-n.y+w.x-e.x); // how much is the neighborhood pushing on me

/////////////////////
// adapted from : https://www.shadertoy.com/view/4tGfDW   	
#define SPIN_PERMITIVITY 0.1    
/**/C.w = 
    mix(mu.w,C.w,SPIN_PERMITIVITY) // neighbors trade spin
    +			 SPIN_PERMITIVITY*(// the curl puts spin into the cell
        (e.y - w.y - n.x + s.x) - C.w // difference between the curl and the spin
    );
/**/C.xy += 
    abs(C.w)* // the spin of the cell 
    0.25*vec2(
        n.w-s.w,  // the baseball shooter force from top to bottom - so it shoots from left to right
        w.w-e.w); // the baseball shooter force from the left to the right - so it shoots up or down
////////////////////// 
    
   
  // boundary conditions
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = mix(C,vec4(0,0,0,1),0.01);
   if (U.x < 1. || iFrame < 1) C = vec4(0.15,0,0,0);
   #else
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = mix(C,vec4(0.7,0,0,1),0.01);
   if (length(U-vec2(0.9,0.5)*R)<0.02*R.x) C = mix(C,vec4(-0.7,0,0,1),0.01);
   if (length(U-vec2(0.5,0.1)*R)<0.02*R.x) C = mix(C,vec4(0,0.7,0,1),0.01);
   if (length(U-vec2(0.5,0.9)*R)<0.02*R.x) C = mix(C,vec4(0,-0.7,0,1),0.01);
   if (iFrame < 1) C = vec4(0);
   #endif
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<

vec2 R;
vec4 T ( vec2 U ) {return texture(iChannel0,U/R);}
vec4 P ( vec2 U ) {return texture(iChannel1,U/R);}
void mainImage( out vec4 C, in vec2 U )
{   R = iResolution.xy;
 	U = U-0.5*T(U).xy;
 	U = U-0.5*T(U).xy;
 	C = P(U);
   #ifdef VORTEX_SHEDDING_MODE
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C = vec4(0,0,0,1.5);
   if (U.x < 1. || iFrame < 1) C = vec4(0);
   #else
   if (length(U-vec2(0.1,0.5)*R)<0.02*R.x) C.x = 1.;
   if (length(U-vec2(0.9,0.5)*R)<0.02*R.x) C.y = 1.;
   if (length(U-vec2(0.5,0.1)*R)<0.02*R.x) C.z = 1.;
   if (length(U-vec2(0.5,0.9)*R)<0.02*R.x) C.w = 1.;
   if (iFrame < 1) C = vec4(0);
   #endif
 	if (iFrame < 1) C = vec4(0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//#define VORTEX_SHEDDING_MODE