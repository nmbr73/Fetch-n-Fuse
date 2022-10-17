

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Uncomment different defines in Buf B to run different shaders on TV

// Postprocessing Pass
// Motion blur, Depth of Field, Vignetting & Tonemap


#define ENABLE_DOF
#define ENABLE_MOTION_BLUR



vec3 Tonemap( vec3 x )
{
#if 0 
    
    vec3 luminanceCoeffsBT709 = vec3( 0.2126f, 0.7152f, 0.0722f );
    float f = dot( x, luminanceCoeffsBT709 );
    x /= f;        
    f = 1.0f - exp(-f);    
    x *= f;    
    x = mix( x, vec3(f), f*f );
    
    return x;
#else       
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
#endif    
}


float GetVignetting( const in vec2 vUV, float fScale, float fPower, float fStrength )
{
	vec2 vOffset = (vUV - 0.5) * sqrt(2.0) * fScale;
	
	float fDist = max( 0.0, 1.0 - length( vOffset ) );
    
	float fShade = 1.0 - pow( fDist, fPower );
    
    fShade = 1.0 - fShade * fStrength;

	return fShade;
}




float GetCoC( float fDistance, float fPlaneInFocus )
{
#ifdef ENABLE_DOF    
	// http://http.developer.nvidia.com/GPUGems/gpugems_ch23.html

    float fAperture = min(1.0, fPlaneInFocus * fPlaneInFocus * 0.5);
    float fFocalLength = 0.03;
    
	return abs(fAperture * (fFocalLength * (fDistance - fPlaneInFocus)) /
          (fDistance * (fPlaneInFocus - fFocalLength)));  
#else
    return 0.0f;
#endif    
}

// Depth of field pass

#define BLUR_TAPS 64
float fGolden = 3.141592 * (3.0 - sqrt(5.0));

#define MOD2 vec2(4.438975,3.972973)

float Hash( float p ) 
{
    // https://www.shadertoy.com/view/4djSRW - Dave Hoskins
	vec2 p2 = fract(vec2(p) * MOD2);
    p2 += dot(p2.yx, p2.xy+19.19);
	return fract(p2.x * p2.y);    
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    CameraState camCurr;
	Cam_LoadState( camCurr, iChannel0, ivec2(0) );

    CameraState camPrev;
	Cam_LoadState( camPrev, iChannel0, ivec2(3,0) );
    
	vec2 vUV = fragCoord.xy / iResolution.xy;
 	//vUV -= camCurr.vJitter / iResolution.xy;    // TAA has removed jitter

    vec4 vSample = texelFetch( iChannel0, ivec2(fragCoord.xy), 0 ).rgba;
    
    int iObjectId;
    float fDepth = DecodeDepthAndObjectId( vSample.w, iObjectId );
    
    vec3 vRayOrigin, vRayDir;
    
    Cam_GetCameraRay( vUV, iResolution.xy, camCurr, vRayOrigin, vRayDir );    
    vec3 vWorldPos = vRayOrigin + vRayDir * fDepth;
        
    vec2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev), iResolution.xy );// - camPrev.vJitter / iResolution.xy;
        
	vec3 vResult = vec3(0.0);
    
    float fTot = 0.0;
    
    float fPlaneInFocus = camCurr.fPlaneInFocus;
        
	float fCoC = GetCoC( fDepth, camCurr.fPlaneInFocus );
        
    float r = 1.0;
    vec2 vangle = vec2(0.0,fCoC); // Start angle
    
    float fWeight = max( 0.001, fCoC );    
    vResult.rgb = vSample.rgb * fWeight;
    fTot += fWeight;
    
#if defined(ENABLE_DOF) || defined(ENABLE_MOTION_BLUR)    
    float fMotionBlurTaps = float(BLUR_TAPS);
    
    float fShutterAngle = 0.5;
    
    float f = 0.0;
    float fIndex = 0.0;
    for(int i=1; i<BLUR_TAPS; i++)
    {
        float fRandomT = Hash( iTime + fIndex + vUV.x + vUV.y * 12.345);
        float fOrderedT = fIndex / fMotionBlurTaps;
        
        float fDofT = fOrderedT;
        float fMotionT = fRandomT;
        
        vec2 vTapUV = vUV;
        #ifdef ENABLE_MOTION_BLUR
        vTapUV = mix( vTapUV, vPrevUV, (fMotionT - 0.5) * fShutterAngle );
        #endif
                
        // http://blog.marmakoide.org/?p=1
        
        float fTheta = fDofT * fGolden * fMotionBlurTaps;
        float fRadius = fCoC * sqrt( fDofT * fMotionBlurTaps ) / sqrt( fMotionBlurTaps );        
        
        vTapUV += vec2( sin(fTheta), cos(fTheta) ) * fRadius;
        
        vec4 vTapSample = textureLod( iChannel0, vTapUV, 0.0 ).rgba;
	    //vec4 vTapTexel = texelFetch( iChannel0, ivec2(vTapUV.xy * iResolution.xy), 0 ).rgba;
        
        int iTapObjectId;
        float fTapDepth = DecodeDepthAndObjectId( vTapSample.w, iTapObjectId );
        
        if ( fTapDepth > 0.0 )
        {            
  		  	float fCurrCoC = GetCoC( fTapDepth, fPlaneInFocus );
            
            float fCurrWeight = max( 0.001, fCurrCoC );
            
    		vResult += vTapSample.rgb * fCurrWeight;
        	fTot += fCurrWeight;
        }
        f += 1.0;
        fIndex += 1.0;
    }
#endif    
    vResult /= fTot;
    
	fragColor = vec4(vResult, 1.0);    
    
    float fShade = GetVignetting( vUV, 0.7, 2.0, 1.0 );
    
    fragColor.rgb *= fShade;
    
    fragColor.rgb = Tonemap( fragColor.rgb );
    fragColor.a = 1.0;
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Update Logic for Commodore 64 by hubbe
// https://www.shadertoy.com/view/Xs3XW4

// TODO: break

#define CURSOR 0
#define STATE 1
#define MEMORY 2

vec4 old_memory[MEMORY];
vec4 memory[MEMORY];

#define STATE_READY 0
#define STATE_PRINT_READY 1
#define STATE_PRINT_READY_NL 2
#define STATE_LISTING 3
#define STATE_RUNNING 4
#define STATE_BREAK 5

#define LINE_ZERO 30
#define MAX_LINES 200

float vec4pick(int c, vec4 v) {
    if (c == 0) return v.x;
    if (c == 1) return v.y;
    if (c == 2) return v.z;
    return v.w;
}

int vec4toint(int c, vec4 v) {
    c = int(mod(float(c), 8.0));
    float tmp = vec4pick(c / 2, v);
    if (c != (c/2) * 2) {
        return int(mod(tmp, 256.0));
    } else {
        return int(tmp) / 256;
    }
}

vec4 vec4tochar(int c, vec4 v) {
    return vec4(vec4toint(c, v), 14/* fg */, 6 /* bg */, 0);
}


void init_screen(out vec4 fragColor, int x, int y) {
    fragColor = vec4(96, 14, 6, 0);

    if(y == 1) {
        if (x > 3 && x < 35) fragColor.x = 42.0;
        if (x > 7 && x < 31) fragColor.x = 96.0;
        x -= 9;
        vec4 tmp;
        if (x < 0) return;
        if (x > 20) return;
        int n = x / 8;
        if (n == 0) tmp = vec4(0x030F, 0x0D0D, 0x0F04, 0x0F12);  // COMMODOR
        if (n == 1) tmp = vec4(0x0560, 0x3634, 0x6002, 0x0113);  // E 64 BAS
        if (n == 2) tmp = vec4(0x0903, 0x6016, 0x3200, 0x0000);  // IC V2
        fragColor = vec4tochar(x, tmp);
    }
    if (y == 3) {
        int n = x / 8;
        vec4 tmp;
        if (n == 0) tmp = vec4(0x6036, 0x340B, 0x6012, 0x010D); //  64K RAM
        if (n == 1) tmp = vec4(0x6013, 0x1913, 0x1405, 0x0D60); //  SYSTEM 
        if (n == 2) tmp = vec4(0x6033, 0x3839, 0x3131, 0x6002); //  38911 B
        if (n == 3) tmp = vec4(0x0113, 0x0903, 0x6002, 0x1914); // ASIC BYT
        if (n == 4) tmp = vec4(0x0513, 0x6006, 0x1205, 0x0560); // ES FREE
        fragColor = vec4tochar(x, tmp);
    }
}

int key = -1;
int scroll = 0;

void NL() {
   memory[CURSOR].x = 0.0;
   memory[CURSOR].y += 1.0;
   if (memory[CURSOR].y >= 20.0) {
       scroll += 1;
       memory[CURSOR].y -= 1.0;
   }
}

void putc(int c) {
    key = c;
    memory[CURSOR].x += 1.0;
    if (memory[CURSOR].x > 40.0) NL();
}

int screen_pos(vec4 v) {
    int x = int(v.x + 0.5);
    int y = int(v.y + 0.5);
    return x + y * 40;
}

vec4 peek(int x, int y) {
    return texelFetch(iChannel0, ivec2(x, y), 0 );
}

vec4 peek(int pos) {
    int y = pos / 40;
    int x = pos - y * 40;
    return peek(x, y);
}

vec4 itoa(int x, int p) {
	int c = 96;
    int len = 1;
    if (x > 9) len = 2;
    if (x > 99) len = 3;
    if (p < len) {
        int power10 = 1;
        if (len - p == 2) power10 = 10;
        if (len - p == 3) power10 = 100;
        c = 48 + int(mod(float(x / power10), 10.0));        
    }
    return vec4(c, 14, 6, 0);
}

int copy_from;
int copy_to;
int copy_length;

#define MSG_SYNTAX_ERROR -1
#define MSG_READY -2
#define MSG_ZERO -3
#define MSG_BREAK -4

void copy(int pos, inout vec4 tmp) {
    int c = pos - copy_to;
    if (c >= 0 && c < copy_length) {
        tmp = vec4(0,0,0,0);
        if (copy_from == MSG_SYNTAX_ERROR) {
            vec4 ch;
            if (c / 8 == 0)
              ch = vec4(0x3F13, 0x190E, 0x1401, 0x1860);  // ?SYNTAX 
            if (c / 8 == 1)
              ch = vec4(0x6005, 0x1212, 0x0F12, 0x0000);  // ERROR
            tmp = vec4tochar(c, ch);
        } else if (copy_from == MSG_READY) {
            vec4 ch = vec4(0x1205, 0x0104, 0x192E, 0);
            tmp = vec4tochar(c, ch) ; 
        } else if (copy_from == MSG_ZERO) {
            tmp = vec4(0);
        } else if (copy_from == MSG_BREAK) {
            vec4 ch;
            if (c < 8)
              tmp = vec4tochar(c, vec4(0x0212, 0x0501, 0x0B60, 0x090E));  // BREAK IN
            if (c == 8)
              tmp = vec4(96, 14, 6, 0);
            if (c > 8)
              tmp = itoa(int(memory[STATE].y), c - 9);
        } else {
	        tmp = peek(copy_from + c);
            if (tmp.x >= 128.0) tmp.x -= 128.0;
        }
    }
}

void memcpy(int dst, int src, int len) {
    copy_from = src;
    copy_to = dst;
    copy_length = len;
}


void print(int msg, int msg_len) {
    NL();
    memcpy(screen_pos(memory[CURSOR]) - 40, msg, msg_len);
}

void list() {
      memory[STATE].x = float(STATE_LISTING);
      memory[STATE].y = float(0);
}

int getchar(int x, int y) {
    int c = int(peek(x, y).x);
    if (c > 128) c -= 128;
    return c;
}

int getchar(int pos) {
    int c = int(peek(pos).x);
    if (c > 128) c -= 128;
    return c;
}

void skipwhite(inout int pos) {
    int c = getchar(pos);
    if (c == 96) pos = pos + 1;    
    c = getchar(pos);
    if (c == 96) pos = pos + 1;    
    c = getchar(pos);
    if (c == 96) pos = pos + 1;    
}

bool strtod(inout int pos, inout int value) {
  skipwhite(pos);
  int c = getchar(pos);
  int num = c - 48;
  if (num < 0 || num > 9) return false;
  value = num;
  pos = pos + 1;
  c = getchar(pos);
  num = c - 48;
  if (num < 0 || num > 9) return true;
  value = value * 10 + num;
  pos = pos + 1;
  c = getchar(pos);
  num = c - 48;
  if (num < 0 || num > 9) return true;
  value = value * 10 + num;
  return true;  
}

void skipnum(inout int pos) {
    int value;
    strtod(pos, value);
}

