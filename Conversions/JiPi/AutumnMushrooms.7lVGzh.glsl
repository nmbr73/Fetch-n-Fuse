

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by sebastien durand - 11/2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// *****************************************************************************
// Based on
// iq - Quadratic Bezier - https://www.shadertoy.com/view/ldj3Wh
// *****************************************************************************

#define AA 1

#define ZERO (min(iFrame,0))

#define nose 2.
#define hat 3.
#define dress 4.
#define leg 5.
#define foot 6.
#define bear 7.
#define mushrom 9.

//-----------------------------------------------------------------------------------

vec3 getPtOnBez(vec3 p0, vec3 p1, vec3 p2, float t) {
    return (1. - t) * (1. - t) * p0 + 2. * (1. - t) * t * p1 + t * t * p2;
}

//-----------------------------------------------------------------------------------
// Mercury
float fOpUnionRound(float a, float b, float r) {
	return max(r, min (a, b)) - length(max(vec2(r - a,r - b), vec2(0)));
}

float pModPolar(inout vec2 p, float rep) {
	float an = 3.141592/rep,
         a = atan(p.y, p.x) + an,
         r = length(p),
         c = floor(.5*a/an);
	a = mod(a,2.*an) - an;
	p = vec2(cos(a), sin(a))*r;
	if (abs(c) >= rep*.5) c = abs(c);
	return c;
}

//-----------------------------------------------------------------------------------
// iq - https://www.shadertoy.com/view/ldj3Wh
vec2 sdBezier(in vec3 p,in vec3 b0,in vec3 b1,in vec3 b2 ) {
    b0 -= p; b1 -= p; b2 -= p;
    vec3 b01 = cross(b0,b1), b12 = cross(b1,b2), b20 = cross(b2,b0),
         n =  b01+b12+b20;
    float a = -dot(b20,n), b = -dot(b01,n), d = -dot(b12,n), m = -dot(n,n);
    vec3  g =  (d-b)*b1 + (b+a*.5)*b2 + (-d-a*.5)*b0;
    float t = clamp((a*.5+b-.5*(a*a*.25-b*d)*dot(g,b0-2.*b1+b2)/dot(g,g))/m, 0., 1.);
    return vec2(length(mix(mix(b0,b1,t), mix(b1,b2,t),t)),t);
}

vec2 sdCapsule(in vec3 p,in vec3 a,in vec3 b) {
  vec3 pa = p - a, ba = b - a;
  float h = clamp(dot(pa,ba)/dot(ba,ba), 0., 1.);
  return vec2(length(pa - ba*h), h);
}

float sdEllipsoid(in vec3 p,in vec3 r) {
  float k0 = length(p/r);
  return k0*(k0-1.)/length(p/(r*r));
}

//-----------------------------------------------------------------------------------
// Scene modeling
vec2 sdMush(in vec3 p) {
    float d = .5*(sdBezier(p, vec3(0,0,0), vec3(.4,3,0), vec3(.2,4.2,.3)).x-.1);
	d = fOpUnionRound(d,max(sdEllipsoid(p-vec3(.3,3.3,.3), vec3(1.2,.7,1.2)),
                    -sdEllipsoid(p-vec3(.27,3.,.25), vec3(1.3,.7,1.5))),.5);
	d = min(d,sdEllipsoid(p-vec3(-3.,.6,6.), vec3(.8,.6,.8)));
	d = min(d,sdEllipsoid(p-vec3(4.5,.5,2.5), vec3(.6,.5,.6)));
	d = min(d,sdEllipsoid(p-vec3(-6.5,.5,-8.5), vec3(.6,.5,.6)));
	return vec2(d, mushrom);
}

vec2 sdHand(vec3 p, vec3 p10, vec3 p11, vec3 n) {
    vec3 knee = .5*(p11+p10) + n,
         p2 = getPtOnBez(p10, knee, p11, .2),
         nn = normalize(p10-p2);
    // harm     
    vec2 h = sdBezier(p, p11, knee, p10);
    float d,
          dm = max(h.x - .1 - .05*h.y, -length(p-p10)+.2),
          hm=h.y;
    // fingers
    d = sdCapsule(p, p10+vec3(.03*sign(n.x),0,0), p2).x;
    p += nn*.05;
    d = min(d, sdCapsule(p, p10+vec3(0,.05,0), p2+vec3(0,.05,0)).x);
    d = min(d, sdCapsule(p, p10-vec3(-.02*sign(n.x),.05,0), p2-vec3(0,.05,0)).x);
    p += nn*.05;
    d = min(d, sdCapsule(p, p10-vec3(0,.1,0), p2-vec3(0,.1,0)).x);
    d -= .05;
    return d < dm ? vec2(d, nose) : vec2(dm, dress);
}

