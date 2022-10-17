
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 2' to iChannel0
// Connect Image 'Texture: Pebbles' to iChannel2


#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by Sebastien DURAND - 2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//----------------------------------------------------------------
// Thanks to
// Iq: Deformed tubes, distance field, shadows, etc. 
// Shane: Texture 3D, render
//----------------------------------------------------------------

// The far plane. I'd like this to be larger, but the extra iterations required to render the 
// additional scenery starts to slow things down on my slower machine.
#define FAR 80.0f
#define PI 3.14159265f
#define ZERO 0 //_fminf(0,iFrame)

#define slab   0.05f
#define ani0    2.0f  // Start Grow
#define ani1    6.0f  
#define ani2   10.0f  // start graine
#define ani3   20.0f  // start move cam up
#define ani4   30.0f  // start move to frogs
#define ani5   46.0f  // start turn arround frogs
#define ani5b  64.0f // To center of frogs
#define ani5b2 83.0f // Eye bottom
#define ani5c  86.0f // Move hand 
#define ani5d 101.0f // Enter the ground
#define ani6  122.0f // Under the ground
#define ani7  129.0f



//float dhaloLight, dhaloFrog;
//float sanim01, sanim12, sanim23, sanim34, sanim45, sanim56, sanim5cd, sanim56r, sanim67;
//float gPulse, gPulseGround;

// --------------------------------------------------------------

__DEVICE__ float2 rotate( float2 v, float a ) { return to_float2( v.x*_cosf(a)+v.y*_sinf(a), -v.x*_sinf(a)+v.y*_cosf(a) ); }
__DEVICE__ float2 sincos( float x ) { return to_float2( _sinf(x), _cosf(x) ); }
__DEVICE__ float3 opU( float3 d1, float3 d2 ){ return (d1.x<d2.x) ? d1 : d2;}


// --------------------------------------------------------------
// hash functions
// --------------------------------------------------------------
__DEVICE__ float hash( float2 p ) { return fract(_sinf(1.0f+dot(p,to_float2(127.1f,311.7f)))*43758.545f); }
__DEVICE__ float hash( float n ){ return fract(_cosf(n)*45758.5453f); }
__DEVICE__ float hash( float3 p ) { return fract(_sinf(dot(p, to_float3(7, 157, 113)))*45758.5453f); }
__DEVICE__ float3 hash3( float2 p ) {
    float3 q = to_float3( dot(p,to_float2(127.1f,311.7f)), 
           dot(p,to_float2(269.5f,183.3f)), 
           dot(p,to_float2(419.2f,371.9f)) );
  return fract_f3(sin_f3(q)*43758.5453f);
}
// --------------------------------------------------------------



// Grey scale.
__DEVICE__ float getGrey(float3 p){ return dot(p, to_float3(0.299f, 0.587f, 0.114f)); }


// IQ's smooth minium function. 
__DEVICE__ float sminP(float a, float b , float s){
    float h = clamp(0.5f + 0.5f*(b-a)/s, 0.0f , 1.0f);
    return _mix(b, a, h) - h*(1.0f-h)*s;
}

// Smooth maximum, based on the function above.
__DEVICE__ float smaxP(float a, float b, float s){
    float h = clamp( 0.5f + 0.5f*(a-b)/s, 0.0f, 1.0f);
    return _mix(b, a, h) + h*(1.0f-h)*s;
}


// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){
    p.x*=1.777f;
    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= (n.x + n.y + n.z );  
  return swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
  //return swi3(texture(tex, swi2(p,z,x)),x,y,z);
}


//--------------------------------------------------
// From Mercury
// Repeat around the origin by a fixed angle.
// For easier use, num of repetitions is use to specify the angle.
__DEVICE__ float pModPolar(inout float2 *p, float rep) {
  float angle = 2.0f*PI/rep,
         a = _atan2f((*p).y, (*p).x) + angle*0.5f,
         r = length(*p),
         c = _floor(a/angle);
  a = mod_f(a, angle) - angle*0.5f;
  *p = to_float2(_cosf(a), _sinf(a))*r;
  // For an odd number of repetitions, fix cell index of the cell in -x direction
  // (cell index would be e.g. -5 and 5 in the two halves of the cell):
  if (_fabs(c) >= rep*0.5f) c = _fabs(c);
  return c;
}
//-----------------------------------------------------


__DEVICE__ float sdSegment( float3 p, float3 a, float3 b, float r) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );  
    return length(pa - ba*h) - r;
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r) {
    float k0 = length(p/r), k1 = length(p/(r*r));
    return k0*(k0-1.0f)/k1;
}

// capsule with bump in the middle -> use for arms and legs
__DEVICE__ float sdBumpCapsule( float3 p, float3 a, float3 b, float r, float k) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f ),
       dd = k*_cosf(3.141592f*h+1.57f);  // Little adaptation
    return length(pa - ba*h) - r+dd; 
}

// ------------------------------------------------------------------

