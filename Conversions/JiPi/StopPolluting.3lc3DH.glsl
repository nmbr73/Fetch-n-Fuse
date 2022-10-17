

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//////////////////////////////////////////////////
//
// As Easy As ABC! (Stop Polluting!)
//
// by Timo Kinnunen 2019
//
// Based on https://www.shadertoy.com/view/wsjXWh
//
// Use mouse to clear some stuff
// and press space bar to reinitialize
// the screen (useful after going fullscreen)!
//
///////////////////////////////////////////////////


vec4 A(vec2 U) {return texture(iChannel0,U/R);}
vec4 B(vec2 U) {return texture(iChannel1,U/R);}
vec4 K(vec2 U) {return texture(iChannel2,(U+.5)/vec2(256,3));}
vec4 F(vec2 U) {return textureLod(iChannel3,U,0.);}
//vec4 F(vec2 U) {return texture(iChannel3,U);}

float ln (vec3 p, vec3 a, vec3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}

// linear white point
const float W = 11.2;

// Filmic Reinhard, a simpler tonemapping
// operator with a single coefficient
// regulating the toe size.

// The operator ensures that f(0.5) = 0.5

// T = 0: no toe, classic Reinhard
const float T = 0.01;

float filmic_reinhard_curve (float x) {
    float q = (T + 1.0)*x*x;    
	return q / (q + x + T);
}

float inverse_filmic_reinhard_curve (float x) {
    float q = -2.0 * (T + 1.0) * (x - 1.0);
    return (x + sqrt(x*(x + 2.0*T*q))) / q;
}

vec3 filmic_reinhard(vec3 x) {
    float w = filmic_reinhard_curve(W);
    return vec3(
        filmic_reinhard_curve(x.r),
        filmic_reinhard_curve(x.g),
        filmic_reinhard_curve(x.b)) / w;
}

vec3 inverse_filmic_reinhard(vec3 x) {
    x *= filmic_reinhard_curve(W);
    return vec3(
        inverse_filmic_reinhard_curve(x.r),
        inverse_filmic_reinhard_curve(x.g),
        inverse_filmic_reinhard_curve(x.b));
}

///////////////////////////////////////////////

// ACES fitted
// from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

float impulse(float k, float x) {
    float h = k* x;
    return h* exp(1.- h);
}
float cubicPulse(float c, float w, float x) {
    x = abs(x - c);
    if(x > w) return 0.;
    x /= w;
    return 1.0 - x*x*(3.0-2.0*x);
}
vec3 hsv2rgb(float h, float s, float v) {
	return v* mix(vec3(1),clamp(abs(mod(h* 6.+ vec3(0,4,2),6.)- 3.)- 1.,0.,1.),s);
}

vec2 map(vec3 U) {
    return vec2(dist(U.xz,B(U.xz))-U.y*.0625*.0625,1);
}

const float maxHei = 10.0;
#define ZERO 0

vec2 castRay( in vec3 ro, in vec3 rd) {
    vec2 res = vec2(-1.0,-1.0);
    
    float t = .0001;
    for( int i=0; i<70; i++ ) {
        vec2 h = map( ro+rd*t );
        if( abs(h.x)<(0.00001*t) ) {
            res = vec2(t,h.y); 
            break;
        }
        t += h.x;
    }
    return res;
}

// http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
    // bounding volume
    float tp = (maxHei-ro.y)/rd.y; if( tp>0.0 ) tmax = min( tmax, tp );

    float res = 1.0;
    float t = mint;
    for( int i=ZERO; i<16; i++ )
    {
		float h = map( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( res<0.005 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 calcNormal( in vec3 pos )
{
#if 1
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize( e.xyy*map( pos + e.xyy ).x + 
					  e.yyx*map( pos + e.yyx ).x + 
					  e.yxy*map( pos + e.yxy ).x + 
					  e.xxx*map( pos + e.xxx ).x );
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+0.0005*e).x;
    }
    return normalize(n);
#endif    
}

float calcAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=ZERO; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 ) * (0.5+0.5*nor.y);
}



