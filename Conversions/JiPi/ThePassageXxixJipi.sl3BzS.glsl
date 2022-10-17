

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by SHAU - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------

/**
 * Still life of The Passage XXIX by H.R.Giger
 * https://wikioo.org/paintings.php?refarticle=A25TB8&titlepainting=hr+giger+passage+XXIX&artistname=H.R.+Giger
 */
 
#define R iResolution.xy

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

void mainImage(out vec4 C, vec2 U)
{
    vec3 col = texture(iChannel0,U/R).xyz;
    if (U.x<R.x*0.25 || U.x>R.x*0.75) col = vec3(0.75,0.7,0.8);
    col = pow(ACESFilm(col),vec3(0.3545));
    C = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Created by SHAU - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//-----------------------------------------------------

#define R iResolution.xy
#define ZERO (min(iFrame,0))
#define EPS .002
#define FAR 100.
#define T iTime

//Fabrice - compact rotation
mat2 rot(float x) {return mat2(cos(x), sin(x), -sin(x), cos(x));}

//Shane IQ
float n3D(vec3 p) {    
	const vec3 s = vec3(7, 157, 113);
	vec3 ip = floor(p); 
    p -= ip; 
    vec4 h = vec4(0., s.yz, s.y + s.z) + dot(ip, s);
    p = p * p * (3. - 2. * p);
    h = mix(fract(sin(h) * 43758.5453), fract(sin(h + s.x) * 43758.5453), p.x);
    h.xy = mix(h.xz, h.yw, p.y);
    return mix(h.x, h.y, p.z);
}

//distance functions from IQ
//https://iquilezles.org/articles/distfunctions
float sdCappedCylinder( vec3 p, float h, float r )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(h,r);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float dot2( in vec2 v ) { return dot(v,v); }
float sdCappedCone( vec3 p, float h, float r1, float r2 )
{
  vec2 q = vec2( length(p.xy), p.z );
  vec2 k1 = vec2(r2,h);
  vec2 k2 = vec2(r2-r1,2.0*h);
  vec2 ca = vec2(q.x-min(q.x,(q.y<0.0)?r1:r2), abs(q.y)-h);
  vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot2(k2), 0.0, 1.0 );
  float s = (cb.x<0.0 && ca.y<0.0) ? -1.0 : 1.0;
  return s*sqrt( min(dot2(ca),dot2(cb)) );
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xy)-t.x,p.z);
  return length(q)-t.y;
}

float sdTriPrism( vec3 p, vec2 h )
{
  vec3 q = abs(p);
  return max(q.z-h.y,max(q.x*0.866025-p.y*0.5,-p.y)-h.x*0.5);
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float sdEllipsoid(vec3 p, vec3 r)
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0)/k1;
}

float smin(float a, float b, float k) {
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0., 1.);
	return mix(b, a, h) - k * h * (1. - h);
}

float smax(float a, float b, float k) {
	float h = clamp( 0.5 + 0.5 * (b - a) / k, 0.0, 1.0 );
	return mix(a, b, h) + k * h * (1.0 - h);
}

vec2 near(vec2 a, vec2 b)
{
    float s = step(a.x,b.x);
    return s*a + (1.0-s)*b;
}

