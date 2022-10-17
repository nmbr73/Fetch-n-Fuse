

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// set your module and phase in Buf C

#define SIZE (2.*floor(iResolution.x/4.*.8)) // 256. //Size must be changed in each tab.


//Display modes
#define MAGNITUDE 0.
#define PHASE     1.
#define COMPONENT 2.
#define REAL      3.
#define IMAG      4.

//Scaling
#define LOG 0
#define LINEAR 1

#define MAG_SCALE LOG

vec4 rainbow(float x)  { return .5 + .5 * cos(6.2832*(x - vec4(0,1,2,0)/3.)); }
vec4 rainbow(vec2 C)   { return rainbow(atan(C.y,C.x)/3.1416 + .5); }

vec4 paintDFT(vec2 F, float mode) {
    // F /= SIZE;
    return 
         mode == MAGNITUDE 
     #if   MAG_SCALE == LOG
                           ?  vec4(log(length(F)))
     #elif MAG_SCALE == LINEAR
                           ?  vec4(length(F))
     #endif
       : mode == PHASE     ?  rainbow(F)        
       : mode == COMPONENT ?  .5+.5*vec4(F, 0,0)
       : mode == REAL      ?  .5+.5*vec4(F.x)
       : mode == IMAG      ?  .5+.5*vec4(F.y)
       : vec4(-1); // error
}

void mainImage( out vec4 O,  vec2 uv )
{
    vec2 R = iResolution.xy, U=uv;
    //O = texture(iChannel0,uv/R).xxxx; return;
        
    vec2 pixel = ( uv - iResolution.xy/2.) / SIZE  + vec2(2,1)/2.,
         tile  = floor(pixel),
         stile = floor(mod(2.*pixel,2.));
         uv = fract(pixel) * SIZE / R ;

    O-=O;
    
    vec2 DISPLAY_MODE = floor(texture(iChannel3, .5/R).zw); // persistant key flag.
    bool INSET =  texture(iChannel3,vec2(1.5,.5)/R).w == 0.;
    
    if (tile.y==-1. && abs(tile.x-.5)<1.) {                // buttons displaying current flags value
        for (float i=0.; i<5.; i++) 
            O += smoothstep(.005,.0,abs(length(uv*R/SIZE-vec2(.2+i/7.,.97))-.025));
        float v = tile.x==0. ? DISPLAY_MODE[0] : DISPLAY_MODE[1];
        O.b += smoothstep(.03,.02,length(uv*R/SIZE-vec2(.2+v/7.,.97)));
    }
    
    if(tile == vec2(0,0))                                             // --- Input + DFT (Left)
        if (stile == vec2(0) && INSET )
             O += paintDFT(texture(iChannel1, 2.*uv).xy, DISPLAY_MODE[0]); // initial spectrum
        else O += texture(iChannel0, uv).x;                                // initial texture
      //else O += length(texture(iChannel0, uv).rgb);

    if(tile == vec2(1,0))                                            // --- Output +DFT (Right)
        if (stile == vec2(0) && INSET)
             O += paintDFT(texture(iChannel3, 2.*uv).xy, DISPLAY_MODE[1]); // initial Fourier
        else O += paintDFT(texture(iChannel2, uv).xy, DISPLAY_MODE[1]);    // final texture
    //  else O += paintDFT(texture(iChannel2, fract(.5+uv*R/SIZE)*SIZE/R).xy, DISPLAY_MODE[1]); // fftshift
        
    if(tile.y>0.) O += texture(iChannel0, U/iResolution.xy).x; // displayed opt values in top margin   
    if (tile != mod(tile,vec2(2,1))) O+=.3;    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// source signal

#define SIZE (2.*floor(iResolution.x/4.*.8)) // 256. //Size must be changed in each tab.


void mainImage( out vec4 O, vec2 U )
{
//  NB: out of frame image (e.g. counters & sliders ) will be transmitted to final display.
    
    if ( max(U.x,U.y) < SIZE )
        O.x = length( texture( iChannel0, U/SIZE ).rb );

#if 0        // mouse controled impulse, for tests
    U += floor( vec2(3,1.8)/2.*SIZE - iResolution.xy/2. ), // why 3,1.8 and not 2,1?
#define P(x,y)   10.* smoothstep(1.,0.,length(U-vec2(x,y)*SIZE-iMouse.xy) )
    O.x = P(0,0)+P(1,0)+P(1,1)+P(0,1);
#endif

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Fourier transform of the input

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> buf.zw -> buf.xy -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


#define SIZE (2.*floor(iResolution.x/4.*.8)) // 256. //Size must be changed in each tab.

//#define tex(ch,x,y) texture(ch, vec2(x,y)/iResolution.xy )
#define tex(ch,x,y)  texelFetch(ch, ivec2(x,y), 0)

vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x) * vec2(cos(b),sin(b)); } 
// #define ang(a)  vec2(cos(a), sin(a))
// vec2 cmul (vec2 a,float t) { vec2 b=ang(t); return mat2(b,-b.y,b.x)*a; } 

