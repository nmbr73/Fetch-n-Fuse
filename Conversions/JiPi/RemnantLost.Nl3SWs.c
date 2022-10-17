
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



  #define THRESHOLD 0.1f
  #define FAR 2000.0f
  #define SCALE 2.8f
  #define MINRAD2 0.25f



#define TSIZE 256.0f
#define TWRAP 255


#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 make_uint2(UI0, UI1)
#define UI3 make_uint3(UI0, UI1, 2798796415U)
#define UIF (1.0f / (float)(0xfffffffeU))

__DEVICE__ float hash12(uint2 q)
{
  q *= UI2;
  uint n = (q.x ^ q.y) * UI0;
  return (float)(n) * UIF;
}

__DEVICE__ float2 hash22(float2 vq)
{
  uint2 q;// = uto_float2(vq);
  q.x=(uint)vq.x;
  q.y=(uint)vq.y;

  q *= UI2;
  q = (q.x ^ q.y) * UI2;
  return to_float2_cuint(q) * UIF;
}
//----------------------------------------------------------------------------------------
__DEVICE__ float2 noise2D( in float2 n )
{
    float2 p = _floor(n);
    n = fract_f2(n);
    n = n*n*(3.0f-2.0f*n);
    
    float2 res = _mix(_mix( hash22(p), hash22(p+to_float2(1.0f ,0.0f)),n.x),
                      _mix( hash22(p + to_float2(0.0f,1.0f)), hash22(p + to_float2(1.0f,1.0f)),n.x),n.y);
    return res;
}

__DEVICE__ float sMin( float a, float b, float k )
{
    
  float h = clamp(0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ mat3 rotateX(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                      to_float3(1, 0, 0),
                      to_float3(0, c, -s),
                      to_float3(0, s, c)
    );
}

// Rotation matrix around the Y axis.
__DEVICE__ mat3 rotateY(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                      to_float3(c, 0, s),
                      to_float3(0, 1, 0),
                      to_float3(-s, 0, c)
    );
}

// Rotation matrix around the Z axis.
__DEVICE__ mat3 rotateZ(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                      to_float3(c, -s, 0),
                      to_float3(s, c, 0),
                      to_float3(0, 0, 1)
    );
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


// Create derivative noise texture
// PLEASE DO SELECTABLE BUFFER SIZES GUYS!

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

__DEVICE__ float derivHash(int2 q2)
{
  
   //uint2 q = make_uint2((q2+10) & make_int2(TWRAP));  // ...Seeded and wrapped.
   uint2 q = make_uint2((q2.x+10) & TWRAP,(q2.y+10) & TWRAP);  // ...Seeded and wrapped.
   float f = hash12(q);
   return _powf(f, 2.0f)*1.5f;
}


