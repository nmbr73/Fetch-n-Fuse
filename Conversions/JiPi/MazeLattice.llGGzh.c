
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


/*

    Maze Lattice
    ------------

  Applying a geometric pattern and edging to Fizzer's maze lattice. I'm not sure what he technically 
  calls it, but it has a lattice feel, and as he infers in his article, it's a 3D structure with 
  maze-like qualities. Either way, I particularly like it because it's cleverly constructed, interesting 
  looking, and very cheap to produce. Basically, it's one of those distance fields that gives you your 
  money's worth. :)

  The only interesting thing here is the distance field equation that contains the maze-like lattice. I've 
  given a rough explanation behind its construction, but it's much better to read Fizzer's well explained 
  article, which I've provided a link to below.

  The geometric surfacing pattern is a 2D hexagonal Truchet design, which is applied to each of the flat 
    face sections in accordance to the surface normal orientation. It's a standard way to apply 2D patterns 
  to a cuboid-based surface, and is contained in the "texFaces" function.

  I used an edging algorithm to obtain the edges, which involves extra distance function and bump calls. 
  I've since thought of a better way to make that happen which will cut down on cost and complexity, so 
  I'll apply that in due course.

  Anyway, I have a reflection\refraction version, based on the same surface that I'll release later.
  
  Distance field based on the article accompanying the following:
  Maze Explorer - fizzer
    https://www.shadertoy.com/view/XsdGzM

  Accompanying article is here:
  Implicit Maze-Like Patterns
    http://amietia.com/slashmaze.html

  Truchet shaders:

    hexagonal truchet ( 352 ) - FabriceNeyret2
    https://www.shadertoy.com/view/Xdt3D8
 
    hexagonal tiling - mattz
    https://www.shadertoy.com/view/4d2GzV
    


*/

// Maximum ray distance.
#define FAR 40.0f

// I love this distance field. So elegant, and I can thank Fizzer for coming up with it.
// The idea is about as simple as it gets. Break space into octahedrons then use each
// otahedral cell to obtain a unique ID. Use that ID to render a randomly oriented square 
// tube and you're done.
//
// I've done a little trimming and shuffling, which probably confuses things slightly. 
// Either way, it's worth reading the following article for a much clearer explanation:
//
// Implicit Maze-Like Patterns - Fizzer
// http://amietia.com/slashmaze.html
//
__DEVICE__ float map(in float3 p) {
    
    
   // Cubes, for a simpler, more orderly scene.
   //p = _fabs(fract(p) - 0.5f);    
   //return _fmaxf(max(p.x, p.y), p.z) - 0.225f;
   
   // Unique identifier for the cube, but needs to be converted to a unique ID
   // for the nearest octahedron. The extra ".5" is to save a couple of 
   // of calculations. See below.
   float3 ip = _floor(p) + 0.5f;
    
   p -= ip; // Break space into cubes. Equivalent to: fract(p) - .5.
    
   // Stepping trick used to identify faces in a cube. The center of the cube face also
   // happens to be the center of the nearest octahedron, so that works out rather well. 
   // The result needs to be factored a little (see the hash line), but it basically  
   // provides a unique octahedral ID. Fizzer provided a visual of this, which is easier 
   // to understand, and worth taking a look at.
   float3 q = abs_f3(p); 
   q = step(swi3(q,y,z,x), swi3(q,x,y,z))*step(swi3(q,z,x,y), swi3(q,x,y,z))*sign_f3(p); // Used for cube mapping also.
   
   // Put the ID into a hash function to produce a unique random number. Reusing "q" to
   // save declaring a float. Don't know if it's faster, but it looks neater, I guess.
   q.x = fract(_sinf(dot(ip + q*0.5f, to_float3(111.67f, 147.31f, 27.53f)))*43758.5453f);
    
   // Use the random number to orient a square tube in one of three random axial
   // directions... See Fizzer's article explanation. It's better. :) By the way, it's
   // possible to rewrite this in "step" form, but I don't know if it's quicker, so I'll
   // leave it as is for now.
   swi2S(p,x,y, abs_f2(q.x>0.333f ? q.x>0.666f ? swi2(p,x,z) : swi2(p,y,z) : swi2(p,x,y)));
   return _fmaxf(p.x, p.y) - 0.2f;   

}

