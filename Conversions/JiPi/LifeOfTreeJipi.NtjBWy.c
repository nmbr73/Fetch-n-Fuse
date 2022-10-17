
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

__DEVICE__ inline mat3 add_mat3( mat3 A, mat3 B) {  
  mat3 C;  

  C.r0 = to_float3(A.r0.x + B.r0.x, A.r0.y + B.r0.y,A.r0.z + B.r0.z);  
  C.r1 = to_float3(A.r1.x + B.r1.x, A.r1.y + B.r1.y,A.r1.z + B.r1.z); 
  C.r2 = to_float3(A.r2.x + B.r2.x, A.r2.y + B.r2.y,A.r2.z + B.r2.z);

  return C;  
  }

__DEVICE__ inline mat3 mul_mat3_f( mat3 A, float B)  
  {  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  }

// Created by sebastien durand /2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// TODO debug this for new chrome !!

//#define NO_LEAF
//#define BENJAMIN_BUTTON

#define ZERO 0 //_fminf(0,iFrame) 

 
#define  PI   3.14159265358979323846f
#define  PI2  2.0f*PI
#define  AU   149597870.61f // Astronomical Unit = Mean distance of the earth from the sun - Km
#define  TORAD  PI/180.0f
#define  TODEG  180.0f/PI
    
__DEVICE__ const float 
    LATITUDE = 42.0f, 
    LONGITUDE = 1.0f,
    UTC_HOUR = 7.8f,
    OLD_TREE = 1.3f,
    VERY_OLD_TREE = 3.0f;

__DEVICE__ const float3
          Julia    = {-0.1825f,-0.905f,-0.2085f},
          ba       = {0.0f,0.7f,0.0f},
          ba2      = {0.0f,2.1f,0.0f},
          SUN_COLOR= {1.0f, 0.75f, 0.6f};

__DEVICE__ float oobdb,oobd2;
    //oobdb = 1.0f/dot(ba,ba),
    //oobd2 = 1.0f/dot(ba2,ba2);

__DEVICE__ float jGlobalTime,life, season, treeStep, frc, f2, thinBranch, rBranch, clpOld, clpVeryOld;  
__DEVICE__ int bEnd, fStart, fEnd;
__DEVICE__ float3 sunLight, sunColour;


__DEVICE__ bool intersectSphere(in float3 ro, in float3 rd, in float r) {
  float b = dot(rd,ro), d = b*b - dot(ro,ro) + r*r;
  return (d>0.0f && -_sqrtf(d)-b > 0.0f);
}


// - Tools ---------------------------------------------------------------------------
__DEVICE__ float hash(in float n) { return fract(_sinf(n) * 43758.5453123f); }

// polynomial smooth _fminf (k = 0.1f) - iq;
__DEVICE__ float smin(in float a, in float b, in float k ){
    float h = clamp(0.5f+0.5f*(b-a)/k, 0.0f, 1.0f);
    return _mix(b,a,h) - k*h*(1.0f-h);
}

__DEVICE__ float Noise(in float2 x) {
    float2 p=_floor(x), f=fract_f2(x);
    f *= f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f;
    return _mix(_mix(hash(n+ 0.0f), hash(n+ 1.0f),f.x),
                _mix(hash(n+57.0f), hash(n+58.0f),f.x),f.y);

}

__DEVICE__ mat3 rotAA(in float3 v, in float angle){//axis angle rotation
  float   c=_cosf(angle);
  float3  s=v*_sinf(angle);
  //return to_mat3(swi3(v,x,x,x)*v,swi3(v,y,y,y)*v,swi3(v,z,z,z)*v)*(1.0f-c)+
  //       to_mat3(c,-s.z,s.y,s.z,c,-s.x,-s.y,s.x,c);
  return add_mat3(to_mat3_f3(swi3(v,x,x,x)*v*(1.0f-c),swi3(v,y,y,y)*v*(1.0f-c),swi3(v,z,z,z)*v*(1.0f-c)) ,
                  to_mat3(c,-s.z,s.y,s.z,c,-s.x,-s.y,s.x,c));
}

// -----------------------------------------------------------------------------------

__DEVICE__ mat3 rmx,mrot;

// - Shapes & distances --------------------------------------------------------------

