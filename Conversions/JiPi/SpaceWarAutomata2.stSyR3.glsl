

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 col = texelFetch( iChannel0, ivec2(fragCoord), 0 ).rgb;

    fragColor.rgb = vec3(col);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define KEYBOARD iChannel1
#define KEY_RESET 82
//#define hash(p)  fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)

vec3 Cell( in ivec2 p )
{
    // do wrapping
    ivec2 r = ivec2(textureSize(iChannel0, 0));
    p = (p+r) % r;
    
    // fetch texel
    return texelFetch(iChannel0, p, 0 ).rgb;
}


float h21(vec2 a) {
    return fract(sin(dot(a.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

bool key_down(int key) {
    return int(texelFetch(KEYBOARD, ivec2(key, 0), 0).x) == 1;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    ivec2 px = ivec2( fragCoord );    

    if (key_down(KEY_RESET) || iFrame==0) 
    {    
        float d = length((fragCoord -.5* iResolution.xy ) / iResolution.y);
        vec2 f = fragCoord + 0.001 * iTime;
        vec3 g = vec3(h21(f), h21(f + 1.), h21(f - 1.));
        fragColor.rgb = step(0.8, g);
        return;
    }
        
    // center cell
    vec3 e = Cell(px); 
    
    // neighbour cells
    vec3 t = Cell(px + ivec2(0,-1));
    vec3 b = Cell(px + ivec2(0,1));
    vec3 l = Cell(px + ivec2(-1,0));
    vec3 r = Cell(px + ivec2(1,0));   
    
    // "average" of neighbours
    vec3 k = 0.5 * max(t + b, l + r);
    
    // difference between "average" and center
    vec3 j = abs(e - k);
    
    for (int i = 0; i < 3; i++) {
        if (e[i] < k[i] - 0.3)
            e[i] = 1. * k[i] +  10000. * j[i] * e[i] * e[i] * e[i]; 
        else if (k[i] > 0.01 && e[i] > 0.46 && j[i] < 0.5)
            e[i]= k[i] + 0.3 * j[i];      
        else 
            e[i] = k[i];
            e[i] -= 0.16 * j[i];
            e[i] *= 0.99;
    }    
    
    vec3 e2 = e;
    for(int i = 0; i < 3; i++) {
        if (e[(i+1)%3] < e[i]) {
            //e2[(i-1)%3] += 0.01;
            //e2[(i)%3] -= 0.05;// * fragCoord.x/iResolution.x;
            //e2[(i+1)%3] -= 0.01;
            
            e2[i] = mix(e[i], e[(i+1)%3], 0.6);
        }
    }
    e = e2;
    //*/
    e = clamp(e,0.,1.);
     
    fragColor = vec4( e, 0.0 );
}