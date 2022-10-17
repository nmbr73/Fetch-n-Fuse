

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
////////////////////////////////////////////////////////////////////////////////////////////
// Copyright © 2017 Kim Berkeby
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
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

vec2 R;

  #define FastNoise(posX)   (  texture(iChannel1, (posX+0.5)/iResolution.xy).x)
  #define readAlpha(memPos) (  texture(iChannel2, memPos).w)
  //#define read(memPos)      (  texelFetch(iChannel0, memPos, 0).w)
  //#define readRGB(memPos)   (  texelFetch(iChannel0, memPos, 0).xyz)
  #define read(memPos)      (  texture(iChannel0, (vec2(memPos)+0.5)/R).w)
  #define readRGB(memPos)   (  texture(iChannel0, (vec2(memPos)+0.5)/R).xyz)
  
  
  #define CLOUDLEVEL -70.0
  #define PI acos(-1.)
  #pragma optimize(off) 
mat3 cameraMatrix;
vec3 planePos=vec3(0.);
vec3 sunPos=vec3(0.);
const vec3 eps = vec3(0.02, 0.0, 0.0);

float GetExplosionIntensity(float life)
{
  return mix(1., .0, smoothstep(0., 5.0, distance(life, 5.)));
}

// 3D noise function (IQ)
float fastFBM(vec3 p)
{
  vec3 ip=floor(p);
  p-=ip; 
  vec3 s=vec3(7, 157, 113);
  vec4 h=vec4(0., s.yz, s.y+s.z)+dot(ip, s);
  p=p*p*(3.-2.*p); 
  h=mix(fract(sin(h)*43758.5), fract(sin(h+s.x)*43758.5), p.x);
  h.xy=mix(h.xz, h.yw, p.y);
  return mix(h.x, h.y, p.z);
}

mat3 setCamera(  vec3 ro, vec3 ta, float cr )
{
  vec3 cw = normalize(ta-ro);
  vec3 cp = vec3(sin(cr), cos(cr), 0.0);
  vec3 cu = normalize( cross(cw, cp) );
  vec3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}

vec2 GetScreenPos(vec3 pos)
{
  return vec2(PI*dot( pos, cameraMatrix[0].xyz ), PI* dot( pos, cameraMatrix[1].xyz ));
}

vec3 CalculateSunFlare(vec3 rayDir, vec3 rayOrigin, vec2 screenSpace, float alpha, float enableFlare)
{
  float visibility = pow(max(0., dot(sunPos, rayDir)), 8.0);  
  if (visibility<=0.05) return vec3(0.);

  vec2 sunScreenPos = GetScreenPos(sunPos);

  vec2 uvT = screenSpace-sunScreenPos;
  float sunIntensity = (1.0/(pow(length(uvT)*4.0+1.0, 1.30)))*visibility;

  vec3 flareColor = vec3(0.);
  vec2 offSet = uvT;
  vec2 offSetStep=  0.4*sunScreenPos;
  vec3 color;
  float size=.0, dist=0.;
  
  if(enableFlare>0.)
  {
  // check if center of sun is covered by any object. MATH IS OFF AT SCREEN CHECK POS! sunScreenPos/2.0 +0.5 IS NOT EXACTLY SUN MIDDLE!
  // only draw if not covered by any object
  if (readAlpha( sunScreenPos/2.0 +0.5)<0.50)
  {
    // create flare rings
    for (float i =1.; i<8.; i++)
    {
      color.rg = vec2(abs((sin(i*53.))), 0.65);
      color.b = abs((cos(i*25.)));
      offSet += offSetStep;

      size = 0.05+((1.-sin(i*0.54))*0.28);
      dist = pow(distance(sunScreenPos, offSet), 1.20);

      flareColor += mix(vec3(0.), sunIntensity*(10.*size) * color, smoothstep(size, size-dist, dist))/(1.0-size);
    }
  }
  flareColor*=mix(0., 1.0, smoothstep(0., 0.1, visibility));
  }
    
  // flare star shape
  vec3 sunSpot = vec3(1.30, 1., .80)*sunIntensity*(sin(FastNoise((sunScreenPos.x+sunScreenPos.y)*2.3+atan(uvT.x, uvT.y)*15.)*5.0)*.12);
  // sun glow
  sunSpot+=vec3(1.0, 0.96, 0.90)*sunIntensity*.75;
  sunSpot+=vec3(1.0, 0.76, 0.20)*visibility*0.15;

  return flareColor+(sunSpot*(1.0-alpha));
}
vec3 CalculateExplosionFlare(vec3 rayDir, vec3 rayOrigin, vec2 screenSpace, float alpha, vec3 explosionPos, float enableFlare)
{

  float visibility = max(0., dot(explosionPos, rayDir));  
  if (visibility<=0.15) return vec3(0.);

  vec2 flareScreenPos = GetScreenPos(explosionPos);
  vec2 uvT = screenSpace-flareScreenPos;
  float flareIntensity = 0.2*visibility;
  vec3 flareColor = vec3(0.);
  vec2 offSet = uvT;
  vec2 offSetStep=  0.4*flareScreenPos;
  vec3 color;
  float size=.0, dist=0.; 

    if(enableFlare>0.)
    {
  // create flare rings
  for (float i =1.; i<8.; i++)
  {
    color.rg = vec2(0.75+(0.25*sin(i*i)));
    color.b = 0.75+(0.35*cos(i*i));
    offSet += offSetStep;
    size = 0.05+((1.-sin(i*0.54))*0.38);
    dist = pow(distance(flareScreenPos, offSet), 1.20);

    flareColor += mix(vec3(0.), flareIntensity*(4.*size) * color, smoothstep(size, size-dist, dist))/(1.0-size);
  }
  flareColor/=2.;
    }
  // flare star shape
  vec3 flareSpot = vec3(1.30, 1., .80)*flareIntensity*(sin(FastNoise((flareScreenPos.x+flareScreenPos.y)*5.+atan(uvT.x, uvT.y)*10.)*4.0)*.2+3.5*flareIntensity);
  // flare glow
  flareSpot+=vec3(1.0, 0.7, 0.2)*pow(visibility, 12.0)*0.3;

  return (flareColor+flareSpot)*(1.0-alpha);
}

void pR(inout vec2 p, float a)
{
  p = cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

float DrawExplosion(int id, inout vec4 color, vec3 rayDir, vec3 rayOrigin, vec2 screenSpace, float enableFlare)
{
  id *= 100; 
  float dist =-10000.;
  float life = read(ivec2(122+id, 0));

  // check if explosion has been spawned
  if (life>0. )
  {     
    vec3 pos = normalize(readRGB(ivec2(120+id, 0))-planePos); 
    float eDist = pow(max(0., dot(pos, rayDir)), 2.0); 
    float intensity =GetExplosionIntensity(life);
    dist = eDist*intensity*1.4;
    color.rgb += CalculateExplosionFlare(rayDir, rayOrigin, screenSpace, 1.0-intensity, pos, enableFlare);
    color.rgb = mix(color.rgb, color.rgb+vec3(1.0, 0.4, 0)*0.5, eDist*intensity);
  }   
  return dist;
}  


vec3 AntiAliasing(vec2 uv)
{
  vec2 offset = vec2(0.11218413712, 0.33528304367) * (1.0 / iResolution.xy);

  return (texture(iChannel2, uv + vec2(-offset.x, offset.y)) +
    texture(iChannel2, uv + vec2( offset.y, offset.x)) +
    texture(iChannel2, uv + vec2( offset.x, -offset.y)) +
    texture(iChannel2, uv + vec2(-offset.y, -offset.x))).rgb * 0.25;
}

void mainImage( out vec4 fragColor, vec2 fragCoord )
{  

  R=iResolution.xy;
  vec2 mo = iMouse.xy/iResolution.xy;
  vec2 uv = fragCoord.xy / iResolution.xy;
  vec2 screenSpace = (-iResolution.xy + 2.0*(fragCoord))/iResolution.y;

  // read values from buffer
  vec3 effects = readRGB(ivec2(20, 0));  
  vec3 effects2 = readRGB(ivec2(22, 0)); 
  float turn = read(ivec2(1, 10));
  sunPos = readRGB(ivec2(50, 0));
  planePos = readRGB(ivec2(55, 0));
  // setup camera and ray direction
  vec2 camRot = readRGB(ivec2(57, 0)).xy;
  float CAMZOOM = read(ivec2(52, 0));  
  vec3 rayOrigin = vec3(CAMZOOM*cos(camRot.x), 3.+CAMZOOM*sin(camRot.y), -3.+CAMZOOM*sin(camRot.x) );
  pR(rayOrigin.xz, -turn);
  cameraMatrix  = setCamera( rayOrigin, vec3(0., 0., -3. ), 0.0 );
  vec3 rayDir = cameraMatrix * normalize( vec3(screenSpace.xy, 2.0) );

  vec2 d = abs((uv - 0.5) * 2.0);
  d = pow(d, vec2(2.0, 2.0));
  float minDist = -1000.0;


  vec4 color;

  // chromatic aberration?
  if (effects.z>0.)
  {
    float offSet = distance(uv, vec2(0.5))*0.005;
    // AA pass?
    if (effects2.y>0.)
    {
      color.rgb = vec3(AntiAliasing(uv + offSet).r, AntiAliasing(uv).g, AntiAliasing(uv - offSet).b);
    } else
    {
      color.rgb = vec3(texture(iChannel2, uv + offSet).r, texture(iChannel2, uv).g, texture(iChannel2, uv - offSet).b);
    }
  }
  // no chromatic aberration 
  else
  {
    // AA pass?
    if (effects2.y>0.)
    {
      color.rgb=AntiAliasing(uv);
    } else
    {
      color.rgb = texture(iChannel2, uv).rgb;
    }
  }

  color.a=textureLod(iChannel2, uv, 0.).a;

  // add sun with lens flare effect
  color.rgb += CalculateSunFlare(rayDir, rayOrigin, screenSpace, clamp(color.a, 0., 1.0),effects2.x);

  // add explosion light effects
  minDist = max(minDist, DrawExplosion(0, color, rayDir, rayOrigin, screenSpace,effects2.x));
  minDist = max(minDist, DrawExplosion(1, color, rayDir, rayOrigin, screenSpace,effects2.x));

  float cloudDistance = distance(rayOrigin.y+planePos.y, -CLOUDLEVEL);

  // grain noise
  if (effects.y>0.)
  {
    vec2 grainTexPos = ((fragCoord.xy + iTime*60.0*vec2(10, 35.))*mix(0.6, 0.2, smoothstep(5.0, 0., cloudDistance)))/iChannelResolution[0].xy;
    vec2 filmNoise = textureLod( iChannel1, grainTexPos, 0. ).rb;
    // scale up effect when flying through clouds
    color.rgb *= mix( vec3(1), mix(vec3(1, .5, 0), vec3(0, .5, 1), filmNoise.x), mix(.04, 0.7, smoothstep(5.0, 0., cloudDistance))*filmNoise.y );
  }

  // flying though clouds
  color = mix(color, clamp(color+max(0.4, fastFBM(rayOrigin+planePos)*2.), 0., 1.0), smoothstep(5.0, 0., cloudDistance));


  // Lens dirt when looking into strong light source
  if (effects.x>0.)
  {
    minDist=max(minDist, pow(max(0., dot(sunPos, rayDir)), 2.0));     
    float dirtTex = textureLod( iChannel3, (fragCoord.xy / iResolution.x), 0.3 ).r*2.5;

    color.rgb += 0.04*dirtTex*minDist;
  }

  fragColor =  vec4(pow(color.rgb, vec3(1.0/1.1)), 1.0 ) * (0.5 + 0.5*pow( 16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.2 ));
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//////////////////////////////////////////////////////////////////////////////////////
// DATA BUFFER  -  PLANE MOVEMENT, KEYBOARD CHECKS AND MISSILE UPDATE (IF LAUNCHED)
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Keyoard input. Used to capture key-presses.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = This buffer (A). Read and write data to update movement in this shader.
// Channel 3 = Lichen texture. Used to create landscape height map used in collision detection.

vec2 R;

  #define PI acos(-1.)
  #define keyClick(ascii)   ( texelFetch(iChannel0, ivec2(ascii, 0), 0).x > 0.)
  #define keyPress(ascii)   ( texelFetch(iChannel0, ivec2(ascii, 1), 0).x > 0.)
  //#define read(memPos)      ( texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos)   ( texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (vec2(memPos)+0.5)/R).w)
  #define readRGB(memPos)   (  texture(iChannel2, (vec2(memPos)+0.5)/R).xyz)
  
  #define MAX_HEIGHT 150. 
  #define MIN_HEIGHT 0. 
  #define STARTHEIGHT 40.
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


vec3 sunPos=vec3(0.);
vec3 planePos=vec3(0.);
float explosionCount=0.;


struct Missile
{ 
  vec3 pos;
  float life;
  vec3 orientation;   // roll,pitch,turn amount
    vec3 startPos;
};

struct Explosion
{ 
  vec3 pos;
  float life;
};



mat3 setCamera(  vec3 ro, vec3 ta, float cr )
{
  vec3 cw = normalize(ta-ro);
  vec3 cp = vec3(sin(cr), cos(cr), 0.0);
  vec3 cu = normalize( cross(cw, cp) );
  vec3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}

mat2 r2(float r) {
  float c=cos(r), s=sin(r);
  return mat2(c, s, -s, c);
}

void pR(inout vec2 p, float a) 
{
  p*=r2(a);
}

float noise2D( in vec2 pos, float lod)
{   
  vec2 f = fract(pos);
  f = f*f*(3.0-2.0*f);
  vec2 rg = textureLod( iChannel1, (((floor(pos).xy+vec2(37.0, 17.0)) + f.xy)+ 0.5)/64.0, lod).yx;  
  return -1.0+2.0*mix( rg.x, rg.y, 0.5 );
}

float noise2D( in vec2 pos )
{
  return noise2D(pos, 0.0);
}

// 3D noise function (IQ)
float fastFBM(vec3 p)
{
  vec3 ip=floor(p);
  p-=ip; 
  vec3 s=vec3(7, 157, 113);
  vec4 h=vec4(0., s.yz, s.y+s.z)+dot(ip, s);
  p=p*p*(3.-2.*p); 
  h=mix(fract(sin(h)*43758.5), fract(sin(h+s.x)*43758.5), p.x);
  h.xy=mix(h.xz, h.yw, p.y);
  return mix(h.x, h.y, p.z);
}

float hash(float h) 
{
  return fract(sin(h) * 43758.5453123);
}

float noise(vec3 x) 
{
  vec3 p = floor(x);
  vec3 f = fract(x);
  f = f * f * (3.0 - 2.0 * f);

  float n = p.x + p.y * 157.0 + 113.0 * p.z;
  return -1.0+2.0*mix(
    mix(mix(hash(n + 0.0), hash(n + 1.0), f.x), 
    mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y), 
    mix(mix(hash(n + 113.0), hash(n + 114.0), f.x), 
    mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

float NoTreeZone(vec3 p)
{
    float dist = distance(readRGB(ivec2(140, 0)).xz,p.xz);
    dist = min(dist,distance(readRGB(ivec2(142, 0)).xz,p.xz));
    dist = min(dist,distance(readRGB(ivec2(144, 0)).xz,p.xz));
    dist = min(dist,distance(readRGB(ivec2(146, 0)).xz,p.xz));
    dist = min(dist,distance(readRGB(ivec2(148, 0)).xz,p.xz));
    return dist;
}
float GetTerrainHeight( vec3 p)
{
  vec2 p2 = (p.xz+planePos.xz)*0.0005;

  float heightDecrease = mix(1.0,0.,smoothstep(0.,15.0,NoTreeZone(p+planePos)));
    
  float mainHeight = -2.3+fastFBM((p+vec3(planePos.x, 0., planePos.z))*0.025)*max(11., abs(22.*noise2D(p2))); 
  mainHeight-=heightDecrease;
    
  float terrainHeight=mainHeight;
  p2*=4.0;
  terrainHeight += textureLod( iChannel3, p2, 2.7 ).x*1.; 
  p2*=2.0;
  terrainHeight -= textureLod( iChannel3, p2, 1.2 ).x*.7;
  p2*=3.0;
  terrainHeight -= textureLod( iChannel3, p2, 0.5 ).x*.1;

  terrainHeight=mix(terrainHeight, mainHeight*1.4, smoothstep(1.5, 3.5, terrainHeight)); 

  return   terrainHeight;
}

float GetTreeHeight( vec3 p, float terrainHeight)
{
  if(NoTreeZone(p+planePos)<25.) return 0.;
  float treeHeight = textureLod(iChannel3, (p.xz+planePos.xz)*0.006, .1).x;
  float tree = mix(0., mix(0., mix(0., 2.0, smoothstep(0.3, 0.86, treeHeight)), smoothstep(1.5, 3.5, terrainHeight)), step(0.3, treeHeight)); 
  tree -= tree*0.75;
  tree*=4.0;

  return  tree;
}

vec3 TranslatePos(vec3 p, float _direction, float _pitch, float _roll)
{
  pR(p.xz, _direction);
  pR(p.zy, _pitch);

  return p;
}

void LaunchMissile(inout Missile missile, vec3 startPos, vec3 orientation)
{
  missile.life=4.0; 
  missile.orientation = orientation;
  missile.pos =  startPos;
  missile.startPos= planePos;
  missile.orientation.y *=cos(missile.orientation.x-PI);
}

void UpdateMissile(inout Missile missile, float id, inout vec4 fragColor, vec2 fragCoord, vec3 moveDiff)
{
  float adressStep = id*100.;
     
  Explosion explosion;
 
  // read variables for explosion s
  explosion.pos = readRGB(ivec2(120+int(adressStep), 0));    
  explosion.life = read(ivec2(122+int(adressStep), 0));

  // update active missile and save variables
  if ( missile.life>0.)
  {
    missile.life-= 0.015;
    vec3 velocityAdd = vec3(0., 0., 1.4);

    pR(velocityAdd.yz, missile.orientation.y);
    pR(velocityAdd.xz, -missile.orientation.z);

    missile.pos += velocityAdd; // add velocity movement to pos
    missile.pos.xz-=moveDiff.xz; // add plane movement to pos

    // ground collision check                 
    vec3 testPoint = missile.pos;
      
    testPoint+=vec3(4.8 - (9.6*id), -0.4, -3.0);
    pR(testPoint.xz, missile.orientation.z);
    testPoint-=vec3(4.8 - (9.6*id), -0.4, -3.0);
    testPoint.y+=missile.startPos.y;
      
    float tHeight = GetTerrainHeight(testPoint);
    tHeight+=GetTreeHeight(testPoint, tHeight);

    // does missile hit terrain?
    if (testPoint.y<tHeight)
    {
      // if colliding, kill missile and spawn explosion.             
       explosion.pos =  missile.pos+missile.startPos;
       explosion.pos.y = tHeight-3.0;
       explosion.life=10.0;
       missile.life=-10.;
       explosionCount+=2.0;
       explosionCount = mod(explosionCount,10.);
    }

    fragColor.a = mix(missile.life, fragColor.a, step(1., distance(fragCoord.xy, vec2(100.0+adressStep, 0.0))));
    fragColor.rgb = mix(missile.startPos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(102.0+adressStep, 0.0))));
    fragColor.rgb = mix(missile.orientation, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(108.0+adressStep, 0.0)))); 
    fragColor.rgb = mix(missile.pos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(116.0+adressStep, 0.0))));
            
  }
  // ##################################################################

  // update explosion
  if ( explosion.life>0.)
  {   
    explosion.life-= 0.115;
   // explosion.life= 9.715;
    fragColor.rgb = mix(explosion.pos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(120.0+adressStep, 0.0)))); 
    fragColor.a = mix(explosion.life, fragColor.a, step(1., distance(fragCoord.xy, vec2(122.0+adressStep, 0.0))));
      
    // terrain holes
    fragColor.rgb = mix(mix(explosion.pos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(140.0+explosionCount, 0.0)))),fragColor.rgb,step(0.4,distance(5.0,explosion.life)));
  }
}


