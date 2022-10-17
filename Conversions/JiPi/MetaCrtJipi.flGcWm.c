
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define PI 3.141592654f


///////////////////////////
// Hash Functions
///////////////////////////

// From: Hash without Sine by Dave Hoskins
// https://www.shadertoy.com/view/4djSRW

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
//#define HASHSCALE1 .1031
//#define HASHSCALE3 to_float3(0.1031f, 0.1030f, 0.0973f)
//#define HASHSCALE4 to_float4(1031, 0.1030f, 0.0973f, 0.1099f)

// For smaller input rangers like audio tick or 0-1 UVs use these...
#define HASHSCALE1 443.8975f
#define HASHSCALE3 to_float3(443.897f, 441.423f, 437.195f)
#define HASHSCALE4 to_float3(443.897f, 441.423f, 437.195f, 444.129f)


//----------------------------------------------------------------------------------------
//  1 out, 1 in...
__DEVICE__ float hash11(float p)
{
  float3 p3  = fract_f3(to_float3_s(p) * HASHSCALE1);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}

//  2 out, 1 in...
__DEVICE__ float2 hash21(float p)
{
  float3 p3 = fract_f3(to_float3_s(p) * HASHSCALE3);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));

}

///  2 out, 3 in...
__DEVICE__ float2 hash23(float3 p3)
{
  p3 = fract_f3(p3 * HASHSCALE3);
  p3 += dot(p3, swi3(p3,y,z,x)+19.19f);
  return fract((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

//  1 out, 3 in...
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * HASHSCALE1);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}


///////////////////////////
// Data Storage
///////////////////////////

__DEVICE__ float4 LoadVec4( __TEXTURE2D__  sampler, in int2 vAddr, float2 iResolution )
{
  //return texelFetch( sampler, vAddr, 0 );
  return texture( sampler, (make_float2_cint(vAddr)+0.5f)/iResolution );
}

__DEVICE__ float3 LoadVec3( __TEXTURE2D__  sampler, in int2 vAddr, float2 iResolution )
{
  return swi3(LoadVec4( sampler, vAddr ),x,y,z);
}

__DEVICE__ bool AtAddress( int2 p, int2 c ) 
{
  //return all( equal( p, c ) ); 
  return (p.x==c.x && p.y==c.y);
}

__DEVICE__ void StoreVec4( in int2 vAddr, in float4 vValue, inout float4 *fragColor, in int2 fragCoord )
{
    *fragColor = AtAddress( fragCoord, vAddr ) ? vValue : *fragColor;
}

__DEVICE__ void StoreVec3( in int2 vAddr, in float3 vValue, inout float4 *fragColor, in int2 fragCoord )
{
    StoreVec4( vAddr, to_float4_aw( vValue, 0.0f ), fragColor, fragCoord);
}

///////////////////////////
// Camera
///////////////////////////

struct CameraState
{
    float3 vPos;
    float3 vTarget;
    float fFov;
    float2 vJitter;
    float fPlaneInFocus;
};
    
__DEVICE__ void Cam_LoadState( out CameraState *cam, __TEXTURE2D__  sampler, int2 addr, float2 iResolution )
{
    float4 vPos = LoadVec4( sampler, addr + to_int2(0,0), iResolution );
    (*cam).vPos = swi3(vPos,x,y,z);
    float4 targetFov = LoadVec4( sampler, addr + to_int2(1,0), iResolution );
    (*cam).vTarget = swi3(targetFov,x,y,z);
    (*cam).fFov = targetFov.w;
    float4 jitterDof = LoadVec4( sampler, addr + to_int2(2,0), iResolution );
    (*cam).vJitter = swi2(jitterDof,x,y);
    (*cam).fPlaneInFocus = jitterDof.z;
}

__DEVICE__ void Cam_StoreState( int2 addr, const in CameraState cam, inout float4 *fragColor, in int2 fragCoord )
{
    StoreVec4( addr + to_int2(0,0), to_float4_aw( cam.vPos, 0 ), fragColor, fragCoord );
    StoreVec4( addr + to_int2(1,0), to_float4_aw( cam.vTarget, cam.fFov ), fragColor, fragCoord );    
    StoreVec4( addr + to_int2(2,0), to_float4( cam.vJitter, cam.fPlaneInFocus, 0 ), fragColor, fragCoord );    
}

__DEVICE__ mat3 Cam_GetWorldToCameraRotMatrix( const CameraState cameraState )
{
  float3 vForward = normalize( cameraState.vTarget - cameraState.vPos );
  float3 vRight = normalize( cross(to_float3(0, 1, 0), vForward) );
  float3 vUp = normalize( cross(vForward, vRight) );
    
  return to_mat3_f3( vRight, vUp, vForward );
}

__DEVICE__ float2 Cam_GetViewCoordFromUV( float2 vUV, float2 res )
{
  float2 vWindow = vUV * 2.0f - 1.0f;
  vWindow.x *= res.x / res.y;

  return vWindow;  
}

__DEVICE__ void Cam_GetCameraRay( float2 vUV, float2 res, CameraState cam, out float3 *vRayOrigin, out float3 *vRayDir )
{
    float2 vView = Cam_GetViewCoordFromUV( vUV, res );
    *vRayOrigin = cam.vPos;
    float fPerspDist = 1.0f / _tanf( radians( cam.fFov ) );
    *vRayDir = normalize( mul_mat3_f3(Cam_GetWorldToCameraRotMatrix( cam ) , to_float3_aw( vView, fPerspDist )) );
}

__DEVICE__ float2 Cam_GetUVFromWindowCoord( float2 vWindow, float2 res )
{
    float2 vScaledWindow = vWindow;
    vScaledWindow.x *= res.y / res.x;

    return (vScaledWindow * 0.5f + 0.5f);
}

__DEVICE__ float2 Cam_WorldToWindowCoord(const in float3 vWorldPos, const in CameraState cameraState )
{
    float3 vOffset = vWorldPos - cameraState.vPos;
    float3 vCameraLocal;

    vCameraLocal = mul_f3_mat3(vOffset , Cam_GetWorldToCameraRotMatrix( cameraState ));
  
    float2 vWindowPos = swi2(vCameraLocal,x,y) / (vCameraLocal.z * _tanf( radians( cameraState.fFov ) ));
    
    return vWindowPos;
}

__DEVICE__ float EncodeDepthAndObject( float depth, int objectId )
{
    //depth = _fmaxf( 0.0f, depth );
    //objectId = _fmaxf( 0, objectId + 1 );
    //return _exp2f(-depth) + float(objectId);
    return depth;
}

__DEVICE__ float DecodeDepthAndObjectId( float value, out int *objectId )
{
    *objectId = 0;
    return _fmaxf(0.0f, value);
    //*objectId = int( _floor( value ) ) - 1; 
    //return _fabs( -_log2f(fract(value)) );
}

///////////////////////////////
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Update Logic for Commodore 64 by hubbe
// https://www.shadertoy.com/view/Xs3XW4

// TODO: break

#define CURSOR 0
#define STATE 1
#define MEMORY 2

__DEVICE__ float4 old_memory[MEMORY];
__DEVICE__ float4 memory[MEMORY];

#define STATE_READY           0
#define STATE_PRINT_READY     1
#define STATE_PRINT_READY_NL  2
#define STATE_LISTING         3
#define STATE_RUNNING         4
#define STATE_BREAK           5

#define LINE_ZERO 30
#define MAX_LINES 200

__DEVICE__ float vec4pick(int c, float4 v) {
    if (c == 0) return v.x;
    if (c == 1) return v.y;
    if (c == 2) return v.z;
    return v.w;
}

__DEVICE__ int vec4toint(int c, float4 v) {
    c = (int)(mod_f((float)(c), 8.0f));
    float tmp = vec4pick(c / 2, v);
    if (c != (c/2) * 2) {
        return (int)(mod_f(tmp, 256.0f));
    } else {
        return (int)(tmp) / 256;
    }
}

__DEVICE__ float4 vec4tochar(int c, float4 v) {
    return to_float4(vec4toint(c, v), 14/* fg */, 6 /* bg */, 0);
}


__DEVICE__ void init_screen(out float4 *fragColor, int x, int y) {
    *fragColor = to_float4(96, 14, 6, 0);

    if(y == 1) {
        if (x > 3 && x < 35) (*fragColor).x = 42.0f;
        if (x > 7 && x < 31) (*fragColor).x = 96.0f;
        x -= 9;
        float4 tmp;
        if (x < 0) return;
        if (x > 20) return;
        int n = x / 8;
        if (n == 0) tmp = to_float4(0x030F, 0x0D0D, 0x0F04, 0x0F12);  // COMMODOR
        if (n == 1) tmp = to_float4(0x0560, 0x3634, 0x6002, 0x0113);  // E 64 BAS
        if (n == 2) tmp = to_float4(0x0903, 0x6016, 0x3200, 0x0000);  // IC V2
        *fragColor = vec4tochar(x, tmp);
    }
    if (y == 3) {
        int n = x / 8;
        float4 tmp;
        if (n == 0) tmp = to_float4(0x6036, 0x340B, 0x6012, 0x010D); //  64K RAM
        if (n == 1) tmp = to_float4(0x6013, 0x1913, 0x1405, 0x0D60); //  SYSTEM 
        if (n == 2) tmp = to_float4(0x6033, 0x3839, 0x3131, 0x6002); //  38911 B
        if (n == 3) tmp = to_float4(0x0113, 0x0903, 0x6002, 0x1914); // ASIC BYT
        if (n == 4) tmp = to_float4(0x0513, 0x6006, 0x1205, 0x0560); // ES FREE
        *fragColor = vec4tochar(x, tmp);
    }
}

__DEVICE__ int key = -1;
__DEVICE__ int scroll = 0;

__DEVICE__ void NL() {
   memory[CURSOR].x = 0.0f;
   memory[CURSOR].y += 1.0f;
   if (memory[CURSOR].y >= 20.0f) {
       scroll += 1;
       memory[CURSOR].y -= 1.0f;
   }
}

__DEVICE__ void putc(int c) {
    key = c;
    memory[CURSOR].x += 1.0f;
    if (memory[CURSOR].x > 40.0f) NL();
}

__DEVICE__ int screen_pos(float4 v) {
    int x = (int)(v.x + 0.5f);
    int y = (int)(v.y + 0.5f);
    return x + y * 40;
}

__DEVICE__ float4 peek(int x, int y, __TEXTURE2D__ iChannel0, float2 iResolution) {
    //return texelFetch(iChannel0, to_int2(x, y), 0 );
    return texture(iChannel0, (make_float2(to_int2(x, y))+0.5f)/iResolution );
}

__DEVICE__ float4 peek(int pos, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int y = pos / 40;
    int x = pos - y * 40;
    return peek(x, y, iChannel0, iResolution);
}

__DEVICE__ float4 itoa(int x, int p) {
  int c = 96;
    int len = 1;
    if (x > 9) len = 2;
    if (x > 99) len = 3;
    if (p < len) {
        int power10 = 1;
        if (len - p == 2) power10 = 10;
        if (len - p == 3) power10 = 100;
        c = 48 + int(mod_f((float)(x / power10), 10.0f));        
    }
    return to_float4(c, 14, 6, 0);
}

__DEVICE__ int copy_from;
__DEVICE__ int copy_to;
__DEVICE__ int copy_length;

#define MSG_SYNTAX_ERROR -1
#define MSG_READY        -2
#define MSG_ZERO         -3
#define MSG_BREAK        -4

__DEVICE__ void copy(int pos, inout float4 *tmp, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int c = pos - copy_to;
    if (c >= 0 && c < copy_length) {
        *tmp = to_float4(0,0,0,0);
        if (copy_from == MSG_SYNTAX_ERROR) {
            float4 ch;
            if (c / 8 == 0)
              ch = to_float4(0x3F13, 0x190E, 0x1401, 0x1860);  // ?SYNTAX 
            if (c / 8 == 1)
              ch = to_float4(0x6005, 0x1212, 0x0F12, 0x0000);  // ERROR
            *tmp = vec4tochar(c, ch);
        } else if (copy_from == MSG_READY) {
            float4 ch = to_float4(0x1205, 0x0104, 0x192E, 0);
            *tmp = vec4tochar(c, ch) ; 
        } else if (copy_from == MSG_ZERO) {
            *tmp = to_float4_s(0);
        } else if (copy_from == MSG_BREAK) {
            float4 ch;
            if (c < 8)
              *tmp = vec4tochar(c, to_float4(0x0212, 0x0501, 0x0B60, 0x090E));  // BREAK IN
            if (c == 8)
              *tmp = to_float4(96, 14, 6, 0);
            if (c > 8)
              *tmp = itoa((int)(memory[STATE].y), c - 9);
        } else {
          *tmp = peek(copy_from + c, iChannel0, iResolution);
          if ((*tmp).x >= 128.0f) (*tmp).x -= 128.0f;
        }
    }
}

__DEVICE__ void memcpy(int dst, int src, int len) {
    copy_from = src;
    copy_to = dst;
    copy_length = len;
}


__DEVICE__ void print(int msg, int msg_len) {
    NL();
    memcpy(screen_pos(memory[CURSOR]) - 40, msg, msg_len);
}

__DEVICE__ void list() {
      memory[STATE].x = (float)(STATE_LISTING);
      memory[STATE].y = (float)(0);
}

__DEVICE__ int getchar(int x, int y, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int c = int(peek(x, y, iChannel0,iResolution).x);
    if (c > 128) c -= 128;
    return c;
}

__DEVICE__ int getchar(int pos, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int c = int(peek(pos).x, iChannel0, iResolution);
    if (c > 128) c -= 128;
    return c;
}

__DEVICE__ void skipwhite(inout int *pos, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int c = getchar(*pos, iChannel0, iResolution);
    if (c == 96) *pos = *pos + 1;    
    c = getchar(*pos, iCannel0, iResolution);
    if (c == 96) *pos = *pos + 1;    
    c = getchar(*pos, iCannel0, iResolution);
    if (c == 96) *pos = *pos + 1;    
}

__DEVICE__ bool strtod(inout int *pos, inout int *value, __TEXTURE2D__ iChannel0, float2 iResolution) {
  skipwhite(&pos);
  int c = getchar(pos, iCannel0, iResolution);
  int num = c - 48;
  if (num < 0 || num > 9) return false;
  value = num;
  pos = pos + 1;
  c = getchar(pos, iChannel0, iResolution);
  num = c - 48;
  if (num < 0 || num > 9) return true;
  value = value * 10 + num;
  pos = pos + 1;
  c = getchar(pos, iChannel0, iResolution);
  num = c - 48;
  if (num < 0 || num > 9) return true;
  value = value * 10 + num;
  return true;  
}

__DEVICE__ void skipnum(inout int *pos, __TEXTURE2D__ iChannel0, float2 iResolution) {
    int value;
    strtod(pos, value);
}

__DEVICE__ void parse(int pos, __TEXTURE2D__ iChannel0, float2 iResolution) {
    skipwhite(pos);
    int c1 = getchar(pos);
    int c2 = getchar(pos + 1);
    int c3 = getchar(pos + 2);
    int c4 = getchar(pos + 3);
    if (c1 == 12 && c2 == 9 && c3 == 19 && c4 == 20) { // list
        list();
        
    } else if (c1 == 18 && c2 == 21 && c3 == 14) { // run
        memory[STATE].x = (float)(STATE_RUNNING);
        int line = 0;
        int p = pos + 3;
        strtod(p, line);
        memory[STATE].y = (float)(line);
    } else if (c1 == 7 && c2 == 15 && c3 == 20 && c2 == 15) { // goto
        memory[STATE].x = (float)(STATE_RUNNING);
        int line = 0;
        int p = pos + 4;
        strtod(p, line);
        memory[STATE].y = (float)(line);
    } else if (c1 == 16 && c2 == 18 && c3 == 9 && c4 == 14) {
        // print
        NL();
        int p = pos + 7;
        int len = 0;
        for (int l = 0; l < 33; l++) {
            if (len == 0 && (int)(peek(p + l).x) == 34)
                len = l;
        }
        
        memcpy(screen_pos(memory[CURSOR]) - 40, pos + 7, len);
    } else if (c1 == 96 && c2 == 96 && c3 == 96 && c4 == 96) {
        // Do nothing
    } else {
        int value = 0;
        int p = pos;
        if (strtod(p, value)) {
            if (getchar(p) == 96 && getchar(p+1) == 96 && getchar(p+2) == 96) {
              memcpy((LINE_ZERO + value) * 40, MSG_ZERO, 10);
            } else {
              memcpy((LINE_ZERO + value) * 40, pos, 40);
            }
        } else {
          NL();
          NL();
          // ?SYNTAX ERROR
          memcpy(screen_pos(memory[CURSOR]) - 40, MSG_SYNTAX_ERROR, 14);
          memory[STATE].x = (float)(STATE_PRINT_READY);
        }
    }
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  
    fragCoord+=0.5f;

    copy_length = 0;
    int x = (int)(fragCoord.x);
    int y = (int)(fragCoord.y);
    if (x > 40 && y > 25) 
    {
      //discard;
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    if (iFrame < 3) {
      memory[CURSOR] = to_float4(0, 5, 0, 0);
      memory[STATE].x = (float)(STATE_PRINT_READY);
    } else {
      for (int i = 0; i < MEMORY; i++) {
        memory[i] = peek(i + 40, 0, iChannel0, iResolution);
        old_memory[i] = memory[i];
      }
    } 

    fragColor = peek(x, y, iChannel0, iResolution);

    if (memory[STATE].x == (float)(STATE_LISTING)) {
        int line = int(memory[STATE].y);
        memory[STATE].x = (float)(STATE_PRINT_READY_NL);
        
        for (int i = 0; i < 50; i++) {
            if (getchar(0, LINE_ZERO + line + i) != 0) {
                memory[STATE].x = (float)(STATE_LISTING);
                memory[STATE].y = (float)(line + i + 1);
                NL();
                memcpy(screen_pos(memory[CURSOR]) - 40, 40 * (LINE_ZERO + line + i), 40);
                break;
            }
        }
    } else if (memory[STATE].x == (float)(STATE_RUNNING)) {
        bool esc = texture(iChannel1, to_float2(27.5f / 256.0f, 0.5f/3.0f)).x > 0.5f;
        if (esc) {
            NL();
            memory[STATE].x = (float)(STATE_BREAK);
        } else {
             int line = (int)(memory[STATE].y);
          memory[STATE].x = (float)(STATE_PRINT_READY_NL);
        
          for (int i = 0; i < 50; i++) {
              if (getchar(0, LINE_ZERO + line + i, iChannel0, iResolution) != 0) {
                  memory[STATE].x = (float)(STATE_RUNNING);
                  memory[STATE].y = (float)(line + i + 1);
                  int pos = 40 * (LINE_ZERO + line + i);
                  skipnum(pos, iChannel0, iResolution);
                  parse(pos, iChannel0, iResolution);
                  break;
              }
          }
        }
    } else if (memory[STATE].x == (float)(STATE_BREAK)) {
      memory[STATE].x = float(STATE_PRINT_READY);
        print(MSG_BREAK, 12);
    } else if (memory[STATE].x == (float)(STATE_PRINT_READY)) {
      memory[STATE].x = (float)(STATE_READY);
        print(MSG_READY, 6);
    } else if (memory[STATE].x == (float)(STATE_PRINT_READY_NL)) {
      memory[STATE].x = (float)(STATE_READY);
        NL();
        print(MSG_READY, 6);
    } else {
      bool shift = texture(iChannel1, to_float2(16.5f / 256.0f, 0.5f/3.0f)).x > 0.5f;

      for (int key = 0; key < 64; key++) {
          float key_val = texture(iChannel1, to_float2(((float)(key) + 32.5f)/256.0f, 0.5f)).x;
          if (key_val > 0.6f) {
              if (key > 32)
                  putc(key - 32 + (shift ? 64 : 0));
              else if (key == 0)
                  putc(96);
              else if (key >= 16)
                  putc(key + 32 + (shift ? -16 : 0));
          }
      }
    
      if (texture(iChannel1, to_float2(13.5f/256.0f, 0.5f)).x > 0.6f) {
          int y = int(memory[CURSOR].y);
          NL();
          parse(y * 40, iChannel0, iResolution);
          // Enter
      }
        if (texture(iChannel1, to_float2(8.5f/256.0f, 0.5f)).x > 0.6f) {
            int x = int(memory[CURSOR].x);
            if (x > 0) {
                x = x - 1;
                int p = screen_pos(memory[CURSOR]);
                memcpy(p - 1, p, 40 - x);
                memory[CURSOR].x = float(x);
            }
        }
    }
     
    if (x >= 0 && x < 40 && y >=0 && y < 20) {
      if (iFrame < 2) {
        init_screen(fragColor, x, y);
        SetFragmentShaderComputedColor(fragColor);
        return;
      }
      fragColor = peek(x, y + scroll, iChannel0, iResolution);
      int sp = x + y * 40;
      
      if (sp + 40 * scroll == screen_pos(old_memory[CURSOR])) {
          fragColor.x = mod_f(fragColor.x, 128.0f);
          if (key != -1)
          {
              fragColor.x = float(key);
          }
      }

      if (sp == screen_pos(memory[CURSOR])) {
          if (fract(iTime) > 0.5f) {
            fragColor.x += 128.0f;
         }
      }
      copy(sp, &fragColor, iChannel0, iResolution);
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    copy(x + y * 40, &fragColor, iChannel0, iResolution);
    if (x >= 0 && x < 40 && y >= 20 && y <= 25) {
       fragColor = to_float4(96, 14, 6, 0);
    }
    if (y == 0) {
      for (int i = 0; i < MEMORY; i++) {
        if (i + 40 == x) {
          fragColor = memory[i];
          SetFragmentShaderComputedColor(fragColor);
          return;
        }
      }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Wood' to iChannel2
// Connect Buffer B 'Preset: Keyboard' to iChannel1
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Screen image rendering


// https://www.shadertoy.com/view/lslGDn
#define SHADERTOY

// Commodore 64 by hubbe
// https://www.shadertoy.com/view/Xs3XW4
//#define C64

// https://www.shadertoy.com/view/lsl3Rn
//#define SPECTRUM

// https://www.shadertoy.com/view/MdsGzr
//#define ELITE

// https://www.shadertoy.com/view/4lG3Wz
//#define MANDELBROT

#ifdef ELITE

/////////////////////////////////////
// Settings

#define EMULATE_8BIT

#ifdef EMULATE_8BIT
  #define LIMIT_FRAMERATE
  //#define SCANLINE_EFFECT
  #define NON_AA_LINES
  #define LOW_RESOLUTION
  #define XOR_PIXELS
#endif

#ifndef NON_AA_LINES
#ifdef XOR_PIXELS
#undef XOR_PIXELS
#endif
#endif

float kFramesPerSecond = 7.5f;

#ifdef LOW_RESOLUTION
__DEVICE__ float2 kWindowResolution = to_float2(256.0f, 192.0f);
#else
__DEVICE__ float2 kWindowResolution = iResolution;
#endif

__DEVICE__ float kAALineWidth = 1.0f;

/////////////////////////////////////
// Time

__DEVICE__ float GetSceneTime(float iTime)
{
  #ifdef LIMIT_FRAMERATE
    return (_floor(iTime * kFramesPerSecond) / kFramesPerSecond);
  #else
    return iTime;
  #endif
}

/////////////////////////////////////
// Line Rasterization

#ifdef NON_AA_LINES
__DEVICE__ float RasterizeLine(const in float2 vPixel, const in float2 vA, const in float2 vB)
{
  // vPixel is the centre of the pixel to be rasterized
  
  float2 vAB = vB - vA;  
  float2 vAbsAB = abs_f2(vAB);
  float fGradientSelect = step(vAbsAB.y, vAbsAB.x);

  float2 vAP = vPixel - vA;

  float fAB = _mix(vAB.y, vAB.x, fGradientSelect);
  float fAP = _mix(vAP.y, vAP.x, fGradientSelect);
  
  // figure out the co-ordinates we intersect the vPixelCentre x or y axis
  float t = fAP / fAB;  
  float2 vIntersection = vA + (vB - vA) * t;
  float2 vIntersectionDist = _fabs(vIntersection - vPixel);
  
  float2 vResult = step(vIntersectionDist, to_float2_s(0.5f));

  // mask out parts of the line beyond the beginning or end
  float fClipSpan = step(t, 1.0f) * step(0.0f, t);  
  
  // select the x or y axis result based on the gradient of the line
  return _mix(vResult.x, vResult.y, fGradientSelect) * fClipSpan;
}
#else
__DEVICE__ float RasterizeLine(const in float2 vPixel, const in float2 vA, const in float2 vB)
{
  // AA version based on distance to line
  
  // vPixel is the co-ordinate within the pixel to be rasterized
  
  float2 vAB = vB - vA;  
  float2 vAP = vPixel - vA;
  
  float2 vDir = normalize(vAB);
  float fLength = length(vAB);
  
  float t = clamp(dot(vDir, vAP), 0.0f, fLength);
  float2 vClosest = vA + t * vDir;
  
  float fDistToClosest = 1.0f - (length(vClosest - vPixel) / kAALineWidth);

  float i =  clamp(fDistToClosest, 0.0f, 1.0f);
  
  return _sqrtf(i);
}
#endif

/////////////////////////////////////
// Matrix Fun

__DEVICE__ mat4 SetRotTrans( float3 r, float3 t )
{
    float a = _sinf(r.x); float b = _cosf(r.x); 
    float c = _sinf(r.y); float d = _cosf(r.y); 
    float e = _sinf(r.z); float f = _cosf(r.z); 

    float ac = a*c;
    float bc = b*c;

    return to_mat4( d*f,      d*e,       -c, 0.0f,
                    ac*f-b*e, ac*e+b*f, a*d, 0.0f,
                    bc*f+a*e, bc*e-a*f, b*d, 0.0f,
                    t.x,      t.y,      t.z, 1.0f );
}

__DEVICE__ mat4 SetProjection( float d )
{
    return to_mat4( 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, d,
                    0.0f, 0.0f, 0.0f, 0.0f );
}

__DEVICE__ mat4 SetWindow( float2 s, float2 t )
{
    return to_mat4( s.x, 0.0f, 0.0f, 0.0f,
                 0.0f, s.y, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f, 0.0f,
                 t.x, t.y, 0.0f, 1.0f );
}

/////////////////////////////////////
// Window Border Setup

__DEVICE__ const float2 kWindowMin = to_float2(0.1f, 0.1f);
__DEVICE__ const float2 kWindowMax = to_float2(0.9f, 0.9f);
__DEVICE__ const float2 kWindowRange = kWindowMax - kWindowMin;

__DEVICE__ float2 ScreenUvToWindowPixel(float2 vUv)
{
  #ifdef LOW_RESOLUTION
    vUv = ((vUv - kWindowMin) / kWindowRange);
  #endif
  return vUv * kWindowResolution;
}

__DEVICE__ float IsPixelInWindow(float2 vPixel)
{
  float2 vResult =  step(vPixel, kWindowResolution)
                  * step(to_float2_s(0.0f), vPixel);
  return _fminf(vResult.x, vResult.y);
}

/////////////////////////////

__DEVICE__ const int kVertexCount = 30;
__DEVICE__ float3 kVertices[kVertexCount];

__DEVICE__ void SetupVertices()
{
    kVertices[0] = to_float3(40, 0.0f, 95);
    kVertices[1] = to_float3(-40, 0.0f, 95);
    kVertices[2] = to_float3(00, 32.5f, 30);
    kVertices[3] = to_float3(-150,-3.8f,-10);
    kVertices[4] = to_float3(150,-3.8f,-10);
    kVertices[5] = to_float3(-110, 20,-50);
    kVertices[6] = to_float3(110, 20,-50);
    kVertices[7] = to_float3(160,-10,-50);
    kVertices[8] = to_float3(-160,-10,-50);
    kVertices[9] = to_float3(0, 32.5f,-50);
    kVertices[10] = to_float3(-40,-30,-50);
    kVertices[11] = to_float3(40,-30,-50);
    kVertices[12] = to_float3(-45, 10,-50);
    kVertices[13] = to_float3(-10, 15,-50);
    kVertices[14] = to_float3( 10, 15,-50);
    kVertices[15] = to_float3(45, 10,-50);      
    kVertices[16] = to_float3(45,-15,-50);
    kVertices[17] = to_float3(10,-20,-50);
    kVertices[18] = to_float3(-10,-20,-50);
    kVertices[19] = to_float3(-45,-15,-50);
    kVertices[20] = to_float3(-2,-2, 95);
    kVertices[21] = to_float3(-2,-2, 112.5f);
    kVertices[22] = to_float3(-100,-7.5f,-50);
    kVertices[23] = to_float3(-100, 7.5f,-50);
    kVertices[24] = to_float3(-110, 0,-50);
    kVertices[25] = to_float3( 100, 7.5f,-50);
    kVertices[26] = to_float3( 110, 0,-50);
    kVertices[27] = to_float3( 100,-7.5f,-50);
    kVertices[28] = to_float3(  0,0, 95);
    kVertices[29] = to_float3(  0,0, 112.5f);    
}

__DEVICE__ float BackfaceCull(float2 A, float2 B, float2 C)
{
  float2 AB = B - A;
  float2 AC = C - A;
  float c = AB.x * AC.y - AB.y * AC.x;
  return step(c, 0.0f);
}

__DEVICE__ float Accumulate( const float x, const float y )
{
#ifdef XOR_PIXELS
  return x + y;
#else
  return _fmaxf(x, y);
#endif
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
  fragCoord+=0.5f; 
  
  float2 uv = fragCoord / iResolution;
  
  // get window pixel co-ordinates for centre of current pixel
  float2 vWindowPixelCords = ScreenUvToWindowPixel(uv);
  float2 vPixel = _floor(vWindowPixelCords) + 0.5f;
  
  // Setup Transform
  mat4 mTransform;

  {
    float3 vRot = to_float3(0.1f, 0.2f, 0.3f) * GetSceneTime(iTime);
    
    /*if(iMouse.z > 0.0f)
    {
      float2 vUnitMouse = swi2(iMouse,x,y) / iResolution;
      vRot= to_float3(swi2(vUnitMouse,y,x) * to_float2(1.0f, 1.0f) + to_float2(1.5f, 0.5f), 0.0f) * 3.14159f * 2.0f;
    }*/
    
    float3 vTrans = to_float3(0.0f, 0.0f, 350.0f);
    mat4 mRotTrans = SetRotTrans( vRot, vTrans );
    mat4 mProjection = SetProjection( 1.0f );
    mat4 mWindow = SetWindow( to_float2(1.0f, iResolution.x/iResolution.y) * kWindowResolution, to_float2_s(0.5f) * kWindowResolution );
  
    mTransform = mul_mat4_mat4(mul_mat4_mat4(mWindow , mProjection) , mRotTrans);
  }

  // Transform Vertices to Window Pixel Co-ordinates
  SetupVertices();
  
  float2 vScrVtx[kVertexCount];
  for(int i=0; i<kVertexCount; i++)
  {
    float4 vhPos = mul_mat4_f4(mTransform , to_float4_aw(kVertices[i], 1.0f));
    vScrVtx[i] = swi2(vhPos,x,y) / vhPos.w;
  }

  // Cull Faces
  const int kFaceCount = 14;
  float fFaceVisible[kFaceCount];
  
  // hull 
  fFaceVisible[0] = BackfaceCull( vScrVtx[2], vScrVtx[1], vScrVtx[0] );
  fFaceVisible[1] = BackfaceCull( vScrVtx[0], vScrVtx[1], vScrVtx[10] );
  fFaceVisible[2] = BackfaceCull( vScrVtx[6], vScrVtx[2], vScrVtx[0] );
  fFaceVisible[3] = BackfaceCull( vScrVtx[0], vScrVtx[4], vScrVtx[6] );
  fFaceVisible[4] = BackfaceCull( vScrVtx[0], vScrVtx[11], vScrVtx[7] );
  fFaceVisible[5] = BackfaceCull( vScrVtx[1], vScrVtx[2], vScrVtx[5] );

  fFaceVisible[6] = BackfaceCull( vScrVtx[5], vScrVtx[3], vScrVtx[1] );
  fFaceVisible[7] = BackfaceCull( vScrVtx[1], vScrVtx[3], vScrVtx[8] );
  fFaceVisible[8] = BackfaceCull( vScrVtx[5], vScrVtx[2], vScrVtx[9] );
  fFaceVisible[9] = BackfaceCull( vScrVtx[2], vScrVtx[6], vScrVtx[9] );
  fFaceVisible[10] = BackfaceCull( vScrVtx[5], vScrVtx[8], vScrVtx[3] );
  fFaceVisible[11] = BackfaceCull( vScrVtx[7], vScrVtx[6], vScrVtx[4] );
  fFaceVisible[12] = BackfaceCull( vScrVtx[9], vScrVtx[6], vScrVtx[7] );
  
  // engines - all culled together
  fFaceVisible[13] = BackfaceCull( vScrVtx[14], vScrVtx[15], vScrVtx[16] );

  // Draw Lines
  
  float fResult = 0.0f;
  
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[2]) * _fmaxf(fFaceVisible[0], fFaceVisible[2]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[4]) * _fmaxf(fFaceVisible[3], fFaceVisible[4]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[6]) * _fmaxf(fFaceVisible[2], fFaceVisible[3]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[1], vScrVtx[0]) * _fmaxf(fFaceVisible[0], fFaceVisible[1]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[1], vScrVtx[10]) * _fmaxf(fFaceVisible[1], fFaceVisible[7]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[1]) * _fmaxf(fFaceVisible[0], fFaceVisible[5]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[5]) * _fmaxf(fFaceVisible[5], fFaceVisible[8]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[9]) * _fmaxf(fFaceVisible[8], fFaceVisible[9]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[3], vScrVtx[1]) * _fmaxf(fFaceVisible[6], fFaceVisible[7]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[3], vScrVtx[8]) * _fmaxf(fFaceVisible[7], fFaceVisible[10]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[4], vScrVtx[6]) * _fmaxf(fFaceVisible[3], fFaceVisible[11]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[1]) * _fmaxf(fFaceVisible[5], fFaceVisible[6]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[3]) * _fmaxf(fFaceVisible[6], fFaceVisible[10]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[8]) * _fmaxf(fFaceVisible[10], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[6], vScrVtx[2]) * _fmaxf(fFaceVisible[2], fFaceVisible[9]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[6], vScrVtx[9]) * _fmaxf(fFaceVisible[9], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[7], vScrVtx[4]) * _fmaxf(fFaceVisible[4], fFaceVisible[11]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[7], vScrVtx[6]) * _fmaxf(fFaceVisible[11], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[8], vScrVtx[10]) * _fmaxf(fFaceVisible[7], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[9], vScrVtx[5]) * _fmaxf(fFaceVisible[8], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[10], vScrVtx[11]) * _fmaxf(fFaceVisible[1], fFaceVisible[12]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[11], vScrVtx[0]) * _fmaxf(fFaceVisible[1], fFaceVisible[4]));
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[11], vScrVtx[7]) * _fmaxf(fFaceVisible[4], fFaceVisible[12]));

  if(fFaceVisible[13] > 0.0f)  
  {
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[12], vScrVtx[13] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[13], vScrVtx[18] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[14], vScrVtx[15] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[15], vScrVtx[16] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[16], vScrVtx[17] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[17], vScrVtx[14] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[18], vScrVtx[19] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[19], vScrVtx[12] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[25], vScrVtx[26] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[26], vScrVtx[27] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[27], vScrVtx[25] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[22], vScrVtx[23] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[23], vScrVtx[24] ));
    fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[24], vScrVtx[22] ));
  }
  
  // gun
  fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[28], vScrVtx[29]));

  #ifdef XOR_PIXELS  
  fResult = mod_f(fResult, 2.0f);
  #endif
  
  // Clip pixel to window border
  fResult *= IsPixelInWindow(vPixel);
  
  // Scanline Effect
  #ifdef SCANLINE_EFFECT  
    float fScanlineEffect = _cosf((vWindowPixelCords.y + 0.5f) * 3.1415f * 2.0f) * 0.5f + 0.5f;
    fResult = (fResult * 0.9f + 0.1f) * (fScanlineEffect * 0.2f + 0.8f);
  #endif
    
  fragColor = to_float4_aw(to_float3_s(fResult),1.0f);
  SetFragmentShaderComputedColor(fragColor);
  
}

