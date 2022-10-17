
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'https://soundcloud.com/josephraphael/yann-tiersen-amelie-piano-version-soundtrack?in=user-624915920/sets/yan-tiersen' to iChannel0


// The Greatest Adventure by Martijn Steinrucken aka BigWings - 2016
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
// Song: Yann Tiersen – Comptine d’un autre été - L’après midi
// https://soundcloud.com/reminiscience/yann-tiersen-comptine-dun-autre-ete-lapres-midi 
//
// My entry for the 2017 ShaderToy competition
//
// I thought I had one more day, luckily I found out in time that I didn't :) Pulled an all-nighter to get this done in time.
// I didn't have time to optimized and beautify so forgive me for the horrible code.
// 
// I figured the greatest adventure there is is life itself. 
//
// https://www.shadertoy.com/view/MsSBD1

#define INVERTMOUSE 1.0f

#define MAX_STEPS 200
#define MIN_DISTANCE 0.1f
#define MAX_DISTANCE 4.0f
#define RAY_PRECISION 0.0003f

#define BG_STEPS 20.0f

#define S(x,y,z) smoothstep(x,y,z)
#define L(x, y, z) clamp((z-x)/(y-x), 0.0f, 1.0f)
#define B(x,y,z,w) S(x-z, x+z, w)*S(y+z, y-z, w)
#define sat(x) clamp(x,0.0f,1.0f)
#define SIN(x) _sinf(x)*0.5f+.5

#define BOKEH_SIZE 0.03f

#define FLAMECOL to_float3(0.99f, 0.6f, 0.35f)
#define FLAMEBLUE to_float3(0.1f, 0.1f, 1.0f)
#define CANDLECOL  to_float3(0.2f, 0.5f, 0.2f)
#define CANDLE_HEIGHT 10.0f
#define SONG_LENGTH 134.0f
#define DEATH_TIME 126.3f

#define halfpi 1.570796326794896619f
#define pi     3.141592653589793238f
#define twopi  6.283185307179586f

//float2 m; // mouse

//float3 A;  // 0=birth  1=death  x = 0->1 y = 1->0 z = smoothstep(x)


//float gHeight;

__DEVICE__ float X2(float _x) {return _x*_x;}
__DEVICE__ float N( float _x ) { return fract(_sinf(_x*12.35462f)*5346.1764f); }
__DEVICE__ float N2(float _x, float _y) { return N(_x + _y*23414.324f); }

__DEVICE__ float N21(float2 t) {return fract(_sinf((t.x+t.y*10.0f)*9e2));}
__DEVICE__ float N31(float3 t) {return fract(_sinf((t.x+t.y*10.0f+ t.z*100.0f)*9e2));}
__DEVICE__ float4 N14(float t) {return fract_f4(sin_f4(to_float4(1.0f, 3.0f, 5.0f, 7.0f)*9e2));}

__DEVICE__ float LN(float _x) {return _mix(N(_floor(_x)), N(_floor(_x+1.0f)), fract(_x));}

struct ray {
    float3 o;
    float3 d;
};



struct de {
    // data type used to pass the various bits of information used to shade a de object
    float a;// age
    float d;  // distance to the object
    float fh;  // flame height
    float m;   // material
    float f;  // flame
    float w;  // distance to wick
    float fd;  // distance to flame
    float b;  // bump  
    
    float s; // closest flame pass
    float sd;
    float2 uv;
    
    // params that only get calculate when marcing is complete
    float3 id;  // cell id
    float4 n;   // cell id based random values
    float3 p;  // the world-space coordinate of the fragment
    float3 nor;  // the world-space normal of the fragment  
};
    
struct rc {
    // data type used to handle a repeated coordinate
    float3 id;  // holds the floor'ed coordinate of each cell. Used to identify the cell.
    float3 h;    // half of the size of the cell
    float3 p;    // the repeated coordinate
};


__DEVICE__ ray GetRay(float2 uv, float3 p, float3 lookAt, float zoom) {
  
    float3 f = normalize(lookAt-p),
       r = cross(to_float3(0,1,0), f),
       u = cross(f, r),
       c = p+f*zoom,
       i = c+r*uv.x+u*uv.y;  // point in 3d space where cam ray intertsects screen
    
    ray cr;
float zzzzzzzzzzzzzzzzzzzzzzz;    
    cr.o = p;            
    cr.d = normalize(i-p);    // ray dir is vector from cam pos to screen intersect 
  return cr;
}