void parse(int pos) {
    skipwhite(pos);
    int c1 = getchar(pos);
    int c2 = getchar(pos + 1);
    int c3 = getchar(pos + 2);
    int c4 = getchar(pos + 3);
    if (c1 == 12 && c2 == 9 && c3 == 19 && c4 == 20) { // list
        list();
        
    } else if (c1 == 18 && c2 == 21 && c3 == 14) { // run
        memory[STATE].x = float(STATE_RUNNING);
        int line = 0;
        int p = pos + 3;
        strtod(p, line);
        memory[STATE].y = float(line);
    } else if (c1 == 7 && c2 == 15 && c3 == 20 && c2 == 15) { // goto
        memory[STATE].x = float(STATE_RUNNING);
        int line = 0;
        int p = pos + 4;
        strtod(p, line);
        memory[STATE].y = float(line);
    } else if (c1 == 16 && c2 == 18 && c3 == 9 && c4 == 14) {
        // print
        NL();
        int p = pos + 7;
        int len = 0;
        for (int l = 0; l < 33; l++) {
            if (len == 0 && int(peek(p + l).x) == 34)
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
          memory[STATE].x = float(STATE_PRINT_READY);
        }
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    copy_length = 0;
	int x = int(fragCoord.x);
    int y = int(fragCoord.y);
    if (x > 40 && y > 25) discard;
    
    if (iFrame < 3) {
    	memory[CURSOR] = vec4(0, 5, 0, 0);
        memory[STATE].x = float(STATE_PRINT_READY);
    } else {
	    for (int i = 0; i < MEMORY; i++) {
    		memory[i] = peek(i + 40, 0);
            old_memory[i] = memory[i];
   		}
    } 

    fragColor = peek(x, y);

    if (memory[STATE].x == float(STATE_LISTING)) {
        int line = int(memory[STATE].y);
        memory[STATE].x = float(STATE_PRINT_READY_NL);
        
        for (int i = 0; i < 50; i++) {
            if (getchar(0, LINE_ZERO + line + i) != 0) {
                memory[STATE].x = float(STATE_LISTING);
                memory[STATE].y = float(line + i + 1);
                NL();
                memcpy(screen_pos(memory[CURSOR]) - 40, 40 * (LINE_ZERO + line + i), 40);
                break;
            }
        }
    } else if (memory[STATE].x == float(STATE_RUNNING)) {
        bool esc = texture(iChannel1, vec2(27.5 / 256.0, 0.5/3.0)).x > 0.5;
        if (esc) {
            NL();
            memory[STATE].x = float(STATE_BREAK);
        } else {
           	int line = int(memory[STATE].y);
	        memory[STATE].x = float(STATE_PRINT_READY_NL);
        
    	    for (int i = 0; i < 50; i++) {
        	    if (getchar(0, LINE_ZERO + line + i) != 0) {
            	    memory[STATE].x = float(STATE_RUNNING);
                	memory[STATE].y = float(line + i + 1);
    	            int pos = 40 * (LINE_ZERO + line + i);
	                skipnum(pos);
        	        parse(pos);
            	    break;
        	    }
     	   }
        }
    } else if (memory[STATE].x == float(STATE_BREAK)) {
  		memory[STATE].x = float(STATE_PRINT_READY);
        print(MSG_BREAK, 12);
    } else if (memory[STATE].x == float(STATE_PRINT_READY)) {
  		memory[STATE].x = float(STATE_READY);
        print(MSG_READY, 6);
    } else if (memory[STATE].x == float(STATE_PRINT_READY_NL)) {
  		memory[STATE].x = float(STATE_READY);
        NL();
        print(MSG_READY, 6);
    } else {
 	   bool shift = texture(iChannel1, vec2(16.5 / 256.0, 0.5/3.0)).x > 0.5;

    	for (int key = 0; key < 64; key++) {
        	float key_val = texture(iChannel1, vec2((float(key) + 32.5)/256.0, 0.5)).x;
	        if (key_val > 0.6) {
    	        if (key > 32)
        	        putc(key - 32 + (shift ? 64 : 0));
            	else if (key == 0)
                	putc(96);
	            else if (key >= 16)
    	            putc(key + 32 + (shift ? -16 : 0));
        	}
 	   }
    
  	  if (texture(iChannel1, vec2(13.5/256.0, 0.5)).x > 0.6) {
          int y = int(memory[CURSOR].y);
    	    NL();
     	   parse(y * 40);
  	      // Enter
  	  }
        if (texture(iChannel1, vec2(8.5/256.0, 0.5)).x > 0.6) {
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
        return;
      }
      fragColor = peek(x, y + scroll);
      int sp = x + y * 40;
      
      if (sp + 40 * scroll == screen_pos(old_memory[CURSOR])) {
          fragColor.x = mod(fragColor.x, 128.0);
          if (key != -1)
          {
              fragColor.x = float(key);
          }
      }

      if (sp == screen_pos(memory[CURSOR])) {
          if (fract(iTime) > 0.5) {
            fragColor.x += 128.0;
         }
      }
      copy(sp, fragColor);
      return;
    }
    copy(x + y * 40, fragColor);
    if (x >= 0 && x < 40 && y >= 20 && y <= 25) {
       fragColor = vec4(96, 14, 6, 0);
    }
    if (y == 0) {
 		for (int i = 0; i < MEMORY; i++) {
 	    	if (i + 40 == x) {
				fragColor = memory[i];
            	return;
          	}
        }
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
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

float kFramesPerSecond = 7.5;

#ifdef LOW_RESOLUTION
vec2 kWindowResolution = vec2(256.0, 192.0);
#else
vec2 kWindowResolution = iResolution.xy;
#endif

float kAALineWidth = 1.0;

/////////////////////////////////////
// Time

float GetSceneTime()
{
	#ifdef LIMIT_FRAMERATE
		return (floor(iTime * kFramesPerSecond) / kFramesPerSecond);
	#else
		return iTime;
	#endif
}

/////////////////////////////////////
// Line Rasterization

#ifdef NON_AA_LINES
float RasterizeLine(const in vec2 vPixel, const in vec2 vA, const in vec2 vB)
{
	// vPixel is the centre of the pixel to be rasterized
	
	vec2 vAB = vB - vA;	
	vec2 vAbsAB = abs(vAB);
	float fGradientSelect = step(vAbsAB.y, vAbsAB.x);

	vec2 vAP = vPixel - vA;

	float fAB = mix(vAB.y, vAB.x, fGradientSelect);
	float fAP = mix(vAP.y, vAP.x, fGradientSelect);
	
	// figure out the co-ordinates we intersect the vPixelCentre x or y axis
	float t = fAP / fAB;	
	vec2 vIntersection = vA + (vB - vA) * t;
	vec2 vIntersectionDist = abs(vIntersection - vPixel);
	
	vec2 vResult = step(vIntersectionDist, vec2(0.5));

	// mask out parts of the line beyond the beginning or end
	float fClipSpan = step(t, 1.0) * step(0.0, t);	
	
	// select the x or y axis result based on the gradient of the line
	return mix(vResult.x, vResult.y, fGradientSelect) * fClipSpan;
}
#else
float RasterizeLine(const in vec2 vPixel, const in vec2 vA, const in vec2 vB)
{
	// AA version based on distance to line
	
	// vPixel is the co-ordinate within the pixel to be rasterized
	
	vec2 vAB = vB - vA;	
	vec2 vAP = vPixel - vA;
	
	vec2 vDir = normalize(vAB);
	float fLength = length(vAB);
	
	float t = clamp(dot(vDir, vAP), 0.0, fLength);
	vec2 vClosest = vA + t * vDir;
	
	float fDistToClosest = 1.0 - (length(vClosest - vPixel) / kAALineWidth);

	float i =  clamp(fDistToClosest, 0.0, 1.0);
	
	return sqrt(i);
}
#endif

/////////////////////////////////////
// Matrix Fun

mat4 SetRotTrans( vec3 r, vec3 t )
{
    float a = sin(r.x); float b = cos(r.x); 
    float c = sin(r.y); float d = cos(r.y); 
    float e = sin(r.z); float f = cos(r.z); 

    float ac = a*c;
    float bc = b*c;

    return mat4( d*f,      d*e,       -c, 0.0,
                 ac*f-b*e, ac*e+b*f, a*d, 0.0,
                 bc*f+a*e, bc*e-a*f, b*d, 0.0,
                 t.x,      t.y,      t.z, 1.0 );
}

mat4 SetProjection( float d )
{
    return mat4( 1.0, 0.0, 0.0, 0.0,
				 0.0, 1.0, 0.0, 0.0,
				 0.0, 0.0, 1.0, d,
				 0.0, 0.0, 0.0, 0.0 );
}

mat4 SetWindow( vec2 s, vec2 t )
{
    return mat4( s.x, 0.0, 0.0, 0.0,
				 0.0, s.y, 0.0, 0.0,
				 0.0, 0.0, 1.0, 0.0,
				 t.x, t.y, 0.0, 1.0 );
}

/////////////////////////////////////
// Window Border Setup

const vec2 kWindowMin = vec2(0.1, 0.1);
const vec2 kWindowMax = vec2(0.9, 0.9);
const vec2 kWindowRange = kWindowMax - kWindowMin;

vec2 ScreenUvToWindowPixel(vec2 vUv)
{
	#ifdef LOW_RESOLUTION
		vUv = ((vUv - kWindowMin) / kWindowRange);
	#endif
	return vUv * kWindowResolution;
}

float IsPixelInWindow(vec2 vPixel)
{
	vec2 vResult = step(vPixel, kWindowResolution)
				* step(vec2(0.0), vPixel);
	return min(vResult.x, vResult.y);
}

/////////////////////////////

const int kVertexCount = 30;
vec3 kVertices[kVertexCount];

void SetupVertices()
{
	kVertices[0] = vec3(40, 0.0, 95);
    kVertices[1] = vec3(-40, 0.0, 95);
    kVertices[2] = vec3(00, 32.5, 30);
    kVertices[3] = vec3(-150,-3.8,-10);
    kVertices[4] = vec3(150,-3.8,-10);
    kVertices[5] = vec3(-110, 20,-50);
    kVertices[6] = vec3(110, 20,-50);
    kVertices[7] = vec3(160,-10,-50);
    kVertices[8] = vec3(-160,-10,-50);
    kVertices[9] = vec3(0, 32.5,-50);
    kVertices[10] = vec3(-40,-30,-50);
    kVertices[11] = vec3(40,-30,-50);
    kVertices[12] = vec3(-45, 10,-50);
    kVertices[13] = vec3(-10, 15,-50);
    kVertices[14] = vec3( 10, 15,-50);
    kVertices[15] = vec3(45, 10,-50);      
    kVertices[16] = vec3(45,-15,-50);
    kVertices[17] = vec3(10,-20,-50);
    kVertices[18] = vec3(-10,-20,-50);
    kVertices[19] = vec3(-45,-15,-50);
    kVertices[20] = vec3(-2,-2, 95);
    kVertices[21] = vec3(-2,-2, 112.5);
    kVertices[22] = vec3(-100,-7.5,-50);
    kVertices[23] = vec3(-100, 7.5,-50);
    kVertices[24] = vec3(-110, 0,-50);
    kVertices[25] = vec3( 100, 7.5,-50);
    kVertices[26] = vec3( 110, 0,-50);
    kVertices[27] = vec3( 100,-7.5,-50);
    kVertices[28] = vec3(  0,0, 95);
    kVertices[29] = vec3(  0,0, 112.5);    
}

float BackfaceCull(vec2 A, vec2 B, vec2 C)
{
	vec2 AB = B - A;
	vec2 AC = C - A;
	float c = AB.x * AC.y - AB.y * AC.x;
	return step(c, 0.0);
}

float Accumulate( const float x, const float y )
{
#ifdef XOR_PIXELS
	return x + y;
#else
	return max(x, y);
#endif
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	// get window pixel co-ordinates for centre of current pixel
	vec2 vWindowPixelCords = ScreenUvToWindowPixel(uv);
	vec2 vPixel = floor(vWindowPixelCords) + 0.5;
	
	// Setup Transform
	mat4 mTransform;

	{
		vec3 vRot = vec3(0.1, 0.2, 0.3) * GetSceneTime();
		
		/*if(iMouse.z > 0.0)
		{
			vec2 vUnitMouse = iMouse.xy / iResolution.xy;
			vRot= vec3(vUnitMouse.yx * vec2(1.0, 1.0) + vec2(1.5, 0.5), 0.0) * 3.14159 * 2.0;
		}*/
		
		vec3 vTrans = vec3(0.0, 0.0, 350.0);
		mat4 mRotTrans = SetRotTrans( vRot, vTrans );
		mat4 mProjection = SetProjection( 1.0 );
		mat4 mWindow = SetWindow( vec2(1.0, iResolution.x/iResolution.y) * kWindowResolution, vec2(0.5) * kWindowResolution );
	
		mTransform = mWindow * mProjection * mRotTrans;
	}

	// Transform Vertices to Window Pixel Co-ordinates
	SetupVertices();
	
	vec2 vScrVtx[kVertexCount];
	for(int i=0; i<kVertexCount; i++)
	{
		vec4 vhPos = mTransform * vec4(kVertices[i], 1.0);
		vScrVtx[i] = vhPos.xy / vhPos.w;
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
	
	float fResult = 0.0;
	
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[2]) * max(fFaceVisible[0], fFaceVisible[2]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[4]) * max(fFaceVisible[3], fFaceVisible[4]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[0], vScrVtx[6]) * max(fFaceVisible[2], fFaceVisible[3]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[1], vScrVtx[0]) * max(fFaceVisible[0], fFaceVisible[1]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[1], vScrVtx[10]) * max(fFaceVisible[1], fFaceVisible[7]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[1]) * max(fFaceVisible[0], fFaceVisible[5]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[5]) * max(fFaceVisible[5], fFaceVisible[8]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[2], vScrVtx[9]) * max(fFaceVisible[8], fFaceVisible[9]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[3], vScrVtx[1]) * max(fFaceVisible[6], fFaceVisible[7]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[3], vScrVtx[8]) * max(fFaceVisible[7], fFaceVisible[10]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[4], vScrVtx[6]) * max(fFaceVisible[3], fFaceVisible[11]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[1]) * max(fFaceVisible[5], fFaceVisible[6]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[3]) * max(fFaceVisible[6], fFaceVisible[10]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[5], vScrVtx[8]) * max(fFaceVisible[10], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[6], vScrVtx[2]) * max(fFaceVisible[2], fFaceVisible[9]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[6], vScrVtx[9]) * max(fFaceVisible[9], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[7], vScrVtx[4]) * max(fFaceVisible[4], fFaceVisible[11]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[7], vScrVtx[6]) * max(fFaceVisible[11], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[8], vScrVtx[10]) * max(fFaceVisible[7], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[9], vScrVtx[5]) * max(fFaceVisible[8], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[10], vScrVtx[11]) * max(fFaceVisible[1], fFaceVisible[12]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[11], vScrVtx[0]) * max(fFaceVisible[1], fFaceVisible[4]));
	fResult = Accumulate(fResult, RasterizeLine( vPixel, vScrVtx[11], vScrVtx[7]) * max(fFaceVisible[4], fFaceVisible[12]));

	if(fFaceVisible[13] > 0.0)	
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
	fResult = mod(fResult, 2.0);
	#endif
	
	// Clip pixel to window border
	fResult *= IsPixelInWindow(vPixel);
	
	// Scanline Effect
	#ifdef SCANLINE_EFFECT	
		float fScanlineEffect = cos((vWindowPixelCords.y + 0.5) * 3.1415 * 2.0) * 0.5 + 0.5;
		fResult = (fResult * 0.9 + 0.1) * (fScanlineEffect * 0.2 + 0.8);
	#endif
		
	fragColor = vec4(vec3(fResult),1.0);
}

#endif

#ifdef SPECTRUM

// Screen Image - @P_Malin

//#define LOADING_LOOP
 
vec2 kResolution = vec2(256.0, 192.0);
 
// Border phases

const float kPhaseBlank = 0.0;
const float kPhaseSilent = 1.0;
const float kPhaseHeader = 2.0;
const float kPhaseData = 3.0;
const float kPhaseRunning = 4.0;
 
// Loading phases

const vec3 vTimeSilent1  = vec3(1.0,	5.0,                       kPhaseSilent);
const vec3 vTimeHeader1  = vec3(2.0,  vTimeSilent1.y + 2.0,      kPhaseHeader);
const vec3 vTimeData1    = vec3(3.0,  vTimeHeader1.y + 0.125,    kPhaseData);
 
const vec3 vTimeBlank2   = vec3(4.0,  vTimeData1.y + 1.0,        kPhaseBlank);
const vec3 vTimeSilent2  = vec3(5.0,  vTimeBlank2.y + 2.0,       kPhaseSilent);
const vec3 vTimeHeader2  = vec3(6.0,  vTimeSilent2.y + 2.0,      kPhaseHeader);
const vec3 vTimeData2    = vec3(7.0,  vTimeHeader2.y + 1.0,      kPhaseData);
 
const vec3 vTimeSilent3  = vec3(8.0,  vTimeData2.y + 2.0,        kPhaseSilent);
const vec3 vTimeHeader3  = vec3(9.0,  vTimeSilent3.y + 2.0,      kPhaseHeader);
const vec3 vTimeData3    = vec3(10.0, vTimeHeader3.y + 0.125,    kPhaseData);
 
const vec3 vTimeSilent4  = vec3(11.0, vTimeData3.y + 2.0,        kPhaseSilent);
const vec3 vTimeHeader4  = vec3(12.0, vTimeSilent4.y + 2.0,      kPhaseHeader);
const vec3 vTimeData4    = vec3(13.0, vTimeHeader4.y + 38.0,     kPhaseData);
 
const vec3 vTimeRunning  = vec3(14.0, vTimeData4.y + 10.0,       kPhaseRunning);
 
const vec3 vTimeTotal    = vec3(15.0, vTimeRunning.y,            kPhaseBlank);
       
vec4 GetPhase(float fTime)
{             
        vec3 vResult = vTimeRunning;
                
        vResult = mix( vResult, vTimeData4, step(fTime, vTimeData4.y ) );
        vResult = mix( vResult, vTimeHeader4, step(fTime, vTimeHeader4.y ) );
        vResult = mix( vResult, vTimeSilent4, step(fTime, vTimeSilent4.y ) );
 
        vResult = mix( vResult, vTimeData3, step(fTime, vTimeData3.y ) );
        vResult = mix( vResult, vTimeHeader3, step(fTime, vTimeHeader3.y ) );
        vResult = mix( vResult, vTimeSilent3, step(fTime, vTimeSilent3.y ) );
               
        vResult = mix( vResult, vTimeData2, step(fTime, vTimeData2.y ) );
        vResult = mix( vResult, vTimeHeader2, step(fTime, vTimeHeader2.y ) );
        vResult = mix( vResult, vTimeSilent2, step(fTime, vTimeSilent2.y ) );
        vResult = mix( vResult, vTimeBlank2, step(fTime, vTimeBlank2.y ) );
 
        vResult = mix( vResult, vTimeData1, step(fTime, vTimeData1.y ) );
        vResult = mix( vResult, vTimeHeader1, step(fTime, vTimeHeader1.y ) );
        vResult = mix( vResult, vTimeSilent1, step(fTime, vTimeSilent1.y ) );
               
        return vec4(vResult.z, vResult.x, fTime - vResult.y, vResult.y);
}
 
float GetRasterPosition(in vec2 fragCoord)
{
        return (fragCoord.x + fragCoord.y * iResolution.x) / (iResolution.x * iResolution.y);
}
 
float IsBorder(vec2 vScreenUV)
{
        if(vScreenUV.x < 0.0)
                        return 1.0;
        if(vScreenUV.x >= 1.0)
                        return 1.0;
        if(vScreenUV.y < 0.0)
                        return 1.0;
        if(vScreenUV.y >= 1.0)
                        return 1.0;
       
        return 0.0;
}
 
 
vec3 GetBorderColour(float fPhase,in vec2 fragCoord)
{
	float raster = GetRasterPosition(fragCoord);
	
	vec3 vCol = vec3(0.0);
	
	if(fPhase == kPhaseBlank)
	{                       
		vCol = vec3(1.0);           
	}
	else  
	if(fPhase == kPhaseSilent)
	{
		float fBlend = step(fract(iTime * 0.5), 0.5);
		vCol = mix( vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 0.0), fBlend);           
	}
	else
	if(fPhase == kPhaseHeader)
	{
		float fBarSize = 12.0;
		float fScrollSpeed = 10.0;
		float fBlend = step(fract(raster * fBarSize + iTime * fScrollSpeed), 0.5);
		vCol = mix( vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 0.0), fBlend);           
	}
	else
	if(fPhase == kPhaseData)
	{
		float fBarSize = 25.0;
		float fScrollSpeed = 1.0;
		float fBlend = step(fract(raster * fBarSize + iTime * fScrollSpeed + sin(iTime * 20.0 + raster * 16.0)), 0.5);
		vCol = mix(vec3(1.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), fBlend);                     
	}
	
	return vCol;
}
 
 
float GetLoadingScreenIntensity( vec2 vPos )
{
	vec2 vUV = vPos / kResolution;
	float r = 0.25;
	vec2 vDist = (vUV - 0.5) / r;
	float len = length(vDist);
	vec3 vNormal = vec3(vDist.x, sqrt(1.0 - len * len), vDist.y);
	vec3 vLight = normalize( vec3(1.0, 1.0, -1.0) );
	if(len < 1.0)
	{
		return max(0.0, dot(vNormal, vLight));
	}
	
	return 0.7 - vUV.y * 0.6;
}
 
float CrossHatch(float fIntensity, vec2 vPos)
{
	vec2 vGridPos = mod(vPos, 4.0);
	
	float fThreshold = fract(vGridPos.x * 0.25 + vGridPos.y * 0.5) * 0.75 + fract(vGridPos.y * 0.25 + vGridPos.x * 0.5) * 0.25;
	
	return step(fIntensity, fThreshold);
}
 
float GetLoadingScreenPixel( vec2 vPos )
{
        return CrossHatch(GetLoadingScreenIntensity(vPos), vPos);
}
 
vec2 GetScreenPixelCoord( vec2 vScreenUV )
{
        vec2 vPixelPos = floor(vScreenUV * kResolution);
        vPixelPos.y = 192.0 - vPixelPos.y;
       
        return vPixelPos;
}
 
