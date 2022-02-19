
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Rusty Metal' to iChannel0




// Hexagon: 0, Dodecahedron: 1, Circle: 2.
// Squares, stars, etc, are possible too, but I didn't include those.
#define SHAPE 0


// Details usually make a scene more interesting. In this case, however, they seemed a
// little expensive, so I left them out.
//
// I wanted to include the grooves, at least, but I figured speed on slower machines was
// more important.
//#define ADD_DETAIL_GROOVE
//#define ADD_DETAIL_BOLT

// Animating the neon lights, or not. I find them a little too distracting,
// so the default is "off."
//#define ANIMATE_LIGHTS

// If Borg green is more your thing. :)
//#define GREEN_GLOW

// Maximum ray distance.
#define FAR 50.0f

// Standard 2D rotation formula.
__DEVICE__ mat2 r2(in float a){ float c = _cosf(a), s = _sinf(a); return mat2(c, -s, s, c); }

// float2 to float hash.
__DEVICE__ float hash21(float2 p){

    float n = dot(p, to_float2(7.163f, 157.247f));
    return fract(_sinf(n)*43758.5453f);
}

// float3 to float hash.
__DEVICE__ float hash31(float3 p){

    float n = dot(p, to_float3(13.163f, 157.247f, 7.951f));
    return fract(_sinf(n)*43758.5453f);
}


__DEVICE__ float smax(float a, float b, float k){

   float f = _fmaxf(0.0f, 1.0f - _fabs(b - a)/k);
   return _fmaxf(a, b) + k*0.25f*f*f;
}


__DEVICE__ float3 tex3D(__TEXTURE2D__ t, in float3 p, in float3 n){

    n = _fmaxf(_fabs(n) - 0.2f, 0.001f); // n = _fmaxf(n*n - 0.1f, 0.001f), etc.
    //n /= dot(n, to_float3(1)); // Rough renormalization approximation.
    n /= length(n); // Renormalizing.

    float3 tx = _tex2DVecN(t, p.y, p.z, 15).xyz; // Left and right sides.
    float3 ty = _tex2DVecN(t, p.z, p.x, 15).xyz; // Top and bottom.
    float3 tz = _tex2DVecN(t, p.x, p.y, 15).xyz; // Front and back.

    return (tx*tx*n.x + ty*ty*n.y + tz*tz*n.z);

}

__DEVICE__ float noise3D(in float3 p){

  const float3 s = to_float3(113.0f, 157.0f, 1.0f);
  float3 ip = _floor(p);

  float4 h = to_float4(0.0f, s.y,s.z, s.y + s.z) + dot(ip, s);

  p -= ip;

  p = p*p*(3.0f - 2.0f*p);

  h = _mix(fract(_sinf(h)*43758.5453f), fract(_sinf(h + s.x)*43758.5453f), p.x);

  float2 uv = _mix(swi2(h,x,z), swi2(h,y,w), p.y);

  float n = _mix(uv.x, uv.y, p.z); // Range: [0, 1].

  return n;
}

// Simple fBm to produce some clouds.
__DEVICE__ float fbm(in float3 p){

    // Four layers of 3D noise.
    //p /= 1.5f;
    //p -= to_float3(0, 0, iTime*1.0f);
    return 0.5333f*noise3D( p ) + 0.2667f*noise3D( p*2.02f ) + 0.1333f*noise3D( p*4.03f ) + 0.0667f*noise3D( p*8.03f );

}



// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
__DEVICE__ float2 path(in float z){

    //return to_float2(0);

    //return to_float2(_sinf(z * 0.15f)*2.4f, _cosf(z * 0.075f)*0.15f);

    return to_float2(_sinf(z * 0.15f)*2.4f, 0);
}




//////
//float objID, svObjID;


