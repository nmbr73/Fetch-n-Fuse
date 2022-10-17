

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of https://shadertoy.com/view/tsdGRM

void mainImage( out vec4 O, vec2 U )
{
    O = sqrt( T(U) );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float E = 100.,              // attractive intensity (in normalized coordinates)
     dt = 1./600.,           // time step
     scale = 8.,             // random scale (larger=smoother)
     spread = 1.;            // source spread ( 1e-5... 1 )

vec2 F(vec4 O) {             // --- Force ( O.xy = pos )
    return E * (2.*texture(iChannel1,O.xy/scale).xy-1.); // random force field
 // gravity: F = C - O.xy; float d = length(F); return E * F / (d*d*d);
}
vec4 physics(vec4 O) {       // --- simple Newton step
    O.zw += F(O) * dt;       // velocity
    O.xy += O.zw * dt;       // location
    return O;
}
void mainImage( out vec4 O, vec2 u ) // --- simulates trajectories
{
    O = T(u);
    
    vec2 U = ( 2.*u - R ) / R.y;
    if (iMouse.y > 0.) scale = exp2(7.*iMouse.y/R.y);
    
    if ( T(2).xy != R ) {
        O -= O;
        if (u.x<2.) O = vec4(-.5,spread*U.y, 3.,0.);// init positions .xy and velocities .zw
      //else if (u.x<2.) O = vec4(U,0,0); 
        else if (u==vec2(2.5)) O.xy = R;     // for testing window size change
        return;
    }
    if (u==vec2(2.5)) return;
    if ( u.x > 3. && iMouse.z > 0. ) O -= O; // click to clear screen
    
    if (u.x==.5) {                           // ---simulates physics
        dt /= 2.; vec4 P = physics(O); dt *= 2.; // https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
        O.xy += P.zw * dt;
        O.zw =  P.zw + F(O)*dt/2.;
                                             // --- wrap
        if (O.y > 1.) O.y -= 2.; else if (O.y < -1.) O.y += 2.;
            float r = R.x/R.y;
        if (O.x > r) O.x -= 2.*r; else if (O.x < -r) O.x += 2.*r;
    }
    else if (u.x==1.5)                       // prev position
        O = T(vec2(.5,u.y));
    else {                                   // --- draw rays
        O *= 1. - 7.*dt;                     // fade past
        float d = 1e5, i_, i=0., v;
        for (; i < R.y; i += 1. ) {
            vec2 P0 = T(vec2(1,i)).xy, P1 = T(vec2(0,i)).xy, L=P1-P0;
            if (dot(L,L)<.25) {
                v = line( U, P0, P1);
                if ( v < d ) d = v, i_ = i;
            }
        }
        O += S( sqrt(d) -0./R.y ) * hue(i_/R.y);
    }
    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define T(U) texelFetch( iChannel0, ivec2(U), 0 )
#define R   (iResolution.xy)

// utils from https://www.shadertoy.com/view/llySRh

float line(vec2 p, vec2 a,vec2 b) { 
    p -= a, b -= a;
    float h = dot(p, b) / dot(b, b),                  // proj coord on line
          c = clamp(h, 0., 1.);
    p -= b * h; return h==c ? dot(p,p): 1e5;          // dist² to segment
}

#define S(v) smoothstep( 3./R.y, 0., v)

#define hue(v)  ( .6 + .6 * cos( 6.3*(v)  + vec4(0,23,21,0)  ) )
