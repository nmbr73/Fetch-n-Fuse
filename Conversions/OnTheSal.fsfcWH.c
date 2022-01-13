
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Organic 3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define swi2S(a,b,c,d) {float2 tmp = d; (a).b = tmp.x; (a).c = tmp.y;} 

//-----------------------------------------------------
// Created by sebastien durand - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------
// inspired by nguyen nhut work https://twitter.com/nguyenhut_art
// Teapot alone: https://www.shadertoy.com/view/XsSGzG
//-----------------------------------------------------

#define PI 3.14159265

#define WITH_AO

#define ZERO (_fminf(0, iframe))


#define GROUND 0.0f
#define SKIN 1.0f
#define SHOES1 2.0f
#define SHOES2 2.5f
#define SHORT 3.0f
#define BAG 4.0f
#define SHIRT 5.0f
#define TEAPOT 6.0f
#define BED 7.0f
#define METAL 8.0f
//#define PANEL 9.0f
#define BONE 10.0f


//float gTime;
__DEVICE__ int iframe;


__DEVICE__ float3 shoulder1, elbow1, wrist1, head,
       shoulder2, elbow2, wrist2;
__DEVICE__ float3 foot1, ankle1, knee1, hip1,
       foot2, ankle2, knee2, hip2;

__DEVICE__ mat2 rot, rot1, rot2;


#define U(a,b) ((a).x*(b).y-(b).x*(a).y)



__DEVICE__ float smin(float a, float b, float k){
    float h = clamp(0.5f+0.5f*(b-a)/k, 0.0f, 1.0f);
    return _mix(b,a,h)-k*h*(1.0f-h);
}

__DEVICE__ float2 min2(float2 a, float2 b) {
    return a.x<b.x ? a : b; 
}

#ifdef BONE
// Adated from gaz [Bones] https://www.shadertoy.com/view/ldG3zc
__DEVICE__ float2 sdBone(in float3 p) {
    p.x -= 80.0f;
    const float m = 200.0f;
    float scale = 0.5f + _floor(_fabs(p.x)/m);
    p.x = mod_f(p.x+m*0.5f,m)-m*0.5f;
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot))
    p /= scale;
    p -= to_float3(0.0f,0.05f,2.5f);
    float d = length(p-to_float3(1.2f,0.2f,-0.35f)) - 0.2f;
    p.y -= 0.2f*p.x*p.x;
    p.y *= _cosf(_atan2f(0.6f*p.x,1.0f));
    float n = clamp(p.x,-0.7f,1.0f);
    float2 sg = to_float2(length(swi2(p,x,y)-to_float2(n,0)),(n+0.7f)/1.7f),
    p0 = pow_f2(abs_f2(to_float2(sg.x, p.z)), to_float2_s(3));
    d = smin(d,_powf(p0.x+p0.y, 1.0f/3.0f) -(0.3f*_powf(sg.y-0.5f,2.0f)+0.2f), 0.3f);
    return to_float2(0.7f*scale*d, BONE);
}
#endif

// Distance to Bezier
// inspired by [iq:https://www.shadertoy.com/view/ldj3Wh]
// calculate distance to 2D bezier curve on xy but without forgeting the z component of p
// total distance is corrected using pytagore just before return
__DEVICE__ float2 sdBezier(float2 m, float2 n, float2 o, float3 p) {
  float2 q = swi2(p,x,y);
  m-= q; n-= q; o-= q;
  float x = U(m, o), y = 2.0f * U(n, m), z = 2.0f * U(o, n);
  float2 i = o - m, j = o - n, k = n - m, 
     s = 2.0f * (x * i + y * j + z * k), 
     r = m + (y * z - x * x) * to_float2(s.y, -s.x) / dot(s, s);
  float t = clamp((U(r, i) + 2.0f * U(k, r)) / (x + x + y + z), 0.0f,1.0f); // parametric position on curve
  r = m + t * (k + k + t * (j - k)); // distance on 2D xy space
  return to_float2(_sqrtf(dot(r, r) + p.z * p.z), t); // distance on 3D space
}

//-----------------------------------------------------------------------------------
// iq - https://www.shadertoy.com/view/ldj3Wh
__DEVICE__ float2 sdBezier(in float3 p,in float3 b0,in float3 b1,in float3 b2 ) {
    b0 -= p; b1 -= p; b2 -= p;
    float3 b01 = cross(b0,b1), b12 = cross(b1,b2), b20 = cross(b2,b0),
         n =  b01+b12+b20;
    float a = -dot(b20,n), b = -dot(b01,n), d = -dot(b12,n), m = -dot(n,n);
    float3  g =  (d-b)*b1 + (b+a*0.5f)*b2 + (-d-a*0.5f)*b0;
    float t = clamp((a*0.5f+b-0.5f*(a*a*0.25f-b*d)*dot(g,b0-2.0f*b1+b2)/dot(g,g))/m, 0.0f, 1.0f);
    return to_float2(length(_mix(_mix(b0,b1,t), _mix(b1,b2,t),t)),t);
}

