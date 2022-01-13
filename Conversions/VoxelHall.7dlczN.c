
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Blue Noise' to iChannel0


/*
    Voxel Hall Colors
    
    Forked from Shane's Voxel Corridor.
    I remove the texture and added a surface color with a time gradient.
    Changing the ambient light's color would have been more "correct" but I liked
    the way this looks.
    I also experimented a bit with different bumpmap textures, and settled on just using 
    the original procedural bricks.

    Original author's comments below.
    --------------
    
  Voxel Corridor
  --------------

  I love the voxel aesthetic, so after looking at some of Akohdr's examples, I went on a bit 
  of a voxel trip and put this simple scene together... Although, "scene" would  be putting 
  it loosely. :)

  Quasi-discreet distance calculations sound simple enough to perform in theory, but are just 
  plain fiddly to code, so I was very thankful to have fb39ca4's, IQ's, Reinder's, and everyone 
  elses voxel examples to refer to.

  The code is pretty straight forward. I tried my best to write it in such way that enables
  someone to plug in any normal distance function and have it render the voxelized version.

  Mainly based on the following:

  Voxel Ambient Occlusion - fb39ca4
    https://www.shadertoy.com/view/ldl3DS

  Minecraft - Reinder
    https://www.shadertoy.com/view/4ds3WS

  Other examples:
  Rounded Voxels - IQ
    https://www.shadertoy.com/view/4djGWR

  Sampler - w23
  https://www.shadertoy.com/view/MlfGRM

  Text In Space - akohdr
  https://www.shadertoy.com/view/4d3SWB

*/

