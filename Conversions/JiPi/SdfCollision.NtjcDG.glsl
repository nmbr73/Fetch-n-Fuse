

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2. * fragCoord - iResolution.xy) / iResolution.y;
    
    float w = length(fwidth(uv));
    
    float d = map(uv);
    float c = smoothstep(-w, w, abs(d));

    vec3 col = vec3(c);
    
    vec4 buf = texture(iChannel0, uv);
    vec2 bp = buf.xy;
    
    float ball = length(uv - bp) - BALL_R;
    ball = smoothstep(-w, w, abs(ball));
    
    col *= ball;
    
    fragColor = vec4(col, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float ia = BALL_INIT_ANGLE * 3.1415926536 / 180.;
const vec2 ip = BALL_INIT_POS;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    vec4 d = texture(iChannel0, uv);
    
    if (d.z == 0.) {
        d.zw = vec2(cos(ia), sin(ia)) * .5 + .5;
        d.xy = ip;
    } else {
        vec2 v = d.zw * 2. - 1.;
        vec2 p = d.xy;
        p += v * .0166 * MOVE_SPEED;
        
        float dist = map(p);
        if (dist > -BALL_R) {
            p -= (dist + BALL_R) * v;
            
            vec2 n = nrm(p);
            vec2 o = reflect(v, n);
            
            d.zw = o * .5 + .5;
        }
        
        d.xy = p;
    }
    
    fragColor = d;
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Header: Enable access to inputs from Common tab (Author: https://www.shadertoy.com/view/ttf3R4)
#if __LINE__ < 17
    #define _ST_TAB_COMMON
#endif
#ifdef _ST_TAB_COMMON
    uniform float iTime;
#endif
// End header

// ------------------------------------------------------------

#define BALL_R .05
#define BALL_INIT_POS vec2(0., .3)
#define BALL_INIT_ANGLE 45.

#define MOVE_SPEED 2.2

float box(vec2 p, vec2 s) {
    vec2 bd = abs(p) - s;
    return length(max(bd, 0.)) + min(max(bd.x, bd.y), 0.);
}

float star(vec2 p, float r, float rf) {
    const vec2 k1 = vec2(0.809016994375, -0.587785252292);
    const vec2 k2 = vec2(-k1.x, k1.y);
    p.x = abs(p.x);
    p -= 2.0 * max(dot(k1,p),0.0) * k1;
    p -= 2.0 * max(dot(k2,p),0.0) * k2;
    p.x = abs(p.x);
    p.y -= r;
    vec2 ba = rf * vec2(-k1.y, k1.x) - vec2(0, 1);
    float h = clamp( dot(p, ba) / dot(ba, ba), 0.0, r);
    return length(p - ba * h) * sign(p.y * ba.x - p.x * ba.y);
}

float moon(vec2 p, float d, float ra, float rb) {
    p.y = abs(p.y);
    float a = (ra * ra - rb * rb + d * d) / (2. * d);
    float b = sqrt(max(ra * ra - a * a, 0.));
    if (d * (p.x * b - p.y * a) > d * d * max(b - p.y, 0.))
        return length(p - vec2(a, b));
    return max((length(p) - ra), -(length(p - vec2(d, 0)) - rb));
}

float map(vec2 p) {
    float d = 0.;
    
    vec4 r = vec4(.0, .5, .2, .3);
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x  = (p.y > 0.0) ? r.x : r.y;
    vec2 q = abs(p) - vec2(1.6, .9) + r.x;
    d = min(max(q.x, q.y), 0.) + length(max(q, 0.)) - r.x;
    
    float c = length(p) - .2;
    
    float cs = cos(iTime), ss = sin(iTime);
    mat2 rot1 = mat2(cs, -ss, ss, cs);
    mat2 rot2 = mat2(cs, ss, -ss, cs);
    
    vec2 cp = abs(rot1 * (p - vec2(-1., .5)));
    c = min(c, length(cp - min(cp.x + cp.y, .25) * .5) - .025);
    
    c = min(c, box(rot1 * (p - vec2(-1., -.5)), vec2(.1, .2)));
    
    c = min(c, star(rot2 * (p - vec2(1., .5)), .18, .4));
    
    c = min(c, moon(rot2 * (p - vec2(1., -.5)), .05, .18, .14));
    
    d = max(-c, d);
    
    return d;
}

vec2 nrm(vec2 p) {
    vec2 eps = vec2(.001, .0);
	return normalize(vec2(
		map(p + eps.xy) - map(p-eps.xy),
		map(p + eps.yx) - map(p-eps.yx)
	));
}