#endif



#ifdef SPECTRUM

// Screen Image - @P_Malin

//#define LOADING_LOOP
 
__DEVICE__ float2 kResolution = to_float2(256.0f, 192.0f);
 
// Border phases

const float kPhaseBlank = 0.0f;
const float kPhaseSilent = 1.0f;
const float kPhaseHeader = 2.0f;
const float kPhaseData = 3.0f;
const float kPhaseRunning = 4.0f;
 
// Loading phases

const float3 vTimeSilent1  = to_float3(1.0f,  5.0f,                       kPhaseSilent);
const float3 vTimeHeader1  = to_float3(2.0f,  vTimeSilent1.y + 2.0f,      kPhaseHeader);
const float3 vTimeData1    = to_float3(3.0f,  vTimeHeader1.y + 0.125f,    kPhaseData);
 
const float3 vTimeBlank2   = to_float3(4.0f,  vTimeData1.y + 1.0f,        kPhaseBlank);
const float3 vTimeSilent2  = to_float3(5.0f,  vTimeBlank2.y + 2.0f,       kPhaseSilent);
const float3 vTimeHeader2  = to_float3(6.0f,  vTimeSilent2.y + 2.0f,      kPhaseHeader);
const float3 vTimeData2    = to_float3(7.0f,  vTimeHeader2.y + 1.0f,      kPhaseData);
 
const float3 vTimeSilent3  = to_float3(8.0f,  vTimeData2.y + 2.0f,        kPhaseSilent);
const float3 vTimeHeader3  = to_float3(9.0f,  vTimeSilent3.y + 2.0f,      kPhaseHeader);
const float3 vTimeData3    = to_float3(10.0f, vTimeHeader3.y + 0.125f,    kPhaseData);
 
const float3 vTimeSilent4  = to_float3(11.0f, vTimeData3.y + 2.0f,        kPhaseSilent);
const float3 vTimeHeader4  = to_float3(12.0f, vTimeSilent4.y + 2.0f,      kPhaseHeader);
const float3 vTimeData4    = to_float3(13.0f, vTimeHeader4.y + 38.0f,     kPhaseData);
 
const float3 vTimeRunning  = to_float3(14.0f, vTimeData4.y + 10.0f,       kPhaseRunning);
 
const float3 vTimeTotal    = to_float3(15.0f, vTimeRunning.y,            kPhaseBlank);
       