// Very basic raymarching equation. I thought I might need to use something more sophisticated,
// but it turns out that this structure raymarches reasonably well. Not all surfaces do.
__DEVICE__ float trace(float3 ro, float3 rd){

    float t = 0.0f;
    for(int i=0; i< 72; i++){
        float d = map(ro + rd*t);
        if (_fabs(d) < 0.002f*(t*0.125f + 1.0f) || t>FAR) break;
        t += d;
    } 
    return _fminf(t, FAR);
}

// The reflections are pretty subtle, so not much effort is being put into them. Only a few iterations.
__DEVICE__ float refTrace(float3 ro, float3 rd){

    float t = 0.0f;
    for(int i=0; i< 16; i++){
        float d = map(ro + rd*t);
        if (_fabs(d) < 0.005f*(t*0.25f + 1.0f) || t>FAR) break;
        t += d;
    } 
    return t;
}

// The normal function with some edge detection rolled into it. Sometimes, it's possible to get away
// with six taps, but we need a bit of epsilon value variance here, so there's an extra six.
__DEVICE__ float3 normal(in float3 p, inout float *edge) { 
  
    float2 e = to_float2(0.034f, 0); // Larger epsilon for greater sample spread, thus thicker edges.

    // Take some distance function measurements from either side of the hit point on all three axes.
    float d1 = map(p + swi3(e,x,y,y)), d2 = map(p - swi3(e,x,y,y));
    float d3 = map(p + swi3(e,y,x,y)), d4 = map(p - swi3(e,y,x,y));
    float d5 = map(p + swi3(e,y,y,x)), d6 = map(p - swi3(e,y,y,x));
    float d = map(p)*2.0f;  // The hit point itself - Doubled to cut down on calculations. See below.
     
    // Edges - Take a geometry measurement from either side of the hit point. Average them, then see how
    // much the value differs from the hit point itself. Do this for X, Y and Z directions. Here, the sum
    // is used for the overall difference, but there are other ways. Note that it's mainly sharp surface 
    // curves that register a discernible difference.
    *edge = _fabs(d1 + d2 - d) + _fabs(d3 + d4 - d) + _fabs(d5 + d6 - d);
    //edge = _fmaxf(max(_fabs(d1 + d2 - d), _fabs(d3 + d4 - d)), _fabs(d5 + d6 - d)); // Etc.
    
    
    // Once you have an edge value, it needs to normalized, and smoothed if possible. How you 
    // do that is up to you. This is what I came up with for now, but I might tweak it later.
    //
    *edge = smoothstep(0.0f, 1.0f, _sqrtf(*edge/e.x*8.0f));
    
    // Curvature. All this, just to take out the inner edges.
    float crv = (d1 + d2 + d3 + d4 + d5 + d6 - d*3.0f)/e.x;;
    //crv = clamp(crv*32.0f, 0.0f, 1.0f);
    if (crv<0.0f) *edge = 0.0f; // Comment out to see what it does.


    // Redoing the calculations for the normal with a more precise epsilon value. If you can roll the 
    // edge and normal into one, it saves a lot of map calls. Unfortunately, we want wide edges, so
    // there are six more, making 12 map calls in all. Ouch! :)
    e = to_float2(0.005f, 0);
    d1 = map(p + swi3(e,x,y,y)), d2 = map(p - swi3(e,x,y,y));
    d3 = map(p + swi3(e,y,x,y)), d4 = map(p - swi3(e,y,x,y));
    d5 = map(p + swi3(e,y,y,x)), d6 = map(p - swi3(e,y,y,x)); 
    
    // Return the normal.
    // Standard, normalized gradient mearsurement.
    return normalize(to_float3(d1 - d2, d3 - d4, d5 - d6));
}

