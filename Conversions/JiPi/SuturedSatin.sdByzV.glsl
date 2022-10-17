

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define BUMP 0.3

// dispersion amount
#define DISP_SCALE 0.3

// minimum IOR
#define MIN_IOR 1.1

// chromatic dispersion samples, higher values decrease banding
#define SAMPLES 9

// time scale
#define TIME 0.1*iTime

// sharpness of the sample weight distributions, higher values increase separation of colors
#define SHARP 15.0

#define FILMIC
#ifdef FILMIC
// tweaked version of a filmic curve from paniq with a softer left knee
vec3 contrast(vec3 x) {
    x=log(1.0+exp(x*10.0-7.2));
    return (x*(x*6.2+0.5))/(x*(x*6.2+1.7)+0.06);
}
#else
#define SIGMOID_CONTRAST 8.0
vec3 contrast(vec3 x) {
	return (1.0 / (1.0 + exp(-SIGMOID_CONTRAST * (x - 0.5))));    
}
#endif

vec3 sampleWeights(float i) {
	return vec3(exp(-SHARP*pow(i-0.25,2.0)), exp(-SHARP*pow(i-0.5,2.0)), exp(-SHARP*pow(i-0.75,2.0)));
}

mat3 cameraMatrix() {
    vec3 ro = vec3(sin(TIME),0.0,cos(TIME));
    vec3 ta = vec3(0,1.5,0);  
    vec3 w = normalize(ta - ro);
    vec3 u = normalize(cross(w,vec3(0,1,0)));
    vec3 v = normalize(cross(u,w));
    return mat3(u,v,w);
}

// same as the normal refract() but returns the coefficient
vec3 refractK(vec3 I, vec3 N, float eta, out float k) {
    k = max(0.0,1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I)));
    if (k <= 0.0)
        return vec3(0.0);
    else
        return eta * I - (eta * dot(N, I) + sqrt(k)) * N;
}

vec3 sampleDisp(vec2 uv, vec3 disp) {
	vec2 p = uv - 0.5;

    // camera movement
    mat3 camMat = cameraMatrix();

    vec3 rd = normz(camMat * vec3(p.xy, 1.0));
    vec3 norm = normz(camMat * disp);
    
    vec3 col = vec3(0);
    const float SD = 1.0 / float(SAMPLES);
    float wl = 0.0;
    vec3 denom = vec3(0);
    for(int i = 0; i < SAMPLES; i++) {
        vec3 sw = sampleWeights(wl);
        denom += sw;
        float k;
        vec3 refr = refractK(rd, norm, MIN_IOR + wl * DISP_SCALE, k);
        vec3 refl = reflect(rd, norm);
        col += sw * mix(texture(iChannel1, refl).xyz, texture(iChannel1, refr).xyz, k);
        wl  += SD;
    }
    
    return col / denom;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec2 texel = 1. / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.xy;

    vec2 n  = vec2(0.0, texel.y);
    vec2 e  = vec2(texel.x, 0.0);
    vec2 s  = vec2(0.0, -texel.y);
    vec2 w  = vec2(-texel.x, 0.0);

    float d   = texture(iChannel0, uv).x;
    // uncomment to just render the heightmap
    //#define SIMPLE
    #ifdef SIMPLE
    fragColor = 0.5+0.02*vec4(d);
    #else
    float d_n  = texture(iChannel0, (uv+n)  ).x;
    float d_e  = texture(iChannel0, (uv+e)  ).x;
    float d_s  = texture(iChannel0, (uv+s)  ).x;
    float d_w  = texture(iChannel0, (uv+w)  ).x; 
    float d_ne = texture(iChannel0, (uv+n+e)).x;
    float d_se = texture(iChannel0, (uv+s+e)).x;
    float d_sw = texture(iChannel0, (uv+s+w)).x;
    float d_nw = texture(iChannel0, (uv+n+w)).x; 

    float dxn[3];
    float dyn[3];
    float dcn[3];
    
    dcn[0] = 0.5;
    dcn[1] = 1.0; 
    dcn[2] = 0.5;

    dyn[0] = d_nw - d_sw;
    dyn[1] = d_n  - d_s; 
    dyn[2] = d_ne - d_se;

    dxn[0] = d_ne - d_nw; 
    dxn[1] = d_e  - d_w; 
    dxn[2] = d_se - d_sw; 

    // The section below is an antialiased version of 
    // Shane's Bumped Sinusoidal Warp shadertoy here:
    // https://www.shadertoy.com/view/4l2XWK
	#define SRC_DIST 8.0
    vec3 sp = vec3(uv-0.5, 0);
    vec3 light = vec3(cos(iTime/2.0)*0.5, sin(iTime/2.0)*0.5, -SRC_DIST);
    vec3 ld = light - sp;
    float lDist = max(length(ld), 0.001);
    ld /= lDist;
    float aDist = max(distance(vec3(light.xy,0),sp) , 0.001);
    float atten = min(0.07/(0.25 + aDist*0.5 + aDist*aDist*0.05), 1.);
    vec3 rd = normalize(vec3(uv - 0.5, 1.));

    float spec = 0.0;
	float den = 0.0;
    
    vec3 dispCol = vec3(0);
    
    // compute dispersion and specular with antialiasing
    vec3 avd = vec3(0);
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            vec2 dxy = vec2(dxn[i], dyn[j]);
            float w = dcn[i] * dcn[j];
            vec3 bn = reflect(normalize(vec3(BUMP*dxy, -1.0)), vec3(0,1,0));
            avd += w * bn;
            den += w;
            dispCol += w * sampleDisp(uv, bn);
            spec += w * ggx(bn, vec3(0,1,0), ld, 0.3, 1.0);
        }
    }

    avd /= den;
    spec /= den;
    dispCol /= den;
    
    // end bumpmapping section

    fragColor =  vec4(contrast(.75*dispCol),1) + 1.0*vec4(0.9, 0.85, 0.8, 1)*spec;

    #endif

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define TIMESTEP 0.7

