

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define time iTime+1.2 //(iChannelTime[0]+1.2)

#define resolution iResolution

#define rot(a) mat2(cos(a),sin(a),-sin(a),cos(a))
#define sm(a,b) smoothstep(a,b,time)
#define hash(p) fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)

float st, det = .01, t, sc = 0., on = 0., tr = 1., mat = 0., y;
vec3 col = vec3(0), carpos, cardir, pal, glcol;
vec2 pf1, pf2, pf3, e = vec2(0, .001);


mat3 lookat(vec3 dir) {
    dir = normalize(dir);
    vec3 rt = normalize(vec3(-dir.z, 0, dir));
    return mat3(rt, cross(rt, dir), dir);
}


float is(float s) {
    return step(abs(sc - s), .1);

}

vec3 path(float tt) {
    vec3 p = vec3(sin(tt * .5 + cos(tt * .2)) + sin(tt * .1), 5., tt);
    tt += 70. * step(time, 80.);
    p.y -= smoothstep(290., 280., tt) * 10. + smoothstep(270., 265., tt) * 5. + smoothstep(240., 235., tt) * 5.;
    p.x *= .5 + tr * .5;
    p.x *= sm(57., 55.) + sm(89., 91.);
    return p;
}

vec3 carpath(float t) {
    vec3 p = path(t);
    p.y += 1. - tr + sm(52., 55.) * 4. * sm(82., 80.);
    p.x *= sm(55., 52.) + sm(105., 107.);
    p.z -= 375.;
    p.xz *= rot(-sm(75., 80.) * 3.1416);
    p.z += 375.;
    if (time > 89.) p = path(p.z);
    return p;
}

vec3 fractal(vec2 p)
{
    p = abs(fract(p * .05) - .5);
    float ot1 = 1000., ot2 = ot1;
    for (float i = 0.; i < 6.; i++) {
        p = abs(p) / clamp(abs(p.x * p.y), 0.15, 5.) - vec2(1.5, 1);
        ot1 = min(ot1, abs(p.x) + step(fract(time * .7 + float(i) * .2), .5 * p.y));
        ot2 = min(ot2, length(p));
    }
    ot1 = smoothstep(.1, .05, ot1);
    return time < 75. ? vec3(p, 0) * ot2 * ot1 * .3 + ot1 * .3 : vec3(p.x, -1, p.y * .5) * ot2 * ot1 * .3 + ot1 * .3; 
}


float map(vec2 p) {
    if (y > 10.) return 0.;
    vec2 ppp = p.yx; ppp.x -= 311.;
    vec3 pa = path(p.y);
    float h = 0.;
    p.x -= pa.x * sm(24., 25.);
    float d = floor(p.y * 3.) / 3. - carpos.z - sm(52., 57.) * 20.;
    p.x *= 1. + smoothstep(0., 2., d) * 2. * is(1.);
    pf1 = p;
    if (time < 24.) {
        p -= carpos.xz;
        p *= rot(-.5 * time / max(.5, floor(length(p))));
        pf2 = vec2((atan(p.x, p.y) / 3.1416), length(p));
        return pa.y - .5 - floor(length(p)) * sm(18., 17.) * (sm(5., 8.) - .5) * .7;
    }
    float b = step(300. + step(75., time) * 46., p.y);
    p = floor(p * 3.) / 3.;
    pf3 = p;
    h += hash(p) * 3. * sm(24., 26.) * (1. - b * .9) * sm(550., 500.);
    if (sc > 1.) p = floor(p), h += (clamp(hash(p + .1), .75, 1.) - .75) * 20.;
    if (time > 22. && b < .5) h *= smoothstep(0.5, 5. - d, abs(p.x) * 1.5) * (sc > 1. ? 2. : 1.); // barre
    h += pa.y - .5;
    return h;
}

float de(vec3 p) {
    p -= carpos;
    st = .1;
    float bound = length(p * vec3(1, 2, 1)) - 3. + tr;
    if (bound > 0.) return bound + 5.;
    p = lookat(cardir * vec3(.5, 0, 1)) * p;
    p.xy *= rot(sin(time * 1.5) * .2 + cardir.x);
    p.yz *= rot(t * 1.5 * step(.2, tr));
    p.xz *= rot(.5 * tr);
    float mat1 = exp(-.8 * length(sin(p * 6.)));
    float d1 = length(p) - .5;
    float d = 100.;
    p.xy *= rot(smoothstep(.3, .5, abs(p.x)) * sign(p.x) * .2);
    p.y *= 1.2 + smoothstep(.3, .4, abs(p.x));
    p.x *= 1. - min(.7, abs(p.z - .4));
    p.z += smoothstep(0., .6, p.x * p.x);
    p.z -= smoothstep(.1, .0, abs(p.x)) * .5 * min(p.z, 0.);
    d = length(p) - .5;
    d += abs(p.y) * smoothstep(.6, .3, abs(p.x));
    p.y += 5.;
    d = mix(d, d1, sqrt(tr));
    mat = mix(exp(-.8 * length(sin(p * 6.3))), mat1, tr) + step(abs(p.x), .03) * .1;
    mat *= min(1., on * 4.);
    if (d < 2.) st = .05;
    return d * .6;
}

vec4 hit(vec3 p) {
    float h = map(p.xz), d = de(p);
    return vec4(p.y < h, d < det * 2., h, d);
}

