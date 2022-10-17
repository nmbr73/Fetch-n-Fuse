

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Tone mapping and post processing
float hash(float c){return fract(sin(dot(c,12.9898))*43758.5453);}

// linear white point
const float W = 1.2;
const float T2 = 7.5;

float filmic_reinhard_curve (float x) {
    float q = (T2*T2 + 1.0)*x*x;    
	return q / (q + x + T2*T2);
}

vec3 filmic_reinhard(vec3 x) {
    float w = filmic_reinhard_curve(W);
    return vec3(
        filmic_reinhard_curve(x.r),
        filmic_reinhard_curve(x.g),
        filmic_reinhard_curve(x.b)) / w;
}

const int N = 8;
vec3 ca(sampler2D t, vec2 UV, vec4 sampl){
	vec2 uv = 1.0 - 2.0 * UV;
	vec3 c = vec3(0);
	float rf = 1.0;
	float gf = 1.0;
    float bf = 1.0;
	float f = 1.0/float(N);
	for(int i = 0; i < N; ++i){
		c.r += f*texture(t, 0.5-0.5*(uv*rf) ).r;
		c.g += f*texture(t, 0.5-0.5*(uv*gf) ).g;
		c.b += f*texture(t, 0.5-0.5*(uv*bf) ).b;
		rf *= 0.9972;
		gf *= 0.998;
        bf /= 0.9988;
		c = clamp(c,0.0, 1.0);
	}
	return c;
}

