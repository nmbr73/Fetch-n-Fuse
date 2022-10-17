

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of "Lovely Bubbles Fawq" by xenn. https://shadertoy.com/view/fddSz7
// 2021-09-30 10:00:20

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec3 ob, nb, a;
    ob = texture(iChannel0, uv).rgb;
    nb = texture(iChannel1, uv).rgb;
    vec3 xb = texture(iChannel2, uv).rgb;
    xb += (xb * xb) - xb;
    
    //a = col * len
    a = vec3(0.5 * (abs(sin(iTime / 3.0))), 0.25, 0.5 * (1.1 * (abs(cos(iTime / 3.0))))) * .5;

    fragColor = min(vec4(mix(ob, nb, (a / 0.50)), 1.),(xb,1.));
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
#if USE_THIN_FILM_LOOKUP
    {
        int segmentCount = 32;
        int segment = iFrame % segmentCount;
        int currSegment = int(floor((fragCoord.y * float(segmentCount) / iResolution.y)));
        
        if ( segment != currSegment )
        {
            fragColor = texelFetch( iChannel0, ivec2(fragCoord), 0 );
            return;
        }
    }

    vec2 uv = fragCoord/iResolution.xy;
        
    vec3 result = GetThinFilmColour(uv.x, uv.y);  

    fragColor = vec4(result,1.0);
#else
    discard;
#endif
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define iFeedbackColorShiftZoom 0.058
//#define iFeedbackColorShiftImpact 0.001
#define iBlob1ColorPulseSpeed 0.03456
#define iBlob2ColorPulseSpeed -0.02345
#define Margins .0











