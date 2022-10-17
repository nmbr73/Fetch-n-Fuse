
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define A texture(iChannel0,(u+i)/R)

//#define M void mainImage(out float4 r, float2 u) { float2 x, i = u-u, f = i; r -= r
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

// r.w -> >1.0 ist Flüssigkeit und Druck 
// r.z -> 0.0 ist linke Flüssigkeit
// r.z -> ~ 1.0 ist rechte Flüssigkeit

// r.x, r.y Geschwindigkeit


__KERNEL__ void TinypaintstreamsJipiFuse__Buffer_A(float4 r, float2 u, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(MasterReset, 0);  
    CONNECT_CHECKBOX1(Reset, 0);

    CONNECT_POINT0(LeftSourceXY, 0.0f, 0.0f );
    CONNECT_POINT1(RightSourceXY, 0.0f, 0.0f );
    
    //CONNECT_SLIDER0(LeftGrav, -10.0f, 10.0f, 0.0f);
    //CONNECT_SLIDER1(RightGrav, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT2(LeftGravXY, 0.0f, 0.0f );
    CONNECT_POINT3(RightGravXY, 0.0f, 0.0f );
    
    
    CONNECT_SLIDER2(Gravity, -10.0f, 10.0f, 0.0f);
    

    u+=0.5f;

    float2 x, i = u-u, f = i; r -= r; float G, m = A.w;
    // iterate over neighbouring cells
    // swi2(A,x,y) = velocity
    // A.z = pigment
    // A.w = density
    for(int k = 81; k-->0;)
        i = to_float2(k%9,k/9)-4.0f,
        x = i + swi2(A,x,y), // move particle according to velocity

        // Gaussian diffusion, sigma=_sqrtf(0.5f)
        G = A.w / _expf(dot(x,x)) / 3.142f,

        // advection
        r += to_float4_aw(swi3(A,x,y,z), 1) * G,
    
        // pressure forces (smoothed particle hydrodynamics)
        f -= ( m*m-m       // pressure at current position
                           // density * (density - reference fluid density)
             + A.w*A.w-A.w // pressure at neighbour position
             ) * G * x;    // gradient of smoothing kernel

    if(r.w > 0.0f) // not vacuum
    {
        //swi3(r,x,y,z) /= r.w, // convert momentum to velocity, normalise pigment
        r.x /= r.w; // convert momentum to velocity, normalise pigment
        r.y /= r.w; // convert momentum to velocity, normalise pigment
        r.z /= r.w; // convert momentum to velocity, normalise pigment
        swi2S(r,x,y, swi2(r,x,y) + clamp(f / r.w, -0.1f, 0.1f)); // acceleration
    }
    // gravity
    r.y -= 0.005f+Gravity;

    // boundary
    if(u.y < 9.0f) r.y += 1.0f;
    
    // streams
    r = length(u - R * (to_float2(0.2f, 0.9f)+LeftSourceXY)) < 9.0f ?
            to_float4(_sinf(iTime) + 2.0f+LeftGravXY.x, -1+LeftGravXY.y, 0, 1) :          //Linke Flüssigkeit
        length(u - R * (to_float2(0.8f, 0.9f)+RightSourceXY)) < 9.0f ?
            to_float4(_sinf(iTime) - 2.0f+RightGravXY.x, -1+RightGravXY.y, 1, 1) :        //Rechte Flüssigkeit
        (iFrame < 2 || Reset) && u.y > 0.8f*R.y ? // init
            to_float4(0,0, u.x < R.x/2.0f, 1) :
        r;

  if(MasterReset)
    r = to_float4_s(0.0f);
                
  SetFragmentShaderComputedColor(r);        
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__KERNEL__ void TinypaintstreamsJipiFuse(float4 r, float2 u, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0)
{
  
  CONNECT_CHECKBOX2(ManualColor, 0);
  CONNECT_CHECKBOX3(PreMul, 0);
  CONNECT_COLOR0(ColorLeft,  1.0f, 1.0f, 0.0f, 1.0f);
  CONNECT_COLOR1(ColorRight, 1.0f, 0.0f, 1.0f, 1.0f);
  
  u+=0.5f;  

  float2 x, i = u-u, f = i; r -= r - 1.0f + A.w * to_float4(0, A.z, 1.0f - A.z, 0);  //Original

  if(ManualColor)
  {
    //float4 Left  = (A.z == 0.0f) ? A.w * ColorLeft : to_float4_s(0.0f);
    //float4 Right = (A.z == 1.0f) ? A.w * ColorRight: to_float4_s(0.0f);
    float4 Left  = (1.0f-A.z  ) * A.w * (to_float4_s(1.0f)-ColorLeft);
    float4 Right =  A.z         * A.w * (to_float4_s(1.0f)-ColorRight);
    r = to_float4_s(1.0f) - _mix(Left,Right,0.5f)*2.0f;
    r.w = ColorLeft.w;
  }

  if(PreMul)
    r *= A.w;

  SetFragmentShaderComputedColor(r);        
}