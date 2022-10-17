
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define R iResolution


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Lichen' to iChannel3
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel0
// Connect Buffer A 'Previsualization: Buffer A' to iChannel2


//////////////////////////////////////////////////////////////////////////////////////
// DATA BUFFER  -  PLANE MOVEMENT, KEYBOARD CHECKS AND MISSILE UPDATE (IF LAUNCHED)
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Keyoard input. Used to capture key-presses.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = This buffer (A). Read and write data to update movement in this shader.
// Channel 3 = Lichen texture. Used to create landscape height map used in collision detection.

//float2 R;


  #define PI _acosf(-1.0f)
//  #define keyClick(ascii)   ( texelFetch(iChannel0, to_int2(ascii, 0), 0).x > 0.0f)
//  #define keyPress(ascii)   ( texelFetch(iChannel0, to_int2(ascii, 1), 0).x > 0.0f)
  //#define read(memPos)      ( texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos)   ( texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (make_float2(memPos)+0.5f)/R).w)
  #define readRGB(memPos)   (  swi3(texture(iChannel2, (make_float2(memPos)+0.5f)/R),x,y,z))
  
  #define MAX_HEIGHT 150.0f 
  #define MIN_HEIGHT 0.0f 
  #define STARTHEIGHT 40.0f
  #pragma optimize(off) 
// SPACE   FIRE MISSILE
#define MISSILE_KEY 32  
// S    ROLL LEFT
#define ROLL_LEFT_KEY 83  
// F    ROLL RIGHT
#define ROLL_RIGHT_KEY 70      
// W    YAW LEFT   (PLANE STRIFE)
#define LEFT_KEY 87    
// R    YAW RIGHT   (PLANE STRIFE)
#define RIGHT_KEY 82     
// E    PITCH DOWN
#define UP_KEY 69     
// D    PITCH UP
#define DOWN_KEY 68     
// SHIFT  INC SPEED
#define SPEED_INCREASE_KEY 16     
// CTRL   DEC SPEED
#define SPEED_DECREASE_KEY 17    
// F1     ZOOM OUT
#define ZOOMOUT_KEY 112
// F2     ZOOM IN
#define ZOOMIN_KEY 113

// Alternative controls if uncommented  (lets you use arrow keys to control the plane)
/* 
// ENTER   FIRE MISSILE
#define MISSILE_KEY 13
// LEFT ARROW    ROLL LEFT
#define ROLL_LEFT_KEY 37  
// RIGHT ARROW    ROLL RIGHT
#define ROLL_RIGHT_KEY 39     
// DELETE   YAW LEFT   (PLANE STRIFE)
#define LEFT_KEY 46    
// PAGE DOWN    YAW RIGHT   (PLANE STRIFE)
#define RIGHT_KEY 34     
// UP ARROW    PITCH DOWN
#define UP_KEY 38     
// DOWN ARROW    PITCH UP
#define DOWN_KEY 40     
// SHIFT  INC SPEED
#define SPEED_INCREASE_KEY 16     
// CTRL   DEC SPEED
#define SPEED_DECREASE_KEY 17    
// F1     ZOOM OUT
#define ZOOMOUT_KEY 112
// F2     ZOOM IN
#define ZOOMIN_KEY 113
*/


//float3 sunPos=to_float3_s(0.0f);
//float3 planePos=to_float3_s(0.0f);
float explosionCount=0.0f;


struct Missile
{ 
  float3 pos;
  float life;
  float3 orientation;   // roll,pitch,turn amount
  float3 startPos;
};

struct Explosion
{ 
  float3 pos;
  float life;
};



__DEVICE__ mat3 setCamera(  float3 ro, float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr), 0.0f);
  float3 cu = normalize( cross(cw, cp) );
  float3 cv = normalize( cross(cu, cw) );
  return to_mat3_f3( cu, cv, cw );
}

__DEVICE__ mat2 r2(float r) {
  float c=_cosf(r), s=_sinf(r);
  return to_mat2(c, s, -s, c);
}

__DEVICE__ float2 pR(float2 p, float a) 
{
  p = mul_f2_mat2(p,r2(a));
  return p;
}

__DEVICE__ float noise2D( in float2 pos, __TEXTURE2D__ iChannel1)
{   
  float2 f = fract_f2(pos);
  f = f*f*(3.0f-2.0f*f);
  float2 rg = swi2(texture( iChannel1, (((_floor(pos)+to_float2(37.0f, 17.0f)) + swi2(f,x,y))+ 0.5f)/64.0f),y,x);
  return -1.0f+2.0f*_mix( rg.x, rg.y, 0.5f );
}

//__DEVICE__ float noise2D( in float2 pos )
//{
//  return noise2D(pos, 0.0f);
//}

// 3D noise function (IQ)
__DEVICE__ float fastFBM(float3 p)
{
  float3 ip=_floor(p);
  p-=ip; 
  float3 s=to_float3(7, 157, 113);
  float4 h=to_float4(0.0f, s.y, s.z, s.y+s.z)+dot(ip, s);
  p=p*p*(3.0f-2.0f*p); 
  h=_mix(fract_f4(sin_f4(h)*43758.5f), fract_f4(sin_f4(h+s.x)*43758.5f), p.x);
  swi2S(h,x,y, _mix(swi2(h,x,z), swi2(h,y,w), p.y));
  return _mix(h.x, h.y, p.z);
}

__DEVICE__ float hash(float h) 
{
  return fract(_sinf(h) * 43758.5453123f);
}

__DEVICE__ float noise(float3 x) 
{
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f * f * (3.0f - 2.0f * f);

  float n = p.x + p.y * 157.0f + 113.0f * p.z;
  return -1.0f+2.0f*_mix(
    _mix(_mix(hash(n + 0.0f), hash(n + 1.0f), f.x), 
    _mix(hash(n + 157.0f), hash(n + 158.0f), f.x), f.y), 
    _mix(_mix(hash(n + 113.0f), hash(n + 114.0f), f.x), 
    _mix(hash(n + 270.0f), hash(n + 271.0f), f.x), f.y), f.z);
}

__DEVICE__ float NoTreeZone(float3 p, float2 R, __TEXTURE2D__ iChannel2)
{
    float dist =             distance_f2(swi2(readRGB(to_int2(140, 0)),x,z),swi2(p,x,z));
          dist = _fminf(dist,distance_f2(swi2(readRGB(to_int2(142, 0)),x,z),swi2(p,x,z)));
          dist = _fminf(dist,distance_f2(swi2(readRGB(to_int2(144, 0)),x,z),swi2(p,x,z)));
          dist = _fminf(dist,distance_f2(swi2(readRGB(to_int2(146, 0)),x,z),swi2(p,x,z)));
          dist = _fminf(dist,distance_f2(swi2(readRGB(to_int2(148, 0)),x,z),swi2(p,x,z)));
    return dist;
}
__DEVICE__ float GetTerrainHeight( float3 p, float2 planePos, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3)
{
  float2 p2 = (swi2(p,x,z)+swi2(planePos,x,z))*0.0005f;

  float heightDecrease = _mix(1.0f,0.0f,smoothstep(0.0f,15.0f,NoTreeZone(p+planePos)));
    
  float mainHeight = -2.3f+fastFBM((p+to_float3(planePos.x, 0.0f, planePos.z))*0.025f)*_fmaxf(11.0f, _fabs(22.0f*noise2D(p2, iChannel1))); 
  mainHeight-=heightDecrease;
    
  float terrainHeight=mainHeight;
  p2*=4.0f;
  terrainHeight += texture( iChannel3, p2 ).x*1.0f; 
  p2*=2.0f;
  terrainHeight -= texture( iChannel3, p2 ).x*0.7f;
  p2*=3.0f;
  terrainHeight -= texture( iChannel3, p2 ).x*0.1f;

  terrainHeight=_mix(terrainHeight, mainHeight*1.4f, smoothstep(1.5f, 3.5f, terrainHeight)); 

  return   terrainHeight;
}

__DEVICE__ float GetTreeHeight( float3 p, float terrainHeight, float2 R, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3, float3 planePos)
{
  if(NoTreeZone(p+planePos,R,iChannel2)<25.0f) return 0.0f;
  float treeHeight = texture(iChannel3, (swi2(p,x,z)+swi2(planePos,x,z))*0.006f).x;
  float tree = _mix(0.0f, _mix(0.0f, _mix(0.0f, 2.0f, smoothstep(0.3f, 0.86f, treeHeight)), smoothstep(1.5f, 3.5f, terrainHeight)), step(0.3f, treeHeight)); 
  tree -= tree*0.75f;
  tree*=4.0f;

  return  tree;
}

__DEVICE__ float3 TranslatePos(float3 p, float _direction, float _pitch, float _roll)
{
  swi2S(p,x,z, pR(swi2(p,x,z), _direction));
  swi2S(p,z,y, pR(swi2(p,z,y), _pitch));

  return p;
}

__DEVICE__ Missile LaunchMissile(Missile missile, float3 startPos, float3 orientation, float3 planePos)
{
  missile.life=4.0f; 
  missile.orientation = orientation;
  missile.pos =  startPos;
  missile.startPos= planePos;
  missile.orientation.y *=_cosf(missile.orientation.x-PI);
  
  return missile;
}

__DEVICE__ Missile UpdateMissile( Missile missile, float id, inout float4 *fragColor, float2 fragCoord, float3 moveDiff, float3 planePos, float explosionCount, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3)
{
  float adressStep = id*100.0f;
     
  Explosion explosion;
 
  // read variables for explosion s
  explosion.pos = readRGB(to_int2(120+int(adressStep), 0));    
  explosion.life = read(to_int2(122+int(adressStep), 0));

  // update active missile and save variables
  if ( missile.life>0.0f)
  {
    missile.life-= 0.015f;
    float3 velocityAdd = to_float3(0.0f, 0.0f, 1.4f);

    pR(swi2(velocityAdd,y,z), missile.orientation.y);
    pR(swi2(velocityAdd,x,z), -missile.orientation.z);

    missile.pos += velocityAdd; // add velocity movement to pos
    missile.swi2(pos,x,z)-=swi2(moveDiff,x,z); // add plane movement to pos

    // ground collision check                 
    float3 testPoint = missile.pos;
      
    testPoint+=to_float3(4.8f - (9.6f*id), -0.4f, -3.0f);
    pR(swi2(testPoint,x,z), missile.orientation.z);
    testPoint-=to_float3(4.8f - (9.6f*id), -0.4f, -3.0f);
    testPoint.y+=missile.startPos.y;
      
    float tHeight = GetTerrainHeight(testPoint, planePos,iChannel1,iChannel3);
    tHeight+=GetTreeHeight(testPoint, tHeight, planePos);

    // does missile hit terrain?
    if (testPoint.y<tHeight)
    {
      // if colliding, kill missile and spawn explosion.             
       explosion.pos =  missile.pos+missile.startPos;
       explosion.pos.y = tHeight-3.0f;
       explosion.life=10.0f;
       missile.life=-10.0f;
       explosionCount+=2.0f;
       explosionCount = mod_f(explosionCount,10.0f);
    }

    (*fragColor).w = _mix(missile.life, (*fragColor).w, step(1.0f, distance_f4(*fragCoord, to_float2(100.0f+adressStep, 0.0f))));
    
    float3 fragColorxyz = swi3(*fragColor,x,y,z);
    fragColorxyz = _mix(missile.startPos, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(102.0f+adressStep, 0.0f))));
    fragColorxyz = _mix(missile.orientation, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(108.0f+adressStep, 0.0f)))); 
    fragColorxyz = _mix(missile.pos, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(116.0f+adressStep, 0.0f))));
            
  }
  // ##################################################################

  // update explosion
  if ( explosion.life>0.0f)
  {   
    explosion.life-= 0.115f;
   // explosion.life= 9.715f;
    fragColorxyz = _mix(explosion.pos, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(120.0f+adressStep, 0.0f)))); 
    (*fragColor).w = _mix(explosion.life, (*fragColor).w, step(1.0f, distance_f2(fragCoord, to_float2(122.0f+adressStep, 0.0f))));
      
    // terrain holes
    fragColorxyz = _mix(_mix(explosion.pos, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(140.0f+explosionCount, 0.0f)))),fragColorxyz,step(0.4f,distance_f2(5.0f,explosion.life)));
  }
  
  *fragColor=to_float4_aw(fragColorxyz,(*fragColor).w);
  
  return missile;
}


__DEVICE__ float4 ToggleEffects(float4 fragColor, float2 fragCoord, float2 R, __TEXTURE2D__ iChannel2)
{
   // read and save effect values from buffer  
   float3 effects =  _mix(to_float3(-1.0f,1.0f,1.0f), readRGB(to_int2(20, 0)), step(1.0f, (float)(iFrame)));
   effects.x*=1.0f+(-2.0f*float(keyPress(49))); //1-key  LENSDIRT
   effects.y*=1.0f+(-2.0f*float(keyPress(50))); //2-key  GRAINFILTER
   effects.z*=1.0f+(-2.0f*float(keyPress(51))); //3-key  ChromaticAberration
   
   float3 effects2 =  _mix(to_float3(1.0f,1.0f,1.0f), readRGB(to_int2(22, 0)), step(1.0f, (float)(iFrame)));
   effects2.y*=1.0f+(-2.0f*float(keyPress(52))); //4-key  AA-pass
   effects2.x*=1.0f+(-2.0f*float(keyPress(53))); //5-key  lens flare

   float3 fragColorxyz = swi3(fragColor,x,y,z);
   fragColorxyz = _mix(effects, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(20.0f, 0.0f))));  
   fragColorxyz = _mix(effects2, fragColorxyz, step(1.0f, distance_f2(fragCoord, to_float2(22.0f, 0.0f))));  
   
   return to_float4_aw(fragColorxyz,fragColor.w);
}