float PixelAddress( vec2 vPixelPos )
{               
        float fBand = floor(vPixelPos.y / 64.0);
       
        float fBandPos = mod(vPixelPos.y, 64.0);
 
        float fCharRow = mod(fBandPos, 8.0);
        float fCharPos = floor(fBandPos / 8.0);
 
        float fBytePos = floor(vPixelPos.x / 8.0);
 
        float fLineTime = fBand * 64.0 + fCharRow * 8.0 + fCharPos;
        return (fBytePos + fLineTime * (256.0 / 8.0));
}
 
float AttributeAddress(vec2 vCharPos)
{             
	float kAttributeStart = 256.0 * 192.0 / 8.0;
	return kAttributeStart + vCharPos.x + vCharPos.y * 32.0;
}
 
float GetCharByte(const in float value)
{
        float result = 0.0;
        result = mix(result, 0.0, step(value, 919.0) );
        result = mix(result, 32.0, step(value, 918.5) );
        result = mix(result, 28.0, step(value, 914.5) );
        result = mix(result, 0.0, step(value, 913.5) );
        result = mix(result, 56.0, step(value, 894.5) );
        result = mix(result, 68.0, step(value, 893.5) );
        result = mix(result, 56.0, step(value, 890.5) );
        result = mix(result, 0.0, step(value, 889.5) );
        result = mix(result, 84.0, step(value, 878.5) );
        result = mix(result, 104.0, step(value, 874.5) );
        result = mix(result, 0.0, step(value, 873.5) );
        result = mix(result, 56.0, step(value, 851.5) );
        result = mix(result, 4.0, step(value, 830.5) );
        result = mix(result, 60.0, step(value, 829.5) );
        result = mix(result, 68.0, step(value, 828.5) );
        result = mix(result, 60.0, step(value, 826.5) );
        result = mix(result, 0.0, step(value, 825.5) );
        result = mix(result, 60.0, step(value, 782.5) );
        result = mix(result, 68.0, step(value, 781.5) );
        result = mix(result, 60.0, step(value, 780.5) );
        result = mix(result, 4.0, step(value, 779.5) );
        result = mix(result, 56.0, step(value, 778.5) );
        result = mix(result, 0.0, step(value, 777.5) );
        result = mix(result, 60.0, step(value, 670.5) );
        result = mix(result, 66.0, step(value, 669.5) );
        result = mix(result, 2.0, step(value, 668.5) );
        result = mix(result, 60.0, step(value, 667.5) );
        result = mix(result, 64.0, step(value, 666.5) );
        result = mix(result, 60.0, step(value, 665.5) );
        result = mix(result, 0.0, step(value, 664.5) );
        result = mix(result, 64.0, step(value, 646.5) );
        result = mix(result, 124.0, step(value, 644.5) );
        result = mix(result, 66.0, step(value, 643.5) );
        result = mix(result, 124.0, step(value, 641.5) );
        result = mix(result, 0.0, step(value, 640.5) );
        result = mix(result, 60.0, step(value, 638.5) );
        result = mix(result, 66.0, step(value, 637.5) );
        result = mix(result, 60.0, step(value, 633.5) );
        result = mix(result, 0.0, step(value, 632.5) );
        result = mix(result, 66.0, step(value, 630.5) );
        result = mix(result, 70.0, step(value, 629.5) );
        result = mix(result, 74.0, step(value, 628.5) );
        result = mix(result, 82.0, step(value, 627.5) );
        result = mix(result, 98.0, step(value, 626.5) );
        result = mix(result, 66.0, step(value, 625.5) );
        result = mix(result, 0.0, step(value, 624.5) );
        result = mix(result, 126.0, step(value, 614.5) );
        result = mix(result, 64.0, step(value, 613.5) );
        result = mix(result, 0.0, step(value, 608.5) );
        result = mix(result, 62.0, step(value, 590.5) );
        result = mix(result, 8.0, step(value, 589.5) );
        result = mix(result, 62.0, step(value, 585.5) );
        result = mix(result, 0.0, step(value, 584.5) );
        result = mix(result, 60.0, step(value, 574.5) );
        result = mix(result, 66.0, step(value, 573.5) );
        result = mix(result, 78.0, step(value, 572.5) );
        result = mix(result, 64.0, step(value, 571.5) );
        result = mix(result, 66.0, step(value, 570.5) );
        result = mix(result, 60.0, step(value, 569.5) );
        result = mix(result, 0.0, step(value, 568.5) );
        result = mix(result, 120.0, step(value, 550.5) );
        result = mix(result, 68.0, step(value, 549.5) );
        result = mix(result, 66.0, step(value, 548.5) );
        result = mix(result, 68.0, step(value, 546.5) );
        result = mix(result, 120.0, step(value, 545.5) );
        result = mix(result, 0.0, step(value, 544.5) );
        result = mix(result, 66.0, step(value, 526.5) );
        result = mix(result, 126.0, step(value, 524.5) );
        result = mix(result, 66.0, step(value, 523.5) );
        result = mix(result, 60.0, step(value, 521.5) );
        result = mix(result, 0.0, step(value, 520.5) );
        result = mix(result, 16.0, step(value, 470.5) );
        result = mix(result, 0.0, step(value, 469.5) );
        result = mix(result, 16.0, step(value, 467.5) );
        result = mix(result, 0.0, step(value, 466.5) );
        return result;   
}
 
float GetBit( float fByte, float fBit )
{
        return mod(floor(fByte / pow(2.0, 7.0-fBit)), 2.0) ;
}
 
float GetCharPixel( float fChar, vec2 vPos )
{
        float fCharAddress = fChar * 8.0 + vPos.y;
       
        float fCharBin = GetCharByte(fCharAddress);
       
        return GetBit(fCharBin, vPos.x);
}
 
float GetProgramStringChar(float fPos)
{
        float fChar = 32.0;    
        fChar = mix(fChar, 76.0, step(fPos, 12.5) );
        fChar = mix(fChar, 83.0, step(fPos, 11.5) );
        fChar = mix(fChar, 76.0, step(fPos, 10.5) );
        fChar = mix(fChar, 71.0, step(fPos, 9.5) );
        fChar = mix(fChar, 32.0, step(fPos, 8.5) );
        fChar = mix(fChar, 58.0, step(fPos, 7.5) );
        fChar = mix(fChar, 109.0, step(fPos, 6.5) );
        fChar = mix(fChar, 97.0, step(fPos, 5.5) );
        fChar = mix(fChar, 114.0, step(fPos, 4.5) );
        fChar = mix(fChar, 103.0, step(fPos, 3.5) );
        fChar = mix(fChar, 111.0, step(fPos, 2.5) );
        fChar = mix(fChar, 114.0, step(fPos, 1.5) );
        fChar = mix(fChar, 80.0, step(fPos, 0.5) );
        return fChar;
}
 
float GetLoadingStringChar(float fPos)
{
        float fChar = 32.0;    
        fChar = mix(fChar, 76.0, step(fPos, 11.0) );
        fChar = mix(fChar, 83.0, step(fPos, 10.5) );
        fChar = mix(fChar, 76.0, step(fPos, 9.5) );
        fChar = mix(fChar, 71.0, step(fPos, 8.5) );
        fChar = mix(fChar, 32.0, step(fPos, 7.5) );
        fChar = mix(fChar, 71.0, step(fPos, 6.5) );
        fChar = mix(fChar, 78.0, step(fPos, 5.5) );
        fChar = mix(fChar, 73.0, step(fPos, 4.5) );
        fChar = mix(fChar, 68.0, step(fPos, 3.5) );
        fChar = mix(fChar, 65.0, step(fPos, 2.5) );
        fChar = mix(fChar, 79.0, step(fPos, 1.5) );
        fChar = mix(fChar, 76.0, step(fPos, 0.5) );
        return fChar;
}
 
float GetProgramText(vec2 vPixelPos)
{     
        vec2 vCharCoord = floor(vPixelPos / 8.0);
       
        float fChar = GetProgramStringChar(vCharCoord.x);
       
        if(vCharCoord.y != 0.0)
                fChar = 32.0;
       
        return GetCharPixel(fChar, mod(vPixelPos, 8.0));
}
 
float GetLoadingText(vec2 vPixelPos)
{     
        vec2 vCharCoord = floor(vPixelPos / 8.0);
       
        float fChar = GetLoadingStringChar(vCharCoord.x);
       
        float inString = 1.0;
        if(vCharCoord.x < 0.0)
                fChar = 32.0;
       
        if(vCharCoord.y != 0.0)
                fChar = 32.0;
       
        return GetCharPixel(fChar, mod(vPixelPos, 8.0));
}
 
