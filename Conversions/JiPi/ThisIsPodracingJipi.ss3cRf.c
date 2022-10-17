
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0
// Connect Image 'Texture: Organic 4' to iChannel1
// Connect Image 'Texture: Gray Noise Medium' to iChannel2
// Connect Image 'Texture: TexturBlöcke' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define S smoothstep
//#define AA 2
//#define T iTime*4.0f
#define PI 3.1415926535897932384626433832795f
#define TAU 6.283185f



__DEVICE__ mat2 Rot(float a) {
    float s = _sinf(a);
    float c = _cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float smin( float a, float b, float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}
__DEVICE__ mat3 rotationMatrixY (float theta)
{
    float c = _cosf (theta);
    float s = _sinf (theta);
    return to_mat3_f3(
                      to_float3(c, 0, s),
                      to_float3(0, 1, 0),
                      to_float3(-s, 0, c)
                     );
}
__DEVICE__ mat3 rotationMatrixX(float theta){
  float c = _cosf (theta);
  float s = _sinf (theta);
  return to_mat3_f3(
                to_float3(1, 0, 0),
                to_float3(0, c, -s),
                to_float3(0, s, c)
                );
}
__DEVICE__ mat3 rotationMatrixZ(float theta){
  float c = _cosf (theta);
  float s = _sinf (theta);
  return to_mat3_f3(
              to_float3(c, -s, 0),
              to_float3(s, c, 0),
              to_float3(0, 0, 1)
            );
}
__DEVICE__ float3 rotateX (float3 p, float theta)
{
  return mul_mat3_f3(rotationMatrixX(theta) , p);
}
__DEVICE__ float3 rotateY (float3 p, float theta)
{
  return mul_f3_mat3(p,rotationMatrixY(theta)); 
}
__DEVICE__ float3 rotateZ (float3 p, float theta)
{
  return mul_f3_mat3(p,rotationMatrixZ(theta)); 
}

__DEVICE__ float rounding( in float d, in float h )
{
  return d - h;
}


__DEVICE__ float opUnion( float d1, float d2 )
{
    return _fminf(d1,d2);
}

__DEVICE__ float opSmoothUnion( float d1, float d2, float k )
{
    float h = _fmaxf(k-_fabs(d1-d2),0.0f);
    return _fminf(d1, d2) - h*h*0.25f/k;
}

__DEVICE__ float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); 
}
// ================================
// SDF
// ================================
__DEVICE__ float sdCircle( in float3 p, in float r )
{
  return length(p)-r;
}
__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}
__DEVICE__ float sdCappedCylinder( float3 p, float h, float r )
{
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - to_float2(h,r);
  return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}
__DEVICE__ float sdCappedTorus(in float3 p, in float2 sc, in float ra, in float rb)
{
  p.x = _fabs(p.x);
  float k = (sc.y*p.x>sc.x*p.y) ? dot(swi2(p,x,y),sc) : length(swi2(p,x,y));
  return _sqrtf( dot(p,p) + ra*ra - 2.0f*ra*k ) - rb;
}
__DEVICE__ float ndot(float2 a, float2 b ) { return a.x*b.x - a.y*b.y; }
__DEVICE__ float sdRhombus(float3 p, float la, float lb, float h, float ra)
{
  p = abs_f3(p);
  float2 b = to_float2(la,lb);
  float f = clamp( (ndot(b,b-2.0f*swi2(p,x,z)))/dot(b,b), -1.0f, 1.0f );
  float2 q = to_float2(length(swi2(p,x,z)-0.5f*b*to_float2(1.0f-f,1.0f+f))*sign_f(p.x*b.y+p.z*b.x-b.x*b.y)-ra, p.y-h);
  return _fminf(_fmaxf(q.x,q.y),0.0f) + length(_fmaxf(q,to_float2_s(0.0f)));
}
__DEVICE__ float sdEllipsoid( float3 p, float3 r )
{
  float k0 = length(p/r);
  float k1 = length(p/(r*r));
  return k0*(k0-1.0f)/k1;
}
__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}
__DEVICE__ float sdPlane( float3 p, float3 n, float h )
{
  // n must be normalized
  return dot(p,n) + h;
}