// The 2D hexagonal isosuface function: If you were to render a horizontal line and one that
// slopes at 60 degrees, mirror, then combine them, you'd arrive at the following.
__DEVICE__ float hex(in float2 p){

    //return length(p);
    p = _fabs(p);

    // Below is equivalent to:
    return _fmaxf(p.x*0.866025f + p.y*0.5f, p.y);

    //return _fmaxf(dot(p, s*0.5f), p.x); // Hexagon.

}

__DEVICE__ float hexPylon(float2 p2, float pz, float r, float ht){

    float3 p = to_float3(p2.x, pz, p2.y);
    float3 b = to_float3(r, ht, r);


    #if SHAPE == 0
    // Hexagon.
    swi2(p,x,z) = _fabs(swi2(p,x,z));
    swi2(p,x,z) = to_float2(p.x*0.866025f + p.z*0.5f, p.z);
    // The ".015" is a subtle rounding factor. Zero gives sharp edges,
    // and larger numbers give a more rounded look.
    return length(_fmaxf(_fabs(p) - b + 0.015f, 0.0f)) - 0.015f;
    #elif SHAPE == 1
    // Dodecahedron.
    swi2(p,x,z) = _fabs(swi2(p,x,z));
    p2 = swi2(p,x,z)*0.8660254f + swi2(p,z,x)*0.5f;
    swi2(p,x,z) = to_float2(_fmaxf(p2.x, p2.y), _fmaxf(p.z, p.x));
    // The ".015" is a subtle rounding factor. Zero gives sharp edges,
    // and larger numbers give a more rounded look.
    return length(_fmaxf(_fabs(p) - b + 0.015f, 0.0f)) - 0.015f;
    #else
    // Cylinder -- IQ's cylinder function, to be precise, so I think this particular
    // function is a proper distance field.
    swi2(p,x,y) = _fabs(to_float2(length(swi2(p,x,z)), p.y)) - swi2(b,x,y) + 0.015f;
    return _fminf(max(p.x, p.y), 0.0f) + length(_fmaxf(swi2(p,x,y), 0.0f)) - 0.015f;
    #endif


}



__DEVICE__ float objDist(float2 p, float pH, float r, float ht, inout float id, float dir){

    // Neon light height: Four levels, plus the height is divided by two.
    const float s = 1.0f/16.0f; //1.0f/4.0f/2.0f*0.5f;

    // Main hexagon pylon.
    float h1 = hexPylon(p, pH, r, ht);

    #ifdef ADD_DETAIL_GROOVE
    // I like this extra detail, but it was a little too expensive.
    h1 = _fmaxf(h1, -hexPylon(p, pH + ht, r - 0.06f, s/4.0f)); // Extra detail.
    #endif

    #ifdef ADD_DETAIL_BOLT
    // An alternative extra detail. Also a little on the expensive side.
    h1 = _fminf(h1, hexPylon(p, pH, 0.1f, ht + s/4.0f)); // Extra detail.
    #endif



    // Thin hexagon slab -- sitting just below the top of the main hexagon. It's
    // lit differently to represent the neon portion.
    float h2 = hexPylon(p, pH + ht - s, r + 0.01f, s/3.0f);


    // Opens a space around the neon lit hexagon. Used, if the radius of "h2" is
    // less that "h1," which isn't the case here.
    //h1 = smax(h1, -(_fabs(pH + ht - s) - s/3.0f), 0.015f);

    // Identifying the main hexagon pylon or the neon lit portion.
    id = h1<h2? 0.0f : 1.0f;

    // Return the closest object.
    return _fminf(h1, h2);

}

// Height field for the hexagon.
__DEVICE__ float hexHeight(float2 p){

    // Random height.
    //return hash21(p + 57.0f)*0.75f;

    // Any kind of cheap flowing height field will do.
    return dot(_sinf(p*2.0f - _cosf(swi2(p,y,x)*1.4f)), to_float2_s(0.25f)) + 0.5f;


    // Two layers. Not used, because we're trying to keep costs down.
    //float n1 = dot(_sinf(p*2.0f - _cosf(swi2(p,y,x)*1.4f)), to_float2_s(0.25f)) + 0.5f;
    //float n2 = dot(_sinf(swi2(p,y,x)*8.0f - _cosf(p*6.0f)), to_float2_s(0.25f)) + 0.5f;
    //return n1*0.85f + n2*0.15f;
}