vec3 bsearch(vec3 p, vec3 dir) {
    float ste = st*-.5;
    for (float h2 = 1., i = h2; i++ < 21.;)
    {
        p += dir * ste;
        vec4 hi = hit(p);
        float h = max(hi.x, hi.y);
        if (abs(h - h2) > .001) {
            ste *= -.5;
            h2 = h;
        }
    }
    return p;
}

vec3 march(vec3 from, vec3 dir) {
    vec3 p, cl = vec3(0.), pr = p;
    float td = 2. + hash(dir.xy + time) * .1, g = 0., eg = 0., ref = 0.;
    p = from + td * dir;
    vec4 h;
    for (int i = 0; i < 300; i++) {
        p += dir * st;
        y = p.y;
        td += st;
        h = hit(p);
        if (h.y > .5 && ref == 0.) {
            pr = p;
            ref = 1.;
            p -= .2 * dir;
            for (int i = 0; i < 20; i++) {
                float d = de(p) * .5;
                p += d * dir;
                if (d < det) break;
            }
            dir = reflect(dir, normalize(vec3(de(p + e.yxx), de(p + e.xyx), de(p + e.xxy)) - de(p)));
            p += hash(dir.xy + time) * .1 * dir;
        }
        g = max(g, max(0., .2 - h.w) / .2) * mat;
        eg += .01 / (.1 + h * h * 20.).w * mat;
        if (h.x > .5 || td > 25. || (h.y > .5 && mat > .4)) break;
    }
    if (h.x > .9) {
        p -= dir * det;
        p = bsearch(p, dir);
        vec3 ldir = normalize(p - (carpos + vec3(0., 2., 0.)));
        vec3 n = normalize(vec3(map(p.xz + e.yx) - map(p.xz - e.yx), 2. * e.y, map(p.xz + e.xy) - map(p.xz - e.xy)));;
        n.y *= -1.;
        float cam = max(.2, dot(dir, n)) * step(on, .9 - is(3.)) * .8;
        cl = (max(cam * .3, dot(ldir, n)) * on + cam) * .8 * pal;
        float dl = length(p.xz - carpos.xz) * 1.3 * (1. - sm(52., 55.) * .5);
        cl *= min(.8, exp(-.15 * dl * dl));
        cl += (fractal(pf1) * sm(20., 22.) + fractal(pf2 * 5.) * sm(25., 23.) + fractal(pf3 * .2) * 2. * float(1. < sc) * -n.y + fractal(p.xy).g * n.z * 2. * is(2.) + .7 * step(abs(pf1.x), .3) * step(.7, fract(pf1.y * 4.)) * step(pf1.y, 292.)
            * step(1.5, sc)) * exp(-.3 * dl) * .7;
        mat = 0.;
    }
    else {
        cl = pal * ref * .3 + smoothstep(7., 0., length(p.xz)) * .13;
    }
    if (td > 25.) cl = fractal(p.xz * .2) * max(0.,dir.y);
    cl = mix(cl, vec3(ref), .2 * ref) + exp(-.3 * length(p + vec3(0, 17, -157))) * glcol * 5. * is(3.);
    p -= carpos;
    if (time > 80. && time < 89. && length(p) < 2.) cl += fractal(p.zx*2.);
    return cl + (g + eg) * glcol;
}

vec4 main2()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy - .5; uv.x *= 1.8;
    tr = sm(50., 48.) + sm(86.4, 89.);
    on = sm(14., 15.) * abs(sin(time * .7)) * .6 - fract(sin(time) * 10.) * step(20., time);
    if (time > 21.) on = 1.;
    if (time > 110.) on = step(time, 114.3) * abs(sin(time * 8.));
    pal = mix(vec3(.6, 1, .5) * .75, vec3(1, .5, .5), sm(74., 75.));
    glcol = mix(vec3(.5, 1, .5) * on * .8, vec3(1.5, .5, .5), sm(74., 75.));
    t = (max(21.2, time) - (time - 114.5) * sm(90., 116.5))*5.;
    vec3 from = carpath(t - 2.);
    vec3 cam = vec3(-4., 4., 2.);
    if (time < 28.) cam.xz *= rot(-max(8., time) * .7 + 2.5);
    if (time > 28.) from = carpath(t - 2.), cam = vec3(-3, 4, -2), sc = 1.;
    if (time > 35.) from = carpath(t + 4.), cam = vec3(0, 3, 0);
    if (time > 41.5) from = carpath(t - 3.), cam = vec3(1, 4. - tr * 2., 0);
    if (time > 52.) from = carpath(t + 5.), cam = vec3(sin(time) * 3., 4, 0);
    if (time > 55.) from = carpath(t + 5.), cam = mix(cam, vec3(-5, 6, -8), sm(55., 58.)), sc = 2.;
    if (abs(time - 67.) < 3.) from = path(68. * 5.), cam = vec3(4, 3, -0.5);
    if (time > 85.) sc = 3.;
    cam.z += sm(77., 78.) * 4.;
    cam.y *= .5 + .5 * sm(87., 85.);
    cam.x += sm(90., 92.) * 10.;
    cam = mix(cam, vec3(3, 2, 10), sm(105., 110.));
    from += cam;
    carpos = carpath(t);
    vec3 carpos2 = carpath(t + 1. * (1. - is(0.)));
    from = mix(from, carpos2 + cam * .8, sm(93., 95.));
    cardir = normalize(carpath(t + 1.) - carpos);
    return march(from, lookat(normalize(carpos2 - from)) * normalize(vec3(uv, 1.2 + sm(85., 86.) - sm(90., 91.)))).rgbr * sm(1., 3.) * sm(117.5, 115.5);
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = main2();
}