__KERNEL__ void TurnNBurnFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  
  fragCoord+=0.5f;

  float2 R=iResolution;
 
  float2 mo = iMouse.xy/iResolution;
  float2 uv = fragCoord / iResolution;
  float2 screenSpace = (-iResolution + 2.0f*(fragCoord))/iResolution.y;

  // read plane values from buffer
  float turn = _mix(1.0f, read(to_int2(1, 10)), step(1.0f, (float)(iFrame)));
  float roll = _mix(3.14f, read(to_int2(1, 1)), step(1.0f, (float)(iFrame)));
  float rudderAngle = read(to_int2(6, 1));
  float speed = read(to_int2(10, 1));
  float pitch = read(to_int2(15, 1));
  float explosionCount = read(to_int2(3, 0));  
    
  float3 sunPos = _mix(normalize( to_float3(-1.0f, 0.3f, -0.50f) ), readRGB(to_int2(50, 0)), step(1.0f, (float)(iFrame)));
  float3 planePos = _mix(to_float3(-400, STARTHEIGHT, -100), readRGB(to_int2(55, 0)), step(1.0f, (float)(iFrame)));
  float CAMZOOM = _mix(13.9f, read(to_int2(52, 0)), step(1.0f, (float)(iFrame)));  
  float2 camRot = to_float2(-1.0f, 0.340f);

  // setup camera and ray direction
  camRot.x+=mo.x*16.0f; 
  camRot.y+=mo.y*16.0f; 
 
  // limit roll
  roll=mod_f(roll, 6.28f);
 
  // add turn angle based on roll  
  float turnAmount = _mix(0.0f, 1.57f, smoothstep(0.0f, 1.57f, 1.57f-distance(1.57f, roll-3.14f)));
  turnAmount += _mix(0.0f, -1.57f, smoothstep(0.0f, 1.57f, 1.57f-distance(-1.57f, roll-3.14f)));
  float PitchAdd = _sinf(pitch);
 
  // YAW
  turn+=0.02f*rudderAngle;
  // add turn angle  
  turn+=turnAmount*0.015f;
  turn-=0.1f*(((pitch*0.25f)*_cosf(roll-1.57f)));
  
  turn= mod_f(turn,PI*2.0f);
  float3 oldPlanePos = to_float3(planePos.x, planePos.y, planePos.z);

  // move plane
  swi2S(planePos,x,z, swi2(planePos,x,z) + to_float2(_cosf(turn+1.5707963f)*0.5f,  _sinf(turn+1.5707963f)*0.5f)*(0.7f+speed)*_cosf(pitch));
  planePos.y = clamp(planePos.y+((PitchAdd*0.25f)*_cosf(roll-PI)), MIN_HEIGHT, MAX_HEIGHT);

  rudderAngle*=0.97f;
  // check key inputs
  rudderAngle-=0.03f*float(keyClick(LEFT_KEY));
  rudderAngle+=0.03f*float(keyClick(RIGHT_KEY));
  rudderAngle=clamp(rudderAngle, -0.4f, 0.4f);;
  roll-=0.055f*float(keyClick(ROLL_LEFT_KEY));
  roll+=0.055f*float(keyClick(ROLL_RIGHT_KEY));

  speed+=(0.02f*float(keyClick(SPEED_INCREASE_KEY)));
  speed-=(0.02f*float(keyClick(SPEED_DECREASE_KEY)));
  speed=clamp(speed, -0.3f, 1.0f);
   
  // prevent plane from getting into terrain
  float tHeight = GetTerrainHeight(planePos, planePos,iChannel1,iChannel3);
  tHeight+=GetTreeHeight(planePos, tHeight,planePos);
  float minHeight = tHeight+12.0f;
  planePos.y = _fmaxf(planePos.y,minHeight);
    
   // pitch = _sinf(pitch);
  pitch-=(_mix(0.02f, 0.0f, smoothstep(0.0f, 3.0f, 3.0f-_fabs(distance(planePos.y, minHeight))))*(float)(keyClick(UP_KEY))); //e-key
  pitch+=(_mix(0.02f, 0.0f, smoothstep(0.0f, 3.0f, 3.0f-_fabs(distance(planePos.y, MAX_HEIGHT))))*(float)(keyClick(DOWN_KEY))); //d-key
  pitch = clamp(pitch, -1.25f, 1.25f);
  pitch*=0.97f;

  turnAmount += _mix(0.0f, -1.57f, smoothstep(0.0f, 1.57f, 1.57f-distance(-1.57f, roll-3.14f)));
  fragColor = to_float4(swi3(texture(iChannel2, uv),x,y,z),0.0f);
    
  // ------------------------- MISSILES ------------------------------
  // NOTE: MISSILES ARE RENDERED IN BUFFER B TOGETHER WITH THE TERRAIN     
  int adressStep = 0;
  bool launchLocked=false;
  Missile missile;
  for (int i=0; i<2; i++)
  {
    adressStep = i*100;
      
    // read variables for missiles
    missile.life = read(to_int2(100 + adressStep, 0));
    missile.startPos = readRGB(to_int2(102 + adressStep, 0));  
    missile.orientation = readRGB(to_int2(108 + adressStep, 0));
    missile.pos = readRGB(to_int2(116 + adressStep, 0));

  // if missile is "dead" check if a new missile is being lanched by pressing the M-key
  if (keyPress(MISSILE_KEY) && !launchLocked)
  {    
   if (missile.life<=0.0f)
   {
      missile = LaunchMissile(missile, to_float3(4.8f- (9.6f*(float)(i)), -0.4f, -3.0f), to_float3(roll, pitch, turn), planePos);  
      launchLocked=true;
   } 
 }    
 

  missile = UpdateMissile(missile, (float)(i), &fragColor, fragCoord, (planePos-oldPlanePos), planePos, explosionCount, iChannel1, iChannel3 );
  // ##################################################################
  }

  fragColor = ToggleEffects(fragColor, fragCoord, R, iChannel2);
   
  CAMZOOM-=0.3f*(float)(keyClick(ZOOMIN_KEY));
  CAMZOOM+=0.3f*(float)(keyClick(ZOOMOUT_KEY));
  CAMZOOM=clamp(CAMZOOM, 10.0f, 30.0f);;
  
  // save roll,speed and scroll values etc to buffer A 
  fragColor.w = _mix(turn, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(1.0f, 10.0f)))); 
  fragColor.w = _mix(speed, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(10.0f, 1.0f)))); 
  fragColor.w = _mix(roll, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(1.0f, 1.0f)))); 
  fragColor.w = _mix(pitch, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(15.0f, 1.0f)))); 
  fragColor.w = _mix(rudderAngle, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(6.0f, 1.0f))));
  fragColor.w = _mix(explosionCount, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(3.0f, 0.0f)))); 
  swi3S(fragColor,x,y,z, _mix(sunPos, swi3(fragColor,x,y,z), step(1.0f, distance_f2(fragCoord, to_float2(50.0f, 0.0f)))));
  fragColor.w = _mix(CAMZOOM, fragColor.w, step(1.0f, distance_f2(fragCoord, to_float2(52.0f, 0.0f))));
  swi3S(fragColor,x,y,z, _mix(planePos, swi3(fragColor,x,y,z), step(1.0f, distance_f2(fragCoord, to_float2(55.0f, 0.0f)))));
  swi3S(fragColor,x,y,z, _mix(to_float3(swi2(camRot,x,y), 0.0f), swi3(fragColor,x,y,z), step(1.0f, distance_f2(fragCoord, to_float2(57.0f, 0.0f)))));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Lichen' to iChannel3
// Connect Buffer B 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer B 'Texture: RGBA Noise Medium' to iChannel0
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2


//////////////////////////////////////////////////////////////////////////////////////
// TERRAIN BUFFER  -   RENDERS TERRAIN AND LAUNCHED MISSILES + EXPLOSIONS 
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Fine noise texture. Used in noise functions.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = Buffer A. Read data from data-buffer.
// Channel 3 = Lichen texture. Used to create landscape height map and textures.

float2 R;

  //#define read(memPos)    (  texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos) (  texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (to_float2(memPos)+0.5f)/R).w)
  #define readRGB(memPos)   (  texture(iChannel2, (to_float2(memPos)+0.5f)/R).xyz)
  
  #define MAX_HEIGHT 150.0f 
  #define WATER_LOD 0.4
  #define CLOUDLEVEL -70.0
  #define PI _acosf(-1.0f)
  #pragma optimize(off) 
  // remove on or several of below defines, if FPS is too low
  #define SHADOWS
  #define QUALITY_TREE
  #define QUALITY_REFLECTIONS
  #define EXACT_EXPLOSIONS
  // ---------------------------------------------------------

float turn=0.0f;
float2 cloudPos=to_float2_s(0.0f);
float eFlameDist=10000.0f;
float3 checkPos=to_float3_s(0.0f);
float3 sunPos=to_float3_s(0.0f);
const float3 sunColor = to_float3(1.00f, 0.90f, 0.85f);

const float3 eps = to_float3(0.02f, 0.0f, 0.0f);
float3 planePos=to_float3_s(0.0f);

struct RayHit
{
  bool hit;  
  float3 hitPos;
  float3 normal;
  float dist;
  float depth;
  float eFlameDist;
};

struct Missile
{ 
  float3 pos;
  float life;
  float3 orientation;   // roll,pitch,turn amount
  float3 origin;
};
    
struct Explosion
{ 
  float3 pos;
  float life;
};

__DEVICE__ mat2 r2(float r) {
  float c=_cosf(r), s=_sinf(r);
  return mat2(c, s, -s, c);
}

#define r3(r) mat2(_sinf(to_float4(-1, 0, 0, 1)*_acosf(0.0f)+r))

__DEVICE__ void pR(inout float2 p, float a)
{
  p*=r2(a);
}

__DEVICE__ float sgn(float x)
{   
  return (x<0.0f)?-1.:1.;
}

__DEVICE__ float hash(float h) 
{
  return fract(_sinf(h) * 43758.5453123f);
}

__DEVICE__ float noise(float3 x) 
{
  float3 p = _floor(x);
  float3 f = fract(x);
  f = f * f * (3.0f - 2.0f * f);

  float n = p.x + p.y * 157.0f + 113.0f * p.z;
  return -1.0f+2.0f*_mix(
    _mix(mix(hash(n + 0.0f), hash(n + 1.0f), f.x), 
    _mix(hash(n + 157.0f), hash(n + 158.0f), f.x), f.y), 
    _mix(mix(hash(n + 113.0f), hash(n + 114.0f), f.x), 
    _mix(hash(n + 270.0f), hash(n + 271.0f), f.x), f.y), f.z);
}

__DEVICE__ float fbm(float3 p) 
{
  float f = 0.5000f * noise(p);
  p *= 2.01f;
  f += 0.2500f * noise(p);
  p *= 2.02f;
  f += 0.1250f * noise(p);
  return f;
}

__DEVICE__ float noise2D( in float2 pos, float lod)
{   
  float2 f = fract(pos);
  f = f*f*(3.0f-2.0f*f);
  float2 rg = textureLod( iChannel1, (((_floor(pos).xy+to_float2(37.0f, 17.0f)) + swi2(f,x,y))+ 0.5f)/64.0f, lod).yx;  
  return -1.0f+2.0f*_mix( rg.x, rg.y, 0.5f );
}
__DEVICE__ float noise2D( in float2 pos )
{
  return noise2D(pos, 0.0f);
}

// 3D noise function (IQ)
__DEVICE__ float fastFBM(float3 p)
{
  float3 ip=_floor(p);
  p-=ip; 
  float3 s=to_float3(7, 157, 113);
  float4 h=to_float4(0.0f, swi2(s,y,z), s.y+s.z)+dot(ip, s);
  p=p*p*(3.0f-2.0f*p); 
  h=_mix(fract(_sinf(h)*43758.5f), fract(_sinf(h+s.x)*43758.5f), p.x);
  h.xy=_mix(swi2(h,x,z), swi2(h,y,w), p.y);
  return _mix(h.x, h.y, p.z);
}
__DEVICE__ float fastFBMneg(float3 p)
{
  return -1.0f+2.0f*fastFBM(p);
}
__DEVICE__ float2 pModPolar(float2 p, float repetitions) {
  float angle = 2.0f*PI/repetitions;
  float a = _atan2f(p.y, p.x) + angle/2.0f;
  float r = length(p);
  float c = _floor(a/angle);
  a = mod_f(a, angle) - angle/2.0f;
  p = to_float2(_cosf(a), _sinf(a))*r;
  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
  return p;
}

__DEVICE__ float pModInterval1(inout float p, float size, float start, float stop) {
  float halfsize = size*0.5f;
  float c = _floor((p + halfsize)/size);
  p = mod_f(p+halfsize, size) - halfsize;
  if (c > stop) {
    p += size*(c - stop);
    c = stop;
  }
  if (c <start) {
    p += size*(c - start);
    c = start;
  }
  return c;
}
__DEVICE__ float pMirror (inout float p, float dist) {
  float s = sgn(p);
  p = _fabs(p)-dist;
  return s;
}

__DEVICE__ float fCylinder(float3 p, float r, float height) {
  float d = length(swi2(p,x,y)) - r;
  d = _fmaxf(d, _fabs(p.z) - height);
  return d;
}
__DEVICE__ float sdEllipsoid( float3 p, float3 r )
{
  return (length( p/swi3(r,x,y,z) ) - 1.0f) * r.y;
}
__DEVICE__ float sdHexPrism( float3 p, float2 h )
{
  float3 q = _fabs(p);
  return _fmaxf(q.y-h.y, _fmaxf((q.z*0.866025f+q.x*0.5f), q.x)-h.x);
}
__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 d = _fabs(p) - b;
  return _fminf(max(d.x, _fmaxf(d.y, d.z)), 0.0f) + length(_fmaxf(d, 0.0f));
}
__DEVICE__ float fSphere(float3 p, float r) {
  return length(p) - r;
}

__DEVICE__ float GetExplosionIntensity(Explosion ex)
{
  return _mix(1.0f, 0.0f, smoothstep(0.0f, 5.0f, distance(ex.life, 5.0f)));
}

__DEVICE__ float NoTreeZone(float3 p)
{
  float dist = distance(readRGB(to_int2(140, 0)).xz, swi2(p,x,z));
  dist = _fminf(dist, distance(readRGB(to_int2(142, 0)).xz, swi2(p,x,z)));
  dist = _fminf(dist, distance(readRGB(to_int2(144, 0)).xz, swi2(p,x,z)));
  dist = _fminf(dist, distance(readRGB(to_int2(146, 0)).xz, swi2(p,x,z)));
  dist = _fminf(dist, distance(readRGB(to_int2(148, 0)).xz, swi2(p,x,z)));
  return dist;
}
__DEVICE__ float GetTerrainHeight( float3 p)
{
  float2 p2 = (swi2(p,x,z)+swi2(planePos,x,z))*0.0005f;

  float heightDecrease = _mix(1.0f, 0.0f, smoothstep(0.0f, 15.0f, NoTreeZone(p+planePos)));

  float mainHeight = -2.3f+fastFBM((p+to_float3(planePos.x, 0.0f, planePos.z))*0.025f)*_fmaxf(11.0f, _fabs(22.0f*noise2D(p2))); 
  mainHeight-=heightDecrease;

  float terrainHeight=mainHeight;
  p2*=4.0f;
  terrainHeight += textureLod( iChannel3, p2, 2.7f ).x*1.0f; 
  p2*=2.0f;
  terrainHeight -= textureLod( iChannel3, p2, 1.2f ).x*0.7f;
  p2*=3.0f;
  terrainHeight -= textureLod( iChannel3, p2, 0.5f ).x*0.1f;

  terrainHeight=_mix(terrainHeight, mainHeight*1.4f, smoothstep(1.5f, 3.5f, terrainHeight)); 

  return   terrainHeight;
}

__DEVICE__ float GetTreeHeight( float3 p, float terrainHeight)
{
  if (NoTreeZone(p+planePos)<25.0f) return 0.0f;
  float treeHeight = textureLod(iChannel3, (swi2(p,x,z)+swi2(planePos,x,z))*0.006f, 0.1f).x;
  float tree = _mix(0.0f, _mix(0.0f, _mix(0.0f, 2.0f, smoothstep(0.3f, 0.86f, treeHeight)), smoothstep(1.5f, 3.5f, terrainHeight)), step(0.3f, treeHeight)); 
  tree -= tree*0.75f;
  tree*=4.0f;

  return  tree;
}

__DEVICE__ float MapTerrainSimple( float3 p)
{
  float terrainHeight = GetTerrainHeight(p);   
  return  p.y - _fmaxf((terrainHeight+GetTreeHeight(p, terrainHeight)), 0.0f);
}

__DEVICE__ float GetStoneHeight(float3 p, float terrainHeight)
{
  return (textureLod(iChannel1, (swi2(p,x,z)+swi2(planePos,x,z))*0.05f, 0.0f).x*_fmaxf(0.0f, -0.3f+(1.25f*terrainHeight)));
}

__DEVICE__ float MapTerrain( float3 p)
{   
  float terrainHeight = GetTerrainHeight(p);   
  terrainHeight= _mix(terrainHeight+GetStoneHeight(p, terrainHeight), terrainHeight, smoothstep(0.0f, 1.5f, terrainHeight));
  terrainHeight= _mix(terrainHeight+(textureLod(iChannel1, (swi2(p,x,z)+swi2(planePos,x,z))*0.0015f, 0.0f).x*_fmaxf(0.0f, -0.3f+(0.5f*terrainHeight))), terrainHeight, smoothstep(1.2f, 12.5f, terrainHeight));

  terrainHeight= _mix(terrainHeight-0.30f, terrainHeight, smoothstep(-0.5f, 0.25f, terrainHeight));
  float water=0.0f;
  if (terrainHeight<=0.0f)
  {   
    water = (-0.5f+(0.5f*(noise2D((swi2(p,x,z)+swi2(planePos,x,z)+ to_float2(-iTime*0.4f, iTime*0.25f))*2.60f, WATER_LOD))));
    water*=(-0.5f+(0.5f*(noise2D((swi2(p,x,z)+swi2(planePos,x,z)+ to_float2(iTime*0.3f, -iTime*0.25f))*2.90f), WATER_LOD)));
  }
  return   p.y -  _fmaxf((terrainHeight+GetTreeHeight(p, terrainHeight)), -water*0.04f);
}


__DEVICE__ float MapTree( float3 p)
{  
  float terrainHeight = GetTerrainHeight(p);
  float treeHeight =GetTreeHeight(p, terrainHeight);

  // get terrain height at position and tree height onto that
  return  p.y - terrainHeight-treeHeight;
}

__DEVICE__ float3 calcTreeNormal( in float3 pos )
{    
  return normalize( to_float3(MapTree(pos+swi3(eps,x,y,y)) - MapTree(pos-swi3(eps,x,y,y)), 0.5f*2.0f*eps.x, MapTree(pos+swi3(eps,y,y,x)) - MapTree(pos-swi3(eps,y,y,x)) ) );
}

