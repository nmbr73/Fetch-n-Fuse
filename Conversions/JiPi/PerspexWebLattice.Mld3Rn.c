
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


/*
  Perspex Web Lattice
  -------------------
  
  I felt that Shadertoy didn't have enough Voronoi examples, so I made another one. :) I'm
  not exactly sure what it's supposed to be... My best guess is that an Alien race with no 
  common sense designed a monitor system with physics defying materials. :)

  Technically speaking, there's not much to it. It's just some raymarched 2nd order Voronoi.
  The dark perspex-looking web lattice is created by manipulating the Voronoi value slightly 
  and giving the effected region an ID value so as to color it differently, but that's about
  it. The details are contained in the "heightMap" function.

  There's also some subtle edge detection in order to give the example a slight comic look. 
  3D geometric edge detection doesn't really differ a great deal in concept from 2D pixel 
  edge detection, but it obviously involves more processing power. However, it's possible to 
  combine the edge detection with the normal calculation and virtually get it for free. Kali 
  uses it to great effect in his "Fractal Land" example. It's also possible to do a
  tetrahedral version... I think Nimitz and some others may have done it already. Anyway, 
  you can see how it's done in the "nr" (normal) function.

  Geometric edge related examples:

  Fractal Land - Kali
  https://www.shadertoy.com/view/XsBXWt

  Rotating Cubes - Shau
  https://www.shadertoy.com/view/4sGSRc

  Voronoi mesh related:

    // I haven't really looked into this, but it's interesting.
  Weaved Voronoi - FabriceNeyret2 
    https://www.shadertoy.com/view/ltsXRM

*/

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define FAR 2.0f

//int id = 0; // Object ID - Red perspex: 0; Black lattice: 1.


// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){
   
    n = _fmaxf((abs_f3(n) - 0.2f), to_float3_s(0.001f));
    n /= (n.x + n.y + n.z ); // Roughly normalized.
    
    p = swi3(texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z,x,y,z);
    
    // Loose sRGB to RGB conversion to counter final value gamma correction...
    // in case you're wondering.
    return p*p;
}


// Compact, self-contained version of IQ's 3D value noise function. I have a transparent noise
// example that explains it, if you require it.
__DEVICE__ float n3D(float3 p){
    
  const float3 s = to_float3(7, 157, 113);
  float3 ip = _floor(p); p -= ip; 
  float4 h = to_float4(0.0f, s.y, s.z, s.y + s.z) + dot(ip, s);
  p = p*p*(3.0f - 2.0f*p); //p *= p*p*(p*(p * 6.0f - 15.0f) + 10.0f);
  h = _mix(fract_f4(sin_f4(h)*43758.5453f), fract_f4(sin_f4(h + s.x)*43758.5453f), p.x);
  swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y));
  return _mix(h.x, h.y, p.z); // Range: [0, 1].
}

// float2 to float2 hash.
__DEVICE__ float2 hash22(float2 p, float iTime) { 

    // Faster, but doesn't disperse things quite as nicely. However, when framerate
    // is an issue, and it often is, this is a good one to use. Basically, it's a tweaked 
    // amalgamation I put together, based on a couple of other random algorithms I've 
    // seen around... so use it with caution, because I make a tonne of mistakes. :)
    float n = _sinf(dot(p, to_float2(41, 289)));
    //return fract(to_float2(262144, 32768)*n); 
    
    // Animated.
    p = fract_f2(to_float2(262144, 32768)*n); 
    // Note the ".45," insted of ".5" that you'd expect to see. When edging, it can open 
    // up the cells ever so slightly for a more even spread. In fact, lower numbers work 
    // even better, but then the random movement would become too restricted. Zero would 
    // give you square cells.
    return sin_f2( p*6.2831853f + iTime )*0.45f + 0.5f; 
    
}