void ToggleEffects(inout vec4 fragColor, vec2 fragCoord)
{
   // read and save effect values from buffer  
   vec3 effects =  mix(vec3(-1.0,1.0,1.0), readRGB(ivec2(20, 0)), step(1.0, float(iFrame)));
   effects.x*=1.0+(-2.*float(keyPress(49))); //1-key  LENSDIRT
   effects.y*=1.0+(-2.*float(keyPress(50))); //2-key  GRAINFILTER
   effects.z*=1.0+(-2.*float(keyPress(51))); //3-key  ChromaticAberration
   
   vec3 effects2 =  mix(vec3(1.0,1.0,1.0), readRGB(ivec2(22, 0)), step(1.0, float(iFrame)));
   effects2.y*=1.0+(-2.*float(keyPress(52))); //4-key  AA-pass
   effects2.x*=1.0+(-2.*float(keyPress(53))); //5-key  lens flare

   fragColor.rgb = mix(effects, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(20.0, 0.0))));  
   fragColor.rgb = mix(effects2, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(22.0, 0.0))));  
}


void mainImage( out vec4 fragColor, vec2 fragCoord )
{  

    R=iResolution.xy;

  vec2 mo = iMouse.xy/iResolution.xy;
  vec2 uv = fragCoord.xy / iResolution.xy;
  vec2 screenSpace = (-iResolution.xy + 2.0*(fragCoord))/iResolution.y;

  // read plane values from buffer
  float turn = mix(1.0, read(ivec2(1, 10)), step(1.0, float(iFrame)));
  float roll = mix(3.14, read(ivec2(1, 1)), step(1.0, float(iFrame)));
  float rudderAngle = read(ivec2(6, 1));
  float speed = read(ivec2(10, 1));
  float pitch = read(ivec2(15, 1));
  explosionCount = read(ivec2(3, 0));  
    
  sunPos = mix(normalize( vec3(-1.0, 0.3, -.50) ), readRGB(ivec2(50, 0)), step(1.0, float(iFrame)));
  planePos = mix(vec3(-400, STARTHEIGHT, -100), readRGB(ivec2(55, 0)), step(1.0, float(iFrame)));
  float CAMZOOM = mix(13.9, read(ivec2(52, 0)), step(1.0, float(iFrame)));  
  vec2 camRot = vec2(-1., 0.340);

  // setup camera and ray direction
  camRot.x+=mo.x*16.; 
  camRot.y+=mo.y*16.; 
 
  // limit roll
  roll=mod(roll, 6.28);
 
    // add turn angle based on roll  
  float turnAmount = mix(0., 1.57, smoothstep(0., 1.57, 1.57-distance(1.57, roll-3.14)));
  turnAmount += mix(0., -1.57, smoothstep(0., 1.57, 1.57-distance(-1.57, roll-3.14)));
  float PitchAdd = sin(pitch);
 
  // YAW
  turn+=0.02*rudderAngle;
  // add turn angle  
  turn+=turnAmount*0.015;
  turn-=0.1*(((pitch*0.25)*cos(roll-1.57)));
  
    turn= mod(turn,PI*2.);
  vec3 oldPlanePos = vec3(planePos.x, planePos.y, planePos.z);

  // move plane
  planePos.xz += vec2(cos(turn+1.5707963)*0.5,  sin(turn+1.5707963)*0.5)*(0.7+speed)*cos(pitch);
  planePos.y = clamp(planePos.y+((PitchAdd*0.25)*cos(roll-PI)), MIN_HEIGHT, MAX_HEIGHT);

  rudderAngle*=0.97;
  // check key inputs
  rudderAngle-=0.03*float(keyClick(LEFT_KEY));
  rudderAngle+=0.03*float(keyClick(RIGHT_KEY));
  rudderAngle=clamp(rudderAngle, -0.4, 0.4);;
  roll-=0.055*float(keyClick(ROLL_LEFT_KEY));
  roll+=0.055*float(keyClick(ROLL_RIGHT_KEY));

  speed+=(0.02*float(keyClick(SPEED_INCREASE_KEY)));
  speed-=(0.02*float(keyClick(SPEED_DECREASE_KEY)));
  speed=clamp(speed, -0.3, 1.);
   
  // prevent plane from getting into terrain
  float tHeight = GetTerrainHeight(planePos);
  tHeight+=GetTreeHeight(planePos, tHeight);
  float minHeight = tHeight+12.;
  planePos.y = max(planePos.y,minHeight);
    
   // pitch = sin(pitch);
  pitch-=(mix(0.02, 0., smoothstep(0., 3., 3.0-abs(distance(planePos.y, minHeight))))*float(keyClick(UP_KEY))); //e-key
  pitch+=(mix(0.02, 0., smoothstep(0., 3., 3.0-abs(distance(planePos.y, MAX_HEIGHT))))*float(keyClick(DOWN_KEY))); //d-key
  pitch = clamp(pitch, -1.25, 1.25);
  pitch*=0.97;

  turnAmount += mix(0., -1.57, smoothstep(0., 1.57, 1.57-distance(-1.57, roll-3.14)));
  fragColor = vec4(textureLod(iChannel2, uv,0.).rgb,0.);
    
  // ------------------------- MISSILES ------------------------------
  // NOTE: MISSILES ARE RENDERED IN BUFFER B TOGETHER WITH THE TERRAIN     
  int adressStep = 0;
  bool launchLocked=false;
  Missile missile;
  for (int i=0; i<2; i++)
  {
    adressStep = i*100;
      
    // read variables for missiles
    missile.life = read(ivec2(100 + adressStep, 0));
    missile.startPos = readRGB(ivec2(102 + adressStep, 0));  
    missile.orientation = readRGB(ivec2(108 + adressStep, 0));
    missile.pos = readRGB(ivec2(116 + adressStep, 0));

  // if missile is "dead" check if a new missile is being lanched by pressing the M-key
  if (keyPress(MISSILE_KEY) && !launchLocked)
  {    
   if (missile.life<=0.)
   {
      LaunchMissile(missile, vec3(4.8- (9.6*float(i)), -0.4, -3.0), vec3(roll, pitch, turn));  
      launchLocked=true;
   } 
 }    

  UpdateMissile(missile, float(i), fragColor, fragCoord, (planePos-oldPlanePos));
  // ##################################################################
  }

  ToggleEffects(fragColor, fragCoord);
   
  CAMZOOM-=0.3*float(keyClick(ZOOMIN_KEY));
  CAMZOOM+=0.3*float(keyClick(ZOOMOUT_KEY));
  CAMZOOM=clamp(CAMZOOM, 10., 30.);;
  
  // save roll,speed and scroll values etc to buffer A 
  fragColor.a = mix(turn, fragColor.a, step(1., distance(fragCoord.xy, vec2(1.0, 10.0)))); 
  fragColor.a = mix(speed, fragColor.a, step(1., distance(fragCoord.xy, vec2(10.0, 1.0)))); 
  fragColor.a = mix(roll, fragColor.a, step(1., distance(fragCoord.xy, vec2(1.0, 1.0)))); 
  fragColor.a = mix(pitch, fragColor.a, step(1., distance(fragCoord.xy, vec2(15.0, 1.0)))); 
  fragColor.a = mix(rudderAngle, fragColor.a, step(1., distance(fragCoord.xy, vec2(6.0, 1.0))));
  fragColor.a = mix(explosionCount, fragColor.a, step(1., distance(fragCoord.xy, vec2(3.0, 0.0)))); 
  fragColor.rgb = mix(sunPos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(50.0, 0.0))));
  fragColor.a = mix(CAMZOOM, fragColor.a, step(1., distance(fragCoord.xy, vec2(52.0, 0.0))));
  fragColor.rgb = mix(planePos, fragColor.rgb, step(1., distance(fragCoord.xy, vec2(55.0, 0.0))));
  fragColor.rgb = mix(vec3(camRot.xy, 0.), fragColor.rgb, step(1., distance(fragCoord.xy, vec2(57.0, 0.0))));
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//////////////////////////////////////////////////////////////////////////////////////
// TERRAIN BUFFER  -   RENDERS TERRAIN AND LAUNCHED MISSILES + EXPLOSIONS 
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Fine noise texture. Used in noise functions.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = Buffer A. Read data from data-buffer.
// Channel 3 = Lichen texture. Used to create landscape height map and textures.