vec3 render(vec3 pos, vec3 rd, vec3 upv, vec3 nor, vec3 col, float occ, vec3 lig, vec3 blg, float bla, vec3 ligc) {
    vec3 ref = reflect(rd, nor);
    vec3 hal = normalize(lig-rd);
    
    float NoH = dot(nor, hal);
    float HoV = dot(hal, rd);
    float NoV = dot(nor, rd);
    float RoY = dot(ref, upv);
    float NoY = dot(nor, upv);
    float NoL = dot(nor, lig);
    float NoB = dot(nor, blg);

    // lighting
    float amb = clamp(0.5+0.5*NoY, 0.0, 1.0);
    float dif = clamp(NoL, 0.0, 1.0);
    float bac = clamp(NoB, 0.0, 1.0)*bla;
    float dom = smoothstep(-0.2, 0.2, RoY);
    float fre = pow(clamp(1.0-abs(NoV),0.0,1.0), 8.0);
    
    //dif *= calcSoftshadow(pos, lig, 0.02, 2.5);
    //dom *= calcSoftshadow(pos, ref, 0.02, 2.5);
    
    float speN = pow(clamp(NoH,0.0,1.0),4.*4.);
    float speV = pow(clamp(1.0+HoV,0.0,1.0),4.*1.0);
    //speV = 0.04+0.96*speV;
    float spe = speN*dif*speV;
    
    vec3 bacc = vec3(0.25,0.25,0.25)+vec3(0,.125,.25)*ligc;
    vec3 lin = vec3(0.0);
    lin += 0.30*dif*vec3(1.00,0.80,0.55);
    lin += 0.05*amb*vec3(0.40,0.60,1.00)*occ;
    lin += 0.05*dom*vec3(0.40,0.60,1.00)*occ;
    lin += 0.25*bac*bacc*occ;
    lin += 0.05*fre*vec3(1.00,1.00,1.00)*occ;
    col *= lin*1.;
    //col += vec3(speV,dif,speN); 
    col += 90.0*spe*ligc;
    //col += spe;
    return col;
}