__DEVICE__ float4 getHex(float2 p, float pH){

    float4 litID=to_float4_s(0.0f); // ??!?!?!?


    // Helper vector. If you're doing anything that involves regular triangles or hexagons, the
    // 30-60-90 triangle will be involved in some way, which has sides of 1, _sqrtf(3) and 2.
    const float2 s = to_float2(0.866025f, 1.0f);//const float2 s = to_float2(1, 1.7320508f); //



    // The hexagon centers: Two sets of repeat hexagons are required to fill in the space, and
    // the two sets are stored in a "vec4" in order to group some calculations together. The hexagon
    // center we'll eventually use will depend upon which is closest to the current point. Since
    // the central hexagon point is unique, it doubles as the unique hexagon ID.
    float4 hC = _floor(to_float4_f2f2(p, p - to_float2(0, 0.5f))/swi4(s,x,y,x,y)) + to_float4(0, 0, 0, 0.5f);
    float4 hC2 = _floor(to_float4_f2f2(p - to_float2(0.5f, 0.25f), p - to_float2(0.5f, 0.75f))/swi4(s,x,y,x,y)) + to_float4(0.5f, 0.25f, 0.5f, 0.75f);

    // Centering the coordinates with the hexagon centers above.
    float4 h = to_float4_f2f2(p - (swi2(hC,x,y) + 0.5f)*s, p - (swi2(hC,z,w) + 0.5f)*s);
    float4 h2 = to_float4_f2f2(p - (swi2(hC2,x,y) + 0.5f)*s, p - (swi2(hC2,z,w) + 0.5f)*s);

    // Hexagon height.
    float4 ht = to_float4(hexHeight(swi2(hC,x,y)), hexHeight(swi2(hC,z,w)), hexHeight(swi2(hC2,x,y)), hexHeight(swi2(hC2,z,w)));
    // Restricting the heights to five levels... The ".02" was a hack to take out the lights
    // on the ground tiles, or something. :)
    ht = _floor(ht*4.99f)/4.0f/2.0f + 0.02f;

    // The pylon radius. Lower numbers leave gaps, and heigher numbers give overlap. There's not a
    // lot of room for movement, so numbers above ".3," or so give artefacts.
    const float r = 0.25f; // 0.21f to .3.
    float4 obj = to_float4(objDist(swi2(h,x,y), pH, r, ht.x, litID.x, 1.0f), objDist(swi2(h,z,w), pH, r, ht.y, litID.y, -1.0f),
                    objDist(swi2(h2,x,y), pH, r, ht.z, litID.z, -1.0f), objDist(swi2(h2,z,w), pH, r, ht.w, litID.w, 1.0f));


    //tempD = _fminf(min(obj.x, obj.y), _fminf(obj.z, obj.w));

    // Nearest hexagon center (with respect to p) to the current point. In other words, when
    // "h.xy" is zero, we're at the center. We're also returning the corresponding hexagon ID -
    // in the form of the hexagonal central point.
    //
    h = obj.x<obj.y ? to_float4_f2f2(swi2(h,x,y), swi2(hC,x,y)) : to_float4_f2f2(swi2(h,z,w), swi2(hC,z,w));
    h2 = obj.z<obj.w ? to_float4_f2f2(swi2(h2,x,y), swi2(hC2,x,y)) : to_float4_f2f2(swi2(h2,z,w), swi2(hC2,z,w));

    float2 oH = obj.x<obj.y ? to_float2(obj.x, litID.x) : to_float2(obj.y, litID.y);
    float2 oH2 = obj.z<obj.w ? to_float2(obj.z, litID.z) : to_float2(obj.w, litID.w);

    //return oH<oH2 ? to_float4(swi2(h,x,y), swi2(hC,x,y)) : to_float4(swi2(h2,x,y), swi2(hC2,x,y));
    return oH.x<oH2.x ? to_float4_f2f2(oH,  swi2(h,z,w)) : to_float4_f2f2(oH2, swi2(h2,z,w));

}

