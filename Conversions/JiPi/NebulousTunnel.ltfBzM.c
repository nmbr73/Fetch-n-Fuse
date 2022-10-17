
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*

  Nebulous Tunnel
  ---------------

    Volumetrically raymarching an organic distance field to produce a tunnel winding
  through a nebulous substrate... I'm not entirely sure how to describe it, but I
  guess it resembles a microbial dust flythough. :) Either way, there's nothing new 
  here - I just thought it'd be fun to make.  

  There are two common ways to apply volumetrics to a distance field: One is to
  step evenly through the field accumulating layer colors at each point. Layers 
  are assigned a weight according to the distance from the viewer. 

  The other method is to sphere-trace to the surface, then accumulate color once
  the ray crosses a certain surface distance threshold. The color weighing is 
  calculated according to distance from the surface.  

  The first method gives an overall gaseous kind of effect. The latter method 
  displays more of the underlying surface and is the one I'm using here. You can 
  also use hybrids of the two. There's no right or wrong way to do it. It all 
  depends on what you're trying to achieve.

  Volumetric calculations are generally more expensive, due to the number of steps
  involved, and the need to calculate lighting at each one. In a perfect world, 
  you'd calculate normals (numerically, or analytically, if possible), etc, to give 
  better results. Unfortunately, that's expensive, so it's common to use a cheap 
  (directional derivative) lighting trick to get the job done. If you're interested, 
  IQ explains it here:

    Directional Derivative - iquilezles.org/articles/derivative

  I've tried to keep the GPU workload down do a dull roar, but that was at the
  expense of quality via various detail sacrifices. However, hopefully, it will run 
  at a reasonal pace on moderate systems. By the way, if you prefer a slighly more 
  conventional look, uncomment the "WHITE_FLUFFY_CLOUDS" and the "BETWEEN_LAYERS" 
  define.
  
  
  Based on:
  
  Cloudy Spikeball - Duke
    https://www.shadertoy.com/view/MljXDw
    // Port from a demo by Las - Worth watching.
    // http://www.pouet.net/topic.php?which=7920&page=29&x=14&y=9

*/

#define FAR 50.0f

// More conventional look. Probably more pleasing to the eye, but a vacuum cleaner 
// dust flythrough was the look I was going for. :D
//#define WHITE_FLUFFY_CLOUDS

// A between cloud layers look. Works better with the white clouds. Needs work, but
// it's there to give you a different perspective.
//#define BETWEEN_LAYERS

// Fabrice's concise, 2D rotation formula.
//mat2 r2(float th){ float2 a = _sinf(to_float2(1.5707963f, 0) + th); return mat2(a, -a.y, a.x); }
// Standard 2D rotation formula - See Nimitz's comment.
__DEVICE__ mat2 r2(in float a){ float c = _cosf(a), s = _sinf(a); return to_mat2(c, s, -s, c); }

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
__DEVICE__ float3 hash33(float3 p){ 
    float n = _sinf(dot(p, to_float3(7, 157, 113)));    
    return fract_f3(to_float3(2097152, 262144, 32768)*n); 
}


// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
__DEVICE__ float n3D( in float3 p, __TEXTURE2D__ iChannel0 ){
    
  //return texture(iChannel1, p/24.0f, 0.25f).x;
    
  float3 i = _floor(p); p -= i; p *= p*(3.0f - 2.0f*p);
  swi2S(p,x,y, swi2(texture(iChannel0, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f),y,x));
  return _mix(p.x, p.y, p.z);
}