vec2 R;

  //#define read(memPos)    (  texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos) (  texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (vec2(memPos)+0.5)/R).w)
  #define readRGB(memPos)   (  texture(iChannel2, (vec2(memPos)+0.5)/R).xyz)
  
  #define MAX_HEIGHT 150. 
  #define WATER_LOD 0.4
  #define CLOUDLEVEL -70.0
  #define PI acos(-1.)
  #pragma optimize(off) 
  // remove on or several of below defines, if FPS is too low
  #define SHADOWS
  #define QUALITY_TREE
  #define QUALITY_REFLECTIONS
  #define EXACT_EXPLOSIONS
  // ---------------------------------------------------------

float turn=0.;
vec2 cloudPos=vec2(0.);
float eFlameDist=10000.0;
vec3 checkPos=vec3(0.);
vec3 sunPos=vec3(0.);
const vec3 sunColor = vec3(1.00, 0.90, 0.85);

const vec3 eps = vec3(0.02, 0.0, 0.0);
vec3 planePos=vec3(0.);

struct RayHit
{
  bool hit;  
  vec3 hitPos;
  vec3 normal;
  float dist;
  float depth;
  float eFlameDist;
};

struct Missile
{ 
  vec3 pos;
  float life;
  vec3 orientation;   // roll,pitch,turn amount
  vec3 origin;
};
    
struct Explosion
{ 
  vec3 pos;
  float life;
};

mat2 r2(float r) {
  float c=cos(r), s=sin(r);
  return mat2(c, s, -s, c);
}

#define r3(r) mat2(sin(vec4(-1, 0, 0, 1)*acos(0.)+r))

void pR(inout vec2 p, float a)
{
  p*=r2(a);
}

float sgn(float x)
{   
  return (x<0.)?-1.:1.;
}

float hash(float h) 
{
  return fract(sin(h) * 43758.5453123);
}

float noise(vec3 x) 
{
  vec3 p = floor(x);
  vec3 f = fract(x);
  f = f * f * (3.0 - 2.0 * f);

  float n = p.x + p.y * 157.0 + 113.0 * p.z;
  return -1.0+2.0*mix(
    mix(mix(hash(n + 0.0), hash(n + 1.0), f.x), 
    mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y), 
    mix(mix(hash(n + 113.0), hash(n + 114.0), f.x), 
    mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

float fbm(vec3 p) 
{
  float f = 0.5000 * noise(p);
  p *= 2.01;
  f += 0.2500 * noise(p);
  p *= 2.02;
  f += 0.1250 * noise(p);
  return f;
}

float noise2D( in vec2 pos, float lod)
{   
  vec2 f = fract(pos);
  f = f*f*(3.0-2.0*f);
  vec2 rg = textureLod( iChannel1, (((floor(pos).xy+vec2(37.0, 17.0)) + f.xy)+ 0.5)/64.0, lod).yx;  
  return -1.0+2.0*mix( rg.x, rg.y, 0.5 );
}
float noise2D( in vec2 pos )
{
  return noise2D(pos, 0.0);
}

// 3D noise function (IQ)
float fastFBM(vec3 p)
{
  vec3 ip=floor(p);
  p-=ip; 
  vec3 s=vec3(7, 157, 113);
  vec4 h=vec4(0., s.yz, s.y+s.z)+dot(ip, s);
  p=p*p*(3.-2.*p); 
  h=mix(fract(sin(h)*43758.5), fract(sin(h+s.x)*43758.5), p.x);
  h.xy=mix(h.xz, h.yw, p.y);
  return mix(h.x, h.y, p.z);
}
float fastFBMneg(vec3 p)
{
  return -1.0+2.0*fastFBM(p);
}
vec2 pModPolar(vec2 p, float repetitions) {
  float angle = 2.*PI/repetitions;
  float a = atan(p.y, p.x) + angle/2.;
  float r = length(p);
  float c = floor(a/angle);
  a = mod(a, angle) - angle/2.;
  p = vec2(cos(a), sin(a))*r;
  if (abs(c) >= (repetitions/2.)) c = abs(c);
  return p;
}

float pModInterval1(inout float p, float size, float start, float stop) {
  float halfsize = size*0.5;
  float c = floor((p + halfsize)/size);
  p = mod(p+halfsize, size) - halfsize;
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
float pMirror (inout float p, float dist) {
  float s = sgn(p);
  p = abs(p)-dist;
  return s;
}

float fCylinder(vec3 p, float r, float height) {
  float d = length(p.xy) - r;
  d = max(d, abs(p.z) - height);
  return d;
}
float sdEllipsoid( vec3 p, vec3 r )
{
  return (length( p/r.xyz ) - 1.0) * r.y;
}
float sdHexPrism( vec3 p, vec2 h )
{
  vec3 q = abs(p);
  return max(q.y-h.y, max((q.z*0.866025+q.x*0.5), q.x)-h.x);
}
float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}
float fSphere(vec3 p, float r) {
  return length(p) - r;
}

float GetExplosionIntensity(Explosion ex)
{
  return mix(1., .0, smoothstep(0., 5.0, distance(ex.life, 5.)));
}

float NoTreeZone(vec3 p)
{
  float dist = distance(readRGB(ivec2(140, 0)).xz, p.xz);
  dist = min(dist, distance(readRGB(ivec2(142, 0)).xz, p.xz));
  dist = min(dist, distance(readRGB(ivec2(144, 0)).xz, p.xz));
  dist = min(dist, distance(readRGB(ivec2(146, 0)).xz, p.xz));
  dist = min(dist, distance(readRGB(ivec2(148, 0)).xz, p.xz));
  return dist;
}
float GetTerrainHeight( vec3 p)
{
  vec2 p2 = (p.xz+planePos.xz)*0.0005;

  float heightDecrease = mix(1.0, 0., smoothstep(0., 15.0, NoTreeZone(p+planePos)));

  float mainHeight = -2.3+fastFBM((p+vec3(planePos.x, 0., planePos.z))*0.025)*max(11., abs(22.*noise2D(p2))); 
  mainHeight-=heightDecrease;

  float terrainHeight=mainHeight;
  p2*=4.0;
  terrainHeight += textureLod( iChannel3, p2, 2.7 ).x*1.; 
  p2*=2.0;
  terrainHeight -= textureLod( iChannel3, p2, 1.2 ).x*.7;
  p2*=3.0;
  terrainHeight -= textureLod( iChannel3, p2, 0.5 ).x*.1;

  terrainHeight=mix(terrainHeight, mainHeight*1.4, smoothstep(1.5, 3.5, terrainHeight)); 

  return   terrainHeight;
}

float GetTreeHeight( vec3 p, float terrainHeight)
{
  if (NoTreeZone(p+planePos)<25.) return 0.;
  float treeHeight = textureLod(iChannel3, (p.xz+planePos.xz)*0.006, .1).x;
  float tree = mix(0., mix(0., mix(0., 2.0, smoothstep(0.3, 0.86, treeHeight)), smoothstep(1.5, 3.5, terrainHeight)), step(0.3, treeHeight)); 
  tree -= tree*0.75;
  tree*=4.0;

  return  tree;
}

float MapTerrainSimple( vec3 p)
{
  float terrainHeight = GetTerrainHeight(p);   
  return  p.y - max((terrainHeight+GetTreeHeight(p, terrainHeight)), 0.);
}

float GetStoneHeight(vec3 p, float terrainHeight)
{
  return (textureLod(iChannel1, (p.xz+planePos.xz)*0.05, 0.).x*max(0., -0.3+(1.25*terrainHeight)));
}

float MapTerrain( vec3 p)
{   
  float terrainHeight = GetTerrainHeight(p);   
  terrainHeight= mix(terrainHeight+GetStoneHeight(p, terrainHeight), terrainHeight, smoothstep(0., 1.5, terrainHeight));
  terrainHeight= mix(terrainHeight+(textureLod(iChannel1, (p.xz+planePos.xz)*0.0015, 0.).x*max(0., -0.3+(.5*terrainHeight))), terrainHeight, smoothstep(1.2, 12.5, terrainHeight));

  terrainHeight= mix(terrainHeight-0.30, terrainHeight, smoothstep(-0.5, 0.25, terrainHeight));
  float water=0.;
  if (terrainHeight<=0.)
  {   
    water = (-0.5+(0.5*(noise2D((p.xz+planePos.xz+ vec2(-iTime*0.4, iTime*0.25))*2.60, WATER_LOD))));
    water*=(-0.5+(0.5*(noise2D((p.xz+planePos.xz+ vec2(iTime*.3, -iTime*0.25))*2.90), WATER_LOD)));
  }
  return   p.y -  max((terrainHeight+GetTreeHeight(p, terrainHeight)), -water*0.04);
}


float MapTree( vec3 p)
{  
  float terrainHeight = GetTerrainHeight(p);
  float treeHeight =GetTreeHeight(p, terrainHeight);

  // get terrain height at position and tree height onto that
  return  p.y - terrainHeight-treeHeight;
}

vec3 calcTreeNormal( in vec3 pos )
{    
  return normalize( vec3(MapTree(pos+eps.xyy) - MapTree(pos-eps.xyy), 0.5*2.0*eps.x, MapTree(pos+eps.yyx) - MapTree(pos-eps.yyx) ) );
}

vec4 TraceTrees( vec3 origin, vec3 direction, int steps, float terrainHeight)
{
  vec4 treeCol =vec4(0.5, 0.5, 0.5, 0.0);
  float intensity=0.0, t = .0, dist = 0.0;
  vec3 rayPos, nn;
  float precis=.0, dif =0.0, densAdd =.0;
  float treeHeight = 0.0;
  float td =.0;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    treeHeight = GetTreeHeight(rayPos, terrainHeight);
    dist = rayPos.y - (terrainHeight + treeHeight);  
    precis = 0.015*t;

    if (treeHeight>0.1 && dist<precis)
    {
      nn= calcTreeNormal(rayPos);  
      dif = clamp( dot( nn, sunPos ), 0.0, 1.0 );

      densAdd = (precis-dist)*3.0*td;
      treeCol.rgb+=(0.5*td)*dif;
      treeCol.a+=(1.-treeCol.a)*densAdd;
    } 
    if (treeCol.a > 0.99) 
    {
      break;
    }
    td = max(0.04, dist*0.5);
    t+=td;
  }

  return clamp(treeCol, 0., 1.);
}


RayHit TraceTerrainReflection( vec3 origin, vec3 direction, int steps)
{
  RayHit result;
  float precis = 0.0, maxDist = 100.0, t = 0.0, dist = 0.0;
  vec3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapTerrainSimple( rayPos);
    precis = 0.01*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5;
  }

  return result;
}

RayHit TraceTerrain( vec3 origin, vec3 direction, int steps)
{
  RayHit result;
  float precis = 0.0, maxDist = 400.0, t = 0.0, dist = 0.0;
  vec3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapTerrain( rayPos);
    precis = 0.001*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5;
  }

  return result;
}

float SoftShadow( in vec3 origin, in vec3 direction )
{
  float res = 2.0, t = 0.0, h;
  for ( int i=0; i<16; i++ )
  {
    h = MapTerrain(origin+direction*t);
    res = min( res, 3.5*h/t );
    t += clamp( h, 0.02, 0.8);
    if ( h<0.002 ) break;
  }
  return clamp( res, 0.0, 1.0 );
}


vec3 calcNormal( in vec3 pos )
{    
  return normalize( vec3(MapTerrain(pos+eps.xyy) - MapTerrain(pos-eps.xyy), 0.5*2.0*eps.x, MapTerrain(pos+eps.yyx) - MapTerrain(pos-eps.yyx) ) );
}

float GetCloudHeight(vec3 p)
{    
  vec3 p2 = (p+vec3(planePos.x, 0., planePos.z)+vec3(cloudPos.x, 0., cloudPos.y))*0.03;

  float i  = (-0.3+noise(p2))*4.4; 
  p2*=2.52;
  i +=abs(noise( p2 ))*1.7; 
  p2*=2.53;
  i += noise( p2 )*1.; 
  p2*=2.51;
  i += noise(p2 )*0.5;
  p2*=4.22;
  i += noise( p2)*0.2;
  return i*3.;
}

float GetCloudHeightBelow(vec3 p)
{    
  vec3 p2 = (p+vec3(planePos.x, 0., planePos.z)+vec3(cloudPos.x, 0., cloudPos.y))*0.03;

  float i  = (-0.3+noise(p2))*4.4; 
  p2*=2.52;
  i +=noise( p2 )*1.7; 
  p2*=2.53;
  i += noise( p2 )*1.; 
  p2*=2.51;
  i += noise(p2 )*0.5;
  p2*=3.42;
  i += noise( p2)*0.2;
  i*=0.5;
  i-=0.25*i; 

  return i*5.;
}

float GetHorizon( vec3 p)
{
  return sdEllipsoid(p, vec3(1000., -CLOUDLEVEL, 1000.));
}

float MapCloud( vec3 p)
{
  return GetHorizon(p) - max(-3., (1.3*GetCloudHeight(p)));
}

vec4 TraceClouds( vec3 origin, vec3 direction, vec3 skyColor, int steps)
{
  vec4 cloudCol=vec4(skyColor*vec3(0.65, 0.69, 0.72)*1.3, 0.0);
  cloudCol.rgb=mix(cloudCol.rgb, sunColor, 0.32);

  float density = 0.0, t = .0, dist = 0.0;
  vec3 rayPos;
  float precis; 
  float td =.0;
  float densAdd;
  float sunDensity;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    density = max(-5., 1.7+(GetCloudHeight(rayPos)*1.3));
    dist = GetHorizon(rayPos)-(density);

    precis = 0.01*t;
    if (dist<precis && density>-5.1)
    {    
      sunDensity = MapCloud(rayPos+sunPos*3.);
      densAdd =  mix(0., 0.5*(1.0-cloudCol.a), smoothstep(-5.1, 4.3, density));
      cloudCol.rgb-=clamp((density-sunDensity), 0., 1.0)*0.06*sunColor*densAdd;
      cloudCol.rgb += 0.003*max(0., sunDensity)*density*densAdd;
      

      cloudCol.a+=(1.-cloudCol.a)*densAdd;
    } 

    if (cloudCol.a > 0.99) break; 

    td = max(0.12, dist*0.45);
    t+=td;
  }

  // mix clouds color with sky color
  float mixValue = smoothstep(100., 620., t);
  cloudCol.rgb = mix(cloudCol.rgb, skyColor, mixValue);

  return cloudCol;
}