// Some IDs. One to save the unique hexagonal center coordinates and an ID for the part of the
// pylon that is lit. These were added on the fly. There'd be cleaner ways to do this.
float2 v2Rnd, svV2Rnd;
float gLitID;



// Reducing the heightmap function to a single texel lookup - via the stone texture which was
// generated outside the distance function in the onscreen buffer, of course.
//
// Using the single pass system, there would have been no other option than to generate the stone
// texture several times a frame... or beg someone behind the scenes to provide a 2D multilayered
// Voronoi heightmap. :)
__DEVICE__ float heightMap(in float3 p){

    // The stone texture is tileable, or repeatable, which means the pattern is slightly
    // repetitive, but not too bad, all things considered. Note that the offscreen buffer
    // doesn't wrap, so you have to do that yourself. Ie: fract(p) - Range [0, 1].
    //return Voronoi(swi2(p,x,y)*2.0f);//texture2D(texChannel0, fract(p/2.0f), -100.0f).w;

    const float sc = 1.0f;
    float4 h = getHex(swi2(p,x,z)*sc, -p.y*sc);

    v2Rnd = swi2(h,z,w);

    gLitID = h.y;

    return h.x/sc;

}

///////

// Standard setup for a plane at zero level with a perturbed surface on it.
__DEVICE__ float map(float3 p){

    float c = heightMap(p);

    //objID = 1.0f;

    return c*0.7f;

}

// Global glow variable.
float3 glow;

// Determines whether the neon light should be switched on, or not.
__DEVICE__ float getRndID(float2 p){

    #ifdef ANIMATE_LIGHTS
    // Blinking version. Interesting, but I found it too distracting.
    float rnd = hash21(p);
    return smoothstep(0.5f, 0.875f, _sinf(rnd*6.283f + iTime));
    #else
    return hash21(p) - 0.75f;
    #endif


}

// Standard raymarching routine, with some custom glow mixed in.
__DEVICE__ float trace(float3 ro, float3 rd){

    // Applying some jitter to the jump off point to alleviate volumetric banding.
    float t = hash31(ro + rd)*0.25f, d, ad;

    glow = to_float3_s(0.0f);

    // It's a kind of expensive function, so I'm trying to minimize the iteration number.
    // In fact, since the GPU unrolls everything, this number should always be minimized.
    for (int i = 0; i<80; i++){

        d = map(ro + rd*t);
        ad = _fabs(d);

       if(ad<0.001f*(t*0.125f + 1.0f) || t>FAR) break;

        // Applying some glow. There are probably better ways to go about it, but this
        // will suffice. If the ray passes within "gd" units of the neon object, add some
        // distance-based glow.
        const float gd = 0.1f;
        float rnd = getRndID(v2Rnd);
        if(rnd>0.0f && gLitID == 1.0f && ad<gd) { // && ad<.05
      float gl = 0.2f*(gd - ad)/gd/(1.0f + ad*ad/gd/gd*8.0f);
            // Colors are possible, but I just wanted the scaler value, which is colorized
            // outside the loop.
            glow += gl;
        }

        t += d;  // Advance the ray.
    }


    return _fminf(t, FAR);
}




