

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Fork of a fork of a fork of a fork of a fork of a fork of a fork of a fork of a 

//Chromatic aberration, film grain and tone mapping

float NoiseSeed;

float randomFloat(){
  NoiseSeed = sin(NoiseSeed) * 84522.13219145687;
  return fract(NoiseSeed);
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    
    if(fragCoord.y / iResolution.y < Margins || fragCoord.y / iResolution.y > 1.0-Margins){
        fragColor = vec4(ACESFilm(vec3(0)), 1.0);
        return;
    }
    
    NoiseSeed = float(iFrame)* .003186154 + fragCoord.y * 17.2986546543 + fragCoord.x;
    
    vec2 uv = fragCoord.xy/iResolution.xy;
    
    vec2 d = (uv-vec2(.5)) * .0075;
    vec3 color = vec3(texture(iChannel0, uv - 0.0 * d).r,
                      texture(iChannel0, uv - 1.0 * d).g,
                      texture(iChannel0, uv - 2.0 * d).b);
                                  
    vec3 col = vec3(texture(iChannel1, uv - 0.0 * d).r,
                      texture(iChannel1, uv - 1.0 * d).g,
                      texture(iChannel1, uv - 2.0 * d).b);
                      
        vec3 col2 = vec3(texture(iChannel2, uv - 0.0 * d).r,
                      texture(iChannel2, uv - 1.0 * d).g,
                      texture(iChannel2, uv - 2.0 * d).b);
                      
                //      col = min(col,color);
                      color = min(col2,col);
               //       col2 = mix(col,color,0.5);
             //        col2 = min(col,color);
                      
                       
 //     color = mix(col,color,col2);
    float noise = .9 + randomFloat()*.15;
  	fragColor = vec4(ACESFilm(((color * 0.5)+ (mix(col,color,col2)))*noise), 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
                                                                                                                                                                                                                                                                                        // See Image tab for details, also visit:
//
// https://xemantic.github.io/shader-web-background/
//
// In the original shader-web-background these values are provided as uniforms
// feel free to play with them and if you will find something prettier than
// the equilibrium I established, please send it back to me :)
const vec2  iFeedbackZoomCenter       = vec2(0., 0.);
const float iFeedbackZoomRate         = -.001;
const float iFeedbackFadeRate         = .998;
const float iFeedbackColorShiftZoom   = .005;
const float iFeedbackColorShiftImpact = -0.0001;
const vec2  iDrawCenter               = vec2(0., 0.);
const float iDrawIntensity            = .025;
const float iBlobEdgeSmoothing        = .01;
const float iBlob1Radius              = .75;
const float iBlob1PowFactor           = 20.;
const float iBlob1ColorPulseSpeed     = .024;
const float iBlob2Radius              = .65;
const float iBlob2PowFactor           = 20.;
const float iBlob2ColorPulseSpeed     = .01234;
const float iBlob2ColorPulseShift     = 0.0;
const float iColorShiftOfRadius       = 0.5;
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
        mod(iTime * iBlob1ColorPulseSpeed, 1.)
    );
    
    vec3 iBlob2Color = spectral_zucconi6(
        mod(iTime * iBlob2ColorPulseSpeed + iBlob2ColorPulseShift, 1.)
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
    stShift += (feedbk.br/colorShift.gr - .5) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    vec3 prevColor = mix(repeatedTexture(iChannel2, uv - stShift).rgb,repeatedTexture(iChannel3, uv - stShift).rgb,repeatedTexture(iChannel1, uv - stShift).rgb);
     vec3 prevColor2 = repeatedTexture(iChannel2, uv - stShift).rgb+repeatedTexture(iChannel3, uv - stShift).rgb * repeatedTexture(iChannel1, uv - stShift).rgb/512.;
    prevColor +=prevColor2/512.;
    prevColor *= iFeedbackFadeRate;

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

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//#define iFeedbackColorShiftZoom 0.2
//#define iFeedbackColorShiftImpact 0.001
//#define iBlob1ColorPulseSpeed 0.03456
//#define iBlob2ColorPulseSpeed 0.02345
#define Margins .0
#define iTime iTime / 1.0

#define PI 3.14159265359

vec2 getFontSymbolSampleUV(int x, int y, vec2 uv) {
	return uv * vec2(1./16.) + vec2(float(x) / 16., float(y) / 16.);
}

float getHeight(vec2 uv, sampler2D bumpMap, float maxHeight) {
    return texture(bumpMap, uv).r*maxHeight;
}

vec3 getSlope(vec2 uv, float height, vec2 axis, sampler2D bumpMap, float maxHeight, vec2 texelSize) {
    float h1 = getHeight(uv+texelSize*axis, bumpMap, maxHeight);
    float h2 = getHeight(uv-texelSize*axis, bumpMap, maxHeight);
    return vec3(1,((height-h2) + (h1-height)) / 2.,0);
}

vec3 bump2Normal(vec2 uv, sampler2D bumpMap, float maxHeight, vec2 texelSize) {
    
    float height = getHeight(uv, bumpMap, maxHeight);
        
    vec3 slopeX = getSlope(uv, height, vec2(1., 0.), bumpMap, maxHeight, texelSize).zyx;
    vec3 slopeY = getSlope(uv, height, vec2(0., 1.), bumpMap, maxHeight, texelSize).xyz;
    return cross(slopeX, slopeY).zyx;
}

vec3 normal2rgb(vec3 normal) {
    return normalize(normal.rbg * vec3(0.5, 0.5, 0.5) + vec3(0.5, 0.5, 0.5));
}

float radius = 20.;
float intensity = 5.;
float fade = .99;

float pi = 3.14159265359;

float blurRadius = 3.;

float sinWave(float v, float m){return (sin(v*m*pi*2.)+1.)/2.;}

//from https://github.com/hughsk/glsl-hsv2rgb
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 xy2hue(vec2 v){
    float h = (atan(v.y, v.x)/(pi) + 1.)/2.;
    float b = length(v);
    return vec4(hsv2rgb(vec3(h, 1., b)), 1.);
}







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
//    pi = 3.14159,
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

//vec3 hsv2rgb(vec3 cc)
//{
//    vec4 K = vec4(1., 2. / 3., 1. / 3., 3.);
//    vec3 p = abs(fract(cc.xxx + K.xyz) * 6. - K.www);
//    return cc.z * mix(K.xxx, clamp(p - K.xxx, 0., 1.), cc.y);
//}

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

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

#define RotNum 7
#define SUPPORT_EVEN_ROTNUM

#define Res  iChannelResolution[0]
#define Res1 iChannelResolution[1]

//#define keyTex iChannel3
//#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float ang = 2.0*3.1415926535/float(RotNum);
mat2 mu = mat2(cos(ang),sin(ang),-sin(ang),cos(ang));
mat2 mh = mat2(cos(ang*0.5),sin(ang*0.5),-sin(ang*0.5),cos(ang*0.5));

vec4 randS(vec2 uv)
{
    return texture(iChannel1,uv*Res.xy/Res1.xy)-vec4(0.5);
}

float getRot(vec2 pos, vec2 b)
{
    vec2 p = b;
    float rot=0.0;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(texture(iChannel0,fract((pos+p)/Res.xy)).xy-vec2(0.5),p.yx*vec2(1,-1));
        p = mu*p;
    }
    return rot/float(RotNum)/dot(b,b);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord.xy;
    float rnd = randS(vec2(float(iFrame)/Res.x,0.5/Res1.y)).x;
    
    vec2 b = vec2(cos(ang*rnd),sin(ang*rnd));
    vec2 v=vec2(0);
    float bbMax=0.7*Res.y; bbMax*=bbMax;
    for(int l=0;l<20;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        vec2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=p.yx*getRot(pos+p,-mh*b);
#else
            // this is faster but works only for odd RotNum
            v+=p.yx*getRot(pos+p,b);
#endif
            p = mu*p;
        }
        b*=2.0;
    }
    
     vec2 uv = fragCoord/iResolution.xy; // Normalized pixel coordinates (from 0 to 1)

//  vec4 col = texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  vec4 col2 = texture(iChannel3,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  vec4 blend = mix(col2,col,0.5);
  
//  fragColor=blend;
  
   vec4 col = texture(iChannel0,fract((pos-v*vec2(-1,1)*2.0)/Res.xy));
  vec4 col2 = texture(iChannel2,fract((pos-v*vec2(-1,1)*2.0)/Res.xy));
  vec4 col3 = texture(iChannel3,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  col = max(col,col2);
//  col2= min(col,col2);
  vec4 blend = mix(col,col2,col3);
 // blend = (blend*0.5)* ((max(col,blend))*0.5)+ ((min(col,blend))*0.5);
  blend = clamp(blend, 0., 1.);
  fragColor=blend,(fract((pos-v*vec2(-1,1)*2.0)/Res.xy));
  //  fragColor=texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
    
    // add a little "motor" in the center
 //   vec2 scr=(fragCoord.xy/Res.xy)*2.0-vec2(1.0);
//    fragColor.xy += (0.001*scr.xy / (dot(scr,scr)/0.1+0.3));
    
 //   if(iFrame<=4 || KEY_I>0.5) fragColor=texture(iChannel2,fragCoord.xy/Res.xy);
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
const float iFeedbackFadeRate         = .996;
const float iFeedbackColorShiftZoom   = .005;
const float iFeedbackColorShiftImpact = 0.0001;
const vec2  iDrawCenter               = vec2(0., 0.);
const float iDrawIntensity            = .1;
const float iBlobEdgeSmoothing        = .1;
const float iBlob1Radius              = .85;
const float iBlob1PowFactor           = 20.;
const float iBlob1ColorPulseSpeed     = -.04;
const float iBlob2Radius              = .78;
const float iBlob2PowFactor           = 20.;
const float iBlob2ColorPulseSpeed     = .1234;
const float iBlob2ColorPulseShift     = 0.0;
const float iColorShiftOfRadius       = 0.5;
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
        mod(iTime * iBlob1ColorPulseSpeed, 1.)
    );
    
    vec3 iBlob2Color = spectral_zucconi6(
        mod(iTime * iBlob2ColorPulseSpeed + iBlob2ColorPulseShift, 1.)
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
    stShift += (feedbk.br/colorShift.gr - .5) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    vec3 prevColor = mix(repeatedTexture(iChannel2, uv - stShift).rgb,repeatedTexture(iChannel3, uv - stShift).rgb,repeatedTexture(iChannel1, uv - stShift).rgb);
     vec3 prevColor2 = repeatedTexture(iChannel2, uv - stShift).rgb+repeatedTexture(iChannel3, uv - stShift).rgb * repeatedTexture(iChannel1, uv - stShift).rgb/512.;
    prevColor +=prevColor2/512.;
    prevColor *= iFeedbackFadeRate;

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
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

#define RotNum 7
//#define SUPPORT_EVEN_ROTNUM

#define Res  iChannelResolution[0]
#define Res1 iChannelResolution[1]

//#define keyTex iChannel3
//#define KEY_I texture(keyTex,vec2((105.5-32.0)/256.0,(0.5+0.0)/3.0)).x

const float ang = 2.0*3.1415926535/float(RotNum);
mat2 mu = mat2(cos(ang),sin(ang),-sin(ang),cos(ang));
mat2 mh = mat2(cos(ang*0.5),sin(ang*0.5),-sin(ang*0.5),cos(ang*0.5));

vec4 randS(vec2 uv)
{
    return texture(iChannel1,uv*Res.xy/Res1.xy)-vec4(0.5);
}

float getRot(vec2 pos, vec2 b)
{
    vec2 p = b;
    float rot=0.0;
    for(int i=0;i<RotNum;i++)
    {
        rot+=dot(texture(iChannel0,fract((pos+p)/Res.xy)).xy-vec2(0.5),p.yx*vec2(1,-1));
        p = mu*p;
    }
    return rot/float(RotNum)/dot(b,b);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord.xy;
    float rnd = randS(vec2(float(iFrame)/Res.x,0.5/Res1.y)).x;
    
    vec2 b = vec2(cos(ang*rnd),sin(ang*rnd));
    vec2 v=vec2(0);
    float bbMax=0.7*Res.y; bbMax*=bbMax;
    for(int l=0;l<20;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        vec2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=p.yx*getRot(pos+p,-mh*b);
#else
            // this is faster but works only for odd RotNum
            v+=p.yx*getRot(pos+p,b);
#endif
            p = mu*p;
        }
        b*=2.0;
    }
    
     vec2 uv = fragCoord/iResolution.xy; // Normalized pixel coordinates (from 0 to 1)

//  vec4 col = texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  vec4 col2 = texture(iChannel3,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  vec4 blend = mix(col2,col,0.5);
  
//  fragColor=blend;
  
   vec4 col = texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
  vec4 col2 = texture(iChannel2,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
  vec4 col3 = texture(iChannel3,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
//  col = max(col,col2);
//  col2= min(col,col2);
  vec4 blend = mix(col,col2,col3);
 // blend = (blend*0.5)* ((max(col,blend))*0.5)+ ((min(col,blend))*0.5);
  blend = clamp(blend, 0., 1.);
  fragColor=blend,(fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
  //  fragColor=texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
    
    // add a little "motor" in the center
 //   vec2 scr=(fragCoord.xy/Res.xy)*2.0-vec2(1.0);
//    fragColor.xy += (0.001*scr.xy / (dot(scr,scr)/0.1+0.3));
    
 //   if(iFrame<=4 || KEY_I>0.5) fragColor=texture(iChannel2,fragCoord.xy/Res.xy);
}
