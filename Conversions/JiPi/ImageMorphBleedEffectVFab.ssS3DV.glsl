

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Image Morph Bleed Effect" by _bm. https://shadertoy.com/view/NdS3WK
// 2021-04-09 11:50:28
// Refactoring, commenting and extending

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture(iChannel0, fragCoord / iResolution.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// --- metrics for color similarity 

//#define dist(c1,c2) distance(c1,c2)
  #define dist(c1,c2) distance(c1.rgb,c2.rgb)
//#define dist(c1,c2) distance( c1 / max(c1.r,max(c1.g,c1.b)), c2 / max(c2.r,max(c2.g,c2.b)) )
//#define dist(c1,c2) distance(c1.r,c2.r)
//#define dist(c1,c2) max(0.,1.-distance(c1.rgb,c2.rgb))

#define DIST 8.

#define hash(s)  ( texture(iChannel2, U + iTime * s /R ).xy - .5 )

#define T(s)       texture(iChannel0, U + amnt* hash(s)/R )
 
void mainImage( out vec4 O, vec2 u )
{
    vec2 R = iResolution.xy, 
         U = u/R;
         
    vec4 bg = texture(iChannel3, U);             // target image
    
    O = texture(iChannel0, U);                   // current stage
    float amnt = DIST * dist(bg, O);             // difference to target
    vec4 c1 = T(1.93937174e6),                   // pull alternate value
         c2 = T(1.12380517e5);                   // as far as unconverged
   
    float a = dist(c1, bg),                      // difference to target
          b = dist(c2, bg);
    if ( b < a ) a = b, c1 = c2;                 // get the closest
    O =  mix(c1, O, min(2.*a, 1.) );             //  is close enought, blend it weigthed by convergence

    if (O==vec4(0))  O = texture(iChannel1, U);  // init texture (for the color palette)
}