__KERNEL__ void RemnantLostFuse__Buffer_A(float4 colour, float2 coord, int iFrame, float2 iResolution)
{

coord+=0.5f;


// Draw it ony once to relavent area...

// Phänomen!!!!! Das Bild wird NICHT wieder eingelesen, wie es geschrieben wurde, durch die Eingrenzung von coord kommt es beim Auslesen zu einem vollkommen falschen Bild - Buggy

// It seems the buffers are also doubled as I need to draw 2 frames...
  if (coord.x < TSIZE && coord.y < TSIZE)
  {
    //colour = texture(iChannel0, coord/R*10.); //discard;
    //if (iFrame < 2 && coord.x < TSIZE && coord.y < TSIZE)  // Org
    //if(iFrame < 2)
    {

      float4 data, n;
      int2 co = make_int2(_floor(coord));

      float a = derivHash(co);
      float b = derivHash((co+to_int2(1,0)));
      float c = derivHash((co+to_int2(0,1)));
      float d = derivHash((co+to_int2(1,1)));

      // Pre-calc all the sums...
      data.x = a;       
      data.y = b-a;       
      data.z = c-a;       
      data.w = a - b - c + d;

      colour = data;
    }
    //else 
      //colour = texture(iChannel0, coord/R); //discard;
  } 
  else
      colour = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(colour);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'https://soundcloud.com/sergenarcissovmusic/space-ambient?utm_source=clipboard&utm_medium=text&utm_campaign=social_sharing' to iChannel1


// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// https://www.shadertoy.com/view/Nl3SWs

#define USE_TEX_NOISE_DERIVATIVE
// Uses a texture to store derivative noise data.
// Needs only one texture look up.
// We can't set buffer sizes, so the speed up is good not that great,
// because there's still an enormous amount of texture cache thrashing.
// It really needs a 256x256 buffer.

// PLEASE DO SELECTABLE BUFFER SIZES GUYS!




//-------------------------------------------------------------------------------------------------------

#ifdef USE_TEX_NOISE_DERIVATIVE

// Uses one texture look up...
__DEVICE__ float3 noiseD(in float2 _x,float2 R, __TEXTURE2D__ iChannel0) 
{
  float2 f = fract_f2(_x);
//    float2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
//    float2 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
  float2 u = f*f*(3.0f-2.0f*f);
  float2 du = 6.0f*f*(1.0f-f);


  int2 p = make_int2(_floor(_x));
  //float4 n = texelFetch(iChannel0, p & TWRAP, 0);
  float4 n = texture(iChannel0, (make_float2(make_int2(p.x & TWRAP,p.y & TWRAP))+0.5f)/R);


  float2 tmp = du * (swi2(n,y,z) + n.w*swi2(u,y,x));

  return to_float3(n.x + n.y * u.x + n.z * u.y + n.w * u.x*u.y, tmp.x,tmp.y );
}

#else

// iq's original code from 'elevated'...
__DEVICE__ float3 noiseD(in float2 _x, float2 R, __TEXTURE2D__ iChannel0 )
{
  float2 f = fract_f2(_x);
    
//    float2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
//    float2 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
  float2 u = f*f*(3.0f-2.0f*f);
  float2 du = 6.0f*f*(1.0f-f);

  int2 p = to_int2(_floor(_x));
  float a = texelFetch(iChannel0, p&TWRAP, 0 ).x;
  float b = texelFetch(iChannel0, (p+to_int2(1,0))&TWRAP, 0 ).x;
  float c = texelFetch(iChannel0, (p+to_int2(0,1))&TWRAP, 0 ).x;
  float d = texelFetch(iChannel0, (p+to_int2(1,1))&TWRAP, 0 ).x;

  return to_float3(a + (b-a) * u.x+(c-a) *u.y+(a-b-c+d)*u.x*u.y,
         du*(to_float2(b-a,c-a)+(a-b-c+d)*swi2(u,y,x)));
}

#endif

//-------------------------------------------------------------------------------------------------------
// Basic 3D noise using texture channel...
__DEVICE__ float noise( in float3 p, float2 R, __TEXTURE2D__ iChannel3 )
{
  
  float3 f = fract_f3(p);
  p = _floor(p);
  f = f*f*(3.0f-2.0f*f);
  
  float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y);
  float2 rg = swi2(texture( iChannel3, (uv+ 0.5f)/256.0f),y,x);
  return _mix( rg.x, rg.y, f.z );
}

//-------------------------------------------------------------------------------------------------------

#define ANG2 1.2
#define ANG3 1.4


//-------------------------------------------------------------------------------------------------------
__DEVICE__ float terrain( in float2 p, float z, float2 R, __TEXTURE2D__ iChannel0)
{
  
  const mat2 rotMat = mul_mat2_f(to_mat2(_cosf(ANG2), _sinf(ANG3), -_sinf(ANG3), _cosf(ANG2)) , 2.1f);
  p = p*0.0015f;
  float2  d = to_float2_s(0.0f);
  float a = 0.0f, b = 120.0f;
    
  // Decrease iteration detail with distance...
  int iter = 13-(int)(_log2f(z*0.2f+0.02f));
  iter = clamp(iter, 2,13);
    
  for (int i = 0; i < iter; i++)
  {
    float3 n = noiseD(p,R,iChannel0);
        
    d += swi2(n,y,z);
    a += b*n.x/(1.0f+dot(d,d));
    b *= 0.47f;
    p = mul_mat2_f2(rotMat,p);
  } 

  return a;
}

