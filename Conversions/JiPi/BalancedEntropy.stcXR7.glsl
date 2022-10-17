

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define deg (3.14159/180.)
mat2 r2d(float a) {
    return mat2(cos(a),sin(a),-sin(a),cos(a));
}

//rgb2hsv2rgb from https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
// All components are in the range [0…1], including hue.
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// All components are in the range [0…1], including hue.
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
// 2D Random
float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                 * 43758.5453123);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

//from https://www.shadertoy.com/view/ttBXRG
float staircase( in float x, in float k )
{
    float i = floor(x);
    float f = fract(x);
    
    float a = 0.5*pow(2.0*((f<0.5)?f:1.0-f), k);
    f = (f<0.5)?a:1.0-a;
    
    return i+f;
}


float bitm(vec2 uv,int c) {
    float h = 5.;
    float w = 3.;
    int p = int(pow(2.,(w)));
    float line1 = 9591.;
    uv = floor(vec2(uv.x*w,uv.y*h))/vec2(w,w);
    float c1 = 0.;
    float cc = uv.x + uv.y*w;
    c1 = mod( floor( float(c) / exp2(ceil(cc*w-0.6))) ,2.);
    c1 *= step(0.,uv.x)*step(0.,uv.y);
    c1 *= step(0.,(-uv.x+0.99))*step(0.,(-uv.y+1.6));
    return (c1);
}
#define logo 1
vec3 slogo(vec2 uv, float ar) {
    if (logo == 0) {
        return vec3(0.);
    }
    vec2 px = vec2(1./3.,1./5.);
    float ls = 4.1;
    uv.x = 0.993-uv.x;
    uv *= 8.*ls;
    ls += 2.;
    float ul = length(uv);
    uv -= px.yx*0.5*0.5*ls;
    ul = length(vec2(uv.x*0.5,uv.y)-0.5);
    uv.x *= ar*1.75;
    int s = 29671;
    int c = 29263;
    int r = 31469;
    int y = 23186;
    uv.x= 5.-uv.x;
    float b = bitm(uv,s);
    uv.x -= 1./3.*4.;
    b += bitm(uv,c);
    uv.x -= 1./3.*4.;
    b += bitm(uv,r);
    uv.x -= 1./3.*4.;
    b += bitm(uv,y);
    float rr = step(0.,uv.x+px.x*13.)*step(0.,uv.y+px.y)*step(0.,(-uv.x+px.x*4.))*step(0.,(-uv.y+px.y*6.));
    b = clamp(b,0.,1.);
    //b = rr*floor(b);
    float ptime = iTime;
    vec3 l = hsv2rgb(vec3(b+ptime,0,rr-b*1.9))*rr;
    //l -= length(uv)*0.5;
    //l -= ul*rr*0.6;
    l -= 0.1-clamp(ul*0.1,rr*1.-b,0.1);
    //l -= 3.-ul*2;
    //l = clamp(l,-1.,1.);
    return vec3(l);
}

//iq sdbox and sdvertcalcapsule functions :)
float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdVerticalCapsule( vec3 p, float h, float r )
{
  p.y -= clamp( p.y, 0.0, h );
  return length( p ) - r;
}

float abx(float a,float b) {
    return abs(abs(a)-b)-b;
}

float dty(vec3 p, vec3 s) {
    float ls = s.x;
    float lr = s.y;
    //p.zy *= r2d((3.14159/180.)*-sin(iTime+p.y*20));
    //p.xy *= r2d((3.14159/180.)*iTime);
    float d = sdBox(p+vec3(0.,0,0.),vec3(lr*1.));
    //float d = sdVerticalCapsule(p+vec3(0.,ls/2,0.),ls,lr);
    //float d = length(p)-0.001;
    //d = length(p)-0.014;
    return d;
}