void mainImage( out vec4 O, vec2 uv )
{
    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) return;
        
    for(float n = 0.; n < SIZE; n++)  {
        vec2 xn = tex(iChannel0, n+.5, uv.y).xy,
             yn = tex(iChannel1, uv.x, n+.5).zw,
             a = - 6.2831853 * (uv-.5 -SIZE/2.) * n/SIZE;
        
        O.zw += cmul(xn, a.x);
        O.xy += cmul(yn, a.y);
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// do your operation in spectral domain here

#define SIZE (2.*floor(iResolution.x/4.*.8)) // 256. //Size must be changed in each tab.

bool keyPress(int ascii) { return (texture(iChannel2,vec2((.5+float(ascii))/256.,0.25)).x > 0.); }
bool keyToggle(int ascii) {return (texture(iChannel2,vec2((.5+float(ascii))/256.,0.75)).x > 0.); }
float rand(vec2 uv) { return fract(1e5*sin(dot(uv,vec2(17.4,123.7)))); }
float gauss(float x) { return exp(-.5*x*x); }
#define ang(a)  vec2(cos(a), sin(a))
vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x) * vec2(cos(b),sin(b)); } 

void mainImage( out vec4 O, vec2 U )
{
    vec2 R = iResolution.xy;
    if ( U==vec2(.5)) {
        if (iFrame==0) O.zw = vec2(0,3);
        else           O.zw = texture(iChannel1, U/R).zw;  
        if ( keyPress(32) ) 
            if (iMouse.x/R.x<.5) O.z = mod(O.z+.1, 5.) ; // persistant key flag for left window
            else                 O.w = mod(O.w+.1, 5.) ; // persistant key flag for right window
        return;
    }
    if ( U==vec2(1.5,.5)) { O.w = float(keyToggle(64+19)); return; } // key S state

    vec2 T = texture(iChannel0, U / R).xy;
    U -= .5;  // freq 0 must exist
    vec2 X = 2.*U/SIZE - 1.;
    float l = length(X), s = sign(-X.x), y = iMouse.y/R.y;
    

#if 0
    // --- your custom Fourier-space filter here --------------------
    float  
       // F = 1.;                                  // 1: neutral
       // F = l*10.;                               // 2: derivative
       // F = .01/(.01+l);                         // 3: integral
       // F = gauss(l/.125)*30.;                   // 4: gaussian blur   <---
          F = smoothstep(.03,.0, abs(l-.3)) * 20.; // 5.1: ring filter   <---
       // F = smoothstep(.2,.0, abs(l-.4)) * 20.;  // 5.2: ring filter   <---
       // F = smoothstep(-.1,.1,l-y) * 20.;        // 6: kill LF (mouse tuning)
       // F = float(fract(U.x/2.)*fract(U.y/2.)>0.);   // odd  freq only
       // F = float(fract(U.x/2.)+fract(U.y/2.)==0.);  // even freq only
       // F = SIZE/length(T);                      // 7: flat modulus
                                                   // --- play with phases --------
       // F = ang(6.2832*rand(U))*4.;              // 10: white noise
       // F = gauss(l/.05)*10.;                    // 11: modulus profile : gauss
       // F = gauss(abs(l-.12)/.005)*10.;          // 12: modulus profile : ring (blue noise)
       // F = cmul(T,iTime*s);                     // 13: phase shift with time
    
    T *= F;
#else
    // --- clamp almost empty spectrum regions ---
    //if (length(T)/sqrt(SIZE)<1e-4) T = vec2(0); 
    //if (length(T)/sqrt(SIZE)<3.) T = length(T)*ang(6.2832*rand(X)); 
    
    // --- direct tuning of the spectrum -----------------------------
      // T = length(T) * vec2(1,0);                // keep modulus, kills phases
      // T = length(T) * round(normalize(T));      // keep modulus, quantize phases: 9
      // T = length(T) * sign(T);                  // keep modulus, quantize phases: 4
      // T = length(T) * vec2(sign(T.x+T.y));      // keep modulus, quantize phases: 2
      // T = length(T) * ang(6.2832*rand(X));      // keep modulus, random phases
      // T = normalize(T)*100.;                    // keep phases only, mod = 1
      // T = normalize(T) * rand(X) * 200.;        // keep phases only, random mod
         T = normalize(T) * pow(l, -1.6) * 5.;     // keep phases only, mod = l^-1.6
      // T = pow(l, -1.6) * vec2(sign(T.x+T.y)) * 5.; // quantize phase: 2 , mod = l^-1.6
#endif

	if (l==0.) T*=0.;                    // cancels DC
    O = vec4(T,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// invFourier transform 

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> buf.zw -> buf.xy -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


#define SIZE (2.*floor(iResolution.x/4.*.8)) // 256. //Size must be changed in each tab.

//#define tex(ch,x,y) texture(ch, vec2(x,y)/iResolution.xy )
#define tex(ch,x,y)  texelFetch(ch, ivec2(x,y), 0)

vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x)*vec2(cos(b), sin(b)); } 
#define W(uv)   mod(uv+SIZE/2.,SIZE)                    // wrap [-1/2,1/2] to [0,1]


void mainImage( out vec4 O, vec2 uv )
{
    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) return;

    for(float n = 0.; n < SIZE; n++)  {
        float m = W(n);       // W to warp 0,0 to mid-window.
        vec2 xn = tex(iChannel0, m+.5, uv.y).xy,
             yn = tex(iChannel1, uv.x, m+.5).zw,
             a =  6.2831853 *  (uv-.5) * n/SIZE;
        
        O.zw += cmul(xn, a.x);
        O.xy += cmul(yn, a.y);
    }
    O /= SIZE;
}