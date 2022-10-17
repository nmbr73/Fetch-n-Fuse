
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R   (iResolution)
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//#define T(U) texelFetch( iChannel0, to_int2(U), 0 )

#define T(U) texture(iChannel0, (make_float2(to_int2_cfloat(U))+0.5f)/R)



// utils from https://www.shadertoy.com/view/llySRh

__DEVICE__ float line(float2 p, float2 a,float2 b) { 
    p -= a, b -= a;
    float h = dot(p, b) / dot(b, b),                  // proj coord on line
          c = clamp(h, 0.0f, 1.0f);
    p -= b * h; return h==c ? dot(p,p): 1e5;          // dist² to segment
}

#define S(v) smoothstep( 3.0f/R.y, 0.0f, v)

#define hue(v)  ( 0.6f + 0.6f * cos_f4( 6.3f*(v)  + to_float4(0,23,21,0)  ) )

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0



__DEVICE__ float2 F(float4 O, float scale, float E, __TEXTURE2D__ iChannel1) {             // --- Force ( swi2(O,x,y) = pos )
    return E * (2.0f*swi2(texture(iChannel1, swi2(O,x,y)/scale),x,y)-1.0f); // random force field
 // gravity: F = C - swi2(O,x,y); float d = length(F); return E * F / (d*d*d);
}
__DEVICE__ float4 physics(float4 O, float dt, float scale, float E, __TEXTURE2D__ iChannel1) {      // --- simple Newton step
    //swi2(O,z,w) += F(O) * dt;              // velocity
    O.z += F(O, scale, E,iChannel1).x * dt;              // velocity
    O.w += F(O, scale, E,iChannel1).y * dt;              // velocity

    //swi2(O,x,y) += swi2(O,z,w) * dt;       // location
    O.x += O.z * dt;       // location
    O.y += O.w * dt;       // location
    
    return O;
}
__KERNEL__ void TrajectoryRandomForceFieldFuse__Buffer_A(float4 O, float2 u, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    u+=0.5f;

float E = 100.0f,              // attractive intensity (in normalized coordinates)
     dt = 1.0f/600.0f,         // time step
     scale = 8.0f,             // random scale (larger=smoother)
     spread = 1.0f;            // source spread ( 1e-5... 1 )


    O = T(u);
    
    float2 U = ( 2.0f*u - R ) / R.y;
    if (iMouse.y > 0.0f) scale = _exp2f(7.0f*iMouse.y/R.y);
    
    if ( T(to_float2(2,2)).x != R.x || T(to_float2(2,2)).y != R.y ) {
        O -= O;
        if (u.x<2.0f)                  O = to_float4(-0.5f,spread*U.y, 3.0f,0.0f);// init positions .xy and velocities .zw
      //else if (u.x<2.0f) O = to_float4(U,0,0); 
        else if (u.x==2.5f&&u.y==2.5f) O.x=R.x,O.y=R.y;//swi2(O,x,y) = R;     // for testing window size change
        
        SetFragmentShaderComputedColor(O);
        return;
    }
    if ( u.x==2.5f&&u.y==2.5f ) { SetFragmentShaderComputedColor(O); return;}
    if ( u.x > 3.0f && iMouse.z > 0.0f ) O -= O; // click to clear screen
    
    if (u.x==0.5f) {                           // ---simulates physics
        dt /= 2.0f; float4 P = physics(O,dt, scale, E,iChannel1); dt *= 2.0f; // https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
        //swi2(O,x,y) += swi2(P,z,w) * dt;
        O.x += P.z * dt;
        O.y += P.w * dt;
        
        swi2S(O,z,w,  swi2(P,z,w) + F(O, scale, E, iChannel1)*dt/2.0f);
                                             // --- wrap
        if (O.y > 1.0f) O.y -= 2.0f; else if (O.y < -1.0f) O.y += 2.0f;
            float r = R.x/R.y;
        if (O.x > r) O.x -= 2.0f*r; else if (O.x < -r) O.x += 2.0f*r;
    }
    else if (u.x==1.5f)                       // prev position
        O = T(to_float2(0.5f,u.y));
    else {                                   // --- draw rays
        O *= 1.0f - 7.0f*dt;                     // fade past
        float d = 1e5, i_, i=0.0f, v;
        for (; i < R.y; i += 1.0f ) {
            float2 P0 = swi2(T(to_float2(1,i)),x,y), P1 = swi2(T(to_float2(0,i)),x,y), L=P1-P0;
            if (dot(L,L)<0.25f) {
                v = line( U, P0, P1);
                if ( v < d ) d = v, i_ = i;
            }
        }

        O += S( _sqrtf(d) - 0.0f/R.y ) * hue(i_/R.y);
    }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of https://shadertoy.com/view/tsdGRM

__KERNEL__ void TrajectoryRandomForceFieldFuse(float4 O, float2 U, float2 iResolution)
{

    O = sqrt_f4( T(U) );


  SetFragmentShaderComputedColor(O);
}