vec2 sdLeg(vec3 p, vec3 foot10, vec3 foot11, vec3 n) {
    vec3 knee = .5*(foot11+foot10) + n,
         p4 = getPtOnBez(foot10, knee, foot11, .2);
    vec2 h = sdBezier(p, foot11, knee, foot10);
    float d,dm = h.x - .1, 
          hm = leg + h.y;
    // foot
    h = sdCapsule(p, foot10, p4);
    d = max(h.x - .2, -length(p- mix(foot10, p4, 2.5)) + .4);
    if (d<dm) { dm=d; hm=dress;}
    p.y += .1; 
    h = sdCapsule(p, foot10, foot10 + n);
    d = h.x - .2;
    if (d<dm) { dm=d; hm=hat; }
    return vec2(dm,hm);
}

float invMix(float v0, float v1, float v) {
    return v1 == v0 ? 1. : (v-v0)/(v1-v0);
}

float getAmp(float frequency) { return texture(iChannel2, vec2(frequency / 512.0, 0)).x; }

#define BPM 127.
vec2 sdLutin(in vec3 p, in float lid) {
    float t = iChannelTime[2]*2.11666,
          a1 = (getAmp(lid*lid * 20.)*.5+.5)*cos(9.*t+1.57 * lid),
          anim = (getAmp(lid*lid * 40.)*.5+.5)*cos(6.*t+1.57 * lid),
          gg = .5*cos(lid*110.);
          
    vec3 head = vec3(0,2.5+gg,0),
         hips = vec3(0,1.2+gg,0);
   
    head += .2*vec3(.5,.5,.2)*(a1 + .5*anim);
    hips += .3*vec3(.5,.2,.2)*anim;   
    
    vec3 epaule = head - vec3(0,.7,0),
         c = head + vec3(0,.9,-.8),
         b = head + vec3(0,.65,-.3);

    float d, dm, hm = nose;
    
    // nez
    dm = sdEllipsoid(p- head - vec3(0,0,.5), vec3(.3,.15,.3));
    
    // bras
    float s = p.x>0.?1.:-1.;
    vec2 h = sdHand(p, epaule + vec3(s*1.,-.7+s*.3*anim- .3*gg,.5+ .4*gg), epaule+vec3(s*.4,0.,-.05), vec3(s*.2,-.2,-.2));
    if( h.x<dm ) { dm=h.x; hm = h.y; }
 
    // body
    vec3 pb = p;
    pb.z /= .7;
    pb.z -= .2*cos(p.y)*smoothstep(epaule.y,hips.y,invMix(epaule.y,hips.y, p.y));
    h = sdCapsule(pb, epaule+vec3(0,-.15,0), hips-vec3(0,.7,0));
    d = max(h.x - mix(.4,.6,h.y), -length(pb-hips+vec3(0,.8,0)) + .7);
    if (d<dm) { hm = dress; }
    dm = .7*fOpUnionRound(d, dm, .15);
    
    // legs  
    h = sdLeg(p, vec3(s*.5,-.7,0), hips + vec3(s*.25,-.2,0), vec3(s*.2,0,.3));
    if (h.x<dm) { dm=h.x; hm=h.y; }
    
     // bonet
    vec3 p3 = p + vec3(0,.1,-.1);
    h = sdBezier( p3, head-vec3(0,.05,0), b, c );
    d = .7*max( h.x - .5 + .5*h.y, -length(p3-(head-vec3(0,.8,-.6))) + 1.);
    if( d<dm ) { dm=d; hm=hat; }

    // barbe
    vec3 p4 = p;
    float k = mix(1.,3.,smoothstep(head.y, head.y-1., p4.y));
    p4.z*=k;
    h = sdBezier( p4, head, head - vec3(0,1.,.0), vec3(head.x, head.y, head.z*k) - vec3(-.2*anim,1,-3.));
    d = .7*fOpUnionRound(h.x - .3*sin(3.14*h.y), length(p-(head-vec3(0,.4,-.2)))-.5, .15);
    if (d<dm) { dm=d; hm=bear; }

	return vec2(dm*.9, 10.*lid + hm );
}

vec2 map(in vec3 p) {
	vec2 h2, h1 = sdMush(p-vec3(0,-1.05,0));
    float id = pModPolar(p.xz, 6.),
          d = sdEllipsoid(p-vec3(3,1.35,0), vec3(1.3,2.9,1.6));
    if (d>0.) h2 = vec2(d+.1,0);
    else      h2 = sdLutin((p - vec3(3,0,0)).zyx, id);
    return h1.x<h2.x ? h1 : h2;
}

