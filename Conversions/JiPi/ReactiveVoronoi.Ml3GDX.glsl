

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by genis sole - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.

// A remastered version of this can be found here: https://www.shadertoy.com/view/MtyGWK 
// Adds a better traversal, stronger lighting, softer shadows and AO.

const float PI = 3.1416;

vec2 hash2( vec2 p )
{
    // procedural white noise	
	return fract(sin(vec2(dot(p,vec2(127.1,311.7)),
                          dot(p,vec2(269.5,183.3))))*43758.5453);
}

// From http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm
vec3 voronoi( in vec2 x )
{
    vec2 n = floor(x);
    vec2 f = fract(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
	vec2 mg, mr;

    float md = 8.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        vec2 g = vec2(float(i),float(j));
		vec2 o = hash2( n + g );
		#ifdef ANIMATE
        o = 0.5 + 0.5*sin( iTime + 6.2831*o );
        #endif
        vec2 r = g + o - f;
        float d = dot(r,r);

        if( d<md )
        {
            md = d;
            mr = r;
            mg = g;
        }
    }

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 8.0;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = mg + vec2(float(i),float(j));
		vec2 o = hash2( n + g );
		#ifdef ANIMATE
        o = 0.5 + 0.5*sin( iTime + 6.2831*o );
        #endif	
        vec2 r = g + o - f;

        if( dot(mr-r,mr-r)>0.00001 )
        md = min( md, dot( 0.5*(mr+r), normalize(r-mr) ) );
    }

    return vec3( md, mr );
}


// Modified version of the above iq's voronoi borders. 
// Returns the distance to the border in a given direction.
vec3 voronoi( in vec2 x, in vec2 dir)
{
    vec2 n = floor(x);
    vec2 f = fract(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
	vec2 mg, mr;

    float md = 8.0;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        vec2 g = vec2(float(i),float(j));
		vec2 o = hash2( n + g );
        vec2 r = g + o - f;
        float d = dot(r,r);

        if( d<md )
        {
            md = d;
            mr = r;
            mg = g;
        }
    }

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 1e5;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        vec2 g = mg + vec2(float(i),float(j));
		vec2 o = hash2( n + g );
		vec2 r = g + o - f;

    
 		if( dot(r-mr,r-mr) > 1e-5 ) {
            vec2 l = r-mr;
            
            if (dot(dir, l) > 1e-5) {
            	md = min(md, dot(0.5*(mr+r), l)/dot(dir, l));
            }
        }
        
    }
    
    return vec3( md, n+mg);
}

bool IRayAABox(in vec3 ro, in vec3 rd, in vec3 invrd, in vec3 bmin, in vec3 bmax, 
               out vec3 p0, out vec3 p1) 
{
    vec3 t0 = (bmin - ro) * invrd;
    vec3 t1 = (bmax - ro) * invrd;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    float fmin = max(max(tmin.x, tmin.y), tmin.z);
    float fmax = min(min(tmax.x, tmax.y), tmax.z);
    
    p0 = ro + rd*fmin;
    p1 = ro + rd*fmax;
 
    return fmax >= fmin;   
}

vec3 AABoxNormal(vec3 bmin, vec3 bmax, vec3 p) 
{
    vec3 n1 = -(1.0 - smoothstep(0.0, 0.03, p - bmin));
    vec3 n2 = (1.0 -  smoothstep(0.0, 0.03, bmax - p));
    
    return normalize(n1 + n2);
}

const vec3 background = vec3(0.04);
const vec3 scmin = -vec3(1.77, 1.0, 1.77);
const vec3 scmax = vec3(1.77, 1.5, 1.77);

// From http://iquilezles.org/www/articles/palettes/palettes.htm
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

vec3 color(vec2 p) {
    return pal(3.434+(hash2(p).x*0.02), 
               vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,0.7,0.4),vec3(0.0,0.15,0.20)  );
}

float disp(in vec2 p) {
    return scmin.y + 0.1 + hash2(p).x * 0.5 + texture(iChannel0, vec2(hash2(p).x, 0.0)).r*2.0;
}

vec4 map(in vec2 p, in vec2 dir) {
    vec3 v = voronoi(p*2.0, dir)*0.5;
    return vec4(v, disp(v.yz));
}

