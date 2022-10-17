
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Random Noise: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
__DEVICE__ float rand(float2 n) { 
  return fract(_sinf(dot(n, to_float2(12.9898f, 4.1414f))) * 43758.5453f);
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float iTime, float EMERGE_PROP)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q, (tex+MulOff.y)*MulOff.x, Blend);   
          

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          //Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
          
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), U , Blend));
          Q = _mix(Q,to_float4_s((tex.x+MulOff.y)*MulOff.x), Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          //Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), to_float2((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x), Blend));
          Q = _mix( Q, to_float4_s(step(rand(to_float2(U.x + iTime, U.y - iTime)), EMERGE_PROP)), Blend);

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,U.x,U.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,to_float4_f2f2(U, (swi2(tex,x,y)+MulOff.y)*MulOff.x), Blend);
    }
  
  return Q;
}





// The probability that a texel at the border (or inside the brush) will be live
//#define EMERGE_PROP 0.25f
//#define BRUSH_RADIUS 10.0f



__KERNEL__ void ShaderOfLifeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    //CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_SLIDER0(EMERGE_PROP, 0.0f, 1.0f, 0.25f);
    CONNECT_SLIDER1(BRUSH_RADIUS, 0.0f, 20.0f, 10.0f);

    //Blending
    CONNECT_SLIDER3(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER5(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, All,  Position, Direction, Invers, Special);
    CONNECT_POINT0(Par1, 0.0f, 0.0f);
    
  
    fragCoord+=0.5f;

    // Press Space to bring the Apocalypse.
    if(Reset)
    {
        fragColor = to_float4(0.0f, 0.0f, 0.0f, 1.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }

    int2 coord = to_int2_cfloat(fragCoord);
    int2 resolution = to_int2_cfloat(iResolution);
    
    if(
        coord.y == 0 || coord.y == resolution.y - 1 ||coord.x == 0 || coord.x == resolution.x - 1 ||
        (iMouse.z > 0.0f  && distance_f2(swi2(iMouse,x,y), fragCoord) <= BRUSH_RADIUS)  //&& iMouse.w < 0.0
    ){
        fragColor = to_float4_s(step(rand(to_float2(fragCoord.x + iTime, fragCoord.y - iTime)), EMERGE_PROP));
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    
    const int NEIGHBOR_COUNT = 8;

    const int2 NEIGHBORS[NEIGHBOR_COUNT] = {
                                            to_int2(-1, -1), to_int2( 0, -1), to_int2( 1, -1),
                                            to_int2(-1,  0),                  to_int2( 1,  0),
                                            to_int2(-1,  1), to_int2( 0,  1), to_int2( 1,  1)
                                           };

    int count = 0;
    for(int index = 0; index < NEIGHBOR_COUNT; index++)
    {
      //count += int(texelFetch(iChannel0, coord + NEIGHBORS[index], 0).r != 0.0f);
      count += int(texture(iChannel0, (make_float2(coord + NEIGHBORS[index])+0.5f)/iResolution).x != 0.0f);
    }

//    bool result = texelFetch(iChannel0, coord, 0).r != 0.0f;
    bool result = texture(iChannel0, (make_float2(coord)+0.5f)/iResolution).x != 0.0f;
    if(result)
    { // Alive
      if(count < 2 || count > 3) result = false; // Death due to under- or over-population
    } 
    else 
    { // Dead
      if(count == 3) result = true; // Birth of a new cell
    }

    fragColor = to_float4_s(result);
float AAAAAAAAAAAAAAA;    
    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/iResolution, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iTime, EMERGE_PROP);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// How fast the energy in a texel dissipates after the cell dies
//#define DISSIPATION_RATE 0.01f

// Inputs:
//  iChannel 0 <- Buffer A ... The Game of Life 2D grid of live cells.
//  iChannel 1 <- Buffer B ... The Energy left behind by the last live cell that occupied the texel.

__KERNEL__ void ShaderOfLifeFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER2(DISSIPATION_RATE, 0.0f, 1.0f, 0.01f);

    fragCoord+=0.5f;

    int2 coord = to_int2_cfloat(fragCoord);
    //fragColor = texelFetch(iChannel0, coord, 0) + (1.0f - DISSIPATION_RATE) * texelFetch(iChannel1, coord, 0);
    fragColor = texture(iChannel0, (make_float2(coord)+0.5f)/iResolution) + (1.0f - DISSIPATION_RATE) * texture(iChannel1, (make_float2(coord)+0.5f)/iResolution);
    fragColor = _fminf(to_float4_s(1.0f), fragColor); // Clamp the energy value to avoid values > 1.0
float BBBBBBBBBBBBBBBBB;
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// -------------------------------------------------------------------
// Shader of Life: Yet Another Implementation of Conway's Game of Life
// -------------------------------------------------------------------

// Buffer A: (Inputs: iChannel0 <- Buffer A)
//   Implements of Conway's Game of Life and handles the user input.
//   To keep thing interesting over time, the liveness of the border cells are drived by a pseudo random function.
//
// Buffer B: (Inputs: iChannel0 <- Buffer A, iChannel1 <- Buffer B)
//   Keeps track of the energy left behind in each cell.
//   If the texel's energy is 1.0f, it contains a live cell.
//   If the cell dies, the texel's energy dissipate over time.
//
// Image: (Inputs: iChannel0 <- Buffer B)
//   Maps the cell energy to a fragment color.

// -------------------------------------------------------------------

// User Interactions:
//  - Hold the mouse button and drag to add live cells
//  - Press space to kill all the cells.

// -------------------------------------------------------------------

__KERNEL__ void ShaderOfLifeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    // procedural palette generation: https://iquilezles.org/articles/palettes/
    float3 a = to_float3(0.5f, 0.5f, 0.5f);
    float3 b = to_float3(0.5f, 0.5f, 0.5f);
    float3 c = to_float3(1.2f, 1.3f, 1.5f);
    float3 d = to_float3(0.00f, 0.10f, 0.20f);
float IIIIIIIIIIIIIIIIIII;    
    //float energy = texelFetch(iChannel0, to_int2(fragCoord), 0).r;
    float energy = texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution).x;
    float3 color = a + b * cos_f3(radians(360.0f) * (c * energy + d));
    fragColor    = to_float4_aw(energy * color, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}