float dfHandle(vec3 p, float r)
{
    p.xy *= rot(r);
    float t = min(sdTorus(p,vec2(1.6,0.3)),
                  sdCapsule(p,vec3(0.0,0.0,-0.3),vec3(0.0,0.0,4.0),0.3));
    t = min(t,sdCapsule(p,vec3(-2.0,0.0,0.0),vec3(2.0,0.0,0.0),0.2));
    t = min(t,sdCapsule(p,vec3(0.0,-2.0,0.0),vec3(0.0,2.0,0.0),0.2));
    return t;
}
vec2 map(vec3 p) 
{
    //backplane
    float t = min(sdBox(p-vec3(0.0,0.0,3.2),vec3(100,100,1)),
            sdTorus(p-vec3(0.0,13.0,2.2),vec2(25.0,0.2)));
    t = max(t,-sdBox(p-vec3(0,1,0),vec3(5.0,7.0,10.0)));
    t = min(t,sdBox(p-vec3(-23.0,2.0,3.2),vec3(13.0,6.0,2.5)));
    t = min(t,sdBox(p-vec3(-9.0,6.5,3.2),vec3(1.0,1.5,2.5)));
    t = min(t,sdBox(p-vec3(-8.0,-2.8,2.4),vec3(1.8,0.4,2.0)));  
    t = smin(t,sdBox(p-vec3(-13.5,-4.0,2.0),vec3(1.0,2.0,2.0)),0.5);
    //side hinge
    float th = min(sdCappedCylinder(p-vec3(-8.0,7.2,1.0),1.0,1.0),
                   sdBox(p-vec3(-18.0,7.2,1.0),vec3(10.0,1.0,1.0)));
    th = max(th,-p.y-p.x*0.1+5.0);
    t = min(t,th);
    //web
    t = min(t,sdCappedCylinder(p.xzy-vec3(-10.0,3.2,5.0),0.3,4.0));
    t = min(t,sdBox(p-vec3(-10.0,0.0,3.2),vec3(0.3,5.0,4.0)));
    t = min(t,sdBox(p-vec3(-8.5,5.0,3.2),vec3(1.5,0.3,4.0)));
    //FRONT DOOR
    float td = sdBox(p,vec3(5.0,5.0,0.2));
    td = max(td,-sdCappedCone(p,1.0,4.8,3.4));
    td = min(td,sdTorus(p-vec3(0.0,0.0,-0.2),vec2(4.4,0.2)));
    //bottom hinge
    td = smin(td,sdCappedCylinder(p.yxz-vec3(-5.5,0.0,0.5),1.0,5.0),0.2);
    //top hinge
    td = smin(td,sdCappedCylinder(p.yxz-vec3(5.25,0.0,0.0),0.5,5.0),0.2);
    vec3 q = p-vec3(2.0,0.0,0.0)*clamp(round(p/vec3(2.0,0.0,0.0)),-2.0,2.0);
    td = max(td,-sdCappedCylinder(q.yxz-vec3(5.25,0.0,0.0),0.7,0.5));
    td = min(td,sdCappedCylinder(q.yxz-vec3(5.25,0.0,0.0),0.5,0.42));
    
    //clean edges????
    //td = max(td,abs(p.x)-5.0);
    
    q = p;
    q.x = abs(q.x);
    //backplane again
    t = min(t,sdBox(q-vec3(5.0,20.0,3.2),vec3(1.0,14.0,1.3)));
    //rivets
    float metal = min(sdCappedCylinder(q.xzy-vec3(5.0,0.0,5.0),0.2,1.2),
                      sdCappedCylinder(q.xzy-vec3(4.0,0.0,5.0),0.2,1.2));
    metal = min(metal,sdCappedCylinder(q.xzy-vec3(5.0,0.0,4.2),0.2,1.2));
    q.y = abs(q.y);
    td = min(td,sdBox(q-vec3(4.5,3.0,0.0),vec3(0.5,0.3,0.6)));
    metal = min(metal,sdCappedCylinder(q.xzy-vec3(4.5,0.0,3.0),0.2,0.8));
    t = min(t,td);
    //top bracket
    q = p - vec3(0.0,5.5,0.0);
    q.yz *= rot(0.35);
    t = min(t,sdBox(q-vec3(0.0,4.5,0.0),vec3(4.0,4.0,0.1)));
    float b = sdBox(q-vec3(0.0,1.8,-0.7),vec3(5.45,1.8,0.2));
    b = smax(b,sdCappedCylinder(q.xzy-vec3(0.0,0.0,0.0),5.4,1.0),0.2);
    b = smax(b,-sdCappedCylinder(q.xzy-vec3(0.0,0.0,0.0),3.0,1.0),0.2);
    b = smin(b,sdBox(vec3(abs(p.x),p.yz)-vec3(4.2,4.8,-0.65),vec3(1.2,1.0,0.2)),0.2);
    t = min(t,b-0.1);
    //sides
    q = p;
    q.x = abs(q.x);
    t = min(t,sdBox(q-vec3(4.9,-0.125,0.0),vec3(0.1,5.375,0.5)));
    t = min(t,sdBox(q-vec3(5.3,-0.3,1.5),vec3(0.15,6.3,2.1)));
    t = min(t,max(sdTriPrism(q.yzx-vec3(-7.0,0.0,5.3),vec2(2.0,0.15)),p.z-3.0));
    //handles
    t = min(t,sdBox(vec3(p.x,abs(p.y-3.6),p.z)-vec3(13.0,6.0,2.8),vec3(3.0,0.5,1.0)));
    metal = min(metal,dfHandle(vec3(p.x,abs(p.y-3.6),p.z)-vec3(13.0,6.0,1.0),0.3));    
    //cut
    //top
    t = max(t,-sdBox(p-vec3(0.0,5.0,0.0),vec3(1.6,2.0,1.0)));
    //bottom
    t = max(t,-sdBox(p-vec3(0.0,-6.0,0.0),vec3(1.6,0.8,2.0)));
    metal = min(metal,sdCappedCylinder(p.yxz-vec3(-5.5,-0.5,0.5),0.6,7.0));    
    //bottom pistons
    q.yz *= rot(-0.1);
    metal = min(metal,sdCappedCylinder(q-vec3(0.0,-7.0,0.0),1.8,0.2));
    metal = min(metal,sdCappedCylinder(q-vec3(1.0,-16.0,0.0),0.4,9.0));
    float white = sdCappedCylinder(q-vec3(1.0,-14.0,0.0),0.7,5.0);
    t = min(t,sdCappedCylinder(q-vec3(1.0,-14.0,0.0),0.5,5.4));
    t = min(t,sdCappedCylinder(q-vec3(1.0,-18.0,0.0),1.0,0.2));
    t = min(t,sdBox(q-vec3(0.0,-18.0,0.0),vec3(1.0,0.2,1.0)));
    metal = smin(metal,sdBox(q-vec3(0.0,-6.5,0.0),vec3(1.0,0.5,1.0)),0.2);
    white = smin(white,sdBox(q-vec3(1.8,-10.5,0.0),vec3(0.4,1.0,0.6)),0.2); 
    //LEFT CYLINDER
    q = p - vec3(-7.4,-5.5,0.5);
    float tlc = smin(sdCappedCylinder(q-vec3(0.0,6.7,0.0),1.0,3.0)-0.5,
                     sdCappedCylinder(q-vec3(0.0,5.0,0.0),1.6,0.3),0.1);
    tlc = min(tlc, sdCappedCylinder(q.yzx-vec3(2.6,0.0,0.0),0.6,0.8));
    tlc = max(tlc,-sdCappedCylinder(q.yzx-vec3(2.6,0.0,0.0),0.5,0.9));
    metal = min(metal,sdCappedCylinder(q.yxz,0.9,0.7));
    float tcc = smin(sdCappedCylinder(q.yxz,1.5,0.5),
                     sdCappedCylinder(q.yxz-vec3(1.5,0.0,0.0),0.4,0.5),0.2); 
    tcc = smin(tcc,sdCappedCylinder(q.yxz-vec3(-0.8,0.0,-1.2),0.4,0.5),0.2);
    tcc = max(tcc,abs(q.x)-0.5);
    metal = min(metal,tcc);
    t = min(t,tlc);
    //RIGHT CYLINDER
    q = p - vec3(6.4,-5.5,0.5);
    float trc = sdCappedCylinder(q-vec3(0.0,6.0,0.0),0.2,2.0)-0.3;
    trc = max(trc,-sdTorus(q.xzy-vec3(0.0,0.0,5.0),vec2(0.5,0.1)));
    trc = max(trc,-sdTorus(q.xzy-vec3(0.0,0.0,7.0),vec2(0.5,0.1)));
    metal = min(metal,sdCappedCylinder(q-vec3(0.0,6.0,0.0),0.2,3.0));
    trc = min(trc,sdCappedCylinder(q-vec3(0.0,2.4,0.0),0.4,0.5));
    trc = max(trc,-sdBox(q-vec3(0.0,1.2,0.0),vec3(0.15,1.2,1.0)));
    metal = min(metal,sdBox(q-vec3(0.0,1.2,0.0),vec3(0.1,1.0,0.3)));
    metal = min(metal,sdCappedCylinder(q.yxz,1.2,0.06));
    metal = min(metal,sdCappedCylinder(q.yxz,1.0,0.3));
    //armature
    q.y -= 9.4;
    trc = min(trc,sdCappedCylinder(q.xzy,0.6,0.6));
    metal = min(metal,sdCappedCylinder(q.xzy,0.3,0.8));
    trc = min(trc,sdBox(q-vec3(-0.8,1.0,0.0),vec3(0.5,1.0,0.2)));
    q.xy *= rot(0.8);
    trc = min(trc,sdBox(q-vec3(0.0,0.0,0.0),vec3(0.2,1.2,0.2)));
    t = min(t,trc);
    //wires
    q = p - vec3(-9.0,-10.0,1.0);
    q.x += sin(q.y*0.3);
    q.z -= q.x*0.05;
    q.xy *= rot(-0.5);
    float tw = min(max(sdTorus(q,vec2(5.0,0.2)),q.y),
                   max(sdTorus(q-vec3(8.0,0.0,0.0),vec2(3.0,0.2)),-q.y));
    tw = min(tw,max(sdTorus(q-vec3(-10.0,0.0,0.0),vec2(5.0,0.2)),-q.y));
    tw = max(tw,q.x-7.0);
    tw = max(tw,-q.x-7.0);
    q = p - vec3(-9.0,-12.0,1.0);
    q.z -= q.x*-0.05;
    tw = min(tw,max(sdTorus(q,vec2(4.3,0.2)),q.y));
    tw = min(tw,max(sdTorus(q-vec3(6.3,0.0,0.0),vec2(2.0,0.2)),-q.y));    
    tw = min(tw,sdCapsule(q,vec3(-4.3,0.0,0.0),vec3(-4.3,8.0,0.0),0.2));
    tw = max(tw,q.x-8.0);
    //flesh
    q = p-vec3(0,1,3);
    float nz = n3D(vec3(p.x*8.1,p.y,p.z*9.17));
    nz *= smoothstep(1.0,2.0,abs(p.x))*smoothstep(4.0,2.0,abs(p.x))*
          smoothstep(4.0,-1.6,p.y)*smoothstep(-4.0,-1.6,p.y) *
          0.06;
    float tf = sdEllipsoid(q-vec3(0,0.6,0),vec3(2.6+nz,4.8+nz,1.6+nz));
    tf = smin(tf,sdEllipsoid(q-vec3(0,6,2.8),vec3(4.0,8.0,4.0)),1.0);
    q.x = abs(q.x);
    tf = smin(tf,sdEllipsoid(q-vec3(5.0,-2.0,3.3),vec3(4,20,4)),2.0);
    //cut
    nz = n3D(p)*0.3;
    float tfc = sdEllipsoid(q-vec3(0,0,-1),vec3(0.5,4.4,4.0));
    tfc = smin(tfc,sdEllipsoid(q-vec3(0,1.6,-0.4),vec3(0.4+nz,1.0+nz,4.2)),0.1);
    tfc = smin(tfc,sdEllipsoid(q-vec3(0,-0.7,-0.6),vec3(0.7+nz,2.9+nz,4.2)),0.1);
    tf = smax(tf,-tfc,0.2);
    //join
    float tfl = sdEllipsoid(q-vec3(0,0.6,-0.7),vec3(0.4,5.6,1.0));
    tfl = smin(tfl,sdEllipsoid(q-vec3(0,1.6,-0.6),vec3(0.4+nz,1.0+nz,1.2)),0.1);
    tfl = smin(tfl,sdEllipsoid(q-vec3(0,-1.1,-0.6),vec3(0.6+nz,2.4+nz,1.2)),0.1);
    tfl = smax(tfl,-sdEllipsoid(q-vec3(0,-1.8,-0.6),vec3(0.15+nz,3.2+nz,10.0)),0.2);
    tf = max(tf,abs(p.x)-5.0);
    tf = max(tf,abs(p.y-1.0)-7.0); 
    vec2 n = near(vec2(t,1.0),vec2(metal,2.0));
    n = near(n,vec2(tw,3.0));
    n = near(n,vec2(tf,4.0));
    n = near(n,vec2(tfl,5.0));
    return near(n,vec2(white,6.0));
}