vec4 TraceCloudsBelow( vec3 origin, vec3 direction, vec3 skyColor, int steps)
{
  vec4 cloudCol=vec4(vec3(0.95, 0.95, 0.98)*0.7, 0.0);
  cloudCol.rgb=mix(cloudCol.rgb, sunColor, 0.2);

  float density = 0.0, t = .0, dist = 0.0;
  vec3 rayPos;
  float precis; 
  float td =.0;
  float energy=1.0;
  float densAdd=0.;
  float sunDensity;

  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    density = clamp(GetCloudHeightBelow(rayPos), 0., 1.)*2.;          
    dist = -GetHorizon(rayPos);

    precis = 0.015*t;
    if (dist<precis && density>0.001)
    {    
      densAdd = 0.14*density/td;
      sunDensity = clamp(GetCloudHeightBelow(rayPos+sunPos*3.), -0.6, 2.)*2.; 
      cloudCol.rgb-=sunDensity*0.02*cloudCol.a*densAdd; 
      cloudCol.a+=(1.-cloudCol.a)*densAdd;

      cloudCol.rgb += 0.03*max(0., density-sunDensity)*densAdd;

      cloudCol.rgb+=mix(vec3(0.), vec3(1.0, 1.0, 0.9)*0.013, energy)*sunColor;
      energy*=0.96;
    } 

    if (cloudCol.a > 0.99) break; 

    td = max(1.4, dist);
    t+=td;
  }
    
  // mix clouds color with sky color
  cloudCol.rgb = mix(cloudCol.rgb, vec3(0.97), smoothstep(100., 960., t)); 
  cloudCol.a = mix(cloudCol.a, 0., smoothstep(0., 960., t));

  return cloudCol;
}

float getTrailDensity( vec3 p)
{
  return noise(p*3.)*1.;
}

void TranslateMissilePos(inout vec3 p, Missile missile)
{  
  p = p-(missile.pos);  
  p+=missile.origin;
  pR(p.xz, missile.orientation.z);
  pR(p.xy, -missile.orientation.x +PI);
  p-=missile.origin;
}

vec2 MapSmokeTrail( vec3 p, Missile missile)
{
  TranslateMissilePos(p, missile);
  float spreadDistance = 1.5;
  p.z+=3.82;

  // map trail by using mod op and ellipsoids
  float s = pModInterval1(p.z, -spreadDistance, .0, min(12., (missile.pos.z-planePos.z)/spreadDistance));     
  float dist = sdEllipsoid(p+vec3(0.0, 0.0, .4), vec3(0.6, 0.6, 3.));   
  dist-= getTrailDensity(p+vec3(10.*s))*0.25;

  return vec2(dist, s);
}


vec4 TraceSmoketrail( vec3 origin, vec3 direction, int steps, Missile missile)
{
  vec4 trailCol =vec4(0.5, 0.5, 0.5, 0.0);
  float height = 0.0, t = .0;
  vec2 dist = vec2(0.0);
  vec3 rayPos;
  float precis; 
  float td =.0;
  for ( int i=0; i<steps; i++ )
  {
    rayPos = origin+direction*t;
    dist = MapSmokeTrail(rayPos, missile);  
    precis = 0.002*t;
    if (dist.x<precis)
    {     
      trailCol.rgb+=(0.5*(getTrailDensity(rayPos+sunPos*.17)))*0.03;

      float densAdd =(precis-dist.x)*0.20;
      trailCol.a+=(1.-trailCol.a)*densAdd/(1.+(pow(dist.y, 2.0)*0.021));
    } 

    if (trailCol.a > 0.99) break; 

    td = max(0.04, dist.x);
    t+=td;
  }

  return clamp(trailCol, 0., 1.);
}


float MapExplosion( vec3 p, Explosion ex)
{ 
  checkPos = (ex.pos)-vec3(planePos.x, 0., planePos.z); 
  checkPos=p-checkPos;

  float testDist = fSphere(checkPos, 20.0);
  if (testDist>10.)  return testDist;

  float intensity =GetExplosionIntensity(ex);
  float d= fSphere(checkPos, intensity*15.);  

  // terrain clipping
  #ifdef EXACT_EXPLOSIONS
    d=max(d, -MapTerrain(p));
  #else
    d = max(d, -sdBox(checkPos+vec3(0., 50., 0.), vec3(50., 50.0, 50.0)));
  #endif

  // add explosion "noise/flames"
  float displace = fbm(((checkPos) + vec3(1, -2, -1)*iTime)*0.5);
  return d + (displace * 1.5*max(0., 4.*intensity));
}


RayHit TraceExplosion(in vec3 origin, in vec3 direction, int steps, Explosion ex)
{
  RayHit result;
  float precis = 0.0, maxDist = 350.0, t = 0.0, dist = 0.0;
  vec3 rayPos;

  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t; 
    dist = MapExplosion( rayPos, ex);
    precis = 0.01*t;

    if (dist<precis || t>maxDist )
    {             
      result.hit=!(t>maxDist);
      result.depth = t; 
      result.dist = dist;                              
      result.hitPos = origin+((direction*t));     
      break;
    }

    t += dist*0.5;
  }

  return result;
}

// inspired by https://www.shadertoy.com/view/XdfGz8
vec3 GetExplosionColor(float x)
{
  vec3 col1= vec3(240., 211., 167.)/255.;
  vec3 col2 = vec3(210., 90., 60.)/255.;
  vec3 col3 = vec3(84., 20., 13.)/255.;

  float t = fract(x*3.);
  vec3 c= mix(col2, col3, t);
  c= mix(mix(col1, col2, t), c, step(0.666, x));
  return mix(mix(vec3(4, 4, 4), col1, t), c, step(0.333, x));
}

vec3 GetExplosionLight(float specLevel, vec3 normal, RayHit rayHit, vec3 rayDir, vec3 origin)
{                
  vec3 reflectDir = reflect( rayDir, normal );

  vec3 lightTot = vec3(0.0);
  float amb = clamp( 0.5+0.5*normal.y, 0.0, 1.0 );
  float dif = clamp( dot( normal, sunPos ), 0.0, 1.0 );
  float bac = clamp( dot( normal, normalize(vec3(-sunPos.x, 0.0, -sunPos.z)) ), 0.0, 1.0 ) * clamp(1.0-rayHit.hitPos.y/20.0, 0.0, 1.0);

  float fre = pow( clamp(1.0+dot(normal, rayDir), 0.0, 1.0), 2.0 );
  specLevel*= pow(clamp( dot( reflectDir, sunPos ), 0.0, 1.0 ), 7.0);
  float skylight = smoothstep( -0.1, 0.1, reflectDir.y );

  lightTot += 1.5*dif*vec3(1.00, 0.90, 0.85);
  lightTot += 0.50*skylight*vec3(0.40, 0.60, 0.95);
  lightTot += 1.00*specLevel*vec3(0.9, 0.8, 0.7)*dif;
  lightTot += 0.50*bac*vec3(0.25, 0.25, 0.25);
  lightTot += 0.25*fre*vec3(1.00, 1.00, 1.00);

  return clamp(lightTot, 0., 10.);
}


void DrawExplosion(int id, RayHit marchResult, inout vec3 color, vec3 rayDir, vec3 rayOrigin)
{
  Explosion explosion;
  id *= 100;
  explosion.life = read(ivec2(122+id, 0));

  // check if explosion has been spawned
  if (explosion.life>0.)
  {  
    explosion.pos = readRGB(ivec2(120+id, 0)); 

    vec3 testPoint = explosion.pos-planePos;
    // ensure the explosions starts on ground
    // explosion.pos.y=GetTerrainHeight(testPoint);

    // explosion light flash    
    if (marchResult.hit)
    {
      float intensity = GetExplosionIntensity(explosion);

      vec3 testCol = color.rgb+vec3(1.0, 0.59, 0.28)*2.5;
      color.rgb=mix(color.rgb, mix(testCol, color.rgb, smoothstep(0., 40.0*intensity, distance(testPoint.xz, marchResult.hitPos.xz))), intensity);
    }

    // trace explosion  
    RayHit exploTest = TraceExplosion(rayOrigin, rayDir, 68, explosion);   
    if (exploTest.hit)
    {
      color.rgb = GetExplosionColor(clamp(0.5+((fbm((exploTest.hitPos + vec3(1, -2, -1)*iTime)*0.5))), 0.0, 0.99));
      color.rgb = mix(color.rgb, color.rgb*0.45, smoothstep(0., 12., distance(exploTest.hitPos.y, GetTerrainHeight(testPoint))));
    }

    color.rgb = mix(color.rgb*3.0, color.rgb, smoothstep(0., 12.4, exploTest.dist));
  }
  ////////////////////////////////////////////////////////////
}

float MapFlare( vec3 p, Missile missile)
{
  TranslateMissilePos(p, missile);
  return sdEllipsoid( p+ vec3(0., 0., 2.4), vec3(.05, 0.05, .15));
}

float TraceEngineFlare(in vec3 origin, in vec3 direction, Missile missile)
{
  float t = 0.0;
  vec3 rayPos = vec3(0.0);
  float dist=10000.;

  for ( int i=0; i<10; i++ )
  {
    rayPos =origin+direction*t;
    dist = min(dist, MapFlare( rayPos, missile));
    t += dist;
  }

  return dist;
}

float MapMissile(vec3 p, Missile missile)
{
  float d= fCylinder( p, 0.70, 1.7);
  if (d<1.0)
  {
    d = fCylinder( p, 0.12, 1.2);   
    d =min(d, sdEllipsoid( p- vec3(0, 0, 1.10), vec3(0.12, 0.12, 1.0))); 

    checkPos = p;  
    pR(checkPos.xy, 0.785);
    checkPos.xy = pModPolar(checkPos.xy, 4.0);

    d=min(d, sdHexPrism( checkPos-vec3(0., 0., .60), vec2(0.50, 0.01)));
    d=min(d, sdHexPrism( checkPos+vec3(0., 0., 1.03), vec2(0.50, 0.01)));
    d = max(d, -sdBox(p+vec3(0., 0., 3.15), vec3(3.0, 3.0, 2.0)));
    d = max(d, -fCylinder(p+vec3(0., 0., 2.15), 0.09, 1.2));
  }
  return d;
}

float MapFlyingMissile( vec3 p, Missile missile)
{
  TranslateMissilePos(p, missile);  
  // map missile flame
  eFlameDist = min(eFlameDist, sdEllipsoid( p+ vec3(0., 0., 2.2+cos(iTime*90.0)*0.23), vec3(.17, 0.17, 1.0)));
  // map missile 
  return min(MapMissile(p, missile), eFlameDist);
}

