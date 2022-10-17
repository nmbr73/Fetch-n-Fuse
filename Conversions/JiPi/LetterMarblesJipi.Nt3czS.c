
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// 0: Cube, 1: Sphere
#define BOUNDTYPE 0 

#ifdef XXX
__DEVICE__ int sphereAmount = 26; // Amount of balls
__DEVICE__ float sphereRadius = 0.5f; // Radius of balls
__DEVICE__ float3 bounds = {2.1f,2.1f,1.5f}; //to_float3(2.1f,2.1f,1.5f); // Bounding rectangle
__DEVICE__ float boundRadius = 2.6f;
__DEVICE__ float blur = 0.8f; // 0: no blur, 1: all previous frames
__DEVICE__ float dofAmount = 0.03f; // How "big" the square "lens" is for depth of field
__DEVICE__ float ballIOR = 0.93f; //  (inverse?) index of refraction of air to balls; Not optimized for ior > 1
__DEVICE__ float rotationSpeed = 0.15f;
#endif

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void LetterMarblesJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);

    CONNECT_INTSLIDER0(sphereAmount, 0, 50, 26);      // Amount of balls
    CONNECT_SLIDER2(sphereRadius, -1.0f, 1.0f, 0.5f); // Radius of balls
    CONNECT_COLOR0(bounds, 2.1f, 2.1f, 1.5f, 1.0f);   // Bounding rectangle
        
    CONNECT_SLIDER3(boundRadius, -1.0f, 10.0f, 2.6f);
    CONNECT_SLIDER4(blur, -1.0f, 2.0f, 0.8f);            // 0: no blur, 1: all previous frames
    CONNECT_SLIDER5(dofAmount, -1.0f, 1.0f, 0.03f);      // How "big" the square "lens" is for depth of field
    CONNECT_SLIDER6(ballIOR, -1.0f, 1.0f, 0.5f);         // (inverse?) index of refraction of air to balls; Not optimized for ior > 1
    CONNECT_SLIDER7(rotationSpeed, -1.0f, 1.0f, 0.15f);    
      
  
    fragCoord+=0.5f;

    // Store all Spheres
    //int ri = to_int2_cfloat(fragCoord).x;
    int ri = (int)fragCoord.x;
    //float4 rp = texelFetch(iChannel0,to_int2(ri,0), 0);
    //float4 rv = texelFetch(iChannel0,to_int2(ri,1), 0);
    float4 rp = texture(iChannel0,(make_float2(to_int2(ri,0))+0.5f)/iResolution);
    float4 rv = texture(iChannel0,(make_float2(to_int2(ri,1))+0.5f)/iResolution);
    
    bool vel = (int)(fragCoord.y) == 1;
    
    float4 r = vel ? rv : rp;
    
    if(ri < sphereAmount) {
        for(int i = 0; i < sphereAmount; i++) {
            if(i != ri) { // Collision
                //float4 s = texelFetch(iChannel0,to_int2(i,0), 0);
                float4 s = texture(iChannel0,(make_float2(to_int2(i,0))+0.5f)/iResolution);
                float3 delt = swi3((rp - s),x,y,z);
                float dis = length(delt) - sphereRadius * 2.0f;
                if(dis < 0.0f){
                    if(vel){
                        dis = _fminf(dis,-0.01f);
                        r.x -= normalize(delt).x * dis * 0.05f;
                        r.y -= normalize(delt).y * dis * 0.05f;
                        r.z -= normalize(delt).z * dis * 0.05f;
                        r.x *= 0.9f;
                        r.y *= 0.9f;
                        r.z *= 0.9f;
                    }
                }
            }
        }
        // Velocity Update
        if(vel){
            r.z += -0.001f;
            r.x *= 0.9995f;
            r.y *= 0.9995f;
            r.z *= 0.9995f;
        }else{
            r.x += rv.x;
            r.y += rv.y;
            r.z += rv.z;
        }
        
        // Bounds
        float bounce = 0.1f;
        // Cube Bounds
        if(BOUNDTYPE == 0){
        if(rp.x - sphereRadius < -bounds.x){
            r.x = vel ? _fabs(rv.x)*bounce : - bounds.x + sphereRadius;
        }
        if(rp.x + sphereRadius > bounds.x){
            r.x = vel ? - _fabs(rv.x)*bounce : bounds.x - sphereRadius;
        }
        if(rp.y - sphereRadius < -bounds.y){
            r.y = vel ? _fabs(rv.y)*bounce : - bounds.y + sphereRadius;
        }
        if(rp.y + sphereRadius > bounds.y){
            r.y = vel ? - _fabs(rv.y)*bounce : bounds.y - sphereRadius;
        }
        if(rp.z - sphereRadius < -bounds.z){
            r.z = vel ? _fabs(rv.z)*bounce : - bounds.z + sphereRadius;
        }}
        // Sphere Bounds
        if(BOUNDTYPE == 1){
          if(length(swi3(rp,x,y,z)) > boundRadius - sphereRadius){
            if(vel){
                float len = length(swi3(rp,x,y,z))-(boundRadius - sphereRadius);
                swi3S(r,x,y,z, swi3(r,x,y,z) - 0.9f * normalize(swi3(rp,x,y,z)) * len);
            }else{
                swi3S(r,x,y,z, (normalize(swi3(rp,x,y,z)) * (boundRadius - sphereRadius)));

            }
          }
        }
                
        fragColor = r;
    }
    // Take
    if(iFrame % 1500 == 1500*ri/sphereAmount){
        if(BOUNDTYPE == 0){
            fragColor = to_float4(0,0,5.0f,ri);
        }
        if(BOUNDTYPE == 1){
            fragColor = to_float4(0,0,boundRadius - sphereRadius,ri);
        
        }
        if(vel){
            fragColor = to_float4(0.0f,0.0f,-0.15f,fragCoord.x);
        }
    }
    // Initials
    if(iFrame <= 2 || Reset){
        fragColor = to_float4(0.9f*mod_f((float)(ri), 5.0f)-2.5f,0.9f*mod_f((float)(ri)/5.0f,6.0f)-2.5f, 0.9f*mod_f((float)(ri)/12.0f,6.0f)-1.0f,(float)(ri));
        if(vel){
            fragColor = to_float4(0.0f,0.0f,0.0f,ri);
        }
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Cubemap: St Peters Basilica Blurred_0' to iChannel2
// Connect Buffer B 'Texture: Font 1' to iChannel0
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel3


#define DOF 1

#define rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))

__DEVICE__ float rand(float3 po){ // Lazy random function, but only used once/twice per pixel
    return (_sinf((_cosf(po.x*290.65f+po.y*25.6f+po.z*2.97f)*5632.75f+849.2f*_cosf(po.y*534.24f+po.x)+2424.64f*_cosf(po.z*473.76f))));
}

// Font SDF code: https://www.shadertoy.com/view/llcXRl
__DEVICE__ float2 matmin(float2 a, float2 b)
{
    if (a.x < b.x) return a;
    else return b;
}
__DEVICE__ float2 matmax(float2 a, float2 b)
{
    if (a.x > b.x) return a;
    else return b;
}


__DEVICE__ float4 SampleFontTex(float2 uv, int letter, __TEXTURE2D__ iChannel0){
    float2 fl = _floor(uv + 0.5f);
    uv = fl + fract_f2(uv + 0.5f)-0.5f + to_float2((float)(letter%16), (float)(letter/16));
    // Sample the font texture. Make sure to not use mipmaps.
    // Add a small amount to the distance field to prevent a strange bug on some gpus. Slightly mysterious. :(
    return texture(iChannel0, (uv+0.5f)*(1.0f/16.0f)) + to_float4(0.0f, 0.0f, 0.0f, 0.000000001f);
}

#ifdef XXX
__DEVICE__ float4 SampleFontTex(float2 p, int c, __TEXTURE2D__ iChannel0) {
    float2 uv = (p + to_float2((float)(c%16), (float)(15-c/16)) + 0.5f)/16.0f;
    //return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel0, uv) - 127.0f/255.0f);
    return texture(iChannel0, uv) - 127.0f/255.0f;
}
#endif

__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); }