#define PI 3.14159265f
#define FAR 60.0f

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define swi2S(a,b,c,d) {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 

// 2x2 matrix rotation. Note the absence of "cos." It's there, but in disguise, and comes courtesy
// of Fabrice Neyret's "ouside the box" thinking. :)
__DEVICE__ mat2 rot2( float a ){ float2 v = sin_f2(to_float2(1.570796f, 0) + a);  return to_mat2(v.x,v.y, -v.y, v.x); }

// Tri-Planar blending function. Based on an old Nvidia tutorial.
__DEVICE__ float3 tex3D( __TEXTURE__ tex, in float3 p, in float3 n ){
  
    n = _fmaxf(abs_f3(n), to_float3_s(0.001f));//n = _fmaxf((_fabs(n) - 0.2f)*7.0f, 0.001f); //  etc.
    n /= (n.x + n.y + n.z ); 
    p = swi3(texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z,x,y,z);
    return p*p;
}

// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
__DEVICE__ float2 path(in float z){ 
    //return to_float2(0); // Straight.
    float a = _sinf(z * 0.11f);
    float b = _cosf(z * 0.14f);
    return to_float2(a*4.0f -b*1.5f, b*1.7f + a*1.5f); 
}

/*
// Alternate distance field -- Twisted planes. 
__DEVICE__ float map(float3 p){
    
     // You may need to reposition the light to work in with the shadows, but for
     // now, I'm repositioning the scene up a bit.
     p.y -= 0.75f;
     swi2(p,x,y) -= path(p.z); // Move the scene around a sinusoidal path.
     swi2(p,x,y) = rot2(p.z/8.0f)*swi2(p,x,y); // Twist it about XY with respect to distance.
    
     float n = dot(_sinf(p*1.0f + _sinf(swi3(p,y,z,x)*0.5f + iTime*0.0f)), to_float3_s(0.25f)); // Sinusoidal layer.
     
     return 4.0f - _fabs(p.y) + n; // Warped double planes, "_fabs(p.y)," plus surface layers.
 
}
*/

// Standard perturbed tunnel function.
//
__DEVICE__ float map(float3 p){
     
     // Offset the tunnel about the XY plane as we traverse Z.
     swi2S(p,x,y,swi2(p,x,y) - path(p.z));
    
     // Standard tunnel. Comment out the above first.
     float n = 5.0f - length(swi2(p,x,y)*to_float2(1, 0.8f));
    
     // Square tunnel. Almost redundant in a voxel renderer. :)
     //n = 4.0f - _fmaxf(_fabs(p.x), _fabs(p.y)); 
     
     // Tunnel with a floor.
     return _fminf(p.y + 3.0f, n); //n = _fminf(-_fabs(p.y) + 3.0f, n);
 
}

/*
__DEVICE__ float brickShade(float2 p){
    
    p.x -= step(p.y, 1.0f)*0.5f;
    
    p = fract(p);
    
    return _powf(16.0f*p.x*p.y*(1.0f-p.x)*(1.0f-p.y), 0.25f);
    
}
*/

// The brick groove pattern. Thrown together too quickly.
// Needs some tidy up, but it's quick enough for now.
//
const float w2h = 2.0f; // Width to height ratio.
const float mortW = 0.05f; // Morter width.

__DEVICE__ float brickMorter(float2 p){
  
    p.x -= step(1.0f, p.y)*0.5f;
    
    p = abs_f2(fract_f2(p + to_float2(0, 0.5f)) - 0.5f)*2.0f;
    
    // Smooth grooves. Better for bump mapping.
    return smoothstep(0.0f, mortW, p.x)*smoothstep(0.0f, mortW*w2h, p.y);
    
}

__DEVICE__ float brick(float2 p){
    
  p = fract_f2(p*to_float2(0.5f/w2h, 0.5f))*2.0f;

    return brickMorter(p);
}


// Surface bump function. Cheap, but with decent visual impact.
__DEVICE__ float bumpSurf3D( in float3 p, in float3 n){

    n = abs_f3(n);

    if      (n.x>0.5f)   swi2S(p,x,y, swi2(p,z,y))
    else if (n.y>0.5f)   swi2S(p,x,y, swi2(p,z,x))
     
    
    return brick(swi2(p,x,y));
    
}

// Standard function-based bump mapping function.
__DEVICE__ float3 doBumpMap(in float3 p, in float3 nor, float bumpfactor){
    
    const float2 e = to_float2(0.001f, 0);
    float ref = bumpSurf3D(p, nor);                 
    float3 grad = (to_float3(bumpSurf3D(p - swi3(e,x,y,y), nor),
                             bumpSurf3D(p - swi3(e,y,x,y), nor),
                             bumpSurf3D(p - swi3(e,y,y,x), nor) )-ref)/e.x;                     
          
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
  
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
__DEVICE__ float3 doBumpMap( __TEXTURE__ tx, in float3 p, in float3 n, float bf){
   
    const float2 e = to_float2(0.001f, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = to_mat3_f3( tex3D(tx, p - swi3(e,x,y,y), n), tex3D(tx, p - swi3(e,y,x,y), n), tex3D(tx, p - swi3(e,y,y,x), n));
    
    float3 g = mul_f3_mat3(to_float3(0.299f, 0.587f, 0.114f),m); // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), to_float3(0.299f, 0.587f, 0.114f)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
    
}


// This is just a slightly modified version of fb39ca4's code, with some
// elements from IQ and Reinder's examples. They all work the same way:
// Obtain the current voxel, then test the distance field for a hit. If
// the ray has moved into the voxelized isosurface, break. Otherwise, move
// to the next voxel. That involves a bit of decision making - due to the
// nature of voxel boundaries - and the "mask," "side," etc, variable are
// an evolution of that. If you're not familiar with the process, it's 
// pretty straight forward, and there are a lot of examples on Shadertoy, 
// plus a lot more articles online.
//
__DEVICE__ float3 voxelTrace(float3 ro, float3 rd, out float3 *mask){
    
    float3 p = _floor(ro) + 0.5f;

  float3 dRd = 1.0f/abs_f3(rd); // 1.0f/_fmaxf(_fabs(rd), to_float3_s(0.0001f));
  rd = sign_f3(rd);
  float3 side = dRd*(rd * (p - ro) + 0.5f);
    
  *mask = to_float3_s(0);
  
  for (int i = 0; i < 64; i++) {
    
    if (map(p)<0.0f) break;
        
    // Note that I've put in the messy reverse step to accomodate
    // the "less than or equals" logic, rather than just the "less than."
    // Without it, annoying seam lines can appear... Feel free to correct
    // me on that, if my logic isn't up to par. It often isn't. :)
    *mask = step(side, swi3(side,y,z,x))*(1.0f-step(swi3(side,z,x,y), side));
    side += *mask*dRd;
    p += *mask * rd;
  }
    
    return p;    
}


///////////
//
// This is a trimmed down version of fb39ca4's voxel ambient occlusion code with some 
// minor tweaks and adjustments here and there. The idea behind voxelized AO is simple. 
// The execution, not so much. :) So damn fiddly. Thankfully, fb39ca4, IQ, and a few 
// others have done all the hard work, so it's just a case of convincing yourself that 
// it works and using it.
//
// Refer to: Voxel Ambient Occlusion - fb39ca4
// https://www.shadertoy.com/view/ldl3DS
//
__DEVICE__ float4 voxelAO(float3 p, float3 d1, float3 d2) {
   
    // Take the four side and corner readings... at the correct positions...
    // That's the annoying bit that I'm glad others have worked out. :)
    float4 side = to_float4(map(p + d1), map(p + d2), map(p - d1), map(p - d2));
    float4 corner = to_float4(map(p + d1 + d2), map(p - d1 + d2), map(p - d1 - d2), map(p + d1 - d2));
  
    // Quantize them. It's either occluded, or it's not, so to speak.
    side = step(side, to_float4_s(0));
    corner = step(corner, to_float4_s(0));
    
    // Use the side and corner values to produce a more honed in value... kind of.
    return to_float4_s(1.0f) - (side + swi4(side,y,z,w,x) + _fmaxf(corner, side*swi4(side,y,z,w,x)))/3.0f;    
  
}

__DEVICE__ float calcVoxAO(float3 vp, float3 sp, float3 rd, float3 mask) {
    
    // Obtain four AO values at the appropriate quantized positions.
    float4 vAO = voxelAO(vp - sign_f3(rd)*mask, swi3(mask,z,x,y), swi3(mask,y,z,x));
    
    // Use the fractional voxel postion and and the proximate AO values
    // to return the interpolated AO value for the surface position.
    sp = fract_f3(sp);
    float2 uv = swi2(sp,y,z)*mask.x + swi2(sp,z,x)*mask.y + swi2(sp,x,y)*mask.z;
    return _mix(_mix(vAO.z, vAO.w, uv.x), _mix(vAO.y, vAO.x, uv.x), uv.y);

}
///////////

__KERNEL__ void VoxelHallFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  
    // Screen coordinates.
    float2 uv = (fragCoord - iResolution*0.5f)/iResolution.y;
    
    // Camera Setup.
    float3 camPos = to_float3(0.0f, 0.5f, iTime*8.0f); // Camera position, doubling as the ray origin.
    float3 lookAt = camPos + to_float3(0.0f, 0.0f, 0.25f);  // "Look At" position.

 
    // Light positioning. 
    float3 lightPos = camPos + to_float3(0, 2.5f, 8);// Put it a bit in front of the camera.

    // Using the Z-value to perturb the XY-plane.
    // Sending the camera, "look at," and two light vectors down the tunnel. The "path" function is 
    // synchronized with the distance function. Change to "path2" to traverse the other tunnel.
    swi2S(lookAt,x,y,  swi2(lookAt,x,y) + path(lookAt.z));
    swi2S(camPos,x,y,   swi2(camPos,x,y) + path(camPos.z));
    swi2S(lightPos,x,y, swi2(lightPos,x,y) + path(lightPos.z));

    // Using the above to produce the unit ray-direction vector.
    float FOV = PI/2.0f; // FOV - Field of view.
    float3 forward = normalize(lookAt-camPos);
    float3 right = normalize(to_float3(forward.z, 0.0f, -forward.x )); 
    float3 up = cross(forward, right);

    // rd - Ray direction.
    float3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    
    //vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    //rd = normalize(to_float3(swi2(rd,x,y), rd.z - dot(swi2(rd,x,y), swi2(rd,x,y))*0.25f));    
    
    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
    swi2S(rd,x,y,  mul_mat2_f2(rot2( path(lookAt.z).x/24.0f) , swi2(rd,x,y)));

    // Raymarch the voxel grid.
    float3 mask;
    float3 vPos = voxelTrace(camPos, rd, &mask);
  
    // Using the voxel position to determine the distance from the camera to the hit point.
    // I'm assuming IQ is responsible for this clean piece of logic.
    float3 tCube = (vPos-camPos - 0.5f*sign_f3(rd))/rd;
    float t = _fmaxf(max(tCube.x, tCube.y), tCube.z);
    
  
    // Initialize the scene color.
    float3 sceneCol = to_float3_s(0);
  
    // The ray has effectively hit the surface, so light it up.
    if(t<FAR){
  
     
        // Surface position and surface normal.
        float3 sp = camPos + rd*t;
        
        // Voxel normal.
        float3 sn = -1.0f*(mask * sign_f3( rd ));

        // Sometimes, it's necessary to save a copy of the unbumped normal.
        float3 snNoBump = sn;
        
        // I try to avoid it, but it's possible to do a texture bump and a function-based
        // bump in succession. It's also possible to roll them into one, but I wanted
        // the separation... Can't remember why, but it's more readable anyway.
        //
        // Texture scale factor.
        const float tSize0 = 1.0f/4.0f;
        // Texture-based bump mapping.
        //sn = doBumpMap(iChannel0, sp*tSize0, sn, 0.02f);

        // Function based bump mapping. Comment it out to see the under layer. It's pretty
        // comparable to regular beveled Voronoi... Close enough, anyway.
        sn = doBumpMap(sp, sn, 0.15f);
        
       
        // Ambient occlusion.
        float ao = calcVoxAO(vPos, sp, rd, mask) ;//calculateAO(sp, sn);//*0.75f + 0.25f;

        
        // Light direction vectors.
        float3 ld = lightPos-sp;

        // Distance from respective lights to the surface point.
        float lDist = _fmaxf(length(ld), 0.001f);
      
        // Normalize the light direction vectors.
        ld /= lDist;
      
        // Light attenuation, based on the distances above.
        float atten = 1.0f/(1.0f + lDist*0.2f + lDist*0.1f); // + distlpsp*distlpsp*0.025
        
        // Ambient light.
        float ambience = 0.25f;
        
        // Diffuse lighting.
        float diff = _fmaxf( dot(sn, ld), 0.0f);
       
        // Specular lighting.
        float spec = _powf(_fmaxf( dot( reflect(-ld, sn), -rd ), 0.0f ), 32.0f);
 
        // Object texturing.
        float3 tint;
        //tint = to_float3(0.94f, 0.38f, 0.57f); // rose
        //tint = to_float3(1, 0.6f, 1.0f); // lavender
        //tint = to_float3_s(0.5f); // greyscale
        //tint =  0.7f + 0.5f*_cosf(6.28318f*(to_float3(1.0f,0.1f,0.4f)*iTime*0.25f + to_float3(0.5f,0.15f,0.25f))) ;
        tint = 0.7f + 0.5f*cos_f3(6.28318f*(to_float3(2.0f,1.0f,0.0f)*iTime*0.25f + to_float3(0.5f,0.2f,0.25f)));
        float3 texCol = tint + step(_fabs(snNoBump.y), 0.5f);

      // Combining the above terms to produce the final color. It was based more on acheiving a
        // certain aesthetic than science.
        sceneCol = texCol*(diff + ambience) + to_float3(1.0f, 1.0f, 1.0f) *spec;
        

      // Shading.  
        sceneCol *= ao;
  }
       
  // Blend in a bit of logic-defying fog for atmospheric effect. :)
  sceneCol = _mix(sceneCol, to_float3(0.08f, 0.16f, 0.34f), smoothstep(0.0f, 0.95f, t/FAR)); // _expf(-0.002f*t*t), etc.

  // Clamp and present the badly gamma corrected pixel to the screen.
  fragColor = to_float4_aw(sqrt_f3(clamp(sceneCol, 0.0f, 1.0f)), 1.0f);
  


  SetFragmentShaderComputedColor(fragColor);
}