__DEVICE__ float sdFrog(float3 p, float sanim5cd,float iTime) {

    const mat2 rot = to_mat2(_cosf(-0.3f),_sinf(-0.3f),-_sinf(-0.3f),_cosf(-0.3f));

    float2 pxz = swi2(p,x,z);
    float id = pModPolar(&pxz, 12.0f);
    p.x=pxz.x;p.z=pxz.y;
    
    p.x -= 12.0f;
    p.y += 0.1f;
    
    float scale = 0.4f + 0.2f*hash(id);
    p /= scale;
    swi2S(p,x,z, swi2(p,x,z) + 2.0f*fract(11.0f*scale));
    
    float dFrog = length(p-to_float3(0.31f,1.6f,0));
    if (dFrog> 3.3f+1.0f/scale) return dFrog;
    
    float kRot = sanim5cd*0.2f*_cosf(id + 5.0f*iTime); 
    swi2S(p,x,z, rotate(swi2(p,x,z), 0.5f*kRot));
    float3 pr = p;
    swi2S(pr,x,y, mul_f2_mat2(swi2(pr,x,y) , rot));
    
    float sgn = sign_f(p.z);
    p.z = _fabs(p.z);
    pr.z = _fabs(pr.z);
    
    float dEye = length(pr-to_float3(-2.0f,2.0f,0.7f)) - 0.7f;
  
    float dLeg = sminP(sdBumpCapsule(p,to_float3(2.1f,0.7f,0.3f),to_float3(0.0f,2.5f, 2.7f),0.2f,0.3f),
                 sminP(sdBumpCapsule(p,to_float3(0.0f,2.4f,2.8f),to_float3(1.8f,0.6f,1.3f),0.2f,0.2f),
                           sdSegment(p,to_float3(1.8f,0.6f,1.3f),to_float3(1.2f,0.3f,1.6f),0.2f),0.05f) ,0.05f);
    float dFeet =  _fminf(sdSegment(p,to_float3(1.1f,0.25f,1.55f),to_float3(0.2f,0.3f,1.5f),0.08f),
                   _fminf(sdSegment(p,to_float3(1.2f,0.25f,1.6f),to_float3(-0.0f,0.3f,2.0f),0.08f),
                          sdSegment(p,to_float3(1.1f,0.25f,1.75f),to_float3(0.3f,0.3f,2.3f),0.08f)));
    float dFinger = _fminf(min(length(p-to_float3(0.3f,0.3f,1.5f)),
                               length(p-to_float3(0.1f,0.3f,2.0f))),
                               length(p-to_float3(0.4f,0.3f,2.3f))) - 0.12f;
  
    float3 pFinger = p;
    pFinger.z += 4.0f*sgn*kRot;
    
    float dLeg2 = sminP(sdBumpCapsule(p,to_float3(-1.0f,1.6f,1.6f),to_float3(-0.2f,1.2f, 2.2f-sgn*kRot),0.2f,0.1f),
                        sdBumpCapsule(p,to_float3(-0.2f,1.2f,2.3f-sgn*kRot),to_float3(-1.0f,0.3f,2.0f-4.0f*sgn*kRot),0.2f,0.1f),0.1f);
    
    float dFeet2 =  _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-1.8f,0.3f,1.5f),0.1f),
                    _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-2.0f,0.3f,2.0f),0.1f),
                    _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-1.3f,0.3f,1.1f),0.1f),
                           sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-0.85f,0.3f,1.2f),0.1f)
                          )));
    float dFinger2 = _fminf(min(length(pFinger-to_float3(-1.8f,0.3f,1.5f)),
                                length(pFinger-to_float3(-2.0f,0.3f,2.0f))),
                     _fminf(length(pFinger-to_float3(-1.3f,0.3f,1.1f)),
                            length(pFinger-to_float3(-0.85f,0.3f,1.2f))) ) - 0.12f;
    
    dFeet2 = sminP(dFinger2, dFeet2,0.1f); 
    dFeet = sminP(dFinger, dFeet,0.1f); 
    dLeg = sminP(dLeg, dFeet,0.2f);
    dLeg2 = sminP(dLeg2, dFeet2,0.2f);
    
    float dd = _fmaxf(max(0.5f-p.y, dot(pr-to_float3(3.0f,1.3f,0), normalize(to_float3(1,2,1)))),
               _fmaxf(dot(pr-to_float3(-3.4f,1.8f,-0.1f), normalize(to_float3(-1.9f,1.0f,2))),
               _fminf(0.8f-pr.y, dot(pr-to_float3(-0.5f,0.3f,2.0f), normalize(to_float3(-1.5f,-2.2f,1))))));
  
    float dBody = sdEllipsoid(pr-to_float3(-0.5f,1.2f,0), to_float3(2.7f,1.3f,2));
    dBody = smaxP(dd, dBody, 0.1f);
    dBody = sminP(dBody, dEye, 0.2f);
    
    float d = dBody;
    
    d = smaxP(dd, d, 0.1f);
    d = sminP(d, dEye, 0.2f);
    d = smaxP(d, -_fminf(dLeg,dLeg2), 0.3f);
    d = sminP(d, dLeg, 0.2f);
    d = sminP(d, dLeg2, 0.15f);
    d = smaxP(d, -(length(p - to_float3(-1.5f,2.4f,0.8f)) - 0.5f), 0.4f);
    d = smaxP(d, -(length(pr - to_float3(-3.05f,1.55f,0.18f))), 0.1f);
    
    float kFrog = 0.55f*smoothstep(0.9f,1.0f,_cosf(iTime+102.0f*id));
    d = sminP(d, sdEllipsoid(pr-to_float3(-2.0f,0.65f-0.2f*kFrog,0.0f), _mix(to_float3(0.5f,0.15f,0.9f), to_float3(1.0f,1.0f,1.8f), kFrog)), 0.2f);
    dEye = length(p-to_float3(-1.5f,2.4f,0.8f)) - 0.5f;
    
    return scale*_fminf(d,dEye);
}


__DEVICE__ float mapTube( float3 p, float sanim01, float iTime ) {
    float2 id = _floor( (swi2(p,x,z)+5.0f)/10.0f );
    
    float k = hash(id.x+101.0f*id.y);
    if (k>0.3f || k<0.1f) return 999.0f;
    
    float tt = mod_f(iTime,1.5f)/1.5f;
    float ss = _powf(tt,0.2f)*0.5f + 0.5f;
    float pulseY = sanim01*(0.25f+ss*0.5f*_sinf(tt*6.2831f*3.0f+p.y)*_expf(-tt*4.0f));
    ss = pulseY*0.25f;

    swi2S(p,x,z, mod_f(swi2(p,x,z)+5.0f, 10.0f) - 5.0f);
    swi2S(p,x,z, swi2(p,x,z) + 0.5f*sin_f2( 2.0f + p.y*to_float2(0.53f,0.32f) - to_float2(1.57f,0.0f) ));

    return _fminf( _fminf(length(swi2(p,x,z)+0.15f*sincos(p.y)), 
                          length(swi2(p,x,z)+0.15f*sincos(p.y+4.0f))) - 0.15f*(0.8f+0.2f*_sinf(2.0f*p.y)) - ss, 
                   _fminf(length(swi2(p,x,z)+0.15f*sincos(p.y+2.0f)) - 0.15f*(0.8f+0.2f*_sinf(2.0f*p.y + 2.0f*(p.y-iTime)))-ss-0.01f, 
                          length(swi2(p,x,z)+0.15f*sincos(p.y+5.0f)) - 0.08f*(0.8f+0.2f*_sinf(2.0f*p.y + ss + 8.0f*(p.y-iTime)))-0.02f-0.3f*ss));
}