__DEVICE__ float4 GetPhase(float fTime)
{             
        float3 vResult = vTimeRunning;
                
        vResult = _mix( vResult, vTimeData4, step(fTime, vTimeData4.y ) );
        vResult = _mix( vResult, vTimeHeader4, step(fTime, vTimeHeader4.y ) );
        vResult = _mix( vResult, vTimeSilent4, step(fTime, vTimeSilent4.y ) );
 
        vResult = _mix( vResult, vTimeData3, step(fTime, vTimeData3.y ) );
        vResult = _mix( vResult, vTimeHeader3, step(fTime, vTimeHeader3.y ) );
        vResult = _mix( vResult, vTimeSilent3, step(fTime, vTimeSilent3.y ) );
               
        vResult = _mix( vResult, vTimeData2, step(fTime, vTimeData2.y ) );
        vResult = _mix( vResult, vTimeHeader2, step(fTime, vTimeHeader2.y ) );
        vResult = _mix( vResult, vTimeSilent2, step(fTime, vTimeSilent2.y ) );
        vResult = _mix( vResult, vTimeBlank2, step(fTime, vTimeBlank2.y ) );
 
        vResult = _mix( vResult, vTimeData1, step(fTime, vTimeData1.y ) );
        vResult = _mix( vResult, vTimeHeader1, step(fTime, vTimeHeader1.y ) );
        vResult = _mix( vResult, vTimeSilent1, step(fTime, vTimeSilent1.y ) );
               
        return to_float4(vResult.z, vResult.x, fTime - vResult.y, vResult.y);
}
 
__DEVICE__ float GetRasterPosition(in float2 fragCoord, float2 iResolution)
{
        return (fragCoord.x + fragCoord.y * iResolution.x) / (iResolution.x * iResolution.y);
}
 
__DEVICE__ float IsBorder(float2 vScreenUV)
{
        if(vScreenUV.x < 0.0f)
                        return 1.0f;
        if(vScreenUV.x >= 1.0f)
                        return 1.0f;
        if(vScreenUV.y < 0.0f)
                        return 1.0f;
        if(vScreenUV.y >= 1.0f)
                        return 1.0f;
       
        return 0.0f;
}
 
 
__DEVICE__ float3 GetBorderColour(float fPhase,in float2 fragCoord, float2 iResolution, float iTime)
{
  float raster = GetRasterPosition(fragCoord, iResolution);
  
  float3 vCol = to_float3_s(0.0f);
  
  if(fPhase == kPhaseBlank)
  {                       
    vCol = to_float3_s(1.0f);           
  }
  else  
  if(fPhase == kPhaseSilent)
  {
    float fBlend = step(fract(iTime * 0.5f), 0.5f);
    vCol = _mix( to_float3(0.0f, 1.0f, 1.0f), to_float3(1.0f, 0.0f, 0.0f), fBlend);           
  }
  else
  if(fPhase == kPhaseHeader)
  {
    float fBarSize = 12.0f;
    float fScrollSpeed = 10.0f;
    float fBlend = step(fract(raster * fBarSize + iTime * fScrollSpeed), 0.5f);
    vCol = _mix( to_float3(0.0f, 1.0f, 1.0f), to_float3(1.0f, 0.0f, 0.0f), fBlend);           
  }
  else
  if(fPhase == kPhaseData)
  {
    float fBarSize = 25.0f;
    float fScrollSpeed = 1.0f;
    float fBlend = step(fract(raster * fBarSize + iTime * fScrollSpeed + _sinf(iTime * 20.0f + raster * 16.0f)), 0.5f);
    vCol = _mix(to_float3(1.0f, 1.0f, 0.0f), to_float3(0.0f, 0.0f, 1.0f), fBlend);                     
  }
  
  return vCol;
}
 
 
__DEVICE__ float GetLoadingScreenIntensity( float2 vPos )
{
  float2 vUV = vPos / kResolution;
  float r = 0.25f;
  float2 vDist = (vUV - 0.5f) / r;
  float len = length(vDist);
  float3 vNormal = to_float3_aw(vDist.x, _sqrtf(1.0f - len * len), vDist.y);
  float3 vLight = normalize( to_float3(1.0f, 1.0f, -1.0f) );
  if(len < 1.0f)
  {
    return _fmaxf(0.0f, dot(vNormal, vLight));
  }
  
  return 0.7f - vUV.y * 0.6f;
}
 
__DEVICE__ float CrossHatch(float fIntensity, float2 vPos)
{
  float2 vGridPos = mod_f(vPos, 4.0f);
  
  float fThreshold = fract(vGridPos.x * 0.25f + vGridPos.y * 0.5f) * 0.75f + fract(vGridPos.y * 0.25f + vGridPos.x * 0.5f) * 0.25f;
  
  return step(fIntensity, fThreshold);
}
 
__DEVICE__ float GetLoadingScreenPixel( float2 vPos )
{
        return CrossHatch(GetLoadingScreenIntensity(vPos), vPos);
}
 
__DEVICE__ float2 GetScreenPixelCoord( float2 vScreenUV )
{
        float2 vPixelPos = _floor(vScreenUV * kResolution);
        vPixelPos.y = 192.0f - vPixelPos.y;
       
        return vPixelPos;
}
 
__DEVICE__ float PixelAddress( float2 vPixelPos )
{               
        float fBand = _floor(vPixelPos.y / 64.0f);
       
        float fBandPos = mod_f(vPixelPos.y, 64.0f);
 
        float fCharRow = mod_f(fBandPos, 8.0f);
        float fCharPos = _floor(fBandPos / 8.0f);
 
        float fBytePos = _floor(vPixelPos.x / 8.0f);
 
        float fLineTime = fBand * 64.0f + fCharRow * 8.0f + fCharPos;
        return (fBytePos + fLineTime * (256.0f / 8.0f));
}
 
__DEVICE__ float AttributeAddress(float2 vCharPos)
{             
  float kAttributeStart = 256.0f * 192.0f / 8.0f;
  return kAttributeStart + vCharPos.x + vCharPos.y * 32.0f;
}
 
__DEVICE__ float GetCharByte(const in float value)
{
        float result = 0.0f;
        result = _mix(result, 0.0f, step(value, 919.0f) );
        result = _mix(result, 32.0f, step(value, 918.5f) );
        result = _mix(result, 28.0f, step(value, 914.5f) );
        result = _mix(result, 0.0f, step(value, 913.5f) );
        result = _mix(result, 56.0f, step(value, 894.5f) );
        result = _mix(result, 68.0f, step(value, 893.5f) );
        result = _mix(result, 56.0f, step(value, 890.5f) );
        result = _mix(result, 0.0f, step(value, 889.5f) );
        result = _mix(result, 84.0f, step(value, 878.5f) );
        result = _mix(result, 104.0f, step(value, 874.5f) );
        result = _mix(result, 0.0f, step(value, 873.5f) );
        result = _mix(result, 56.0f, step(value, 851.5f) );
        result = _mix(result, 4.0f, step(value, 830.5f) );
        result = _mix(result, 60.0f, step(value, 829.5f) );
        result = _mix(result, 68.0f, step(value, 828.5f) );
        result = _mix(result, 60.0f, step(value, 826.5f) );
        result = _mix(result, 0.0f, step(value, 825.5f) );
        result = _mix(result, 60.0f, step(value, 782.5f) );
        result = _mix(result, 68.0f, step(value, 781.5f) );
        result = _mix(result, 60.0f, step(value, 780.5f) );
        result = _mix(result, 4.0f, step(value, 779.5f) );
        result = _mix(result, 56.0f, step(value, 778.5f) );
        result = _mix(result, 0.0f, step(value, 777.5f) );
        result = _mix(result, 60.0f, step(value, 670.5f) );
        result = _mix(result, 66.0f, step(value, 669.5f) );
        result = _mix(result, 2.0f, step(value, 668.5f) );
        result = _mix(result, 60.0f, step(value, 667.5f) );
        result = _mix(result, 64.0f, step(value, 666.5f) );
        result = _mix(result, 60.0f, step(value, 665.5f) );
        result = _mix(result, 0.0f, step(value, 664.5f) );
        result = _mix(result, 64.0f, step(value, 646.5f) );
        result = _mix(result, 124.0f, step(value, 644.5f) );
        result = _mix(result, 66.0f, step(value, 643.5f) );
        result = _mix(result, 124.0f, step(value, 641.5f) );
        result = _mix(result, 0.0f, step(value, 640.5f) );
        result = _mix(result, 60.0f, step(value, 638.5f) );
        result = _mix(result, 66.0f, step(value, 637.5f) );
        result = _mix(result, 60.0f, step(value, 633.5f) );
        result = _mix(result, 0.0f, step(value, 632.5f) );
        result = _mix(result, 66.0f, step(value, 630.5f) );
        result = _mix(result, 70.0f, step(value, 629.5f) );
        result = _mix(result, 74.0f, step(value, 628.5f) );
        result = _mix(result, 82.0f, step(value, 627.5f) );
        result = _mix(result, 98.0f, step(value, 626.5f) );
        result = _mix(result, 66.0f, step(value, 625.5f) );
        result = _mix(result, 0.0f, step(value, 624.5f) );
        result = _mix(result, 126.0f, step(value, 614.5f) );
        result = _mix(result, 64.0f, step(value, 613.5f) );
        result = _mix(result, 0.0f, step(value, 608.5f) );
        result = _mix(result, 62.0f, step(value, 590.5f) );
        result = _mix(result, 8.0f, step(value, 589.5f) );
        result = _mix(result, 62.0f, step(value, 585.5f) );
        result = _mix(result, 0.0f, step(value, 584.5f) );
        result = _mix(result, 60.0f, step(value, 574.5f) );
        result = _mix(result, 66.0f, step(value, 573.5f) );
        result = _mix(result, 78.0f, step(value, 572.5f) );
        result = _mix(result, 64.0f, step(value, 571.5f) );
        result = _mix(result, 66.0f, step(value, 570.5f) );
        result = _mix(result, 60.0f, step(value, 569.5f) );
        result = _mix(result, 0.0f, step(value, 568.5f) );
        result = _mix(result, 120.0f, step(value, 550.5f) );
        result = _mix(result, 68.0f, step(value, 549.5f) );
        result = _mix(result, 66.0f, step(value, 548.5f) );
        result = _mix(result, 68.0f, step(value, 546.5f) );
        result = _mix(result, 120.0f, step(value, 545.5f) );
        result = _mix(result, 0.0f, step(value, 544.5f) );
        result = _mix(result, 66.0f, step(value, 526.5f) );
        result = _mix(result, 126.0f, step(value, 524.5f) );
        result = _mix(result, 66.0f, step(value, 523.5f) );
        result = _mix(result, 60.0f, step(value, 521.5f) );
        result = _mix(result, 0.0f, step(value, 520.5f) );
        result = _mix(result, 16.0f, step(value, 470.5f) );
        result = _mix(result, 0.0f, step(value, 469.5f) );
        result = _mix(result, 16.0f, step(value, 467.5f) );
        result = _mix(result, 0.0f, step(value, 466.5f) );
        return result;   
}
 
__DEVICE__ float GetBit( float fByte, float fBit )
{
        return mod_f(_floor(fByte / _powf(2.0f, 7.0f-fBit)), 2.0f) ;
}
 
__DEVICE__ float GetCharPixel( float fChar, float2 vPos )
{
        float fCharAddress = fChar * 8.0f + vPos.y;
       
        float fCharBin = GetCharByte(fCharAddress);
       
        return GetBit(fCharBin, vPos.x);
}
 
__DEVICE__ float GetProgramStringChar(float fPos)
{
        float fChar = 32.0f;    
        fChar = _mix(fChar, 76.0f, step(fPos, 12.5f) );
        fChar = _mix(fChar, 83.0f, step(fPos, 11.5f) );
        fChar = _mix(fChar, 76.0f, step(fPos, 10.5f) );
        fChar = _mix(fChar, 71.0f, step(fPos, 9.5f) );
        fChar = _mix(fChar, 32.0f, step(fPos, 8.5f) );
        fChar = _mix(fChar, 58.0f, step(fPos, 7.5f) );
        fChar = _mix(fChar, 109.0f, step(fPos, 6.5f) );
        fChar = _mix(fChar, 97.0f, step(fPos, 5.5f) );
        fChar = _mix(fChar, 114.0f, step(fPos, 4.5f) );
        fChar = _mix(fChar, 103.0f, step(fPos, 3.5f) );
        fChar = _mix(fChar, 111.0f, step(fPos, 2.5f) );
        fChar = _mix(fChar, 114.0f, step(fPos, 1.5f) );
        fChar = _mix(fChar, 80.0f, step(fPos, 0.5f) );
        return fChar;
}
 
__DEVICE__ float GetLoadingStringChar(float fPos)
{
        float fChar = 32.0f;    
        fChar = _mix(fChar, 76.0f, step(fPos, 11.0f) );
        fChar = _mix(fChar, 83.0f, step(fPos, 10.5f) );
        fChar = _mix(fChar, 76.0f, step(fPos, 9.5f) );
        fChar = _mix(fChar, 71.0f, step(fPos, 8.5f) );
        fChar = _mix(fChar, 32.0f, step(fPos, 7.5f) );
        fChar = _mix(fChar, 71.0f, step(fPos, 6.5f) );
        fChar = _mix(fChar, 78.0f, step(fPos, 5.5f) );
        fChar = _mix(fChar, 73.0f, step(fPos, 4.5f) );
        fChar = _mix(fChar, 68.0f, step(fPos, 3.5f) );
        fChar = _mix(fChar, 65.0f, step(fPos, 2.5f) );
        fChar = _mix(fChar, 79.0f, step(fPos, 1.5f) );
        fChar = _mix(fChar, 76.0f, step(fPos, 0.5f) );
        return fChar;
}
 
__DEVICE__ float GetProgramText(float2 vPixelPos)
{     
        float2 vCharCoord = _floor(vPixelPos / 8.0f);
       
        float fChar = GetProgramStringChar(vCharCoord.x);
       
        if(vCharCoord.y != 0.0f)
                fChar = 32.0f;
       
        return GetCharPixel(fChar, mod_f(vPixelPos, 8.0f));
}
 
__DEVICE__ float GetLoadingText(float2 vPixelPos)
{     
        float2 vCharCoord = _floor(vPixelPos / 8.0f);
       
        float fChar = GetLoadingStringChar(vCharCoord.x);
       
        float inString = 1.0f;
        if(vCharCoord.x < 0.0f)
                fChar = 32.0f;
       
        if(vCharCoord.y != 0.0f)
                fChar = 32.0f;
       
        return GetCharPixel(fChar, mod_f(vPixelPos, 8.0f));
}
 
__DEVICE__ float GetScreenPixel(float2 vScreenPixel)
{
  // plasma thing
  float f = _sinf(vScreenPixel.x *0.0432f + _sinf(vScreenPixel.y * 0.0423f)+ iTime * 3.0f);
  f = f + _sinf(vScreenPixel.y * 0.0454513f + _sinf(vScreenPixel.x * 0.07213f) + iTime * 5.0f);
  f = f + _sinf(vScreenPixel.x * 0.043353f + _sinf(vScreenPixel.y * 0.043413f) + iTime * 8.0f);
  f = f + _sinf(vScreenPixel.y * 0.0443513f + _sinf(vScreenPixel.x * 0.036313f) + iTime * 10.0f);
  f = f * 0.125f + 0.5f;
  
  return CrossHatch(f, vScreenPixel);
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  fragCoord+=0.5f;
           
  float fSequenceTime = iTime;
  
  #ifdef LOADING_LOOP
  fSequenceTime = mod_f(fSequenceTime, vTimeTotal.y);
  #endif
  
  float3 col = to_float3_s(1.0f);
  
  float4 vPhase = GetPhase(fSequenceTime);
  
  float2 vUV = ( fragCoord / iResolution );
  float2 vScreenUV = (vUV - 0.1f) / 0.8f;
  if(IsBorder(vScreenUV) > 0.0f)
  {
    col = GetBorderColour(vPhase.x, fragCoord, iResolution, iTime);
  }
  else
  {
    float2 vScreenCoord = GetScreenPixelCoord(vScreenUV);
    float2 vAttribCoord = _floor(vScreenCoord / 8.0f);

    float fPixelValue = 0.0f;
    float3 vInk = to_float3_s(0.0f);
    float3 vPaper = to_float3_s(1.0f);
    
    if(vPhase.x != kPhaseRunning)
    {
      // loading
      float fLoadScreenTime = fSequenceTime - vTimeHeader4.y;
                           
      float fAddressLoaded = fLoadScreenTime * 192.0f;
      if(PixelAddress(vScreenCoord) > fAddressLoaded)
      {
        if(vPhase.y < 4.0f)
        {
          col = to_float3_s(1.0f);
        }
        else
        if(vPhase.y < 8.0f)
        {
          float2 vTextPos = to_float2(0.0f, 8.0f);
          fPixelValue = GetProgramText(vScreenCoord - vTextPos);
        }
        else
        {
          float2 vTextPos = to_float2(10.0f * 8.0f, 19.0f * 8.0f);
          fPixelValue = GetLoadingText(vScreenCoord - vTextPos);
        }        
      }
      else
      {
        // loading screen
        fPixelValue = GetLoadingScreenPixel(vScreenCoord);
                            
      }
    
      if(AttributeAddress(vAttribCoord) < fAddressLoaded)
      {
        vInk = to_float3(0.0f, 0.0f, 1.0f);
        vPaper = to_float3(1.0f, 1.0f, 0.0f);
      }    
    }
    else
    {
      // running
      fPixelValue = GetScreenPixel(vScreenCoord);
      
      float2 vTextPos = to_float2(-8.0f * 8.0f, 8.0f);
      float fAttribValue = GetLoadingText(vAttribCoord - vTextPos );
      vPaper = _mix(to_float3(0.0f, 1.0f, 1.0f), to_float3(1.0f, 0.0f, 0.0f), fAttribValue);
      vInk = to_float3(0.0f, 0.0f, 1.0f);
    }     
  
    //fPixelValue = GetScreenPixel(vScreenCoord); // force final effect      
    //fPixelValue = GetLoadingScreenPixel( vScreenCoord); // force loading screen
  
    col = _mix(vPaper, vInk, fPixelValue);
    
  }

  float kBrightness = 0.8f;
  fragColor = to_float4_aw( col * kBrightness, 1.0f );  
  SetFragmentShaderComputedColor(fragColor);
  
}

#endif