// Faffin' about...
#define ANG4 0.785398f
#define ANG5 0.785398f

//mat3 rot3D;
//-------------------------------------------------------------------------------------------------------
__DEVICE__ float mapRemnant(float3 pos, mat3 rot3D, float minRad2, float4 scale, float absScalem1, float AbsScaleRaisedTo1mIters) 
{
  
  pos = pos + to_float3(300.0f,200.0f,-200);

  pos = mul_mat3_f3(rot3D , pos);
  //pos = mul_f3_mat3(pos,rot3D);

  
  //swi2(pos,z,y) = rotRemXY * swi2(pos,z,y);
  //swi2(pos,x,z) = rotRemXZ * swi2(pos,x,z);

  float4 p = to_float4_aw(pos*0.006f,1);

  float4 p0 = p;
  for (int i = 0; i < 8; i++)
  {
    swi3S(p,x,y,z, clamp(swi3(p,x,y,z), -1.0f, 1.0f) * 2.0f - swi3(p,x,y,z));

    float r2 = dot(swi3(p,x,y,z), swi3(p,x,y,z));
    p *= clamp(_fmaxf(minRad2/r2, minRad2), 0.0f, 1.0f);

    p = p*scale + p0;
  }
  float l = ((length(swi3(p,x,y,z)) - absScalem1) / p.w - AbsScaleRaisedTo1mIters) /0.006f;

  return l;
}

__DEVICE__ float3 remnantColour(float3 pos, float minRad2, float4 scale, float gTime, float3 surfaceColour1, float3 surfaceColour2, float3 surfaceColour3) 
{
  const mat2 rotRemXZ = to_mat2(_cosf(ANG4), _sinf(ANG4), -_sinf(ANG4), _cosf(ANG4));
  const mat2 rotRemXY = to_mat2(_cosf(ANG5), _sinf(ANG5), -_sinf(ANG5), _cosf(ANG5));
  
  pos = pos + to_float3(300.0f,200.0f,-200);
  float4 p = to_float4_aw(pos*0.006f,1);
  swi2S(p,z,y, mul_mat2_f2(rotRemXY , swi2(p,z,y)));
  swi2S(p,x,z, mul_mat2_f2(rotRemXZ , swi2(p,x,z)));


  float4 p0 = p;
  float trap = 1.0f;
    
  for (int i = 0; i < 6; i++)
  {
        
    swi3S(p,x,y,z, clamp(swi3(p,x,y,z), -1.0f, 1.0f) * 2.0f - swi3(p,x,y,z));

    float r2 = dot(swi3(p,x,y,z), swi3(p,x,y,z));
    p *= clamp(_fmaxf(minRad2/r2, minRad2), 0.0f, 1.0f);

    p = p*scale + p0;
    trap = _fminf(trap, r2);
  }
  // |c.x|: log final distance (fractional iteration count)
  // |c.y|: spherical orbit trap at (0,0,0)
  float2 c = clamp(to_float2( 0.0001f*length(p)-1.0f, _sqrtf(trap) ), 0.0f, 1.0f);

  float t = mod_f(length(pos*0.006f) - gTime*32.0f, 16.0f);
  float3 surf = _mix( surfaceColour1, to_float3(0.1f, 2.0f, 5.0f), smoothstep(0.0f, 0.3f, t) * smoothstep(0.6f, 0.3f, t));
  return _mix(_mix(surf, surfaceColour2, c.y), surfaceColour3, c.x);
}


//-------------------------------------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
__DEVICE__ float3 getSky(in float3 rd, float3 sunLight, float3 cloudColour, float3 sunColour)
{
  float sunAmount = _fmaxf( dot( rd, sunLight), 0.0f );
  float v = _powf(1.0f-_fmaxf(rd.y,0.0f),5.0f);
  float3  sky = _mix(to_float3(0.0f, 0.1f, 0.2f), cloudColour, v);
  sky = sky + sunColour * _powf(sunAmount, 4.0f) * 0.2f;
  sky = sky + sunColour * _powf(sunAmount, 800.0f)*5.0f;
  return clamp(sky, 0.0f, 1.0f);
}