// ================================
// FBM
// ===============================

__DEVICE__ float noise( in float3 x, __TEXTURE2D__ iChannel2 )
{
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f*f*(3.0f-2.0f*f);
  float2 uv  = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z);
  float2 rg1 = swi2(texture( iChannel2, (uv+ to_float2(0.5f,0.5f))/256.0f),y,x);
  float2 rg2 = swi2(texture( iChannel2, (uv+ to_float2(1.5f,0.5f))/256.0f),y,x);
  float2 rg3 = swi2(texture( iChannel2, (uv+ to_float2(0.5f,1.5f))/256.0f),y,x);
  float2 rg4 = swi2(texture( iChannel2, (uv+ to_float2(1.5f,1.5f))/256.0f),y,x);
  float2 rg  = _mix( _mix(rg1,rg2,f.x), _mix(rg3,rg4,f.x), f.y );
  return _mix( rg.x, rg.y, f.z );
}



__DEVICE__ float fbm4( in float3 q, __TEXTURE2D__ iChannel2 )
{
  const mat3 m = to_mat3( 0.00f,  0.80f,  0.60f,
                         -0.80f,  0.36f, -0.48f,
                         -0.60f, -0.48f,  0.64f );
  
    float f  = 0.5000f*noise( q, iChannel2 ); q = mul_mat3_f3(m,q*2.02f);
          f += 0.2500f*noise( q, iChannel2 ); q = mul_mat3_f3(m,q*2.03f);
          f += 0.1250f*noise( q, iChannel2 ); q = mul_mat3_f3(m,q*2.01f);
          f += 0.0625f*noise( q, iChannel2 );
    return f;
}

__DEVICE__ float fbm( float2 p, __TEXTURE2D__ iChannel2 )
{
    const mat2 m2 = to_mat2(0.8f,-0.6f,0.6f,0.8f);
    float f = 0.0f;
    f += 0.5000f*texture( iChannel2, p/256.0f ).x; p = mul_mat2_f2(m2,p)*2.02f;
    f += 0.2500f*texture( iChannel2, p/256.0f ).x; p = mul_mat2_f2(m2,p)*2.03f;
    f += 0.1250f*texture( iChannel2, p/256.0f ).x; p = mul_mat2_f2(m2,p)*2.01f;
    f += 0.0625f*texture( iChannel2, p/256.0f ).x;
    return f/0.9375f;
}


// ================================
// SHIP
// ================================


__DEVICE__ float createReactor(float3 p, float rad, float len){

  p = to_float3(p.x, p.y, _fabs(p.z)-0.5f);
  float reactor1 = sdCappedCylinder(p, rad-0.02f, len);
  reactor1 = rounding(reactor1, 0.02f);
  float3 q = p;

  q += to_float3(rad *0.8f, 0.0f,0.0f);
  float feature1 = sdCappedCylinder(q, rad * 0.5f, len * 0.3f);
  reactor1 = opUnion(reactor1, feature1);
  q = p;
  q += to_float3(0.0f,-len,0.0f);
  float fire = sdCircle(q, 0.6f * rad);
  reactor1 = opUnion(reactor1, fire);
    
  return reactor1;
}