float GetScreenPixel(vec2 vScreenPixel)
{
	// plasma thing
	float f = sin(vScreenPixel.x *0.0432 + sin(vScreenPixel.y * 0.0423)+ iTime * 3.0);
	f = f + sin(vScreenPixel.y * 0.0454513 + sin(vScreenPixel.x * 0.07213) + iTime * 5.0);
	f = f + sin(vScreenPixel.x * 0.043353 + sin(vScreenPixel.y * 0.043413) + iTime * 8.0);
	f = f + sin(vScreenPixel.y * 0.0443513 + sin(vScreenPixel.x * 0.036313) + iTime * 10.0);
	f = f * 0.125 + 0.5;
	
	return CrossHatch(f, vScreenPixel);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{           
	float fSequenceTime = iTime;
	
	#ifdef LOADING_LOOP
	fSequenceTime = mod(fSequenceTime, vTimeTotal.y);
	#endif
	
	vec3 col = vec3(1.0);
	
	vec4 vPhase = GetPhase(fSequenceTime);
	
	vec2 vUV = ( fragCoord.xy / iResolution.xy );
	vec2 vScreenUV = (vUV - 0.1) / 0.8;
	if(IsBorder(vScreenUV) > 0.0)
	{
		col = GetBorderColour(vPhase.x, fragCoord);
	}
	else
	{
		vec2 vScreenCoord = GetScreenPixelCoord(vScreenUV);
		vec2 vAttribCoord = floor(vScreenCoord / 8.0);

		float fPixelValue = 0.0;
		vec3 vInk = vec3(0.0);
		vec3 vPaper = vec3(1.0);
		
		if(vPhase.x != kPhaseRunning)
		{
			// loading
			float fLoadScreenTime = fSequenceTime - vTimeHeader4.y;
										       
			float fAddressLoaded = fLoadScreenTime * 192.0;
			if(PixelAddress(vScreenCoord) > fAddressLoaded)
			{
				if(vPhase.y < 4.0)
				{
					col = vec3(1.0);
				}
				else
				if(vPhase.y < 8.0)
				{
					vec2 vTextPos = vec2(0.0, 8.0);
					fPixelValue = GetProgramText(vScreenCoord - vTextPos);
				}
				else
				{
					vec2 vTextPos = vec2(10.0 * 8.0, 19.0 * 8.0);
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
				vInk = vec3(0.0, 0.0, 1.0);
				vPaper = vec3(1.0, 1.0, 0.0);
			}		
		}
		else
		{
			// running
			fPixelValue = GetScreenPixel(vScreenCoord);
			
			vec2 vTextPos = vec2(-8.0 * 8.0, 8.0);
			float fAttribValue = GetLoadingText(vAttribCoord - vTextPos );
			vPaper = mix(vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 0.0), fAttribValue);
			vInk = vec3(0.0, 0.0, 1.0);
		}     
	
		//fPixelValue = GetScreenPixel(vScreenCoord); // force final effect			
		//fPixelValue = GetLoadingScreenPixel( vScreenCoord); // force loading screen
	
		col = mix(vPaper, vInk, fPixelValue);
		
	}

	float kBrightness = 0.8;
	fragColor = vec4( col * kBrightness, 1.0 );  
}

#endif

#ifdef C64
highp vec4 font2(int c) {
  vec4 v = vec4(0);
  v=mix(v, vec4(0x3c66, 0x6e6e, 0x6062, 0x3c00), step(-0.500, float(c)));
  v=mix(v, vec4(0x183c, 0x667e, 0x6666, 0x6600), step(0.500, float(c)));
  v=mix(v, vec4(0x7c66, 0x667c, 0x6666, 0x7c00), step(1.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x6060, 0x6066, 0x3c00), step(2.500, float(c)));
  v=mix(v, vec4(0x786c, 0x6666, 0x666c, 0x7800), step(3.500, float(c)));
  v=mix(v, vec4(0x7e60, 0x6078, 0x6060, 0x7e00), step(4.500, float(c)));
  v=mix(v, vec4(0x7e60, 0x6078, 0x6060, 0x6000), step(5.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x606e, 0x6666, 0x3c00), step(6.500, float(c)));
  v=mix(v, vec4(0x6666, 0x667e, 0x6666, 0x6600), step(7.500, float(c)));
  v=mix(v, vec4(0x3c18, 0x1818, 0x1818, 0x3c00), step(8.500, float(c)));
  v=mix(v, vec4(0x1e0c, 0xc0c, 0xc6c, 0x3800), step(9.500, float(c)));
  v=mix(v, vec4(0x666c, 0x7870, 0x786c, 0x6600), step(10.500, float(c)));
  v=mix(v, vec4(0x6060, 0x6060, 0x6060, 0x7e00), step(11.500, float(c)));
  v=mix(v, vec4(0x6377, 0x7f6b, 0x6363, 0x6300), step(12.500, float(c)));
  v=mix(v, vec4(0x6676, 0x7e7e, 0x6e66, 0x6600), step(13.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x6666, 0x6666, 0x3c00), step(14.500, float(c)));
  v=mix(v, vec4(0x7c66, 0x667c, 0x6060, 0x6000), step(15.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x6666, 0x663c, 0xe00), step(16.500, float(c)));
  v=mix(v, vec4(0x7c66, 0x667c, 0x786c, 0x6600), step(17.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x603c, 0x666, 0x3c00), step(18.500, float(c)));
  v=mix(v, vec4(0x7e18, 0x1818, 0x1818, 0x1800), step(19.500, float(c)));
  v=mix(v, vec4(0x6666, 0x6666, 0x6666, 0x3c00), step(20.500, float(c)));
  v=mix(v, vec4(0x6666, 0x6666, 0x663c, 0x1800), step(21.500, float(c)));
  v=mix(v, vec4(0x6363, 0x636b, 0x7f77, 0x6300), step(22.500, float(c)));
  v=mix(v, vec4(0x6666, 0x3c18, 0x3c66, 0x6600), step(23.500, float(c)));
  v=mix(v, vec4(0x6666, 0x663c, 0x1818, 0x1800), step(24.500, float(c)));
  v=mix(v, vec4(0x7e06, 0xc18, 0x3060, 0x7e00), step(25.500, float(c)));
  v=mix(v, vec4(0x3c30, 0x3030, 0x3030, 0x3c00), step(26.500, float(c)));
  v=mix(v, vec4(0xc12, 0x307c, 0x3062, 0xfc00), step(27.500, float(c)));
  v=mix(v, vec4(0x3c0c, 0xc0c, 0xc0c, 0x3c00), step(28.500, float(c)));
  v=mix(v, vec4(0x18, 0x3c7e, 0x1818, 0x1818), step(29.500, float(c)));
  v=mix(v, vec4(0x10, 0x307f, 0x7f30, 0x1000), step(30.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x0, 0x0), step(31.500, float(c)));
  v=mix(v, vec4(0x1818, 0x1818, 0x0, 0x1800), step(32.500, float(c)));
  v=mix(v, vec4(0x6666, 0x6600, 0x0, 0x0), step(33.500, float(c)));
  v=mix(v, vec4(0x6666, 0xff66, 0xff66, 0x6600), step(34.500, float(c)));
  v=mix(v, vec4(0x183e, 0x603c, 0x67c, 0x1800), step(35.500, float(c)));
  v=mix(v, vec4(0x6266, 0xc18, 0x3066, 0x4600), step(36.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x3c38, 0x6766, 0x3f00), step(37.500, float(c)));
  v=mix(v, vec4(0x60c, 0x1800, 0x0, 0x0), step(38.500, float(c)));
  v=mix(v, vec4(0xc18, 0x3030, 0x3018, 0xc00), step(39.500, float(c)));
  v=mix(v, vec4(0x3018, 0xc0c, 0xc18, 0x3000), step(40.500, float(c)));
  v=mix(v, vec4(0x66, 0x3cff, 0x3c66, 0x0), step(41.500, float(c)));
  v=mix(v, vec4(0x18, 0x187e, 0x1818, 0x0), step(42.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x18, 0x1830), step(43.500, float(c)));
  v=mix(v, vec4(0x0, 0x7e, 0x0, 0x0), step(44.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x18, 0x1800), step(45.500, float(c)));
  v=mix(v, vec4(0x3, 0x60c, 0x1830, 0x6000), step(46.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x6e76, 0x6666, 0x3c00), step(47.500, float(c)));
  v=mix(v, vec4(0x1818, 0x3818, 0x1818, 0x7e00), step(48.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x60c, 0x3060, 0x7e00), step(49.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x61c, 0x666, 0x3c00), step(50.500, float(c)));
  v=mix(v, vec4(0x60e, 0x1e66, 0x7f06, 0x600), step(51.500, float(c)));
  v=mix(v, vec4(0x7e60, 0x7c06, 0x666, 0x3c00), step(52.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x607c, 0x6666, 0x3c00), step(53.500, float(c)));
  v=mix(v, vec4(0x7e66, 0xc18, 0x1818, 0x1800), step(54.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x663c, 0x6666, 0x3c00), step(55.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x663e, 0x666, 0x3c00), step(56.500, float(c)));
  v=mix(v, vec4(0x0, 0x1800, 0x18, 0x0), step(57.500, float(c)));
  v=mix(v, vec4(0x0, 0x1800, 0x18, 0x1830), step(58.500, float(c)));
  v=mix(v, vec4(0xe18, 0x3060, 0x3018, 0xe00), step(59.500, float(c)));
  v=mix(v, vec4(0x0, 0x7e00, 0x7e00, 0x0), step(60.500, float(c)));
  v=mix(v, vec4(0x7018, 0xc06, 0xc18, 0x7000), step(61.500, float(c)));
  v=mix(v, vec4(0x3c66, 0x60c, 0x1800, 0x1800), step(62.500, float(c)));
  v=mix(v, vec4(0x0, 0xff, 0xff00, 0x0), step(63.500, float(c)));
  v=mix(v, vec4(0x81c, 0x3e7f, 0x7f1c, 0x3e00), step(64.500, float(c)));
  v=mix(v, vec4(0x1818, 0x1818, 0x1818, 0x1818), step(65.500, float(c)));
  v=mix(v, vec4(0x0, 0xff, 0xff00, 0x0), step(66.500, float(c)));
  v=mix(v, vec4(0x0, 0xffff, 0x0, 0x0), step(67.500, float(c)));
  v=mix(v, vec4(0xff, 0xff00, 0x0, 0x0), step(68.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xffff, 0x0), step(69.500, float(c)));
  v=mix(v, vec4(0x3030, 0x3030, 0x3030, 0x3030), step(70.500, float(c)));
  v=mix(v, vec4(0xc0c, 0xc0c, 0xc0c, 0xc0c), step(71.500, float(c)));
  v=mix(v, vec4(0x0, 0xe0, 0xf038, 0x1818), step(72.500, float(c)));
  v=mix(v, vec4(0x1818, 0x1c0f, 0x700, 0x0), step(73.500, float(c)));
  v=mix(v, vec4(0x1818, 0x38f0, 0xe000, 0x0), step(74.500, float(c)));
  v=mix(v, vec4(0xc0c0, 0xc0c0, 0xc0c0, 0xffff), step(75.500, float(c)));
  v=mix(v, vec4(0xc0e0, 0x7038, 0x1c0e, 0x703), step(76.500, float(c)));
  v=mix(v, vec4(0x307, 0xe1c, 0x3870, 0xe0c0), step(77.500, float(c)));
  v=mix(v, vec4(0xffff, 0xc0c0, 0xc0c0, 0xc0c0), step(78.500, float(c)));
  v=mix(v, vec4(0xffff, 0x303, 0x303, 0x303), step(79.500, float(c)));
  v=mix(v, vec4(0x3c, 0x7e7e, 0x7e7e, 0x3c00), step(80.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xff, 0xff00), step(81.500, float(c)));
  v=mix(v, vec4(0x367f, 0x7f7f, 0x3e1c, 0x800), step(82.500, float(c)));
  v=mix(v, vec4(0x6060, 0x6060, 0x6060, 0x6060), step(83.500, float(c)));
  v=mix(v, vec4(0x0, 0x7, 0xf1c, 0x1818), step(84.500, float(c)));
  v=mix(v, vec4(0xc3e7, 0x7e3c, 0x3c7e, 0xe7c3), step(85.500, float(c)));
  v=mix(v, vec4(0x3c, 0x7e66, 0x667e, 0x3c00), step(86.500, float(c)));
  v=mix(v, vec4(0x1818, 0x6666, 0x1818, 0x3c00), step(87.500, float(c)));
  v=mix(v, vec4(0x606, 0x606, 0x606, 0x606), step(88.500, float(c)));
  v=mix(v, vec4(0x81c, 0x3e7f, 0x3e1c, 0x800), step(89.500, float(c)));
  v=mix(v, vec4(0x1818, 0x18ff, 0xff18, 0x1818), step(90.500, float(c)));
  v=mix(v, vec4(0xc0c0, 0x3030, 0xc0c0, 0x3030), step(91.500, float(c)));
  v=mix(v, vec4(0x1818, 0x1818, 0x1818, 0x1818), step(92.500, float(c)));
  v=mix(v, vec4(0x0, 0x33e, 0x7636, 0x3600), step(93.500, float(c)));
  v=mix(v, vec4(0xff7f, 0x3f1f, 0xf07, 0x301), step(94.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x0, 0x0), step(95.500, float(c)));
  v=mix(v, vec4(0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0), step(96.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xffff, 0xffff), step(97.500, float(c)));
  v=mix(v, vec4(0xff00, 0x0, 0x0, 0x0), step(98.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x0, 0xff), step(99.500, float(c)));
  v=mix(v, vec4(0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0), step(100.500, float(c)));
  v=mix(v, vec4(0xcccc, 0x3333, 0xcccc, 0x3333), step(101.500, float(c)));
  v=mix(v, vec4(0x303, 0x303, 0x303, 0x303), step(102.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xcccc, 0x3333), step(103.500, float(c)));
  v=mix(v, vec4(0xfffe, 0xfcf8, 0xf0e0, 0xc080), step(104.500, float(c)));
  v=mix(v, vec4(0x303, 0x303, 0x303, 0x303), step(105.500, float(c)));
  v=mix(v, vec4(0x1818, 0x181f, 0x1f18, 0x1818), step(106.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xf0f, 0xf0f), step(107.500, float(c)));
  v=mix(v, vec4(0x1818, 0x181f, 0x1f00, 0x0), step(108.500, float(c)));
  v=mix(v, vec4(0x0, 0xf8, 0xf818, 0x1818), step(109.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0x0, 0xffff), step(110.500, float(c)));
  v=mix(v, vec4(0x0, 0x1f, 0x1f18, 0x1818), step(111.500, float(c)));
  v=mix(v, vec4(0x1818, 0x18ff, 0xff00, 0x0), step(112.500, float(c)));
  v=mix(v, vec4(0x0, 0xff, 0xff18, 0x1818), step(113.500, float(c)));
  v=mix(v, vec4(0x1818, 0x18f8, 0xf818, 0x1818), step(114.500, float(c)));
  v=mix(v, vec4(0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0), step(115.500, float(c)));
  v=mix(v, vec4(0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0), step(116.500, float(c)));
  v=mix(v, vec4(0x707, 0x707, 0x707, 0x707), step(117.500, float(c)));
  v=mix(v, vec4(0xffff, 0x0, 0x0, 0x0), step(118.500, float(c)));
  v=mix(v, vec4(0xffff, 0xff00, 0x0, 0x0), step(119.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xff, 0xffff), step(120.500, float(c)));
  v=mix(v, vec4(0x303, 0x303, 0x303, 0xffff), step(121.500, float(c)));
  v=mix(v, vec4(0x0, 0x0, 0xf0f0, 0xf0f0), step(122.500, float(c)));
  v=mix(v, vec4(0xf0f, 0xf0f, 0x0, 0x0), step(123.500, float(c)));
  v=mix(v, vec4(0x1818, 0x18f8, 0xf800, 0x0), step(124.500, float(c)));
  v=mix(v, vec4(0xf0f0, 0xf0f0, 0x0, 0x0), step(125.500, float(c)));
  v=mix(v, vec4(0xf0f0, 0xf0f0, 0xf0f, 0xf0f), step(126.500, float(c)));
  return v;
}

highp vec4 font(int c) {
    if (c < 128) return font2(c);
    return vec4(0xffff) - font2(c - 128);
}

vec4 colors(int c) {
    if (c ==  0) return vec4(0x00,0x00,0x00,1);
    if (c ==  1) return vec4(0xFF,0xFF,0xFF,1);
    if (c ==  2) return vec4(0x68,0x37,0x2B,1);
    if (c ==  3) return vec4(0x70,0xA4,0xB2,1);
    if (c ==  4) return vec4(0x6F,0x3D,0x86,1);
    if (c ==  5) return vec4(0x58,0x8D,0x43,1);
    if (c ==  6) return vec4(0x35,0x28,0x79,1);
    if (c ==  7) return vec4(0xB8,0xC7,0x6F,1);
    if (c ==  8) return vec4(0x6F,0x4F,0x25,1);
    if (c ==  9) return vec4(0x43,0x39,0x00,1);
    if (c == 10) return vec4(0x9A,0x67,0x59,1);
    if (c == 11) return vec4(0x44,0x44,0x44,1);
    if (c == 12) return vec4(0x6C,0x6C,0x6C,1);
    if (c == 13) return vec4(0x9A,0xD2,0x84,1);
    if (c == 14) return vec4(0x6C,0x5E,0xB5,1);
    if (c == 15) return vec4(0x95,0x95,0x95,1);
    return vec4(0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
	vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 1.1 - 0.05;
    if ( any( lessThan( uv, vec2(0) ) ) || any( greaterThanEqual( uv, vec2(1) ) ) )
    {
        fragColor = colors(14) / 180.0;
        return;
    }
    vec2 sz = vec2(40.0, 20.0);
    vec2 fb_pos = floor(uv * sz) + vec2(0.5, 0.5);
    fb_pos.y = sz.y - fb_pos.y;
    fb_pos /= iResolution.xy;
    
	vec4 fb = texture(iChannel0, fb_pos);
    highp vec4 char = font(int(fb.x));

    vec2 p = mod(uv * sz * 8.0, 8.0);
	int line = 7 - int(p.y);
    highp float pixels = 0.0;
    if (line == 0) pixels = char.x / 256.0;
    if (line == 1) pixels = char.x;
    if (line == 2) pixels = char.y / 256.0;
    if (line == 3) pixels = char.y;
    if (line == 4) pixels = char.z / 256.0;
    if (line == 5) pixels = char.z;
    if (line == 6) pixels = char.w / 256.0;
    if (line == 7) pixels = char.w;

    if (mod(pixels * pow(2.0, floor(p.x)), 256.0) > 127.5) {
        fragColor = colors(int(fb.y)) / 180.0;
    } else {
        fragColor = colors(int(fb.z)) / 180.0;
    }
}
#endif

#ifdef MANDELBROT

///////////////////////////
// Keyboard
///////////////////////////

const float KEY_SPACE = 32.5/256.0;
const float KEY_LEFT  = 37.5/256.0;
const float KEY_UP    = 38.5/256.0;
const float KEY_RIGHT = 39.5/256.0;
const float KEY_DOWN  = 40.5/256.0;

const float KEY_PLUS 	= 187.5/256.0;
const float KEY_MINUS  	= 189.5/256.0;

bool Key_IsPressed(float key)
{
    return texture( iChannel1, vec2(key, 0.0) ).x > 0.0;
}

bool Key_IsToggled(float key)
{
    return texture( iChannel1, vec2(key, 1.0) ).x > 0.0;
}

///////////////////////////


float VGARainbowChannel( float i, float a, float b, float c, float d, float e )
{    
    if ( i >= 8.0 ) i = 16.0 - i;
    if ( i <= 0.0 ) return a;
    if ( i == 1.0 ) return b;
    if ( i == 2.0 ) return c;
    if ( i == 3.0 ) return d;
    if ( i >= 4.0 ) return e;
    return a;
}

vec3 VGARainbow( float i, float a, float e )
{
    vec3 vi = mod( vec3( i ) + vec3(0,16,8), vec3(24) );

    float b = floor(a * 3./4. + e * 1.0 / 4.0 + 0.25);
    float c = floor(a * 2./4. + e * 2.0 / 4.0 + 0.25);
    float d = floor(a * 1./4. + e * 3.0 / 4.0 + 0.25);
    
    vec3 col;
    col.r = VGARainbowChannel( vi.r, a, b, c, d, e );
    col.g = VGARainbowChannel( vi.g, a, b, c, d, e );
    col.b = VGARainbowChannel( vi.b, a, b, c, d, e );

    return col;
}

vec3 VGAPaletteEntry( float i )
{
    i = floor( i );
    
    // EGA
    if ( i < 16.0 )
    {
        vec3 col;
        col.b  = floor( mod( i / 1.0, 2.0  )) * 2.0;
        col.g  = floor( mod( i / 2.0, 2.0  )) * 2.0;
        col.r  = floor( mod( i / 4.0, 2.0  )) * 2.0;        
        
        col += floor( mod( i / 8.0, 2.0  ) );
        
        if ( i == 6.0 ) col = vec3(2,1,0); // Special brown!

        return col * 21.;
    }

    // Greys
    if ( i == 16.0 ) return vec3(0.0);
    
    if ( i < 32.0 )
    {        
        float x = (i - 17.0);        
        return vec3( floor( .00084 * x * x * x * x - .01662 * x * x * x + .1859 * x * x + 2.453 * x + 5.6038 ) );
    }
    
    // Rainbows
    float rainbowIndex = mod( i - 32.0, 24.0 );
    float rainbowType = floor( (i - 32.0) / 24.0 );
    
    float rainbowTypeMod = floor( mod( rainbowType, 3.0 ) );
    float rainbowTypeDiv = floor( rainbowType / 3.0 );
    
    float rainbowLow = 0.;
    if ( rainbowTypeMod == 1.0 ) rainbowLow = 31.0;
    if ( rainbowTypeMod == 2.0 ) rainbowLow = 45.0;
    
    float rainbowHigh = 63.;
    if ( rainbowTypeDiv == 1.0 )
    {
        rainbowHigh = 28.0;
        rainbowLow = floor( rainbowLow / 2.2 );
    }
    if ( rainbowTypeDiv == 2.0 )
    {
        rainbowHigh = 16.0;
        rainbowLow = floor( rainbowLow / 3.8 );
    }
    
    if ( rainbowType < 9.0 )
    {
	    return VGARainbow( rainbowIndex, rainbowLow, rainbowHigh );
    }
    
    return vec3( 0.0 );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 vUV = fragCoord.xy / iResolution.xy;
    
    vec2 vFakeResolution = vec2(640,480);
    vUV = floor(vUV * vFakeResolution) / vFakeResolution;
    
    vec2 vFocus = vec2(-0.5, 0.0);
    vec2 vScale = vec2(2.0);
    
    if ( false )  // disable mouse control
    if ( iMouse.z > 0.0 )
    {
    	vFocus += 2.0 * ((iMouse.xy / iResolution.xy) * 2.0 - 1.0);
    	vScale *= 0.02;
    }
    
    vScale.y /= iResolution.x / iResolution.y;
    
    vec2 z = vec2(0);
    vec2 c = vFocus + (vUV * 2.0 - 1.0) * vScale;
    
    bool bInside = true;
    
    float fIter = 0.0;
    for(int iter = 0; iter < 512; iter++)
    {        
 		z = mat2(z,-z.y,z.x) * z + c;
     
        if ( dot(z,z) > 4.0 )            
        {
            bInside = false;
            break;
        }       
        
        fIter++;
    }
    
    float fIndex = 0.0;
    if ( bInside ) 
    {
        //fIndex = 0.0; // black set
        fIndex = 1.0; // blue set
    }
    else
    {
 
        if ( Key_IsToggled( KEY_PLUS ) || Key_IsToggled( KEY_RIGHT ) )
        {
        	fIter += iTime * 10.0;
        }
        else
        if ( Key_IsToggled( KEY_MINUS ) || Key_IsToggled( KEY_LEFT ) )
        {
        	fIter -= iTime * 10.0;
        }
        
    	fIndex = 1.0 + mod( fIter, 255.0 );
    }
    
	fragColor.rgb = VGAPaletteEntry( fIndex ) / 63.0;
    fragColor.a = 1.0;
}

#endif

#ifdef SHADERTOY
// Shadertoy font shader - @P_Malin

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

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


float TestCurve(vec2 uv)
{
	uv = 1.0 - uv;
    return 1.0 - dot(uv, uv);
}

float Cross( const in vec2 A, const in vec2 B )
{
    return A.x * B.y - A.y * B.x;
}

vec2 GetUV(const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P)
{
    vec2 vPB = B - P;
    float f1 = Cross(A-B, vPB);
    float f2 = Cross(B-C, vPB);
    float f3 = Cross(C-A, C-P);
    
    return vec2(f1, f2) / (f1 + f2 + f3);
}

float InCurve( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
    vec2 vCurveUV = GetUV(A, B, C, P);
    
    float fResult = -1.0;

	fResult = max(fResult, (-vCurveUV.x));
	fResult = max(fResult, (-vCurveUV.y));
	fResult = max(fResult, (vCurveUV.x + vCurveUV.y - 1.0));

	float fCurveResult = TestCurve(vCurveUV);
		
	fResult = max(fResult, fCurveResult);	
	
    return fResult;
}

float InCurve2( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
    vec2 vCurveUV = GetUV(A, B, C, P);
	
    float fResult = -1.0;

	fResult = max(fResult, (vCurveUV.x + vCurveUV.y - 1.0));
	
	float fCurveResult = -TestCurve(vCurveUV);
	
	fResult = max(fResult, fCurveResult);	
	
    return fResult;
}

float InTri( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
	#ifdef CURVES_ONLY
	return 1.0;
	#endif
	
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(A-C, C-P);
	
    return (max(max(f1, f2), f3));
}

float InQuad( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 D, const in vec2 P )
{
	#ifdef CURVES_ONLY
	return 1.0;
	#endif
	
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(D-C, C-P);
    float f4 = Cross(A-D, D-P);
    
    return (max(max(max(f1, f2), f3), f4));
}


float Glyph0(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.112, 0.056 );
    const vec2  vP1 = vec2 ( 0.136, 0.026 );
    const vec2  vP2 = vec2 ( 0.108, 0.022 );
    const vec2  vP3 = vec2 ( 0.083, 0.017 ); 
    const vec2  vP4 = vec2 ( 0.082, 0.036 ); 
    const vec2  vP5 = vec2 ( 0.088, 0.062 ); 
    const vec2  vP6 = vec2 ( 0.115, 0.086 ); 
    const vec2  vP7 = vec2 ( 0.172, 0.147 ); 
    const vec2  vP8 = vec2 ( 0.100, 0.184 ); 
    const vec2  vP9 = vec2 ( 0.034, 0.206 ); 
    const vec2 vP10 = vec2 ( 0.021, 0.160 ); 
    const vec2 vP11 = vec2 ( 0.011, 0.114 ); 
    const vec2 vP12 = vec2 ( 0.052, 0.112 ); 
    const vec2 vP13 = vec2 ( 0.070, 0.108 ); 
    const vec2 vP14 = vec2 ( 0.075, 0.126 );
    const vec2 vP15 = vec2 ( 0.049, 0.124 );
    const vec2 vP16 = vec2 ( 0.047, 0.148 );
    const vec2 vP17 = vec2 ( 0.046, 0.169 );
    const vec2 vP18 = vec2 ( 0.071, 0.171 );
    const vec2 vP19 = vec2 ( 0.098, 0.171 ); 
    const vec2 vP20 = vec2 ( 0.097, 0.143 ); 
    const vec2 vP21 = vec2 ( 0.100, 0.118 ); 
    const vec2 vP22 = vec2 ( 0.080, 0.100 ); 
    const vec2 vP23 = vec2 ( 0.055, 0.083 ); 
    const vec2 vP24 = vec2 ( 0.050, 0.052 ); 
    const vec2 vP25 = vec2 ( 0.052, 0.004 ); 
    const vec2 vP26 = vec2 ( 0.107, 0.010 ); 
    const vec2 vP27 = vec2 ( 0.148, 0.011 ); 
    const vec2 vP28 = vec2 ( 0.140, 0.041 ); 
    const vec2 vP29 = vec2 ( 0.139, 0.069 ); 

    float fDist = 1.0;

	fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP8,vP9,vP10, uv) );
	fDist = min( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = min( fDist, InCurve2(vP12,vP13,vP14, uv) );
	fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve(vP16,vP17,vP18, uv) );
    fDist = min( fDist, InCurve(vP18,vP19,vP20, uv) );
    fDist = min( fDist, InCurve(vP20,vP21,vP22, uv) );
	fDist = min( fDist, InCurve2(vP22,vP23,vP24, uv) );
    fDist = min( fDist, InCurve2(vP24,vP25,vP26, uv) );
    fDist = min( fDist, InCurve2(vP26,vP27,vP28, uv) );
    fDist = min( fDist, InCurve2(vP28,vP29,vP0, uv) );
	fDist = min( fDist, InCurve(vP0,vP1,vP2, uv) );
	fDist = min( fDist, InCurve(vP2,vP3,vP4, uv) );
    fDist = min( fDist, InCurve(vP4,vP5,vP6, uv) );


    fDist = min( fDist, InTri(vP0, vP1, vP28, uv) );
	fDist = min( fDist, InQuad(vP26, vP1, vP2, vP3, uv) );
    fDist = min( fDist, InTri(vP3, vP4, vP24, uv) );
    fDist = min( fDist, InTri(vP4, vP5, vP24, uv) );
    fDist = min( fDist, InTri(vP24, vP5, vP22, uv) );
    fDist = min( fDist, InTri(vP5, vP6, vP22, uv) );
    fDist = min( fDist, InTri(vP22, vP6, vP21, uv) );
    fDist = min( fDist, InTri(vP6, vP8, vP21, uv) );
    fDist = min( fDist, InTri(vP21, vP8, vP20, uv) );
    fDist = min( fDist, InTri(vP20, vP8, vP19, uv) );
    fDist = min( fDist, InTri(vP19, vP8, vP18, uv) );
    fDist = min( fDist, InTri(vP18, vP8, vP10, uv) );
    fDist = min( fDist, InTri(vP10, vP16, vP17, uv) );
    fDist = min( fDist, InTri(vP10, vP15, vP16, uv) );
    fDist = min( fDist, InTri(vP10, vP12, vP16, uv) );
    fDist = min( fDist, InTri(vP12, vP14, vP15, uv) );

    return fDist;
}