__DEVICE__ float4 TraceTrees( float3 origin, float3 direction, int steps, float terrainHeight)
{
  float4 treeCol =to_float4(0.5f, 0.5f, 0.5f, 0.0f);
  float intensity=0.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos, nn;
  float precis=0.0f, dif =0.0f, densAdd =0.0f;
  float treeHeight = 0.0f;
  float td =0.0f;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    treeHeight = GetTreeHeight(rayPos, terrainHeight);
    dist = rayPos.y - (terrainHeight + treeHeight);  
    precis = 0.015f*t;

    if (treeHeight>0.1f && dist<precis)
    {
      nn= calcTreeNormal(rayPos);  
      dif = clamp( dot( nn, sunPos ), 0.0f, 1.0f );

      densAdd = (precis-dist)*3.0f*td;
      swi3(treeCol,x,y,z)+=(0.5f*td)*dif;
      treeCol.w+=(1.0f-treeCol.w)*densAdd;
    } 
    if (treeCol.w > 0.99f) 
    {
      break;
    }
    td = _fmaxf(0.04f, dist*0.5f);
    t+=td;
  }

  return clamp(treeCol, 0.0f, 1.0f);
}


RayHit TraceTerrainReflection( float3 origin, float3 direction, int steps)
{
  RayHit result;
  float precis = 0.0f, maxDist = 100.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapTerrainSimple( rayPos);
    precis = 0.01f*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5f;
  }

  return result;
}

RayHit TraceTerrain( float3 origin, float3 direction, int steps)
{
  RayHit result;
  float precis = 0.0f, maxDist = 400.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapTerrain( rayPos);
    precis = 0.001f*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5f;
  }

  return result;
}

__DEVICE__ float SoftShadow( in float3 origin, in float3 direction )
{
  float res = 2.0f, t = 0.0f, h;
  for ( int i=0; i<16; i++ )
  {
    h = MapTerrain(origin+direction*t);
    res = _fminf( res, 3.5f*h/t );
    t += clamp( h, 0.02f, 0.8f);
    if ( h<0.002f ) break;
  }
  return clamp( res, 0.0f, 1.0f );
}


__DEVICE__ float3 calcNormal( in float3 pos )
{    
  return normalize( to_float3(MapTerrain(pos+swi3(eps,x,y,y)) - MapTerrain(pos-swi3(eps,x,y,y)), 0.5f*2.0f*eps.x, MapTerrain(pos+swi3(eps,y,y,x)) - MapTerrain(pos-swi3(eps,y,y,x)) ) );
}

__DEVICE__ float GetCloudHeight(float3 p)
{    
  float3 p2 = (p+to_float3(planePos.x, 0.0f, planePos.z)+to_float3(cloudPos.x, 0.0f, cloudPos.y))*0.03f;

  float i  = (-0.3f+noise(p2))*4.4f; 
  p2*=2.52f;
  i +=_fabs(noise( p2 ))*1.7f; 
  p2*=2.53f;
  i += noise( p2 )*1.0f; 
  p2*=2.51f;
  i += noise(p2 )*0.5f;
  p2*=4.22f;
  i += noise( p2)*0.2f;
  return i*3.0f;
}

__DEVICE__ float GetCloudHeightBelow(float3 p)
{    
  float3 p2 = (p+to_float3(planePos.x, 0.0f, planePos.z)+to_float3(cloudPos.x, 0.0f, cloudPos.y))*0.03f;

  float i  = (-0.3f+noise(p2))*4.4f; 
  p2*=2.52f;
  i +=noise( p2 )*1.7f; 
  p2*=2.53f;
  i += noise( p2 )*1.0f; 
  p2*=2.51f;
  i += noise(p2 )*0.5f;
  p2*=3.42f;
  i += noise( p2)*0.2f;
  i*=0.5f;
  i-=0.25f*i; 

  return i*5.0f;
}

__DEVICE__ float GetHorizon( float3 p)
{
  return sdEllipsoid(p, to_float3(1000.0f, -CLOUDLEVEL, 1000.0f));
}

__DEVICE__ float MapCloud( float3 p)
{
  return GetHorizon(p) - _fmaxf(-3.0f, (1.3f*GetCloudHeight(p)));
}

__DEVICE__ float4 TraceClouds( float3 origin, float3 direction, float3 skyColor, int steps)
{
  float4 cloudCol=to_float4(skyColor*to_float3(0.65f, 0.69f, 0.72f)*1.3f, 0.0f);
  cloudCol.xyz=_mix(swi3(cloudCol,x,y,z), sunColor, 0.32f);

  float density = 0.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos;
  float precis; 
  float td =0.0f;
  float densAdd;
  float sunDensity;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    density = _fmaxf(-5.0f, 1.7f+(GetCloudHeight(rayPos)*1.3f));
    dist = GetHorizon(rayPos)-(density);

    precis = 0.01f*t;
    if (dist<precis && density>-5.1f)
    {    
      sunDensity = MapCloud(rayPos+sunPos*3.0f);
      densAdd =  _mix(0.0f, 0.5f*(1.0f-cloudCol.w), smoothstep(-5.1f, 4.3f, density));
      swi3(cloudCol,x,y,z)-=clamp((density-sunDensity), 0.0f, 1.0f)*0.06f*sunColor*densAdd;
      swi3(cloudCol,x,y,z) += 0.003f*_fmaxf(0.0f, sunDensity)*density*densAdd;
      

      cloudCol.w+=(1.0f-cloudCol.w)*densAdd;
    } 

    if (cloudCol.w > 0.99f) break; 

    td = _fmaxf(0.12f, dist*0.45f);
    t+=td;
  }

  // mix clouds color with sky color
  float mixValue = smoothstep(100.0f, 620.0f, t);
  swi3(cloudCol,x,y,z) = _mix(swi3(cloudCol,x,y,z), skyColor, mixValue);

  return cloudCol;
}

__DEVICE__ float4 TraceCloudsBelow( float3 origin, float3 direction, float3 skyColor, int steps)
{
  float4 cloudCol=to_float4(to_float3(0.95f, 0.95f, 0.98f)*0.7f, 0.0f);
  cloudCol.xyz=_mix(swi3(cloudCol,x,y,z), sunColor, 0.2f);

  float density = 0.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos;
  float precis; 
  float td =0.0f;
  float energy=1.0f;
  float densAdd=0.0f;
  float sunDensity;

  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    density = clamp(GetCloudHeightBelow(rayPos), 0.0f, 1.0f)*2.0f;          
    dist = -GetHorizon(rayPos);

    precis = 0.015f*t;
    if (dist<precis && density>0.001f)
    {    
      densAdd = 0.14f*density/td;
      sunDensity = clamp(GetCloudHeightBelow(rayPos+sunPos*3.0f), -0.6f, 2.0f)*2.0f; 
      swi3(cloudCol,x,y,z)-=sunDensity*0.02f*cloudCol.w*densAdd; 
      cloudCol.w+=(1.0f-cloudCol.w)*densAdd;

      swi3(cloudCol,x,y,z) += 0.03f*_fmaxf(0.0f, density-sunDensity)*densAdd;

      swi3(cloudCol,x,y,z)+=_mix(to_float3_s(0.0f), to_float3(1.0f, 1.0f, 0.9f)*0.013f, energy)*sunColor;
      energy*=0.96f;
    } 

    if (cloudCol.w > 0.99f) break; 

    td = _fmaxf(1.4f, dist);
    t+=td;
  }
    
  // mix clouds color with sky color
  swi3(cloudCol,x,y,z) = _mix(swi3(cloudCol,x,y,z), to_float3_s(0.97f), smoothstep(100.0f, 960.0f, t)); 
  cloudCol.w = _mix(cloudCol.w, 0.0f, smoothstep(0.0f, 960.0f, t));

  return cloudCol;
}

__DEVICE__ float getTrailDensity( float3 p)
{
  return noise(p*3.0f)*1.0f;
}

__DEVICE__ void TranslateMissilePos(inout float3 p, Missile missile)
{  
  p = p-(missile.pos);  
  p+=missile.origin;
  pR(swi2(p,x,z), missile.orientation.z);
  pR(swi2(p,x,y), -missile.orientation.x +PI);
  p-=missile.origin;
}

__DEVICE__ float2 MapSmokeTrail( float3 p, Missile missile)
{
  TranslateMissilePos(p, missile);
  float spreadDistance = 1.5f;
  p.z+=3.82f;

  // map trail by using mod op and ellipsoids
  float s = pModInterval1(p.z, -spreadDistance, 0.0f, _fminf(12.0f, (missile.pos.z-planePos.z)/spreadDistance));     
  float dist = sdEllipsoid(p+to_float3(0.0f, 0.0f, 0.4f), to_float3(0.6f, 0.6f, 3.0f));   
  dist-= getTrailDensity(p+to_float3_aw(10.0f*s))*0.25f;

  return to_float2(dist, s);
}


__DEVICE__ float4 TraceSmoketrail( float3 origin, float3 direction, int steps, Missile missile)
{
  float4 trailCol =to_float4(0.5f, 0.5f, 0.5f, 0.0f);
  float height = 0.0f, t = 0.0f;
  float2 dist = to_float2_s(0.0f);
  float3 rayPos;
  float precis; 
  float td =0.0f;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    dist = MapSmokeTrail(rayPos, missile);  
    precis = 0.002f*t;
    if (dist.x<precis)
    {     
      swi3(trailCol,x,y,z)+=(0.5f*(getTrailDensity(rayPos+sunPos*0.17f)))*0.03f;

      float densAdd =(precis-dist.x)*0.20f;
      trailCol.w+=(1.0f-trailCol.w)*densAdd/(1.0f+(_powf(dist.y, 2.0f)*0.021f));
    } 

    if (trailCol.w > 0.99f) break; 

    td = _fmaxf(0.04f, dist.x);
    t+=td;
  }

  return clamp(trailCol, 0.0f, 1.0f);
}


__DEVICE__ float MapExplosion( float3 p, Explosion ex)
{ 
  checkPos = (ex.pos)-to_float3(planePos.x, 0.0f, planePos.z); 
  checkPos=p-checkPos;

  float testDist = fSphere(checkPos, 20.0f);
  if (testDist>10.0f)  return testDist;

  float intensity =GetExplosionIntensity(ex);
  float d= fSphere(checkPos, intensity*15.0f);  

  // terrain clipping
  #ifdef EXACT_EXPLOSIONS
    d=_fmaxf(d, -MapTerrain(p));
  #else
    d = _fmaxf(d, -sdBox(checkPos+to_float3(0.0f, 50.0f, 0.0f), to_float3(50.0f, 50.0f, 50.0f)));
  #endif

  // add explosion "noise/flames"
  float displace = fbm(((checkPos) + to_float3(1, -2, -1)*iTime)*0.5f);
  return d + (displace * 1.5f*_fmaxf(0.0f, 4.0f*intensity));
}


RayHit TraceExplosion(in float3 origin, in float3 direction, int steps, Explosion ex)
{
  RayHit result;
  float precis = 0.0f, maxDist = 350.0f, t = 0.0f, dist = 0.0f;
  float3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapExplosion( rayPos, ex);
    precis = 0.01f*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5f;
  }

  return result;
}

// inspired by https://www.shadertoy.com/view/XdfGz8
__DEVICE__ float3 GetExplosionColor(float x)
{
  float3 col1= to_float3(240.0f, 211.0f, 167.0f)/255.0f;
  float3 col2 = to_float3(210.0f, 90.0f, 60.0f)/255.0f;
  float3 col3 = to_float3(84.0f, 20.0f, 13.0f)/255.0f;

  float t = fract(x*3.0f);
  float3 c= _mix(col2, col3, t);
  c= _mix(mix(col1, col2, t), c, step(0.666f, x));
  return _mix(mix(to_float3(4, 4, 4), col1, t), c, step(0.333f, x));
}

__DEVICE__ float3 GetExplosionLight(float specLevel, float3 normal, RayHit rayHit, float3 rayDir, float3 origin)
{                
  float3 reflectDir = reflect( rayDir, normal );

  float3 lightTot = to_float3_s(0.0f);
  float amb = clamp( 0.5f+0.5f*normal.y, 0.0f, 1.0f );
  float dif = clamp( dot( normal, sunPos ), 0.0f, 1.0f );
  float bac = clamp( dot( normal, normalize(to_float3(-sunPos.x, 0.0f, -sunPos.z)) ), 0.0f, 1.0f ) * clamp(1.0f-rayHit.hitPos.y/20.0f, 0.0f, 1.0f);

  float fre = _powf( clamp(1.0f+dot(normal, rayDir), 0.0f, 1.0f), 2.0f );
  specLevel*= _powf(clamp( dot( reflectDir, sunPos ), 0.0f, 1.0f ), 7.0f);
  float skylight = smoothstep( -0.1f, 0.1f, reflectDir.y );

  lightTot += 1.5f*dif*to_float3(1.00f, 0.90f, 0.85f);
  lightTot += 0.50f*skylight*to_float3(0.40f, 0.60f, 0.95f);
  lightTot += 1.00f*specLevel*to_float3(0.9f, 0.8f, 0.7f)*dif;
  lightTot += 0.50f*bac*to_float3(0.25f, 0.25f, 0.25f);
  lightTot += 0.25f*fre*to_float3(1.00f, 1.00f, 1.00f);

  return clamp(lightTot, 0.0f, 10.0f);
}


__DEVICE__ void DrawExplosion(int id, RayHit marchResult, inout float3 color, float3 rayDir, float3 rayOrigin)
{
  Explosion explosion;
  id *= 100;
  explosion.life = read(to_int2(122+id, 0));

  // check if explosion has been spawned
  if (explosion.life>0.0f)
  {  
    explosion.pos = readRGB(to_int2(120+id, 0)); 

    float3 testPoint = explosion.pos-planePos;
    // ensure the explosions starts on ground
    // explosion.pos.y=GetTerrainHeight(testPoint);

    // explosion light flash    
    if (marchResult.hit)
    {
      float intensity = GetExplosionIntensity(explosion);

      float3 testCol = swi3(color,x,y,z)+to_float3(1.0f, 0.59f, 0.28f)*2.5f;
      color.xyz=_mix(swi3(color,x,y,z), _mix(testCol, swi3(color,x,y,z), smoothstep(0.0f, 40.0f*intensity, distance(swi2(testPoint,x,z), marchResult.swi2(hitPos,x,z)))), intensity);
    }

    // trace explosion  
    RayHit exploTest = TraceExplosion(rayOrigin, rayDir, 68, explosion);   
    if (exploTest.hit)
    {
      swi3(color,x,y,z) = GetExplosionColor(clamp(0.5f+((fbm((exploTest.hitPos + to_float3(1, -2, -1)*iTime)*0.5f))), 0.0f, 0.99f));
      swi3(color,x,y,z) = _mix(swi3(color,x,y,z), swi3(color,x,y,z)*0.45f, smoothstep(0.0f, 12.0f, distance(exploTest.hitPos.y, GetTerrainHeight(testPoint))));
    }

    swi3(color,x,y,z) = _mix(swi3(color,x,y,z)*3.0f, swi3(color,x,y,z), smoothstep(0.0f, 12.4f, exploTest.dist));
  }
  ////////////////////////////////////////////////////////////
}

__DEVICE__ float MapFlare( float3 p, Missile missile)
{
  TranslateMissilePos(p, missile);
  return sdEllipsoid( p+ to_float3(0.0f, 0.0f, 2.4f), to_float3(0.05f, 0.05f, 0.15f));
}

__DEVICE__ float TraceEngineFlare(in float3 origin, in float3 direction, Missile missile)
{
  float t = 0.0f;
  float3 rayPos = to_float3_s(0.0f);
  float dist=10000.0f;

  for ( int i=0; i<10; i++ )
  {
    rayPos =origin+direction*t;
    dist = _fminf(dist, MapFlare( rayPos, missile));
    t += dist;
  }

  return dist;
}

__DEVICE__ float MapMissile(float3 p, Missile missile)
{
  float d= fCylinder( p, 0.70f, 1.7f);
  if (d<1.0f)
  {
    d = fCylinder( p, 0.12f, 1.2f);   
    d =_fminf(d, sdEllipsoid( p- to_float3(0, 0, 1.10f), to_float3(0.12f, 0.12f, 1.0f))); 

    checkPos = p;  
    pR(swi2(checkPos,x,y), 0.785f);
    swi2(checkPos,x,y) = pModPolar(swi2(checkPos,x,y), 4.0f);

    d=_fminf(d, sdHexPrism( checkPos-to_float3(0.0f, 0.0f, 0.60f), to_float2(0.50f, 0.01f)));
    d=_fminf(d, sdHexPrism( checkPos+to_float3(0.0f, 0.0f, 1.03f), to_float2(0.50f, 0.01f)));
    d = _fmaxf(d, -sdBox(p+to_float3(0.0f, 0.0f, 3.15f), to_float3(3.0f, 3.0f, 2.0f)));
    d = _fmaxf(d, -fCylinder(p+to_float3(0.0f, 0.0f, 2.15f), 0.09f, 1.2f));
  }
  return d;
}