//-------------------------------------------------------------------------------------------------------
// Merge grass into the sky background for correct fog colouring...
__DEVICE__ float3 applyFog( in float3  rgb, in float3 sky, in float dis,in float3 pos, in float3 dir)
{
  float fog = _expf(-dis*dis* 0.000001f);
  fog = clamp(fog-smoothstep(80.0f, 0.0f, pos.y)*0.3f, 0.0f, 1.0f);
    
  return _mix(sky, rgb, fog);
}


//-------------------------------------------------------------------------------------------------------
// Calculate sun light...
__DEVICE__ float3  DoLighting(in float3 dif, in float3 pos, in float3 nor, in float3 eyeDir, in float dis, float3 sunLight, float3 sunColour)
{
  float h = dot(sunLight,nor);
  float3 mat = dif * sunColour*(_fmaxf(h, 0.0f)+0.04f);
  float3 ref = reflect(eyeDir, nor);
  mat += sunColour * _powf(_fmaxf(dot(ref, sunLight), 0.0f), 80.0f)*0.5f;
         
  return _fminf(mat, to_float3_s(1.0f));
}


//-------------------------------------------------------------------------------------------------------
// Map the whole scene with two objects...
__DEVICE__ float map(float3 p, float z, mat3 rot3D, float2 R, __TEXTURE2D__ iChannel0, float minRad2, float4 scale, float absScalem1, float AbsScaleRaisedTo1mIters)
{
  return  _fminf(p.y-terrain(swi2(p,x,z), z,R,iChannel0), mapRemnant(p, rot3D, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters));
}

//-------------------------------------------------------------------------------------------------------

// March the whole scene...
__DEVICE__ float rayMarch(in float3 rO, in float3 rD, in float st, mat3 rot3D, float2 R, __TEXTURE2D__ iChannel0, float minRad2, float4 scale, float absScalem1, float AbsScaleRaisedTo1mIters)
{
  float t = st;
  float d = 0.0f;

  float oldT = t;
   
  float3 p;

  //for(int j = _fminf(0, iFrame); j < 160 && t < FAR; j++)
    for(int j = 0; j < 160 && t < FAR; j++)
  {
      p = rO + t*rD;
      d = map(p, t, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters);
      if (_fabs(d) < THRESHOLD) break;
      oldT = t;
      t += d;// + t * 0.001f; // Adding the current 't' thins out the Box too much.
  }
 
  return t;
}

//-------------------------------------------------------------------------------------------------------
__DEVICE__ float3 CameraPath( float t, float2 R, __TEXTURE2D__ iChannel0, float gTime )
{
  float2 p = to_float2(240.0f+1200.0f * _sinf(1.4f*t), 800.0f * _cosf(1.0f*t) );
  return to_float3(p.x,   terrain(p, 9000.0f,R,iChannel0)+95.0f+_cosf(gTime*3.0f-2.7f)*50.0f, p.y);
} 


//-------------------------------------------------------------------------------------------------------
__DEVICE__ float3 getNormal(float3 p, float dis, mat3 rot3D, float2 R, __TEXTURE2D__ iChannel0, float minRad2, float4 scale, float absScalem1, float AbsScaleRaisedTo1mIters)
{
  dis = dis*2.0f/iResolution.y;
  float2 e = to_float2(0,clamp(dis*dis, 0.001f, 144.0f));
  return normalize(map(p, 0.0f, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters)
                   -to_float3(map(p - swi3(e,y,x,x), 0.0f, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters), 
                              map(p - swi3(e,x,y,x), 0.0f, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters), 
                              map(p - swi3(e,x,x,y), 0.0f, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters)));
}

//------------------------------------------------------------------------------
__DEVICE__ float shadow( in float3 ro, in float3 rd, in float dis, mat3 rot3D, float2 R, __TEXTURE2D__ iChannel0, float minRad2, float4 scale, float absScalem1, float AbsScaleRaisedTo1mIters)
{
  float res = 1.0f;
  float t = 20.0f;
  float h;
  
  for (int i = 0; i < 25; i++)
  {
    float3 p =  ro + rd*t;

    h = map(p, dis, rot3D, R, iChannel0, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters);
    res = _fminf(0.2f*h / t*t, res);
    t += h+3.0f;
  }
    return clamp(res, 0.2f, 1.0f);
}


