

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

//fragColor = vec4( 1. - step(3. * texelFetch( iChannel0, ivec2(fragCoord), 0 ).x ,0.5));
//fragColor = vec4(3. * texelFetch( iChannel0, ivec2(fragCoord), 0 ).x );
fragColor = vec4(3. * texture( iChannel0, (vec2(ivec2(fragCoord))+0.5)/iResolution.xy ).x );



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
    
   if (key_down(KEY_RESET) || iFrame == 0 )
   {
   vec2 U = fragCoord/iResolution.y ; 
   U.x -= 0.375;
   U = step( fract(3. * abs(U-0.5)),vec2(3. / iResolution.y) );
   float f = max(U.x,U.y) * hash1(fragCoord.x*13.0 + 10.131 * iTime + hash1(fragCoord.y*73.1));
   fragColor = vec4( step(0.5,f), 0,0,0 );
   return;
   }
    
  // center cell
  float e = Cell(px); 

  // neighbour cells
  float t = Cell(px + ivec2(0,-1));
  float b = Cell(px + ivec2(0,1));
  float l = Cell(px + ivec2(-1,0));
  float r = Cell(px + ivec2(1,0));   

  float tl = Cell(px + ivec2(-1,-1));
  float tr = Cell(px + ivec2(1,-1));
  float bl = Cell(px + ivec2(-1,1));
  float br = Cell(px + ivec2(1,1));

  float t2 = Cell(px + ivec2(0,-2));
  float b2 = Cell(px + ivec2(0,2));
  float l2 = Cell(px + ivec2(-2,0));
  float r2 = Cell(px + ivec2(2,0));   


float q = hash1(fragCoord.x*13.0+ 10.131 * iTime + hash1(fragCoord.y*73.1));

vec2 uv = fragCoord/iResolution.xy;
vec2 uv2 = floor(1000. * uv) / 1000.;
float q2 = hash1(uv2.x*13.0 + hash1(uv2.y*73.1));

if (q2 > 0.55)
{
float sumNeighbours = S(t) + S(b) + S(l) + S(r) + S(tl) + S(tr) + S(bl) + S(br);

if ( sumNeighbours == 1. && e < 1.)
e =  step(0.5, 0.6 * q);

else if (e > 0. && e < 2.)
{
if (tl * tr > 0. || tl * bl > 0. || bl * br >0. || tr * br > 0.)
e = 0.;
else if (t * l > 0. || t * r > 0. || b * l > 0. || b * r > 0.)
e = 0.;
//else if (t2 * b2 > 0. || l2 * r2 > 0.)
else if ((t2 > 0. && t <1.) || (b2 > 0. && b < 1.) || (r2 >0. && r < 1.) || (l2 >0. && l <1.))
e = 0.;
else if (sumNeighbours == 2.)
e = 2.;
}
}
else
e = 0.;
    
fragColor = vec4( e, 0.0, 0.0, 0.0 );
}