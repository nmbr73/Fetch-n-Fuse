

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(out vec4 O, vec2 U) {  O = texelFetch( iChannel0, ivec2(U), 0);  O.x = 1.-O.x; }
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

void mainImage( out vec4 O,  vec2 U )
{  
    vec2 R = iResolution.xy;
    U /= R;
    
   	vec4 C = T(0.),
         D = 4.5* ( T(.66) - C );             // laplacian
    
    float dt = 2.,
          f = .01 + U.x/13.,
          k = .04 + U.y/35.,
          s = C.x*C.y*C.y;

    C += dt * vec4( -s + f*(1.-C.x) + .2*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + .1*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( iMouse.xy  -U*R ) < 10.
            ? vec4(.25,.5,0,0)                // mouse paint
            : C;

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

void mainImage( out vec4 O,  vec2 U )
{  
    vec2 R = iResolution.xy;
    U /= R;
    
   	vec4 C = T(0.),
         D = 4.5* ( T(.66) - C );             // laplacian
    
    float dt = 2.,
          f = .01 + U.x/13.,
          k = .04 + U.y/35.,
          s = C.x*C.y*C.y;

    C += dt * vec4( -s + f*(1.-C.x) + .2*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + .1*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( iMouse.xy  -U*R ) < 10.
            ? vec4(.25,.5,0,0)                // mouse paint
            : C;

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

void mainImage( out vec4 O,  vec2 U )
{  
    vec2 R = iResolution.xy;
    U /= R;
    
   	vec4 C = T(0.),
         D = 4.5* ( T(.66) - C );             // laplacian
    
    float dt = 2.,
          f = .01 + U.x/13.,
          k = .04 + U.y/35.,
          s = C.x*C.y*C.y;

    C += dt * vec4( -s + f*(1.-C.x) + .2*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + .1*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( iMouse.xy  -U*R ) < 10.
            ? vec4(.25,.5,0,0)                // mouse paint
            : C;

}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// directly inspired from https://www.shadertoy.com/view/XtjcWK
// But using MIPmap to evaluate Laplacian

#define T(z) texture(iChannel0, U, z)

void mainImage( out vec4 O,  vec2 U )
{  
    vec2 R = iResolution.xy;
    U /= R;
    
   	vec4 C = T(0.),
         D = 4.5* ( T(.66) - C );             // laplacian
    
    float dt = 2.,
          f = .01 + U.x/13.,
          k = .04 + U.y/35.,
          s = C.x*C.y*C.y;

    C += dt * vec4( -s + f*(1.-C.x) + .2*D.x, // Gray-Scott Model + integration
                     s - (f+k)*C.y  + .1*D.y, // http://mrob.com/pub/comp/xmorphia/
                     0, 0 );

    O = length( iMouse.xy  -U*R ) < 10.
            ? vec4(.25,.5,0,0)                // mouse paint
            : C;

}