//-------------------------------------------------------------------------------------------------------
__DEVICE__ float3 getDiffuse(float3 pos, float3 dir,  float3 nor, float dis, float minRad2, float4 scale, float gTime, float3 surfaceColour1, float3 surfaceColour2, float3 surfaceColour3, float2 R, __TEXTURE2D__ iChannel0)
{
    float3 dif = to_float3_s(0);
    if ((pos.y-terrain(swi2(pos,x,z), dis,R,iChannel0)) < THRESHOLD)
    {
        float n = _cosf(pos.y*0.03f+pos.x*0.01f+0.8f)*0.5f+0.5f;
    
        dif = _mix(to_float3(0.9f,0.5f,0.3f), to_float3(1.0f, 0.8f, 0.7f), n);
        float s = _fmaxf(0.0f,nor.y*nor.y);
        dif = _mix(dif, to_float3(s*0.3f, s*0.3f,0.1f), clamp(nor.x+nor.z, 0.0f, 0.8f));
     }else
     {
         //float n = noise(pos*0.3f, R.iChannel3);
         dif = to_float3(0.2f,0,0);
         dif = remnantColour(pos,minRad2,scale,gTime,surfaceColour1,surfaceColour2,surfaceColour3);
     }
    return dif;
}


//-------------------------------------------------------------------------------------------------------

__DEVICE__ float3 getCamera(float3 *cameraPos, float2 uv,float3 *cw, float3 *cu, float3 *cv, float gTime, float2 R, __TEXTURE2D__ iChannel0)
{
  float3 camTar;
  *cameraPos = CameraPath(gTime + 0.0f,R,iChannel0,gTime);

  camTar   = CameraPath(gTime + 0.3f,R,iChannel0,gTime);

  camTar.y = (*cameraPos).y;
  
  float roll = 0.4f*_sinf(gTime+0.5f);
  *cw = normalize(camTar-*cameraPos);
  float3 cp = to_float3(_sinf(roll), _cosf(roll),0.0f);
  *cu = cross(*cw,cp);
  *cv = cross(*cu,*cw);
  
  return normalize(uv.x* *cu + uv.y* *cv + 1.0f* *cw);;
}