//-------------------------------------------------------
// Ray marching
vec3 intersect( in vec3 ro, in vec3 rd ) {
    vec3 res = vec3(-1.);
    float maxd = 25.;
    // plane
    float tp = (-.85-ro.y)/rd.y;
    if (tp>0.) {
        res = vec3(tp,0,0);
        maxd = min(maxd,tp);
    }
    // Lutins
    float t = 2., l = 0.;
    for( int i=ZERO; i<92; i++ ) {
	    vec2 h = map(ro+rd*t);
        if (h.x<.004 || t>maxd) break;
        t += h.x;
		l = h.y;
    }
    return t<maxd ? vec3(t, l, 1.) : res;
}

vec3 calcNormal( in vec3 pos ) {
 // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0);
    for( int i=ZERO; i<4; i++) {
        vec3 e = .5773*(2.*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.);
        n += e*map(pos+.002*e).x;
    }
    return normalize(n);
}

float softshadow( in vec3 ro, in vec3 rd, float mint, float k ) {
    float res = 1., t = mint, h = 1.;
    for( int i=ZERO; i<48; i++ ) {
        h = map(ro + rd*t).x;
        res = min( res, k*h/t );
		t += clamp( h, .002, 2. );
        if( res<.001 ) break;
    }
    return clamp(res,0.,1.);
}

float map2( in vec3 pos ) {
    return min(pos.y+.85, map(pos).x);
}


float calcAO( in vec3 pos, in vec3 nor) {
    float h,d,ao = 0.;
    for( int i=ZERO; i<8; i++ ) {
        h = .02 + .5*float(i)/7.;
        d = map2( pos + h*nor );
        ao += h-d;
    }
    return clamp( 1.5 - ao*.6, 0., 1. );
}


//------------------------------------------------------------------------
// [Shane] - Desert Canyon - https://www.shadertoy.com/view/Xs33Df
//------------------------------------------------------------------------
// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D(sampler2D tex, in vec3 p, in vec3 n){
    n = max(n*n, .001);
    n /= (n.x + n.y + n.z );  
	return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
}

// Grey scale.
float grey(vec3 p){ return dot(p, vec3(.299, .587, .114)); }

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
vec3 doBumpMap( sampler2D tex, in vec3 p, in vec3 n, float k){
    const float ep = .001;
    vec3 grad = vec3( grey(tex3D(tex, vec3(p.x-ep, p.y, p.z), n)),
                      grey(tex3D(tex, vec3(p.x, p.y-ep, p.z), n)),
                      grey(tex3D(tex, vec3(p.x, p.y, p.z-ep), n)));
    grad = (grad - grey(tex3D(tex, p, n)))/ep;             
    grad -= n*dot(n, grad);          
    return normalize(n + grad*k);
}

// iq palette
vec3 pal(in float t) {
    return .5 + .5*cos(6.28318*(t+vec3(.0,.33,.67)) );
}