__DEVICE__ float MapFlyingMissile( float3 p, Missile missile)
{
  TranslateMissilePos(p, missile);  
  // map missile flame
  eFlameDist = _fminf(eFlameDist, sdEllipsoid( p+ to_float3(0.0f, 0.0f, 2.2f+_cosf(iTime*90.0f)*0.23f), to_float3(0.17f, 0.17f, 1.0f)));
  // map missile 
  return _fminf(MapMissile(p, missile), eFlameDist);
}

RayHit TraceMissile(in float3 origin, in float3 direction, int steps, Missile missile)
{
  RayHit result;
  float maxDist = 450.0f;
  float t = 0.0f, glassDist = 0.0f, dist = 100000.0f;
  float3 rayPos;
  eFlameDist=10000.0f;
  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t;
    dist = MapFlyingMissile(rayPos, missile);

    if (dist<0.01f || t>maxDist )
    {                
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));   

      result.eFlameDist = eFlameDist;
      break;
    }
    t += dist;
  }

  return result;
}

__DEVICE__ float SoftShadowMissile( in float3 origin, in float3 direction, Missile missile )
{
  float res = 2.0f, t = 0.02f, h;
  for ( int i=0; i<8; i++ )
  {
    h = MapMissile(origin+direction*t, missile);
    res = _fminf( res, 7.5f*h/t );
    t += clamp( h, 0.05f, 0.2f );
    if ( h<0.001f || t>2.5f ) break;
  }
  return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float3 GetMissileLightning(float specLevel, float3 normal, RayHit rayHit, float3 rayDir, float3 origin, Missile missile)
{       
  float dif = clamp( dot( normal, sunPos ), 0.0f, 1.0f );
  float3 reflectDir = reflect( rayDir, normal );
  specLevel= 3.5f*_powf(clamp( dot( reflectDir, sunPos ), 0.0f, 1.0f ), 9.0f/3.0f);

  float fre = _powf( 1.0f-_fabs(dot( normal, rayDir )), 2.0f );
  fre = _mix( 0.03f, 1.0f, fre );   
  float amb = clamp( 0.5f+0.5f*normal.y, 0.0f, 1.0f );

  float shadow = SoftShadowMissile(origin+((rayDir*rayHit.depth)*0.998f), sunPos, missile);
  dif*=shadow;
  float skyLight = smoothstep( -0.1f, 0.1f, reflectDir.y );

  float3 lightTot = (to_float3_s(0.7f)*amb); 
  lightTot+=to_float3_s(0.85f)*dif;
  lightTot += 1.00f*specLevel*dif;
  lightTot += 0.80f*skyLight*to_float3(0.40f, 0.60f, 1.00f);
  lightTot= _mix(lightTot*0.7f, lightTot*1.2f, fre );

  return lightTot*sunColor;
}

__DEVICE__ float3 calcMissileNormal( in float3 pos, Missile missile )
{    
  return normalize( to_float3(MapFlyingMissile(pos+swi3(eps,x,y,y), missile) - MapFlyingMissile(pos-swi3(eps,x,y,y), missile), 0.5f*2.0f*eps.x, MapFlyingMissile(pos+swi3(eps,y,y,x), missile) - MapFlyingMissile(pos-swi3(eps,y,y,x), missile) ) );
}

__DEVICE__ mat3 setCamera(  float3 ro, float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3_aw(_sinf(cr), _cosf(cr), 0.0f);
  float3 cu = normalize( cross(cw, cp) );
  float3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}

// set sky color tone. 2 gradient passes using MIX.
__DEVICE__ float3 GetSkyColor(float3 rayDir)
{ 
  return _mix(mix(to_float3(0.15f, 0.19f, 0.24f), to_float3(220.0f, 230.0f, 240.0f)/255.0f, smoothstep(1.0f, 0.30f, rayDir.y)), _mix(to_float3(229.0f, 221.0f, 230)/200.0f, sunColor, 0.15f), smoothstep(0.15f, -0.13f, rayDir.y));
}

// scene lightning
__DEVICE__ float3 GetSceneLight(float specLevel, float3 normal, RayHit rayHit, float3 rayDir, float3 origin)
{                
  float3 reflectDir = reflect( rayDir, normal );

  float3 lightTot = to_float3_s(0.0f);
  float amb = clamp( 0.5f+0.5f*normal.y, 0.0f, 1.0f );
  float dif = clamp( dot( normal, sunPos ), 0.0f, 1.0f );
  float bac = clamp( dot( normal, normalize(to_float3(-sunPos.x, 0.0f, -sunPos.z)) ), 0.0f, 1.0f ) * clamp(1.0f-rayHit.hitPos.y/20.0f, 0.0f, 1.0f);
  ;
  float fre = _powf( clamp(1.0f+dot(normal, rayDir), 0.0f, 1.0f), 2.0f );
  specLevel*= _powf(clamp( dot( reflectDir, sunPos ), 0.0f, 1.0f ), 7.0f);
  float skylight = smoothstep( -0.1f, 0.1f, reflectDir.y );

  float shadow=1.0f; 
  #ifdef SHADOWS
    shadow = SoftShadow(origin+((rayDir*rayHit.depth)*0.988f), sunPos);
  #endif

    lightTot += 1.5f*dif*to_float3(1.00f, 0.90f, 0.85f)*shadow;
  lightTot += 0.50f*skylight*to_float3(0.40f, 0.60f, 0.95f);
  lightTot += 1.00f*specLevel*to_float3(0.9f, 0.8f, 0.7f)*dif;
  lightTot += 0.50f*bac*to_float3(0.25f, 0.25f, 0.25f);
  lightTot += 0.25f*fre*to_float3(1.00f, 1.00f, 1.00f)*shadow;

  return clamp(lightTot, 0.0f, 10.0f)*sunColor;
}

__DEVICE__ float3 GetSceneLightWater(float specLevel, float3 normal, RayHit rayHit, float3 rayDir, float3 origin)
{                
  float3 reflectDir = reflect( rayDir, normal );
  float amb = clamp( 0.5f+0.5f*normal.y, 0.0f, 1.0f );
  float dif = clamp( dot( normal, sunPos ), 0.0f, 1.0f );
  float bac = clamp( dot( normal, normalize(to_float3(-sunPos.x, 0.0f, -sunPos.z)) ), 0.0f, 1.0f ) * clamp(1.0f-rayHit.hitPos.y/20.0f, 0.0f, 1.0f);

  specLevel*= _powf(clamp( dot( reflectDir, sunPos ), 0.0f, 1.0f ), 9.0f);

  float skylight = smoothstep( -0.1f, 0.1f, reflectDir.y );
  float fre = _powf( 1.0f-_fabs(dot( normal, rayDir )), 4.0f );
  fre = _mix( 0.03f, 1.0f, fre );   

  float3 reflection = to_float3_s(1.0f);
  float3 lightTot = to_float3_s(0.0f);

  lightTot += 1.15f*dif*to_float3(1.00f, 0.90f, 0.85f);
  lightTot += 1.00f*specLevel*to_float3(0.9f, 0.8f, 0.7f)*dif;    
  lightTot= _mix(lightTot, reflection, fre );
  lightTot += 0.70f*skylight*to_float3(0.70f, 0.70f, 0.85f);
  lightTot += 1.30f*bac*to_float3(0.25f, 0.25f, 0.25f);
  lightTot += 0.25f*amb*to_float3(0.80f, 0.90f, 0.95f);  
  return clamp(lightTot, 0.0f, 10.0f);
}


__DEVICE__ void ApplyFog(inout float3 color, float3 skyColor, float3 rayOrigin, float3 rayDir, float depth)   
{
  float mixValue = smoothstep(50.0f, 15000.0f, _powf(depth, 2.0f)*0.1f);
  float sunVisibility = _fmaxf(0.0f, dot(sunPos, rayDir));
  // horizontal fog
  float3 fogColor = _mix(sunColor*0.7f, skyColor, mixValue);  
  fogColor = _mix(fogColor, sunColor, smoothstep(0.0f, 1.0f, sunVisibility));   
  color = _mix(color, fogColor, mixValue);

  // vertical fog
  float heightAmount = 0.01f;
  float fogAmount = 0.2f * _expf(-rayOrigin.y*heightAmount) * (1.0f-_expf( -depth*rayDir.y*heightAmount ))/rayDir.y;
  color = _mix(color, fogColor, fogAmount);
}



__KERNEL__ void TurnNBurnFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  
  fragCoord+=0.5f;
  float2 R = iResolution;

  float2 mo = iMouse.xy/iResolution;
  float2 uv = fragCoord / iResolution;
  float2 screenSpace = (-iResolution + 2.0f*(fragCoord))/iResolution.y;

  // read plane data from buffer
  turn = read(to_int2(1, 10));
  float roll = read(to_int2(1, 1));
  float speed = read(to_int2(10, 1));
  float pitch = read(to_int2(15, 1));
  sunPos =  readRGB(to_int2(50, 0));
  planePos = readRGB(to_int2(55, 0));
  float CAMZOOM = read(to_int2(52, 0));  

  // setup camera and ray direction
  float2 camRot = swi2(readRGB(to_int2(57, 0)),x,y);

  cloudPos = to_float2(-iTime*0.3f, iTime*0.45f);

  float3 rayOrigin = to_float3_aw(CAMZOOM*_cosf(camRot.x), planePos.y+CAMZOOM*_sinf(camRot.y), -3.0f+CAMZOOM*_sinf(camRot.x) );    
  pR(swi2(rayOrigin,x,z), -turn);
  mat3 ca = setCamera( rayOrigin, to_float3(0.0f, planePos.y, -3.0f ), 0.0f );
  float3 rayDir = ca * normalize( to_float3(swi2(screenSpace,x,y), 2.0f) );

  // create sky color fade
  float3 skyColor = GetSkyColor(rayDir);
  float3 color = skyColor;
  float alpha=0.0f;

  RayHit marchResult = TraceTerrain(rayOrigin, rayDir, 1200);

  // is terrain hit?
  if (marchResult.hit)
  { 

    alpha=1.0f;
    marchResult.normal = calcNormal(marchResult.hitPos);  

    float specLevel=0.7f;
    color=to_float3_s(0.5f);

    // create terrain texture
    float3 colorRocks= to_float3(_mix(texture(iChannel3, (marchResult.swi2(hitPos,x,z)+swi2(planePos,x,z))*0.01f).rgb, texture(iChannel3, (marchResult.swi2(hitPos,x,z)+to_float2(10000.0f, 10000.0f)+swi2(planePos,x,z))*0.01f).rgb, fastFBM(marchResult.hitPos)));
    color =colorRocks;
    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), color*3.0f, _fabs(noise2D((marchResult.swi2(hitPos,x,z)+swi2(planePos,x,z))*0.4f, 1.0f))); 

    // grass
    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), ((color+noise2D((marchResult.swi2(hitPos,x,z)+swi2(planePos,x,z))*24.0f, 1.0f))+to_float3(0.5f, 0.4f, 0.1f))*0.3f, smoothstep(0.2f, 2.0f, marchResult.hitPos.y)); 

    float stoneHeight = GetStoneHeight(marchResult.hitPos, (GetTerrainHeight(marchResult.hitPos)));     
    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), to_float3(0.5f+(noise(marchResult.hitPos+to_float3(planePos.x, 0.0f, planePos.z))*0.3f)), smoothstep(1.0f, 0.0f, stoneHeight));
    specLevel = _mix(specLevel, specLevel*2.6f, smoothstep(1.0f, 0.0f, stoneHeight));

    // beach
    swi3(color,x,y,z) = _mix((color+to_float3(1.2f, 1.1f, 1.0f))*0.5f, swi3(color,x,y,z), smoothstep(0.3f, 0.7f, marchResult.hitPos.y)); 


    float burn = NoTreeZone(marchResult.hitPos+planePos);
    color=_mix(color*0.1f, color, smoothstep(0.0f, 25.0f, burn));

    // create slight wave difference between water and beach level
    float wave = _fmaxf(0.0f, _cosf(_fabs(noise2D((marchResult.swi2(hitPos,x,z)+swi2(planePos,x,z))))+(iTime*0.5f)+(length(marchResult.swi2(hitPos,x,z))*0.03f))*0.09f);

    float3 light;
    // check if terrain is below water level
    if (marchResult.hitPos.y<0.3f+wave)
    {
      float3 terrainHit = rayOrigin+((rayDir*marchResult.depth)*0.998f);
      float3 refDir = reflect(rayDir, marchResult.normal);
      float4 testClouds = TraceCloudsBelow(terrainHit, refDir, skyColor, 30);

      color = to_float3_s(0.3f);

      float sunVisibility = _fmaxf(0.0f, dot(sunPos, rayDir));

      // calculate water fresnel  
      float dotNormal = dot(rayDir, marchResult.normal);
      float fresnel = _powf(1.0f-_fabs(dotNormal), 4.0f);  
      float3 rayRef = rayDir-marchResult.normal*dotNormal;

      //swi3(color,x,y,z)  = _mix(mix(to_float3_s(1.0f), (to_float3_s(0.7f)+sunColor)*1.50f, smoothstep(150.0f, 350.0f,marchResult.depth)), swi3(color,x,y,z), smoothstep(-TERRAINLEVEL-0.37f, -TERRAINLEVEL+0.25f, marchResult.hitPos.y));
      swi3(color,x,y,z)  = _mix(color*0.7f, swi3(color,x,y,z), smoothstep(-3.0f, -0.15f, marchResult.hitPos.y));

      color = color+(sunColor*_powf(sunVisibility, 5.0f));

      // sea color
      color = _mix(mix(color, color+fresnel, fresnel ), color, smoothstep(-0.1f, 0.15f, marchResult.hitPos.y));

      float3 reflection = color;

      #ifdef QUALITY_REFLECTIONS
        // cast rays from water surface onto terrain. If terrain is hit, color water dark in these areas.
        RayHit reflectResult = TraceTerrainReflection(terrainHit, refDir, 100); 

      if (reflectResult.hit==true)
      {
        reflection  = _mix(color, to_float3(0.01f, 0.03f, 0.0f), 0.9f);
      }
      #endif
        light = GetSceneLightWater(specLevel, marchResult.normal, marchResult, rayDir, rayOrigin);   
      color=_mix(mix(swi3(color,x,y,z), swi3(testClouds,x,y,z), testClouds.w*0.26f), _mix(swi3(color,x,y,z), swi3(testClouds,x,y,z), testClouds.w), smoothstep(0.0f, 0.7f, fresnel)); 
      color=_mix(mix(swi3(color,x,y,z), reflection, 0.5f), reflection, smoothstep(0.0f, 0.7f, fresnel)); 
      color=_mix(color, color+(0.5f*fresnel), smoothstep(0.0f, 0.3f, fresnel)); 

      color=color*light;
      color = _mix(color, skyColor, smoothstep(320.0f, 400.0f, marchResult.depth));
    } 
    // terrain is ABOVE water level  
    else
    {
      // get lightning based on material
      light = GetSceneLight(specLevel, marchResult.normal, marchResult, rayDir, rayOrigin);   

      // apply lightning
      color = color*light;

      #ifdef QUALITY_TREE
        // add trees
        float4 treeColor = TraceTrees(rayOrigin, rayDir, 28, marchResult.hitPos.y-0.3f );      
      color =clamp( _mix( color, swi3(treeColor,x,y,z)*((noise2D((marchResult.swi2(hitPos,x,z)+swi2(planePos,x,z))*36.0f, 3.0f)+to_float3(0.56f, 0.66f, 0.45f))*0.6f)*sunColor*(0.30f+(0.6f*light)), treeColor.w ), 0.02f, 1.0f); 
      #endif
    }

    color = _mix(color, (color+sunColor)*0.6f, smoothstep(70.0f, 300.0f, marchResult.depth));
    // add haze when high above ground  
    color = _mix(color, color+to_float3(0.37f, 0.58f, 0.9f)*sunColor, _mix(0.0f, 0.75f, smoothstep(-CLOUDLEVEL*0.65f, MAX_HEIGHT, planePos.y)));  
    ApplyFog(color, skyColor, rayOrigin, rayDir, marchResult.depth);
  } else
  {
    // add volumetric clouds 
    // below cloud level
    if (rayOrigin.y<-CLOUDLEVEL && rayDir.y>0.0f)
    {  
      float4 cloudColor=TraceCloudsBelow(rayOrigin, rayDir, skyColor, 60);    

      // make clouds slightly light near the sun
      float sunVisibility = _powf(_fmaxf(0.0f, dot(sunPos, rayDir)), 2.0f)*0.10f;
      swi3(color,x,y,z) = _mix(swi3(color,x,y,z), _fmaxf(to_float3_s(0.0f), swi3(cloudColor,x,y,z)+sunVisibility), cloudColor.w);      
      //swi3(color,x,y,z) = _mix(swi3(color,x,y,z), swi3(cloudColor,x,y,z), cloudColor.w);       
      alpha+=cloudColor.w*0.86f;
    }
  }

  // add volumetric clouds 
  // above cloud level
  if (rayOrigin.y>=-CLOUDLEVEL)
  {  
    float4 cloudColor=TraceClouds(rayOrigin, rayDir, skyColor, 80);    
    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), swi3(cloudColor,x,y,z), cloudColor.w);
  }

  rayDir = ca * normalize( to_float3(swi2(screenSpace,x,y), 2.0f) );
  DrawExplosion(0, marchResult, color, rayDir, rayOrigin);
  DrawExplosion(1, marchResult, color, rayDir, rayOrigin);


  // #################################################################### //    
  // ##############             MISSILES             #################### //     
  // #################################################################### //    

  rayOrigin = to_float3_aw(CAMZOOM*_cosf(camRot.x), CAMZOOM*_sinf(camRot.y), CAMZOOM*_sinf(camRot.x) );
  pR(swi2(rayOrigin,x,z), -turn);
  ca = setCamera( rayOrigin, to_float3(0.0f, 0.0f, 0.0f ), 0.0f );
  rayDir = ca * normalize( to_float3(swi2(screenSpace,x,y), 2.0f) );

  int adressStep = 0;
  Missile missile;
  for (int i=0; i<2; i++)
  {
    adressStep = i*100;
    missile.life = read(to_int2(100 + adressStep, 0));
    // check if missile is launched
    if (missile.life>0.0f)
    {
      missile.origin = to_float3(4.8f - (9.6f*float(i)), -0.4f, -3.0f);       
      missile.orientation = readRGB(to_int2(108+adressStep, 0));
      missile.pos = readRGB(to_int2(116+adressStep, 0));

      // calculate engine flare
      float lightDist = TraceEngineFlare(rayOrigin, rayDir, missile);

      // add engine flares for missiles based on engine distance
      float3 lightFlares=to_float3_s(0.0f);
      lightFlares =  _mix((to_float3(1.0f, 0.4f, 0.2f)), to_float3_s(0.0f), smoothstep(0.0f, 1.1f, lightDist));             
      lightFlares =  _mix(lightFlares+(2.0f*to_float3(1.0f, 0.5f, 0.2f)), lightFlares, smoothstep(0.0f, 0.7f, lightDist));
      lightFlares =  _mix(lightFlares+to_float3(1.0f, 1.0f, 1.0f), lightFlares, smoothstep(0.0f, 0.2f, lightDist));

      // rayTrace missile
      RayHit marchResult = TraceMissile(rayOrigin, rayDir, 64, missile);

      // apply color and lightning to missile if hit in raymarch test    
      if (marchResult.hit)
      {
        marchResult.normal = calcMissileNormal(marchResult.hitPos, missile);  

        // create texture map and set specular levels
        float4 col = to_float4(0.45f, 0.45f, 0.45f, 0.8f);

        // flame
        col.xyz=_mix(swi3(col,x,y,z), to_float3(1.2f, 0.55f, 0.30f)*2.5f, smoothstep(0.16f, 0.0f, marchResult.eFlameDist));

        // get lightning based on material
        float3 light = GetMissileLightning(col.w, marchResult.normal, marchResult, rayDir, rayOrigin, missile);   

        // apply lightning
        swi3(color,x,y,z) = swi3(col,x,y,z)*light;

        alpha = 1.0f; 

        lightFlares = _mix(lightFlares, to_float3_s(0.0f), step(0.1f, distance(marchResult.dist, marchResult.eFlameDist)));
      }

      swi3(color,x,y,z)+=lightFlares;

      //draw smoke trail behind missile
      float4 trailColor = TraceSmoketrail(rayOrigin, rayDir, 48, missile);     
      swi3(color,x,y,z) = _mix(swi3(color,x,y,z), swi3(trailColor,x,y,z), trailColor.w);
      alpha+=trailColor.w;   

      if (marchResult.hit) 
      { 
        break;
      }
    }
  }
  // #################################################################### //
  // #################################################################### //

  fragColor = to_float4(swi3(color,x,y,z), _fminf(1.0f, alpha));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer C 'Cubemap: Forest Blurred_0' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel2
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