void mainImage( out vec4 Q, in vec2 U) {
    vec4 a = A(U);
    vec4 b = B(U);
    vec4 df = distN(U,b);
    float s = df.x;
    vec2 sGrad = s - vec2(dist(U-vec2(1,0),b),dist(U-vec2(0,1),b));
    vec2 azGrad = a.z - vec2(A(U-vec2(1,0)).z, A(U-vec2(0,1)).z);
    //float m000 = smoothstep(-0.1,1.1,s-a.z*1.995);
    float m000 = smoothstep(-0.1,1.1,s);
    float m001 = 1.-m000;
    float m010 = smoothstep(-0.1-a.z*12.995,1.1-a.z*12.995,s);
    float m011 = 1.-m010;
    float m100 = m010-m000;
    
    float m0 = smoothstep(-0.1,1.1,0.+s-a.z*1.995);
    float m1 = 1.-m0;
    float m2 = smoothstep(.25,-.95,s);
    float m3 = 1.-m2;
    float pulse = dot(sGrad,a.xy);
    float pulse1 = impulse(2.,max(.0,pulse));
    float pulse2 = cubicPulse(-1.,.95,min(.0,pulse));
    float am0 = exp2(-1.-.125*.125*s);
    float am1 = exp2(-1.-.125*s)*(1.+1.25*pulse1-.875*pulse2);
    float am2 = 0.5-0.5*cos(1.53*s);
    float am3 = m1*m2;
    float am4 = 0.5-0.5*cos(1.131*s);

    float wave = pow(pow(abs(s),.9),.9);
    wave *= 1.+.5*pulse1;
    wave *= 1.-.75*pulse2;
    wave -= 2.*fract(2.*iTime-8.*length((iMouse.xy-U)/R));

    float hue = fract(.41+b.z*300.61)*.75-.0625;
    hue = decodeColor(b.z)*.75-.0625;
    float hueM = 1.-2.*fract(.41+b.z*33.61);
    hueM = .03*hueM + .01*sign(hueM);
    hueM = mod(hueM,.0625*.5);
    float sat = 1.-fract(.31+b.z*211.5)*.25;
    float val = 1.0;
    Q = vec4(0);
    //Q = 0.5+0.5*sin(1.6*dist(U,b)+A(U).z*vec4(1,2,3,4));
    

    vec4 col0 = vec4(1,2,3,4);
    vec4 col1 = vec4(1,1,1,1)*(0.5+0.5*sin(PI*wave));
    vec4 col2 = (0.5+0.5*sin(PI*.5*s+a.z*vec4(1,2,3,4)*3.5));
    vec4 col3 = (0.5+0.5*sin(.31+b.z*vec4(1,2,3,4)*41.5));
    vec4 col4 = (0.5+0.5*sin(.91+b.z*vec4(1,2,3,4)*41.5));
    col3 = hsv2rgb(hue+hueM,.5+.5*sat,.1+.6*val).xyzz;
    col4 = hsv2rgb(hue-hueM,.1+.9*sat,.3+.7*val).xyzz;
    //Q += .25*m000*vec4(0,1,0,0);
    //Q += .25*m010*vec4(1,0,1,0);
    
    float outline = smoothstep(0.5,2.,abs(s-.75));
    vec4 colWater = vec4(0);
    colWater += .5*am0*col0;
    vec4 test = clamp(2.+sin(33.*a.z)-vec4(1,2,0,0),0.,1.);
    float rippleWaves = .5*exp2(-1.-.125*s)*(1.+1.25*pulse1-.875*pulse2)*(0.5+0.5*sin(PI*wave))*smoothstep(-.9,.1,s);
    float rippleWavesMask = smoothstep(0.0,0.0625,a.z);
    colWater += rippleWaves* rippleWavesMask;
    colWater += 8.*length(azGrad)*sin(33.*a.z);
    colWater *= .5+.5*outline;
    vec4 colShape = vec4(0);
    colShape += .5*m1*am2*col2;
    colShape += mix(col3,col4,am4);
    colShape *= outline;
    colShape += .5*(1.-outline);
    
    vec3 ro = vec3(R*.5,max(R.x,R.y));
    vec3 pos = vec3(U,0);
    float rz = distance(pos,ro);
    vec3 rd = normalize(pos-ro);
    vec2 sun2D = rot(R, iTime*1.0025);
    vec3 sunO = vec3(iMouse.xy,.125*ro.z); 
    vec3 sunD = sunO-pos;
    float submerge = cubicPulse(2.1,.2,a.z)*(0.0+1.5*sin(54.01*a.z+iTime)*sin(54.01*a.z+iTime));
    submerge = (a.z-1.9375)*1.-2.*max(-.0625*s-.5,0.);
    submerge = clamp(submerge,0.,1.);
    vec3 res;
    {
        vec3 axis = sunO;
        vec3 nor = normalize(df.yzw);
        vec4 alb = colShape;
        vec3 lig = normalize(sunD);
        vec3 blg = normalize(vec3(-1,-1,.5)*lig);
        float bla = clamp(-10.+15.0*nor.z,0.0,1.0);
        vec3 upv = vec3(0,0,1);
        float occ = clamp(1.-dot(vec2(1),exp2(32.*(abs(2.*U/R-1.)-1.))),0.0,1.0);
        vec4 colShape2 = vec4(0);
        vec3 ligc = vec3(1);
        vec3 col = res = render(pos, rd, upv, nor, colShape.rgb, occ, lig, blg, bla, ligc);
        for(float i = 0.; i < 1.; i += 1./8.) {
            sunD = sunO- pos;
            sunD += vec3(450.*rot(normalize(sunD.xy+.00001),i*2.*PI),.0625*ro.z);
            lig = normalize(sunD);
            blg = normalize(vec3(-1,-1,.5)*lig);
            ligc = hsv2rgb(i,.875,1.);
            vec3 rend = render(pos, rd, upv, nor, colShape.rgb, occ, lig, blg, bla, ligc);
            //res *= exp(-
            colShape2 += vec4(rend,1);//max(vec3(0),res*8.-7.-col);
        }
        colShape.rgb = 
            //col + 
            colShape2.rgb / max(1.,colShape2.a-50.);
            //col + (sqrt(1.+1.*colShape2)-1.)/1.;
    }
    float h001 = submerge;
    float h000 = smoothstep(-.09,2.9,s)*(1.-h001);
    float h002 = (1.-h000)*(1.-h001);

    Q += 1.*h000*colWater;
    Q += 1.*h001*.6*colWater;
    Q += 1.*h001*.3*colShape;
    Q += 1.*h002*colShape;
    

            
    Q.rgb = pow(Q.rgb, vec3(0.833*2.));
    Q.rgb *= 1.07*.99968;
    Q.rgb = ACESFitted(Q.rgb);
    Q.rgb = clamp(linear_srgb(Q.rgb), 0.0, 1.0);

}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution.xy
#define PI acos(-1.)