float Glyph1(const in vec2 uv, const in vec2 vOffset)
{
    vec2 vP0 = vec2 ( 0.171, 0.026 ) + vOffset;
    vec2 vP1 = vec2 ( 0.204, 0.022 ) + vOffset;
    const vec2 vP2 = vec2 ( 0.170, 0.185 );
    const vec2 vP3 = vec2 ( 0.137, 0.185 );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}

float Glyph3(const in vec2 uv, vec2 vOffset)
{
    vec2 vP0 = vec2 ( 0.212, 0.112 ) + vOffset;
    vec2 vP2 = vec2 ( 0.243, 0.112 ) + vOffset;
    const vec2  vP4 = vec2 ( 0.234, 0.150 );
    const vec2  vP5 = vec2 ( 0.230, 0.159 );
    const vec2  vP6 = vec2 ( 0.243, 0.164 );
    const vec2  vP7 = vec2 ( 0.257, 0.164 );
    const vec2  vP8 = vec2 ( 0.261, 0.148 );
    const vec2 vP10 = vec2 ( 0.265, 0.164 );
    const vec2 vP11 = vec2 ( 0.256, 0.180 );
    const vec2 vP12 = vec2 ( 0.239, 0.185 );
    const vec2 vP13 = vec2 ( 0.194, 0.194 );
    const vec2 vP14 = vec2 ( 0.203, 0.150 );
    const vec2 vP16 = vec2 ( 0.212, 0.113 );

    float fDist = 1.0;
    fDist = min( fDist, InCurve(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = min( fDist, InCurve2(vP12,vP13,vP14, uv) );

    fDist = min( fDist, InQuad(vP0, vP2, vP4, vP14, uv) );
    fDist = min( fDist, InTri(vP14, vP4, vP5, uv) );
    fDist = min( fDist, InTri(vP14, vP5, vP12, uv) );
    fDist = min( fDist, InTri(vP5, vP6, vP12, uv) );
    fDist = min( fDist, InTri(vP6, vP7, vP12, uv) );
    fDist = min( fDist, InTri(vP6, vP10, vP12, uv) );
    fDist = min( fDist, InTri(vP8, vP10, vP7, uv) );
    
    return fDist;
}

float Glyph4(const in vec2 uv)
{
    vec2 vP = uv - vec2(0.305, 0.125);
    vP /= 0.065;
    vP.x *= 1.5;
    vP.x += vP.y * 0.25;
    
    vec2 vP2 = vP;

    vP.y = abs(vP.y);
    vP.y = pow(vP.y, 1.2);
    float f= length(vP);
    
    vP2.x *= 1.2;
    float f2 = length(vP2 * 1.5 - vec2(0.6, 0.0));
        
    return max(f - 1.0, 1.0 - f2) / 20.0;
} 

float Glyph5(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.507, 0.138 );
    const vec2  vP1 = vec2 ( 0.510, 0.065 );
    const vec2  vP2 = vec2 ( 0.570, 0.066 );
    const vec2  vP3 = vec2 ( 0.598, 0.066 );
    const vec2  vP4 = vec2 ( 0.594, 0.092 );
    const vec2  vP5 = vec2 ( 0.599, 0.131 );
    const vec2  vP6 = vec2 ( 0.537, 0.137 );
    const vec2  vP8 = vec2 ( 0.538, 0.125 );
    const vec2  vP9 = vec2 ( 0.564, 0.129 );
    const vec2 vP10 = vec2 ( 0.574, 0.100 );
    const vec2 vP11 = vec2 ( 0.584, 0.085 );
    const vec2 vP12 = vec2 ( 0.571, 0.079 );
    const vec2 vP13 = vec2 ( 0.557, 0.081 );
    const vec2 vP14 = vec2 ( 0.549, 0.103 );
    const vec2 vP15 = vec2 ( 0.518, 0.166 );
    const vec2 vP16 = vec2 ( 0.557, 0.166 );
    const vec2 vP17 = vec2 ( 0.589, 0.163 );
    const vec2 vP18 = vec2 ( 0.602, 0.137 );
    const vec2 vP20 = vec2 ( 0.602, 0.152 );
    const vec2 vP21 = vec2 ( 0.572, 0.194 );
    const vec2 vP22 = vec2 ( 0.537, 0.185 );
    const vec2 vP23 = vec2 ( 0.503, 0.189 );
    
    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = min( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = min( fDist, InCurve(vP10,vP11,vP12, uv) ); 
    fDist = min( fDist, InCurve(vP12,vP13,vP14, uv) );
    fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve(vP16,vP17,vP18, uv) ); 
    fDist = min( fDist, InCurve2(vP20,vP21,vP22, uv) ); 
    fDist = min( fDist, InCurve2(vP22,vP23,vP0, uv) );

    fDist = min( fDist, InTri(vP0, vP2, vP13, uv) );
    fDist = min( fDist, InTri(vP13, vP2, vP12, uv) );
    fDist = min( fDist, InTri(vP2, vP11, vP12, uv) );
    fDist = min( fDist, InTri(vP2, vP4, vP11, uv) );
    fDist = min( fDist, InTri(vP11, vP4, vP10, uv) );
    fDist = min( fDist, InTri(vP10, vP4, vP9, uv) );
    fDist = min( fDist, InTri(vP6, vP8, vP9, uv) );
    fDist = min( fDist, InTri(vP0, vP13, vP14, uv) );
    fDist = min( fDist, InTri(vP0, vP14, vP15, uv) );
    fDist = min( fDist, InTri(vP15, vP16, vP22, uv) );
    fDist = min( fDist, InTri(vP16, vP17, vP22, uv) );
    fDist = min( fDist, InTri(vP17, vP18, vP20, uv) );
    
    return fDist;
}

float Glyph6(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.638 , 0.087 ); 
    const vec2  vP1 = vec2 ( 0.648 , 0.073 ); 
    const vec2  vP2 = vec2 ( 0.673 , 0.068 ); 
    const vec2  vP3 = vec2 ( 0.692 , 0.069 ); 
    const vec2  vP4 = vec2 ( 0.687 , 0.086 ); 
    const vec2  vP5 = vec2 ( 0.688 , 0.104 ); 
    const vec2  vP6 = vec2 ( 0.672 , 0.102 ); 
    const vec2  vP7 = vec2 ( 0.659 , 0.099 ); 
    const vec2  vP8 = vec2 ( 0.663 , 0.092 ); 
    const vec2  vP9 = vec2 ( 0.662 , 0.086 ); 
    const vec2 vP10 = vec2 ( 0.655 , 0.086 ); 
    const vec2 vP11 = vec2 ( 0.644 , 0.087 ); 
    const vec2 vP12 = vec2 ( 0.637 , 0.102 ); 
    const vec2 vP13 = vec2 ( 0.638 , 0.094 ); 

    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = min( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) ); 
    fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) ); 
    fDist = min( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = min( fDist, InCurve(vP10,vP11,vP12, uv) );

    fDist = min( fDist, InQuad(vP2, vP4, vP6, vP8, uv) );
    fDist = min( fDist, InTri(vP9, vP2, vP8, uv) );
    fDist = min( fDist, InTri(vP10, vP2, vP9, uv) );
    fDist = min( fDist, InQuad(vP0, vP2, vP10, vP11, uv) );
    fDist = min( fDist, InTri(vP11, vP12, vP0, uv) );
    
    return fDist;
}

float Glyph7(const in vec2 uv)
{
    const vec2 vP0 = vec2 ( 0.693 , 0.068 );
    const vec2 vP1 = vec2 ( 0.748 , 0.069 );
    const vec2 vP2 = vec2 ( 0.747 , 0.078 );
    const vec2 vP3 = vec2 ( 0.691 , 0.077 );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}


float Glyph8(const in vec2 uv)
{ 
    vec2 vP = uv - vec2(0.788, 0.125);
    vP /= 0.065;
    vP.x *= 1.4;
    vP.x += vP.y * 0.25;
    
    vec2 vP2 = vP;
    
    vP.y = abs(vP.y);
    vP.y = pow(vP.y, 1.2);
    float f= length(vP);
    
    vP2.x *= 1.5;
    float f2 = length(vP2 * 1.5 - vec2(0.3, 0.0));
    
    
    return max(f - 1.0, 1.0 - f2) / 20.0;
}

float Glyph11(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.921 , 0.070 );
    const vec2  vP2 = vec2 ( 0.955 , 0.070 );
    const vec2  vP4 = vec2 ( 0.926 , 0.202 );
    const vec2  vP5 = vec2 ( 0.926 , 0.240 );
    const vec2  vP6 = vec2 ( 0.885 , 0.243 );
    const vec2  vP7 = vec2 ( 0.852 , 0.239 );
    const vec2  vP8 = vec2 ( 0.859 , 0.219 );
    const vec2  vP9 = vec2 ( 0.862 , 0.192 );
    const vec2 vP10 = vec2 ( 0.889 , 0.189 );
    const vec2 vP12 = vec2 ( 0.928 , 0.178 );
    const vec2 vP13 = vec2 ( 0.949 , 0.173 );
    const vec2 vP14 = vec2 ( 0.951 , 0.162 );
    const vec2 vP15 = vec2 ( 0.960 , 0.150 );
    const vec2 vP16 = vec2 ( 0.960 , 0.144 );
    const vec2 vP18 = vec2 ( 0.971 , 0.144 );
    const vec2 vP19 = vec2 ( 0.968 , 0.157 );
    const vec2 vP20 = vec2 ( 0.957 , 0.171 );
    const vec2 vP21 = vec2 ( 0.949 , 0.182 );
    const vec2 vP22 = vec2 ( 0.922 , 0.189 );
    const vec2 vP24 = vec2 ( 0.900 , 0.196 );
    const vec2 vP25 = vec2 ( 0.866 , 0.205 );
    const vec2 vP26 = vec2 ( 0.871 , 0.217 );
    const vec2 vP27 = vec2 ( 0.871 , 0.225 );
    const vec2 vP28 = vec2 ( 0.880 , 0.224 );
    const vec2 vP29 = vec2 ( 0.889 , 0.218 );
    const vec2 vP30 = vec2 ( 0.893 , 0.203 );

    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP8,vP9,vP10, uv) );
    fDist = min( fDist, InCurve(vP12,vP13,vP14, uv) );

    fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve2(vP18,vP19,vP20, uv) );
    fDist = min( fDist, InCurve2(vP20,vP21,vP22, uv) );

    fDist = min( fDist, InCurve(vP24,vP25,vP26, uv) );
    fDist = min( fDist, InCurve(vP26,vP27,vP28, uv) );
    fDist = min( fDist, InCurve(vP28,vP29,vP30, uv) );
    
    fDist = min( fDist, InQuad(vP0, vP2, vP4, vP30, uv) );

    fDist = min( fDist, InQuad(vP10, vP12, vP22, vP24, uv) );
        
    fDist = min( fDist, InTri(vP30, vP4, vP6, uv) );
    fDist = min( fDist, InTri(vP30, vP6, vP29, uv) );
    fDist = min( fDist, InTri(vP28, vP29, vP6, uv) );
    fDist = min( fDist, InTri(vP28, vP6, vP27, uv) );
    
    fDist = min( fDist, InTri(vP8, vP27, vP6, uv) );
    
    fDist = min( fDist, InTri(vP8, vP26, vP27, uv) );
    fDist = min( fDist, InTri(vP8, vP25, vP26, uv) );
    fDist = min( fDist, InTri(vP25, vP10, vP24, uv) );
    
    fDist = min( fDist, InTri(vP12, vP13, vP20, uv) );
    fDist = min( fDist, InTri(vP12, vP20, vP22, uv) );
    fDist = min( fDist, InTri(vP13, vP14, vP20, uv) );
    fDist = min( fDist, InTri(vP15, vP20, vP14, uv) );
    fDist = min( fDist, InTri(vP15, vP18, vP20, uv) );
    fDist = min( fDist, InTri(vP15, vP16, vP18, uv) );
    
    return fDist;
}

float Shadertoy(in vec2 uv)
{
    float fResult = 1.0;
    
    fResult = min(fResult, Glyph0(uv)); // S

    vec2 vUVOffset = vec2(0.001, 0.0); // tail of h
    vec2 vTailOffset = vec2(0.0, 0.0);  
    float fUVScale = 1.0;

    if(uv.x < 0.3)
    {
        if(uv.y < 0.12)
        {
            // top of h
            fUVScale = -1.0;
            vUVOffset = vec2(0.448, 0.25);  
            vTailOffset = vec2(0.0, 0.0);   
        }
    }
    else if(uv.x < 0.4)    
    {
        // tail of a
        vUVOffset = vec2(-0.124, 0.0);  
        vTailOffset = vec2(0.01, -0.04);    
    }
    else if(uv.x < 0.6)
    {
        // tail of d
        vUVOffset = vec2(-0.248, 0.0);  
        vTailOffset = vec2(0.02, -0.1); 
    }
    else if(uv.x < 0.83)
    {
        // stalk of t
        vUVOffset = vec2(-0.48, 0.0);   
        vTailOffset = vec2(0.02, -0.1); 
    }
    else
    {
        // start of y
        vUVOffset = vec2(-0.645, 0.0);  
        vTailOffset = vec2(0.005, -0.042);  
    }
    
    fResult = min(fResult, Glyph3(uv * fUVScale + vUVOffset, vTailOffset)); // tails h, a, d, t, start of y and top of h


    vec2 vUVOffset3 = vec2(0.0, 0.0);   // vertical of h
    vec2 vTailOffset3 = vec2(0.0, 0.0);
    
    if(uv.x > 0.5)
    {
        // vertical of r
        vUVOffset3 = vec2(-0.45, 0.0);  
        vTailOffset3 = vec2(-0.01, 0.04);   
    }
    
    fResult = min(fResult, Glyph1(uv + vUVOffset3, vTailOffset3)); // vertical of h, r

    vec2 vUVOffset2 = vec2(0.0, 0.0); // curve of a
    if(uv.x > 0.365)
    {
        vUVOffset2 = vec2(-0.125, 0.0); // curve of d
    }

    fResult = min(fResult, Glyph4(uv + vUVOffset2)); // curve of a, d
    
    fResult = min(fResult, Glyph5(uv)); // e

    vec2 vUVOffset4 = vec2(0.001, 0.0); // top of r
    vec2 vUVScale4 = vec2(1.0, 1.0);        
    
    if(uv.x > 0.7)
    {
        // o loop
        vUVOffset4.x = 1.499;
        vUVOffset4.y = 0.19;
        
        vUVScale4.x = -1.0;
        vUVScale4.y = -1.0;
    }
    
    fResult = min(fResult, Glyph6(uv * vUVScale4 + vUVOffset4)); // top of r and o loop

    fResult = min(fResult, Glyph7(uv)); // cross t    
    
    fResult = min(fResult, Glyph8(uv)); // o1
    
    fResult = min(fResult, Glyph11(uv)); // y2        

    return fResult; 
}

