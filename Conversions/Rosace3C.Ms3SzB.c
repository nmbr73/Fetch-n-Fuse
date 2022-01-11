
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// inspired from Shane's ribbon variant of https://www.shadertoy.com/view/ls3XWM



__KERNEL__ void Rosace3CFuse(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(Random,false);
    CONNECT_SLIDER0(Seed,0.0,1.0,0.0);

    // iDate.w

    float h = iResolution.y;  U = 4.0f*(U+swi2(iMouse,x,y))/h;                    // normalized coordinates
    float2 K = _ceil(U); U = 2.0f*fract(U)-1.0f;  // or K = 1.0f+2.0f*_floor(U) to avoid non-fractionals
    float a = _atan2f(U.y,U.x), r=length(U), v=0.0f, A;                       // polar coordinates

    for(int i=0; i<7; i++)
        // if fractional, there is K.y turns to close the loop via K.x wings.
        v = _fmaxf(v,   ( 1.0f + 0.8f* _cosf(A= K.x/K.y*a + iTime) ) / 1.8f  // 1+_cosf(A) = depth-shading
                   * smoothstep(1.0f, 1.0f-120.0f/h, 8.0f*_fabs(r-0.2f*_sinf(A)-0.5f))), // ribbon (antialiased)
        a += 6.28f;                                                       // next turn

    if (!Random)
    {  O = v*to_float4(0.8f,1,0.3f,1); O.y= _sqrtf(O.y);                              // greenify
    } else {
      O = v*(0.5f+0.5f*_sinf(K.x+17.0f*K.y+Seed+to_float4(0,2.1f,-2.1f,0)));           // random colors
    }
    SetFragmentShaderComputedColor(O);

}