//////////////////////////////////////////////////////////////////////////////////////
// PLANE BUFFER   -   RENDERS PLANE ONLY
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Buffer B. Get the colors of the terrain buffer render.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = Buffer A. Read data from data-buffer.
// Channel 3 = Forest blurred cube map. Used in reflections in plane window and hull.

float2 R;

  #pragma optimize(off) 
#define PI _acosf(-1.0f)
  //#define read(memPos)    (  texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos) (  texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (to_float2(memPos)+0.5f)/R).w)
  #define readRGB(memPos)   (  texture(iChannel2, (to_float2(memPos)+0.5f)/R).xyz)
  
  
  #define RAYSTEPS 300
  #define CLOUDLEVEL -70.0
  float turn=0.0f, pitch = 0.0f, roll=0.0f, rudderAngle = 0.0f;
float speed = 0.5f;
float3 checkPos=to_float3_s(0.0f);
float3 sunPos=to_float3_s(0.0f);
const float3 sunColor = to_float3(1.00f, 0.90f, 0.85f);
float3 planePos=to_float3_s(0.0f);
const float3 eps = to_float3(0.02f, 0.0f, 0.0f);

float winDist=10000.0f;
float engineDist=10000.0f;
float eFlameDist=10000.0f;
float blackDist=10000.0f;
float bombDist=10000.0f;
float bombDist2=10000.0f;
float missileDist=10000.0f;
float frontWingDist=10000.0f;
float rearWingDist=10000.0f;
float topWingDist=10000.0f;
float2 missilesLaunched=to_float2_s(0.0f);

__DEVICE__ float sgn(float x) 
{   
  return (x<0.0f)?-1.:1.;
}

struct RayHit
{
  bool hit;  
  float3 hitPos;
  float3 normal;
  float dist;
  float depth;

  float winDist;
  float engineDist;
  float eFlameDist;
  float blackDist;
  float bombDist;
  float bombDist2;
  float missileDist;
  float frontWingDist;
  float rearWingDist;
  float topWingDist;
};

__DEVICE__ float noise2D( in float2 pos, float lod)
{   
  float2 f = fract(pos);
  f = f*f*(3.0f-2.0f*f);
  float2 rg = textureLod( iChannel1, (((_floor(pos).xy+to_float2(37.0f, 17.0f)) + swi2(f,x,y))+ 0.5f)/64.0f, lod).yx;  
  return -1.0f+2.0f*_mix( rg.x, rg.y, 0.5f );
}
__DEVICE__ float noise2D( in float2 pos )
{
  return noise2D(pos, 0.0f);
}

__DEVICE__ float noise( in float3 x )
{
  float3 p = _floor(x);
  float3 f = fract(x);

  float a = textureLod( iChannel1, x.xy/64.0f + (p.z+0.0f)*120.7123f, 0.1f ).x;
  float b = textureLod( iChannel1, x.xy/64.0f + (p.z+1.0f)*120.7123f, 0.1f ).x;
  return _mix( a, b, f.z );
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 d = _fabs(p) - b;
  return _fminf(max(d.x, _fmaxf(d.y, d.z)), 0.0f) + length(_fmaxf(d, 0.0f));
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,z))-t.x, p.y);
  return length(q)-t.y;
}

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa, ba)/dot(ba, ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float sdEllipsoid( float3 p, float3 r )
{
  return (length( p/swi3(r,x,y,z) ) - 1.0f) * r.y;
}

__DEVICE__ float sdConeSection( float3 p, float h, float r1, float r2 )
{
  float d1 = -p.z - h;
  float q = p.z - h;
  float si = 0.5f*(r1-r2)/h;
  float d2 = _fmaxf( _sqrtf( dot(swi2(p,x,y), swi2(p,x,y))*(1.0f-si*si)) + q*si - r2, q );
  return length(_fmaxf(to_float2(d1, d2), 0.0f)) + _fminf(max(d1, d2), 0.0f);
}

__DEVICE__ float fCylinder(float3 p, float r, float height) {
  float d = length(swi2(p,x,y)) - r;
  d = _fmaxf(d, _fabs(p.z) - height);
  return d;
}
__DEVICE__ float fSphere(float3 p, float r) {
  return length(p) - r;
}

__DEVICE__ float sdHexPrism( float3 p, float2 h )
{
  float3 q = _fabs(p);
  return _fmaxf(q.y-h.y, _fmaxf((q.z*0.866025f+q.x*0.5f), q.x)-h.x);
}

__DEVICE__ float fOpPipe(float a, float b, float r) {
  return length(to_float2(a, b)) - r;
}

__DEVICE__ float2 pModPolar(float2 p, float repetitions) {
  float angle = 2.0f*PI/repetitions;
  float a = _atan2f(p.y, p.x) + angle/2.0f;
  float r = length(p);
  float c = _floor(a/angle);
  a = mod_f(a, angle) - angle/2.0f;
  p = to_float2(_cosf(a), _sinf(a))*r;
  if (_fabs(c) >= (repetitions/2.0f)) c = _fabs(c);
  return p;
}

__DEVICE__ float pModInterval1(inout float p, float size, float start, float stop) {
  float halfsize = size*0.5f;
  float c = _floor((p + halfsize)/size);
  p = mod_f(p+halfsize, size) - halfsize;
  if (c > stop) {
    p += size*(c - stop);
    c = stop;
  }
  if (c <start) {
    p += size*(c - start);
    c = start;
  }
  return c;
}

__DEVICE__ float pMirror (inout float p, float dist) {
  float s = sgn(p);
  p = _fabs(p)-dist;
  return s;
}

__DEVICE__ mat2 r2(float r)
{
  float c=_cosf(r), s=_sinf(r);
  return mat2(c, s, -s, c);
}

#define r3(r) mat2(_sinf(to_float4(-1, 0, 0, 1)*_acosf(0.0f)+r))
  __DEVICE__ void pR(inout float2 p, float a) 
{
  p*=r2(a);
}

__DEVICE__ float fOpUnionRound(float a, float b, float r) {
  float2 u = _fmaxf(to_float2(r - a, r - b), to_float2(0));
  return _fmaxf(r, _fminf (a, b)) - length(u);
}

__DEVICE__ float fOpIntersectionRound(float a, float b, float r) {
  float2 u = _fmaxf(to_float2(r + a, r + b), to_float2(0));
  return _fminf(-r, _fmaxf (a, b)) + length(u);
}

// limited by euler rotation. I wont get a good plane rotation without quaternions! :-(
__DEVICE__ float3 TranslatePos(float3 p, float _pitch, float _roll)
{
  pR(swi2(p,x,y), _roll-PI);
  p.z+=5.0f;
  pR(swi2(p,z,y), _pitch);
  p.z-=5.0f; 
  return p;
}

__DEVICE__ float MapEsmPod(float3 p)
{
  float dist = fCylinder( p, 0.15f, 1.0f);   
  checkPos =  p- to_float3(0, 0, -1.0f);
  pModInterval1(checkPos.z, 2.0f, 0.0f, 1.0f);
  return _fminf(dist, sdEllipsoid(checkPos, to_float3(0.15f, 0.15f, 0.5f)));
}

__DEVICE__ float MapMissile(float3 p)
{
  float d= fCylinder( p, 0.70f, 1.7f);
  if (d<1.0f)
  {
    missileDist = _fminf(missileDist, fCylinder( p, 0.12f, 1.2f));   
    missileDist =_fminf(missileDist, sdEllipsoid( p- to_float3(0, 0, 1.10f), to_float3(0.12f, 0.12f, 1.0f))); 

    checkPos = p;  
    pR(swi2(checkPos,x,y), 0.785f);
    swi2(checkPos,x,y) = pModPolar(swi2(checkPos,x,y), 4.0f);

    missileDist=_fminf(missileDist, sdHexPrism( checkPos-to_float3(0.0f, 0.0f, 0.60f), to_float2(0.50f, 0.01f)));
    missileDist=_fminf(missileDist, sdHexPrism( checkPos+to_float3(0.0f, 0.0f, 1.03f), to_float2(0.50f, 0.01f)));
    missileDist = _fmaxf(missileDist, -sdBox(p+to_float3(0.0f, 0.0f, 3.15f), to_float3(3.0f, 3.0f, 2.0f)));
    missileDist = _fmaxf(missileDist, -fCylinder(p+to_float3(0.0f, 0.0f, 2.15f), 0.09f, 1.2f));
  }
  return missileDist;
}

__DEVICE__ float MapFrontWing(float3 p, float mirrored)
{
  missileDist=10000.0f;

  checkPos = p;
  pR(swi2(checkPos,x,y), -0.02f);
  float wing =sdBox( checkPos- to_float3(4.50f, 0.25f, -4.6f), to_float3(3.75f, 0.04f, 2.6f)); 

  if (wing<5.0f) //Bounding Box test
  {
    // cutouts
    checkPos = p-to_float3(3.0f, 0.3f, -0.30f);
    pR(swi2(checkPos,x,z), -0.5f);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, to_float3(6.75f, 1.4f, 2.0f)), 0.1f);

    checkPos = p - to_float3(8.0f, 0.3f, -8.80f);
    pR(swi2(checkPos,x,z), -0.05f);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, to_float3(10.75f, 1.4f, 2.0f)), 0.1f);

    checkPos = p- to_float3(9.5f, 0.3f, -8.50f);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, to_float3(2.0f, 1.4f, 6.75f)), 0.6f);

    // join wing and engine
    wing=_fminf(wing, sdCapsule(p- to_float3(2.20f, 0.3f, -4.2f), to_float3(0, 0, -1.20f), to_float3(0, 0, 0.8f), 0.04f));
    wing=_fminf(wing, sdCapsule(p- to_float3(3.0f, 0.23f, -4.2f), to_float3(0, 0, -1.20f), to_float3(0, 0, 0.5f), 0.04f));    

    checkPos = p;
    pR(swi2(checkPos,x,z), -0.03f);
    wing=_fminf(wing, sdConeSection(checkPos- to_float3(0.70f, -0.1f, -4.52f), 5.0f, 0.25f, 0.9f));   

    checkPos = p;
    pR(swi2(checkPos,y,z), 0.75f);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos- to_float3(3.0f, -0.5f, 1.50f), to_float3(3.75f, 3.4f, 2.0f)), 0.12f); 
    pR(swi2(checkPos,y,z), -1.95f);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos- to_float3(2.0f, 0.70f, 2.20f), to_float3(3.75f, 3.4f, 2.0f)), 0.12f); 

    checkPos = p- to_float3(0.47f, 0.0f, -4.3f);
    pR(swi2(checkPos,y,z), 1.57f);
    wing=_fminf(wing, sdTorus(checkPos-to_float3(0.0f, -3.0f, 0.0f), to_float2(0.3f, 0.05f)));   

    // flaps
    wing =_fmaxf(wing, -sdBox( p- to_float3(3.565f, 0.1f, -6.4f), to_float3(1.50f, 1.4f, 0.5f)));
    wing =_fmaxf(wing, -_fmaxf(sdBox( p- to_float3(5.065f, 0.1f, -8.4f), to_float3(0.90f, 1.4f, 2.5f)), -sdBox( p- to_float3(5.065f, 0.0f, -8.4f), to_float3(0.89f, 1.4f, 2.49f))));

    checkPos = p- to_float3(3.565f, 0.18f, -6.20f+0.30f);
    pR(swi2(checkPos,y,z), -0.15f+(0.8f*pitch));
    wing =_fminf(wing, sdBox( checkPos+to_float3(0.0f, 0.0f, 0.30f), to_float3(1.46f, 0.007f, 0.3f)));

    // missile holder
    float holder = sdBox( p- to_float3(3.8f, -0.26f, -4.70f), to_float3(0.04f, 0.4f, 0.8f));

    checkPos = p;
    pR(swi2(checkPos,y,z), 0.85f);
    holder=_fmaxf(holder, -sdBox( checkPos- to_float3(2.8f, -1.8f, -3.0f), to_float3(1.75f, 1.4f, 1.0f))); 
    holder=_fmaxf(holder, -sdBox( checkPos- to_float3(2.8f, -5.8f, -3.0f), to_float3(1.75f, 1.4f, 1.0f))); 
    holder =fOpUnionRound(holder, sdBox( p- to_float3(3.8f, -0.23f, -4.70f), to_float3(1.0f, 0.03f, 0.5f)), 0.1f); 

    // bomb
    bombDist = fCylinder( p- to_float3(3.8f, -0.8f, -4.50f), 0.35f, 1.0f);   
    bombDist =_fminf(bombDist, sdEllipsoid( p- to_float3(3.8f, -0.8f, -3.50f), to_float3(0.35f, 0.35f, 1.0f)));   
    bombDist =_fminf(bombDist, sdEllipsoid( p- to_float3(3.8f, -0.8f, -5.50f), to_float3(0.35f, 0.35f, 1.0f)));   

    // missiles
    checkPos = p-to_float3(2.9f, -0.45f, -4.50f);

    // check if any missile has been fired. If so, do NOT mod missile position  
    float maxMissiles =0.0f; 
    if (mirrored>0.0f) maxMissiles =  _mix(1.0f, 0.0f, step(1.0f, missilesLaunched.x));
    else maxMissiles =  _mix(1.0f, 0.0f, step(1.0f, missilesLaunched.y)); 

    pModInterval1(checkPos.x, 1.8f, 0.0f, maxMissiles);
    holder = _fminf(holder, MapMissile(checkPos));

    // ESM Pod
    holder = _fminf(holder, MapEsmPod(p-to_float3(7.2f, 0.06f, -5.68f)));

    // wheelholder
    wing=_fminf(wing, sdBox( p- to_float3(0.6f, -0.25f, -3.8f), to_float3(0.8f, 0.4f, 0.50f)));

    wing=_fminf(bombDist, _fminf(wing, holder));
  }

  return wing;
}

