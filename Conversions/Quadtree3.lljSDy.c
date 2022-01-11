
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__KERNEL__ void Quadtree3Fuse(float4 o, float2 U, float iTime, float2 iResolution)
{

    o -= o;
    float r=0.1f, t=iTime, H = iResolution.y;
    U /=  H;                              // object : disc(P,r)
    float2 P = 0.5f+0.5f*to_float2(_cosf(t),_sinf(t*0.7f)), fU;
    U*=0.5f; P*=0.5f;                         // unzoom for the whole domain falls within [0,1]^n

    o.z = 0.25f;                            // backgroud = cold blue
    o.w = 1.0f;

    for (int i=0; i<7; i++) {             // to the infinity, and beyond ! :-)
        fU = _fminf(U,1.0f-U); if (_fminf(fU.x,fU.y) < 3.0f*r/H) { o--; break; } // cell border
      if (length(P-0.5f) - r > 0.7f) break; // cell is out of the shape

                // --- iterate to child cell
        fU = step(0.5f,U);                  // select child
        U = 2.0f*U - fU;                    // go to new local frame
        P = 2.0f*P - fU;  r *= 2.0f;

        o += 0.13f;                         // getting closer, getting hotter
    }

  o.gb *= smoothstep(0.9f,1.0f,length(P-U)/r); // draw object

  SetFragmentShaderComputedColor(o);

}