uint decodeAscii(float b_z) {
    return uint(fract(b_z)*256.);
}
float decodeColor(float b_z) {
    return fract(b_z*256.);
}
float decodeSize(float b_z) {
    return floor(b_z);
}
float ln (vec2 p, vec2 a, vec2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.,1.));}


//---------------------------------------------------------------------------------

float linear_srgb(float x) {
    return mix(1.055*pow(x, 1./2.4) - 0.055, 12.92*x, step(x,0.0031308));
}
vec3 linear_srgb(vec3 x) {
    return mix(1.055*pow(x, vec3(1./2.4)) - 0.055, 12.92*x, step(x,vec3(0.0031308)));
}

float srgb_linear(float x) {
    return mix(pow((x + 0.055)/1.055,2.4), x / 12.92, step(x,0.04045));
}
vec3 srgb_linear(vec3 x) {
    return mix(pow((x + 0.055)/1.055,vec3(2.4)), x / 12.92, step(x,vec3(0.04045)));
}

//---------------------------------------------------------------------------------


// https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * 443.8975);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 pModPolar(vec2 p, float repetitions) {
	float angle = 2.*PI/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	float r = length(p);
	a = mod(a,angle) - angle/2.;
	p = vec2(cos(a), sin(a))*r;
	return p;
}
float vmax(vec2 v) {
	return max(v.x, v.y);
}
float vmin(vec2 v) {
	return min(v.x, v.y);
}
float fBox(vec2 p, vec2 b) {
	vec2 d = abs(p) - b;
	return length(max(d, vec2(0))) + vmax(min(d, vec2(0)));
}

float StarPolygon(vec2 p, float repetitions, float radius, float inner) {
    float angle = PI/repetitions;
    vec2 p2 = pModPolar(p.yx, repetitions);
    float x = abs(p2.y);
    float y = p2.x - radius;
    float offset = (PI*.5 - angle)*inner;
    float uvRotation = angle + offset;
    vec2 uv = cos(uvRotation)*vec2(x, y) + sin(uvRotation)*vec2(-y, x);
    
    float corner = radius*sin(angle)/cos(offset);
    float li = length(vec2(max(uv.x - corner, 0.0), uv.y));
    float lo = length(vec2(min(uv.x, 0.0), uv.y));
    return mix(-li, lo, step(0.0, uv.y));
}
float something(vec2 u, float z) {
    float a = 36.4*z;
    u = abs(u)-.25*z;
    u *= mat2(cos(a),-sin(a),sin(a),cos(a));
    u = abs(u)-z*vec2(.3,1);
    return max(u.x,u.y);
}

