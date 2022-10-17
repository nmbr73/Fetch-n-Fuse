

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Conway's Game of Life
// https://iquilezles.org/articles/gameoflife
//
// State based simulation. Buffer A contains the simulated world,
// and it reads and writes to itself to perform the simulation.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4( texelFetch( iChannel0, ivec2(fragCoord), 0 ).xyzw );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// I implemented three variants of the algorithm with different
// interpretations:
//
// VARIANT = 0: traditional
// VARIANT = 1: box fiter
// VARIANT = 2: high pass filter
#define VARIANT 0

float Cell( in ivec2 p )
{
    // do wrapping
    ivec2 r = ivec2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    
    // fetch texel
    return (texelFetch(iChannel0, p, 0 ).x > 0.5 ) ? texelFetch(iChannel0, p, 0 ).w : texelFetch(iChannel0, p, 0 ).w;
}

float hash1( float n )
{
    return fract(sin(n)*138.5453123);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 px = ivec2( fragCoord );
    
#if VARIANT==0
    float d0 = 0.5;
    float d1 = 0.125;
    float d2 = 0.0;

	
    float k = 0.0;
    float x = 0.0;
    int n = 5;
    
    for(int i=-n;i<=n;i++){
        for(int j=-n;j<=n;j++){
            x = x+texture(iChannel1, vec2(float(i+n)+0.5,float(j+n)+0.5)/float(2*n+1)).x;
            }
        }
    
    for(int i=-n;i<=n;i++){
        for(int j=-n;j<=n;j++){
            k = k + Cell(px+ivec2(int(i),int(j)))*(1.0/x)*texture(iChannel1, vec2(float(i+n)+0.5,float(j+n)+0.5)/float(2*n+1)).x;        
            }
        }
        
    /*    
    float k =    d2*Cell(px+ivec2(-5,-5)) + d1*Cell(px+ivec2(0,-5)) + d2*Cell(px+ivec2(5,-5))
        +  d1*Cell(px+ivec2(-5, 0))+ d0*Cell(px)               +  d1*Cell(px+ivec2(5, 0))
        + d2*Cell(px+ivec2(-5, 5)) +  d1*Cell(px+ivec2(0, 5)) + d2*Cell(px+ivec2(5, 5));
          */  
    float e = Cell(px);
    float f = k;
    
#endif
    

    if( iFrame==0 ) f = 100.0*step(0.99, hash1(fragCoord.x*13.0+hash1(fragCoord.y*71.1)));
    
    float fx = fragCoord.x-iMouse.x;
    float fy = fragCoord.y-iMouse.y;
    vec4 m = iMouse;
    if (sign(m.z)>0.0){
    f = f + 10.0*(1.0-step(10.0, (fx*fx+fy*fy)));
    }
	
	fragColor = vec4( texture(iChannel2,vec2(f/4.0,0.5)).xyz, f );
}