// 2D 2nd-order Voronoi: Obviously, this is just a rehash of IQ's original. I've tidied
// up those if-statements. Since there's less writing, it should go faster. That's how 
// it works, right? :)
//
__DEVICE__ float Voronoi(in float2 p, float iTime){
    
  float2 g = _floor(p), o; p -= g;
  
  float3 d = to_float3_s(1); // 1.4f, etc. "d.z" holds the distance comparison value.
    
  for(int _y = -1; _y <= 1; _y++){
    for(int _x = -1; _x <= 1; _x++){
            
      o = to_float2(_x, _y);
      o += hash22(g + o, iTime) - p;
            
      d.z = dot(o, o); 
            // More distance metrics.
            //o = _fabs(o);
            //d.z = _fmaxf(o.x*0.8666f + o.y*0.5f, o.y);// 
            //d.z = _fmaxf(o.x, o.y);
            //d.z = (o.x*0.7f + o.y*0.7f);
            
      d.y = _fmaxf(d.x, _fminf(d.y, d.z));
      d.x = _fminf(d.x, d.z); 
                       
    }
  }
  
  return _fmaxf(d.y/1.2f - d.x*1.0f, 0.0f)/1.2f;
  //return d.y - d.x; // return 1.0f-d.x; // etc.
    
}

// The height map values. In this case, it's just a Voronoi variation. By the way, I could
// optimize this a lot further, but it's not a particularly taxing distance function, so
// I've left it in a more readable state.
__DEVICE__ float heightMap(float3 p, float iTime, int *id){
    
    *id =0;
    float c = Voronoi(swi2(p,x,y)*4.0f,iTime); // The fiery bit.
    
    // For lower values, reverse the surface direction, smooth, then
    // give it an ID value of one. Ie: this is the black web-like
    // portion of the surface.
    if (c<0.07f) {c = smoothstep(0.7f, 1.0f, 1.0f-c)*0.2f; *id = 1; }

    return c;
}

// Standard back plane height map. Put the plane at to_float3(0, 0, 1), then add some height values.
// Obviously, you don't want the values to be too large. The one's here account for about 10%
// of the distance between the plane and the camera.
__DEVICE__ float m(float3 p, float iTime, int *id){
   
    float h = heightMap(p,iTime,id); // texture(iChannel0, p.xy/2.0f).x; // Texture work too.
    
    return 1.0f - p.z - h*0.1f;
    
}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ.
__DEVICE__ float3 nr(in float3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    float2 e = to_float2(0.005f, -0.005f); 
    return normalize(swi3(e,x,y,y) * m(p + swi3(e,x,y,y)) + swi3(e,y,y,x) * m(p + swi3(e,y,y,x)) + swi3(e,y,x,y) * m(p + swi3(e,y,x,y)) + swi3(e,x,x,x) * m(p + swi3(e,x,x,x)));
}
*/

/*
// Standard normal function - for comparison with the one below.
__DEVICE__ float3 nr(in float3 p) {
  const float2 e = to_float2(0.005f, 0);
  return normalize(to_float3(m(p + swi3(e,x,y,y)) - m(p - swi3(e,x,y,y)), m(p + swi3(e,y,x,y)) - m(p - swi3(e,y,x,y)),  m(p + swi3(e,y,y,x)) - m(p - swi3(e,y,y,x))));
}
*/

// The normal function with some edge detection rolled into it.
__DEVICE__ float3 nr(float3 p, inout float *edge, float iTime, int *id) { 
  
  float2 e = to_float2(0.005f, 0);

  // Take some distance function measurements from either side of the hit point on all three axes.
  float d1 = m(p + swi3(e,x,y,y),iTime,id), d2 = m(p - swi3(e,x,y,y),iTime,id);
  float d3 = m(p + swi3(e,y,x,y),iTime,id), d4 = m(p - swi3(e,y,x,y),iTime,id);
  float d5 = m(p + swi3(e,y,y,x),iTime,id), d6 = m(p - swi3(e,y,y,x),iTime,id);
  float d = m(p,iTime,id)*2.0f;  // The hit point itself - Doubled to cut down on calculations. See below.
     
    // Edges - Take a geometry measurement from either side of the hit point. Average them, then see how
    // much the value differs from the hit point itself. Do this for X, Y and Z directions. Here, the sum
    // is used for the overall difference, but there are other ways. Note that it's mainly sharp surface 
    // curves that register a discernible difference.
    *edge = _fabs(d1 + d2 - d) + _fabs(d3 + d4 - d) + _fabs(d5 + d6 - d);
    //*edge = _fmaxf(max(_fabs(d1 + d2 - d), _fabs(d3 + d4 - d)), _fabs(d5 + d6 - d)); // Etc.
    
    // Once you have an edge value, it needs to normalized, and smoothed if possible. How you 
    // do that is up to you. This is what I came up with for now, but I might tweak it later.
    *edge = smoothstep(0.0f, 1.0f, _sqrtf(*edge/e.x*2.0f));
  
    // Return the normal.
    // Standard, normalized gradient mearsurement.
    return normalize(to_float3(d1 - d2, d3 - d4, d5 - d6));
}