#ifdef C64
highp float4 font2(int c) {
  float4 v = to_float4(0);
  v=_mix(v, to_float4(0x3c66, 0x6e6e, 0x6062, 0x3c00), step(-0.500f, float(c)));
  v=_mix(v, to_float4(0x183c, 0x667e, 0x6666, 0x6600), step(0.500f, float(c)));
  v=_mix(v, to_float4(0x7c66, 0x667c, 0x6666, 0x7c00), step(1.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x6060, 0x6066, 0x3c00), step(2.500f, float(c)));
  v=_mix(v, to_float4(0x786c, 0x6666, 0x666c, 0x7800), step(3.500f, float(c)));
  v=_mix(v, to_float4(0x7e60, 0x6078, 0x6060, 0x7e00), step(4.500f, float(c)));
  v=_mix(v, to_float4(0x7e60, 0x6078, 0x6060, 0x6000), step(5.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x606e, 0x6666, 0x3c00), step(6.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x667e, 0x6666, 0x6600), step(7.500f, float(c)));
  v=_mix(v, to_float4(0x3c18, 0x1818, 0x1818, 0x3c00), step(8.500f, float(c)));
  v=_mix(v, to_float4(0x1e0c, 0xc0c, 0xc6c, 0x3800), step(9.500f, float(c)));
  v=_mix(v, to_float4(0x666c, 0x7870, 0x786c, 0x6600), step(10.500f, float(c)));
  v=_mix(v, to_float4(0x6060, 0x6060, 0x6060, 0x7e00), step(11.500f, float(c)));
  v=_mix(v, to_float4(0x6377, 0x7f6b, 0x6363, 0x6300), step(12.500f, float(c)));
  v=_mix(v, to_float4(0x6676, 0x7e7e, 0x6e66, 0x6600), step(13.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x6666, 0x6666, 0x3c00), step(14.500f, float(c)));
  v=_mix(v, to_float4(0x7c66, 0x667c, 0x6060, 0x6000), step(15.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x6666, 0x663c, 0xe00), step(16.500f, float(c)));
  v=_mix(v, to_float4(0x7c66, 0x667c, 0x786c, 0x6600), step(17.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x603c, 0x666, 0x3c00), step(18.500f, float(c)));
  v=_mix(v, to_float4(0x7e18, 0x1818, 0x1818, 0x1800), step(19.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x6666, 0x6666, 0x3c00), step(20.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x6666, 0x663c, 0x1800), step(21.500f, float(c)));
  v=_mix(v, to_float4(0x6363, 0x636b, 0x7f77, 0x6300), step(22.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x3c18, 0x3c66, 0x6600), step(23.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x663c, 0x1818, 0x1800), step(24.500f, float(c)));
  v=_mix(v, to_float4(0x7e06, 0xc18, 0x3060, 0x7e00), step(25.500f, float(c)));
  v=_mix(v, to_float4(0x3c30, 0x3030, 0x3030, 0x3c00), step(26.500f, float(c)));
  v=_mix(v, to_float4(0xc12, 0x307c, 0x3062, 0xfc00), step(27.500f, float(c)));
  v=_mix(v, to_float4(0x3c0c, 0xc0c, 0xc0c, 0x3c00), step(28.500f, float(c)));
  v=_mix(v, to_float4(0x18, 0x3c7e, 0x1818, 0x1818), step(29.500f, float(c)));
  v=_mix(v, to_float4(0x10, 0x307f, 0x7f30, 0x1000), step(30.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x0, 0x0), step(31.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x1818, 0x0, 0x1800), step(32.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0x6600, 0x0, 0x0), step(33.500f, float(c)));
  v=_mix(v, to_float4(0x6666, 0xff66, 0xff66, 0x6600), step(34.500f, float(c)));
  v=_mix(v, to_float4(0x183e, 0x603c, 0x67c, 0x1800), step(35.500f, float(c)));
  v=_mix(v, to_float4(0x6266, 0xc18, 0x3066, 0x4600), step(36.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x3c38, 0x6766, 0x3f00), step(37.500f, float(c)));
  v=_mix(v, to_float4(0x60c, 0x1800, 0x0, 0x0), step(38.500f, float(c)));
  v=_mix(v, to_float4(0xc18, 0x3030, 0x3018, 0xc00), step(39.500f, float(c)));
  v=_mix(v, to_float4(0x3018, 0xc0c, 0xc18, 0x3000), step(40.500f, float(c)));
  v=_mix(v, to_float4(0x66, 0x3cff, 0x3c66, 0x0), step(41.500f, float(c)));
  v=_mix(v, to_float4(0x18, 0x187e, 0x1818, 0x0), step(42.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x18, 0x1830), step(43.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x7e, 0x0, 0x0), step(44.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x18, 0x1800), step(45.500f, float(c)));
  v=_mix(v, to_float4(0x3, 0x60c, 0x1830, 0x6000), step(46.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x6e76, 0x6666, 0x3c00), step(47.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x3818, 0x1818, 0x7e00), step(48.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x60c, 0x3060, 0x7e00), step(49.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x61c, 0x666, 0x3c00), step(50.500f, float(c)));
  v=_mix(v, to_float4(0x60e, 0x1e66, 0x7f06, 0x600), step(51.500f, float(c)));
  v=_mix(v, to_float4(0x7e60, 0x7c06, 0x666, 0x3c00), step(52.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x607c, 0x6666, 0x3c00), step(53.500f, float(c)));
  v=_mix(v, to_float4(0x7e66, 0xc18, 0x1818, 0x1800), step(54.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x663c, 0x6666, 0x3c00), step(55.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x663e, 0x666, 0x3c00), step(56.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x1800, 0x18, 0x0), step(57.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x1800, 0x18, 0x1830), step(58.500f, float(c)));
  v=_mix(v, to_float4(0xe18, 0x3060, 0x3018, 0xe00), step(59.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x7e00, 0x7e00, 0x0), step(60.500f, float(c)));
  v=_mix(v, to_float4(0x7018, 0xc06, 0xc18, 0x7000), step(61.500f, float(c)));
  v=_mix(v, to_float4(0x3c66, 0x60c, 0x1800, 0x1800), step(62.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xff, 0xff00, 0x0), step(63.500f, float(c)));
  v=_mix(v, to_float4(0x81c, 0x3e7f, 0x7f1c, 0x3e00), step(64.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x1818, 0x1818, 0x1818), step(65.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xff, 0xff00, 0x0), step(66.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xffff, 0x0, 0x0), step(67.500f, float(c)));
  v=_mix(v, to_float4(0xff, 0xff00, 0x0, 0x0), step(68.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xffff, 0x0), step(69.500f, float(c)));
  v=_mix(v, to_float4(0x3030, 0x3030, 0x3030, 0x3030), step(70.500f, float(c)));
  v=_mix(v, to_float4(0xc0c, 0xc0c, 0xc0c, 0xc0c), step(71.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xe0, 0xf038, 0x1818), step(72.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x1c0f, 0x700, 0x0), step(73.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x38f0, 0xe000, 0x0), step(74.500f, float(c)));
  v=_mix(v, to_float4(0xc0c0, 0xc0c0, 0xc0c0, 0xffff), step(75.500f, float(c)));
  v=_mix(v, to_float4(0xc0e0, 0x7038, 0x1c0e, 0x703), step(76.500f, float(c)));
  v=_mix(v, to_float4(0x307, 0xe1c, 0x3870, 0xe0c0), step(77.500f, float(c)));
  v=_mix(v, to_float4(0xffff, 0xc0c0, 0xc0c0, 0xc0c0), step(78.500f, float(c)));
  v=_mix(v, to_float4(0xffff, 0x303, 0x303, 0x303), step(79.500f, float(c)));
  v=_mix(v, to_float4(0x3c, 0x7e7e, 0x7e7e, 0x3c00), step(80.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xff, 0xff00), step(81.500f, float(c)));
  v=_mix(v, to_float4(0x367f, 0x7f7f, 0x3e1c, 0x800), step(82.500f, float(c)));
  v=_mix(v, to_float4(0x6060, 0x6060, 0x6060, 0x6060), step(83.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x7, 0xf1c, 0x1818), step(84.500f, float(c)));
  v=_mix(v, to_float4(0xc3e7, 0x7e3c, 0x3c7e, 0xe7c3), step(85.500f, float(c)));
  v=_mix(v, to_float4(0x3c, 0x7e66, 0x667e, 0x3c00), step(86.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x6666, 0x1818, 0x3c00), step(87.500f, float(c)));
  v=_mix(v, to_float4(0x606, 0x606, 0x606, 0x606), step(88.500f, float(c)));
  v=_mix(v, to_float4(0x81c, 0x3e7f, 0x3e1c, 0x800), step(89.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x18ff, 0xff18, 0x1818), step(90.500f, float(c)));
  v=_mix(v, to_float4(0xc0c0, 0x3030, 0xc0c0, 0x3030), step(91.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x1818, 0x1818, 0x1818), step(92.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x33e, 0x7636, 0x3600), step(93.500f, float(c)));
  v=_mix(v, to_float4(0xff7f, 0x3f1f, 0xf07, 0x301), step(94.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x0, 0x0), step(95.500f, float(c)));
  v=_mix(v, to_float4(0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0), step(96.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xffff, 0xffff), step(97.500f, float(c)));
  v=_mix(v, to_float4(0xff00, 0x0, 0x0, 0x0), step(98.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x0, 0xff), step(99.500f, float(c)));
  v=_mix(v, to_float4(0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0), step(100.500f, float(c)));
  v=_mix(v, to_float4(0xcccc, 0x3333, 0xcccc, 0x3333), step(101.500f, float(c)));
  v=_mix(v, to_float4(0x303, 0x303, 0x303, 0x303), step(102.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xcccc, 0x3333), step(103.500f, float(c)));
  v=_mix(v, to_float4(0xfffe, 0xfcf8, 0xf0e0, 0xc080), step(104.500f, float(c)));
  v=_mix(v, to_float4(0x303, 0x303, 0x303, 0x303), step(105.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x181f, 0x1f18, 0x1818), step(106.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xf0f, 0xf0f), step(107.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x181f, 0x1f00, 0x0), step(108.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xf8, 0xf818, 0x1818), step(109.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0x0, 0xffff), step(110.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x1f, 0x1f18, 0x1818), step(111.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x18ff, 0xff00, 0x0), step(112.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0xff, 0xff18, 0x1818), step(113.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x18f8, 0xf818, 0x1818), step(114.500f, float(c)));
  v=_mix(v, to_float4(0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0), step(115.500f, float(c)));
  v=_mix(v, to_float4(0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0), step(116.500f, float(c)));
  v=_mix(v, to_float4(0x707, 0x707, 0x707, 0x707), step(117.500f, float(c)));
  v=_mix(v, to_float4(0xffff, 0x0, 0x0, 0x0), step(118.500f, float(c)));
  v=_mix(v, to_float4(0xffff, 0xff00, 0x0, 0x0), step(119.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xff, 0xffff), step(120.500f, float(c)));
  v=_mix(v, to_float4(0x303, 0x303, 0x303, 0xffff), step(121.500f, float(c)));
  v=_mix(v, to_float4(0x0, 0x0, 0xf0f0, 0xf0f0), step(122.500f, float(c)));
  v=_mix(v, to_float4(0xf0f, 0xf0f, 0x0, 0x0), step(123.500f, float(c)));
  v=_mix(v, to_float4(0x1818, 0x18f8, 0xf800, 0x0), step(124.500f, float(c)));
  v=_mix(v, to_float4(0xf0f0, 0xf0f0, 0x0, 0x0), step(125.500f, float(c)));
  v=_mix(v, to_float4(0xf0f0, 0xf0f0, 0xf0f, 0xf0f), step(126.500f, float(c)));
  return v;
}

highp float4 font(int c) {
    if (c < 128) return font2(c);
    return to_float4_s(0xffff) - font2(c - 128);
}