/*
 * Conway Ticket
 * 
 * Copyright (C) 2021  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
ivec2 boardSize = ivec2(125),
    ci = ivec2(1,0);
vec2 blockSize,
    xij;
vec3 c = vec3(1,0,-1);
float stepTimeDelta = .05,
    pi = 3.14159,
    fsaa = 144.,
    bpm = 90.,
    spb = 60./90.,
    scale,
    nbeats,
    stepTime;

// Creative Commons Attribution-ShareAlike 4.0 International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
float hash12(vec2 p)
{
	vec3 p3  = fract(p.xyx * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float lfnoise(float y)
{
    vec2 t = y*c.xx;
    vec2 i = floor(t);
    t = smoothstep(c.yy, c.xx, fract(t));
    vec2 v1 = vec2(hash12(i), hash12(i+c.xy)),
    v2 = vec2(hash12(i+c.yx), hash12(i+c.xx));
    v1 = c.zz+2.*mix(v1, v2, t.y);
    return mix(v1.x, v1.y, t.x);
}

float m(vec2 x)
{
    return max(x.x,x.y);
}

float d210(vec2 x)
{
    return min(max(max(max(max(min(max(max(m(abs(vec2(abs(abs(x.x)-.25)-.25, x.y))-vec2(.2)), -m(abs(vec2(x.x+.5, abs(abs(x.y)-.05)-.05))-vec2(.12,.02))), -m(abs(vec2(abs(x.x+.5)-.1, x.y-.05*sign(x.x+.5)))-vec2(.02,.07))), m(abs(vec2(x.x+.5,x.y+.1))-vec2(.08,.04))), -m(abs(vec2(x.x, x.y-.04))-vec2(.02, .08))), -m(abs(vec2(x.x, x.y+.1))-vec2(.02))), -m(abs(vec2(x.x-.5, x.y))-vec2(.08,.12))), -m(abs(vec2(x.x-.5, x.y-.05))-vec2(.12, .07))), m(abs(vec2(x.x-.5, x.y))-vec2(.02, .08)));
}

float dbox3(vec3 x, vec3 b)
{
  b = abs(x) - b;
  return length(max(b,0.))
         + min(max(b.x,max(b.y,b.z)),0.);
}

float setStateF(ivec2 index, ivec2 where, float oldState, float newState)
{
    return all(equal(index, where)) ? newState : oldState;
}

// Distance to star
float dstar(vec2 x, float N, vec2 R)
{
    float d = pi/N,
        p0 = acos(x.x/length(x)),
        p = mod(p0, d);
    vec2 a = mix(R,R.yx,mod(round((p-p0)/d),2.)),
    	p1 = a.x*c.xy,
        ff = a.y*vec2(cos(d),sin(d))-p1;
    return dot(length(x)*vec2(cos(p),sin(p))-p1,ff.yx*c.zx)/length(ff);
}

float dhexagonpattern(vec2 p) 
{
    vec2 q = vec2(p.x*1.2, p.y + p.x*.6),
        qi = floor(q),
        pf = fract(q);
    float v = mod(qi.x + qi.y, 3.);
    
    return dot(step(pf.xy,pf.yx), 1.-pf.yx + step(1.,v)*(pf.x+pf.y-1.) + step(2.,v)*(pf.yx-2.*pf.xy));
}

// x: material
// y: distance
// z: reflectivity
vec3 add(vec3 a, vec3 b)
{
    if(a.y < b.y) return a;
    return b;
}

vec3 hsv2rgb(vec3 cc)
{
    vec4 K = vec4(1., 2. / 3., 1. / 3., 3.);
    vec3 p = abs(fract(cc.xxx + K.xyz) * 6. - K.www);
    return cc.z * mix(K.xxx, clamp(p - K.xxx, 0., 1.), cc.y);
}

vec2 rgb2sv(vec3 cc)
{
    vec4 K = vec4(0., -1. / 3., 2. / 3., -1.),
        p = mix(vec4(cc.bg, K.wz), vec4(cc.gb, K.xy), step(cc.b, cc.g)),
        q = mix(vec4(p.xyw, cc.r), vec4(cc.r, p.yzx), step(p.x, cc.r));
    return vec2((q.x - min(q.w, q.y)) / (q.x + 1.e-10), q.x);
}



#define pi acos(-1.)


#define sint(a) (asin(sin(a))*2. - 1.)

#define rot(a) mat2(cos(a),-sin(a),sin(a),cos(a))

//#define pmod(p,d) mod(p - (d)*0.5, (d)) - 0.5*(d)

float r11(float i){ return fract(sin(i*12.126)*12.6);}

#define xor(a,b,c) min(max((a),-(b)), max((b),-(a) - c)) 

float ss( float c, float power, float bias){
    c = clamp(c,-0.,1.);
    //c = smoothstep(0.,1.,c);
    
    c = pow(c,1. + bias);
    
    float a = pow( abs(c), power);
    float b = 1.-pow( abs(c - 1.), power);
    
    return mix(a,b,c);
}
float valueNoise(float i, float p){ return mix(r11(floor(i)),r11(floor(i) + 1.), ss(fract(i), p,0.6));}

float valueNoiseStepped(float i, float p, float steps){ return mix(  floor(r11(floor(i))*steps)/steps, floor(r11(floor(i) + 1.)*steps)/steps, ss(fract(i), p,0.6));}


// See: https://www.shadertoy.com/view/ls2Bz1
// Spectral Colour Schemes
// By Alan Zucconi
// Website: www.alanzucconi.com
// Twitter: @AlanZucconi

// Example of different spectral colour schemes
// to convert visible wavelengths of light (400-700 nm) to RGB colours.

// The function "spectral_zucconi6" provides the best approximation
// without including any branching.
// Its faster version, "spectral_zucconi", is advised for mobile applications.


// Read "Improving the Rainbow" for more information
// http://www.alanzucconi.com/?p=6703



float saturate (float x)
{
    return min(1.0, max(0.0,x));
}
vec3 saturate (vec3 x)
{
    return min(vec3(1.,1.,1.), max(vec3(0.,0.,0.),x));
}

// --- Spectral Zucconi --------------------------------------------
// By Alan Zucconi
// Based on GPU Gems: https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch08.html
// But with values optimised to match as close as possible the visible spectrum
// Fits this: https://commons.wikimedia.org/wiki/File:Linear_visible_spectrum.svg
// With weighter MSE (RGB weights: 0.3, 0.59, 0.11)
vec3 bump3y (vec3 x, vec3 yoffset)
{
	vec3 y = vec3(1.,1.,1.) - x * x;
	y = saturate(y-yoffset);
	return y;
}

// --- Spectral Zucconi 6 --------------------------------------------

// Based on GPU Gems
// Optimised by Alan Zucconi
vec3 spectral_zucconi6 (float x)
{
	// w: [400, 700]
	// x: [0,   1]

	const vec3 c1 = vec3(3.54585104, 2.93225262, 2.41593945);
	const vec3 x1 = vec3(0.69549072, 0.49228336, 0.27699880);
	const vec3 y1 = vec3(0.02312639, 0.15225084, 0.52607955);

	const vec3 c2 = vec3(3.90307140, 3.21182957, 3.96587128);
	const vec3 x2 = vec3(0.11748627, 0.86755042, 0.66077860);
	const vec3 y2 = vec3(0.84897130, 0.88445281, 0.73949448);

	return
		bump3y(c1 * (x - x1), y1) +
		bump3y(c2 * (x - x2), y2) ;
}

// Use a lookup texture in Buffer A for thin film interference instead of calculating it at every intersection
#define USE_THIN_FILM_LOOKUP 0

float N_Air = 1.0f;
float N_Water = 1.33f;

float PI = 3.141592654;

// used to prevent loop unrolling
// This will be zero but the compiler doesn't know that as iFrame is a uniform
#define ZERO min(iFrame,0)

// https://en.wikipedia.org/wiki/Fresnel_equations
float FresnelS(float ni, float nt, float cosi, float cost)
{
    return ((nt * cosi) - (ni * cost)) / ((nt * cosi) + (ni * cost));
}

float FresnelP(float ni, float nt, float cosi, float cost)
{
    return ((ni * cosi) - (nt * cost)) / ((ni * cosi) + (nt * cost));
}

float Fresnel(float ni, float nt, float cosi, float cost )
{    
    float Rs = FresnelS( ni, nt, cosi, cost );
    float Rp = FresnelP( ni, nt, cosi, cost );

    return (Rs * Rs + Rp * Rp) * 0.5;
}

float FresnelR0(float ni, float nt)
{
    float R0 = (ni-nt) / (ni+nt);
    R0 *= R0;
    return R0;
}

// https://en.wikipedia.org/wiki/Snell%27s_law
float GetCosT( float ni, float nt, float cosi )
{
    float n = ni/nt;
    float sinT2 = n*n*(1.0-cosi*cosi);
    
    // Total internal reflection
    if (sinT2 >= 1.0)
    {
        return 1.0;
    } 

    float cost = sqrt(1.0 - sinT2);
    return cost;
}


// https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

float SmoothNoise3d(vec3 p)
{
    vec3 fl = floor(p);
    vec3 fr = p - fl;
    
    vec3 ot = fr*fr*(3.0-2.0*fr);
    vec3 zt = 1.0f - ot;
    
    
    float result = 0.0f;
    
    result += hash13(fl + vec3(0,0,0)) * (zt.x * zt.y * zt.z);
    result += hash13(fl + vec3(1,0,0)) * (ot.x * zt.y * zt.z);

    result += hash13(fl + vec3(0,1,0)) * (zt.x * ot.y * zt.z);
    result += hash13(fl + vec3(1,1,0)) * (ot.x * ot.y * zt.z);

    result += hash13(fl + vec3(0,0,1)) * (zt.x * zt.y * ot.z);
    result += hash13(fl + vec3(1,0,1)) * (ot.x * zt.y * ot.z);

    result += hash13(fl + vec3(0,1,1)) * (zt.x * ot.y * ot.z);
    result += hash13(fl + vec3(1,1,1)) * (ot.x * ot.y * ot.z);

    return result;
}

const mat3 m3 = mat3( 0.00,  0.80,  0.60,
					-0.80,  0.36, -0.48,
					-0.60, -0.48,  0.64 );

float Noise(vec3 p, float o)
{
    float result = 0.0f;
    float a = 1.0f;
    float t= 0.0;
    float f = 0.5;
    float s= 2.0f;
    
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    p.x += o;
    result += SmoothNoise3d(p) * a; t+= a; p = m3 * p * s; a = a * f;
    result = result / t;
    
    return result;
}



// Spectrum to xyz approx function from http://jcgt.org/published/0002/02/01/paper.pdf
// Inputs:  Wavelength in nanometers
float xFit_1931( float wave )
{
    float t1 = (wave-442.0)*((wave<442.0)?0.0624:0.0374),
          t2 = (wave-599.8)*((wave<599.8)?0.0264:0.0323),
          t3 = (wave-501.1)*((wave<501.1)?0.0490:0.0382);
    return 0.362*exp(-0.5*t1*t1) + 1.056*exp(-0.5*t2*t2)- 0.065*exp(-0.5*t3*t3);
}
float yFit_1931( float wave )
{
    float t1 = (wave-568.8)*((wave<568.8)?0.0213:0.0247),
          t2 = (wave-530.9)*((wave<530.9)?0.0613:0.0322);
    return 0.821*exp(-0.5*t1*t1) + 0.286*exp(-0.5*t2*t2);
}
float zFit_1931( float wave )
{
    float t1 = (wave-437.0)*((wave<437.0)?0.0845:0.0278),
          t2 = (wave-459.0)*((wave<459.0)?0.0385:0.0725);
    return 1.217*exp(-0.5*t1*t1) + 0.681*exp(-0.5*t2*t2);
}

#define xyzFit_1931(w) vec3( xFit_1931(w), yFit_1931(w), zFit_1931(w) ) 

vec3 XYZtosRGB( vec3 XYZ )
{
    // XYZ to sRGB
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
   mat3 m = mat3 (
        3.2404542, -1.5371385, -0.4985314,
		-0.9692660,  1.8760108,  0.0415560,
 		0.0556434, -0.2040259,  1.0572252 );
    
    return XYZ * m;
}

vec3 WavelengthToXYZ( float f )
{    
    return xyzFit_1931( f );    
}



// from  https://github.com/amandaghassaei/SoapFlow/blob/main/python/Thin%20Film%20Interference.ipynb
float ThinFilmAmplitude( float wavelength, float thickness, float cosi )
{
    float ni = N_Air;
    float nt = N_Water;
    
    float cost = GetCosT( ni, nt, cosi );

    // # The wavelength inside a medium is scaled by the index of refraction.
    // wavelength_soap = wavelength / n_soap
    // wavelength_air = wavelength / n_air
    // # First calc phase shift of reflection at rear surface, based on film thickness.
    // phaseDelta = 2 * thickness / math.cos(theta) * 2 * math.pi / wavelength_soap  
    // # There is an additional path to compute, the segment AJ from:
    // # https://www.glassner.com/wp-content/uploads/2014/04/CG-CGA-PDF-00-11-Soap-Bubbles-2-Nov00.pdf
    // phaseDelta -= 2 * thickness * math.tan(theta) * math.sin(incidentAngle) * 2 * math.pi / wavelength_air
    // Simplified to:
    float phaseDelta = 2.0 * thickness * nt * cost * 2.0 * PI / wavelength;
    
    // https://en.wikipedia.org/wiki/Reflection_phase_change
    if (ni < nt)
        phaseDelta -= PI;
    if (ni > nt)
        phaseDelta += PI;

    float front_refl_amp = Fresnel(cosi, cost, ni, nt);
    float front_trans_amp = 1.0 - front_refl_amp;
    float rear_refl_amp = front_trans_amp * Fresnel(cost, cosi, nt, ni);
    
    rear_refl_amp /= front_refl_amp;
    front_refl_amp = 1.0f;
        
    // http://scipp.ucsc.edu/~haber/ph5B/addsine.pdf
    return sqrt(front_refl_amp * front_refl_amp + rear_refl_amp * rear_refl_amp + 2.0 * front_refl_amp * rear_refl_amp * cos(phaseDelta));
}

vec3 GetThinFilmColour( float cosi, float thicknessN )
{
    float thicknessMin = 100.0;//1.0f;
    float thicknessMax = 1500.0;//2500.0f;
    
    float thickness = mix(thicknessMin, thicknessMax, thicknessN);

    vec3 result = vec3(0.0);
    
    float t = 0.0;
    
    vec3 white = vec3(0.0);
    
    for (float wavelength = 380.0; wavelength<=780.0; wavelength += 50.0)
    {
        float amplitude = ThinFilmAmplitude( wavelength, thickness, cosi );
        
        vec3 XYZ = WavelengthToXYZ( wavelength );
    
        white += XYZ;
    
        result += XYZ * amplitude;
        t += 1.0f;
    }

    result = XYZtosRGB( result );
      
    result /= t;
    //result /= white;
    //result = vec3(1.0);
    
    return result;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Lovely Bubbles
// by @P_Malin
// https://www.shadertoy.com/view/Nl2SRc
//
// Some lovely shadertoy bubbles.
// I've wanted to implement something with thin film interference for a while.


// CAMERA

vec2 GetWindowCoord( vec2 uv )
{
	vec2 window = uv * 2.0 - 1.0;
	window.x *= iResolution.x / iResolution.y;

	return window;	
}

vec3 GetCameraRayDir( vec2 window, vec3 cameraPos, vec3 cameraTarget, float fov )
{
	vec3 forward = normalize( cameraTarget - cameraPos );
	vec3 right = normalize( cross( vec3(0.0, 1.0, 0.0), forward ) );
	vec3 up = normalize( cross( forward, right ) );
							  
	vec3 dir = normalize(window.x * right + window.y * up + forward * fov);

	return dir;
}


// POSTFX

float Vignette( vec2 uv, float size )
{
    float d = length( (uv - 0.5f) * 2.0f ) / length(vec2(1.0));
    
    d /= size;
    
    float s = d * d * ( 3.0f - 2.0f * d );
    
    float v = mix ( d, s, 0.6f );
    
    return max(0.0, 1.0f - v);
}

vec3 ApplyTonemap( vec3 linearCol )
{
	const float kExposure = 0.5;
	
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    vec3 x = linearCol * kExposure;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
}

vec3 ApplyGamma( vec3 linearCol )
{
	const float kGamma = 2.2;

	return pow( linearCol, vec3(1.0/kGamma) );	
}

vec3 ApplyPostFX( vec2 uv, vec3 col )
{    
    col *= 1.3;

    col *= 0.1 + 0.9 * Vignette( uv, 1.0 );

    col *= vec3(1.0, 0.95, 0.8); // warmer
  
    col = ApplyTonemap(col);
	col = ApplyGamma(col);
    
	return col;
}
	


// Scene

float speed = 1.0;

float BubbleOriginForward( float t )
{
    t = t * 30.0;
    if ( t > 0.0)
    {
        t = t / (1.0+t/10.0f);

    }
    return t + iTime * speed;
}

float BubbleOriginInverse( float r )
{
    r = r- iTime * speed;
    if( r > 0.0)
    {
        r = -10.0f * r / (r - 10.0f);
    }
    r = r / 30.0f;
    return r;
}

float Scene_Distance(vec3 pos)
{

    vec3 vPos = pos;
    vPos.x += 3.0;

    float scale = 50.0;
    
    vPos /= scale;

    // wobble
    vec3 offset = vec3(0);
    offset += sin( pos.yzx * 8.91 + iTime * 10.0 ) * 0.001;
    offset += sin( pos.zxy * 7.89 + iTime * 10.0 ) * 0.001;    
    offset *= 0.08;
    
    float f = BubbleOriginForward( vPos.x );
    
    f = floor(f);
    
    float minD = 1000000.0;
    
    for (float b=-1.0; b<=2.0; b+=1.0)
    {
        float p = f + b;
        vec3 o = vPos;
        o.x = BubbleOriginInverse( p );
                
        o.x -= vPos.x;

         float spreadBlend = 1.0 - clamp( vPos.x * 3.0 + 0.2, 0.0, 1.0);
         
         float spread = spreadBlend;
         
         spread *= 0.05;

         o.y += sin(p * 123.3456) * spread;
         o.z += sin(p * 234.5678) * spread;
         
         o += offset;
           
         float rad = sin( p * 456.8342 ) * 0.5 + 0.5;
                             
         float d = length(o) - 0.005f - rad * rad * 0.02f;
         
         minD = min( minD, d );
    }
    
     return minD * scale;
}

vec3 Scene_GetNormal( vec3 pos )
{
    const float delta = 0.0001;
    
    vec4 samples;
    for( int i=ZERO; i<=4; i++ )
    {
        vec4 offset = vec4(0);
        offset[i] = delta;
        samples[i] = Scene_Distance( pos + offset.xyz );
    }
    
    vec3 normal = samples.xyz - samples.www;    
    return normalize( normal );
}    

float Scene_Trace( vec3 rayOrigin, vec3 rayDir, float minDist, float maxDist, float side )
{
	float t = minDist;

    const int kRaymarchMaxIter = 128;
	for(int i=0; i<kRaymarchMaxIter; i++)
	{		
        float epsilon = 0.0001 * t;
		float d = Scene_Distance( rayOrigin + rayDir * t ) * side;
        if ( abs(d) < epsilon )
		{
			break;
		}
                        
        if ( t > maxDist )
        {
	        t = maxDist + 1.0f;
            break;
        }       
        
        t += d;        
	}
    
    return t;
}

vec3 GetSkyColour( vec3 dir )
{
	vec3 result = vec3(0.0);
	
    vec3 envMap = texture(iChannel1, dir).rgb;
    envMap = envMap * envMap;
    float kEnvmapExposure = 0.99999;
    result = -log2(1.0 - envMap * kEnvmapExposure);

    return result;	
}

float FilmThickness( vec3 pos )
{
    return Noise(pos * 0.3f, iTime * 0.5);
}

void Shade( inout vec3 colour, inout vec3 remaining, vec3 pos, vec3 rayDir, vec3 normal )
{
    float NdotV = max( dot(normal, -rayDir), 0.0 );

    float filmThickness = FilmThickness(pos);

    vec3 reflection = GetSkyColour( reflect( rayDir, normal ) );
    
#if 1
    // Extra highlight
    vec3 LightColour = vec3(1,.9,.7) * 0.8;
    vec3 L = normalize(vec3(1.0, 2.0, 0.0));
    float NdotL = max( dot( normal, L ), 0.0 );
    float NdotH = max( dot( normal, normalize(L-rayDir) ), 0.0 );
    reflection += (pow(NdotH,10000.0) * 10000.0) * NdotL * LightColour;
    //vReflection += (pow(NdotH,1000.0) * 2000.0) * NdotL * LightColour;
    reflection += (pow(NdotH,100.0) * 200.0) * NdotL * LightColour;
    reflection += (pow(NdotH,10.0) * 20.0) * NdotL * LightColour;
#endif     
     
    float ni = N_Air;
    float nt = N_Water;     
    
    float cosi = NdotV;
    float cost = GetCosT( ni, nt, cosi );
    float fresnelA = Fresnel( ni, nt, cosi, cost );
    float fresnelB = Fresnel( nt, ni, cost, cosi );

    float fresnelFactor = 1.0f - (1.0f - fresnelA) * (1.0f - fresnelB);
    
    vec3 fresnel = vec3(fresnelFactor);

    vec3 thinFilmColour;
#if USE_THIN_FILM_LOOKUP
    thinFilmColour = texture(iChannel0, vec2(NdotV, filmThickness) ).rgb;
#else
    thinFilmColour = GetThinFilmColour(NdotV, filmThickness);
#endif
    fresnel *= thinFilmColour;
    
    colour += reflection * fresnel * remaining;
    remaining *= (1.0f - fresnel);


#if 0
    float fGlassThickness = 0.5;
    vec3 vGlassColor = vec3(1,0.5, 0.25);

	float fOpticalDepth = fGlassThickness / NdotV;
    vec3 vExtinction = exp2( -fOpticalDepth * (1.0 - vGlassColor) ); 
    remaining *= vExtinction;
#endif    
}


vec3 GetSceneColour( vec3 rayOrigin, vec3 rayDir )
{    
    float kFarClip = 200.0;

	vec3 colour = vec3(0);
    vec3 remaining = vec3(1);
    
    float side = 1.0;
    
    float minDist = 0.0;
    
    for( int i=0; i<10; i++ )
    {
        float t = Scene_Trace( rayOrigin, rayDir, minDist, kFarClip, side );
        
        if ( t>=kFarClip )
        {
            break;
        }
        
        minDist = t + 0.1f;
        
        vec3 hitPos = rayOrigin + rayDir * t;

        vec3 normal = Scene_GetNormal( hitPos );

        Shade(colour, remaining, hitPos, rayDir, normal * side );
        
        side = side * -1.0f;
    }
    
 //   colour += GetSkyColour(rayDir) * remaining; 
	
	return colour;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{        
	vec2 uv = fragCoord.xy / iResolution.xy;

    float heading = 0.3f + sin(iTime * 0.3) * 0.1;

    float elevation = 1.8 + sin(iTime * 0.134) * 0.1;
    
    float fov = 2.5 + sin( iTime * 0.234) * 0.5;
    
    float cameraDist = 10.0;
	vec3 cameraPos = vec3(sin(heading) * sin(-elevation), cos(-elevation), cos(heading) * sin(-elevation)) * cameraDist;
	vec3 cameraTarget = vec3(sin(iTime * 0.1542) * 3.0, 0.0, 0.0);

	vec3 rayOrigin = cameraPos;
	vec3 rayDir = GetCameraRayDir( GetWindowCoord(uv), cameraPos, cameraTarget, fov );
	
	vec3 sceneCol = GetSceneColour( rayOrigin, rayDir );
	
	vec3 final = ApplyPostFX( uv, sceneCol );
	
	fragColor = vec4(final, 1.0);
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
                                                                                                                                                                                                                                                                                        // See Image tab for details, also visit:
//
// https://xemantic.github.io/shader-web-background/
//
// In the original shader-web-background these values are provided as uniforms
// feel free to play with them and if you will find something prettier than
// the equilibrium I established, please send it back to me :)
const vec2  iFeedbackZoomCenter       = vec2(0., 0.);
const float iFeedbackZoomRate         = .001;
const float iFeedbackFadeRate         = .999;
//const float iFeedbackColorShiftZoom   = .05;
const float iFeedbackColorShiftImpact = 0.0015;
const vec2  iDrawCenter               = vec2(0., 0.);
const float iDrawIntensity            = 2.5;
const float iBlobEdgeSmoothing        = .02;
const float iBlob1Radius              = .65;
const float iBlob1PowFactor           = 20.;
//const float iBlob1ColorPulseSpeed     = .042;
const float iBlob2Radius              = .7;
const float iBlob2PowFactor           = 20.;
//const float iBlob2ColorPulseSpeed     = .0234;
const float iBlob2ColorPulseShift     = 0.0;
const float iColorShiftOfRadius       =  0.05;
const float iFeedbackMouseShiftFactor = .003;

/*
  Normally it would be provided by texture parameters, but on Mac/iOS the texture REPEAT
  seems to work only for power-of-2 texture sizes.
 */