vec3 normal(vec3 p) 
{  
    vec4 n = vec4(0.0);
    for (int i=ZERO; i<4; i++) 
    {
        vec4 s = vec4(p, 0.0);
        s[i] += EPS;
        n[i] = map(s.xyz).x;
    }
    return normalize(n.xyz-n.w);
}

//IQ - https://iquilezles.org/articles/raymarchingdf
float AO(vec3 p, vec3 n) 
{
    float ra = 0., w = 1., d = 0.;
    for (int i=ZERO; i<5; i++){
        d = float(i) / 5.;
        ra += w * (d - map(p + n*d).x);
        if (ra>1.) break;
        w *= .5;
    }
    return 1. - clamp(ra,0.,1.);
}

vec2 march(vec3 ro, vec3 rd) 
{
    float t = 0.0, id = 0.0;   
    for (int i=ZERO; i<100; i++)
    {
        vec2 ns = map(ro + rd*t);
        if (abs(ns.x)<EPS)
        {
            id = ns.y;
            break;
        }
        t += ns.x;
        if (t>FAR) 
        {
            t = -1.0;
            break;
        }
        
    }
    return vec2(t,id);
}

//IQ
//https://www.shadertoy.com/view/lsKcDD
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax, int technique )
{
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<64; i++ )
    {
		float h = map( ro + rd*t ).x;

        // traditional technique
        if( technique==0 )
        {
        	res = min( res, 10.0*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

            float y = h*h/(2.0*ph);
            float d = sqrt(h*h-y*y);
            res = min( res, 10.0*d/max(0.0,t-y) );
            ph = h;
        }
        
        t += h;
        
        if( res<0.0001 || t>tmax ) break;
        
    }
    res = clamp( res, 0.0, 1.0 );
    return res*res*(3.0-2.0*res);
}

