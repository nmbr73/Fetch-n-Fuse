

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by Sebastien Durand - 2022
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// -----------------------------------------------
// The goal of this Shader was to show that we can create a very simple Cut dispersal 
// operator applicable to any scene with a simple function call (opCutDispersal). (opCutVoronoi ).
// -----------------------------------------------
// Other cutting space: 
//   [CrashTest]                       https://www.shadertoy.com/view/wsSGDD
//   [Voronoi broke the scene]         https://www.shadertoy.com/view/7tBBDw
//   [Cubic Dispersal broke the scene] https://www.shadertoy.com/view/7t2fRK
// -----------------------------------------------
// inspired by  Tater [Cubic Dispersal] https://www.shadertoy.com/view/fldXWS


#define WITH_EDGE
//#define WITH_MIN_BLOCK_SIZE // introduce imprecisions

float tOpen;

// SPACE txt
//int[] gtxt = int[] (83,80,65,67,69);
// SCENE
int[] gtxt = int[] (83,67,69,78,69);

mat2 rot(float a) {
    return mat2(cos(a), sin(a), -sin(a), cos(a));;
}

vec3 hash33(vec3 p) {   
	p = vec3(dot(p,vec3(127.1,311.7, 74.7)),
			 dot(p,vec3(269.5,183.3,246.1)),
			 dot(p,vec3(113.5,271.9,124.6)));
    return fract(sin(p)*43758.5453123);
}

float sdBox( vec3 p, vec3 b ) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// --------------------------------------
// Space Operators
// --------------------------------------

// [iq] https://www.shadertoy.com/view/4lyfzw
float opExtrussion(vec3 p, float sdf, float h) {
    vec2 w = vec2(sdf, abs(p.z) - h);
  	return min(max(w.x,w.y),0.) + length(max(w,0.));
}

// [iapafoto] https://www.shadertoy.com/view/7t2fRK
float opCutDispersal(inout vec3 uv, vec3 kdiv) {


    uv.xz *= rot(.2);
    uv.xy *= rot(.3);
    
#ifdef WITH_MIN_BLOCK_SIZE
    float ITERS = 4.;
#else
    float ITERS = 3.;
#endif    
    vec3 l0 = 2.*1.9*vec3(4.8,1.4,1.2);
    vec3 dMin = -l0*.5 - kdiv*pow(2.,ITERS-1.);
    vec3 dMax = l0*.5 + kdiv*pow(2.,ITERS-1.);
    
    float MIN_SIZE = 0.105;
    vec3 diff2 = vec3(1); 
    vec3 posTxt = uv;
    vec3 div0;

    float i = 0.;
    
    for(; i<ITERS;i++){
        // divide the box into quads
        div0 = vec3(.1) + .8*hash33(diff2);  // division sans interval
        
        // here is the magic!
        // conversion of the ratio to keep constant size 
        vec3 dd = kdiv*pow(2.,ITERS-1.-i),
            a0 = div0*l0,
            a2 = a0 + dd,
            l2 = l0 + 2.*dd,
            div2 = a2/l2; // ratio de division en tenant compte des bodures
     
        // On determine la division
        vec3 divide = mix(dMin, dMax, div2);
        
#ifdef WITH_MIN_BLOCK_SIZE
        //Find the minimum dimension size
        vec3 minAxis = min(abs(a0), abs(l0-a0));
        float minSize = min(minAxis.x, min( minAxis.y, minAxis.z));
        
        // if minimum dimension is too small break out
        // => this introduce imprecision in distance field
        bool smallEnough = minSize < MIN_SIZE;
        if (smallEnough && i + 1. > 1.) { break; }
#endif

        l0 = mix(l0-a0, a0, step(uv, divide)); // ne prendre que la partie du bon coté
        
        // update the box domain
        dMax = mix( dMax, divide, step(uv, divide ));
        dMin = mix( divide, dMin, step(uv, divide ));

        //Deterministic seeding for future divisions 
        diff2 = step(uv, divide) - 10.*hash33(diff2);
        posTxt -= dd*(.5 - step(uv, divide));
    }
     
    //Calculate 2d box sdf
    vec3 center = (dMin + dMax)/2.0;
    vec3 dd0 = .5*kdiv*pow(2., ITERS-(i-1.));
    float d = sdBox(uv-center, .5*(dMax - dMin) - .5*dd0);
    uv = posTxt;
    uv.xy *= rot(-.3);
    uv.xz *= rot(-.2);
    return d;
}


