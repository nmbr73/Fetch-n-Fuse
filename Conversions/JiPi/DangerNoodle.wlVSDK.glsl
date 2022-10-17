

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// "Danger Noodle" by Martijn Steinrucken aka BigWings/CountFrolic - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// 
// Email: countfrolic@gmail.com
// Twitter: @The_ArtOfCode
// YouTube: youtube.com/TheArtOfCodeIsCool
//
// Ever since I did a snake scale effect as one of my first ShaderToys
// I have been wanting to do a snake, so here it is.
//
// Watch full screen with sound!

#define MAX_STEPS 200
#define MAX_DIST 60.
#define SURF_DIST .01

#define CAM_MOVE 1.

#define S smoothstep

#define MAT_TONGUE 1.
#define MAT_HEAD 2.
#define MAT_BODY 3.
#define MAT_EYE 4.

// From Dave Hoskins
vec2 Hash22(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

float Hash21(vec2 p) {
	p = fract(p*vec2(123.1031, 324.1030));
    p += dot(p, p+33.33);
    return fract(p.x*p.y);
}

float sabs(float x,float k) {
    float a = (.5/k)*x*x+k*.5;
    float b = abs(x);
    return b<k ? a : b;
}

vec2 RaySphere(vec3 ro, vec3 rd, vec4 s) {
	float t = dot(s.xyz-ro, rd);
    vec3 p = ro + rd * t;
    
    float y = length(s.xyz-p);
    
    vec2 o = vec2(MAX_DIST,MAX_DIST);
    
    if(y<s.w) {
    	float x = sqrt(s.w*s.w-y*y);
        o.x = t-x;
        o.y = t+x;
    }
    
    return o;
}

// From IQ
float smin( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0., 1. );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float smax(float a, float b, float k) {
	return smin(a, b, -k);
}

mat2 Rot(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}

float sdGyroid(vec3 p, float scale, float thickness, float bias) {
	p *= scale;
    return abs(dot(sin(p), cos(p.zxy))+bias)/scale - thickness;
}

float sdSph(vec3 p, vec3 pos, vec3 squash, float r) {
    squash = 1./squash;
	p = (p-pos)*squash;
    return (length(p)-r)/max(squash.x, max(squash.y, squash.z));
}


vec4 Scales(vec2 uv, float overlap, float skew, float point, float blur) {
    
    vec2 gv = fract(uv*5.)-.5;
    vec2 id = floor(uv*5.);
    
    float m = 0.;
    
    gv.y = sabs(gv.y,point);
    
    float w = .5+overlap;
    vec2 p1 = (gv+vec2(overlap,-gv.x*skew))*vec2(1,1.8);
    float a1 = atan(p1.x-w, p1.y);
    
    float waveAmp = .02;
    float waves = 10.;
    float w1 = sin(a1*waves);
    float s1 = S(w, w*blur, length(p1)+w1*waveAmp);
    s1 +=  w1*.1*s1;
    s1 *= mix(1., .5-gv.x, overlap*2.);
    
    gv.x -= 1.;
    vec2 p2 = (gv+vec2(overlap,-gv.x*skew))*vec2(1,1.8);
    float a2 = atan(p2.x-w, p2.y);
    float w2 = sin(a2*waves);
    float s2 = S(w, w*blur, length(p2)+w2*waveAmp);
    s2 += w2*.1*s2;
    
    s2 *= mix(1., .5-gv.x, overlap*2.);
    
    if(s1>s2) {
    	m += s1;
        m -= dot(p1,p1);
    } else {
        m += s2;
        m -= dot(p2,p2);
        id.x += 1.;
    }

    return vec4(1.-m, 0., id);
}

vec4 ScaleTex(vec2 uv, float overlap, float skew, float point, float blur) {

    uv *= 2.;
    vec4 s1 = Scales(uv, overlap, skew, point, blur);
    vec4 s2 = Scales(uv+.1, overlap, skew, point, blur);
    s2.zw -= .5;
    
    return s1.x<s2.x ? s1 : s2;
}


vec3 sdBody(vec3 p) {
    float t = iTime*.3;
    float neckFade = S(3., 10., p.z);
   
    p.x += sin(p.z*.15-t)*neckFade*4.;
    p.y += sin(p.z*.1-t)*neckFade;
    
    vec2 st = vec2(atan(p.x, p.y), p.z);
    
    float body = length(p.xy)-(.86+S(2., 15., p.z)*.6-p.z*.01);
    body = max(.8-p.z, body);   
    
    vec4 scales = vec4(0);
    if(body<.1) {
        vec2 uv = vec2(-st.y*.25, st.x/6.2832+.5);
        float a = sin(st.x+1.57)*.5+.5;
        float fade = a;
        a = S(.1, .4, a);

        uv.y = 1.-abs(uv.y*2.-1.);
        uv.y *= (uv.y-.2)*.4;
        scales = ScaleTex(uv*1.3, .3*a, .3*a, .01, .8);
        body += scales.x*.02*(fade+.2);
    }
    
    body += S(-.4, -.9, p.y)*.2;	// flatten bottom
    return vec3(body, scales.zw);
}

float GetHeadScales(vec3 p, vec3 eye, vec3 mouth, float md) {    
    float t = iTime;
  
    float jitter = .5;
    jitter *= S(.1, .3, abs(md));
    jitter *= S(1.2, .5, p.z);
    
    p.z += .5;
    p.z *= .5;
    
    p.yz *= Rot(.6);
    float y = atan(p.y, p.x);
    vec2 gv = vec2(p.z*5., y*3.);

    vec2 id = floor(gv);
    
    gv = fract(gv)-.5;
    
    float d=MAX_DIST;
    for(float y=-1.; y<=1.; y++) {
        for(float x=-1.; x<=1.; x++) {
            vec2 offs = vec2(x, y);

            vec2 n = Hash22(id+offs);
            vec2 p = offs+sin(n*6.2831)*jitter;
            p -= gv;
            
            float cd = dot(p,p);
            if(cd<d) d = cd;
        }
    }
    
    d += sin(d*20.)*.02;    
    d *= S(.0, .5, length(p.xy)-.1);
    return d*.06;
}

float sdHead(vec3 p) {    
    p.x = abs(p.x*.9);
    float d = sdSph(p, vec3(0,-.05,.154), vec3(1,1,1.986),1.14); 
    d = smax(d, length(p-vec3(0,7.89,.38))-8.7, .2);
    d = smax(d, length(p-vec3(0,-7.71,1.37))-8.7, .15); // top
    
    d = smax(d, 8.85-length(p-vec3(9.16,-1.0,-3.51)), .2);	// cheeks
    
    vec3 ep = p-vec3(.54,.265,-.82);
    float eye = length(ep)-.35;
    float brows = S(.1, .8, p.y-(p.z+.9)*.5);
    brows *= brows*brows;
    brows *= S(.3, -.2, eye);
   	d -= brows*.5;
    d += S(.1, -.2, eye)*.1;
    
    vec2 mp = p.yz-vec2(3.76+S(-.71, -.14, p.z)*(p.z+.5)*.2, -.71); 
    float mouth = length(mp)-4.24;
    d += S(.03,.0,abs(mouth))*S(.59,.0, p.z)*.03;
    
   	d += GetHeadScales(p, ep, mp.xyy, mouth);
    
    d = min(d, eye);
    
    float nostril = length(p.zy-vec2(-1.9-p.x*p.x, .15))-.05;
    d = smax(d, -nostril,.05);
    return d;
}

float sdTongue(vec3 p) {
	float t = iTime*3.;
   
    float inOut = S(.7, .8, sin(t*.5));
    
    if(p.z>-2.||inOut==0.) return MAX_DIST;		// early out
    
    float zigzag = (abs(fract(t*2.)-.5)-.25)*4.; // flicker
    float tl = 2.5;	// length
    
    p+=vec3(0,0.27,2);
    p.z *= -1.;
    float z = p.z;
    p.yz *= Rot(z*.4*zigzag);
    p.z -= inOut*tl;
    
    float width = S(0., -1., p.z);
    float fork = 1.-width;
    
    float r = mix(.05, .02, fork);
	
    p.x = sabs(p.x, .05*width*width);
    p.x -= r+.01;
    p.x -= fork*.2*inOut;

    return length(p-vec3(0,0,clamp(p.z, -tl, 0.)))-r;
}

float GetDist(vec3 P) {
    
    vec3 p = P;
    p.xz *= Rot(sin(iTime*.3)*.1*S(1., 0., p.z));
    float d = sdTongue(p)*.7;
    d = min(d, sdHead(p));
    d = smin(d, sdBody(P).x, .13);
    
    return d;
}

vec3 GetMat(vec3 p) {    
    float d = MAX_DIST;
    
    float tongue = sdTongue(p)*.7;
    float head = sdHead(p);
    vec3 body = sdBody(p);
    
    float closest = min(tongue, min(head, body.x));
    if(closest == tongue) {
        return vec3(MAT_TONGUE, 0, 0);
    } else if(closest==head) {
        p.x = abs(p.x*.9);
        vec3 ep = p-vec3(.54,.265,-.82);
        float eye = length(ep)-.35;
        if(eye<SURF_DIST)
        	return vec3(MAT_EYE, ep.yz);
        else
            return vec3(MAT_BODY, 0, 0);
            
    }else if(closest==body.x) {
        return vec3(MAT_BODY, body.yz);
    }
}


float RayMarch(vec3 ro, vec3 rd) {
	float dO=0.;
    
    for(int i=0; i<MAX_STEPS; i++) {
    	vec3 p = ro + rd*dO;
        float dS = GetDist(p);
        dO += dS;
        if(dO>MAX_DIST || abs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

// From Nimitz
vec4 GetNormalAndCurvature(in vec3 p, float eps) {
    vec2 e = vec2(-1., 1.)*eps;   
    float t1 = GetDist(p + e.yxx), t2 = GetDist(p + e.xxy);
    float t3 = GetDist(p + e.xyx), t4 = GetDist(p + e.yyy);

    float c = .25/e.y*(t1 + t2 + t3 + t4 - 4.0*GetDist(p));
    vec3 n = normalize(e.yxx*t1 + e.xxy*t2 + e.xyx*t3 + e.yyy*t4);
    
    return vec4(n, c);
}

vec3 GetRayDir(vec2 uv, vec3 p, vec3 l, float z) {
    vec3 f = normalize(l-p),
        r = normalize(cross(vec3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}


vec4 Material(vec3 ro, vec3 rd, float d) {
    vec3 p = ro + rd * d;
    vec4 n = GetNormalAndCurvature(p, mix(.01, .03, S(8., 20., d)));

    p.xz *= Rot(sin(iTime*.3)*.1*S(1., 0., p.z));
    vec3 mat = GetMat(p);
    
    vec3 col = vec3(n.y*.5+.5);  	// diffuse
	col *= 1.-max(0., .3-n.w);		// curvature shadow
    
    vec3 h = normalize(-rd + vec3(1,1,1));
    float spe = pow(clamp(dot(h, n.xyz), 0.0, 1.0), 32.0);
	
    vec3 ref = reflect(rd, n.xyz);
    vec3 r = texture(iChannel0, ref).rgb;
    
    if(mat.x==MAT_EYE) {
        vec2 sph = RaySphere(
            vec3(abs(p.x*.9),p.yz), 
            vec3(-abs(rd.x), rd.yz), 
            vec4(.3,.265,-.82, .52)
        );

        vec3 sp = p+rd*sph.x;
        mat.yz = sp.yz-vec2(.265,-.82)+.05;

        float t = iTime*.2;
        vec2 p1 = sin(floor(t)*vec2(20., 31.));
        vec2 p2 = sin(floor(t+1.)*vec2(20., 31.));
        p1 = mix(p1, p2, S(.45, .5, fract(t)));
        mat.yz += p1*vec2(.01, .03)*1.;
        float a = atan(mat.y, mat.z);

        float d = abs(mat.z)+mat.y*mat.y;
        col *= vec3(1,1,.1);
        col += S(.1, .0, length(mat.yz*vec2(1,2))-.1)*.1;
        
        float z = S(.7, 1., rd.z*rd.z)*.05;
        col *= S(.02-z, .03+z, d);
        
        vec3 gp = vec3(a, mat.yz)*20.;
        float gyroid = (abs(dot(sin(gp), cos(gp.zxy))));
        col *= 1.+gyroid*.1;
        
        col += r*r*r*.3;
        col += pow(spe, 6.);
    } else if(mat.x==MAT_BODY) {
        float x = mat.y;
        float y = mat.z;
        float wave = S(2., 0., abs(y-2.+sin(x*.5)*1.));
        wave *= S(2., 3., p.z);
        
        float t = iTime*.3;
        float neckFade = S(3., 10., p.z);
        p.y += sin(p.z*.1-t)*neckFade;
        
        vec3 baseCol = mix(vec3(1., 1., .2), vec3(.3, .8, .1), S(-.55, -.1, p.y));
        col *= mix(baseCol, vec3(.2,.4,.2)*.5, wave);
        col += spe*pow(1.-abs(n.w), 5.)*.3;
        
        r = texture(iChannel1, ref).rgb;
        col += r*r*.05;
    } else if(mat.x==MAT_TONGUE) {
    	col *= vec3(.4, .1, .2);
        col += pow(min(1., spe*5.), 5.);
    }
    
    return vec4(col, 1);
}

vec3 Render(vec2 uv, vec2 m, float t) {
    vec3 ro = vec3(0, 0, -3)*(8.+sin(t*.2)*2.*CAM_MOVE);
    ro.yz *= Rot(-m.y*3.14+sin(t*.03)*CAM_MOVE*.2);
    ro.xz *= Rot(-m.x*6.2831*2.+sin(t*.05)*CAM_MOVE);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0,0,sin(t*.11)), 6.);
    
    float d = RayMarch(ro, rd);
    
    vec3 col = vec3(0);
    
    vec2 env = RaySphere(ro, rd, vec4(0,0,0,20));
    
    if(d<MAX_DIST) {
        vec4 snake = Material(ro, rd, d);
    	snake.rgb *= S(60., 10., d);
        col = mix(col, snake.rgb, snake.a);
    } else {
    	col = (rd.y*.5+.5)*vec3(.4, 1.,.2);
        col *= texture(iChannel0, rd).rgb;
        col *= 1.-S(.8, 1., rd.z);
        
        if(env.y>0.)	// vines behind
            col *= S(0., 1.1, sdGyroid(ro + env.y*rd, .4, .1, .0))*.5+.5;
    }
    
    if(env.x>0.)	// vines in front
        col *= S(0., .25, sdGyroid(ro + env.x*rd, .25, .1, .0));
    
    return col;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
	vec2 m = (iMouse.xy-.5*iResolution.xy)/iResolution.xy;    
    if(m.x<-.49 && m.y<-.49) m*=0.;
    
    vec3 col = Render(uv, m, iTime);
    
    col *= 1.5;						// exposure adjustment
    col = pow(col, vec3(.4545));	// gamma correction
    col *= 1.-dot(uv,uv)*.3;		// vignette

    fragColor = vec4(col,1.0);
}