

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

#define ZERO min(0, iFrame)

// Far plane, or max ray distance.
#define FAR 40.

// Minimum surface distance. Used in various calculations.
#define DELTA .001


// Ray passes: For this example, just one intersection and one reflection.
#define PASSES 2

// Global block scale. Something that is obvious to me now is that rotated
// cubes fill in cells that have dimensional rations consisting of the diagonal 
// face length along the X direction the the longest diagonal along the Y direction.
// That ratio turns out to be sqrt(3)/2. Common sense, and obvious... once I'd
// spent ages trying to fit a square peg into a round hole. :)
//
// The divisor, "4", is variable global scaling.
#define GSCALE vec2(1., .8660254)/4. 



// Scene object ID to separate the mesh object from the terrain.
float objID;

// Standard 2D rotation formula.
mat2 rot2(in float a){ float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

// IQ's vec2 to float hash.
float hash21(vec2 p){  return fract(sin(dot(p, vec2(27.609, 57.583)))*43758.5453); }

// Getting the video texture. I've deliberately stretched it out to fit across the screen,
// which means messing with the natural aspect ratio.
//
// By the way, it'd be nice to have a couple of naturally wider ratio videos to choose from. :)
//
vec3 getTex(sampler2D tex, vec2 p){
    
    // Strething things out so that the image fills up the window. You don't need to,
    // but this looks better. I think the original video is in the oldschool 4 to 3
    // format, whereas the canvas is along the order of 16 to 9, which we're used to.
    // If using repeat textures, you'd comment the first line out.
    //p *= vec2(iResolution.y/iResolution.x, 1);
    vec3 tx = texture(tex, p).xyz;
    return tx*tx; // Rough sRGB to linear conversion.
}




// IQ's signed box formula.
float sBoxS(in vec3 p, in vec3 b, in float sf){

  p = abs(p) - b + sf;
  return length(max(p, 0.)) + min(max(max(p.x, p.y), p.z), 0.) - sf;
}

/*
// This is a trimmed down version of one of Gaz's clever routines. I find it a 
// lot cleaner than those functions full of a million trigonometric variables.
vec3 rot3(in vec3 p, vec3 a, float t){
	a = normalize(a);
	vec3 q = cross(a, p), u = cross(q, a);
    return mat3(u, q, a)*vec3(cos(t), sin(t), dot(p, a));
}
*/

// Hacked in at the last minute to differentiate between the block and
// the metallic trimmings.
float blockPartID;


vec3 gCoord;
// Block ID -- It's a bit lazy putting it here, but it works. :)
vec4 gID;

// The extruded image.
float map(vec3 p){
    
    
    
    // Reflecting the wall opposite to give the light something to relect off of.
    p.z = abs(p.z + 1.) - 1.;
    
    // Wall behind the pylons to stop the light getting through, if we decide
    // to contract the boxes.
    float wall = -p.z + .16;
    

    // Scaling: This can be confusing, since the scene looks like it's rendering on 
    // a 3D grid, but it's a 2D grid of rotated boxes all aligned on a flat plane.
    // The "vec2(1, 2)" scaling figure accounts for the fact that two overlapping
    // rows of boxes are rendering -- in a similar way that we render a 2D hexagon grid.
    vec2 s = GSCALE*vec2(1, 2);
    
    // Box, box smoothing factor, and the gap between boxes.
    float bx = 1e5, sf = .001, gap = .003;
    
    
    // The rotated cubes on the diagonal cover a larger area relating to 
    // the diagonal distance. After getting things wrong for way too long,
    // I now know this. :)
    float sc0 = s.x*sqrt(2.)/2.;
    
    
    // The fist layer contains cubes that encroach on neighboring boundaries,
    // so two overlapping grids are necessary... Technically, you could get
    // away with one, but artifacts can arise.
    for(int i = ZERO; i<2; i++){
        
        // Global coordinates.
        vec3 q = p;
        
        vec2 cntr = i==0? vec2(.5) : vec2(0); // Offset position for each row.
        vec2 ip = floor(q.xy/s - cntr) + cntr + .5; // Local tile ID.
        q.xy -= (ip)*s; // New local position.

        // I can't for the life of me remember how I came up with the first 
        // rotation, but I'm guessing the side-on cube step view would involve
        // a 1-1-sqrt(2) triangle, and the angle would arise from that.
        q.yz *= rot2(.61547518786); // atan(1., sqrt(2.)).
        // A quater PI rotation around the Y axis is much easier to visualize.
        q.xz *= rot2(3.14159/4.); 
        
        // Smooth rounded box distance field.
        float bxi = sBoxS(q, vec3(sc0/2. - gap), sf);

        // If we have the closest of the two boxes, update the distance
        // field information.
        if(bxi<bx){
            bx = bxi;                   
            gID.yz = (ip);
            blockPartID = 0.;
            gCoord = q;
        }

    }
    
    
    // The second, third, etc, layers. Here, we're only doing two.
    
    // Scaling: Since the smaller boxes don't enchroach upon one another's 
    // boundaries, only one grid pre layer need be rendered.
    vec2 s2 = GSCALE*2.;
    // Box offset distance (relative to the larger box) and direction.
    float offsD = 0., dir = 1.;
    
    // Render two layers.
    for(int i = ZERO; i<2; i++){
        
        // Reducing size, scaling and offset for each succesive box layer.
        sc0 /= 2.;
        s2 /= 2.;
        offsD += s.x/8./pow(2., float(i));

        // Global coordinates, relatve to the large base layer box.
        vec3 q = p - vec3(0, 1, -1)*offsD;
        
        // Offset each second row by half a cell.
        vec2 cntr = vec2(0); 
        if(dir*mod(floor(q.y/s2.y), 2.)<dir*.5) cntr.x -= .5;
        vec2 ip2 = floor(q.xy/s2 - cntr) + cntr + .5; // Local tile ID.
        q.xy -= (ip2)*s2; // New local position.

        // Randomly break from the loop at certain levels, thus omitting
        // the rendering of all boxes that follow... Draw random boxes
        // would be another way to put it. L)
        if(hash21(ip2 + .03)<.2) break;
        
        // Rotating into the step position: Horizontal X-axis followed by
        // vertical Y axis rotation.
        q.yz *= rot2(.61547518786); // atan(1., sqrt(2.)).
        q.xz *= rot2(3.14159/4.);

        // Smooth rounded box distance field.
        float bxi = sBoxS(q, vec3(sc0/2. - gap), sf);
        
       
        // Updating the smoothing factor and direction for smaller boxes.
        sf *= .7;
        dir = -1.;

        // If we have the closest of the two boxes, update the distance
        // field information.
        if(bxi<bx){
           bx = bxi;
           gID.yz = (ip2);
           blockPartID = float(i + 1);//blPtID;
           gCoord = q;
        }
    
    }
    
    
    
    gID.x = bx;
 
    // Overall object ID.
    //objID = d4.x<wall || d4.x<bx? 0. : bx<wall? 1. : 2.;
    objID = bx<wall? 0. : 1.;
    //objID = 0.;
    
    // Combining the wall with the extruded blocks.
    return min(bx, wall);//min(min(wall, d4.x), bx);
 
}


// Basic raymarcher.
float trace(vec3 ro, vec3 rd){

    // Overall ray distance and scene distance.
    float t = 0., d;
    
    for(int i = ZERO; i<80; i++){
    
        d = map(ro + rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(d)<DELTA || t>FAR) break; // Alternative: .001*max(t*.25, 1.), etc.
        
        //t += i<32? d*.5 : d*.9; // Slower, but more accuracy.
        t += d*.9; 
    }

    return min(t, FAR);
}


// Cheap shadows are hard. In fact, I'd almost say, shadowing particular scenes with limited 
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
float softShadow(vec3 ro, vec3 lp, vec3 n, float k){

    // More would be nicer. More is always nicer, but not really affordable... 
    //Not on my slow test machine, anyway.
    const int iter = 32; 
    
    
    vec3 rd = lp - ro; // Unnormalized direction ray.
    
    ro += n*.0015; // Bumping the shadow off the hit point.
    

    float shade = 1.;
    float t = 0.; 
    float end = max(length(rd), 0.0001);
    //float stepDist = end/float(maxIterationsShad);
    rd /= end;
    
    //rd = normalize(rd + (hash33R(ro + n) - .5)*.03);
    

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, 
    // the lowest number to give a decent shadow is the best one to choose. 
    for (int i = ZERO; i<iter; i++){

        float d = map(ro + rd*t);
        shade = min(shade, k*d/t);
        //shade = min(shade, smoothstep(0., 1., k*d/t)); // Thanks to IQ for this tidbit.
        // So many options here, and none are perfect: 
        // dist += min(h, .2), dist += clamp(h, .01, stepDist), etc.
        t += clamp(d, .01, .1); 
        
        
        // Early exits from accumulative distance function calls tend to be a good thing.
        if (d<0. || t>end) break; 
    }

    // Sometimes, I'll add a constant to the final shade value, which lightens the shadow a bit --
    // It's a preference thing. Really dark shadows look too brutal to me. Sometimes, I'll add 
    // AO also just for kicks. :)
    return max(shade, 0.); 
}


// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
float calcAO(in vec3 p, in vec3 n){

	float sca = 2.5, occ = 0.;
    for( int i = ZERO; i<5; i++ ){
    
        float hr = float(i + 1)*.15/5.;        
        float d = map(p + n*hr);
        occ += (hr - d)*sca;
        sca *= .8;
        
        // Deliberately redundant line that may or may not stop the 
        // compiler from unrolling.
        if(sca>1e5) break;
    }
    
    return clamp(1. - occ, 0., 1.);
}


// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
vec3 getNormal(in vec3 p) {
	
    const vec2 e = vec2(.001, 0);
    
    //vec3 n = normalize(vec3(map(p + e.xyy) - map(p - e.xyy),
    //map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
    
    // This mess is an attempt to speed up compiler time by contriving a break... It's 
    // based on a suggestion by IQ. I think it works, but I really couldn't say for sure.
    float sgn = 1.;
    float mp[6];
    vec3[3] e6 = vec3[3](e.xyy, e.yxy, e.yyx);
    for(int i = ZERO; i<6; i++){
		mp[i] = map(p + sgn*e6[i/2]);
        sgn = -sgn;
        if(sgn>2.) break; // Fake conditional break;
    }
    
    return normalize(vec3(mp[0] - mp[1], mp[2] - mp[3], mp[4] - mp[5]));
}

/*
// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 texBump( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3(tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(.299, .587, .114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(.299, .587, .114)))/e.x; 
    
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
float n3D(in vec3 p){
    
	const vec3 s = vec3(7, 157, 113);
	vec3 ip = floor(p); p -= ip; 
    vec4 h = vec4(0., s.yz, s.y + s.z) + dot(ip, s);
    p = p*p*(3. - 2.*p); //p *= p*p*(p*(p * 6. - 15.) + 10.);
    h = mix(fract(sin(h)*43758.5453), fract(sin(h + s.x)*43758.5453), p.x);
    h.xy = mix(h.xz, h.yw, p.y);
    return mix(h.x, h.y, p.z); // Range: [0, 1].
}

float fBm(vec3 p){
    
    return n3D(p)*.57 + n3D(p*2.)*.28 + n3D(p*4.)*.15;
    
}

// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it 
// does give the impression that the surface is reflecting the surrounds in some way.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
vec3 envMap(vec3 p){
    
    p *= 3.;
    p.x += iTime/2.;
    
    float n3D2 = n3D(p*2.);
   
    // A bit of fBm.
    float c = n3D(p)*.57 + n3D2*.28 + n3D(p*4.)*.15;
    c = smoothstep(.45, 1., c); // Putting in some dark space.
    
    p = vec3(c, c*c, c*c*c*c); // Bluish tinge.
    
    return mix(p, p.xzy, n3D2*.4); // Mixing in a bit of purple.

}
*/

// Face pattern.
float fPat(vec2 p, float sc, vec2 gIP){

    // Simple maze pattern consisting of random diagonal lines.
    // Pretty standard.
    
    // Rotation and scaling.
    p *= rot2(3.14159/4.);
    sc *= 5.5*.7071;
    p *= sc;
    
    // Grid partioning:
    // Cell ID and local coordinates.
    vec2 ip = floor(p);
    p -= ip + .5;
    
    // Random cell flip.
    float rnd = hash21(ip + gIP*.123 +.01);
    if(rnd<.5) p.y = -p.y;
    
    // Render a diagonal line through the center, and
    // at the edges.
    vec2 ap = abs(p - .5);
    float d = abs((ap.x + ap.y)*.7071 - .7071);
    ap = abs(p);
    d = min(d, abs((ap.x + ap.y)*.7071 - .7071));
    d -= .125; // Line width.
    
    // Rescaling.
    return d/sc;
}

// Face pattern.
float fPat2(vec2 p, float sc){

    // Scaling.
    sc *= 11.;
    p *= sc;
    
    // Partioning into a grid of squares.
    p *= rot2(3.14159/2.); 
    p = fract(p + .5) - .5;

    // Squares.
    p = abs(p);
    float d = -(max(p.x, p.y) - (.5 - .125/2.));
    
    // Rescaling.
    return d/sc;
}


// Surface bump function..
float bumpSurf3D(in vec3 p, in vec3 n){
    
    map(p);
    
    vec3 txP = gCoord;
    // The objects (cubes) have been rotated in the distance function,
    // so the normal needs to match... If I had a dollar for every time
    // I've forgotten that... :)
    vec3 txN = n;
    txN.yz *= rot2(.61547518786); // atan(1., sqrt(2.)).
    txN.xz *= rot2(3.14159/4.); // 4.

    vec2 uv = abs(txN.x)>.5? txP.zy : abs(txN.y)>.5? txP.xz : txP.xy;
    //float faceID = abs(txN.x)>.5? 0. : abs(txN.y)>.5? 1. : 2.;
    
    
    
    vec2 scale = GSCALE;
    float sc = scale.x/sqrt(2.)/2. -  .003;//*(blockPartID + 1.);
    sc /= exp2(blockPartID); // Dividing smaller cubes by powers of 2.
                
    vec2 auv = abs(uv);
    float fdf = abs(max(auv.x, auv.y) - sc) - .002;        
    
    //uv += blockPartID<2.? .25 : .0;
    float pat2 = fPat2(uv/1.03 + .25, 2.);// + blockPartID*.5
    
    sc *= exp2(blockPartID);
    pat2 = smoothstep(0., .0002/sc/1., pat2);

    pat2 = mix(0., pat2, (1. - smoothstep(0., .0002/sc, fdf - .02 + .002 + .009)));
    
    return pat2;

}


 
// Standard function-based bump mapping routine: This is the cheaper four tap version. There's
// a six tap version (samples taken from either side of each axis), but this works well enough.
vec3 doBumpMap(in vec3 p, in vec3 n, float bumpfactor){
    
    // Larger sample distances give a less defined bump, but can sometimes lessen the aliasing.
    const vec2 e = vec2(.001, 0);  
    
    // This utter mess is to avoid longer compile times. It's kind of 
    // annoying that the compiler can't figure out that it shouldn't
    // unroll loops containing large blocks of code.
    mat4x3 p4 = mat4x3(p, p - e.xyy, p - e.yxy, p - e.yyx);
    
    vec4 b4;
    for(int i = ZERO; i<4; i++){
        b4[i] = bumpSurf3D(p4[i], n);
        if(n.x>1e5) break; // Fake break to trick the compiler.
    }
    
    // Gradient vector: vec3(df/dx, df/dy, df/dz);
    vec3 grad = (b4.yzw - b4.x)/e.x; 
   
    
    // Six tap version, for comparisson. No discernible visual difference, in a lot of cases.
    //vec3 grad = vec3(bumpSurf3D(p - e.xyy) - bumpSurf3D(p + e.xyy),
    //                 bumpSurf3D(p - e.yxy) - bumpSurf3D(p + e.yxy),
    //                 bumpSurf3D(p - e.yyx) - bumpSurf3D(p + e.yyx))/e.x*.5;
    
  
    // Adjusting the tangent vector so that it's perpendicular to the normal. It's some kind 
    // of orthogonal space fix using the Gram-Schmidt process, or something to that effect.
    grad -= n*dot(n, grad);          
         
    // Applying the gradient vector to the normal. Larger bump factors make things more bumpy.
    return normalize(n + grad*bumpfactor);
	
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){

    
    // Screen coordinates.
	vec2 uv = (fragCoord - iResolution.xy*.5)/iResolution.y;
    
    vec2 ra = vec2(1, iResolution.x/iResolution.y);
    uv *= 1. + (dot(uv*ra, uv*ra)*.05 - .025);
	

    // Ray origin.
	vec3 ro = vec3(iTime/6., 0, -1); 
    // "Look At" position.
    vec3 lk = ro + vec3(.01*0., .01*0., .1);//vec3(0, -.25, iTime);  
 
    // Light positioning.
 	vec3 lp = ro + vec3(.25, .25, .7); //.25// Put near the camera.
	

    // Using the above to produce the unit ray-direction vector.
    float FOV = 1.; // FOV - Field of view.
    vec3 fwd = normalize(lk - ro);
    vec3 rgt = normalize(vec3(fwd.z, 0., -fwd.x )); 
    // "right" and "forward" are perpendicular, due to the dot product being zero. Therefore, I'm 
    // assuming no normalization is necessary? The only reason I ask is that lots of people do 
    // normalize, so perhaps I'm overlooking something?
    vec3 up = cross(fwd, rgt); 

    // Unit direction ray.
    vec3 rd = normalize(uv.x*rgt + uv.y*up + fwd/FOV);
    
    
    // Camera position. Initially set to the ray origin.
    vec3 cam = ro;
    // Surface postion. Also initially set to the ray origin.
    vec3 sp = ro; 
    
    float gSh = 1.;
    float objRef = 1.;
     
    vec3 col = vec3(0);
    
   
    
    float alpha = 1.;
    
    for(int j = ZERO; j<PASSES; j++){
        
        // Layer or pass color. Each pass color gets blended in with
        // the overall result.
        vec3 colL = vec3(0);

        // Used for refractions, but not here.    
        //float distanceFactor = 1.;

        
        // Raymarch to the scene.
        float t = trace(sp, rd);

        // Saving the object ID, block ID and cell object (block part) ID.
        float svObjID = objID;
        vec4 svGID = gID;
        float svBPID = blockPartID;
        vec3 svCoord = gCoord;

        // Advance the ray to the surface. This becomes the new ray origin for the
        // next pass.
        sp += rd*t;
        
        
        // If the ray hits a surface, light it up. By the way, it's customary to put 
        // all of the following inside a single function, but I'm keeping things simple.
        // Blocks within loops used to kill GPU performance, but it doesn't seem to
        // effect the new generation systems.
      
        if(t<FAR){

            // Surface normal.
            vec3 sn = getNormal(sp);// *distanceFactor; // For refractions.
            
            // Function based bump mapping for the edges.
            sn = doBumpMap(sp, sn, .03);

            
            // The reflective ray, which tends to be very helpful when
            // calculating reflections. :)
            vec3 reflection = reflect(rd, sn);
            
            vec3 ld = lp - sp; // Point light direction.
            float lDist = length(ld); // Surface to light distance.
            ld /= max(lDist, .0001); // Normalizing.
            
            
            // Shadows and ambient self shadowing.
            //
            // Shadows are expensive. It'd be nice to include shadows on each bounce,
            // but it's still not really viable, so we just perform them on the 
            // first pass... Years from now, I'm hoping it won't be an issue.
            float sh = 1.;
            if(j == 0) sh = softShadow(sp, lp, sn, 8.);
            float ao = calcAO(sp, sn); // Ambient occlusion.
            gSh = min(sh, gSh); // Adding a touch of light to the shadow.
          
            float att = 1./(1. + lDist*lDist*.05); // Attenuation.

            float dif = max(dot(ld, sn), 0.); // Diffuse lighting.
            float spe = pow(max(dot(reflection, ld), 0.), 32.);
            float fre = clamp(1. + dot(rd, sn), 0., 1.); // Fresnel reflection term.
            
            dif = pow(dif, 4.)*2.; // Ramping up the diffuse.

            float Schlick = pow( 1. - max(dot(rd, normalize(rd + ld)), 0.), 5.);
            float freS = mix(.25, 1., Schlick);  //F0 = .2 - Glass... or close enough.

      
            // Object color.
           vec3 oCol;
            
             
            if(svObjID == 0.) {
            
                // Cube face coloring.
            
                // Face scaling.
                vec2 scale = GSCALE;
                float sc = scale.x/sqrt(2.)/2. -  .003;
                // Dividing smaller cubes by powers of 2.
                float sc2 = sc/exp2(svBPID);
                
                // Local face texture coordinates.
                vec3 txP = svCoord;
                
                // The objects (cubes) have been rotated in the distance function,
                // so the normal needs to match... If I had a dollar for every time
                // I've forgotten that... :)
                vec3 txN = sn;
                txN.yz *= rot2(.61547518786); // atan(1., sqrt(2.)).
                txN.xz *= rot2(3.14159/4.); // 4.
                
                // Using the normal to obtain the UV coordinates for the cube face.
                vec2 uv = abs(txN.x)>.5? txP.zy : abs(txN.y)>.5? txP.xz : txP.xy;
                float faceID = abs(txN.x)>.5? 0. : abs(txN.y)>.5? 1. : 2.;

         
                // Block coloring.
                vec3 tx = getTex(iChannel0, uv + hash21(svGID.yz));
                tx = smoothstep(0., .5, tx);

 
                // Edge coloring, but has been replaced by trimming.
                // Needs to match the box function inside the distance field.
                
                 
                // Random face value (blinking).
                float rndF = hash21(svGID.yz + svBPID/6. + .08);
                rndF = smoothstep(.92, .97, sin(6.2831*rndF + iTime)*.5 + .5);
                // Blinking face light color.
                vec3 blCol = mix(vec3(3.2, .7, .3), vec3(3.4, .4, .6), hash21(svGID.yz + .32));
                blCol = mix(blCol, blCol.zyx, step(.5, hash21(svGID.yz + .21)));
                //blCol = mix(blCol, blCol.zyx, floor(hash21(svGID.yz + .21)*2.999)/2.);
                // Face glow color.
                vec3 glCol = smoothstep(0., 1., 1. - length(uv)/sc2)*blCol*6.;
        
                float pat2 = fPat2(uv, 1./.125);
                vec3 metCol = vec3(1.1, .9, 1.1);
                oCol = tx/2.*metCol;

                //if(svBPID <= 1.){
                    // Applying spherical glow to the face.
                    oCol = mix(oCol, glCol, rndF + .01); 
                    oCol = mix(oCol, vec3(0), (1. - smoothstep(0., .0003/sc2, pat2))*.9);
                // }
               
                // Face pattern.
                float pat = fPat(uv, 1./.125, svGID.yz + faceID/6. + svBPID/6.);
                
                // Edge pattern.
                pat2 = fPat2(uv/1.03 + .25, 2.);
                
                // Edge coloring.
                vec3 eCol = mix(tx*.5 + .5, tx*.5 + .3 + blCol*.15, rndF)/3.;
                // Edge pattern.
                eCol = mix(eCol, vec3(0), (1. - smoothstep(0., .0002/sc, pat2))*.5);
                // Fake bump... Needs work.
                //eCol = mix(eCol, vec3(1), (1. - smoothstep(0., .0004/sc, pat2 - .001))*.35);
                //eCol = mix(eCol, vec3(0), (1. - smoothstep(0., .0002/sc, pat2))*.9);
                eCol *= metCol;

                // Applying the maze pattern to the faces.
                oCol = mix(oCol, oCol/8., (1. - smoothstep(0., .0002/sc, pat)));

                // Applying the edge color.
                vec2 auv = abs(uv);
                float fdf = abs(max(auv.x, auv.y) - sc2) - .002;
                oCol = mix(oCol, vec3(0), (1. - smoothstep(0., .0002/sc, fdf - .02 + .002)));
                oCol = mix(oCol, eCol, (1. - smoothstep(0., .0002/sc, fdf - .02 + .002 + .009)));

                // Different reflectance on different parts of the surface.
                //
                // Reflectance of the glass (or perspex) face pattern.
                float faceRef = mix(.25, .5, (1. - smoothstep(0., .0002/sc, pat)));
                // Reflectance of the face and edge.
                objRef = mix(faceRef, .1, (1. - smoothstep(0., .0002/sc, fdf - .02 + .002 + .009)));
             
       
            }
            else {
                // Dark wall behind the tiny gaps in the blocks. 
                oCol = vec3(0);
                objRef = .0;
            
            }
 
 
            // The color for this layer.
            colL = oCol*(dif*gSh + .1 + vec3(1, .8, .5)*spe*1.*gSh);
        
           
            // Optional environmental mapping. Not used.
            //vec3 envCol = envMap(reflect(rd, sn)).zyx;
            //colL += colL*envCol*3.;
            
            colL *= ao*att;

            // Used for refraction, but not used here.
            //if(distanceFactor<0.)  colL *= exp(-colL*t*5.);
            
            
            // Set the unit direction ray to the new reflected direction, and bump the 
            // ray off of the hit point by a fraction of the normal distance. Anyone who's
            // been doing this for a while knows that you need to do this to stop self
            // intersection with the current launch surface from occurring... It used to 
            // bring me unstuck all the time. I'd spend hours trying to figure out why my
            // reflections weren't working. :)
            rd = reflection;
            sp += sn*DELTA*1.1;

        }

        // Fog: Redundant here, since the ray doesn't go far, but necessary for other setups.
        //float td = length(sp - cam); 
        //vec3 fogCol = vec3(0);
        //colL = mix(colL, fogCol, smoothstep(0., .95, td/FAR));
      
        // This is a more subtle way to blend layers. 
        //col = mix(col, colL, 1./float(1 + j)*alpha);
        // In you face additive blend. Sometimes, I prefer this.
        col += colL*alpha;
        
        // If the hit object's reflective factor is zero, or the ray has reached
        // the far horizon, break.
        if(objRef < .001 || t >= FAR) break;
        
        // Decrease the alpha factor (ray power of sorts) by the hit object's reflective factor.
        alpha *= objRef;
    }
    
    // Rough gamma correction.
    //fragColor = vec4(pow(max(col, 0.), vec3(1./2.2)), 1);
    fragColor = vec4(sqrt(max(col, 0.)), 1);
    
}