__DEVICE__ float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); }

__DEVICE__ float opSmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) + k*h*(1.0f-h); }

__DEVICE__ float sdBox(float3 p, float3 radius){
  float3 dist = abs_f3(p) - radius;
  return _fminf(_fmaxf(dist.x, _fmaxf(dist.y, dist.z)), 0.0f) + length(_fmaxf(dist, to_float3_s(0.0f)));
}
__DEVICE__ float sdSphere( float3 p, float s )
{
  return length(p)-s;
}
__DEVICE__ float2 DistanceToObject(float3 p, int letter, __TEXTURE2D__ iChannel0)
{
  // Load the font texture's distance field.
    float letterDistField = (SampleFontTex(swi2(p,x,y),letter+64, iChannel0).w - 0.5f+1.0f/256.0f);
    // intersect it with a box.
    float cropBox = sdBox(p, to_float3(0.3f, 0.3f, 0.05f));
    float2 letters = matmax(to_float2(letterDistField, 0.0f), to_float2(cropBox, 1.0f));
    return letters;
}

__DEVICE__ float3 findNormal(float3 p, float d, int letter, __TEXTURE2D__ iChannel0){
    return normalize(to_float3(
        DistanceToObject(p + to_float3(d,0,d),letter,iChannel0).x - DistanceToObject(p - to_float3(d,0,0),letter,iChannel0).x,
        DistanceToObject(p + to_float3(0,d,0),letter,iChannel0).x - DistanceToObject(p - to_float3(0,d,0),letter,iChannel0).x,
        DistanceToObject(p + to_float3(0,0,d),letter,iChannel0).x - DistanceToObject(p - to_float3(0,0,d),letter,iChannel0).x
        ));
}

