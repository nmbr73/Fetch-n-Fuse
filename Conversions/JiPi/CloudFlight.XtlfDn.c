
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


/*

  Cloud Flight
  ------------

  Yet another cloud shader. :) However, just to be a little different, this particular one
  takes the camera through the clouds - as opposed to over or under. The scene is supposed 
  to look a little artificial or cartoony, so I wouldn't take it too seriously. :)

  There are plenty of realistic cloud shaders on Shadertoy, so I thought I'd do a stylized
  version. Plus, I had to cheap out on everything, so realism wasn't exactly an option. :)
  The colors and contrast have deliberately been exaggerated to give it more of a surreal 
  feel. The ramped up contrast also serves to allow the viewer to more easily discern the 
  individual cloud shapes. 

  In case anyone was wondering, the cloud shapes were emphasized by assigning a much higher 
  weighting to the larger low frequency base layers. Giving the clouds a denser look was a 
  style choice, but it's possible to make them fluffier and more traditional looking with 
  a few tweaks here and there.

  One of the things you figure out pretty quickly when doing this kind of thing is that when 
  you don't have enough iterations to work with, fake physics tend to work better than real
  physics. For real shadows, you need to traverse toward the light several times. In this 
  example, you're doing so just once, which is bordering on pathetic. :) Therefore, it's 
  necessary to darken the one-tap shadow value to give the impression that more shadowing 
  is happening. However, you'll never produce a real shadow cast, no matter how hard you try.

  It'd be nice to put up an example with volumetric shadowing, etc, but I don't think the
  average GPU is quite fast enough yet. Having said that, I might see what I can come up
  with.
  
  Based on:
  
  Cloudy Spikeball - Duke
    https://www.shadertoy.com/view/MljXDw
    // Port from a demo by Las - Worth watching.
    // http://www.pouet.net/topic.php?which=7920&page=29&x=14&y=9

  // One of my favorite volumetric examples. This one has shadowing. So nice. Fast, all
  // things considered, but still requires a fast machine to run it properly.
  Volumetric Stanford Bunny - SebH
  https://www.shadertoy.com/view/MdlyDs

*/

#define FAR 60.0f


// A between cloud layers look, between the individual clouds, a more conventional fluffy cloud look,
// or right through them. I prefer the latter arrangement, but it doesn't give a clear enough view.
#define ARRANGEMENT 0 // 0, 1, 2 or 3: Layered, path (tunnel in disguise), fluffy layered, just cloud.

// Standard 2D rotation formula - See Nimitz's comment.
__DEVICE__ mat2 r2(in float a){ float c = _cosf(a), s = _sinf(a); return to_mat2(c, s, -s, c); }

// Smooth maximum, based on IQ's smooth minimum.
__DEVICE__ float smax(float a, float b, float s){
    
    float h = clamp(0.5f + 0.5f*(a - b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f - h)*s;
}

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
__DEVICE__ float n3DXXX( in float3 p, __TEXTURE2D__ iChannel0 ){
    
    //return texture(iChannel1, p/24.0f, 0.25f).x;
    
  float3 i = _floor(p); p -= i; p *= p*(3.0f - 2.0f*p);
  swi2S(p,x,y, swi2(texture(iChannel0, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f),y,x)); //lod , -100.0f
  return _mix(p.x, p.y, p.z);
}


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
__DEVICE__ float n3D(float3 p, __TEXTURE2D__ iChannel0, bool NoiseIntern){
    
    if(NoiseIntern)
    {
      float3 i = _floor(p); p -= i; p *= p*(3.0f - 2.0f*p);
      swi2S(p,x,y, swi2(texture(iChannel0, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f),y,x)); //lod , -100.0f
      return _mix(p.x, p.y, p.z);
    }

    // Just some random figures, analogous to stride. You can change this, if you want.
  const float3 s = to_float3(7, 157, 113);
  
  float3 ip = _floor(p); // Unique unit cell ID.
    
    // Setting up the stride vector for randomization and interpolation, kind of. 
    // All kinds of shortcuts are taken here. Refer to IQ's original formula.
    float4 h = to_float4(0.0f, s.y, s.z, s.y + s.z) + dot(ip, s);
    
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
    h = _mix(fract_f4(sin_f4(h)*43758.5453f), fract_f4(sin_f4(h + s.x)*43758.5453f), p.x);
  
    // Interpolating along Y.
    swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y));
    
    // Interpolating along Z, and returning the 3D noise value.
    return _mix(h.x, h.y, p.z); // Range: [0, 1].
  
}





// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
__DEVICE__ float2 path(in float z){ 

    //return to_float2(0); // Straight path.
    return to_float2(_sinf(z*0.075f)*8.0f, _cosf(z*0.1f)*0.75f*2.0f); // Windy path.
    
}

// Distance function. Just some layered noise, and depending on the arrangement, some shapes
// smoothy carved out.
__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0, bool NoiseIntern) {
    
    // Time factor.
    float3 t = to_float3(1, 0.5f, 0.25f)*iTime;

    
    // Two base layers of low fregency noise to shape the clouds. It's been contracted in the Y
    // direction, since a lot clouds seem to look that way.
    float mainLayer = n3D(p*to_float3(0.4f, 1, 0.4f),iChannel0,NoiseIntern)*0.66f + n3D(p*to_float3(0.4f, 1, 0.4f)*2.0f*0.8f,iChannel0,NoiseIntern)*0.34f - 0.0f;    
    
    // Three layers of higher frequency noise to add detail.
    float detailLayer = n3D(p*3.0f + t,iChannel0,NoiseIntern)*0.57f +  n3D(p*6.015f + t*2.0f,iChannel0,NoiseIntern)*0.28f +  n3D(p*12.01f + t*4.0f,iChannel0,NoiseIntern)*0.15f - 0.0f;
    // Two layers, if you're computer can't handle three.
  //float detailLayer = n3D(p*3.0f + t,iChannel0,NoiseIntern)*0.8f +  n3D(p*12.0f + t*4.0f,iChannel0,NoiseIntern)*0.2f;

    // Higher weighting is given to the base layers than the detailed ones.
    float clouds = mainLayer*0.84f + detailLayer*0.16f;
    
    
    #if (ARRANGEMENT != 3)
    // Mapping the hole or plane around the path.
    swi2S(p,x,y, swi2(p,x,y) - path(p.z));
    #endif
    
    
    // Between cloud layers.
    #if (ARRANGEMENT == 0) // Layered.
    //return smax(tn, -_fabs(p.y) + 1.1f + (clouds - 0.5f), 0.5f) + (clouds - 0.5f);
    return smax(clouds, -length(swi2(p,x,y)*to_float2(1.0f/32.0f, 1.0f)) + 1.1f + (clouds - 0.5f), 0.5f) + (clouds - 0.5f);
    #elif (ARRANGEMENT == 1) // Path - Tunnel in disguise.
    // Mapping the hole around the path.
    return smax((clouds - 0.25f)*2.0f, -smax(_fabs(p.x) - 0.5f, _fabs(p.y) - 0.5f, 1.0f), 2.0f);
    #elif (ARRANGEMENT == 2) // Path - Tunnel in disguise.
    // Between layers, but with fluffier clouds.
    return smax(clouds - 0.075f, -length(swi2(p,x,y)*to_float2(1.0f/32.0f, 1.0f)) + 1.1f + (clouds - 0.5f), 0.5f) + (clouds - 0.5f)*0.35f;
    #else // The clouds only.
    return (clouds - 0.25f)*2.0f; 
    //return tn; // Fluffier, but blurrier.
    #endif
}