float ShadowFactor(in vec3 ro, in vec3 rd) {
	vec3 p0 = vec3(0.0);
    vec3 p1 = vec3(0.0);
    
    IRayAABox(ro, rd, 1.0/rd, scmin, scmax, p0, p1);
    p0 = ro + rd*0.02;
    
    vec2 dir = normalize(rd.xz);
    float sf = rd.y / length(rd.xz);

    float m = -1e5;
    
    const int max_steps = 32;
    for (int i = max_steps; i > 0; --i) {
        if (p0.y < m) break;
        
        if (dot((p1 - p0), rd) < 0.0) return 1.0;
  
        vec4 v = map(p0.xz, dir);
        
        m = v.w;
        if (p0.y < m) return 0.0;
        
        p0 += rd*(length(vec2(v.x, v.x*sf)) + 0.02);
    }
    
    p0 += rd * (m - p0.y)/rd.y;
    if (dot((p1 - p0), rd) < 0.0) return 1.0;   
    
    return 0.0;
}

vec3 Shade(in vec3 p, in vec3 n, in vec3 ld, in vec2 c) {
    vec3 col = color(c);
	return (col * 0.15 + col * max(0.0, dot(n,ld)) * ShadowFactor(p, ld) * 0.85) * 3.5;
}

vec3 Render(in vec3 ro, in vec3 rd, in vec3 ld) {
    vec3 p0 = vec3(0.0);
    vec3 p1 = vec3(0.0);
    
    if (!IRayAABox(ro, rd, 1.0/rd, scmin, scmax, p0, p1)) return background;
    
    vec2 dir = normalize(rd.xz);
    float sf = rd.y / length(rd.xz);
    
    vec2 lvp = vec2(0);
    vec2 vp = p0.xz;
    
    float m = -1e5;
    
    vec3 n = vec3(0.0);
    
    const int max_steps = 32;
    for (int i = max_steps; i > 0; --i) {
        if (p0.y < m) {
            n = vec3(0.0, 1.0, 0.0);
            break;
        }
        
        if (dot((p1 - p0), rd) < 0.0) return background;
  
        vec4 v = map(p0.xz, dir);
		
        lvp = vp;
        vp = v.yz;
        
        m = v.w;
        if (p0.y < m) break;
        
        p0 += rd*(length(vec2(v.x, v.x*sf)) + 0.02);
    }
    
    
    
    if (n.y != 0.0) {
    	p0 += rd * (-p0.y + m)/rd.y;
        if (dot((p1 - p0), rd) < 0.0) return background;
    }
    
    n = normalize(mix(vec3(normalize(lvp - vp), 0.0).xzy, n, 
                  smoothstep(0.00, 0.03, voronoi(p0.xz*2.0).x*0.5)));
    
    if (all(equal(p0.xz, lvp))) {
    	n = AABoxNormal(scmin, scmax, p0); 
    }
    
    return Shade(p0, n, ld, vp);
}

void CameraOrbitRay(in vec2 fragCoord, in float n, in vec3 c, in float d, 
                    out vec3 ro, out vec3 rd, out mat3 t) 
{
    float a = 1.0/max(iResolution.x, iResolution.y);
    rd = normalize(vec3((fragCoord - iResolution.xy*0.5)*a, n));
 
    ro = vec3(0.0, 0.0, -d);
    
    float ff = min(1.0, step(0.001, iMouse.x) + step(0.001, iMouse.y));
    vec2 m = PI*ff + vec2(((iMouse.xy + 0.1) / iResolution.xy) * (PI*2.0));
    m.y = -m.y;
    m.y = sin(m.y*0.5)*0.6 + 0.6;
        
    mat3 rotX = mat3(1.0, 0.0, 0.0, 0.0, cos(m.y), sin(m.y), 0.0, -sin(m.y), cos(m.y));
    mat3 rotY = mat3(cos(m.x), 0.0, -sin(m.x), 0.0, 1.0, 0.0, sin(m.x), 0.0, cos(m.x));
    
    t = rotY * rotX;
    
    ro = t * ro;
    ro = c + ro;

    rd = t * rd;
    
    rd = normalize(rd);
}

vec3 LightDir(in mat3 t) 
{
    vec3 l = normalize(vec3(1.0, 1.0, -1.0));
    return t * l;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 ro = vec3(0.0);
    vec3 rd = vec3(0.0);
    mat3 t = mat3(1.0);
    
    CameraOrbitRay(fragCoord, 1.0, vec3(0.0), 10.0, ro, rd, t);
	fragColor = vec4(pow(Render(ro, rd, LightDir(t)), vec3(0.5454)), 1.0);
}