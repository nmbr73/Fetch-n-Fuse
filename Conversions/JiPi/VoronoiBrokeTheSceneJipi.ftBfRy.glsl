

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
// The goal of this Shader was to show that we can create a very simple voronoi cutting operator
// applicable to any scene with a simple function call (opCutVoronoi ).
// -----------------------------------------------
// Other cutting space : [CrashTest] https://www.shadertoy.com/view/wsSGDD
// -----------------------------------------------


#define WITH_EDGE


float tOpen;

// SPACE txt
//int[] gtxt = int[] (83,80,65,67,69);
// SCENE
int[] gtxt = int[] (74,73,80,73,32);

// [iq] https://www.shadertoy.com/view/XlXcW4
const uint k = 1103515245U;
vec3 hash33(vec3 p ) {
    uvec3 x = uvec3(p.x+10.,p.y+10.,p.z+10.);
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    vec3 o= vec3(x)*(1.0/float(0xffffffffU));
    return .3*cos(.2*(3.+o)*6.2 + 2.2+ o*6.2831853);
}

float hash13(const in vec3 p ) {
	float h = dot(p,vec3(127.1,311.7,758.5453123));	
    return fract(sin(h)*43758.5453123);
}

//---------------------------------------------------------------
// Here is the distance to voronoi3D cell (not exact: over estimate distance on edges)
//---------------------------------------------------------------
float sdVoronoi( in vec3 x, in vec3 cellId) {
    float md = 64.0;    
    vec3 mr = hash33(cellId);
    for( int k=-1; k<=1; k++ )
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ ) {
        if (i==0&&j==0&&k==0) continue;  // skip main cell 
        vec3 g = vec3(i,j,k),            // relative cell Id
             r = g + hash33(cellId + g); // pos of other point
        md = min(md, dot(.5*(mr+r)-x, normalize(r-mr))); // distance
    }
    return -1.2*md;
}

// --------------------------------------
// Space Operators
// --------------------------------------

// [iq] https://www.shadertoy.com/view/4lyfzw
float opExtrussion( in vec3 p, in float sdf, in float h) {
    vec2 w = vec2(sdf, abs(p.z) - h);
  	return min(max(w.x,w.y),0.) + length(max(w,0.));
}

// [iapafoto] https://www.shadertoy.com/view/7tBBDw
float opCutVoronoi(inout vec3 p, float k) {
    k += 1.;
    float d = 999.; //-dout;
    float dm;
    vec3 posTxt = p;

    for( int z=-1; z<=1; z++)
    for( int j=-1; j<=1; j++)
    for( int i=-1; i<=1; i++) {
         vec3 g = vec3(i,j,z)+ /*vec3(1.); // +*/ floor(p/k);
         if (length(k*g-p)<1.5) { // do it only on neighbourhood
             float v = sdVoronoi(p-k*g, g);
             if (d>v) {
                 posTxt = p-k*g+g;
             }
             d  = min(d,v);
         }
    }
    p = posTxt;

    return d;
}

float opSuperCut(inout vec3 p) {
    if (tOpen > 0.005) { // && p.y < 2. && p.y > -2. && p.z < 2.) {
        p *= 2.;
        float d = -.5*opCutVoronoi(p, tOpen);
        p/=2.;
        return d;
    } 
    return 999.;
}

// --------------------------------------
// Distance Functions
// --------------------------------------

// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
float sdFont(in vec2 p, in int c) {
    vec2 uv = (p + vec2(float(c%16), float(15-c/16)) + .5)/16.;
    return max(max(abs(p.x) - .25, max(p.y - .35, -.38 - p.y)), textureLod(iChannel0, uv, 0.).w - 127./255.);
}

