
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/e81e818ac76a8983d746784b423178ee9f6cdcdf7f8e8d719341a6fe2d2ab303.webm' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// source signal

#define SIZE (2.0f*_floor(iResolution.x/4.0f*0.8f)) // 256.0f //Size must be changed in each tab.


__KERNEL__ void FourierWorkflow2PhasesInfoFuse__Buffer_A(float4 O, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  U+=0.5f;

//  NB: out of frame image (e.g. counters & sliders ) will be transmitted to final display.
float AAAAAAAAAAAAAAA;    
    if ( _fmaxf(U.x,U.y) < SIZE )
        O.x = length( swi2(texture( iChannel0, U/SIZE ),x,z));

#if 0        // mouse controled impulse, for tests
    U += _floor( to_float2(3,1.8f)/2.0f*SIZE - iResolution/2.0f ), // why 3,1.8f and not 2,1?
#define P(x,y)   10.0f* smoothstep(1.0f,0.0f,length(U-to_float2(x,y)*SIZE-swi2(iMouse,x,y)) )
    O.x = P(0,0)+P(1,0)+P(1,1)+P(0,1);
#endif

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// Fourier transform of the input

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> swi2(buf,z,w) -> swi2(buf,x,y) -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


//#define SIZE (2.0f*_floor(iResolution.x/4.0f*0.8f)) // 256.0f //Size must be changed in each tab.

#define tex(ch,x,y) texture(ch, to_float2(x,y)/iResolution )
//#define tex(ch,x,y)  texelFetch(ch, to_int2(x,y), 0)

__DEVICE__ float2 cmul (float2 a,float b) { return mul_mat2_f2(to_mat2(a.x,a.y,-a.y,a.x) , to_float2(_cosf(b),_sinf(b))); } 
// #define ang(a)  to_float2(_cosf(a), _sinf(a))
// float2 cmul (float2 a,float t) { float2 b=ang(t); return mat2(b,-b.y,b.x)*a; } 

__KERNEL__ void FourierWorkflow2PhasesInfoFuse__Buffer_B(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  uv+=0.5f;

  O-=O; 
  
  if(uv.x > SIZE || uv.y > SIZE) return;
      
  for(float n = 0.0f; n < SIZE; n++)  {
      float2 xn = swi2(tex(iChannel0, n+0.5f, uv.y),x,y),
             yn = swi2(tex(iChannel1, uv.x, n+0.5f),z,w),
              a = - 6.2831853f * (uv-0.5f -SIZE/2.0f) * n/SIZE;
      
      swi2S(O,z,w, swi2(O,z,w) + cmul(xn, a.x));
      swi2S(O,x,y, swi2(O,x,y) + cmul(yn, a.y));
  }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Preset: Keyboard' to iChannel2
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// do your operation in spectral domain here

//#define SIZE (2.0f*_floor(iResolution.x/4.0f*0.8f)) // 256.0f //Size must be changed in each tab.

//__DEVICE__ bool keyPress(int ascii) { return (texture(iChannel2,to_float2((0.5f+float(ascii))/256.0f,0.25f)).x > 0.0f); }
//__DEVICE__ bool keyToggle(int ascii) {return (texture(iChannel2,to_float2((0.5f+float(ascii))/256.0f,0.75f)).x > 0.0f); }
__DEVICE__ float rand(float2 uv) { return fract(1e5*_sinf(dot(uv,to_float2(17.4f,123.7f)))); }
__DEVICE__ float gauss(float x) { return _expf(-0.5f*x*x); }
#define ang(a)  to_float2(_cosf(a), _sinf(a))
//__DEVICE__ float2 cmul (float2 a,float b) { return mul_mat2_f2(to_mat2(a,-a.y,a.x) , to_float2(_cosf(b),_sinf(b))); } 

__KERNEL__ void FourierWorkflow2PhasesInfoFuse__Buffer_C(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  CONNECT_CHECKBOX0(keyPress32, 0);
  CONNECT_CHECKBOX1(keyToggle, 0);

float CCCCCCCCCCCCCCCC;
  U+=0.5f;

  float2 R = iResolution;
  //if ( U==to_float2_s(0.5f)) {
    if (U.x==0.5f && U.y == 0.5f) {
      if (iFrame==0) O.z = 0.0f, O.w = 3.0f; //swi2(O,z,w) = to_float2(0,3);
      else           O.z = texture(iChannel1, U/R).z, O.w = texture(iChannel1, U/R).w; //swi2(O,z,w) = texture(iChannel1, U/R).zw;  
      if ( keyPress32 ) 
          if (iMouse.x/R.x<0.5f) O.z = mod_f(O.z+0.1f, 5.0f) ; // persistant key flag for left window
          else                   O.w = mod_f(O.w+0.1f, 5.0f) ; // persistant key flag for right window
          
      SetFragmentShaderComputedColor(O);    
      return;
  }
  //if ( U==to_float2(1.5f,0.5f)) { 
  if ( U.x==1.5f && U.y==0.5f ) { 
    O.w = (float)(keyToggle); 
    
    SetFragmentShaderComputedColor(O);
    return; 
  } // key S state

  float2 T = swi2(texture(iChannel0, U / R),x,y);
  U -= 0.5f;  // freq 0 must exist
  float2 X = 2.0f*U/SIZE - 1.0f;
  float l = length(X), s = sign_f(-X.x), y = iMouse.y/R.y;
   

#if 0
    // --- your custom Fourier-space filter here --------------------
    float  
       // F = 1.0f;                                // 1: neutral
       // F = l*10.0f;                             // 2: derivative
       // F = 0.01f/(0.01f+l);                     // 3: integral
       // F = gauss(l/0.125f)*30.0f;               // 4: gaussian blur   <---
          F = smoothstep(0.03f,0.0f, _fabs(l-0.3f)) * 20.0f; // 5.1: ring filter   <---
       // F = smoothstep(0.2f,0.0f, _fabs(l-0.4f)) * 20.0f;  // 5.2: ring filter   <---
       // F = smoothstep(-0.1f,0.1f,l-y) * 20.0f;  // 6: kill LF (mouse tuning)
       // F = float(fract(U.x/2.0f)*fract(U.y/2.0f)>0.0f);   // odd  freq only
       // F = float(fract(U.x/2.0f)+fract(U.y/2.0f)==0.0f);  // even freq only
       // F = SIZE/length(T);                      // 7: flat modulus
                                                   // --- play with phases --------
       // F = ang(6.2832f*rand(U))*4.0f;           // 10: white noise
       // F = gauss(l/0.05f)*10.0f;                // 11: modulus profile : gauss
       // F = gauss(_fabs(l-0.12f)/0.005f)*10.0f;  // 12: modulus profile : ring (blue noise)
       // F = cmul(T,iTime*s);                     // 13: phase shift with time
    
    T *= F;
#else
    // --- clamp almost empty spectrum regions ---
    //if (length(T)/_sqrtf(SIZE)<1e-4) T = to_float2(0); 
    //if (length(T)/_sqrtf(SIZE)<3.0f) T = length(T)*ang(6.2832f*rand(X)); 
    
    // --- direct tuning of the spectrum -----------------------------
      // T = length(T) * to_float2(1,0);             // keep modulus, kills phases
      // T = length(T) * round(normalize(T));        // keep modulus, quantize phases: 9
      // T = length(T) * sign(T);                    // keep modulus, quantize phases: 4
      // T = length(T) * to_float2(sign(T.x+T.y));   // keep modulus, quantize phases: 2
      // T = length(T) * ang(6.2832f*rand(X));       // keep modulus, random phases
      // T = normalize(T)*100.0f;                    // keep phases only, mod = 1
      // T = normalize(T) * rand(X) * 200.0f;        // keep phases only, random mod
         T = normalize(T) * _powf(l, -1.6f) * 5.0f;  // keep phases only, mod = l^-1.6
      // T = _powf(l, -1.6f) * to_float2(sign(T.x+T.y)) * 5.0f; // quantize phase: 2 , mod = l^-1.6
#endif

  if (l==0.0f) T*=0.0f;                    // cancels DC
    O = to_float4(T.x,T.y,0,0);

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// invFourier transform 

// Horizontal + Vertical Discrete Fourier Transform of the input 
// 2 passes pipelined : in -> swi2(buf,z,w) -> swi2(buf,x,y) -> out
// ( adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS# )


//#define SIZE (2.0f*_floor(iResolution.x/4.0f*0.8f)) // 256.0f //Size must be changed in each tab.

//#define tex(ch,x,y) texture(ch, to_float2(x,y)/iResolution )
//#define tex(ch,x,y)  texelFetch(ch, to_int2(x,y), 0)

//__DEVICE__ float2 cmul (float2 a,float b) { return mul_mat2_f2(to_mat2(a,-a.y,a.x) , to_float2(_cosf(b), _sinf(b))); } 
#define W(uv)   mod_f(uv+SIZE/2.0f,SIZE)                    // wrap [-1/2,1/2] to [0,1]


__KERNEL__ void FourierWorkflow2PhasesInfoFuse__Buffer_D(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  uv+=0.5f;

  O-=O; 
float DDDDDDDDDDDDDDDD;  
  if(uv.x > SIZE || uv.y > SIZE) return;

  for(float n = 0.0f; n < SIZE; n++)  {
      float m = W(n);       // W to warp 0,0 to mid-window.
      float2 xn = swi2(tex(iChannel0, m+0.5f, uv.y),x,y),
             yn = swi2(tex(iChannel1, uv.x, m+0.5f),z,w),
              a =  6.2831853f *  (uv-0.5f) * n/SIZE;
      
      swi2S(O,z,w, swi2(O,z,w) + cmul(xn, a.x));
      swi2S(O,x,y, swi2(O,x,y) + cmul(yn, a.y));
  }
  O /= SIZE;


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel3
// Connect Image 'Previsualization: Buffer D' to iChannel2


// set your module and phase in Buf C

//#define SIZE (2.0f*_floor(iResolution.x/4.0f*0.8f)) // 256.0f //Size must be changed in each tab.


//Display modes
#define MAGNITUDE 0.0f
#define PHASE     1.0f
#define COMPONENT 2.0f
#define _REAL      3.0f
#define IMAG      4.0f

//Scaling
#define LOG 0
#define LINEAR 1

#define MAG_SCALE LOG

__DEVICE__ float4 rainbow(float x)  { return 0.5f + 0.5f * cos_f4(6.2832f*(to_float4_s(x) - to_float4(0,1,2,0)/3.0f)); }
__DEVICE__ float4 rainbow(float2 C)   { return rainbow(_atan2f(C.y,C.x)/3.1416f + 0.5f); }

__DEVICE__ float4 paintDFT(float2 F, float mode) {
    // F /= SIZE;
    return 
         mode == MAGNITUDE 
     #if   MAG_SCALE == LOG
                           ?  to_float4_s(_logf(length(F)))
     #elif MAG_SCALE == LINEAR
                           ?  to_float4_s(length(F))
     #endif
       : mode == PHASE     ?  rainbow(F)        
       : mode == COMPONENT ?  0.5f+0.5f*to_float4(F.x, F.y, 0,0)
       : mode == _REAL      ?  0.5f+0.5f*to_float4_s(F.x)
       : mode == IMAG      ?  0.5f+0.5f*to_float4_s(F.y)
       : to_float4_s(-1); // error
}

__KERNEL__ void FourierWorkflow2PhasesInfoFuse(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    float2 R = iResolution, U=uv;
    //O = texture(iChannel0,uv/R).xxxx; return;
float IIIIIIIIIIIIIIIIIIII;        
    float2 pixel = ( uv - iResolution/2.0f) / SIZE  + to_float2(2,1)/2.0f,
           tile  = _floor(pixel),
           stile = _floor(mod_f(2.0f*pixel,2.0f));
           uv = fract_f2(pixel) * SIZE / R ;

    O-=O;
    
    float2 DISPLAY_MODE = _floor(swi2(texture(iChannel3, 0.5f/R),z,w)); // persistant key flag.
    bool INSET =  texture(iChannel3,to_float2(1.5f,0.5f)/R).w == 0.0f;
    
    if (tile.y==-1.0f && _fabs(tile.x-0.5f)<1.0f) {                // buttons displaying current flags value
        for (float i=0.0f; i<5.0f; i+=1.0f) 
            O += smoothstep(0.005f,0.0f,_fabs(length(uv*R/SIZE-to_float2(0.2f+i/7.0f,0.97f))-0.025f));
        float v = tile.x==0.0f ? DISPLAY_MODE.x : DISPLAY_MODE.y;
        O.z += smoothstep(0.03f,0.02f,length(uv*R/SIZE-to_float2(0.2f+v/7.0f,0.97f)));
    }
    
    //if(tile == to_float2(0,0))                                             // --- Input + DFT (Left)
      if(tile.x == 0.0f && tile.y == 0.0f)                                             // --- Input + DFT (Left)
        if (stile.x == 0.0f && stile.y == 0.0f && INSET )
             O += paintDFT(swi2(texture(iChannel1, 2.0f*uv),x,y), DISPLAY_MODE.x); // initial spectrum
        else O += _tex2DVecN(iChannel0,uv.x,uv.y,15).x;                                // initial texture
      //else O += length(_tex2DVecN(iChannel0,uv.x,uv.y,15).rgb);

    //if(tile == to_float2(1,0)) 
      if(tile.x == 1.0f && tile.y == 0.0f)      // --- Output +DFT (Right)
        if (stile.x == 0.0f && stile.y == 0.0f && INSET)
             O += paintDFT(swi2(texture(iChannel3, 2.0f*uv),x,y), DISPLAY_MODE.y); // initial Fourier
        else O += paintDFT(swi2(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y), DISPLAY_MODE.y);    // final texture
    //  else O += paintDFT(texture(iChannel2, fract(0.5f+uv*R/SIZE)*SIZE/R).xy, DISPLAY_MODE[1]); // fftshift
        
    if(tile.y>0.0f) O += texture(iChannel0, U/iResolution).x; // displayed opt values in top margin   
    //if (tile != mod_f(tile,to_float2(2,1))) O+=0.3f;    
    if (tile.x != mod_f(tile.x,2.0f) || tile.y != mod_f(tile.y,1.0f)) O+=0.3f;    

  O.w = 1.0f;

  SetFragmentShaderComputedColor(O);
}