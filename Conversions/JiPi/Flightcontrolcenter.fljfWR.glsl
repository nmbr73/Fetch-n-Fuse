

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{



    fragColor = IC0(ivec2(fragCoord));

// Show tracked particles / voro cells
#if 0
    fragColor *= .5;
    
    ivec2 top0 = unhashXY(int(IC2(ivec2(fragCoord)).x),IR);
    ivec2 top1 = unhashXY(int(IC2(ivec2(fragCoord)).y),IR);
    ivec2 top2 = unhashXY(int(IC2(ivec2(fragCoord)).z),IR);
    ivec2 top3 = unhashXY(int(IC2(ivec2(fragCoord)).w),IR);
    //fragColor.rg = vec2(top) / R;
    float d = 320.;
    fragColor.r += float(top0.x) / d;
    fragColor.g += float(top1.x) / d;
    fragColor.b += float(top2.x) / d;
    fragColor.a = 1.;
#endif
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<


#define R iResolution.xy
#define IR ivec2(iResolution.xy)
#define C0(uv) texture(iChannel0, (uv))
#define C1(uv) texture(iChannel1, (uv))
#define IC0(p) texelFetch(iChannel0, (p), 0)
#define IC1(p) texelFetch(iChannel1, (p), 0)
#define IC2(p) texelFetch(iChannel2, (p), 0)


// Version 1: First try, encoded each particle position at that time
// Version 2: encode prev+next site as integer, as well as phase.
// Version 3: same, but use particle tracking/search instead of exhaustive loop each frame.

// In version 2: Looping over >100 particles resulted in under 30fps on my laptop,
// so for version 3, use particle tracking.

//#define VERSION 1
#define VERSION 2
#define SIZE 8
#define SIZEf 8.
// How many particles to search for each frame. Higher = faster convergence but slower.
#define NEW_SAMPLES 8

float rand11(float x) { return fract(337.1*sin(x*22.13+11.2)); }
vec2 rand22(vec2 p) { return fract(vec2(
    337.1*sin(22.3+27.31*p.y+p.x*22.13),
    437.1*sin(1.7+22.31*p.y+p.x*29.13) )); }
vec3 rand23(vec2 p) { return fract(vec3(
    337.1*sin(22.3+27.31*p.y+p.x*22.13),
    437.1*sin(1.7+22.31*p.y+p.x*29.13),
    407.1*sin(111.7+12.31*p.y+p.x*19.13) )); }
vec3 rand33(vec3 p) { return fract(vec3(
    337.1*sin(22.3+27.31*p.y+p.x*22.13+p.z*19.2),
    437.1*sin(1.7+22.31*p.y+p.x*29.13+p.z*10.3),
    407.1*sin(111.7+12.31*p.y+p.x*19.13+p.z*8.2) )); }

float distanceToLine(vec3 p1, vec3 p2, vec3 q) {
    return length(cross(q-p1, q-p2)) / length(p1-p2);
}

const float CAM_DIST = 4.;
const float FOCAL_COEFF = 1.60;


float xsect_sphere(vec3 ro, vec3 rd, float radius) {
    float a = dot(rd,rd);
    float b = 2. * dot(rd,ro);
    float c = dot(ro,ro) - radius;
    float discrim = b*b - 4.*a*c;
    if (discrim < 0.) return 0.;
    float t1 = -b + sqrt(b*b - 4.*a*c);
    float t2 = -b - sqrt(b*b - 4.*a*c);
    return min(t1,t2) / 2.*a;
}

vec2 randVor22(vec2 g, float iTime) {
//g += iTime * .00001;
//float a = sin(iTime)*.5+.5;
const float a = 1.0;
return rand22(g) * a + .5 - .5 * a;
}

vec3 site(vec2 pp, float t) {
    vec2 g = floor(pp);
    vec2 a = (g+randVor22(g, t)).xy * 3.141 / SIZEf * .5;
    return normalize(vec3(sin(a.y)*cos(a.x), sin(a.y)*sin(a.x), cos(a.y))).xzy;
}
vec3 isite(int i, float t) {
    vec2 p = vec2(i % SIZE, i / SIZE);
    vec2 pp = vec2(p.x,p.y) * 2. - SIZEf * .5;
    return site(pp, t);
}
vec3 partAtPhase(vec4 samp, float t) {
    vec3 a = isite(int(samp.x+.1), t);
    vec3 b = isite(int(samp.y+.1), t);
    float phase = samp.z;
    float amp = fract(samp.w*9999.77) * .2 + .1;
    vec3 part = normalize((1.-phase)*a + phase*b) * (1.0 + amp*(1.0-pow(cos(phase*3.141),2.)));
    return part;
}

vec2 proj(vec3 p, vec2 focal, float tt) {
    return p.xy * focal / (p.z + tt);
}



float hashXY(ivec2 xy, ivec2 ir) {
    return float(xy.y * ir.x + xy.x);
}
ivec2 unhashXY(int h, ivec2 ir) {
    return ivec2(h%ir.x, h/ir.x);
}



    
//float smoo(float v) { return v*v*(3.-2.*v); }
float smoo(float v) { return v; }
vec3 noise33(vec3 x) {
    vec3 o = vec3(0.);
    vec3 fx = floor(x);
    vec3 lx = fract(x);
    for (int i=0; i<8; i++) {
        float dx = (i  ) % 2 == 0 ? 1. : 0.;
        float dy = (i/2) % 2 == 0 ? 1. : 0.;
        float dz = (i/4) % 2 == 0 ? 1. : 0.;
        vec3 v  = rand33(fx + vec3(dx,dy,dz));
        vec3 d = vec3(dx,dy,dz) * 2. * lx + 1. - lx - vec3(dx,dy,dz);
        o += smoo(d.x)*smoo(d.y)*smoo(d.z)*v;
    }
    return o;
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Particle Sim


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    if (iFrame == 0) { fragColor = vec4(0.); return; }
    
    vec4 old = IC0(ivec2(fragCoord));
    vec4 new = old;
    
    float TS = (fragCoord.x * 2. / R.x + 2.) / 24.;
    float AMP = rand11(fragCoord.x * .97) * .2 + .1;
    
    /*
    if (new.a == 0. || new.a >= 1.0) {
        // Random init
        new = vec4(rand33(vec3(fragCoord+iTime*.3336, iTime+float(iFrame)))*2.-1., 0.00001);
        new.rgb = normalize(new.rgb);
    } else {
        // Move particle along path
        new.a += (fragCoord.x * 2. / R.x + 1.) / 60.;
        
        vec3 a = normalize(rand33(vec3(fragCoord+iTime*.3336, iTime+float(iFrame)))*2.-1.);
        vec3 b = normalize(rand33(vec3(fragCoord+iTime*.3336, iTime+float(iFrame)))*2.-1.);
    }
    */
    
    #if VERSION == 1
    float period = floor(iTime * TS);
    float phase  = fract(iTime * TS);
    
    vec3 a = normalize(rand33(vec3(fragCoord,0.)+vec3(period))*2.-1.);
    vec3 b = normalize(rand33(vec3(fragCoord,0.)+vec3(period+1.))*2.-1.);
    new.rgb = normalize((1.-phase)*a + phase*b) * (1.0 + AMP*(1.0-pow(cos(phase*3.141),2.)));
    
    #elif VERSION == 2 || VERSION == 3
    
    // row 0: i | j | phase | timeScale
    // i,j encode xy integer location.
    // Which is then further transformed by the site function.
    
    float S = 2.0 * pow(SIZEf,2.) + .99;
    if (iFrame <= 1) {
        new.z = fragCoord.x/R.x;
        new.y = floor(rand11(fragCoord.x + iTime*9.333 + 11.9) * S );
        new.x = floor(rand11(fragCoord.x + iTime*9.333) * S );
        new.w = ((float(fragCoord.x) / R.x) + 1.) / 320.;
    } else if (new.z > 1.0) {
        new.z = 0.;
        new.x = new.y;
        int i = int(old.y + .1);
        vec2 p = vec2(i % SIZE, i / SIZE) * 2. - 1.;
        vec2 pp = p + (rand22(vec2(fragCoord.x,fragCoord.x*2.3)+iTime) * 2. - 1.) * 2.;
        new.y = floor(rand11(fragCoord.x + iTime*7.333) * S);
        //new.y = new.x + 1.0;
        new.w = ((float(fragCoord.x) / R.x) + 1.) / 320.;
    } else new.z += ((float(fragCoord.x) / R.x) + 1.) / 320.;
    
    #endif
    
    //new.xy = vec2(floor(fragCoord.x/200.));
    
    fragColor = new;
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Main scene + time averaging



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec3 col = vec3(.003,.001,.02);
    float tt = iTime / 33.0;
    //tt = 0.;
    
    vec3 scol = vec3(0.);
    
    mat3 RR = mat3(1.);
    RR[0] = vec3(cos(tt), 0., sin(tt));
    RR[2] = vec3(sin(tt), 0., -cos(tt));
    
    vec2 fuv = vec2(fragCoord-R.xy*.5)/R.y;
    /*
    vec3 ro = CAM_DIST * vec3(sin(tt), 0., -cos(tt));
    vec3 rd_ = normalize(vec3((fragCoord-R.xy*.5)/R.y, FOCAL_COEFF));
    vec3 rd = vec3(rd_.x*cos(tt)-sin(tt)*rd_.z, rd_.y,
                   rd_.x*sin(tt)+rd_.z*cos(tt));
                   */
    vec3 ro = CAM_DIST * RR * vec3(0.,0.,-1.);
    vec3 rd = transpose(RR) * normalize(vec3((fragCoord-R.xy*.5)/R.y, FOCAL_COEFF));
                   
    // Intersect sphere
    float t = xsect_sphere(ro,rd, 1.);
    vec3 p = (ro + t * rd).xyz;
    if (t != 0.) {
        scol = vec3(.04,.11,.21) * (1.+length(fuv)) * 4.;
        scol.g *= .8 + .3 * sin(p.y*3.+iTime);
        scol.r *= .5 + 1.5 * sin(p.x*5.+iTime);
        
        float lat = acos(p.y/length(p));
        float lng = atan(p.z,p.x);
        
        // Do quad pattern
        float y = lat * SIZEf / 3.141 * 2.;
        float x = lng * SIZEf / 3.141 * 2.;
        vec2 pp = vec2(x,y);
        vec2 g = floor(pp);
        vec2 a = g+randVor22(g,iTime);
        float acc = 0.;
        for (int i=0; i<4; i++) {
            vec2 dd = vec2((i==0)?-1.:(i==1)?1.0:0.,(i==2)?-1.:(i==3)?1.0:0.);
            vec2 aa = g+dd + randVor22(g+dd,iTime);
            float d = distanceToLine(vec3(a,1.), vec3(aa,1.), vec3(pp,1.));
            if (dot(a-pp,aa-pp) < 0.)
                acc = max(acc, .09 / (.1+d));
        }
        //if (acc < 1.) scol *= 0.;
        scol *= exp(acc*5.-5.);
        
        //cc *= (1.0 - abs(sin(lng*30.)) * abs(sin(lat*30.))) * 2.1;
        col += scol;
    }
    
    
    // Render particles
    #if VERSION == 1
    vec3 cc = vec3(0.);
    float dampen = 0.;
    int N = IR.x / 4;
    for (int i=0; i<N; i++) {
        vec3 c = vec3(0.);
        vec4 part4 = IC0(ivec2(i,0));
        vec3 part = RR * part4.xyz;
        vec2 ppart = FOCAL_COEFF * part.xy / (part.z + CAM_DIST);
        float d = length(ppart - fuv);
        
        dampen += .005 / (.0001+d);
        if (d > .01) d = 999.;
        c.g += .002/(d+.0002);
        if (d > .01) d = 999.;
        c.b += .004/(d+.0002);
        float AMP = float(i) / float(N);
        cc += c * AMP;
        
        if (t != 0.) {
            float sd = sqrt(pow(clamp(length(p-part4.xyz) - .03,0.,999.),2.));
            vec3 c = (scol) * 15.0;
            col += exp(-sd*100.) * c;
        }
    }
    //col *= 1./dampen;
    col += cc;


    vec4 old = IC1(ivec2(fragCoord));
    float ALPHA = .2;
    col = col * ALPHA + (1.-ALPHA) * old.rgb;
    fragColor = vec4(col,1.0);
    
    #elif VERSION == 2
    
    vec3 cc = vec3(0.);
    float dampen = 0.;
    /*
    int N = IR.x / 10;
    //N = 32;
    //N=64;
    for (int i=0; i<N; i++) {
        float AMP = .5 * float(i) / float(N);
        vec4 samp = IC0(ivec2(i,0));
    */
    ivec4 pidxs = ivec4(IC2(ivec2(fragCoord)));
    for (int i=0; i<4; i++) {
        float AMP = .5;
        
        //ivec2 pidx = ivec2(IC2(unhashXY(int(pidxs[i]),IR)));
        ivec2 pidx = unhashXY(pidxs[i],IR);
        //pidx = ivec2(1,0);
        //pidx.x = 0;
        vec4 samp = IC0(pidx);

        
        vec3 c = vec3(0.);
        
        vec3 part_ = partAtPhase(samp, iTime);
        vec3 part = RR * part_;
        //vec2 ppart = FOCAL_COEFF * part.xy / (part.z + CAM_DIST);
        vec2 ppart = proj(part,vec2(FOCAL_COEFF),CAM_DIST);
        
        
        // version a: use 2d projected distance to shade particle
        if (false) {
            float d = length(ppart - fuv);
            float far_mult = 1.-smoothstep(CAM_DIST-1., CAM_DIST+.5, part.z + CAM_DIST);
            //c.b += .001/(d+.00001);
            c.b += exp(-clamp(d,.001,999.)*200.) * far_mult;
            c.g += exp(-clamp(d,.002,999.)*300.) * far_mult;        
            cc += c;
        }
        
        // version b: use 2d distance to line to shade particle.
        //            line direction comes from instantaneous velocity, after projecting
        else {
            float phase = samp.z;
            float phasePrev = phase - samp.w * 1.;
            samp.z = phasePrev;
            vec3 prevPart = RR * partAtPhase(samp, iTime);

            //vec2 prevPpart = FOCAL_COEFF * prevPart.xy / (prevPart.z + CAM_DIST);
            vec2 prevPpart = proj(prevPart,vec2(FOCAL_COEFF),CAM_DIST);
            //vec3 line = cross(vec3(ppart,1.), vec3(prevPpart,1.));
            float ld = distanceToLine(vec3(ppart,1.), vec3(prevPpart,1.), vec3(fuv,1.));
            float d = length(ppart-fuv);
            float dd = ld * (1.0 / (.001+d));

            float lineLength = .5 - abs(phase-.5) + .1; // goes .1 -> .6 -> .1
            // There's two factors: one for line distance and one for radial distance.
            float cs = 2.* exp(-ld*600.) * exp(-d*25. / lineLength); 
            c = vec3(.4 - fract(samp.w*777.773)*.4, .2, .99);
            //c.r += exp(-d*50.);
            float far_mult = 1.-smoothstep(CAM_DIST-1., CAM_DIST+.5, part.z + CAM_DIST);
            //far_mult = 10.;
            cc += c * cs * far_mult;
        }
        
        
        // Boost at projected point on sphere
        if (t != 0.) {
            float sd = sqrt(pow(clamp(length(p-part_.xyz) - .03,0.,999.),2.));
            vec3 c = (scol) * 5.0;
            col += exp(-sd*19.) * c;
        }
    }
    col += cc;
    
    vec4 old = IC1(ivec2(fragCoord));
    col.rgb = old.rgb * .95 + .05 * col.rgb;
    col.rgb += length(fuv.xy) * vec3(.1,.0,.1) * .06;
    col.rgb += length(fuv.xy+vec2(.5,.2)) * vec3(.001,.3,.52) * abs(fract(iTime*.1)-.5) * .03;
    col.rgb -= col.rgb * rand23(fuv.xy) * .02;
    fragColor = vec4(col,1.);
    
    
    #endif

}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Particle Tracking.
// Each pixel tracks the four nearest particles to it, when particles projected.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    

    vec4 ds = vec4(9999.);
    
    float tt = iTime / 33.0;
    mat3 RR = mat3(1.);
    RR[0] = vec3(cos(tt), 0., sin(tt));
    RR[2] = vec3(sin(tt), 0., -cos(tt));
    
    const float f = FOCAL_COEFF;
    vec2 fuv = vec2(fragCoord-R.xy*.5)/R.y;
    
    vec4 old = IC0(ivec2(fragCoord));
    vec4 new = old;
    ivec2 meXY = ivec2(fragCoord);
    const bool oneRow = true;
    
    for (int j=0; j<4; j++) {
        ds[j] = length(proj(RR*partAtPhase(IC1(unhashXY(int(old[j]),IR)),iTime).xyz,vec2(f),CAM_DIST) - fuv);
    }
    
    // Sample neighbours
    
    for (int j=0; j<4; j++) {
        ivec2 nnXY = (meXY + ivec2((j==0)?-1:(j==1)?1:0, (j==2)?-1:(j==3)?1:0));
        ivec2 nXY = unhashXY(int(IC0(nnXY).x),IR);
        
        if (oneRow) nXY.y = 0;
        vec3 part = RR*partAtPhase(IC1(nXY),iTime);
        if (part.z > 0.) continue; // Discard if behind
        float d = length(proj(part,vec2(f),CAM_DIST) - fuv);
        
        
        if (d <= ds[0]) {
            if (d<ds[0]) {
            ds[3] = ds[2]; new[3] = new[2];
            ds[2] = ds[1]; new[2] = new[1];
            ds[1] = ds[0]; new[1] = new[0];
            }
            ds[0] = d;
            new[0] = hashXY(nXY, IR);
        } else if (d <= ds[1]) {
            if (d<ds[1]) {
            ds[3] = ds[2]; new[3] = new[2];
            ds[2] = ds[1]; new[2] = new[1];
            }
            ds[1] = d;
            new[1] = hashXY(nXY, IR);
        } else if (d <= ds[2]) {
            if (d<ds[2]) {
            ds[3] = ds[2]; new[3] = new[2];
            }
            ds[2] = d;
            new[2] = hashXY(nXY, IR);
        } else if (d <= ds[3]) {
            ds[3] = d;
            new[3] = hashXY(nXY, IR);
        }
        /*
        for (int k=0; k<4; k++) {
            if (d < ds[k]) {
                for (int l=3; l>k; l--) ds[l] = ds[l-1], new[l] = new[l-1];
                ds[k] = d;
                new[k] = hashXY(nXY,IR);
                break;
            }
        }*/
    }
    
    // Sample random

    vec2 size = R.xy;
    size.y = 1.;
    //size.x = 12.;
    size.x = min(R.x,320.); // This controls max particles.
    for (int i=0; i<NEW_SAMPLES; i++) {
        ivec2 nXY = ivec2(rand22(iTime*.0111117+fragCoord) * size);
        
        if (oneRow) nXY.y = 0;
        vec3 part = RR*partAtPhase(IC1(nXY),iTime);
        if (part.z > 0.) continue; // Discard if behind
        float d = length(proj(part,vec2(f),CAM_DIST) - fuv);
        
        if (d <= ds[0]) {
            if (d<ds[0]) {
            ds[3] = ds[2]; new[3] = new[2];
            ds[2] = ds[1]; new[2] = new[1];
            ds[1] = ds[0]; new[1] = new[0];
            }
            ds[0] = d;
            new[0] = hashXY(nXY, IR);
        } else if (d <= ds[1]) {
            if (d<ds[1]) {
            ds[3] = ds[2]; new[3] = new[2];
            ds[2] = ds[1]; new[2] = new[1];
            ds[1] = d;
            }
            new[1] = hashXY(nXY, IR);
        } else if (d <= ds[2]) {
            if (d<ds[2]) {
            ds[3] = ds[2]; new[3] = new[2];
            }
            ds[2] = d;
            new[2] = hashXY(nXY, IR);
        } else if (d <= ds[3]) {
            ds[3] = d;
            new[3] = hashXY(nXY, IR);
        }
        /*
        for (int k=0; k<4; k++) {
            if (d < ds[k]) {
                for (int l=3; l>k; l--) ds[l] = ds[l-1], new[l] = new[l-1];
                ds[k] = d;
                new[k] = hashXY(nXY,IR);
                break;
            }
        }*/
    }
    
    fragColor = new;
}