__DEVICE__ float MapRearWing(float3 p)
{
  float wing2 =sdBox( p- to_float3(2.50f, 0.1f, -8.9f), to_float3(1.5f, 0.017f, 1.3f)); 
  if (wing2<0.15f) //Bounding Box test
  {
    // cutouts
    checkPos = p-to_float3(3.0f, 0.0f, -5.9f);
    pR(swi2(checkPos,x,z), -0.5f);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, to_float3(6.75f, 1.4f, 2.0f)), 0.2f); 

    checkPos = p-to_float3(0.0f, 0.0f, -4.9f);
    pR(swi2(checkPos,x,z), -0.5f);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, to_float3(3.3f, 1.4f, 1.70f)), 0.2f);

    checkPos = p-to_float3(3.0f, 0.0f, -11.70f);
    pR(swi2(checkPos,x,z), -0.05f);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, to_float3(6.75f, 1.4f, 2.0f)), 0.1f); 

    checkPos = p-to_float3(4.30f, 0.0f, -11.80f);
    pR(swi2(checkPos,x,z), 1.15f);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, to_float3(6.75f, 1.4f, 2.0f)), 0.1f);
  }
  return wing2;
} 

__DEVICE__ float MapTailFlap(float3 p, float mirrored)
{
  p.z+=0.3f;
  pR(swi2(p,x,z), rudderAngle*(-1.0f*mirrored)); 
  p.z-=0.3f;

  float tailFlap =sdBox(p- to_float3(0.0f, -0.04f, -0.42f), to_float3(0.025f, 0.45f, 0.30f));

  // tailFlap front cutout
  checkPos = p- to_float3(0.0f, 0.0f, 1.15f);
  pR(swi2(checkPos,y,z), 1.32f);
  tailFlap=_fmaxf(tailFlap, -sdBox( checkPos, to_float3(0.75f, 1.41f, 1.6f)));

  // tailFlap rear cutout
  checkPos = p- to_float3(0.0f, 0, -2.75f);  
  pR(swi2(checkPos,y,z), -0.15f);
  tailFlap=fOpIntersectionRound(tailFlap, -sdBox( checkPos, to_float3(0.75f, 1.4f, 2.0f)), 0.05f);

  checkPos = p- to_float3(0.0f, 0.0f, -0.65f);
  tailFlap = _fminf(tailFlap, sdEllipsoid( checkPos-to_float3(0.00f, 0.25f, 0), to_float3(0.06f, 0.05f, 0.15f)));
  tailFlap = _fminf(tailFlap, sdEllipsoid( checkPos-to_float3(0.00f, 0.10f, 0), to_float3(0.06f, 0.05f, 0.15f)));

  return tailFlap;
}

__DEVICE__ float MapTopWing(float3 p, float mirrored)
{    
  checkPos = p- to_float3(1.15f, 1.04f, -8.5f);
  pR(swi2(checkPos,x,y), -0.15f);  
  float topWing = sdBox( checkPos, to_float3(0.014f, 0.8f, 1.2f));
  if (topWing<0.15f) //Bounding Box test
  {
    float flapDist = MapTailFlap(checkPos, mirrored);

    checkPos = p- to_float3(1.15f, 1.04f, -8.5f);
    pR(swi2(checkPos,x,y), -0.15f);  
    // top border    
    topWing = _fminf(topWing, sdBox( checkPos-to_float3(0, 0.55f, 0), to_float3(0.04f, 0.1f, 1.25f)));

    float flapCutout = sdBox(checkPos- to_float3(0.0f, -0.04f, -1.19f), to_float3(0.02f, 0.45f, 1.0f));
    // tailFlap front cutout
    checkPos = p- to_float3(1.15f, 2.0f, -7.65f);
    pR(swi2(checkPos,y,z), 1.32f);
    flapCutout=_fmaxf(flapCutout, -sdBox( checkPos, to_float3(0.75f, 1.41f, 1.6f)));

    // make hole for tail flap
    topWing=_fmaxf(topWing, -flapCutout);

    // front cutouts
    checkPos = p- to_float3(1.15f, 2.0f, -7.0f);
    pR(swi2(checkPos,y,z), 1.02f);
    topWing=fOpIntersectionRound(topWing, -sdBox( checkPos, to_float3(0.75f, 1.41f, 1.6f)), 0.05f);

    // rear cutout
    checkPos = p- to_float3(1.15f, 1.0f, -11.25f);  
    pR(swi2(checkPos,y,z), -0.15f);
    topWing=fOpIntersectionRound(topWing, -sdBox( checkPos, to_float3(0.75f, 1.4f, 2.0f)), 0.05f);

    // top roll 
    topWing=_fminf(topWing, sdCapsule(p- to_float3(1.26f, 1.8f, -8.84f), to_float3(0, 0, -0.50f), to_float3(0, 0, 0.3f), 0.06f)); 

    topWing = _fminf(topWing, flapDist);
  }
  return topWing;
}

__DEVICE__ float MapPlane( float3 p)
{
  float  d=100000.0f;
  float3 pOriginal = p;
  // rotate position 
  p=TranslatePos(p, pitch, roll);
  float mirrored=0.0f;
  // AABB TEST  
  float test = sdBox( p- to_float3(0.0f, -0.0f, -3.0f), to_float3(7.5f, 4.0f, 10.6f));    
  if (test>1.0f) return test;

  // mirror position at x=0.0. Both sides of the plane are equal.
  mirrored = pMirror(p.x, 0.0f);

  float body= _fminf(d, sdEllipsoid(p-to_float3(0.0f, 0.1f, -4.40f), to_float3(0.50f, 0.30f, 2.0f)));
  body=fOpUnionRound(body, sdEllipsoid(p-to_float3(0.0f, 0.0f, 0.50f), to_float3(0.50f, 0.40f, 3.25f)), 1.0f);
  body=_fminf(body, sdConeSection(p- to_float3(0.0f, 0.0f, 3.8f), 0.1f, 0.15f, 0.06f));   

  body=_fminf(body, sdConeSection(p- to_float3(0.0f, 0.0f, 3.8f), 0.7f, 0.07f, 0.01f));   

  // window
  winDist =sdEllipsoid(p-to_float3(0.0f, 0.3f, -0.10f), to_float3(0.45f, 0.4f, 1.45f));
  winDist =fOpUnionRound(winDist, sdEllipsoid(p-to_float3(0.0f, 0.3f, 0.60f), to_float3(0.3f, 0.6f, 0.75f)), 0.4f);
  winDist = _fmaxf(winDist, -body);
  body = _fminf(body, winDist);
  body=_fminf(body, fOpPipe(winDist, sdBox(p-to_float3(0.0f, 0.0f, 1.0f), to_float3(3.0f, 1.0f, 0.01f)), 0.03f));
  body=_fminf(body, fOpPipe(winDist, sdBox(p-to_float3(0.0f, 0.0f, 0.0f), to_float3(3.0f, 1.0f, 0.01f)), 0.03f));

  // front (nose)
  body=_fmaxf(body, -_fmaxf(fCylinder(p-to_float3(0, 0, 2.5f), 0.46f, 0.04f), -fCylinder(p-to_float3(0, 0, 2.5f), 0.35f, 0.1f)));
  checkPos = p-to_float3(0, 0, 2.5f);
  pR(swi2(checkPos,y,z), 1.57f);
  body=fOpIntersectionRound(body, -sdTorus(checkPos+to_float3(0, 0.80f, 0), to_float2(0.6f, 0.05f)), 0.015f);
  body=fOpIntersectionRound(body, -sdTorus(checkPos+to_float3(0, 2.30f, 0), to_float2(0.62f, 0.06f)), 0.015f);

  // wings       
  frontWingDist = MapFrontWing(p, mirrored);
  d=_fminf(d, frontWingDist);   
  rearWingDist = MapRearWing(p);
  d=_fminf(d, rearWingDist);
  topWingDist = MapTopWing(p, mirrored);
  d=_fminf(d, topWingDist);

  // bottom
  checkPos = p-to_float3(0.0f, -0.6f, -5.0f);
  pR(swi2(checkPos,y,z), 0.07f);  
  d=fOpUnionRound(d, sdBox(checkPos, to_float3(0.5f, 0.2f, 3.1f)), 0.40f);

  float holder = sdBox( p- to_float3(0.0f, -1.1f, -4.30f), to_float3(0.08f, 0.4f, 0.8f));  
  checkPos = p;
  pR(swi2(checkPos,y,z), 0.85f);
  holder=_fmaxf(holder, -sdBox( checkPos- to_float3(0.0f, -5.64f, -2.8f), to_float3(1.75f, 1.4f, 1.0f))); 
  d=fOpUnionRound(d, holder, 0.25f);

  // large bomb
  bombDist2 = fCylinder( p- to_float3(0.0f, -1.6f, -4.0f), 0.45f, 1.0f);   
  bombDist2 =_fminf(bombDist2, sdEllipsoid( p- to_float3(0.0f, -1.6f, -3.20f), to_float3(0.45f, 0.45f, 2.0f)));   
  bombDist2 =_fminf(bombDist2, sdEllipsoid( p- to_float3(0.0f, -1.6f, -4.80f), to_float3(0.45f, 0.45f, 2.0f)));   

  d=_fminf(d, bombDist2);

  d=_fminf(d, sdEllipsoid(p- to_float3(1.05f, 0.13f, -8.4f), to_float3(0.11f, 0.18f, 1.0f)));    

  checkPos = p- to_float3(0, 0.2f, -5.0f);
  d=fOpUnionRound(d, fOpIntersectionRound(sdBox( checkPos, to_float3(1.2f, 0.14f, 3.7f)), -sdBox( checkPos, to_float3(1.0f, 1.14f, 4.7f)), 0.2f), 0.25f);

  d=fOpUnionRound(d, sdEllipsoid( p- to_float3(0, 0.0f, -4.0f), to_float3(1.21f, 0.5f, 2.50f)), 0.75f);

  // engine cutout
  blackDist = _fmaxf(d, fCylinder(p- to_float3(0.8f, -0.15f, 0.0f), 0.5f, 2.4f)); 
  d=_fmaxf(d, -fCylinder(p- to_float3(0.8f, -0.15f, 0.0f), 0.45f, 2.4f)); 

  // engine
  d =_fmaxf(d, -sdBox(p-to_float3(0.0f, 0, -9.5f), to_float3(1.5f, 0.4f, 0.7f)));

  engineDist=fCylinder(p- to_float3(0.40f, -0.1f, -8.7f), 0.42f, 0.2f);
  checkPos = p- to_float3(0.4f, -0.1f, -8.3f);
  pR(swi2(checkPos,y,z), 1.57f);
  engineDist=_fminf(engineDist, sdTorus(checkPos, to_float2(0.25f, 0.25f)));
  engineDist=_fminf(engineDist, sdConeSection(p- to_float3(0.40f, -0.1f, -9.2f), 0.3f, 0.22f, 0.36f));

  checkPos = p-to_float3(0.0f, 0.0f, -9.24f);  
  swi2(checkPos,x,y)-=to_float2(0.4f, -0.1f);
  swi2(checkPos,x,y) = pModPolar(swi2(checkPos,x,y), 22.0f);

  float engineCone = fOpPipe(engineDist, sdBox( checkPos, to_float3(0.6f, 0.001f, 0.26f)), 0.015f);
  engineDist=_fminf(engineDist, engineCone);

  d=_fminf(d, engineDist);
  eFlameDist = sdEllipsoid( p- to_float3(0.4f, -0.1f, -9.45f-(speed*0.07f)+_cosf(iTime*40.0f)*0.014f), to_float3(0.17f, 0.17f, 0.10f));
  d=_fminf(d, eFlameDist);

  d=_fminf(d, winDist);
  d=_fminf(d, body);

  d=_fminf(d, sdBox( p- to_float3(1.1f, 0.0f, -6.90f), to_float3(0.33f, 0.12f, 0.17f))); 
  checkPos = p-to_float3(0.65f, 0.55f, -1.4f);
  pR(swi2(checkPos,y,z), -0.35f);
  d=_fminf(d, sdBox(checkPos, to_float3(0.2f, 0.1f, 0.45f)));

  return _fminf(d, eFlameDist);
}

RayHit TracePlane(in float3 origin, in float3 direction)
{
  RayHit result;
  float maxDist = 150.0f;
  float t = 0.0f, dist = 0.0f;
  float3 rayPos;
  eFlameDist=10000.0f;
  for ( int i=0; i<RAYSTEPS; i++ )
  {
    rayPos =origin+direction*t;
    dist = MapPlane( rayPos);

    if (_fabs(dist)<0.003f || t>maxDist )
    {                
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));   
      result.winDist = winDist;
      result.engineDist = engineDist;
      result.eFlameDist = eFlameDist;
      result.blackDist = blackDist;
      result.bombDist = bombDist;
      result.bombDist2 = bombDist2;
      result.missileDist = missileDist;
      result.frontWingDist = frontWingDist;
      result.rearWingDist = rearWingDist;
      result.topWingDist = topWingDist;
      break;
    }
    t += dist;
  }

  return result;
}

__DEVICE__ float MapLights( float3 p)
{
  float3 pOriginal = p;
  // rotate position 
  p=TranslatePos(p, pitch, roll);   
  // mirror position at x=0.0. Both sides of the plane are equal.
  pMirror(p.x, 0.0f);

  return _fmaxf(sdEllipsoid( p- to_float3(0.4f, -0.1f, -9.5f), to_float3(0.03f, 0.03f, 0.03f+_fmaxf(0.0f, (speed*0.07f)))), -sdBox(p- to_float3(0.4f, -0.1f, -9.6f+2.0f), to_float3(2.0f, 2.0f, 2.0f)));
}

__DEVICE__ float TraceLights(in float3 origin, in float3 direction)
{
  float maxDist = 150.0f;
  float t = 0.0f;
  float3 rayPos;
  float dist=10000.0f;

  for ( int i=0; i<10; i++ )
  {
    rayPos =origin+direction*t;
    dist = _fminf(dist, MapLights( rayPos));
    t += dist;
  }

  return dist;
}

__DEVICE__ float3 calcNormal( in float3 pos )
{    
  return normalize( to_float3(MapPlane(pos+swi3(eps,x,y,y)) - MapPlane(pos-swi3(eps,x,y,y)), 0.5f*2.0f*eps.x, MapPlane(pos+swi3(eps,y,y,x)) - MapPlane(pos-swi3(eps,y,y,x)) ) );
}

__DEVICE__ float SoftShadow( in float3 origin, in float3 direction )
{
  float res = 2.0f, t = 0.02f, h;
  for ( int i=0; i<24; i++ )
  {
    h = MapPlane(origin+direction*t);
    res = _fminf( res, 7.5f*h/t );
    t += clamp( h, 0.05f, 0.2f );
    if ( h<0.001f || t>2.5f ) break;
  }
  return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3_aw(_sinf(cr), _cosf(cr), 0.0f);
  float3 cu = normalize( cross(cw, cp) );
  float3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}


