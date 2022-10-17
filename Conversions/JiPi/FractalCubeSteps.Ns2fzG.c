
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0

#define R iResolution

/*

  Fractal Cube Steps
  ------------------

  I'm sure everyone's seen the hexagon cube examples, and probably coded
    one before. It's a great 2D effect. The fractal cube example is a simple 
    extension on that. The 3D version is less common, but for anyone curious
    as to what that would look like, here it is. There are so many ways to 
    code one of these, and I've coded them all at one time or another, but 
    this is my preferred method.
    
    The first iteration comprises two lines of repeat cubes rotated about
    the X-axis by a ceratin amount and then rotated about the Y axis by 
    another. Determining the correct rotations can be annoying, but
    luckily for me, a younger version of myself got out a pen and paper and
    figured them out years ago, so I only had to copy them. :)
    
    The top layers consist of single offset-row repeat grids containing
    smaller cubes offset and rotated at the same fixed amounts as the first 
    layer... The code in the "map" function should make it clearer.
    
    Anyway, there's not a lot to this, but it might be interesting for 
    anyone who's ever wanted to make one of these.
    
    
   
    Other examples:

  // You can't have a cube example without referencing this. It's all
    // over the internet these days. :)
    Cubes and Spheres - Paulo Falcao
  https://www.shadertoy.com/view/MsX3zr

    // Another isometric style rendering. 
    Cubes are dancing - Flopine
    https://www.shadertoy.com/view/tttBRX
    
    // A 3D cube wall rendered with 2D techniques. Very cool.
    Cyberspace data warehouse - bitless
    https://www.shadertoy.com/view/NlK3Wt

*/

#define ZERO 0 //_fminf(0, iFrame)

// Far plane, or max ray distance.
#define FAR 40.0f

// Minimum surface distance. Used in various calculations.
#define DELTA 0.001f


// Ray passes: For this example, just one intersection and one reflection.
#define PASSES 2

// Global block scale. Something that is obvious to me now is that rotated
// cubes fill in cells that have dimensional rations consisting of the diagonal 
// face length along the X direction the the longest diagonal along the Y direction.
// That ratio turns out to be _sqrtf(3)/2.0f Common sense, and obvious... once I'd
// spent ages trying to fit a square peg into a round hole. :)
//
// The divisor, "4", is variable global scaling.
#define GSCALE to_float2(1.0f, 0.8660254f)/4.0f 





// Standard 2D rotation formula.
__DEVICE__ mat2 rot2(in float a){ float c = _cosf(a), s = _sinf(a); return to_mat2(c, -s, s, c); }

// IQ's float2 to float hash.
__DEVICE__ float hash21(float2 p){  return fract(_sinf(dot(p, to_float2(27.609f, 57.583f)))*43758.5453f); }

// Getting the video texture. I've deliberately stretched it out to fit across the screen,
// which means messing with the natural aspect ratio.
//
// By the way, it'd be nice to have a couple of naturally wider ratio videos to choose from. :)
//
__DEVICE__ float3 getTex(__TEXTURE2D__ tex, float2 p){
    
    // Strething things out so that the image fills up the window. You don't need to,
    // but this looks better. I think the original video is in the oldschool 4 to 3
    // format, whereas the canvas is along the order of 16 to 9, which we're used to.
    // If using repeat textures, you'd comment the first line out.
    //p *= to_float2(iResolution.y/iResolution.x, 1);
    float3 tx = swi3(_tex2DVecN(tex,p.x,p.y,15),x,y,z);
    return tx*tx; // Rough sRGB to linear conversion.
}




// IQ's signed box formula.
__DEVICE__ float sBoxS(in float3 p, in float3 b, in float sf){

  p = abs_f3(p) - b + sf;
  return length(_fmaxf(p, to_float3_s(0.0f))) + _fminf(max(_fmaxf(p.x, p.y), p.z), 0.0f) - sf;
}

/*
// This is a trimmed down version of one of Gaz's clever routines. I find it a 
// lot cleaner than those functions full of a million trigonometric variables.
__DEVICE__ float3 rot3(in float3 p, float3 a, float t){
  a = normalize(a);
  float3 q = cross(a, p), u = cross(q, a);
    return mat3(u, q, a)*to_float3_aw(_cosf(t), _sinf(t), dot(p, a));
}
*/