// Cheap shadows are hard. In fact, I'd almost say, shadowing repeat objects - in a setting like this - with limited
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
__DEVICE__ float softShadow(float3 ro, float3 lp, float k){

    // More would be nicer. More is always nicer, but not really affordable.
    const int maxIterationsShad = 32;

    float3 rd = (lp-ro); // Unnormalized direction ray.

    float shade = 1.0f;
    float dist = 0.01f;
    float end = _fmaxf(length(rd), 0.001f);
    float stepDist = end/float(maxIterationsShad);

    rd /= end;

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest
    // number to give a decent shadow is the best one to choose.
    for (int i=0; i<maxIterationsShad; i++){

        float h = map(ro + rd*dist);
        //shade = _fminf(shade, k*h/dist);
        shade = _fminf(shade, smoothstep(0.0f, 1.0f, k*h/dist)); // Subtle difference. Thanks to IQ for this tidbit.
        //dist += _fminf(h, stepDist); // So many options here, and none are perfect: dist += _fminf( h, 0.2f ), etc
        dist += clamp(h, 0.02f, 0.25f); // So many options here, and none are perfect: dist += _fminf( h, 0.2f ), etc

        // Early exits from accumulative distance function calls tend to be a good thing.
        if (h<0.0f || dist > end) break;
    }

    // I've added 0.5f to the final shade value, which lightens the shadow a bit. It's a preference thing.
    // Really dark shadows look too brutal to me.
    return _fminf(max(shade, 0.0f) + 0.05f, 1.0f);
}



// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical. Due to
// the intricacies of this particular scene, it's kind of needed to reduce jagged effects.
__DEVICE__ float3 getNormal(in float3 p) {
  const float2 e = to_float2(0.0025f, 0);
  return normalize(to_float3(map(p + swi3(e,x,y,y)) - map(p - swi3(e,x,y,y)), map(p + swi3(e,y,x,y)) - map(p - swi3(e,y,x,y)),  map(p + swi3(e,y,y,x)) - map(p - swi3(e,y,y,x))));
}


// Ambient occlusion, for that self shadowed look.
// Based on the original by IQ.
__DEVICE__ float calcAO(in float3 p, in float3 n)
{
  float sca = 4.0f, occ = 0.0f;
    for( int i=1; i<6; i++ ){

        float hr = float(i)*0.125f/5.0f;
        float dd = map(p + hr*n);
        occ += (hr - dd)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - occ, 0.0f, 1.0f);

}


// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
__DEVICE__ float3 texBump( __TEXTURE2D__ tx, in float3 p, in float3 n, float bf){

    const float2 e = to_float2(0.001f, 0);

    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.
    mat3 m = mat3(tex3D(tx, p - swi3(e,x,y,y), n), tex3D(tx, p - swi3(e,y,x,y), n), tex3D(tx, p - swi3(e,y,y,x), n));

    float3 g = to_float3(0.299f, 0.587f, 0.114f)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), to_float3(0.299f, 0.587f, 0.114f)))/e.x;

    // Adjusting the tangent vector so that it's perpendicular to the normal -- Thanks to
    // EvilRyu for reminding me why we perform this step. It's been a while, but I vaguely
    // recall that it's some kind of orthogonal space fix using the Gram-Schmidt process.
    // However, all you need to know is that it works. :)
    g -= n*dot(n, g);

    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.

}


// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it
// does give the impression that the surface is reflecting the surrounds in some way.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95
// https://www.shadertoy.com/view/ldKSDm
__DEVICE__ float3 envMap(float3 p){

    p *= 3.0f;
    //swi2(p,x,z) += iTime*0.5f;

    float n3D2 = noise3D(p*3.0f);

    // A bit of fBm.
    float c = noise3D(p)*0.57f + noise3D(p*2.0f)*0.28f + noise3D(p*4.0f)*0.15f;
    c = smoothstep(0.25f, 1.0f, c); // Putting in some dark space.

    p = to_float3(c, c*c, c*c*c); // Bluish tinge.

    return _mix(p, swi3(p,z,y,x), n3D2*0.25f + 0.75f); // Mixing in a bit of purple.

}


