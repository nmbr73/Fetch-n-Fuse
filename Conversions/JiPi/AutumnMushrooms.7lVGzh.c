
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Wood' to iChannel0
// Connect Image 'Texture: Organic 2' to iChannel1
// Connect Image '/media/a/894a09f482fb9b2822c093630fc37f0ce6cfec02b652e4e341323e4b6e4a4543.mp3' to iChannel2


// Created by sebastien durand - 11/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// *****************************************************************************
// Based on
// iq - Quadratic Bezier - https://www.shadertoy.com/view/ldj3Wh
// *****************************************************************************

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define AA 1

#define ZERO 0 //(_fminf(iFrame,0))

#define nose    2.0f
#define hat     3.0f
#define dress   4.0f
#define leg     5.0f
#define foot    6.0f
#define bear    7.0f
#define mushrom 9.0f

//-----------------------------------------------------------------------------------

__DEVICE__ float3 getPtOnBez(float3 p0, float3 p1, float3 p2, float t) {
    return (1.0f - t) * (1.0f - t) * p0 + 2.0f * (1.0f - t) * t * p1 + t * t * p2;
}

//-----------------------------------------------------------------------------------
// Mercury
__DEVICE__ float fOpUnionRound(float a, float b, float r) {
  return _fmaxf(r, _fminf (a, b)) - length(_fmaxf(to_float2(r - a,r - b), to_float2_s(0)));
}

__DEVICE__ float pModPolar(inout float2 *p, float rep) {
  float an = 3.141592f/rep,
         a = _atan2f((*p).y, (*p).x) + an,
         r = length(*p),
         c = _floor(0.5f*a/an);
  a = mod_f(a,2.0f*an) - an;
  *p = to_float2(_cosf(a), _sinf(a))*r;
  if (_fabs(c) >= rep*0.5f) c = _fabs(c);
  return c;
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

__DEVICE__ float2 sdCapsule(in float3 p,in float3 a,in float3 b) {
  float3 pa = p - a, ba = b - a;
  float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f);
  return to_float2(length(pa - ba*h), h);
}

__DEVICE__ float sdEllipsoid(in float3 p,in float3 r) {
  float k0 = length(p/r);
  return k0*(k0-1.0f)/length(p/(r*r));
}

//-----------------------------------------------------------------------------------
// Scene modeling
__DEVICE__ float2 sdMush(in float3 p) {
    float d = 0.5f*(sdBezier(p, to_float3(0,0,0), to_float3(0.4f,3,0), to_float3(0.2f,4.2f,0.3f)).x-0.1f);
  d = fOpUnionRound(d,_fmaxf(sdEllipsoid(p-to_float3(0.3f,3.3f,0.3f), to_float3(1.2f,0.7f,1.2f)),
                    -sdEllipsoid(p-to_float3(0.27f,3.0f,0.25f), to_float3(1.3f,0.7f,1.5f))),0.5f);
  d = _fminf(d,sdEllipsoid(p-to_float3(-3.0f,0.6f,6.0f), to_float3(0.8f,0.6f,0.8f)));
  d = _fminf(d,sdEllipsoid(p-to_float3(4.5f,0.5f,2.5f), to_float3(0.6f,0.5f,0.6f)));
  d = _fminf(d,sdEllipsoid(p-to_float3(-6.5f,0.5f,-8.5f), to_float3(0.6f,0.5f,0.6f)));
  return to_float2(d, mushrom);
}

__DEVICE__ float2 sdHand(float3 p, float3 p10, float3 p11, float3 n) {
    float3 knee = 0.5f*(p11+p10) + n,
         p2 = getPtOnBez(p10, knee, p11, 0.2f),
         nn = normalize(p10-p2);
    // harm     
    float2 h = sdBezier(p, p11, knee, p10);
    float d,
          dm = _fmaxf(h.x - 0.1f - 0.05f*h.y, -length(p-p10)+0.2f),
          hm=h.y;
    // fingers
    d = sdCapsule(p, p10+to_float3(0.03f*sign_f(n.x),0,0), p2).x;
    p += nn*0.05f;
    d = _fminf(d, sdCapsule(p, p10+to_float3(0,0.05f,0), p2+to_float3(0,0.05f,0)).x);
    d = _fminf(d, sdCapsule(p, p10-to_float3(-0.02f*sign_f(n.x),0.05f,0), p2-to_float3(0,0.05f,0)).x);
    p += nn*0.05f;
    d = _fminf(d, sdCapsule(p, p10-to_float3(0,0.1f,0), p2-to_float3(0,0.1f,0)).x);
    d -= 0.05f;
    return d < dm ? to_float2(d, nose) : to_float2(dm, dress);
}