__DEVICE__ float2 shipMap(float3 pos, float rad, float len, float iTime, float frequency, float amplitudeR[3], float lenReactor){
  float material;
    
  pos = rotateZ(pos, _cosf(iTime/2.0f * frequency) * amplitudeR[0]);
  pos = rotateY(pos, 3.0f*amplitudeR[1]);
  pos = rotateX(pos, amplitudeR[2]);
    
  //Reactor
  float3 q = pos;
  q = rotateZ(q, PI * 0.5f);
  q = rotateX(q, PI * 0.5f);
  float reactor = createReactor(q, rad, lenReactor);

  //LinkBetweenReactors
  q = pos;
  float core = sdRhombus(q, 0.3f, 0.1f, 0.05f, 0.2f );
  float link = opSmoothUnion(reactor, core, 0.1f);
  if(_fabs(link-core)<0.001f)
    material = 1.0f;

  //Guns
  q = to_float3(_fabs(pos.x), pos.y - 0.05f, pos.z); 
  //q = rotateX(q, PI * 0.5f);
  float gun = sdCapsule(q, to_float3(0.1f,0.0f,-0.1f), to_float3(0.1f,0.0f,-0.4f), 0.01f);
  link = opUnion(gun, link);
  if(_fabs(link-gun)<0.001f)
    material = 1.0f;

 
  //Core 
  q = pos + to_float3(0.0f,0.0f,-0.5f);
  float core1 = sdEllipsoid(q, to_float3(0.2f,0.15f,0.8f));
  float d = opSmoothUnion(core1, link, 0.05f);
  if(_fabs(d-core1)<0.001f)
    material = 1.0f;//1.0f; eigentlich 2.0f, aber dann geht Textur verloren -> Umbau
  else if(_fabs(d-link)<0.001f)
    material = 1.0f;//1.0f; eigentlich 2.0f, aber dann geht Textur verloren -> Umbau;

  //Cockpit
  q = pos + to_float3(0.0f,-0.1f,-0.3f);
  float cockpit = sdEllipsoid(q, to_float3(0.1f,0.1f,0.2f));
  d = opUnion(cockpit, d);
  if(_fabs(d-cockpit)<0.001f){
    material = 3.0f;
  }
  return to_float2(d, material);
}
// ==================================================

//===============================
// TERRAIN
//=============================== 

__DEVICE__ float2 terrainMap(float3 pos){
    float hPlane = smoothstep(-0.5f, 0.5f,  0.2f * _sinf(pos.z* 2.0f) * _sinf(pos.x));
    float plane = sdPlane(pos, to_float3(0.0f,2.1f,0.0f),hPlane);
    //ROCKS
    float3 q = to_float3( mod_f(_fabs(pos.x),7.0f)-2.5f,pos.y,mod_f(_fabs(pos.z+3.0f),7.0f)-3.0f);
    float2 id = to_float2( _floor(pos.x/7.0f)-2.5f, _floor((pos.z+3.0f)/7.0f)-3.0f);
    float fid = id.x*121.1f + id.y*31.7f;
    float h   = 1.8f + 1.0f * _sinf(fid*21.7f);
    float wid = 1.0f + 0.8f * _sinf(fid*31.7f);
    float len = 1.0f + 0.8f * _sinf(fid*41.7f);
    h   = _fminf(max(h, 1.0f),2.2f);
    len = _fmaxf(len, 1.5f);
    wid = _fmaxf(wid, 1.5f);
    float ellip = sdEllipsoid(q, to_float3(wid,h,len));
    ellip -= 0.04f*smoothstep(-1.0f,1.0f,_sinf(5.0f*pos.x)+_cosf(5.0f*pos.y)+_sinf(5.0f*pos.z));

    //TORUS
    q = to_float3( mod_f(_fabs(pos.x+5.0f),14.0f)-5.0f,pos.y+0.1f,mod_f(_fabs(pos.z+3.0f),14.0f)-3.0f);
    float torus = sdCappedTorus(q, to_float2(1.0f,0), 1.5f, 0.35f);
    torus -= 0.05f*smoothstep(-1.0f,1.0f,_sinf(9.0f*pos.x)+_cosf(5.0f*pos.y)+_sinf(5.0f*pos.z));
    
    float d = opSmoothUnion(torus, ellip, 0.5f);
    d = opUnion(d, plane);
    
    float material=0.0f;
    if( _fabs(d) < 2.5f)  // Org:0.01f) //Vermeidung weiße Pixel am Rand der Blöcke
        material = 4.0f; 
    if(_fabs(d - plane) < 0.01f) 
        material = 5.0f;
    return to_float2(d, material);
}


//===============================

__DEVICE__ float2 path(in float z, float frequency, float pathpar[5]){ 
#ifdef ORG    
    //return to_float2(0);
    float a = _sinf(z * 0.1f);
    float b = _cosf(z * frequency/2.0f);
    return to_float2(a*1.5f - b*1.0f, b + a*1.5f); 
#endif    
    float a = _sinf(z * pathpar[0]);
    float b = _cosf(z * frequency/pathpar[1]);
    return to_float2(a*pathpar[2] - b*pathpar[3], b + a*pathpar[4]); 
}