// Distance to scene
__DEVICE__ float sdTeapot(float3 p) {
  
  float2 A[15];
  float2 T1[5];
  float2 T2[5];
  
  // Teapot body profil (8 quadratic curves) 
  A[0]=to_float2(0,0);A[1]=to_float2(0.64f,0);A[2]=to_float2(0.64f,0.03f);A[3]=to_float2(0.8f,0.12f);A[4]=to_float2(0.8f,0.3f);A[5]=to_float2(0.8f,0.48f);A[6]=to_float2(0.64f,0.9f);A[7]=to_float2(0.6f,0.93f);
  A[8]=to_float2(0.56f,0.9f);A[9]=to_float2(0.56f,0.96f);A[10]=to_float2(0.12f,1.02f);A[11]=to_float2(0,1.05f);A[12]=to_float2(0.16f,1.14f);A[13]=to_float2(0.2f,1.2f);A[14]=to_float2(0,1.2f);
  // Teapot spout (2 quadratic curves)
  T1[0]=to_float2(1.16f, 0.96f);T1[1]=to_float2(1.04f, 0.9f);T1[2]=to_float2(1,0.72f);T1[3]=to_float2(0.92f, 0.48f);T1[4]=to_float2(0.72f, 0.42f);
  // Teapot handle (2 quadratic curves)
  T2[0]=to_float2(-0.6f, 0.78f);T2[1]=to_float2(-1.16f, 0.84f);T2[2]=to_float2(-1.16f,0.63f);T2[3]=to_float2(-1.2f, 0.42f);;T2[4]=to_float2(-0.72f, 0.24f);

 
  // Distance to Teapot --------------------------------------------------- 
  // precalcul first part of teapot spout
  float2 h = sdBezier(T1[2],T1[3],T1[4], p);
  float a = 99.0f, 
    // distance to teapot handle (-0.06f => make the thickness) 
    b = _fminf(min(sdBezier(T2[0],T2[1],T2[2], p).x, sdBezier(T2[2],T2[3],T2[4], p).x) - 0.06f, 
    // max p.y-0.9f => cut the end of the spout 
                _fmaxf(p.y - 0.9f,
    // distance to second part of teapot spout (_fabs(dist,r1)-dr) => enable to make the spout hole 
                    _fminf(_fabs(sdBezier(T1[0],T1[1],T1[2], p).x - 0.07f) - 0.01f, 
    // distance to first part of teapot spout (tickness incrase with pos on curve) 
                        h.x * (1.0f - 0.75f * h.y) - 0.08f)));
    // distance to teapot body => use rotation symetry to simplify calculation to a distance to 2D bezier curve
    float3 qq= to_float3(_sqrtf(dot(p,p)-p.y*p.y), p.y, 0);
    // the substraction of 0.015f enable to generate a small thickness arround bezier to help convergance
    // the 0.8f factor help convergance  
  for(int i=ZERO;i<13;i+=2) 
    a = _fminf(a, (sdBezier(A[i], A[i + 1], A[i + 2], qq).x - 0.035f) * 0.9f); 
    // smooth minimum to improve quality at junction of handle and spout to the body
  return smin(a,b,0.02f);
}

// Interpolate pos of articulations
__DEVICE__ float3 getPos(float3 arr[9], int it, float kt, float z, float gTime) {
    it = it%8;
    float3 p = _mix(arr[it], arr[it+1], kt);
    return 0.02f*to_float3(p.x+_floor(gTime/8.0f)*168.0f, 150.0f-p.y, p.z*z);
}

//---------------------------------------------------------------------
//    HASH functions (iq)
//---------------------------------------------------------------------

__DEVICE__ float2 hash22(float2 p) {
    p = fract_f2(p * to_float2(5.3983f, 5.4427f));
    p += dot(swi2(p,y,x), swi2(p,x,y) + to_float2(21.5351f, 14.3137f));
    return fract(to_float2(p.x * p.y * 95.4337f, p.x * p.y * 97.597f));
}

//---------------------------------------------------------------------
//   Modeling Primitives
//   [Inigo Quilez] http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
//---------------------------------------------------------------------

__DEVICE__ float sdCap(float3 p, float3 a, float3 b) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

__DEVICE__ float sdCap2(float3 p, float3 a, float3 b, float r1, float r2) {
    float3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h ) - _mix(r1,r2,h);
}

__DEVICE__ float udRoundBox( float3 p, float3 b, float r ) {
  return length(_fmaxf(abs_f3(p)-b,to_float3_s(0.0f)))-r;
}

__DEVICE__ float sdCappedCylinder(float3 p, float2 h ) {
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - h;
  return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}

// approximated
__DEVICE__ float sdEllipsoid( in float3 p, in float3 r ) {
    float k0 = length(p/r);
    return k0*(k0-1.0f)/length(p/(r*r));
}