__DEVICE__ float remap01(float a, float b, float t) { return (t-a)/(b-a); }
__DEVICE__ float remap(float a, float b, float c, float d, float t) { return sat((b-a)/(t-a)) * (d-c) +c; }

__DEVICE__ float DistLine(float3 ro, float3 rd, float3 p) {
  return length(cross(p-ro, rd));
}

__DEVICE__ float2 within(float2 uv, float4 rect) {
  return (uv-swi2(rect,x,y))/swi2(rect,z,w);
}

// DE functions from IQ
// https://www.shadertoy.com/view/Xds3zN

__DEVICE__ float2 smin( float a, float b, float k )
{
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return to_float2(_mix( b, a, h ) - k*h*(1.0f-h), h);
}
__DEVICE__ float2 smin2( float a, float b, float k, float z )
{
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    float g = h*h*(3.0f-2.0f*h);
    return to_float2(_mix( b, a, g ) - k*_powf(g*(1.0f-g), z), g);
}

__DEVICE__ float2 smax( float a, float b, float k )
{
  float h = clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return to_float2(_mix( a, b, h ) + k*h*(1.0f-h), h);
}

__DEVICE__ float sdSphere( float3 p, float3 pos, float s ) { return length(p-pos)-s; }

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r ) {
    float3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h ) - r;
}