__DEVICE__ float map( float3 pos, bool light,float gPulse, float gPulseGround, float *dhaloLight, float *dhaloFrog,float sanim5cd, float sanim01, float sanim56, float sanim23, float iTime, __TEXTURE2D__ iChannel2) {
    float3 p0 = pos;

    float h = _mix(1.0f,0.1f, smoothstep(20.0f, 14.0f, length(swi2(pos,x,z)))) + gPulseGround;
    
        float3 _p0 = to_float3(p0.x+10.5f, p0.y,p0.z);
    
    h *= texture(iChannel2, -1.0f*swi2(_p0,x,z)*0.02f).x;
    pos.y -= h;
    
    float kEat = sanim56*(1.0f+0.1f*gPulse); 
    float3 pFrog = p0-to_float3(0, - 1.5f*kEat,0);
    
    float dFrog = iTime < ani4 ? 999.0f : sdFrog(pFrog,sanim5cd, iTime);

    float dTube = mapTube(pos, sanim01, iTime);

    float2 id = _floor( (swi2(pos,x,z)-1.0f)/2.0f);
    
    swi2S(pos,x,z, mod_f(swi2(pos,x,z)+1.0f, 2.0f) - 1.0f);
    swi2S(pos,x,z, swi2(pos,x,z) + 0.2f*_sinf(dot(120.0f*id,to_float2(1213.15f,1317.34f))));
    swi2S(pos,x,z, swi2(pos,x,z) + to_float2(1,-1)*0.2f*_sinf(3.5f*iTime +3.0f*_cosf(pos.y))*_sinf(0.5f*pos.y));

    float3 posButton = pos;
    
    float len = _fmaxf(sanim01,0.1f)*(0.5f+0.4f*smoothstep(0.4f,0.5f,_cosf(0.3f*iTime+id.x))+0.3f*_sinf(dot(110.0f*id,to_float2(1213.15f,1317.34f))));
    float thi = sanim01*(0.8f+0.4f*_cosf(0.4f*iTime)) * slab * (0.5f+0.3f*_sinf(-3.151592f*posButton.y/len));
   
    float d = 999.0f;
     
    if (hash(id.x+11.0f*id.y) > 0.6f) {
        d = sdSegment( posButton, to_float3(0.0f,-len*0.25f,0.0f), to_float3(0,len,0), 4.0f*thi);
        float dlight = length(pos-to_float3(0,fract(1.0f+_cosf(id.x+3.1f*id.y)+iTime*0.1f)*15.0f,0))-0.05f*sanim23;
        if (light) *dhaloLight = _fminf(*dhaloLight, pos.y > len ? dlight-0.02f*sanim23 : 9999.0f);
        if (sanim23 > 0.0f) {
            d = sminP(d, dlight, 0.3f);
        }
    }
    if (light && sanim56 > 0.0f) {
        *dhaloFrog = _fminf(*dhaloFrog, dFrog -0.02f*sanim56); 
    }

    // Bump arround frog ---------------------------------

    float2 pFrogxz = swi2(pFrog,x,z);
    float idFrog = pModPolar(&pFrogxz, 12.0f);
    pFrog.x=pFrogxz.x;pFrog.z=pFrogxz.y;
    pFrog.x -= 12.0f;
    float scale = 0.4f + 0.2f*hash(idFrog);
    pFrog /= scale;
    swi2S(pFrog,x,z, swi2(pFrog,x,z) + 2.0f*fract(11.0f*scale));
 
    float dBumpFrog = scale*(length(pFrog-to_float3(-1.0f,1.7f-2.1f*kEat*kEat,0))-4.0f*kEat);
    // ---------------------------------------------------

    d = sminP(d, pos.y, 0.3f);
    d = sminP(d, dBumpFrog, 0.6f);
    d = smaxP(d,-dFrog, 0.3f);

    
//    return _fminf(dFrog, _fminf(dTube, smaxP(-_fminf(dTube-0.3f,length(swi2(p0,x,z))-4.2f), d, 1.0f)));
    return _fminf(dFrog, _fminf(dTube, smaxP(-dTube+0.3f, d, 1.0f)));
}


