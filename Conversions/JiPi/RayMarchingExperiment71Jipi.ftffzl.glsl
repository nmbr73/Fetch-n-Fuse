

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by Stephane Cuillerdier - @Aiekick/2017
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float dispSize;

vec3 effect(vec2 p) 
{
    return texture(iChannel1, (p+16.)*0.02).rgb;
}


vec4 displacement(vec3 p)
{
    vec2 g = p.xz;
    vec3 col = 1.-clamp(effect(g*5.),0.,1.);
    //vec3 music = texture(iChannel2, vec2( 0.15, 0.25 )).rgb*2.;
   	float dist = dot(col,vec3(dispSize));
    return vec4(dist,col);
}

vec4 map(vec3 p)
{
    vec4 disp = displacement(p);
    return vec4(length(p) - 4. - (disp.x), disp.yzw);
}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.03, 0., 0. );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
}

float march(vec3 ro, vec3 rd, float rmPrec, float maxd, float mapPrec)
{
    float s = rmPrec;
    float d = 0.;
    for(int i=0;i<180;i++)
    {      
        if (d/s>1e5||d>maxd) break;
        s = map(ro+rd*d).x;
        d += s*0.2;
    }
    return d;
}

void mainImage( out vec4 f, in vec2 g )
{
    dispSize = 0.15;
    
    float time = iTime*0.25;
    float cam_a = time; 
    
    float cam_e = 5.52; 
    float cam_d = 1.88; 
    
  	vec2 s = iResolution.xy;
    vec2 uv = (g+g-s)/s.y;
    
    if (iMouse.z > 0.)
        dispSize = iMouse.x / s.x * 2. - 1.;
    
    
    vec3 col = vec3(0.);
    
    vec3 ro = vec3(-sin(cam_a)*cam_d, cam_e+1., cos(cam_a)*cam_d); //
  	vec3 rov = normalize(-ro);
    vec3 u = normalize(cross(vec3(0,1,0),rov));
  	vec3 v = cross(rov,u);
  	vec3 rd = normalize(rov + uv.x*u + uv.y*v);
    
    float b = .35;
    
    float d = march(ro, rd, 1e-5, 50., .5);
    
    if (d<50.)
    {
        vec2 e = vec2(-1., 1.)*0.005; 
    	vec3 p = ro+rd*d;
        vec3 n = calcNormal(p);
        
        vec3 reflRay = reflect(rd, n);
		vec3 refrRay = refract(rd, n, .7);
        
        vec3 cubeRefl = texture(iChannel0, reflRay).rgb * .5;
        vec3 cubeRefr = texture(iChannel0, refrRay).rgb * .8;
        
        col = cubeRefl + cubeRefr + pow(b, 15.);
        
		vec3  lig = normalize( vec3(-0.6, 0.7, -0.5) );
		float dif = clamp( dot( n, lig ), 0.0, 1.0 );
        float spe = pow(clamp( dot( reflRay, lig ), 0.0, 1.0 ),16.0);

		vec3 brdf = vec3(0);
        brdf += 1.2*dif*vec3(1);
		brdf += .5*spe*vec3(1,.8,.2)*dif;
		
    	col = mix(brdf, map(p).yzw, 0.5);
    }
    else
    {
        col = texture(iChannel0, rd).rgb;
    }
    
	f.rgb = col;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Begin IQ's simplex noise:

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

// End IQ's simplex noise

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec2 normz(vec2 x) {
	return x == vec2(0.0, 0.0) ? vec2(0.0, 0.0) : normalize(x);
}

// reverse advection
vec3 advect(vec2 ab, vec2 vUv, vec2 step, float sc) {
    
    vec2 aUv = vUv - ab * sc * step;
    
    const float _G0 = 0.25; // center weight
    const float _G1 = 0.125; // edge-neighbors
    const float _G2 = 0.0625; // vertex-neighbors
    
    // 3x3 neighborhood coordinates
    float step_x = step.x;
    float step_y = step.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec3 uv =    texture(iChannel0, fract(aUv)).xyz;
    vec3 uv_n =  texture(iChannel0, fract(aUv+n)).xyz;
    vec3 uv_e =  texture(iChannel0, fract(aUv+e)).xyz;
    vec3 uv_s =  texture(iChannel0, fract(aUv+s)).xyz;
    vec3 uv_w =  texture(iChannel0, fract(aUv+w)).xyz;
    vec3 uv_nw = texture(iChannel0, fract(aUv+nw)).xyz;
    vec3 uv_sw = texture(iChannel0, fract(aUv+sw)).xyz;
    vec3 uv_ne = texture(iChannel0, fract(aUv+ne)).xyz;
    vec3 uv_se = texture(iChannel0, fract(aUv+se)).xyz;
    
    return _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const float _K0 = -20.0/6.0; // center weight
    const float _K1 = 4.0/6.0;   // edge-neighbors
    const float _K2 = 1.0/6.0;   // vertex-neighbors
    const float cs = -0.6;  // curl scale
    const float ls = 0.05;  // laplacian scale
    const float ps = -0.8;  // laplacian of divergence scale
    const float ds = -0.05; // divergence scale
    const float dp = -0.04; // divergence update scale
    const float pl = 0.3;   // divergence smoothing
    const float ad = 6.0;   // advection distance scale
    const float pwr = 1.0;  // power when deriving rotation angle from curl
    const float amp = 1.0;  // self-amplification
    const float upd = 0.8;  // update smoothing
    const float sq2 = 0.6;  // diagonal weight

    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec3 uv =    texture(iChannel0, fract(vUv)).xyz;
    vec3 uv_n =  texture(iChannel0, fract(vUv+n)).xyz;
    vec3 uv_e =  texture(iChannel0, fract(vUv+e)).xyz;
    vec3 uv_s =  texture(iChannel0, fract(vUv+s)).xyz;
    vec3 uv_w =  texture(iChannel0, fract(vUv+w)).xyz;
    vec3 uv_nw = texture(iChannel0, fract(vUv+nw)).xyz;
    vec3 uv_sw = texture(iChannel0, fract(vUv+sw)).xyz;
    vec3 uv_ne = texture(iChannel0, fract(vUv+ne)).xyz;
    vec3 uv_se = texture(iChannel0, fract(vUv+se)).xyz;
    
    // uv.x and uv.y are the x and y components, uv.z is divergence 

    // laplacian of all components
    vec3 lapl  = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    float sp = ps * lapl.z;
    
    // calculate curl
    // vectors point clockwise about the center point
    float curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + sq2 * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    
    // compute angle of rotation from curl
    float sc = cs * sign(curl) * pow(abs(curl), pwr);
    
    // calculate divergence
    // vectors point inwards towards the center point
    float div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + sq2 * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    float sd = uv.z + dp * div + pl * lapl.z;

    vec2 norm = normz(uv.xy);
    
    vec3 ab = advect(vec2(uv.x, uv.y), vUv, texel, ad);
    
    // temp values for the update rule
    float ta = amp * ab.x + ls * lapl.x + norm.x * sp + uv.x * ds * sd;
    float tb = amp * ab.y + ls * lapl.y + norm.y * sp + uv.y * ds * sd;

    // rotate
    float a = ta * cos(sc) - tb * sin(sc);
    float b = ta * sin(sc) + tb * cos(sc);
    
    vec3 abd = upd * uv + (1.0 - upd) * vec3(a,b,sd);
    
    /*if (iMouse.z > 0.0) {
    	vec2 d = fragCoord.xy - iMouse.xy;
        float m = exp(-length(d) / 10.0);
        abd.xy += m * normz(d);
    }*/
    
    // initialize with noise
    if(iFrame<180 || reset()) {
        vec3 rnd = vec3(noise(16.0 * vUv + 1.1), noise(16.0 * vUv + 2.2), noise(16.0 * vUv + 3.3));
        fragColor = vec4(rnd, 0);
        fragColor = -0.5 + texture(iChannel1, fragCoord.xy / iResolution.xy);
    } else {
        //fragColor = clamp(vec4(abd,0.0), -1., 1.);
        abd.z = clamp(abd.z, -1.0, 1.0);
        abd.xy = clamp(length(abd.xy) > 1.0 ? normz(abd.xy) : abd.xy, -1.0, 1.0);
        fragColor = vec4(abd, 0.0);
    }
    

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Visualization of the system in Buffer A

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 texel = 1. / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 c = texture(iChannel0, uv).xyz;
    vec3 norm = normalize(c);
    
    vec3 div = vec3(0.1) * norm.z;    
    vec3 rbcol = 0.5 + 0.6 * cross(norm.xyz, vec3(0.5, -0.4, 0.5));
    
    fragColor = vec4(rbcol + div, 0.0);
}