// Ambient occlusion, for that self shadowed look.
// XT95 came up with this particular version. Very nice.
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
__DEVICE__ float calcAO( in float3 p, in float3 n )
{
    
    float ao = 0.0f, l;
    const float nbIte = 12.0f;
    const float falloff = 1.0f;
    
    const float maxDist = 1.0f;
    for( float i=1.0f; i< nbIte+0.5f; i++ ){
    
        l = (i + fract(_cosf(i)*45758.5453f))*0.5f/nbIte*maxDist;
        ao += (l - map( p + n*l ))/ _powf(1.0f + l, falloff);
    }
  
    return clamp( 1.0f - ao*2.0f/nbIte, 0.0f, 1.0f);
}


// Cheap shadows are hard. In fact, I'd almost say, shadowing repeat objects - in a setting like this - with limited 
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
__DEVICE__ float softShadow(float3 ro, float3 lp, float k){

    // More would be nicer. More is always nicer, but not really affordable... Not on my slow test machine, anyway.
    const int maxIterationsShad = 16; 
    
    float3 rd = (lp-ro); // Unnormalized direction ray.

    float shade = 1.0f;
    float dist = 0.05f;    
    float end = _fmaxf(length(rd), 0.001f);
    float stepDist = end/(float)(maxIterationsShad);
    
    rd /= end;

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i=0; i<maxIterationsShad; i++){

        float h = map(ro + rd*dist);
        //shade = _fminf(shade, k*h/dist);
        shade = _fminf(shade, smoothstep(0.0f, 1.0f, k*h/dist)); // Subtle difference. Thanks to IQ for this tidbit.
        //dist += _fminf( h, stepDist ); // So many options here: dist += clamp( h, 0.0005f, 0.2f ), etc.
        dist += clamp(h, 0.02f, 0.25f);
        
        // Early exits from accumulative distance function calls tend to be a good thing.
        if (h<0.001f || dist > end) break; 
    }

    // I've added 0.5f to the final shade value, which lightens the shadow a bit. It's a preference thing.
    return _fminf(max(shade, 0.0f) + 0.3f, 1.0f); 
}


// Simple hexagonal truchet patten. This is based on something Fabrice and Mattz did.
//
// hexagonal truchet ( 352 ) - FabriceNeyret2
// https://www.shadertoy.com/view/Xdt3D8
//
// hexagonal tiling - mattz
// https://www.shadertoy.com/view/4d2GzV
__DEVICE__ float hexTruchet(in float2 p) { 
    
    p *= 6.0f;
    
  // Hexagonal coordinates.
    float2 h = to_float2(p.x + p.y*0.577350269f, p.y*1.154700538f);
    
    // Closest hexagon center.
    float2 f = fract(h); h -= f;
    float c = fract((h.x + h.y)/3.0f);
    h =  c<0.666f ?   c<0.333f ?  h  :  h + 1.0f  :  h  + step(swi2(f,y,x), f); 

    p -= to_float2(h.x - h.y*0.5f, h.y*0.8660254f);
    
    // Rotate (flip, in this case) random hexagons. Otherwise, you'd hava a bunch of circles only.
    // Note that "h" is unique to each hexagon, so we can use it as the random ID.
    c = fract(_cosf(dot(h, to_float2(41.13f, 289.57f)))*43758.5453f); // Reusing "c."
    p -= p*step(c, 0.5f)*2.0f; // Equivalent to: if (c<0.5f) p *= -1.0f;
    
    // Minimum squared distance to neighbors. Taking the square root after comparing, for speed.
    // Three partitions need to be checked due to the flipping process.
    p -= to_float2(-1, 0);
    c = dot(p, p); // Reusing "c" again.
    p -= to_float2(1.5f, 0.8660254f);
    c = _fminf(c, dot(p, p));
    p -= to_float2(0, -1.73205f);
    c = _fminf(c, dot(p, p));
    
    return _sqrtf(c);
    
    // Wrapping the values - or folding the values over (_fabs(c-0.5f)*2.0f, _cosf(c*6.283f*1.0f), etc) - to produce 
    // the nicely lined-up, wavy patterns. I"m perfoming this step in the "map" function. It has to do 
    // with coloring and so forth.
    //c = _sqrtf(c);
    //c = _cosf(c*6.283f*2.0f) + _cosf(c*6.283f*4.0f);
    //return (clamp(c*0.6f+0.5f, 0.0f, 1.0f));

}