__DEVICE__ float2 opU(float2 d1, float2 d2 ) {
  return (d1.x<d2.x) ? d1 : d2;
}

#define MAX_DIST  18.0f

__DEVICE__ float sdSubTree0(in float3 p0) {
  float obj = 0.0f;
  //swi2(p,x,z) = mod_f(swi2(p,x,z), 4.0f) - 2.0f;
  float text, dr=0.74074f, dd, d0 = MAX_DIST, d = d0;
  
  float3 p = p0; //, pBest;
  float2 pt;
  float brBest;
  float k=1.0f;
    int ffEnd = fEnd;
    int ffStart = fStart;
    
  for (int n=ZERO; n<10; n++) {
    if (n>=bEnd) break;
    dd = (length(p-ba*clamp(dot(p,ba)*oobdb,0.0f,1.05f))-rBranch+p.y*thinBranch)*dr;
        d = (dd<d) ? smin(d,dd,0.00155f) : d;
    p.x = _fabs(p.x);
    p = mul_f3_mat3(p,rmx) + k*Julia;
    dr *= 0.74074f;
        ffEnd--;
        ffStart--;
    }
  dd = (length(p-ba*clamp(dot(p,ba)*oobdb,-0.1f,frc))-rBranch+p.y*thinBranch)*dr;
  d = (dd<d) ? smin(d,dd,dr*0.055f) : d;
  k = 0.5f;
   
    for (int n=ZERO; n<6; n++) {
        if (n>ffEnd) break;
    if (n>=ffStart) { // Leafs
      p += (n==ffStart) ? (0.08f+sin_f3(swi3(p,y,z,x)*15.0f)*0.37f) : to_float3_s(0.0f);
      dd = (length(p- ba2*f2*clamp((p.y*ba2.y)*oobd2,0.0f,3.0f))-0.2f)*dr;
      d = (dd<d) ? dd+0.0001f : d;
    }
    p.x = _fabs(p.x);
    p = mul_f3_mat3(p,rmx) + k*Julia;
    dr *= 0.74074f;
    }
    return d;
}
  
__DEVICE__ float sdTree0(in float3 p) {
  // Mix 2 tree to brake symetrie
  float scale =  _mix(life, 1.5f, clpOld);
  //p.z *= 0.8f;
  swi3S(p,x,y,z,swi3(p,x,y,z) / scale);//_mix(life, 1.5f, clpOld);
  p += to_float3(0.0f,0.06f,0.0f)+ sin_f3(swi3(p,y,z,x)*to_float3(5.0f,6.0f,3.0f))*0.017f;
  // first part
    float d1 = sdSubTree0(p);  
    // second part
  p.y /= 1.3f;
  swi3S(p,x,y,z, mul_f3_mat3(swi3(p,x,y,z) , mrot));
  float d2 = sdSubTree0(p);  
  
  d2 *= scale;
  d1 *= scale;
  return _fminf(d1,d2);
}


// first based on Orchard@Night by eiffie (https://www.shadertoy.com/view/4s23Rh)
__DEVICE__ float2 sdSubTree(in float3 p0) {
  float obj = 0.0f;
  //swi2(p,x,z) = mod_f(swi2(p,x,z), 4.0f) - 2.0f;
  float text, dr=0.74074f, dd, d0 = MAX_DIST, d = d0;
  
  float3 p = p0, pBest;
  float2 pt;
  float brBest;
  float k=1.0f;
  for (int n=ZERO; n<14; n++) {
        if (n>fEnd) break;
    if (n<bEnd) {
      dd = (length(p-ba*clamp(dot(p,ba)*oobdb,0.0f,1.05f))-rBranch+p.y*thinBranch)*dr;
      if (dd < d) {
        d = smin(d,dd,0.00155f);
        pBest = p;
      }
    }
    else if(n==bEnd) {
      dd = (length(p-ba*clamp(dot(p,ba)*oobdb,-0.1f,frc))-rBranch+p.y*thinBranch)*dr;
      if (dd < d) {
        d = smin(d,dd,dr*0.055f);
        pBest = p;
      }
      if (d<d0) {
        // TODO find best shade
        text = _cosf(pBest.y*50.0f+2.5f*hash(pBest.x*pBest.y));
        obj = 51.0f + text;
        //d+=text;//0.005f*Noise(swi2(pBest,x,y)*30.0f);
        //d0=d;
      }
      k = 0.5f;
    }  
    else if (n>=fStart && n<=fEnd) { // Leafs
      if(n==fStart) { // Debut feuille
        p +=0.08f+sin_f3(swi3(p,y,z,x)*15.0f)*0.37f;
      //  p.z*=0.6f + 0.4f*_sinf(p.y*0.1f);
      } 
 
      dd = (length(p- ba2*f2*clamp(p.y*ba2.y*oobd2,0.0f,3.0f))-0.2f)*dr;
      if (dd<d) {
        d = dd+0.0001f;
        obj = float(n-fStart+2);
      }
    }
    p.x = _fabs(p.x);
    p = mul_f3_mat3(p,rmx) + k*Julia;
    dr *= 0.74074f;
    }
    return to_float2(d, obj);
}
  