// Advanced lightning pass
__DEVICE__ float3 GetSceneLight(float specLevel, float3 normal, RayHit rayHit, float3 rayDir, float3 origin, float specSize)
{          
  float dif = clamp( dot( normal, sunPos ), 0.0f, 1.0f );
  float3 reflectDir = reflect( rayDir, normal );
  specLevel*= _powf(clamp( dot( reflectDir, sunPos ), 0.0f, 1.0f ), 9.0f/specSize);
  float3 reflection = to_float3(_tex2DVecN(iChannel3,reflectDir.x,reflectDir.y,15).r*1.5f);

  float fre = _powf( 1.0f-_fabs(dot( normal, rayDir )), 2.0f );
  fre = _mix( 0.03f, 1.0f, fre );   
  float amb = clamp( 0.5f+0.5f*normal.y, 0.0f, 1.0f );

  float3 shadowPos = origin+((rayDir*rayHit.depth)*0.998f);

  float shadow = SoftShadow(shadowPos, sunPos);
  dif*=shadow;
  float skyLight = smoothstep( -0.1f, 0.1f, reflectDir.y );
  skyLight *= SoftShadow(shadowPos, reflectDir );

  float3 lightTot = (to_float3_s(0.2f)*amb); 
  lightTot+=to_float3_s(0.85f)*dif;
  lightTot= _mix(lightTot, reflection*_fmaxf(0.3f, shadow), fre );
  lightTot += 1.00f*specLevel*dif;
  lightTot += 0.50f*skyLight*to_float3(0.40f, 0.60f, 1.00f);
  lightTot= _mix(lightTot*0.7f, lightTot*1.2f, fre );

  fre = _powf( 1.0f-_fabs(dot(rayHit.normal, rayDir)), 4.0f);
  fre = _mix(0.0f, _mix( 0.1f, 1.0f, specLevel*0.5f), fre );
  lightTot = _mix( lightTot, lightTot+ to_float3_s(1.6f), fre );

  return lightTot*sunColor;
}

__DEVICE__ float drawRect(float2 p1, float2 p2, float2 uv) 
{
  float4 rect = to_float4_aw(p1, p2);
  float2 hv = step(swi2(rect,x,y), uv) * step(uv, swi2(rect,z,w));
  return hv.x * hv.y;
}

// Thanks IÃ±igo Quilez!
__DEVICE__ float line(float2 p, float2 a, float2 b, float size)
{
  float2 pa = -p - a;
  float2 ba = b - a;
  float h = clamp( dot(pa, ba)/dot(ba, ba), 0.0f, 1.0f );
  float d = length( pa - ba*h );

  return clamp((((1.0f+size) - d)-0.99f)*100.0f, 0.0f, 1.0f);
}

__DEVICE__ void AddLetters(float2 hitPos, inout float3 col, float2 linePos)
{
  // text
  float3 textColor = to_float3_s(0.2f);
  float2 absHitPos2 = to_float2(hitPos.x-1.05f, hitPos.y);

  pModInterval1(absHitPos2.x, 8.0f, linePos.x, linePos.x+10.0f);

  // E
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(1.45f, 0.4f), linePos+to_float2(1.45f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(1.45f, 0.9f), linePos+to_float2(1.1f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(1.45f, 0.65f), linePos+to_float2(1.25f, 0.65f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(1.45f, 0.4f), linePos+to_float2(1.1f, 0.4f), 0.06f));
  // F            
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.9f, 0.4f), linePos+to_float2(0.9f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.9f, 0.9f), linePos+to_float2(0.65f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.9f, 0.65f), linePos+to_float2(0.75f, 0.65f), 0.06f));
  // Z
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.45f, 0.4f), linePos+to_float2(0.1f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.45f, 0.9f), linePos+to_float2(0.1f, 0.9f), 0.06f));
  col =_mix(col, textColor, line(absHitPos2, linePos+to_float2(0.45f, 0.4f), linePos+to_float2(0.1f, 0.4f), 0.06f));
}


__DEVICE__ float3 GetReflectionMap(float3 rayDir, float3 normal)
{
  return texture(iChannel3, reflect( rayDir, normal )).rgb;
}

__DEVICE__ float4 GetMaterial(float3 rayDir, inout RayHit rayHit, float2 fragCoord, inout float specSize)
{
  float3 hitPos =TranslatePos(rayHit.hitPos, pitch, roll);
  float2 center;
  float dist;

  float specLevel=0.7f;
  specSize=0.7f;

  float fre = _powf( 1.0f-_fabs(dot( rayHit.normal, rayDir )), 3.0f );
  fre = _mix( 0.03f, 1.0f, fre );   

  // float3 tint = to_float3(0.62f,0.50f,0.40f)*1.15f;
  float3 tint = to_float3(1.62f, 1.50f, 1.30f)*0.65f;
  float3 brightCamo =1.15f*tint;
  float3 darkCamo = 0.78f*tint;


  float3 baseTexture = _mix(brightCamo, darkCamo, smoothstep(0.5f, 0.52f, noise(hitPos*1.6f)));

  // baseTexture = col;
  float3 col=_mix(brightCamo, darkCamo, smoothstep(0.5f, 0.52f, noise(hitPos*1.6f)));
  float3 reflection = GetReflectionMap(rayDir, rayHit.normal);
  // create base color mixes
  float3 lightColor = (to_float3_s(1.0f));
  float3 darkColor = (to_float3_s(0.25f));
  float3 missilBaseCol =  lightColor*0.5f;
  float3 missilBaseCol2 =  darkColor;
  float3 missilCol = lightColor;
  float3 missilCol2 = lightColor*0.27f;

  if (distance(rayHit.dist, rayHit.topWingDist)<0.01f)
  { 
    // top wing stripes
    col=_mix(darkColor, baseTexture, smoothstep(0.55f, 0.57f, distance(0.85f, hitPos.y)));
    col=_mix(lightColor, col, smoothstep(0.32f, 0.34f, distance(0.95f, hitPos.y)));

    // create star (top wings)    
    center = to_float2(-8.73f, 0.95f)-to_float2(hitPos.z, hitPos.y);
    dist = length(center); 
    col=_mix(darkColor, col, smoothstep(0.24f, 0.26f, dist));
    col=_mix(lightColor, col, smoothstep(0.24f, 0.26f, (dist*1.15f)+_fabs(_cosf( _atan2f(center.y, center.x)*2.5f)*0.13f)));
  } else if (distance(rayHit.dist, rayHit.winDist)<0.01f)
  { 
    // windows
    col=to_float3(0.2f, 0.21f, 0.22f)*reflection;
    specSize=3.2f;
    specLevel=3.5f;
    fre = _powf( 1.0f-_fabs(dot(rayHit.normal, rayDir)), 3.0f);
    fre = _mix( _mix( 0.0f, 0.01f, specLevel ), _mix( 0.4f, 1.0f, specLevel ), fre );
    col = _mix(col, to_float3_s(1.5f), fre );
  } else if (distance(rayHit.dist, rayHit.missileDist)<0.01f)
  {  
    specSize=2.0f;
    specLevel=2.0f;
    // small missiles
    col=_mix(missilBaseCol, missilCol2, smoothstep(-3.35f, -3.37f, hitPos.z));
    col=_mix(col, missilCol, smoothstep(-3.2f, -3.22f, hitPos.z));
    col=_mix(missilCol2, col, smoothstep(0.32f, 0.34f, distance(-4.75f, hitPos.z)));
    col=_mix(missilBaseCol, col, smoothstep(0.25f, 0.27f, distance(-4.75f, hitPos.z)));
  } else if (distance(rayHit.dist, rayHit.bombDist)<0.01f)
  { 
    specSize=2.0f;
    specLevel=1.7f;
    // small bombs   
    col=_mix(missilCol, missilBaseCol, smoothstep(1.18f, 1.2f, distance(-4.5f, hitPos.z)));      
    col=_mix(col, missilCol2, smoothstep(1.3f, 1.32f, distance(-4.5f, hitPos.z)));
  } else if (distance(rayHit.dist, rayHit.bombDist2)<0.01f)
  {   
    specSize=2.0f;
    specLevel=1.8f;
    // large bomb  
    col=_mix(missilBaseCol2, missilCol, smoothstep(1.48f, 1.5f, distance(-4.1f, hitPos.z)));      
    col=_mix(col, missilBaseCol, smoothstep(1.6f, 1.62f, distance(-4.1f, hitPos.z)));      
    col=_mix(missilBaseCol, col, smoothstep(0.45f, 0.47f, distance(-4.1f, hitPos.z)));
  } else
  {
    // remove camo from wing tip
    col =_mix(col, brightCamo, line(to_float2(_fabs(hitPos.x), hitPos.z), to_float2(-7.25f, 5.0f), to_float2(-1.45f, 1.7f), 0.3f));

    // color bottom gray
    col=_mix(lightColor*0.7f, col, step(0.01f, hitPos.y));

    // front
    col = _mix(col, lightColor, smoothstep(3.0f, 3.02f, hitPos.z));  
    col = _mix(col, darkColor, smoothstep(3.08f, 3.1f, hitPos.z));
    col =_mix(col*1.4f, col, smoothstep(0.07f, 0.09f, distance(1.8f, hitPos.z)));


    // front wing stripes
    col=_mix(darkColor, col, smoothstep(1.4f, 1.42f, distance(-6.90f, hitPos.z)));
    col=_mix(lightColor, col, smoothstep(1.3f, 1.32f, distance(-6.90f, hitPos.z)));
    col=_mix(darkColor, col, smoothstep(0.84f, 0.86f, distance(-6.7f, hitPos.z)));
    col=_mix(lightColor, col, smoothstep(0.22f, 0.235f, distance(-6.94f, hitPos.z)));

    // vertical stripes   
    float xMod = mod_f(hitPos.x-0.5f, 11.0f);
    col=_mix(darkColor, col, smoothstep(0.5f, 0.52f, distance(5.0f, xMod)));
    col=_mix(lightColor, col, smoothstep(0.4f, 0.42f, distance(5.0f, xMod)));


    // boxes 
    float2 absHitPos = _fabs(swi2(hitPos,x,z));

    col =_mix(col, col*1.40f, drawRect(to_float2(0.4f, 2.0f)-0.05f, to_float2(0.8f, 2.0f)+0.05f+0.25f, absHitPos));
    col =_mix(col, col*0.2f, drawRect(to_float2(0.4f, 2.0f), to_float2(0.8f, 2.0f)+0.2f, absHitPos));

    // side 17      
    float2 linePos = to_float2(-0.55f, 0.0f);
    float3 textColor = to_float3_s(0.2f);
    if (hitPos.x<0.0f)
    {
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(0.0f, -0.2f), linePos+to_float2(0.0f, 0.2f), 0.04f));
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(-0.2f, -0.2f), linePos+to_float2(-0.4f, -0.2f), 0.04f));
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(-0.4f, -0.2f), linePos+to_float2(-0.25f, 0.2f), 0.04f));
    } else
    {
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(-0.35f, -0.2f), linePos+to_float2(-0.35f, 0.2f), 0.04f));
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(0.1f, -0.2f), linePos+to_float2(-0.15f, -0.2f), 0.04f));
      col =_mix(col, textColor, line(swi2(hitPos,z,y), linePos+to_float2(-0.15f, 0.2f), linePos+to_float2(0.10f, -0.2f), 0.04f));
    }  

    if (hitPos.y>0.15f)
    {
      // letters BoundingBox
      if (drawRect(to_float2(3.2f, 3.8f)-0.05f, to_float2(4.9f, 4.8f), absHitPos)>=1.0f)
      {
        AddLetters(swi2(hitPos,x,z), col, to_float2(-3.70f, 3.60f));
      }

      // more boxes 
      col =_mix(col, col*1.40f, drawRect(to_float2(0.2f, 3.6f)-0.05f, to_float2(1.0f, 3.6f)+0.05f+0.35f, absHitPos)); 
      col =_mix(col, col*0.2f, drawRect(to_float2(0.2f, 3.6f), to_float2(1.0f, 3.6f)+0.3f, absHitPos));          
      col =_mix(col, col*0.2f, drawRect(to_float2(3.5f, 4.8f), to_float2(4.5f, 5.3f), absHitPos));

      // create star (front wings)         
      center = to_float2(5.0f, -5.1f)-to_float2(xMod, hitPos.z);
      dist = length(center);
      col=_mix(lightColor, col, smoothstep(0.8f, 0.82f, dist));
      col=_mix(darkColor, col, smoothstep(0.7f, 0.72f, dist));
      col=_mix(lightColor, col, smoothstep(0.7f, 0.72f, (dist*1.15f)+_fabs(_cosf( _atan2f(center.y, center.x)*2.5f)*0.3f)));
      col=_mix(darkColor, col, smoothstep(0.6f, 0.62f, (dist*1.50f)+_fabs(_cosf( _atan2f(center.y, center.x)*2.5f)*0.3f)));
    } else
    {
      // bottom details
      col =_mix(col, darkColor, line(to_float2(_fabs(hitPos.x), hitPos.z), to_float2(0.0f, -1.5f), to_float2(-0.3f, -1.5f), 0.06f));
      col =_mix(col, darkColor, line(to_float2(_fabs(hitPos.x), hitPos.z), to_float2(-0.3f, -1.5f), to_float2(-0.3f, -1.0f), 0.085f));
    }

    // rear wing stripes
    col=_mix(darkColor, col, smoothstep(0.55f, 0.57f, distance(-9.6f, hitPos.z)));
    col=_mix(lightColor, col, smoothstep(0.5f, 0.52f, distance(-9.6f, hitPos.z)));
    col=_mix(darkColor, col, smoothstep(0.4f, 0.42f, distance(-9.6f, hitPos.z)));

    // esm pods
    col = _mix(col, lightColor*0.75f, smoothstep(7.02f, 7.04f, _fabs(hitPos.x)));

    // stabilizer
    col = _mix(col, lightColor*0.75f, smoothstep(1.72f, 1.74f, _fabs(hitPos.y)));

    // engines exhaust
    col=_mix(mix(to_float3_s(0.7f), reflection, fre), col, step(0.05f, rayHit.engineDist));
    specSize=_mix(4.0f, specSize, step(0.05f, rayHit.engineDist));
    col=_mix(col*0.23f, col, step(0.02f, rayHit.blackDist));
    col=_mix(col+0.5f, col, smoothstep(0.04f, 0.10f, distance(2.75f, hitPos.z)));
  }
  fre = _powf( 1.0f-_fabs(dot(rayHit.normal, rayDir)), 7.0f);
  fre = _mix( 0.0f, _mix( 0.2f, 1.0f, specLevel*0.5f ), fre );
  col = _mix( col, to_float3(1.0f, 1.0f, 1.1f)*1.5f, fre );

  return to_float4_aw(col, specLevel);
}

__KERNEL__ void TurnNBurnFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  fragCoord+=0.5f;
   
  float2 R=iResolution;

  float2 mo = iMouse.xy/iResolution;
  float2 uv = fragCoord / iResolution;
  float2 screenSpace = (-iResolution + 2.0f*(fragCoord))/iResolution.y;
  float2 cloudPos = to_float2(-iTime*1.3f, -iTime*0.95f);
  float CAMZOOM = read(to_int2(52, 0));  

  // read missile data
  missilesLaunched = to_float2(read(to_int2(100, 0)), read(to_int2(200, 0)));

  // read roll and speed values from buffer
  turn = read(to_int2(1, 10));
  roll = read(to_int2(1, 1));
  speed = read(to_int2(10, 1));
  pitch = read(to_int2(15, 1));
  rudderAngle = read(to_int2(6, 1));
  sunPos = readRGB(to_int2(50, 0));
  planePos = readRGB(to_int2(55, 0));
  pR(swi2(sunPos,x,z), turn);

  // setup camera and ray direction
  float2 camRot = readRGB(to_int2(57, 0)).xy;

  float3 rayOrigin = to_float3_aw(CAMZOOM*_cosf(camRot.x), CAMZOOM*_sinf(camRot.y), -3.0f+CAMZOOM*_sinf(camRot.x) );
  mat3 ca = setCamera( rayOrigin, to_float3(0.0f, 0.0f, -3.0f ), 0.0f );
  float3 rayDir = ca * normalize( to_float3(swi2(screenSpace,x,y), 2.0f) );

  // load background from buffer A
  float4 color =  _tex2DVecN(iChannel0,uv.x,uv.y,15);

  // calculate engine flare
  float lightDist = TraceLights(rayOrigin, rayDir);
    
  float3 lightFlares = to_float3_s(0.0f);
  lightFlares =  _mix((to_float3(1.0f, 0.4f, 0.2f)), to_float3_s(0.0f), smoothstep(0.0f, 0.35f, lightDist));             
  lightFlares =  _mix(lightFlares+(2.0f*to_float3(1.0f, 0.5f, 0.2f)), lightFlares, smoothstep(0.0f, 0.15f, lightDist));
  lightFlares =  _mix(lightFlares+to_float3(1.0f, 1.0f, 1.0f), lightFlares, smoothstep(0.0f, 0.08f, lightDist));
  RayHit marchResult = TracePlane(rayOrigin, rayDir);

  if (marchResult.hit)
  {
    float specSize=1.0f;

    marchResult.normal = calcNormal(marchResult.hitPos); 

    // create texture map and set specular levels
    color = GetMaterial(rayDir, marchResult, fragCoord, specSize);

    if (marchResult.dist != marchResult.eFlameDist)
    {
      // get lightning based on material
      float3 light = GetSceneLight(color.w, marchResult.normal, marchResult, rayDir, rayOrigin, specSize);   

      // cloud shadows on plane if below cloud level
      if (planePos.y<=-CLOUDLEVEL)
      {  
        // get cloud shadows at rayMarch hitpos
        float clouds =clamp(_fmaxf(0.0f, -0.15f+noise(marchResult.hitPos+planePos+to_float3(cloudPos.x, 0.0f, cloudPos.y))), 0.0f, 1.0f)*0.5f;

        swi3(color,x,y,z)*= 1.0f-clouds;
        // sun light  
        swi3(color,x,y,z)*= 1.0f+(clouds);
      }   

      // apply lightning
      swi3(color,x,y,z) *=light;

      // balance colors
      swi3(color,x,y,z) = _powf(swi3(color,x,y,z), to_float3(1.0f/1.1f));
    }

    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), to_float3(0.3f, 0.5f, 0.7f), 0.1f);    
    color.a=1.0f;  

    lightFlares = _mix(lightFlares, lightFlares*0.0f, step(0.1f, distance(marchResult.dist, marchResult.eFlameDist)));
  }

  swi3(color,x,y,z)+=lightFlares;
  fragColor = color;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel3
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel2