float sdMessage2D(in vec2 p, in int[5] txt, in float scale) { 
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

float sdMessage3D(in vec3 p, in int[5] txt, in float scale, in float h) { 
    return opExtrussion(p, sdMessage2D(p.xy, txt, scale), h);
}

// --------------------------------------
// Distance to scene
// --------------------------------------
float map(in vec3 p) {
    float dcut = opSuperCut(p),
          dScene = sdMessage3D(p, gtxt,2.,.5);
    return max(dScene, -dcut);
}

// --------------------------------------
// Shading Tools
// --------------------------------------
// Find initial space position
vec4 MCol(in vec3 p) {
    float dcut = opSuperCut(p),
          dScene = sdMessage3D(p, gtxt,2.,.5);
    return vec4(p, dScene >= -dcut ? 1. : 2.);
}

// Shane - normal + edge
vec3 calcNormal(vec3 p, vec3 rd,  inout float edge, inout float crv, float t) { 
    float eps = 4.5/mix(450., min(850., iResolution.y), .35);
    float d = map(p);
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
    vec3 n = vec3(0.0);
    for( int i=min(iFrame, 0); i<4; i++) {
        vec3 e = .57735*(2.*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.);
        n += e*map(p + .001*e);
    }
    return normalize(n - max(.0, dot(n,rd))*rd);
}

// Box:  https://www.shadertoy.com/view/ld23DV
bool iBox( in vec3 ro, in vec3 rd, in vec3 sz, inout float tN, inout float tF) {
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
vec3 render(in vec3 ro, in vec3 rd, in float res, in vec3 pos, in vec3 n, in vec3 cobj, in vec3 light, vec3 cback, float spec) {
    float 
         amb = clamp(.5+.5*n.y, .0, 1.),
         dif = clamp(dot( n, light ), 0., 1.),
         pp = clamp(dot(reflect(-light,n), -rd),0.,1.),
         fre = (.7+.3*dif)*pow( clamp(1.+dot(n,rd),0.,1.), 2.);
    vec3 brdf = .5*(amb)+ 1.*dif*vec3(1.,.9,.7),
         sp = 3.*pow(pp,spec)*vec3(1., .6, .2),
	     col = cobj*(brdf + sp) + fre*(.5*cobj+.5);
    return mix(col, vec3(.02,.2,.2),smoothstep(3.,10.,res));
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr ) {
	vec3 cw = normalize(ta-ro),
         cp = vec3(sin(cr), cos(cr),.0),
         cu = normalize( cross(cw,cp) ),
         cv =          ( cross(cu,cw) );
    return mat3( cu, cv, cw );
}


// --------------------------------------
// Main
// --------------------------------------
void mainImage(out vec4 fragColor, in vec2 fragCoord ) {

    vec2 r = iResolution.xy, 
         m = iMouse.xy / r,
	     q = fragCoord.xy/r.xy;
 
    tOpen = .5*smoothstep(.6,0.,cos(.3*iTime));

    float a = mix(.3,3.*cos(.4*3.*iTime),.5+.5*cos(.2*iTime))+3.14*m.x;
    
    // camera	
    vec3 ta = vec3(0),
         ro = ta + 1.2*vec3(4.5*cos(a), 3.*cos(.4*iTime) + 4.*m.y, 4.5*sin(a));
    mat3 ca = setCamera( ro, ta, .1*cos(.123*iTime) );
 
    vec2 p = (2.*fragCoord-r.xy)/r.y;        
    // ray direction
    vec3 rd = ca * normalize( vec3(p,2.5) );

    float h = .1, t, tN = 0., tF = 10.;
    // Background color
	vec3 c = vec3(.11);

    if (iBox(ro, rd, vec3(2.3,.7,.6)*(1.+tOpen), tN, tF)) {		
        t = tN + .05*hash13(q.xyx);;
	// Ray marching
        for(int i=min(0,iFrame);i<100;i++) { 
            if (h<1e-3 || t>tF) break;
            t += h = map(ro + rd*t);
        }
    }
	
    vec3 lp =  ro + 3.*vec3(.25, 2, -.1);
            
    // Calculate color on point
	if (h < 1e-2) {
		vec3 pos = ro + t * rd;
        float edge = 0., crv = 1.;
        vec4 txt = MCol(pos); 	
        vec3 n = calcNormal(pos, rd, edge, crv, t),     
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
	} else {
        c *= .5+.5*hash13(q.xyx);   
    }
    
    // post prod
    c = pow(c, vec3(.75));
    c = vec3(c* pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y),.7f));
	fragColor = vec4(clamp(c, vec3(0), vec3(1)), t);	
}