// The extruded image.
__DEVICE__ float map(float3 p, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){
   
    // Reflecting the wall opposite to give the light something to relect off of.
    p.z = _fabs(p.z + 1.0f) - 1.0f;
    
    // Wall behind the pylons to stop the light getting through, if we decide
    // to contract the boxes.
    float wall = -p.z + 0.16f;
    

    // Scaling: This can be confusing, since the scene looks like it's rendering on 
    // a 3D grid, but it's a 2D grid of rotated boxes all aligned on a flat plane.
    // The "to_float2(1, 2)" scaling figure accounts for the fact that two overlapping
    // rows of boxes are rendering -- in a similar way that we render a 2D hexagon grid.
    float2 s = GSCALE*to_float2(1, 2);
    
    // Box, box smoothing factor, and the gap between boxes.
    float bx = 1e5, sf = 0.001f, gap = 0.003f;
    
    
    // The rotated cubes on the diagonal cover a larger area relating to 
    // the diagonal distance. After getting things wrong for way too long,
    // I now know this. :)
    float sc0 = s.x*_sqrtf(2.0f)/2.0f;
    
    
    // The fist layer contains cubes that encroach on neighboring boundaries,
    // so two overlapping grids are necessary... Technically, you could get
    // away with one, but artifacts can arise.
    for(int i = ZERO; i<2; i++){
        
        // Global coordinates.
        float3 q = p;
        
        float2 cntr = i==0? to_float2_s(0.5f) : to_float2_s(0); // Offset position for each row.
        float2 ip = _floor(swi2(q,x,y)/s - cntr) + cntr + 0.5f; // Local tile ID.
        swi2S(q,x,y, swi2(q,x,y) - (ip)*s); // New local position.

        // I can't for the life of me remember how I came up with the first 
        // rotation, but I'm guessing the side-on cube step view would involve
        // a 1-1-_sqrtf(2) triangle, and the angle would arise from that.
        swi2S(q,y,z, mul_f2_mat2(swi2(q,y,z) , rot2(0.61547518786f))); // _atan2f(1.0f, _sqrtf(2.0f)).
        // A quater PI rotation around the Y axis is much easier to visualize.
        swi2S(q,x,z, mul_f2_mat2(swi2(q,x,z) , rot2(3.14159f/4.0f))); 
        
        // Smooth rounded box distance field.
        float bxi = sBoxS(q, to_float3_s(sc0/2.0f - gap), sf);

        // If we have the closest of the two boxes, update the distance
        // field information.
        if(bxi<bx){
            bx = bxi;                   
            swi2S(*gID,y,z, (ip));
            *blockPartID = 0.0f;
            *gCoord = q;
        }

    }
    
    
    // The second, third, etc, layers. Here, we're only doing two.
    
    // Scaling: Since the smaller boxes don't enchroach upon one another's 
    // boundaries, only one grid pre layer need be rendered.
    float2 s2 = GSCALE*2.0f;
    // Box offset distance (relative to the larger box) and direction.
    float offsD = 0.0f, dir = 1.0f;
    
    // Render two layers.
    for(int i = ZERO; i<2; i++){
        
        // Reducing size, scaling and offset for each succesive box layer.
        sc0 /= 2.0f;
        s2 /= 2.0f;
        offsD += s.x/8.0f/_powf(2.0f, (float)(i));

        // Global coordinates, relatve to the large base layer box.
        float3 q = p - to_float3(0, 1, -1)*offsD;
        
        // Offset each second row by half a cell.
        float2 cntr = to_float2_s(0); 
        if(dir*mod_f(_floor(q.y/s2.y), 2.0f)<dir*0.5f) cntr.x -= 0.5f;
        float2 ip2 = _floor(swi2(q,x,y)/s2 - cntr) + cntr + 0.5f; // Local tile ID.
        swi2S(q,x,y, swi2(q,x,y) - (ip2)*s2); // New local position.

        // Randomly break from the loop at certain levels, thus omitting
        // the rendering of all boxes that follow... Draw random boxes
        // would be another way to put it. L)
        if(hash21(ip2 + 0.03f)<0.2f) break;
        
        // Rotating into the step position: Horizontal X-axis followed by
        // vertical Y axis rotation.
        swi2S(q,y,z, mul_f2_mat2(swi2(q,y,z) , rot2(0.61547518786f))); // _atan2f(1.0f, _sqrtf(2.0f)).
        swi2S(q,x,z, mul_f2_mat2(swi2(q,x,z) , rot2(3.14159f/4.0f)));

        // Smooth rounded box distance field.
        float bxi = sBoxS(q, to_float3_s(sc0/2.0f - gap), sf);
        
       
        // Updating the smoothing factor and direction for smaller boxes.
        sf *= 0.7f;
        dir = -1.0f;

        // If we have the closest of the two boxes, update the distance
        // field information.
        if(bxi<bx){
           bx = bxi;
           swi2(*gID,y,z) = (ip2);
           *blockPartID = (float)(i + 1);//blPtID;
           *gCoord = q;
        }
    
    }
    
    (*gID).x = bx;
 
    // Overall object ID.
    //objID = d4.x<wall || d4.x<bx? 0.0f : bx<wall? 1.0f : 2.0f;
    *objID = bx<wall? 0.0f : 1.0f;
    //objID = 0.0f;
    
    // Combining the wall with the extruded blocks.
    return _fminf(bx, wall);//_fminf(min(wall, d4.x), bx);
 
}


