
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


/*
    Biomine
    -------
    
    A biocooling system for a futuristic, off-world mine... or a feeding mechanisn for an alien 
  hatchery? I wasn't really sure what I was creating when I started, and I'm still not. :) I at 
  least wanted to create the sense that the tubes were pumping some form of biomatter around 
  without having to resort to full reflective and refractive passes... I kind of got there. :)

  All things considered, there's not a lot to this. Combine a couple of gyroid surfaces, ID them,
  then add their respective material properties. The scene is simple to create, and explained in
  the distance function. There's also some function based, 2nd order cellular bump mapping, for 
  anyone interested.

  The fluid pumped through the tubes was created by indexing the reflected and refracted rays 
  into a basic environment mapping function. Not accurate, but simple, effective and way cheaper
  than the real thing.

  I'd just finished watching some of the Assembly 2016 entries on YouTube, so for better or
  worse, wanted to produce the scene without the help of any in-house textures.

    Related examples: 

    Cellular Tiling - Shane
    https://www.shadertoy.com/view/4scXz2

  Cellular Tiled Tunnel - Shane
  https://www.shadertoy.com/view/MscSDB

*/

// Max ray distance.
#define FAR 50.0f 




// Standard 1x1 hash functions. Using "cos" for non-zero origin result.
__DEVICE__ float hash( float n ){ return fract(_cosf(n)*45758.5453f); }


// 2x2 matrix rotation. Note the absence of "cos." It's there, but in disguise, and comes courtesy
// of Fabrice Neyret's "ouside the box" thinking. :)
__DEVICE__ mat2 rot2( float a ){ float2 v = sin_f2(to_float2(1.570796f+a, a) );  return to_mat2(v.x, v.y, -v.y, v.x); }


// Compact, self-contained version of IQ's 3D value noise function. I have a transparent noise
// example that explains it, if you require it.
__DEVICE__ float noise3D(in float3 p){
    
  const float3 s = to_float3(7, 157, 113);
  float3 ip = _floor(p); p -= ip; 
  float4 h = to_float4(0.0f, s.y, s.z, s.y + s.z) + dot(ip, s);
  p = p*p*(3.0f - 2.0f*p); //p *= p*p*(p*(p * 6.0f - 15.0f) + 10.0f);

  h = _mix(fract_f4(sin_f4(h)*43758.5453f), fract_f4(sin_f4(h + s.x)*43758.5453f), p.x);
  swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y));
  return _mix(h.x, h.y, p.z); // Range: [0, 1].
}

////////
// The cellular tile routine. Draw a few objects (four spheres, in this case) using a minumum
// blend at various 3D locations on a cubic tile. Make the tile wrappable by ensuring the 
// objects wrap around the edges. That's it.
//
// Believe it or not, you can get away with as few as three spheres. If you sum the total 
// instruction count here, you'll see that it's way, way lower than 2nd order 3D Voronoi.
// Not requiring a hash function provides the biggest benefit, but there is also less setup.
// 
// The result isn't perfect, but 3D cellular tiles can enable you to put a Voronoi looking 
// surface layer on a lot of 3D objects for little cost.
//
__DEVICE__ float drawSphere(in float3 p){
  
    p = fract_f3(p)-0.5f;    
    return dot(p, p);
    
    //p = abs_f3(fract_f3(p)-0.5f);
    //return dot(p, to_float3_s(0.5f));  
}


__DEVICE__ float cellTile(in float3 p){
    
    // Draw four overlapping objects (spheres, in this case) at various positions throughout the tile.
    float4 v, d; 
    d.x = drawSphere(p - to_float3(0.81f, 0.62f, 0.53f));
    swi2S(p,x,y, to_float2(p.y-p.x, p.y + p.x)*0.7071f);
    d.y = drawSphere(p - to_float3(0.39f, 0.2f, 0.11f));
    swi2S(p,y,z, to_float2(p.z-p.y, p.z + p.y)*0.7071f);
    d.z = drawSphere(p - to_float3(0.62f, 0.24f, 0.06f));
    swi2S(p,x,z, to_float2(p.z-p.x, p.z + p.x)*0.7071f);
    d.w = drawSphere(p - to_float3(0.2f, 0.82f, 0.64f));

    swi2S(v,x,y, _fminf(swi2(d,x,z), swi2(d,y,w)));
    v.z = _fminf(max(d.x, d.y), _fmaxf(d.z, d.w));
    v.w = _fmaxf(v.x, v.y); 
   
    d.x =  _fminf(v.z, v.w) - _fminf(v.x, v.y); // First minus second order, for that beveled Voronoi look. Range [0, 1].
    //d.x =  _fminf(v.x, v.y); // Minimum, for the cellular look.
        
    return d.x*2.66f; // Normalize... roughly.
}

// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
__DEVICE__ float2 path(in float z){ 
    //return to_float2(0);
    float a = _sinf(z * 0.11f);
    float b = _cosf(z * 0.14f);
    return to_float2(a*4.0f - b*1.5f, b*1.7f + a*1.5f); 
}


// Smooth maximum, based on IQ's smooth minimum function.
__DEVICE__ float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5f + 0.5f*(a-b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f-h)*s;
}


// The distance function. It's a lot simpler than it looks: The biological tubes are just a gyroid lattice.
// The mine tunnel, is created by takoing the negative space, and bore out the center with a cylinder. The
// two are combined with a smooth maximum to produce the tunnel with biotube lattice. On top of that, the 
// whole scene is wrapped around a path and slightly mutated (the first two lines), but that's it.

__DEVICE__ float map(float3 p, float iTime, float *objID){
  
    swi2S(p,x,y, swi2(p,x,y) - path(p.z)); // Wrap the scene around a path.

    p += cos_f3(swi3(p,z,x,y)*1.5707963f)*0.2f; // Perturb slightly. The mutation gives it a bit more of an organic feel.

    // If you're not familiar with a gyroid lattice, this is basically it. Not so great to hone in on, but
    // pretty cool looking and simple to produce.
    float d = dot(cos_f3(p*1.5707963f), sin_f3(swi3(p,y,z,x)*1.5707963f)) + 1.0f;

  // Biotube lattice. The final time-based term makes is heave in and out.
    float bio = d + 0.25f +  dot(sin_f3(p*1.0f + iTime*6.283f + sin_f3(swi3(p,y,z,x)*0.5f)), to_float3_s(0.033f));

    // The tunnel. Created with a bit of trial and error. The smooth maximum against the gyroid rounds it off
    // a bit. The abs term at the end just adds some variation via the beveled edges. Also trial and error.
    float tun = smaxP(3.25f - length(swi2(p,x,y) - to_float2(0, 1)) + 0.5f*_cosf(p.z*3.14159f/32.0f), 0.75f-d, 1.0f) - _fabs(1.5f-d)*0.375f;;// - sf*0.25f;


    *objID = step(tun, bio); // Tunnel and biolattice IDs, for coloring, lighting, bumping, etc, later.

    return _fminf(tun, bio); // Return the distance to the scene.
}


// Surface bump function. Cheap, but with decent visual impact.
__DEVICE__ float bumpSurf3D( in float3 p, float saveID){
    
    float bmp;
    float noi = noise3D(p*96.0f);
    
    if(saveID>0.5f){
      float sf = cellTile(p*0.75f); 
      float vor = cellTile(p*1.5f);
    
      bmp = sf*0.66f + (vor*0.94f + noi*0.06f)*0.34f;
    }
    else {
        p/=3.0f;//
        float ct = cellTile(p*2.0f + sin_f3(p*12.0f)*0.5f)*0.66f+cellTile(p*6.0f + sin_f3(p*36.0f)*0.5f)*0.34f;
        bmp = (1.0f-smoothstep(-0.2f, 0.25f, ct))*0.9f + noi*0.1f;
    }
    return bmp;
}

// Standard function-based bump mapping function.
__DEVICE__ float3 doBumpMap(in float3 p, in float3 nor, float bumpfactor, float saveID){
    
    const float2 e = to_float2(0.001f, 0);
    float ref = bumpSurf3D(p, saveID);                 
    float3 grad = (to_float3(bumpSurf3D(p - swi3(e,x,y,y), saveID),
                             bumpSurf3D(p - swi3(e,y,x,y), saveID),
                             bumpSurf3D(p - swi3(e,y,y,x), saveID) )-ref)/e.x;                     
          
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
}