// Bumping the faces.
__DEVICE__ float bumpFunc(float3 p, float3 n){
    
    // Mapping the 3D object position to the 2D UV coordinate of one of three
    // orientations, which are determined by the dominant normal axis.    
    n = abs_f3(n);
    swi2S(p,x,y, n.x>0.5f? swi2(p,y,z) : n.y>0.5f? swi2(p,x,z) : swi2(p,x,y)); 
    
    // Wavy, 70s looking, hexagonal Truchet pattern.
    float2 sc = (cos_f2(hexTruchet(swi2(p,x,y))*6.283f*to_float2(2, 4)));
    return clamp(dot(sc, to_float2_s(0.6f)) + 0.5f, 0.0f, 1.0f);

}

// Standard function-based bump mapping function.
__DEVICE__ float3 bumpMap(in float3 p, in float3 n, float bumpfactor){
    
    const float2 e = to_float2(0.002f, 0);
    float ref = bumpFunc(p, n);                 
    float3 grad = (to_float3(bumpFunc(p - swi3(e,x,y,y), n),
                             bumpFunc(p - swi3(e,y,x,y), n),
                             bumpFunc(p - swi3(e,y,y,x), n) )-ref)/e.x;                     

    grad -= n*dot(n, grad);          
                      
    return normalize( n + grad*bumpfactor );
  
}

// Bumping the edges with some block partitions. Made up on the spot. 
__DEVICE__ float bumpFunc2(float3 p, float3 n){
    
    // Partition space to produce some smooth blocks.
    p = abs_f3(fract_f3(p*3.0f) - 0.5f);
    float c = _fmaxf(max(p.x, p.y), p.z);
    
    return 1.0f - smoothstep(0.0f, 0.025f, c - 0.47f);
}

// A second function-based bump mapping function. Used for
// the edging. Messy, but probably faster... probably. :)
__DEVICE__ float3 bumpMap2(in float3 p, in float3 n, float bumpfactor){
    
    const float2 e = to_float2(0.002f, 0);
    float ref = bumpFunc2(p, n);                 
    float3 grad = (to_float3(bumpFunc2(p - swi3(e,x,y,y), n),
                             bumpFunc2(p - swi3(e,y,x,y), n),
                             bumpFunc2(p - swi3(e,y,y,x), n) )-ref)/e.x;                     
          
    grad -= n*dot(n, grad);          

    return normalize( n + grad*bumpfactor );
  
}

// Cheap and nasty 2D smooth noise function with inbuilt hash function - based on IQ's 
// original. Very trimmed down. In fact, I probably went a little overboard. I think it 
// might also degrade with large time values. I'll swap it for something more robust later.
__DEVICE__ float n2D(float2 p) {

  float2 i = _floor(p); p -= i; p *= p*(3.0f - p*2.0f);  
  
  float4 ret = fract_f4(sin_f4(to_float4(0, 41, 289, 330) + dot(i, to_float2(41, 289)))*43758.5453f);
      
  return dot(mul_mat2_f2(to_mat2(ret.x,ret.y,ret.z,ret.w), to_float2(1.0f - p.y, p.y)), to_float2(1.0f - p.x, p.x) );

}

// Texturing the sides with a 70s looking hexagonal Truchet pattern.
__DEVICE__ float3 texFaces(in float3 p, in float3 n){
    
    // Use the normal to determine the face. Dominant "n.z," then use the XY plane, etc.
    n = abs_f3(n);
    swi2S(p,x,y, n.x>0.5f? swi2(p,y,z) : n.y>0.5f? swi2(p,x,z) : swi2(p,x,y)); 

    // Some fBm noise based bluish red coloring.
    n = _mix(to_float3(0.3f, 0.1f, 0.02f), to_float3(0.35f, 0.5f, 0.65f), n2D(swi2(p,x,y)*8.0f)*0.66f + n2D(swi2(p,x,y)*16.0f)*0.34f);
    n *= n2D(swi2(p,x,y)*512.0f)*1.2f + 1.4f;
    
    //n =  n*0.3f + _fminf(swi3(n,z,y,x)*to_float3(1.3f, 0.6f, 0.2f)*0.75f, 1.0f)*0.7f;
   
    // Overlaying with the hexagonal Truchet pattern.
    float2 sc = (cos_f2(hexTruchet(swi2(p,x,y))*6.283f*to_float2(2, 4)));
    n *= clamp(dot(sc, to_float2_s(0.6f))+0.5f, 0.0f, 1.0f)*0.95f + 0.05f;
    
    return _fminf(n, to_float3_s(1.0f));

}