vec4 repeatedTexture(in sampler2D channel, in vec2 uv) {
    return texture(channel, mod(uv, 1.));
}

float drawBlob(
    in vec2 st,
    in vec2 center,
    in float radius,
    in float edgeSmoothing
) {
    float dist = length((st - center) / radius);
    return dist * smoothstep(1., 1. - iBlobEdgeSmoothing, dist);
}


void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // in shader-web-background provided as uniforms: start
    float iMinDimension = min(iResolution.x, iResolution.y);
    vec2 iScreenRatioHalf =
        (iResolution.x >= iResolution.y)
            ? vec2(iResolution.y / iResolution.x * .5, .5)
            : vec2(.5, iResolution.x / iResolution.y);
    vec3 iBlob1Color = spectral_zucconi6(
        mod(iTime * (iBlob1ColorPulseSpeed / -1.5), 1.)
    );
    
    vec3 iBlob2Color = spectral_zucconi6(
        mod(iTime * (iBlob2ColorPulseSpeed / -1.5) + iBlob2ColorPulseShift, 1.)
    );
    vec2 iFeedbackShiftVector =
        (iMouse.x > 0. && iMouse.y > 0.)
            ? (iMouse.xy * 2. - iResolution.xy) / iMinDimension * iFeedbackMouseShiftFactor
            : vec2(0);
    // in shader-web-background provided as uniforms: end
            
    
    vec2 uv = fragCoord / iResolution.xy;
    vec2 st = (fragCoord * 2. - iResolution.xy) / iMinDimension;

    vec2  drawDelta = st - iDrawCenter;
    float drawAngle = atan(drawDelta.x, drawDelta.y);
    float drawDist = length(drawDelta);

