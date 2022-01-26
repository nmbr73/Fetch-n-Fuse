
//#define to_int2_f2(A) to_int2(int((A).x),int((A).y))
#define texelFetch(SAMPLER,INT2P,INT) _tex2DVecN( (SAMPLER), (INT2P).x, (INT2P).y, (INT))

// #define sampler2D ... okay, an der Stelle, wo man jetzt iChannel durchschleifen muss habe ich fuer heute aufgegeben.

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------



// standard  Main()

#define T(U) texelFetch( iChannel0, to_int2_f2(U), 0 )

__KERNEL__ void IresolutionImouseIdateEtcFuse__Buffer_A(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{

    float2 R = iResolution,
         U = ( u+u - R ) / R.y;

    O = T(U);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect '/media/a/e81e818ac76a8983d746784b423178ee9f6cdcdf7f8e8d719341a6fe2d2ab303.webm' to iChannel1
// Connect '/media/a/3c33c415862bb7964d256f4749408247da6596f2167dca2c86cc38f83c244aa6.mp3' to iChannel2
// Connect 'Keyboard' to iChannel3
// Connect '/media/a/08b42b43ae9d3c0605da11d0eac86618ea888e62cdd9518ee8b9097488b31560.png' to iChannel0


//=== original link for citation: https://www.shadertoy.com/view/llySRh
//find many other tricks here: https://shadertoyunofficial.wordpress.com/

// --- content:
// - 2D and 3D rotations
// - hue
// - printing chars, text, ints, floats
// - key togggles ( + main special codes )
// - events ( mouse just clicked, texture loaded )
// - antialiased line drawing
// - hash ( 1D, 2D, 3D, etc ).


// --- rotations -----------------------------------------------------

// 2D, or one axis after the other:
#define rot2(a)      mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)) // swi2(V,x,y) *= rot2(a.z)

// 3D rot around an arbitrary axis
#define rot(P,A,a) ( _mix( A*dot(P,A), P, _cosf(a) ) + _sinf(a)*cross(P,A) )


// --- short approx hue -------------- https://www.shadertoy.com/view/ll2cDc

#define hue(v)  ( 0.6f + 0.6f * _cosf( 6.3f*(v)  + to_float4(0,23,21,0)  ) )


// --- printing chars, integers and floats ---------------------------

// --- access to the image of ascii code c

// 2 implementations.
// Use #if 1 for extensive text use (with no superimposition nor color-appearance change)

#define IMMEDIATE_DRAW 1

#if IMMEDIATE_DRAW //  (allows for superimposition and appearance change).

__DEVICE__ float4 char_func(float2 p, int c) {
    //float2 dFdx = dFdx(p/16.0f), dFdy = dFdy(p/16.0f); ... schraeg
    float2 dFdx = p/16.0f;
    float2 dFdy = p/16.0f;
    if (p.x<.0|| p.x>1.0f || p.y<0.|| p.y>1.0f) return to_float4(0,0,0,1e5);
    //if (p.x<.25|| p.x>0.75f || p.y<0.|| p.y>1.0f) return to_float4(0,0,0,1e5); // strange bug with an old driver
  return textureGrad( iChannel0, p/16.0f + fract( to_float2(c, 15-c/16) / 16.0f ),
                        dFdx, dFdy );
  // variants:
  //float l = _log2f(length(fwidth(p/16.0f*iResolution)));
  //return textureLod( iChannel0, p/16.0f + fract( to_float2(c, 15-c/16) / 16.0f ), l);
                   // manual MIPmap to avoid border artifact. Should be textureLod, but don't available everywhere
}
#  define draw_char() to_float4(0)  // stub the final call function is used

#else // Deferred draw (call draw_char() ). Run and compiles faster.
      //     First only determine the valid char id at cur pixel
      //     then call the draw char once at the end.

int char_id = -1; float2 char_pos, dfdx, dfdy;

__DEVICE__ float4 char_func(float2 p, int c) {
    float2 dFdx = dFdx(p/16.0f), dFdy = dFdy(p/16.0f);
 // if ( p.x>.25&& p.x<0.75f && p.y>.0&& p.y<1.0f )  // normal char box
    if ( p.x>.25&& p.x<0.75f && p.y>.1&& p.y<0.85f ) // thighly y-clamped to allow dense text
        char_id = c, char_pos = p, dfdx = dFdx, dfdy = dFdy;
    return to_float4(0);
}
__DEVICE__ float4 draw_char() {
    int c = char_id; float2 p = char_pos;
    return c < 0
        ? to_float4(0,0,0,1e5)
        : textureGrad( iChannel0, p/16.0f + fract( to_float2(c, 15-c/16) / 16.0f ),
                       dfdx, dfdy );
}
#endif

// --- display int4
#if 0
__DEVICE__ float4 pInt(float2 p, float n) {  // webGL2 variant with dynamic size
    float4 v = to_float4_aw(0);
    for (int i = int(n); i>0; i/=10, p.x += 0.5f )
        v += char_func(p, 48+ i%10 );
    return v;
}
#else
__DEVICE__ float4 pInt(float2 p, float n) {
    float4 v = to_float4_aw(0);
    if (n < 0.0f)
        v += char_func(p - to_float2(-0.5f,0), 45 ),
        n = -n;

    for (float i = 3.0f; i>=0.0f; i--)
        n /=  9.999999f, // 10.0f, // for windows :-(
        v += char_func(p - 0.5f*to_float2(i,0), 48+ int(fract(n)*10.0f) );
    return v;
}
#endif

// --- display float4.4
__DEVICE__ float4 pFloat(float2 p, float n) {
    float4 v = to_float4_aw(0);
    if (n < 0.0f) v += char_func(p - to_float2(-0.5f,0), 45 ), n = -n;
    v += pInt(p,_floor(n)); p.x -= 2.0f;
    v += char_func(p, 46);      p.x -= 0.5f;
    v += pInt(p,fract(n)*1e4);
    return v;
}

// --- chars
int CAPS=0;
#define low CAPS=32;
#define caps CAPS=0;
#define spc  U.x-=0.5f;
#define C(c) spc O+= char_func(U,64+CAPS+c);
// NB: use either char.x ( pixel mask ) or char.w ( distance field + 0.5f )


// --- key toggles -----------------------------------------------------

// FYI: LEFT:37  UP:38  RIGHT:39  DOWN:40   PAGEUP:33  PAGEDOWN:34  END : 35  HOME: 36
// Modifiers: SHIFT: 16 CTRL: 17 ALT: 18
// Advice:  Mode: keyToggle(key)  Action: keydown(key)+keyclick(modifier)
#define keyToggle(ascii)  ( texelFetch(iChannel3,to_int2(ascii,2),0).x > 0.0f)
#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,1),0).x > 0.0f)
#define keyClick(ascii)   ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)

#define shift             ( texelFetch(iChannel3,to_int2(16,0),0).x  > 0.0f)
#define ctrl              ( texelFetch(iChannel3,to_int2(17,0),0).x  > 0.0f)
#define alt               ( texelFetch(iChannel3,to_int2(18,0),0).x  > 0.0f)
#define modifier          ( int(shift) +2*int(ctrl) + 4*int(alt) )


// --- events ----------------------------------------------------------

// --- mouse side events https://www.shadertoy.com/view/3dcBRS
#define mouseUp      ( iMouse.z < 0.0f )                  // mouse up even:   mouse button released (well, not just that frame)
#define mouseDown    ( iMouse.z > 0.0f && iMouse.w > 0.0f ) // mouse down even: mouse button just clicked
#define mouseClicked ( iMouse.w < 0.0f )                  // mouse clicked:   mouse button currently clicked

// --- texture loaded
#define textureLoaded(i) ( iChannelResolution[i].x > 0.0f )

// --- (re)init at resolution change or at texture (delayed) load:
// in buffX, store iResolution.x or iChannelResolution[i] somewhere. e.g. (0,0).w
// if ( currentVal != storedVal ) init; storeVal.


// --- antialiased line drawing ------ https://www.shadertoy.com/view/4dcfW8

#define S(d,r,pix) smoothstep( 0.75f, -0.75f, (d)/(pix)-(r))   // antialiased draw. r >= 1.
// segment with disc ends: seamless distance to segment
__DEVICE__ float line(float2 p, float2 a,float2 b) {
    p -= a, b -= a;
    float h = clamp(dot(p, b) / dot(b, b), 0.0f, 1.0f);   // proj coord on line
    return length(p - b * h);                         // dist to segment
}
// line segment without disc ends ( sometime useful with semi-transparency )
__DEVICE__ float line0(float2 p, float2 a,float2 b) {
    p -= a, b -= a;
    float h = dot(p, b) / dot(b, b),                  // proj coord on line
          c = clamp(h, 0.0f, 1.0f);
    return h==c ? length(p - b * h) : 1e5;            // dist to strict segment
}
    // You might directly return smoothstep( 3.0f/R.y, 0.0f, dist),
    //     but more efficient to factor all lines.
    // Indeed we can even return dot(,) and take sqrt at the end of polyline:
    // p -= b*h; return dot(p,p);


// for polylines with acute angles, see: https://www.shadertoy.com/view/fdVXRh


// --- old fashioned float-based hash. Might give user-dependant results --------------------

// nowadays integer noise is safer. see especially ||  : https://www.shadertoy.com/results?query=integer+hash+-
#define hash(p)  fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f)
#define hash2(p) fract(_sinf((p)*mat2(127.1f,311.7f, 269.5f,183.3f)) *43758.5453123f)
#define hash3(p) fract(_sinf((p)*mat3(127.1f,311.7f, 74.7f,  269.5f,183.3f,246.1f,  113.5f,271.9f,124.6f))*43758.5453123f)
#define hash2x3(p) fract(_sinf((p)*mat3x2(127.1f,311.7f,  269.5f,183.3f,  113.5f,271.9f))*43758.5453123f)


__KERNEL__ void IresolutionImouseIdateEtcFuse(float4 O, float2 uv, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, float iChannelTime[], float3 iChannelResolution[], float4 iDate, float iSampleRate, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

    O -= O;
    float2 R = iResolution, U;
    uv /= R.y;
    int lod = int(mod_f(iTime,10.0f));

    U = ( uv - to_float2(0.0f,0.9f) ) * 16.0f;  caps C(18) low C(5)C(19)C(15)C(12) caps C(-6)  // "Resol"
                             U.x-=1.0f; low C(19)C(3)C(18)C(5)C(5)C(14)               // "screen"
    U = ( uv - to_float2(0.6f,0.9f) ) * 16.0f;   low C(20)C(5)C(24)C(20)                       // "text"
    U = ( uv - to_float2(0.85f,0.9f) ) * 16.0f;  low C(12)C(15)C(4) spc C(-48+lod)             // "lod"
    U = ( uv - to_float2(1.15f,0.9f) ) * 16.0f;  low C(19)C(15)C(21)C(14)C(4)                 // "sound"

    U = ( uv - to_float2(0.0f,0.6f) ) * 16.0f;  caps C(13) low C(15)C(21)C(19)C(5) caps C(-6)  // "mouse"
    U = ( uv - to_float2(0.5f,0.6f) ) * 16.0f;  caps C(20) low C(9)C(13)C(5) caps C(-6)        // "Time"
    U = ( uv - to_float2(1.45f,0.55f) ) * 16.0f;  caps C(11) low C(5)C(25) caps C(-6)         // "Key"


    U = ( uv - to_float2(0.1f,0.8f) ) * 8.0f;        // --- column 1
    O += pInt(U, R.x);  U.y += 0.8f;   // window resolution
    O += pInt(U, R.y);  U.y += 0.8f;
    O += pFloat((U-to_float2(-1,0.35f))*1.5f, R.x/R.y);  U.y += 0.8f;
  //O += pInt(U, iResolution.z);  U.y += 0.8f;
    U.y += 0.8f;
    O += pInt(U, iMouse.x);  U.y += 0.8f;        // mouse location
    O += pInt(U, iMouse.y);  U.y += 0.8f;
    U.y += 0.4f;
    O += pInt(U, iMouse.z);  U.y += 0.8f;        // last mouse-click location
    O += pInt(U, iMouse.w);  U.y += 0.8f;

    U = ( uv - to_float2(0.5f,0.8f) ) * 8.0f;        // --- column 2

    if ( !textureLoaded(1) )                   // texture not loaded yet
        if (U.x>0.0f && U.y>-1.5f && U.x<2.5f && U.y<1.5f) O.xr= 0.5f;
    O += pInt(U, iChannelResolution[1].x);  U.y += 0.8f; // texture ( video )
    O += pInt(U, iChannelResolution[1].y);  U.y += 0.8f; // see LOD in column 2b
    //O += pInt(U, iChannelResolution[1].z);  U.y += 0.8f;
    U.y += 0.8f;

    O += pFloat(U, iTime);         U.y += 0.8f;  // time
    O += pInt(U, float(iFrame));   U.y += 0.8f;  // iFrame
    O += pFloat(U, 1.0f/iTimeDelta); U.y += 0.8f;  // FPS

    U.y += 0.8f;

    O += pInt(U, iDate.w/3600.0f);          U.x -= 2.5f;
    O += pInt(U, mod_f(iDate.w/60.0f,60.0f));   U.x -= 2.5f;
    O += pFloat(U, mod_f(iDate.w,60.0f));

    U = ( uv - to_float2(0.8f,0.8f) ) * 8.0f;        // --- column 2b

    if (textureSize(iChannel1,0).x==1 &&  iChannelResolution[1].x > 1.0f) // video/sound lock by stupid new web media policy.
        if (U.x>0.0f && U.y>-1.5f && U.x<2.5f && U.y<1.5f) O.xr= 0.5f; // Colored bg on fonts turned BW later: in immediate mode, should be defered.
                                                                // Or transform char/draw_char for they directly return BW.
    int2 S = textureSize(iChannel1,lod);
    O += pInt(U, float(S.x));  U.y += 0.8f; // texture LOD
    O += pInt(U, float(S.y));  U.y += 0.4f;
    U *= 2.0f; O += pFloat(U, iChannelTime[1]);      // iChannelTime

    U = ( uv - to_float2(0.6f,0.2f) ) * 16.0f;  caps C(8) low C(15)C(21)C(18)  // "Hour"
    U = ( uv - to_float2(0.95f,0.2f) ) * 16.0f;  caps C(13) low C(9)C(14)      // "Min"
    U = ( uv - to_float2(1.25f,0.2f) ) * 16.0f;  caps C(19) low C(5)C(3)      // "Sec"

    U = ( uv - to_float2(1.1f,0.8f) ) * 8.0f;        // --- column 3

    O += pInt(U, iChannelResolution[2].x);  U.y += 0.8f; // sound texture
    O += pInt(U, iChannelResolution[2].y);  U.y += 0.8f;
    // O += pInt(U, iChannelResolution[2].z);  U.y += 0.8f;

    O += pInt(U, iSampleRate/1e4);          U.x -= 2.0f; // iSampleRate
    O += pInt(U, mod_f(iSampleRate,1e4));

    U = ( uv - to_float2(1.4f,0.45f) ) * 8.0f;       // --- column 4

    bool b = false;
    for (int i=0; i<256; i++)
        if (keyClick(i) )  O += pInt(U, float(i)),  // keypressed ascii
                           b=true, U.y += 0.1f *8.0f;
    if (b==false) O += pInt(U, -1.0f);

    O += draw_char().xxxx;
#if IMMEDIATE_DRAW
    O = swi4(O,x,x,x,x);
#endif
   //O*=9.0f;

    // --- non-fonts stuff

    U = (uv*R.y/R-0.9f)/0.1f;
    if (_fminf(U.x,U.y)>0.0f) O = hue(U.x),  // --- hue (already in sRGB final space)
                             O*=O;      // just to fight the final sRGB convertion

    U = (uv -to_float2(0.9f*R.x/R.y,0.8f))*10.0f;              // --- line drawing
    float pix = 10.0f/R.y;               // pixel size
    O+= S( line( U,to_float2(0,0),to_float2(1.1f,0.85f)), 3.0f, pix);
    O+= S( line0(U,to_float2(0.5f,0),to_float2(1.6f,0.85f)), 3.0f, pix);

    U = (uv -0.8f*R/R.y)*10.0f;                        // --- circle, discs, transp and blend
    O += S( _fabs(length(U-to_float2(0.2f,1)) -0.5f), 1.0f, pix); // disc. -.5: relative units. 1: pixel units
    O += S( length(U-to_float2(1.1f,1)) -0.5f, 0.0f, pix) * to_float4(1,0,0,1)*0.5f; // to_float4_aw(pureCol)*opacity
    O += (1.0f-O.wa*S( length(U-to_float2(1.1f,0.3f)) -0.5f, 0.0f, pix) * to_float4(0,1,0,1); // blend below prevs
    float4 C = S( length(U-to_float2(1.1f,-0.3f)) -0.5f, 0.0f, pix) * to_float4(0,0,1,1)*0.5f;  // blend above prevs
    O = C + (1.0f-C.wa*O;

    U = uv -to_float2(0.9f*R.x/R.y,0.7f);        // --- random numbers
    if ( U.x > 0.0f && U.y > 0.0f && U.y < 0.08f )
        U.x > 0.05f*R.x/R.y ? swi3(O,x,y,z) += hash(U) : swi3(O,x,y,z) += hash2x3(U);

    // --- color space corrections
    O = _powf(O, to_float4(1.0f/2.2f) ); // shader result must be in sRGB colorspace -> gamma correction
                               // note that it is very close to _sqrtf(O).
    // similarly, color operations must be done in flat space, while textures are sRGB-encoded: _powf(texture(),to_float4_s(2.2f)) or ~square.


  SetFragmentShaderComputedColor(O);
}