__DEVICE__ float4 colors(int c) {
    if (c ==  0) return to_float4(0x00,0x00,0x00,1);
    if (c ==  1) return to_float4(0xFF,0xFF,0xFF,1);
    if (c ==  2) return to_float4(0x68,0x37,0x2B,1);
    if (c ==  3) return to_float4(0x70,0xA4,0xB2,1);
    if (c ==  4) return to_float4(0x6F,0x3D,0x86,1);
    if (c ==  5) return to_float4(0x58,0x8D,0x43,1);
    if (c ==  6) return to_float4(0x35,0x28,0x79,1);
    if (c ==  7) return to_float4(0xB8,0xC7,0x6F,1);
    if (c ==  8) return to_float4(0x6F,0x4F,0x25,1);
    if (c ==  9) return to_float4(0x43,0x39,0x00,1);
    if (c == 10) return to_float4(0x9A,0x67,0x59,1);
    if (c == 11) return to_float4(0x44,0x44,0x44,1);
    if (c == 12) return to_float4(0x6C,0x6C,0x6C,1);
    if (c == 13) return to_float4(0x9A,0xD2,0x84,1);
    if (c == 14) return to_float4(0x6C,0x5E,0xB5,1);
    if (c == 15) return to_float4(0x95,0x95,0x95,1);
    return to_float4_s(0);
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    
    fragCoord+=0.5f;
    
    float2 uv = fragCoord / iResolution;
    uv = uv * 1.1f - 0.05f;
    if ( any( lessThan( uv, to_float2(0) ) ) || any( greaterThanEqual( uv, to_float2(1) ) ) )
    {
      fragColor = colors(14) / 180.0f;
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    float2 sz = to_float2(40.0f, 20.0f);
    float2 fb_pos = _floor(uv * sz) + to_float2(0.5f, 0.5f);
    fb_pos.y = sz.y - fb_pos.y;
    fb_pos /= iResolution;
    
    float4 fb = _tex2DVecN(iChannel0,fb_pos.x,fb_pos.y,15);
    highp float4 _char = font((int)(fb.x));

    float2 p = mod_f(uv * sz * 8.0f, 8.0f);
    int line = 7 - int(p.y);
    highp float pixels = 0.0f;
    if (line == 0) pixels = _char.x / 256.0f;
    if (line == 1) pixels = _char.x;
    if (line == 2) pixels = _char.y / 256.0f;
    if (line == 3) pixels = _char.y;
    if (line == 4) pixels = _char.z / 256.0f;
    if (line == 5) pixels = _char.z;
    if (line == 6) pixels = _char.w / 256.0f;
    if (line == 7) pixels = _char.w;

    if (mod_f(pixels * _powf(2.0f, _floor(p.x)), 256.0f) > 127.5f) {
        fragColor = colors((int)(fb.y)) / 180.0f;
    } else {
        fragColor = colors((int)(fb.z)) / 180.0f;
    }
    
    SetFragmentShaderComputedColor(fragColor);
}
#endif





#ifdef MANDELBROT

///////////////////////////
// Keyboard
///////////////////////////

const float KEY_SPACE = 32.5f/256.0f;
const float KEY_LEFT  = 37.5f/256.0f;
const float KEY_UP    = 38.5f/256.0f;
const float KEY_RIGHT = 39.5f/256.0f;
const float KEY_DOWN  = 40.5f/256.0f;

const float KEY_PLUS   = 187.5f/256.0f;
const float KEY_MINUS    = 189.5f/256.0f;

__DEVICE__ bool Key_IsPressed(float key)
{
    return texture( iChannel1, to_float2(key, 0.0f) ).x > 0.0f;
}

__DEVICE__ bool Key_IsToggled(float key)
{
    return texture( iChannel1, to_float2(key, 1.0f) ).x > 0.0f;
}

///////////////////////////


__DEVICE__ float VGARainbowChannel( float i, float a, float b, float c, float d, float e )
{    
    if ( i >= 8.0f ) i = 16.0f - i;
    if ( i <= 0.0f ) return a;
    if ( i == 1.0f ) return b;
    if ( i == 2.0f ) return c;
    if ( i == 3.0f ) return d;
    if ( i >= 4.0f ) return e;
    return a;
}

__DEVICE__ float3 VGARainbow( float i, float a, float e )
{
    float3 vi = mod_f( to_float3( i ) + to_float3(0,16,8), to_float3(24) );

    float b = _floor(a * 3.0f/4.0f + e * 1.0f / 4.0f + 0.25f);
    float c = _floor(a * 2.0f/4.0f + e * 2.0f / 4.0f + 0.25f);
    float d = _floor(a * 1.0f/4.0f + e * 3.0f / 4.0f + 0.25f);
    
    float3 col;
    col.x = VGARainbowChannel( vi.x, a, b, c, d, e );
    col.y = VGARainbowChannel( vi.y, a, b, c, d, e );
    col.z = VGARainbowChannel( vi.z, a, b, c, d, e );

    return col;
}

__DEVICE__ float3 VGAPaletteEntry( float i )
{
    i = _floor( i );
    
    // EGA
    if ( i < 16.0f )
    {
        float3 col;
        col.z  = _floor( mod_f( i / 1.0f, 2.0f  )) * 2.0f;
        col.y  = _floor( mod_f( i / 2.0f, 2.0f  )) * 2.0f;
        col.x  = _floor( mod_f( i / 4.0f, 2.0f  )) * 2.0f;        
        
        col += _floor( mod_f( i / 8.0f, 2.0f  ) );
        
        if ( i == 6.0f ) col = to_float3(2,1,0); // Special brown!

        return col * 21.0f;
    }

    // Greys
    if ( i == 16.0f ) return to_float3_s(0.0f);
    
    if ( i < 32.0f )
    {        
        float x = (i - 17.0f);        
        return to_float3_aw( _floor( 0.00084f * x * x * x * x - 0.01662f * x * x * x + 0.1859f * x * x + 2.453f * x + 5.6038f ) );
    }
    
    // Rainbows
    float rainbowIndex = mod_f( i - 32.0f, 24.0f );
    float rainbowType = _floor( (i - 32.0f) / 24.0f );
    
    float rainbowTypeMod = _floor( mod_f( rainbowType, 3.0f ) );
    float rainbowTypeDiv = _floor( rainbowType / 3.0f );
    
    float rainbowLow = 0.0f;
    if ( rainbowTypeMod == 1.0f ) rainbowLow = 31.0f;
    if ( rainbowTypeMod == 2.0f ) rainbowLow = 45.0f;
    
    float rainbowHigh = 63.0f;
    if ( rainbowTypeDiv == 1.0f )
    {
        rainbowHigh = 28.0f;
        rainbowLow = _floor( rainbowLow / 2.2f );
    }
    if ( rainbowTypeDiv == 2.0f )
    {
        rainbowHigh = 16.0f;
        rainbowLow = _floor( rainbowLow / 3.8f );
    }
    
    if ( rainbowType < 9.0f )
    {
      return VGARainbow( rainbowIndex, rainbowLow, rainbowHigh );
    }
    
    return to_float3_s( 0.0f );
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;

    float2 vUV = fragCoord / iResolution;
    
    float2 vFakeResolution = to_float2(640,480);
    vUV = _floor(vUV * vFakeResolution) / vFakeResolution;
    
    float2 vFocus = to_float2(-0.5f, 0.0f);
    float2 vScale = to_float2_s(2.0f);
    
    if ( false )  // disable mouse control
    if ( iMouse.z > 0.0f )
    {
      vFocus += 2.0f * ((swi2(iMouse,x,y) / iResolution) * 2.0f - 1.0f);
      vScale *= 0.02f;
    }
    
    vScale.y /= iResolution.x / iResolution.y;
    
    float2 z = to_float2(0);
    float2 c = vFocus + (vUV * 2.0f - 1.0f) * vScale;
    
    bool bInside = true;
    
    float fIter = 0.0f;
    for(int iter = 0; iter < 512; iter++)
    {        
     z = mat2(z,-z.y,z.x) * z + c;
     
        if ( dot(z,z) > 4.0f )            
        {
            bInside = false;
            break;
        }       
        
        fIter++;
    }
    
    float fIndex = 0.0f;
    if ( bInside ) 
    {
        //fIndex = 0.0f; // black set
        fIndex = 1.0f; // blue set
    }
    else
    {
 
        if ( Key_IsToggled( KEY_PLUS ) || Key_IsToggled( KEY_RIGHT ) )
        {
          fIter += iTime * 10.0f;
        }
        else
        if ( Key_IsToggled( KEY_MINUS ) || Key_IsToggled( KEY_LEFT ) )
        {
          fIter -= iTime * 10.0f;
        }
        
      fIndex = 1.0f + mod_f( fIter, 255.0f );
    }
    
    //swi3(fragColor,x,y,z) = VGAPaletteEntry( fIndex ) / 63.0f;
    //fragColor.w = 1.0f;
    
    fragColor = to_float4_aw(VGAPaletteEntry( fIndex ) / 63.0f, 1.0f);
    SetFragmentShaderComputedColor(fragColor);
}

#endif





#ifdef SHADERTOY
// Shadertoy font shader - @P_Malin

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// ----------------------------------------------------------------------------------------

//#define LOW_QUALITY

// The main characters are made up from a number of curve segments.
// I made another shader to illustrate how these work:
//
//     https://www.shadertoy.com/view/Xds3Dn
//
// The middle of the characters are filled in triangles or convex quadrilaterals
// Enable this define to see just the curved sections:

//#define CURVES_ONLY

// Initially I made most of characters this way but I ran into the constant register limit. 
// To avoid this, the curved sections of the â€˜oâ€™, â€˜aâ€™ and â€˜dâ€™ are oval shapes. 
// Also I managed to cut the constant data down dramatically by sharing a lot of
// the shapes in the font (see the comments in the function Shadertoy() ). 
// For example the tails for â€˜hâ€™, â€™aâ€™, â€˜dâ€™, â€˜tâ€™, the left hand side of the â€˜yâ€™ and the 
// top of the â€˜hâ€™ all use the same shape! 
// I was probably more happy that I should have been when I realised I could share
// the shape making the curve of the â€˜râ€™ with the little loop on the â€˜oâ€™.
//
// I experimented with a distance field version but it looked like it would involve 
// a lot more work and I thought Iâ€™d already spent too much time on this shader :)

#ifdef LOW_QUALITY

  #define AA_X 1
  #define AA_Y 1

#else

  #define AA_X 2
  #define AA_Y 2

#endif


__DEVICE__ float TestCurve(float2 uv)
{
  uv = 1.0f - uv;
    return 1.0f - dot(uv, uv);
}

__DEVICE__ float Cross( const in float2 A, const in float2 B )
{
    return A.x * B.y - A.y * B.x;
}

__DEVICE__ float2 GetUV(const in float2 A, const in float2 B, const in float2 C, const in float2 P)
{
    float2 vPB = B - P;
    float f1 = Cross(A-B, vPB);
    float f2 = Cross(B-C, vPB);
    float f3 = Cross(C-A, C-P);
    
    return to_float2(f1, f2) / (f1 + f2 + f3);
}

__DEVICE__ float InCurve( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
    float2 vCurveUV = GetUV(A, B, C, P);
    
    float fResult = -1.0f;

  fResult = _fmaxf(fResult, (-vCurveUV.x));
  fResult = _fmaxf(fResult, (-vCurveUV.y));
  fResult = _fmaxf(fResult, (vCurveUV.x + vCurveUV.y - 1.0f));

  float fCurveResult = TestCurve(vCurveUV);
    
  fResult = _fmaxf(fResult, fCurveResult);  
  
    return fResult;
}

__DEVICE__ float InCurve2( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
    float2 vCurveUV = GetUV(A, B, C, P);
  
    float fResult = -1.0f;

  fResult = _fmaxf(fResult, (vCurveUV.x + vCurveUV.y - 1.0f));
  
  float fCurveResult = -TestCurve(vCurveUV);
  
  fResult = _fmaxf(fResult, fCurveResult);  
  
    return fResult;
}

__DEVICE__ float InTri( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
  #ifdef CURVES_ONLY
  return 1.0f;
  #endif
  
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(A-C, C-P);
  
    return (_fmaxf(max(f1, f2), f3));
}

__DEVICE__ float InQuad( const in float2 A, const in float2 B, const in float2 C, const in float2 D, const in float2 P )
{
  #ifdef CURVES_ONLY
  return 1.0f;
  #endif
  
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(D-C, C-P);
    float f4 = Cross(A-D, D-P);
    
    return (_fmaxf(max(_fmaxf(f1, f2), f3), f4));
}


__DEVICE__ float Glyph0(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.112f, 0.056f );
    const float2  vP1 = to_float2 ( 0.136f, 0.026f );
    const float2  vP2 = to_float2 ( 0.108f, 0.022f );
    const float2  vP3 = to_float2 ( 0.083f, 0.017f ); 
    const float2  vP4 = to_float2 ( 0.082f, 0.036f ); 
    const float2  vP5 = to_float2 ( 0.088f, 0.062f ); 
    const float2  vP6 = to_float2 ( 0.115f, 0.086f ); 
    const float2  vP7 = to_float2 ( 0.172f, 0.147f ); 
    const float2  vP8 = to_float2 ( 0.100f, 0.184f ); 
    const float2  vP9 = to_float2 ( 0.034f, 0.206f ); 
    const float2 vP10 = to_float2 ( 0.021f, 0.160f ); 
    const float2 vP11 = to_float2 ( 0.011f, 0.114f ); 
    const float2 vP12 = to_float2 ( 0.052f, 0.112f ); 
    const float2 vP13 = to_float2 ( 0.070f, 0.108f ); 
    const float2 vP14 = to_float2 ( 0.075f, 0.126f );
    const float2 vP15 = to_float2 ( 0.049f, 0.124f );
    const float2 vP16 = to_float2 ( 0.047f, 0.148f );
    const float2 vP17 = to_float2 ( 0.046f, 0.169f );
    const float2 vP18 = to_float2 ( 0.071f, 0.171f );
    const float2 vP19 = to_float2 ( 0.098f, 0.171f ); 
    const float2 vP20 = to_float2 ( 0.097f, 0.143f ); 
    const float2 vP21 = to_float2 ( 0.100f, 0.118f ); 
    const float2 vP22 = to_float2 ( 0.080f, 0.100f ); 
    const float2 vP23 = to_float2 ( 0.055f, 0.083f ); 
    const float2 vP24 = to_float2 ( 0.050f, 0.052f ); 
    const float2 vP25 = to_float2 ( 0.052f, 0.004f ); 
    const float2 vP26 = to_float2 ( 0.107f, 0.010f ); 
    const float2 vP27 = to_float2 ( 0.148f, 0.011f ); 
    const float2 vP28 = to_float2 ( 0.140f, 0.041f ); 
    const float2 vP29 = to_float2 ( 0.139f, 0.069f ); 

    float fDist = 1.0f;

  fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = _fminf( fDist, InCurve2(vP8,vP9,vP10, uv) );
  fDist = _fminf( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = _fminf( fDist, InCurve2(vP12,vP13,vP14, uv) );
  fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = _fminf( fDist, InCurve(vP16,vP17,vP18, uv) );
    fDist = _fminf( fDist, InCurve(vP18,vP19,vP20, uv) );
    fDist = _fminf( fDist, InCurve(vP20,vP21,vP22, uv) );
  fDist = _fminf( fDist, InCurve2(vP22,vP23,vP24, uv) );
    fDist = _fminf( fDist, InCurve2(vP24,vP25,vP26, uv) );
    fDist = _fminf( fDist, InCurve2(vP26,vP27,vP28, uv) );
    fDist = _fminf( fDist, InCurve2(vP28,vP29,vP0, uv) );
  fDist = _fminf( fDist, InCurve(vP0,vP1,vP2, uv) );
  fDist = _fminf( fDist, InCurve(vP2,vP3,vP4, uv) );
    fDist = _fminf( fDist, InCurve(vP4,vP5,vP6, uv) );


    fDist = _fminf( fDist, InTri(vP0, vP1, vP28, uv) );
  fDist = _fminf( fDist, InQuad(vP26, vP1, vP2, vP3, uv) );
    fDist = _fminf( fDist, InTri(vP3, vP4, vP24, uv) );
    fDist = _fminf( fDist, InTri(vP4, vP5, vP24, uv) );
    fDist = _fminf( fDist, InTri(vP24, vP5, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP5, vP6, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP22, vP6, vP21, uv) );
    fDist = _fminf( fDist, InTri(vP6, vP8, vP21, uv) );
    fDist = _fminf( fDist, InTri(vP21, vP8, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP20, vP8, vP19, uv) );
    fDist = _fminf( fDist, InTri(vP19, vP8, vP18, uv) );
    fDist = _fminf( fDist, InTri(vP18, vP8, vP10, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP16, vP17, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP15, vP16, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP12, vP16, uv) );
    fDist = _fminf( fDist, InTri(vP12, vP14, vP15, uv) );

    return fDist;
}

__DEVICE__ float Glyph1(const in float2 uv, const in float2 vOffset)
{
    float2 vP0 = to_float2 ( 0.171f, 0.026f ) + vOffset;
    float2 vP1 = to_float2 ( 0.204f, 0.022f ) + vOffset;
    const float2 vP2 = to_float2 ( 0.170f, 0.185f );
    const float2 vP3 = to_float2 ( 0.137f, 0.185f );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}

__DEVICE__ float Glyph3(const in float2 uv, float2 vOffset)
{
    float2 vP0 = to_float2 ( 0.212f, 0.112f ) + vOffset;
    float2 vP2 = to_float2 ( 0.243f, 0.112f ) + vOffset;
    const float2  vP4 = to_float2 ( 0.234f, 0.150f );
    const float2  vP5 = to_float2 ( 0.230f, 0.159f );
    const float2  vP6 = to_float2 ( 0.243f, 0.164f );
    const float2  vP7 = to_float2 ( 0.257f, 0.164f );
    const float2  vP8 = to_float2 ( 0.261f, 0.148f );
    const float2 vP10 = to_float2 ( 0.265f, 0.164f );
    const float2 vP11 = to_float2 ( 0.256f, 0.180f );
    const float2 vP12 = to_float2 ( 0.239f, 0.185f );
    const float2 vP13 = to_float2 ( 0.194f, 0.194f );
    const float2 vP14 = to_float2 ( 0.203f, 0.150f );
    const float2 vP16 = to_float2 ( 0.212f, 0.113f );

    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve(vP4,vP5,vP6, uv) );
    fDist = _fminf( fDist, InCurve(vP6,vP7,vP8, uv) );
    fDist = _fminf( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = _fminf( fDist, InCurve2(vP12,vP13,vP14, uv) );

    fDist = _fminf( fDist, InQuad(vP0, vP2, vP4, vP14, uv) );
    fDist = _fminf( fDist, InTri(vP14, vP4, vP5, uv) );
    fDist = _fminf( fDist, InTri(vP14, vP5, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP5, vP6, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP6, vP7, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP6, vP10, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP8, vP10, vP7, uv) );
    
    return fDist;
}

__DEVICE__ float Glyph4(const in float2 uv)
{
    float2 vP = uv - to_float2(0.305f, 0.125f);
    vP /= 0.065f;
    vP.x *= 1.5f;
    vP.x += vP.y * 0.25f;
    
    float2 vP2 = vP;

    vP.y = _fabs(vP.y);
    vP.y = _powf(vP.y, 1.2f);
    float f= length(vP);
    
    vP2.x *= 1.2f;
    float f2 = length(vP2 * 1.5f - to_float2(0.6f, 0.0f));
        
    return _fmaxf(f - 1.0f, 1.0f - f2) / 20.0f;
} 

__DEVICE__ float Glyph5(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.507f, 0.138f );
    const float2  vP1 = to_float2 ( 0.510f, 0.065f );
    const float2  vP2 = to_float2 ( 0.570f, 0.066f );
    const float2  vP3 = to_float2 ( 0.598f, 0.066f );
    const float2  vP4 = to_float2 ( 0.594f, 0.092f );
    const float2  vP5 = to_float2 ( 0.599f, 0.131f );
    const float2  vP6 = to_float2 ( 0.537f, 0.137f );
    const float2  vP8 = to_float2 ( 0.538f, 0.125f );
    const float2  vP9 = to_float2 ( 0.564f, 0.129f );
    const float2 vP10 = to_float2 ( 0.574f, 0.100f );
    const float2 vP11 = to_float2 ( 0.584f, 0.085f );
    const float2 vP12 = to_float2 ( 0.571f, 0.079f );
    const float2 vP13 = to_float2 ( 0.557f, 0.081f );
    const float2 vP14 = to_float2 ( 0.549f, 0.103f );
    const float2 vP15 = to_float2 ( 0.518f, 0.166f );
    const float2 vP16 = to_float2 ( 0.557f, 0.166f );
    const float2 vP17 = to_float2 ( 0.589f, 0.163f );
    const float2 vP18 = to_float2 ( 0.602f, 0.137f );
    const float2 vP20 = to_float2 ( 0.602f, 0.152f );
    const float2 vP21 = to_float2 ( 0.572f, 0.194f );
    const float2 vP22 = to_float2 ( 0.537f, 0.185f );
    const float2 vP23 = to_float2 ( 0.503f, 0.189f );
    
    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = _fminf( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = _fminf( fDist, InCurve(vP10,vP11,vP12, uv) ); 
    fDist = _fminf( fDist, InCurve(vP12,vP13,vP14, uv) );
    fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = _fminf( fDist, InCurve(vP16,vP17,vP18, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP20,vP21,vP22, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP22,vP23,vP0, uv) );

    fDist = _fminf( fDist, InTri(vP0, vP2, vP13, uv) );
    fDist = _fminf( fDist, InTri(vP13, vP2, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP2, vP11, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP2, vP4, vP11, uv) );
    fDist = _fminf( fDist, InTri(vP11, vP4, vP10, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP4, vP9, uv) );
    fDist = _fminf( fDist, InTri(vP6, vP8, vP9, uv) );
    fDist = _fminf( fDist, InTri(vP0, vP13, vP14, uv) );
    fDist = _fminf( fDist, InTri(vP0, vP14, vP15, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP16, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP16, vP17, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP17, vP18, vP20, uv) );
    
    return fDist;
}

__DEVICE__ float Glyph6(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.638f , 0.087f ); 
    const float2  vP1 = to_float2 ( 0.648f , 0.073f ); 
    const float2  vP2 = to_float2 ( 0.673f , 0.068f ); 
    const float2  vP3 = to_float2 ( 0.692f , 0.069f ); 
    const float2  vP4 = to_float2 ( 0.687f , 0.086f ); 
    const float2  vP5 = to_float2 ( 0.688f , 0.104f ); 
    const float2  vP6 = to_float2 ( 0.672f , 0.102f ); 
    const float2  vP7 = to_float2 ( 0.659f , 0.099f ); 
    const float2  vP8 = to_float2 ( 0.663f , 0.092f ); 
    const float2  vP9 = to_float2 ( 0.662f , 0.086f ); 
    const float2 vP10 = to_float2 ( 0.655f , 0.086f ); 
    const float2 vP11 = to_float2 ( 0.644f , 0.087f ); 
    const float2 vP12 = to_float2 ( 0.637f , 0.102f ); 
    const float2 vP13 = to_float2 ( 0.638f , 0.094f ); 

    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) ); 
    fDist = _fminf( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = _fminf( fDist, InCurve(vP10,vP11,vP12, uv) );

    fDist = _fminf( fDist, InQuad(vP2, vP4, vP6, vP8, uv) );
    fDist = _fminf( fDist, InTri(vP9, vP2, vP8, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP2, vP9, uv) );
    fDist = _fminf( fDist, InQuad(vP0, vP2, vP10, vP11, uv) );
    fDist = _fminf( fDist, InTri(vP11, vP12, vP0, uv) );
    
    return fDist;
}

__DEVICE__ float Glyph7(const in float2 uv)
{
    const float2 vP0 = to_float2 ( 0.693f , 0.068f );
    const float2 vP1 = to_float2 ( 0.748f , 0.069f );
    const float2 vP2 = to_float2 ( 0.747f , 0.078f );
    const float2 vP3 = to_float2 ( 0.691f , 0.077f );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}


__DEVICE__ float Glyph8(const in float2 uv)
{ 
    float2 vP = uv - to_float2(0.788f, 0.125f);
    vP /= 0.065f;
    vP.x *= 1.4f;
    vP.x += vP.y * 0.25f;
    
    float2 vP2 = vP;
    
    vP.y = _fabs(vP.y);
    vP.y = _powf(vP.y, 1.2f);
    float f= length(vP);
    
    vP2.x *= 1.5f;
    float f2 = length(vP2 * 1.5f - to_float2(0.3f, 0.0f));
    
    
    return _fmaxf(f - 1.0f, 1.0f - f2) / 20.0f;
}

__DEVICE__ float Glyph11(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.921f , 0.070f );
    const float2  vP2 = to_float2 ( 0.955f , 0.070f );
    const float2  vP4 = to_float2 ( 0.926f , 0.202f );
    const float2  vP5 = to_float2 ( 0.926f , 0.240f );
    const float2  vP6 = to_float2 ( 0.885f , 0.243f );
    const float2  vP7 = to_float2 ( 0.852f , 0.239f );
    const float2  vP8 = to_float2 ( 0.859f , 0.219f );
    const float2  vP9 = to_float2 ( 0.862f , 0.192f );
    const float2 vP10 = to_float2 ( 0.889f , 0.189f );
    const float2 vP12 = to_float2 ( 0.928f , 0.178f );
    const float2 vP13 = to_float2 ( 0.949f , 0.173f );
    const float2 vP14 = to_float2 ( 0.951f , 0.162f );
    const float2 vP15 = to_float2 ( 0.960f , 0.150f );
    const float2 vP16 = to_float2 ( 0.960f , 0.144f );
    const float2 vP18 = to_float2 ( 0.971f , 0.144f );
    const float2 vP19 = to_float2 ( 0.968f , 0.157f );
    const float2 vP20 = to_float2 ( 0.957f , 0.171f );
    const float2 vP21 = to_float2 ( 0.949f , 0.182f );
    const float2 vP22 = to_float2 ( 0.922f , 0.189f );
    const float2 vP24 = to_float2 ( 0.900f , 0.196f );
    const float2 vP25 = to_float2 ( 0.866f , 0.205f );
    const float2 vP26 = to_float2 ( 0.871f , 0.217f );
    const float2 vP27 = to_float2 ( 0.871f , 0.225f );
    const float2 vP28 = to_float2 ( 0.880f , 0.224f );
    const float2 vP29 = to_float2 ( 0.889f , 0.218f );
    const float2 vP30 = to_float2 ( 0.893f , 0.203f );

    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = _fminf( fDist, InCurve2(vP8,vP9,vP10, uv) );
    fDist = _fminf( fDist, InCurve(vP12,vP13,vP14, uv) );

    fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = _fminf( fDist, InCurve2(vP18,vP19,vP20, uv) );
    fDist = _fminf( fDist, InCurve2(vP20,vP21,vP22, uv) );

    fDist = _fminf( fDist, InCurve(vP24,vP25,vP26, uv) );
    fDist = _fminf( fDist, InCurve(vP26,vP27,vP28, uv) );
    fDist = _fminf( fDist, InCurve(vP28,vP29,vP30, uv) );
    
    fDist = _fminf( fDist, InQuad(vP0, vP2, vP4, vP30, uv) );

    fDist = _fminf( fDist, InQuad(vP10, vP12, vP22, vP24, uv) );
        
    fDist = _fminf( fDist, InTri(vP30, vP4, vP6, uv) );
    fDist = _fminf( fDist, InTri(vP30, vP6, vP29, uv) );
    fDist = _fminf( fDist, InTri(vP28, vP29, vP6, uv) );
    fDist = _fminf( fDist, InTri(vP28, vP6, vP27, uv) );
    
    fDist = _fminf( fDist, InTri(vP8, vP27, vP6, uv) );
    
    fDist = _fminf( fDist, InTri(vP8, vP26, vP27, uv) );
    fDist = _fminf( fDist, InTri(vP8, vP25, vP26, uv) );
    fDist = _fminf( fDist, InTri(vP25, vP10, vP24, uv) );
    
    fDist = _fminf( fDist, InTri(vP12, vP13, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP12, vP20, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP13, vP14, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP20, vP14, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP18, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP16, vP18, uv) );
    
    return fDist;
}