////////////////////////////////////////////////////////////////////////////////////////////
// Copyright © 2017 Kim Berkeby
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
////////////////////////////////////////////////////////////////////////////////////////////
/*

 CONTROLS:
 ---------
 SPACE  FIRE MISSILE
 
 S      ROLL LEFT
 F      ROLL RIGHT  
 E      PITCH DOWN
 D      PITCH UP
 
 W      YAW LEFT   (PLANE TURN) 
 R      YAW RIGHT   (PLANE TURN)
 
 SHIFT  INCREASE SPEED
 CTRL   DECCREASE SPEED 
 
 F1     ZOOM OUT
 F2     ZOOM IN
 
 NOTICE:
 Controls can be changed to use arrow keys if you uncomment the alternative controls in Buf A. 
 
 
 Toggle effects by pressing folloving keys:
 ------------------------------------------
 1-key  = Lens dirt  on/off               (default off)
 2-key  = Grain filter  on/off            (default on)
 3-key  = Chromatic aberration  on/off    (default on)    
 4-key  = Anti aliasing  on/off           (default on)
 5-key  = Lens flare  on/off              (default on)
 
 --------------------------------------------------------
 TO INCREASE PERFORMANCE:
 
 Delete one or several defines from Buf B:
 
 #define SHADOWS
 #define QUALITY_TREE
 #define QUALITY_REFLECTIONS
 #define EXACT_EXPLOSIONS
 --------------------------------------------------------
 
 This shader was made by using distance functions found in HG_SDF:
 http://mercury.sexy
 
 Special thanks to Inigo Quilez for his great tutorials on:
 http://iquilezles.org/
 
 Last but not least, thanks to all the nice people here at ShaderToy! :-D

*/
//////////////////////////////////////////////////////////////////////////////////////
// POST EFFECTS BUFFER
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Buffer A. Read data from data-buffer.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = Buffer C. Get the colors of the render from the last buffer.
// Channel 3 = Organic 2 texture. Used in lens dirt filter.

float2 R;

  #define FastNoise(posX)   (  texture(iChannel1, (posX+0.5f)/iResolution).x)
  #define readAlpha(memPos) (  _tex2DVecN(iChannel2,memPos.x,memPos.y,15).w)
  //#define read(memPos)      (  texelFetch(iChannel0, memPos, 0).w)
  //#define readRGB(memPos)   (  texelFetch(iChannel0, memPos, 0).xyz)
  #define read(memPos)      (  texture(iChannel0, (to_float2(memPos)+0.5f)/R).w)
  #define readRGB(memPos)   (  texture(iChannel0, (to_float2(memPos)+0.5f)/R).xyz)
  
  
  #define CLOUDLEVEL -70.0
  #define PI _acosf(-1.0f)
  #pragma optimize(off) 
mat3 cameraMatrix;
float3 planePos=to_float3_s(0.0f);
float3 sunPos=to_float3_s(0.0f);
const float3 eps = to_float3(0.02f, 0.0f, 0.0f);

__DEVICE__ float GetExplosionIntensity(float life)
{
  return _mix(1.0f, 0.0f, smoothstep(0.0f, 5.0f, distance(life, 5.0f)));
}

// 3D noise function (IQ)
__DEVICE__ float fastFBM(float3 p)
{
  float3 ip=_floor(p);
  p-=ip; 
  float3 s=to_float3(7, 157, 113);
  float4 h=to_float4(0.0f, swi2(s,y,z), s.y+s.z)+dot(ip, s);
  p=p*p*(3.0f-2.0f*p); 
  h=_mix(fract(_sinf(h)*43758.5f), fract(_sinf(h+s.x)*43758.5f), p.x);
  h.xy=_mix(swi2(h,x,z), swi2(h,y,w), p.y);
  return _mix(h.x, h.y, p.z);
}

__DEVICE__ mat3 setCamera(  float3 ro, float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3_aw(_sinf(cr), _cosf(cr), 0.0f);
  float3 cu = normalize( cross(cw, cp) );
  float3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}

__DEVICE__ float2 GetScreenPos(float3 pos)
{
  return to_float2(PI*dot( pos, cameraMatrix[0].xyz ), PI* dot( pos, cameraMatrix[1].xyz ));
}

__DEVICE__ float3 CalculateSunFlare(float3 rayDir, float3 rayOrigin, float2 screenSpace, float alpha, float enableFlare)
{
  float visibility = _powf(_fmaxf(0.0f, dot(sunPos, rayDir)), 8.0f);  
  if (visibility<=0.05f) return to_float3_s(0.0f);

  float2 sunScreenPos = GetScreenPos(sunPos);

  float2 uvT = screenSpace-sunScreenPos;
  float sunIntensity = (1.0f/(_powf(length(uvT)*4.0f+1.0f, 1.30f)))*visibility;

  float3 flareColor = to_float3_s(0.0f);
  float2 offSet = uvT;
  float2 offSetStep=  0.4f*sunScreenPos;
  float3 color;
  float size=0.0f, dist=0.0f;
  
  if(enableFlare>0.0f)
  {
  // check if center of sun is covered by any object. MATH IS OFF AT SCREEN CHECK POS! sunScreenPos/2.0f +0.5f IS NOT EXACTLY SUN MIDDLE!
  // only draw if not covered by any object
  if (readAlpha( sunScreenPos/2.0f +0.5f)<0.50f)
  {
    // create flare rings
    for (float i =1.0f; i<8.0f; i++)
    {
      swi2(color,x,y) = to_float2(_fabs((_sinf(i*53.0f))), 0.65f);
      color.z = _fabs((_cosf(i*25.0f)));
      offSet += offSetStep;

      size = 0.05f+((1.0f-_sinf(i*0.54f))*0.28f);
      dist = _powf(distance(sunScreenPos, offSet), 1.20f);

      flareColor += _mix(to_float3_s(0.0f), sunIntensity*(10.0f*size) * color, smoothstep(size, size-dist, dist))/(1.0f-size);
    }
  }
  flareColor*=_mix(0.0f, 1.0f, smoothstep(0.0f, 0.1f, visibility));
  }
    
  // flare star shape
  float3 sunSpot = to_float3(1.30f, 1.0f, 0.80f)*sunIntensity*(_sinf(FastNoise((sunScreenPos.x+sunScreenPos.y)*2.3f+_atan2f(uvT.x, uvT.y)*15.0f)*5.0f)*0.12f);
  // sun glow
  sunSpot+=to_float3(1.0f, 0.96f, 0.90f)*sunIntensity*0.75f;
  sunSpot+=to_float3(1.0f, 0.76f, 0.20f)*visibility*0.15f;

  return flareColor+(sunSpot*(1.0f-alpha));
}
__DEVICE__ float3 CalculateExplosionFlare(float3 rayDir, float3 rayOrigin, float2 screenSpace, float alpha, float3 explosionPos, float enableFlare)
{

  float visibility = _fmaxf(0.0f, dot(explosionPos, rayDir));  
  if (visibility<=0.15f) return to_float3_s(0.0f);

  float2 flareScreenPos = GetScreenPos(explosionPos);
  float2 uvT = screenSpace-flareScreenPos;
  float flareIntensity = 0.2f*visibility;
  float3 flareColor = to_float3_s(0.0f);
  float2 offSet = uvT;
  float2 offSetStep=  0.4f*flareScreenPos;
  float3 color;
  float size=0.0f, dist=0.0f; 

    if(enableFlare>0.0f)
    {
  // create flare rings
  for (float i =1.0f; i<8.0f; i++)
  {
    swi2(color,x,y) = to_float2(0.75f+(0.25f*_sinf(i*i)));
    color.z = 0.75f+(0.35f*_cosf(i*i));
    offSet += offSetStep;
    size = 0.05f+((1.0f-_sinf(i*0.54f))*0.38f);
    dist = _powf(distance(flareScreenPos, offSet), 1.20f);

    flareColor += _mix(to_float3_s(0.0f), flareIntensity*(4.0f*size) * color, smoothstep(size, size-dist, dist))/(1.0f-size);
  }
  flareColor/=2.0f;
    }
  // flare star shape
  float3 flareSpot = to_float3(1.30f, 1.0f, 0.80f)*flareIntensity*(_sinf(FastNoise((flareScreenPos.x+flareScreenPos.y)*5.0f+_atan2f(uvT.x, uvT.y)*10.0f)*4.0f)*0.2f+3.5f*flareIntensity);
  // flare glow
  flareSpot+=to_float3(1.0f, 0.7f, 0.2f)*_powf(visibility, 12.0f)*0.3f;

  return (flareColor+flareSpot)*(1.0f-alpha);
}

__DEVICE__ void pR(inout float2 p, float a)
{
  p = _cosf(a)*p + _sinf(a)*to_float2(p.y, -p.x);
}

__DEVICE__ float DrawExplosion(int id, inout float4 color, float3 rayDir, float3 rayOrigin, float2 screenSpace, float enableFlare)
{
  id *= 100; 
  float dist =-10000.0f;
  float life = read(to_int2(122+id, 0));

  // check if explosion has been spawned
  if (life>0.0f )
  {     
    float3 pos = normalize(readRGB(to_int2(120+id, 0))-planePos); 
    float eDist = _powf(_fmaxf(0.0f, dot(pos, rayDir)), 2.0f); 
    float intensity =GetExplosionIntensity(life);
    dist = eDist*intensity*1.4f;
    swi3(color,x,y,z) += CalculateExplosionFlare(rayDir, rayOrigin, screenSpace, 1.0f-intensity, pos, enableFlare);
    swi3(color,x,y,z) = _mix(swi3(color,x,y,z), swi3(color,x,y,z)+to_float3(1.0f, 0.4f, 0)*0.5f, eDist*intensity);
  }   
  return dist;
}  


__DEVICE__ float3 AntiAliasing(float2 uv)
{
  float2 offset = to_float2(0.11218413712f, 0.33528304367f) * (1.0f / iResolution);

  return (texture(iChannel2, uv + to_float2(-offset.x, offset.y)) +
    texture(iChannel2, uv + to_float2( offset.y, offset.x)) +
    texture(iChannel2, uv + to_float2( offset.x, -offset.y)) +
    texture(iChannel2, uv + to_float2(-offset.y, -offset.x))).rgb * 0.25f;
}

__KERNEL__ void TurnNBurnFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
  
  fragCoord+=0.5f;
  
  float2 R=iResolution;
  
  float2 mo = iMouse.xy/iResolution;
  float2 uv = fragCoord / iResolution;
  float2 screenSpace = (-iResolution + 2.0f*(fragCoord))/iResolution.y;

  // read values from buffer
  float3 effects = readRGB(to_int2(20, 0));  
  float3 effects2 = readRGB(to_int2(22, 0)); 
  float turn = read(to_int2(1, 10));
  sunPos = readRGB(to_int2(50, 0));
  planePos = readRGB(to_int2(55, 0));
  // setup camera and ray direction
  float2 camRot = readRGB(to_int2(57, 0)).xy;
  float CAMZOOM = read(to_int2(52, 0));  
  float3 rayOrigin = to_float3_aw(CAMZOOM*_cosf(camRot.x), 3.0f+CAMZOOM*_sinf(camRot.y), -3.0f+CAMZOOM*_sinf(camRot.x) );
  pR(swi2(rayOrigin,x,z), -turn);
  cameraMatrix  = setCamera( rayOrigin, to_float3(0.0f, 0.0f, -3.0f ), 0.0f );
  float3 rayDir = cameraMatrix * normalize( to_float3(swi2(screenSpace,x,y), 2.0f) );

  float2 d = _fabs((uv - 0.5f) * 2.0f);
  d = _powf(d, to_float2(2.0f, 2.0f));
  float minDist = -1000.0f;


  float4 color;

  // chromatic aberration?
  if (effects.z>0.0f)
  {
    float offSet = distance(uv, to_float2_s(0.5f))*0.005f;
    // AA pass?
    if (effects2.y>0.0f)
    {
      swi3(color,x,y,z) = to_float3_aw(AntiAliasing(uv + offSet).r, AntiAliasing(uv).g, AntiAliasing(uv - offSet).b);
    } else
    {
      swi3(color,x,y,z) = to_float3_aw(texture(iChannel2, uv + offSet).r, _tex2DVecN(iChannel2,uv.x,uv.y,15).g, texture(iChannel2, uv - offSet).b);
    }
  }
  // no chromatic aberration 
  else
  {
    // AA pass?
    if (effects2.y>0.0f)
    {
      color.xyz=AntiAliasing(uv);
    } else
    {
      swi3(color,x,y,z) = _tex2DVecN(iChannel2,uv.x,uv.y,15).rgb;
    }
  }

  color.a=textureLod(iChannel2, uv, 0.0f).a;

  // add sun with lens flare effect
  swi3(color,x,y,z) += CalculateSunFlare(rayDir, rayOrigin, screenSpace, clamp(color.w, 0.0f, 1.0f),effects2.x);

  // add explosion light effects
  minDist = _fmaxf(minDist, DrawExplosion(0, color, rayDir, rayOrigin, screenSpace,effects2.x));
  minDist = _fmaxf(minDist, DrawExplosion(1, color, rayDir, rayOrigin, screenSpace,effects2.x));

  float cloudDistance = distance(rayOrigin.y+planePos.y, -CLOUDLEVEL);

  // grain noise
  if (effects.y>0.0f)
  {
    float2 grainTexPos = ((fragCoord + iTime*60.0f*to_float2(10, 35.0f))*_mix(0.6f, 0.2f, smoothstep(5.0f, 0.0f, cloudDistance)))/iChannelResolution[0].xy;
    float2 filmNoise = textureLod( iChannel1, grainTexPos, 0.0f ).rb;
    // scale up effect when flying through clouds
    swi3(color,x,y,z) *= _mix( to_float3(1), _mix(to_float3(1, 0.5f, 0), to_float3(0, 0.5f, 1), filmNoise.x), _mix(0.04f, 0.7f, smoothstep(5.0f, 0.0f, cloudDistance))*filmNoise.y );
  }

  // flying though clouds
  color = _mix(color, clamp(color+_fmaxf(0.4f, fastFBM(rayOrigin+planePos)*2.0f), 0.0f, 1.0f), smoothstep(5.0f, 0.0f, cloudDistance));


  // Lens dirt when looking into strong light source
  if (effects.x>0.0f)
  {
    minDist=_fmaxf(minDist, _powf(_fmaxf(0.0f, dot(sunPos, rayDir)), 2.0f));     
    float dirtTex = textureLod( iChannel3, (fragCoord / iResolution.x), 0.3f ).r*2.5f;

    swi3(color,x,y,z) += 0.04f*dirtTex*minDist;
  }

  fragColor =  to_float4(_powf(swi3(color,x,y,z), to_float3_aw(1.0f/1.1f)), 1.0f ) * (0.5f + 0.5f*_powf( 16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.2f ));


  SetFragmentShaderComputedColor(fragColor);
}