vec4 fr(vec3 p) {
    //p.z = 0.;
    //float pfd = 0.95;
    //p = (fract((p*pfd))-0.5)/pfd;
    //p.xz *= r2d((3.14159/180.)*-90);
    float st = iTime*0.2;
    float sm = sin(st*10.)*0.5+0.5;
    //sm = 0.;
    //sm += sin(st*2.)*0.25;
    //sm *= 1.;
    float lp = length(p);
    //sm += sin(st*1)*0.125;
    //float lr = -0.001-sin(lp*20.+iTime)*0.0015;
    float lr = 0.13;
    float ls = 0.5;
    //p *= 2.;
    //ls += spectrum1.y*16.;
    lr += 0.001;
    float d = 10000.;
    float u = 1.;

    float lsm = 0.495;
    vec3 dp = p;

    float rt = iTime*2.;

    float pd = 0.185;
    //float pm = 1.-(sin(iTime)*0.5+0.5);
    float pm = 1.;
    //p *=  2.;
    float tm = iTime;
    tm *= 0.1;
    float ia = -1.;
    float pdm = sin(iTime*1.1+lp*dp.z*29.)*0.5+0.5;
    pdm *= 0.2;
    pdm += 0.3;
    pdm = 0.5;
    //p += sin(p*20.+iTime)/24.;
    float lss = 2.8;
    st += sin(st*2.)*0.25+sin(st*3.)*0.25;
    int sps = 7;
    vec3 rp = p;
    //rp.xy *= r2d(deg*90*floor(iTime*8.));
    //rp.xz *= r2d(deg*90*floor(iTime*20.3));
    //sps += int(rp.x*15.);
    //sps += (sin(iTime)*0.5+0.5)*10.; 
    sps = clamp(sps,1,18);
    //lr = deg*15.;
   lr = 0.2;
   float ad = (sin(iTime*0.4+lp*29.)*0.5+0.5);
    
    for (int i=0;i<sps;i++) {
    d = min(d,dty(p,vec3(ls,lr,0.)));
    //d -= sin(d*80.)*0.005;
    //p += sin(d*200.)*0.001;
    
    //d = min(d,dty(p.yxz,vec3(ls,lr,0.)));
    //d = min(d,dty(p.xzy,vec3(ls,lr,0.)));
    //p.x += 0.1;
    ia += 1.;
    //lr *= 2.9;
    lr *= .5;
    //lr += 0.000002;
    //lr = sin(lr+iTime*0.1)*0.1+0.1;
    //lr += 0.01;
    //lr *= ia+0.0003;
    pd *= u;
    
    p.yz *= r2d((3.14159/180.)*-45.);
    p.zy = abs(p.zy);
    //p.x = abs(p.x)-0.09;
    
    p.yz *= r2d((3.14159/180.)*+45.);
    
    p.xy *= r2d((3.14159/180.)*-45.);

    p.xy = abs(p.xy)-pd;

    pd *= pdm;
    //pd *= sin(iTime*0.1+dp.z*2.)*0.5+0.5;

    p.xy *= r2d((3.14159/180.)*+45.);
    
    //p = abs(p)-0.02;
    
    ls *= lsm;
    ls += 0.002;
    
    }
    //d += sin(p.z*200.)*0.001;
    //p += iTime*0.1;
    //dp.xy *= r2d((iTime*0.1));
    //dp.xz *= r2d((iTime*0.3));
    //p = fract(p*0.5)/0.5;
    d = min(d,dty(p,vec3(ls,lr,0.)));
    //p *= sin(p*10.)*0.5;
    d += (sin(p.y*90.+iTime+dp.x*5.)*0.5+0.5)*0.1;
    d += (sin(p.y*30.+iTime*0.2+dp.x*2.)*0.5+0.5)*0.1;
    //d -= (sin(dp.y*7.+iTime*0.2)*0.5+0.5)*0.05;
    d *= 0.5;
    //d = min(d,dty(p.yxz,vec3(ls,lr,0.)));
    //d = min(d,dty(p.xzy,vec3(ls,lr,0.)));

    return vec4(p,d*0.5);
}


vec4 map(vec3 p) {
    //p += 1.;
    //p.xy *= r2d(1.);
    //p.xz *= r2d(1.);
    
   
    //p.xy += 2.;
    //
    vec3 c = p;
    p.z -= 1.;
    
    //vec3 c = p;
    float cd = 12.*(sin(iTime*0.002+5.)*0.5+0.5);
    //c.z = (fract(c.z*cd))/cd;
    float rt = iTime*0.1;
    //p.xy *= r2d((3.14159/180.)*-45*iTime*9.);
    //p = abs(p);
    //p = abs(p+0.5);
    p.yz *= r2d(deg*35.264);
    p.xz *= r2d(-deg*90.);
    //p.z += 0.5;
    //p = abs(p)-0.5;
    float pfd = 0.9;
    //p = (fract((p-0.25)*pfd)-0.5)/pfd;
    //p.xz *= r2d(.75);
    //p += 0.89;
    p.xy *= r2d(sin(rt*0.5)*0.5);
    p.yz *= r2d(rt);

    
    float d = length(p)-3.;
    //float env = d;
    //float env = mix(d,sdBox(p,vec3(1.966)),0.001+sin(p.x*13.)*0.001);
    float env = mix(d,sdBox(p,vec3(1.966)),0.002);
    //p.xz *= r2d(-rt);
    //p.z += iTime;
    
    //float pz = p.z;
    vec4 frd = fr(p);
    //d = sdf(p);
    
    //d = abs(d)-0.0008;
    
    
    d = frd.w;
    d = min(d,-env);
    //float dl = (sin(iTime)*0.5+0.5)*0.1;
    //d = abs(d+0.01)-0.01;
    float dl = 0.001;
    d = abs(d+dl)-dl;
    //d = mix(d,(length(p)-0.28)*0.7,0.9);
   // d = min(d,sdBox(p,vec3(0.3)));
    c.z *= 2.;
    d = max(d, -(length(c)-0.5));
    //d = max(d, -(length(c)-01.2));
    
    return vec4(p,d);
}