float starPolys(vec2 u, vec4 b) {
    float df = StarPolygon(u,3.+floor(4.*fract(b.z*1.601)),b.z,fract(b.z*2.601));
    return df;
}
float almostIdentityLo(float x, float m) {
    return (x >= m) ? x : (-(1. / 3.) * (1. / m) * (1. / m) * x + (1. / m)) * x * x + (1. / 3.) * m;
}
vec4 sampleCharacter(uint ch, vec2 chUV, sampler2D sampler) {
    uvec2 chPos = uvec2(ch % 16u, ch / 16u);
    vec2 cchUV = clamp(chUV, vec2(0.0078125), vec2(0.9921875));
    cchUV = 0.5+(0.5-0.0078125)*(chUV-0.5)/max(0.5-0.0078125,max(abs(chUV.x-0.5),abs(chUV.y-0.5)));
    vec2 uv = (vec2(chPos) + cchUV) / 16.;

    float l = distance(cchUV, chUV);
    l = fBox(chUV-0.5,vec2(0.5-0.0078125))-0.-0.0078125*0.;
    vec4 s = textureLod(sampler,uv,0.);
    s.gb = s.gb*2.- 1.;
    s.b = -s.b; // texture sampler is VFlipped
    s.a = s.a- .5+ s.r/ 256.;
    //s.a = s.a+ (l >= 0. ? 1.0 : max(0.,s.a*1.-0.5)) * l;
    s.a = s.a + l * max(step(0.,l),s.a);
    return s;
}
vec2 rot(vec2 v, float a) {
    return v * mat2(cos(a),-sin(a),sin(a),cos(a));
}
mat3 rotAA(vec3 u, float a) {
    float c = cos(a), s = sin(a), o = 1.-c;
    return mat3(
        u.x*u.x*o+1.0*c, u.x*u.y*o-u.z*s, u.x*u.z*o+u.y*s,
        u.y*u.x*o+u.z*s, u.y*u.y*o+1.0*c, u.y*u.z*o-u.x*s,
        u.z*u.x*o-u.y*s, u.z*u.y*o+u.x*s, u.z*u.z*o+1.0*c);
}
vec4 alphabet(vec2 u, vec4 b, sampler2D sampler) {
    uint id = decodeAscii(b.z);
    if(id == 208u || id == 80u || id == 93u || id == 143u) id = 252u;
    //else if(id == 0u) id = 80u;
    //else if(id >= 143u) id = 0u;
    float sizev = 16.+ 2.5* b.z;
    vec4 df = sampleCharacter(id,u/sizev+ .5, sampler)*sizev;
    //df.x -= max(.125-df.w,.0);
    //df.w *= sizev;
    df.yz = rot(df.yz, -b.w);
    return df.wyzx;
}
vec2 distRot(vec2 U, vec4 b) {
    return rot(U-b.xy,b.w);
}
//#define distN(U,b) vec3(something(distRot(U,b), b.z),0,0);
//#define distN(U,b) vec3(starPolys(distRot(U,b), b),0,0);
#define distN(U,b) alphabet(distRot(U,b),b,iChannel3)
#define dist(U,b) distN(U,b).x

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 A(vec2 U) {return texture(iChannel0,U/R);}
vec4 B(vec2 U) {return texture(iChannel1,U/R);}
vec4 K(vec2 U) {return texture(iChannel2,(U+.5)/vec2(256,3));}
vec4 F(vec2 U) {return textureLod(iChannel3,U,0.);}