__DEVICE__ float2 sdLeg(float3 p, float3 foot10, float3 foot11, float3 n) {
    float3 knee = 0.5f*(foot11+foot10) + n,
         p4 = getPtOnBez(foot10, knee, foot11, 0.2f);
    float2 h = sdBezier(p, foot11, knee, foot10);
    float d,dm = h.x - 0.1f, 
          hm = leg + h.y;
    // foot
    h = sdCapsule(p, foot10, p4);
    d = _fmaxf(h.x - 0.2f, -length(p- _mix(foot10, p4, 2.5f)) + 0.4f);
    if (d<dm) { dm=d; hm=dress;}
    p.y += 0.1f; 
    h = sdCapsule(p, foot10, foot10 + n);
    d = h.x - 0.2f;
    if (d<dm) { dm=d; hm=hat; }
    return to_float2(dm,hm);
}

__DEVICE__ float invMix(float v0, float v1, float v) {
    return v1 == v0 ? 1.0f : (v-v0)/(v1-v0);
}

__DEVICE__ float getAmp(__TEXTURE2D__ iChannel2, float frequency) { return texture(iChannel2, to_float2(frequency / 512.0f, 0)).x; }

#define BPM 127.0f
__DEVICE__ float2 sdLutin(in float3 p, in float lid, __TEXTURE2D__ iChannel2) {
    float t = 2.11666f,//iChannelTime[2]*2.11666f,
          a1 = 1.0f,//(getAmp(lid*lid * 20.0f,iChannel2)*0.5f+0.5f)*_cosf(9.0f*t+1.57f * lid),
          anim = 1.0f,//(getAmp(lid*lid * 40.0f,iChannel2)*0.5f+0.5f)*_cosf(6.0f*t+1.57f * lid),
          gg = 0.5f*_cosf(lid*110.0f);
          
    float3 head = to_float3(0,2.5f+gg,0),
           hips = to_float3(0,1.2f+gg,0);
   
    head += 0.2f*to_float3(0.5f,0.5f,0.2f)*(a1 + 0.5f*anim);
    hips += 0.3f*to_float3(0.5f,0.2f,0.2f)*anim;   
    
    float3 epaule = head - to_float3(0,0.7f,0),
         c = head + to_float3(0,0.9f,-0.8f),
         b = head + to_float3(0,0.65f,-0.3f);

    float d, dm, hm = nose;
    
    // nez
    dm = sdEllipsoid(p- head - to_float3(0,0,0.5f), to_float3(0.3f,0.15f,0.3f));
    
    // bras
    float s = p.x>0.?1.0f:-1.0f;
    float2 h = sdHand(p, epaule + to_float3(s*1.0f,-0.7f+s*0.3f*anim- 0.3f*gg,0.5f+ 0.4f*gg), epaule+to_float3(s*0.4f,0.0f,-0.05f), to_float3(s*0.2f,-0.2f,-0.2f));
    if( h.x<dm ) { dm=h.x; hm = h.y; }
 
    // body
    float3 pb = p;
    pb.z /= 0.7f;
    pb.z -= 0.2f*_cosf(p.y)*smoothstep(epaule.y,hips.y,invMix(epaule.y,hips.y, p.y));
    h = sdCapsule(pb, epaule+to_float3(0,-0.15f,0), hips-to_float3(0,0.7f,0));
    d = _fmaxf(h.x - _mix(0.4f,0.6f,h.y), -length(pb-hips+to_float3(0,0.8f,0)) + 0.7f);
    if (d<dm) { hm = dress; }
    dm = 0.7f*fOpUnionRound(d, dm, 0.15f);
    
    // legs  
    h = sdLeg(p, to_float3(s*0.5f,-0.7f,0), hips + to_float3(s*0.25f,-0.2f,0), to_float3(s*0.2f,0,0.3f));
    if (h.x<dm) { dm=h.x; hm=h.y; }
    
     // bonet
    float3 p3 = p + to_float3(0,0.1f,-0.1f);
    h = sdBezier( p3, head-to_float3(0,0.05f,0), b, c );
    d = 0.7f*_fmaxf( h.x - 0.5f + 0.5f*h.y, -length(p3-(head-to_float3(0,0.8f,-0.6f))) + 1.0f);
    if( d<dm ) { dm=d; hm=hat; }

    // barbe
    float3 p4 = p;
    float k = _mix(1.0f,3.0f,smoothstep(head.y, head.y-1.0f, p4.y));
    p4.z*=k;
    h = sdBezier( p4, head, head - to_float3(0,1.0f,0.0f), to_float3(head.x, head.y, head.z*k) - to_float3(-0.2f*anim,1,-3.0f));
    d = 0.7f*fOpUnionRound(h.x - 0.3f*_sinf(3.14f*h.y), length(p-(head-to_float3(0,0.4f,-0.2f)))-0.5f, 0.15f);
    if (d<dm) { dm=d; hm=bear; }

  return to_float2(dm*0.9f, 10.0f*lid + hm );
}