/*
// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
__DEVICE__ float cAO(in float3 p, in float3 n)
{
  float sca = 3.0f, occ = 0.0f;
    for(float i=0.0f; i<5.0f; i++){
    
        float hr = 0.01f + i*0.5f/4.0f;        
        float dd = m(n * hr + p);
        occ += (hr - dd)*sca;
        sca *= 0.7f;
    }
    return clamp(1.0f - occ, 0.0f, 1.0f);    
}
*/

/*
// Standard hue rotation formula... compacted down a bit.
__DEVICE__ float3 rotHue(float3 p, float a){

    float2 cs = _sinf(to_float2(1.570796f, 0) + a);

    mat3 hr = mat3(0.299f,  0.587f,  0.114f,  0.299f,  0.587f,  0.114f,  0.299f,  0.587f,  0.114f) +
            mat3(0.701f, -0.587f, -0.114f, -0.299f,  0.413f, -0.114f, -0.300f, -0.588f,  0.886f) * cs.x +
            mat3(0.168f,  0.330f, -0.497f, -0.328f,  0.035f,  0.292f,  1.250f, -1.050f, -0.203f) * cs.y;
               
    return clamp(p*hr, 0.0f, 1.0f);
}
*/

// Simple environment mapping. Pass the reflected vector in and create some
// colored noise with it. The normal is redundant here, but it can be used
// to pass into a 3D texture mapping function to produce some interesting
// environmental reflections.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
__DEVICE__ float3 eMap(float3 rd, float3 sn, float iTime){
    
    float3 sRd = rd; // Save rd, just for some mixing at the end.
    
    // Add a time component, scale, then pass into the noise function.
    //swi2(rd,x,y) -= iTime*0.25f;
    rd.x -= iTime*0.25f;
    rd.y -= iTime*0.25f;
    
    
    rd *= 3.0f;
    
    //vec3 tx = tex3D(iChannel0, rd/3.0f, sn);
    //float c = dot(tx*tx, to_float3(0.299f, 0.587f, 0.114f));
    
    float c = n3D(rd)*0.57f + n3D(rd*2.0f)*0.28f + n3D(rd*4.0f)*0.15f; // Noise value.
    c = smoothstep(0.5f, 1.0f, c); // Darken and add contast for more of a spotlight look.
    
    //vec3 col = to_float3(c, c*c, c*c*c*c).zyx; // Simple, warm coloring.
    float3 col = swi3(to_float3(_fminf(c*1.5f, 1.0f), _powf(c, 2.5f), _powf(c, 12.0f)),z,y,x); // More color.
    
    // Mix in some more red to tone it down and return.
    return mix_f3(col, swi3(col,y,z,x), sRd*0.25f+0.25f); 
    
}

