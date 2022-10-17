

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Code forked from Inigo Quilez's game of life shader
// https://www.shadertoy.com/view/XstGRf
// Reset code stolen from somewhere else - sorry!
// (Press R to reset shader)

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //fragColor = vec4( 1. - step(3. * texelFetch( iChannel0, ivec2(fragCoord), 0 ).x ,0.2));
    //fragColor = vec4( 3. * texelFetch( iChannel0, ivec2(fragCoord), 0 ).x );
    fragColor = vec4( 3.0 * texture( iChannel0,fragCoord/iResolution.xy ).x );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define KEYBOARD iChannel1
#define KEY_RESET 82


float Cell( in ivec2 p )
{
    // do wrapping
    ivec2 r = ivec2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    
    // fetch texel
   // return (texelFetch(iChannel0, p, 0 ).x > 0.5 ) ? 1 : 0;
   //return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (vec2(p)+0.5)/iResolution.xy ).x;
}

float hash1( float n )
{
    return fract(sin(n)*138.5453123);
}

bool key_down(int key) {
    return int(texelFetch(KEYBOARD, ivec2(key, 0), 0).x) == 1;
}

float S(float x)
{
return step(0.5,x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
ivec2 px = ivec2( fragCoord );
    
if (key_down(KEY_RESET) || iFrame == 0)
{    
float f = 0.;
if (fragCoord.x > 0.5 * iResolution.x && fragCoord.x < 0.5 * iResolution.x + 1.
 && fragCoord.y > 0.5 * iResolution.y && fragCoord.y < 0.5 * iResolution.y + 1.)
f = 3.;

fragColor = vec4( f, 0.0, 0.0, 0.0 );
return;
}
    
// center cell
float e = Cell(px); 

// neighbour cells
float t = Cell(px + ivec2(0,-1));
float b = Cell(px + ivec2(0,1));
float l = Cell(px + ivec2(-1,0));
float r = Cell(px + ivec2(1,0));   


// 2 up, 3 right, 4 down, 5 left

if (fragCoord.y > iMouse.w && fragCoord.y < iMouse.w + 1.
 && fragCoord.x > iMouse.z && fragCoord.x < iMouse.z + 1.)
e = 3.;
else if (e > 1.)
e = 1.;
else if (b == 2.)
e = 2.;
else if (t == 4.)
e = 4.;
else if (l == 3.)
e = 3.;
else if (r == 5.)
e = 5.;
else 
e -= 0.005;

float q = hash1(fragCoord.x*13.0 + 0.1 * iTime + hash1(fragCoord.y*73.1));
if (q > 0.95) // probability direction will change
{
// turn anticlockwise 
// could easily be replaced with a function but I'm lazy atm
if (e == 2.)
e = 3.;
else if (e == 3.)
e = 4.;
else if (e == 4.)
e = 5.;
else if (e == 5.)
e = 2.;
}


fragColor = vec4( e, 0.0, 0.0, 0.0 );
}