__DEVICE__ float sdEar(in float3 p) {
    float3 p_ear = 1.5f*p;
    float d = _fmaxf(-sdEllipsoid(p_ear-to_float3(0.005f,0.015f,0.02f), to_float3(0.07f,0.1f,0.07f)), 
                      sdEllipsoid(p_ear, to_float3(0.08f,0.12f,0.09f)));
    d = _fmaxf(p_ear.z, d); 
    d = smin(d, sdEllipsoid(p_ear+to_float3(0.035f,0.045f,0.01f), to_float3(0.04f,0.04f,0.018f)), 0.01f);
    return d/1.5f;
}


__DEVICE__ mat3 baseArm1, baseArm2, baseBag, baseFoot1, baseFoot2;

__DEVICE__ float2 sdMan(in float3 pos, float gTime){
    float3 p0 = pos;
    float2 res = to_float2(999,0);
    
    // Legs
    float dSkin = _fminf(
        _fminf(sdCap(pos, ankle1, knee1), 
               sdCap(pos, knee1, hip1)),
        _fminf(sdCap(pos, ankle2, knee2),
               sdCap(pos, knee2, hip2)))-0.1f;
            
    // Foot1 flat part - vector base linked to leg 1
    float dShoes1 = _fmaxf(sdCap(pos, foot1, ankle1) - 0.15f, -dot(mul_f3_mat3((pos-ankle1),baseFoot1)-to_float3(0,0,-0.13f), to_float3(0,0,1))); 

    // Leg 2
    float dShoes2 = _fmaxf(sdCap(pos, foot2, ankle2) - 0.15f, -dot(mul_f3_mat3((pos-ankle2),baseFoot2)-to_float3(0,0,-0.13f), to_float3(0,0,1)));  
    
    float3 ep0 = _mix(shoulder1,shoulder2,0.5f),
         ha0 = _mix(hip1,hip2,0.5f);

    // Head
    float3 h1 = to_float3(0,0.17f,0), h2 = to_float3(0.02f,-0.11f,0),
         h = _mix(h1,h2,0.2f);
    dSkin = _fminf(dSkin, sdCap(pos, head - h, head - h2)-0.25f);
    
    float3 posHead = pos-head;
    swi2S(posHead,x,z, mul_f2_mat2(swi2(posHead,x,z) , rot1))
    float3 posEar = posHead;
    posEar.z = _fabs(posEar.z);
    posEar-=to_float3(0.0f,-0.08f,0.29f);
    swi2S(posEar,z,x, mul_f2_mat2(swi2(posEar,z,x) , rot))
 
     // ear / noze
    dSkin = smin(dSkin, _fminf(sdEar(swi3(posEar,z,y,x)),
                               sdCap(posHead, -1.0f* _mix(h1,h2,0.4f), -1.0f* _mix(h1,h2,0.4f) + to_float3(0.28f,0,0))- 0.04f),0.02f);
    // Torso
    float3 a = _mix(ha0,ep0,0.15f), b = _mix(ha0,ep0,0.78f);
    
    // Neck
    float dNeck = sdCap(pos, ep0-to_float3(0.08f,0,0), head-to_float3(0.08f,0.1f,0))- 0.1f;
    dSkin = smin(dSkin, dNeck,0.06f);
  
    float dTorso = smin(sdCap(pos,shoulder1,shoulder2)-0.11f, sdCap2(pos, a, b, 0.23f,0.28f),0.095f);
    dSkin = _fminf(dSkin, dTorso);
   
    dTorso = smin(dTorso, sdCap(pos, shoulder1, _mix(shoulder1,elbow1,0.3f))- 0.1f,0.05f);

    // Arm 1
    dSkin = smin(dSkin, sdCap(pos, shoulder1, elbow1)- 0.1f,0.05f);
    dSkin = _fminf(dSkin, sdCap2(pos, elbow1, wrist1-0.05f*normalize(wrist1-elbow1), 0.1f, 0.08f));
   
    float3 p2 = mul_f3_mat3((pos-wrist1),baseArm1); // change to hand base
    float d2 = sdCap(p2,to_float3(-0.1f,0.12f,0.04f),to_float3(-0.04f,0.18f,0.06f))-0.05f;
    p2.z -= 1.5f*p2.x*p2.x;
    d2 = _fminf(d2,sdEllipsoid(p2-to_float3(0.02f,0.05f,0), to_float3(0.17f,0.14f,0.07f)));

    // Arm 2
    dTorso = smin(dTorso, sdCap(pos, shoulder2, _mix(shoulder2,elbow2,0.3f))- 0.1f,0.05f);
    dSkin = smin(dSkin, sdCap2(pos, shoulder2, elbow2, 0.11f,0.1f),0.05f);
    dSkin = _fminf(dSkin, sdCap2(pos, elbow2, wrist2-0.105f*normalize(wrist2-elbow2), 0.1f, 0.08f));
    
    p2 = mul_f3_mat3((pos - wrist2),baseArm2); // change to hand base
    d2 = _fminf(d2,sdCap(p2,to_float3(-0.1f,0.12f,0.04f),to_float3(-0.04f,0.18f,0.06f))-0.05f);
    p2.z -= 1.5f*p2.x*p2.x;
    d2 = _fminf(d2,sdEllipsoid(p2-to_float3(0.02f,0.05f,0), to_float3(0.17f,0.14f,0.07f)));
    
     dSkin = smin(d2, dSkin, 0.1f);

    // Short
    float dShort = _fminf(sdCap(pos, hip1, _mix(hip1,knee1,0.7f)), 
                          sdCap(pos, hip2, _mix(hip2,knee2,0.7f)))-0.14f;  
    dShort = _fminf(dShort, smin(dShort, sdEllipsoid(pos-ha0-to_float3(0.01f,0.06f,0),to_float3(0.23f,0.3f,0.3f)), 0.05f));
 
    // Belt
    float3 p3 = p0;
    p3.y -= ha0.y+0.2f;
    float dBelt = _fmaxf(max(_fminf(dTorso-0.05f,dShort-0.02f), p3.y), -p3.y-0.16f);
    dShoes1 = _fminf(dShoes1, dBelt); 

    float dMetal = _mix(dBelt,sdCappedCylinder(swi3(p0-ha0-to_float3(0.2f,0.11f,0),z,x,y), to_float2(0.07f,0.14f)),0.5f);
    float dNeck2 = length(pos - ep0)-0.25f;
    dTorso = _fmaxf(max(dTorso-0.03f, -dNeck2), -dSkin + 0.005f);
    dTorso = _fmaxf(dTorso, -pos.y + a.y);

    pos.x += 0.2f;
    pos = pos - ep0;
    pos  = mul_f3_mat3(pos, baseBag);
    float3 pos0 = pos;
    
    // Backpack
    float ta = _cosf(2.8f+0.25f*PI*gTime);

    // Water
    float3 p = pos - to_float3(-0.50f,-0.5f,-0.4f);
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , rot2))
    dMetal = _fminf(dMetal, _fminf(sdCap(p,to_float3_s(0), to_float3(0,-0.4f-0.04f*ta,-0.04f*ta))- 0.1f,
                                   sdCap(p,to_float3(0,0.1f,0.01f*ta), to_float3(0,0.15f,0.015f*ta))-0.07f)); 
    pos.z = _fabs(pos.z);
    float dPack = sdBezier(pos, to_float3(-0.2f,0.2f,0.17f),to_float3(0.9f,0.2f,0.36f),to_float3(-0.2f,-0.6f,0.3f)).x-0.04f;
   
    dPack = _mix(dPack, dTorso,0.2f);

    pos = pos0;
  
    float ta2 = _cosf(2.5f+0.25f*PI*gTime);
    pos.y += 0.15f*ta2*ta2;
    pos.z += 0.06f*ta2*ta2*ta2;
  
    float dBed = sdCappedCylinder(swi3(pos,y,z,x)+to_float3(-0.35f,0,0.5f), to_float2(0.2f,0.5f)); 
    
    // Teapot
    float3 posTeapot = 3.0f*swi3(pos - to_float3(-0.9f,0.25f,0.4f),y,x,z);
    swi2S(posTeapot,x,y, mul_f2_mat2(swi2(posTeapot,x,y) , rot2))
    swi2S(posTeapot,y,z, mul_f2_mat2(swi2(posTeapot,y,z) , rot))
    swi2S(posTeapot,y,x, swi2(posTeapot,y,x) * -1.0f)
    float dTeapot = 0.7f*sdTeapot(posTeapot-to_float3(1.0f,-1.0f,0))/3.0f;
    
    dPack = _fminf(dPack, sdCappedCylinder(pos-to_float3(-0.75f,0.23f,0.26f), to_float2(0.05f,0.03f))); 
    pos.z = _fabs(pos.z);
    dPack = _fminf(dPack, sdCappedCylinder(swi3(pos,y,z,x)-to_float3(0.35f,0.25f,-0.5f), to_float2(0.22f,0.05f))); 
    dPack = _fminf(dPack, udRoundBox(pos0-to_float3(-0.33f,-0.25f,-0.35f), to_float3(0.08f,0.1f,0.1f),0.04f)); 
    dPack = smin(dPack, udRoundBox(pos0-to_float3(-0.33f,-0.2f,0), to_float3(0.1f,0.3f,0.2f), 0.15f), 0.12f);
    
    // Little box
    dBed = _fminf(dBed, udRoundBox(pos0-to_float3(-0.33f,-0.1f,-0.35f), to_float3(0.08f,0.1f,0.1f),0.0f)); 
    
    // Cap
    pos = posHead + h2;
    float d = pos.x*0.2f-pos.y+0.15f*_cosf(5.0f*pos.z)-0.12f;
    float dHat = _fmaxf(sdCap(pos, to_float3_s(0), to_float3(0.2f,0,0))-0.27f, -d);
    dHat = _fminf(dHat, _mix(dSkin, sdCappedCylinder(pos, to_float2(0.25f,0.23f)),0.4f)-0.01f);      
    dHat = _fmaxf(dHat, d-0.02f);
    dPack = _fminf(dPack, dHat);

    // Asssociate distance to materials
    res = min2(res, to_float2(dTeapot, TEAPOT));
    res = min2(res, to_float2(dTorso, SHIRT));
    res = min2(res, to_float2(dShort, SHORT));
    res = min2(res, to_float2(dShoes1, SHOES1));
    res = min2(res, to_float2(dShoes2, SHOES2));
    res = min2(res, to_float2(dSkin, SKIN));
    res = min2(res, to_float2(dMetal, METAL));
    res = min2(res, to_float2(dPack, BAG));
    res = min2(res, to_float2(dBed, BED));
    
    // Distance field is not percise for cap, teapot and hands
    res.x *= 0.8f;
    return res;
}