vec3 feedbk = repeatedTexture(iChannel1, uv - st).rgb;
    vec3 colorShift = repeatedTexture(
        iChannel0,
        uv - st * iFeedbackColorShiftZoom * iScreenRatioHalf
    ).rgb;

    vec2 stShift = vec2(0);
    stShift += iFeedbackZoomRate * (st - iFeedbackZoomCenter);
    stShift += (feedbk.bg/colorShift.gr - .5) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    vec3 prevColor = repeatedTexture(iChannel2, uv - stShift).rgb;
    prevColor *= iFeedbackFadeRate;
    
    vec3 prevColor2 = repeatedTexture(iChannel3, uv - stShift).rgb;
    prevColor2 *= iFeedbackFadeRate;
    
 //   prevColor = mix(prevColor, prevColor2, 0.0995);
    prevColor = min(prevColor, prevColor2);

    vec3 drawColor = vec3(0);
   

    float radius =
        1.
        + (colorShift.r + colorShift.g + colorShift.b) * iColorShiftOfRadius;
    drawColor +=
        pow(
          drawBlob(st, iDrawCenter, radius * iBlob1Radius, iBlobEdgeSmoothing),
          iBlob1PowFactor
        ) * iBlob1Color;
    drawColor +=
        pow(
          drawBlob(st, iDrawCenter, radius * iBlob2Radius, iBlobEdgeSmoothing),
          iBlob2PowFactor
        ) * iBlob2Color;

    vec3 color = vec3(0);
    drawColor *= iDrawIntensity;
    prevColor *= iFeedbackFadeRate;
    color += prevColor;
    color += drawColor;

    color = clamp(color, 0., 1.);
    fragColor = vec4(color, 1.);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// This buffer is the feedback loop

