

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS#
// see also https://www.shadertoy.com/view/4dGGz1 to play with spectrum.
// NB: see new version here: https://www.shadertoy.com/view/XtScWt <<<<

// set your module and phase in Buf A

#define SIZE 256. //Size must be changed in each tab.

//Display modes
#define MAGNITUDE 0.
#define PHASE 1.
#define COMPONENT 2.

float DISPLAY_MODE = MAGNITUDE;

//Scaling
#define LOG 0
#define LINEAR 1

#define MAG_SCALE LOG

vec4 rainbow(float x)  { return .5 + .5 * cos(6.2832*(x - vec4(0,1,2,0)/3.)); }
vec4 rainbow(vec2 C)   { return rainbow(atan(C.y,C.x)/3.1416 + .5); }

vec4 paintDFT(vec2 F) {
  if (DISPLAY_MODE == MAGNITUDE)
     #if MAG_SCALE == LOG
        return vec4( log(length(F)) / log(SIZE*SIZE) );
     #elif MAG_SCALE == LINEAR
        return vec4( length(F) / SIZE );
     #endif

    else if ( DISPLAY_MODE == PHASE )     return rainbow(F);        
    else /* if ( DISPLAY_MODE == COMPONENT ) */ return vec4(.5 + .5*F/SIZE, 0,0);        
}

float message(vec2 p) {  // the alert function to add to your shader
    int x = int(p.x+1.)-1, y=int(p.y)-10,  i;
    if (x<1||x>32||y<0||y>2) return -1.; 
    i = ( y==2? i=  757737252: y==1? i= 1869043565: y==0? 623593060: 0 )/ int(exp2(float(32-x)));
 	return i==2*(i/2) ? 1. : 0.;
}


void mainImage( out vec4 O,  vec2 uv )
{
    vec2 R = iResolution.xy;
    if (iResolution.y<200.) // alert for the icon
        {   float c=message(uv/8.); if(c>=0.){ O=vec4(c,0,0,0);return; } }
        
    vec2 pixel = ( uv - iResolution.xy/2.) / SIZE  + vec2(2,1)/2.,
         tile  = floor(pixel),
         stile = floor(mod(2.*pixel,2.));
         uv = fract(pixel) * SIZE / R ;

    O-=O;
    
    DISPLAY_MODE = floor(texture(iChannel3, .5/R).w); // persistant key flag.
    if (tile.y==-1. && abs(tile.x-.5)<1.) {   // buttons displaying current flags value
        for (float i=0.; i<3.; i++) 
            O += smoothstep(.005,.0,abs(length(uv*R/SIZE-vec2(.2+i/7.,.97))-.025));
        float v = DISPLAY_MODE;
        O.b += smoothstep(.03,.02,length(uv*R/SIZE-vec2(.2+v/7.,.97)));
    }
    
    if(tile == vec2(0,0))  //Input + DFT (Left)
        if (stile == vec2(0) )
             O += paintDFT(texture(iChannel1, 2.*uv).xy);
        else O += length(texture(iChannel0, uv).rgb);

    if(tile == vec2(1,0))  // Output +DFT (Right)
        if (stile == vec2(0) )
             O += paintDFT(texture(iChannel3, 2.*uv).xy);
        else 
            O += .5+.5*texture(iChannel2, uv).x;
          //O += length(texture(iChannel2, uv).xy);

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// creation of the input

#define SIZE 256. //Size must be changed in each tab.

void mainImage( out vec4 O, vec2 uv )
{
    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) return;
        
    vec2 R = iResolution.xy;
    O.x = length( texture(iChannel0, uv/R* R.y/SIZE).rgb );
    // O.y = ...
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Fourier transform of the input

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> buf.zw -> buf.xy -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


#define SIZE 256. //Size must be changed in each tab.

vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x) * vec2(cos(b),sin(b)); } 
// #define ang(a)  vec2(cos(a), sin(a))
// vec2 cmul (vec2 a,float t) { vec2 b=ang(t); return mat2(b,-b.y,b.x)*a; } 

void mainImage( out vec4 O, vec2 uv )
{
    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) return;
        
    for(float n = 0.; n < SIZE; n++)  {
        vec2 xn = texture(iChannel0, vec2(n+.5, uv.y) / iResolution.xy).xy,
             yn = texture(iChannel1, vec2(uv.x, n+.5) / iResolution.xy).zw,
             a = - 6.2831853 * (uv-.5 -SIZE/2.) * n/SIZE;
        
        O.zw += cmul(xn, a.x);
        O.xy += cmul(yn, a.y);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// do your operation in spectral domain here

#define SIZE 256. //Size must be changed in each tab.

bool keyPress(int ascii) { return (texture(iChannel2,vec2((.5+float(ascii))/256.,0.25)).x > 0.); }
float rand(vec2 uv) { return fract(1e5*sin(dot(uv,vec2(17.4,123.7)))); }
float gauss(float x) { return exp(-.5*x*x); }
#define ang(a)  vec2(cos(a), sin(a))
vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x) * vec2(cos(b),sin(b)); } 

void mainImage( out vec4 O, vec2 U )
{
    vec2 T = texture(iChannel0, U / iResolution.xy).xy;
    U -= .5;
    vec2 X = 2.*U/SIZE - 1.;
    float l = length(X), s = sign(-X.x), y = iMouse.y/iResolution.y;
    
# if 1        
    // --- your custom Fourier-space filter here --------------------
    float    
       // F = l*10.;                     // derivative
       // F = .01/(.01+l);               // integral
       // F = gauss(l/.125);             // gaussian blur
          F = smoothstep(.3,.0, abs(l-.5)) * 20.; // ring filter
       // F = smoothstep(-.1,.1,l-y) * 20.; // kill LF (mouse tuning)
       // F = float(fract(U.x/2.)*fract(U.y/2.)>0.);   // odd  freq only
       // F = float(fract(U.x/2.)+fract(U.y/2.)==0.);  // even freq only

    T *= F;
    O = vec4(T,F,F);
    
# else
    // --- or, your custom Fourier-space function here ------------
    //            see also https://www.shadertoy.com/view/4dGGz1
       T = ang(6.2832*rand(U));                // white noise
    // T *= gauss(l/.05)*10.;                  // modulus profile : gauss
       T *= gauss(abs(l-.12)/.005)*10.;        // modulus profile : ring (blue noise)
       T = cmul(T,iTime*s);              // phase shift with time
    
    T *= SIZE;
             
    O = vec4(T,T);
    
#endif
    
    if ( U==vec2(0)) {
        O.w = texture(iChannel1,U/iResolution.xy).w;
        if ( keyPress(32) ) O.w = mod(O.w+.1, 3.) ; // persistant key flag
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// invFourier transform 

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> buf.zw -> buf.xy -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


#define SIZE 256. //Size must be changed in each tab.

vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x)*vec2(cos(b), sin(b)); } 
#define W(uv)   mod(uv+SIZE/2.,SIZE)                    // wrap [-1/2,1/2] to [0,1]


void mainImage( out vec4 O, vec2 uv )
{
    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) return;

    for(float n = 0.; n < SIZE; n++)  {
        float m = W(n);       // W to warp 0,0 to mid-window.
        vec2 xn = texture(iChannel0, vec2(m+.5, uv.y) / iResolution.xy).xy,
             yn = texture(iChannel1, vec2(uv.x, m+.5) / iResolution.xy).zw,
             a =  6.2831853 *  (uv-.5) * n/SIZE;
        
        O.zw += cmul(xn, a.x);
        O.xy += cmul(yn, a.y);
    }
    O /= SIZE;
}