/*
// Textureless 3D Value Noise:
//
// This is a rewrite of IQ's original. It's self contained, which makes it much
// easier to copy and paste. I've also tried my best to minimize the amount of 
// operations to lessen the work the GPU has to do, but I think there's room for
// improvement. I have no idea whether it's faster or not. It could be slower,
// for all I know, but it doesn't really matter, because in its current state, 
// it's still no match for IQ's texture-based, smooth 3D value noise.
//
// By the way, a few people have managed to reduce the original down to this state, 
// but I haven't come across any who have taken it further. If you know of any, I'd
// love to hear about it.
//
// I've tried to come up with some clever way to improve the randomization line
// (h = _mix(fract...), but so far, nothing's come to mind.
__DEVICE__ float n3D(float3 p){
    
    // Just some random figures, analogous to stride. You can change this, if you want.
  const float3 s = to_float3(7, 157, 113);
  
  float3 ip = _floor(p); // Unique unit cell ID.
    
    // Setting up the stride vector for randomization and interpolation, kind of. 
    // All kinds of shortcuts are taken here. Refer to IQ's original formula.
    float4 h = to_float4(0.0f, swi2(s,y,z), s.y + s.z) + dot(ip, s);
    
  p -= ip; // Cell's fractional component.
  
    // A bit of cubic smoothing, to give the noise that rounded look.
    p = p*p*(3.0f - 2.0f*p);
    
    // Smoother version of the above. Weirdly, the extra calculations can sometimes
    // create a surface that's easier to hone in on, and can actually speed things up.
    // Having said that, I'm sticking with the simpler version above.
  //p = p*p*p*(p*(p * 6.0f - 15.0f) + 10.0f);
    
    // Even smoother, but this would have to be slower, surely?
  //vec3 p3 = p*p*p; p = ( 7.0f + ( p3 - 7.0f ) * p ) * p3;  
  
    // Cosinusoidal smoothing. OK, but I prefer other methods.
    //p = 0.5f - 0.5f*_cosf(p*3.14159f);
    
    // Standard 3D noise stuff. Retrieving 8 random scalar values for each cube corner,
    // then interpolating along X. There are countless ways to randomize, but this is
    // the way most are familar with: fract(_sinf(x)*largeNumber).
    h = _mix(fract(_sinf(h)*43758.5453f), fract(_sinf(h + s.x)*43758.5453f), p.x);
  
    // Interpolating along Y.
    swi2(h,x,y) = _mix(swi2(h,x,z), swi2(h,y,w), p.y);
    
    // Interpolating along Z, and returning the 3D noise value.
    return _mix(h.x, h.y, p.z); // Range: [0, 1].
  
}
*/


// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
__DEVICE__ float trigNoise3D(in float3 p, float iTime, __TEXTURE2D__ iChannel0){
    
    float res = 0.0f, sum = 0.0f;
    
    // IQ's cheap, texture-lookup noise function. Very efficient, but still 
    // a little too processor intensive for multiple layer usage in a largish 
    // "for loop" setup. Therefore, just a couple of layers are used here.
    //float n = n3D(p*8.0f + iTime*0.2f); // Not great.
    float n = n3D(p*6.0f + iTime*0.2f, iChannel0)*0.67f +  n3D(p*12.0f + iTime*0.4f, iChannel0)*0.33f; // Compromise.
    // Nicer, but I figured too many layers was pushing it. :)
    //float n = n3D(p*6.0f + iTime*0.2f)*0.57f +  n3D(p*12.0f + iTime*0.4f)*0.28f +  n3D(p*24.0f + iTime*0.8f)*0.15f;

    // Two sinusoidal layers. I'm pretty sure you could get rid of one of 
    // the swizzles (I have a feeling the GPU doesn't like them as much), 
    // which I'll try to do later.
    
    float3 t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f + 3.14159265f/4.0f))*0.5f + 0.5f;
    p = p*1.5f + (t - 1.5f); //  + iTime*0.1
    res += (dot(t, to_float3_s(0.333f)));

    t = sin_f3(swi3(p,y,z,x)*3.14159265f + cos_f3(swi3(p,z,x,y)*3.14159265f + 3.14159265f/4.0f))*0.5f + 0.5f;
    res += (dot(t, to_float3_s(0.333f)))*0.7071f; 
       
  return ((res/1.7071f))*0.85f + n*0.15f;
}