__DEVICE__ float2 sdTree(in float3 p) {
  // Mix 2 tree to brake symetrie
  float scale =  _mix(life, 1.5f, clpOld);
  //p.z *= 0.8f;
  swi3S(p,x,y,z, swi3(p,x,y,z) / scale);//_mix(life, 1.5f, clpOld);
  p += to_float3(0.0f,0.06f,0.0f)+ sin_f3(swi3(p,y,z,x)*to_float3(5.0f,6.0f,3.0f))*0.017f;
  // first part
    float2 d1 = sdSubTree(p);  
    // second part
  p.y /= 1.3f;
  swi3S(p,x,y,z, mul_f3_mat3(swi3(p,x,y,z) , mrot));
  float2 d2 = sdSubTree(p);  
  
  d2.x *= scale;
  d1.x *= scale;
  float d = smin(d1.x,d2.x, 0.05f*clamp(3.0f-p.y,0.0f,10.0f));
  return to_float2(d, (d1.x<d2.x) ? d1.y : d2.y);
}

  
__DEVICE__ float sdLandscape(in float3 p) {
    // Artificials hills
  float h=(0.6f-0.6f*_cosf(p.x/1.5f)*_cosf(p.z/1.5f))+(season>0.75f?0.002f:0.006f)*Noise(swi2(p,z,x)*35.0f); // very regular patern + a little bit of noise
  return p.y+h-(season>0.75f?0.08f:0.0f);
}

//- Scene --------------------------------------------------------------------


__DEVICE__ float2 mapLandscape(in float3 p) {
  return to_float2(sdLandscape(p), 1.0f);
}

__DEVICE__ float2 mapTree(in float3 p) {
  float 
        a = _mix(0.045f,0.001f,clpOld)*p.y*_cosf(jGlobalTime*10.0f), // (life < OLD_TREE ? 22.0f : _mix(22.0f,1.0f,clamp(life- OLD_TREE,0.0f,1.0f)))); 
        c = _cosf(a),
        s = _sinf(a);
  swi2S(p,x,z,mul_f2_mat2(swi2(p,x,z) , to_mat2(c,-s,s,c)));
  p.y *= (1.05f+ _mix(0.04f*_cosf(jGlobalTime/4.0f*6.28f+0.8f), 0.0f, clpOld));
  return sdTree(p);
}
__DEVICE__ float mapTree0(in float3 p) {
  float 
        a = _mix(0.045f,0.001f,clpOld)*p.y*_cosf(jGlobalTime*10.0f), // (life < OLD_TREE ? 22.0f : _mix(22.0f,1.0f,clamp(life- OLD_TREE,0.0f,1.0f)))); 
        c = _cosf(a),
        s = _sinf(a);
  swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , to_mat2(c,-s,s,c)));
  p.y *= (1.05f+ _mix(0.04f*_cosf(jGlobalTime/4.0f*6.28f+0.8f), 0.0f, clpOld));
  return sdTree0(p);
}

__DEVICE__ float map0(in float3 p, bool tree, bool landscape) {
    float res = 1000.0f;
    if (tree)      res = _fminf(res, mapTree0(p));
    if (landscape) res = _fminf(res, sdLandscape(p));
  return res;
}

__DEVICE__ float2 map(in float3 p, bool tree, bool landscape) {
    float2 res = to_float2(1000,-1);
    if (tree)      res = opU(res, mapTree(p));
    if (landscape) res = opU(res, mapLandscape(p));
  return res;
}