void mainImage(out vec4 fragColor,vec2 fragCoord){
    const float brightness = 1.0;
    vec2 pp = fragCoord.xy/iResolution.xy;
    vec2 r = iResolution.xy;
    vec2 p = 1.-2.*fragCoord.xy/r.xy;
    p.y *= r.y/r.x;
   
    // a little chromatic aberration
    vec4 sampl = texture(iChannel0, pp);
    vec3 color = ca(iChannel1, pp, sampl).rgb;
    
    // final output
    float vignette = 1.25 / (1.1 + 1.1*dot(p, p));
    vignette *= vignette;
    vignette = mix(1.0, smoothstep(0.1, 1.1, vignette), 0.25);
    float noise = .012*vec3(hash(length(p)*iTime)).x;
    color = color*vignette+noise;
    color = filmic_reinhard(brightness*color);
    
    color = smoothstep(-0.025, 1.0,color);
    
    color = pow(color, vec3(1.0/2.2));
    fragColor = vec4(color, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
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

float getVal(vec2 uv)
{
    return length(texture(iChannel0,uv).xyz);
}
    
vec2 getGrad(vec2 uv,float delta)
{
    vec2 d=vec2(delta,0);
    return vec2(
        getVal(uv+d.xy)-getVal(uv-d.xy),
        getVal(uv+d.yx)-getVal(uv-d.yx)
    )/delta;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 n = vec3(getGrad(uv,1.0/iResolution.y),-500.0* (1. * abs(cos(iTime / 3.25))) - 95.);
    //n *= n;
    n=normalize(n);
    fragColor=vec4(n,1);
    vec3 light = normalize(vec3(1.0,1.0,2.0 ));
    float diff=clamp(dot(n,light),0.5,1.0);
    float spec=clamp(dot(reflect(light,n),vec3(0,0,-1)),0.0,1.0);
    spec=pow(spec,36.0)*1.5;
    //spec=0.0;
	fragColor = mix(texture(iChannel0,uv)*vec4(diff)+vec4(spec),texture(iChannel1,uv)*vec4(diff)+vec4(spec),0.5);
}
// Fork of "not a fluid simulation" by pali6. https://shadertoy.com/view/sdd3zj
// 2021-09-01 08:39:43

/*
	Transverse Chromatic Aberration

	Based on https://github.com/FlexMonkey/Filterpedia/blob/7a0d4a7070894eb77b9d1831f689f9d8765c12ca/Filterpedia/customFilters/TransverseChromaticAberration.swift

	Simon Gladman | http://flexmonkey.blogspot.co.uk | September 2017


int sampleCount = 50;
float blur = 0.6; 
float falloff = 2.50; 

// use iChannel0 for video, iChannel1 for test grid
#define INPUTA iChannel0
#define INPUTB iChannel1
#define INPUTC iChannel2
#define INPUTD iChannel3

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 destCoord = fragCoord.xy / iResolution.xy;

    vec2 direction = normalize(destCoord - 0.5); 
    vec2 velocity = direction * blur * pow(length(destCoord - 0.5), falloff);
	float inverseSampleCount = 1.0 / float(sampleCount); 
    
    mat3x2 increments = mat3x2(velocity * .250 * inverseSampleCount,
                               velocity * .50 * inverseSampleCount,
                               velocity * 1.0 * inverseSampleCount);

    vec3 accumulator = vec3(0);
    mat3x2 offsets = mat3x2(0); 
    
    for (int i = 0; i < sampleCount; i++) {
        accumulator.r += texture(INPUTA, destCoord + offsets[0]).r; 
        accumulator.g += texture(INPUTA, destCoord + offsets[1]).g; 
        accumulator.b += texture(INPUTA, destCoord + offsets[2]).b; 
        
        accumulator.r += texture(INPUTB, destCoord + offsets[0]).r; 
        accumulator.g += texture(INPUTB, destCoord + offsets[1]).g; 
        accumulator.b += texture(INPUTB, destCoord + offsets[2]).b; 
        
        accumulator.r *= texture(INPUTC, destCoord + offsets[0]).r; 
        accumulator.g *= texture(INPUTC, destCoord + offsets[1]).g; 
        accumulator.b *= texture(INPUTC, destCoord + offsets[2]).b; 
        
         accumulator.r += texture(INPUTD, destCoord + offsets[0]).r; 
        accumulator.g += texture(INPUTD, destCoord + offsets[1]).g; 
        accumulator.b += texture(INPUTD, destCoord + offsets[2]).b; 
        
        
        offsets -= increments;
        
   //     accumulator.rgb = clamp(accumulator.rgb, 0., 1.);
 
// vec4 blendy = (blend + accumulator),1.0;
    }
    vec3 blend  = texture(INPUTA, destCoord + offsets[0]).rgb;
    blend = clamp(blend.rgb, 0., 1.);
    vec3 blendo = (accumulator / float(sampleCount));
 //blend += blend + accumulator;
fragColor = mix(vec4(blend.rgb,1.0),vec4(blendo.rgb,1.0),0.975);
//	fragColor = vec4(accumulator / float(sampleCount), 1.0);
}

*/
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define iFeedbackColorShiftZoom 0.0011
//#define iFeedbackColorShiftImpact 0.001
#define iBlob1ColorPulseSpeed -0.03456
#define iBlob2ColorPulseSpeed 0.04321
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

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/** 
 * Brightness Contrast Saturation Hue
 * Demo: https://www.shadertoy.com/view/MdjBRy
 * starea @ ShaderToy
 * 
 * Forked and remixed from: 
 * [1] https://shadertoy.com/view/llGSzK
 * [2] https://shadertoy.com/view/MsjXRt
 *
 * Created 7/26/2017
 * Updated 8/11/2017
 **/

/*
mat4 brightnessMatrix( float b ) {
    return mat4( 
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        b, b, b, 1 );
}
*/
void brightnessAdjust( inout vec4 color, in float b) {
    color.rgb += b;
}

/*
mat4 contrastMatrix( float c ) {
	float t = 0.5 - c * 0.5;
    return mat4( 
        c, 0, 0, 0,
        0, c, 0, 0,
        0, 0, c, 0,
        t, t, t, 1 );

}
*/

void contrastAdjust( inout vec4 color, in float c) {
    float t = 0.5 - c * 0.5; 
    color.rgb = color.rgb * c + t;
}

mat4 saturationMatrix( float saturation ) {
    vec3 luminance = vec3( 0.3086, 0.6094, 0.0820 );
    float oneMinusSat = 1.0 - saturation;
    vec3 red = vec3( luminance.x * oneMinusSat );
    red.r += saturation;
    
    vec3 green = vec3( luminance.y * oneMinusSat );
    green.g += saturation;
    
    vec3 blue = vec3( luminance.z * oneMinusSat );
    blue.b += saturation;
    
    return mat4( 
        red,     0,
        green,   0,
        blue,    0,
        0, 0, 0, 1 );
}

int modi(int x, int y) {
    return x - y * (x / y);
}

int and(int a, int b) {
    int result = 0;
    int n = 1;
	const int BIT_COUNT = 32;

    for(int i = 0; i < BIT_COUNT; i++) {
        if ((modi(a, 2) == 1) && (modi(b, 2) == 1)) {
            result += n;
        }

        a >>= 1;
        b >>= 1;
        n <<= 1;

        if (!(a > 0 && b > 0))
            break;
    }
    return result;
}

// forked from https://www.shadertoy.com/view/llGSzK
// performance optimized by Ruofei
vec4 vibrance(vec4 inCol, float vibrance) //r,g,b 0.0 to 1.0,  vibrance 1.0 no change, 0.0 image B&W.
{
 	vec4 outCol;
    if (vibrance <= 1.0)
    {
        float avg = dot(inCol.rgb, vec3(0.3, 0.6, 0.1));
        outCol.rgb = mix(vec3(avg), inCol.rgb, vibrance); 
    }
    else // vibrance > 1.0
    {
        float hue_a, a, f, p1, p2, p3, i, h, s, v, amt, _max, _min, dlt;
        float br1, br2, br3, br4, br5, br2_or_br1, br3_or_br1, br4_or_br1, br5_or_br1;
        int use;

        _min = min(min(inCol.r, inCol.g), inCol.b);
        _max = max(max(inCol.r, inCol.g), inCol.b);
        dlt = _max - _min + 0.00001 /*Hack to fix divide zero infinities*/;
        h = 0.0;
        v = _max;

		br1 = step(_max, 0.0);
        s = (dlt / _max) * (1.0 - br1);
        h = -1.0 * br1;

		br2 = 1.0 - step(_max - inCol.r, 0.0); 
        br2_or_br1 = max(br2, br1);
        h = ((inCol.g - inCol.b) / dlt) * (1.0 - br2_or_br1) + (h*br2_or_br1);

		br3 = 1.0 - step(_max - inCol.g, 0.0); 
        
        br3_or_br1 = max(br3, br1);
        h = (2.0 + (inCol.b - inCol.r) / dlt) * (1.0 - br3_or_br1) + (h*br3_or_br1);

        br4 = 1.0 - br2*br3;
        br4_or_br1 = max(br4, br1);
        h = (4.0 + (inCol.r - inCol.g) / dlt) * (1.0 - br4_or_br1) + (h*br4_or_br1);

        h = h*(1.0 - br1);

        hue_a = abs(h); // between h of -1 and 1 are skin tones
        a = dlt;      // Reducing enhancements on small rgb differences

        // Reduce the enhancements on skin tones.    
        a = step(1.0, hue_a) * a * (hue_a * 0.67 + 0.33) + step(hue_a, 1.0) * a;                                    
        a *= (vibrance - 1.0);
        s = (1.0 - a) * s + a * pow(s, 0.25);

        i = floor(h);
        f = h - i;

        p1 = v * (1.0 - s);
        p2 = v * (1.0 - (s * f));
        p3 = v * (1.0 - (s * (1.0 - f)));

        inCol.rgb = vec3(0.0); 
        i += 6.0;
        //use = 1 << ((int)i % 6);
        use = int(pow(2.0,mod(i,6.0)));
        a = float(and(use , 1)); // i == 0;
        use >>= 1;
        inCol.rgb += a * vec3(v, p3, p1);
 
        a = float(and(use , 1)); // i == 1;
        use >>= 1;
        inCol.rgb += a * vec3(p2, v, p1); 

        a = float( and(use,1)); // i == 2;
        use >>= 1;
        inCol.rgb += a * vec3(p1, v, p3);

        a = float(and(use, 1)); // i == 3;
        use >>= 1;
        inCol.rgb += a * vec3(p1, p2, v);

        a = float(and(use, 1)); // i == 4;
        use >>= 1;
        inCol.rgb += a * vec3(p3, p1, v);

        a = float(and(use, 1)); // i == 5;
        use >>= 1;
        inCol.rgb += a * vec3(v, p1, p2);

        outCol = inCol;
    }
    return outCol;
}

// remixed from mAlk's https://www.shadertoy.com/view/MsjXRt
vec4 shiftHue(in vec3 col, in float Shift)
{
    vec3 P = vec3(0.55735) * dot(vec3(0.55735), col);
    vec3 U = col - P;
    vec3 V = cross(vec3(0.55735), U);    
    col = U * cos(Shift * 6.2832) + V * sin(Shift * 6.2832) + P;
    return vec4(col, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // uniforms
	float brightness = 0.15;
	float contrast = 1.2;
    float saturation = 1.5;
    float _vibrance = 4.0;
    float _hue = abs(sin(iTime * 0.1)); 
    
    vec4 color = texture( iChannel0, fragCoord/iResolution.xy );
    
    fragColor = saturationMatrix(saturation) * color; 
    brightnessAdjust(color, brightness); 
    contrastAdjust(color, contrast); 
    fragColor = vibrance(fragColor, _vibrance);
    fragColor = shiftHue(fragColor.rgb, _hue);
    fragColor.a = 1.0;
    if (iMouse.z > 0.5) fragColor = color; 
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
const float iFeedbackFadeRate         = .998;
//const float iFeedbackColorShiftZoom   = .05;
const float iFeedbackColorShiftImpact = 0.0001;
const vec2  iDrawCenter               = vec2(0., 0.);
const float iDrawIntensity            = 2.5;
const float iBlobEdgeSmoothing        = .15;
const float iBlob1Radius              = .33;
const float iBlob1PowFactor           = 20.;
//const float iBlob1ColorPulseSpeed     = .04;
const float iBlob2Radius              = .55;
const float iBlob2PowFactor           = 20.;
//const float iBlob2ColorPulseSpeed     = .01234;
const float iBlob2ColorPulseShift     = -.0;
const float iColorShiftOfRadius       = .5;
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
    stShift += (feedbk.bg/colorShift.rb - .5) * iFeedbackColorShiftImpact;
    stShift += iFeedbackShiftVector;
    stShift *= iScreenRatioHalf;

    vec3 prevColor = repeatedTexture(iChannel2, uv - stShift).rgb;
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
    vec4 blend = texture(iChannel3,uv);
vec4 blendy = texture(iChannel2,uv);
vec4    blendo = max(blend,blendy);
    fragColor = mix(blendo,vec4(color, 1.),0.95);
//      fragColor = min(blendo,vec4(color, 1.));
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

#define RotNum 5
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
  vec4 col2 = texture(iChannel3,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
  vec4 blend = max(col,col2);
  
  fragColor=blend,(fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
  //  fragColor=texture(iChannel0,fract((pos+v*vec2(-1,1)*2.0)/Res.xy));
    
    // add a little "motor" in the center
 //   vec2 scr=(fragCoord.xy/Res.xy)*2.0-vec2(1.0);
//    fragColor.xy += (0.001*scr.xy / (dot(scr,scr)/0.1+0.3));
    
 //   if(iFrame<=4 || KEY_I>0.5) fragColor=texture(iChannel2,fragCoord.xy/Res.xy);
}