__DEVICE__ float textureFrog(float3 p, out float4 *out_posIdFrog, float iTime, float sanim5cd) {
    const mat2 rot = to_mat2(_cosf(-0.3f),_sinf(-0.3f),-_sinf(-0.3f),_cosf(-0.3f));
    
    float2 pxz = swi2(p,x,z);
    float id = pModPolar(&pxz, 12.0f);
    p.x=pxz.x;p.z=pxz.y;

    p.x -= 12.0f;
    p.y += 0.1f;
    
    float scale = 0.4f + 0.2f*hash(id);
    p /= scale;
    swi2S(p,x,z, swi2(p,x,z) + 2.0f*fract(11.0f*scale));
    
    float dFrog = length(p-to_float3(0.31f,1.6f,0));
    if (dFrog> 3.3f+1.0f/scale) return dFrog;
        
   
    float kRot = sanim5cd*0.2f*_cosf(id + 5.0f*iTime); 
    swi2S(p,x,z, rotate(swi2(p,x,z), 0.5f*kRot));
    float3 p0 = p;
    float3 pr = p;
    swi2S(pr,x,y, mul_f2_mat2(swi2(pr,x,y) , rot));
    
    float sgn = sign_f(p.z);
    p.z = _fabs(p.z);
    pr.z = _fabs(pr.z);
    
    float dEye = length(pr-to_float3(-2.0f,2.0f,0.7f)) - 0.7f;
  
    float dLeg = sminP(sdBumpCapsule(p,to_float3(2.1f,0.7f,0.3f),to_float3(0.0f,2.5f, 2.7f),0.2f,0.3f),
                 sminP(sdBumpCapsule(p,to_float3(0.0f,2.4f,2.8f),to_float3(1.8f,0.6f,1.3f),0.2f,0.2f),
                           sdSegment(p,to_float3(1.8f,0.6f,1.3f),to_float3(1.2f,0.3f,1.6f),0.2f),0.05f) ,0.05f);
    float dFeet =  _fminf(sdSegment(p,to_float3(1.1f,0.25f,1.55f),to_float3(0.2f,0.3f,1.5f),0.08f),
                   _fminf(sdSegment(p,to_float3(1.2f,0.25f,1.6f),to_float3(-0.0f,0.3f,2.0f),0.08f),
                          sdSegment(p,to_float3(1.1f,0.25f,1.75f),to_float3(0.3f,0.3f,2.3f),0.08f)));
    float dFinger = _fminf(min(length(p-to_float3(0.3f,0.3f,1.5f)),
                               length(p-to_float3(0.1f,0.3f,2.0f))),
                               length(p-to_float3(0.4f,0.3f,2.3f))) - 0.12f;
  
    float3 pFinger = p;
    pFinger.z += 4.0f*sgn*kRot;
    float dLeg2 = sminP(sdBumpCapsule(p,to_float3(-1.0f,1.6f,1.6f),to_float3(-0.2f,1.2f, 2.2f-sgn*kRot),0.2f,0.1f),
                        sdBumpCapsule(p,to_float3(-0.2f,1.2f,2.3f-sgn*kRot),to_float3(-1.0f,0.3f,2.0f-4.0f*sgn*kRot),0.2f,0.1f),0.1f);
    
    float dFeet2 =  _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-1.8f,0.3f,1.5f),0.1f),
                    _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-2.0f,0.3f,2.0f),0.1f),
                    _fminf(sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-1.3f,0.3f,1.1f),0.1f),
                           sdSegment(pFinger,to_float3(-1.0f,0.2f,2.0f),to_float3(-0.85f,0.3f,1.2f),0.1f)
                              )));
    float dFinger2 = _fminf(min(length(pFinger-to_float3(-1.8f,0.3f,1.5f)),
                                length(pFinger-to_float3(-2.0f,0.3f,2.0f))),
                         _fminf(length(pFinger-to_float3(-1.3f,0.3f,1.1f)),
                                length(pFinger-to_float3(-0.85f,0.3f,1.2f))) ) - 0.12f;
    
    dFeet2 = sminP(dFinger2, dFeet2,0.1f); 
    dFeet = sminP(dFinger, dFeet,0.1f); 
    dLeg = sminP(dLeg, dFeet,0.2f);
    dLeg2 = sminP(dLeg2, dFeet2,0.2f);
    
    float dd = _fmaxf(max(0.5f-p.y, dot(pr-to_float3(3.0f,1.3f,0), normalize(to_float3(1,2,1)))),
               _fmaxf(dot(pr-to_float3(-3.4f,1.8f,-0.1f), normalize(to_float3(-1.9f,1.0f,2))),
               _fminf(0.8f-pr.y, dot(pr-to_float3(-0.5f,0.3f,2.0f), normalize(to_float3(-1.5f,-2.2f,1))))));
  
    float dBody = sdEllipsoid(pr-to_float3(-0.5f,1.2f,0), to_float3(2.7f,1.3f,2));
    dBody = smaxP(dd, dBody, 0.1f);
    dBody = sminP(dBody, dEye, 0.2f);

    float d = dBody;
    
    d = smaxP(dd, d, 0.1f);
    d = sminP(d, dEye, 0.2f);
    d = smaxP(d, -_fminf(dLeg,dLeg2), 0.3f);
    d = sminP(d, dLeg, 0.2f);
    d = sminP(d, dLeg2, 0.15f);
    d = smaxP(d, -(length(p - to_float3(-1.5f,2.4f,0.8f)) - 0.5f), 0.4f);
    d = smaxP(d, -(length(pr - to_float3(-3.05f,1.55f,0.18f))), 0.1f);
    
    float kFrog = 0.55f*smoothstep(0.9f,1.0f,_cosf(iTime+102.0f*id));
    d = sminP(d, sdEllipsoid(pr-to_float3(-2.0f,0.65f-0.2f*kFrog,0.0f), _mix(to_float3(0.5f,0.15f,0.9f), to_float3(1.0f,1.0f,1.8f), kFrog)), 0.2f);
    float3 pEye = p-to_float3(-1.5f,2.4f,0.8f);
    dEye = length(pEye) - 0.5f;
    
    *out_posIdFrog = d<dEye ? to_float4_aw(p0, 20.0f+_fabs(id)) : to_float4_aw(pEye, 31.0f+_fabs(id));
    
    return scale*_fminf(d,dEye);
}


__DEVICE__ float texturePtTube(float3 p, out float4 *out_idPosTube, float iTime, float sanim01) {

    float2 id = _floor( (swi2(p,x,z)+5.0f)/10.0f );

    float k = hash(id.x+101.0f*id.y);
    if (k>0.3f || k<0.1f) return 999.0f;
    
    float tt = mod_f(iTime,1.5f)/1.5f;
    float ss = _powf(tt,0.2f)*0.5f + 0.5f;
    float pulseY = sanim01*(0.25f+ss*0.5f*_sinf(tt*6.2831f*3.0f+p.y)*_expf(-tt*4.0f));
    ss = pulseY*0.25f;

    swi2S(p,x,z, mod_f2( swi2(p,x,z)+5.0f, 10.0f ) - 5.0f);
    swi2S(p,x,z, swi2(p,x,z) + 0.5f*sin_f2( 2.0f + (p.y)*to_float2(0.53f,0.32f) - to_float2(1.57f,0.0f) ));



    float3 p1 = p; 
    swi2S(p1,x,z, swi2(p1,x,z) + 0.15f*sincos(p.y));
    float3 p2 = p; 
    swi2S(p2,x,z, swi2(p2,x,z) + 0.15f*sincos(p.y+2.0f));
    float3 p3 = p; 
    swi2S(p3,x,z, swi2(p3,x,z) + 0.15f*sincos(p.y+4.0f));
    float3 p4 = p; 
    swi2S(p4,x,z, swi2(p4,x,z) + 0.15f*sincos(p.y+5.0f));   
    
    float h1 = length(swi2(p1,x,z)),
          h2 = length(swi2(p2,x,z)),
          h3 = length(swi2(p3,x,z)),
          h4 = length(swi2(p4,x,z));

    float3 res = opU( opU(to_float3(h1-0.15f*(0.8f+0.2f*_sinf(2.0f*p.y))-ss, 10.0f, p.y), 
                          to_float3(h2-0.15f*(0.8f+0.2f*_sinf(2.0f*p.y+2.0f*(p2.y-iTime)))-ss-0.01f, 11.0f, p.y)), 
                      opU(to_float3(h3-0.15f*(0.8f+0.2f*_sinf(2.0f*p.y))-ss, 12.0f, p.y),
                          to_float3(h4-0.08f*(0.8f+0.2f*_sinf(2.0f*p.y+ss+8.0f*(p.y-iTime)))-0.02f-0.3f*ss, 13.0f, p.y) ));

    *out_idPosTube = to_float4_aw(res.y == 10.0f ? p1 : res.y == 11.0f ? p2 : res.y == 12.0f ? p3 : p4, res.y);
    return res.x;
}