// --------------------------------------
// Distance Functions
// --------------------------------------

// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
float sdFont(vec2 p, int c) {
    vec2 uv = (p + vec2(float(c%16), float(15-c/16)) + .5)/16.;
    return max(max(abs(p.x) - .25, max(p.y - .35, -.38 - p.y)), textureLod(iChannel0, uv, 0.).w - 127./255.);
}

float sdMessage2D(vec2 p, int[5] txt, float scale) { 
    p.y += .1;
    p /= scale;
 	float d = 999., w = .45; // letter width  
    p.x += w*float(txt.length()-1)*.5; // center text arround 0
    for (int id = 0; id<5; id++){
    	d = min(d, sdFont(p, txt[id]));   
    	p.x -= w; 
    }
    return scale*d;
}

float sdMessage3D(in vec3 p, int[5] txt, float scale, float h) { 
    return opExtrussion(p, sdMessage2D(p.xy, txt, scale), h);
}

// --------------------------------------
// Distance to scene
// --------------------------------------
float map(vec3 p) {
    float dcut = opCutDispersal(p, .7*vec3(.8,.4,.8)*tOpen), //opSuperCut(p),
          dScn = sdMessage3D(p, gtxt,4.,1.);
    return max(dScn, dcut);
}

// --------------------------------------
// Shading Tools
// --------------------------------------
// Find initial space position
vec4 MCol(vec3 p) {
    float dcut = opCutDispersal(p, .7*vec3(.8,.4,.8)*tOpen),
          dScn = sdMessage3D(p, gtxt,4.,1.);
    return vec4(p, dScn >= dcut ? 1. : 2.);
}

// Shane - normal + edge
vec3 normal(vec3 p, vec3 rd, inout float edge, float t) { 
    float eps = 4.5/mix(450., min(850., iResolution.y), .35),
          d = map(p);
#ifdef WITH_EDGE
    vec3 e = vec3(eps, 0, 0),
         da = vec3(-2.*d);
    for(int i = min(iFrame,0); i<3; i++) {
        for( int j=min(iFrame,0); j<2; j++ )
            da[i] += map(p + e*float(1-2*j));
        e = e.zxy;
    }
    da = abs(da);
    edge = da.x + da.y + da.z;
    edge = smoothstep(0., 1., sqrt(edge/e.x*2.));
#endif
    vec3 n = vec3(0);
    for( int i=min(iFrame, 0); i<4; i++) {
        vec3 e = .57735*(2.*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.);
        n += e*map(p + .001*e);
    }
    return normalize(n - max(.0, dot(n,rd))*rd);
}

// Box:  https://www.shadertoy.com/view/ld23DV
bool iBox( vec3 ro, vec3 rd, vec3 sz, inout float tN, inout float tF) {
    vec3 m = sign(rd)/max(abs(rd), 1e-8),
         n = m*ro,
         k = abs(m)*sz,
         t1 = -n - k,
         t2 = -n + k;
	tN = max( max( t1.x, t1.y ), t1.z );
	tF = min( min( t2.x, t2.y ), t2.z );
    return !(tN > tF || tF <= 0.0);
}

//----------------------------------
// Texture 3D (Shane)
//----------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){  
    n = max(n*n, .001);
    n /= n.x + n.y + n.z;  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf){   
    const vec2 e = vec2(.001, 0);
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    vec3 g = vec3(.299, .587, .114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(.299, .587, .114)) )/e.x; 
    g -= n*dot(n, g);
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
}