vec3 calcNorm(vec3 p) {
    //float eps = 0.01*(sin(p.z*0.1)*0.5+0.5);
    float eps = 0.0008;
    vec2 h = vec2(eps,0.);
    return normalize(vec3(map(p-h.xyy).w-map(p+h.xyy).w,
                          map(p-h.yxy).w-map(p+h.yxy).w,
                          map(p-h.yyx).w-map(p+h.yyx).w));
}

float edges(vec3 p) {
    float eps = 0.001;
    //return calcNorm(p)
    vec3 n1 = calcNorm(p*(1.-eps));
    vec3 n2 = calcNorm(p*(1.));
    return clamp(abs((n1.x+n1.y+n1.z)-(n2.x+n2.y+n2.z)),0.,1.);
    //return (map(p+(eps)).w-map(p-(eps)).w)*18.;
}


#define render 0
vec2 RM(vec3 ro, vec3 rd) {
    float dO = 0.;
    float ii = 0.;
    int steps = 130;
    //steps = int(steps*(sin(iTime)*0.5+0.5));
    if (render == 1) {
        steps = 150;
    }
    float d = 130.;
    for (int i=0;i<steps;i++) {
        vec3 p = ro+rd*dO;
        //ro += calcNormL(p)*0.02;
        //ro += lens(p).xyz*0.2;
        float dS = map(p).w;
        dO += dS*(d/float(steps));
        //dO += dS*(100./float(steps))*(dO+2.)*0.15;
        ii += 0.5*d/float(steps);
        if (dO > 1000. || dS < 0.00009) {
            break;
        }
    }
    return vec2(dO,ii);
}

vec3 color(vec3 p, vec2 d) {
    return vec3(p);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 tv = fragCoord.xy / iResolution.xy;
    
    vec2 R = iResolution.xy;
    float ar = R.x/R.y;
    //uv.x *= ar;
    
    uv -= 0.5;
    uv.x *= ar;
    float c= length(uv);
    vec3 col = vec3(0.);
    vec3 ro = vec3(0.,0.,0);
    //ro.z = -2.;
    vec3 rd = normalize(vec3(uv,1.));
    
        vec3 bak = texture(iChannel0, tv).rgb;
    float rn = noise(vec2(noise(fract(rd.xy*220.+iTime*0.01)*90.)*100.));
    //if (rn > 0.2) {
    //if (1 == 1) {
    if (rn < clamp(c*.7,0.,1.0)+0.3 && render != 0) {
        col = bak;
        fragColor = vec4(col,1.0);
        return;
    }
    
    vec2 d = RM(ro,rd);
    vec3 p = ro+rd*d.x;
    vec4 mp = map(p);
    vec3 n = calcNorm(p);
    float lp = length(p);
    
    vec3 na = n;
    //vec3

    
    vec2 dd = d;
    float ga = 1.;
    float time = iTime;
    vec4 m = mp;
    vec4 mr = mp;
    vec3 b = vec3(0.);
    float e = edges(p);
    //e = clamp(e,0.,0.1)*8;
    //col += d.x*ga*0.1;
    //col += n*ga*0.1+d.y*ga*0.05;
    //col += d.x*.1;
    
    for(int i=0;i<2;i++) {
        if (d.x > 1000.) {
            //col *= 0.;
            ga = 0.;
        }
        //col += d.y*0.02*ga;
        col += hsv2rgb(vec3(d.x+iTime*0.01,d.y*0.015,d.y*0.02*ga));
        //col += (abs(e)*30.-0.3)*ga;
        //col += n+d.y*0.1-2.;
        //col += (n+d.y*0.1-2.+d.x*0.1)*ga;
        //col += abs((n*1.8+d.y*0.05-2.+d.x*0.1))*ga;
        //col += n*d.y*ga*0.01;
        //col += n*ga;
        //col += n*ga*d.x;
        //col += vec3(d.x*0.1)*ga;
        //col += d.x*ga*0.1;
        //col += n*ga*0.1+d.y*ga*0.05;
        ga *= .3;
        //ga -= 0.2;
        //ga *= 0.2;
        ro = p-n*0.002;
        rd = reflect(rd,n);
        d = RM(ro,rd);
        //b += d.x*0.02*ga;
        p = ro+rd*d.x;
        n = calcNorm(p);
        e = edges(p);
        mr = map(p);
        //col += d.x*.1;
        
        //col += n*ga*0.1+d.y*ga*0.25;
    }
    //col += d.y*ga*0.01;
    //col += e;
    col += hsv2rgb(vec3(d.x,d.y*0.015,d.y*0.02*ga));
    //col *= 0.6;
    col = rgb2hsv(col);
    col.x += 0.7;
    //col.x += iTime;
    col.y *= 1.2;
    col.z -= dd.x*0.15;
    col = hsv2rgb(col);
    vec2 ttv = tv;
    ttv -= 0.5;
    //ttv *= 0.99;
    ttv += 0.5;

    col += slogo(ttv,ar)*.6;
    vec2 ux = uv;
    fragColor = vec4(col,1.0);
}