RayHit TraceMissile(in vec3 origin, in vec3 direction, int steps, Missile missile)
{
  RayHit result;
  float maxDist = 450.0;
  float t = 0.0, glassDist = 0.0, dist = 100000.0;
  vec3 rayPos;
  eFlameDist=10000.0;
  for ( int i=0; i<steps; i++ )
  {
    rayPos =origin+direction*t;
    dist = MapFlyingMissile(rayPos, missile);

    if (dist<0.01 || t>maxDist )
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

float SoftShadowMissile( in vec3 origin, in vec3 direction, Missile missile )
{
  float res = 2.0, t = 0.02, h;
  for ( int i=0; i<8; i++ )
  {
    h = MapMissile(origin+direction*t, missile);
    res = min( res, 7.5*h/t );
    t += clamp( h, 0.05, 0.2 );
    if ( h<0.001 || t>2.5 ) break;
  }
  return clamp( res, 0.0, 1.0 );
}

vec3 GetMissileLightning(float specLevel, vec3 normal, RayHit rayHit, vec3 rayDir, vec3 origin, Missile missile)
{       
  float dif = clamp( dot( normal, sunPos ), 0.0, 1.0 );
  vec3 reflectDir = reflect( rayDir, normal );
  specLevel= 3.5*pow(clamp( dot( reflectDir, sunPos ), 0.0, 1.0 ), 9.0/3.);

  float fre = pow( 1.0-abs(dot( normal, rayDir )), 2.0 );
  fre = mix( .03, 1.0, fre );   
  float amb = clamp( 0.5+0.5*normal.y, 0.0, 1.0 );

  float shadow = SoftShadowMissile(origin+((rayDir*rayHit.depth)*0.998), sunPos, missile);
  dif*=shadow;
  float skyLight = smoothstep( -0.1, 0.1, reflectDir.y );

  vec3 lightTot = (vec3(0.7)*amb); 
  lightTot+=vec3(0.85)*dif;
  lightTot += 1.00*specLevel*dif;
  lightTot += 0.80*skyLight*vec3(0.40, 0.60, 1.00);
  lightTot= mix(lightTot*.7, lightTot*1.2, fre );

  return lightTot*sunColor;
}

vec3 calcMissileNormal( in vec3 pos, Missile missile )
{    
  return normalize( vec3(MapFlyingMissile(pos+eps.xyy, missile) - MapFlyingMissile(pos-eps.xyy, missile), 0.5*2.0*eps.x, MapFlyingMissile(pos+eps.yyx, missile) - MapFlyingMissile(pos-eps.yyx, missile) ) );
}

mat3 setCamera(  vec3 ro, vec3 ta, float cr )
{
  vec3 cw = normalize(ta-ro);
  vec3 cp = vec3(sin(cr), cos(cr), 0.0);
  vec3 cu = normalize( cross(cw, cp) );
  vec3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}

// set sky color tone. 2 gradient passes using MIX.
vec3 GetSkyColor(vec3 rayDir)
{ 
  return mix(mix(vec3(0.15, 0.19, 0.24), vec3(220., 230., 240.0)/255., smoothstep(1.0, .30, rayDir.y)), mix(vec3(229.0, 221., 230)/200., sunColor, 0.15), smoothstep(0.15, -0.13, rayDir.y));
}

// scene lightning
vec3 GetSceneLight(float specLevel, vec3 normal, RayHit rayHit, vec3 rayDir, vec3 origin)
{                
  vec3 reflectDir = reflect( rayDir, normal );

  vec3 lightTot = vec3(0.0);
  float amb = clamp( 0.5+0.5*normal.y, 0.0, 1.0 );
  float dif = clamp( dot( normal, sunPos ), 0.0, 1.0 );
  float bac = clamp( dot( normal, normalize(vec3(-sunPos.x, 0.0, -sunPos.z)) ), 0.0, 1.0 ) * clamp(1.0-rayHit.hitPos.y/20.0, 0.0, 1.0);
  ;
  float fre = pow( clamp(1.0+dot(normal, rayDir), 0.0, 1.0), 2.0 );
  specLevel*= pow(clamp( dot( reflectDir, sunPos ), 0.0, 1.0 ), 7.0);
  float skylight = smoothstep( -0.1, 0.1, reflectDir.y );

  float shadow=1.; 
  #ifdef SHADOWS
    shadow = SoftShadow(origin+((rayDir*rayHit.depth)*0.988), sunPos);
  #endif

    lightTot += 1.5*dif*vec3(1.00, 0.90, 0.85)*shadow;
  lightTot += 0.50*skylight*vec3(0.40, 0.60, 0.95);
  lightTot += 1.00*specLevel*vec3(0.9, 0.8, 0.7)*dif;
  lightTot += 0.50*bac*vec3(0.25, 0.25, 0.25);
  lightTot += 0.25*fre*vec3(1.00, 1.00, 1.00)*shadow;

  return clamp(lightTot, 0., 10.)*sunColor;
}

vec3 GetSceneLightWater(float specLevel, vec3 normal, RayHit rayHit, vec3 rayDir, vec3 origin)
{                
  vec3 reflectDir = reflect( rayDir, normal );
  float amb = clamp( 0.5+0.5*normal.y, 0.0, 1.0 );
  float dif = clamp( dot( normal, sunPos ), 0.0, 1.0 );
  float bac = clamp( dot( normal, normalize(vec3(-sunPos.x, 0.0, -sunPos.z)) ), 0.0, 1.0 ) * clamp(1.0-rayHit.hitPos.y/20.0, 0.0, 1.0);

  specLevel*= pow(clamp( dot( reflectDir, sunPos ), 0.0, 1.0 ), 9.0);

  float skylight = smoothstep( -0.1, 0.1, reflectDir.y );
  float fre = pow( 1.0-abs(dot( normal, rayDir )), 4.0 );
  fre = mix( .03, 1.0, fre );   

  vec3 reflection = vec3(1.0);
  vec3 lightTot = vec3(0.0);

  lightTot += 1.15*dif*vec3(1.00, 0.90, 0.85);
  lightTot += 1.00*specLevel*vec3(0.9, 0.8, 0.7)*dif;    
  lightTot= mix(lightTot, reflection, fre );
  lightTot += 0.70*skylight*vec3(0.70, 0.70, 0.85);
  lightTot += 1.30*bac*vec3(0.25, 0.25, 0.25);
  lightTot += 0.25*amb*vec3(0.80, 0.90, 0.95);  
  return clamp(lightTot, 0., 10.);
}


void ApplyFog(inout vec3 color, vec3 skyColor, vec3 rayOrigin, vec3 rayDir, float depth)   
{
  float mixValue = smoothstep(50., 15000., pow(depth, 2.)*0.1);
  float sunVisibility = max(0., dot(sunPos, rayDir));
  // horizontal fog
  vec3 fogColor = mix(sunColor*0.7, skyColor, mixValue);  
  fogColor = mix(fogColor, sunColor, smoothstep(0., 1., sunVisibility));   
  color = mix(color, fogColor, mixValue);

  // vertical fog
  float heightAmount = .01;
  float fogAmount = 0.2 * exp(-rayOrigin.y*heightAmount) * (1.0-exp( -depth*rayDir.y*heightAmount ))/rayDir.y;
  color = mix(color, fogColor, fogAmount);
}



void mainImage( out vec4 fragColor, vec2 fragCoord )
{  

    R=iResolution.xy;

  vec2 mo = iMouse.xy/iResolution.xy;
  vec2 uv = fragCoord.xy / iResolution.xy;
  vec2 screenSpace = (-iResolution.xy + 2.0*(fragCoord))/iResolution.y;

  // read plane data from buffer
  turn = read(ivec2(1, 10));
  float roll = read(ivec2(1, 1));
  float speed = read(ivec2(10, 1));
  float pitch = read(ivec2(15, 1));
  sunPos =  readRGB(ivec2(50, 0));
  planePos = readRGB(ivec2(55, 0));
  float CAMZOOM = read(ivec2(52, 0));  

  // setup camera and ray direction
  vec2 camRot = readRGB(ivec2(57, 0)).xy;

  cloudPos = vec2(-iTime*0.3, iTime*0.45);

  vec3 rayOrigin = vec3(CAMZOOM*cos(camRot.x), planePos.y+CAMZOOM*sin(camRot.y), -3.+CAMZOOM*sin(camRot.x) );    
  pR(rayOrigin.xz, -turn);
  mat3 ca = setCamera( rayOrigin, vec3(0., planePos.y, -3. ), 0.0 );
  vec3 rayDir = ca * normalize( vec3(screenSpace.xy, 2.0) );

  // create sky color fade
  vec3 skyColor = GetSkyColor(rayDir);
  vec3 color = skyColor;
  float alpha=0.;

  RayHit marchResult = TraceTerrain(rayOrigin, rayDir, 1200);

  // is terrain hit?
  if (marchResult.hit)
  { 

    alpha=1.0;
    marchResult.normal = calcNormal(marchResult.hitPos);  

    float specLevel=0.7;
    color=vec3(0.5);

    // create terrain texture
    vec3 colorRocks= vec3(mix(texture(iChannel3, (marchResult.hitPos.xz+planePos.xz)*.01).rgb, texture(iChannel3, (marchResult.hitPos.xz+vec2(10000.0, 10000.0)+planePos.xz)*.01).rgb, fastFBM(marchResult.hitPos)));
    color =colorRocks;
    color.rgb = mix(color.rgb, color*3., abs(noise2D((marchResult.hitPos.xz+planePos.xz)*0.4, 1.0))); 

    // grass
    color.rgb = mix(color.rgb, ((color+noise2D((marchResult.hitPos.xz+planePos.xz)*24., 1.0))+vec3(0.5, 0.4, .1))*0.3, smoothstep(0.2, 2.0, marchResult.hitPos.y)); 

    float stoneHeight = GetStoneHeight(marchResult.hitPos, (GetTerrainHeight(marchResult.hitPos)));     
    color.rgb = mix(color.rgb, vec3(0.5+(noise(marchResult.hitPos+vec3(planePos.x, 0., planePos.z))*0.3)), smoothstep(1., .0, stoneHeight));
    specLevel = mix(specLevel, specLevel*2.6, smoothstep(1., .0, stoneHeight));

    // beach
    color.rgb = mix((color+vec3(1.2, 1.1, 1.0))*0.5, color.rgb, smoothstep(0.3, 0.7, marchResult.hitPos.y)); 


    float burn = NoTreeZone(marchResult.hitPos+planePos);
    color=mix(color*0.1, color, smoothstep(0., 25., burn));

    // create slight wave difference between water and beach level
    float wave = max(0., cos(abs(noise2D((marchResult.hitPos.xz+planePos.xz)))+(iTime*.5)+(length(marchResult.hitPos.xz)*0.03))*0.09);

    vec3 light;
    // check if terrain is below water level
    if (marchResult.hitPos.y<0.3+wave)
    {
      vec3 terrainHit = rayOrigin+((rayDir*marchResult.depth)*0.998);
      vec3 refDir = reflect(rayDir, marchResult.normal);
      vec4 testClouds = TraceCloudsBelow(terrainHit, refDir, skyColor, 30);

      color = vec3(0.3);

      float sunVisibility = max(0., dot(sunPos, rayDir));

      // calculate water fresnel  
      float dotNormal = dot(rayDir, marchResult.normal);
      float fresnel = pow(1.0-abs(dotNormal), 4.);  
      vec3 rayRef = rayDir-marchResult.normal*dotNormal;

      //color.rgb  = mix(mix(vec3(1.0), (vec3(0.7)+sunColor)*1.50, smoothstep(150., 350.,marchResult.depth)), color.rgb, smoothstep(-TERRAINLEVEL-0.37, -TERRAINLEVEL+0.25, marchResult.hitPos.y));
      color.rgb  = mix(color*.7, color.rgb, smoothstep(-3.0, -0.15, marchResult.hitPos.y));

      color = color+(sunColor*pow(sunVisibility, 5.0));

      // sea color
      color = mix(mix(color, color+fresnel, fresnel ), color, smoothstep(-0.1, 0.15, marchResult.hitPos.y));

      vec3 reflection = color;

      #ifdef QUALITY_REFLECTIONS
        // cast rays from water surface onto terrain. If terrain is hit, color water dark in these areas.
        RayHit reflectResult = TraceTerrainReflection(terrainHit, refDir, 100); 

      if (reflectResult.hit==true)
      {
        reflection  = mix(color, vec3(.01, 0.03, .0), 0.9);
      }
      #endif
        light = GetSceneLightWater(specLevel, marchResult.normal, marchResult, rayDir, rayOrigin);   
      color=mix(mix(color.rgb, testClouds.rgb, testClouds.a*.26), mix(color.rgb, testClouds.rgb, testClouds.a), smoothstep(0., 0.7, fresnel)); 
      color=mix(mix(color.rgb, reflection, 0.5), reflection, smoothstep(0., 0.7, fresnel)); 
      color=mix(color, color+(0.5*fresnel), smoothstep(0., 0.3, fresnel)); 

      color=color*light;
      color = mix(color, skyColor, smoothstep(320., 400., marchResult.depth));
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
        vec4 treeColor = TraceTrees(rayOrigin, rayDir, 28, marchResult.hitPos.y-0.3 );      
      color =clamp( mix( color, treeColor.rgb*((noise2D((marchResult.hitPos.xz+planePos.xz)*36., 3.0)+vec3(0.56, 0.66, .45))*0.6)*sunColor*(.30+(0.6*light)), treeColor.a ), 0.02, 1.); 
      #endif
    }

    color = mix(color, (color+sunColor)*0.6, smoothstep(70., 300., marchResult.depth));
    // add haze when high above ground  
    color = mix(color, color+vec3(0.37, 0.58, 0.9)*sunColor, mix(0., 0.75, smoothstep(-CLOUDLEVEL*0.65, MAX_HEIGHT, planePos.y)));  
    ApplyFog(color, skyColor, rayOrigin, rayDir, marchResult.depth);
  } else
  {
    // add volumetric clouds 
    // below cloud level
    if (rayOrigin.y<-CLOUDLEVEL && rayDir.y>0.)
    {  
      vec4 cloudColor=TraceCloudsBelow(rayOrigin, rayDir, skyColor, 60);    

      // make clouds slightly light near the sun
      float sunVisibility = pow(max(0., dot(sunPos, rayDir)), 2.0)*0.10;
      color.rgb = mix(color.rgb, max(vec3(0.), cloudColor.rgb+sunVisibility), cloudColor.a);      
      //color.rgb = mix(color.rgb, cloudColor.rgb, cloudColor.a);       
      alpha+=cloudColor.a*0.86;
    }
  }

  // add volumetric clouds 
  // above cloud level
  if (rayOrigin.y>=-CLOUDLEVEL)
  {  
    vec4 cloudColor=TraceClouds(rayOrigin, rayDir, skyColor, 80);    
    color.rgb = mix(color.rgb, cloudColor.rgb, cloudColor.a);
  }

  rayDir = ca * normalize( vec3(screenSpace.xy, 2.0) );
  DrawExplosion(0, marchResult, color, rayDir, rayOrigin);
  DrawExplosion(1, marchResult, color, rayDir, rayOrigin);


  // #################################################################### //    
  // ##############             MISSILES             #################### //     
  // #################################################################### //    

  rayOrigin = vec3(CAMZOOM*cos(camRot.x), CAMZOOM*sin(camRot.y), CAMZOOM*sin(camRot.x) );
  pR(rayOrigin.xz, -turn);
  ca = setCamera( rayOrigin, vec3(0., 0., 0. ), 0.0 );
  rayDir = ca * normalize( vec3(screenSpace.xy, 2.0) );

  int adressStep = 0;
  Missile missile;
  for (int i=0; i<2; i++)
  {
    adressStep = i*100;
    missile.life = read(ivec2(100 + adressStep, 0));
    // check if missile is launched
    if (missile.life>0.)
    {
      missile.origin = vec3(4.8 - (9.6*float(i)), -0.4, -3.0);       
      missile.orientation = readRGB(ivec2(108+adressStep, 0));
      missile.pos = readRGB(ivec2(116+adressStep, 0));

      // calculate engine flare
      float lightDist = TraceEngineFlare(rayOrigin, rayDir, missile);

      // add engine flares for missiles based on engine distance
      vec3 lightFlares=vec3(0.);
      lightFlares =  mix((vec3(1., 0.4, 0.2)), vec3(0.), smoothstep(0., 1.1, lightDist));             
      lightFlares =  mix(lightFlares+(2.*vec3(1., 0.5, 0.2)), lightFlares, smoothstep(0., 0.7, lightDist));
      lightFlares =  mix(lightFlares+vec3(1., 1., 1.), lightFlares, smoothstep(0., 0.2, lightDist));

      // rayTrace missile
      RayHit marchResult = TraceMissile(rayOrigin, rayDir, 64, missile);

      // apply color and lightning to missile if hit in raymarch test    
      if (marchResult.hit)
      {
        marchResult.normal = calcMissileNormal(marchResult.hitPos, missile);  

        // create texture map and set specular levels
        vec4 col = vec4(0.45, 0.45, 0.45, 0.8);

        // flame
        col.rgb=mix(col.rgb, vec3(1.2, .55, 0.30)*2.5, smoothstep(.16, 0., marchResult.eFlameDist));

        // get lightning based on material
        vec3 light = GetMissileLightning(col.a, marchResult.normal, marchResult, rayDir, rayOrigin, missile);   

        // apply lightning
        color.rgb = col.rgb*light;

        alpha = 1.; 

        lightFlares = mix(lightFlares, vec3(.0), step(0.1, distance(marchResult.dist, marchResult.eFlameDist)));
      }

      color.rgb+=lightFlares;

      //draw smoke trail behind missile
      vec4 trailColor = TraceSmoketrail(rayOrigin, rayDir, 48, missile);     
      color.rgb = mix(color.rgb, trailColor.rgb, trailColor.a);
      alpha+=trailColor.a;   

      if (marchResult.hit) 
      { 
        break;
      }
    }
  }
  // #################################################################### //
  // #################################################################### //

  fragColor = vec4(color.rgb, min(1.0, alpha));
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//////////////////////////////////////////////////////////////////////////////////////
// PLANE BUFFER   -   RENDERS PLANE ONLY
//////////////////////////////////////////////////////////////////////////////////////
// Channel 0 = Buffer B. Get the colors of the terrain buffer render.
// Channel 1 = LowRes noise texture. Used in fast noise functions.
// Channel 2 = Buffer A. Read data from data-buffer.
// Channel 3 = Forest blurred cube map. Used in reflections in plane window and hull.

vec2 R;

  #pragma optimize(off) 
#define PI acos(-1.)
  //#define read(memPos)    (  texelFetch(iChannel2, memPos, 0).a)
  //#define readRGB(memPos) (  texelFetch(iChannel2, memPos, 0).rgb)
  #define read(memPos)      (  texture(iChannel2, (vec2(memPos)+0.5)/R).w)
  #define readRGB(memPos)   (  texture(iChannel2, (vec2(memPos)+0.5)/R).xyz)
  
  
  #define RAYSTEPS 300
  #define CLOUDLEVEL -70.0
  float turn=0., pitch = 0., roll=0., rudderAngle = 0.;
float speed = 0.5;
vec3 checkPos=vec3(0.);
vec3 sunPos=vec3(0.);
const vec3 sunColor = vec3(1.00, 0.90, 0.85);
vec3 planePos=vec3(0.);
const vec3 eps = vec3(0.02, 0.0, 0.0);

float winDist=10000.0;
float engineDist=10000.0;
float eFlameDist=10000.0;
float blackDist=10000.0;
float bombDist=10000.0;
float bombDist2=10000.0;
float missileDist=10000.0;
float frontWingDist=10000.0;
float rearWingDist=10000.0;
float topWingDist=10000.0;
vec2 missilesLaunched=vec2(0.);

float sgn(float x) 
{   
  return (x<0.)?-1.:1.;
}

struct RayHit
{
  bool hit;  
  vec3 hitPos;
  vec3 normal;
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

float noise2D( in vec2 pos, float lod)
{   
  vec2 f = fract(pos);
  f = f*f*(3.0-2.0*f);
  vec2 rg = textureLod( iChannel1, (((floor(pos).xy+vec2(37.0, 17.0)) + f.xy)+ 0.5)/64.0, lod).yx;  
  return -1.0+2.0*mix( rg.x, rg.y, 0.5 );
}
float noise2D( in vec2 pos )
{
  return noise2D(pos, 0.0);
}

float noise( in vec3 x )
{
  vec3 p = floor(x);
  vec3 f = fract(x);

  float a = textureLod( iChannel1, x.xy/64.0 + (p.z+0.0)*120.7123, 0.1 ).x;
  float b = textureLod( iChannel1, x.xy/64.0 + (p.z+1.0)*120.7123, 0.1 ).x;
  return mix( a, b, f.z );
}

float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x, p.y);
  return length(q)-t.y;
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa, ba)/dot(ba, ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float sdEllipsoid( vec3 p, vec3 r )
{
  return (length( p/r.xyz ) - 1.0) * r.y;
}

float sdConeSection( vec3 p, float h, float r1, float r2 )
{
  float d1 = -p.z - h;
  float q = p.z - h;
  float si = 0.5*(r1-r2)/h;
  float d2 = max( sqrt( dot(p.xy, p.xy)*(1.0-si*si)) + q*si - r2, q );
  return length(max(vec2(d1, d2), 0.0)) + min(max(d1, d2), 0.);
}

float fCylinder(vec3 p, float r, float height) {
  float d = length(p.xy) - r;
  d = max(d, abs(p.z) - height);
  return d;
}
float fSphere(vec3 p, float r) {
  return length(p) - r;
}

float sdHexPrism( vec3 p, vec2 h )
{
  vec3 q = abs(p);
  return max(q.y-h.y, max((q.z*0.866025+q.x*0.5), q.x)-h.x);
}

float fOpPipe(float a, float b, float r) {
  return length(vec2(a, b)) - r;
}

vec2 pModPolar(vec2 p, float repetitions) {
  float angle = 2.*PI/repetitions;
  float a = atan(p.y, p.x) + angle/2.;
  float r = length(p);
  float c = floor(a/angle);
  a = mod(a, angle) - angle/2.;
  p = vec2(cos(a), sin(a))*r;
  if (abs(c) >= (repetitions/2.)) c = abs(c);
  return p;
}

float pModInterval1(inout float p, float size, float start, float stop) {
  float halfsize = size*0.5;
  float c = floor((p + halfsize)/size);
  p = mod(p+halfsize, size) - halfsize;
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

float pMirror (inout float p, float dist) {
  float s = sgn(p);
  p = abs(p)-dist;
  return s;
}

mat2 r2(float r)
{
  float c=cos(r), s=sin(r);
  return mat2(c, s, -s, c);
}

#define r3(r) mat2(sin(vec4(-1, 0, 0, 1)*acos(0.)+r))
  void pR(inout vec2 p, float a) 
{
  p*=r2(a);
}

float fOpUnionRound(float a, float b, float r) {
  vec2 u = max(vec2(r - a, r - b), vec2(0));
  return max(r, min (a, b)) - length(u);
}

float fOpIntersectionRound(float a, float b, float r) {
  vec2 u = max(vec2(r + a, r + b), vec2(0));
  return min(-r, max (a, b)) + length(u);
}

// limited by euler rotation. I wont get a good plane rotation without quaternions! :-(
vec3 TranslatePos(vec3 p, float _pitch, float _roll)
{
  pR(p.xy, _roll-PI);
  p.z+=5.;
  pR(p.zy, _pitch);
  p.z-=5.; 
  return p;
}

float MapEsmPod(vec3 p)
{
  float dist = fCylinder( p, 0.15, 1.0);   
  checkPos =  p- vec3(0, 0, -1.0);
  pModInterval1(checkPos.z, 2.0, .0, 1.0);
  return min(dist, sdEllipsoid(checkPos, vec3(0.15, 0.15, .5)));
}

float MapMissile(vec3 p)
{
  float d= fCylinder( p, 0.70, 1.7);
  if (d<1.0)
  {
    missileDist = min(missileDist, fCylinder( p, 0.12, 1.2));   
    missileDist =min(missileDist, sdEllipsoid( p- vec3(0, 0, 1.10), vec3(0.12, 0.12, 1.0))); 

    checkPos = p;  
    pR(checkPos.xy, 0.785);
    checkPos.xy = pModPolar(checkPos.xy, 4.0);

    missileDist=min(missileDist, sdHexPrism( checkPos-vec3(0., 0., .60), vec2(0.50, 0.01)));
    missileDist=min(missileDist, sdHexPrism( checkPos+vec3(0., 0., 1.03), vec2(0.50, 0.01)));
    missileDist = max(missileDist, -sdBox(p+vec3(0., 0., 3.15), vec3(3.0, 3.0, 2.0)));
    missileDist = max(missileDist, -fCylinder(p+vec3(0., 0., 2.15), 0.09, 1.2));
  }
  return missileDist;
}

float MapFrontWing(vec3 p, float mirrored)
{
  missileDist=10000.0;

  checkPos = p;
  pR(checkPos.xy, -0.02);
  float wing =sdBox( checkPos- vec3(4.50, 0.25, -4.6), vec3(3.75, 0.04, 2.6)); 

  if (wing<5.) //Bounding Box test
  {
    // cutouts
    checkPos = p-vec3(3.0, 0.3, -.30);
    pR(checkPos.xz, -0.5);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, vec3(6.75, 1.4, 2.0)), 0.1);

    checkPos = p - vec3(8.0, 0.3, -8.80);
    pR(checkPos.xz, -0.05);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, vec3(10.75, 1.4, 2.0)), 0.1);

    checkPos = p- vec3(9.5, 0.3, -8.50);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos, vec3(2.0, 1.4, 6.75)), 0.6);

    // join wing and engine
    wing=min(wing, sdCapsule(p- vec3(2.20, 0.3, -4.2), vec3(0, 0, -1.20), vec3(0, 0, 0.8), 0.04));
    wing=min(wing, sdCapsule(p- vec3(3., 0.23, -4.2), vec3(0, 0, -1.20), vec3(0, 0, 0.5), 0.04));    

    checkPos = p;
    pR(checkPos.xz, -0.03);
    wing=min(wing, sdConeSection(checkPos- vec3(0.70, -0.1, -4.52), 5.0, 0.25, 0.9));   

    checkPos = p;
    pR(checkPos.yz, 0.75);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos- vec3(3.0, -.5, 1.50), vec3(3.75, 3.4, 2.0)), 0.12); 
    pR(checkPos.yz, -1.95);
    wing=fOpIntersectionRound(wing, -sdBox( checkPos- vec3(2.0, .70, 2.20), vec3(3.75, 3.4, 2.0)), 0.12); 

    checkPos = p- vec3(0.47, 0.0, -4.3);
    pR(checkPos.yz, 1.57);
    wing=min(wing, sdTorus(checkPos-vec3(0.0, -3., .0), vec2(.3, 0.05)));   

    // flaps
    wing =max(wing, -sdBox( p- vec3(3.565, 0.1, -6.4), vec3(1.50, 1.4, .5)));
    wing =max(wing, -max(sdBox( p- vec3(5.065, 0.1, -8.4), vec3(0.90, 1.4, 2.5)), -sdBox( p- vec3(5.065, 0., -8.4), vec3(0.89, 1.4, 2.49))));

    checkPos = p- vec3(3.565, 0.18, -6.20+0.30);
    pR(checkPos.yz, -0.15+(0.8*pitch));
    wing =min(wing, sdBox( checkPos+vec3(0.0, 0.0, 0.30), vec3(1.46, 0.007, 0.3)));

    // missile holder
    float holder = sdBox( p- vec3(3.8, -0.26, -4.70), vec3(0.04, 0.4, 0.8));

    checkPos = p;
    pR(checkPos.yz, 0.85);
    holder=max(holder, -sdBox( checkPos- vec3(2.8, -1.8, -3.0), vec3(1.75, 1.4, 1.0))); 
    holder=max(holder, -sdBox( checkPos- vec3(2.8, -5.8, -3.0), vec3(1.75, 1.4, 1.0))); 
    holder =fOpUnionRound(holder, sdBox( p- vec3(3.8, -0.23, -4.70), vec3(1.0, 0.03, 0.5)), 0.1); 

    // bomb
    bombDist = fCylinder( p- vec3(3.8, -0.8, -4.50), 0.35, 1.);   
    bombDist =min(bombDist, sdEllipsoid( p- vec3(3.8, -0.8, -3.50), vec3(0.35, 0.35, 1.0)));   
    bombDist =min(bombDist, sdEllipsoid( p- vec3(3.8, -0.8, -5.50), vec3(0.35, 0.35, 1.0)));   

    // missiles
    checkPos = p-vec3(2.9, -0.45, -4.50);

    // check if any missile has been fired. If so, do NOT mod missile position  
    float maxMissiles =0.; 
    if (mirrored>0.) maxMissiles =  mix(1.0, 0., step(1., missilesLaunched.x));
    else maxMissiles =  mix(1.0, 0., step(1., missilesLaunched.y)); 

    pModInterval1(checkPos.x, 1.8, .0, maxMissiles);
    holder = min(holder, MapMissile(checkPos));

    // ESM Pod
    holder = min(holder, MapEsmPod(p-vec3(7.2, 0.06, -5.68)));

    // wheelholder
    wing=min(wing, sdBox( p- vec3(0.6, -0.25, -3.8), vec3(0.8, 0.4, .50)));

    wing=min(bombDist, min(wing, holder));
  }

  return wing;
}