//----------------------------------
// Shading
//----------------------------------
vec3 render(vec3 ro, vec3 rd, float res, vec3 pos, vec3 n, vec3 cobj, vec3 light, vec3 cback, float spec) {
    float 
         amb = clamp(.5+.5*n.y, .0, 1.),
         dif = clamp(dot( n, light ), 0., 1.),
         pp = clamp(dot(reflect(-light,n), -rd),0.,1.),
         fre = (.7+.3*dif)*pow( clamp(1.+dot(n,rd),0.,1.), 2.);
    vec3 brdf = .5*(amb)+ 1.*dif*vec3(1.,.9,.7),
         sp = 3.*pow(pp,spec)*vec3(1, .6, .2),
	     col = cobj*(brdf + sp) + fre*(.5*cobj+.5);
    return mix(col, vec3(.02,.2,.2),smoothstep(6.,20.,res));
}

mat3 setCamera(vec3 ro, vec3 ta, float r ) {
	vec3 w = normalize(ta-ro),
         p = vec3(sin(r), cos(r),.0),
         u = normalize( cross(w,p) ),
         v =          ( cross(u,w) );
    return mat3( u, v, w );
}


// --------------------------------------
// Main
// --------------------------------------
void mainImage(out vec4 fragColor, vec2 fragCoord ) {

    vec2 r = iResolution.xy, 
         m = iMouse.xy / r,
	     q = fragCoord.xy/r.xy;
 
    tOpen = .4*smoothstep(.6,0.,cos(.3*iTime));

    float a = mix(.3,3.*cos(.4*3.*iTime),.5+.5*cos(.2*iTime))+3.14*m.x;
    
    // camera	
    vec3 ta = vec3(0),
         ro = ta + 2.4*vec3(4.5*cos(a), 3.*cos(.4*iTime) + 4.*m.y, 4.5*sin(a));
    mat3 ca = setCamera( ro, ta, .1*cos(.123*iTime) );
  
    // ray direction
    vec3 rd = ca * normalize( vec3((2.*fragCoord-r.xy)/r.y, 2.5));

    float h = .1, t, tN = 0., tF = 20.;
    
    // Background color
	vec3 c = .09*vec3(hash33(q.xyx).x + 1.);

    if (iBox(ro, rd, vec3(4.8,1.4,1.2)*(1.+vec3(1.,2.,3.)*tOpen), tN, tF)) {		
        t = tN;// - .02*hash33(q.xyx).x;
	// Ray marching
        for(int i=min(0,iFrame);i<200;i++) { 
            if (h<1e-3 || t>tF) break;
            t += h = map(ro + rd*t);
        }
    
        // light pos
        vec3 lp =  ro + 3.*vec3(.25, 2, -.1);

        // Calculate color on point
        if (t<tF) {
            vec3 pos = ro + t * rd;
            float edge = 0.;
            vec4 txt = MCol(pos); 	
            vec3 n = normal(pos, rd, edge, t),     
                 cobj = txt.w<1.5 ? vec3(.7) : 1.5*vec3(.8,.4,.0);
            if (txt.w<1.5) {
                n = doBumpMap(iChannel1, txt.xyz*2., n, .01);
            } else {
                n = doBumpMap(iChannel2, txt.xyz*2., n, .02);
            }
            // keep in visible side
            n = normalize(n - max(.0,dot(n,rd))*rd);
            // Shading
            c = render(ro, rd, t, pos, n, cobj, normalize(lp-pos), c, txt.w<1.5 ? 99. : 16.);
    #ifdef WITH_EDGE
            c *= 1. - edge*.8;
    #endif
        } 
    } //else{
    //c *= 2.; 
    //}
    
    
    // post prod
    c = pow(c, vec3(.75));
    c = vec3(c* pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y),.7f));
	fragColor = vec4(c, t);	
}