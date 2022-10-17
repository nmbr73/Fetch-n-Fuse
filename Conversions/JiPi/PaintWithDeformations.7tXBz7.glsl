

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// variant of   swirl2 https://shadertoy.com/view/7l2cRz
//            + displacement https://www.shadertoy.com/view/sdjyRV

// === applies the uv field to a texture =================

//#define fwidth(v) length(vec2(dFdx(v),dFdy(v)))

void mainImage( out vec4 O, vec2 u )
{
    u = T(u).xy;                    // distorted uv field
    O = texture( iChannel1, u );    // apply map
    
    float v = length(u-.5);         // apply circle
    O = mix( O, vec4(1,0,0,0), smoothstep( 1.5, 0.,  abs(v-.3)/fwidth(v) ) );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// === distort the uv field ====================

#define keyDown(a) ( texelFetch(iChannel3,ivec2(a,0),0).x > 0.)

#define ortho(D)     vec2( -(D).y, (D).x )              // vec ortho to D
//#define S(D)       ortho(D) / max( dot(D,D) , 1e-3 )  // irrotational swirl
  #define S(D)       ortho(D) / max( length(D)*smoothstep(0.,3.,length(D)) , 1e-3 )  / 6e4 // irrotational swirl
//#define S(D)       ortho(D) * smoothstep(1.,0.,length(D)/.25) *.1

float f(float x) {
    x = abs(x)/.2; 
	return x < 1. ? - .02 * smoothstep(1.,0.,x) : 0.;
}

void mainImage( out vec4 O, vec2 u )
{
    if ( u== vec2(.5) )                                 // previous mouse pos
      { O = vec4(iMouse.xy*sign(iMouse.z), T(0).xy);return; }
      
    vec2 U = u / R.y, V,
         M =   iMouse.xy / R.y,
         d = ( iMouse.xy - T(0).zw ) / R.y;             // mouse drag direction
    if (iMouse.z > 0.)  {                               // if click
       float s = keyDown(37) ? -1. : keyDown(39) ? 1.   // key pressed
               : keyDown(38) ? 2.  : keyDown(40) ? -2. : 0., l;
       if ( abs(s) == 1. )                              // --- left or right key : swirl
           U +=  S(U-M)  * sign(s);                 // swirl field
       else  if ( abs(s) == 2. )                        // up or bottom key : grow/shrink
           l = length(U-M),
           U = M + (U-M) * ( 1. + f(l) * sign(s) ); 
       else if ( T(0).z > 0. )                          // --- drag & no key: displace
           U -= .5*d *smoothstep(.2,0., length(U-M) );  // displacement field
    }
    O = texture( iChannel0, U *R.y/R ); 
    if keyDown(8) O = texture( iChannel1, (N-1. + U *R.y/R ) / N ); 
    
    if ( iFrame < 1 ) O = vec4(u/R,0,0);                // init uv field
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R      iResolution.xy
#define T(U)   texelFetch( iChannel0, ivec2(U), 0 )

#define N 2.   // N² undo levels

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// === undo buffer 
// stores N² frames: if click, or every second if mouse drag

void mainImage( out vec4 O, vec2 U )
{
    if (iMouse.w <= 0. && ( iMouse.z<=0. || iFrame%60>0 ))  { O = T(U); return; }
    U *= N/R;
    vec2 I = floor(U), F = fract(U);
    if (I.x>0.) I.x--; else I.x = N-1., I.y--;
    O = I.y < 0. ? texture(iChannel1, F)
                 : texture(iChannel0,(I+F)/N);
}