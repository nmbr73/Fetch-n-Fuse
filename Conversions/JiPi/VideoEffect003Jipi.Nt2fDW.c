
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)





// -- handy defines taken from https://www.shadertoy.com/view/3tBGzh
#define A(U) _tex2DVecN(iChannel0,(U).x,(U).y,15)
#define B(U) _tex2DVecN(iChannel1,(U).x,(U).y,15)
#define C(U) _tex2DVecN(iChannel2,(U).x,(U).y,15)

// -- Utility functions
__DEVICE__ float3 noGreen(float3 vid){ // remove green background
    float start = 0.2f;
    float stp = 1.0f-start;
    float3 col = 1.0f * vid * smoothstep(start,start+stp,length(swi2(vid,x,z)));
    return col;
}

__DEVICE__ float hash (float p) // from https://www.shadertoy.com/view/3tBGzh
{
    float4 p4 = fract_f4(to_float4_s(p) * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
    p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
    return (fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x))*2.0f-1.0f).x;    
}

__DEVICE__ float2 noise2(float2 v, float2 w){return to_float2(hash(v.y*w.x), hash(v.x*w.y));}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel1



// -- Simulation

// Get state of particle at preceding frame. Taken from https://www.shadertoy.com/view/3tBGzh
__DEVICE__ float4 TA(float2 U, float2 R, __TEXTURE2D__ iChannel0){return A(U-swi2(A(U),x,y));}

__KERNEL__ void VideoEffect003JipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);

    CONNECT_SLIDER0(decay, 0.0f, 2.0f, 0.99f);
    CONNECT_SLIDER1(emission, 0.0f, 1.0f, 0.02f);
     
    fragCoord+=0.5f;
    
    // -- Parameters
    //float decay = 0.99f; // how much of its preceding intensity a particle gets each frame 
    //float emission = 0.02f; // how intense a new particle is

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;

    float3 col = to_float3_s(0);
    col += noGreen(swi3(B(uv),x,y,z)); // get particle input
    
    float4 last = TA(uv,R,iChannel0); // state of particle at preceding frame
    last.w *= decay; // lower intensity of particle a little
    last.w += emission*length(col); // add intensity from input
    
    float width = 10.0f; // make extreme values of noise more extreme
    float factor = 10000.0f; // make big noise
    float2 noise = noise2(uv*factor, swi2(last,x,y)*factor);
    noise *= 1.0f + width * smoothstep(1.0f, 1.41421f, length(noise)); // apply noise extremism 
    //swi2(last,x,y) = 0.01f*noise; // scale noise
    last.x = 0.01f*noise.x; // scale noise
    last.y = 0.01f*noise.y; // scale noise
    last.x *= 1.5f; // scale horizontal noise up a bit
    last.y += 0.01f*(-0.3f+ last.w); // simulate kind of heat/gravity effect
    last.y *= 1.0f + width*smoothstep(0.0f, 1.0f, last.y); // intensify heat effect 

    float4 // neighborhood of the particle. Taken from https://www.shadertoy.com/view/3tBGzh
        n = TA(uv+to_float2(0,width),R,iChannel0),
        e = TA(uv+to_float2(width,0),R,iChannel0),
        s = TA(uv-to_float2(0,width),R,iChannel0),
        w = TA(uv-to_float2(width,0),R,iChannel0);
    
    float cohesion = 0.8f; // particle direction is influenced by others
    swi2S(last,x,y, _mix(swi2(last,x,y), (swi2(n,x,y)+swi2(s,x,y)+swi2(w,x,y)+swi2(e,x,y))/4.0f, cohesion));
    
    //last = clamp(last, -1.0f, 1.0f);
    
    fragColor = last;
    
    if ( iFrame<1 || Reset) 
      fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// -- Add colors
__DEVICE__ float4 TB(float2 U,float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1){return B(U-swi2(A(U),x,y));}
__DEVICE__ float4 S(float2 U,float2 R, __TEXTURE2D__ iChannel0){return A(U-swi2(A(U),x,y));}

__KERNEL__ void VideoEffect003JipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_SLIDER0(decay, 0.0f, 2.0f, 0.99f);
    CONNECT_SLIDER1(emission, 0.0f, 1.0f, 0.02f);
 
    fragCoord+=0.5f;
    
    // -- Parameters
    //float decay = 0.99f; // how much of its preceding intensity a particle gets each frame 
    //float emission = 0.02f; // how intense a new particle is
    
    float2 uv = fragCoord / iResolution;
    float3 col = to_float3_s(0);
    col += noGreen(swi3(C(uv),x,y,z)) * emission;
    col += swi3(TB(uv,R,iChannel0,iChannel1),x,y,z) * decay;
    
    fragColor = to_float4_aw(col,1.0f);
    
    if ( iFrame<1 || Reset) 
      fragColor = to_float4(0.0f,0.0f,0.0f,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0


// -- Input

__KERNEL__ void VideoEffect003JipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float4 col = to_float4_s(0);
    col += A(uv);
    //float t = iTime / 2.0f;
    //col += to_float4(_sinf(t)*0.5f+1.0f, _sinf(t/2.0f), _sinf(t*15.0f)*0.5f+1.0f, 1.0f);
    //col *= smoothstep(0.1f, 0.0f, _fabs(length(0.8f * to_float2(_cosf(t),_sinf(t)) - (uv*2.0f-1.0f) )));
    //col *= smoothstep(0.1f,0.0f,_fabs(length(iMouse.xy/iResolution - uv)));
    
    fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// -- Output

__KERNEL__ void VideoEffect003JipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    fragCoord+=0.5f;

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Output to screen
    fragColor = (B(uv)) + (Color-0.5f)*B(uv).x;

  SetFragmentShaderComputedColor(fragColor);
}