__DEVICE__ float Shadertoy(in float2 uv)
{
    float fResult = 1.0f;
    
    fResult = _fminf(fResult, Glyph0(uv)); // S

    float2 vUVOffset = to_float2(0.001f, 0.0f); // tail of h
    float2 vTailOffset = to_float2(0.0f, 0.0f);  
    float fUVScale = 1.0f;

    if(uv.x < 0.3f)
    {
        if(uv.y < 0.12f)
        {
            // top of h
            fUVScale = -1.0f;
            vUVOffset = to_float2(0.448f, 0.25f);  
            vTailOffset = to_float2(0.0f, 0.0f);   
        }
    }
    else if(uv.x < 0.4f)    
    {
        // tail of a
        vUVOffset = to_float2(-0.124f, 0.0f);  
        vTailOffset = to_float2(0.01f, -0.04f);    
    }
    else if(uv.x < 0.6f)
    {
        // tail of d
        vUVOffset = to_float2(-0.248f, 0.0f);  
        vTailOffset = to_float2(0.02f, -0.1f); 
    }
    else if(uv.x < 0.83f)
    {
        // stalk of t
        vUVOffset = to_float2(-0.48f, 0.0f);   
        vTailOffset = to_float2(0.02f, -0.1f); 
    }
    else
    {
        // start of y
        vUVOffset = to_float2(-0.645f, 0.0f);  
        vTailOffset = to_float2(0.005f, -0.042f);  
    }
    
    fResult = _fminf(fResult, Glyph3(uv * fUVScale + vUVOffset, vTailOffset)); // tails h, a, d, t, start of y and top of h


    float2 vUVOffset3 = to_float2(0.0f, 0.0f);   // vertical of h
    float2 vTailOffset3 = to_float2(0.0f, 0.0f);
    
    if(uv.x > 0.5f)
    {
        // vertical of r
        vUVOffset3 = to_float2(-0.45f, 0.0f);  
        vTailOffset3 = to_float2(-0.01f, 0.04f);   
    }
    
    fResult = _fminf(fResult, Glyph1(uv + vUVOffset3, vTailOffset3)); // vertical of h, r

    float2 vUVOffset2 = to_float2(0.0f, 0.0f); // curve of a
    if(uv.x > 0.365f)
    {
        vUVOffset2 = to_float2(-0.125f, 0.0f); // curve of d
    }

    fResult = _fminf(fResult, Glyph4(uv + vUVOffset2)); // curve of a, d
    
    fResult = _fminf(fResult, Glyph5(uv)); // e

    float2 vUVOffset4 = to_float2(0.001f, 0.0f); // top of r
    float2 vUVScale4 = to_float2(1.0f, 1.0f);        
    
    if(uv.x > 0.7f)
    {
        // o loop
        vUVOffset4.x = 1.499f;
        vUVOffset4.y = 0.19f;
        
        vUVScale4.x = -1.0f;
        vUVScale4.y = -1.0f;
    }
    
    fResult = _fminf(fResult, Glyph6(uv * vUVScale4 + vUVOffset4)); // top of r and o loop

    fResult = _fminf(fResult, Glyph7(uv)); // cross t    
    
    fResult = _fminf(fResult, Glyph8(uv)); // o1
    
    fResult = _fminf(fResult, Glyph11(uv)); // y2        

    return fResult; 
}

__DEVICE__ float2 GetUVCentre(const float2 vInputUV)
{
  float2 vFontUV = vInputUV;
    vFontUV.y -= 0.35f;
    
  return vFontUV;
}

__DEVICE__ float2 GetUVScroll(const float2 vInputUV, float t)
{
  float2 vFontUV = vInputUV;
  vFontUV *= 0.25f;
  
    vFontUV.y -= 0.005f;
  vFontUV.x += t * 3.0f - 1.5f;
  
  return vFontUV;
}

__DEVICE__ float2 GetUVRepeat(const float2 vInputUV, float t2)
{
  float2 vFontUV = vInputUV;
  
  vFontUV *= to_float2(1.0f, 4.0f);
  
  vFontUV.x += _floor(vFontUV.y) * t2;
  
  vFontUV = fract(vFontUV);
  
  vFontUV /= to_float2(1.0f, 4.0f);
    
  return vFontUV;
}

__DEVICE__ float2 GetUVRotate(const float2 vInputUV, float t)
{
  float2 vFontUV = vInputUV - 0.5f;
  
  float s = _sinf(t);
  float c = _cosf(t);
  
  vFontUV = to_float2(  vFontUV.x * c + vFontUV.y * s,
              -vFontUV.x * s + vFontUV.y * c );
  
  vFontUV += 0.5f;
  
  return vFontUV;
}

__DEVICE__ float3 StyleDefault( float f )
{
  return _mix(to_float3_s(0.25f), to_float3_s(1.0f), f);
}

__DEVICE__ float3 StyleScanline( float f, in float2 fragCoord )
{
  float fShade = f * 0.8f + 0.2f;
  
    // disable
  //fShade *= mod_f(fragCoord.y, 2.0f);
  
  return _mix(to_float3(0.01f, 0.2f, 0.01f), to_float3(0.01f, 1.0f, 0.02f), fShade);
}

__DEVICE__ float3 StyleStamp( float fFont, float2 uv )
{
  float3 t1 = texture(iChannel2, uv + 0.005f).rgb;
  float3 t2 = _tex2DVecN(iChannel2,uv.x,uv.y,15).rgb;
  float dt = clamp(0.5f + (t1.x - t2.x), 0.0f, 1.0f);
  float fWear = clamp((0.9f - t2.x) * 4.0f, 0.0f, 1.0f);
  float f =  clamp(fFont * fWear, 0.0f, 1.0f);
  return _mix( to_float3(1.0f, 0.98f, 0.9f) * (dt * 0.1f + 0.9f), to_float3(0.7f, 0.0f, 0.0f), f);
}

__DEVICE__ float3 StyleWood( float fFont, float2 uv )
{
  float3 t = _tex2DVecN(iChannel2,uv.x,uv.y,15).rgb;
  float fWear = fFont * smoothstep(0.0f, 0.4f, t.z);
  return _mix(t, to_float3_s(0.0f), fWear);
}

__DEVICE__ float4 GetRandom4(float x)
{
  return fract(to_float4(987.65f, 432.10f, 765.43f, 210.98f) * _sinf(to_float4(123.456f, 789.123f, 456.789f, 567.890f) * x));
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
  
  fragCoord+=0.5f;
  
  float fSequenceLength = 5.0f;
  
  float fTime = iTime;
  
  float fBlendSpeed = 0.05f;
  
  // Skip the initial fade-in
  fTime += fBlendSpeed * fSequenceLength;
  
  float fInt = _floor(fTime / fSequenceLength);
  float fFract = fract(fTime / fSequenceLength);
  
  float4 vRandom4 = GetRandom4(fInt);
  float2 vRandom2 = _floor(swi2(vRandom4,x,y) * to_float2(1234.56f, 123.45f));
  
  float fUVEffect = mod_f(vRandom2.x, 4.0f);
  float fScreenEffect = mod_f(vRandom2.y, 4.0f);

  if(fInt < 0.5f)
  {
    fUVEffect = 0.0f;
    fScreenEffect = 0.0f;
  }

  float4 vResult = to_float4_s(0.0f);
    
  float fX = 0.0f;
  for(int iX=0; iX<AA_X; iX++)
  {
    float fY = 0.0f;
    for(int y=0; y<AA_Y; y++)
    {
  
      float2 vUV = (fragCoord + to_float2(fX, fY)) / iResolution;
      vUV.x = ((vUV.x - 0.5f) * (iResolution.x / iResolution.y)) + 0.5f;    
      vUV.y = 1.0f - vUV.y;
        
      float2 vFontUV = vUV;
      float2 vBgUV = vUV;
      
            if ( false ) 
      if(iMouse.z > 0.0f)
      {
        fUVEffect = 999.0f;
        fScreenEffect = 0.0f;
        fFract = 0.5f;
        
        vFontUV *= 0.25f;
        vFontUV += swi2(iMouse,x,y) / iResolution;
        vFontUV.y -= 0.5f;
        vBgUV = vFontUV;
      }  
      
      if(fUVEffect < 0.5f)
      {
        vFontUV = GetUVCentre(vBgUV);
      }
      else
      if(fUVEffect < 1.5f)
      {
        vBgUV = GetUVScroll(vBgUV, fFract);
        vFontUV = vBgUV;
      }
      else
      if(fUVEffect < 2.5f)
      {
        float fSpeed = 0.1f + vRandom4.z;
        vBgUV.x += fFract * fSpeed;
        vFontUV = GetUVRepeat(vBgUV, 0.25f);
      }
      else
      if(fUVEffect < 3.5f)
      {
        float fSpeed = 1.0f + vRandom4.z * 2.0f;
        if(vRandom4.w > 0.5f)
        {
          fSpeed = -fSpeed;
        }
        vBgUV = GetUVRotate(vBgUV, 1.0f + fSpeed * fFract);
        vFontUV = GetUVRepeat(vBgUV, 0.0f);
      }
      
      float fShadertoy = step(Shadertoy(vFontUV), 0.0f);
        
      if(fScreenEffect < 0.5f)
      {
        vResult += to_float4_aw(StyleDefault(fShadertoy), 1.0f);
      }
      else if(fScreenEffect < 1.5f)
      {
        vResult += to_float4_aw(StyleScanline(fShadertoy, fragCoord), 1.0f);
      }
      else if(fScreenEffect < 2.5f)
      {
        vResult += to_float4_aw(StyleStamp(fShadertoy, vBgUV), 1.0f);
      }
      else
      {
        vResult += to_float4_aw(StyleWood(fShadertoy, vBgUV), 1.0f);
      }

      fY += 1.0f / (float)(AA_Y);
    }
    
    fX += 1.0f / (float)(AA_X);
  }
  
  swi3(vResult,x,y,z) /= vResult.w;

  float fFade = 0.0f;  
  if(fFract > (1.0f - fBlendSpeed))
  {
    fFade = smoothstep(1.0f - fBlendSpeed, 1.0f, fFract);
  }

  if(fFract < fBlendSpeed)
  {
    fFade = smoothstep(fBlendSpeed, 0.0f, fFract);
  }

  vResult = _mix(vResult, to_float4_s(1.0f), fFade);
  
  fragColor = to_float4(swi3(vResult,x,y,z), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}





// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Cubemap: Forest_0' to iChannel1
// Connect Buffer C 'Texture: Organic 4' to iChannel2
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Scene Rendering

#define ENABLE_TAA_JITTER

#define kMaxTraceDist 1000.0
#define kFarDist 1100.0

#define MAT_FG_BEGIN   10

///////////////////////////
// Scene
///////////////////////////

struct SceneResult
{
  float fDist;
  int iObjectId;
    float3 vUVW;
};
    
__DEVICE__ void Scene_Union( inout SceneResult a, in SceneResult b )
{
    if ( b.fDist < a.fDist )
    {
        a = b;
    }
}

    
__DEVICE__ void Scene_Subtract( inout SceneResult a, in SceneResult b )
{
    if ( a.fDist < -b.fDist )
    {
        a.fDist = -b.fDist;
        a.iObjectId = b.iObjectId;
        a.vUVW = b.vUVW;
    }
}

SceneResult Scene_GetDistance( float3 vPos );    

__DEVICE__ float3 Scene_GetNormal(const in float3 vPos)
{
    const float fDelta = 0.0001f;
    float2 e = to_float2( -1, 1 );
    
    float3 vNormal = 
        Scene_GetDistance( swi3(e,y,x,x) * fDelta + vPos ).fDist * swi3(e,y,x,x) + 
        Scene_GetDistance( swi3(e,x,x,y) * fDelta + vPos ).fDist * swi3(e,x,x,y) + 
        Scene_GetDistance( swi3(e,x,y,x) * fDelta + vPos ).fDist * swi3(e,x,y,x) + 
        Scene_GetDistance( swi3(e,y,y,y) * fDelta + vPos ).fDist * swi3(e,y,y,y);
    
    return normalize( vNormal );
}    
    
SceneResult Scene_Trace( const in float3 vRayOrigin, const in float3 vRayDir, float minDist, float maxDist )
{  
    SceneResult result;
    result.fDist = 0.0f;
    result.vUVW = to_float3_s(0.0f);
    result.iObjectId = -1;
    
  float t = minDist;
  const int kRaymarchMaxIter = 128;
  for(int i=0; i<kRaymarchMaxIter; i++)
  {    
        float epsilon = 0.0001f * t;
    result = Scene_GetDistance( vRayOrigin + vRayDir * t );
        if ( _fabs(result.fDist) < epsilon )
    {
      break;
    }
                        
        if ( t > maxDist )
        {
            result.iObjectId = -1;
          t = maxDist;
            break;
        }       
        
        if ( result.fDist > 1.0f )
        {
            result.iObjectId = -1;            
        }    
        
        t += result.fDist;        
  }
    
    result.fDist = t;


    return result;
}    

__DEVICE__ float Scene_TraceShadow( const in float3 vRayOrigin, const in float3 vRayDir, const in float fMinDist, const in float fLightDist )
{
    //return 1.0f;
    //return ( Scene_Trace( vRayOrigin, vRayDir, 0.1f, fLightDist ).fDist < fLightDist ? 0.0f : 1.0f;
    
  float res = 1.0f;
    float t = fMinDist;
    for( int i=0; i<16; i++ )
    {
    float h = Scene_GetDistance( vRayOrigin + vRayDir * t ).fDist;
        res = _fminf( res, 8.0f*h/t );
        t += clamp( h, 0.02f, 0.10f );
        if( h<0.0001f || t>fLightDist ) break;
    }
    return clamp( res, 0.0f, 1.0f );    
}

__DEVICE__ float Scene_GetAmbientOcclusion( const in float3 vPos, const in float3 vDir )
{
    float fOcclusion = 0.0f;
    float fScale = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float fOffsetDist = 0.001f + 0.1f*float(i)/4.0f;
        float3 vAOPos = vDir * fOffsetDist + vPos;
        float fDist = Scene_GetDistance( vAOPos ).fDist;
        fOcclusion += (fOffsetDist - fDist) * fScale;
        fScale *= 0.4f;
    }
    
    return clamp( 1.0f - 30.0f*fOcclusion, 0.0f, 1.0f );
}

///////////////////////////
// Lighting
///////////////////////////
    
struct SurfaceInfo
{
    float3 vPos;
    float3 vNormal;
    float3 vBumpNormal;    
    float3 vAlbedo;
    float3 vR0;
    float fSmoothness;
    float3 vEmissive;
};
    
SurfaceInfo Scene_GetSurfaceInfo( const in float3 vRayOrigin,  const in float3 vRayDir, SceneResult traceResult );

struct SurfaceLighting
{
    float3 vDiffuse;
    float3 vSpecular;
};
    
SurfaceLighting Scene_GetSurfaceLighting( const in float3 vRayDir, in SurfaceInfo surfaceInfo );

__DEVICE__ float Light_GIV( float dotNV, float k)
{
  return 1.0f / ((dotNV + 0.0001f) * (1.0f - k)+k);
}

__DEVICE__ void Light_Add(inout SurfaceLighting lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightDir, const in float3 vLightColour)
{
  float fNDotL = clamp(dot(vLightDir, surface.vBumpNormal), 0.0f, 1.0f);
  
  lighting.vDiffuse += vLightColour * fNDotL;
    
  float3 vH = normalize( -vViewDir + vLightDir );
  float fNdotV = clamp(dot(-vViewDir, surface.vBumpNormal), 0.0f, 1.0f);
  float fNdotH = clamp(dot(surface.vBumpNormal, vH), 0.0f, 1.0f);
    
  float alpha = 1.0f - surface.fSmoothness;
  // D

  float alphaSqr = alpha * alpha;
  float denom = fNdotH * fNdotH * (alphaSqr - 1.0f) + 1.0f;
  float d = alphaSqr / (PI * denom * denom);

  float k = alpha / 2.0f;
  float vis = Light_GIV(fNDotL, k) * Light_GIV(fNdotV, k);

  float fSpecularIntensity = d * vis * fNDotL;    
  lighting.vSpecular += vLightColour * fSpecularIntensity;    
}

__DEVICE__ void Light_AddPoint(inout SurfaceLighting lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightPos, const in float3 vLightColour)
{    
    float3 vPos = surface.vPos;
  float3 vToLight = vLightPos - vPos;  
    
  float3 vLightDir = normalize(vToLight);
  float fDistance2 = dot(vToLight, vToLight);
  float fAttenuation = 100.0f / (fDistance2);
  
  float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1f, length(vToLight) );
  
  Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

__DEVICE__ void Light_AddDirectional(inout SurfaceLighting lighting, SurfaceInfo surface, const in float3 vViewDir, const in float3 vLightDir, const in float3 vLightColour)
{  
  float fAttenuation = 1.0f;
  float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1f, 10.0f );
  
  Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

__DEVICE__ float3 Light_GetFresnel( float3 vView, float3 vNormal, float3 vR0, float fGloss )
{
    float NdotV = _fmaxf( 0.0f, dot( vView, vNormal ) );

    return vR0 + (to_float3_s(1.0f) - vR0) * _powf( 1.0f - NdotV, 5.0f ) * _powf( fGloss, 20.0f );
}

__DEVICE__ void Env_AddPointLightFlare(inout float3 vEmissiveGlow, const in float3 vRayOrigin, const in float3 vRayDir, const in float fIntersectDistance, const in float3 vLightPos, const in float3 vLightColour)
{
    float3 vToLight = vLightPos - vRayOrigin;
    float fPointDot = dot(vToLight, vRayDir);
    fPointDot = clamp(fPointDot, 0.0f, fIntersectDistance);

    float3 vClosestPoint = vRayOrigin + vRayDir * fPointDot;
    float fDist = length(vClosestPoint - vLightPos);
  vEmissiveGlow += _sqrtf(vLightColour * 0.05f / (fDist * fDist));
}

__DEVICE__ void Env_AddDirectionalLightFlareToFog(inout float3 vFogColour, const in float3 vRayDir, const in float3 vLightDir, const in float3 vLightColour)
{
  float fDirDot = clamp(dot(vLightDir, vRayDir) * 0.5f + 0.5f, 0.0f, 1.0f);
  float kSpreadPower = 2.0f;
  vFogColour += vLightColour * _powf(fDirDot, kSpreadPower) * 0.25f;
}


///////////////////////////
// Rendering
///////////////////////////

float4 Env_GetSkyColor( const float3 vViewPos, const float3 vViewDir );
float3 Env_ApplyAtmosphere( const in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist );
float3 FX_Apply( in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist);

