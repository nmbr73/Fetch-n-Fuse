

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

//fragColor = vec4( 4.5* texelFetch( iChannel0, ivec2(fragCoord), 0 ).x );
fragColor = vec4( 4.5* texture( iChannel0, (vec2(ivec2(fragCoord))+0.5)/iResolution.xy ).x );



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
   return texture(iChannel0, (vec2(p)+0.5)/iResolution.xy).x;
}

float hash1( float n )
{
    return fract(sin(n)*138.5453123);
}

bool key_down(int key) {
    return int(texelFetch(KEYBOARD, ivec2(key, 0), 0).x) == 1;
}


// goes through 0.5,0.5, 0 derivative at 0,0  and 1,1
// not in use
float p(float x)
{
return (1. / 9.) * (-4. * x * x + 8. * x + 5.) * x * x * (2.-x) * (2.-x);
}

// not in use
float gain(float x, float k)
{
  float a = 0.5*pow(2.0*((x<0.5)?x:1.0-x), k);
  return (x<0.5)?a:1.0-a;
}

// not in use
float p4(float x)
{
return (0.9 + 0.1 * cos( 2. * 3.14159 * x)) * x;
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 px = ivec2( fragCoord );
    
    if (key_down(KEY_RESET)) {
    // if you want a random reset, uncomment
    
  //  float f = hash1(fragCoord.x*13.0+ 10.131 * iTime + hash1(fragCoord.y*73.1));
    
    float x= fragCoord.x / iResolution.y - 0.375;
    float y = fragCoord.y / iResolution.y;
    vec2 dir = vec2(x,y) - 0.5;
    float d = length(dir);
    float m =0.15;
    float f = (1. - step(d,0.4)) * hash1(fragCoord.x*13.0+ 10.131 * iTime + hash1(fragCoord.y*73.1));


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

  float h = 0.5 * (l+r);
  float v = 0.5 * (t + b);  
  //float k = max(h,v); // "average" of neighbours
float k = 0.5 *(h+v);

 // difference between center and average
 float j = abs(e - k);

if ( e < k)
e += 0.1;
else if (e > k)
e -= 0.115;

if ( e < 0.5)
e -= 0.001;

if (e < 0.1)
e = k;// - 0.01;// 0.01
else if ( e > 0.9 && k > 0.5)
e = k;

//e = 0.999 * e + 0.001 * k;
e -= 0.0135 * step(k,0.1);
 e = max(min(e,1.),0.); // probably not necessary - cap values in between 0. and 1.

  float f = e;
   if( iFrame==0 ) 
   {   
   // f = hash1(fragCoord.x*13.0 + hash1(fragCoord.y*73.1));
      float x= fragCoord.x / iResolution.y - 0.375;
    float y = fragCoord.y / iResolution.y;
    vec2 dir = vec2(x,y) - 0.5;
    float d = length(dir);
    float m =0.3;
 f = (1. - d/m) *  step(d,m)
*hash1(fragCoord.x*13.0+ 10.131 * iTime + hash1(fragCoord.y*73.1));

}
    
	fragColor = vec4( f, 0.0, 0.0, 0.0 );
}