// Basic raymarcher.
__DEVICE__ float trace(in float3 ro, in float3 rd, float iTime, float *objID){

    float t = 0.0f, h;
    for(int i = 0; i < 72; i++){
        h = map(ro+rd*t, iTime, objID);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(_fabs(h)<0.002f*(t*0.125f + 1.0f) || t>FAR) break; // Alternative: 0.001f*_fmaxf(t*0.25f, 1.0f)        
        t += step(h, 1.0f)*h*0.2f + h*0.5f;
    }
    return _fminf(t, FAR);
}

// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
__DEVICE__ float3 getNormal(in float3 p, float iTime, float *objID) {
  const float2 e = to_float2(0.002f, 0);
  return normalize(to_float3( map(p + swi3(e,x,y,y), iTime,objID) - map(p - swi3(e,x,y,y), iTime,objID), 
                              map(p + swi3(e,y,x,y), iTime,objID) - map(p - swi3(e,y,x,y), iTime,objID),
                              map(p + swi3(e,y,y,x), iTime,objID) - map(p - swi3(e,y,y,x), iTime,objID)));
}

// XT95's really clever, cheap, SSS function. The way I've used it doesn't do it justice,
// so if you'd like to really see it in action, have a look at the following:
//
// Alien Cocoons - XT95: https://www.shadertoy.com/view/MsdGz2
//
__DEVICE__ float thickness( in float3 p, in float3 n, float maxDist, float falloff, float iTime, float *objID )
{
  const float nbIte = 6.0f;
  float ao = 0.0f;
    
    for( float i=1.0f; i< nbIte+0.5f; i++ ){
      float l = (i*0.75f + fract(_cosf(i)*45758.5453f)*0.25f)/nbIte*maxDist;
      ao += (l + map( p -n*l, iTime, objID )) / _powf(1.0f + l, falloff);
    }
    return clamp( 1.0f-ao/nbIte, 0.0f, 1.0f);
}

/*
// Shadows.
__DEVICE__ float softShadow(float3 ro, float3 rd, float start, float end, float k){

    float shade = 1.0f;
    const int maxIterationsShad = 20;

    float dist = start;
    //float stepDist = end/float(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down.
    for (int i=0; i<maxIterationsShad; i++){
    
        float h = map(ro + rd*dist);
        shade = _fminf(shade, k*h/dist);

        // +=h, +=clamp( h, 0.01f, 0.25f ), +=_fminf( h, 0.1f ), +=stepDist, +=_fminf(h, stepDist*2.0f), etc.
        dist += clamp( h, 0.01f, 0.25f);//_fminf(h, stepDist);
        
        // Early exits from accumulative distance function calls tend to be a good thing.
        if (h<0.001f || dist > end) break; 
    }

    // Shadow value.
    return _fminf(max(shade, 0.0f) + 0.5f, 1.0f); 
}
*/


// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function, and in many cases, it gives really, really nice results. For a better version, and 
// usage, refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
__DEVICE__ float calculateAO( in float3 p, in float3 n, float iTime, float *objID )
{
  
  float ao = 0.0f, l;
  const float maxDist = 4.0f;
  const float nbIte = 6.0f;
  //const float falloff = 0.9f;
  for( float i=1.0f; i< nbIte+0.5f; i++ ){
    l = (i + hash(i))*0.5f/nbIte*maxDist;
    ao += (l - map( p + n*l, iTime, objID ))/(1.0f+ l);// / _powf(1.0f+l, falloff);
  }
  return clamp(1.0f- ao/nbIte, 0.0f, 1.0f);
}

/*
/////
// Code block to produce some layers of smokey haze. Not sophisticated at all.
// If you'd like to see a much more sophisticated version, refer to Nitmitz's
// Xyptonjtroz example. Incidently, I wrote this off the top of my head, but
// I did have that example in mind when writing this.

// Hash to return a scalar value from a 3D vector.
__DEVICE__ float hash31(float3 p){ return fract(_sinf(dot(p, to_float3(127.1f, 311.7f, 74.7f)))*43758.5453f); }

// Four layers of cheap cell tile noise to produce some subtle mist.
// Start at the ray origin, then take four samples of noise between it
// and the surface point. Apply some very simplistic lighting along the 
// way. It's not particularly well thought out, but it doesn't have to be.
__DEVICE__ float getMist(in float3 ro, in float3 rd, in float3 lp, in float t){

    float mist = 0.0f;
    ro += rd*t/64.0f; // Edge the ray a little forward to begin.
    
    for (int i = 0; i<8; i++){
      // Lighting. Technically, a lot of these points would be
      // shadowed, but we're ignoring that.
      float sDi = length(lp-ro)/FAR; 
      float sAtt = _fminf(1.0f/(1.0f + sDi*0.25f + sDi*sDi*0.25f), 1.0f);
      // Noise layer.
      //float n = trigNoise3D(ro/2.0f);//noise3D(ro/2.0f)*0.66f + noise3D(ro/1.0f)*0.34f;
      float n = cellTile(ro/2.0f);
      mist += n*sAtt;//trigNoise3D
      // Advance the starting point towards the hit point.
      ro += rd*t/8.0f;
    }
    
    // Add a little noise, then clamp, and we're done.
    return clamp(mist/4.0f + hash31(ro)*0.2f-0.1f, 0.0f, 1.0f);
}
*/

