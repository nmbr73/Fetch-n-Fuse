
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// creation of the input

#define SIZE 256.0f //Size must be changed in each tab.

__KERNEL__ void FourierWorkflowFuse__Buffer_A(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0)
{

    uv+=0.5f; 

    O-=O; 
    
    if(uv.x > SIZE || uv.y > SIZE) 
    { 
      SetFragmentShaderComputedColor(O);
      return;
    }
    
    float2 R = iResolution;
    O.x = length( swi3(texture(iChannel0, uv/R* R.y/SIZE),x,y,z));//.rgb );
    // O.y = ...

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


//#define SIZE 256.0f //Size must be changed in each tab.

__DEVICE__ float2 cmul (float2 a,float b) { return mul_mat2_f2(to_mat2(a.x,a.y,-a.y,a.x) , to_float2(_cosf(b),_sinf(b))); } 
// #define ang(a)  to_float2(_cosf(a), _sinf(a))
// float2 cmul (float2 a,float t) { float2 b=ang(t); return mat2(b,-b.y,b.x)*a; } 

__KERNEL__ void FourierWorkflowFuse__Buffer_B(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    uv+=0.5f; 
    
    O-=O; 
float BBBBBBBBBBBBBBBBB;    
    if(uv.x > SIZE || uv.y > SIZE) 
    {
      SetFragmentShaderComputedColor(O);
      
      return;
    }
        
    for(float n = 0.0f; n < SIZE; n++)  {
        float2 xn = swi2(texture(iChannel0, to_float2(n+0.5f, uv.y) / iResolution),x,y),
               yn = swi2(texture(iChannel1, to_float2(uv.x, n+0.5f) / iResolution),z,w),
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

//#define SIZE 256.0f //Size must be changed in each tab.

//__DEVICE__ bool keyPress(int ascii) { return (texture(iChannel2,to_float2((0.5f+float(ascii))/256.0f,0.25f)).x > 0.0f); }
__DEVICE__ float rand(float2 uv) { return fract(1e5*_sinf(dot(uv,to_float2(17.4f,123.7f)))); }
__DEVICE__ float gauss(float x) { float qqqqqqqqqqqqqq; return _expf(-0.5f*x*x); }
#define ang(a)  to_float2(_cosf(a), _sinf(a))
//__DEVICE__ float2 cmul (float2 a,float b) { return mat2(a,-a.y,a.x) * to_float2(_cosf(b),_sinf(b)); } 

__KERNEL__ void FourierWorkflowFuse__Buffer_C(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_CHECKBOX0(keyPress, 0);

    U+=0.5f; 

    float2 T = swi2(texture(iChannel0, U / iResolution),x,y);
    U -= 0.5f;
    float2 X = 2.0f*U/SIZE - 1.0f;
    float l = length(X), s = sign_f(-X.x), y = iMouse.y/iResolution.y;
float CCCCCCCCCCCCCCCCCCCC;    
# if 1        
    // --- your custom Fourier-space filter here --------------------
    float    
       // F = l*10.0f;                     // derivative
       // F = 0.01f/(0.01f+l);               // integral
       // F = gauss(l/0.125f);             // gaussian blur
          F = smoothstep(0.3f,0.0f, _fabs(l-0.5f)) * 20.0f; // ring filter
       // F = smoothstep(-0.1f,0.1f,l-y) * 20.0f; // kill LF (mouse tuning)
       // F = float(fract(U.x/2.0f)*fract(U.y/2.0f)>0.0f);   // odd  freq only
       // F = float(fract(U.x/2.0f)+fract(U.y/2.0f)==0.0f);  // even freq only

    T *= F;
    O = to_float4(T.x,T.y,F,F);
    
# else
    // --- or, your custom Fourier-space function here ------------
    //            see also https://www.shadertoy.com/view/4dGGz1
       T = ang(6.2832f*rand(U));                // white noise
    // T *= gauss(l/0.05f)*10.0f;                  // modulus profile : gauss
       T *= gauss(_fabs(l-0.12f)/0.005f)*10.0f;        // modulus profile : ring (blue noise)
       T = cmul(T,iTime*s);              // phase shift with time
    
    T *= SIZE;
             
    O = to_float4_f2f2(T,T);
    
#endif
    
    //if ( U==to_float2_s(0)) {
      if ( U.x==0.0f&U.y==0.0f ) {
        O.w = texture(iChannel1,U/iResolution).w;
        if ( keyPress )  O.w = mod_f(O.w+0.1f, 3.0f) ; // persistant key flag
    }


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


//#define SIZE 256.0f //Size must be changed in each tab.

//__DEVICE__ float2 cmul (float2 a,float b) { return to_mat2(a,-a.y,a.x)*to_float2(_cosf(b), _sinf(b)); } 
#define W(uv)   mod_f(uv+SIZE/2.0f,SIZE)                    // wrap [-1/2,1/2] to [0,1]


__KERNEL__ void FourierWorkflowFuse__Buffer_D(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    uv+=0.5f; 

    O-=O; 
float DDDDDDDDDDDDDDDDD;    
    if(uv.x > SIZE || uv.y > SIZE) 
    {
      SetFragmentShaderComputedColor(O);  
      return;
    }

    for(float n = 0.0f; n < SIZE; n++)  {
        float m = W(n);       // W to warp 0,0 to mid-window.
        float2 xn = swi2(texture(iChannel0, to_float2(m+0.5f, uv.y) / iResolution),x,y),
               yn = swi2(texture(iChannel1, to_float2(uv.x, m+0.5f) / iResolution),z,w),
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


// adapted from  Flyguy's https://www.shadertoy.com/view/MscGWS#
// see also https://www.shadertoy.com/view/4dGGz1 to play with spectrum.
// NB: see new version here: https://www.shadertoy.com/view/XtScWt <<<<

// set your module and phase in Buf A

#define SIZE 256.0f //Size must be changed in each tab.

//Display modes
#define MAGNITUDE 0.0f
#define PHASE     1.0f
#define COMPONENT 2.0f

__DEVICE__ float DISPLAY_MODE = MAGNITUDE;

//Scaling
#define LOG 0
#define LINEAR 1

#define MAG_SCALE LOG

__DEVICE__ float4 rainbow(float x)    { return 0.5f + 0.5f * cos_f4(6.2832f*(to_float4_s(x) - to_float4(0,1,2,0)/3.0f)); }
__DEVICE__ float4 rainbow(float2 C)   { return rainbow(_atan2f(C.y,C.x)/3.1416f + 0.5f); }

__DEVICE__ float4 paintDFT(float2 F) {
  if (DISPLAY_MODE == MAGNITUDE)
     #if MAG_SCALE == LOG
        return to_float4_s( _logf(length(F)) / _logf(SIZE*SIZE) );
     #elif MAG_SCALE == LINEAR
        return to_float4_s( length(F) / SIZE );
     #endif

    else if ( DISPLAY_MODE == PHASE )     return rainbow(F);        
    else /* if ( DISPLAY_MODE == COMPONENT ) */ return to_float4(0.5f + 0.5f*F.x/SIZE,0.5f + 0.5f*F.y/SIZE, 0,0);        
}

__DEVICE__ float message(float2 p) {  // the alert function to add to your shader
float rrrrrrrrrrrrrrrrrrr;    
    int x = (int)(p.x+1.0f)-1, y=(int)(p.y)-10,  i;
    if (x<1||x>32||y<0||y>2) return -1.0f; 
    i = ( y==2? i=  757737252: y==1? i= 1869043565: y==0? 623593060: 0 )/ (int)(_exp2f((float)(32-x)));
   return i==2*(i/2) ? 1.0f : 0.0f;
}


__KERNEL__ void FourierWorkflowFuse(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    uv+=0.5f; 

    float2 R = iResolution;
    if (iResolution.y<200.0f) // alert for the icon
    {   
      float c=message(uv/8.0f); 
      if(c>=0.0f)
      { 
        O=to_float4(c,0,0,0);

        SetFragmentShaderComputedColor(O);
        return; 
      } 
    }
float IIIIIIIIIIIIIIIIII;        
    float2 pixel = ( uv - iResolution/2.0f) / SIZE  + to_float2(2,1)/2.0f,
           tile  = _floor(pixel),
           stile = _floor(mod_f2(2.0f*pixel,2.0f));
           uv = fract_f2(pixel) * SIZE / R ;

    O-=O;
    
    DISPLAY_MODE = _floor(texture(iChannel3, 0.5f/R).w); // persistant key flag.
    if (tile.y==-1.0f && _fabs(tile.x-0.5f)<1.0f) {   // buttons displaying current flags value
        for (float i=0.0f; i<3.0f; i++) 
            O += smoothstep(0.005f,0.0f,_fabs(length(uv*R/SIZE-to_float2(0.2f+i/7.0f,0.97f))-0.025f));
        float v = DISPLAY_MODE;
        O.z += smoothstep(0.03f,0.02f,length(uv*R/SIZE-to_float2(0.2f+v/7.0f,0.97f)));
    }
    
    //if(tile == to_float2(0,0))  //Input + DFT (Left)
      if(tile.x == 0.0f && tile.y == 0.0f)  //Input + DFT (Left)
        //if (stile == to_float2_s(0) )
        if (stile.x == 0.0f && stile.y == 0.0f)
             O += paintDFT(swi2(texture(iChannel1, 2.0f*uv),x,y));
        else O += length(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z));//.rgb);

    //if(tile == to_float2(1,0))  // Output +DFT (Right)
      if(tile.x == 1.0f && tile.y == 0.0f)  // Output +DFT (Right)
        //if (stile == to_float2_s(0) )
          if (stile.x == 0.0f && stile.y == 0.0f)
             O += paintDFT(swi2(texture(iChannel3, 2.0f*uv),x,y));
        else 
            O += 0.5f+0.5f*_tex2DVecN(iChannel2,uv.x,uv.y,15).x;
          //O += length(_tex2DVecN(iChannel2,uv.x,uv.y,15).xy);

  SetFragmentShaderComputedColor(O);
}