__DEVICE__ void checkDist(inout float *d, float c, inout int *obj, int obi){
    if(c < *d){
        *d = c;
        *obj = obi;
    }
}
__DEVICE__ float sdScene(float3 p, inout int *obj, float2 iResolution, __TEXTURE2D__ iChannel1, float sphereRadius, int sphereAmount, float3 bounds, float boundRadius){
    float dist = 10000.0f;
    *obj = -1;
    for(int i = 0; i < sphereAmount; i++){
        //float4 s = texelFetch(iChannel1,to_int2(i,0), 0);
        float4 s = texture(iChannel1,(make_float2(to_int2(i,0))+0.5f)/iResolution);
        checkDist(&dist, sdSphere(p - to_float3(s.x,s.y,s.z),sphereRadius), obj, i+1);
    }
    float douter = 0.0f;
    
    if(BOUNDTYPE == 0){ // Outer Box
     douter = opSmoothSubtraction(sdBox(p+to_float3(0.0f,0.0f,0), bounds-0.2f) - 0.2f,
                                  sdBox(p+to_float3(0.0f,0.0f,2.9f), bounds+0.5f)-0.2f,0.2f);               
    }
    if(BOUNDTYPE == 1){ // Outer Bowl
     douter = opSmoothSubtraction(sdSphere(p+to_float3(0.0f,0.0f,0), boundRadius-0.15f) - 0.2f,
                                  sdSphere(p+to_float3(0.0f,0.0f,0.4f), boundRadius-0.2f)-0.2f,0.2f);               
    }

    checkDist(&dist, douter, obj, 0);
    return dist;
}
__DEVICE__ float sdScene(float3 p, float2 iResolution, __TEXTURE2D__ iChannel1, float sphereRadius, int sphereAmount, float3 bounds, float boundRadius){
    int obj = 0;
    return sdScene(p, &obj, iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius);
}