// Smooth maximum, based on IQ's smooth minimum.
__DEVICE__ float smax(float a, float b, float s){
    
    float h = clamp(0.5f + 0.5f*(a - b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f - h)*s;
}

// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
__DEVICE__ float2 path(in float z){ 

    //return to_float2(0); // Straight path.
    return to_float2(_sinf(z*0.075f)*8.0f, _cosf(z*0.1f)*0.75f); // Windy path.
}

// Distance function.
__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0) {
    
    // Cheap and nasty fBm emulation. Two noise layers and a couple of sinusoidal layers.
    // Sadly, you get what you pay for. :) Having said that, it works fine here.
    float tn = trigNoise3D(p*0.5f, iTime, iChannel0);
    
    // Mapping the tunnel around the path.
    swi2S(p,x,y, swi2(p,x,y) - path(p.z));
    
#ifndef BETWEEN_LAYERS
    // Smoothly carve out the windy tunnel path from the nebulous substrate.
    return smax(tn - 0.025f, -length(swi2(p,x,y)) + 0.25f, 2.0f);
#else
    // Between cloud layers... I guess. The "trigNoise3D" function above would have
    // to be reworked to look more like clouds, but it gives you a rough idea.
    return smax(tn - 0.05f, -_fabs(p.y - sign_f(p.y)*(tn - 0.5f)) + 0.125f, 2.0f);
#endif


}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ.
__DEVICE__ float3 fNorm(in float3 p){

    // Note the large sampling distance.
    float2 e = to_float2(0.005f, -0.005f); 
    return normalize(swi3(e,x,y,y) * map(p + swi3(e,x,y,y)) + swi3(e,y,y,x) * map(p + swi3(e,y,y,x)) + swi3(e,y,x,y) * map(p + swi3(e,y,x,y)) + swi3(e,x,x,x) * map(p + swi3(e,x,x,x)));
}
*/

// Less accurate 4 tap (3 extra taps, in this case) normal calculation. Good enough for this example.
__DEVICE__ float3 fNorm(in float3 p, float d, float iTime, __TEXTURE2D__ iChannel0){
    
    // Note the large sampling distance.
    float2 e = to_float2(0.01f, 0); 

    // Return the normal.
    return normalize(to_float3(d - map(p - swi3(e,x,y,y),iTime, iChannel0), d - map(p - swi3(e,y,x,y), iTime, iChannel0), d - map(p - swi3(e,y,y,x), iTime, iChannel0)));
}