__DEVICE__ float4 Scene_GetColorAndDepth( float3 vRayOrigin, float3 vRayDir )
{
  float3 vResultColor = to_float3_s(0.0f);
            
  SceneResult firstTraceResult;
    
    float fStartDist = 0.0f;
    float fMaxDist = 10.0f;
    
    float3 vRemaining = to_float3_s(1.0f);
    
  for( int iPassIndex=0; iPassIndex < 3; iPassIndex++ )
    {
      SceneResult traceResult = Scene_Trace( vRayOrigin, vRayDir, fStartDist, fMaxDist );

        if ( iPassIndex == 0 )
        {
            firstTraceResult = traceResult;
        }
        
        float3 vColor = to_float3_aw(0);
        float3 vReflectAmount = to_float3(0);
        
    if( traceResult.iObjectId < 0 )
    {
            vColor = Env_GetSkyColor( vRayOrigin, vRayDir ).rgb;
        }
        else
        {
            
            SurfaceInfo surfaceInfo = Scene_GetSurfaceInfo( vRayOrigin, vRayDir, traceResult );
            SurfaceLighting surfaceLighting = Scene_GetSurfaceLighting( vRayDir, surfaceInfo );
                
            // calculate reflectance (Fresnel)
      vReflectAmount = Light_GetFresnel( -vRayDir, surfaceInfo.vBumpNormal, surfaceInfo.vR0, surfaceInfo.fSmoothness );
      
      vColor = (surfaceInfo.vAlbedo * surfaceLighting.vDiffuse + surfaceInfo.vEmissive) * (to_float3_s(1.0f) - vReflectAmount); 
            
            float3 vReflectRayOrigin = surfaceInfo.vPos;
            float3 vReflectRayDir = normalize( reflect( vRayDir, surfaceInfo.vBumpNormal ) );
            fStartDist = 0.001f / _fmaxf(0.0000001f,_fabs(dot( vReflectRayDir, surfaceInfo.vNormal ))); 

            vColor += surfaceLighting.vSpecular * vReflectAmount;            

      vColor = Env_ApplyAtmosphere( vColor, vRayOrigin, vRayDir, traceResult.fDist );
      vColor = FX_Apply( vColor, vRayOrigin, vRayDir, traceResult.fDist );
            
            vRayOrigin = vReflectRayOrigin;
            vRayDir = vReflectRayDir;
        }
        
        vResultColor += vColor * vRemaining;
        vRemaining *= vReflectAmount;        
    }
 
    return to_float4( vResultColor, EncodeDepthAndObject( firstTraceResult.fDist, firstTraceResult.iObjectId ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////
// Scene Description
/////////////////////////

// Materials

#define MAT_SKY       -1
#define MAT_DEFAULT    0
#define MAT_SCREEN     1
#define MAT_TV_CASING    2
#define MAT_TV_TRIM      3
#define MAT_CHROME       4


__DEVICE__ float3 PulseIntegral( float3 x, float s1, float s2 )
{
    // Integral of function where result is 1.0f between s1 and s2 and 0 otherwise        

    // V1
    //if ( x > s2 ) return s2 - s1;
  //else if ( x > s1 ) return x - s1;
  //return 0.0f; 
    
    // V2
    //return clamp( (x - s1), 0.0f, s2 - s1);
    //return t;
    
    return clamp( (x - s1), to_float3_s(0.0f), to_float3(s2 - s1));
}

__DEVICE__ float PulseIntegral( float x, float s1, float s2 )
{
    // Integral of function where result is 1.0f between s1 and s2 and 0 otherwise        

    // V1
    //if ( x > s2 ) return s2 - s1;
  //else if ( x > s1 ) return x - s1;
  //return 0.0f; 
    
    // V2
    //return clamp( (x - s1), 0.0f, s2 - s1);
    //return t;
    
    return clamp( (x - s1), (0.0f), (s2 - s1));
}

__DEVICE__ float3 Bayer( float2 vUV, float2 vBlur )
{
    float3 x = to_float3(vUV.x);
    float3 y = to_float3(vUV.y);           

    x += to_float3(0.66f, 0.33f, 0.0f);
    y += 0.5f * step( fract( x * 0.5f ), to_float3_s(0.5f) );
        
    //x -= 0.5f;
    //y -= 0.5f;
    
    x = fract( x );
    y = fract( y );
    
    // cell centered at 0.5
    
    float2 vSize = to_float2(0.16f, 0.75f);
    
    float2 vMin = 0.5f - vSize * 0.5f;
    float2 vMax = 0.5f + vSize * 0.5f;
    
    float3 vResult= to_float3_s(0.0f);
    
    float3 vResultX = (PulseIntegral( x + vBlur.x, vMin.x, vMax.x) - PulseIntegral( x - vBlur.x, vMin.x, vMax.x)) / _fminf( vBlur.x, 1.0f);
    float3 vResultY = (PulseIntegral(y + vBlur.y, vMin.y, vMax.y) - PulseIntegral(y - vBlur.y, vMin.y, vMax.y))  / _fminf( vBlur.y, 1.0f);
    
    vResult = _fminf(vResultX,vResultY)  * 5.0f;
        
    //vResult = to_float3_s(1.0f);
    
    return vResult;
}

__DEVICE__ float3 GetPixelMatrix( float2 vUV )
{
#if 1
    float2 dx = dFdx( vUV );
    float2 dy = dFdy( vUV );
    float dU = length( to_float2( dx.x, dy.x ) );
    float dV = length( to_float2( dx.y, dy.y ) );
    if (dU <= 0.0f || dV <= 0.0f ) return to_float3_s(1.0f);
    return Bayer( vUV, to_float2(dU, dV) * 1.0f);
#else
    return to_float3_s(1.0f);
#endif
}

__DEVICE__ float Scanline( float y, float fBlur )
{   
    float fResult = _sinf( y * 10.0f ) * 0.45f + 0.55f;
    return _mix( fResult, 1.0f, _fminf( 1.0f, fBlur ) );
}


__DEVICE__ float GetScanline( float2 vUV )
{
#if 1
    vUV.y *= 0.25f;
    float2 dx = dFdx( vUV );
    float2 dy = dFdy( vUV );
    float dV = length( to_float2( dx.y, dy.y ) );
    if (dV <= 0.0f ) return 1.0f;
    return Scanline( vUV.y, dV * 1.3f );
#else
    return 1.0f;
#endif
}


float2 kScreenRsolution = to_float2(480.0f, 576.0f);

struct Interference
{
    float noise;
    float scanLineRandom;
};

__DEVICE__ float InterferenceHash(float p)
{
    float hashScale = 0.1031f;

    float3 p3  = fract(to_float3(p, p, p) * hashScale);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}


__DEVICE__ float InterferenceSmoothNoise1D( float x )
{
    float f0 = _floor(x);
    float fr = fract(x);

    float h0 = InterferenceHash( f0 );
    float h1 = InterferenceHash( f0 + 1.0f );

    return h1 * fr + h0 * (1.0f - fr);
}


__DEVICE__ float InterferenceNoise( float2 uv )
{
  float displayVerticalLines = 483.0f;
    float scanLine = _floor(uv.y * displayVerticalLines); 
    float scanPos = scanLine + uv.x;
  float timeSeed = fract( iTime * 123.78f );
    
    return InterferenceSmoothNoise1D( scanPos * 234.5f + timeSeed * 12345.6f );
}
    
Interference GetInterference( float2 vUV )
{
    Interference interference;
        
    interference.noise = InterferenceNoise( vUV );
    interference.scanLineRandom = InterferenceHash(vUV.y * 100.0f + fract(iTime * 1234.0f) * 12345.0f);
    
    return interference;
}
    
__DEVICE__ float3 SampleScreen( float3 vUVW )
{   
    float3 vAmbientEmissive = to_float3_s(0.1f);
    float3 vBlackEmissive = to_float3_s(0.02f);
    float fBrightness = 1.75f;
    float2 vResolution = to_float2(480.0f, 576.0f);
    float2 vPixelCoord = swi2(vUVW,x,y) * vResolution;
    
    float3 vPixelMatrix = GetPixelMatrix( vPixelCoord );
    float fScanline = GetScanline( vPixelCoord );
      
    float2 vTextureUV = swi2(vUVW,x,y);
    //vec2 vTextureUV = vPixelCoord;
    vTextureUV = _floor(vTextureUV * vResolution * 2.0f) / (vResolution * 2.0f);
    
    Interference interference = GetInterference( vTextureUV );

    float noiseIntensity = 0.1f;
    
    //vTextureUV.x += (interference.scanLineRandom * 2.0f - 1.0f) * 0.025f * noiseIntensity;
    
    
    float3 vPixelEmissive = textureLod( iChannel0, swi2(vTextureUV,x,y), 0.0f ).rgb;
        
    vPixelEmissive = clamp( vPixelEmissive + (interference.noise - 0.5f) * 2.0f * noiseIntensity, 0.0f, 1.0f );
    
  float3 vResult = (vPixelEmissive * vPixelEmissive * fBrightness + vBlackEmissive) * vPixelMatrix * fScanline + vAmbientEmissive;
    
    // TODO: feather edge?
    if( any( greaterThanEqual( swi2(vUVW,x,y), to_float2_s(1.0f) ) ) || any ( lessThan( swi2(vUVW,x,y), to_float2_s(0.0f) ) ) || ( vUVW.z > 0.0f ) )
    {
        return to_float3_s(0.0f);
    }
    
    return vResult;
    
}

SurfaceInfo Scene_GetSurfaceInfo( const in float3 vRayOrigin,  const in float3 vRayDir, SceneResult traceResult )
{
    SurfaceInfo surfaceInfo;
    
    surfaceInfo.vPos = vRayOrigin + vRayDir * (traceResult.fDist);
    
    surfaceInfo.vNormal = Scene_GetNormal( surfaceInfo.vPos ); 
    surfaceInfo.vBumpNormal = surfaceInfo.vNormal;
    surfaceInfo.vAlbedo = to_float3_s(1.0f);
    surfaceInfo.vR0 = to_float3_s( 0.02f );
    surfaceInfo.fSmoothness = 1.0f;
    surfaceInfo.vEmissive = to_float3_s( 0.0f );
    //return surfaceInfo;
        
    if ( traceResult.iObjectId == MAT_DEFAULT )
    {
      surfaceInfo.vR0 = to_float3_s( 0.02f );
      surfaceInfo.vAlbedo = textureLod( iChannel2, traceResult.swi2(vUVW,x,z) * 2.0f, 0.0f ).rgb;
        surfaceInfo.vAlbedo = surfaceInfo.vAlbedo * surfaceInfo.vAlbedo;
                        
      surfaceInfo.fSmoothness = clamp( 1.0f - surfaceInfo.vAlbedo.x * surfaceInfo.vAlbedo.x * 2.0f, 0.0f, 1.0f);
        
    }
    
    if ( traceResult.iObjectId == MAT_SCREEN )
    {
        surfaceInfo.vAlbedo = to_float3_s(0.02f); 
        surfaceInfo.vEmissive = SampleScreen( traceResult.vUVW );        
    }

    if ( traceResult.iObjectId == MAT_TV_CASING )
    {
        surfaceInfo.vAlbedo = to_float3(0.5f, 0.4f, 0.3f); 
      surfaceInfo.fSmoothness = 0.4f;        
    }
    
    if ( traceResult.iObjectId == MAT_TV_TRIM )
    {
        surfaceInfo.vAlbedo = to_float3(0.03f, 0.03f, 0.05f); 
      surfaceInfo.fSmoothness = 0.5f;
    }    

    if ( traceResult.iObjectId == MAT_CHROME )
    {
        surfaceInfo.vAlbedo = to_float3(0.01f, 0.01f, 0.01f); 
      surfaceInfo.fSmoothness = 0.9f;
      surfaceInfo.vR0 = to_float3_s( 0.8f );
    }    
 
    return surfaceInfo;
}

// Scene Description

__DEVICE__ float SmoothMin( float a, float b, float k )
{
  //return _fminf(a,b);
  
  
    //float k = 0.06f;
  float h = clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float UdRoundBox( float3 p, float3 b, float r )
{
    //vec3 vToFace = _fabs(p) - b;
    //vec3 vConstrained = _fmaxf( vToFace, 0.0f );
    //return length( vConstrained ) - r;
    return length(_fmaxf(_fabs(p)-b,0.0f))-r;
}

SceneResult Scene_GetCRT( float3 vScreenDomain, float2 vScreenWH, float fScreenCurveRadius, float fBevel, float fDepth )
{
    SceneResult resultScreen;
#if 1
    float3 vScreenClosest;
    swi2(vScreenClosest,x,y) = _fmaxf(_fabs(swi2(vScreenDomain,x,y))-vScreenWH,0.0f);
    float2 vCurveScreenDomain = swi2(vScreenDomain,x,y);
    vCurveScreenDomain = clamp( vCurveScreenDomain, -vScreenWH, vScreenWH );
    float fCurveScreenProjection2 = fScreenCurveRadius * fScreenCurveRadius - vCurveScreenDomain.x * vCurveScreenDomain.x - vCurveScreenDomain.y * vCurveScreenDomain.y;
    float fCurveScreenProjection = _sqrtf( fCurveScreenProjection2 ) - fScreenCurveRadius;
    vScreenClosest.z = vScreenDomain.z - clamp( vScreenDomain.z, -fCurveScreenProjection, fDepth );
    resultScreen.vUVW.z = vScreenDomain.z + fCurveScreenProjection;        
    resultScreen.fDist = (length( vScreenClosest ) - fBevel) * 0.95f;
    //resultScreen.fDist = (length( vScreenDomain - to_float3(0,0,fScreenCurveRadius)) - fScreenCurveRadius - fBevel);    
#endif    
    
#if 0
    float3 vScreenClosest;
    swi3(vScreenClosest,x,y,z) = _fmaxf(_fabs(swi3(vScreenDomain,x,y,z))-to_float3_aw(vScreenWH, fDepth),0.0f);
    float fRoundDist = length( swi3(vScreenClosest,x,y,z) ) - fBevel;
    float fSphereDist = length( vScreenDomain - to_float3(0,0,fScreenCurveRadius) ) - (fScreenCurveRadius + fBevel);    
    resultScreen.fDist = _fmaxf(fRoundDist, fSphereDist);
#endif    
    
    resultScreen.swi2(vUVW,x,y) = (swi2(vScreenDomain,x,y) / vScreenWH) * 0.5f + 0.5f;
  resultScreen.iObjectId = MAT_SCREEN;
    return resultScreen;
}

SceneResult Scene_GetComputer( float3 vPos )
{
    SceneResult resultComputer;
    resultComputer.vUVW = swi3(vPos,x,z,y);
  
    float fXSectionStart = -0.2f;
    float fXSectionLength = 0.15f;
    float fXSectionT = clamp( (vPos.z - fXSectionStart) / fXSectionLength, 0.0f, 1.0f);
    float fXSectionR1 = 0.03f;
    float fXSectionR2 = 0.05f;
    float fXSectionR = _mix( fXSectionR1, fXSectionR2, fXSectionT );
    float fXSectionZ = fXSectionStart + fXSectionT * fXSectionLength;
    
    float2 vXSectionCentre = to_float2(fXSectionR, fXSectionZ );
    float2 vToPos = swi2(vPos,y,z) - vXSectionCentre;
    float l = length( vToPos );
    if ( l > fXSectionR ) l = fXSectionR;
    float2 vXSectionClosest = vXSectionCentre + normalize(vToPos) * l;
    //float fXSectionDist = length( vXSectionClosest ) - fXSectionR;
    
    float x = _fmaxf( _fabs( vPos.x ) - 0.2f, 0.0f );

    resultComputer.fDist = length( to_float3(x, vXSectionClosest - swi2(vPos,y,z)) )-0.01f;
    //resultComputer.fDist = x;
        
    resultComputer.iObjectId = MAT_TV_CASING;
/*
    float3 vKeyPos = swi3(vPos,x,y,z) - to_float3(0,0.125f,0);
    vKeyPos.y -= vKeyPos.z * (fXSectionR2 - fXSectionR1) * 2.0f / fXSectionLength;
    float fDomainRepeatScale = 0.02f;
    if ( fract(vKeyPos.z * 0.5f / fDomainRepeatScale + 0.25f) > 0.5f) vKeyPos.x += fDomainRepeatScale * 0.5f;
    float2 vKeyIndex = round(swi2(vKeyPos,x,z) / fDomainRepeatScale);
    vKeyIndex.x = clamp( vKeyIndex.x, -8.0f, 8.0f );
    vKeyIndex.y = clamp( vKeyIndex.y, -10.0f, -5.0f );
    //swi2(vKeyPos,x,z) = (fract( swi2(vKeyPos,x,z) / fDomainRepeatScale ) - 0.5f) * fDomainRepeatScale;
    swi2(vKeyPos,x,z) = (swi2(vKeyPos,x,z) - (vKeyIndex) * fDomainRepeatScale);
    swi2(vKeyPos,x,z) /= 0.7f + vKeyPos.y;
    SceneResult resultKey;    
    resultKey.vUVW = swi3(vPos,x,z,y);
    resultKey.fDist = UdRoundBox( vKeyPos, to_float3_s(0.01f), 0.001f );
    resultKey.iObjectId = MAT_TV_TRIM;
    Scene_Union( resultComputer, resultKey );
*/    
    return resultComputer;
}

SceneResult Scene_GetDistance( float3 vPos )
{
    SceneResult result;
    
  //result.fDist = vPos.y;
    float fBenchBevel = 0.01f;
    result.fDist = UdRoundBox( vPos - to_float3(0,-0.02f-fBenchBevel,0.0f), to_float3(2.0f, 0.02f, 1.0f), fBenchBevel );
    result.vUVW = vPos;
  result.iObjectId = MAT_DEFAULT;        
    
    float3 vSetPos = to_float3(0.0f, 0.0f, 0.0f);
    float3 vScreenPos = vSetPos + to_float3(0.0f, 0.25f, 0.00f);
    
    //vPos.x = fract( vPos.x - 0.5f) - 0.5f;
    
    float2 vScreenWH = to_float2(4.0f, 3.0f) / 25.0f;

    SceneResult resultSet;
    resultSet.vUVW = swi3(vPos,x,z,y);
  resultSet.fDist = UdRoundBox( vPos - vScreenPos - to_float3(0.0f,-0.01f,0.2f), to_float3(0.21f, 0.175f, 0.18f), 0.01f );
    resultSet.iObjectId = MAT_TV_CASING;
    Scene_Union( result, resultSet );

    SceneResult resultSetRecess;
    resultSetRecess.vUVW = swi3(vPos,x,z,y);
    resultSetRecess.fDist = UdRoundBox( vPos - vScreenPos - to_float3(0.0f,-0.0f, -0.05f), to_float3_aw(vScreenWH + 0.01f, 0.05f) + 0.005f, 0.015f );
    resultSetRecess.iObjectId = MAT_TV_TRIM;
  Scene_Subtract( result, resultSetRecess );
    
    SceneResult resultSetBase;
    resultSetBase.vUVW = swi3(vPos,x,z,y);
    float fBaseBevel = 0.03f;
  resultSetBase.fDist = UdRoundBox( vPos - vSetPos - to_float3(0.0f,0.04f,0.22f), to_float3(0.2f, 0.04f, 0.17f) - fBaseBevel, fBaseBevel );
    resultSetBase.iObjectId = MAT_TV_CASING;
    Scene_Union( result, resultSetBase );

  SceneResult resultScreen = Scene_GetCRT( vPos - vScreenPos, vScreenWH, 0.75f, 0.02f, 0.1f );
    Scene_Union( result, resultScreen );    
    
    //SceneResult resultComputer = Scene_GetComputer( vPos - to_float3(0.0f, 0.0f, -0.1f) );
    //Scene_Union( result, resultComputer );

    SceneResult resultSphere;
    resultSet.vUVW = swi3(vPos,x,z,y);
  resultSet.fDist = length(vPos - to_float3(0.35f,0.075f,-0.1f)) - 0.075f;
    resultSet.iObjectId = MAT_CHROME;
    Scene_Union( result, resultSet );    
    
    return result;
}



// Scene Lighting

float3 g_vSunDir = normalize(to_float3(0.3f, 0.4f, -0.5f));
float3 g_vSunColor = to_float3(1, 0.95f, 0.8f) * 3.0f;
float3 g_vAmbientColor = to_float3(0.8f, 0.8f, 0.8f) * 1.0f;

SurfaceLighting Scene_GetSurfaceLighting( const in float3 vViewDir, in SurfaceInfo surfaceInfo )
{
    SurfaceLighting surfaceLighting;
    
    surfaceLighting.vDiffuse = to_float3_s(0.0f);
    surfaceLighting.vSpecular = to_float3_s(0.0f);    
    
    Light_AddDirectional( surfaceLighting, surfaceInfo, vViewDir, g_vSunDir, g_vSunColor );
    
    Light_AddPoint( surfaceLighting, surfaceInfo, vViewDir, to_float3(1.4f, 2.0f, 0.8f), to_float3(1,1,1) * 0.2f );
    
    float fAO = Scene_GetAmbientOcclusion( surfaceInfo.vPos, surfaceInfo.vNormal );
    // AO
    surfaceLighting.vDiffuse += fAO * (surfaceInfo.vBumpNormal.y * 0.5f + 0.5f) * g_vAmbientColor;
    
    return surfaceLighting;
}

// Environment

__DEVICE__ float4 Env_GetSkyColor( const float3 vViewPos, const float3 vViewDir )
{
  float4 vResult = to_float4( 0.0f, 0.0f, 0.0f, kFarDist );

#if 1
    float3 vEnvMap = textureLod( iChannel1, swi3(vViewDir,z,y,x), 0.0f ).rgb;
    swi3(vResult,x,y,z) = vEnvMap;
#endif    
    
#if 0
    float3 vEnvMap = textureLod( iChannel1, swi3(vViewDir,z,y,x), 0.0f ).rgb;
    vEnvMap = vEnvMap * vEnvMap;
    float kEnvmapExposure = 0.999f;
    swi3(vResult,x,y,z) = -_log2f(1.0f - vEnvMap * kEnvmapExposure);

#endif
    
    // Sun
    //float NdotV = dot( g_vSunDir, vViewDir );
    //swi3(vResult,x,y,z) += smoothstep( _cosf(radians(0.7f)), _cosf(radians(0.5f)), NdotV ) * g_vSunColor * 5000.0f;

    return vResult;  
}

__DEVICE__ float Env_GetFogFactor(const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist )
{    
  float kFogDensity = 0.00001f;
  return _expf(fDist * -kFogDensity);  
}

__DEVICE__ float3 Env_GetFogColor(const in float3 vDir)
{    
  return to_float3(0.2f, 0.5f, 0.6f) * 2.0f;    
}

__DEVICE__ float3 Env_ApplyAtmosphere( const in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist )
{
    //return vColor;
    float3 vResult = vColor;
    
    
  float fFogFactor = Env_GetFogFactor( vRayOrigin, vRayDir, fDist );
  float3 vFogColor = Env_GetFogColor( vRayDir );  
  //Env_AddDirectionalLightFlareToFog( vFogColor, vRayDir, g_vSunDir, g_vSunColor * 3.0f);    
    vResult = _mix( vFogColor, vResult, fFogFactor );

    return vResult;      
}


__DEVICE__ float3 FX_Apply( in float3 vColor, const in float3 vRayOrigin,  const in float3 vRayDir, const in float fDist)
{    
    return vColor;
}


__DEVICE__ float4 MainCommon( float3 vRayOrigin, float3 vRayDir )
{
  float4 vColorLinAndDepth = Scene_GetColorAndDepth( vRayOrigin, vRayDir );    
    swi3(vColorLinAndDepth,x,y,z) = _fmaxf( swi3(vColorLinAndDepth,x,y,z), to_float3_s(0.0f) );
    
    float4 vFragColor = vColorLinAndDepth;
    
    float fExposure = 2.0f;
    
    swi3(vFragColor,x,y,z) *= fExposure;
    
    vFragColor.w = vColorLinAndDepth.w;
    
    return vFragColor;
}

CameraState GetCameraPosition( int index )
{
    CameraState cam;

    float3 vFocus = to_float3(0,0.25f,-0.012f);   
    
    if ( index > 9 )
    {
      index = int(hash11(float(index) / 10.234f) * 100.0f);
      index = index % 10;
    }

    //index=2;
    
    if ( index == 0 )
    {
        cam.vPos = to_float3(-0.1f,0.2f,-0.08f);
        cam.vTarget = to_float3(0,0.25f,0.1f);
        cam.fFov = 10.0f;
    }
    if ( index == 1 )
    {
        cam.vPos = to_float3(0.01f,0.334f,-0.03f);
        cam.vTarget = to_float3(0,0.3f,0.1f);
        cam.fFov = 10.0f;
    }
    if ( index == 2 )
    {
        cam.vPos = to_float3(-0.8f,0.3f,-1.0f);
        cam.vTarget = to_float3(0.4f,0.18f,0.5f);
        cam.fFov = 10.0f;
    }
    if ( index == 3 )
    {
        cam.vPos = to_float3(-0.8f,1.0f,-1.5f);
        cam.vTarget = to_float3(0.2f,0.0f,0.5f);
        cam.fFov = 10.0f;
    }
    if ( index == 4 )
    {
        cam.vPos = to_float3(-0.8f,0.3f,-1.0f);
        cam.vTarget = to_float3(0.4f,0.18f,0.5f);
        cam.fFov = 20.0f;
    }
    if ( index == 5 )
    {
        cam.vPos = to_float3(-0.244f,0.334f,-0.0928f);
        cam.vTarget = to_float3(0,0.25f,0.1f);
        cam.fFov = 20.0f;
    }
    if ( index == 6 )
    {
        cam.vPos = to_float3(0.0f,0.1f,-0.5f);
        cam.vTarget = to_float3(0.2f,0.075f,-0.1f);
        vFocus = cam.vTarget; 
        cam.fFov = 15.0f;
    }
    if ( index == 7 )
    {
        cam.vPos = to_float3(-0.01f,0.01f,-0.25f);
        cam.vTarget = to_float3(0.01f,0.27f,0.1f);
        vFocus = cam.vTarget; 
        cam.fFov = 23.0f;
    }
    if ( index == 8 )
    {
        cam.vPos = to_float3(-0.23f,0.3f,-0.05f);
        cam.vTarget = to_float3(0.1f,0.2f,0.1f);
        cam.fFov = 15.0f;
    }
    if ( index == 9 )
    {
        cam.vPos = to_float3(0.4f,0.2f,-0.2f);
        cam.vTarget = to_float3(-0.1f,0.25f,0.1f);
        cam.fFov = 12.0f;
    }
    
    cam.fPlaneInFocus = length( vFocus - cam.vPos);
    cam.vJitter = to_float2_s(0.0f);        
    
    return cam;
}

__KERNEL__ void MetaCrtJipiFuse__Buffer_C(float4 vFragColor, float2 vFragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    float2 vUV = swi2(vFragCoord,x,y) / iResolution; 

    CameraState cam;
    
    {
      CameraState camA;
      CameraState camB;
    
        float fSeqTime = iTime;
        float fSequenceSegLength = 5.0f;
        float fSeqIndex = _floor(fSeqTime / fSequenceSegLength);
        float fSeqPos = fract(fSeqTime / fSequenceSegLength);
        int iIndex = int(fSeqIndex);
    int iIndexNext = int(fSeqIndex) + 1;
        camA = GetCameraPosition(iIndex);
        camB = GetCameraPosition(iIndexNext);
        
        float t = smoothstep(0.3f, 1.0f, fSeqPos);
        cam.vPos = _mix(camA.vPos, camB.vPos, t );
        cam.vTarget = _mix(camA.vTarget, camB.vTarget, t );
        cam.fFov = _mix(camA.fFov, camB.fFov, t );
        cam.fPlaneInFocus = _mix(camA.fPlaneInFocus, camB.fPlaneInFocus, t );
    }
    
    if ( iMouse.z > 0.0f )
    {
        float fDist = 0.01f + 3.0f * (iMouse.y / iResolution.y);

        float fAngle = (iMouse.x / iResolution.x) * radians(360.0f);
      //float fElevation = (iMouse.y / iResolution.y) * radians(90.0f);
      float fElevation = 0.15f * radians(90.0f);    

        cam.vPos = to_float3_aw(_sinf(fAngle) * fDist * _cosf(fElevation),_sinf(fElevation) * fDist,_cosf(fAngle) * fDist * _cosf(fElevation));
        cam.vTarget = to_float3(0,0.25f,0.1f);
        cam.vPos +=cam.vTarget;
        cam.fFov = 20.0f / (1.0f + fDist * 0.5f);
      float3 vFocus = to_float3(0,0.25f,-0.012f);      
      cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
    
#if 0
    {
        float fDist = 0.5f;

        float fAngle = 0.6f * PI * 2.0f;
        float fElevation = 0.2f;
        
        cam.vPos = to_float3_aw(_sinf(fAngle) * fDist * _cosf(fElevation),_sinf(fElevation) * fDist,_cosf(fAngle) * fDist * _cosf(fElevation));
        cam.vTarget = to_float3(0.05f,0.25f,0.1f);
        cam.vPos +=cam.vTarget;
        cam.fFov = 22.0f;
      float3 vFocus = to_float3(0,0.25f,-0.012f);      
      cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
#endif
    
#ifdef ENABLE_TAA_JITTER
    cam.vJitter = hash21( fract( iTime ) ) - 0.5f;
#endif
    
            
    float3 vRayOrigin, vRayDir;
    float2 vJitterUV = vUV + cam.vJitter / iResolution;
    Cam_GetCameraRay( vJitterUV, iResolution, cam, vRayOrigin, vRayDir );
 
    float fHitDist = 0.0f;
    vFragColor = MainCommon( vRayOrigin, vRayDir );
    
    
  Cam_StoreState( to_int2(0), cam, vFragColor, to_int2(swi2(vFragCoord,x,y)) );    


  SetFragmentShaderComputedColor(vFragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Temporal Anti-aliasing Pass

#define ENABLE_TAA

#define iChannelCurr iChannel0
#define iChannelHistory iChannel1

__DEVICE__ float3 Tonemap( float3 x )
{
    float a = 0.010f;
    float b = 0.132f;
    float c = 0.010f;
    float d = 0.163f;
    float e = 0.101f;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );
}

__DEVICE__ float3 TAA_ColorSpace( float3 color )
{
    return Tonemap(color);
}


__KERNEL__ void MetaCrtJipiFuse__Buffer_D(float4 vFragColor, float2 vFragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    CameraState camCurr;
  Cam_LoadState( camCurr, iChannelCurr, to_int2(0) );
    
    CameraState camPrev;
  Cam_LoadState( camPrev, iChannelHistory, to_int2(0) );

    float2 vUV = swi2(vFragCoord,x,y) / iResolution;
   float2 vUnJitterUV = vUV - camCurr.vJitter / iResolution;    
    
    vFragColor = textureLod(iChannelCurr, vUnJitterUV, 0.0f);
    
    
#ifdef ENABLE_TAA
    float3 vRayOrigin, vRayDir;
    Cam_GetCameraRay( vUV, iResolution, camCurr, vRayOrigin, vRayDir );    
    float fDepth;
    int iObjectId;
    float4 vCurrTexel = texelFetch( iChannelCurr, to_int2(swi2(vFragCoord,x,y)), 0);
    fDepth = DecodeDepthAndObjectId( vCurrTexel.w, iObjectId );
    float3 vWorldPos = vRayOrigin + vRayDir * fDepth;
    
    float2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev), iResolution );// + camPrev.vJitter / iResolution;
        
    if ( all( greaterThanEqual( vPrevUV, to_float2(0) )) && all( lessThan( vPrevUV, to_float2(1) )) )
  {
        float3 vMin = to_float3( 10000);
        float3 vMax = to_float3(-10000);
        
      int2 vCurrXY = to_int2(_floor(swi2(vFragCoord,x,y)));    
        
        int iNeighborhoodSize = 1;
        for ( int iy=-iNeighborhoodSize; iy<=iNeighborhoodSize; iy++)
        {
            for ( int ix=-iNeighborhoodSize; ix<=iNeighborhoodSize; ix++)
            {
                int2 iOffset = to_int2(ix, iy);
            float3 vTest = TAA_ColorSpace( texelFetch( iChannelCurr, vCurrXY + iOffset, 0 ).rgb );
                                
                vMin = _fminf( vMin, vTest );
                vMax = _fmaxf( vMax, vTest );
            }
        }
        
        float epsilon = 0.001f;
        vMin -= epsilon;
        vMax += epsilon;
        
        float fBlend = 0.0f;
        
        //ivec2 vPrevXY = to_int2(_floor(swi2(vPrevUV,x,y) * iResolution));
        float4 vHistory = textureLod( iChannelHistory, vPrevUV, 0.0f );

        float3 vPrevTest = TAA_ColorSpace( swi3(vHistory,x,y,z) );
        if( all( greaterThanEqual(vPrevTest, vMin ) ) && all( lessThanEqual( vPrevTest, vMax ) ) )
        {
            fBlend = 0.9f;
            //vFragColor.x *= 0.0f;
        }
        
        swi3(vFragColor,x,y,z) = _mix( swi3(vFragColor,x,y,z), swi3(vHistory,x,y,z), fBlend);
    }  
    else
    {
        //vFragColor.gb *= 0.0f;
    }

#endif
    
    swi3(vFragColor,x,y,z) += (hash13( to_float3_aw( vFragCoord, iTime ) ) * 2.0f - 1.0f) * 0.03f;
    
  Cam_StoreState( to_int2(0), camCurr, vFragColor, to_int2(swi2(vFragCoord,x,y)) );    
  Cam_StoreState( to_int2(3,0), camPrev, vFragColor, to_int2(swi2(vFragCoord,x,y)) );    


  SetFragmentShaderComputedColor(vFragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer D' to iChannel0


// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Uncomment different defines in Buf B to run different shaders on TV

// Postprocessing Pass
// Motion blur, Depth of Field, Vignetting & Tonemap


#define ENABLE_DOF
#define ENABLE_MOTION_BLUR



__DEVICE__ float3 Tonemap( float3 x )
{
#if 0 
    
    float3 luminanceCoeffsBT709 = to_float3( 0.2126f, 0.7152f, 0.0722f );
    float f = dot( x, luminanceCoeffsBT709 );
    x /= f;        
    f = 1.0f - _expf(-f);    
    x *= f;    
    x = _mix( x, to_float3_aw(f), f*f );
    
    return x;
#else       
    float a = 0.010f;
    float b = 0.132f;
    float c = 0.010f;
    float d = 0.163f;
    float e = 0.101f;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
#endif    
}


__DEVICE__ float GetVignetting( const in float2 vUV, float fScale, float fPower, float fStrength )
{
  float2 vOffset = (vUV - 0.5f) * _sqrtf(2.0f) * fScale;
  
  float fDist = _fmaxf( 0.0f, 1.0f - length( vOffset ) );
    
  float fShade = 1.0f - _powf( fDist, fPower );
    
    fShade = 1.0f - fShade * fStrength;

  return fShade;
}




__DEVICE__ float GetCoC( float fDistance, float fPlaneInFocus )
{
#ifdef ENABLE_DOF    
  // http://http.developer.nvidia.com/GPUGems/gpugems_ch23.html

    float fAperture = _fminf(1.0f, fPlaneInFocus * fPlaneInFocus * 0.5f);
    float fFocalLength = 0.03f;
    
  return _fabs(fAperture * (fFocalLength * (fDistance - fPlaneInFocus)) /
          (fDistance * (fPlaneInFocus - fFocalLength)));  
#else
    return 0.0f;
#endif    
}

// Depth of field pass

#define BLUR_TAPS 64
float fGolden = 3.141592f * (3.0f - _sqrtf(5.0f));

#define MOD2 to_float2(4.438975f,3.972973f)

__DEVICE__ float Hash( float p ) 
{
    // https://www.shadertoy.com/view/4djSRW - Dave Hoskins
  float2 p2 = fract(to_float2(p) * MOD2);
    p2 += dot(swi2(p2,y,x), swi2(p2,x,y)+19.19f);
  return fract(p2.x * p2.y);    
}


__KERNEL__ void MetaCrtJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    CameraState camCurr;
  Cam_LoadState( camCurr, iChannel0, to_int2(0) );

    CameraState camPrev;
  Cam_LoadState( camPrev, iChannel0, to_int2(3,0) );
    
  float2 vUV = fragCoord / iResolution;
   //vUV -= camCurr.vJitter / iResolution;    // TAA has removed jitter

    float4 vSample = texelFetch( iChannel0, to_int2(fragCoord), 0 ).rgba;
    
    int iObjectId;
    float fDepth = DecodeDepthAndObjectId( vSample.w, iObjectId );
    
    float3 vRayOrigin, vRayDir;
    
    Cam_GetCameraRay( vUV, iResolution, camCurr, vRayOrigin, vRayDir );    
    float3 vWorldPos = vRayOrigin + vRayDir * fDepth;
        
    float2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev), iResolution );// - camPrev.vJitter / iResolution;
        
  float3 vResult = to_float3_s(0.0f);
    
    float fTot = 0.0f;
    
    float fPlaneInFocus = camCurr.fPlaneInFocus;
        
  float fCoC = GetCoC( fDepth, camCurr.fPlaneInFocus );
        
    float r = 1.0f;
    float2 vangle = to_float2(0.0f,fCoC); // Start angle
    
    float fWeight = _fmaxf( 0.001f, fCoC );    
    swi3(vResult,x,y,z) = swi3(vSample,x,y,z) * fWeight;
    fTot += fWeight;
    
#if defined(ENABLE_DOF) || defined(ENABLE_MOTION_BLUR)    
    float fMotionBlurTaps = float(BLUR_TAPS);
    
    float fShutterAngle = 0.5f;
    
    float f = 0.0f;
    float fIndex = 0.0f;
    for(int i=1; i<BLUR_TAPS; i++)
    {
        float fRandomT = Hash( iTime + fIndex + vUV.x + vUV.y * 12.345f);
        float fOrderedT = fIndex / fMotionBlurTaps;
        
        float fDofT = fOrderedT;
        float fMotionT = fRandomT;
        
        float2 vTapUV = vUV;
        #ifdef ENABLE_MOTION_BLUR
        vTapUV = _mix( vTapUV, vPrevUV, (fMotionT - 0.5f) * fShutterAngle );
        #endif
                
        // http://blog.marmakoide.org/?p=1
        
        float fTheta = fDofT * fGolden * fMotionBlurTaps;
        float fRadius = fCoC * _sqrtf( fDofT * fMotionBlurTaps ) / _sqrtf( fMotionBlurTaps );        
        
        vTapUV += to_float2( _sinf(fTheta), _cosf(fTheta) ) * fRadius;
        
        float4 vTapSample = textureLod( iChannel0, vTapUV, 0.0f ).rgba;
      //vec4 vTapTexel = texelFetch( iChannel0, to_int2(swi2(vTapUV,x,y) * iResolution), 0 ).rgba;
        
        int iTapObjectId;
        float fTapDepth = DecodeDepthAndObjectId( vTapSample.w, iTapObjectId );
        
        if ( fTapDepth > 0.0f )
        {            
          float fCurrCoC = GetCoC( fTapDepth, fPlaneInFocus );
            
            float fCurrWeight = _fmaxf( 0.001f, fCurrCoC );
            
        vResult += swi3(vTapSample,x,y,z) * fCurrWeight;
          fTot += fCurrWeight;
        }
        f += 1.0f;
        fIndex += 1.0f;
    }
#endif    
    vResult /= fTot;
    
  fragColor = to_float4_aw(vResult, 1.0f);    
    
    float fShade = GetVignetting( vUV, 0.7f, 2.0f, 1.0f );
    
    swi3(fragColor,x,y,z) *= fShade;
    
    swi3(fragColor,x,y,z) = Tonemap( swi3(fragColor,x,y,z) );
    fragColor.w = 1.0f;


  SetFragmentShaderComputedColor(fragColor);
}