

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
    Use the mouse to rotate around.
*/

#define MAX_RM_ITER 128.

float mdot(vec3 u, vec3 v){
    return max(0., dot(u, v));
}

mat2 rot(float a){
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

float fbmT(vec2 p){
    return texture(iChannel0, p).x;
}

float fbmF(vec2 p){
    return texture(iChannel0, p).y;
}

float trimapTerrain(vec3 p, vec3 n){
    float a = 0.29;
    float c = cos(a);
    float s = sin(a);
    mat2 r = mat2(c, -s, s, c);
    p.xz *= r; // attempt to break symetries
    
    p = 0.25*p+0.5;
    
    float fx = fbmT(p.yz);
    float fy = fbmT(p.xz);
    float fz = fbmT(p.xy);
    
    n = abs(n);
    
    return dot(n, vec3(fx, fy, fz));
}

float trimapForest(vec3 p, vec3 n){
    p = 0.25*p+0.5;
    
    float fx = fbmF(p.yz);
    float fy = fbmF(p.xz);
    float fz = fbmF(p.xy);
    
    n = abs(n);
    
    return dot(n, vec3(fx, fy, fz));
}

const float radius = 1.5;
const float atmRadius = 1.9;

vec2 sphereItsc(vec3 ro, vec3 rd, float r){
    float b = dot(ro, rd);
    float d = b*b - dot(ro, ro) + r*r;
    if(d < 0.0) {
        return vec2(-1., -1.);
    };
    return vec2(-b-sqrt(d), -b+sqrt(d));
}

vec2 seaItsc(vec3 ro, vec3 rd)
{
    return sphereItsc(ro, rd, radius);
}

vec2 atmoItsc(vec3 ro, vec3 rd){
    return sphereItsc(ro, rd, atmRadius);
}


float sdf(vec3 p){
    float sd = length(p) - radius;
    
    vec3 n = normalize(p);
    
    float dsp = trimapTerrain(p, n);
    dsp *= 0.1;
    
    return sd - dsp;
}

vec3 raymarch(vec3 ro, vec3 rd){
    const float eps = 0.0001;
    const float k = 0.65;
    float i = 0.;
    float t = 0.;
    float dmin = 100000.;
    vec3 p = ro;
    for(; i < MAX_RM_ITER; i++){
        float d = sdf(p);
        dmin = min(32.*d / t, dmin);
        if(d < eps){
            break;
        }
        p += k*d*rd;
        t += k*d;
    }
    return vec3(t, i, dmin);
}

vec3 normal(vec3 p){
    vec2 h = vec2(0.001, 0.);
    return normalize(vec3(
        sdf(p+h.xyy) - sdf(p-h.xyy),
        sdf(p+h.yxy) - sdf(p-h.yxy),
        sdf(p+h.yyx) - sdf(p-h.yyx)
    ));
}

float specRef(vec3 n, vec3 ldir, vec3 rd, float a){
    vec3 r = reflect(-ldir, n);
    return pow(mdot(r, -rd), a);
}

vec3 terrainCol(vec3 p, vec3 n){
    vec3 col = vec3(0.);
    float h = length(p);
    float r = radius;
    float t = smoothstep(r-0.1, r, h);
    
    col = mix(vec3(0.6, 0.5, 0.4), vec3(0.9, 1., 0.5), t);
    
    t = smoothstep(r, r+0.01, h);
    float f = trimapForest(p, n);
    f = smoothstep(0.1, 0.5, f);
    vec3 green = mix(vec3(0.4, 0.6, 0.1), vec3(0.,0.4,0.3), f);
    col = mix(col, green, t);
    
    t = smoothstep(r+0.02, r+0.05, h);
    col = mix(col, vec3(0.7, 0.5, 0.3), t);
    
    vec3 up = normalize(p);
    float s = 1.-mdot(n, up);
    col = mix(col, vec3(0.5), min(4.*s, 1.));
    
    t = smoothstep(r+0.06, r+0.08, h);
    col = mix(col, vec3(1.), t);
    
    return col;
}

float atmDensity(vec3 p){
    float r = (length(p) - radius)/(atmRadius - radius);
    return exp(-r) * (1. - r);
}

vec3 evalAtmosphere(vec3 p1, vec3 p2, vec3 ldir){
    float dt = 0.1;
    float ds = 0.1;
    float dst = length(p2 - p1);
    
    vec3 coefs = vec3(
        pow(400./700., 4.),
        pow(400./530., 4.),
        pow(400./440., 4.)
    );
    
    vec3 scattered = vec3(0.);
    
    for(float t = 0.; t <= 1.; t += dt){
        vec3 p = p1 + (p2 - p1) * t;
        
        float s = 0.;
        vec3 q = p;
        float opd = 0.;
        while(dot(q, q) < atmRadius * atmRadius){
            q += ds * ldir;
            opd += atmDensity(q);
        }
        
        vec3 tsm = exp(-opd * coefs);
        
        scattered += atmDensity(p) * tsm * dst * dt;
    }
    
    return scattered;
}


float shadow(vec3 p, vec3 ldir, vec3 n){
    float dmin = raymarch(p + n*0.025, ldir).z;
    return smoothstep(0., 1., dmin);
}

vec3 background(vec3 rd, vec3 ldir){
    float f = mdot(rd, ldir);
    f = smoothstep(0.999, 0.9994, f);
    return mix(vec3(0.), vec3(1., 0.99, 0.7), f);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = vec4(texture(iChannel0, uv).rrr,1.0);
    uv -= 0.5;
    uv.x *= iResolution.x / iResolution.y;
    
    vec2 mr = iMouse.xy/iResolution.xy;
    mr -= 0.5;
    mr.x *= iResolution.x / iResolution.y;
    mr *= -3.141592;
    
    vec3 col = vec3(0.);
    
    vec3 ro = vec3(0.,0.,-5.);
    vec3 rd = normalize(vec3(uv, 1.));
    ro.yz *= rot(mr.y);
    rd.yz *= rot(mr.y);
    ro.xz *= rot(mr.x);
    rd.xz *= rot(mr.x);
    
    float rxz = iTime * 0.1;
    ro.xz *= rot(rxz);
    rd.xz *= rot(rxz);
    
    vec3 ldir = normalize(vec3(1.,1.,-1.));
    //ldir.yz *= rot(mr.y);
    //ldir.xz *= rot(mr.x);
    ldir.xz *= rot(rxz);
    
    vec3 rm = raymarch(ro, rd);
    float t = rm.x;
    float i = rm.y;
    
    vec2 ts = seaItsc(ro, rd);
    
    vec2 ta = atmoItsc(ro, rd);
    
    col = background(rd, ldir);
    
    if(i < MAX_RM_ITER || ts.x > 0. || ta.x > 0.){  
        vec3 n = vec3(0.);
        vec3 m = vec3(0.);
        
        if(i < MAX_RM_ITER){
            m = ro + t * rd;
            n = normal(ro + t * rd);
            col = terrainCol(m, n);
        }
        
        float t2 = t;
        if(ts.x > 0. && (ts.x < t || i == MAX_RM_ITER)){
            m = ro + ts.x * rd;
            n = normalize(m);
            t2 = ts.x;
        }
        
        // sea coloring
        float sdepth = min(ts.y-ts.x, t-ts.x);
        if(sdepth > 0.){
            float od = 1. - exp(-sdepth*50.);
            float a = 1. - exp(-sdepth*200.);
            vec3 scol = mix(vec3(0.,0.8,0.9), vec3(0., 0.3, 0.4), od);
            scol += specRef(n, ldir, rd, 50.);
            col = mix(col, scol, a);
        }
        
        float lig = mdot(n, ldir);
        if(ts.x > 0. || i < MAX_RM_ITER){
            lig *= shadow(m, ldir, n);
            col *= lig;
        }
        
        // adding the atmosphere
        if(min(ta.y-ta.x, t-ta.x) > 0.){
            vec3 p1 = ro + ta.x * rd;
            vec3 p2 = ro + min(ta.y, t2) * rd;
            vec3 scattered = evalAtmosphere(p1, p2, ldir);
            col = mix(col, vec3(0.5, 0.8, 1.), scattered);
        }
    }
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float rand(vec2 p){
    p = 50.0*fract(p*0.3183099 + vec2(2.424, -3.145));
    return fract(p.x*p.y*(p.x+p.y));
}

float noise(vec2 x){
    vec2 p = floor(x);
    vec2 w = fract(x);
    vec2 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    
    vec2 h = vec2(1,0);
    float a = rand(p+h.yy);
    float b = rand(p+h.xy);
    float c = rand(p+h.yx);
    float d = rand(p+h.xx);
    
    return -1. + 2.*(a + (b-a)*u.x + (c-a)*u.y + (a - b - c + d)*u.x*u.y);
}

float sat(float x){
    return clamp(x, 0., 1.);
}

float fbm(vec2 p, int oct, float lac, float gain){
    p += vec2(0., 15.);
    
    mat2 r = 0.2*mat2(3., -4., 4., 3.);
    
    float f = 0.;
    float s = 1.;
    for(int i = 0; i < oct; i++){
        f += noise(p) * s;
        p = lac*r*p;
        s *= gain;
    }
    
    return f;
}

float fbm(vec2 p){
    return fbm(p, 12, 2., 0.45);
}

float fbm2(vec2 p){
    return fbm(p, 4, 2., 0.45);
}

float smin( float a, float b, float k )
{
    float res = exp2( -k*a ) + exp2( -k*b );
    return -log2( res )/k;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord - 0.5*iResolution.xy;
    uv /= iResolution.xy;
    //uv.x *= iResolution.x / iResolution.y;
    
    // generate the terrain texture
    /*float elevation = fbm(uv*10.+273.);
    elevation *= 0.7;*/
    float elevation = fbm(uv*10.+273.0, 8, 2., 0.4);
    float mask = max(elevation-0.1, 0.);
    elevation = smin(elevation, 0.2, 32.);
    elevation += fbm(uv*20.+273., 12, 2., 0.5) * 0.1;
    elevation += max(fbm(uv*16.+273., 12, 2., 0.5), 0.) * 0.5 * mask;

    
    // generate the forest areas
    float forest = fbm2(uv*8.+vec2(0.5));
    forest = sat(forest);
    
    fragColor = vec4(elevation, forest, 0., 1.0);
}