void mainImage( out vec4 fragColor, in vec2 fragCoord) {
    vec4 tot = vec4(0);
    
#if AA>1
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
        // pixel coordinates
        vec2 o = vec2(float(m),float(n)) / float(AA) - .5,
             p = (-iResolution.xy + 2.*(fragCoord+o))/iResolution.y;
#else    
        vec2 p = (-iResolution.xy + 2.*fragCoord)/iResolution.y;
#endif

        //-----------------------------------------------------
        // camera
        //-----------------------------------------------------
        float an = 2. + .3*iTime + 3.*smoothstep(.9,.95, sin(.32*iTime)) +  3.*smoothstep(.9,.95, sin(.06*iTime));

        vec3 ro = mix(1.75,1.2,smoothstep(-.9,-.8, cos(.15*iTime+.001*iTime*iTime)))*vec3(10.0*sin(an),5.0,10.0*cos(an)),
             ta = vec3(.02*an,0,0);

        // camera matrix
        float a = .1*cos(.1*iTime);
        vec3 ww = normalize(ta - ro),
             uu = normalize(cross(ww,normalize(vec3(sin(a),cos(a),0)))),
             vv = normalize(cross(uu,ww));

        // create view ray
        vec3 rd = normalize( p.x*uu + p.y*vv + 2.5*ww );

        //-----------------------------------------------------
        // render
        //-----------------------------------------------------

        vec3 lig = normalize(vec3(-.2,.6,.9));
        float sun = pow( clamp( dot(rd,lig), 0., 1. ), 8. );

        // raymarch
        vec3 tmat = intersect(ro,rd);

        // geometry
        vec3 nor, pos = ro + tmat.x*rd;
        if( tmat.z<.5)
             nor = vec3(0,1,0);
        else nor = calcNormal(pos);
        
        vec3 ref = reflect( rd, nor );

        // materials
        vec3 mate = vec3(.5);

        float lid = floor(tmat.y*.1);
        tmat.y = mod(tmat.y,10.);
        vec3 col, 
             col1 = pal(lid/6.),
             col2 = pal(lid/6. + .33);

        if (tmat.y < nose) {
            float k = texture(iChannel1, .051*pos.xz).x;
            mate = texture(iChannel1, .2*pos.xz).xyz;
            mate = .12*pow(mate,vec3(.3));
            mate = .5*mix(mate, vec3(.65,.5,.0), .2*smoothstep(.5,.6,k));
            nor = doBumpMap(iChannel1, .2*pos, nor, .02);
        } else if (tmat.y < hat) {
            mate = vec3(.68,.475,.446);
        } else if (tmat.y < dress) {
            mate = col2;
        } else if (tmat.y < leg) {
            mate = col1;
        } else if (tmat.y < bear) {
            mate = mix(col2, vec3(.2), smoothstep( -0.1, 0.1, cos( 40.0*tmat.y )));
        } else if (tmat.y < mushrom) {
            vec3 p2 = pos;
            float lid2 = pModPolar(p2.xz, 6.),
            a1 = cos(6.*iTime+1.57 * lid2),
            anim = cos(4.*iTime+1.57 * lid2);
            vec3 head = vec3(0,2.5,0) + vec3(0,.5*cos(lid2*110.),0),
            hips = vec3(0,1.2,0) + vec3(0,.5*cos(lid2*110.),0);
            head += .2*vec3(.5,.5,.2)*(a1 + .5*anim);
            nor = doBumpMap(iChannel0, 1.5*(pos-head)*vec3(1,.2,1), nor, .12);
            mate = lid2<0. ? vec3(211,110,76)/256. : vec3(1.);
        } else {
            vec3 p = pos - vec3(.2,4.2,.3);
            float r = length(p.xz);
            if (r<2.) {
                mate = mix(vec3(.7), .5*vec3(1,.5,1), smoothstep(.5,1.5,pos.y));
                nor = doBumpMap(iChannel0, vec3(.1*atan(p.x,p.z),.1*r,0), nor, .01);
            } else {
                mate = vec3(.7);
            }
            mate = 2.*mix(.25*vec3(1,.7,.6),mate,smoothstep(.2,.3,tex3D(iChannel1, .5*pos, nor).x));
        }

        float occ = calcAO(pos, nor);

        // lighting
        float sky = clamp(nor.y,0.,1.),
             bou = clamp(-nor.y,0.,1.),
             dif = max(dot(nor,lig),0.),
             bac = max(.3 + .7*dot(nor,-lig),0.),
             fre = pow( clamp( 1. + dot(nor,rd), 0., 1. ), 5.),
             spe = .5*max( 0., pow( clamp( dot(lig,reflect(rd,nor)), 0., 1.), 8.)),
             sha = 0.; 
        if (dif>.001) sha=softshadow(pos+.01*nor, lig, .0005, 32.);

        // lights
        vec3 brdf = 2.*dif*vec3(1.25,.9,.6)*sha;
        brdf += 1.5*sky*vec3(.1,.15,.35)*occ;
        brdf += bou*vec3(.3)*occ;
        brdf += bac*vec3(.3,.25,.2)*occ;
        brdf += fre*occ*dif;

        // surface-light interacion
        col = mate.xyz* brdf;
        col += (1.-mate.xyz)*spe*vec3(1,.95,.9)*sha*2.*(.2+.8*fre)*occ;

        // fog
        col = mix( col, 3.*vec3(.09,.13,.15), smoothstep(7.,30.,tmat.x) );
		col += .4*vec3(1,.68,.7)*sun;
        tot += vec4(col, tmat.x);
#if AA>1
    }
    tot /= float(AA*AA);
#endif

    // Gamma
	tot.xyz = pow(clamp(tot.xyz,0.,1.), vec3(0.5));
    // Vigneting
    vec2 q = fragCoord/iResolution.xy;
    tot.xyz *= pow(16.*q.x*q.y*(1.-q.x)*(1.-q.y), .32); 
    fragColor = vec4(tot.xyz,1.);
}