float MapRearWing(vec3 p)
{
  float wing2 =sdBox( p- vec3(2.50, 0.1, -8.9), vec3(1.5, 0.017, 1.3)); 
  if (wing2<0.15) //Bounding Box test
  {
    // cutouts
    checkPos = p-vec3(3.0, 0.0, -5.9);
    pR(checkPos.xz, -0.5);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, vec3(6.75, 1.4, 2.0)), 0.2); 

    checkPos = p-vec3(0.0, 0.0, -4.9);
    pR(checkPos.xz, -0.5);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, vec3(3.3, 1.4, 1.70)), 0.2);

    checkPos = p-vec3(3.0, 0.0, -11.70);
    pR(checkPos.xz, -0.05);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, vec3(6.75, 1.4, 2.0)), 0.1); 

    checkPos = p-vec3(4.30, 0.0, -11.80);
    pR(checkPos.xz, 1.15);
    wing2=fOpIntersectionRound(wing2, -sdBox( checkPos, vec3(6.75, 1.4, 2.0)), 0.1);
  }
  return wing2;
} 

float MapTailFlap(vec3 p, float mirrored)
{
  p.z+=0.3;
  pR(p.xz, rudderAngle*(-1.*mirrored)); 
  p.z-=0.3;

  float tailFlap =sdBox(p- vec3(0., -0.04, -.42), vec3(0.025, .45, .30));

  // tailFlap front cutout
  checkPos = p- vec3(0., 0., 1.15);
  pR(checkPos.yz, 1.32);
  tailFlap=max(tailFlap, -sdBox( checkPos, vec3(.75, 1.41, 1.6)));

  // tailFlap rear cutout
  checkPos = p- vec3(0., 0, -2.75);  
  pR(checkPos.yz, -0.15);
  tailFlap=fOpIntersectionRound(tailFlap, -sdBox( checkPos, vec3(.75, 1.4, 2.0)), 0.05);

  checkPos = p- vec3(0., 0., -.65);
  tailFlap = min(tailFlap, sdEllipsoid( checkPos-vec3(0.00, 0.25, 0), vec3(0.06, 0.05, 0.15)));
  tailFlap = min(tailFlap, sdEllipsoid( checkPos-vec3(0.00, 0.10, 0), vec3(0.06, 0.05, 0.15)));

  return tailFlap;
}