// iq noise

float hash( float n )
{
    return fract(sin(n)*43758.5453123);
}

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);

    f = f*f*(3.0-2.0*f);

    float n = p.x + p.y*157.0;

    return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
               mix( hash(n+157.0), hash(n+158.0),f.x),f.y);
}

// hue by netgrind(?)

vec3 hue(vec3 color, float shift) {

    const vec3  kRGBToYPrime = vec3 (0.299, 0.587, 0.114);
    const vec3  kRGBToI     = vec3 (0.596, -0.275, -0.321);
    const vec3  kRGBToQ     = vec3 (0.212, -0.523, 0.311);

    const vec3  kYIQToR   = vec3 (1.0, 0.956, 0.621);
    const vec3  kYIQToG   = vec3 (1.0, -0.272, -0.647);
    const vec3  kYIQToB   = vec3 (1.0, -1.107, 1.704);

    // Convert to YIQ
    float   YPrime  = dot (color, kRGBToYPrime);
    float   I      = dot (color, kRGBToI);
    float   Q      = dot (color, kRGBToQ);

    // Calculate the hue and chroma
    float   hue     = atan (Q, I);
    float   chroma  = sqrt (I * I + Q * Q);

    // Make the user's adjustments
    hue += shift;

    // Convert back to YIQ
    Q = chroma * sin (hue);
    I = chroma * cos (hue);

    // Convert back to RGB
    vec3    yIQ   = vec3 (YPrime, I, Q);
    color.r = dot (yIQ, kYIQToR);
    color.g = dot (yIQ, kYIQToG);
    color.b = dot (yIQ, kYIQToB);

    return color;
}