__DEVICE__ float3 getObjectColor(float3 p, float3 n){


    //swi2(p,x,y) -= path(p.z);
    float sz0 = 1.0f/2.0f;

    // Texel retrieval.
    float3 txP = p;
    //swi2(txP,x,z) *= r2(getRndID(svVRnd)*6.2831f);
    float3 col = tex3D(iChannel0, txP*sz0, n );
    col = smoothstep(-0.0f, 0.5f, col);//*to_float3(0.5f, 0.8f, 1.5f);
    col = _mix(col, to_float3(1)*dot(col, to_float3(0.299f, 0.587f, 0.114f)), 0.5f);
    // Darken the surfaces to bring more attention to the neon lights.
    col /= 16.0f;


    // Unique random ID for the hexagon pylon.
    float rnd = getRndID(svV2Rnd);

    // Subtly coloring the unlit hexagons... I wasn't feeling it. :)
    //if(svLitID==1.0f && rnd<=0.0f) col *= to_float3(1, 0.85f, 0.75f)*4.0f;

    // Applying the glow.
    //
    // It's took a while to hit upon the right combination. You can create a cheap lit object
    // effect by simply ramping up the object's color intensity. However, your eyes can tell that
    // it's lacking that volumetric haze. Volumetric haze is achievable via a volumetric appoach.
    // However, it's prone to patchiness. The solutionm, of course, is to combine the smoothness
    // of direct object coloring with a portion of the glow. That's what is happining here.

    // Object glow.
    float oGlow = 0.0f;

    // Color every lit object with a gradient based on its vertical positioning.
    if(rnd>0.0f && svLitID==1.0f) {

        float ht = hexHeight(svV2Rnd);
      ht = _floor(ht*4.99f)/4.0f/2.0f + 0.02f;
        const float s = 1.0f/4.0f/2.0f*0.5f; // Four levels, plus the height is divided by two.

        oGlow = _mix(1.0f, 0.0f, clamp((_fabs(p.y - (ht - s)))/s*3.0f*1.0f, 0.0f, 1.0f));
        oGlow = smoothstep(0.0f, 1.0f, oGlow*1.0f);
    }

    // Mix the object glow in with a small potion of the volumetric glow.
    glow = _mix(glow, to_float3_aw(oGlow), 0.75f);

    // Colorizing the glow, depending on your requirements. I've used a colorful orangey palette,
    // then have modified the single color according to a made up 3D transcental function.
    //glow = _powf(to_float3(1, 1.05f, 1.1f)*glow.x, to_float3(6, 3, 1));
    glow = _powf(to_float3(1.5f, 1, 1)*glow, to_float3(1, 3, 6)); // Mild firey orange.
    glow = _mix(glow, swi3(glow,x,z,y), dot(_sinf(p*4.0f - _cosf(swi3(p,y,z,x)*4.0f)), to_float3_s(0.166f)) + 0.5f); // Mixing in some pink.
    glow = _mix(glow, swi3(glow,z,y,x), dot(_cosf(p*2.0f - _sinf(swi3(p,y,z,x)*2.0f)), to_float3_s(0.166f)) + 0.5f); // Blue tones.
    //glow = _mix(swi3(glow,z,y,x), glow, smoothstep(-0.1f, 0.1f, dot(_sinf(p + _cosf(swi3(p,y,z,x))), to_float3_s(0.166f))));

    #ifdef GREEN_GLOW
    glow = swi3(glow,y,x,z);
    #endif


    return col;

}