__DEVICE__ float2 map(in float3 pos, float frequency, float radius, float lenReactor, float amplitudeR[3], float iTime, float3 shippos, float pathpar[5])
{
    float material=0.0f;

    float3 terrainPos = pos;
    swi2S(terrainPos,x,z, swi2(terrainPos,x,z) - path(pos.z, frequency, pathpar));
    float2 terrain = terrainMap(terrainPos);

    float3 p = pos;
    p.z += iTime + shippos.z;
    p.x -= path(pos.z, frequency, pathpar).x + shippos.x;
    p.y -= 0.3f + shippos.y;
                
    float2 ship = shipMap(p, radius, lenReactor, iTime, frequency, amplitudeR, lenReactor);

    float d = _fminf(ship.x, terrain.x);
    if(_fabs(d - ship.x) < 0.01f){
      material = ship.y;
    }
    else if(_fabs(d - terrain.x) < 0.01f){
      material = terrain.y;
    }
    return to_float2(d, material);
}


__DEVICE__ float2 RayMarch(float3 ro, float3 rd, out int *mat, float frequency, float radius, float lenReactor, float amplitudeR[3], float iTime, float3 shippos, float pathpar[5],
                           int MAX_STEPS, float MAX_DIST, float SURF_DIST) {
  float dO=0.0f;
    float dM=MAX_DIST;
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
      float2 res = map(p,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar);
      float dS = 0.75f*res.x;
      *mat = (int)(map(p,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar).y);
      if(dS<dM) dM = dS;
      dO += dS;
      if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    return to_float2(dO, dM);
}

__DEVICE__ float3 GetNormal(float3 p, float frequency, float radius, float lenReactor, float amplitudeR[3], float iTime, float3 shippos, float pathpar[5]) {
    //int mat = 0;
    float d = map(p,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar).x;
    float2 e = to_float2(0.001f, 0);
    float3 n = d - to_float3(
                            map(p-swi3(e,x,y,y),frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar).x,
                            map(p-swi3(e,y,x,y),frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar).x,
                            map(p-swi3(e,y,y,x),frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar).x); // !!!!!!!!!!!!
    return normalize(n);
}

__DEVICE__ float3 R(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
           r = normalize(cross(to_float3(0,1,0), f)),
           u = cross(f,r),
           c = p+f*z,
           i = c + uv.x*r + uv.y*u,
           d = normalize(i-p);
    return d;
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, in float time, float frequency, float radius, float lenReactor, float amplitudeR[3], float iTime, float3 shippos, float pathpar[5] )
{
  float occ = 0.0f;
  float sca = 1.0f;
  for( int i=0; i<5; i++ )
  {
      float h = 0.01f + 0.12f*(float)(i)/4.0f;
      float d = map( pos+h*nor,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar).x;
      occ += (h-d)*sca;
      sca *= 0.95f;
  }
  return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );
}

// https://iquilezles.org/articles/rmshadows
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, float tmin, float tmax, const float k, float frequency, float radius, float lenReactor, float amplitudeR[3], float iTime, float3 shippos, float pathpar[5] )
{
  float res = 1.0f;
  float t = tmin;
  for( int i=0; i<50; i++ )
  {
  float h = map( ro + rd*t,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar).x;
      res = _fminf( res, k*h/t );
      t += clamp( h, 0.02f, 0.20f );
      if( res<0.005f || t>tmax ) break;
  }
  return clamp( res, 0.0f, 1.0f );
}