//////
// Simple environment mapping. Pass the reflected vector in and create some
// colored noise with it. The normal is redundant here, but it can be used
// to pass into a 3D texture mapping function to produce some interesting
// environmental reflections.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm
__DEVICE__ float3 eMap(float3 rd, float3 sn, float iTime){
    
    // Add a time component, scale, then pass into the noise function.
    rd.y += iTime;
    rd /= 3.0f;

    // Biotube texturing.
    float ct = cellTile(rd*2.0f + sin_f3(rd*12.0f)*0.5f)*0.66f + cellTile(rd*6.0f + sin_f3(rd*36.0f)*0.5f)*0.34f;
    float3 texCol = (to_float3(0.25f, 0.2f, 0.15f)*(1.0f-smoothstep(-0.1f, 0.3f, ct)) + to_float3(0.02f, 0.02f, 0.53f)/6.0f); 
    return smoothstep(to_float3_s(0.0f), to_float3_s(1.0f), texCol);
}


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}


__KERNEL__ void BiomineFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{



  CONNECT_CHECKBOX0(Textur, 0);
  CONNECT_COLOR0(TubeColor, 0.35f, 0.25f, 0.2f, 1.0f);
  CONNECT_COLOR1(SceneColor, 0.7f, 0.9f, 1.0f, 1.0f);
  CONNECT_COLOR2(SkyColor, 2.0f, 0.9f, 0.8f, 1.0f);
  CONNECT_COLOR3(WallColor, 0.3f, 0.3f, 0.3f, 1.0f);
  //CONNECT_COLOR3(Scene3Color, 0.8f, 0.95f, 1.0f, 1.0f);
  //CONNECT_COLOR4(Scene4Color, 1.0f, 0.07f, 0.15f, 1.0f);
  
  CONNECT_SLIDER0(SPS, -1.0f, 10.0f, 3.0f);
  CONNECT_SLIDER1(Dist, -1.0f, 10.0f, 1.0f);

  CONNECT_SLIDER2(refmul, -1.0f, 10.0f, 1.0f); 
  CONNECT_SLIDER3(refoff, -1.0f, 10.0f, 0.0f);
  
  CONNECT_SLIDER4(WallColIntens, -1.0f, 10.0f, 0.85f);


  // Variables used to identify the objects. In this case, there are just two - the biotubes and
  // the tunnel walls.
  float objID = 0.0f; // Biotubes: 0, Tunnel walls: 1.
  float saveID = 0.0f;

float zzzzzzzzzzzzzzzzzz;

  // Screen coordinates.
  float2 uv = (fragCoord - iResolution*0.5f)/iResolution.y;
  
  
  float fAngle = (iMouse.x / iResolution.x - 0.0f) * radians(360.0f);
  float fElevation = (iMouse.y / iResolution.y - 0.5f) * radians(90.0f);
  
  float fDist = Dist;//1.0f; 

  float3 cam_mPos = to_float3(_sinf(fAngle) * fDist * _cosf(fElevation), _sinf(fElevation) * fDist, _cosf(fAngle) * fDist * _cosf(fElevation));
  
  
  // Camera Setup.
  float3 lookAt = to_float3(0, 1, iTime*2.0f + 0.1f) ;  // "Look At" position.
  float3 camPos = lookAt + cam_mPos;// + to_float3(0.0f, 0.0f, -0.1f);           // Camera position, doubling as the ray origin.

 
  // Light positioning. 
  float3 lightPos = camPos + to_float3(0, 0.5f, 5);// Put it a bit in front of the camera.

  // Using the Z-value to perturb the XY-plane.
  // Sending the camera, "look at," and light vector down the tunnel. The "path" function is 
  // synchronized with the distance function.
  swi2S(lookAt,x,y, swi2(lookAt,x,y) + path(lookAt.z));
  swi2S(camPos,x,y, swi2(camPos,x,y) + path(camPos.z));
  swi2S(lightPos,x,y, swi2(lightPos,x,y) + path(lightPos.z));

  // Using the above to produce the unit ray-direction vector.
  float FOV = 3.14159265f/2.0f; // FOV - Field of view.
  float3 forward = normalize(lookAt-camPos);
  float3 right = normalize(to_float3(forward.z, 0.0f, -forward.x )); 
  float3 up = cross(forward, right);

  // rd - Unit ray direction.
  float3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    
  // Lens distortion, if preferable.
  //vec3 rd = (forward + FOV*uv.x*right + FOV*uv.y*up);
  //rd = normalize(to_float3(swi2(rd,x,y), rd.z - dot(swi2(rd,x,y), swi2(rd,x,y))*0.25f));    
  
  // Swiveling the camera about the XY-plane (from left to right) when turning corners.
  // Naturally, it's synchronized with the path in some kind of way.
  swi2S(rd,x,y, mul_mat2_f2(rot2( path(lookAt.z).x/16.0f ) , swi2(rd,x,y)));
    
  // Standard ray marching routine. I find that some system setups don't like anything other than
  // a "break" statement (by itself) to exit. 
  float t = trace(camPos, rd, iTime, &objID);
    
  // Save the object ID just after the "trace" function, since other map calls can change it, which
  // will distort the results.
  saveID = objID; 

  // Initialize the scene color.
  float3 sceneCol = to_float3_s(0);
  
  // The ray has effectively hit the surface, so light it up.
  if(t<FAR){
      // Surface position and surface normal.
      float3 sp = t * rd+camPos;
      float3 sn = getNormal(sp, iTime, &objID);
        
      // Function based bump mapping. Comment it out to see the under layer. It's pretty
      // comparable to regular beveled Voronoi... Close enough, anyway.
      if(saveID>0.5f) sn = doBumpMap(sp, sn, 0.2f, saveID);
      else            sn = doBumpMap(sp, sn, 0.008f, saveID);
      
      // Ambient occlusion.
      float ao = calculateAO(sp, sn, iTime, &objID);
      
      // Light direction vectors.
      float3 ld = lightPos-sp;

      // Distance from respective lights to the surface point.
      float distlpsp = _fmaxf(length(ld), 0.001f);
      
      // Normalize the light direction vectors.
      ld /= distlpsp;
      
      // Light attenuation, based on the distances above.
      float atten = 1.0f/(1.0f + distlpsp*0.25f); // + distlpsp*distlpsp*0.025
      
      // Ambient light.
      float ambience = 0.5f;
      
      // Diffuse lighting.
      float diff = _fmaxf( dot(sn, ld), 0.0f);
     
      // Specular lighting.
      float spec = _powf(_fmaxf( dot( reflect(-ld, sn), -rd ), 0.0f ), 32.0f);

      
      // Fresnel term. Good for giving a surface a bit of a reflective glow.
      float fre = _powf( clamp(dot(sn, rd) + 1.0f, 0.0f, 1.0f), 1.0f);
      
      // Object texturing and coloring. 
      float3 texCol;        
      
      if(saveID>0.5f){ // Tunnel walls.
        // Two second texture algorithm. Terrible, but it's dark, so no one will notice. :)
        texCol = swi3(WallColor,x,y,z)*(noise3D(sp*32.0f)*0.66f + noise3D(sp*64.0f)*0.34f)*(1.0f-cellTile(sp*16.0f)*0.75f);
        // Darkening the crevices with the bump function. Cheap, but effective.
        texCol *= smoothstep(-0.1f, 0.5f, cellTile(sp*0.75f)*0.66f+cellTile(sp*1.5f)*0.34f)*WallColIntens+0.15f;//0.85f+0.15f; 
      }
      else { // The biotubes.
        // Cheap, sinewy, vein-like covering. Smoothstepping Voronoi is the main mechanism involved.
        float3 sps = sp/SPS;//3.0f;
        float ct = cellTile(sps*2.0f + sin_f3(sps*12.0f)*0.5f)*0.66f + cellTile(sps*6.0f + sin_f3(sps*36.0f)*0.5f)*0.34f;
        
        if(Textur)
          texCol = swi3(_tex2DVecN( iChannel0, sn.x,sn.y,15),x,y,z);//*(1.0f-smoothstep(-0.1f, 0.25f, ct)) + to_float3(0.1f, 0.01f, 0.004f);
        else
          texCol = swi3(TubeColor,x,y,z)*(1.0f-smoothstep(-0.1f, 0.25f, ct)) + to_float3(0.1f, 0.01f, 0.004f);
      }
              
      /////////   
      // Translucency, courtesy of XT95. See the "thickness" function.
      float3 hf =  normalize(ld + sn);
      float th = thickness( sp, sn, 1.0f, 1.0f, iTime, &objID );
      float tdiff =  _powf( clamp( dot(rd, -hf), 0.0f, 1.0f), 1.0f);
      float trans = (tdiff + 0.0f)*th;  
      trans = _powf(trans, 4.0f);        
      ////////        

      
      // Darkening the crevices. Otherwise known as cheap, scientifically-incorrect shadowing.  
      float shading = 1.0f;//crv*0.5f+0.5f; 
        
      
      // Shadows - Better, but they really drain the GPU, so I ramped up the fake shadowing so 
      // that it's not as noticeable.
      //shading *= softShadow(sp, ld, 0.05f, distlpsp, 8.0f);
    
      // Combining the above terms to produce the final color. It was based more on acheiving a
      // certain aesthetic than science.
#ifdef ORG
      sceneCol = texCol*(diff + ambience) + to_float3(0.7f, 0.9f, 1.0f)*spec;// + to_float3(0.5f, 0.8f, 1)*spec2;
      if(saveID<0.5f) sceneCol += to_float3(0.7f, 0.9f, 1.0f)*spec*spec;
      sceneCol += texCol*to_float3(0.8f, 0.95f, 1)*_powf(fre, 4.0f)*2.0f;
      sceneCol += to_float3(1, 0.07f, 0.15f)*trans*1.5f;
#endif      
      sceneCol = texCol*(diff + ambience) + to_float3(0.7f, 0.9f, 1.0f)*spec;// + to_float3(0.5f, 0.8f, 1)*spec2;
      if(saveID<0.5f) sceneCol += to_float3(0.7f, 0.9f, 1.0f)*spec*spec;
      sceneCol += texCol*to_float3(0.8f, 0.95f, 1)*_powf(fre, 4.0f)*2.0f;
      sceneCol += swi3(SceneColor,x,y,z)*trans*1.5f;
      
      // Fake reflection and refraction on the biotubes. Not a proper reflective and 
      // refractive pass, but it does a reasonable job, and is much cheaper.
      float3 ref, em;
      
      if(saveID<0.5f){ // Biotubes.
          
        // Fake reflection and refraction to give a bit of a fluid look, albeit
        // in a less than physically correct fashion.
        ref = reflect(rd, sn);
        em = eMap(ref, sn, iTime);
        sceneCol += em*0.5f;
        ref = _refract_f3(rd, sn, 1.0f/1.3f,refmul,refoff);//svn*0.5f + n*.5
        em = eMap(ref, sn, iTime);
        sceneCol += em*to_float3(2, 0.2f, 0.3f)*1.5f;
      }

      // Shading.
      sceneCol *= atten*shading*ao;
  }
       
  // Blend the scene and the background; It's commented out, but you could also integrate some some 
  // very basic, 8-layered smokey haze.
  //float mist = getMist(camPos, rd, lightPos, t);
  float3 sky = swi3(SkyColor,x,y,z);//to_float3(2.0f, 0.9f, 0.8f);//* _mix(1.0f, 0.75f, mist);//*(rd.y*0.25f + 1.0f);
  sceneCol = _mix(sky, sceneCol, 1.0f/(t*t/FAR/FAR*8.0f + 1.0f));

  // Clamp and present the pixel to the screen.
  fragColor = to_float4_aw(sqrt_f3(clamp(sceneCol, 0.0f, 1.0f)), 1.0f);
  
  SetFragmentShaderComputedColor(fragColor);
}