// Basic raymarcher.
__DEVICE__ float trace(float3 ro, float3 rd, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){

    // Overall ray distance and scene distance.
    float t = 0.0f, d;
    
    for(int i = ZERO; i<80; i++){
    
        d = map(ro + rd*t, objID,blockPartID,gCoord,gID);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(_fabs(d)<DELTA || t>FAR) break; // Alternative: 0.001f*_fmaxf(t*0.25f, 1.0f), etc.
        
        //t += i<32? d*0.5f : d*0.9f; // Slower, but more accuracy.
        t += d*0.9f; 
    }

    return _fminf(t, FAR);
}


// Cheap shadows are hard. In fact, I'd almost say, shadowing particular scenes with limited 
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
__DEVICE__ float softShadow(float3 ro, float3 lp, float3 n, float k, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){

    // More would be nicer. More is always nicer, but not really affordable... 
    //Not on my slow test machine, anyway.
    const int iter = 32; 
    
    
    float3 rd = lp - ro; // Unnormalized direction ray.
    
    ro += n*0.0015f; // Bumping the shadow off the hit point.
    

    float shade = 1.0f;
    float t = 0.0f; 
    float end = _fmaxf(length(rd), 0.0001f);
    //float stepDist = end/float(maxIterationsShad);
    rd /= end;
    
    //rd = normalize(rd + (hash33R(ro + n) - 0.5f)*0.03f);
    

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, 
    // the lowest number to give a decent shadow is the best one to choose. 
    for (int i = ZERO; i<iter; i++){

        float d = map(ro + rd*t, objID,blockPartID,gCoord,gID);
        shade = _fminf(shade, k*d/t);
        //shade = _fminf(shade, smoothstep(0.0f, 1.0f, k*d/t)); // Thanks to IQ for this tidbit.
        // So many options here, and none are perfect: 
        // dist += _fminf(h, 0.2f), dist += clamp(h, 0.01f, stepDist), etc.
        t += clamp(d, 0.01f, 0.1f); 
        
        
        // Early exits from accumulative distance function calls tend to be a good thing.
        if (d<0.0f || t>end) break; 
    }

    // Sometimes, I'll add a constant to the final shade value, which lightens the shadow a bit --
    // It's a preference thing. Really dark shadows look too brutal to me. Sometimes, I'll add 
    // AO also just for kicks. :)
    return _fmaxf(shade, 0.0f); 
}


// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
__DEVICE__ float calcAO(in float3 p, in float3 n, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){

  float sca = 2.5f, occ = 0.0f;
    for( int i = ZERO; i<5; i++ ){
    
        float hr = (float)(i + 1)*0.15f/5.0f;        
        float d = map(p + n*hr, objID,blockPartID,gCoord,gID);
        occ += (hr - d)*sca;
        sca *= 0.8f;
        
        // Deliberately redundant line that may or may not stop the 
        // compiler from unrolling.
        if(sca>1e5) break;
    }
    
    return clamp(1.0f - occ, 0.0f, 1.0f);
}


// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
__DEVICE__ float3 getNormal(in float3 p, float *objID, float *blockPartID, float3 *gCoord, float4 *gID) {
  
    const float2 e = to_float2(0.001f, 0);
    
    //vec3 n = normalize(to_float3(map(p + swi3(e,x,y,y), objID,blockPartID,gCoord,gID) - map(p - swi3(e,x,y,y), objID,blockPartID,gCoord,gID),
    //map(p + swi3(e,y,x,y), objID,blockPartID,gCoord,gID) - map(p - swi3(e,y,x,y), objID,blockPartID,gCoord,gID),  map(p + swi3(e,y,y,x), objID,blockPartID,gCoord,gID) - map(p - swi3(e,y,y,x), objID,blockPartID,gCoord,gID)));
    
    // This mess is an attempt to speed up compiler time by contriving a break... It's 
    // based on a suggestion by IQ. I think it works, but I really couldn't say for sure.
    float sgn = 1.0f;
    float mp[6];
    float3 e6[3] = {swi3(e,x,y,y), swi3(e,y,x,y), swi3(e,y,y,x)};
    for(int i = ZERO; i<6; i++){
    mp[i] = map(p + sgn*e6[i/2], objID,blockPartID,gCoord,gID);
        sgn = -sgn;
        if(sgn>2.0f) break; // Fake conditional break;
    }
    
    return normalize(to_float3(mp[0] - mp[1], mp[2] - mp[3], mp[4] - mp[5]));
}

/*
// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
__DEVICE__ float3 texBump( sampler2D tx, in float3 p, in float3 n, float bf){
   
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
*/

/*
// Compact, self-contained version of IQ's 3D value noise function. I have a transparent noise
// example that explains it, if you require it.
__DEVICE__ float n3D(in float3 p){
    
  const float3 s = to_float3(7, 157, 113);
  float3 ip = _floor(p); p -= ip; 
    float4 h = to_float4(0.0f, swi2(s,y,z), s.y + s.z) + dot(ip, s);
    p = p*p*(3.0f - 2.0f*p); //p *= p*p*(p*(p * 6.0f - 15.0f) + 10.0f);
    h = _mix(fract(_sinf(h)*43758.5453f), fract(_sinf(h + s.x)*43758.5453f), p.x);
    swi2(h,x,y) = _mix(swi2(h,x,z), swi2(h,y,w), p.y);
    return _mix(h.x, h.y, p.z); // Range: [0, 1].
}

__DEVICE__ float fBm(float3 p){
    
    return n3D(p)*0.57f + n3D(p*2.0f)*0.28f + n3D(p*4.0f)*0.15f;
    
}

// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it 
// does give the impression that the surface is reflecting the surrounds in some way.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
__DEVICE__ float3 envMap(float3 p){
    
    p *= 3.0f;
    p.x += iTime/2.0f;
    
    float n3D2 = n3D(p*2.0f);
   
    // A bit of fBm.
    float c = n3D(p)*0.57f + n3D2*0.28f + n3D(p*4.0f)*0.15f;
    c = smoothstep(0.45f, 1.0f, c); // Putting in some dark space.
    
    p = to_float3(c, c*c, c*c*c*c); // Bluish tinge.
    
    return _mix(p, swi3(p,x,z,y), n3D2*0.4f); // Mixing in a bit of purple.

}
*/

// Face pattern.
__DEVICE__ float fPat(float2 p, float sc, float2 gIP){

    // Simple maze pattern consisting of random diagonal lines.
    // Pretty standard.
    
    // Rotation and scaling.
    p = mul_f2_mat2(p,rot2(3.14159f/4.0f));
    sc *= 5.5f*0.7071f;
    p *= sc;
    
    // Grid partioning:
    // Cell ID and local coordinates.
    float2 ip = _floor(p);
    p -= ip + 0.5f;
    
    // Random cell flip.
    float rnd = hash21(ip + gIP*0.123f +0.01f);
    if(rnd<0.5f) p.y = -p.y;
    
    // Render a diagonal line through the center, and
    // at the edges.
    float2 ap = abs_f2(p - 0.5f);
    float d = _fabs((ap.x + ap.y)*0.7071f - 0.7071f);
    ap = abs_f2(p);
    d = _fminf(d, _fabs((ap.x + ap.y)*0.7071f - 0.7071f));
    d -= 0.125f; // Line width.
    
    // Rescaling.
    return d/sc;
}

// Face pattern.
__DEVICE__ float fPat2(float2 p, float sc){

    // Scaling.
    sc *= 11.0f;
    p *= sc;
    
    // Partioning into a grid of squares.
    p = mul_f2_mat2(p,rot2(3.14159f/2.0f)); 
    p = fract_f2(p + 0.5f) - 0.5f;

    // Squares.
    p = abs_f2(p);
    float d = -(_fmaxf(p.x, p.y) - (0.5f - 0.125f/2.0f));
    
    // Rescaling.
    return d/sc;
}