//- Render --------------------------------------------------------------------------

__DEVICE__ float2 castRay(in float3 ro, in float3 rd, in float maxd) {
    bool withTree = intersectSphere(ro-to_float3(0.0f,2.6f,0.0f), rd, 2.7f);
    // TODO intersect plane for landscape
    
  float precis=0.004f, h=precis*2.0f, t=5.0f;
  float res;
    for( int i=ZERO; i<60; i++ ) {    
    if (_fabs(h) < precis && t > maxd) break; 
        t += h;
        res = map0(ro+rd*t, withTree, true);
        h = res;
    }
    return to_float2(t, (t<maxd) ? map(ro+rd*t, withTree, true).y : -1.0f);
}


//- Lighting -----------------------------------------------------------------------


// Rolling hills by Dave Hoskins (https://www.shadertoy.com/view/Xsf3zX)
// Grab all sky information for a given ray from camera
__DEVICE__ float3 GetSky(in float3 rd, in bool withSun) {
  float sunAmount = withSun ? _fmaxf( dot( rd, sunLight), 0.0f ) : 0.0f;
  return clamp( 
      _mix(to_float3(0.1f, 0.2f, 0.3f), to_float3(0.32f, 0.32f, 0.32f), _powf(1.0f-_fmaxf(rd.y,0.0f),6.0f)) +
      sunColour * sunAmount * sunAmount * 0.25f +
      SUN_COLOR * _fminf(_powf(sunAmount, 800.0f)*1.5f, 0.3f)
    , 0.0f, 1.0f);
}


// Merge grass into the sky background for correct fog colouring...
__DEVICE__ float3 ApplyFog(in float3  rgb, in float dis, in float3 sky){
  return _mix(rgb, sky, clamp(dis*dis*0.003f, 0.0f, 1.0f));
}

// iq
__DEVICE__ float softshadow(in float3 ro, in float3 rd) {
#ifdef FAST
  return 1.0f;
#else

  float res = 1.0f, h, t = 0.02f;
    for( int i=ZERO; i<16; i++ ) {
  //  if (t < maxt) {
    h = map0(ro + rd*t, true, true);
    res = _fminf( res, 1.0f*h/t );
    t += 0.3f;
  //  }
    }
    return clamp(res, 0.0f, 1.0f);
#endif  
}

__DEVICE__ const float2 eps = {0.001f, 0.0f};
/*
__DEVICE__ float3 calcNormal(in float3 p, in bool tree, in bool landscape) {
    float2 e = to_float2(eps.x, -eps.x); 
    return normalize(swi3(e,x,y,y) * map0(p + swi3(e,x,y,y), tree, landscape) +
                     swi3(e,y,y,x) * map0(p + swi3(e,y,y,x), tree, landscape) + 
                     swi3(e,y,x,y) * map0(p + swi3(e,y,x,y), tree, landscape) + 
                     swi3(e,x,x,x) * map0(p + swi3(e,x,x,x), tree, landscape));
}
*/
__DEVICE__ float3 calcNormal( float3 pos, float3 ray, float t,  in bool tree, in bool landscape, float2 iResolution) {
  //  float2 e = to_float2(eps.x, -eps.x); 
  //  return normalize(swi3(e,x,y,y) * map(pos + swi3(e,x,y,y)).x + swi3(e,y,y,x) * map(pos + swi3(e,y,y,x)).x + swi3(e,y,x,y) * map(pos + swi3(e,y,x,y)).x + swi3(e,x,x,x) * map(pos + swi3(e,x,x,x)).x);;

  float pitch = 0.2f * t / iResolution.x;
    
//#ifdef FAST
//  // don't sample smaller than the interpolation errors in Noise()
  pitch = _fmaxf( pitch, 0.005f );
//#endif
  
  float2 d = to_float2(-1,1) * pitch;

  float3 p0 = pos+swi3(d,x,x,x); // tetrahedral offsets
  float3 p1 = pos+swi3(d,x,y,y);
  float3 p2 = pos+swi3(d,y,x,y);
  float3 p3 = pos+swi3(d,y,y,x);
  
  float f0 =  map0(p0, tree, landscape);
  float f1 = map0(p1, tree, landscape);
  float f2 = map0(p2, tree, landscape);
  float f3 = map0(p3, tree, landscape);
  
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
  //return normalize(grad);  // prevent normals pointing away from camera (caused by precision errors)
  return normalize(grad - _fmaxf(0.0f,dot (grad,ray ))*ray);
}