__DEVICE__ float2 map(in float3 p, __TEXTURE2D__ iChannel2) {
  float2 h2, h1 = sdMush(p-to_float3(0,-1.05f,0));
  float2 pxz = swi2(p,x,z);
  float id = pModPolar(&pxz, 16.0f),
         d = sdEllipsoid(p-to_float3(3,1.35f,0), to_float3(1.3f,2.9f,1.6f));
  p.x=pxz.x;p.z=pxz.y;       
  if (d>0.0f) h2 = to_float2(d+0.1f,0);
  else        h2 = sdLutin(swi3(p - to_float3(3,0,0),z,y,x), id,iChannel2);
  return h1.x<h2.x ? h1 : h2;
}

//-------------------------------------------------------
// Ray marching
__DEVICE__ float3 intersect( in float3 ro, in float3 rd, __TEXTURE2D__ iChannel2 ) {
    float3 res = to_float3_s(-1.0f);
    float maxd = 25.0f;
    // plane
    float tp = (-0.85f-ro.y)/rd.y;
    if (tp>0.0f) {
        res = to_float3(tp,0,0);
        maxd = _fminf(maxd,tp);
    }
    // Lutins
    float t = 2.0f, l = 0.0f;
    for( int i=ZERO; i<92; i++ ) {
      float2 h = map(ro+rd*t,iChannel2);
      if (h.x<0.004f || t>maxd) break;
      t += h.x;
      l = h.y;
    }
    return t<maxd ? to_float3(t, l, 1.0f) : res;
}

__DEVICE__ float3 calcNormal( in float3 pos, __TEXTURE2D__ iChannel2 ) {
 // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    float3 n = to_float3_s(0);
    for( int i=ZERO; i<4; i++) {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.002f*e,iChannel2).x;
    }
    return normalize(n);
}

__DEVICE__ float softshadow( in float3 ro, in float3 rd, float mint, float k, __TEXTURE2D__ iChannel2 ) {
    float res = 1.0f, t = mint, h = 1.0f;
    for( int i=ZERO; i<48; i++ ) {
        h = map(ro + rd*t,iChannel2).x;
        res = _fminf( res, k*h/t );
        t += clamp( h, 0.002f, 2.0f );
        if( res<0.001f ) break;
    }
    return clamp(res,0.0f,1.0f);
}

__DEVICE__ float map2( in float3 pos, __TEXTURE2D__ iChannel2 ) {
    return _fminf(pos.y+0.85f, map(pos,iChannel2).x);
}


__DEVICE__ float calcAO( in float3 pos, in float3 nor, __TEXTURE2D__ iChannel2) {
    float h,d,ao = 0.0f;
    for( int i=ZERO; i<8; i++ ) {
        h = 0.02f + 0.5f*(float)(i)/7.0f;
        d = map2( pos + h*nor,iChannel2 );
        ao += h-d;
    }
    return clamp( 1.5f - ao*0.6f, 0.0f, 1.0f );
}