#ifdef PANEL

__DEVICE__ float sdFont(in float2 p, in int c, __TEXTURE__ iChannel1) {
    float2 uv = (p + to_float2((float)(c%16), (float)(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(max(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel1, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage2D(in float2 p, int i0, in int[15] txt, in float scale, __TEXTURE__ iChannel1) { 
    p /= scale;
   float d = 999.0f, w = 0.45f; // letter width  
    p.x += w*(float)(txt.length()-1)*0.5f; // center text arround 0
    for (int id = i0; id<15; id++){
        if (txt[id] == 0) break;
      d = _fminf(d, sdFont(p, txt[id]),iChannel1);   
      p.x -= w; 
    }
    return scale*d;
}

__DEVICE__ float2 sdPanel(float3 p) {
    p.x -= 300.0f;
    p.z += 2.5f;
    float d = udRoundBox(p, to_float3(0.05f,1.9f,0.05f),0.01f);
    d = _fminf(d, udRoundBox(p-to_float3(0,1.5f,0.07f), to_float3(0.7f,0.25f,0.02f),0.01f));  
    return to_float2(d, PANEL);
}

#endif

__DEVICE__ float2 map(in float3 p0, float gTime){
    // Little stones    
    float2 size = to_float2(35.0f,20.0f),
         id = _floor((swi2(p0,x,z) + size*0.5f)/size);
    float3 pos = p0;
    swi2S(pos,x,z, mod_f(swi2(p0,x,z) + size*0.5f,size) - size*0.5f)
    float2 h = 1.0f-2.0f*hash22(id);
    float r = 0.15f+0.25f*_fabs(h.x),
          d = length(pos - to_float3(h.x*5.0f,-r*0.4f,7.0f*h.y))-r;
    float2 res = to_float2(d,GROUND);
#ifdef PANEL
    res = min2(res, sdPanel(p0)); 
#endif
#ifdef BONE
    res = min2(res, sdBone(p0));
#endif    
    d = length(p0-hip1)-2.0f;
    if (d<0.0f) {
        return min2(sdMan(p0,gTime), res);
    } else {    
        return min2(to_float2(d+0.1f,999.0f), res);
    }
}

//---------------------------------------------------------------------
//   Ray marching scene if ray intersect bbox
//---------------------------------------------------------------------

__DEVICE__ float2 Trace( in float3 ro, in float3 rd, float gTime) {
    float2 res = to_float2(999,0);
    float t = 0.5f;
    for( int i=ZERO; i<128 && t<100.0f; i++ ) {
        float2 h = map( ro+rd*t,gTime);
        if( _fabs(h.x)<0.0005f*t ) { 
            res = to_float2(t,h.y); 
            break;
        }
        t += h.x;
    }
    return res;
}

//------------------------------------------------------------------------
// [Shane] - Desert Canyon - https://www.shadertoy.com/view/Xs33Df
//------------------------------------------------------------------------
// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float tex3D(__TEXTURE2D__ tex, in float3 p, in float3 n){

    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= (n.x + n.y + n.z );  
  return (texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z).x;
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 n, float k){
    const float ep = 0.001f;
    float3 grad = to_float3( tex3D(tex, to_float3(p.x-ep, p.y, p.z), n),
                      tex3D(tex, to_float3(p.x, p.y-ep, p.z), n),
                      tex3D(tex, to_float3(p.x, p.y, p.z-ep), n));
    grad = (grad - tex3D(tex, p, n))/ep;             
    grad -= n*dot(n, grad);          
    return normalize(n + grad*k);
}

//---------------------------------------------------------------------
//   Ambiant occlusion
//---------------------------------------------------------------------

#ifdef WITH_AO
__DEVICE__ float calcAO( in float3 pos, in float3 nor, float gTime ){
  float dd, hr, sca = 1.0f, totao = 0.0f;
    float3 aopos; 
    for( int aoi=ZERO; aoi<5; aoi++ ) {
        hr = 0.01f + 0.05f*float(aoi);
        aopos = nor * hr + pos;
        totao += -(map(aopos,gTime).x-hr)*sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - 4.0f*totao, 0.0f, 1.0f);
}
#endif

__DEVICE__ float textureInvader(float2 uv) {
  float y = 7.0f-_floor((uv.y)*16.0f+4.0f);
  if (y<0.0f || y>7.0f) return 0.0f;
  float x = _floor((_fabs(uv.x))*16.0f),
        v=y>6.5? 6.:y>5.5? 40.:y>4.5? 47.:y>3.5?63.:
          y>2.5? 27.:y>1.5? 15.:y>0.5? 4.:8.;
  return _floor(mod_f(v/_powf(2.0f,x), 2.0f)) == 0.0f ? 0.: 1.0f;
}

__DEVICE__ float3 doColor(in float3 p, in float3 rd, in float3 n, in float2 res, float gTime, __TEXTURE__ iChannel0, __TEXTURE__ iChannel1){
    // sky dome
    float3 skyColor =  0.5f*to_float3(0.5f, 0.6f, 0.9f),
         col = skyColor - _fmaxf(rd.y,0.0f)*0.5f;
  
    float ss = 0.5f, sp = 0.0f;
#ifdef BONE    
    if (res.y == GROUND || res.y == BONE) {
#else    
    if (res.y == GROUND) {
#endif    
        col = 0.7f+ 0.5f *to_float3_s(texture(iChannel0,0.1f*swi2(p,z,x)).x)+p.y;
        col.x *= 0.8f;
        n = doBumpMap(iChannel0, 0.1f*p, n, 0.002f);
        ss = 0.0f;
        sp = 0.3f;
    } else 
    if (res.y == SHOES1) {
        sp = 0.1f;
        float3 pFoot = mul_f3_mat3((p-ankle1),baseFoot1);
        col = _mix(to_float3(0.1f,0.1f,0), to_float3(0,0,0.1f), smoothstep(-0.1f,-0.09f,pFoot.z));
    } else if (res.y == SHOES2) {
        col = to_float3(0,0,0.1f);
        sp = 0.1f;
        float3 pFoot = mul_f3_mat3((p-ankle2),baseFoot2);
        col = _mix(to_float3(0.1f,0.1f,0), to_float3(0,0,0.1f), smoothstep(-0.1f,-0.09f,pFoot.z));
    } else if (res.y == SKIN) {
        col = to_float3(222,177,144)/255.0f;
        ss = 1.0f;
        sp = 0.1f;
        if (p.x>head.x) {
      // Draw simple face
            float3 phead = (p - head);
            swi2S(phead,x,z, mul_f2_mat2(swi2(phead,x,z) , rot1))
            float2 p2 = swi2(phead,z,y);
            p2.x = _fabs(p2.x);
            float d = length(p2-to_float2(0.1f,0))-0.02f;
            d = _fminf(d, _fmaxf(length(p2-to_float2(0.0f,-0.18f))-0.05f, -length(p2-to_float2(0.0f,-0.14f))+0.07f))            ;
            col = _mix(to_float3_s(0), col, smoothstep(0.0f,0.01f,d));
        }
    } else if (res.y == METAL) {
        col = to_float3(0.8f,0.9f,1.0f);
        sp = 2.0f;
    } else if (res.y == SHORT) {
        col = to_float3(0.35f,0.7f,0.85f);
    } else if (res.y == BAG) {
//        float3 ep0 = _mix(shoulder1,shoulder2,0.5f);
//        p.x += 0.2f;
//        p -= ep0;
        col = to_float3(0.3f,0.5f,0.2f);
//        nor = doBumpMap(iChannel0, 0.1f*(p*baseBag), nor, 0.002f);
        sp = 0.1f;
    } else if (res.y == SHIRT) {
        col = to_float3(0.3f,0.4f,0.5f);
        float2 p2 = swi2(p,z,y);
        p2.y -= _mix(_mix(shoulder2,shoulder1,0.5f),
                     _mix(hip2,hip1,0.5f),0.5f).y;
        col *= 1.0f-0.5f*textureInvader(p2*1.9f-to_float2(0,0.3f));
        ss = 0.2f;
    } else if (res.y == TEAPOT) {
        col = to_float3(1.0f,0.01f,0.01f);
        sp = 0.5f;
    } else if (res.y == BED) {
        col = to_float3(1.0f,0.5f,0.01f);
        ss = 1.0f;
        sp = 0.3f;
    }
#ifdef PANEL
    else if (res.y == PANEL) {
        col = to_float3(0.8f,0.4f,0.1f);
        if (p.z > -2.4f) {
            int[] gtxt = int[] (72,69,76,76,79,0,0,70,65,66,82,73,67,69,33);
            float d = _fminf(sdMessage2D(swi2(p,x,y)-to_float2(300.66f,1.6f),0, gtxt, 0.3f,iChannel1),
                             sdMessage2D(swi2(p,x,y)-to_float2(300.5f,1.4f),7, gtxt, 0.3f,iChannel1));
            col = _mix(to_float3(0), col, smoothstep(0.0f,0.01f,d));
        }
    }
#endif
    else {
        return col;
    }
#ifdef BONE    
    if (res.y == BONE) {
        ss = 1.0f;
        sp = 1.0f;
    }
#endif    
    float2 d = to_float2(res.x, res.y);
    float3 ld = -1.0f*normalize(to_float3(50,100,-100)-p);
    // IQ sss version
    float sss = ss*0.2f*clamp(0.5f+0.5f*dot(ld,n),0.0f,1.0f)*(2.0f+dot(rd,n));
    float3 r = reflect(rd,n);
    float diff = _fmaxf(0.0f,dot(n,ld)),
         amb = dot(n,ld)*0.45f+0.55f,
         spec = _powf(_fmaxf(0.0f,dot(r,ld)),40.0f),
         fres = _powf(_fabs(0.7f+dot(rd,n)),3.0f),   
         ao = calcAO(p, n,gTime);
    // ligthing     
    col = col*_mix(1.2f*to_float3(0.25f,0.08f,0.13f),to_float3(0.984f,0.996f,0.804f), _mix(amb,diff,0.75f)) + 
          spec*sp+fres*_mix(col,to_float3_s(1),0.7f)*0.4f;
    // kind of sub surface scatering      
    col += sss*to_float3(1.0f,0.3f,0.2f);
    // sky light reflected from the ground
    col += _fmaxf(0.0f,dot(to_float3(0,-1,0), n))*0.1f*skyColor;
    // ambiant occusion
    col *= _mix(ao,1.0f,0.5f);
    // fade in distance
    return _mix( col, skyColor, smoothstep(30.0f,100.0f, res.x) );
}


//---------------------------------------------------------------------
//   Calculate normal
// inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
//---------------------------------------------------------------------
__DEVICE__ float3 normal(in float3 pos, float3 rd, float t, float gTime ) {
    float3 n = to_float3_s(0);
    for( int i=ZERO; i<4; i++) {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.002f*e,gTime).x;
    }
  return normalize(n - _fmaxf(0.0f, dot(n,rd ))*rd);
}

//---------------------------------------------------------------------
//   Camera
//---------------------------------------------------------------------

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, in float r) {
  float3 w = normalize(ta-ro),
         p = to_float3(_sinf(r), _cosf(r),0.0f),
         u = normalize( cross(w,p) ),
         v = normalize( cross(u,w) );
    return to_mat3_f3( u, v, w );
}

//---------------------------------------------------------------------
//   Entry point
//---------------------------------------------------------------------
//#define iTime (iTime + 250.0f) // Direct to big Bone
__KERNEL__ void OnTheSalFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

iframe = iFrame-0;

//---------------------------------------------------------------------
//    Animation
//---------------------------------------------------------------------

//                       Contact           Down               Pass               Up      

float3 HEAD[9] = { to_float3(50,24,0),  to_float3(73,30,0),   to_float3(94,20,0),   to_float3(117,15,0),  
                   to_float3(135,24,0), to_float3(158,30,0),  to_float3(179,20,0),  to_float3(202,15,0), to_float3(218,24,0)};

float3 SHOULDER[9] = { to_float3(44,47,16),to_float3(66,53,16), to_float3(91,43,16), to_float3(115,38,16), 
                       to_float3(136,50,16), to_float3(158,55,16), to_float3(176,43,16), to_float3(85+111,37,16), to_float3(212,47,16)};

float3 ELBOW[9] = { to_float3(25,64,25), to_float3(46,67,25),  to_float3(88,70,25),  to_float3(120,65,25),
                    to_float3(139,72,25),to_float3(172,67,25), to_float3(176,71,25), to_float3(177,61,25), to_float3(193,64,25)};

float3 WRIST[9] = { to_float3(20,85,15), to_float3(35,76,20), to_float3(88,100,25), to_float3(128,89,25), 
                    to_float3(164,85,15), to_float3(187,81,20),to_float3(85+88,98,25),to_float3(85+82,81,20), to_float3(188,85,15)};

float3 HIP[9] = { to_float3(42,90,10),  to_float3(62,95,10),   to_float3(83,88,10),   to_float3(107,83,10),  
                  to_float3(127,92,10), to_float3(147,94,10),  to_float3(168,91,10),  to_float3(192,85,10), to_float3(210,90,10)};

float3 KNEE[9] = { to_float3(29,118,7),  to_float3(48,120,8),   to_float3(97,117,10),  to_float3(130,107,10), 
                   to_float3(144,120,7), to_float3(167,118,7),  to_float3(167,118,7),  to_float3(181,111,7), to_float3(197,118,7)};

float3 ANKLE[9] = { to_float3(5,134,5),   to_float3(22,132,6),   to_float3(71,122,10),  to_float3(113,127,10), 
                    to_float3(162,146,5), to_float3(164,146,5),  to_float3(164,146,5),  to_float3(168,137,5), to_float3(173,134,5)};

float3 FOOT[9] = { to_float3(14,150,10), to_float3(16,150,10),  to_float3(63,139,10),  to_float3(119,143,10), 
                   to_float3(178,139,10),to_float3(182,150,10), to_float3(182,150,10), to_float3(182,150,10), to_float3(182,150,10)};
    
    //iTime += 130.0f;
    float gTime = (iTime+130.0f)*6.0f;
  
    // Animation
    int it = int(_floor(gTime));
    float kt = fract(gTime), dz = 1.0f;
   
    head = getPos(HEAD, it, kt, dz, gTime);

    shoulder1 = getPos(SHOULDER, it, kt, -dz, gTime);
    elbow1 = getPos(ELBOW, it, kt, -dz, gTime);
    wrist1 = getPos(WRIST, it, kt, -dz, gTime);
    
    foot1 = getPos(FOOT, it, kt, dz, gTime);
    ankle1 = getPos(ANKLE, it, kt, dz, gTime);
    knee1 = getPos(KNEE, it, kt, dz, gTime);
    hip1 = getPos(HIP, it, kt, dz, gTime);
    
    shoulder2 = getPos(SHOULDER, it+4, kt, dz, gTime);
    elbow2 = getPos(ELBOW, it+4, kt, dz, gTime);
    wrist2 = getPos(WRIST, it+4, kt, dz, gTime);

    foot2 = getPos(FOOT, it+4, kt, -dz, gTime);
    ankle2 = getPos(ANKLE, it+4, kt, -dz, gTime);
    knee2 = getPos(KNEE, it+4, kt, -dz, gTime);
    hip2 = getPos(HIP, it+4, kt, -dz, gTime);
    
    float dx = it%8 < 4 ? -85.0f*0.02f : 85.0f*0.02f; 
    foot2.x += dx;
    ankle2.x += dx;
    knee2.x += dx;
    hip2.x += dx;

    shoulder2.x += dx;
    elbow2.x += dx;
    wrist2.x += dx;
    
    float3 v1 = normalize(wrist1-elbow1),
    v0 = normalize(wrist1-shoulder1),
    v3 = normalize(cross(v1,v0)),
    v2 = cross(v1,v3);
    baseArm1 = to_mat3_f3(v0,v2,-v3);
    
    v1 = normalize(wrist2-elbow2),
    v0 = normalize(wrist2-shoulder2),
    v3 = normalize(cross(v1,v0)),
    v2 = cross(v1,v3);
    baseArm2 = to_mat3_f3(v0,v2,v3);
    
    v1 = normalize(shoulder1-shoulder2);
    v0 = normalize(_mix(hip1,hip2,0.5f)-_mix(shoulder1,shoulder2,0.5f));
    v2 = normalize(cross(v1,v0));
    v3 = normalize(cross(v1,v2));
    baseBag = to_mat3_f3(-v2,v3,v1);
    
    v2 = normalize(knee1 - ankle1);
    v1 = normalize(ankle1 - foot1-v2*0.1f);
    v3 = cross(v1,v2);
    baseFoot1 = to_mat3_f3(v1,v3,-1.0f*cross(v1,v3));
    
    v2 = normalize(knee2 - ankle2);
    v1 = normalize(ankle2 - foot2-v2*0.1f);
    v3 = cross(v1,v2);
    baseFoot2 = to_mat3_f3(v1,v3,-1.0f*cross(v1,v3));
    
    float a = -1.5708f*0.4f;
    rot = to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a));
     
    a = 0.2f*_cosf(0.4f*iTime) + 0.3f*_cosf(0.05f*iTime);
    rot1 = to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a));
     
    a = 0.5f*_cosf(0.5f*3.141592f*gTime);
    a = a*a;
    rot2 = to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a));
    