__DEVICE__ float calcAO(in float3 pos, in float3 nor) {
#ifdef FAST  
  return 1.0f;
#else
  float dd, hr=0.01f, totao=0.0f, sca=1.0f;  
    for(int aoi=ZERO; aoi<5; aoi++ ) {
        dd = map(nor * hr + pos, true, true).x;
        totao += -(dd-hr)*sca;
        sca *= 0.75f;
        hr += 0.05f;
    }
    return clamp(1.0f-4.0f*totao, 0.0f, 1.0f);
#endif
}

__DEVICE__ float3 render(in float3 ro, in float3 rd, float2 iResolution) { 
    float3 col = to_float3_s(0.0f);
    float2 res = castRay(ro,rd,MAX_DIST);
    float t = res.x, m = res.y;
    float ao = 1.0f, sh = 1.0f;
    if( m>-0.5f ) {
        float3 nor, pos = ro + t*rd;
        

    if (m >= 2.0f && m<10.0f) {
            nor = calcNormal( pos, rd, t, true, false, iResolution);
            col = _mix(to_float3(0.35f*m,3.0f-0.08f*m,0.0f), to_float3(3.0f-0.08f*m,(m*0.35f),0.0f), season*1.3f); // Automne
    } else if (m<2.0f) {
            nor = calcNormal( pos, rd, t, false, true, iResolution);
            col = season>0.75f?to_float3_s(1):
            season>0.3f?to_float3(1.3f,0.9f,0.3f):
                        to_float3(0.3f,0.9f,0.3f);
    } else {
            nor = calcNormal( pos, rd, t, true, false, iResolution);
            col = to_float3_s(0.7f+0.3f*(m-50.5f));
    } 
  
    ao = calcAO(pos, nor);
        float 
        amb = 1.0f,//clamp(0.5f+0.5f*nor.y, 0.0f, 1.0f),
            dif = clamp(dot( nor, sunLight ), 0.0f, 1.0f),
            bac = clamp(dot( nor, normalize((sunLight))), 0.0f, 1.0f);//*clamp( 1.0f-pos.y,0.0f,1.0f);

  //  if( dif>0.02f ) {
    sh = 4.0f*softshadow( pos, sunLight); 
    dif *= sh;
  //}

    float3 brdf = 0.1f*ao*(amb*to_float3(0.10f,0.11f,0.13f) + bac*0.15f) + 1.2f*dif;

    float 
        pp = clamp(dot(reflect(rd,nor), sunLight),0.0f,1.0f),
        spe = sh*_powf(pp,16.0f),
        fre = ao*_powf( clamp(1.0f+dot(nor,rd),0.0f,1.0f), 2.0f);

    col = col*brdf + col*spe + 0.5f*fre;//*(0.5f+0.5f*col);
      //  * _expf(-0.01f*t*t);

    // TODO calculate sun color from position and use it here
    if(spe>0.0f){
      col+=0.2f*sunColour*_powf(_fmaxf(0.0f,dot(reflect(rd,nor),sunLight)),8.0f)*spe;
    }
    #ifdef MIE_RAYLEIGH_SKY
      float3 gp = to_float3(ro.x, earth.rs + 3500.0f, ro.z);
      col = ApplyFog(col, res.x, compute_sky_light(gp, rd, earth, sun));
    #else
      col = ApplyFog(col, res.x, GetSky(rd, true));
    #endif
    //    DoLighting(col, pos, nor, rd, res.x);

  } else {
    #ifdef MIE_RAYLEIGH_SKY
      float3 gp = to_float3(ro.x, earth.rs + 3500.0f, ro.z);
      col = compute_sky_light(gp, rd, earth, sun);
    #else
      col = GetSky(rd, true);
    #endif
    col = ApplyFog( col, res.x, col);
  }
  
  return (clamp(col,0.0f,1.0f) );
}


//*********************************************************************************
//    +----------------------------------------------------------------+
//    |   Position of the sun in the sky at a given location and time  |
// +----------------------------------------------------------------+
// Based on LOW-Precision formulae for planetary positions by T.C. Van Flandern and H-F. Pulkkinen
// http://articles.adsabs.harvard.edu/cgi-bin/nph-iarticle_query?1979ApJS...41..391V&defaultprint=YES&filetype=.pdf