__KERNEL__ void ThisIsPodracingJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{

    CONNECT_CHECKBOX0(Tex, 0);
    CONNECT_CHECKBOX1(Tracking, 1);
    CONNECT_CHECKBOX2(PlaneTex, 0);
    
    CONNECT_INTSLIDER0(AA, 1, 3, 1);
    
    CONNECT_COLOR0(ColorSky, 0.7f, 0.7f, 0.9f, 1.0f);
    CONNECT_COLOR1(ColorReactor, 0.4f, 0.8f, 1.0f, 1.0f);
    CONNECT_COLOR2(ColorCockpit, 0.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_COLOR3(ColorGround, 0.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_COLOR4(Color5, 0.5f, 0.4f, 0.2f, 1.0f);
    CONNECT_COLOR5(ColorFog, 0.5f, 0.4f, 0.2f, 1.0f);


    CONNECT_SLIDER0(ViewZ, -50.0f, 50.0f, 0.0f); 
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER1(CamZ, -50.0f, 50.0f, 0.0f); 
    CONNECT_POINT1(CamXY, 0.0f, 0.0f);
    CONNECT_SLIDER2(amplitudeRz, -1.0f, 5.0f, 0.55f); 
    CONNECT_SLIDER3(amplitudeRy, -1.0f, 5.0f, 1.0f); 
    CONNECT_SLIDER4(amplitudeRx, -1.0f, 5.0f, 0.0f); 
    CONNECT_SLIDER5(frequency, -1.0f, 5.0f, 0.8f); 
    CONNECT_SLIDER6(lenReactor, -1.0f, 5.0f, 0.4f); 
    CONNECT_SLIDER7(radius, -1.0f, 1.0f, 0.12f); 
    
    CONNECT_SLIDER8(Tex1, -1.0f, 2.0f, 0.33f); 
    CONNECT_SLIDER9(Tex2, -1.0f, 2.0f, 0.33f); 
    CONNECT_SLIDER10(Tex3, -1.0f, 2.0f, 0.33f); 
    
    CONNECT_SLIDER11(ShipPosZ, -5.0f, 5.0f, 0.0f); 
    CONNECT_POINT2(ShipPosXY, 0.0f, 0.0f);
    
    CONNECT_SLIDER12(Clouds, -5.0f, 5.0f, 0.0005f); 
    
    CONNECT_SLIDER13(PathPar1, -5.0f, 5.0f, 0.1f); 
    CONNECT_SLIDER14(PathPar2, -5.0f, 5.0f, 2.0f); 
    CONNECT_SLIDER15(PathPar3, -5.0f, 5.0f, 1.5f); 
    CONNECT_SLIDER16(PathPar4, -5.0f, 5.0f, 1.0f); 
    CONNECT_SLIDER17(PathPar5, -5.0f, 5.0f, 1.5f); 
    
    //#define MAX_STEPS 600
    CONNECT_INTSLIDER1(MAX_STEPS, 100, 1000, 300);
    
    #define MAX_DIST 1000.0f //750.0f
    #define SURF_DIST 0.0001f
    //CONNECT_SLIDER18(MAX_DIST, -5.0f, 1500.0f, 1000.0f); 
    
    CONNECT_SLIDER18(TimeModulo, -50.0f, 50.0f, 0.0f); 
    
    CONNECT_SLIDER19(Fog, -0.10f, 0.10f, 0.00001f); 
    
    float pathpar[5] = {PathPar1,PathPar2,PathPar3,PathPar4,PathPar5};
    
    // Aufdruck auf den Steinen eingrenzen
    CONNECT_POINT3(TexUL, 0.0f, 0.0f);
    CONNECT_POINT4(TexOR, 0.0f, 0.0f);
    CONNECT_POINT5(TexOffset, 0.0f, 0.0f);
    
    float3 shippos = to_float3_aw(ShipPosXY, ShipPosZ);
    
    float amplitudeR[3] = {amplitudeRz,amplitudeRy,amplitudeRx};

    //CONNECT_SCREW0(romul, -1.0f, 5.0f, 1.0f);
    CONNECT_SLIDER20(romul, -1.0f, 5.0f, 1.0f);

    //float speed = 1.9f;
    //float amplitude = 2.0f;
    //float amplitudeR = 0.55f;
    //float frequency = 0.8f;
    //float lenReactor = 0.4f;
    //float radius = 0.12f;

    float ratio = iResolution.y/iResolution.x;

    //Zeitnormal
    iTime = iTime*4.0f;

    if (TimeModulo > 0.0f)
       iTime = _fmod(iTime, TimeModulo);   

    float2 m = swi2(iMouse,x,y)/iResolution;
    
    float3 col = to_float3(0.0f,0.0f,0.0f);
    float3 ro = to_float3(0, 1.0f, 1.0f)*3.5f;
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+1.0f))); // 
    //swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f))); //im Org auskommentiert
        
    // View
    ro.z = ro.z - (Tracking ? iTime : 0.0f) + CamZ;
    ro.x += path(ro.z, frequency, pathpar).x + CamXY.x;
    //ro.y += Test;
    ro.y = _fmaxf(ro.y, -0.1f);
    ro.y = _fminf(ro.y, 1.0f);
    
    ro.y += CamXY.y;
    
    ro *= romul;
    
    float3 colsky;
    
    for(int _x=0; _x<AA; _x++) {
      for(int _y=0; _y<AA; _y++) {
          
        float2 offs = to_float2(_x, _y)/(float)(AA) -0.5f;

        float2 uv = (fragCoord+offs-0.5f*iResolution)/iResolution.y;
        float3 dir = to_float3(ro.x+ViewXY.x, 1.0f+ViewXY.y, path(ro.z, frequency,pathpar).y - (Tracking ? iTime : 0.0f) + ViewZ);
        float3 rd = R(uv, ro, dir, 1.0f);
        
        // Sky
        col = swi3(ColorSky,x,y,z);//to_float3(0.7f,0.7f,0.9f);
        col -= _fmaxf(rd.y,0.0f)*1.0f; 
        // clouds
        float2 sc = swi2(ro,x,z) + swi2(rd,x,z)*(200.0f-ro.y)/rd.y;
        col = _mix( col, to_float3(1.0f,0.95f,1.0f), 0.5f*smoothstep(0.4f,0.9f,fbm(Clouds*sc, iChannel2)) );

 
        int mat = -1;
        float dist = RayMarch(ro, rd, &mat,frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar,MAX_STEPS,MAX_DIST,SURF_DIST).x;        
        
        float3 p = ro + rd * dist;
        float3 movingPos = p;
        movingPos.z += iTime;
        swi2S(movingPos,x,z, path(movingPos.z, frequency,pathpar));
        float3 f0 = to_float3_s(0.0f);
        float3 te = to_float3_s(0.0f);
        float3 gd = to_float3_s(0.0f);
        
        switch(mat){
            //Metal
            case 1:
                te = 0.5f * swi3(texture(iChannel0, swi2(movingPos,x,y)* 2.0f),x,y,z)
                   + 0.5f * swi3(texture(iChannel0, swi2(movingPos,x,z)),x,y,z);
                te  = 0.4f * te;
                col = te;
                f0  = te;
                break;
            //Reactor
            case 2:
                f0  = swi3(ColorReactor,x,y,z);//to_float3(0.4f,0.8f,1.0f);
                col = f0;
                break;
            //Cockpit
            case 3:
                col = swi3(ColorCockpit,x,y,z);//to_float3(0.0f,0.0f,0.0f);
                break;
            //Ground
            case 4:
                f0 = swi3(ColorGround,x,y,z);//to_float3_s(0.0f);

                if(Tex)
                {
                  //if (p.x > TexUL.x && p.x < TexOR.x && p.y > TexUL.y && p.y < TexOR.y)
                  if (p.x+TexOffset.x > TexUL.x && p.x+TexOffset.x < TexOR.x && p.y+TexOffset.y > TexUL.y && p.y+TexOffset.y < TexOR.y)  
                
                  gd = Tex1 * swi3(texture(iChannel1, to_float2(p.x*ratio,p.y)*2.0f),x,y,z)
                      +Tex2 * swi3(texture(iChannel1, to_float2(p.y*ratio,p.z)),x,y,z)
                      +Tex3 * swi3(texture(iChannel1, to_float2(p.x*ratio,p.z)),x,y,z);
                }
                //else
                { // Original
                  if (!Tex || (!(p.x+TexOffset.x > TexUL.x && p.x+TexOffset.x < TexOR.x && p.y+TexOffset.y > TexUL.y && p.y+TexOffset.y < TexOR.y)))//p.x > TexUL.x && p.x < TexOR.x && p.y > TexUL.y && p.y < TexOR.y)))
                  gd += 0.33f * swi3(texture(iChannel3, swi2(p,x,y)* 2.0f),x,y,z)
                      + 0.33f * swi3(texture(iChannel3, swi2(p,y,z)),x,y,z)
                      + 0.33f * swi3(texture(iChannel3, swi2(p,x,z)),x,y,z);
                }
                
                gd  = 0.5f * gd;
                col = gd;
           
                break;
            case 5: 
                if(PlaneTex)
                  col = swi3(texture(iChannel4, to_float2(p.x*ratio,p.z)),x,y,z);// 0.0f;
                else
                  col *= swi3(Color5,x,y,z);//to_float3(0.5f, 0.4f, 0.2f);
                //break;
            case -1:
                col *= to_float3(1.0f,1.0f,1.0f);
                
                break;
        }
        

        if(dist<MAX_DIST) {
            
            float3 lightPos = to_float3(0.0f,10.0f,4.0f);
            //vec3 lightPos = movingPos + to_float3(0.0f,10.0f,4.0f);
            float3 l = normalize(lightPos);
            float3 n = GetNormal(p,frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar);
            
            float occ = calcAO(p, n, iTime,frequency,radius,lenReactor,amplitudeR, iTime, shippos, pathpar);
            //Top Light
            {
              float dif = clamp(dot(n, l), 0.0f, 1.0f);
              float3 ref = reflect(rd, n);
              float3 spe = to_float3_s(1.0f) * smoothstep(0.4f,0.6f,ref.y);
              float fre = clamp(1.0f+dot(rd, n), 0.0f, 1.0f);
              spe *= f0; //+ (1.0f-f0) * _powf(fre,5.0f); ???????????????????
              spe *= 6.0f;
              //float shadow = calcSoftshadow(p, l, 0.1f, 2.0f, 32.0f,frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar );
             // dif *= shadow;
              col += 0.55f*to_float3(0.7f,0.7f,0.9f)*dif*occ;
              col += to_float3(0.7f,0.7f,0.9f)*spe*dif*f0;  
            }
      
            //Side Light
            {
              float3 lightPos = normalize(to_float3(-2.7f,1.2f,-0.4f));
              float dif = clamp(dot(n, lightPos), 0.0f, 1.0f);
              float shadow = calcSoftshadow(p, lightPos, 0.001f, 2.0f, 16.0f,frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar );

              float3 hal = normalize(lightPos-rd);
              float3 spe = to_float3_s(1.0f) * _powf(clamp(dot(hal, n), 0.0f, 1.0f),32.0f);
              spe *= f0 + (1.0f-f0) * _powf(1.0f-clamp(dot(hal, lightPos), 0.0f, 1.0f),5.0f);

              dif *= shadow;
              col += 0.5f*to_float3(1.0f,0.6f,0.3f)*dif*occ;
              col += 1.0f*to_float3(1.0f,0.6f,0.3f)*spe*f0;
            }
            
            //Bottom light
            {
              float dif = clamp(0.5f -0.5f * n.y,0.0f ,1.0f);
              col += 0.15f*dif*occ;
            }
            //Reactor Light
            {
              //vec3 lightPos = normalize(to_float3(_fabs(movingPos.x) - 0.5f,0.0f, lenReactor));
              //float dif = clamp(dot(n, lightPos), 0.0f, 1.0f);
              //float shadow = calcSoftshadow(p, lightPos, 0.001f, 0.5f, 8.0f,frequency,radius,lenReactor,amplitudeR, iTime, shippos,pathpar );
              //col += (0.7f + 0.3f * _sinf(iTime))*to_float3(1.0f,1.0f,2.0f) * dif * shadow;
            }
            col = _mix( col, 0.9f*swi3(ColorFog,x,y,z), 1.0f-_expf( -Fog*dist*dist*dist ) );
        }
      }
    }
    
    //col /= (float)(AA*AA); //Überflüssig
    
    col = clamp(col,0.0f,1.0f);
    col = col*col*(3.0f-2.0f*col);
        
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}