// Using the hit point, unit direction ray, etc, to color the
// scene. Diffuse, specular, falloff, etc. It's all pretty
// standard stuff.
__DEVICE__ float3 doColor(in float3 sp, in float3 rd, in float3 sn, in float3 lp, in float t){

    float3 sceneCol = to_float3(0.0f);

    if(t<FAR){

           // Texture bump the normal.
      float sz0 = 1.0f/1.0f;
      float3 txP = sp;
        //swi2(txP,x,y) -= path(txP.z);
        //swi2(txP,x,z) *= r2(getRndID(svVRnd)*6.2831f);
        sn = texBump(iChannel0, txP*sz0, sn, 0.005f);///(1.0f + t/FAR)


        // Retrieving the normal at the hit point.
        //sn = getNormal(sp);
        float sh = softShadow(sp, lp, 12.0f);
        float ao = calcAO(sp, sn);
        sh = _fminf(sh + ao*0.3f, 1.0f);

        float3 ld = lp - sp; // Light direction vector.
        float lDist = _fmaxf(length(ld), 0.001f); // Light to surface distance.
        ld /= lDist; // Normalizing the light vector.

        // Attenuating the light, based on distance.
        float atten = 1.5f/(1.0f + lDist*0.1f + lDist*lDist*0.02f);

        // Standard diffuse term.
        float diff = _fmaxf(dot(sn, ld), 0.0f);
        //if(svLitID == 0.0f) diff = _powf(diff, 4.0f)*2.0f;
        // Standard specualr term.
        float spec = _powf(_fmaxf( dot( reflect(-ld, sn), -rd ), 0.0f ), 32.0f);
        float fres = clamp(1.0f + dot(rd, sn), 0.0f, 1.0f); // Fresnel reflection term.
        //float Schlick = _powf( 1.0f - _fmaxf(dot(rd, normalize(rd + ld)), 0.0f), 5.0f);
        //float fre2 = _mix(0.5f, 1.0f, Schlick);  //F0 = .5.



        // Coloring the object. You could set it to a single color, to
        // make things simpler, if you wanted.
        float3 objCol = getObjectColor(sp, sn);


        // Combining the above terms to produce the final scene color.
        sceneCol = objCol*(diff + to_float3(1, 0.6f, 0.3f)*spec*4.0f + 0.5f*ao + to_float3(0.3f, 0.5f, 1)*fres*fres*2.0f);

        // Fake environment mapping.
        sceneCol += _powf(sceneCol, to_float3_s(1.0f))*envMap(reflect(rd, sn))*4.0f;


        // Applying the shadows and ambient occlusion.
        sceneCol *= atten*sh*ao;

        // For whatever reason, I didn't want the shadows and such to effect the glow, so I layered
        // it over the top.
        sceneCol += (objCol*6.0f + 1.0f)*glow; //*(sh*0.35f + 0.65f);

        //sceneCol = to_float3(sh);

    }




    // Return the color. Done once every pass... of which there are
    // only two, in this particular instance.
    return sceneCol;

}




__KERNEL__ void NeonLitHexagonsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

float svLitID;

    // Screen coordinates.
  float2 uv = (fragCoord - iResolution*0.5f) / iResolution.y;


  // Camera Setup.
  //vec3 lookAt = to_float3(0.0f, 0.25f, iTime*2.0f);  // "Look At" position.
  //vec3 camPos = lookAt + to_float3(2.0f, 1.5f, -1.5f); // Camera position, doubling as the ray origin.

  float3 lk = to_float3(0.0f, 1.25f, iTime*2.0f);  // "Look At" position.
  float3 ro = lk + to_float3(0.0f, 0.175f, -0.25f); // Camera position, doubling as the ray origin.


    // Light position. Set in the vicinity the ray origin.
    float3 lp = ro + to_float3(0.0f, 1.0f, 4.0f); //4

  // Using the Z-value to perturb the XY-plane.
  // Sending the camera, "look at," and two light vectors down the tunnel. The "path" function is
  // synchronized with the distance function. Change to "path2" to traverse the other tunnel.
  swi2(lk,x,y) += path(lk.z);
  swi2(ro,x,y) += path(ro.z);
  swi2(lp,x,y) += path(lp.z);

    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159f/3.0f; // FOV - Field of view.
    float3 forward = normalize(lk-ro);
    float3 right = normalize(to_float3(forward.z, 0.0f, -forward.x ));
    float3 up = cross(forward, right);

    // rd - Ray direction.
    float3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);

    // Camera lean.
    //swi2(rd,x,y) *= r2(path(lk.z).x/32.0f);
    /////////


    float3 sceneColor, passColor, sn, sSn;



    // FIRST PASS.

    float t = trace(ro, rd);
    svV2Rnd = v2Rnd;
    svLitID = gLitID;


    //getGlow(ro, rd, t);

    // Fog based off of distance from the camera. Not used here.
    float fog = smoothstep(0.0f, FAR-1.0f, t);

    // Advancing the ray origin, "ro," to the new hit point.
    ro += rd*t;

    // Retrieving the normal at the hit point.
    //sn = getNormal(ro);
    //float edge = 0.0f, crv = 1.0f, ef = 5.0f;
  //sn = getNormal(ro, edge, crv, ef);//
    //sSn = sn; // Save the unpeturbed normal.
    sn = getNormal(ro);



    // Retrieving the color at the hit point, which is now "ro." I agree, reusing
    // the ray origin to describe the surface hit point is kind of confusing. The reason
    // we do it is because the reflective ray will begin from the hit point in the
    // direction of the reflected ray. Thus the new ray origin will be the hit point.
    // See "traceRef" below.
    passColor = doColor(ro, rd, sn, lp, t);
    sceneColor = passColor;//*(1.0f - edge*0.8f);//_mix(passColor, to_float3_aw(0), fog); //



    // Shading. Shadows, ambient occlusion, etc. We're only performing this on the
    // first pass. Not accurate, but faster, and in most cases, not that noticeable.
    //float sh = softShadow(ro, lp, 12.0f);
    //sh *= calcAO(ro, sn);