// Less accurate 4 tap (3 extra taps, in this case) normal calculation. Good enough for this example.
__DEVICE__ float3 fNorm(in float3 p, float d, float iTime, __TEXTURE2D__ iChannel0, bool NoiseIntern){
    
    // Note the large sampling distance.
    float2 e = to_float2(0.075f, 0); 

    // Return the normal.
    return normalize(to_float3(d - map(p - swi3(e,x,y,y),iTime,iChannel0, NoiseIntern), d - map(p - swi3(e,y,x,y),iTime,iChannel0,NoiseIntern), d - map(p - swi3(e,y,y,x),iTime,iChannel0,NoiseIntern)));
}


__KERNEL__ void CloudFlightFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  CONNECT_CHECKBOX0(NoiseExtern, 0);

    
  // Screen coordinates.
  float2 uv = (fragCoord - iResolution*0.5f)/iResolution.y;
    
  // Subtle pixel blur.
  //vec2 hv = mod_f(fragCoord, 2.0f);
  //uv += (1.0f - step(hv, to_float2(1))*2.0f)*1.0f/iResolution.y;


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
    float3 ld = normalize(to_float3(-0.2f, 0.3f, 0.8f));
    
    
    // Using the light position to produce a blueish sky and sun. Pretty standard.
    float3 sky = _mix(to_float3(1, 1, 0.9f), to_float3(0.19f, 0.35f, 0.56f), rd.y*0.5f + 0.5f);
    //sky = _mix(sky, _mix(to_float3(1, 0.8f, 0.7f), to_float3(0.31f, 0.52f, 0.73f), rd.y*0.5f + 0.5f), 0.5f);
    
    
    // Sun position in the sky - Note that the sun has been cheated down a little lower for 
    // aesthetic purposes. All this is fake anyway.
    float3 fakeLd = normalize(to_float3(-0.2f, 0.3f, 0.8f*1.5f));
    float sun = clamp(dot(fakeLd, rd), 0.0f, 1.0f);
    
    
    
    // Combining the clouds, sky and sun to produce the final color.
    sky += to_float3(1, 0.3f, 0.05f)*_powf(sun, 5.0f)*0.25f; 
    sky += to_float3(1, 0.4f, 0.05f)*_powf(sun, 8.0f)*0.35f; 
    sky += to_float3(1, 0.9f, 0.7f)*_powf(sun, 128.0f)*0.5f; 

    // Ramping up the sky contrast a bit.
    sky *= sqrt_f3(sky); 
    
    // I thought I'd mix in a tiny bit of sky color with the clouds here... It seemed like a
    // good idea at the time. :)
    float3 cloudCol = _mix(sky, to_float3(1, 0.9f, 0.8f), 0.66f);
    


    // Raymarching loop.
    for (int i=0; i<64; i++) {

        sp = ro + rd*t; // Current ray position.
        d = map(sp,iTime,iChannel0,NoiseExtern); // Closest distance to the surface... particle.
        
        // Loop break conditions - If the ray hits the surface, the accumulated density maxes out,
        // or if the total ray distance goes beyong the maximum, break.
        if(d<0.001f*(1.0f + t*0.125f) || td>1.0f || t>FAR) break;


        // If we get within a certain distance, "h," of the surface, accumulate some surface values.
        //
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
        //td += w*w*8.0f + 1.0f/64.0f; // More transparent looking... kind of.
        td += w + 1.0f/64.0f; // Looks cleaner, but a little washed out.

       
       
        // Lighting calculations.
        // Standard diffuse calculation, using a more expensive four tap tetrahedral normal.
        // However, this will work with point light and enables are normal-based lighting.
        //float diff = _fmaxf(dot(ld, fNorm(sp, d,iChannel0,NoiseIntern)), 0.0f)*2.0f;
        
        // Directional derivative-based diffuse calculation. Uses only two function taps,
        // but only works with unidirectional light.
        d2 = map(sp + ld*0.1f,iTime,iChannel0,NoiseExtern);
        // Possibly quicker than the line above, but I feel it overcomplicates things... Maybe. 
        //d2 = d<h? map(sp + ld*0.1f,iTime,iChannel0,NoiseIntern) : d;
        float diff = _fmaxf(d2 - d, 0.0f)*20.0f; 
        //float diff = _fmaxf(d2*d2 - d*d, 0.0f)*20.0f; // Slightly softer diffuse effect.
        


        // Accumulating the color. You can do this any way you like.
        //
        // Note that "1. - d2" is a very lame one-tap shadow value - Basically, you're traversing
        // toward the light once. It's artificially darkened more by multiplying by "d," etc, which
        // was made up on the spot. It's not very accurate, but it's better than no shadowing at all.
        // Also note, that diffuse light gives a shadowy feel, but is not shadowing.
        col += w*_fmaxf(d*d*(1.0f - d2)*3.0f - 0.05f, 0.0f)*(diff*cloudCol*2.0f + to_float3(0.95f, 1, 1.05f))*2.5f; // Darker, brooding.
        // Other variations - Tweak them to suit your needs.
        //col += w*d*(_sqrtf(diff)*to_float3(1, 0.85f, 0.7f)*2.0f + 2.0f); // Whiter, softer, fluffier.
        //col += w*d*((1.0f - _expf(-diff*8.0f)*1.25f)*to_float3(1, 0.85f, 0.7f)*2.0f + 2.0f);
        
       
        // Optional extra: Color-based jittering. Roughens up the clouds that hit the camera lens.
        col *= 0.98f + fract(rnd*289.0f + t*41.13f)*0.04f;

        // Enforce minimum stepsize. This is probably the most important part of the procedure.
        // It reminds me a little of of the soft shadows routine.
        t += _fmaxf(d*0.5f, 0.05f); //
        //t += 0.25f; // t += d*0.5f;// These also work - It depends what you're trying to achieve.

    }
    
    // Clamp above zero... It might not need it, but just in case.
    col = _fmaxf(col, to_float3_s(0.0f));
    
    
    // Postprocessing the cloud color just to more evenly match the background. Made up.
    col *= _mix(to_float3_s(1), sky, 0.25f);

    
    // Fogging out the volumetric substance. The fog blend is heavier than usual. It was a style
    // choice - Not sure if was the right one though. :)
    col = _mix(col, sky, smoothstep(0.0f, 0.85f, t/FAR));
    col = _mix(col, sky*sky*2.0f, 1.0f - 1.0f/(1.0f+ t*t*0.001f));//
   //col += to_float3(1, 0.4f, 0.2f)*_powf(sun, 16.0f)*0.25f;   
    
    // More postprocessing. Adding some very subtle fake warm highlights.
    float3 fCol = _mix(pow_f3(to_float3(1.3f, 1, 1)*col, to_float3(1, 2, 10)), sky, 0.5f);
    col = _mix(fCol, col, dot(cos_f3(rd*6.0f +sin_f3(swi3(rd,y,z,x)*6.0f)), to_float3_s(0.333f))*0.1f + 0.9f);
    
    
    // If it were up to me, I'd be ramping up the contrast, but I figured it might be a little to
    // broody for some. :)
    //col *= _sqrtf(col)*1.5f;
 
    
    // Subtle vignette.
    uv = fragCoord/iResolution;
    //col *= _powf(16.0f*uv.x*uv.y*(1.0f - uv.x)*(1.0f - uv.y) , 0.25f)*0.35f + 0.65f;
    // Colored variation.
    col = _mix(pow_f3(_fminf(swi3(to_float3(1.5f, 1, 1),z,y,x)*col, to_float3_s(1.0f)), to_float3(1, 3, 16)), col, 
               _powf(16.0f*uv.x*uv.y*(1.0f - uv.x)*(1.0f - uv.y) , 0.125f)*0.5f + 0.5f);
 
    // Done.
    fragColor = to_float4_aw(sqrt_f3(_fminf(col, to_float3_s(1.0f))), 1.0f);
    

  SetFragmentShaderComputedColor(fragColor);
}