float MapTopWing(vec3 p, float mirrored)
{    
  checkPos = p- vec3(1.15, 1.04, -8.5);
  pR(checkPos.xy, -0.15);  
  float topWing = sdBox( checkPos, vec3(0.014, 0.8, 1.2));
  if (topWing<.15) //Bounding Box test
  {
    float flapDist = MapTailFlap(checkPos, mirrored);

    checkPos = p- vec3(1.15, 1.04, -8.5);
    pR(checkPos.xy, -0.15);  
    // top border    
    topWing = min(topWing, sdBox( checkPos-vec3(0, 0.55, 0), vec3(0.04, 0.1, 1.25)));

    float flapCutout = sdBox(checkPos- vec3(0., -0.04, -1.19), vec3(0.02, .45, 1.0));
    // tailFlap front cutout
    checkPos = p- vec3(1.15, 2., -7.65);
    pR(checkPos.yz, 1.32);
    flapCutout=max(flapCutout, -sdBox( checkPos, vec3(.75, 1.41, 1.6)));

    // make hole for tail flap
    topWing=max(topWing, -flapCutout);

    // front cutouts
    checkPos = p- vec3(1.15, 2., -7.);
    pR(checkPos.yz, 1.02);
    topWing=fOpIntersectionRound(topWing, -sdBox( checkPos, vec3(.75, 1.41, 1.6)), 0.05);

    // rear cutout
    checkPos = p- vec3(1.15, 1., -11.25);  
    pR(checkPos.yz, -0.15);
    topWing=fOpIntersectionRound(topWing, -sdBox( checkPos, vec3(.75, 1.4, 2.0)), 0.05);

    // top roll 
    topWing=min(topWing, sdCapsule(p- vec3(1.26, 1.8, -8.84), vec3(0, 0, -.50), vec3(0, 0, 0.3), 0.06)); 

    topWing = min(topWing, flapDist);
  }
  return topWing;
}

float MapPlane( vec3 p)
{
  float  d=100000.0;
  vec3 pOriginal = p;
  // rotate position 
  p=TranslatePos(p, pitch, roll);
  float mirrored=0.;
  // AABB TEST  
  float test = sdBox( p- vec3(0., -0., -3.), vec3(7.5, 4., 10.6));    
  if (test>1.0) return test;

  // mirror position at x=0.0. Both sides of the plane are equal.
  mirrored = pMirror(p.x, 0.0);

  float body= min(d, sdEllipsoid(p-vec3(0., 0.1, -4.40), vec3(0.50, 0.30, 2.)));
  body=fOpUnionRound(body, sdEllipsoid(p-vec3(0., 0., .50), vec3(0.50, 0.40, 3.25)), 1.);
  body=min(body, sdConeSection(p- vec3(0., 0., 3.8), 0.1, 0.15, 0.06));   

  body=min(body, sdConeSection(p- vec3(0., 0., 3.8), 0.7, 0.07, 0.01));   

  // window
  winDist =sdEllipsoid(p-vec3(0., 0.3, -0.10), vec3(0.45, 0.4, 1.45));
  winDist =fOpUnionRound(winDist, sdEllipsoid(p-vec3(0., 0.3, 0.60), vec3(0.3, 0.6, .75)), 0.4);
  winDist = max(winDist, -body);
  body = min(body, winDist);
  body=min(body, fOpPipe(winDist, sdBox(p-vec3(0., 0., 1.0), vec3(3.0, 1., .01)), 0.03));
  body=min(body, fOpPipe(winDist, sdBox(p-vec3(0., 0., .0), vec3(3.0, 1., .01)), 0.03));

  // front (nose)
  body=max(body, -max(fCylinder(p-vec3(0, 0, 2.5), .46, 0.04), -fCylinder(p-vec3(0, 0, 2.5), .35, 0.1)));
  checkPos = p-vec3(0, 0, 2.5);
  pR(checkPos.yz, 1.57);
  body=fOpIntersectionRound(body, -sdTorus(checkPos+vec3(0, 0.80, 0), vec2(.6, 0.05)), 0.015);
  body=fOpIntersectionRound(body, -sdTorus(checkPos+vec3(0, 2.30, 0), vec2(.62, 0.06)), 0.015);

  // wings       
  frontWingDist = MapFrontWing(p, mirrored);
  d=min(d, frontWingDist);   
  rearWingDist = MapRearWing(p);
  d=min(d, rearWingDist);
  topWingDist = MapTopWing(p, mirrored);
  d=min(d, topWingDist);

  // bottom
  checkPos = p-vec3(0., -0.6, -5.0);
  pR(checkPos.yz, 0.07);  
  d=fOpUnionRound(d, sdBox(checkPos, vec3(0.5, 0.2, 3.1)), 0.40);

  float holder = sdBox( p- vec3(0., -1.1, -4.30), vec3(0.08, 0.4, 0.8));  
  checkPos = p;
  pR(checkPos.yz, 0.85);
  holder=max(holder, -sdBox( checkPos- vec3(0., -5.64, -2.8), vec3(1.75, 1.4, 1.0))); 
  d=fOpUnionRound(d, holder, 0.25);

  // large bomb
  bombDist2 = fCylinder( p- vec3(0., -1.6, -4.0), 0.45, 1.);   
  bombDist2 =min(bombDist2, sdEllipsoid( p- vec3(0., -1.6, -3.20), vec3(0.45, 0.45, 2.)));   
  bombDist2 =min(bombDist2, sdEllipsoid( p- vec3(0., -1.6, -4.80), vec3(0.45, 0.45, 2.)));   

  d=min(d, bombDist2);

  d=min(d, sdEllipsoid(p- vec3(1.05, 0.13, -8.4), vec3(0.11, 0.18, 1.0)));    

  checkPos = p- vec3(0, 0.2, -5.0);
  d=fOpUnionRound(d, fOpIntersectionRound(sdBox( checkPos, vec3(1.2, 0.14, 3.7)), -sdBox( checkPos, vec3(1., 1.14, 4.7)), 0.2), 0.25);

  d=fOpUnionRound(d, sdEllipsoid( p- vec3(0, 0., -4.), vec3(1.21, 0.5, 2.50)), 0.75);

  // engine cutout
  blackDist = max(d, fCylinder(p- vec3(.8, -0.15, 0.), 0.5, 2.4)); 
  d=max(d, -fCylinder(p- vec3(.8, -0.15, 0.), 0.45, 2.4)); 

  // engine
  d =max(d, -sdBox(p-vec3(0., 0, -9.5), vec3(1.5, 0.4, 0.7)));

  engineDist=fCylinder(p- vec3(0.40, -0.1, -8.7), .42, 0.2);
  checkPos = p- vec3(0.4, -0.1, -8.3);
  pR(checkPos.yz, 1.57);
  engineDist=min(engineDist, sdTorus(checkPos, vec2(.25, 0.25)));
  engineDist=min(engineDist, sdConeSection(p- vec3(0.40, -0.1, -9.2), 0.3, .22, .36));

  checkPos = p-vec3(0., 0., -9.24);  
  checkPos.xy-=vec2(0.4, -0.1);
  checkPos.xy = pModPolar(checkPos.xy, 22.0);

  float engineCone = fOpPipe(engineDist, sdBox( checkPos, vec3(.6, 0.001, 0.26)), 0.015);
  engineDist=min(engineDist, engineCone);

  d=min(d, engineDist);
  eFlameDist = sdEllipsoid( p- vec3(0.4, -0.1, -9.45-(speed*0.07)+cos(iTime*40.0)*0.014), vec3(.17, 0.17, .10));
  d=min(d, eFlameDist);

  d=min(d, winDist);
  d=min(d, body);

  d=min(d, sdBox( p- vec3(1.1, 0., -6.90), vec3(.33, .12, .17))); 
  checkPos = p-vec3(0.65, 0.55, -1.4);
  pR(checkPos.yz, -0.35);
  d=min(d, sdBox(checkPos, vec3(0.2, 0.1, 0.45)));

  return min(d, eFlameDist);
}

RayHit TracePlane(in vec3 origin, in vec3 direction)
{
  RayHit result;
  float maxDist = 150.0;
  float t = 0.0, dist = 0.0;
  vec3 rayPos;
  eFlameDist=10000.0;
  for ( int i=0; i<RAYSTEPS; i++ )
  {
    rayPos =origin+direction*t;
    dist = MapPlane( rayPos);

    if (abs(dist)<0.003 || t>maxDist )
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

float MapLights( vec3 p)
{
  vec3 pOriginal = p;
  // rotate position 
  p=TranslatePos(p, pitch, roll);   
  // mirror position at x=0.0. Both sides of the plane are equal.
  pMirror(p.x, 0.0);

  return max(sdEllipsoid( p- vec3(0.4, -0.1, -9.5), vec3(0.03, 0.03, 0.03+max(0., (speed*0.07)))), -sdBox(p- vec3(0.4, -0.1, -9.6+2.0), vec3(2.0, 2.0, 2.0)));
}

float TraceLights(in vec3 origin, in vec3 direction)
{
  float maxDist = 150.0;
  float t = 0.0;
  vec3 rayPos;
  float dist=10000.;

  for ( int i=0; i<10; i++ )
  {
    rayPos =origin+direction*t;
    dist = min(dist, MapLights( rayPos));
    t += dist;
  }

  return dist;
}

vec3 calcNormal( in vec3 pos )
{    
  return normalize( vec3(MapPlane(pos+eps.xyy) - MapPlane(pos-eps.xyy), 0.5*2.0*eps.x, MapPlane(pos+eps.yyx) - MapPlane(pos-eps.yyx) ) );
}

float SoftShadow( in vec3 origin, in vec3 direction )
{
  float res = 2.0, t = 0.02, h;
  for ( int i=0; i<24; i++ )
  {
    h = MapPlane(origin+direction*t);
    res = min( res, 7.5*h/t );
    t += clamp( h, 0.05, 0.2 );
    if ( h<0.001 || t>2.5 ) break;
  }
  return clamp( res, 0.0, 1.0 );
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
  vec3 cw = normalize(ta-ro);
  vec3 cp = vec3(sin(cr), cos(cr), 0.0);
  vec3 cu = normalize( cross(cw, cp) );
  vec3 cv = normalize( cross(cu, cw) );
  return mat3( cu, cv, cw );
}


// Advanced lightning pass
vec3 GetSceneLight(float specLevel, vec3 normal, RayHit rayHit, vec3 rayDir, vec3 origin, float specSize)
{          
  float dif = clamp( dot( normal, sunPos ), 0.0, 1.0 );
  vec3 reflectDir = reflect( rayDir, normal );
  specLevel*= pow(clamp( dot( reflectDir, sunPos ), 0.0, 1.0 ), 9.0/specSize);
  vec3 reflection = vec3(texture(iChannel3, reflectDir ).r*1.5);

  float fre = pow( 1.0-abs(dot( normal, rayDir )), 2.0 );
  fre = mix( .03, 1.0, fre );   
  float amb = clamp( 0.5+0.5*normal.y, 0.0, 1.0 );

  vec3 shadowPos = origin+((rayDir*rayHit.depth)*0.998);

  float shadow = SoftShadow(shadowPos, sunPos);
  dif*=shadow;
  float skyLight = smoothstep( -0.1, 0.1, reflectDir.y );
  skyLight *= SoftShadow(shadowPos, reflectDir );

  vec3 lightTot = (vec3(0.2)*amb); 
  lightTot+=vec3(0.85)*dif;
  lightTot= mix(lightTot, reflection*max(0.3, shadow), fre );
  lightTot += 1.00*specLevel*dif;
  lightTot += 0.50*skyLight*vec3(0.40, 0.60, 1.00);
  lightTot= mix(lightTot*.7, lightTot*1.2, fre );

  fre = pow( 1.0-abs(dot(rayHit.normal, rayDir)), 4.0);
  fre = mix(0., mix( .1, 1.0, specLevel*0.5), fre );
  lightTot = mix( lightTot, lightTot+ vec3(1.6), fre );

  return lightTot*sunColor;
}

float drawRect(vec2 p1, vec2 p2, vec2 uv) 
{
  vec4 rect = vec4(p1, p2);
  vec2 hv = step(rect.xy, uv) * step(uv, rect.zw);
  return hv.x * hv.y;
}

// Thanks IÃ±igo Quilez!
float line(vec2 p, vec2 a, vec2 b, float size)
{
  vec2 pa = -p - a;
  vec2 ba = b - a;
  float h = clamp( dot(pa, ba)/dot(ba, ba), 0.0, 1.0 );
  float d = length( pa - ba*h );

  return clamp((((1.0+size) - d)-0.99)*100.0, 0.0, 1.0);
}

void AddLetters(vec2 hitPos, inout vec3 col, vec2 linePos)
{
  // text
  vec3 textColor = vec3(0.2);
  vec2 absHitPos2 = vec2(hitPos.x-1.05, hitPos.y);

  pModInterval1(absHitPos2.x, 8., linePos.x, linePos.x+10.);

  // E
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(1.45, 0.4), linePos+vec2(1.45, .9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(1.45, 0.9), linePos+vec2(1.1, .9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(1.45, 0.65), linePos+vec2(1.25, 0.65), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(1.45, 0.4), linePos+vec2(1.1, .4), 0.06));
  // F            
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.9, 0.4), linePos+vec2(0.9, .9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.9, 0.9), linePos+vec2(.65, .9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.9, 0.65), linePos+vec2(.75, 0.65), 0.06));
  // Z
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.45, 0.4), linePos+vec2(.1, 0.9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.45, 0.9), linePos+vec2(.1, 0.9), 0.06));
  col =mix(col, textColor, line(absHitPos2, linePos+vec2(0.45, 0.4), linePos+vec2(.1, 0.4), 0.06));
}


