

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// -- Output

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    // Output to screen
    fragColor = vec4(B(uv));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// -- Simulation

// Get state of particle at preceding frame. Taken from https://www.shadertoy.com/view/3tBGzh
vec4 T(vec2 U){return A(U-A(U).xy);}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    vec3 col = vec3(0);
    col += noGreen(B(uv).rgb); // get particle input
    
    vec4 last = T(uv); // state of particle at preceding frame
    last.w *= decay; // lower intensity of particle a little
    last.w += emission*length(col); // add intensity from input
    
    float width = 10.; // make extreme values of noise more extreme
    float factor = 10000.; // make big noise
    vec2 noise = noise2(uv*factor, last.xy*factor);
    noise *= 1. + width * smoothstep(1., 1.41421, length(noise)); // apply noise extremism 
    last.xy = 0.01*noise; // scale noise
    last.x *= 1.5; // scale horizontal noise up a bit
    last.y += 0.01*(-.3+ last.w); // simulate kind of heat/gravity effect
    last.y *= 1. + width*smoothstep(0., 1., last.y); // intensify heat effect 

    vec4 // neighborhood of the particle. Taken from https://www.shadertoy.com/view/3tBGzh
        n = T(uv+vec2(0,width)),
        e = T(uv+vec2(width,0)),
        s = T(uv-vec2(0,width)),
        w = T(uv-vec2(width,0));
    
    float cohesion = .8; // particle direction is influenced by others
    last.xy = mix(last.xy, (n.xy+s.xy+w.xy+e.xy)/4., cohesion);
    
    //last = clamp(last, -1., 1.);
    
    fragColor = last;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// -- Add colors
vec4 T(vec2 U){return B(U-A(U).xy);}
vec4 S(vec2 U){return A(U-A(U).xy);}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec3 col = vec3(0);
    col += noGreen(C(uv).rgb) * emission;
    col += T(uv).rgb * decay;
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// -- Parameters
float decay = 0.99; // how much of its preceding intensity a particle gets each frame 
float emission = 0.02; // how intense a new particle is


// -- handy defines taken from https://www.shadertoy.com/view/3tBGzh
#define A(U) texture(iChannel0, U)
#define B(U) texture(iChannel1, U)
#define C(U) texture(iChannel2, U)

// -- Utility functions
vec3 noGreen(vec3 vid){ // remove green background
    float start = 0.2;
    float stp = 1.-start;
    vec3 col = 1. * vid * smoothstep(start,start+stp,length(vid.rb));
    return col;
}

float hash (float p) // from https://www.shadertoy.com/view/3tBGzh
{
	vec4 p4 = fract(vec4(p) * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+19.19);
    return (fract((p4.xxyz+p4.yzzw)*p4.zywx)*2.-1.).x;    
}

vec2 noise2(vec2 v, vec2 w){return vec2(hash(v.y*w.x), hash(v.x*w.y));}


// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// -- Input

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec4 col = vec4(0);
    col += A(uv);
    //float t = iTime / 2.;
    //col += vec4(sin(t)*.5+1., sin(t/2.), sin(t*15.)*.5+1., 1.);
    //col *= smoothstep(0.1, 0., abs(length(0.8 * vec2(cos(t),sin(t)) - (uv*2.-1.) )));
    //col *= smoothstep(0.1,0.,abs(length(iMouse.xy/iResolution.xy - uv)));
    
    fragColor = col;
}