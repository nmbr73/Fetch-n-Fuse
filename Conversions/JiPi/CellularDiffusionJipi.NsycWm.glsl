

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 draw = texture(iChannel0,uv);
    // normal draw
    fragColor = draw;
    
    
    // for the flashing image, rotate colour by frame num
    int col = colourr(draw.rgb);
    int rot = int(texture(iChannel1,vec2(0,0)).x);
    fragColor = vec4(get_col((col-rot+2) % 3), 1.);
}


/*
Doing some reading now to see how i was meant to do it
https://www.youtube.com/watch?v=TORwMc2AaRE
https://softologyblog.wordpress.com/2018/03/23/rock-paper-scissors-cellular-automata/
my result is very similar to the one with a random variable.
Added the random variable, works well.
Interesting how the flashing version looks almost the same on a larger scale.
*/
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime;
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    vec3 oldcol = texture(iChannel0,uv).rgb;
    int uvcol = colourr(oldcol);
    // slower to get all but whatever
    // get neighbouring colours
    int Ncol = colourr(texture(iChannel0,(fragCoord+vec2(0,1))/iResolution.xy).rgb);
    int Scol = colourr(texture(iChannel0,(fragCoord-vec2(0,1))/iResolution.xy).rgb);
    int Ecol = colourr(texture(iChannel0,(fragCoord+vec2(1,0))/iResolution.xy).rgb);
    int Wcol = colourr(texture(iChannel0,(fragCoord-vec2(1,0))/iResolution.xy).rgb);
    
    
    int NEcol = colourr(texture(iChannel0,(fragCoord+vec2(1,1))/iResolution.xy).rgb);
    int SEcol = colourr(texture(iChannel0,(fragCoord+vec2(1,-1))/iResolution.xy).rgb);
    int NWcol = colourr(texture(iChannel0,(fragCoord+vec2(-1,1))/iResolution.xy).rgb);
    int SWcol = colourr(texture(iChannel0,(fragCoord+vec2(-1,-1))/iResolution.xy).rgb);
    
    
    int prey = (uvcol+1)%3;
    int pred = (uvcol+2)%3;
    

    // sum neighbours
    int preyN = int(Ncol==prey) + int(Scol==prey) + int(Ecol==prey) + int(Wcol==prey);
    int predN = int(Ncol==pred) + int(Scol==pred) + int(Ecol==pred) + int(Wcol==pred);
    
    int predN2 = int(NEcol==pred) + int(SEcol==pred) + int(NWcol==pred) + int(SWcol==pred);
    
    
    vec3 newcol;
    
    /*
    // another method, stable without rotating colours
    // but doesnt look good.
    // need to turn off rgb rotation in Image main
    float thresh = 2.5;
    float kernal_diag = 1.;
    
    //if (time > 2.) {thresh = 3.;}
    if (float(predN) + kernal_diag*float(predN2) > thresh + randf(uv,time)*2.) {
      newcol = get_col(pred);
      //newcol = get_col(uvcol);
    } else {

      newcol = oldcol;
    }
    */
    
    
    
    // this code block almost behaves how i want, but it flashes
    // this is fixed by switching rgb order every other frame when printing
    // also has a weird miniture pattern that gives it a crt tv look.
    int thresh = 4;
    if ((predN2 + predN) < thresh) {
      newcol = get_col(pred);
    } else {
      newcol = oldcol;
    }
    
    
    
    
    // initialise to randomness
    if(int(texture(iChannel1,vec2(0,0)).y)==0) {
      float x = randf(uv,time)*3.;
      newcol = get_col(int(x));
      //newcol = get_col(int(uv*3.));
    }

    // Output to screen
    fragColor = vec4(newcol,1.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// store an iteration number for switching frames
// store init flag
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec4 draw = texture(iChannel1,uv);
    if (fragCoord.x < 1. && fragCoord.y < 1.){
      draw.x = float(int(draw.x + 1.) % 3);
      draw.y = 1.;
    }
    
    
    fragColor = draw;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }

float randf(vec2 uv, float t) {
  return rand(uv*(t+1.0));
}

int colourr(vec3 col) {
  //return int(dot(col, vec3(0,1,2)));
  if (col.x > 0.5) {
    return 0;
  } else if (col.y > 0.5) {
    return 1;
  }
  return 2; 
}

vec3 get_col(int k) {
  //k = k%3;
  if (k == 0) {
      return vec3(1.,0.,0.);
  } else if (k == 1) {
      return vec3(0.,1.,0.);
  } else {
      return vec3(0.,0.,1.);
  }
  
}