__DEVICE__ float3 findNormal(float3 p, float d, float2 iResolution, __TEXTURE2D__ iChannel1, float sphereRadius, int sphereAmount, float3 bounds, float boundRadius){
    return normalize(to_float3(
        sdScene(p + to_float3(d,0,d), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius) - sdScene(p - to_float3(d,0,0), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius),
        sdScene(p + to_float3(0,d,0), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius) - sdScene(p - to_float3(0,d,0), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius),
        sdScene(p + to_float3(0,0,d), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius) - sdScene(p - to_float3(0,0,d), iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius)));
}

// For the transmission rays
__DEVICE__ float3 snellLaw(float3 s, float3 N, float ior){
    //return ior * cross(N,cross(-N, s)) - N * _sqrtf(1.0f- ior * ior * dot(cross(N,s),cross(N,s)));
    float c = dot(-N,s);
    return ior * s + (ior * c - _sqrtf(1.0f-ior*ior*(1.0f-c*c))) * N;

}
// For the Specular Rays
__DEVICE__ float3 refl(float3 d, float3 n){

    return d - 2.f * n * dot(d,n);
}

__DEVICE__ float4 raycastBall(inout float3 *pos, inout float3 *dir, int object, __TEXTURE2D__ iChannel0, float sphereRadius){
    int inrays = 30;
    float4 col = to_float4(0,0,0,0);
    for(int i = 0; i < inrays; i++){
        float d = sdBox(*pos, to_float3(1,1,1) * sphereRadius/3.0f);
        d = DistanceToObject(*pos,object,iChannel0).x;
        d = _fminf(d, -length(*pos)+sphereRadius+0.02f); // distance to outer edge
        if(d < 0.01f && length(*pos) < sphereRadius-0.05f) {
            i = inrays;
            col = clamp(to_float4(0.5f+_sinf((float)(object) + 2.1f), 0.5f+_sinf((float)(object)+4.2f), 0.5f+_sinf((float)(object)), 1),0.0f,1.0f); // color of letters  
        }else if(length(*pos) > sphereRadius + 0.01f){ // Reaches outside of sphere
            i = inrays;
        }else {
            *pos += *dir * d;
        }
    }
    return col;
}


