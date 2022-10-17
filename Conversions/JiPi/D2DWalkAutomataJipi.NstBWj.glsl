

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
//float x = texelFetch( iChannel0, ivec2(fragCoord), 0 ).x;
float x = texture( iChannel0, fragCoord/iResolution.xy ).x;

if (x > 1.)
{

fragColor = vec4(x,x,x,1.);

}
else
fragColor = (0.8 + 0.1 * cos( iTime)) * vec4(x,0.,0.,1.);
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
//   return texelFetch(iChannel0, p, 0 ).x;
   return texture(iChannel0, (vec2(p)+0.5)/iResolution.xy ).x;
}

float hash1( float n )
{
    return fract(sin(n)*138.5453123);
}

bool key_down(int key) {
    return int(texelFetch(KEYBOARD, ivec2(key, 0), 0).x) == 1;
}


float f(float x)
{
return 16. * x * x * (1. - x ) * (1. - x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
ivec2 px = ivec2( fragCoord );

/*
if (iFrame == 0)
{    
float f = 0.;
if (fragCoord.x > 0.5 * iResolution.x && fragCoord.x < 0.5 * iResolution.x + 1.
 && fragCoord.y > 0.5 * iResolution.y && fragCoord.y < 0.5 * iResolution.y + 1.)
f = 3.;

fragColor = vec4( f, 0.0, 0.0, 0.0 );
return;
}
*/

if (iFrame == 0 || key_down(KEY_RESET))
{
// change the 0.99 to get different amounts of particles spawning!
float f = hash1(fragCoord.x*13.0 + 1. + 0.1 * iTime + hash1(fragCoord.y*73.1));
if (f > 0.99 && mod(floor(fragCoord.x),2.) == 0. && mod(floor(fragCoord.y),2.) == 0.)
f = 3.;
else 
f = 0.;
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

float t2 = Cell(px + ivec2(0,-2));
float b2 = Cell(px + ivec2(0,2));
float l2 = Cell(px + ivec2(-2,0));
float r2 = Cell(px + ivec2(2,0));   

float tl = Cell(px + ivec2(-1,-1));
float tr = Cell(px + ivec2(1,-1));
float bl = Cell(px + ivec2(-1,1));
float br = Cell(px + ivec2(1,1));

float sum = t + b + l + r;

// 2 up, 3 right, 4 down, 5 left

/*
if (iFrame % 60 == 0 && fragCoord.x > 0.5 * iResolution.x && fragCoord.x < 0.5 * iResolution.x + 1.
 && fragCoord.y > 0.5 * iResolution.y && fragCoord.y < 0.5 * iResolution.y + 1.)
e = 3.;
else  */
if (fragCoord.y > iMouse.y && fragCoord.y < iMouse.y + 1.
 && fragCoord.x > iMouse.x && fragCoord.x < iMouse.x + 1.)
e = 3.;
else if (e > 1. || (((b == 2. && sum == 2.) || (l == 3. && sum == 3.)
|| (t == 4. && sum == 4.) || (r == 5. && sum == 5.))))// && tl + tr + bl + br == 0.))
e = 1.;
else if (b2 == 2.)
e = 2.;
else if (l2 == 3.)
e = 3.;
else if (t2 == 4.)
e = 4.;
else if (r2 == 5.)
e = 5.;
else if (e <= 1.) // (A) get rid of me to keep the trail alive forever
e -= 0.005;

float n = 0.;
if (t > 0. && t <= 1.)
n++;
if (r > 0. && r <= 1.)
n++;
if (l > 0. && l <= 1.)
n++;
if (b > 0. && b <= 1.)
n++;

/*
if (n == 0. && e > 1.)
{
int m = iFrame % 7;
if (m < 2)
e = 2.;
else if ( m < 4)
e = 5.;
else if (m < 6)
e = 4.;
else 
e = 3.;
}
*/

if (n == 0. && e > 1.)
{
float e2 = 0.;
//int L = int(100. * (1. - f(fragCoord.x / iResolution.x) * f(fragCoord.y / iResolution.y))); // change this value for bigger squares

int L = int(250. * (1. - 0.5 * cos(0.25 * iTime)) * (1. - f(fract(2. * (fragCoord.x / iResolution.y - 0.375))) * f(fract(2. * fragCoord.y / iResolution.y)))); // change this value for bigger squares
//int L = 2;
//int L = iFrame % 60; // 5
int m = iFrame % (4 * L);
if (m < L)
e2 = 2.;
else if ( m < 2 *L )
e2 = 5.;
else if (m < 3 * L)
e2 = 4.;
else
e2 = 3.;

if (mod(e - e2,2.) != 0.)
e = e2;
}





fragColor = vec4( e, 0.0, 0.0, 0.0 );
}