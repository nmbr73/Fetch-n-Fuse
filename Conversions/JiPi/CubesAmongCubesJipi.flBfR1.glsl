

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float a;
};

mat2 rot(float a){
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

float mdot(vec3 a, vec3 b){
    return clamp(dot(a, b), 0., 1.);
}

float rand(vec2 u){
    u = 50.*fract(u);
    return fract(u.x*u.y*(u.x+u.y));
}

float udbox(vec3 p, vec3 a, float r){
    return length(max(abs(p) - a, 0.)) - r;
}

float sdcone(vec3 p, vec3 u, float a){
    // u : direction of the cone
    float y = dot(u, p);
    float x = length(p - y*u);
    return x*cos(a) - y*sin(a);
}

float noise(vec3 p){
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = smoothstep(0., 1., f);
    p = i + f;
    return texture(iChannel0, (p+0.5)/32.).r;
}

float fbm(vec3 p){
    float f;
    f = 0.5 * noise(p * 1.);
    f += 0.25 * noise(p * 2.);
    f += 0.125 * noise(p * 4.);
    return f;
}



const vec2 h = vec2(-1., 1.);
const vec3 path[] = vec3[](
    h.xxx, h.xyx, h.yyx, h.yxx, h.yxy, h.yyy, h.xyy, h.xxy
);
vec3 positions[7];

void updateMovingCubes(){
    // Updates the positions of the 6 moving cubes
    float t0 = 8.*iTime/6.;
    float h = 1./6.;
    int ki = 0;
    float kf = 0.;
    for(; ki < 6; ki++){
        float t = mod(t0 + kf*h, 8.);
        int i = int(floor(t));
        float f = min(6.*fract(t), 1.);
        vec3 p1 = path[(i+ki)%8];
        vec3 p2 = path[(i+ki+1)%8];
        vec3 p = mix(p1, p2, smoothstep(0., 1., f));
        positions[ki] = p;
        kf++;
    }
}

float cuboidDst(vec3 p){
    // Returns the distance to the moving cubes
    p.xz *= rot(0.8*iTime);
    p.yz *= rot(1.1*iTime+sin(iTime));
    
    float a = 0.8;
    
    //float t = mod(10.*iTime, 22.);
    float sp = 1.;// + 0.25*smoothstep(0., 1., t)*smoothstep(10., 9., t);
    
    float r = 0.2;
    float d = 10000.;
    for(int k = 0; k < 6; k++){
        vec3 pos = sp*positions[k];
        d = min(d, udbox(p - pos, vec3(a), r));
    }
    
    return d;
}


vec3 spotPos = vec3(5., 10., 5.);
vec3 spotDir = normalize(-vec3(5., 10., 5.));
float spotAngle = 0.4;

vec3 lightCol = vec3(1., 1., 1.);

vec3 cuboidPos = vec3(0., 1.8, 0.);

vec2 moveDir = 0.5 * vec2(-1., -1.);


Material defBoxesMat = Material(
    vec3(0.08, 0.0, 0.0),
    vec3(0.3, 0.2, 0.1),
    vec3(0.3, 0.3, 0.3),
    100.
);

Material cuboidMat = Material(
    vec3(0.05, 0., 0.),
    vec3(0.05,0.,0.05),
    vec3(1.),
    10.
);


vec3 cubeLocal(vec3 p, vec2 o){
    // Returns the local space of a ground block
    vec3 q;
    q.xz = p.xz - o;

    float d2c = length(o - cuboidPos.xz);
    float att = smoothstep(-2., -4., d2c) + smoothstep(2., 4., d2c) - 0.3;
    float w = 80.;
    float t = iTime;
    float f = (0.8+0.5*abs(sin(w*o.y+t)+cos(w*o.x-t)))*att;

    q.y = p.y - f - 0.3;

    return q;
}

float gridCubeDst(vec3 p, vec2 c, vec2 offset, out vec2 closestO, float curD){
    // Returns the distance to a single ground block
    vec2 o = c + offset;
    vec3 q = cubeLocal(p, o);

    float dst = udbox(q+vec3(0., 1.5, 0.), vec3(0.3, 2., 0.3), 0.1);

    float sd = step(0., dst - curD);
    closestO = sd*closestO + (1.-sd)*o;
    
    return dst;
}


float sdf(vec3 p, out Material mat){
    float d = 10000.;
    
    // Evaluate distance to the closest ground block by
    // checking its neighbors (thus simulating an infinite grid)
    // It's not very efficient but I don't know if it can be
    // improved
    vec2 c = floor(p.xz) + 0.5;
    vec2 closestO = vec2(0.);
    vec3 h = vec3(1., 0., -1.);
    for(float x = -1.; x <= 1.; x++){
        for(float y = -1.; y <= 1.; y++){
            d = min(d, gridCubeDst(p, c, vec2(x, y), closestO, d));
        }
    }
    
    // Select the color of the material based on the
    // closest ground block
    mat = defBoxesMat;
    vec3 col = sin((closestO.x*closestO.y) + vec3(0., 0.4, 0.8));
    mat.kd = defBoxesMat.kd + 0.15 * col;
    mat.kd = clamp(1.3*mat.kd, 0., 1.);
    
    // Adding noise to a block
    vec3 q = cubeLocal(p, closestO);
    d += 0.01*fbm(10.*q + 10.*rand(closestO));
    
    
    // Distance and material to the moving cubes
    q = p-cuboidPos;
    float dc = cuboidDst(q/0.3)*0.3;
    if(dc < d){
        mat = cuboidMat;
    }
    d = min(d, dc);

    return d;
}

float shadow(vec3 ro, vec3 rd, float tmin, float tmax){
    Material _;
    float t = tmin;
    float res = 1.;
    for(int i = 0; i < 50 && t < tmax; i++)
    {
        float d = sdf(ro + rd*t, _);
        if(d < 0.0001)
            return 0.;
        res = min(res, 32.*d/t);
        t += d;
    }
    return res;
}

vec2 raymarch(vec3 ro, vec3 rd, float tmin, float tmax, out Material mat){
    float maxSteps = 100.;
    float t = tmin;
    for(float i = 0.; i < maxSteps && t < tmax; i++){
        float d = sdf(ro + rd * t, mat);
        if(d < 0.001){
            return vec2(t, i/maxSteps);
        }
        t += d*0.8;
    }
    
    return vec2(-1., -1.);
}

vec3 normal(vec3 p){
    vec2 h = vec2(1., 0.) * 0.0001;
    Material _;
    return normalize(vec3(
        sdf(p+h.xyy, _) - sdf(p-h.xyy, _),
        sdf(p+h.yxy, _) - sdf(p-h.yxy, _),
        sdf(p+h.yyx, _) - sdf(p-h.yyx, _)
    ));
}

vec3 lighting(vec3 p, vec3 n, vec3 ro, Material mat){
    vec3 lightDir = -spotDir;
    
    float shad = shadow(p, lightDir, 0.0, 10.);

    vec3 v = normalize(ro - p);
    vec3 r = reflect(-lightDir, n);
    vec3 h = normalize(lightDir + v);
    
    vec3 amb = vec3(0.);
    vec3 dif = vec3(0.);
    vec3 spc = vec3(0.);
    
    vec3 q = p;
    q -= spotPos;
    float spd = sdcone(q, spotDir, spotAngle);
    
    vec3 col = vec3(0.);
    
    amb = mat.ka * lightCol;
    dif = mat.kd * lightCol * mdot(n, lightDir);
    spc = mat.ks * lightCol * pow(mdot(n, h), mat.a);

    float lig = clamp(-spd, 0., 1.5);
    col = (amb + (dif + spc) * shad) * lig;
    
    return clamp(col, 0., 1.);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 col = vec3(0.);
    
    vec2 uv = fragCoord/iResolution.xy;
    uv -= 0.5;
    uv.x *= iResolution.x/iResolution.y;
    
    vec3 ro = vec3(-1., 4., -5.);
    vec3 rd = normalize(vec3(uv, 1.));
    
    rd.yz *= rot(0.5);
    mat2 rr = rot(iTime*0.1+2. - iMouse.x*0.01);
    ro.xz *= rr;
    rd.xz *= rr;
    
    vec2 disp = moveDir * iTime - iMouse.y * 0.01;
    ro.xz += disp;
    cuboidPos.xz += disp;
    spotPos.xz += disp;
    
    updateMovingCubes();
    
    Material mat;
    vec2 hit = raymarch(ro, rd, 0., 20., mat);
    float t = hit.x;
    
    if(t > 0.){
        vec3 p = ro + rd * t;
        vec3 n = normal(p);
        p += n * 0.01;
        col = lighting(p, n, ro, mat);
    }

    fragColor = vec4(col,1.0);
}