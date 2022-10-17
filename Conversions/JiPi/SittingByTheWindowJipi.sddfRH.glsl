

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// shader derived from Heartfelt - by Martijn Steinrucken aka BigWings - 2017
// https://www.shadertoy.com/view/ltffzl
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define S(a, b, t) smoothstep(a, b, t)
//#define DEBUG
#define size 0.2
#define CAM // uncomment to switch from webcam input to iChannel1 texture


vec3 N13(float p) {
   //  from DAVE HOSKINS
   vec3 p3 = fract(vec3(p) * vec3(.1031,.11369,.13787));
   p3 += dot(p3, p3.yzx + 19.19);
   return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

vec4 N14(float t) {
	return fract(sin(t*vec4(123., 1024., 1456., 264.))*vec4(6547., 345., 8799., 1564.));
}
float N(float t) {
    return fract(sin(t*12345.564)*7658.76);
}

float Saw(float b, float t) {
	return S(0., b, t)*S(1., b, t);
}



vec2 Drops(vec2 uv, float t) {
    
    vec2 UV = uv;
    
    // DEFINE GRID
    uv.y += t*0.8;
    vec2 a = vec2(6., 1.);
    vec2 grid = a*2.;
    vec2 id = floor(uv*grid);
    
    // RANDOM SHIFT Y
    float colShift = N(id.x); 
    uv.y += colShift;
    
    // DEFINE SPACES
    id = floor(uv*grid);
    vec3 n = N13(id.x*35.2+id.y*2376.1);
    vec2 st = fract(uv*grid)-vec2(.5, 0);
    
    // POSITION DROPS
    //clamp(2*x,0,2)+clamp(1-x*.5, -1.5, .5)+1.5-2
    float x = n.x-.5;
    
    float y = UV.y*20.;
    float distort = sin(y+sin(y));
    x += distort*(.5-abs(x))*(n.z-.5);
    x *= .7;
    float ti = fract(t+n.z);
    y = (Saw(.85, ti)-.5)*.9+.5;
    vec2 p = vec2(x, y);
    
    // DROPS
    float d = length((st-p)*a.yx);
    
    float dSize = size; 
    
    float Drop = S(dSize, .0, d);
    
    
    float r = sqrt(S(1., y, st.y));
    float cd = abs(st.x-x);
    
    // TRAILS
    float trail = S((dSize*.5+.03)*r, (dSize*.5-.05)*r, cd);
    float trailFront = S(-.02, .02, st.y-y);
    trail *= trailFront;
    
    
    // DROPLETS
    y = UV.y;
    y += N(id.x);
    float trail2 = S(dSize*r, .0, cd);
    float droplets = max(0., (sin(y*(1.-y)*120.)-st.y))*trail2*trailFront*n.z;
    y = fract(y*10.)+(st.y-.5);
    float dd = length(st-vec2(x, y));
    droplets = S(dSize*N(id.x), 0., dd);
    float m = Drop+droplets*r*trailFront;
    
    #ifdef DEBUG
    m += st.x>a.y*.45 || st.y>a.x*.165 ? 1.2 : 0.; //DEBUG SPACES
    #endif
    
    
    return vec2(m, trail);
}

float StaticDrops(vec2 uv, float t) {
	uv *= 30.;
    
    vec2 id = floor(uv);
    uv = fract(uv)-.5;
    vec3 n = N13(id.x*107.45+id.y*3543.654);
    vec2 p = (n.xy-.5)*0.5;
    float d = length(uv-p);
    
    float fade = Saw(.025, fract(t+n.z));
    float c = S(size, 0., d)*fract(n.z*10.)*fade;

    return c;
}

vec2 Rain(vec2 uv, float t) {
    float s = StaticDrops(uv, t); 
    vec2 r1 = Drops(uv, t);
    vec2 r2 = Drops(uv*1.8, t);
    
    #ifdef DEBUG
    float c = r1.x;
    #else
    float c = s+r1.x+r2.x;
    #endif
    
    c = S(.3, 1., c);
    
    #ifdef DEBUG
    return vec2(c, r1.y);
    #else
    return vec2(c, max(r1.y, r2.y));
    #endif
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = (fragCoord.xy-.5*iResolution.xy) / iResolution.y;
    vec2 UV = fragCoord.xy/iResolution.xy;
    float T = iTime;
    
    
    float t = T*.2;
    
    float rainAmount = 0.8;
    

    
    UV = (UV-.5)*(.9)+.5;
    
    vec2 c = Rain(uv, t);

   	vec2 e = vec2(.001, 0.); //pixel offset
   	float cx = Rain(uv+e, t).x;
   	float cy = Rain(uv+e.yx, t).x;
   	vec2 n = vec2(cx-c.x, cy-c.x); //normals
    
    float blur = 5.0;    
    float focus = blur-c.y*blur*0.75;
    
    #ifdef CAM

    // BLUR derived from existical https://www.shadertoy.com/view/Xltfzj
        float Pi = 6.28318530718; // Pi*2
    
        // GAUSSIAN BLUR SETTINGS {{{
        float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
        float Quality = 3.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
        float Size = 32.0; // BLUR SIZE (Radius)
        // GAUSSIAN BLUR SETTINGS }}}

        vec2 Radius = Size/iResolution.xy;

        vec3 col = texture(iChannel0, UV).rgb;
            // Blur calculations
        for( float d=0.0; d<Pi; d+=Pi/Directions)
        {
            for(float i=1.0/Quality; i<=1.0; i+=1.0/Quality)
            {
                #ifdef DEBUG
                vec3 tex = texture( iChannel0, UV+c+vec2(cos(d),sin(d))*Radius*i).rgb;
                #else
                vec3 tex = texture( iChannel0, UV+n+vec2(cos(d),sin(d))*Radius*i).rgb;
                #endif

                col += tex;            
            }
        }

        col /= Quality * Directions - 0.0;

        vec3 tex = texture( iChannel0, UV+n).rgb;
        c.y = clamp(c.y, 0.0, 1.);

        col -= c.y;
        col += c.y*(tex+0.75);

    #else
    vec3 col = textureLod(iChannel1, UV+n, focus).rgb;
    #endif
    
    fragColor = vec4(col, 1.);
}