//-------------------------------------------------------------------------------------------------------
__KERNEL__ void RemnantLostFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel3)
{
  fragCoord+=0.5f;

  CONNECT_CHECKBOX0(Textur, 0);
  CONNECT_CHECKBOX1(Kinostreifen, 1);
  CONNECT_SLIDER0(RotY,0.0f,1.0f,0.785398f);
  CONNECT_SLIDER1(RotX,0.0f,1.0f,0.785398f);
  CONNECT_SLIDER2(RotZ,0.0f,1.0f,0.0f);

  __TEXTURE2D__ iCh = iChannel0; //Versuch BufferA zu ersetzen - geht nicht PNG hat zwar Alpha, verfälscht aber alle Farben!!!!!!!
  if (Textur)
    iCh = iChannel1;
    

  const float minRad2 = clamp(MINRAD2, 1.0e-9, 1.0f);
  const float absScalem1 = _fabs(SCALE - 1.0f);
  const float AbsScaleRaisedTo1mIters = _powf(_fabs(SCALE), (float)(1-10));
  const float3 surfaceColour1 = to_float3(0.4f, 0.0f, 0.0f);
  const float3 surfaceColour2 = to_float3(0.4f, 0.4f, 0.4f);
  const float3 surfaceColour3 = to_float3(0.4f, 0.1f, 0.00f);
  const float4 scale =to_float4(SCALE, SCALE, SCALE, _fabs(SCALE)) / minRad2;

  const float3 sunLight  = normalize( to_float3(  1.1f, 0.8f,  -0.8f ) );
  const float3 sunColour = to_float3(1.0f, 0.8f, 0.7f);
  const float3 cloudColour = to_float3(0.35f, 0.25f, 0.25f);

  float3 cameraPos;
  //float gTime = 0.0f;



  float m = (iMouse.x/iResolution.x)*50.0f;
  float gTime = (iTime*0.8f+m+403.0f)*0.1f;
  float2 xy = fragCoord / iResolution;
  float2 uv = (-1.0f + 2.0f * xy) * to_float2(iResolution.x/iResolution.y,1.0f);

  //mat3 rot3D = mul_mat3_mat3(rotateY(0.785398f) , rotateX(0.785398f));
  //mat3 rot3D = mul_mat3_mat3(rotateY(RotY) , rotateX(RotX));
  mat3 rot3D = mul_mat3_mat3(rotateX(RotX), rotateY(RotY)); // !!!!! Multiplikation verkehrt rum !!!!!!
  
  rot3D = mul_mat3_mat3(rot3D, rotateZ(RotZ));
  

  if (_fabs(xy.y - 0.5f) > 0.37f && Kinostreifen) //Schwarze Kinostreifen
  {
    // Top and bottom cine-crop - what a waste! :)
    fragColor=to_float4_s(0.0f);
    SetFragmentShaderComputedColor(fragColor);
    return;
  }

  float3 cw, cu, cv;
  float3 dir = getCamera(&cameraPos,uv,&cw,&cu,&cv,gTime,R,iCh);

  float3 col;
  float dist;

  float st = hash12(make_uint2((uint)(fragCoord.x*iTime),(uint)(fragCoord.y*iTime)))*50.0f; //??????????????????
  dist = rayMarch(cameraPos, dir, st, rot3D, R, iCh, minRad2, scale, absScalem1, AbsScaleRaisedTo1mIters);
    
  float3 sky = getSky(dir,sunLight,cloudColour,sunColour);
    
  if (dist >= FAR)
  {
    // Completely missed the scene...
    col = sky;
  }
  else
  {
    // Render the objcets...
    float3 pos = cameraPos + dist * dir;
    float3 nor = getNormal(pos, dist, rot3D,R,iCh,minRad2,scale,absScalem1,AbsScaleRaisedTo1mIters);
    float3 dif = getDiffuse(pos, dir, nor, dist,minRad2,scale,gTime,surfaceColour1,surfaceColour2,surfaceColour3,R,iChannel0);
    col = DoLighting(dif, pos, nor,dir, dist,sunLight,sunColour);
    col *= shadow( pos, sunLight, dist, rot3D, R,iCh,minRad2,scale,absScalem1,AbsScaleRaisedTo1mIters);
    col = applyFog(col, sky, dist, pos, dir);
  }


  // My usual Sun flare stuff...
  float bri = dot(cw, sunLight)*0.75f;
  if (bri > 0.0f)
  {
    float2 sunPos = to_float2( dot( sunLight, cu ), dot( sunLight, cv ) );
    float2 uvT = uv-sunPos;
    uvT = uvT*(length(uvT));
    bri = _powf(bri, 6.0f)*0.8f;

    // glare = the red shifted blob...
    float glare1 = _fmaxf(dot(normalize(to_float3(dir.x, dir.y+0.3f, dir.z)),sunLight),0.0f)*1.4f;
    // glare2 is the cyan ring...
    float glare2 = _fmaxf(1.0f-length(uvT+sunPos*0.5f)*4.0f, 0.0f);
    uvT = _mix (uvT, uv, -2.3f);
    // glare3 is a purple splodge...
    float glare3 = _fmaxf(1.0f-length(uvT+sunPos*5.0f)*1.2f, 0.0f);

    col += bri * to_float3(1.0f, 0.0f, 0.0f)  * _powf(glare1, 12.5f)*0.1f;
    col += bri * to_float3(0.2f, 1.0f, 1.0f) * _powf(glare2, 2.0f)*3.0f;
    col += bri * sunColour * _powf(glare3, 2.0f)*3.5f;
  }
    
  // Post screen effects...
  //col = smoothstep(0.0f, 1.0f, col);
  // Contrast...
  col = col*0.3f + (col*col*(3.0f-2.0f*col))*0.7f;
  // Gamma...
  col = sqrt_f3(col);
  
  fragColor=to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