__DEVICE__ float julianDay2000(in int yr, in int mn, in int day, in int hr, in int m, in int s) {
  int im = (mn-14)/12, 
  ijulian = day - 32075 + 1461*(yr+4800+im)/4 + 367*(mn-2-im*12)/12 - 3*((yr+4900+im)/100)/4;
  float f = (float)(ijulian)-2451545.0f;
  return f - 0.5f + (float)(hr)/24.0f + (float)(m)/1440.0f + (float)(s)/86400.0f;
}

__DEVICE__ float julianDay2000(in float unixTimeMs) {
  return (unixTimeMs/86400.0f) - 10957.5f;// = + 2440587.5f-2451545; 
}

__DEVICE__ float2 SunAtTime(in float julianDay2000, in float latitude, in float longitude) {
  float zs,rightAscention, declination, sundist,
    t  = julianDay2000,  //= jd - 2451545.0f, // nb julian days since 01/01/2000 (1 January 2000 = 2451545 Julian Days)
    t0 = t/36525.0f,         // nb julian centuries since 2000      
    t1 = t0+1.0f,           // nb julian centuries since 1900
    Ls = fract(0.779072f+0.00273790931f*t)*PI2, // mean longitude of sun
    Ms = fract(0.993126f+0.0027377785f *t)*PI2, // mean anomaly of sun
    GMST = 280.46061837f + 360.98564736629f*t + (0.000387933f - t0/38710000.0f)*t0*t0, // Greenwich Mean Sidereal Time   
// position of sun
    v = (0.39785f-0.00021f*t1)*_sinf(Ls)-0.01f*_sinf(Ls-Ms)+0.00333f*_sinf(Ls+Ms),
    u = 1.0f-0.03349f*_cosf(Ms)-0.00014f*_cosf(2.0f*Ls)+0.00008f*_cosf(Ls),
    w = -0.0001f-0.04129f * _sinf(2.0f*Ls)+(0.03211f-0.00008f*t1)*_sinf(Ms)
        +0.00104f*_sinf(2.0f*Ls-Ms)-0.00035f*_sinf(2.0f*Ls+Ms);
// calcul distance of sun
  sundist = 1.00021f*_sqrtf(u)*AU;
// calcul right ascention
  zs = w / _sqrtf(u-v*v);
  rightAscention = Ls + _atan2f(zs/_sqrtf(1.0f-zs*zs), 1.0f);
// calcul declination
  zs = v / _sqrtf(u);
  declination = _atan2f(zs/_sqrtf(1.0f-zs*zs), 1.0f);
// position relative to geographic location
  float
    sin_dec = _sinf(declination),   cos_dec = _cosf(declination),
    sin_lat = _sinf(TORAD*latitude),cos_lat = _cosf(TORAD*latitude),
    lmst = mod_f((GMST + longitude)/15.0f, 24.0f);
  if (lmst<0.0f) lmst += 24.0f;
  lmst = TORAD*lmst*15.0f;
  float
    ha = lmst - rightAscention,       
    elevation = asin(sin_lat * sin_dec + cos_lat * cos_dec * _cosf(ha)),
    azimuth   = _acosf((sin_dec - (sin_lat*_sinf(elevation))) / (cos_lat*_cosf(elevation)));
  return to_float2(_sinf(ha)>0.? azimuth:PI2-azimuth, elevation);
}

__DEVICE__ float3 getSunVector(in float jd, in float latitude, in float longitude) {
  float2 ae = SunAtTime(jd, latitude, longitude);
  // X = north, Y = top
  return normalize(to_float3(-_cosf(ae.x)*_cosf(ae.y), _sinf(ae.y), _sinf(ae.x)*_cosf(ae.y)));
}


__KERNEL__ void LifeOfTreeJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

   rmx =  mul_mat3_f(rotAA(normalize(to_float3(0.2174f,1,0.02174f)),1.62729f),1.35f);
   mrot = rotAA(normalize(to_float3(0.1f,0.95f,0.02f)), 2.0f);


    oobdb = 1.0f/dot(ba,ba);
    oobd2 = 1.0f/dot(ba2,ba2);