/*
    // SECOND PASS - REFLECTED RAY

    // Standard reflected ray, which is just a reflection of the unit
    // direction ray off of the intersected surface. You use the normal
    // at the surface point to do that. Hopefully, it's common sense.
    rd = reflect(rd, normalize(sSn*0.66f + sn*0.34f));




    // The reflected pass begins where the first ray ended, which is the suface
    // hit point, or in a few cases, beyond the far plane. By the way, for the sake
    // of simplicity, we'll perform a reflective pass for non hit points too. Kind
    // of wasteful, but not really noticeable. The direction of the new ray will
    // obviously be in the direction of the reflected ray. See just above.
    //
    // To anyone who's new to this, don't forgot to nudge the ray off of the
    // initial surface point. Otherwise, you'll intersect with the surface
    // you've just hit. After years of doing this, I still forget on occasion.
    t = traceRef(ro +  rd*0.01f, rd);
    svVRnd = vRnd;
    svObjID = objID;

    // Advancing the reflected ray origin, "ro," to the new hit point.
    ro += rd*t;

    // Retrieving the new normal at the reflected hit point.
    //sn = getNormal(ro);
    float edge2 = 0.0f, crv2 = 1.0f;//, ef2 = 8.0f;
  sn = getNormal(ro, edge2, crv2, ef);//getNormal(sp);


    // Coloring the reflected hit point, then adding a portion of it to the final scene color.
    // How much you add depends on what you're trying to accomplish.
    passColor = doColor(ro, rd, sn, lp, t);
    sceneColor = sceneColor*0.5f + passColor*(1.0f - edge2*0.8f);//_mix(passColor, to_float3_aw(0), fog);

*/

    //sceneColor *= (1.0f - edge*0.8f);


    // APPLYING SHADOWS
    //
    // Multiply the shadow from the first pass by the final scene color. Ideally, you'd check to
    // see if the reflected point was in shadow, and incorporate that too, but we're cheating to
    // save cycles and skipping it. It's not really noticeable anyway. By the way, ambient
    // occlusion would make it a little nicer, but we're saving cycles and keeping things simple.
    //sceneColor *= sh;

    sceneColor = _mix(sceneColor, to_float3_aw(0), fog);


    // Square vignette.
    uv = fragCoord/iResolution;
    sceneColor = _fminf(sceneColor, 1.0f)*_powf( 16.0f*uv.x*uv.y*(1.0f - uv.x)*(1.0f - uv.y) , 0.0625f);

    // Clamping the scene color, then presenting to the screen.
  fragColor = to_float4(_sqrtf(clamp(sceneColor, 0.0f, 1.0f)), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}