
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R      iResolution
#define T(U)   texture( iChannel0, (make_float2(to_int2_cfloat(U))+0.5f)/R )

#define N 2.0f   // N² undo levels

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// === distort the uv field ====================

//#define keyDown(a) ( texelFetch(iChannel3,to_int2(a,0),0).x > 0.0f)

#define ortho(D)     to_float2( -(D).y, (D).x )              // vec ortho to D
//#define S(D)       ortho(D) / _fmaxf( dot(D,D) , 1e-3 )  // irrotational swirl
  #define S(D)       ortho(D) / _fmaxf( length(D)*smoothstep(0.0f,3.0f,length(D)) , 1e-3 )  / 6e4 // irrotational swirl
//#define S(D)       ortho(D) * smoothstep(1.0f,0.0f,length(D)/0.25f) *0.1f

__DEVICE__ float f(float _x) {
    _x = _fabs(_x)/0.2f; 
  return _x < 1.0f ? - 0.02f * smoothstep(1.0f,0.0f,_x) : 0.0f;
}

__KERNEL__ void PaintWithDeformationsFuse__Buffer_A(float4 O, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_BUTTON0(Modus, 0, Left, Right, Grow, Shrink,  Undo);
    //                        UP    Down    Left  Right  Undo
    u+=0.5f;

    if ( u.x == 0.5f && u.y == 0.5f )                                 // previous mouse pos
      { O = to_float4_f2f2(swi2(iMouse,x,y)*sign_f(iMouse.z), swi2(T(to_float2_s(0.0f)),x,y)); SetFragmentShaderComputedColor(O); return; }
      
    float2 U = u / R.y, V,
         M =   swi2(iMouse,x,y) / R.y,
         d = ( swi2(iMouse,x,y) - swi2(T(to_float2_s(0.0f)),z,w) ) / R.y;             // mouse drag direction
    if (iMouse.z > 0.0f)  {                               // if click
       
       float s = Modus==1 ? -1.0f : Modus==2 ? 1.0f      // key pressed
               : Modus==3 ? 2.0f  : Modus==4 ? -2.0f : 0.0f, l;
               
       if ( _fabs(s) == 1.0f )                        // --- left or right key : swirl
           U +=  S(U-M)  * sign_f(s);                 // swirl field
       else  if ( _fabs(s) == 2.0f )                  // up or bottom key : grow/shrink
           l = length(U-M),
           U = M + (U-M) * ( 1.0f + f(l) * sign_f(s) ); 
       else if ( T(to_float2_s(0.0f)).z > 0.0f )                           // --- drag & no key: displace
           U -= 0.5f*d *smoothstep(0.2f,0.0f, length(U-M) );  // displacement field
    }
    O = texture( iChannel0, U *R.y/R ); 
    
    if (Modus==5) O = texture( iChannel1, (N-1.0f + U *R.y/R ) / N ); 
    
    if ( iFrame < 1 || Reset ) O = to_float4(u.x/R.x,u.y/R.y,0,0);                // init uv field


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// === undo buffer 
// stores N² frames: if click, or every second if mouse drag

__KERNEL__ void PaintWithDeformationsFuse__Buffer_B(float4 O, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{  
    U+=0.5f;

    if (iMouse.w <= 0.0f && ( iMouse.z<=0.0f || iFrame%60>0 ))  { O = T(U);  SetFragmentShaderComputedColor(O); return; }
    U *= N/R;
    float2 I = _floor(U), F = fract_f2(U);
    if (I.x>0.0f) I.x--; else I.x = N-1.0f, I.y--;
    O = I.y < 0.0f ? _tex2DVecN(iChannel1,F.x,F.y,15)
                   : texture(iChannel0,(I+F)/N);

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2

__DEVICE__ float _fwidth(float inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}



// variant of   swirl2 https://shadertoy.com/view/7l2cRz
//            + displacement https://www.shadertoy.com/view/sdjyRV

// === applies the uv field to a texture =================

//#define fwidth(v) length(to_float2(dFdx(v),dFdy(v)))

__KERNEL__ void PaintWithDeformationsFuse(float4 O, float2 u, float2 iResolution, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 1.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_SLIDER0(Par, -10.0f, 10.0f, 1.0f);
    CONNECT_CHECKBOX2(OvalOff, 0);
    CONNECT_SLIDER1(OvalSize, -1.0f, 2.0f, 0.3f);
  
    u+=0.5f;
  
    u = swi2(T(u),x,y);                       // distorted uv field
    O = _tex2DVecN( iChannel1,u.x,u.y,15);    // apply map
    
    float v = length(u-0.5f);         // apply circle
    //O = _mix( O, to_float4(1,0,0,0), smoothstep( 1.5f, 0.0f,  _fabs(v-0.3f)/_fwidth(v, R, Par) ) );
    if (!OvalOff) O = _mix( O, Color, smoothstep( 1.5f, 0.0f,  _fabs(v-OvalSize)/_fwidth(v, R, Par) ) );

    O.w=Color.w;

  SetFragmentShaderComputedColor(O);
}