// Surface bump function..
__DEVICE__ float bumpSurf3D(in float3 p, in float3 n, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){
    
    map(p, objID,blockPartID,gCoord,gID);
    
    float3 txP = *gCoord;
    // The objects (cubes) have been rotated in the distance function,
    // so the normal needs to match... If I had a dollar for every time
    // I've forgotten that... :)
    float3 txN = n;
    swi2S(txN,y,z, mul_f2_mat2(swi2(txN,y,z) , rot2(0.61547518786f))); // _atan2f(1.0f, _sqrtf(2.0f)).
    swi2S(txN,x,z, mul_f2_mat2(swi2(txN,x,z) , rot2(3.14159f/4.0f))); // 4.

    float2 uv = _fabs(txN.x)>.5? swi2(txP,z,y) : _fabs(txN.y)>.5? swi2(txP,x,z) : swi2(txP,x,y);
    //float faceID = _fabs(txN.x)>.5? 0.0f : _fabs(txN.y)>.5? 1.0f : 2.0f;
    
    
    
    float2 scale = GSCALE;
    float sc = scale.x/_sqrtf(2.0f)/2.0f -  0.003f;//*(*blockPartID + 1.0f);
    sc /= _exp2f(*blockPartID); // Dividing smaller cubes by powers of 2.
                
    float2 auv = abs_f2(uv);
    float fdf = _fabs(_fmaxf(auv.x, auv.y) - sc) - 0.002f;        
    
    //uv += *blockPartID<2.? 0.25f : 0.0f;
    float pat2 = fPat2(uv/1.03f + 0.25f, 2.0f);// + *blockPartID*.5
    
    sc *= _exp2f(*blockPartID);
    pat2 = smoothstep(0.0f, 0.0002f/sc/1.0f, pat2);

    pat2 = _mix(0.0f, pat2, (1.0f - smoothstep(0.0f, 0.0002f/sc, fdf - 0.02f + 0.002f + 0.009f)));
    
    return pat2;

}


 
// Standard function-based bump mapping routine: This is the cheaper four tap version. There's
// a six tap version (samples taken from either side of each axis), but this works well enough.
__DEVICE__ float3 doBumpMap(in float3 p, in float3 n, float bumpfactor, float *objID, float *blockPartID, float3 *gCoord, float4 *gID){
    
    // Larger sample distances give a less defined bump, but can sometimes lessen the aliasing.
    const float2 e = to_float2(0.001f, 0);  
    
    // This utter mess is to avoid longer compile times. It's kind of 
    // annoying that the compiler can't figure out that it shouldn't
    // unroll loops containing large blocks of code.
    float3 p4[4] = {p, p - swi3(e,x,y,y), p - swi3(e,y,x,y), p - swi3(e,y,y,x)};
float zzzzzzzzzzzzzzzzzzz;    
    float _b4[4];
    for(int i = ZERO; i<4; i++){
        _b4[i] = bumpSurf3D(p4[i], n, objID,blockPartID,gCoord,gID);
        if(n.x>1e5) break; // Fake break to trick the compiler.
    }
    
    // Gradient vector: to_float3(df/dx, df/dy, df/dz);
    //float3 grad = (swi3(b4,y,z,w) - b4.x)/e.x; 
    float3 grad = (to_float3(_b4[1],_b4[2],_b4[3]) - _b4[0])/e.x; 
   
    
    // Six tap version, for comparisson. No discernible visual difference, in a lot of cases.
    //vec3 grad = to_float3(bumpSurf3D(p - swi3(e,x,y,y), objID,blockPartID,gCoord,gID) - bumpSurf3D(p + swi3(e,x,y,y), objID,blockPartID,gCoord,gID),
    //                 bumpSurf3D(p - swi3(e,y,x,y), objID,blockPartID,gCoord,gID) - bumpSurf3D(p + swi3(e,y,x,y), objID,blockPartID,gCoord,gID),
    //                 bumpSurf3D(p - swi3(e,y,y,x), objID,blockPartID,gCoord,gID) - bumpSurf3D(p + swi3(e,y,y,x), objID,blockPartID,gCoord,gID))/e.x*0.5f;
    
  
    // Adjusting the tangent vector so that it's perpendicular to the normal. It's some kind 
    // of orthogonal space fix using the Gram-Schmidt process, or something to that effect.
    grad -= n*dot(n, grad);          
         
    // Applying the gradient vector to the normal. Larger bump factors make things more bumpy.
    return normalize(n + grad*bumpfactor);
  
}