float fractalNoise(vec2 pos) {
	float n = 0.;
	float scale = 1. / 1.5;
	for (int i = 0; i < 5; i += 1) {
		n += noise(pos) * scale;
		scale *= 0.5;
		pos *= 2.;
	}
	return n;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   	vec2 uv = fragCoord.xy / iResolution.xy;
    
    // Convert the uv's to polar coordinates to scale up  
    vec2 polarUv = (uv * 2.0 - 1.0);
    float angle = atan(polarUv.y, polarUv.x);
    
    // Scale up the length of the vector by a noise function feeded by the angle and length of the vector
    float ll = length(polarUv)*0.5 - fractalNoise(vec2(sin(angle*4. + iTime*2.) + length(uv)*10., length(uv)*20. + sin(angle*4.)))*0.005 ;
    
    vec3 base = texture(iChannel0, uv).rgb;
    
    // Convert the scaled coordinates back to cartesian
    vec2 offs = vec2(cos(angle)*ll + 0.5, sin(angle)*ll + 0.5);
    
    // sample the last texture with uv's slightly scaled up
    vec3 overlay = texture(iChannel1, offs.xy).rgb;
    
    // Since the colors of the iChannel0 are monochrome, set a color channel to zero to do a hue shift
 //   base.b = 0.0;
    
    // Apply a hue shift to the overlaid image so it cascades in the feedback loop
    overlay = hue(overlay, .5);
      overlay += (base + base) * (overlay / .50);
    
    // Additively blend the colors together
    vec4 col = vec4(clamp(vec3(0.),vec3(1.),base + overlay*0.99), 1.0);
    
   // col *= (col + col) * (overlay,1.0);
    //(overlay,1.0);
    
    fragColor = col ;
}