__KERNEL__ void PerspexWebLatticeFuse(float4 c, float2 u, float iTime, float2 iResolution, sampler2D iChannel0)
{

    int id = 0; // Object ID - Red perspex: 0; Black lattice: 1.

    // Unit direction ray, camera origin and light position.
    float3 r = normalize(to_float3_aw(u - iResolution*0.5f, iResolution.y)), 
    o = to_float3_s(0), l = o + to_float3(0, 0, -1);
   
    // Rotate the canvas. Note that sine and cosine are kind of rolled into one.
    float2 a = sin_f2(to_float2(1.570796f, 0) + iTime/8.0f); // Fabrice's observation.
    swi2S(r,x,y, mul_mat2_f2(to_mat2(a.x, a.y, -a.y, a.x) , swi2(r,x,y)));

    
    // Standard raymarching routine. Raymarching a slightly perturbed back plane front-on
    // doesn't usually require many iterations. Unless you rely on your GPU for warmth,
    // this is a good thing. :)
    float d, t = 0.0f;
    
    for(int i=0; i<32;i++){
        
        d = m(o + r*t,iTime, &id);
        // There isn't really a far plane to go beyond, but it's there anyway.
        if(_fabs(d)<0.001f || t>FAR) break;
        t += d*0.7f;
    }
    
    t = _fminf(t, FAR);
    
    // Set the initial scene color to black.
    c = to_float4_s(0);
    
    float edge = 0.0f; // Edge value - to be passed into the normal.
    
    float3 _c = to_float3_s(0);
    
    if(t<FAR){
    
        float3 p = o + r*t, n = nr(p, &edge,iTime, &id);

        l -= p; // Light to surface vector. Ie: Light direction vector.
        d = _fmaxf(length(l), 0.001f); // Light to surface distance.
        l /= d; // Normalizing the light direction vector.

 
        // Obtain the height map (destorted Voronoi) value, and use it to slightly
        // shade the surface. Gives a more shadowy appearance.
        float hm = heightMap(p,iTime,&id);
        
        // Texture value at the surface. Use the heighmap value above to distort the
        // texture a bit.
        float3 tx = tex3D(iChannel0, (p*2.0f + hm*0.2f), n);
        //tx = _floor(tx*15.999f)/15.0f; // Quantized cartoony colors, if you get bored enough.

        //swi3(c,x,y,z) = to_float3_s(1.0f)*(hm*0.8f + 0.2f); // Applying the shading to the final color.
        _c = to_float3_s(1.0f)*(hm*0.8f + 0.2f); // Applying the shading to the final color.
        
        _c *= to_float3_s(1.5f)*tx; // Multiplying by the texture value and lightening.

        
        // Color the cell part with a fiery (I incorrectly spell it firey all the time) 
        // palette and the latticey web thing a very dark color.
        //
        _c.x = dot(_c, to_float3(0.299f, 0.587f, 0.114f)); // Grayscale.
        if (id==0) _c *= to_float3(_fminf(_c.x*1.5f, 1.0f), _powf(_c.x, 5.0f), _powf(_c.x, 24.0f))*2.0f;
        else       _c *= 0.1f;
        
        // Hue rotation, for anyone who's interested.
        //swi3(c,x,y,z) = rotHue(swi3(c,x,y,z), mod_f(iTime/16.0f, 6.283f));
       
        
        float df = _fmaxf(dot(l, n), 0.0f); // Diffuse.
        float sp = _powf(_fmaxf(dot(reflect(-l, n), -r), 0.0f), 32.0f); // Specular.
        
        if(id == 1) sp *= sp; // Increase specularity on the dark lattice.
        
        // Applying some diffuse and specular lighting to the surface.
        _c = _c*(df + 0.75f) + to_float3(1, 0.97f, 0.92f)*sp + to_float3(0.5f, 0.7f, 1)*_powf(sp, 32.0f);
        
        // Add the fake environmapping. Give the dark surface less reflectivity.
        float3 em = eMap(reflect(r, n), n, iTime); // Fake environment mapping.
        if(id == 1) em *= 0.5f;
        _c += em;
        
        // Edges.
        //if(id == 0)swi3(c,x,y,z) += edge*0.1f; // Lighter edges.
        _c *= 1.0f - edge*0.8f; // Darker edges.
        
        // Attenuation, based on light to surface distance.    
        _c *= 1.0f/(1.0f + d*d*0.125f);
        
        // AO - The effect is probably too subtle, in this case, so we may as well
        // save some cycles.
        // _c *= cAO(p, n);
        
    }
        
    // Vignette.
    //vec2 uv = u/iResolution;
    //_c = _mix(_c, to_float3(0, 0, 0.5f), 0.1f -_powf(16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.25f)*0.1f);
    
    // Apply some statistically unlikely (but close enough) 2.0f gamma correction. :)
    c = to_float4_aw(sqrt_f3(clamp(_c, to_float3_s(0.0f), to_float3_s(1.0f))), 1.0f);

  SetFragmentShaderComputedColor(c);
}