//****************************************************************************************************************************************
__KERNEL__ void FractalCubeStepsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_SLIDER0(iMouseZ, -1.0f, 1.0f, 0.0f);

  // Scene object ID to separate the mesh object from the terrain.
  float objID;
  
  // Hacked in at the last minute to differentiate between the block and
  // the metallic trimmings.
  float blockPartID;

  float3 gCoord;
  // Block ID -- It's a bit lazy putting it here, but it works. :)
  float4 gID;

    
  // Screen coordinates.
  float2 uv = (fragCoord - iResolution*0.5f)/iResolution.y;
    
  float2 ra = to_float2(1, iResolution.x/iResolution.y);
  uv *= 1.0f + (dot(uv*ra, uv*ra)*0.05f - 0.025f);
  

  // Ray origin.
  float3 ro = to_float3(iTime/6.0f, 0, -1); 
  // "Look At" position.
  float3 lk = ro + to_float3(0.01f*0.0f, 0.01f*0.0f, 0.1f);//to_float3(0, -0.25f, iTime);  
 
  // Light positioning.
  float3 lp = ro + to_float3(0.25f, 0.25f, 0.7f); //0.25f// Put near the camera.
  

  // Using the above to produce the unit ray-direction vector.
  float FOV = 1.0f; // FOV - Field of view.
  float3 fwd = normalize(lk - ro);
  float3 rgt = normalize(to_float3(fwd.z, 0.0f, -fwd.x )); 
  // "right" and "forward" are perpendicular, due to the dot product being zero. Therefore, I'm 
  // assuming no normalization is necessary? The only reason I ask is that lots of people do 
  // normalize, so perhaps I'm overlooking something?
  float3 up = cross(fwd, rgt); 

  // Unit direction ray.
  float3 rd = normalize(uv.x*rgt + uv.y*up + fwd/FOV);
    
    
  // Camera position. Initially set to the ray origin.
  float3 cam = ro;
  // Surface postion. Also initially set to the ray origin.
  float3 sp = ro; 

  sp += to_float3(iMouse.x/R.x-0.5f,iMouse.y/R.y-0.5f,iMouseZ);
    
  float gSh = 1.0f;
  float objRef = 1.0f;
    
  float3 col = to_float3_s(0);
    
   
    
  float alpha = 1.0f;
   
  for(int j = ZERO; j<PASSES; j++){
        
        // Layer or pass color. Each pass color gets blended in with
        // the overall result.
        float3 colL = to_float3_s(0);

        // Used for refractions, but not here.    
        //float distanceFactor = 1.0f;

        
        // Raymarch to the scene.
        float t = trace(sp, rd, &objID,&blockPartID,&gCoord,&gID);

        // Saving the object ID, block ID and cell object (block part) ID.
        float svObjID = objID;
        float4 svGID = gID;
        float svBPID = blockPartID;
        float3 svCoord = gCoord;

        // Advance the ray to the surface. This becomes the new ray origin for the
        // next pass.
        sp += rd*t;
        
        
        // If the ray hits a surface, light it up. By the way, it's customary to put 
        // all of the following inside a single function, but I'm keeping things simple.
        // Blocks within loops used to kill GPU performance, but it doesn't seem to
        // effect the new generation systems.
      
        if(t<FAR){

            // Surface normal.
            float3 sn = getNormal(sp, &objID,&blockPartID,&gCoord,&gID);// *distanceFactor; // For refractions.
            
            // Function based bump mapping for the edges.
            sn = doBumpMap(sp, sn, 0.03f, &objID,&blockPartID,&gCoord,&gID);

            
            // The reflective ray, which tends to be very helpful when
            // calculating reflections. :)
            float3 reflection = reflect(rd, sn);
            
            float3 ld = lp - sp; // Point light direction.
            float lDist = length(ld); // Surface to light distance.
            ld /= _fmaxf(lDist, 0.0001f); // Normalizing.
            
            
            // Shadows and ambient self shadowing.
            //
            // Shadows are expensive. It'd be nice to include shadows on each bounce,
            // but it's still not really viable, so we just perform them on the 
            // first pass... Years from now, I'm hoping it won't be an issue.
            float sh = 1.0f;
            if(j == 0) sh = softShadow(sp, lp, sn, 8.0f, &objID,&blockPartID,&gCoord,&gID);
            float ao = calcAO(sp, sn, &objID,&blockPartID,&gCoord,&gID); // Ambient occlusion.
            gSh = _fminf(sh, gSh); // Adding a touch of light to the shadow.
          
            float att = 1.0f/(1.0f + lDist*lDist*0.05f); // Attenuation.

            float dif = _fmaxf(dot(ld, sn), 0.0f); // Diffuse lighting.
            float spe = _powf(_fmaxf(dot(reflection, ld), 0.0f), 32.0f);
            float fre = clamp(1.0f + dot(rd, sn), 0.0f, 1.0f); // Fresnel reflection term.
            
            dif = _powf(dif, 4.0f)*2.0f; // Ramping up the diffuse.

            float Schlick = _powf( 1.0f - _fmaxf(dot(rd, normalize(rd + ld)), 0.0f), 5.0f);
            float freS = _mix(0.25f, 1.0f, Schlick);  //F0 = 0.2f - Glass... or close enough.

      
            // Object color.
            float3 oCol;
            
             
            if(svObjID == 0.0f) {
            
                // Cube face coloring.
            
                // Face scaling.
                float2 scale = GSCALE;
                float sc = scale.x/_sqrtf(2.0f)/2.0f -  0.003f;
                // Dividing smaller cubes by powers of 2.
                float sc2 = sc/_exp2f(svBPID);
                
                // Local face texture coordinates.
                float3 txP = svCoord;
                
                // The objects (cubes) have been rotated in the distance function,
                // so the normal needs to match... If I had a dollar for every time
                // I've forgotten that... :)
                float3 txN = sn;
                swi2S(txN,y,z, mul_f2_mat2(swi2(txN,y,z) , rot2(0.61547518786f))); // _atan2f(1.0f, _sqrtf(2.0f)).
                swi2S(txN,x,z, mul_f2_mat2(swi2(txN,x,z) , rot2(3.14159f/4.0f)));  // 4.0f
                
                // Using the normal to obtain the UV coordinates for the cube face.
                float2 uv = _fabs(txN.x)>0.5f? swi2(txP,z,y) : _fabs(txN.y)>0.5f? swi2(txP,x,z) : swi2(txP,x,y);
                float faceID = _fabs(txN.x)>0.5f? 0.0f : _fabs(txN.y)>0.5f? 1.0f : 2.0f;

         
                // Block coloring.
                float3 tx = getTex(iChannel0, uv + hash21(swi2(svGID,y,z)));
                tx = smoothstep(to_float3_s(0.0f), to_float3_s(0.5f), tx);

 
                // Edge coloring, but has been replaced by trimming.
                // Needs to match the box function inside the distance field.
                
                 
                // Random face value (blinking).
                float rndF = hash21(swi2(svGID,y,z) + svBPID/6.0f + 0.08f);
                rndF = smoothstep(0.92f, 0.97f, _sinf(6.2831f*rndF + iTime)*0.5f + 0.5f);
                // Blinking face light color.
                float3 blCol = _mix(to_float3(3.2f, 0.7f, 0.3f), to_float3(3.4f, 0.4f, 0.6f), hash21(swi2(svGID,y,z) + 0.32f));
                blCol = _mix(blCol, swi3(blCol,z,y,x), step(0.5f, hash21(swi2(svGID,y,z) + 0.21f)));
                //blCol = _mix(blCol, swi3(blCol,z,y,x), _floor(hash21(swi2(svGID,y,z) + 0.21f)*2.999f)/2.0f);
                // Face glow color.
                float3 glCol = smoothstep(0.0f, 1.0f, 1.0f - length(uv)/sc2)*blCol*6.0f;
        
                float pat2 = fPat2(uv, 1.0f/0.125f);
                float3 metCol = to_float3(1.1f, 0.9f, 1.1f);
                oCol = tx/2.0f*metCol;

                //if(svBPID <= 1.0f){
                    // Applying spherical glow to the face.
                    oCol = _mix(oCol, glCol, rndF + 0.01f); 
                    oCol = _mix(oCol, to_float3_s(0), (1.0f - smoothstep(0.0f, 0.0003f/sc2, pat2))*0.9f);
                // }
               
                // Face pattern.
                float pat = fPat(uv, 1.0f/0.125f, swi2(svGID,y,z) + faceID/6.0f + svBPID/6.0f);
                
                // Edge pattern.
                pat2 = fPat2(uv/1.03f + 0.25f, 2.0f);
                
                // Edge coloring.
                float3 eCol = _mix(tx*0.5f + 0.5f, tx*0.5f + 0.3f + blCol*0.15f, rndF)/3.0f;
                // Edge pattern.
                eCol = _mix(eCol, to_float3_s(0), (1.0f - smoothstep(0.0f, 0.0002f/sc, pat2))*0.5f);
                // Fake bump... Needs work.
                //eCol = _mix(eCol, to_float3(1), (1.0f - smoothstep(0.0f, 0.0004f/sc, pat2 - 0.001f))*0.35f);
                //eCol = _mix(eCol, to_float3(0), (1.0f - smoothstep(0.0f, 0.0002f/sc, pat2))*0.9f);
                eCol *= metCol;

                // Applying the maze pattern to the faces.
                oCol = _mix(oCol, oCol/8.0f, (1.0f - smoothstep(0.0f, 0.0002f/sc, pat)));

                // Applying the edge color.
                float2 auv = abs_f2(uv);
                float fdf = _fabs(_fmaxf(auv.x, auv.y) - sc2) - 0.002f;
                oCol = _mix(oCol, to_float3_s(0), (1.0f - smoothstep(0.0f, 0.0002f/sc, fdf - 0.02f + 0.002f)));
                oCol = _mix(oCol, eCol, (1.0f - smoothstep(0.0f, 0.0002f/sc, fdf - 0.02f + 0.002f + 0.009f)));

                // Different reflectance on different parts of the surface.
                //
                // Reflectance of the glass (or perspex) face pattern.
                float faceRef = _mix(0.25f, 0.5f, (1.0f - smoothstep(0.0f, 0.0002f/sc, pat)));
                // Reflectance of the face and edge.
                objRef = _mix(faceRef, 0.1f, (1.0f - smoothstep(0.0f, 0.0002f/sc, fdf - 0.02f + 0.002f + 0.009f)));
             
       
            }
            else {
                // Dark wall behind the tiny gaps in the blocks. 
                oCol = to_float3_s(0);
                objRef = 0.0f;
            
            }
 
 
            // The color for this layer.
            colL = oCol*(dif*gSh + 0.1f + to_float3(1, 0.8f, 0.5f)*spe*1.0f*gSh);
        
           
            // Optional environmental mapping. Not used.
            //vec3 envCol = envMap(reflect(rd, sn)).zyx;
            //colL += colL*envCol*3.0f;
            
            colL *= ao*att;

            // Used for refraction, but not used here.
            //if(distanceFactor<0.0f)  colL *= _expf(-colL*t*5.0f);
            
            
            // Set the unit direction ray to the new reflected direction, and bump the 
            // ray off of the hit point by a fraction of the normal distance. Anyone who's
            // been doing this for a while knows that you need to do this to stop self
            // intersection with the current launch surface from occurring... It used to 
            // bring me unstuck all the time. I'd spend hours trying to figure out why my
            // reflections weren't working. :)
            rd = reflection;
            sp += sn*DELTA*1.1f;

        }

        // Fog: Redundant here, since the ray doesn't go far, but necessary for other setups.
        //float td = length(sp - cam); 
        //vec3 fogCol = to_float3(0);
        //colL = _mix(colL, fogCol, smoothstep(0.0f, 0.95f, td/FAR));
      
        // This is a more subtle way to blend layers. 
        //col = _mix(col, colL, 1.0f/float(1 + j)*alpha);
        // In you face additive blend. Sometimes, I prefer this.
        col += colL*alpha;
        
        // If the hit object's reflective factor is zero, or the ray has reached
        // the far horizon, break.
        if(objRef < 0.001f || t >= FAR) break;
        
        // Decrease the alpha factor (ray power of sorts) by the hit object's reflective factor.
        alpha *= objRef;
    }
    
    // Rough gamma correction.
    //fragColor = to_float4_aw(_powf(_fmaxf(col, 0.0f), to_float3_aw(1.0f/2.2f)), 1);
    fragColor = to_float4_aw(sqrt_f3(_fmaxf(col, to_float3_s(0.0f))), 1);
    

  SetFragmentShaderComputedColor(fragColor);
}