__DEVICE__ float sdCylinder( float3 p, float2 h ) {
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - h;
  return _fminf(_fmaxf(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}

__DEVICE__ float sdCone( float3 p, float3 c ) {
    float2 q = to_float2( length(swi2(p,x,z)), p.y );
    float d1 = -q.y-c.z;
    float d2 = _fmaxf( dot(q,swi2(c,x,y)), q.y);
    return length(_fmaxf(to_float2(d1,d2),to_float2_s(0.0f))) + _fminf(max(d1,d2), 0.0f);
}

__DEVICE__ float3 Bend( float3 p, float center, float strength ) {
    p.y-=center;
    float c = _cosf(strength*p.y);
    float s = _sinf(strength*p.y);
    mat2  m = to_mat2(c,-s,s,c);
    float3  q = to_float3_aw(mul_mat2_f2(m,swi2(p,x,y)),p.z);
    q.y+=center;
    return q;
}

__DEVICE__ float pModPolar(inout float2 *p, float repetitions) {
  float angle = 2.0f*pi/repetitions;
  float a = _atan2f((*p).y, (*p).x) + angle/2.0f;
  float r = length(*p);
  float c = _floor(a/angle);
  a = mod_f(a,angle) - angle/2.0f;
  *p = to_float2(_cosf(a), _sinf(a))*r;
  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
  return c;
}

__DEVICE__ de fmap( float3 p, de n, float3 A, float iTime, float gHeight ) {
    // mapping the flame
     
    float t = iTime;
     
    float y = sat(remap01(0.0f, CANDLE_HEIGHT, p.y));
    float h = 1.0f-y;
    float a = _atan2f(p.x, p.z);
    float l = length(swi2(p,x,z));
    
    float baby = S(0.025f, 0.0f, h);
    float child = S(0.3f, 0.05f, h);         
    float adult = 1.0f-child;
    float adult2 = S(0.2f, 0.5f, A.x);
    float death = S(0.8f, 1.0f, A.z);
    
    float height = gHeight+0.055f+baby*0.05f-adult2*0.04f-adult*0.02f+death*0.02f;
   
    de o;
    o.m = 1.0f;
    
    p.z *= 1.5f;
    
    float size = 0.04f-baby*0.01f+adult2*0.04f-death*0.02f;
    
    float flame = MAX_DISTANCE;
    
    float flameHeight = 0.2f-0.1f*baby+adult2*0.1f-death*0.2f;
   
    float smth = 0.02f-baby*0.01f+adult2*0.03f-death*0.03f;
    
    float x = adult*0.01f-adult2*0.03f+death*0.04f;
    float wave = _sinf(p.y*10.0f-t*4.0f);
    wave *= _sinf(t);
    wave *= 0.01f;
    
    float baseSize = 0.7f+flameHeight*0.4f;
    float start = S(0.01f, 0.0f, A.x);
    baseSize *= 1.0f-start*0.5f;
    flameHeight *= 1.0f-start;
    
    float flicker = LN(t+A.x*t);
    flameHeight *= 1.0f-flicker*flicker*flicker*A.x*0.5f;
    
    for(float i=0.0f; i<1.0f; i+=1.0f/10.0f) {
        float fH = height + i*flameHeight;
        size = baseSize * _mix(0.05f, 0.01f, i);
        
        float fX = x+wave*i*i;
        flame = smin(flame, sdSphere(p, to_float3(fX, fH, 0.0f), size), smth).x;
    }
    
    o.fh = flameHeight;
    
    o.d = flame/1.5f;
    return o;
}

__DEVICE__ float4 map( float3 p, float3 A, float gHeight ) {
   
    float y = sat(remap01(0.0f, CANDLE_HEIGHT, p.y));
    float h = 1.0f-y;
    float a = _atan2f(p.x, p.z);
    float l = length(swi2(p,x,z));
    
    float cRadius = _mix(0.3f, 0.1f, S(0.0f, 1.0f, y)); // overall growth in thickness
    float baby = S(0.025f, 0.0f, h);
    float child = S(0.3f, 0.05f, h);         
    float adult = 1.0f-child;
    float adult2 = S(0.2f, 0.5f, A.x);
    float death = S(0.8f, 1.0f, A.z);
    
    cRadius *= 1.0f-child*0.6f;
 
    float inside = l/cRadius;
    float oMask = S(0.8f, 0.9f, inside);
    
    //candle
    float candle = sdCylinder(p, to_float2(cRadius, gHeight));
    if(baby>0.0f) {
      float topCone = sdCone(p-to_float3(0,gHeight+0.075f,0), to_float3(1,cRadius*12.6f,0.075f));
      candle = _fminf(candle, topCone);
    }
    
    float d = candle-0.003f*baby;
    
    float chamfer = 0.06f*adult2;//0.1f*(1.0f-baby);
    float crRadius = cRadius*(0.6f+adult2*0.3f)+ (1.0f-baby)*0.015f;
    float crHeight = gHeight+0.03f*child-0.05f*adult2+0.07f*baby;
    crHeight = gHeight+0.05f+baby*0.06f-adult*0.02f-0.02f*adult2;
    float crBlend = 0.001f+adult2*0.02f+0.02f*adult;
    float crater = sdCylinder(p-to_float3(0, crHeight+chamfer, 0), to_float2(crRadius-chamfer, 0.05f))-chamfer;
    d = smax(d, -crater, crBlend).x;

    // drips
    float id = _floor(a*40.0f/twopi);
    float n = N(id);
    float age = n*n-y;
    
    float drips = -_fabs(_sinf(a*20.0f)*_sinf(p.y*0.5f+n*pi))*_fmaxf(0.0f, age);
    drips = _fmaxf(0.0f, _sinf(a*20.0f));
    drips *= S(0.85f, 1.0f, inside);
    float bump = -_fmaxf(drips*0.7f*_sinf(p.y+n*twopi)-y, 0.0f);  // add drips
    
    // baby bday candle
    float x = a+p.y*10.0f;
    float bday = _powf(_fabs(_sinf(x*4.0f)),8.0f);
    float coneMask = S(0.9755f, 0.96f, y);

    bday = bday*oMask*child*coneMask;
    d -= bday*0.005f;
    
    // candle getting fucked up toward the bottom
    float3 q = p*25.0f;
    bump += _sinf(p.y)*(_sinf(q.x+_sinf(q.y+_sinf(q.z*pi)))+_sinf(q.x*2.0f+q.z+q.y))*0.5f*S(0.3f, 0.1f, y);
    bump += _sinf(sin(p.y*15.0f)+a*10.0f)*S(0.11f, 0.0f, y)*2.0f;
    d += bump*0.01f*oMask;
    
    // floor
     d = smin(d, p.y, 0.3f).x;  
    
    // wick
    float wBottom = crHeight-0.05f-0.01f*adult2;
    float wHeight = 0.08f-0.01f*death-0.02f*child-baby*0.02f;
    float wDiam = 0.008f+adult2*0.003f-0.002f*child;
    float wBlend = (0.04f+0.05f*adult)*(1.0f-baby)+0.1f*adult2;
    wBlend = (1.0f-baby)*0.02f+0.05f*adult2-0.05f*death;
    float wBend = 10.1f*adult2*(1.0f-death)-5.0f*adult;
    float3 wPos = Bend(p, wBottom, wBend);
    float wick = sdCapsule(wPos, to_float3(0, wBottom, 0), to_float3(0, wBottom+wHeight, 0), wDiam);
    
    float3 wUv = wPos*80.0f*CANDLE_HEIGHT;
    float wBump = _sinf(wUv.y*0.2f)*_sinf(wUv.z)*_sinf(wUv.x);
    wBump *= sat(remap01(wBottom, wBottom+wHeight, p.y));
    wBump *= 0.001f+0.001f*adult2;
    
    d = smin2(d, wick-wBump, wBlend, 1.0f+adult2*3.0f).x;
    
    // floor drips
    float2 par = swi2(p,x,z);
    float j = pModPolar(&par, 4.0f);
    swi2S(p,x,z, par);
    
    n = N(j);
    float bottom = sdSphere(p, to_float3(1, 0, 0)*0.4f, 0.3f*(n+0.3f));
    bottom = smax(bottom, p.y-0.05f, 0.05f).x;
    d = smin(d, bottom, 0.02f).x;
    
    //d = candle;
    
    return to_float4(d, wick, wBottom+wHeight, bump+bday);
}

__DEVICE__ de castRay( ray r, float3 A, float iTime, float gHeight ) {
    
    float t = iTime;
    float dS;
    
    de o;
    o.d = MIN_DISTANCE;
    o.w = MAX_DISTANCE;
    o.m = -1.0f;
    o.n = _mix(N14(_floor(t)), N14(_floor(t+1.0f)), fract(t));
    
    float4 d;
    for( int i=0; i<MAX_STEPS; i++ ) {
        o.p =  r.o+r.d*o.d;
 
        d = map(o.p,A,gHeight);
        
        o.w = _fminf(o.w, d.y);  // keep track of closest wick dist 
        if( d.x<RAY_PRECISION || o.d>MAX_DISTANCE ) break;
        
        o.d += d.x;
    }
    o.a = d.z;
    o.b = d.w;
    
    if(d.x<RAY_PRECISION) o.m = 1.0f;
    
    
    // marching the flame
    de res;
    
    o.s = 1000.0f;
    o.fd = 0.0f;
    float n = 1.0f;
    for( int i=0; i<MAX_STEPS; i++ )
    {
      res = fmap( r.o+r.d*o.fd, o, A, iTime, gHeight );
        o.fh = res.fh;
        if( res.d<RAY_PRECISION || o.fd>MAX_DISTANCE ) break;
        if(res.d<o.s) {
            o.s = res.d;
            o.sd = o.fd;
        }
        
        o.fd += res.d;
    }
    
    if(res.d<RAY_PRECISION)
        o.f=1.0f;
    
    return o;
}

__DEVICE__ float3 calcNormal( de o, float3 A,float gHeight )
{
  float3 eps = to_float3( 0.01f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(o.p+swi3(eps,x,y,y),A,gHeight).x - map(o.p-swi3(eps,x,y,y),A,gHeight).x,
      map(o.p+swi3(eps,y,x,y),A,gHeight).x - map(o.p-swi3(eps,y,x,y),A,gHeight).x,
      map(o.p+swi3(eps,y,y,x),A,gHeight).x - map(o.p-swi3(eps,y,y,x),A,gHeight).x );
  return normalize(nor);
}

__DEVICE__ float3 FlameNormal( float3 p, de n, float3 A, float iTime, float gHeight )
{
  
  float3 eps = to_float3( 0.001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      fmap(p+swi3(eps,x,y,y), n,A,iTime,gHeight).d - fmap(p-swi3(eps,x,y,y), n,A,iTime,gHeight).d,
      fmap(p+swi3(eps,y,x,y), n,A,iTime,gHeight).d - fmap(p-swi3(eps,y,x,y), n,A,iTime,gHeight).d,
      fmap(p+swi3(eps,y,y,x), n,A,iTime,gHeight).d - fmap(p-swi3(eps,y,y,x), n,A,iTime,gHeight).d );
  return normalize(nor);
}

__DEVICE__ float3 Background(ray r, float iTime) {
    
    float t = iTime;
    
    float _x = _atan2f(r.d.x, r.d.z); //????
    float _y = dot(to_float3(0,1,0), r.d); //??????
    
    float d = 2.1f;
        
    float2 size = to_float2_s(2);
    float2 h = size / 2.0f;
    
    float blur = 0.3f;
    
    float3 col = to_float3_s(0);
    
    for(float i=0.0f; i<BG_STEPS; i++) {
        float3 p = r.o + r.d*d;
                
        float2 id = _floor(swi2(p,x,z)/size);                // used to give a unique id to each cell
        float3 q = p;
        swi2S(q,x,z, mod_f2f2(swi2(p,x,z), size)-h);                // make grid of flames
        
        float3 cP = to_float3(0, N21(id)*CANDLE_HEIGHT, 0);
        
        float dRayFlame = DistLine(q, r.d, cP);            // closest ray, point dist
        
        float dOriFlame = d + length(cP-p);    // approximate distance from ray origin to flame 
        float bSize = dRayFlame/dOriFlame;
        float3 flame = FLAMECOL;
        flame *= S(BOKEH_SIZE, BOKEH_SIZE-BOKEH_SIZE*blur, bSize);
        
        flame *= 100.0f;
        flame /= (dOriFlame*dOriFlame);
        float flicker = LN(t+id.x*100.0f+id.y*345.0f);
        //flicker = _mix(0.3f, 1.0f, S(0.2f, 0.5f, flicker));
        flame *= 1.0f-flicker*flicker*0.7f;
        col += flame;
        
        // step to the next cell
        
        float2 rC = ((2.0f*step(to_float2_s(0.0f), swi2(r.d,x,z))-1.0f)*h-swi2(q,x,z))/swi2(r.d,x,z);    // ray to cell boundary
        float dC = _fminf(rC.x, rC.y)+0.01f;
        
        d += dC;
    }
   
    return col;
}


__DEVICE__ float3 render( float2 uv, ray cam, float3 A, float iTime, float gHeight  ) {
    
    //float t = iChannelTime[0];
    float t = iTime;
    
    float3 col = to_float3_s(0.0f);
    de o = castRay(cam,A,iTime, gHeight);
    
    float3 n = calcNormal(o,A, gHeight);
    
    float death2 = S(0.9999f, 0.998f, A.x);
    
    if(o.m==-1.0f)
        col = Background(cam,iTime)*S(0.25f, 0.45f, A.x)*S(1.0f, 0.8f, A.x);//*X2(A.x*A.y)*4.0f;
    else if(o.m==1.0f) {
      
      float y = sat(remap01(0.0f, CANDLE_HEIGHT, o.p.y));
      float h = 1.0f-y;
float rrrrrrrrrrrrrrrrrrrrrrrrrrrrr;    
      float baby = S(0.025f, 0.0f, h);
      float child = S(0.3f, 0.05f, h);         
      float adult = 1.0f-child;
      float adult2 = S(0.2f, 0.5f, A.x);
      float death = S(0.8f, 1.0f, A.z);
        
        float3 sssColor = _mix(_mix(to_float3_s(1), to_float3(1, 0, 0), o.b), CANDLECOL, adult);
        
        float cRadius = _mix(0.3f, 0.1f, S(0.0f, 1.0f, y));
        
        float l = length(swi2(o.p,x,z));
        float inside = l/cRadius;
        
        col = to_float3_s(dot(normalize(to_float3(1, 1,1)), n)*0.5f+0.5f);
    
        float height = _mix(CANDLE_HEIGHT, 0.1f, A.z);

        // add wick glow
        y = o.p.y-height;
        float wickH = y-o.p.x*0.4f-0.02f-baby*0.15f;
        float wickGlow = S(0.009f, 0.025f, wickH)*S(0.2f, 0.15f, inside);
        
        float3 fPos = to_float3(0,height,0);
        fPos.y += 0.2f;
        fPos -= o.p;
        float lambert = sat(dot(normalize(fPos), n))/dot(fPos, fPos);
        lambert *= S(1.0005f, 0.998f, A.x);
        lambert *= 1.0f-S(0.85f, 0.999f, A.x)*0.75f;
        lambert *= 0.1f;
        float3 light = FLAMECOL*lambert;

        col *= S(1.5f, 0.0f, length(swi2(o.p,x,z)));
        col *= 0.1f;
        col += light*(1.0f-wickGlow*0.7f);
        
        col += FLAMECOL*FLAMECOL*wickGlow*2.0f;
        
        float3 r = reflect(cam.d, n);
        float phong = sat(dot(normalize(fPos), r));
        phong = _powf(phong, 10.0f);
        phong *= S(1.5f, 0.8f, inside);
        phong *= S(1.0005f, 0.998f, A.x);
        col += phong*FLAMECOL*o.fh*2.0f;
float ppppppppppppppppppppppppp;        
        
        
        // add sss
        float sssHeight = _mix(0.4f, 1.0f, adult)*o.fh;
        float sss = S(-sssHeight, -0.0f, y);
        sss *= inside*S(0.3f, 0.1f, A.x)*2.0f+S(0.7f, 0.98f, inside);
        sss *= 1.0f+o.b*0.3f;
        sss *= S(1.0f, 0.9f, A.x);
        col += sss*sssColor;
        
        //col = to_float3(phong);
        
    }
    
     if(o.f>0.0f&&o.fd<o.d) {
      float3 p = cam.o+cam.d*o.fd;
         
      float _y = sat(remap01(0.0f, CANDLE_HEIGHT, p.y));
      float h = 1.0f-_y;
    
      float baby = S(0.025f, 0.0f, h);
      float child = S(0.3f, 0.05f, h);         
      float adult = 1.0f-child;
      float adult2 = S(0.2f, 0.5f, A.x);
      float death = S(0.8f, 1.0f, A.z);
        
         
        float3 n = FlameNormal(p, o, A, iTime, gHeight);
        float fresnel = sat(dot(n, -cam.d));
        float flame = 1.0f;//fresnel;
        
        _y = p.y-gHeight;
       
        float topFade = S(o.fh, o.fh*0.5f, _y-baby*0.13f);
        float bottomFade = S(0.0f, 0.1f, _y);
        float wickFade = S(0.0f, 0.03f, o.w)*0.8f +0.2f;
         
         float spikes = _sinf(uv.x*200.0f+t*15.0f)*0.25f;
         spikes += _sinf(uv.x*324.0f-t*5.0f)*0.5f;
         spikes += _sinf(uv.x*145.0f+t);
         spikes *= S(0.1f, 0.6f, _y);
         spikes = 1.0f-spikes;
         
         col = _mix(col, FLAMECOL*3.0f, bottomFade*wickFade*fresnel*topFade*death2*spikes);
        
         float blue = S(0.4f, -0.0f, _y);
         blue *= fresnel;//S(0.7f, 0.3f, fresnel);
         
         topFade = S(0.1f, 0.0f, _y);
         bottomFade = S(-0.04f, 0.01f, _y);
         wickFade = _mix(wickFade, 1.0f, 1.0f-bottomFade);
         col += FLAMEBLUE*blue*bottomFade*topFade*wickFade*death2;
        
         //col = to_float3(spikes);
    }
    
    
    // add some glow
    float gw = S(0.0f, 0.3f, A.x);
    gw *= S(1.0f, 0.8f, A.x);
    float glow = S(0.1f*gw, 0.0f, o.s)*0.15f*gw;
    glow*=death2;
    col += glow*FLAMECOL;
    
    return col;
}

__KERNEL__ void ThegreatestadventureJipi042Fuse(float4 o, float2 uv, float iTime, float2 iResolution, float4 iMouse, float iChannelTime[])
{

    float t = fract(iTime/SONG_LENGTH)*SONG_LENGTH;
    
    //uv = (2.0f*uv - (swi2(o,x,y)=iResolution) ) / o.y ;    // -1 <> 1
    
    o.x=iResolution.x;
    o.y=iResolution.y;
    uv = (2.0f*uv - swi2(o,x,y)) / o.y ;    // -1 <> 1
    
    
    float2 m = swi2(iMouse,x,y)/iResolution;          // 0 <> 1
    
    float t2 = sat(t/DEATH_TIME);
    float3 A = to_float3(t2, 1.0f-t2, S(0.0f, 1.0f, t2));
    //A = to_float3(m.y);
    
    float gHeight = _mix(CANDLE_HEIGHT, 0.1f, A.z);
    
    float turn = (0.1f-m.x)*twopi+t*0.06f;
    float s = _sinf(turn);
    float c = _cosf(turn);
    mat3 rotX = to_mat3(    c,  0.0f, s,
                         0.0f, 1.0f, 0.0f,
                            s,  0.0f, -c);
    
    float y = _mix(CANDLE_HEIGHT+0.1f, 0.1f, A.z);
    //y-=0.1f;
    float3 lookAt = to_float3(0.0f, y, 0.0f);
    //lookAt.y += S(0.2f, 0.0f, A.x)*0.1f;
    
    y += A.x*A.x*A.x*A.z*0.5f;          // raise cam towards the end
    
    
    float dist = 0.5f+A.z+S(0.3f, 0.0f, A.x)*0.25f;
   // dist = 0.45f; //y-=0.2f;
    float3 pos = mul_f3_mat3(to_float3(0.0f, y, -dist) , rotX);
     
    ray r = GetRay(uv, pos, lookAt, 2.0f);

    float3 col = render(uv, r, A, iTime, gHeight);
    
    o = to_float4_aw(col, 1.0f);
    
    o *= S(0.0f, 1.0f, t)*S(SONG_LENGTH, 130.0f, t);  // fade in/out

  SetFragmentShaderComputedColor(o);
}