vec2 GetUVCentre(const vec2 vInputUV)
{
	vec2 vFontUV = vInputUV;
    vFontUV.y -= 0.35;
		
	return vFontUV;
}

vec2 GetUVScroll(const vec2 vInputUV, float t)
{
	vec2 vFontUV = vInputUV;
	vFontUV *= 0.25;
	
    vFontUV.y -= 0.005;
	vFontUV.x += t * 3.0 - 1.5;
	
	return vFontUV;
}

vec2 GetUVRepeat(const vec2 vInputUV, float t2)
{
	vec2 vFontUV = vInputUV;
	
	vFontUV *= vec2(1.0, 4.0);
	
	vFontUV.x += floor(vFontUV.y) * t2;
	
	vFontUV = fract(vFontUV);
	
	vFontUV /= vec2(1.0, 4.0);
		
	return vFontUV;
}

vec2 GetUVRotate(const vec2 vInputUV, float t)
{
	vec2 vFontUV = vInputUV - 0.5;
	
	float s = sin(t);
	float c = cos(t);
	
	vFontUV = vec2(  vFontUV.x * c + vFontUV.y * s,
			        -vFontUV.x * s + vFontUV.y * c );
	
	vFontUV += 0.5;
	
	return vFontUV;
}

vec3 StyleDefault( float f )
{
	return mix(vec3(0.25), vec3(1.0), f);
}

vec3 StyleScanline( float f, in vec2 fragCoord )
{
	float fShade = f * 0.8 + 0.2;
	
    // disable
	//fShade *= mod(fragCoord.y, 2.0);
	
	return mix(vec3(0.01, 0.2, 0.01), vec3(0.01, 1.0, 0.02), fShade);
}

vec3 StyleStamp( float fFont, vec2 uv )
{
	vec3 t1 = texture(iChannel2, uv + 0.005).rgb;
	vec3 t2 = texture(iChannel2, uv).rgb;
	float dt = clamp(0.5 + (t1.x - t2.x), 0.0, 1.0);
	float fWear = clamp((0.9 - t2.x) * 4.0, 0.0, 1.0);
	float f =  clamp(fFont * fWear, 0.0, 1.0);
	return mix( vec3(1.0, 0.98, 0.9) * (dt * 0.1 + 0.9), vec3(0.7, 0.0, 0.0), f);
}

vec3 StyleWood( float fFont, vec2 uv )
{
	vec3 t = texture(iChannel2, uv).rgb;
	float fWear = fFont * smoothstep(0.0, 0.4, t.b);
	return mix(t, vec3(0.0), fWear);
}

vec4 GetRandom4(float x)
{
	return fract(vec4(987.65, 432.10, 765.43, 210.98) * sin(vec4(123.456, 789.123, 456.789, 567.890) * x));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	float fSequenceLength = 5.0;
	
	float fTime = iTime;
	
	float fBlendSpeed = 0.05;
	
	// Skip the initial fade-in
	fTime += fBlendSpeed * fSequenceLength;
	
	float fInt = floor(fTime / fSequenceLength);
	float fFract = fract(fTime / fSequenceLength);
	
	vec4 vRandom4 = GetRandom4(fInt);
	vec2 vRandom2 = floor(vRandom4.xy * vec2(1234.56, 123.45));
	
	float fUVEffect = mod(vRandom2.x, 4.0);
	float fScreenEffect = mod(vRandom2.y, 4.0);

	if(fInt < 0.5)
	{
		fUVEffect = 0.0;
		fScreenEffect = 0.0;
	}

	vec4 vResult = vec4(0.0);
		
	float fX = 0.0;
	for(int iX=0; iX<AA_X; iX++)
	{
		float fY = 0.0;
		for(int y=0; y<AA_Y; y++)
		{
	
			vec2 vUV = (fragCoord.xy + vec2(fX, fY)) / iResolution.xy;
			vUV.x = ((vUV.x - 0.5) * (iResolution.x / iResolution.y)) + 0.5;    
			vUV.y = 1.0 - vUV.y;
				
			vec2 vFontUV = vUV;
			vec2 vBgUV = vUV;
			
            if ( false ) 
			if(iMouse.z > 0.0)
			{
				fUVEffect = 999.0;
				fScreenEffect = 0.0;
				fFract = 0.5;
				
				vFontUV *= 0.25;
				vFontUV += iMouse.xy / iResolution.xy;
				vFontUV.y -= 0.5;
				vBgUV = vFontUV;
			}	
			
			if(fUVEffect < 0.5)
			{
				vFontUV = GetUVCentre(vBgUV);
			}
			else
			if(fUVEffect < 1.5)
			{
				vBgUV = GetUVScroll(vBgUV, fFract);
				vFontUV = vBgUV;
			}
			else
			if(fUVEffect < 2.5)
			{
				float fSpeed = 0.1 + vRandom4.z;
				vBgUV.x += fFract * fSpeed;
				vFontUV = GetUVRepeat(vBgUV, 0.25);
			}
			else
			if(fUVEffect < 3.5)
			{
				float fSpeed = 1.0 + vRandom4.z * 2.0;
				if(vRandom4.w > 0.5)
				{
					fSpeed = -fSpeed;
				}
				vBgUV = GetUVRotate(vBgUV, 1.0 + fSpeed * fFract);
				vFontUV = GetUVRepeat(vBgUV, 0.0);
			}
			
			float fShadertoy = step(Shadertoy(vFontUV), 0.0);
				
			if(fScreenEffect < 0.5)
			{
				vResult += vec4(StyleDefault(fShadertoy), 1.0);
			}
			else if(fScreenEffect < 1.5)
			{
				vResult += vec4(StyleScanline(fShadertoy, fragCoord), 1.0);
			}
			else if(fScreenEffect < 2.5)
			{
				vResult += vec4(StyleStamp(fShadertoy, vBgUV), 1.0);
			}
			else
			{
				vResult += vec4(StyleWood(fShadertoy, vBgUV), 1.0);
			}

			fY += 1.0 / float(AA_Y);
		}
		
		fX += 1.0 / float(AA_X);
	}
	
	vResult.xyz /= vResult.w;

	float fFade = 0.0;	
	if(fFract > (1.0 - fBlendSpeed))
	{
		fFade = smoothstep(1.0 - fBlendSpeed, 1.0, fFract);
	}

	if(fFract < fBlendSpeed)
	{
		fFade = smoothstep(fBlendSpeed, 0.0, fFract);
	}

	vResult = mix(vResult, vec4(1.0), fFade);
	
    fragColor = vec4(vResult.xyz, 1.0);
}
#endif

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Scene Rendering

#define ENABLE_TAA_JITTER

#define kMaxTraceDist 1000.0
#define kFarDist 1100.0

#define MAT_FG_BEGIN 	10

///////////////////////////
// Scene
///////////////////////////

struct SceneResult
{
	float fDist;
	int iObjectId;
    vec3 vUVW;
};
    
void Scene_Union( inout SceneResult a, in SceneResult b )
{
    if ( b.fDist < a.fDist )
    {
        a = b;
    }
}

    
void Scene_Subtract( inout SceneResult a, in SceneResult b )
{
    if ( a.fDist < -b.fDist )
    {
        a.fDist = -b.fDist;
        a.iObjectId = b.iObjectId;
        a.vUVW = b.vUVW;
    }
}

SceneResult Scene_GetDistance( vec3 vPos );    

vec3 Scene_GetNormal(const in vec3 vPos)
{
    const float fDelta = 0.0001;
    vec2 e = vec2( -1, 1 );
    
    vec3 vNormal = 
        Scene_GetDistance( e.yxx * fDelta + vPos ).fDist * e.yxx + 
        Scene_GetDistance( e.xxy * fDelta + vPos ).fDist * e.xxy + 
        Scene_GetDistance( e.xyx * fDelta + vPos ).fDist * e.xyx + 
        Scene_GetDistance( e.yyy * fDelta + vPos ).fDist * e.yyy;
    
    return normalize( vNormal );
}    
    
SceneResult Scene_Trace( const in vec3 vRayOrigin, const in vec3 vRayDir, float minDist, float maxDist )
{	
    SceneResult result;
    result.fDist = 0.0;
    result.vUVW = vec3(0.0);
    result.iObjectId = -1;
    
	float t = minDist;
	const int kRaymarchMaxIter = 128;
	for(int i=0; i<kRaymarchMaxIter; i++)
	{		
        float epsilon = 0.0001 * t;
		result = Scene_GetDistance( vRayOrigin + vRayDir * t );
        if ( abs(result.fDist) < epsilon )
		{
			break;
		}
                        
        if ( t > maxDist )
        {
            result.iObjectId = -1;
	        t = maxDist;
            break;
        }       
        
        if ( result.fDist > 1.0 )
        {
            result.iObjectId = -1;            
        }    
        
        t += result.fDist;        
	}
    
    result.fDist = t;


    return result;
}    

float Scene_TraceShadow( const in vec3 vRayOrigin, const in vec3 vRayDir, const in float fMinDist, const in float fLightDist )
{
    //return 1.0;
    //return ( Scene_Trace( vRayOrigin, vRayDir, 0.1, fLightDist ).fDist < fLightDist ? 0.0 : 1.0;
    
	float res = 1.0;
    float t = fMinDist;
    for( int i=0; i<16; i++ )
    {
		float h = Scene_GetDistance( vRayOrigin + vRayDir * t ).fDist;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.0001 || t>fLightDist ) break;
    }
    return clamp( res, 0.0, 1.0 );    
}

float Scene_GetAmbientOcclusion( const in vec3 vPos, const in vec3 vDir )
{
    float fOcclusion = 0.0;
    float fScale = 1.0;
    for( int i=0; i<5; i++ )
    {
        float fOffsetDist = 0.001 + 0.1*float(i)/4.0;
        vec3 vAOPos = vDir * fOffsetDist + vPos;
        float fDist = Scene_GetDistance( vAOPos ).fDist;
        fOcclusion += (fOffsetDist - fDist) * fScale;
        fScale *= 0.4;
    }
    
    return clamp( 1.0 - 30.0*fOcclusion, 0.0, 1.0 );
}

///////////////////////////
// Lighting
///////////////////////////
    
struct SurfaceInfo
{
    vec3 vPos;
    vec3 vNormal;
    vec3 vBumpNormal;    
    vec3 vAlbedo;
    vec3 vR0;
    float fSmoothness;
    vec3 vEmissive;
};
    
SurfaceInfo Scene_GetSurfaceInfo( const in vec3 vRayOrigin,  const in vec3 vRayDir, SceneResult traceResult );

struct SurfaceLighting
{
    vec3 vDiffuse;
    vec3 vSpecular;
};
    
SurfaceLighting Scene_GetSurfaceLighting( const in vec3 vRayDir, in SurfaceInfo surfaceInfo );

float Light_GIV( float dotNV, float k)
{
	return 1.0 / ((dotNV + 0.0001) * (1.0 - k)+k);
}

void Light_Add(inout SurfaceLighting lighting, SurfaceInfo surface, const in vec3 vViewDir, const in vec3 vLightDir, const in vec3 vLightColour)
{
	float fNDotL = clamp(dot(vLightDir, surface.vBumpNormal), 0.0, 1.0);
	
	lighting.vDiffuse += vLightColour * fNDotL;
    
	vec3 vH = normalize( -vViewDir + vLightDir );
	float fNdotV = clamp(dot(-vViewDir, surface.vBumpNormal), 0.0, 1.0);
	float fNdotH = clamp(dot(surface.vBumpNormal, vH), 0.0, 1.0);
    
	float alpha = 1.0 - surface.fSmoothness;
	// D

	float alphaSqr = alpha * alpha;
	float denom = fNdotH * fNdotH * (alphaSqr - 1.0) + 1.0;
	float d = alphaSqr / (PI * denom * denom);

	float k = alpha / 2.0;
	float vis = Light_GIV(fNDotL, k) * Light_GIV(fNdotV, k);

	float fSpecularIntensity = d * vis * fNDotL;    
	lighting.vSpecular += vLightColour * fSpecularIntensity;    
}

void Light_AddPoint(inout SurfaceLighting lighting, SurfaceInfo surface, const in vec3 vViewDir, const in vec3 vLightPos, const in vec3 vLightColour)
{    
    vec3 vPos = surface.vPos;
	vec3 vToLight = vLightPos - vPos;	
    
	vec3 vLightDir = normalize(vToLight);
	float fDistance2 = dot(vToLight, vToLight);
	float fAttenuation = 100.0 / (fDistance2);
	
	float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1, length(vToLight) );
	
	Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

void Light_AddDirectional(inout SurfaceLighting lighting, SurfaceInfo surface, const in vec3 vViewDir, const in vec3 vLightDir, const in vec3 vLightColour)
{	
	float fAttenuation = 1.0;
	float fShadowFactor = Scene_TraceShadow( surface.vPos, vLightDir, 0.1, 10.0 );
	
	Light_Add( lighting, surface, vViewDir, vLightDir, vLightColour * fShadowFactor * fAttenuation);
}

vec3 Light_GetFresnel( vec3 vView, vec3 vNormal, vec3 vR0, float fGloss )
{
    float NdotV = max( 0.0, dot( vView, vNormal ) );

    return vR0 + (vec3(1.0) - vR0) * pow( 1.0 - NdotV, 5.0 ) * pow( fGloss, 20.0 );
}

void Env_AddPointLightFlare(inout vec3 vEmissiveGlow, const in vec3 vRayOrigin, const in vec3 vRayDir, const in float fIntersectDistance, const in vec3 vLightPos, const in vec3 vLightColour)
{
    vec3 vToLight = vLightPos - vRayOrigin;
    float fPointDot = dot(vToLight, vRayDir);
    fPointDot = clamp(fPointDot, 0.0, fIntersectDistance);

    vec3 vClosestPoint = vRayOrigin + vRayDir * fPointDot;
    float fDist = length(vClosestPoint - vLightPos);
	vEmissiveGlow += sqrt(vLightColour * 0.05 / (fDist * fDist));
}

void Env_AddDirectionalLightFlareToFog(inout vec3 vFogColour, const in vec3 vRayDir, const in vec3 vLightDir, const in vec3 vLightColour)
{
	float fDirDot = clamp(dot(vLightDir, vRayDir) * 0.5 + 0.5, 0.0, 1.0);
	float kSpreadPower = 2.0;
	vFogColour += vLightColour * pow(fDirDot, kSpreadPower) * 0.25;
}


///////////////////////////
// Rendering
///////////////////////////

vec4 Env_GetSkyColor( const vec3 vViewPos, const vec3 vViewDir );
vec3 Env_ApplyAtmosphere( const in vec3 vColor, const in vec3 vRayOrigin,  const in vec3 vRayDir, const in float fDist );
vec3 FX_Apply( in vec3 vColor, const in vec3 vRayOrigin,  const in vec3 vRayDir, const in float fDist);

vec4 Scene_GetColorAndDepth( vec3 vRayOrigin, vec3 vRayDir )
{
	vec3 vResultColor = vec3(0.0);
            
	SceneResult firstTraceResult;
    
    float fStartDist = 0.0f;
    float fMaxDist = 10.0f;
    
    vec3 vRemaining = vec3(1.0);
    
	for( int iPassIndex=0; iPassIndex < 3; iPassIndex++ )
    {
    	SceneResult traceResult = Scene_Trace( vRayOrigin, vRayDir, fStartDist, fMaxDist );

        if ( iPassIndex == 0 )
        {
            firstTraceResult = traceResult;
        }
        
        vec3 vColor = vec3(0);
        vec3 vReflectAmount = vec3(0);
        
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
			
			vColor = (surfaceInfo.vAlbedo * surfaceLighting.vDiffuse + surfaceInfo.vEmissive) * (vec3(1.0) - vReflectAmount); 
            
            vec3 vReflectRayOrigin = surfaceInfo.vPos;
            vec3 vReflectRayDir = normalize( reflect( vRayDir, surfaceInfo.vBumpNormal ) );
            fStartDist = 0.001 / max(0.0000001,abs(dot( vReflectRayDir, surfaceInfo.vNormal ))); 

            vColor += surfaceLighting.vSpecular * vReflectAmount;            

			vColor = Env_ApplyAtmosphere( vColor, vRayOrigin, vRayDir, traceResult.fDist );
			vColor = FX_Apply( vColor, vRayOrigin, vRayDir, traceResult.fDist );
            
            vRayOrigin = vReflectRayOrigin;
            vRayDir = vReflectRayDir;
        }
        
        vResultColor += vColor * vRemaining;
        vRemaining *= vReflectAmount;        
    }
 
    return vec4( vResultColor, EncodeDepthAndObject( firstTraceResult.fDist, firstTraceResult.iObjectId ) );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////