//------------------------------------------------------------------------
// [Shane] - Desert Canyon - https://www.shadertoy.com/view/Xs33Df
//------------------------------------------------------------------------
// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D(__TEXTURE2D__ tex, in float3 p, in float3 n){
    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= (n.x + n.y + n.z );  
  return swi3(texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z,x,y,z);
}

// Grey scale.
__DEVICE__ float grey(float3 p){ return dot(p, to_float3(0.299f, 0.587f, 0.114f)); }

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 n, float k){
    const float ep = 0.001f;
    float3 grad = to_float3( grey(tex3D(tex, to_float3(p.x-ep, p.y, p.z), n)),
                             grey(tex3D(tex, to_float3(p.x, p.y-ep, p.z), n)),
                             grey(tex3D(tex, to_float3(p.x, p.y, p.z-ep), n)));
    grad = (grad - grey(tex3D(tex, p, n)))/ep;             
    grad -= n*dot(n, grad);          
    return normalize(n + grad*k);
}

// iq palette
__DEVICE__ float3 pal(in float t) {
    return 0.5f + 0.5f*cos_f3(6.28318f*(t+to_float3(0.0f,0.33f,0.67f)) );
}


__KERNEL__ void AutumnMushroomsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float iChannelTime[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    float4 tot = to_float4_s(0);
    
#if AA>1
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
        // pixel coordinates
        float2 o = to_float2((float)(m),(float)(n)) / (float)(AA) - 0.5f,
        p = (-iResolution + 2.0f*(fragCoord+o))/iResolution.y;
#else    
        float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;
#endif

        //-----------------------------------------------------
        // camera
        //-----------------------------------------------------
        float an = 2.0f + 0.3f*iTime + 3.0f*smoothstep(0.9f,0.95f, _sinf(0.32f*iTime)) +  3.0f*smoothstep(0.9f,0.95f, _sinf(0.06f*iTime));

        float3 ro = _mix(1.75f,1.2f,smoothstep(-0.9f,-0.8f, _cosf(0.15f*iTime+0.001f*iTime*iTime)))*to_float3(10.0f*_sinf(an),5.0f,10.0f*_cosf(an)),
        ta = to_float3(0.02f*an,0,0);

        // camera matrix
        float a = 0.1f*_cosf(0.1f*iTime);
        float3 ww = normalize(ta - ro),
               uu = normalize(cross(ww,normalize(to_float3(_sinf(a),_cosf(a),0)))),
               vv = normalize(cross(uu,ww));

        // create view ray
        float3 rd = normalize( p.x*uu + p.y*vv + 2.5f*ww );

        //-----------------------------------------------------
        // render
        //-----------------------------------------------------

        float3 lig = normalize(to_float3(-0.2f,0.6f,0.9f));
        float sun = _powf( clamp( dot(rd,lig), 0.0f, 1.0f ), 8.0f );

        // raymarch
        float3 tmat = intersect(ro,rd,iChannel2);

        // geometry
        float3 nor, pos = ro + tmat.x*rd;
        if( tmat.z<0.5f)
             nor = to_float3(0,1,0);
        else nor = calcNormal(pos,iChannel2);
        
        float3 ref = reflect( rd, nor );

        // materials
        float3 mate = to_float3_s(0.5f);

        float lid = _floor(tmat.y*0.1f);
        tmat.y = mod_f(tmat.y,10.0f);
        float3 col, 
             col1 = pal(lid/6.0f),
             col2 = pal(lid/6.0f + 0.33f);

        if (tmat.y < nose) {
            float k = texture(iChannel1, 0.051f*swi2(pos,x,z)).x;
            mate = swi3(texture(iChannel1, 0.2f*swi2(pos,x,z)),x,y,z);
            mate = 0.12f*pow_f3(mate,to_float3_s(0.3f));
            mate = 0.5f*_mix(mate, to_float3(0.65f,0.5f,0.0f), 0.2f*smoothstep(0.5f,0.6f,k));
            nor = doBumpMap(iChannel1, 0.2f*pos, nor, 0.02f);
        } else if (tmat.y < hat) {
            mate = to_float3(0.68f,0.475f,0.446f);
        } else if (tmat.y < dress) {
            mate = col2;
        } else if (tmat.y < leg) {
            mate = col1;
        } else if (tmat.y < bear) {
            mate = _mix(col2, to_float3_s(0.2f), smoothstep( -0.1f, 0.1f, _cosf( 40.0f*tmat.y )));
        } else if (tmat.y < mushrom) {
            float3 p2 = pos;
            float2 p2xz = swi2(p2,x,z);
            float lid2 = pModPolar(&p2xz, 16.0f),
                    a1 = _cosf(6.0f*iTime+1.57f * lid2),
                  anim = _cosf(4.0f*iTime+1.57f * lid2);
            p2.x=p2xz.x;p2.z=p2xz.y;
            
            float3 head = to_float3(0,2.5f,0) + to_float3(0,0.5f*_cosf(lid2*110.0f),0),
            hips = to_float3(0,1.2f,0) + to_float3(0,0.5f*_cosf(lid2*110.0f),0);
            head += 0.2f*to_float3(0.5f,0.5f,0.2f)*(a1 + 0.5f*anim);
            nor = doBumpMap(iChannel0, 1.5f*(pos-head)*to_float3(1,0.2f,1), nor, 0.12f);
            mate = lid2<0.0f ? to_float3(211,110,76)/256.0f : to_float3_s(1.0f);
        } else {
            float3 p = pos - to_float3(0.2f,4.2f,0.3f);
            float r = length(swi2(p,x,z));
            if (r<2.0f) {
                mate = _mix(to_float3_s(0.7f), 0.5f*to_float3(1,0.5f,1), smoothstep(0.5f,1.5f,pos.y));
                nor = doBumpMap(iChannel0, to_float3(0.1f*_atan2f(p.x,p.z),0.1f*r,0), nor, 0.01f);
            } else {
                mate = to_float3_s(0.7f);
            }
            mate = 2.0f*_mix(0.25f*to_float3(1,0.7f,0.6f),mate,smoothstep(0.2f,0.3f,tex3D(iChannel1, 0.5f*pos, nor).x));
        }

        float occ = calcAO(pos, nor,iChannel2);
float zzzzzzzzzzzzzzzzzzzzzz;
        // lighting
        float sky = clamp(nor.y,0.0f,1.0f),
             bou = clamp(-nor.y,0.0f,1.0f),
             dif = _fmaxf(dot(nor,lig),0.0f),
             bac = _fmaxf(0.3f + 0.7f*dot(nor,-lig),0.0f),
             fre = _powf( clamp( 1.0f + dot(nor,rd), 0.0f, 1.0f ), 5.0f),
             spe = 0.5f*_fmaxf( 0.0f, _powf( clamp( dot(lig,reflect(rd,nor)), 0.0f, 1.0f), 8.0f)),
             sha = 0.0f; 
        if (dif>0.001f) sha=softshadow(pos+0.01f*nor, lig, 0.0005f, 32.0f,iChannel2);

        // lights
        float3 brdf = 2.0f*dif*to_float3(1.25f,0.9f,0.6f)*sha;
        brdf += 1.5f*sky*to_float3(0.1f,0.15f,0.35f)*occ;
        brdf += bou*to_float3_s(0.3f)*occ;
        brdf += bac*to_float3(0.3f,0.25f,0.2f)*occ;
        brdf += fre*occ*dif;

        // surface-light interacion
        col = swi3(mate,x,y,z)* brdf;
        col += (1.0f-swi3(mate,x,y,z))*spe*to_float3(1,0.95f,0.9f)*sha*2.0f*(0.2f+0.8f*fre)*occ;

        // fog
        col = _mix( col, 3.0f*to_float3(0.09f,0.13f,0.15f), smoothstep(7.0f,30.0f,tmat.x) );
        col += 0.4f*to_float3(1,0.68f,0.7f)*sun;
        tot += to_float4_aw(col, tmat.x);
#if AA>1
    }
    tot /= (float)(AA*AA);
#endif

    // Gamma
    swi3S(tot,x,y,z, pow_f3(clamp(swi3(tot,x,y,z),0.0f,1.0f), to_float3_s(0.5f)));
    // Vigneting
    float2 q = fragCoord/iResolution;
    swi3S(tot,x,y,z, swi3(tot,x,y,z) * _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.32f)); 
    fragColor = to_float4_aw(swi3(tot,x,y,z),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}