vec3 camera(vec2 U, vec3 ro, vec3 la, float fl) 
{
    vec2 uv = (U - R*.5) / R.y;
    vec3 fwd = normalize(la-ro),
         rgt = normalize(vec3(fwd.z,0.0,-fwd.x));
    return normalize(fwd + fl*uv.x*rgt + fl*uv.y*cross(fwd, rgt));
}

void mainImage(out vec4 C, vec2 U)
{
    vec3 pc = vec3(0),
         la = vec3(0,-5.0,0),
         lp = vec3(10,45,-40),
         ro = vec3(0,-5.0,-22);
    
    vec3 rd = camera(U,ro,la,1.4);
    
    vec2 s = march(ro,rd);
    if (s.x>0.0)
    {
        vec3 p = ro + rd*s.x;
        vec3 n = normal(p);
        vec3 ld = normalize(lp - p);
        vec3 ld2 = normalize(vec3(10,45,-40)-p);
        float spg = 0.0;
        float spm = 0.0;
        
        vec3 sc = vec3(0.7);
        if (s.y==1.0)
        {
            spm = 0.2+n3D(1.7+p*0.7)*0.4;
            spg = 0.3;
            sc = mix(vec3(0.7,0.6,0.8),vec3(0.7),n3D(vec3(p.x,p.y*0.3,p.z)));
        }
        if (s.y==2.0)
        {
            spg = 1.0;
            sc = texture(iChannel1,reflect(rd,n)).xyz;
        }
        if (s.y==3.0)
        {
            spm = 0.3;
            sc = vec3(0.2);
        }
        if (s.y==4.0)
        {
            spg = smoothstep(3.0,0.0,abs(p.x));
            spm = 0.2;
            sc = mix(vec3(1.0,0.8,0.7),
                     vec3(1,0.3,0.3),
                     spg);
            sc = mix(sc,vec3(1.0,0.8,0.7),n3D(p*3.0));
            float nz = n3D(51.2+vec3(p.x*19.31+sin(p.y*0.9)*1.4,p.y,p.z*17.37));
            nz *= smoothstep(3.0,1.0,p.y)*smoothstep(3.0,0.0,abs(p.x));
            sc = mix(sc,vec3(0),min(1.0,nz*2.));
            sc *= max(0.0,dot(ld2,n));
        }
        if (s.y==5.0)
        {
            spg = 1.0;
            sc = mix(vec3(1,0.3,0.3),vec3(1,0.7,0.7),n3D(p*3.0));
            sc *= max(0.0,dot(ld2,n));
        }
        if (s.y==6.0)
        {
            spg = 1.0;
            sc = vec3(1);
        }
        float ao = AO(p,n);
        float specg = pow(max(dot(reflect(-ld,n),-rd),0.0),32.0);
        float specm = pow(max(dot(reflect(-ld,n),-rd),0.0),4.0);
        float sh = calcSoftshadow(p,ld,EPS,FAR,0);

        pc += sc * max(0.0,dot(ld,n));        
        //pc += vec3(0,0,0.02)*max(0.0,-n.y); 
        pc += vec3(1)*specg*spg;
        pc += vec3(1)*specm*spm;
        pc *= ao;
        pc *= sh;
        if (s.y==4.0||s.y==5.0)
        {
            //fix shadow
            float ct = length(p.xy-vec2(0.6,-1.3));
            pc *= smoothstep(0.01,0.0,ct-4.7);
        }
    }
    C = vec4(pc,1.0);
}