#define PI 3.14159265359

#define A iChannel0
#define B iChannel2

float sigmoid(float x) {
    return .5 + .5 * tanh(-5. * (x - 1.));
}

void mainImage( out vec4 r, in vec2 fragCoord )
{
    vec2 p = fragCoord.xy / iResolution.xy;
    vec2 stepSize = 1. / iResolution.xy;
    
    // initialize with noise
    if(iFrame < 10) {
        vec3 rnd = vec3(noise(8.0 * p + 1.1), noise(8.0 * p + 2.2), noise(8.0 * p + 3.3));
        r = vec4(rnd,0);
        return;
    }
    
    vec3 ma = vec3(0);
    float gcurve = 0.;
    for (int i = 0; i < 12; i++) {
        float angle = float(i) * PI/6.;
        vec2 offset = vec2(cos(angle), sin(angle));
        vec3 spring = vec3(offset, 0) + texture(B, p + offset / iResolution.xy).xyz - texture(B,p).xyz;
        ma += sigmoid(length(spring)) * spring;
        
        float angle1 = float(i+1) * PI/6.;
        vec2 offset1 = vec2(cos(angle1), sin(angle1));
        vec3 spring1 = vec3(offset1, 0) + texture(B, p + offset1 / iResolution.xy).xyz - texture(B,p).xyz;
        gcurve += PI/6. - acos(dot(normz(spring), normz(spring1)));
    }
    
    vec3 dv = texBlur(A, p - stepSize * texture(A,p).xy).xyz;
    dv += texLapl(A, p - stepSize * texture(A,p).xy).xyz;
    dv = texBlur(A, p - stepSize * 48. * (2.87 + 1e4 * gcurve) * (texture(B,p).xy - dv.xy)).xyz;
    dv += TIMESTEP * ma;

    float t = 0.05 * iTime;
    vec2 m = fract(vec2(noise(vec2(t,0)), noise(vec2(0,t)))) * iResolution.xy;
    if (iMouse.z > 0.0) m = iMouse.xy;
    vec2 d = fragCoord - m;
    dv += 5. * exp(-length(d) / 50.0) * normz(vec3(d, 0.0));
    
    // hard clamping
    //dv = length(dv) > 1.0 ? normz(dv) : dv;
    // soft clamping
    dv -= 0.005 * pow(length(dv), 3.) * normz(dv);
    r.xyz = mix(texture(A,p).xyz, dv, 0.5);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define TIMESTEP 0.1

#define A iChannel0
#define B iChannel1

void mainImage( out vec4 r, in vec2 fragCoord )
{
    vec2 p = fragCoord.xy / iResolution.xy;
    
    // initialize with noise
    if(iFrame < 10) {
        vec3 rnd = vec3(noise(16.0 * p + 1.1), noise(16.0 * p + 2.2), noise(16.0 * p + 3.3));
        r = vec4(rnd,0);
        return;
    }
    
    vec3 du = texBlur(B,p).xyz + TIMESTEP * texBlur(A,p).xyz;
    // hard clamping
    //du = length(du) > 1.0 ?  normz(du) : du;
    // soft clamping
    du -= 0.005 * pow(length(du), 3.) * normz(du);
    r.xyz = mix(texture(B,p).xyz, du, 1.);
    //r.w = texBlur(A,p).w;
    r.w = texGradX(A,p).x + texGradY(A,p).y;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// based on Suture Fluid

vec3 rotateAxis(vec3 p, vec3 axis, float angle) {
    return mix(dot(axis, p) * axis, p, cos(angle)) + cross(axis, p) * sin(angle);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;

    if (iFrame < 10) {
        fragColor = vec4(noise(16.0 * uv + 1.1), noise(16.0 * uv + 2.2), noise(16.0 * uv + 3.3), 0);
        return;
    }

    float divergence = mix(texture(iChannel0, uv).w, texLapl(iChannel0, uv).w, .25); // divergence smoothing
    divergence = mix(divergence, texGradX(iChannel0, uv).x + texGradY(iChannel0, uv).y, 1.); // divergence update

    vec2 stepSize = 6. / iResolution.xy;
    vec3 velocity = texture(iChannel0, uv).xyz;
    vec3 advected = texBlur(iChannel0, uv - stepSize * velocity.xy).xyz;
    advected += 2. * texLapl(iChannel0, uv - stepSize * velocity.xy).xyz;
    advected += .5 * texLapl(iChannel0, uv).xyz;
    advected -= .5 * texture(iChannel0, uv).xyz * divergence;
    advected -= .8 * texLapl(iChannel0, uv).z * normz(velocity);
    advected -= clamp(iTime - 5., 0., 9.) * texBlur(iChannel0, uv).w * normz(velocity);

    vec3 curl = vec3(
        texGradY(iChannel0, uv).z - 0.,
        0. - texGradX(iChannel0, uv).z,
        texGradX(iChannel0, uv).y - texGradY(iChannel0, uv).x);
    if (length(curl) > 0.)
        advected = rotateAxis(advected, normalize(curl), 10. * length(curl));
        
    advected += 1.5 * curl;

    if (length(advected) > 1.) advected = normalize(advected);
    divergence = clamp(divergence, -1., 1.);
    fragColor = mix(texture(iChannel0, uv), vec4(advected, divergence), .2); // update smoothing
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// This convolves the Laplacian values with a specially-designed Poisson solver kernel.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const float _K0 = 20.0/6.0; // center weight

    vec2 uv = fragCoord / iResolution.xy;
    vec2 texel = 1.0 / iResolution.xy;
    
    /* 
		Poisson solver kernel, computed using a custom tool. The curve ended up being very close
    	to exp(-x) times a constant (0.43757*exp(-1.0072*x), R^2 = 0.9997).
    	The size of the kernel is truncated such that 99% of the summed kernel weight is accounted for. 
	*/
    float a[121] = float[](
        1.2882849374994847E-4, 3.9883638750009155E-4, 9.515166750018973E-4, 0.0017727328875003466, 0.0025830133546736567, 0.002936729756271805, 0.00258301335467621, 0.0017727328875031007, 9.515166750027364E-4, 3.988363875000509E-4, 1.2882849374998886E-4,
        3.988363875000656E-4, 0.00122005053750234, 0.0029276701875229076, 0.005558204850002636, 0.008287002243739282, 0.009488002668845403, 0.008287002243717386, 0.005558204850002533, 0.002927670187515983, 0.0012200505375028058, 3.988363875001047E-4,
        9.515166750033415E-4, 0.0029276701875211478, 0.007226947743770152, 0.014378101312275642, 0.02243013709214819, 0.026345595431380788, 0.02243013709216395, 0.014378101312311218, 0.007226947743759695, 0.0029276701875111384, 9.515166750008558E-4,
        0.0017727328875040689, 0.005558204850002899, 0.014378101312235814, 0.030803252137257802, 0.052905271651623786, 0.06562027788638072, 0.052905271651324026, 0.03080325213733769, 0.014378101312364885, 0.005558204849979354, 0.0017727328874979902,
        0.0025830133546704635, 0.008287002243679713, 0.02243013709210261, 0.052905271651950365, 0.10825670746239457, 0.15882720544362505, 0.10825670746187367, 0.05290527165080182, 0.02243013709242713, 0.008287002243769156, 0.0025830133546869602,
        0.00293672975627608, 0.009488002668872716, 0.026345595431503218, 0.06562027788603421, 0.15882720544151602, 0.44102631192030745, 0.15882720544590473, 0.06562027788637015, 0.026345595431065568, 0.009488002668778417, 0.0029367297562566848,
        0.0025830133546700966, 0.008287002243704267, 0.022430137092024266, 0.05290527165218751, 0.10825670746234733, 0.1588272054402839, 0.1082567074615041, 0.052905271651381314, 0.022430137092484193, 0.00828700224375486, 0.002583013354686416,
        0.0017727328875014527, 0.005558204850013428, 0.01437810131221156, 0.03080325213737849, 0.05290527165234342, 0.06562027788535467, 0.05290527165227899, 0.03080325213731504, 0.01437810131229074, 0.005558204849973625, 0.0017727328874977803,
        9.515166750022218E-4, 0.002927670187526038, 0.0072269477437592895, 0.014378101312185454, 0.02243013709218059, 0.02634559543148722, 0.0224301370922164, 0.014378101312200022, 0.007226947743773282, 0.0029276701875125123, 9.515166750016471E-4,
        3.988363875000695E-4, 0.0012200505375021846, 0.002927670187525898, 0.005558204849999022, 0.008287002243689638, 0.009488002668901728, 0.008287002243695645, 0.0055582048500028335, 0.002927670187519828, 0.0012200505375025872, 3.988363874999818E-4,
        1.2882849374993535E-4, 3.9883638750004726E-4, 9.515166750034058E-4, 0.0017727328875029819, 0.0025830133546718525, 0.002936729756279661, 0.002583013354672541, 0.0017727328875033709, 9.515166750023861E-4, 3.988363874999023E-4, 1.2882849374998856E-4
    );
    
    float b[121] = float[](
        8673174.0, 1.5982146E7, 2.5312806E7, 3.4957296E7, 4.2280236E7, 4.5059652E7, 4.2280236E7, 3.4957296E7, 2.5312806E7, 1.5982146E7, 8673174.0,
        1.5982146E7, 2.9347785E7, 4.6341531E7, 6.3895356E7, 7.7184405E7, 8.2245411E7, 7.7184405E7, 6.3895356E7, 4.6341531E7, 2.9347785E7, 1.5982146E7,
        2.5312806E7, 4.6341531E7, 7.2970173E7, 1.00453608E8, 1.21193181E8, 1.29118131E8, 1.21193181E8, 1.00453608E8, 7.2970173E7, 4.6341531E7, 2.5312806E7,
        3.4957296E7, 6.3895356E7, 1.00453608E8, 1.38192768E8, 1.66613346E8, 1.77507756E8, 1.66613346E8, 1.38192768E8, 1.00453608E8, 6.3895356E7, 3.4957296E7,
        4.2280236E7, 7.7184405E7, 1.21193181E8, 1.66613346E8, 2.00759625E8, 2.13875721E8, 2.00759625E8, 1.66613346E8, 1.21193181E8, 7.7184405E7, 4.2280236E7,
        4.5059652E7, 8.2245411E7, 1.29118131E8, 1.77507756E8, 2.13875721E8, 2.27856753E8, 2.13875721E8, 1.77507756E8, 1.29118131E8, 8.2245411E7, 4.5059652E7,
        4.2280236E7, 7.7184405E7, 1.21193181E8, 1.66613346E8, 2.00759625E8, 2.13875721E8, 2.00759625E8, 1.66613346E8, 1.21193181E8, 7.7184405E7, 4.2280236E7,
        3.4957296E7, 6.3895356E7, 1.00453608E8, 1.38192768E8, 1.66613346E8, 1.77507756E8, 1.66613346E8, 1.38192768E8, 1.00453608E8, 6.3895356E7, 3.4957296E7,
        2.5312806E7, 4.6341531E7, 7.2970173E7, 1.00453608E8, 1.21193181E8, 1.29118131E8, 1.21193181E8, 1.00453608E8, 7.2970173E7, 4.6341531E7, 2.5312806E7,
        1.5982146E7, 2.9347785E7, 4.6341531E7, 6.3895356E7, 7.7184405E7, 8.2245411E7, 7.7184405E7, 6.3895356E7, 4.6341531E7, 2.9347785E7, 1.5982146E7,
        8673174.0, 1.5982146E7, 2.5312806E7, 3.4957296E7, 4.2280236E7, 4.5059652E7, 4.2280236E7, 3.4957296E7, 2.5312806E7, 1.5982146E7, 8673174.0
 	);
    
    vec4 ac = vec4(0);
    vec4 bc = vec4(0);
    vec4 bcw = vec4(0);
    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
            vec4 tx0 = vec4(texture(iChannel1, uv + texel * vec2(i,j)).w);
            vec4 tx1 = texture(iChannel3, (uv + texel * vec2(i,j)));
            ac  += -a[index] * tx0;
            bcw +=  b[index];
            bc  +=  b[index] * tx1;
        }
    }
    
    bc /= bcw;
    fragColor = vec4(ac + bc);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
vec2 normz(vec2 x) { return x == vec2(0) ? vec2(0) : normalize(x); }
vec3 normz(vec3 x) { return x == vec3(0) ? vec3(0) : normalize(x); }


/* Texture Stencil Library https://www.shadertoy.com/view/ssBczm

The MIT License

Copyright (c) 2022 David A Roberts <https://davidar.io/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

vec4 texStencil(sampler2D ch, vec2 uv, float coeff[9]) {
    vec2 texel = 1. / vec2(textureSize(ch, 0));
    const vec2 stencilOffset[9] = vec2[](
        vec2(-1, 1), vec2( 0, 1), vec2( 1, 1),
        vec2(-1, 0), vec2( 0, 0), vec2( 1, 0),
        vec2(-1,-1), vec2( 0,-1), vec2( 1,-1)
    );
    vec4 r = vec4(0);
    for (int i = 0; i < 9; i++)
        r += coeff[i] * texture(ch, uv + texel * stencilOffset[i]);
    return r;
}

// Gaussian/binomial blur
// https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians/
vec4 texBlur(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        .0625, .125, .0625,
        .125,  .25,  .125,
        .0625, .125, .0625
    ));
}

// Laplacian, optimal 9-point stencil
// https://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1928&context=cstech
vec4 texLapl(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        1.,   4., 1.,
        4., -20., 4.,
        1.,   4., 1.
    )) / 6.;
}

// horizontal gradient (Sobel filter)
vec4 texGradX(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
        -1., 0., 1.,
        -2., 0., 2.,
        -1., 0., 1.
    )) / 8.;
}

// vertical gradient (Sobel filter)
vec4 texGradY(sampler2D ch, vec2 uv) {
    return texStencil(ch, uv, float[](
         1.,  2.,  1.,
         0.,  0.,  0.,
        -1., -2., -1.
    )) / 8.;
}





// IQ's simplex noise:

// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)),
			  dot(p,vec2(269.5,183.3)) );

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2 i = floor( p + (p.x+p.y)*K1 );
	
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = step(a.yx,a.xy);    
    vec2 b = a - o + K2;
	vec2 c = a - 1.0 + 2.0*K2;

    vec3 h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );

	vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));

    return dot( n, vec3(70.0) );
	
}


// GGX from Noby's Goo shader https://www.shadertoy.com/view/lllBDM

// MIT License: https://opensource.org/licenses/MIT
float G1V(float dnv, float k){
    return 1.0/(dnv*(1.0-k)+k);
}

float ggx(vec3 n, vec3 v, vec3 l, float rough, float f0){
    float alpha = rough*rough;
    vec3 h = normalize(v+l);
    float dnl = clamp(dot(n,l), 0.0, 1.0);
    float dnv = clamp(dot(n,v), 0.0, 1.0);
    float dnh = clamp(dot(n,h), 0.0, 1.0);
    float dlh = clamp(dot(l,h), 0.0, 1.0);
    float f, d, vis;
    float asqr = alpha*alpha;
    const float pi = 3.14159;
    float den = dnh*dnh*(asqr-1.0)+1.0;
    d = asqr/(pi * den * den);
    dlh = pow(1.0-dlh, 5.0);
    f = f0 + (1.0-f0)*dlh;
    float k = alpha/1.0;
    vis = G1V(dnl, k)*G1V(dnv, k);
    float spec = dnl * d * f * vis;
    return spec;
}