vec3 GetReflectionMap(vec3 rayDir, vec3 normal)
{
  return texture(iChannel3, reflect( rayDir, normal )).rgb;
}

vec4 GetMaterial(vec3 rayDir, inout RayHit rayHit, vec2 fragCoord, inout float specSize)
{
  vec3 hitPos =TranslatePos(rayHit.hitPos, pitch, roll);
  vec2 center;
  float dist;

  float specLevel=0.7;
  specSize=0.7;

  float fre = pow( 1.0-abs(dot( rayHit.normal, rayDir )), 3.0 );
  fre = mix( .03, 1.0, fre );   

  // vec3 tint = vec3(0.62,.50,0.40)*1.15;
  vec3 tint = vec3(1.62, 1.50, 1.30)*0.65;
  vec3 brightCamo =1.15*tint;
  vec3 darkCamo = 0.78*tint;


  vec3 baseTexture = mix(brightCamo, darkCamo, smoothstep(0.5, 0.52, noise(hitPos*1.6)));

  // baseTexture = col;
  vec3 col=mix(brightCamo, darkCamo, smoothstep(0.5, 0.52, noise(hitPos*1.6)));
  vec3 reflection = GetReflectionMap(rayDir, rayHit.normal);
  // create base color mixes
  vec3 lightColor = (vec3(1.0));
  vec3 darkColor = (vec3(0.25));
  vec3 missilBaseCol =  lightColor*0.5;
  vec3 missilBaseCol2 =  darkColor;
  vec3 missilCol = lightColor;
  vec3 missilCol2 = lightColor*0.27;

  if (distance(rayHit.dist, rayHit.topWingDist)<.01)
  { 
    // top wing stripes
    col=mix(darkColor, baseTexture, smoothstep(0.55, 0.57, distance(0.85, hitPos.y)));
    col=mix(lightColor, col, smoothstep(.32, 0.34, distance(0.95, hitPos.y)));

    // create star (top wings)    
    center = vec2(-8.73, 0.95)-vec2(hitPos.z, hitPos.y);
    dist = length(center); 
    col=mix(darkColor, col, smoothstep(0.24, 0.26, dist));
    col=mix(lightColor, col, smoothstep(0.24, 0.26, (dist*1.15)+abs(cos( atan(center.y, center.x)*2.5)*0.13)));
  } else if (distance(rayHit.dist, rayHit.winDist)<.01)
  { 
    // windows
    col=vec3(0.2, 0.21, 0.22)*reflection;
    specSize=3.2;
    specLevel=3.5;
    fre = pow( 1.0-abs(dot(rayHit.normal, rayDir)), 3.0);
    fre = mix( mix( .0, .01, specLevel ), mix( .4, 1.0, specLevel ), fre );
    col = mix(col, vec3(1.5), fre );
  } else if (distance(rayHit.dist, rayHit.missileDist)<.01)
  {  
    specSize=2.;
    specLevel=2.;
    // small missiles
    col=mix(missilBaseCol, missilCol2, smoothstep(-3.35, -3.37, hitPos.z));
    col=mix(col, missilCol, smoothstep(-3.2, -3.22, hitPos.z));
    col=mix(missilCol2, col, smoothstep(.32, 0.34, distance(-4.75, hitPos.z)));
    col=mix(missilBaseCol, col, smoothstep(.25, 0.27, distance(-4.75, hitPos.z)));
  } else if (distance(rayHit.dist, rayHit.bombDist)<.01)
  { 
    specSize=2.;
    specLevel=1.7;
    // small bombs   
    col=mix(missilCol, missilBaseCol, smoothstep(1.18, 1.2, distance(-4.5, hitPos.z)));      
    col=mix(col, missilCol2, smoothstep(1.3, 1.32, distance(-4.5, hitPos.z)));
  } else if (distance(rayHit.dist, rayHit.bombDist2)<.01)
  {   
    specSize=2.;
    specLevel=1.8;
    // large bomb  
    col=mix(missilBaseCol2, missilCol, smoothstep(1.48, 1.5, distance(-4.1, hitPos.z)));      
    col=mix(col, missilBaseCol, smoothstep(1.6, 1.62, distance(-4.1, hitPos.z)));      
    col=mix(missilBaseCol, col, smoothstep(0.45, 0.47, distance(-4.1, hitPos.z)));
  } else
  {
    // remove camo from wing tip
    col =mix(col, brightCamo, line(vec2(abs(hitPos.x), hitPos.z), vec2(-7.25, 5.), vec2(-1.45, 1.7), 0.3));

    // color bottom gray
    col=mix(lightColor*0.7, col, step(0.01, hitPos.y));

    // front
    col = mix(col, lightColor, smoothstep(3.0, 3.02, hitPos.z));  
    col = mix(col, darkColor, smoothstep(3.08, 3.1, hitPos.z));
    col =mix(col*1.4, col, smoothstep(.07, .09, distance(1.8, hitPos.z)));


    // front wing stripes
    col=mix(darkColor, col, smoothstep(1.4, 1.42, distance(-6.90, hitPos.z)));
    col=mix(lightColor, col, smoothstep(1.3, 1.32, distance(-6.90, hitPos.z)));
    col=mix(darkColor, col, smoothstep(.84, 0.86, distance(-6.7, hitPos.z)));
    col=mix(lightColor, col, smoothstep(.22, 0.235, distance(-6.94, hitPos.z)));

    // vertical stripes   
    float xMod = mod(hitPos.x-0.5, 11.0);
    col=mix(darkColor, col, smoothstep(0.5, 0.52, distance(5., xMod)));
    col=mix(lightColor, col, smoothstep(0.4, 0.42, distance(5., xMod)));


    // boxes 
    vec2 absHitPos = abs(hitPos.xz);

    col =mix(col, col*1.40, drawRect(vec2(0.4, 2.0)-0.05, vec2(0.8, 2.0)+0.05+0.25, absHitPos));
    col =mix(col, col*0.2, drawRect(vec2(0.4, 2.0), vec2(0.8, 2.0)+0.2, absHitPos));

    // side 17      
    vec2 linePos = vec2(-0.55, 0.);
    vec3 textColor = vec3(0.2);
    if (hitPos.x<0.)
    {
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(0., -0.2), linePos+vec2(0., .2), 0.04));
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(-0.2, -0.2), linePos+vec2(-.4, -.2), 0.04));
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(-0.4, -0.2), linePos+vec2(-.25, .2), 0.04));
    } else
    {
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(-0.35, -0.2), linePos+vec2(-0.35, .2), 0.04));
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(0.1, -0.2), linePos+vec2(-.15, -.2), 0.04));
      col =mix(col, textColor, line(hitPos.zy, linePos+vec2(-0.15, 0.2), linePos+vec2(.10, -.2), 0.04));
    }  

    if (hitPos.y>0.15)
    {
      // letters BoundingBox
      if (drawRect(vec2(3.2, 3.8)-0.05, vec2(4.9, 4.8), absHitPos)>=1.)
      {
        AddLetters(hitPos.xz, col, vec2(-3.70, 3.60));
      }

      // more boxes 
      col =mix(col, col*1.40, drawRect(vec2(0.2, 3.6)-0.05, vec2(1., 3.6)+0.05+0.35, absHitPos)); 
      col =mix(col, col*0.2, drawRect(vec2(0.2, 3.6), vec2(1., 3.6)+0.3, absHitPos));          
      col =mix(col, col*0.2, drawRect(vec2(3.5, 4.8), vec2(4.5, 5.3), absHitPos));

      // create star (front wings)         
      center = vec2(5., -5.1)-vec2(xMod, hitPos.z);
      dist = length(center);
      col=mix(lightColor, col, smoothstep(0.8, 0.82, dist));
      col=mix(darkColor, col, smoothstep(0.7, 0.72, dist));
      col=mix(lightColor, col, smoothstep(0.7, 0.72, (dist*1.15)+abs(cos( atan(center.y, center.x)*2.5)*0.3)));
      col=mix(darkColor, col, smoothstep(0.6, 0.62, (dist*1.50)+abs(cos( atan(center.y, center.x)*2.5)*0.3)));
    } else
    {
      // bottom details
      col =mix(col, darkColor, line(vec2(abs(hitPos.x), hitPos.z), vec2(0., -1.5), vec2(-0.3, -1.5), 0.06));
      col =mix(col, darkColor, line(vec2(abs(hitPos.x), hitPos.z), vec2(-0.3, -1.5), vec2(-0.3, -1.), 0.085));
    }

    // rear wing stripes
    col=mix(darkColor, col, smoothstep(.55, 0.57, distance(-9.6, hitPos.z)));
    col=mix(lightColor, col, smoothstep(.5, 0.52, distance(-9.6, hitPos.z)));
    col=mix(darkColor, col, smoothstep(.4, 0.42, distance(-9.6, hitPos.z)));

    // esm pods
    col = mix(col, lightColor*0.75, smoothstep(7.02, 7.04, abs(hitPos.x)));

    // stabilizer
    col = mix(col, lightColor*0.75, smoothstep(1.72, 1.74, abs(hitPos.y)));

    // engines exhaust
    col=mix(mix(vec3(0.7), reflection, fre), col, step(.05, rayHit.engineDist));
    specSize=mix(4., specSize, step(.05, rayHit.engineDist));
    col=mix(col*0.23, col, step(.02, rayHit.blackDist));
    col=mix(col+0.5, col, smoothstep(.04, 0.10, distance(2.75, hitPos.z)));
  }
  fre = pow( 1.0-abs(dot(rayHit.normal, rayDir)), 7.0);
  fre = mix( 0., mix( .2, 1.0, specLevel*0.5 ), fre );
  col = mix( col, vec3(1.0, 1.0, 1.1)*1.5, fre );

  return vec4(col, specLevel);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
  R=iResolution.xy;

  vec2 mo = iMouse.xy/iResolution.xy;
  vec2 uv = fragCoord.xy / iResolution.xy;
  vec2 screenSpace = (-iResolution.xy + 2.0*(fragCoord))/iResolution.y;
  vec2 cloudPos = vec2(-iTime*1.3, -iTime*.95);
  float CAMZOOM = read(ivec2(52, 0));  

  // read missile data
  missilesLaunched = vec2(read(ivec2(100, 0)), read(ivec2(200, 0)));

  // read roll and speed values from buffer
  turn = read(ivec2(1, 10));
  roll = read(ivec2(1, 1));
  speed = read(ivec2(10, 1));
  pitch = read(ivec2(15, 1));
  rudderAngle = read(ivec2(6, 1));
  sunPos = readRGB(ivec2(50, 0));
  planePos = readRGB(ivec2(55, 0));
  pR(sunPos.xz, turn);

  // setup camera and ray direction
  vec2 camRot = readRGB(ivec2(57, 0)).xy;

  vec3 rayOrigin = vec3(CAMZOOM*cos(camRot.x), CAMZOOM*sin(camRot.y), -3.+CAMZOOM*sin(camRot.x) );
  mat3 ca = setCamera( rayOrigin, vec3(0., 0., -3. ), 0.0 );
  vec3 rayDir = ca * normalize( vec3(screenSpace.xy, 2.0) );

  // load background from buffer A
  vec4 color =  texture(iChannel0, uv);

  // calculate engine flare
  float lightDist = TraceLights(rayOrigin, rayDir);
    
  vec3 lightFlares = vec3(0.);
  lightFlares =  mix((vec3(1., 0.4, 0.2)), vec3(0.), smoothstep(0., .35, lightDist));             
  lightFlares =  mix(lightFlares+(2.*vec3(1., 0.5, 0.2)), lightFlares, smoothstep(0., 0.15, lightDist));
  lightFlares =  mix(lightFlares+vec3(1., 1., 1.), lightFlares, smoothstep(0., 0.08, lightDist));
  RayHit marchResult = TracePlane(rayOrigin, rayDir);

  if (marchResult.hit)
  {
    float specSize=1.0;

    marchResult.normal = calcNormal(marchResult.hitPos); 

    // create texture map and set specular levels
    color = GetMaterial(rayDir, marchResult, fragCoord, specSize);

    if (marchResult.dist != marchResult.eFlameDist)
    {
      // get lightning based on material
      vec3 light = GetSceneLight(color.a, marchResult.normal, marchResult, rayDir, rayOrigin, specSize);   

      // cloud shadows on plane if below cloud level
      if (planePos.y<=-CLOUDLEVEL)
      {  
        // get cloud shadows at rayMarch hitpos
        float clouds =clamp(max(0., -0.15+noise(marchResult.hitPos+planePos+vec3(cloudPos.x, 0., cloudPos.y))), 0., 1.)*.5;

        color.rgb*= 1.0-clouds;
        // sun light  
        color.rgb*= 1.+(clouds);
      }   

      // apply lightning
      color.rgb *=light;

      // balance colors
      color.rgb = pow(color.rgb, vec3(1.0/1.1));
    }

    color.rgb = mix(color.rgb, vec3(0.3, 0.5, 0.7), 0.1);    
    color.a=1.0;  

    lightFlares = mix(lightFlares, lightFlares*0., step(0.1, distance(marchResult.dist, marchResult.eFlameDist)));
  }

  color.rgb+=lightFlares;
  fragColor = color;
}