__DEVICE__ float4 texturePt(float3 pos, float iTime, __TEXTURE2D__ iChannel2, float gPulse, float gPulseGround, float sanim5cd, float sanim56, float sanim01) {
    float3 p0 = pos;

    float h = _mix(1.0f,0.1f, smoothstep(20.0f, 14.0f, length(swi2(pos,x,z)))) + gPulseGround;
    

    
    h *= texture(iChannel2, -1.0f*swi2(p0,x,z)*0.02f).x;
    pos.y -= h;
    
   
    float kEat = sanim56*(1.0f+0.1f*gPulse); 
    float3 pFrog = p0-to_float3(0,-1.5f*kEat,0);
    float4 idPosFrog;
    float dFrog = textureFrog(pFrog, &idPosFrog,iTime,sanim5cd);

    float4 idPosTube;
    float dTube = texturePtTube(pos, &idPosTube,iTime, sanim01);
         
    float2 id = _floor( (swi2(pos,x,z)-1.0f)/2.0f);    
    swi2S(pos,x,z, swi2(pos,x,z) - 0.2f*_sinf(dot(120.0f*id,to_float2(1213.15f,1317.34f))));
    swi2S(pos,x,z, swi2(pos,x,z) + to_float2(1,-1)*0.2f*_sinf(3.5f*iTime +3.0f*_cosf(pos.y))*_sinf(0.5f*pos.y));

    return dTube < 0.01f ? idPosTube : dFrog < 0.01f ? idPosFrog : to_float4_aw(pos,1.0f);
}


#define EDGE_WIDTH 0.001f

__DEVICE__ float3 trace(in float3 ro, in float3 rd, in float maxd, float gPulse, float gPulseGround, float *dhaloLight, float *dhaloFrog,float sanim5cd, float sanim01, float sanim56, float sanim23, float iTime, __TEXTURE2D__ iChannel2) {
  // edge detection
    *dhaloLight = 9999.0f; // reset closest trap
    *dhaloFrog = 9999.0f;
    float lastt,lastDistEval = 1e10;
    float edge = 0.0f;
    float iter = 0.0f;

    float t = hash(rd);
    float d = 999.0f;//map(rd*t + ro);
    for (int i=ZERO; i<240; i++){
    d = 0.7f*map(rd*t + ro, true,gPulse,gPulseGround, dhaloLight, dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2);
        if ( _fabs(d) < 0.002f || t > maxd) break;
        t += _fminf(0.9f,d);
    }
    return to_float3_s(t);
}


// Tetrahedral normal, courtesy of IQ.
// -- Calculate normals -------------------------------------

__DEVICE__ float3 calcNormal(in float3 pos, in float3 ray, in float t, float2 iResolution, float gPulse, float gPulseGround, float *dhaloLight, float *dhaloFrog,float sanim5cd, float sanim01, float sanim56, float sanim23, float iTime, __TEXTURE2D__ iChannel2) {

  float pitch = 0.2f * t / iResolution.x;
  pitch = _fmaxf( pitch, 0.002f );
  
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x), p1 = pos+swi3(d,x,y,y), p2 = pos+swi3(d,y,x,y), p3 = pos+swi3(d,y,y,x);
  float f0 = map(p0,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2),
        f1 = map(p1,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2), 
        f2 = map(p2,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2), 
        f3 = map(p3,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2);
  
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f, dot (grad,ray))*ray);
}




// The iterations should be higher for proper accuracy, but in this case, I wanted less accuracy, just to leave
// behind some subtle trails of light in the caves. They're fake, but they look a little like light streaming 
// through some cracks... kind of.
__DEVICE__ float softShadow(in float3 ro, in float3 rd, in float start, in float end, in float k, float gPulse, float gPulseGround, float *dhaloLight, float *dhaloFrog,float sanim5cd, float sanim01, float sanim56, float sanim23, float iTime, __TEXTURE2D__ iChannel2){

    float shade = 1.0f;
    const int maxIterationsShad = 32; 
    float dist = start;
    float stepDist = end/(float)(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i=ZERO; i<maxIterationsShad; i++){
        float h = map(ro + rd*dist,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2);
        shade = _fminf(shade, smoothstep(0.0f, 1.0f, k*h/dist));
        dist += clamp(h, 0.1f, stepDist*2.0f);
        if (_fabs(h)<0.001f || dist > end) break; 
    }
    return _fminf(max(shade, 0.0f) + 0.1f, 1.0f); 
}



// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function and have been looking for an excuse to use it. For a better version, and usage, 
// refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
__DEVICE__ float calculateAO(in float3 pos, in float3 nor, float gPulse, float gPulseGround, float *dhaloLight, float *dhaloFrog,float sanim5cd, float sanim01, float sanim56, float sanim23, float iTime, __TEXTURE2D__ iChannel2) {
    float dd, hr=0.01f, totao=0.0f, sca=1.0f;
    for(int aoi=ZERO; aoi<4; aoi++ ) {
        dd = map(nor * hr + pos,false,gPulse,gPulseGround,dhaloLight,dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2);
        totao += -(dd-hr)*sca;
        sca *= 0.8f;
        hr += 0.06f;
    }
    return clamp(1.0f-4.0f*totao, 0.0f, 1.0f);
}



// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 nor, float bumpfactor){
   
    const float eps = 0.001f;
    
    float3 grad = to_float3( getGrey(tex3D(tex, to_float3(p.x-eps, p.y, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y-eps, p.z), nor)),
                             getGrey(tex3D(tex, to_float3(p.x, p.y, p.z-eps), nor)));
    
    grad = (grad - getGrey(tex3D(tex,  p , nor)))/eps;             
    grad -= nor*dot(nor, grad);          
    return normalize( nor + grad*bumpfactor );
  
}

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d ) {
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float3 palette(float id, float k) {
    return 2.0f*pal( k, to_float3(0.5f,0.8f,0.8f),to_float3(0.6f,0.3f,0.5f),to_float3(1.0f,0.2f,1.0f), to_float3_s((id-10.0f)*0.01f) );
}


// HSV to RGB conversion 
// [iq: https://www.shadertoy.com/view/MsS3Wc]
__DEVICE__ float3 hsv2rgb_smooth(float _x, float _y, float _z) {
    float3 rgb = clamp( abs_f3(mod_f3(_x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f);
    rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  
    return _z * _mix( to_float3_s(1), rgb, _y);
}


// -------------------------------------------------------------------
// pupils effect came from lexicobol shader:
// https://www.shadertoy.com/view/XsjXz1
// -------------------------------------------------------------------



__DEVICE__ float iqnoise( in float2 _x, float u, float v )
{
    float2 p = _floor(_x);
    float2 f = fract(_x);
    float k = 1.0f+63.0f*_powf(1.0f-v,4.0f);
    float va = 0.0f;
    float wt = 0.0f;
    for( int j=-2+ZERO; j<=2; j++ )
    for( int i=-2+ZERO; i<=2; i++ ) {
        float2 g = to_float2(i,j);
        float3 o = hash3( p + g )*to_float3(u,u,1.0f);
        float2 r = g - f + swi2(o,x,y);
        float d = dot(r,r);
        float ww = _powf( 1.0f-smoothstep(0.0f,1.414f,_sqrtf(d)), k );
        va += o.z*ww;
        wt += ww;
    }
  
    return va/wt;
}

__DEVICE__ float noise ( float2 _x)
{
  return iqnoise(_x, 0.0f, 1.0f);
}



__DEVICE__ float fbm( float2 p)
{
  mat2 m = to_mat2( 0.8f, 0.6f, -0.6f, 0.8f);
  float f = 0.0f;
    f += 0.5000f * noise(p); p = mul_f2_mat2(p,m)* 2.02f;
    f += 0.2500f * noise(p); p = mul_f2_mat2(p,m)* 2.03f;
    f += 0.1250f * noise(p); p = mul_f2_mat2(p,m)* 2.01f;
    f += 0.0625f * noise(p); p = mul_f2_mat2(p,m)* 2.04f;
    f /= 0.9375f;
    return f;
}

__DEVICE__ float3 iris(float2 p, float open)
{

    float r = _sqrtf( dot (p,p));
    float r_pupil = 0.15f + 0.15f*smoothstep(0.5f,2.0f,open);
    
    float dPupil = length(to_float2(_fabs(p.x)+0.2f, p.y)) - 0.35f;// + 0.15f*smoothstep(0.5f,2.0f,open);

    float a = _atan2f(p.y, p.x); // + 0.01f*iTime;
    float3 col = to_float3_s(1.0f);
    
    float ss = 0.5f;// + 0.5f * _sinf(iTime * 2.0f);
    float anim = 1.0f + 0.05f*ss* clamp(1.0f-r, 0.0f, 1.0f);
    r *= anim;
        
    if( r< 0.8f) {
        col = to_float3(0.12f, 0.60f, 0.57f);
        float f = fbm(5.0f * p);
        col = _mix(col, 2.0f*to_float3(1.0f,0.8f,0.12f), f); 
        
        f = 1.0f - smoothstep( 0.0f, 0.1f, dPupil);
        col = _mix(col, to_float3(0.12f,1.0f, 0.30f), f); 
        
        a += 0.05f * fbm(20.0f*p);
        
        f = smoothstep(0.3f, 1.0f, fbm(to_float2(5.0f * r, 20.0f * a))); // white highlight
        col = _mix(col, to_float3_s(1.0f), f);
        
        f = smoothstep(0.3f, 1.0f, fbm(to_float2(5.0f * r, 5.0f * a))); // yellow highlight
        col = _mix(col, to_float3(1.5f,0.8f,0.12f), f);
        
        f = smoothstep(0.5f, 1.0f, fbm(to_float2(5.0f * r, 15.0f * a))); // dark highlight
        col *= 1.0f - f;
        
        f = smoothstep(0.55f, 0.8f, r); //dark at edge
        col *= 1.0f - 0.6f*f;
        
        f = smoothstep( 0.0f, 0.05f, dPupil); //pupil
        col *= f; 
        
        f = smoothstep(0.75f, 0.8f, r);
        col = 0.5f*_mix(col, to_float3_s(1.0f), f);
    }

  return 3.0f*col;
}



#ifdef STEREOGRAPHIC
__DEVICE__ float3 getStereoDir(float2 fragCoord, float2 iResolution)
{

    float2 p = fragCoord / iResolution;
    float t = 3.0f+iTime*0.08f, ct = _cosf(t), st = _sinf(t);
    float m = 0.5f;
    p = (p * 2.0f * m - m)*0.7f;
    p.x *= iResolution.x/iResolution.y;
    p = mul_f2_mat2(p,to_mat2(ct,st,-st,ct));

  return normalize(to_float3(2.0f*p.x,dot(p,p)-1.0f,2.0f*p.y));
}  
#endif

__DEVICE__ static float aktTime;

__KERNEL__ void TheMeaningOfLifeJipi914Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Stop, 0);
    CONNECT_COLOR0(ColorGround, 0.5f, 0.5f, 0.5f, 1.0f);

    

    if(Stop == 0) aktTime = iTime;    

    float dhaloLight, dhaloFrog;
    
    float tt = mod_f(iTime,1.5f)/1.5f;
    float ss = _powf(tt,0.2f)*0.5f + 0.5f;

    float sanim01 = smoothstep(ani0,ani1,iTime);
    float sanim12 = smoothstep(ani1,ani2,iTime);
    float sanim23 = smoothstep(ani2,ani3,iTime);
    float sanim34 = smoothstep(ani3,ani4,iTime);
    float sanim45 = smoothstep(ani4,ani5,iTime);
    float sanim56r = smoothstep(ani5b,ani5c,iTime); 
    float sanim5c = smoothstep(ani5,ani5c,iTime); 
    float sanim5cd = smoothstep(ani5c,ani5d,iTime); 
    float sanim56 = smoothstep(ani5d,ani6,iTime); 
    float sanim67 = smoothstep(ani6,ani7,iTime);
    
    // Heart pulse
    float gPulse = (0.25f+ss*0.5f*_sinf(tt*6.2831f*3.0f)*_expf(-tt*4.0f));

    // Ground Pulse
    float gPulseGround = 0.2f+0.8f*_mix(gPulse, 0.0f, sanim45 + sanim67);
    
    // Screen coordinates.
    float2 u = (fragCoord - iResolution*0.5f)/iResolution.y;
      
    float2 uv = swi2(fragCoord,x,y) / iResolution;

    // Camera Setup.
    //float a = 0.1f*iTime + 2.0f*3.141592f*iMouse.x/iResolution.x;
    float a = 0.1f*aktTime + 2.0f*3.141592f*iMouse.x/iResolution.x;
    
    float3 ro = to_float3(-92.0f,  4.5f, -78.0f);
    ro = _mix(ro, to_float3(-106.0f,  8.0f, -84.0f), sanim34); 
    ro = _mix(ro, to_float3(_cosf(-a), 0.3f, _sinf(-a))*26.0f, sanim45);
    ro = _mix(ro, to_float3(1,3,0), sanim5c);
    ro = _mix(ro, to_float3(1,3,0) + to_float3(1.0f,0.2f,0.8f)*(iTime - ani6), sanim67);
    
    float3 lookAt = ro + to_float3(0.25f, -0.22f, 0.5f); // Camera position, doubling as the ray origin.
    lookAt = _mix(lookAt, to_float3(0, 0, 0), sanim34); // Camera position, doubling as the ray origin.
    lookAt = _mix(lookAt, to_float3(_cosf(a), 0.25f, _sinf(a))*7.0f, sanim5c); // Camera position, doubling as the ray origin.
   
    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159f/6.0f; // FOV - Field of view.
    float3 forward = normalize(lookAt-ro);
    float3 right = normalize(to_float3(forward.z, 0.0f, -forward.x )); 
    float3 up = cross(forward, right);

    // rd - Ray direction.
    float3 rd = normalize(forward + FOV*u.x*right + FOV*u.y*up);


    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
    //swi2(rd,x,z) = rot2( /*iMouse.x/iResolution.x +*/ path(lookAt.z).x/64.0f )*swi2(rd,x,z);

    // Usually, you'd just make this a unit directional light, and be done with it, but I
    // like some of the angular subtleties of point lights, so this is a point light a
    // long distance away. Fake, and probably not advisable, but no one will notice.

    float3 res = trace(ro, rd, FAR,gPulse,gPulseGround, &dhaloLight, &dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2);
    float t = res.x;
    
    // Standard sky routine. Worth learning. For outdoor scenes, you render the sky, then the
    // terrain, then mix together with a fog falloff. Pretty straight forward.
    float3 sky = 0.3f*to_float3(1.0f,1.3f,1.3f);//getSky(ro, rd, normalize(lp - ro));
    float3 col = sky;
  
 //   float3 lp = _mix(ro+to_float3(5.05f,-1.5f,-5.05f), ro+to_float3(0.05f,12.5f,-5.05f), sanim56r);
 //   lp = _mix(lp, ro+to_float3(5.05f,-1.5f,-5.05f), sanim67);
    
     float3 lp = (forward*0.5f+up-right)*FAR/*to_float3(FAR*0.5f, FAR, FAR)*/ + to_float3(0, 0, ro.z);

    if (t < FAR){
      
        float3 sp = ro+t*rd; // Surface point.
        float4 spt = texturePt(sp,iTime,iChannel2,gPulse,gPulseGround, sanim5cd, sanim56, sanim01); // Surface points on objects coords (to enable textures to follow object moves)
        float3 sn = calcNormal( sp, rd, t,iResolution,gPulse,gPulseGround, &dhaloLight, &dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2 ); // Surface normal.

        // Light direction
        float3 ld = normalize(lp-sp);

        // Texture scale factor.        
        const float tSize1 = 1.0f/3.0f;
        float k;
        float3 colTxt;
        
        if (spt.w > 30.0f) {
            // Frog eyes
            float3 pe = swi3(spt,x,y,z);
            float a = 0.2f*_cosf(0.1f*iTime),
                  ca = _cosf(a), sa = _sinf(a);
            swi2S(pe,x,z, mul_f2_mat2(swi2(pe,x,z) , to_mat2(ca, sa, -sa, ca)));
            float b = _mix(3.1f-1.5f*fract(iTime*0.2f+0.17f*spt.w), 4.2f, step(ani5b2, iTime)),//sanim56r),
                 cb = _cosf(b), sb = _sinf(b);
            swi2S(pe,x,y, mul_f2_mat2(swi2(pe,x,y) , to_mat2(cb, sb, -sb, cb)));
            colTxt = iris((swi2(pe,z,y)), 20.5f);

        } else if (spt.w > 19.0f) {
            // Frog Body
            float3 hh = hash3(to_float2(spt.w,spt.w));
            colTxt = hsv2rgb_smooth(spt.w*0.2f, 0.6f, 0.7f);

            colTxt = mix_f3(colTxt, to_float3_s(0.0f), 0.7f*smoothstep(to_float3_s(0.6f),to_float3_s(0.7f), hh*0.5f+tex3D(iChannel2, swi3(spt,x,y,z)*tSize1, sn).x));
            colTxt = _mix(colTxt, to_float3(0,1,1), 0.1f*smoothstep(0.0f,1.0f, -sn.y));
            colTxt = 0.7f*sqrt_f3(colTxt);
            sn = doBumpMap(iChannel2, 2.0f*swi3(spt,x,y,z)*tSize1, sn, 0.2f/(1.0f + t/FAR));
            
        } else if (spt.w > 5.0f) { //Stränge

            k = tex3D(iChannel0, swi3(spt,x,y,z)*tSize1 + 0.1f*spt.w, sn).x;
            colTxt = _mix(to_float3(1.2f,0.5f,0.4f), palette(1.0f, spt.w), 0.7f+0.3f*_cosf(spt.w+4.0f*spt.y-5.0f*iTime));     
            colTxt = _mix(colTxt, to_float3(1,0,0), 0.2f+0.5f*smoothstep(0.2f,0.8f,k));

            sn = doBumpMap(iChannel0, swi3(spt,x,y,z)*tSize1, sn, 0.007f/(1.0f + t/FAR));

        }  else { // Boden
            k = tex3D(iChannel0, swi3(spt,x,y,z)*tSize1, sn).x; //0.0f;
//             colTxt = _mix(to_float3(1,0.5f,0.3f), 4.0f*(0.6f+0.5f*_sinf(0.1f*iTime+0.01f*length(swi2(spt,x,z))))*to_float3(0,1,1), 0.5f+0.5f*smoothstep(0.4f,0.7f,k+0.05f*_cosf(2.0f*iTime)));
            colTxt = _mix(0.3f*to_float3(1,0.5f,0.3f), 1.3f*to_float3(0,1,1), 0.4f+0.6f*smoothstep(0.4f,0.7f,k+0.05f*_cosf(2.0f*iTime))); //Org
            //colTxt = _mix(0.3f*swi3(ColorGround,x,y,z), 1.3f*to_float3(0,1,1), 0.4f+0.6f*smoothstep(0.4f,0.7f,k+0.05f*_cosf(2.0f*iTime)));
            colTxt = _mix(colTxt, 1.7f*to_float3(1.8f,1.8f,0.5f), sanim01*smoothstep(0.4f,0.0f,_fabs(0.8f-spt.y)));

            colTxt += swi3(ColorGround,x,y,z)-0.5;

            sn = doBumpMap(iChannel0, swi3(spt,x,y,z)*tSize1, sn, 0.007f/(1.0f + t/FAR));//_fmaxf(1.0f-length(fwidth(sn)), 0.001f)*hash(sp)/(1.0f+t/FAR)
        }
       
        // prevent normals pointing away from camera (caused by precision errors)
        sn = normalize(sn - _fmaxf(0.0f, dot(sn,rd))*rd);       

        float d2 = 1.0f;//RayMarchOut(sp+rd*(0.05f*4.0f + noise.x*0.05f), ld);
           
        float shd = softShadow(sp, ld, 0.005f, 4.0f, 8.0f,gPulse,gPulseGround, &dhaloLight, &dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2); // Shadows.
        float ao = calculateAO(sp, sn, gPulse,gPulseGround, &dhaloLight, &dhaloFrog,sanim5cd,sanim01,sanim56,sanim23,iTime,iChannel2); // Ambient occlusion.
        float dif = _fmaxf( dot( ld, sn ), 0.0f); // Diffuse term.
        float spe = _powf(_fmaxf( dot( reflect(-ld, sn), -rd ), 0.0f ), 29.0f); // Specular term.
        float fre = clamp(1.0f + dot(rd, sn), 0.0f, 1.0f); // Fresnel reflection term.
       
        // Schlick approximation. I use it to tone down the specular term. It's pretty subtle,
        // so could almost be aproximated by a constant, but I prefer it. Here, it's being
        // used to give a hard clay consistency... It "kind of" works.
        float Schlick = _powf( 1.0f - _fmaxf(dot(rd, normalize(rd + ld)), 0.0f), 5.0f);
        float fre2 = _mix(0.2f, 1.0f, Schlick);  //F0 = 0.2f - Hard clay... or close enough.
       
        // Overal global ambience. Without it, the cave sections would be pretty dark. It's made up,
        // but I figured a little reflectance would be in amongst it... Sounds good, anyway. :)
        float amb = fre*fre2 + 0.06f*ao;
        
        // Coloring the soil - based on depth. Based on a line from Dave Hoskins's "Skin Peeler."
        col = colTxt;
        col = (col*(dif*d2 + 0.1f) + fre2*spe*2.0f)*shd*ao + amb*col;
    } 
    
    col = 0.5f*_mix(col, sky, smoothstep(5.0f, FAR, t));
   
    // Light
    if (dhaloLight < t) {
        float BloomFalloff = 50000.0f; 
     col += _mix(1.5f*to_float3(1.0f,1.0f,0.4f), sky, 0.5f+0.5f*smoothstep(5.0f, FAR, dhaloLight))/(1.0f+dhaloLight*dhaloLight*dhaloLight*BloomFalloff);
    }
    if (dhaloFrog < t) {
        float BloomFalloff = 50000.0f; 
     col += _mix(sanim56*to_float3(1.0f,1.0f,0.4f), to_float3_s(0), 0.5f+0.5f*smoothstep(5.0f, FAR, dhaloFrog))/(1.0f+dhaloFrog*dhaloFrog*dhaloFrog*BloomFalloff);
    }
   
    
    // gamma correction
    col = pow_f3(_fmaxf(col, to_float3_s(0.0f)), to_float3_s(0.7f));

    u = fragCoord/iResolution;
    col *= _powf( 16.0f*u.x*u.y*(1.0f-u.x)*(1.0f-u.y) , 0.32f);

    
    fragColor = to_float4_aw(clamp(col, 0.0f, 1.0f), 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}