vec4 T (vec2 U) {
	return A(U-A(U).xy);
}
void mainImage( out vec4 Q, in vec2 U ) {
    vec4 mo = A(vec2(0));
    if(U.x < 1. && U.y < 1.) {
        if (iMouse.z > 0.) {
            if (mo.z > 0.) {
                Q = vec4(iMouse.xy/R, mo.xy);
            } else {
                Q =  vec4(iMouse.xy/R, iMouse.xy/R);
            }
        } else {
            Q = vec4(0.);
        }
        return;
    }
    Q = T(U);
    vec4 b = B(U);
    vec4 q = T(b.xy);
    float p = smoothstep(2.,0.,dist(U,b));
    vec2 r = normalize(U-b.xy);
    //vec2 k = vec2(-r.y,r.x);
    float o = 1.+2.*p;
    vec4 
        n = T(U+vec2(0,o)),
        e = T(U+vec2(o,0)),
        s = T(U-vec2(0,o)),
        w = T(U-vec2(o,0)),
        m = n+e+s+w;
    Q.x -= .25*(e.z-w.z+Q.w*(n.w-s.w));
    Q.y -= .25*(n.z-s.z+Q.w*(e.w-w.w));
    Q.z  = .25*((s.y-n.y+w.x-e.x)+m.z);
    Q.w  = .25*((n.x-s.x+w.y-e.y)-Q.w);
    
    Q.xy += p*(0.25*m.xy-Q.xy);
    Q.z += .05*p;
    Q.z*=0.975;
    if (mo.z > 0. && mo.xy != mo.zw) {
        float l = ln(U, mo.xy*R, mo.zw*R);
        Q.xy += .006*(mo.xy*R - mo.zw*R)*smoothstep(40.,0.,l);
    }
    
    if (R.x-U.x < 5.) Q.x = -abs(Q.x)*.999;
    if (R.y-U.y < 5.) Q.y = -abs(Q.y)*.999;
    if (U.x < 5.) Q.x = abs(Q.x)*.999;
    if (U.y < 5.) Q.y = abs(Q.y)*.999;
    if(mod(float(iFrame), 60.) == 1. && (R.x-U.x < 1. && R.y < U.y+U.y || U.x < 1. && R.y >= U.y+U.y)) {
        Q.xy = vec2(.6*(1.-2.*smoothstep(.45,.55,U.y/R.y)),.0);
    }
    
    //if (R.x-U.x<1.) Q.xy=vec2(-.3,.0);
    
    //if (R.x-U.x < 5.||U.y < 5.||R.y-U.y<5.) Q.xy *= 0.5;
    
    if (iFrame < 1 || K(vec2(32,1)).x > 0.1) {
        Q = vec4(.3-.6*smoothstep(.45,.55,U.y/R.y),0,0,0)*0.0;
    }
    if (iFrame < 240) {
        Q.xy *= smoothstep(200.,300.,clamp(float(240 - iFrame)/150.-1.,0.,1.)*100. + distance(R*.5,U));
    }
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 A(vec2 U) {return texture(iChannel0,U/R);}
vec4 B(vec2 U) {return texture(iChannel1,U/R);}
vec4 K(vec2 U) {return texture(iChannel2,(U+.5)/vec2(256,3));}
vec4 F(vec2 U) {return textureLod(iChannel3,U,0.);}
void swap (vec2 U, inout vec4 A, vec4 B) {if (A.z == 0. || !(dist(U,B) >= dist(U,A))) A = B;}
float ascii2id(float Q_z) {return
// ((15.-floor(Q_z/16.))*16.+(15.-mod(Q_z,16.))*1. )
// ((    floor(Q_z/16.))*16.+(15.-mod(Q_z,16.))*1. )
// ((15.-floor(Q_z/16.))*1. +(15.-mod(Q_z,16.))*16.)
// ((    floor(Q_z/16.))*1. +(15.-mod(Q_z,16.))*16.)
   ((15.-floor(Q_z/16.))*16.+(    mod(Q_z,16.))*1. )
// ((    floor(Q_z/16.))*16.+(    mod(Q_z,16.))*1. )
// ((15.-floor(Q_z/16.))*1. +(    mod(Q_z,16.))*16.)
// ((    floor(Q_z/16.))*1. +(    mod(Q_z,16.))*16.)
;
}
float uv2color(vec2 rnd) {
    return mod(rnd.x*.5+rnd.y*2.+float(iFrame % 1024)+iDate.w,32.)/32.;
}
float encodeSizeAsciiColor(float size, float ascii, float color) {
    return size+(ascii2id(ascii)+color)/256.;
}
    
void mainImage( out vec4 Q, in vec2 U ) {
    Q = B(U);

    swap(U,Q,B(U+vec2(8,8)));
    swap(U,Q,B(U+vec2(8,-8)));
    swap(U,Q,B(U-vec2(8,8)));
    swap(U,Q,B(U-vec2(8,-8)));

    swap(U,Q,B(U+vec2(4,0)));
    swap(U,Q,B(U+vec2(0,-4)));
    swap(U,Q,B(U-vec2(4,0)));
    swap(U,Q,B(U-vec2(0,-4)));

    swap(U,Q,B(U+vec2(2,2)));
    swap(U,Q,B(U+vec2(2,-2)));
    swap(U,Q,B(U-vec2(2,2)));
    swap(U,Q,B(U-vec2(2,-2)));

    swap(U,Q,B(U+vec2(1,0)));
    swap(U,Q,B(U+vec2(0,-1)));
    swap(U,Q,B(U-vec2(1,0)));
    swap(U,Q,B(U-vec2(0,-1)));

    Q.xyw += A(Q.xy).xyw;
    /*
    if((R.x-U.x < 1. && R.y < U.y+U.y || U.x < 1. && R.y >= U.y+U.y)) {
        float key = -1.;
        for(float i = 0.; i < 256.; i++) {
            if(K(vec2(i,0)).x > 0.1) key = i;
        }
        if(key >= 0. || mod(float(iFrame), 60.) == 1.) {
            float y = floor(U.y/32.)*32.+16.;
            float prog = 1.+floor(R.y/(40.+iTime*2.));
            if(mod(floor(U.y/32.),prog) == mod(-floor(float(iFrame)/60.),prog)) {
                Q.x = U.x < 1. ? -1. : R.x;
                Q.y = y;
                Q.z = -2.5*log(1e-5+(0.5+0.5*sin(y+(y+.45)*mod(float(iFrame),1e3)))*(.0625+.9375*smoothstep(120.,10.,min(100.,iTime))));
                Q.w = 0.;
                if(key>=0.) {
                    Q.z = encodeSizeAsciiColor(floor(Q.z),key,fract(fract(float(iFrame)/256.)+.0*Q.z*256.));
                }
            }
        }
    }
    */
    if(mod(float(iFrame), 60.) == 1. && (R.x-U.x < 1. && R.y < U.y+U.y || U.x < 1. && R.y >= U.y+U.y)) {
        //float y = round((U.y+5.)/20.)*20.-5.;
        float y = floor(U.y/32.)*32.+16.;
        float prog = 1.+floor(R.y/(40.+iTime*2.));
        if(mod(floor(U.y/32.),prog) == mod(-floor(float(iFrame)/60.),prog)) {
            float key = -1.;
            float rnd = hash13(vec3(U.x-501.61,y+101.61,float(iFrame)/60.));
            float rnd2 = fract(PI*(PI+rnd));
            float beg = 0.;
            float expo = 10.-mod(floor(float(iFrame)/240.),10.);
            float expoKey = -1.;

            vec2 seg = vec2(0);
            seg = vec2(  0,  3);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(49,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2(  3,  2);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(50,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2(  5,  4);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(51,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2(  9,  7);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(52,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2( 16, 12);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(53,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2( 28,  1);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(54,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2( 29,  1);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(55,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2( 30,  2);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(56,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2( 32, 96);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(57,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = vec2(128,128);expo--;if(expo > 0.0)expoKey = seg.x+rnd2*seg.y;if(K(vec2(48,2)).x < .1 && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            key = floor(key);
            float anyKey = -1.;
            for(float i = 0.; i < 256.; i++) {
                if(K(vec2(i,0)).x > 0.1) anyKey = i;
            }
            if(key < 0.) {
                key = anyKey;
            }
            if(anyKey < 0.0) {
	            key = floor(expoKey);
            }
            Q.x = U.x < 1. ? -1. : R.x;
            Q.y = y;
            Q.z = -2.5*log(1e-5+(0.5+0.5*sin(y+(y+.45)*mod(float(iFrame),1e3)))*(.0625+.9375*smoothstep(100.,10.,min(100.,iTime))));
                           Q.w = 0.;
            if(key>=0.) {
                Q.z = encodeSizeAsciiColor(floor(Q.z),key,fract(fract(float(iFrame)/256.)+.0*Q.z*256.));
            }
        }
    }
/*
    if (R.x-U.x < 1. && mod(float(iFrame) , 60.) == 1.) {
        float y = round((U.y+5.)/20.)*20.-5.;
        Q = vec4(
            R.x,y,
        0.5+0.5*sin(y+(y+.45)*mod(float(iFrame),1e3)),0.
       );
       Q.z = -1.5*log(1e-4+Q.z);
    }
    
*/    
    
    if(iFrame < 1 || K(vec2(32,1)).x > 0.1) {
        vec4 q = vec4(floor((U+2.-R*.5)/4./5.),0,0);
        if(q.y == 5.) {
            q.z = (q.x == -8.) ? 80.  :// P
            /* */ (q.x == -6.) ? 111. :// o
            /* */ (q.x == -4.) ? 108. :// l
            /* */ (q.x == -2.) ? 108. :// l
            /* */ (q.x ==  0.) ? 117. :// u
            /* */ (q.x ==  2.) ? 116. :// t
            /* */ (q.x ==  4.) ? 105. :// i
            /* */ (q.x ==  6.) ? 110. :// n
            /* */ (q.x ==  8.) ? 103. :// g
            0.0;
        } else if(q.y == -1.) {
            q.z = (q.x == -9.) ? 73.  :// I
            /* */ (q.x == -7.) ? 115. :// s
            /* */ (q.x == -3.) ? 101. :// e
            /* */ (q.x == -1.) ? 97.  :// a
            /* */ (q.x ==  1.) ? 115. :// s
            /* */ (q.x ==  3.) ? 121. :// y
            /* */ (q.x ==  7.) ? 97.  :// a
            /* */ (q.x ==  9.) ? 115. :// s
            0.0;
        } else if(q.y == -5.) {
            q.z = (q.x == -11.) ? 65.  :// A
            /* */ (q.x ==  -9.) ? 45.  :// -
            /* */ (q.x ==  -7.) ? 66.  :// B
            /* */ (q.x ==  -5.) ? 45.  :// -
            /* */ (q.x ==  -3.) ? 116. :// t
            /* */ (q.x ==  -2.) ? 111. :// o
            /* */ (q.x ==  -1.) ? 115. :// s
            /* */ (q.x ==   0.) ? 115. :// s
            /* */ (q.x ==   2.) ? 45.  :// -
            /* */ (q.x ==   4.) ? 105. :// i
            /* */ (q.x ==   5.) ? 110. :// n
            /* */ (q.x ==   7.) ? 45.  :// -
            /* */ (q.x ==   9.) ? 67.  :// C
            /* */ (q.x ==  11.) ? 33.  :// !
            0.0;
        } else { q.z = 0.; }
        if(q.z > 0.) {
            Q = vec4(q.xy*5.*4.-2.+R*.5,encodeSizeAsciiColor(30.,q.z,uv2color(q.xy)),0);
        } else {
            Q = vec4(-R,0,0);
        }
    } else if(iMouse.z>0.) {
        vec2 pa = iMouse.xy;
        vec2 pb = A(vec2(0)).zw*R;
        if(ln(U, pa, pb) < 40. + distance(pa,pb)*0.5) {
        	Q = vec4(-R,0,0);
        }
    }
}