__KERNEL__ void NebulousTunnelFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    
  // Screen coordinates.
  float2 uv = (fragCoord - iResolution*0.5f)/iResolution.y;

  // Ray origin. Moving along the Z-axis.
  float3 ro = to_float3(0, 0, iTime*4.0f);
  float3 lk = ro + to_float3(0, 0, 0.25f);  // "Look At" position.
   
  // Using the Z-value to perturb the XY-plane.
  // Sending the camera, "look at," and light vector down the path. The "path" function is 
  // synchronized with the distance function.
  swi2S(ro,x,y, swi2(ro,x,y) + path(ro.z));
  swi2S(lk,x,y, swi2(lk,x,y) + path(lk.z));

    
    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159f/2.75f; // FOV - Field of view.
    float3 forward = normalize(lk-ro);
    float3 right = normalize(to_float3(forward.z, 0.0f, -forward.x )); 
    float3 up = cross(forward, right);

    // rd - Ray direction.
    float3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    //rd = normalize(to_float3(swi2(rd,x,y), rd.z - length(swi2(rd,x,y))*0.15f));
    
    // Camera swivel - based on path position.
    float2 sw = path(lk.z);
    swi2S(rd,x,y, mul_f2_mat2(swi2(rd,x,y) , r2(-sw.x/24.0f)));
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z) , r2(-sw.y/16.0f)));
float zzzzzzzzzzzzzzzzz;    

    // The ray is effectively marching through discontinuous slices of noise, so at certain
    // angles, you can see the separation. A bit of randomization can mask that, to a degree.
    // At the end of the day, it's not a perfect process. Anyway, the hash below is used to
    // at jitter to the jump off point (ray origin).
    //    
    // It's also used for some color based jittering inside the loop.
    float3 rnd = hash33(swi3(rd,y,z,x) + fract(iTime));

    // Local density, total density, and weighting factor.
    float lDen = 0.0f, td = 0.0f, w = 0.0f;

    // Closest surface distance, a second sample distance variable, and total ray distance 
    // travelled. Note the comparitively large jitter offset. Unfortunately, due to cost 
    // cutting (64 largish steps, it was  necessary to get rid of banding.
    float d = 1.0f, d2 = 0.0f, t = dot(rnd, to_float3_s(0.333f));

    // Distance threshold. Higher numbers give thicker clouds, but fill up the screen too much.    
    const float h = 0.5f;


    // Initializing the scene color to black, and declaring the surface position vector.
    float3 col = to_float3_s(0), sp;
    
    // Directional light. Don't quote me on it, but I think directional derivative lighting
    // only works with unidirectional light... Thankfully, the light source is the cun which 
    // tends to be unidirectional anyway.
    float3 ld = normalize(to_float3(-0.2f, 0.3f, 0.4f));
    
    // Using the light position to produce a blueish sky and sun. Pretty standard.
    float3 sky = to_float3(0.6f, 0.8f, 1.0f)*_fminf((1.5f+rd.y*0.5f)/2.0f, 1.0f);   
    sky = _mix(to_float3(1, 1, 0.9f), to_float3(0.31f, 0.42f, 0.53f), rd.y*0.5f + 0.5f);
    //sky = _mix(to_float3(1, 0.8f, 0.7f), to_float3(0.31f, 0.52f, 0.73f), rd.y*0.5f + 0.5f);
    
    
    // Sun position in the sky - Note that the sun has been cheated down a little lower for 
    // aesthetic purposes. All this is fake anyway.
    float sun = clamp(dot(normalize(to_float3(-0.2f, 0.3f, 0.4f*4.0f)), rd), 0.0f, 1.0f);
    
    
    // Combining the clouds, sky and sun to produce the final color.
    sky += to_float3(1, 0.3f, 0.05f)*_powf(sun, 5.0f)*0.25f; 
    sky += to_float3(1, 0.4f, 0.05f)*_powf(sun, 16.0f)*0.35f; 


    // Raymarching loop.
    for (int i=0; i<64; i++) {

        sp = ro + rd*t; // Current ray position.
        d = map(sp, iTime, iChannel0); // Closest distance to the surface... particle.
        
        // Loop break conditions - If the ray hits the surface, the accumulated density maxes out,
        // or if the total ray distance goes beyong the maximum, break.
        if(d<0.001f*(1.0f + t*0.125f) || td>1.0f || t>FAR) break;

        // If we get within a certain distance, "h," of the surface, accumulate some surface values.
        //
        // Values further away have less influence on the total. When you accumulate layers, you'll
        // Values further away have less influence on the total. When you accumulate layers, you'll
        // usually need some kind of weighting algorithm based on some identifying factor - in this
        // case, it's distance. This is one of many ways to do it. In fact, you'll see variations on 
        // the following lines all over the place.
        //
        // On a side note, you could wrap the next few lines in an "if" statement to save a
        // few extra "map" calls, etc. However, some cards hate branching, nesting, etc, so it
        // could be a case of diminishing returns... Not sure what the right call is, so I'll 
        // leave it to the experts. :)
        w = d<h? (1.0f - td)*(h - d) : 0.0f;   

        // Use the weighting factor to accumulate density. How you do this is up to you. 
        //td += w*w*8.0f + 1.0f/60.0f; // More transparent looking... kind of.
        td += w + 1.0f/64.0f; // Looks cleaner, but a little washed out.

       
        // Lighting calculations.
        // Standard diffuse calculation using a cheap 4 tap normal. "d" is passed in, so that means 
        // only 3 extra taps. It's more expensive than 2 tap (1 extra tap) directional derivative
        // lighting. However, it will work with point light, and enables better lighting.
        //float diff = _fmaxf(dot(ld, fNorm(sp, d)), 0.0f);
        
        // Directional derivative-based diffuse calculation. Uses only two function taps,
        // but only works with unidirectional light. By the way, the "1 + t*.125" is a fake
        // tweak to hightlight further down the tunnel, but it doesn't belong there. :)
        d2 = map(sp + ld*0.02f*(1.0f + t*0.125f), iTime, iChannel0);
        // Possibly quicker than the line above, but I feel it overcomplicates things... Maybe. 
        //d2 = d<h? map(sp + ld*0.02f*(1.0f + t*0.125f), iTime,iChannel0) : d;
        float diff = _fmaxf(d2 - d, 0.0f)*50.0f*(1.0f + t*0.125f);
        //float diff = _fmaxf(d2*d2 - d*d, 0.0f)*100.0f; // Slightly softer diffuse effect.
        

        // Accumulating the color. You can do this any way you like.
        //
    #ifdef WHITE_FLUFFY_CLOUDS
        // I wanted dust, but here's something fluffier - The effects you can create are endless.
        col += w*d*(diff*to_float3(1, 0.85f, 0.7f) + 2.5f)*1.25f;
        // Other variations - Tweak them to suit your needs.
        //col += w*d*(_sqrtf(diff)*to_float3(1, 0.85f, 0.7f)*2.0f + 3.0f);
        //col += w*d*((1.0f-_expf(-diff*8.0f))*to_float3(1, 0.85f, 0.7f)*1.5f + 2.5f);
        
        
    #else
        col += w*d*(0.5f + diff*to_float3(1, 0.8f, 0.6f)*4.0f);
    #endif
        
        // Optional extra: Color-based jittering. Roughens up the clouds that hit the camera lens.
        col *= 0.98f + fract_f3(rnd*289.0f + t*41.13f)*0.04f;

        // Enforce minimum stepsize. This is probably the most important part of the procedure.
        // It reminds me a little of of the soft shadows routine.
        t += _fmaxf(d*0.5f, 0.05f); //
        //t += 0.25f; // t += d*0.5f;// These also work - It depends what you're trying to achieve.

    }
    
    col = _fmaxf(col, to_float3_s(0.0f));
    
    
  #ifndef WHITE_FLUFFY_CLOUDS
    // Adding a bit of a firey tinge to the volumetric substance.
    col = _mix(pow_f3(to_float3(1.3f, 1, 1)*col, to_float3(1, 2, 10)), col, dot(cos_f3(rd*6.0f + sin_f3(swi3(rd,y,z,x)*6.0f)), to_float3_s(0.333f))*0.2f + 0.8f);
  #endif
 
    
    
    // Fogging out the volumetric substance. The fog blend is heavier than usual. It was a style
    // choice - Not sure if was the right one though. :)
    col = _mix(col, sky, smoothstep(0.0f, 0.55f, t/FAR));
    col += to_float3(1, 0.4f, 0.05f)*_powf(sun, 16.0f)*0.25f;   
    
    // Tweaking the contrast.
    col = pow_f3(col, to_float3_s(1.5f));
    
    // Subtle vignette.
    uv = fragCoord/iResolution;
    col *= _powf(16.0f*uv.x*uv.y*(1.0f - uv.x)*(1.0f - uv.y) , 0.25f)*0.35f + 0.65f;
    // Colored varation.
    //col = _mix(_powf(_fminf(to_float3(1.5f, 1, 1).zyx*col, 1.0f), to_float3(1, 3, 16)), col, 
               //_powf(16.0f*uv.x*uv.y*(1.0f - uv.x)*(1.0f - uv.y) , 0.125f)*0.5f + 0.5f);
 
    // Done.
    fragColor = to_float4_aw(sqrt_f3(_fminf(col, to_float3_s(1.0f))), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}