// Scene Description
/////////////////////////

// Materials

#define MAT_SKY		 	-1
#define MAT_DEFAULT 	 0
#define MAT_SCREEN		 1
#define MAT_TV_CASING    2
#define MAT_TV_TRIM      3
#define MAT_CHROME       4


vec3 PulseIntegral( vec3 x, float s1, float s2 )
{
    // Integral of function where result is 1.0 between s1 and s2 and 0 otherwise        

    // V1
    //if ( x > s2 ) return s2 - s1;
	//else if ( x > s1 ) return x - s1;
	//return 0.0f; 
    
    // V2
    //return clamp( (x - s1), 0.0f, s2 - s1);
    //return t;
    
    return clamp( (x - s1), vec3(0.0f), vec3(s2 - s1));
}

float PulseIntegral( float x, float s1, float s2 )
{
    // Integral of function where result is 1.0 between s1 and s2 and 0 otherwise        

    // V1
    //if ( x > s2 ) return s2 - s1;
	//else if ( x > s1 ) return x - s1;
	//return 0.0f; 
    
    // V2
    //return clamp( (x - s1), 0.0f, s2 - s1);
    //return t;
    
    return clamp( (x - s1), (0.0f), (s2 - s1));
}

vec3 Bayer( vec2 vUV, vec2 vBlur )
{
    vec3 x = vec3(vUV.x);
    vec3 y = vec3(vUV.y);           

    x += vec3(0.66, 0.33, 0.0);
    y += 0.5 * step( fract( x * 0.5 ), vec3(0.5) );
        
    //x -= 0.5f;
    //y -= 0.5f;
    
    x = fract( x );
    y = fract( y );
    
    // cell centered at 0.5
    
    vec2 vSize = vec2(0.16f, 0.75f);
    
    vec2 vMin = 0.5 - vSize * 0.5;
    vec2 vMax = 0.5 + vSize * 0.5;
    
    vec3 vResult= vec3(0.0);
    
    vec3 vResultX = (PulseIntegral( x + vBlur.x, vMin.x, vMax.x) - PulseIntegral( x - vBlur.x, vMin.x, vMax.x)) / min( vBlur.x, 1.0);
    vec3 vResultY = (PulseIntegral(y + vBlur.y, vMin.y, vMax.y) - PulseIntegral(y - vBlur.y, vMin.y, vMax.y))  / min( vBlur.y, 1.0);
    
    vResult = min(vResultX,vResultY)  * 5.0;
        
    //vResult = vec3(1.0);
    
    return vResult;
}

vec3 GetPixelMatrix( vec2 vUV )
{
#if 1
    vec2 dx = dFdx( vUV );
    vec2 dy = dFdy( vUV );
    float dU = length( vec2( dx.x, dy.x ) );
    float dV = length( vec2( dx.y, dy.y ) );
    if (dU <= 0.0 || dV <= 0.0 ) return vec3(1.0);
    return Bayer( vUV, vec2(dU, dV) * 1.0);
#else
    return vec3(1.0);
#endif
}

float Scanline( float y, float fBlur )
{   
    float fResult = sin( y * 10.0 ) * 0.45 + 0.55;
    return mix( fResult, 1.0f, min( 1.0, fBlur ) );
}


float GetScanline( vec2 vUV )
{
#if 1
    vUV.y *= 0.25;
    vec2 dx = dFdx( vUV );
    vec2 dy = dFdy( vUV );
    float dV = length( vec2( dx.y, dy.y ) );
    if (dV <= 0.0 ) return 1.0;
    return Scanline( vUV.y, dV * 1.3 );
#else
    return 1.0;
#endif
}


vec2 kScreenRsolution = vec2(480.0f, 576.0f);

struct Interference
{
    float noise;
    float scanLineRandom;
};

float InterferenceHash(float p)
{
    float hashScale = 0.1031;

    vec3 p3  = fract(vec3(p, p, p) * hashScale);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}


float InterferenceSmoothNoise1D( float x )
{
    float f0 = floor(x);
    float fr = fract(x);

    float h0 = InterferenceHash( f0 );
    float h1 = InterferenceHash( f0 + 1.0 );

    return h1 * fr + h0 * (1.0 - fr);
}


float InterferenceNoise( vec2 uv )
{
	float displayVerticalLines = 483.0;
    float scanLine = floor(uv.y * displayVerticalLines); 
    float scanPos = scanLine + uv.x;
	float timeSeed = fract( iTime * 123.78 );
    
    return InterferenceSmoothNoise1D( scanPos * 234.5 + timeSeed * 12345.6 );
}
    
Interference GetInterference( vec2 vUV )
{
    Interference interference;
        
    interference.noise = InterferenceNoise( vUV );
    interference.scanLineRandom = InterferenceHash(vUV.y * 100.0 + fract(iTime * 1234.0) * 12345.0);
    
    return interference;
}
    
vec3 SampleScreen( vec3 vUVW )
{   
    vec3 vAmbientEmissive = vec3(0.1);
    vec3 vBlackEmissive = vec3(0.02);
    float fBrightness = 1.75;
    vec2 vResolution = vec2(480.0f, 576.0f);
    vec2 vPixelCoord = vUVW.xy * vResolution;
    
    vec3 vPixelMatrix = GetPixelMatrix( vPixelCoord );
    float fScanline = GetScanline( vPixelCoord );
      
    vec2 vTextureUV = vUVW.xy;
    //vec2 vTextureUV = vPixelCoord;
    vTextureUV = floor(vTextureUV * vResolution * 2.0) / (vResolution * 2.0f);
    
    Interference interference = GetInterference( vTextureUV );

    float noiseIntensity = 0.1;
    
    //vTextureUV.x += (interference.scanLineRandom * 2.0f - 1.0f) * 0.025f * noiseIntensity;
    
    
    vec3 vPixelEmissive = textureLod( iChannel0, vTextureUV.xy, 0.0 ).rgb;
        
    vPixelEmissive = clamp( vPixelEmissive + (interference.noise - 0.5) * 2.0 * noiseIntensity, 0.0, 1.0 );
    
	vec3 vResult = (vPixelEmissive * vPixelEmissive * fBrightness + vBlackEmissive) * vPixelMatrix * fScanline + vAmbientEmissive;
    
    // TODO: feather edge?
    if( any( greaterThanEqual( vUVW.xy, vec2(1.0) ) ) || any ( lessThan( vUVW.xy, vec2(0.0) ) ) || ( vUVW.z > 0.0 ) )
    {
        return vec3(0.0);
    }
    
    return vResult;
    
}

SurfaceInfo Scene_GetSurfaceInfo( const in vec3 vRayOrigin,  const in vec3 vRayDir, SceneResult traceResult )
{
    SurfaceInfo surfaceInfo;
    
    surfaceInfo.vPos = vRayOrigin + vRayDir * (traceResult.fDist);
    
    surfaceInfo.vNormal = Scene_GetNormal( surfaceInfo.vPos ); 
    surfaceInfo.vBumpNormal = surfaceInfo.vNormal;
    surfaceInfo.vAlbedo = vec3(1.0);
    surfaceInfo.vR0 = vec3( 0.02 );
    surfaceInfo.fSmoothness = 1.0;
    surfaceInfo.vEmissive = vec3( 0.0 );
    //return surfaceInfo;
        
    if ( traceResult.iObjectId == MAT_DEFAULT )
    {
    	surfaceInfo.vR0 = vec3( 0.02 );
	    surfaceInfo.vAlbedo = textureLod( iChannel2, traceResult.vUVW.xz * 2.0, 0.0 ).rgb;
        surfaceInfo.vAlbedo = surfaceInfo.vAlbedo * surfaceInfo.vAlbedo;
                        
    	surfaceInfo.fSmoothness = clamp( 1.0 - surfaceInfo.vAlbedo.r * surfaceInfo.vAlbedo.r * 2.0, 0.0, 1.0);
        
    }
    
    if ( traceResult.iObjectId == MAT_SCREEN )
    {
        surfaceInfo.vAlbedo = vec3(0.02); 
        surfaceInfo.vEmissive = SampleScreen( traceResult.vUVW );        
    }

    if ( traceResult.iObjectId == MAT_TV_CASING )
    {
        surfaceInfo.vAlbedo = vec3(0.5, 0.4, 0.3); 
	    surfaceInfo.fSmoothness = 0.4;        
    }
    
    if ( traceResult.iObjectId == MAT_TV_TRIM )
    {
        surfaceInfo.vAlbedo = vec3(0.03, 0.03, 0.05); 
	    surfaceInfo.fSmoothness = 0.5;
    }    

    if ( traceResult.iObjectId == MAT_CHROME )
    {
        surfaceInfo.vAlbedo = vec3(0.01, 0.01, 0.01); 
	    surfaceInfo.fSmoothness = 0.9;
    	surfaceInfo.vR0 = vec3( 0.8 );
    }    
 
    return surfaceInfo;
}

// Scene Description