// ------------------------------------
 
    // Screen 
    float2 q = fragCoord/iResolution, 
         m = swi2(iMouse,x,y)/iResolution.y - 0.5f,
         p = -1.0f+2.0f*q;
         p.x *= iResolution.x/iResolution.y;        
      
    // Camera  
    float3 ro = to_float3( hip1.x+12.0f*_cosf(PI*(0.05f*iTime+m.x)),
                           4.5f+2.0f*(_sinf(0.1f*iTime))+4.0f*(m.y+0.3f),
                           hip1.z+12.0f*_sinf(PI*(0.05f*iTime+m.x)));
    float3 ta = hip1;
    ta.x +=1.2f;
    ta.y = 1.2f;
    mat3 ca = setCamera(ro, ta, 0.0f);
    float3 rd = mul_mat3_f3(ca , normalize(to_float3_aw(swi2(p,x,y),4.5f) ));
  
    // Ray intersection with scene
    float2 res = Trace(ro, rd,gTime);
    if (rd.y >= 0.0f) {
       res = min2(res, to_float2(999.0f,100.0f));
    } else {        
       res = min2(res, to_float2(-ro.y / rd.y,GROUND));
    }

    
    // Rendering
    float3 pos = ro + rd*res.x;
    float3 n = pos.y<0.02f ? to_float3(0,1,0) : normal(pos, rd, res.x, gTime);
    float3 col = doColor(pos, rd, n, res, gTime, iChannel0, iChannel1);
    col = pow_f3( col, to_float3_s(0.4545f) );                 // Gamma    
    col *= _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.1f); // Vigneting
     
    fragColor = to_float4_aw(col,1);


  SetFragmentShaderComputedColor(fragColor);
}