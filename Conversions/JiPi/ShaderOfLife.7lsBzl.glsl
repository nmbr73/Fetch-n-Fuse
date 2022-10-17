

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// -------------------------------------------------------------------
// Shader of Life: Yet Another Implementation of Conway's Game of Life
// -------------------------------------------------------------------

// Buffer A: (Inputs: iChannel0 <- Buffer A)
//   Implements of Conway's Game of Life and handles the user input.
//   To keep thing interesting over time, the liveness of the border cells are drived by a pseudo random function.
//
// Buffer B: (Inputs: iChannel0 <- Buffer A, iChannel1 <- Buffer B)
//   Keeps track of the energy left behind in each cell.
//   If the texel's energy is 1.0, it contains a live cell.
//   If the cell dies, the texel's energy dissipate over time.
//
// Image: (Inputs: iChannel0 <- Buffer B)
//   Maps the cell energy to a fragment color.

// -------------------------------------------------------------------

// User Interactions:
//  - Hold the mouse button and drag to add live cells
//  - Press space to kill all the cells.

// -------------------------------------------------------------------

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // procedural palette generation: https://iquilezles.org/articles/palettes/
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.2, 1.3, 1.5);
    vec3 d = vec3(0.00, 0.10, 0.20);
    
    float energy = texelFetch(iChannel0, ivec2(fragCoord), 0).r;
    vec3 color = a + b * cos(radians(360.0) * (c * energy + d));
    fragColor.rgb = energy * color;
    fragColor.a = 1.0;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Random Noise: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

// The probability that a texel at the border (or inside the brush) will be live
#define EMERGE_PROP 0.25
#define BRUSH_RADIUS 10.0

#define SPACE_ASCII 32
#define JUST_PRESSED 1

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    // Press Space to bring the Apocalypse.
    if(texelFetch(iChannel1, ivec2(SPACE_ASCII, JUST_PRESSED), 0).x != 0.0)
    {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    ivec2 coord = ivec2(fragCoord);
    ivec2 resolution = ivec2(iResolution.xy);
    
    if(
        coord.y == 0 || coord.y == resolution.y - 1 ||coord.x == 0 || coord.x == resolution.x - 1 ||
        (iMouse.z > 0.0 && iMouse.w < 0.0 && distance(iMouse.xy, fragCoord) <= BRUSH_RADIUS)
    ){
        fragColor = vec4(step(rand(vec2(fragCoord.x + iTime, fragCoord.y - iTime)), EMERGE_PROP));
        return;
    }
    
    
    const int NEIGHBOR_COUNT = 8;

    const ivec2 NEIGHBORS[NEIGHBOR_COUNT] = ivec2[NEIGHBOR_COUNT](
        ivec2(-1, -1), ivec2( 0, -1), ivec2( 1, -1),
        ivec2(-1,  0),                ivec2( 1,  0),
        ivec2(-1,  1), ivec2( 0,  1), ivec2( 1,  1)
    );

    int count = 0;
    for(int index = 0; index < NEIGHBOR_COUNT; index++)
    {
        count += int(texelFetch(iChannel0, coord + NEIGHBORS[index], 0).r != 0.0);
    }

    bool result = texelFetch(iChannel0, coord, 0).r != 0.0;
    if(result)
    { // Alive
        if(count < 2 || count > 3) result = false; // Death due to under- or over-population
    } 
    else 
    { // Dead
        if(count == 3) result = true; // Birth of a new cell
    }

    fragColor = vec4(result);
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// How fast the energy in a texel dissipates after the cell dies
#define DISSIPATION_RATE 0.01

// Inputs:
//  iChannel 0 <- Buffer A ... The Game of Life 2D grid of live cells.
//  iChannel 1 <- Buffer B ... The Energy left behind by the last live cell that occupied the texel.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 coord = ivec2(fragCoord);
    fragColor = texelFetch(iChannel0, coord, 0) + (1.0 - DISSIPATION_RATE) * texelFetch(iChannel1, coord, 0);
    fragColor = min(vec4(1.0), fragColor); // Clamp the energy value to avoid values > 1.0
}