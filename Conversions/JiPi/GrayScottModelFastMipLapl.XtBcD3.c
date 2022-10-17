
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0


// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

__KERNEL__ void GrayScottModelFastMipLaplFuse__Buffer_A(float4 O, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    float2 R = iResolution;
    U /= R;
    
     float4 C = T(0.0f),
         D = 4.5f* ( T(0.66f) - C );             // laplacian
    
    float dt = 2.0f,
          f = 0.01f + U.x/13.0f,
          k = 0.04f + U.y/35.0f,
          s = C.x*C.y*C.y;

    C += dt * to_float4_aw( -s + f*(1.0f-C.x) + 0.2f*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + 0.1f*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( swi2(iMouse,x,y)  -U*R ) < 10.
            ? to_float4(0.25f,0.5f,0,0)                // mouse paint
            : C;



  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

__KERNEL__ void GrayScottModelFastMipLaplFuse__Buffer_B(float4 O, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    float2 R = iResolution;
    U /= R;
    
     float4 C = T(0.0f),
         D = 4.5f* ( T(0.66f) - C );             // laplacian
    
    float dt = 2.0f,
          f = 0.01f + U.x/13.0f,
          k = 0.04f + U.y/35.0f,
          s = C.x*C.y*C.y;

    C += dt * to_float4_aw( -s + f*(1.0f-C.x) + 0.2f*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + 0.1f*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( swi2(iMouse,x,y)  -U*R ) < 10.
            ? to_float4(0.25f,0.5f,0,0)                // mouse paint
            : C;



  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

__KERNEL__ void GrayScottModelFastMipLaplFuse__Buffer_C(float4 O, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    float2 R = iResolution;
    U /= R;
    
     float4 C = T(0.0f),
         D = 4.5f* ( T(0.66f) - C );             // laplacian
    
    float dt = 2.0f,
          f = 0.01f + U.x/13.0f,
          k = 0.04f + U.y/35.0f,
          s = C.x*C.y*C.y;

    C += dt * to_float4_aw( -s + f*(1.0f-C.x) + 0.2f*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + 0.1f*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( swi2(iMouse,x,y)  -U*R ) < 10.
            ? to_float4(0.25f,0.5f,0,0)                // mouse paint
            : C;



  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

__KERNEL__ void GrayScottModelFastMipLaplFuse__Buffer_D(float4 O, float2 U, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    float2 R = iResolution;
    U /= R;
    
     float4 C = T(0.0f),
         D = 4.5f* ( T(0.66f) - C );             // laplacian
    
    float dt = 2.0f,
          f = 0.01f + U.x/13.0f,
          k = 0.04f + U.y/35.0f,
          s = C.x*C.y*C.y;

    C += dt * to_float4_aw( -s + f*(1.0f-C.x) + 0.2f*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + 0.1f*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( swi2(iMouse,x,y)  -U*R ) < 10.
            ? to_float4(0.25f,0.5f,0,0)                // mouse paint
            : C;



  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer D' to iChannel0


__KERNEL__ void GrayScottModelFastMipLaplFuse(float4 O, float2 U, sampler2D iChannel0)
{
  O = texelFetch( iChannel0, to_int2(U), 0);  O.x = 1.0f-O.x; 

  SetFragmentShaderComputedColor(O);
}