float SmoothMin( float a, float b, float k )
{
	//return min(a,b);
	
	
    //float k = 0.06;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

float UdRoundBox( vec3 p, vec3 b, float r )
{
    //vec3 vToFace = abs(p) - b;
    //vec3 vConstrained = max( vToFace, 0.0 );
    //return length( vConstrained ) - r;
    return length(max(abs(p)-b,0.0))-r;
}

SceneResult Scene_GetCRT( vec3 vScreenDomain, vec2 vScreenWH, float fScreenCurveRadius, float fBevel, float fDepth )
{
    SceneResult resultScreen;
#if 1
    vec3 vScreenClosest;
    vScreenClosest.xy = max(abs(vScreenDomain.xy)-vScreenWH,0.0);
    vec2 vCurveScreenDomain = vScreenDomain.xy;
    vCurveScreenDomain = clamp( vCurveScreenDomain, -vScreenWH, vScreenWH );
    float fCurveScreenProjection2 = fScreenCurveRadius * fScreenCurveRadius - vCurveScreenDomain.x * vCurveScreenDomain.x - vCurveScreenDomain.y * vCurveScreenDomain.y;
    float fCurveScreenProjection = sqrt( fCurveScreenProjection2 ) - fScreenCurveRadius;
    vScreenClosest.z = vScreenDomain.z - clamp( vScreenDomain.z, -fCurveScreenProjection, fDepth );
    resultScreen.vUVW.z = vScreenDomain.z + fCurveScreenProjection;        
    resultScreen.fDist = (length( vScreenClosest ) - fBevel) * 0.95;
    //resultScreen.fDist = (length( vScreenDomain - vec3(0,0,fScreenCurveRadius)) - fScreenCurveRadius - fBevel);    
#endif    
    
#if 0
    vec3 vScreenClosest;
    vScreenClosest.xyz = max(abs(vScreenDomain.xyz)-vec3(vScreenWH, fDepth),0.0);
    float fRoundDist = length( vScreenClosest.xyz ) - fBevel;
    float fSphereDist = length( vScreenDomain - vec3(0,0,fScreenCurveRadius) ) - (fScreenCurveRadius + fBevel);    
    resultScreen.fDist = max(fRoundDist, fSphereDist);
#endif    
    
    resultScreen.vUVW.xy = (vScreenDomain.xy / vScreenWH) * 0.5 + 0.5f;
	resultScreen.iObjectId = MAT_SCREEN;
    return resultScreen;
}

SceneResult Scene_GetComputer( vec3 vPos )
{
    SceneResult resultComputer;
    resultComputer.vUVW = vPos.xzy;
	
    float fXSectionStart = -0.2;
    float fXSectionLength = 0.15;
    float fXSectionT = clamp( (vPos.z - fXSectionStart) / fXSectionLength, 0.0, 1.0);
    float fXSectionR1 = 0.03;
    float fXSectionR2 = 0.05;
    float fXSectionR = mix( fXSectionR1, fXSectionR2, fXSectionT );
    float fXSectionZ = fXSectionStart + fXSectionT * fXSectionLength;
    
    vec2 vXSectionCentre = vec2(fXSectionR, fXSectionZ );
    vec2 vToPos = vPos.yz - vXSectionCentre;
    float l = length( vToPos );
    if ( l > fXSectionR ) l = fXSectionR;
    vec2 vXSectionClosest = vXSectionCentre + normalize(vToPos) * l;
    //float fXSectionDist = length( vXSectionClosest ) - fXSectionR;
    
    float x = max( abs( vPos.x ) - 0.2f, 0.0 );

    resultComputer.fDist = length( vec3(x, vXSectionClosest - vPos.yz) )-0.01;
    //resultComputer.fDist = x;
        
    resultComputer.iObjectId = MAT_TV_CASING;
/*
    vec3 vKeyPos = vPos.xyz - vec3(0,0.125,0);
    vKeyPos.y -= vKeyPos.z * (fXSectionR2 - fXSectionR1) * 2.0 / fXSectionLength;
    float fDomainRepeatScale = 0.02;
    if ( fract(vKeyPos.z * 0.5 / fDomainRepeatScale + 0.25) > 0.5) vKeyPos.x += fDomainRepeatScale * 0.5;
    vec2 vKeyIndex = round(vKeyPos.xz / fDomainRepeatScale);
    vKeyIndex.x = clamp( vKeyIndex.x, -8.0, 8.0 );
    vKeyIndex.y = clamp( vKeyIndex.y, -10.0, -5.0 );
    //vKeyPos.xz = (fract( vKeyPos.xz / fDomainRepeatScale ) - 0.5) * fDomainRepeatScale;
    vKeyPos.xz = (vKeyPos.xz - (vKeyIndex) * fDomainRepeatScale);
    vKeyPos.xz /= 0.7 + vKeyPos.y;
    SceneResult resultKey;    
    resultKey.vUVW = vPos.xzy;
    resultKey.fDist = UdRoundBox( vKeyPos, vec3(0.01), 0.001 );
    resultKey.iObjectId = MAT_TV_TRIM;
    Scene_Union( resultComputer, resultKey );
*/    
    return resultComputer;
}

SceneResult Scene_GetDistance( vec3 vPos )
{
    SceneResult result;
    
	//result.fDist = vPos.y;
    float fBenchBevel = 0.01;
    result.fDist = UdRoundBox( vPos - vec3(0,-0.02-fBenchBevel,0.0), vec3(2.0, 0.02, 1.0), fBenchBevel );
    result.vUVW = vPos;
	result.iObjectId = MAT_DEFAULT;        
    
    vec3 vSetPos = vec3(0.0, 0.0, 0.0);
    vec3 vScreenPos = vSetPos + vec3(0.0, 0.25, 0.00);
    
    //vPos.x = fract( vPos.x - 0.5) - 0.5;
    
    vec2 vScreenWH = vec2(4.0, 3.0) / 25.0;

    SceneResult resultSet;
    resultSet.vUVW = vPos.xzy;
	resultSet.fDist = UdRoundBox( vPos - vScreenPos - vec3(0.0,-0.01,0.2), vec3(.21, 0.175, 0.18), 0.01 );
    resultSet.iObjectId = MAT_TV_CASING;
    Scene_Union( result, resultSet );

    SceneResult resultSetRecess;
    resultSetRecess.vUVW = vPos.xzy;
    resultSetRecess.fDist = UdRoundBox( vPos - vScreenPos - vec3(0.0,-0.0, -0.05), vec3(vScreenWH + 0.01, 0.05) + 0.005, 0.015 );
    resultSetRecess.iObjectId = MAT_TV_TRIM;
	Scene_Subtract( result, resultSetRecess );
    
    SceneResult resultSetBase;
    resultSetBase.vUVW = vPos.xzy;
    float fBaseBevel = 0.03;
	resultSetBase.fDist = UdRoundBox( vPos - vSetPos - vec3(0.0,0.04,0.22), vec3(0.2, 0.04, 0.17) - fBaseBevel, fBaseBevel );
    resultSetBase.iObjectId = MAT_TV_CASING;
    Scene_Union( result, resultSetBase );

	SceneResult resultScreen = Scene_GetCRT( vPos - vScreenPos, vScreenWH, 0.75f, 0.02f, 0.1f );
    Scene_Union( result, resultScreen );    
    
    //SceneResult resultComputer = Scene_GetComputer( vPos - vec3(0.0, 0.0, -0.1) );
    //Scene_Union( result, resultComputer );

    SceneResult resultSphere;
    resultSet.vUVW = vPos.xzy;
	resultSet.fDist = length(vPos - vec3(0.35,0.075,-0.1)) - 0.075;
    resultSet.iObjectId = MAT_CHROME;
    Scene_Union( result, resultSet );    
    
    return result;
}



// Scene Lighting

vec3 g_vSunDir = normalize(vec3(0.3, 0.4, -0.5));
vec3 g_vSunColor = vec3(1, 0.95, 0.8) * 3.0;
vec3 g_vAmbientColor = vec3(0.8, 0.8, 0.8) * 1.0;

SurfaceLighting Scene_GetSurfaceLighting( const in vec3 vViewDir, in SurfaceInfo surfaceInfo )
{
    SurfaceLighting surfaceLighting;
    
    surfaceLighting.vDiffuse = vec3(0.0);
    surfaceLighting.vSpecular = vec3(0.0);    
    
    Light_AddDirectional( surfaceLighting, surfaceInfo, vViewDir, g_vSunDir, g_vSunColor );
    
    Light_AddPoint( surfaceLighting, surfaceInfo, vViewDir, vec3(1.4, 2.0, 0.8), vec3(1,1,1) * 0.2 );
    
    float fAO = Scene_GetAmbientOcclusion( surfaceInfo.vPos, surfaceInfo.vNormal );
    // AO
    surfaceLighting.vDiffuse += fAO * (surfaceInfo.vBumpNormal.y * 0.5 + 0.5) * g_vAmbientColor;
    
    return surfaceLighting;
}

// Environment

vec4 Env_GetSkyColor( const vec3 vViewPos, const vec3 vViewDir )
{
	vec4 vResult = vec4( 0.0, 0.0, 0.0, kFarDist );

#if 1
    vec3 vEnvMap = textureLod( iChannel1, vViewDir.zyx, 0.0 ).rgb;
    vResult.rgb = vEnvMap;
#endif    
    
#if 0
    vec3 vEnvMap = textureLod( iChannel1, vViewDir.zyx, 0.0 ).rgb;
    vEnvMap = vEnvMap * vEnvMap;
    float kEnvmapExposure = 0.999;
    vResult.rgb = -log2(1.0 - vEnvMap * kEnvmapExposure);

#endif
    
    // Sun
    //float NdotV = dot( g_vSunDir, vViewDir );
    //vResult.rgb += smoothstep( cos(radians(.7)), cos(radians(.5)), NdotV ) * g_vSunColor * 5000.0;

    return vResult;	
}

float Env_GetFogFactor(const in vec3 vRayOrigin,  const in vec3 vRayDir, const in float fDist )
{    
	float kFogDensity = 0.00001;
	return exp(fDist * -kFogDensity);	
}

vec3 Env_GetFogColor(const in vec3 vDir)
{    
	return vec3(0.2, 0.5, 0.6) * 2.0;		
}

vec3 Env_ApplyAtmosphere( const in vec3 vColor, const in vec3 vRayOrigin,  const in vec3 vRayDir, const in float fDist )
{
    //return vColor;
    vec3 vResult = vColor;
    
    
	float fFogFactor = Env_GetFogFactor( vRayOrigin, vRayDir, fDist );
	vec3 vFogColor = Env_GetFogColor( vRayDir );	
	//Env_AddDirectionalLightFlareToFog( vFogColor, vRayDir, g_vSunDir, g_vSunColor * 3.0);    
    vResult = mix( vFogColor, vResult, fFogFactor );

    return vResult;	    
}


vec3 FX_Apply( in vec3 vColor, const in vec3 vRayOrigin,  const in vec3 vRayDir, const in float fDist)
{    
    return vColor;
}


vec4 MainCommon( vec3 vRayOrigin, vec3 vRayDir )
{
	vec4 vColorLinAndDepth = Scene_GetColorAndDepth( vRayOrigin, vRayDir );    
    vColorLinAndDepth.rgb = max( vColorLinAndDepth.rgb, vec3(0.0) );
    
    vec4 vFragColor = vColorLinAndDepth;
    
    float fExposure = 2.0f;
    
    vFragColor.rgb *= fExposure;
    
    vFragColor.a = vColorLinAndDepth.w;
    
    return vFragColor;
}

CameraState GetCameraPosition( int index )
{
    CameraState cam;

    vec3 vFocus = vec3(0,0.25,-0.012);   
    
    if ( index > 9 )
    {
    	index = int(hash11(float(index) / 10.234) * 100.0);
    	index = index % 10;
    }

    //index=2;
    
    if ( index == 0 )
    {
        cam.vPos = vec3(-0.1,0.2,-0.08);
        cam.vTarget = vec3(0,0.25,0.1);
        cam.fFov = 10.0;
    }
    if ( index == 1 )
    {
        cam.vPos = vec3(0.01,0.334,-0.03);
        cam.vTarget = vec3(0,0.3,0.1);
        cam.fFov = 10.0;
    }
    if ( index == 2 )
    {
        cam.vPos = vec3(-0.8,0.3,-1.0);
        cam.vTarget = vec3(0.4,0.18,0.5);
        cam.fFov = 10.0;
    }
    if ( index == 3 )
    {
        cam.vPos = vec3(-0.8,1.0,-1.5);
        cam.vTarget = vec3(0.2,0.0,0.5);
        cam.fFov = 10.0;
    }
    if ( index == 4 )
    {
        cam.vPos = vec3(-0.8,0.3,-1.0);
        cam.vTarget = vec3(0.4,0.18,0.5);
        cam.fFov = 20.0;
    }
    if ( index == 5 )
    {
        cam.vPos = vec3(-0.244,0.334,-0.0928);
        cam.vTarget = vec3(0,0.25,0.1);
        cam.fFov = 20.0;
    }
    if ( index == 6 )
    {
        cam.vPos = vec3(0.0,0.1,-0.5);
        cam.vTarget = vec3(0.2,0.075,-0.1);
        vFocus = cam.vTarget; 
        cam.fFov = 15.0;
    }
    if ( index == 7 )
    {
        cam.vPos = vec3(-0.01,0.01,-0.25);
        cam.vTarget = vec3(0.01,0.27,0.1);
        vFocus = cam.vTarget; 
        cam.fFov = 23.0;
    }
    if ( index == 8 )
    {
        cam.vPos = vec3(-0.23,0.3,-0.05);
        cam.vTarget = vec3(0.1,0.2,0.1);
        cam.fFov = 15.0;
    }
    if ( index == 9 )
    {
        cam.vPos = vec3(0.4,0.2,-0.2);
        cam.vTarget = vec3(-0.1,0.25,0.1);
        cam.fFov = 12.0;
    }
    
    cam.fPlaneInFocus = length( vFocus - cam.vPos);
    cam.vJitter = vec2(0.0);        
    
    return cam;
}

void mainImage( out vec4 vFragColor, in vec2 vFragCoord )
{
    vec2 vUV = vFragCoord.xy / iResolution.xy; 

    CameraState cam;
    
    {
    	CameraState camA;
    	CameraState camB;
    
        float fSeqTime = iTime;
        float fSequenceSegLength = 5.0;
        float fSeqIndex = floor(fSeqTime / fSequenceSegLength);
        float fSeqPos = fract(fSeqTime / fSequenceSegLength);
        int iIndex = int(fSeqIndex);
		int iIndexNext = int(fSeqIndex) + 1;
        camA = GetCameraPosition(iIndex);
        camB = GetCameraPosition(iIndexNext);
        
        float t = smoothstep(0.3, 1.0, fSeqPos);
        cam.vPos = mix(camA.vPos, camB.vPos, t );
        cam.vTarget = mix(camA.vTarget, camB.vTarget, t );
        cam.fFov = mix(camA.fFov, camB.fFov, t );
        cam.fPlaneInFocus = mix(camA.fPlaneInFocus, camB.fPlaneInFocus, t );
    }
    
    if ( iMouse.z > 0.0 )
    {
        float fDist = 0.01 + 3.0 * (iMouse.y / iResolution.y);

        float fAngle = (iMouse.x / iResolution.x) * radians(360.0);
    	//float fElevation = (iMouse.y / iResolution.y) * radians(90.0);
    	float fElevation = 0.15f * radians(90.0);    

        cam.vPos = vec3(sin(fAngle) * fDist * cos(fElevation),sin(fElevation) * fDist,cos(fAngle) * fDist * cos(fElevation));
        cam.vTarget = vec3(0,0.25,0.1);
        cam.vPos +=cam.vTarget;
        cam.fFov = 20.0 / (1.0 + fDist * 0.5);
    	vec3 vFocus = vec3(0,0.25,-0.012);	    
	    cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
    
#if 0
    {
        float fDist = 0.5;

        float fAngle = 0.6 * PI * 2.0f;
        float fElevation = 0.2;
        
        cam.vPos = vec3(sin(fAngle) * fDist * cos(fElevation),sin(fElevation) * fDist,cos(fAngle) * fDist * cos(fElevation));
        cam.vTarget = vec3(0.05,0.25,0.1);
        cam.vPos +=cam.vTarget;
        cam.fFov = 22.0;
    	vec3 vFocus = vec3(0,0.25,-0.012);	    
	    cam.fPlaneInFocus = length( vFocus - cam.vPos );
    }
#endif
    
#ifdef ENABLE_TAA_JITTER
    cam.vJitter = hash21( fract( iTime ) ) - 0.5f;
#endif
    
            
    vec3 vRayOrigin, vRayDir;
    vec2 vJitterUV = vUV + cam.vJitter / iResolution.xy;
    Cam_GetCameraRay( vJitterUV, iResolution.xy, cam, vRayOrigin, vRayDir );
 
    float fHitDist = 0.0f;
    vFragColor = MainCommon( vRayOrigin, vRayDir );
    
    
	Cam_StoreState( ivec2(0), cam, vFragColor, ivec2(vFragCoord.xy) );    
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Temporal Anti-aliasing Pass

#define ENABLE_TAA

#define iChannelCurr iChannel0
#define iChannelHistory iChannel1

vec3 Tonemap( vec3 x )
{
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );
}

vec3 TAA_ColorSpace( vec3 color )
{
    return Tonemap(color);
}


void mainImage( out vec4 vFragColor, in vec2 vFragCoord )
{
    CameraState camCurr;
	Cam_LoadState( camCurr, iChannelCurr, ivec2(0) );
    
    CameraState camPrev;
	Cam_LoadState( camPrev, iChannelHistory, ivec2(0) );

    vec2 vUV = vFragCoord.xy / iResolution.xy;
 	vec2 vUnJitterUV = vUV - camCurr.vJitter / iResolution.xy;    
    
    vFragColor = textureLod(iChannelCurr, vUnJitterUV, 0.0);
    
    
#ifdef ENABLE_TAA
    vec3 vRayOrigin, vRayDir;
    Cam_GetCameraRay( vUV, iResolution.xy, camCurr, vRayOrigin, vRayDir );    
    float fDepth;
    int iObjectId;
    vec4 vCurrTexel = texelFetch( iChannelCurr, ivec2(vFragCoord.xy), 0);
    fDepth = DecodeDepthAndObjectId( vCurrTexel.w, iObjectId );
    vec3 vWorldPos = vRayOrigin + vRayDir * fDepth;
    
    vec2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev), iResolution.xy );// + camPrev.vJitter / iResolution.xy;
        
    if ( all( greaterThanEqual( vPrevUV, vec2(0) )) && all( lessThan( vPrevUV, vec2(1) )) )
	{
        vec3 vMin = vec3( 10000);
        vec3 vMax = vec3(-10000);
        
	    ivec2 vCurrXY = ivec2(floor(vFragCoord.xy));    
        
        int iNeighborhoodSize = 1;
        for ( int iy=-iNeighborhoodSize; iy<=iNeighborhoodSize; iy++)
        {
            for ( int ix=-iNeighborhoodSize; ix<=iNeighborhoodSize; ix++)
            {
                ivec2 iOffset = ivec2(ix, iy);
		        vec3 vTest = TAA_ColorSpace( texelFetch( iChannelCurr, vCurrXY + iOffset, 0 ).rgb );
                                
                vMin = min( vMin, vTest );
                vMax = max( vMax, vTest );
            }
        }
        
        float epsilon = 0.001;
        vMin -= epsilon;
        vMax += epsilon;
        
        float fBlend = 0.0f;
        
        //ivec2 vPrevXY = ivec2(floor(vPrevUV.xy * iResolution.xy));
        vec4 vHistory = textureLod( iChannelHistory, vPrevUV, 0.0 );

        vec3 vPrevTest = TAA_ColorSpace( vHistory.rgb );
        if( all( greaterThanEqual(vPrevTest, vMin ) ) && all( lessThanEqual( vPrevTest, vMax ) ) )
        {
            fBlend = 0.9;
            //vFragColor.r *= 0.0;
        }
        
        vFragColor.rgb = mix( vFragColor.rgb, vHistory.rgb, fBlend);
    }  
    else
    {
        //vFragColor.gb *= 0.0;
    }

#endif
    
    vFragColor.rgb += (hash13( vec3( vFragCoord, iTime ) ) * 2.0 - 1.0) * 0.03;
    
	Cam_StoreState( ivec2(0), camCurr, vFragColor, ivec2(vFragCoord.xy) );    
	Cam_StoreState( ivec2(3,0), camPrev, vFragColor, ivec2(vFragCoord.xy) );    
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.141592654


///////////////////////////
// Hash Functions
///////////////////////////

// From: Hash without Sine by Dave Hoskins
// https://www.shadertoy.com/view/4djSRW

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
//#define HASHSCALE1 .1031
//#define HASHSCALE3 vec3(.1031, .1030, .0973)
//#define HASHSCALE4 vec4(1031, .1030, .0973, .1099)

// For smaller input rangers like audio tick or 0-1 UVs use these...
#define HASHSCALE1 443.8975
#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
#define HASHSCALE4 vec3(443.897, 441.423, 437.195, 444.129)


//----------------------------------------------------------------------------------------
//  1 out, 1 in...
float hash11(float p)
{
	vec3 p3  = fract(vec3(p) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//  2 out, 1 in...
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * HASHSCALE3);
	p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//  1 out, 3 in...
float hash13(vec3 p3)
{
	p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}


///////////////////////////
// Data Storage
///////////////////////////

vec4 LoadVec4( sampler2D sampler, in ivec2 vAddr )
{
    return texelFetch( sampler, vAddr, 0 );
}

vec3 LoadVec3( sampler2D sampler, in ivec2 vAddr )
{
    return LoadVec4( sampler, vAddr ).xyz;
}

bool AtAddress( ivec2 p, ivec2 c ) { return all( equal( p, c ) ); }

void StoreVec4( in ivec2 vAddr, in vec4 vValue, inout vec4 fragColor, in ivec2 fragCoord )
{
    fragColor = AtAddress( fragCoord, vAddr ) ? vValue : fragColor;
}

void StoreVec3( in ivec2 vAddr, in vec3 vValue, inout vec4 fragColor, in ivec2 fragCoord )
{
    StoreVec4( vAddr, vec4( vValue, 0.0 ), fragColor, fragCoord);
}

///////////////////////////
// Camera
///////////////////////////

struct CameraState
{
    vec3 vPos;
    vec3 vTarget;
    float fFov;
    vec2 vJitter;
    float fPlaneInFocus;
};
    
void Cam_LoadState( out CameraState cam, sampler2D sampler, ivec2 addr )
{
    vec4 vPos = LoadVec4( sampler, addr + ivec2(0,0) );
    cam.vPos = vPos.xyz;
    vec4 targetFov = LoadVec4( sampler, addr + ivec2(1,0) );
    cam.vTarget = targetFov.xyz;
    cam.fFov = targetFov.w;
    vec4 jitterDof = LoadVec4( sampler, addr + ivec2(2,0) );
    cam.vJitter = jitterDof.xy;
    cam.fPlaneInFocus = jitterDof.z;
}

void Cam_StoreState( ivec2 addr, const in CameraState cam, inout vec4 fragColor, in ivec2 fragCoord )
{
    StoreVec4( addr + ivec2(0,0), vec4( cam.vPos, 0 ), fragColor, fragCoord );
    StoreVec4( addr + ivec2(1,0), vec4( cam.vTarget, cam.fFov ), fragColor, fragCoord );    
    StoreVec4( addr + ivec2(2,0), vec4( cam.vJitter, cam.fPlaneInFocus, 0 ), fragColor, fragCoord );    
}

mat3 Cam_GetWorldToCameraRotMatrix( const CameraState cameraState )
{
    vec3 vForward = normalize( cameraState.vTarget - cameraState.vPos );
	vec3 vRight = normalize( cross(vec3(0, 1, 0), vForward) );
	vec3 vUp = normalize( cross(vForward, vRight) );
    
    return mat3( vRight, vUp, vForward );
}

vec2 Cam_GetViewCoordFromUV( vec2 vUV, vec2 res )
{
	vec2 vWindow = vUV * 2.0 - 1.0;
	vWindow.x *= res.x / res.y;

	return vWindow;	
}

void Cam_GetCameraRay( vec2 vUV, vec2 res, CameraState cam, out vec3 vRayOrigin, out vec3 vRayDir )
{
    vec2 vView = Cam_GetViewCoordFromUV( vUV, res );
    vRayOrigin = cam.vPos;
    float fPerspDist = 1.0 / tan( radians( cam.fFov ) );
    vRayDir = normalize( Cam_GetWorldToCameraRotMatrix( cam ) * vec3( vView, fPerspDist ) );
}

vec2 Cam_GetUVFromWindowCoord( vec2 vWindow, vec2 res )
{
    vec2 vScaledWindow = vWindow;
    vScaledWindow.x *= res.y / res.x;

    return (vScaledWindow * 0.5 + 0.5);
}

vec2 Cam_WorldToWindowCoord(const in vec3 vWorldPos, const in CameraState cameraState )
{
    vec3 vOffset = vWorldPos - cameraState.vPos;
    vec3 vCameraLocal;

    vCameraLocal = vOffset * Cam_GetWorldToCameraRotMatrix( cameraState );
	
    vec2 vWindowPos = vCameraLocal.xy / (vCameraLocal.z * tan( radians( cameraState.fFov ) ));
    
    return vWindowPos;
}

float EncodeDepthAndObject( float depth, int objectId )
{
    //depth = max( 0.0, depth );
    //objectId = max( 0, objectId + 1 );
    //return exp2(-depth) + float(objectId);
    return depth;
}

float DecodeDepthAndObjectId( float value, out int objectId )
{
    objectId = 0;
    return max(0.0, value);
    //objectId = int( floor( value ) ) - 1; 
    //return abs( -log2(fract(value)) );
}

///////////////////////////////