// Terxturing the edges with something subtle.
__DEVICE__ float3 texEdges(in float3 p, in float3 n){
    
    float bf = bumpFunc2(p, n); // Bump function.
    
    // 2D face selection.
    n = abs_f3(n);
    swi2S(p,x,y, n.x>0.5f? swi2(p,y,z) : n.y>0.5f? swi2(p,x,z) : swi2(p,x,y)); 

    // Mixing color with some fBm noise.
    n = _mix(to_float3(0.3f, 0.1f, 0.02f), to_float3(0.35f, 0.5f, 0.65f), n2D(swi2(p,x,y)*8.0f)*0.66f + n2D(swi2(p,x,y)*16.0f)*0.34f);
    n *= n2D(swi2(p,x,y)*512.0f)*0.85f + 0.15f; 
    
    // More coloring.
    n = _fminf((n + 0.35f)*to_float3(1.05f, 1, 0.9f), to_float3_s(1.0f));
    
    // Running the bump function over the top for some extra depth.
    n *= bf*0.75f+0.25f;
    
    return n;
}

__KERNEL__ void MazeLatticeFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
 
 
    
    // Unit direction ray vector: Note the absence of a divide term. I came across
    // this via a comment Shadertoy user "coyote" made. I'm pretty happy with this.
    float3 rd = to_float3_aw(2.0f*fragCoord - iResolution, iResolution.y);
    
    // Barrel distortion;
    rd = normalize(to_float3_aw(swi2(rd,x,y), _sqrtf(_fmaxf(rd.z*rd.z - dot(swi2(rd,x,y), swi2(rd,x,y))*0.2f, 0.0f))));
    
    // Rotating the ray with Fabrice's cost cuttting matrix. I'm still pretty happy with this also. :)
    float2 m = sin_f2(to_float2(1.57079632f, 0) + iTime/4.0f);
    swi2S(rd,x,y, mul_f2_mat2(swi2(rd,x,y), to_mat2(m.x, m.y, -m.y, m.x)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z), to_mat2(m.x, m.y, -m.y, m.x)));
    
    // Ray origin: Sending it along the Z-axis.
    float3 ro = to_float3(0, 0, iTime);
    // Alternate: Set off in the YZ direction. Note the ".5." It's an old lattice trick.
    //vec3 ro = to_float3(0, iTime/2.0f + 0.5f, iTime/2.0f);
    
    float3 lp = ro + to_float3(0.2f, 1.0f, 0.3f); // Light, near the ray origin.
    
    // Set the initial scene color to black.
    float3 col = to_float3_s(0);

    
    float t = trace(ro, rd); // Raymarch.
    
    // Normally, you'd only light up the scene if the distance is less than the outer boundary.
    // However, in this case, since most rays hit, I'm clamping to the far distance, and doing
    // the few extra calculations. The payoff (I hope) is not having a heap of nested code.
    // Whether that results in more speed, or not, I couldn't really say, but I'd imagine you'd
    // receive a slight gain... maybe. If the scene were more open, you wouldn't do this.
    //if(t<FAR){

        float edge;
        float3 sp = ro + rd*t; // Surface position.
        float3 sn = normal(sp, &edge); // Surface normal.

      // Saving a copy of the unbumped normal, since the texture routine require it.
      // I found that out the hard way. :)
        float3 svn = sn;
    
      // Bump mapping the faces and edges. The bump factor is reduced with distance
      // to lessen artifacts.
        if(edge<0.001f) sn = bumpMap(sp, sn, 0.01f/(1.0f + t*0.25f));
        else            sn = bumpMap2(sp, sn, 0.03f/(1.0f + t*0.25f));

        float3 ref = reflect(rd, sn); // Reflected ray.

        float3 oCol = texFaces(sp, svn); // Texture color at the surface point.
        if(edge>0.001f) oCol = texEdges(sp, svn);


        float sh = softShadow(sp, lp, 16.0f); // Soft shadows.
        float ao = calcAO(sp, sn); // Self shadows. Not too much.

        float3 ld = lp - sp; // Light direction.
        float lDist = _fmaxf(length(ld), 0.001f); // Light to surface distance.
        ld /= lDist; // Normalizing the light direction vector.

        float diff = _fmaxf(dot(ld, sn), 0.0f); // Diffuse component.
        float spec = _powf(_fmaxf(dot(reflect(-ld, sn), -rd), 0.0f), 32.0f); // Specular.

        float atten = 1.25f/(1.0f + lDist*0.1f + lDist*lDist*0.05f); // Attenuation.


        ///////////////
        // Cheap reflection: Not entirely accurate, but the reflections are pretty subtle, so not much 
        // effort is being put in.
        //
        float rt = refTrace(sp + ref*0.1f, ref); // Raymarch from "sp" in the reflected direction.
        float rEdge;
        float3 rsp = sp + ref*rt; // Reflected surface hit point.
        float3 rsn = normal(rsp, &rEdge); // Normal at the reflected surface.
        //rsn = bumpMap(rsp, rsn, 0.005f); // We're skipping the reflection bump to save some calculations.

        float3 rCol = texFaces(rsp, rsn); // Texel at "rsp."    
        if(rEdge>0.001f)  rCol = texEdges(rsp, rsn); // Reflection edges.

        float rDiff = _fmaxf(dot(rsn, normalize(lp-rsp)), 0.0f); // Diffuse light at "rsp."
        float rSpec = _powf(_fmaxf(dot(reflect(-1.0f*normalize(lp-rsp), rsn), -ref), 0.0f), 8.0f); // Diffuse light at "rsp."
        float rlDist = length(lp - rsp);
        // Reflected color. Not entirely accurate, but close enough. 
        rCol = (rCol*(rDiff*1.0f + to_float3(0.45f, 0.4f, 0.3f)) + to_float3(1.0f, 0.6f, 0.2f)*rSpec*2.0f);
        rCol *= 1.25f/(1.0f + rlDist*0.1f + rlDist*rlDist*0.05f);    
        ////////////////


        // Combining the elements above to light and color the scene.
        col = oCol*(diff*1.0f + to_float3(0.45f, 0.4f, 0.3f)) + to_float3(1.0f, 0.6f, 0.2f)*spec*2.0f;


        // Adding the reflection to the edges and faces. Technically, there should be less on the faces,
        // but after all that trouble, I thought I'd bump it up a bit. :)
        if(edge<0.001f) col += rCol*0.2f;
        else col += rCol*0.35f;
        // Alternate way to mix in the reflection. Sometimes, it's preferable, but not here.
        //if(edge<0.001f) col = _mix(col, rCol, 0.35f)*1.4f;
        //else col = _mix(col, rCol, 0.5f)*1.4f;


        // Shading the scene color and clamping. By the way, we're letting the color go beyond the maximum to
        // let the structure subtly glow a bit... Not really natural, but it looks a little shinier.
        col = _fminf(col*atten*sh*ao, to_float3_s(1.0f));
    
    //}
    
    // Mixing in some hazy bluish orange background.
    float3 bg = _mix(swi3(to_float3(0.5f, 0.7f, 1),z,y,x), swi3(to_float3(1, 0.7f, 0.3f),z,y,x), -rd.y*0.35f + 0.35f);
    col = _mix(col, bg, smoothstep(0.0f, FAR-25.0f, t)); //_fminf(swi3(bg,z,y,x)*to_float3(1.3f, 0.6f, 0.2f)*1.5f, 1.0f)
    
    // Postprocesing - A subtle vignette with a bit of warm coloring... I wanted to warm the atmosphere up
    // a bit. Uncomment it, if you want to see the bluer -possibly more natural looking - unprocessed version.
    float2 uv = fragCoord/iResolution;
    float vig = _powf(16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.125f);
    col *= to_float3(1.2f, 1.1f, 0.85f)*vig;

    // Rough gamma correction.
    fragColor = to_float4_aw(sqrt_f3(clamp(col, 0.0f, 1.0f)), 1.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}