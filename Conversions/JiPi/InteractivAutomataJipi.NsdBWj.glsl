

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
float x =  texelFetch( iChannel0, ivec2(fragCoord), 0 ).x;

x = min(x,1.);
x = 4. * x * (1. - x);
vec3 col = vec3(34.,32.,52.) / 255.;
vec3 col2 = vec3(69.,40.,60.) / 255.;
vec3 col3 = vec3(172.,50.,50.) / 255.;
vec3 col4 = vec3(223.,113.,38.) / 255.;
vec3 col5 = vec3(255.,182.,45.) / 255.;
vec3 col6 = vec3(251.,242.,54.) / 255.;

//vec3 col6 = vec3(1.);
float m = 1. / 7.;
if (x < m)
fragColor = vec4(col,1.);
else if (x < 2. * m)
fragColor = vec4(col2,1.);
else if (x < 3. * m)
fragColor = vec4(col3,1.);
else if (x < 4. * m)
fragColor = vec4(col4,1.);
else if (x < 5. * m)
fragColor = vec4(col5,1.);
else if (x < 6. * m)
fragColor = vec4(col6,1.);
else
fragColor = vec4(col,1.);

//fragColor = max(fragColor,4. * x * (1. -x));
//fragColor = vec4(x,x,x,1.);

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
   return texelFetch(iChannel0, p, 0 ).x;
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
float d = length(fragCoord / iResolution.y - vec2(0.875, 0.5));
   float g = hash1(fragCoord.x * 13.0  + 10.131 * iTime + 100.19414 + hash1(fragCoord.y*73.1 + 1931.1));
    g = step(0.995,g);
fragColor = vec4(g, 0.0, 0.0, 0.0 );
return;
}
    
// center cell
float e = Cell(px); 

// neighbour cells
float t = Cell(px + ivec2(0,-1));
float b = Cell(px + ivec2(0,1));
float l = Cell(px + ivec2(-1,0));
float r = Cell(px + ivec2(1,0));   

// "average" of neighbours
float k = max(0.5 * (t + b), 0.5 * (l + r));

// difference between "average" and center
float j = abs(e - k);

// this stuff makes a completely different automata
/*
float  count = 2. * (step(e,t) + step(e,b) + step(e,l) + step(e,r))  -4.;
if (count >= -1.)
e += 8. * k * e * e * j; // change 8. and 2. for better results
else if (count < 0.)
e -= 2. * j;
*/

// slightly different pattern:
// if (e <= 0.05) // 0.04 works well too
// e =  k +  (30. + 10. * hash1(e)) * e * j;
float m = 0.;
float olde = e;

/*
float c = 0.5 * (1. + cos(1. * iTime));
float ux = fragCoord.x / iResolution.y;
float uy = fragCoord.y / iResolution.y;
ux = 4. * ux * (1.-ux);
uy = 4. * uy * (1.-uy);
*/
if (j < 0.15 && e < 0.9)
e = (7.2 + 0.2 * e) * j ;

//e =  k * max(e,1. - 16. * e * e * (1.-e) * (1.-e));
e = k * max(e, 1. - 8. * e * (1.-e));
e = max(1.001 * e, (0.88 + 1.4 * sign(k-e) * (0.5 + 0.9 * olde) * j) * k);

e *= 1. - 0.9 * olde * e *  j * k;// 0.99 * (1. - 0.2 * e * j);

float lth = length(fragCoord - iMouse.xy);
if (lth < 100.) // 200. * j // or k
 e *= pow(lth / 100.,2.);
 //e *= pow(lth / (200. * j), 2.);

e = max(0., min(1.,e));
fragColor = vec4( e, 0.0, 0.0, 0.0 );
}