// Picture params -------------------------------------------------------------------------
  float2 q = fragCoord/iResolution;
  float2 p = -1.0f+2.0f*q;
  p.x *= iResolution.x/iResolution.y;
  float2 mo = swi2(iMouse,x,y)/iResolution;

// - Camera  -------------------------------------------------------------------------------
    float a0=1.5f+iTime/12.0f, r=2.5f;
    float3 
        ro = to_float3(-9.0f+r*_cosf(a0+PI2*mo.x), 
                       _fmaxf(0.75f,1.5f+0.6f*mo.y),
                       5.5f+r*_sinf(a0+PI2*mo.x)),
        ta = to_float3(1.6f, 1.9f, -3.5f ),
        // Camera tx
        cw = normalize(ta-ro),
        cu = cross(cw,to_float3(0.0f,1.0f,0.0f)),
        cv = cross(cu,cw),
        rd = normalize(p.x*cu + p.y*cv + 2.5f*cw);
      
// - Time / Season / Position of sun according to location and time ----------------------- 
#ifdef BENJAMIN_BUTTON
    jGlobalTime = 20000.0f-iTime; // For Benjamin button tree, inverse the sign !
#else
  jGlobalTime = 1.0f+iTime*0.75f;    
#endif
  life =0.5f+mod_f(jGlobalTime,68.0f)/24.0f;
  season = mod_f(jGlobalTime, 4.0f)/4.0f;
  float 
        day = _floor(season*365.0f)+100.0f,
      jd2000 = julianDay2000((day*24.0f+UTC_HOUR)*3600.0f);
  
  float2 ae = SunAtTime(jd2000, LATITUDE, LONGITUDE);
  // X = north, Y = top
  sunLight = normalize(to_float3(-_cosf(ae.x)*_cosf(ae.y), _sinf(ae.y), _sinf(ae.x)*_cosf(ae.y)));

#ifdef MIE_RAYLEIGH_SKY
  sun.azi = ae.x;
  sun.alt = ae.y; 
#endif
  
  if (sunLight.y < 0.12f) {
      sunColour = _mix(to_float3(2.0f,0.4f,0.2f), SUN_COLOR, smoothstep(0.0f,1.0f, sunLight.y*8.0f));
  } else {
    sunColour = SUN_COLOR;
  }
 
// - Scene parameters according to season --------------------------------------------------
  treeStep = clamp(life*life,0.0f,1.0f);
  frc = fract(treeStep*10.0f);
  bEnd = int(treeStep*10.0f);
  f2 = _sinf(season*(3.14f/0.8f)); /*- 0.33f*_cosf(a*3.0f) + 0.2f*_cosf(a*5.0f)*/;
  f2 = treeStep*treeStep*clamp(2.0f*f2,0.0f,1.0f);
  f2 = _mix(f2, 0.0f, clamp(2.0f*(life-OLD_TREE),0.0f,1.0f));
#ifdef NO_LEAF
  fEnd = fStart = 20;
#else
  fStart = (season>.8||life<.6||life>1.5f*OLD_TREE) ? 20 : bEnd+1;//bEnd>6? bEnd+2:8; //1+int(3.0f*(_sinf(season*(3.14f/0.8f))));
  fEnd =  fStart + (bEnd<8 ? 1:3);
#endif
  thinBranch=0.018f;
  rBranch = _mix(0.01f,0.07f,treeStep);
  clpOld = clamp(life-OLD_TREE,0.0f,1.0f);
  clpVeryOld = clamp(life-VERY_OLD_TREE,0.0f,1.0f);
    
// - Render ------------------------------------------------------------------------------
    float3 col = render(ro, rd, iResolution);
    
// - Post prod ---------------------------------------------------------------------------    
  col = (sqrt_f3(col)
          + sunColour*_mix(0.0f, 0.051f, sunLight.y<0.05f ? 1.0f-sunLight.y*20.0f : 0.0f) // extra light at sunshine
    )
      * _mix(1.2f, 5.0f, 3.0f*clpVeryOld) // I see the light !
      * _powf(_fabs(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y)), _mix(0.2f, 0.25f, clamp(life-VERY_OLD_TREE-0.3f,0.0f,1.0f))); // vigneting
        
    fragColor=to_float4_aw( col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}