__DEVICE__ float4 raycast(float3 pos, float3 dir, int maxmarch, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, float ballIOR, float sphereRadius, int sphereAmount, float3 bounds, float boundRadius){ // Casts a ray from position in direction, returns color of object. TODO: out direction, materials.
    float4 col = to_float4(0,0,0,0);
    float lastd=0.0f;
    float totalDist = 0.0f;
    int ob = -1;
    int obhit = 6;
    float ior = ballIOR;
    float4 totalcol = to_float4(0,0,0,0);
    float totaldim = 1.0f;
    for(int i = 0; i < maxmarch; i++){
         float d = sdScene(pos + dir * lastd, &ob, iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius);
         
         if(d < 0.01f && ob >= 1 && obhit > 1){ // Hits a sphere within 0.05
         
             // Get hitsphere from buffer
             //float4 s = texelFetch(iChannel1,to_int2(ob-1,0), 0);
             float4 s = texture(iChannel1,(make_float2(to_int2(ob-1,0))+0.5f)/iResolution);
             float3 ballRot = to_float3(1.0f*s.z,1.0f*s.y,0);
             pos += dir * lastd*1.0f;
             float3 norm = normalize(pos - swi3(s,x,y,z));
             float3 ref = refl(dir, norm);
             // Add partial fresnel reflections
             float fres = _fabs(dot(dir,norm));
             //totalcol +=  texture(iChannel2, swi3(ref,x,z,y), -100.0f) * totaldim * (1.0f-fres);
             totalcol +=  decube_f3(iChannel2, swi3(ref,x,z,y)) * totaldim * (1.0f-fres);
             totaldim *= (fres);
           
             // Updates position, direction when entering sphere 
             float3 newpo = -1.0f*swi3(s,x,y,z) + pos;
             float3 newdir = normalize(snellLaw(dir,norm,ior));  // Refraction
             float4 col2 = to_float4(1,1,0,1);

             // Global to Local Transformations 
             swi2S(newpo,x,z, mul_f2_mat2(swi2(newpo,x,z) , rot(ballRot.y)));
             swi2S(newdir,x,z, mul_f2_mat2(swi2(newdir,x,z) , rot(ballRot.y)));
             swi2S(newpo,y,z, mul_f2_mat2(swi2(newpo,y,z) , rot(ballRot.x)));
             swi2S(newdir,y,z, mul_f2_mat2(swi2(newdir,y,z) , rot(ballRot.x)));
             
             col2 = raycastBall(&newpo, &newdir, ob, iChannel0, sphereRadius); // Casts into sphere
               
             if(col2.w > 0.5f){ // hit letter
                 float3 norma = findNormal(newpo,0.03f,ob, iChannel0);
                 swi2S(norma,y,z, mul_f2_mat2(swi2(norma,y,z) , rot(-ballRot.x)));
                 swi2S(norma,x,z, mul_f2_mat2(swi2(norma,x,z) , rot(-ballRot.y)));
                 float3 reflcol = swi3(decube_f3(iChannel2, swi3(norma,x,z,y)),x,y,z) * totaldim;
                 totalcol += to_float4_aw(reflcol * swi3(col2,x,y,z),0);
                 col = col2;
                 i = maxmarch;
float xxxxxxxxxxxxx;                 
             }else{ // go through sphere
             
                 // Local to Global Transformations 
                 swi2S(newpo,y,z, mul_f2_mat2(swi2(newpo,y,z) , rot(-ballRot.x)));
                 swi2S(newdir,y,z, mul_f2_mat2(swi2(newdir,y,z) , rot(-ballRot.x)));
                 swi2S(newpo,x,z, mul_f2_mat2(swi2(newpo,x,z) , rot(-ballRot.y)));
                 swi2S(newdir,x,z, mul_f2_mat2(swi2(newdir,x,z) , rot(-ballRot.y)));
                 dir = newdir;
                 pos = swi3(s,x,y,z) + newpo;
                 float3 newnorm = normalize(pos - swi3(s,x,y,z));
                 
                 dir = normalize(snellLaw(dir,-newnorm,1.0f/ior)); // Out refraction
                 lastd = 0.0f;
                 obhit -= 1;
             }
         
         } else if(d < 0.005f) { // Hits bowl
              i = maxmarch;
              float3 norm = findNormal(pos,0.0001f, iResolution, iChannel1, sphereRadius, sphereAmount, bounds, boundRadius);
                
                
              norm += 0.5f*rand(_floor(pos*10.0f));
              float s = 1.0f;
              
              float3 refla = refl(dir,norm);
              //col = to_float4(texture(iChannel2, refla,-100.0f)) * 0.2f;
              col = (decube_f3(iChannel2, refla)) * 0.2f;
              
              totalcol += col * totaldim;

        } else if(d>200.0f) {
              i = maxmarch;
              
              totalcol +=  decube_f3(iChannel2, swi3(dir,x,z,y)) * totaldim  * totaldim;
         } else {
              pos += dir * lastd; // march ray
              totalDist += lastd;
              lastd = d;

         }
    }
    return totalcol;
}
__KERNEL__ void LetterMarblesJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_INTSLIDER0(sphereAmount, 0, 50, 26);      // Amount of balls
    CONNECT_SLIDER2(sphereRadius, -1.0f, 1.0f, 0.5f); // Radius of balls
    CONNECT_COLOR0(bounds, 2.1f, 2.1f, 1.5f, 1.0f);   // Bounding rectangle
        
    CONNECT_SLIDER3(boundRadius, -1.0f, 10.0f, 2.6f);
    CONNECT_SLIDER4(blur, -1.0f, 2.0f, 0.8f);            // 0: no blur, 1: all previous frames
    CONNECT_SLIDER5(dofAmount, -1.0f, 1.0f, 0.03f);      // How "big" the square "lens" is for depth of field
    CONNECT_SLIDER6(ballIOR, -1.0f, 1.0f, 0.5f);         // (inverse?) index of refraction of air to balls; Not optimized for ior > 1
    CONNECT_SLIDER7(rotationSpeed, -1.0f, 1.0f, 0.15f); 
    

    fragCoord+=0.5f;    

    float4 outray = to_float4(1,1,1,1);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.x;
    
    float dofrang = 2.0f*4.5f;
    float2 jitter = to_float2(0,0);
    if(DOF==1){jitter = dofAmount*to_float2(rand(to_float3_aw(fragCoord,iTime)),rand(to_float3_aw(fragCoord,iTime+1.0f)));}
    
    
    float3 pos = to_float3(0.0f,-4.5f,1.5f) + to_float3(jitter.x,0,jitter.y);
    float3 dir = normalize(to_float3(uv.x-jitter.x/dofrang,0.5f,uv.y-jitter.y/dofrang));
    
    if(iMouse.z > 0.0f){
        pos -= to_float3(0.0f,0.0f,1.5f);

        swi2S(dir,z,y, mul_f2_mat2(swi2(dir,z,y) , rot(-2.0f*iMouse.y/iResolution.y+1.5f)));
        swi2S(pos,z,y, mul_f2_mat2(swi2(pos,z,y) , rot(-2.0f*iMouse.y/iResolution.y+1.5f)));

        swi2S(dir,x,y, mul_f2_mat2(swi2(dir,x,y) , rot(-3.14f*iMouse.x/iResolution.x)));
        swi2S(pos,x,y, mul_f2_mat2(swi2(pos,x,y) , rot(-3.14f*iMouse.x/iResolution.x)));
    }else{
        swi2S(dir,z,y, mul_f2_mat2(swi2(dir,z,y) , rot(0.4f)));
        swi2S(dir,x,y, mul_f2_mat2(swi2(dir,x,y) , rot(iTime*rotationSpeed)));
        swi2S(pos,x,y, mul_f2_mat2(swi2(pos,x,y) , rot(iTime*rotationSpeed)));
    }
    
    
    //float4 precolor = texelFetch(iChannel3,to_int2(fragCoord),0);
    float4 precolor = texture(iChannel3,(make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);

    outray = raycast(pos,dir,250, iResolution, iChannel0, iChannel1, iChannel2, ballIOR, sphereRadius, sphereAmount, swi3(bounds,x,y,z), boundRadius);
    
    #ifdef xxxx
    if(iFrame < 10 || Reset){
      fragColor = to_float4_aw(swi3(outray,x,y,z),1);
    }else{
      fragColor = precolor * blur + to_float4_aw(swi3(outray,x,y,z),1) * (1.0f-blur);
    }
    #endif
    
    if(iFrame > 10){
      fragColor = precolor * blur + to_float4_aw(swi3(outray,x,y,z),1) * (1.0f-blur);
    }else{
      fragColor = to_float4_aw(swi3(outray,x,y,z),1);
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel3


__KERNEL__ void LetterMarblesJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel3)
{

CONNECT_SLIDER0(Alpha, -1.0f, 1.0f, 1.0f);
CONNECT_SLIDER1(Brightness, -1.0f, 1.0f, 0.4545f);

  //fragColor = pow_f4(texelFetch(iChannel3,to_int2(fragCoord),0), to_float4(1.0f/2.2f) );
  //fragColor = pow_f4(texture(iChannel3,(make_float2(to_int2_cfloat(fragCoord))+0.5f/iResolution)), to_float4_s(1.0f/2.2f) );

  fragColor = pow_f4(texture(iChannel3,fragCoord/iResolution), to_float4_s(Brightness));
  fragColor.w = Alpha;

  SetFragmentShaderComputedColor(fragColor);
}